/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tsplay.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   :
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

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"

#define PLAY_DMX_ID  0
#define MAX_WINDOW_SIZE 16
#define TS_READ_SIZE    (188 * 1000)
#define WAIT_EOS_TIMEOUT_COUNT 500
#define WAIT_EOS_SLEEP_TIME (100 * 1000)

typedef struct {
    hi_handle win;
} tsplay_vid_ctx;

typedef struct {
    hi_handle track;
} tsplay_aud_ctx;

typedef struct {
    hi_handle avplay;
    tsplay_vid_ctx vid;
    tsplay_aud_ctx aud;
} tsplay_ctx;

static FILE            *g_ts_file = HI_NULL;
static pthread_t        g_ts_thread;
static pthread_mutex_t  g_ts_mtx;
static hi_bool          g_is_stop_ts_thread = HI_FALSE;
static hi_bool          g_is_search_pmt = HI_FALSE;
static hi_handle        g_ts_buf = HI_INVALID_HANDLE;
static pmt_compact_tbl *g_prog_tbl = HI_NULL;
static hi_bool          g_is_eos = HI_FALSE;
static hi_bool          g_quit_play = HI_FALSE;
static hi_bool          g_single_play = HI_FALSE;
static hi_bool          g_sync_off = HI_FALSE;

static DIR *g_media_dir = HI_NULL;
static hi_u32 g_media_cnt = 0;
static hi_bool g_loop_play = HI_TRUE;

static const char *g_suffix[] = {".ts", ".trp", NULL};

static hi_bool is_ts_file(const char *file)
{
    if (file == HI_NULL) {
        printf("[ERROR] file is NULL\n");
        return HI_FALSE;
    }

    for (int i = 0; g_suffix[i]; i++) {
        hi_s32 offset = strlen(file) - strlen(g_suffix[i]);
        if (offset > 0 && strcasecmp(file + offset, g_suffix[i]) == 0) {
            return HI_TRUE;
        }
    }

    printf("[ERROR] [%s] is not a ts file\n", file ? file : "null");

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

static hi_bool process_rewind(hi_handle avplay)
{
    hi_u32 count = 0;
    if (g_single_play && g_loop_play) {
        printf("read ts file end and rewind!\n");
        rewind(g_ts_file);
        return HI_TRUE;
    } else if (g_is_search_pmt) {
        return HI_TRUE;
    }

    hi_unf_avplay_flush_stream(avplay,HI_NULL);
    printf("read ts file end and wait eos!\n");

    while (!g_is_eos && !g_is_stop_ts_thread) {
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
    hi_unf_stream_buf   stream_buf;
    hi_u32                len;
    hi_s32                ret;
    tsplay_ctx           *ctx = args;
    hi_bool               rewind = HI_FALSE;

    while (!g_is_stop_ts_thread) {
        pthread_mutex_lock(&g_ts_mtx);

        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, TS_READ_SIZE, &stream_buf, 100);
        if (ret != HI_SUCCESS ) {
            pthread_mutex_unlock(&g_ts_mtx);
            usleep(1000);
            continue;
        }

        len = fread(stream_buf.data, sizeof(hi_s8), TS_READ_SIZE, g_ts_file);
        if (len <= 0) {
            rewind = process_rewind(ctx->avplay);
            pthread_mutex_unlock(&g_ts_mtx);

            if (rewind) {
                usleep(1000);
                continue;
            } else {
                break;
            }
        }

        ret = hi_unf_dmx_put_ts_buffer(g_ts_buf, len, 0);
        if (ret != HI_SUCCESS ) {
            printf("[%s %u] hi_unf_dmx_put_ts_buffer failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        }
        pthread_mutex_unlock(&g_ts_mtx);
        usleep(1000);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS){
        printf("[%s %u] hi_unf_dmx_reset_ts_buffer failed 0x%x\n", __FUNCTION__, __LINE__, ret);
    }

    printf("stopped put ts stream, ts_thread end\n");
    g_is_stop_ts_thread = HI_TRUE;
    g_is_eos = HI_FALSE;

    return HI_NULL;
}

static hi_s32 proc_evt_func(hi_handle avplay, hi_unf_avplay_event_type evt, void *para, hi_u32 para_len)
{
    switch(evt) {
        case HI_UNF_AVPLAY_EVENT_EOS :
            printf("[INFO] End of stream!\n");
            g_is_eos = HI_TRUE;
            break;
        default:
            break;
    }

    return HI_SUCCESS;
}

static void config_stdin_flag(hi_bool nonblock)
{
    hi_s32 flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags < 0) {
        printf("[ERROR] get flags failed.\n");
        return;
    }

    hi_bool flags_nonblock = flags & O_NONBLOCK;
    if (nonblock == flags_nonblock) {
        return;
    }

    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    flags = fcntl(STDIN_FILENO, F_SETFL, flags);
    if (flags < 0) {
        printf("[ERROR] set flags nonblock[%d] failed.\n", nonblock);
    }
}

static void wait_eos()
{
    hi_u32 count = 0;
    while(!g_is_eos) {
        usleep(WAIT_EOS_SLEEP_TIME);
        count++;

        if (count > WAIT_EOS_TIMEOUT_COUNT) {
            printf(" err wait timeout !\n");
            break;
        }
    }
}

static void process_input_cmd(tsplay_ctx *ctx)
{
    hi_s32 ret;
    hi_char input_cmd[32] = {0};
    hi_u32 prog_num = 0;
    hi_u32 tplay_speed = 2;

    hi_u32 window_size = MAX_WINDOW_SIZE;
    hi_unf_win_attr win_attr = {0};

    if (ctx == HI_NULL) {
        return;
    }

    printf("input  q  to quit\n" \
           "input  t  to tplay\n" \
           "input  r  to resume\n" \
           "input  g  to get status\n" \
           "input  s  to stop send stream and wait for the evt of eos!\n");

    while (!g_is_stop_ts_thread) {
        memset(input_cmd, 0, sizeof(input_cmd));
        ret = read(STDIN_FILENO, input_cmd, sizeof(input_cmd));
        if (ret <= 0) {
            usleep(500 * 1000);
            continue;
        }

        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            g_quit_play = HI_TRUE;
            break;
        }

        if ('n' == input_cmd[0]) {
            printf("play next file.\n");
            break;
        }

        if ('t' == input_cmd[0]) {
            hi_unf_avplay_tplay_opt tplay_opt;
            printf("%dx tplay\n",tplay_speed);

            tplay_opt.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
            tplay_opt.speed_integer = tplay_speed;
            tplay_opt.speed_decimal = 0;

            hi_unf_avplay_tplay(ctx->avplay, &tplay_opt);
            tplay_speed = (32 == tplay_speed * 2) ? (2) : (tplay_speed * 2) ;
            continue;
        }

        if ('p' == input_cmd[0]) {
            printf("pause\n");
            hi_unf_avplay_pause(ctx->avplay, HI_NULL);
            continue;
        }

        if ('r' == input_cmd[0]) {
            printf("resume\n");
            hi_unf_avplay_resume(ctx->avplay, HI_NULL);
            tplay_speed = 2;
            continue;
        }

        if ('f' == input_cmd[0]) {
            hi_unf_win_freeze_mode freeze_mode = HI_UNF_WIN_FREEZE_MODE_DISABLE;
            hi_unf_win_get_freeze_mode(ctx->vid.win, &freeze_mode);
            if (freeze_mode == HI_UNF_WIN_FREEZE_MODE_DISABLE) {
                hi_unf_win_set_freeze_mode(ctx->vid.win, HI_UNF_WIN_FREEZE_MODE_LAST);
            } else if (freeze_mode != HI_UNF_WIN_FREEZE_MODE_DISABLE) {
                hi_unf_win_set_freeze_mode(ctx->vid.win, HI_UNF_WIN_FREEZE_MODE_DISABLE);
            }
            printf("window freeze %d\n", freeze_mode);
            continue;
        }

        if ('w' == input_cmd[0]) {
            window_size /= 2;

            if (window_size == 1) {
                window_size = MAX_WINDOW_SIZE;
            }

            printf("Set window size = %u\n", window_size);
            ret = hi_unf_win_get_attr(ctx->vid.win, &win_attr);
            if (ret != HI_SUCCESS) {
                printf("[ERROR] GetWindowAttr failed, ret = 0x%x\n", ret);
                continue;
            }

            win_attr.output_rect.x = (MAX_WINDOW_SIZE - window_size) * 5;
            win_attr.output_rect.y = (MAX_WINDOW_SIZE - window_size) * 10;
            win_attr.output_rect.width = 3840 / MAX_WINDOW_SIZE * window_size;
            win_attr.output_rect.height = 2160 / MAX_WINDOW_SIZE * window_size;

            const hi_handle win_handle[1] = {ctx->vid.win};
            const hi_unf_video_rect outrect[1] = {win_attr.output_rect};
            ret = hi_unf_win_set_outrect(win_handle, outrect, 1);
            if (ret != HI_SUCCESS) {
                printf("[ERROR] SetWindowAttr failed, ret = 0x%x\n", ret);
                continue;
            }
        }

        if ('g' == input_cmd[0]) {
            hi_unf_sync_status status_info = {0};
            hi_unf_avplay_get_status_info(ctx->avplay, HI_UNF_AVPLAY_STATUS_TYPE_SYNC, &status_info);
            printf("localtime %lld playtime %lld\n", status_info.local_time,
                status_info.play_time);
            continue;
        }

        if ('s' == input_cmd[0]) {
            printf("stop send stream, and wait for the eos!\n");
            g_is_stop_ts_thread = HI_TRUE;
            hi_unf_avplay_flush_stream(ctx->avplay,HI_NULL);
            wait_eos();
            break;
        }

        if ('c' == input_cmd[0]) {
            prog_num = atoi(&input_cmd[1]);
            if (prog_num == 0) {
                prog_num = 1;
            }
            printf("change channel to %u\n", prog_num);

            pthread_mutex_lock(&g_ts_mtx);
            rewind(g_ts_file);
            hi_unf_dmx_reset_ts_buffer(g_ts_buf);

            ret = hi_adp_avplay_play_prog(ctx->avplay, g_prog_tbl, prog_num - 1, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
            pthread_mutex_unlock(&g_ts_mtx);
            if (ret != HI_SUCCESS) {
                printf("call hi_adp_avplay_play_prog failed!\n");
                break;
            }
            continue;
        }

        printf("please input the q to quit!\n");
    }
}

static hi_s32 init_play(hi_unf_video_format enc_fmt)
{
    hi_s32 ret = HI_SUCCESS;

    hi_unf_sys_init();
    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_snd_init failed.\n");
        return HI_FAILURE;
    }

#ifdef HI_BOOT_HOMOLOGOUS_SUPPORT
    ret = hi_adp_disp_init_mutex(enc_fmt);
#else
    ret = hi_adp_disp_init(enc_fmt);
#endif
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_disp_init failed.\n");
        return HI_FAILURE;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_win_init failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_init failed.\n");
        return HI_FAILURE;
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.buffer_size = 0x1000000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_create_ts_buffer failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (HI_SUCCESS != ret) {
        printf("[ERROR] call hi_unf_dmx_attach_ts_port failed.\n");
        return HI_FAILURE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_avplay_init failed.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 deinit_play()
{
    hi_unf_avplay_deinit();
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();
    hi_adp_disp_deinit();
    hi_adp_snd_deinit();
    hi_unf_sys_deinit();

    if (g_media_dir != HI_NULL) {
        closedir(g_media_dir);
        g_media_dir = HI_NULL;
    }

    return HI_SUCCESS;
}

static hi_s32 ts_vid_open(hi_handle avplay, tsplay_vid_ctx *vid)
{
    hi_s32 ret = HI_SUCCESS;

    ret = hi_adp_win_create(HI_NULL, &vid->win);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_win_create failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_chan_open failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_win_attach_src(vid->win, avplay);
    if (HI_SUCCESS != ret) {
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

static hi_s32 ts_aud_open(hi_handle avplay, tsplay_aud_ctx *aud)
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

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &aud->track);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_create_track failed.\n");
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

static hi_s32 ts_play(const hi_char *file)
{
    hi_s32 ret = HI_SUCCESS;
    hi_unf_avplay_stop_opt stop_opt = {0};
    tsplay_ctx context = {0};

    context.avplay = HI_INVALID_HANDLE;
    context.vid.win = HI_INVALID_HANDLE;
    context.aud.track = HI_INVALID_HANDLE;

    g_ts_file = fopen(file, "rb");
    if (g_ts_file == HI_NULL) {
        printf("[ERROR] open file %s error: %s\n", file, strerror(errno));
        return HI_FAILURE;
    } else {
        printf("[INFO] begin to play %s\n", file);
    }

    hi_unf_avplay_attr avplay_attr = {0};
    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DMX_ID;

    ret |= hi_unf_avplay_create(&avplay_attr, &context.avplay);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_create failed.\n");
        goto close_file;
    }

    ret = hi_unf_avplay_register_event(context.avplay, HI_UNF_AVPLAY_EVENT_EOS, proc_evt_func);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_register_event failed.\n");
        goto destroy_av;
    }

    pthread_mutex_init(&g_ts_mtx, HI_NULL);
    g_is_stop_ts_thread = HI_FALSE;
    g_is_search_pmt = HI_TRUE;
    pthread_create(&g_ts_thread, HI_NULL, ts_thread, &context);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call HIADP_Search_GetAllPmt failed.\n");
        goto ts_free;
    }

#ifdef TSPLAY_SUPPORT_VID_CHAN
    ret = ts_vid_open(context.avplay, &context.vid);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call ts_vid_open failed.\n");
        goto ts_free;
    }
#endif

#ifdef TSPLAY_SUPPORT_AUD_CHAN
    ret = ts_aud_open(context.avplay, &context.aud);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call ts_aud_open failed.\n");
        goto ts_free;
    }
#endif

    hi_unf_sync_attr sync_attr = {0};
    (void)hi_unf_avplay_get_attr(context.avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);

    sync_attr.ref_mode = g_sync_off ? HI_UNF_SYNC_REF_MODE_NONE : HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(context.avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        printf("[WARN] call hi_unf_avplay_set_attr failed.\n");
    }

    pthread_mutex_lock(&g_ts_mtx);
    rewind(g_ts_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    g_is_search_pmt = HI_FALSE;

    ret = hi_adp_avplay_play_prog(context.avplay, g_prog_tbl, 0, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    pthread_mutex_unlock(&g_ts_mtx);

    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_avplay_play_prog failed.\n");
        goto out;
    }

    process_input_cmd(&context);

out:
    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    (void)hi_unf_avplay_stop(context.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);

ts_free:
    hi_adp_search_free_all_pmt(g_prog_tbl);
    g_prog_tbl = HI_NULL;
    hi_adp_search_de_init();

    g_is_stop_ts_thread = HI_TRUE;
    pthread_join(g_ts_thread, HI_NULL);

    if (context.aud.track != HI_INVALID_HANDLE) {
        (void)ts_aud_close(context.avplay, &context.aud);
    }

    if (context.vid.win != HI_INVALID_HANDLE) {
        (void)ts_vid_close(context.avplay, &context.vid);
    }

destroy_av:
    (void)hi_unf_avplay_destroy(context.avplay);

close_file:
    if (g_ts_file != HI_NULL) {
        fclose(g_ts_file);
        g_ts_file = HI_NULL;
    }

    return ret;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_video_format enc_fmt = HI_UNF_VIDEO_FMT_1080P_60;

    if (argc < 2) {
        printf("Usage: %s file/dir [vo_format]\n", argv[0]);
        printf("\tvo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n");
        printf("Example:\n\t%s ./test.ts 1080P50\n", argv[0]);
        return HI_SUCCESS;
    }

    if (argc >= 3) {
        enc_fmt = hi_adp_str2fmt(argv[2]);
    }

    if (argc >= 4 && strcasecmp(argv[3], "sync_off") == 0) {
        g_sync_off = HI_TRUE;
    }

    const hi_char *file = get_media_name(argv[1]);
    if (file == HI_NULL) {
        printf("[ERROR] get file failed.\n");
        return HI_FAILURE;
    }

    config_stdin_flag(HI_FALSE);
    ret = init_play(enc_fmt);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] init play failed.\n");
        goto out;
    }

    do {
        ret = ts_play(file);
        if (ret != HI_SUCCESS) {
            printf("[ERROR] tsplay %s failed.\n", file);
        } else if (g_quit_play) {
            break;
        }

        file = get_media_name(HI_NULL);
    } while (file != HI_NULL);

out:
    (void)deinit_play();
    config_stdin_flag(HI_FALSE);

    return ret;
}

