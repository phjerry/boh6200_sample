/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : ts_transcode.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   :
  History       :
  1.Date        : 2010/01/26
    Author      : sdk
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

#include "hi_errno.h"
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
#include "hi_unf_acodec_aacenc.h"
#include "hi_unf_aenc.h"
#include "hi_unf_venc.h"

#define AENC_SUPPORT

FILE               *g_ts_file = HI_NULL;
FILE               *g_ts_play_file = HI_NULL;
static pthread_t   g_ts_thd;
static pthread_mutex_t g_ts_mutex;
static hi_bool     g_stop_ts_thread = HI_FALSE;
hi_handle          g_ts_buffer;
pmt_compact_tbl    *g_prog_table = HI_NULL;

static pthread_t g_ts_play_thd;
static pthread_mutex_t g_ts_play_mtx;
static hi_handle g_ts_play_buf = HI_FALSE;
static hi_bool g_ts_play_stop = HI_FALSE;

#define  PLAY_DMX_ID     0
#define  PLAY_DMX_ID_1   1
#define  VENC_GOP_SIZE   100
#define  VENC_MAX_WIDTH  3840
#define  VENC_MAX_HEIGHT 2160
#define  VENC_INPUT_FRAME_RATE  (50 * 1000)
#define  VENC_TARGET_FRAME_RATE (50 * 1000)

#define TS_READ_SIZE    (188 * 1000)

hi_void TsTthread(hi_void *args)
{
    hi_unf_stream_buf     StreamBuf;
    hi_u32                Readlen;
    hi_s32                ret;


    while (!g_stop_ts_thread)
    {
        pthread_mutex_lock(&g_ts_mutex);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buffer, TS_READ_SIZE, &StreamBuf, 1000);
        if (ret != HI_SUCCESS)
        {
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        Readlen = fread(StreamBuf.data, sizeof(hi_s8), TS_READ_SIZE, g_ts_file);
        if(Readlen <= 0)
        {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_buffer, &StreamBuf, 0, Readlen);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_ts_mutex);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buffer);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}

hi_void ts_play_thread(hi_void *args)
{
    hi_unf_stream_buf     StreamBuf;
    hi_u32                Readlen;
    hi_s32                ret;

    while (!g_ts_play_stop)
    {
        pthread_mutex_lock(&g_ts_play_mtx);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_play_buf, TS_READ_SIZE, &StreamBuf, 1000);
        if (ret != HI_SUCCESS)
        {
            pthread_mutex_unlock(&g_ts_play_mtx);
            continue;
        }

        Readlen = fread(StreamBuf.data, sizeof(hi_s8), TS_READ_SIZE, g_ts_play_file);
        if(Readlen <= 0)
        {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_play_file);
            pthread_mutex_unlock(&g_ts_play_mtx);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_play_buf, &StreamBuf, 0, Readlen);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_play_mtx);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_play_buf);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_reset_ts_buffer in ts_play_thread failed.\n");
    }

    return;
}


volatile hi_bool g_stop_save_thread = HI_TRUE;
volatile hi_bool g_save_stream = HI_FALSE;

#define VID_FILENAME_PATH  "./vid.h264"
#define AUD_FILENAME_PATH  "./aud.aac"
#define VID_FILE_MAX_LEN   (20*1024*1024)
//#define FILE_LEN_LIMIT

typedef struct
{
    hi_handle venc;
    hi_handle aenc;
}thread_args;

hi_void* SaveThread(hi_void *pArgs)
{
    thread_args *thread_para = (thread_args*)pArgs;
    hi_handle venc_chn;
    FILE *pVidFile = HI_NULL;
    FILE *pAudFile = HI_NULL;
    hi_s32 ret;

    venc_chn = thread_para->venc;
    pVidFile = fopen(VID_FILENAME_PATH,"w");
    if ( HI_NULL == pVidFile )
    {
        printf("open file %s failed\n",VID_FILENAME_PATH);
        return HI_NULL;
    }

    pAudFile = fopen(AUD_FILENAME_PATH,"w");
    if ( HI_NULL == pVidFile )
    {
        printf("open file %s failed\n",AUD_FILENAME_PATH);
        fclose(pVidFile);
        return HI_NULL;
    }

    while (!g_stop_save_thread)
    {
        hi_bool bGotStream = HI_FALSE;

        if (g_save_stream)
        {
            hi_unf_venc_stream venc_stream;
#ifdef FILE_LEN_LIMIT
            if (ftell(pVidFile) >= VID_FILE_MAX_LEN)
            {
                fclose(pVidFile);
                fclose(pAudFile);
                pVidFile = fopen(VID_FILENAME_PATH,"w");
                pAudFile = fopen(AUD_FILENAME_PATH,"w");
                printf("stream files are truncated to zero\n");
            }
#endif

            /*save video stream*/
            ret = hi_unf_venc_acquire_stream(venc_chn, &venc_stream, 0);
            if (ret == HI_SUCCESS)
            {
                fwrite(venc_stream.virt_addr, 1, venc_stream.slc_len, pVidFile);
                hi_unf_venc_release_stream(venc_chn, &venc_stream);
                fflush(pVidFile);
                bGotStream = HI_TRUE;
            }
            else if (ret != HI_ERR_VENC_BUF_EMPTY)
            {
                printf("hi_unf_venc_acquire_stream failed:%#x\n", ret);
            }

#ifdef AENC_SUPPORT
            hi_unf_es_buf aenc_stream;
            hi_handle     aenc_chn;

            aenc_chn = thread_para->aenc;
            /*save audio stream*/
            ret = hi_unf_aenc_acquire_stream(aenc_chn, &aenc_stream, 0);
            if (ret == HI_SUCCESS)
            {
                fwrite(aenc_stream.buf, 1, aenc_stream.buf_len, pAudFile);
                fflush(pAudFile);
                hi_unf_aenc_release_stream(aenc_chn, &aenc_stream);
                bGotStream = HI_TRUE;
            }
            else if (ret != HI_ERR_AENC_OUT_BUF_EMPTY)
            {
                printf("hi_unf_aenc_acquire_stream failed:%#x\n", ret);
            }
#endif
        }

        if (bGotStream == HI_FALSE)
        {
            usleep(10 * 1000);
        }
    }

    fclose(pVidFile);
    fclose(pAudFile);
    return HI_NULL;
}

typedef struct {
    hi_handle window;
    hi_handle venc;
    hi_bool   pause;
} transcode_thread_ctx;

static hi_bool g_stop_transcode_thread = HI_FALSE;
static void *transcode_thread(void *arg)
{
    hi_s32 ret;
    transcode_thread_ctx *thread_ctx = arg;
    hi_unf_video_frame_info a_frame;
    hi_unf_video_frame_info d_frame;

    hi_bool acquired = HI_FALSE;
    hi_bool dequeued = HI_FALSE;
    hi_bool wait = HI_FALSE;

    while (!g_stop_transcode_thread) {
        if (wait) {
            usleep(10 * 1000);
        }

        wait = HI_TRUE;
        if (thread_ctx->pause) {
            continue;
        }

        /* WINDOW->VENC */
        if (!acquired) {
            ret = hi_unf_win_acquire_frame(thread_ctx->window, &a_frame, 0);
            if (ret == HI_SUCCESS) {
                acquired = HI_TRUE;
            }
        }

        if (acquired) {
            ret = hi_unf_venc_queue_frame(thread_ctx->venc, &a_frame);
            if (ret == HI_SUCCESS) {
                acquired = HI_FALSE;
                wait = HI_FALSE;
            }
        }

        /* VENC->WINDOW */
        if (!dequeued) {
            ret = hi_unf_venc_dequeue_frame(thread_ctx->venc, &d_frame);
            if (ret == HI_SUCCESS) {
                dequeued = HI_TRUE;
            }
        }

        if (dequeued) {
            ret = hi_unf_win_release_frame(thread_ctx->window, &d_frame);
            if (ret == HI_SUCCESS) {
                dequeued = HI_FALSE;
                wait = HI_FALSE;
            }
        }
    }

    return HI_NULL;
}

hi_s32 avplay_play(hi_handle avplay, pmt_compact_tbl *pProgTbl, hi_u32 prog_num, hi_bool is_aud_play)
{
    hi_unf_avplay_stop_opt  stop_opt;
    hi_u32                  vid_pid;
    hi_unf_vcodec_type      vid_type;
    hi_s32                  ret;
    hi_u32                  aud_pid;
    hi_u32                  aud_type;

    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_STILL;
    stop_opt.timeout = 0;

    ret = hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_stop failed.\n");
        return ret;
    }

    prog_num = prog_num % pProgTbl->prog_num;
    if (pProgTbl->proginfo[prog_num].v_element_num > 0)
    {
        vid_pid = pProgTbl->proginfo[prog_num].v_element_pid;
        vid_type = pProgTbl->proginfo[prog_num].video_type;
    }
    else
    {
        vid_pid = INVALID_TSPID;
        vid_type = HI_UNF_VCODEC_TYPE_MAX;
    }

    if (vid_pid != INVALID_TSPID)
    {
        ret = hi_adp_avplay_set_vdec_attr(avplay, vid_type, HI_UNF_VDEC_WORK_MODE_NORMAL);
        ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VID_PID, &vid_pid);
        if (ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call hi_adp_avplay_set_vdec_attr failed.\n");
            return ret;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call hi_unf_avplay_start failed.\n");
            return ret;
        }
    }

    if (pProgTbl->proginfo[prog_num].a_element_num > 0)
    {
        aud_pid  = pProgTbl->proginfo[prog_num].a_element_pid;
        aud_type = pProgTbl->proginfo[prog_num].audio_type;
    }
    else
    {
        aud_pid = INVALID_TSPID;
        aud_type = 0xffffffff;
    }

    if (HI_TRUE == is_aud_play && aud_pid != INVALID_TSPID)
    {
        ret  = hi_adp_avplay_set_adec_attr(avplay, aud_type);

        ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &aud_pid);
        if (HI_SUCCESS != ret)
        {
            SAMPLE_COMMON_PRINTF("hi_adp_avplay_set_adec_attr failed:%#x\n", ret);
            return ret;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_start to start audio failed.\n");
            //return ret;
        }
    }

    return HI_SUCCESS;
}

static hi_s32 ts_play_prepare(hi_handle *win_handle, hi_handle *track_handle, hi_handle *avplay_handle)
{
    hi_s32 ret;
    hi_handle                 avplay, win, track;
    hi_unf_avplay_attr        avplay_attr;
    hi_unf_audio_track_attr   track_attr;
    hi_unf_sync_attr          sync_attr;

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        return HI_FAILURE;
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    tsbuf_attr.buffer_size = 0x1000000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_play_buf);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DETACH;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DMX_ID;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_create failed.\n");
        goto TSBUF_FREE;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

    ret |= hi_adp_win_create(HI_NULL, &win);
    if (HI_SUCCESS != ret)
    {
        printf("call VoInit failed.\n");
        hi_adp_win_deinit();
        goto ACHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win, avplay);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
    }
    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay);
    if (ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_set_attr failed.\n");
        goto TRACK_DETACH;
    }

    *win_handle = win;
    *track_handle = track;
    *avplay_handle = avplay;

    return HI_SUCCESS;

TRACK_DETACH:
    hi_unf_snd_detach(track, avplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);

WIN_DETACH:
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);
    hi_unf_win_destroy(win);

ACHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay);

TSBUF_FREE:
    hi_unf_dmx_destroy_ts_buffer(g_ts_play_buf);

DMX_DETACH:
    hi_unf_dmx_detach_ts_port(PLAY_DMX_ID_1);

    return HI_FAILURE;

}

static hi_s32 ts_play_exit(hi_handle *win, hi_handle *track, hi_handle *avplay)
{
    hi_unf_snd_detach(*track, *avplay);
    hi_unf_snd_destroy_track(*track);

    hi_unf_win_set_enable(*win, HI_FALSE);
    hi_unf_win_detach_src(*win, *avplay);
    hi_unf_win_destroy(*win);

    hi_unf_avplay_chan_close(*avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_destroy(*avplay);

    hi_unf_dmx_destroy_ts_buffer(g_ts_play_buf);
    hi_unf_dmx_detach_ts_port(PLAY_DMX_ID);

    return HI_SUCCESS;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32             ret;
    hi_handle          enc_win;
    hi_handle          venc;
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    hi_handle               win;
#endif
    hi_handle               avplay = HI_INVALID_HANDLE;
    hi_unf_avplay_stop_opt  stop;
    hi_char                 input_cmd[32];
    hi_unf_video_format     enFormat = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32                  prog_num;
    thread_args             save_args;
    pthread_t               thdSave;
    hi_unf_venc_attr        venc_chan_attr;
    hi_unf_sync_attr        sync_attr;

#ifdef AENC_SUPPORT
    hi_handle                  enc_track;
    hi_handle                  aenc;
    hi_unf_audio_track_attr    track_attr;
    hi_unf_aenc_attr           aenc_attr;
    hi_unf_acodec_aac_enc_config     private_config;
#endif
    hi_u32 width;
    hi_u32 height;

    if (argc < 2)
    {
        printf("Usage: %s file [width] [height] [fmt]\n", argv[0]);
        printf("Example: %s ./test.ts 1280 720 1080P60\n", argv[0]);
        printf("       : %s ./test.ts 1080P60\n", argv[0]);
        printf("Note: if not set width and height, set them to default value: 1280 720");
        printf("Note: if not set format, set it to default value: 1080P50");
        return 0;
    }

    g_ts_file = fopen(argv[1], "rb");
    if (!g_ts_file)
    {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    g_ts_play_file = fopen(argv[1], "rb");
    if (!g_ts_play_file)
    {
        fclose(g_ts_file);
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    if (argv[2] != NULL && argv[3] != NULL) {
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    } else {
        width = 1280;
        height = 720;
    }

    if (argc >= 5) {
        enFormat = hi_adp_str2fmt(argv[4]);
    } else if (argc ==3) {
        enFormat = hi_adp_str2fmt(argv[2]);
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (HI_SUCCESS != ret)
    {
        printf("call SndInit failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(enFormat);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();

#ifdef PLAY_AND_TRANSCODE_SUPPORT
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (HI_SUCCESS != ret)
    {
        printf("call VoInit failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }
#endif

    ret = hi_unf_dmx_init();
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_dmx_init failed.\n");
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID_1, HI_UNF_DMX_PORT_RAM_1);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto VO_DEINIT;
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    tsbuf_attr.buffer_size = 0x1000000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_1, &tsbuf_attr, &g_ts_buffer);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_avplay_init failed.\n");
        goto TSBUF_FREE;
    }

    hi_unf_avplay_attr        avplay_attr;
    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DMX_ID_1;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

#ifdef PLAY_AND_TRANSCODE_SUPPORT
    ret = hi_unf_win_attach_src(win, avplay);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
    }
    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }
#endif

#ifdef PLAY_AND_TRANSCODE_SUPPORT
#ifdef AENC_SUPPORT
    hi_handle                   track;

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay);
    if (ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }
#endif
#endif

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_set_attr failed.\n");
        goto WIN_DETACH;
    }

    /*create virtual window */
    hi_unf_win_attr virtul_win_attr = {0};

    virtul_win_attr.disp_id = HI_UNF_DISPLAY0;
    virtul_win_attr.priority = HI_UNF_WIN_WIN_PRIORITY_AUTO;
    virtul_win_attr.is_virtual = HI_TRUE;
    virtul_win_attr.asp_convert_mode = HI_UNF_WIN_ASPECT_CONVERT_FULL;
    virtul_win_attr.video_format = HI_UNF_FORMAT_YUV_SEMIPLANAR_420_VU;

    virtul_win_attr.output_rect.height = height;
    virtul_win_attr.output_rect.width = width;
    hi_unf_win_create(&virtul_win_attr, &enc_win);

    ret = hi_unf_win_attach_src(enc_win, avplay);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_win_attach_src err:0x%x\n", ret);
    }

    hi_unf_win_set_enable(enc_win, HI_TRUE);

    /*create video encoder*/
    hi_unf_venc_init();
    hi_unf_venc_get_default_attr(&venc_chan_attr);
    venc_chan_attr.gop_mode = HI_UNF_VENC_GOP_MODE_NORMALP;
    venc_chan_attr.config.gop = VENC_GOP_SIZE;
    venc_chan_attr.config.sp_interval = 0;
    venc_chan_attr.venc_type = HI_UNF_VCODEC_TYPE_H264;
    venc_chan_attr.max_width = VENC_MAX_WIDTH;
    venc_chan_attr.max_height = VENC_MAX_HEIGHT;
    venc_chan_attr.config.width = width;
    venc_chan_attr.config.height = height;
    venc_chan_attr.stream_buf_size   = width * height * 2;
    venc_chan_attr.config.target_bitrate = 5 * 1024 * 1024;
    /*?????*/
    venc_chan_attr.config.input_frame_rate  = VENC_INPUT_FRAME_RATE;
    venc_chan_attr.config.target_frame_rate = VENC_TARGET_FRAME_RATE;
    hi_unf_venc_create(&venc, &venc_chan_attr);
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    hi_unf_venc_attach_input(venc, enc_win);
#endif

#ifdef AENC_SUPPORT
    /*create virtual track */
    hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &track_attr);
    track_attr.output_buf_size = 256 * 1024;
    hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &enc_track);

    /*create aenc*/
    hi_unf_aenc_init();
    hi_unf_aenc_register_encoder("libHA.AUDIO.AAC.encode.so");

    /*create audio decoder*/
    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AAC;
    hi_unf_acodec_aac_get_default_config(&private_config);
    hi_unf_acodec_aac_get_enc_default_open_param(&(aenc_attr.param), (hi_void *)&private_config);
    hi_unf_aenc_create(&aenc_attr, &aenc);
    /*attach audio decoder and virtual track*/
    hi_unf_aenc_attach_input(aenc, enc_track);
#endif

    hi_handle av_play, win_play, track_play;

    ret = ts_play_prepare(&win_play, &track_play, &av_play);
    if (ret != HI_SUCCESS) {
        printf("call ts_play_prepare failed: 0x%x.\n", ret);
        goto WIN_DETACH;
    }

    g_stop_save_thread = HI_FALSE;
    save_args.venc = venc;
#ifdef AENC_SUPPORT
    save_args.aenc = aenc;
#endif
    pthread_create(&thdSave, HI_NULL, (hi_void *)SaveThread, &save_args);

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_thread = HI_FALSE;
    pthread_create(&g_ts_thd, HI_NULL, (hi_void *)TsTthread, HI_NULL);

    pthread_mutex_init(&g_ts_play_mtx, NULL);
    g_ts_play_stop = HI_FALSE;
    pthread_create(&g_ts_play_thd, HI_NULL, (hi_void *)ts_play_thread, HI_NULL);

    g_stop_transcode_thread = HI_FALSE;
    pthread_t trans_thread_id;
    transcode_thread_ctx trans_thread_arg;
    trans_thread_arg.window = enc_win;
    trans_thread_arg.venc = venc;
    trans_thread_arg.pause = HI_TRUE;
    pthread_create(&trans_thread_id, HI_NULL, (hi_void *)transcode_thread, &trans_thread_arg);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_table);
    if (ret != HI_SUCCESS)
    {
        printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    prog_num = 0;

    pthread_mutex_lock(&g_ts_mutex);
    rewind(g_ts_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buffer);

    ret = avplay_play(avplay, g_prog_table, prog_num, HI_FALSE);
    if (ret != HI_SUCCESS)
    {
        printf("call avplay_play failed.\n");
        pthread_mutex_unlock(&g_ts_mutex);
        goto AVPLAY_STOP;
    }

    pthread_mutex_unlock(&g_ts_mutex);


    pthread_mutex_lock(&g_ts_play_mtx);
    rewind(g_ts_play_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_play_buf);
    ret = avplay_play(av_play, g_prog_table, prog_num, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        printf("call play failed.\n");
        pthread_mutex_unlock(&g_ts_play_mtx);
        goto AVPLAY_STOP;
    }
    pthread_mutex_unlock(&g_ts_play_mtx);

    printf("please input q to quit\n" \
           "       input r to start transcode\n" \
           "       input s to stop transcode\n" \
           "       input 0~9 to change channel\n"\
           "       input h to print this message\n");
    while (1)
    {
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        if ('h' == input_cmd[0])
        {
            printf("please input q to quit\n" \
                   "       input r to start transcode\n" \
                   "       input s to stop transcode\n" \
                   "       input 0~9 to change channel\n"\
                   "       input h to print this message\n");
            continue;
        }

        if ('r' == input_cmd[0])
        {
            hi_unf_venc_start(venc);
#ifdef AENC_SUPPORT
            hi_unf_snd_attach(enc_track, av_play);
#endif
            trans_thread_arg.pause = HI_FALSE;
            g_save_stream = HI_TRUE;
            printf("start transcode\n");
            continue;
        }

        if ('s' == input_cmd[0])
        {
            trans_thread_arg.pause = HI_TRUE;
            hi_unf_venc_stop(venc);
#ifdef AENC_SUPPORT
            hi_unf_snd_detach(enc_track, av_play);
#endif
            g_save_stream = HI_FALSE;
            printf("stop transcode\n");
            continue;
        }

        if (g_save_stream == HI_TRUE)
        {
            g_save_stream = HI_FALSE;
            trans_thread_arg.pause = HI_TRUE;
            hi_unf_win_set_enable(enc_win, HI_FALSE);
            hi_unf_venc_stop(venc);
#ifdef AENC_SUPPORT
            hi_unf_snd_detach(enc_track, av_play);
#endif
            hi_unf_win_detach_src(enc_win, avplay);
            printf("stop transcode\n");
        }

        prog_num = atoi(input_cmd);

        if (prog_num == 0)
        {
            prog_num = 1;
        }

        pthread_mutex_lock(&g_ts_mutex);
        rewind(g_ts_file);
        hi_unf_dmx_reset_ts_buffer(g_ts_buffer);

        ret = avplay_play(avplay, g_prog_table, prog_num - 1, HI_FALSE);
        if (ret != HI_SUCCESS)
        {
            printf("call SwitchProgfailed!\n");
            pthread_mutex_unlock(&g_ts_mutex);
            break;
        }

        pthread_mutex_unlock(&g_ts_mutex);

        pthread_mutex_lock(&g_ts_play_mtx);
        rewind(g_ts_play_file);
        hi_unf_dmx_reset_ts_buffer(g_ts_play_buf);
        ret = avplay_play(av_play, g_prog_table, prog_num - 1, HI_TRUE);
        if (ret != HI_SUCCESS)
        {
            printf("call play failed.\n");
            pthread_mutex_unlock(&g_ts_play_mtx);
            goto AVPLAY_STOP;
        }
        pthread_mutex_unlock(&g_ts_play_mtx);
    }

AVPLAY_STOP:
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

    hi_unf_avplay_stop(av_play, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_table);
    g_prog_table = HI_NULL;
    hi_adp_search_de_init();

    g_stop_transcode_thread = HI_TRUE;
    pthread_join(trans_thread_id, HI_NULL);

    g_stop_ts_thread = HI_TRUE;
    pthread_join(g_ts_thd, HI_NULL);
    pthread_mutex_destroy(&g_ts_mutex);

    g_ts_play_stop = HI_TRUE;
    pthread_join(g_ts_play_thd, HI_NULL);
    pthread_mutex_destroy(&g_ts_play_mtx);


//ENC_DESTROY:
    g_stop_save_thread = HI_TRUE;
    pthread_join(thdSave,HI_NULL);

#ifdef AENC_SUPPORT
    if (g_save_stream)
    {
        hi_unf_snd_detach(enc_track, av_play);
    }

    hi_unf_aenc_detach_input(aenc);
    hi_unf_aenc_destroy(aenc);
    hi_unf_snd_destroy_track(enc_track);
#endif

    hi_unf_win_set_enable(enc_win, HI_FALSE);

    if (g_save_stream)
    {
        printf("stop venc.");
        hi_unf_venc_stop(venc);
    }

    hi_unf_win_detach_src(enc_win, avplay);
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    hi_unf_venc_detach_input(venc);
#endif
    hi_unf_venc_destroy(venc);
    hi_unf_win_destroy(enc_win);

    (void)ts_play_exit(&win_play, &track_play, &av_play);

//SND_DETACH:
    //hi_unf_snd_detach(track, avplay);

#ifdef PLAY_AND_TRANSCODE_SUPPORT
#ifdef AENC_SUPPORT
TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);
#endif
#endif

WIN_DETACH:
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);
#endif

    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);


AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay);


AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

TSBUF_FREE:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buffer);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(1);
    hi_unf_dmx_deinit();

VO_DEINIT:
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    hi_unf_win_destroy(win);
#endif
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    fclose(g_ts_file);
    g_ts_file = HI_NULL;
    fclose(g_ts_play_file);
    g_ts_play_file = HI_NULL;

    return ret;
}


