/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : sample_ts_capture.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/07/28
  Description   :
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

// #include "hi_unf_ecs.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"

#include "hi_adp.h"
#include "hi_adp_mpi.h"
#include "hi_adp_search.h"
#include "hi_adp_frontend.h"

#if CONFING_SUPPORT_AUDIO
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "HA.AUDIO.DRA.decode.h"
#include "hi_unf_acodec_pcmdec.h"
#include "HA.AUDIO.WMA9STD.decode.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_aacenc.h"
#endif

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

FILE *g_is_file = HI_NULL;
static pthread_t g_is_thd;
static pthread_mutex_t g_is_mutex;
static hi_bool g_stop_is_thread = HI_FALSE;
hi_handle g_is_buf;
pmt_compact_tbl *g_prog_tbl = HI_NULL;

#define PLAY_DMX_ID 0

hi_void is_tthread(hi_void *args)
{
    hi_unf_stream_buf stream_buf;
    hi_u32 read_len;
    hi_s32 ret;

    while (!g_stop_is_thread) {
        pthread_mutex_lock(&g_is_mutex);
        ret = hi_unf_dmx_get_ts_buffer(g_is_buf, 188 * 50, &stream_buf, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_is_mutex);
            continue;
        }

        read_len = fread(stream_buf.data, sizeof(hi_s8), 188 * 50, g_is_file);
        if (read_len <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(g_is_file);
            pthread_mutex_unlock(&g_is_mutex);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_is_buf, &stream_buf, 0, read_len);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_is_mutex);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_is_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}

#if CONFIG_SUPPORT_HIGO

hi_s32 HigoCreateSurface(hi_u32 Width, hi_u32 Height, hi_handle *Layer, hi_handle *Surface)
{
    hi_s32              ret;
    HIGO_LAYER_INFO_S   LayerInfo;

    if ((Layer == HI_NULL) || (Surface == HI_NULL)) {
        return HI_FAILURE;
    }

    *Layer      = HI_NULL;
    *Surface    = HI_NULL;

    ret = HI_GO_GetLayerDefaultParam(HIGO_LAYER_HD_0, &LayerInfo);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_GetLayerDefaultParam failed 0x%x\n", __FUNCTION__, ret);

        goto exit;
    }

    LayerInfo.CanvasWidth   = Width;
    LayerInfo.CanvasHeight  = Height;
    LayerInfo.DisplayWidth  = Width;
    LayerInfo.DisplayHeight = Height;

    ret = HI_GO_CreateLayer(&LayerInfo, Layer);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_CreateLayer failed 0x%x\n", __FUNCTION__, ret);

        goto exit;
    }

    ret = HI_GO_GetLayerSurface(*Layer, Surface);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_GetLayerSurface failed 0x%x\n", __FUNCTION__, ret);

        goto exit;
    }

    ret = HI_GO_SetLayerAlpha(*Layer, 100);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_SetLayerAlpha failed 0x%x\n", __FUNCTION__, ret);

        goto exit;
    }

    return HI_SUCCESS;

exit:
    if (*Layer) {
        HI_GO_DestroyLayer(*Layer);
        *Layer = HI_NULL;
    }

    return HI_FAILURE;
}

hi_void HigoDestroySurface(hi_handle Layer)
{
    hi_s32 ret;

    ret = HI_GO_DestroyLayer(Layer);
    if (HI_SUCCESS != ret) {
        sample_printf("[%s] HI_GO_DestroyLayer failed 0x%x\n", __FUNCTION__, ret);
    }
}
#endif

hi_void vid_frame_to_picture(hi_handle WinHandle, hi_char *FileName)
{
    hi_s32 ret;
    hi_unf_video_frame_info vid_frame;
#if CONFIG_SUPPORT_HIGO
    hi_handle                   MemSurface;
    HIGO_SURINFO_S              MemSurfaceInfo = {0};
    HIGO_ENC_ATTR_S             EncAttr;
#endif
    if (!FileName) {
        sample_printf("[%s] filename error\n", __FUNCTION__);
        return;
    }
#if CONFIG_SUPPORT_HIGO
    switch (ImgType) {
        case HIGO_IMGTYPE_JPEG :
        case HIGO_IMGTYPE_BMP :
            break;

        default :
            sample_printf("[%s] not support format 0x%x\n", __FUNCTION__, ImgType);
            return;
    }
#endif

    ret = hi_unf_win_capture_picture(WinHandle, &vid_frame);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_UNF_VO_CapturePicture failed 0x%x\n", __FUNCTION__, ret);

        return;
    }
#if CONFIG_SUPPORT_HIGO
    MemSurfaceInfo.Width        = vid_frame.u32Width;
    MemSurfaceInfo.Height       = vid_frame.u32Height;
    MemSurfaceInfo.PixelFormat  = HIGO_PF_BUTT;

    switch (vid_frame.enVideoFormat) {
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_422 :
            MemSurfaceInfo.PixelFormat = HIGO_PF_YUV422;
            break;

        case HI_UNF_FORMAT_YUV_SEMIPLANAR_420 :
            MemSurfaceInfo.PixelFormat = HIGO_PF_YUV420;
            break;

        case HI_UNF_FORMAT_YUV_SEMIPLANAR_400 :
            MemSurfaceInfo.PixelFormat = HIGO_PF_YUV400;
            break;

        default :
            sample_printf("[%s] not support format %d\n", __FUNCTION__, vid_frame.enVideoFormat);
    }



    if (MemSurfaceInfo.PixelFormat != HIGO_PF_BUTT) {

        MemSurfaceInfo.Pitch[0]     = vid_frame.stVideoFrameAddr[0].u32YStride;
        MemSurfaceInfo.Pitch[1]     = vid_frame.stVideoFrameAddr[0].u32CStride;
        MemSurfaceInfo.pPhyAddr[0]  = (hi_char *)HI_NULL + vid_frame.stVideoFrameAddr[0].u32YAddr;
        MemSurfaceInfo.pPhyAddr[1]  = (hi_char *)HI_NULL + vid_frame.stVideoFrameAddr[0].u32CAddr;
        MemSurfaceInfo.pVirAddr[0]  = HI_NULL;
        MemSurfaceInfo.pVirAddr[1]  = HI_NULL;
        MemSurfaceInfo.MemType      = HIGO_MEMTYPE_MMZ;
        MemSurfaceInfo.bPubPalette  = HI_FALSE;

        ret = HI_GO_CreateSurfaceFromMem(&MemSurfaceInfo, &MemSurface);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] HI_GO_CreateSurface failed 0x%x\n", __FUNCTION__, ret);
        }

        EncAttr.ExpectType      = ImgType;
        EncAttr.QualityLevel    = 99;

        ret = HI_GO_EncodeToFile(MemSurface, FileName, &EncAttr);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] HI_GO_EncodeToFile failed 0x%x\n", __FUNCTION__, ret);
        }
        else {
            sample_printf("\ncapture to %s\n\n", FileName);
        }

        ret = HI_GO_FreeSurface(MemSurface);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] HI_GO_FreeSurface failed 0x%x\n", __FUNCTION__, ret);
        }
    }
#endif

    ret = hi_unf_win_capture_picture_release(WinHandle, &vid_frame);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_UNF_VO_CapturePictureRelease failed 0x%x\n", __FUNCTION__, ret);
    }
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle win;
    hi_handle avplay;
    hi_unf_avplay_attr avplay_attr;
    hi_unf_sync_attr sync_attr;
    hi_unf_avplay_stop_opt stop;
    hi_char input_cmd[32];
    hi_unf_video_format format = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32 prog_num;
    hi_handle track;
    hi_unf_audio_track_attr track_attr;
    hi_char file_name[MAX_FILENAME_LEN];
    hi_u32 count = 1;
    hi_u32 vir_width = 1280;
    hi_u32 vir_height = 720;

    if (argc < 2) {
        sample_printf("Usage: %s file [vo_format] [enc_format]\n", argv[0]);
        sample_printf("\tvo_format: 2160P30|2160P24|1080P60|1080P50|1080i60|1080i50|720P60|720P50\n");
        sample_printf("\tenc_format: bmp | jpg\n");
        sample_printf("Example:\n\t%s ./test.ts 1080p50\n", argv[0]);

        return 0;
    }

    if (argc >= 3) {
        format = hi_adp_str2fmt(argv[2]);
    }

    g_is_file = fopen(argv[1], "rb");
    if (!g_is_file) {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(format);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init( );
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_dmx_init failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto DMX_DEINIT;
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.buffer_size = 0x1000000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_is_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto TSBUF_FREE;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DMX_ID;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win, avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_UNF_VO_Attacwindow failed:%#x.\n", ret);
        goto ACHN_CLOSE;
    }

    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }
#if CONFIG_SUPPORT_HIGO
    ret = HI_GO_Init();
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_Init failed 0x%x\n", __FUNCTION__, ret);
        goto SND_DETACH;
    }
#endif

    ret = hi_unf_disp_get_virtual_screen(HI_UNF_DISPLAY1, &vir_width, &vir_height);
    if (ret != HI_SUCCESS) {
        sample_printf("HI_UNF_DISP_GetVirtualScreen failed 0x%x\n", ret);
    }

#if CONFIG_SUPPORT_HIGO
    ret = HigoCreateSurface(vir_width, vir_height, &Layer, &LayerSurface);
    if (ret != HI_SUCCESS) {
        goto HIGO_DEINIT;
    }
#endif

    pthread_mutex_init(&g_is_mutex, NULL);
    g_stop_is_thread = HI_FALSE;
    pthread_create(&g_is_thd, HI_NULL, (hi_void *)is_tthread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        sample_printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    prog_num = 0;

    pthread_mutex_lock(&g_is_mutex);
    rewind(g_is_file);
    hi_unf_dmx_reset_ts_buffer(g_is_buf);

    ret = hi_adp_avplay_play_prog(avplay, g_prog_tbl, prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_play_prog failed!\n");
        goto AVPLAY_STOP;
    }

    pthread_mutex_unlock(&g_is_mutex);

    sample_printf("please input q to quit\n");
    sample_printf("       input 0~%u to change channel\n", g_prog_tbl->prog_num);
    sample_printf("       input c to capture\n");

    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            sample_printf("prepare to quit!\n");
            break;
        }

        if (input_cmd[0] == 'c') {
            sprintf(file_name, "capture_%u.bmp", count++);
            vid_frame_to_picture(win, file_name);

            continue;
        }

        prog_num = atoi(input_cmd);
        if (prog_num == 0) {
            prog_num = 1;
        }

        pthread_mutex_lock(&g_is_mutex);
        rewind(g_is_file);
        hi_unf_dmx_reset_ts_buffer(g_is_buf);

        ret = hi_adp_avplay_play_prog(avplay, g_prog_tbl, prog_num - 1, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_adp_avplay_play_prog failed!\n");
            break;
        }

        pthread_mutex_unlock(&g_is_mutex);
    }

AVPLAY_STOP:
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_tbl);
    g_prog_tbl = HI_NULL;
    hi_adp_search_de_init();

    g_stop_is_thread = HI_TRUE;
    pthread_join(g_is_thd, HI_NULL);
    pthread_mutex_destroy(&g_is_mutex);

SND_DETACH:
    hi_unf_snd_detach(track, avplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);

WIN_DETACH:
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);

ACHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

TSBUF_FREE:
    hi_unf_dmx_destroy_ts_buffer(g_is_buf);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

VO_DEINIT:
    hi_unf_win_destroy(win);
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    fclose(g_is_file);
    g_is_file = HI_NULL;

    return ret;
}


