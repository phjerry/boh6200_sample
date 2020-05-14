/******************************************************************************
  Copyright (C), 2011-2021, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : es_ts_switch.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2011/01/26
  Description   :
  History       :
  1.Date        : 2011/01/26
    Author      :
    Modification: Created file
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#include <assert.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

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

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

static hi_handle   g_avplay_es;
static hi_handle   g_avplay_ts;
static pthread_t   g_es_thread;
static pthread_t   g_ts_thread;
static pthread_mutex_t g_ts_mutex;
static pthread_mutex_t g_es_mutex;
static hi_handle   g_track_handle;


typedef struct {
    hi_bool           pause_es_thread;
    hi_bool           stop_es_thread;
    hi_bool           pause_ts_thread;
    hi_bool           stop_ts_thread;
} thread_status;

static thread_status  g_thread_ctx = {0};

typedef enum {
    PLAY_TYPE_ES,
    PLAY_TYPE_TS,
    PLAY_TYPE_BUTT
} play_type;

FILE               *g_ts_file = HI_NULL;
FILE               *g_es_file = HI_NULL;

static hi_bool g_read_frame_size = HI_FALSE;

hi_handle           g_ts_buffer_handle;
pmt_compact_tbl    *g_prog_table = HI_NULL;

#define  PLAY_DMX_ID  0

hi_void es_thread(hi_void *args)
{
    hi_unf_stream_buf stream_buffer;
    hi_u32 read_len;
    hi_s32 ret;
    hi_u32 frame_size = 0;

    while (HI_TRUE) {
        if (g_thread_ctx.stop_es_thread == HI_TRUE) {
            break;
        }

        if (g_thread_ctx.pause_es_thread == HI_TRUE) {
            usleep(1000 * 10);
            continue;
        }

        pthread_mutex_lock(&g_es_mutex);

        if (g_read_frame_size != HI_FALSE) {
            ret = hi_unf_avplay_get_buf(g_avplay_es, HI_UNF_AVPLAY_BUF_ID_ES_VID, 0x100000, &stream_buffer, 0);
        }
        else {
            ret = hi_unf_avplay_get_buf(g_avplay_es, HI_UNF_AVPLAY_BUF_ID_ES_VID, 0x4000, &stream_buffer, 0);
        }

        if (ret == HI_SUCCESS) {
            if (g_read_frame_size != HI_FALSE) {
                read_len = fread(&frame_size, 1, 4, g_es_file);
                if (read_len == 4) {
                }
                else {
                    frame_size = 0x4000;
                }
            }
            else {
                frame_size = 0x4000;
            }

            read_len = fread(stream_buffer.data, sizeof(hi_s8), frame_size, g_es_file);

            if (read_len > 0) {
                hi_unf_avplay_put_buf_opt opt = {0};
                opt.end_of_frame = HI_FALSE;
                opt.is_continue = HI_FALSE;
                ret = hi_unf_avplay_put_buf(g_avplay_es, HI_UNF_AVPLAY_BUF_ID_ES_VID, read_len, 0, &opt);
                if (ret != HI_SUCCESS) {
                    sample_printf("call hi_unf_avplay_put_buf failed.\n");
                }

                frame_size = 0;
            }
            else if (read_len <= 0) {
                sample_printf("read vid file error!\n");
                rewind(g_es_file);
            }
        }
        else {
           /* wait for buffer */
            usleep(1000 * 10);
        }

        pthread_mutex_unlock(&g_es_mutex);
    }

    return;
}

hi_void ts_thread(hi_void *args)
{
    hi_unf_stream_buf   stream_buffer;
    hi_u32                read_len;
    hi_s32                ret;

    while (HI_TRUE) {
        if (g_thread_ctx.stop_ts_thread == HI_TRUE) {
            break;
        }

        if (g_thread_ctx.pause_ts_thread == HI_TRUE) {
            usleep(1000 * 10);
            continue;
        }

        pthread_mutex_lock(&g_ts_mutex);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buffer_handle, 188 * 50, &stream_buffer, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        read_len = fread(stream_buffer.data, sizeof(hi_s8), 188 * 50, g_ts_file);
        if (read_len <= 0) {
            sample_printf("read ts file end and rewind!\n");
            rewind(g_ts_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_buffer_handle, &stream_buffer, 0, read_len);
        if (ret != HI_SUCCESS ) {
            sample_printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_ts_mutex);
    }

    if (g_thread_ctx.pause_ts_thread == HI_FALSE) {
        ret = hi_unf_dmx_reset_ts_buffer(g_ts_buffer_handle);
        if (ret != HI_SUCCESS ) {
            sample_printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
        }
    }

    return;
}

hi_s32 create_common_resource(hi_unf_video_format vid_format, hi_handle *win_handle)
{
    hi_s32 ret;
    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_sys_init failed.\n");
        return ret;
    }

    //hi_adp_mce_exit();

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(vid_format);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_disp_init failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, win_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    return ret;

DISP_DEINIT:
    hi_adp_disp_deinit();

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return ret;
}

hi_s32 destroy_common_resource(hi_handle win_handle)
{
    hi_unf_win_destroy(win_handle);
    hi_adp_win_deinit();
    hi_adp_disp_deinit();
    hi_unf_avplay_deinit();
    hi_unf_sys_deinit();

    return HI_SUCCESS;
}

hi_s32 create_ts_resource(hi_handle win_handle)
{
    hi_s32 ret;
    hi_unf_avplay_attr   avplay_attr;
    hi_unf_sync_attr     sync_attr;

    hi_unf_audio_track_attr stTrackAttr;
    hi_unf_dmx_ts_buffer_attr   tsbuf_attr;

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call SndInit failed.\n");
        return ret;
    }

    ret = hi_unf_dmx_init();
    ret |= hi_unf_dmx_attach_ts_port(PLAY_DMX_ID,HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_dmx_init failed.\n");
        goto SND_DEINIT;
    }

    tsbuf_attr.buffer_size = 0x200000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;

    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_buffer_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DMX_ID;
    ret |= hi_unf_avplay_create(&avplay_attr, &g_avplay_ts);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto TSBUF_FREE;
    }

    ret = hi_unf_avplay_chan_open(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win_handle, g_avplay_ts);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
        goto ACHN_CLOSE;
    }
    ret |= hi_unf_win_set_enable(win_handle, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &stTrackAttr, &g_track_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(g_track_handle, g_avplay_ts);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(g_avplay_ts, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(g_avplay_ts, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    return ret;

SND_DETACH:
    hi_unf_snd_detach(g_track_handle, g_avplay_ts);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(g_track_handle);

WIN_DETACH:
    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, g_avplay_ts);

ACHN_CLOSE:
    hi_unf_avplay_chan_close(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(g_avplay_ts);

TSBUF_FREE:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buffer_handle);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

    return ret;
}

hi_s32 destroy_ts_resource(hi_handle win_handle)
{
    hi_unf_avplay_stop_opt    stop;

    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

    hi_adp_search_free_all_pmt(g_prog_table);
    g_prog_table = HI_NULL;
    hi_adp_search_de_init();

    hi_unf_snd_detach(g_track_handle, g_avplay_ts);

    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, g_avplay_ts);

    hi_unf_avplay_chan_close(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_chan_close(g_avplay_ts, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    hi_unf_avplay_destroy(g_avplay_ts);

    hi_unf_dmx_destroy_ts_buffer(g_ts_buffer_handle);
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

    hi_adp_snd_deinit();

    return HI_SUCCESS;
}

hi_s32 create_es_resource(hi_handle win_handle, hi_unf_vcodec_type vdec_type)
{
    hi_s32 ret;
    hi_unf_avplay_attr   avplay_attr;
    hi_unf_sync_attr     sync_attr;

    ret  = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_ES);
    avplay_attr.vid_buf_size = 0x300000;
    ret |= hi_unf_avplay_create(&avplay_attr, &g_avplay_es);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        return ret;
    }

    ret = hi_unf_avplay_get_attr(g_avplay_es, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;
    ret |= hi_unf_avplay_set_attr(g_avplay_es, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(g_avplay_es, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_win_attach_src(win_handle, g_avplay_es);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_attach_src failed.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_win_set_enable(win_handle, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETATCH;
    }

    ret = hi_adp_avplay_set_vdec_attr(g_avplay_es, vdec_type, HI_UNF_VDEC_WORK_MODE_NORMAL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_set_vdec_attr failed.\n");
        goto WIN_DETATCH;
    }

    ret = hi_unf_avplay_start(g_avplay_es, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_start failed.\n");
        goto WIN_DETATCH;
    }

    return ret;

WIN_DETATCH:
    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, g_avplay_es);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(g_avplay_es, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(g_avplay_es);

    return ret;
}

hi_s32 destroy_es_resource(hi_handle win_handle)
{
    hi_unf_avplay_stop_opt    stop;

    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(g_avplay_es, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop);

    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, g_avplay_es);

    hi_unf_avplay_chan_close(g_avplay_es, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    hi_unf_avplay_destroy(g_avplay_es);

    return HI_SUCCESS;
}

hi_bool ts_thread_pause()
{
    return g_thread_ctx.pause_ts_thread;
}

hi_bool es_thread_pause()
{
    return g_thread_ctx.pause_es_thread;
}

hi_s32 es_ts_switch_mode(hi_handle win_handle, play_type enplay_type, hi_unf_vcodec_type vdec_type)
{
    hi_s32   ret = HI_SUCCESS;

    if (enplay_type == PLAY_TYPE_ES) {
        //destroy ts resource
        if (HI_TRUE != ts_thread_pause()) {
            g_thread_ctx.pause_ts_thread = HI_TRUE;
            pthread_mutex_lock(&g_ts_mutex);
            rewind(g_ts_file);
            hi_unf_dmx_reset_ts_buffer(g_ts_buffer_handle);
            destroy_ts_resource(win_handle);
            pthread_mutex_unlock(&g_ts_mutex);

        }
        //create es resource
        ret = create_es_resource(win_handle, vdec_type);
        if (ret != HI_SUCCESS) {
            sample_printf("call create_es_resource failed.\n");
            return ret;
        }
        //run
        g_thread_ctx.pause_es_thread = HI_FALSE;
    }

    if (enplay_type == PLAY_TYPE_TS) {
        //destroy es resource
        if (HI_TRUE != es_thread_pause()) {
            g_thread_ctx.pause_es_thread = HI_TRUE;
            pthread_mutex_lock(&g_es_mutex);
            rewind(g_es_file);
            destroy_es_resource(win_handle);
            pthread_mutex_unlock(&g_es_mutex);
        }
        //create ts resource
        ret = create_ts_resource(win_handle);
        if (ret != HI_SUCCESS) {
            sample_printf("call create_ts_resource failed.\n");
            return ret;
        }

        //run
        g_thread_ctx.pause_ts_thread = HI_FALSE;
    }

    return ret;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                  ret;
    hi_handle               win_handle;
    hi_char                 input_cmd[32];
    hi_unf_video_format        vid_format = HI_UNF_VIDEO_FMT_1080P_50;
    hi_unf_vcodec_type    vdec_type = HI_UNF_VCODEC_TYPE_MAX;
    hi_u32                  ProgNum;

    if (argc == 5) {
        vid_format = hi_adp_str2fmt(argv[4]);
    }
    else if (argc == 4) {
        vid_format = HI_UNF_VIDEO_FMT_1080P_50;
    }
    else {
        printf("Usage: sample_es_ts_switch esvidfile vidtype tsfile [vo_format]\n"
              "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n");
        printf(" vtype: mpeg2/mpeg4/h263/sor/vp6/vp6f/vp6a/h264/avs/real8/real9/vc1\n");
        printf("Example:./sample_es_ts_switch esvidfile h264 tsfile 1080p50\n");

        return 0;
    }

    if (!strcasecmp("mpeg2", argv[2])) {
        vdec_type = HI_UNF_VCODEC_TYPE_MPEG2;
    }
    else if (!strcasecmp("mpeg4", argv[2])) {
        vdec_type = HI_UNF_VCODEC_TYPE_MPEG4;
    }
    else if (!strcasecmp("h263", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_H263;
    }
    else if (!strcasecmp("sor", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_SORENSON;
    }
    else if (!strcasecmp("vp6", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_VP6;
    }
    else if (!strcasecmp("vp6f", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_VP6F;
    }
    else if (!strcasecmp("vp6a", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_VP6A;
    }
    else if (!strcasecmp("vp8", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_VP8;
    }
    else if (!strcasecmp("h264", argv[2])) {
        vdec_type = HI_UNF_VCODEC_TYPE_H264;
    }
    else if (!strcasecmp("avs", argv[2])) {
        vdec_type = HI_UNF_VCODEC_TYPE_AVS;
    }
    else if (!strcasecmp("real8", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_REAL8;
    }
    else if (!strcasecmp("real9", argv[2])) {
        g_read_frame_size = HI_TRUE;
        vdec_type = HI_UNF_VCODEC_TYPE_REAL9;
    }
    else if (!strcasecmp("vc1", argv[2])) {
        vdec_type = HI_UNF_VCODEC_TYPE_VC1;
    }
    else {
        sample_printf("unsupport vid codec type!\n");
        return -1;
    }

    g_es_file = fopen(argv[1], "rb");
    if (!g_es_file) {
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    g_ts_file = fopen(argv[3], "rb");
    if (!g_ts_file) {
        fclose(g_es_file);
        g_es_file = HI_NULL;
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    ret = create_common_resource(vid_format, &win_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call create_common_resource failed.\n");
        goto FILE_CLOSE;
    }

    pthread_mutex_init(&g_ts_mutex, NULL);
    pthread_mutex_init(&g_es_mutex, NULL);

    g_thread_ctx.stop_ts_thread = HI_FALSE;
    g_thread_ctx.pause_ts_thread = HI_TRUE;
    g_thread_ctx.stop_es_thread = HI_FALSE;
    g_thread_ctx.pause_es_thread = HI_TRUE;

    pthread_create(&g_ts_thread, HI_NULL, (hi_void *)ts_thread, HI_NULL);
    pthread_create(&g_es_thread, HI_NULL, (hi_void *)es_thread, HI_NULL);

    printf("please input e to play esvidfile, t to play tsfile\n");

    while (1) {
        printf("please input the q to quit!, e to play esvidfile, t to play tsfile\n");

#ifndef AUTOSWITCH
        SAMPLE_GET_INPUTCMD(input_cmd);
#endif
        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        if (input_cmd[0] == 'e') {
            if (HI_TRUE != es_thread_pause()) {
                printf("playing esvidfile!\n");
            }
            else {
                printf("prepare to play esvidfile!\n");
                ret = es_ts_switch_mode(win_handle, PLAY_TYPE_ES, vdec_type);
                if (ret != HI_SUCCESS) {
                    sample_printf("call es_ts_switch_mode ES failed.\n");
                    goto DestroyAllRes;
                }
            }
        }
        else if (input_cmd[0] == 't') {
            if (HI_TRUE != ts_thread_pause()) {
                printf("playing tsvidfile!\n");
            }
            else {
                printf("prepare to play tsfile!\n");
                ret = es_ts_switch_mode(win_handle, PLAY_TYPE_TS, vdec_type);
                if (ret != HI_SUCCESS) {
                    sample_printf("call es_ts_switch_mode TS failed.\n");
                    goto DestroyAllRes;
                }

                hi_adp_search_init();
                ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_table);
                if (ret != HI_SUCCESS) {
                    sample_printf("call hi_adp_search_get_all_pmt failed.\n");
                    hi_adp_search_de_init();
                    goto DestroyAllRes;
                }

                ProgNum = 0;

                pthread_mutex_lock(&g_ts_mutex);
                rewind(g_ts_file);
                hi_unf_dmx_reset_ts_buffer(g_ts_buffer_handle);
                ret = hi_adp_avplay_play_prog(g_avplay_ts,g_prog_table, ProgNum, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
                if (ret != HI_SUCCESS) {
                    sample_printf("call SwitchProg failed.\n");
                    pthread_mutex_unlock(&g_ts_mutex);
                    goto DestroyAllRes;;
                }

                pthread_mutex_unlock(&g_ts_mutex);
                continue;
            }
        }

#ifdef AUTOSWITCH
        sleep(1);
        if (input_cmd[0] == 'e') {
            input_cmd[0] = 't';
        }
        else {
            input_cmd[0] = 'e';
        }
#endif
        if (HI_TRUE != ts_thread_pause()) {
            static hi_u32 u32TplaySpeed = 2;
            if (input_cmd[0] == 's') {
                hi_unf_avplay_tplay_opt stTplayOpt;
                printf("%dx tplay\n",u32TplaySpeed);

                stTplayOpt.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
                stTplayOpt.speed_integer = u32TplaySpeed;
                stTplayOpt.speed_decimal = 0;

                hi_unf_avplay_set_decode_mode(g_avplay_ts, HI_UNF_VDEC_WORK_MODE_I);
                hi_unf_avplay_tplay(g_avplay_ts, &stTplayOpt);
                u32TplaySpeed = (32 == u32TplaySpeed * 2) ? (2) : (u32TplaySpeed * 2);
                continue;
            }

            if (input_cmd[0] == 'r') {
                printf("resume\n");
                hi_unf_avplay_set_decode_mode(g_avplay_ts, HI_UNF_VDEC_WORK_MODE_NORMAL);
                hi_unf_avplay_resume(g_avplay_ts, HI_NULL);
                u32TplaySpeed = 2;
                continue;
            }

            ProgNum = atoi(input_cmd);
            if (ProgNum == 0)
                ProgNum = 1;

            pthread_mutex_lock(&g_ts_mutex);
            rewind(g_ts_file);
            hi_unf_dmx_reset_ts_buffer(g_ts_buffer_handle);
            ret = hi_adp_avplay_play_prog(g_avplay_ts, g_prog_table, ProgNum - 1, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
            if (ret != HI_SUCCESS) {
                sample_printf("call SwitchProgfailed!\n");
                pthread_mutex_unlock(&g_ts_mutex);
                break;
            }

            pthread_mutex_unlock(&g_ts_mutex);
        }
    }

DestroyAllRes:
    g_thread_ctx.stop_es_thread = HI_TRUE;
    g_thread_ctx.stop_ts_thread = HI_TRUE;
    pthread_join(g_es_thread, HI_NULL);
    pthread_join(g_ts_thread, HI_NULL);
    pthread_mutex_destroy(&g_es_mutex);
    pthread_mutex_destroy(&g_ts_mutex);

    if (g_thread_ctx.pause_es_thread != HI_TRUE) {
        destroy_es_resource(win_handle);
    }

    if (g_thread_ctx.pause_ts_thread != HI_TRUE) {
        destroy_ts_resource(win_handle);
    }

    destroy_common_resource(win_handle);

FILE_CLOSE:
    fclose(g_es_file);
    g_es_file = HI_NULL;

    fclose(g_ts_file);
    g_ts_file = HI_NULL;

    return ret;
}

