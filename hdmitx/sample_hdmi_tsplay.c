/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tsplay.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   : derive from sample_tsplay.c
  History       :
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "securec.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_video.h"
#include "hi_unf_demux.h"
#include "hi_unf_system.h"
#include "hi_unf_win.h"
#include "hi_adp_mpi.h"
#include "sample_hdmi_common.h"
#include "hdmi_test_cmd.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"

#define TS_PLAY_MAX_NUM   2


#define PLAY_DMX_ID  0
#define PLAY_DMX_ID1  1
#define MAX_WINDOW_SIZE 16
#define TS_READ_SIZE    (188 * 1000)
#define WAIT_EOS_TIMEOUT_COUNT 500
#define WAIT_EOS_SLEEP_TIME (100 * 1000)
#define TIMEOUT_MS  100

typedef struct {
    hi_handle win;
} tsplay_vid_ctx;

typedef struct {
    hi_handle track;
} tsplay_aud_ctx;

typedef struct {
    hi_u32 id;
    hi_handle avplay;
    tsplay_vid_ctx vid;
    tsplay_aud_ctx aud;
} tsplay_ctx;

static tsplay_ctx g_context[TS_PLAY_MAX_NUM] = {0};
static FILE            *g_ts_file[TS_PLAY_MAX_NUM] = {HI_NULL};
static pthread_t        g_ts_thread[TS_PLAY_MAX_NUM];
static pthread_mutex_t  g_ts_mtx[TS_PLAY_MAX_NUM];
static hi_bool          g_is_stop_ts_thread[TS_PLAY_MAX_NUM] = {HI_FALSE};
static hi_bool          g_is_search_pmt[TS_PLAY_MAX_NUM] = {HI_FALSE};
static hi_handle        g_ts_buf[TS_PLAY_MAX_NUM] = {HI_INVALID_HANDLE};
static pmt_compact_tbl *g_prog_tbl[TS_PLAY_MAX_NUM] = {HI_NULL};
static hi_bool          g_is_eos[TS_PLAY_MAX_NUM] = {HI_FALSE};
static hi_bool          g_single_play = HI_FALSE;

static DIR             *g_media_dir = HI_NULL;
static hi_u32           g_media_cnt = 0;
static hi_bool          g_loop_play = HI_TRUE;

static hi_bool is_ts_file(const char *file)
{
    const hi_char *ts_suffix = ".ts";
    hi_s32 ts_suffix_len = strlen(ts_suffix);
    hi_s32 len = 0;

    if (file == HI_NULL || (len = strlen(file)) <= ts_suffix_len) {
        printf("[WARN] [%s] is not a ts file, len : %d\n", file ? file : "null", len);
        return HI_FALSE;
    }

    if (strcasecmp(file + len - ts_suffix_len, ts_suffix) == 0) {
        return HI_TRUE;
    }

    printf("[INFO] [%s] is not a ts file, len : %d\n", file ? file : "null", len);
    return HI_FALSE;
}

static const hi_char *get_ts_name(const hi_char *file)
{
    struct stat stat_buf;
    if (stat(file, &stat_buf) < 0) {
        printf("[ERROR] stat %s failed: %s\n", file, strerror(errno));
        return HI_NULL;
    }

    if (S_ISDIR(stat_buf.st_mode)) {
        return HI_NULL;
    } else {
        return is_ts_file(file) ? file : HI_NULL;
    }
}

static const hi_char *get_media_name(const hi_char *path)
{
    struct dirent *dir_item = HI_NULL;
    const hi_char *media_name = HI_NULL;

    if (g_media_dir == HI_NULL) {
        if (path == HI_NULL) {
            return HI_NULL;
        }

        media_name = get_ts_name(path);
        if (media_name != HI_NULL) {
            g_single_play = HI_TRUE;
            return media_name;
        } else {
            g_media_dir = opendir(path);
            if (g_media_dir == HI_NULL) {
                printf("[ERROR] opendir %s failed: %s\n", path, strerror(errno));
                return HI_NULL;
            }

            chdir(path);
        }
    }

    while (HI_TRUE) {
        dir_item = readdir(g_media_dir);
        if (dir_item == HI_NULL) {
            if (g_media_cnt > 0 && g_loop_play) {
                printf("[INFO] rewind dir.\n");
                g_media_cnt = 0;
                rewinddir(g_media_dir);
                continue;
            }

            break;
        }

        media_name = get_ts_name(dir_item->d_name);
        if (media_name != HI_NULL) {
            g_media_cnt++;
            break;
        }
    }

    return media_name;
}

static hi_bool process_rewind(hi_u32 id, hi_handle avplay)
{
    hi_u32 count = 0;

    if (g_single_play && g_loop_play) {
        rewind(g_ts_file[id]);
        return HI_TRUE;
    } else if (g_is_search_pmt[id]) {
        return HI_TRUE;
    }

    hi_unf_avplay_flush_stream(avplay, HI_NULL);
    printf("read ts file end and wait eos!\n");

    while (!g_is_eos[id] && !g_is_stop_ts_thread[id]) {
        usleep(100 * 1000);

        count++;
        if (count > 300) {
            printf("[WARN] wait eos timeout !\n");
            break;
        }
    }

    return HI_FALSE;
}

static void *ts_thread(void *args)
{
    hi_u32        len;
    hi_s32        ret;
    tsplay_ctx   *ctx = args;
    hi_bool       rewind = HI_FALSE;
    hi_unf_stream_buf stream_buf;

    while (!g_is_stop_ts_thread[ctx->id]) {
        pthread_mutex_lock(&g_ts_mtx[ctx->id]);

        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf[ctx->id], TS_READ_SIZE, &stream_buf, TIMEOUT_MS);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mtx[ctx->id]);
            usleep(1000);
            continue;
        }

        len = fread(stream_buf.data, sizeof(hi_s8), TS_READ_SIZE, g_ts_file[ctx->id]);
        if (len <= 0) {
            rewind = process_rewind(ctx->id, ctx->avplay);
            pthread_mutex_unlock(&g_ts_mtx[ctx->id]);

            if (rewind) {
                usleep(1000);
                continue;
            } else {
                break;
            }
        }

        ret = hi_unf_dmx_put_ts_buffer(g_ts_buf[ctx->id], len, 0);
        if (ret != HI_SUCCESS) {
            printf("[%s %u] hi_unf_dmx_put_ts_buffer(%d) failed 0x%x\n", __FUNCTION__, __LINE__, ctx->id, ret);
        }
        pthread_mutex_unlock(&g_ts_mtx[ctx->id]);
        usleep(1000);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf[ctx->id]);
    if (ret != HI_SUCCESS) {
        printf("[%s %u] hi_unf_dmx_reset_ts_buffer(%d) failed 0x%x\n", __FUNCTION__, __LINE__, ctx->id, ret);
    }

    printf("stopped put ts stream, ts_thread end\n");
    g_is_stop_ts_thread[ctx->id] = HI_TRUE;
    g_is_eos[ctx->id] = HI_FALSE;

    return HI_NULL;
}

static hi_s32 proc_evt_func(hi_handle avplay, hi_unf_avplay_event_type evt, hi_void *para, hi_u32 param_len)
{
    hi_u32 i;
    hi_u32 id = 0;

    for (i = 0; i < TS_PLAY_MAX_NUM; i++) {
        if (g_context[i].avplay == avplay) {
            id = i;
            break;
        }
    }

    switch (evt) {
        case HI_UNF_AVPLAY_EVENT_EOS:
            printf("[INFO] End of stream!\n");
            g_is_eos[id] = HI_TRUE;
            break;
        default:
            break;
    }

    return HI_SUCCESS;
}

static void process_input_cmd()
{
    hi_s32 ret;
    hi_char input_cmd[HDMI_INPUT_CMD_MAX] = {0};

    hi_bool prompt = HI_TRUE;
    fd_set rfds;
    struct timeval tv;

    printf("input  q  to quit\n" \
           "input  h  to get help\n");
    while (HI_TRUE) {
        memset(input_cmd, 0, sizeof(input_cmd));
        if (prompt) {
            prompt = HI_FALSE;
            printf("hdmi_cmd >");
            fflush(stdout);
        }

        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 500000; /* Check whether the tread need to be stopped every 500ms */
        ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
        if (ret <= 0) {
            continue;
        }

        ret = read(STDIN_FILENO, input_cmd, sizeof(input_cmd));
        if (ret <= 0) {
            continue;
        }

        prompt = HI_TRUE;

        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }

        hdmi_test_cmd(input_cmd, ret);

        continue;
    }
}

static hi_s32 open_ts_play(hi_u32 id)
{
    hi_s32 ret;
    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    hi_unf_dmx_port dmx_port;

    ret = sample_snd_open(id);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call sample_snd_open(%d) failed.\n", id);
        return HI_FAILURE;
    }

    ret = sample_disp_open(id);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call sample_disp_open(%d) failed.\n", id);
        goto snd_close;
    }

    tsbuf_attr.buffer_size = 0x1000000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    dmx_port = id ? HI_UNF_DMX_PORT_RAM_1 : HI_UNF_DMX_PORT_RAM_0;
    ret = hi_unf_dmx_create_ts_buffer(dmx_port, &tsbuf_attr, &g_ts_buf[id]);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_create_ts_buffer(%d) failed.\n", id);
        goto disp_close;
    }

    ret = hi_unf_dmx_attach_ts_port(id, dmx_port);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_attach_ts_port(%d) failed.\n", id);
        goto destory_ts_buf;
    }

    return ret;
destory_ts_buf:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf[id]);
disp_close:
    sample_disp_close(id);
snd_close:
    sample_snd_close(id);

    return ret;
}

static hi_void close_ts_play(hi_u32 id)
{
    hi_unf_dmx_detach_ts_port(id);
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf[id]);
    sample_disp_close(id);
    sample_snd_close(id);

    return;
}

static hi_s32 init_ts_play(hi_void)
{
    if (hi_unf_sys_init() != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_sys_init failed.\n");
        goto out;
    }

    if (sample_snd_init() != HI_SUCCESS) {
        HDMI_LOG_ERR("call sample_snd_init failed.\n");
        goto sys_deinit;
    }

    if (sample_disp_init() != HI_SUCCESS) {
        HDMI_LOG_ERR("call sample_disp_hdmi_init failed.\n");
        goto snd_deinit;
    }

    if (hi_adp_win_init() != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_win_init failed.\n");
        goto disp_deinit;
    }

    if (hi_unf_dmx_init() != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_init failed.\n");
        goto win_deinit;
    }

    if (hi_adp_avplay_init() != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_avplay_init failed.\n");
        goto dmx_deinit;
    }
    return HI_SUCCESS;
dmx_deinit:
    (void)hi_unf_dmx_deinit();
win_deinit:
    (void)hi_adp_win_deinit();
disp_deinit:
    (void)sample_disp_deinit();
snd_deinit:
    (void)sample_snd_deinit();
sys_deinit:
    (void)hi_unf_sys_deinit();
out:
    return HI_FAILURE;
}

static hi_void deinit_ts_play()
{
    hi_unf_avplay_deinit();
    hi_unf_dmx_deinit();
    hi_adp_win_deinit();
    sample_disp_deinit();
    sample_snd_deinit();
    hi_unf_sys_deinit();

    if (g_media_dir != HI_NULL) {
        closedir(g_media_dir);
        g_media_dir = HI_NULL;
    }

    return;
}

hi_s32 vo_creatwin(hi_u32 displayid, hi_unf_video_rect *rect, hi_handle *win)
{
    hi_s32 ret;
    hi_unf_win_attr   win_attr;
    memset_s(&win_attr, sizeof(win_attr), 0, sizeof(hi_unf_win_attr));
    win_attr.disp_id = displayid;
    win_attr.priority = HI_UNF_WIN_WIN_PRIORITY_AUTO;
    win_attr.is_virtual = HI_FALSE;
    win_attr.asp_convert_mode = HI_UNF_WIN_ASPECT_CONVERT_FULL;
    win_attr.video_rect.x = 0;
    win_attr.video_rect.y = 0;
    win_attr.video_rect.width  = 0;
    win_attr.video_rect.height = 0;

    if (rect == HI_NULL) {
        memset_s(&win_attr.output_rect, sizeof(win_attr.output_rect), 0x0, sizeof(hi_unf_video_rect));
    } else {
        memcpy_s(&win_attr.output_rect, sizeof(win_attr.output_rect), rect, sizeof(hi_unf_video_rect));
    }

    ret = hi_unf_win_create(&win_attr, win);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_win_create failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 ts_vid_open(hi_u32 displayid, hi_handle avplay, tsplay_vid_ctx *vid)
{
    hi_s32 ret = HI_SUCCESS;

    ret = vo_creatwin(displayid, HI_NULL, &vid->win);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call vo_creatwin(%d) failed.\n", displayid);
        return HI_FAILURE;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_chan_open failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_win_attach_src(vid->win, avplay);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_win_attach_src failed:%#x.\n", ret);
        return HI_FAILURE;
    }

    ret = hi_unf_win_set_enable(vid->win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_win_set_enable failed.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 ts_vid_close(hi_handle avplay, tsplay_vid_ctx *vid)
{
    if (vid->win != HI_INVALID_HANDLE) {
        hi_unf_win_set_enable(vid->win, HI_FALSE);
        hi_unf_win_detach_src(vid->win, avplay);
        hi_unf_win_destroy(vid->win);

        vid->win = HI_INVALID_HANDLE;
    }

    return hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
}

static hi_s32 ts_aud_open(hi_u32 sndid, hi_handle avplay, tsplay_aud_ctx *aud)
{
    hi_s32 ret = HI_SUCCESS;
    hi_unf_audio_track_attr track_attr;

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_get_default_track_attr failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_chan_open failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_snd_create_track(sndid, &track_attr, &aud->track);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_create_track(%d) failed.\n", sndid);
        return HI_FAILURE;
    }

    ret = hi_unf_snd_attach(aud->track, avplay);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_attach failed.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 ts_aud_close(hi_handle avplay, tsplay_aud_ctx *aud)
{
    if (aud->track != HI_INVALID_HANDLE) {
        hi_unf_snd_detach(aud->track, avplay);
        hi_unf_snd_destroy_track(aud->track);

        aud->track = HI_INVALID_HANDLE;
    }

    return hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
}

static hi_s32 avplay_create(hi_u32 id, tsplay_ctx *context)
{
    hi_s32 ret;
    hi_unf_avplay_attr avplay_attr;

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = id;

    ret |= hi_unf_avplay_create(&avplay_attr, &context->avplay);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_create(%d) failed.\n", id);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 avplay_register_event(hi_u32 id, tsplay_ctx *context)
{
    hi_s32 ret;

    ret = hi_unf_avplay_register_event(context->avplay, HI_UNF_AVPLAY_EVENT_EOS, proc_evt_func);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_register_event(%d) failed.\n", id);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 avplay_open(hi_u32 id, tsplay_ctx *context)
{
    hi_s32 ret;

    hi_adp_search_init();
    printf("[INFO] begin to play file(%d)\n", id);
    ret = hi_adp_search_get_all_pmt(id, &g_prog_tbl[id]);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_search_get_all_pmt(%d) failed.\n", id);
        return HI_FAILURE;
    }

#ifdef TSPLAY_SUPPORT_VID_CHAN
    ret = ts_vid_open(id, context->avplay, &context->vid);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call ts_vid_open(%d) failed.\n", id);
        return HI_FAILURE;
    }
#endif

#ifdef TSPLAY_SUPPORT_AUD_CHAN
    ret = ts_aud_open(id, context->avplay, &context->aud);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call ts_aud_open(%d) failed.\n", id);
        return HI_FAILURE;
    }
#endif

    return HI_SUCCESS;
}

static hi_void avplay_sync_set(hi_u32 id, tsplay_ctx *context)
{
    hi_s32 ret;
    hi_unf_sync_attr sync_attr;

    (void)hi_unf_avplay_get_attr(context->avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);

    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret = hi_unf_avplay_set_attr(context->avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        printf("[WARN] call hi_unf_avplay_set_attr(%d) failed.\n", id);
    }

    return;
}

static hi_s32 avplay_play_prog(hi_u32 id, tsplay_ctx *context)
{
    hi_s32 ret;

    pthread_mutex_lock(&g_ts_mtx[id]);
    rewind(g_ts_file[id]);
    hi_unf_dmx_reset_ts_buffer(g_ts_buf[id]);
    g_is_search_pmt[id] = HI_FALSE;

    ret = hi_adp_avplay_play_prog(context->avplay, g_prog_tbl[id], 0,
        HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call HIADP_AVPlay_PlayProg(%d) failed.\n", id);
        pthread_mutex_unlock(&g_ts_mtx[id]);
        return HI_FAILURE;
    }

    pthread_mutex_unlock(&g_ts_mtx[id]);
    return HI_SUCCESS;
}

static hi_void ts_thread_create(hi_u32 id, tsplay_ctx *context)
{
    pthread_mutex_init(&g_ts_mtx[id], HI_NULL);
    g_is_stop_ts_thread[id] = HI_FALSE;
    g_is_search_pmt[id] = HI_TRUE;
    pthread_create(&g_ts_thread[id], HI_NULL, ts_thread, context);

    return;
}

static hi_void ts_play_free(hi_u32 id, tsplay_ctx *context)
{
    hi_adp_search_free_all_pmt(g_prog_tbl[id]);
    g_prog_tbl[id] = HI_NULL;
    hi_adp_search_de_init();

    g_is_stop_ts_thread[id] = HI_TRUE;
    pthread_join(g_ts_thread[id], HI_NULL);

    if (context->aud.track != HI_INVALID_HANDLE) {
        (void)ts_aud_close(context->avplay, &context->aud);
    }

    if (context->vid.win != HI_INVALID_HANDLE) {
        (void)ts_vid_close(context->avplay, &context->vid);
    }

    return;
}

static hi_s32 ts_file_open(hi_u32 id, const hi_char *file)
{
    g_ts_file[id] = fopen(file, "rb");
    if (g_ts_file[id] == HI_NULL) {
        printf("[ERROR] open file(%d) %s error: %s\n", id, file, strerror(errno));
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_void ts_file_close(hi_u32 id)
{
    if (g_ts_file[id] != HI_NULL) {
        fclose(g_ts_file[id]);
        g_ts_file[id] = HI_NULL;
    }

    return;
}

static hi_void ts_stop(hi_u32 id)
{
    hi_unf_avplay_stop_opt stop_opt = {0};

    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    (void)hi_unf_avplay_stop(g_context[id].avplay,
        HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    ts_play_free(id, &g_context[id]);
    (void)hi_unf_avplay_destroy(g_context[id].avplay);
    ts_file_close(id);

    return;
}

static hi_s32 ts_play(hi_u32 id, const hi_char *file)
{
    hi_s32 ret = HI_SUCCESS;
    hi_unf_avplay_stop_opt stop_opt = {0};

    g_context[id].id = id;
    g_context[id].avplay = HI_INVALID_HANDLE;
    g_context[id].vid.win = HI_INVALID_HANDLE;
    g_context[id].aud.track = HI_INVALID_HANDLE;

    ret = ts_file_open(id, file);
    if (ret != HI_SUCCESS) {
        goto close_file;
    }

    ret = avplay_create(id, &g_context[id]);
    if (ret != HI_SUCCESS) {
        goto close_file;
    }

    ret = avplay_register_event(id, &g_context[id]);
    if (ret != HI_SUCCESS) {
        goto destroy_av;
    }

    ts_thread_create(id, &g_context[id]);

    ret = avplay_open(id, &g_context[id]);
    if (ret != HI_SUCCESS) {
        goto ts_free;
    }

    avplay_sync_set(id, &g_context[id]);

    ret = avplay_play_prog(id, &g_context[id]);
    if (ret != HI_SUCCESS) {
        goto out;
    }

    return HI_SUCCESS;
out:
    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    (void)hi_unf_avplay_stop(g_context[id].avplay,
        HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
ts_free:
    ts_play_free(id, &g_context[id]);
destroy_av:
    (void)hi_unf_avplay_destroy(g_context[id].avplay);
close_file:
    ts_file_close(id);

    return ret;
}

hi_s32 change_media_file(hi_u32 id, const hi_char *file)
{
    hi_char *tmp;

    tmp = (hi_char *)get_ts_name(file);
    if (tmp == HI_NULL) {
        printf("\33[31minvalid ts_file:%s\33[0m\n", file);
        return HI_FAILURE;
    }

    if (g_context[id].avplay == HI_NULL) {
        printf("\33[31mavplay destroy\33[0m\n");
        return HI_FAILURE;
    }

    hi_unf_avplay_flush_stream(g_context[id].avplay, HI_NULL);

    ts_stop(id);
    if (ts_play(id, tmp) != HI_SUCCESS) {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 ts_play_start(const hi_char **file)
{
    hi_u32 i;
    hi_u32 j;

    for (i = 0; i < TS_PLAY_MAX_NUM; i++) {
        if (file[i] == HI_NULL) {
            continue;
        }
        if (open_ts_play(i) != HI_SUCCESS) {
            goto close;
        }
    }

    for (i = 0; i < TS_PLAY_MAX_NUM; i++) {
        if (file[i] == HI_NULL) {
            continue;
        }
        if (ts_play(i, file[i]) != HI_SUCCESS) {
            goto stop;
        }
    }
    return HI_SUCCESS;
stop:
    for (j = 0; j < TS_PLAY_MAX_NUM; j++) {
        if (j < i) {
            if (file[j] != HI_NULL) {
                ts_stop(j);
            }
        }
    }
close:
    for (j = 0; j < TS_PLAY_MAX_NUM; j++) {
        if (j < i) {
            if (file[j] != HI_NULL) {
                close_ts_play(j);
            }
        }
    }

    return HI_FAILURE;
}

static hi_void ts_play_stop(const hi_char **file)
{
    hi_u32 i;

    for (i = 0; i < TS_PLAY_MAX_NUM; i++) {
        if (file[i] != HI_NULL) {
            ts_stop(i);
        }
    }

    for (i = 0; i < TS_PLAY_MAX_NUM; i++) {
        if (file[i] != HI_NULL) {
            close_ts_play(i);
        }
    }
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_u32 count = 0;
    hi_u32 i;
    const hi_char *file[TS_PLAY_MAX_NUM] = {HI_NULL};

    if (argc < 3) { /* argc number must be >= 3 */
        printf("Usage: %s file/dir file1/dir1\n", argv[0]);
        printf("Example:\n\t%s ./test.ts ./test1.ts\n", argv[0]);
        return HI_FAILURE;
    }

    for (i = 0; i < TS_PLAY_MAX_NUM; i++) {
        if (strcasecmp("null", argv[i + 1]) == 0) {
            file[i] = get_media_name(HI_NULL);
        } else {
            file[i] = get_media_name(argv[i + 1]);
        }
        if (file[i] == HI_NULL) {
            count++;
        }
    }

    if (count >= TS_PLAY_MAX_NUM) {
        return HI_FAILURE;
    }

    if (init_ts_play() != HI_SUCCESS) {
        printf("init_ts_play fail.\n");
        return HI_FAILURE;
    }

    if (ts_play_start(file) != HI_SUCCESS) {
        goto deinit;
    }

    process_input_cmd();
    ts_play_stop(file);
deinit:
    (void)deinit_ts_play();

    return HI_FAILURE;
}

