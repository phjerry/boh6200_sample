/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample for ts playback on multi-screen
 * Author: sdk
 * Create: 2019-07-21
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "securec.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_svr_dispmng.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_adp_search.h"
#include "hi_adp_mpi.h"

#include "hi_unf_acodec_g711.h"
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_amrwb.h"
#include "hi_unf_acodec_truehdpassthrough.h"
#include "hi_unf_acodec_dolbytruehddec.h"
#include "hi_unf_acodec_dtshddec.h"

#define PLAY_DMX_ID         0
#define MAX_WINDOW_NUM      16
#define TS_READ_SIZE        (188 * 1000)
#define MAX_DISPLAY_NUM     2

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

typedef struct {
    hi_svr_dispmng_display_id display_id;
    hi_unf_video_format format;
    hi_unf_snd_track_type audio_track;
    hi_unf_snd snd;
    int dmx_id;
    int dmx_ram_port_id;
} tsplay_struct_t;

static FILE            *g_ts_file = HI_NULL;
static pthread_t        g_ts_thread;
static pthread_mutex_t  g_ts_mtx;
static hi_bool          g_is_stop_ts_thread = HI_FALSE;
static hi_handle        g_ts_buf = HI_INVALID_HANDLE;
static pmt_compact_tbl *g_prog_tbl = HI_NULL;
static hi_bool          g_is_eos = HI_FALSE;
static hi_bool          g_quit_play = HI_FALSE;
static hi_bool          g_single_play = HI_FALSE;

static DIR *g_media_dir = HI_NULL;
static hi_u32 g_media_cnt = 0;
static hi_bool g_loop_play = HI_TRUE;

static tsplay_struct_t g_tsplay_param[MAX_DISPLAY_NUM] = {
    { HI_SVR_DISPMNG_DISPLAY_ID_MASTER, HI_UNF_VIDEO_FMT_1080P_50, HI_UNF_SND_TRACK_TYPE_MASTER,
        HI_UNF_SND_0, PLAY_DMX_ID,   HI_UNF_DMX_PORT_RAM_0 },
    { HI_SVR_DISPMNG_DISPLAY_ID_SLAVE,  HI_UNF_VIDEO_FMT_1080P_50, HI_UNF_SND_TRACK_TYPE_SLAVE,
        HI_UNF_SND_1, PLAY_DMX_ID + 1, HI_UNF_DMX_PORT_RAM_0 + 1 }
};

hi_s32 hi_adp_snd_init_ext(hi_unf_snd snd)
{
    hi_s32             ret;
    hi_unf_snd_attr  attr;

    ret = hi_unf_snd_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_init failed,ret=%x\n", ret);
        return ret;
    }

    ret = hi_unf_snd_get_default_open_attr(snd, &attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_get_default_open_attr failed.\n");
        return ret;
    }

    ret = hi_unf_snd_open(snd, &attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_open sound %d failed, ret=%x\n", snd, ret);
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 hi_adp_snd_deinit_ext(hi_unf_snd snd)
{
    hi_s32 ret;

    ret = hi_unf_snd_close(snd);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_close failed.\n");
        return ret;
    }

    ret = hi_unf_snd_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_bool is_attached_panel_intf(hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_status status;
    hi_s32 ret, i;

    ret = hi_svr_dispmng_get_display_status(id, &status);
    if (ret != HI_SUCCESS) {
        printf("hi_svr_dispmng_get_display_status fail(ret=0x%X)\n", ret);
        return HI_FALSE;
    }

    for (i = 0; i < status.number; i++) {
        if (status.intf[i].intf_type == HI_SVR_DISPMNG_INTERFACE_PANEL) {
            return HI_TRUE;
        }
    }

    return HI_FALSE;
}

hi_void print_display_mode(const hi_svr_dispmng_display_id id)
{
    hi_s32 ret;
    hi_svr_dispmng_display_mode  mode;

    ret = hi_svr_dispmng_get_display_mode(id, &mode);
    if (ret != HI_SUCCESS) {
        printf("[ERROR]hi_svr_dispmng_get_display_mode fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }

    printf("[INFO]Display Mode:\n");
    printf("\tvic:%d\n", mode.vic);
    printf("\tformat:%d\n", mode.format);
    printf("\tpixel format:%d\n", mode.pixel_format);
    printf("\tpixel width:%d\n", mode.bit_depth);
    printf("\tname:%s\n", mode.name);
    printf("\twidth:%d\n", mode.width);
    printf("\theight:%d\n", mode.height);
    printf("\tframe rate:%d\n", mode.field_rate);
    printf("\tprogressive:%d\n", mode.progressive);
}

hi_s32 hi_adp_disp_init_ext(hi_svr_dispmng_display_id id, hi_unf_video_format format)
{
    hi_s32 ret;

    if (id >= HI_SVR_DISPMNG_DISPLAY_ID_MAX) {
        printf("hi_adp_disp_init_ext invalid display id:%d\n", id);
        return HI_FAILURE;
    }

    ret = hi_svr_dispmng_init();
    if (ret != HI_SUCCESS) {
        printf("hi_svr_dispmng_init failed(ret=0x%X)\n", ret);
        return ret;
    }

    if (!is_attached_panel_intf(id)) {
        ret = hi_adp_disp_set_format(id, format);
        if (ret != HI_SUCCESS) {
            printf("hi_adp_disp_set_format failed 0x%x\n", ret);
            return ret;
        }
    } else {
        printf("[INFO]PANEL interface is attached, It can only use custom timing.\n");
    }
    print_display_mode(id);

    /* Set HDR type to SDR */
    ret = hi_svr_dispmng_set_hdr_type(id, HI_SVR_DISPMNG_HDR_TYPE_SDR);
    if (ret != HI_SUCCESS) {
        printf("[ERROR]hi_svr_dispmng_set_hdr_type fail(ret=0x%X).\n", ret);
    }

    /* Set HDR match content to TRUE */
    ret = hi_svr_dispmng_set_hdr_match_content(id, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("[ERROR]hi_svr_dispmng_set_hdr_match_content fail(ret=0x%X).\n", ret);
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_disp_deinit_ext(hi_svr_dispmng_display_id id)
{
    hi_s32 ret;

    if (id >= HI_SVR_DISPMNG_DISPLAY_ID_MAX) {
        printf("hi_adp_disp_deinit_ext invalid disp:%d\n", id);
        return HI_FAILURE;
    }

    ret = hi_svr_dispmng_deinit();
    if (ret != HI_SUCCESS) {
        printf("hi_svr_dispmng_deinit failed, ret=0x%X.\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_bool is_ts_file(const char *file)
{
    const hi_char *ts_suffix = ".ts";
    hi_s32 ts_suffix_len = strlen(ts_suffix);
    hi_s32 len = strlen(file);

    if (file == HI_NULL) {
        printf("[WARN] [%s] is not a ts file, len : %d\n", "null", len);
        return HI_FALSE;
    }

    if (len <= ts_suffix_len) {
        printf("[WARN] [%s] is not a ts file, len : %d\n", file, len);
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

#define SLEEPING_TIME           500000
#define MAX_RETRY_TIMES         60
static hi_bool process_rewind(hi_handle avplay)
{
    hi_u32 count = 0;
    if (g_single_play && g_loop_play) {
        printf("read ts file end and rewind!\n");
        rewind(g_ts_file);
        return HI_TRUE;
    }

    hi_unf_avplay_flush_stream(avplay, HI_NULL);
    printf("read ts file end and wait eos!\n");

    while (!g_is_eos && !g_is_stop_ts_thread) {
        usleep(SLEEPING_TIME);
        count++;
        if (count > MAX_RETRY_TIMES) {
            printf("[WARN] wait eos timeout !\n");
            break;
        }
    }

    return HI_FALSE;
}

#define TIMEOUT_IN_MS           1000
static void *ts_thread(void *args)
{
    hi_unf_stream_buf   stream_buf;
    hi_u32                len;
    hi_s32                ret;
    tsplay_ctx           *ctx = args;
    hi_bool               rewind = HI_FALSE;

    while (!g_is_stop_ts_thread) {
        pthread_mutex_lock(&g_ts_mtx);

        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, TS_READ_SIZE, &stream_buf, TIMEOUT_IN_MS);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mtx);
            continue;
        }

        len = fread(stream_buf.data, sizeof(hi_s8), TS_READ_SIZE, g_ts_file);
        if (len <= 0) {
            rewind = process_rewind(ctx->avplay);
            pthread_mutex_unlock(&g_ts_mtx);

            if (rewind) {
                continue;
            } else {
                break;
            }
        }

        ret = hi_unf_dmx_put_ts_buffer(g_ts_buf, len, 0);
        if (ret != HI_SUCCESS) {
            printf("[%s %u] HI_UNF_DMX_PutTSBuffer failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        }
        pthread_mutex_unlock(&g_ts_mtx);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("[%s %u] hi_unf_dmx_reset_ts_buffer failed 0x%x\n", __FUNCTION__, __LINE__, ret);
    }

    g_is_stop_ts_thread = HI_TRUE;
    return HI_NULL;
}

static hi_s32 proc_evt_func(hi_handle avplay, hi_unf_avplay_event_type evt, hi_void *para, hi_u32 param_len)
{
    switch (evt) {
        case HI_UNF_AVPLAY_EVENT_EOS:
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

#define WIN_NUMBER_PER_LINE_MAX   4
#define SCREEN_WIDTH             1920
#define SCREEN_HEIGHT            1080
hi_u32 g_window_per_line = WIN_NUMBER_PER_LINE_MAX;
hi_void resize_window(tsplay_ctx *ctx)
{
    hi_s32 ret;
    hi_unf_video_rect rect[1] = {0};
    hi_handle win[1]  = {0};
    hi_u32 win_width, win_height;

    win[0] = ctx->vid.win;
    g_window_per_line--;
    if (g_window_per_line == 0) {
        g_window_per_line = WIN_NUMBER_PER_LINE_MAX;
    }

    win_width = SCREEN_WIDTH / g_window_per_line;
    win_height = SCREEN_HEIGHT / g_window_per_line;
    rect[0].x = (SCREEN_WIDTH - win_width) / 2;
    rect[0].y = (SCREEN_HEIGHT - win_height) / 2;
    rect[0].width  = win_width;
    rect[0].height = win_height;
    ret = hi_unf_win_set_outrect(win, rect, 1);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] hi_unf_win_set_outrect failed, ret = 0x%x\n", ret);
    } else {
        printf("Set window size = (%u, %u).\n", win_width, win_height);
    }
}

hi_bool g_freeze = HI_FALSE;
hi_void freeze_window(tsplay_ctx *ctx)
{
    hi_s32 ret;
    g_freeze = !g_freeze;

    if (g_freeze) {
        ret = hi_unf_win_set_freeze_mode(ctx->vid.win, HI_UNF_WIN_FREEZE_MODE_LAST);
    } else {
        ret = hi_unf_win_set_freeze_mode(ctx->vid.win, HI_UNF_WIN_FREEZE_MODE_DISABLE);
    }

    if (ret != HI_SUCCESS) {
        printf("hi_unf_win_set_freeze_mode faile(ret=0x%X).\n", ret);
    } else {
        printf("window freeze %s\n", g_freeze ? "enable" : "disable");
    }
}

#define MAX_BUFFER_SIZE         32
#define TRICK_PLAY_SPEED_2X     2
#define TRICK_PLAY_SPEED_4X     4
#define TRICK_PLAY_SPEED_8X     8
#define TRICK_PLAY_SPEED_16X    16
#define TRICK_PLAY_SPEED_STEP   2
#define TRICK_PLAY_SPEED_32X    32
static void process_input_cmd(tsplay_ctx *ctx)
{
    hi_s32 ret;
    hi_char input_cmd[MAX_BUFFER_SIZE] = {0};
    hi_u32 prog_num = 0;
    hi_u32 tplay_speed = TRICK_PLAY_SPEED_2X;

    if (ctx == HI_NULL) {
        return;
    }

    printf("input  q  to quit\n" \
           "input  t  to tplay\n" \
           "input  r  to resume\n" \
           "input  f  to freeze\n" \
           "input  g  to get status\n" \
           "input  s  to stop send stream and wait for the evt of eos!\n");

    while (!g_is_stop_ts_thread) {
        memset_s(input_cmd, sizeof(input_cmd), 0, sizeof(input_cmd));
        ret = read(STDIN_FILENO, input_cmd, sizeof(input_cmd));
        if (ret <= 0) {
            usleep(SLEEPING_TIME);
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
            printf("%dx tplay\n", tplay_speed);

            tplay_opt.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
            tplay_opt.speed_integer = tplay_speed;
            tplay_opt.speed_decimal = 0;
            tplay_opt.audio_enable  = HI_FALSE;

            hi_unf_avplay_set_decode_mode(ctx->avplay, HI_UNF_VDEC_WORK_MODE_I);
            hi_unf_avplay_tplay(ctx->avplay, &tplay_opt);
            tplay_speed = (TRICK_PLAY_SPEED_32X == tplay_speed * TRICK_PLAY_SPEED_STEP) ?
                (TRICK_PLAY_SPEED_2X) : (tplay_speed * TRICK_PLAY_SPEED_STEP);
            continue;
        }

        if ('p' == input_cmd[0]) {
            printf("pause\n");
            hi_unf_avplay_pause(ctx->avplay, HI_NULL);
            continue;
        }

        if ('r' == input_cmd[0]) {
            printf("resume\n");
            hi_unf_avplay_set_decode_mode(ctx->avplay, HI_UNF_VDEC_WORK_MODE_NORMAL);
            hi_unf_avplay_resume(ctx->avplay, HI_NULL);
            tplay_speed = TRICK_PLAY_SPEED_2X;
            continue;
        }

        if ('f' == input_cmd[0]) {
            freeze_window(ctx);
            continue;
        }

        if ('w' == input_cmd[0]) {
            resize_window(ctx);
            continue;
        }

        if ('g' == input_cmd[0]) {
            hi_unf_sync_status status = {0};
            hi_unf_avplay_get_status_info(ctx->avplay, HI_UNF_AVPLAY_STATUS_TYPE_SYNC, &status);
            printf("localtime %lld playtime %lld\n", status.local_time,
                status.play_time);
            continue;
        }

        if ('s' == input_cmd[0]) {
            hi_u32 count = 0;

            printf("stop send stream, and wait for the eos!\n");
            g_is_stop_ts_thread = HI_TRUE;
            hi_unf_avplay_flush_stream(ctx->avplay, HI_NULL);
            /* Wait until end of stream or timeout */
            while (!g_is_eos && count < MAX_RETRY_TIMES) {
                usleep(SLEEPING_TIME);
                count++;
            }
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

static hi_s32 init_module(tsplay_struct_t *p_tsplay, hi_unf_video_format enc_fmt)
{
    hi_s32 ret;

    if (p_tsplay == NULL) {
        printf("init_play invalid param\n");
        return HI_FAILURE;
    }

    ret = hi_adp_snd_init_ext(p_tsplay->snd);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call HIADP_Snd_Init failed.\n");
       // return HI_FAILURE;
    }

    ret = hi_adp_disp_init_ext(p_tsplay->display_id, enc_fmt);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_disp_init_ext failed.\n");
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

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_adp_avplay_init failed.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 init_play(tsplay_struct_t *p_tsplay, hi_unf_video_format enc_fmt)
{
    hi_s32 ret;

    if (p_tsplay == NULL) {
        printf("init_play invalid param\n");
        return HI_FAILURE;
    }

    ret = init_module(p_tsplay, enc_fmt);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] initial module failed.\n");
        return HI_FAILURE;
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.buffer_size = 0x1000000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ret = hi_unf_dmx_create_ts_buffer(p_tsplay->dmx_ram_port_id, &tsbuf_attr, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_create_ts_buffer failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_attach_ts_port(p_tsplay->dmx_id, p_tsplay->dmx_ram_port_id);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_dmx_attach_ts_port failed.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 deinit_play(tsplay_struct_t *p_tsplay)
{
    hi_s32 ret;
    if (p_tsplay == NULL) {
        printf("deinit_play invalid param\n");
        return HI_FAILURE;
    }

    printf("deinit_play, display_id:%d, dmx_id:%d, dmx_ram_port_id:%d\n",
        p_tsplay->display_id, p_tsplay->dmx_id, p_tsplay->dmx_ram_port_id);

    hi_unf_avplay_deinit();
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);
    hi_unf_dmx_detach_ts_port(p_tsplay->dmx_id);
    hi_unf_dmx_deinit();
    ret = hi_adp_disp_deinit_ext(p_tsplay->display_id);
    if (ret != HI_SUCCESS) {
        printf("hi_adp_disp_deinit_ext fail.\n");
    }
    ret = hi_adp_snd_deinit_ext(p_tsplay->snd);
    if (ret != HI_SUCCESS) {
        printf("hi_adp_snd_deinit_ext fail.\n");
    }

    if (g_media_dir != HI_NULL) {
        closedir(g_media_dir);
        g_media_dir = HI_NULL;
    }

    printf("deinit_play, display_id:%d end sucess\n", p_tsplay->display_id);

    return HI_SUCCESS;
}

#ifdef TSPLAY_SUPPORT_VID_CHAN
static hi_s32 ts_vid_open(hi_svr_dispmng_display_id id, hi_handle avplay, tsplay_vid_ctx *vid)
{
    hi_s32 ret;

    printf("ts_vid_open, display_id:%d\n", id);

    ret = hi_adp_win_create_ext(HI_NULL, &vid->win, id);
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
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_win_attach_src failed:%#x.\n", ret);
        return HI_FAILURE;
    }

    ret = hi_unf_win_set_enable(vid->win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_win_set_enable failed.\n");
        return HI_FAILURE;
    }

    printf("ts_vid_open, display_id:%d success\n", id);

    return HI_SUCCESS;
}
#endif

static hi_s32 ts_vid_close(hi_handle avplay, tsplay_vid_ctx *vid)
{
    if (vid->win != HI_INVALID_HANDLE) {
        printf("ts_vid_close called\n");
        hi_unf_win_set_enable(vid->win, HI_FALSE);
        hi_unf_win_detach_src(vid->win, avplay);
        hi_unf_win_destroy(vid->win);

        vid->win = HI_INVALID_HANDLE;
    }

    return hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
}

#ifdef TSPLAY_SUPPORT_AUD_CHAN
static hi_s32 ts_aud_open(hi_unf_snd snd, hi_unf_snd_track_type audio_track, hi_handle avplay,
    tsplay_aud_ctx *aud)
{
    hi_s32 ret;

    printf("ts_aud_open,snd:%d,audio_track:%d\n", snd, audio_track);

    hi_unf_audio_track_attr track_attr;
    ret = hi_unf_snd_get_default_track_attr(audio_track, &track_attr);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_get_default_track_attr failed, ret:%d\n", ret);
        return HI_FAILURE;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_avplay_chan_open failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_snd_create_track(snd, &track_attr, &aud->track);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_create_track failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_snd_attach(aud->track, avplay);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call hi_unf_snd_attach failed.\n");
        return HI_FAILURE;
    }

    printf("ts_aud_open, audio_track:%d success\n", audio_track);

    return HI_SUCCESS;
}
#endif

static hi_s32 ts_aud_close(hi_handle avplay, tsplay_aud_ctx *aud)
{
    if (aud->track != HI_INVALID_HANDLE) {
        hi_unf_snd_detach(aud->track, avplay);
        hi_unf_snd_destroy_track(aud->track);

        aud->track = HI_INVALID_HANDLE;
    }

    return hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
}

static hi_s32 ts_play(tsplay_struct_t *p_tsplay, const hi_char *file)
{
    hi_s32 ret;
    hi_unf_avplay_stop_opt stop_opt = {0};
    tsplay_ctx context = {0};

    if (p_tsplay == NULL) {
        printf("ts_play invalid param\n");
        return HI_FAILURE;
    }

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

    hi_unf_avplay_attr avplay_attr;
    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = p_tsplay->dmx_id;

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
    pthread_create(&g_ts_thread, HI_NULL, ts_thread, &context);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(p_tsplay->dmx_id, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call HIADP_Search_GetAllPmt failed.\n");
        goto ts_free;
    }

#ifdef TSPLAY_SUPPORT_VID_CHAN
    ret = ts_vid_open(p_tsplay->display_id, context.avplay, &context.vid);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call ts_vid_open failed.\n");
        goto ts_free;
    }
#endif

#ifdef TSPLAY_SUPPORT_AUD_CHAN
    ret = ts_aud_open(p_tsplay->snd, p_tsplay->audio_track, context.avplay, &context.aud);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] call ts_aud_open failed.\n");
        goto ts_free;
    }
#endif

    hi_unf_sync_attr sync_attr;
    (void)hi_unf_avplay_get_attr(context.avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);

    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;
    ret |= hi_unf_avplay_set_attr(context.avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        printf("[WARN] call hi_unf_avplay_set_attr failed.\n");
    }

    pthread_mutex_lock(&g_ts_mtx);
    rewind(g_ts_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buf);
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

hi_void print_usage(hi_s32 argc, hi_char *argv[])
{
    printf("Usage: %s file/dir [display] [vo_format]\n", argv[0]);
    printf("\tdisplay:0--master|1--slave\n");
    printf("\tvo_format:4320P120|4320P60|2160P60|2160P50|1080P60|1080P50\n");
    printf("Example:\n\t%s ./test.ts 0 1080P50\n", argv[0]);
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_video_format enc_fmt = HI_UNF_VIDEO_FMT_1080P_60;
    tsplay_struct_t *p_tsplay = NULL; ;
    int disp_index = 0;

    if (argc < 2) {
        print_usage(argc, argv);
        return HI_SUCCESS;
    }

    if (argc >= 3) {
        disp_index = atoi(argv[2]);
        if (disp_index < 0 || disp_index > 1) {
            printf("invalid hdmi_index:%s, reset to master display\n", argv[2]);
            disp_index = 0;
        }
    }

    if (argc >= 4) {
        enc_fmt = hi_adp_str2fmt(argv[3]);
    }

    p_tsplay = &g_tsplay_param[disp_index];

    const hi_char *file = get_media_name(argv[1]);
    if (file == HI_NULL) {
        printf("[ERROR] get file failed.\n");
        return HI_FAILURE;
    }

    config_stdin_flag(HI_TRUE);
    ret = init_play(p_tsplay, enc_fmt);
    if (ret != HI_SUCCESS) {
        printf("[ERROR] init play failed.\n");
        goto out;
    }

    do {
        ret = ts_play(p_tsplay, file);
        if (ret != HI_SUCCESS) {
            printf("[ERROR] tsplay %s failed.\n", file);
        } else if (g_quit_play) {
            break;
        }

        file = get_media_name(HI_NULL);
    } while (file != HI_NULL);

out:
    ret = deinit_play(p_tsplay);
    if (ret != HI_SUCCESS) {
        printf("deinit_play fail.\n");
    }
    config_stdin_flag(HI_FALSE);

    return ret;
}

