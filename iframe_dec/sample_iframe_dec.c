/**
\file
\brief I frame decode Sample
\copyright Shenzhen Hisilicon Co., Ltd.
\version draft
\author sdk
\date 2010-8-31
*/

/***************************** Macro Definition ******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "hi_type.h"
#include "hi_unf_avplay.h"
#include "hi_unf_demux.h"
#include "hi_unf_sound.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_unf_disp.h"
#include "jpeg_soft_enc.h"

#ifdef ANDROID
#include "hi_adp_osd.h"
#else
#ifdef HI_HIGO_SUPPORT
#include "hi_go.h"
#endif
#include "hi_unf_pdm.h"
#endif

#ifdef ANDROID
static hi_handle g_surface;
#endif

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

// #ifndef ANDROID
// #if defined(CHIP_TYPE_HI3716MV430) && defined(JPEG_SOFT_ENCODE_ENABLE)
// #define USE_JPEG_SOFT_ENCODE
// #endif

#if defined(CHIP_TYPE_HI3716MV430) && (!defined(JPEG_SOFT_ENCODE_ENABLE))
#ifdef HI_HIGO_SUPPORT
HIGO_IMGTYPE_E g_expect_type = HIGO_IMGTYPE_BMP;
#endif
hi_char g_file_name[] = "Iframe_picture.bmp";
#else
#ifdef HI_HIGO_SUPPORT
HIGO_IMGTYPE_E g_expect_type = HIGO_IMGTYPE_JPEG;
#endif
hi_char g_file_name[] = "Iframe_picture.jpg";
#endif


#define PLAY_DEMUX_ID      0
#define PLAY_TUNER_ID      0
#ifdef MAX_FILENAME_LEN
#else
#define MAX_FILENAME_LEN   256
#endif


#if defined CHIP_TYPE_HI3716MV430
#define MAX_FILESIZE       (3*1024*1024)
#else
#define MAX_FILESIZE       (16*1024*1024)
#endif

/******************************* API declaration *****************************/
static hi_s32 prepare_iframe(hi_char *file_name, hi_s32 protocol, hi_unf_avplay_iframe_info *iframe)
{
    hi_s32 ret;
    hi_s32 iframe_fd, read_len;
    struct stat file_stat;

    switch (protocol) {
        case 0: {
            iframe->type = HI_UNF_VCODEC_TYPE_MPEG2;
            break;
        }
        case 1: {
            iframe->type = HI_UNF_VCODEC_TYPE_MPEG4;
            break;
        }
        case 2: {
            iframe->type = HI_UNF_VCODEC_TYPE_AVS;
            break;
        }
        case 3: {
            iframe->type = HI_UNF_VCODEC_TYPE_H264;
            break;
        }
        case 4: {
            iframe->type = HI_UNF_VCODEC_TYPE_VC1;
            iframe->attr.vc1.advanced_profile = 1;
            iframe->attr.vc1.codec_version = 8;
            break;
        }
        case 5: {
            iframe->type = HI_UNF_VCODEC_TYPE_VC1;
            iframe->attr.vc1.advanced_profile = 0;
            iframe->attr.vc1.codec_version = 5;
            break;
        }
        case 6: {
            iframe->type = HI_UNF_VCODEC_TYPE_VC1;
            iframe->attr.vc1.advanced_profile = 0;
            iframe->attr.vc1.codec_version = 8;
            break;
        }
        default: {
            sample_printf("unsupported protocol\n");
            return HI_FAILURE;
        }
    }

    iframe_fd = open(file_name, O_RDONLY);
    if (iframe_fd == HI_NULL) {
        perror("open");
        printf("open file Failed\n");
        iframe->buf = HI_NULL;
        return HI_FAILURE;
    }

    ret = stat(file_name,&file_stat);
    if (ret != HI_SUCCESS || file_stat.st_size == 0 || (file_stat.st_size > MAX_FILESIZE)) {
        perror("stat");
        printf("stat failed\n");
        iframe->buf = HI_NULL;
        return HI_FAILURE;
    }

    if ((iframe->type == HI_UNF_VCODEC_TYPE_VC1) &&
        (iframe->attr.vc1.advanced_profile != 1) &&
        (iframe->attr.vc1.codec_version != 8)) {
        read_len = read(iframe_fd, &(iframe->buf_size), 4);
        if (read_len != 4) {
            printf("read vc1 Bufsize ERROR ! \n");
            iframe->buf = HI_NULL;
            return HI_FAILURE;
        }
    } else {
        iframe->buf_size = file_stat.st_size;
    }

    iframe->buf = (hi_u8*)malloc(iframe->buf_size);
    if (iframe->buf == HI_NULL) {
        perror("malloc");
        printf("malloc failed\n");
        return HI_FAILURE;
    }

    read_len = read(iframe_fd, iframe->buf, iframe->buf_size);

    if ((iframe->type == HI_UNF_VCODEC_TYPE_VC1) &&
        (iframe->attr.vc1.advanced_profile != 1) &&
        (iframe->attr.vc1.codec_version != 8)) {
        if (read_len < iframe->buf_size - 4) {
            perror("read");
            close(iframe_fd);
            free(iframe->buf);
            iframe->buf = HI_NULL;
            printf("read Data failed\n");
            return HI_FAILURE;
        }
    } else {
        if (read_len < iframe->buf_size) {
            perror("read");
            close(iframe_fd);
            free(iframe->buf);
            iframe->buf = HI_NULL;
            printf("read Data failed\n");
            return HI_FAILURE;
        }
    }

    close(iframe_fd);
    return HI_SUCCESS;
}

void release_iframe(hi_unf_avplay_iframe_info *iframe)
{
    if (iframe->buf != HI_NULL) {
        free(iframe->buf);
        iframe->buf = HI_NULL;
    }

    iframe->buf_size = 0;
}

hi_s32  prepare_avplay(hi_handle *avplay_out,hi_handle *win_out, hi_handle *track_out)
{
    hi_s32                   ret;
    hi_handle                win, avplay;
    hi_unf_avplay_attr     avplay_attr;
    hi_unf_sync_attr       sync_attr;
    hi_unf_audio_track_attr track_attr;

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_snd_init failed.\n");
        return HI_FAILURE;
    }

    ret = hi_adp_disp_init(HI_UNF_VIDEO_FMT_720P_50);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
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

#ifdef HI_FRONTEND_SUPPORT
    ret = hi_adp_dmx_attach_ts_port(PLAY_DEMUX_ID, PLAY_TUNER_ID);
    if (ret != HI_SUCCESS) {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }

#endif

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DEMUX_ID;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    ret |= hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto CHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win, avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_attach_src failed.\n");
        goto CHN_CLOSE;
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

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, track_out);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(*track_out, avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    *avplay_out = avplay;
    *win_out = win;
    return HI_SUCCESS;

SND_DETACH:
    hi_unf_snd_detach(HI_UNF_SND_0, avplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(*track_out);

WIN_DETACH:
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);

CHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

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
    return ret;
}

hi_s32  release_avplay(hi_handle avplay,hi_handle win, hi_handle track)
{
    hi_unf_snd_detach(track, avplay);
    hi_unf_snd_destroy_track(track);
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_destroy(avplay);
    hi_unf_avplay_deinit();
    hi_unf_dmx_detach_ts_port(PLAY_DEMUX_ID);
    hi_unf_dmx_deinit();
    hi_unf_win_destroy(win);
    hi_adp_win_deinit();
    hi_adp_disp_deinit();
    hi_adp_snd_deinit();

    return HI_SUCCESS;
}

#ifdef HI_FRONTEND_SUPPORT
hi_s32 receive_prog(hi_s32 tuner_freq, hi_s32 tuner_srate, hi_u32 third_param, pmt_compact_tbl **prg_tbl)
{
    hi_s32 ret;

    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_frontend_init failed.\n");
        return HI_FAILURE;
    }

    ret = hi_adp_frontend_connect(PLAY_TUNER_ID, tuner_freq, tuner_srate, third_param);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DEMUX_ID, prg_tbl);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_search_get_all_pmt failed\n");
        goto PSISI_FREE;
    }

    return HI_SUCCESS;

PSISI_FREE:
    hi_adp_search_de_init();

TUNER_DEINIT:
    hi_adp_frontend_deinit();
    return ret;
}

#endif

#ifdef HI_FRONTEND_SUPPORT
hi_s32 release_prog(pmt_compact_tbl *prog_tbl)
{
    hi_adp_search_free_all_pmt(prog_tbl);
    hi_adp_search_de_init();
    hi_adp_frontend_deinit();
    return HI_SUCCESS;
}
#endif
hi_s32  prepare_higo(hi_handle *layer_out)
{
#ifdef ANDROID
    HIADP_SURFACE_ATTR_S surface_attr;

    surface_attr.u32Width = 1280;
    surface_attr.u32Height = 720;
    surface_attr.enPixelFormat = HIADP_PF_8888;
    ret = HIADP_OSD_CreateSurface(&surface_attr, &g_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }
#else
#ifdef HI_HIGO_SUPPORT
    hi_s32 ret;
    HIGO_LAYER_INFO_S layer_info ;
    hi_handle layer;

    ret  = HI_GO_Init();
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_GO_Init failed:%#x\n",ret);
        return HI_FAILURE;
    }

    HI_GO_GetLayerDefaultParam(HIGO_LAYER_HD_0, &layer_info);

    layer_info.LayerID =  HIGO_LAYER_HD_0;
    layer_info.ScreenWidth = 1280;
    layer_info.CanvasHeight = 720;
    ret = HI_GO_CreateLayer(&layer_info, &layer);
    if (ret != HI_SUCCESS) {
        sample_printf("-->HI_GO_CreateLayer failed ret = 0x%x line %d \n",ret,__LINE__);
        HI_GO_Deinit();
        return HI_FAILURE;
    }

    *layer_out = layer;

    ret = HI_GO_SetLayerAlpha(layer, 0);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_GO_SetLayerAlpha failed\n");
        HI_GO_DestroyLayer(layer);
        HI_GO_Deinit();
        return HI_FAILURE;
    }
#endif
#endif

    return HI_SUCCESS;
}


hi_s32  process_picture(hi_handle layer, hi_unf_video_frame_info *cap_pic)
{
    hi_s32 ret = HI_FAILURE;

#ifdef ANDROID

    ret = HIADP_OSD_BlitFrameToSurface(cap_pic, g_surface, HI_NULL);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = HIADP_OSD_Refresh(g_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }
#else
#ifdef HI_HIGO_SUPPORT

    hi_handle      layer_surf, mem_surf;
    HIGO_SURINFO_S surf_info;
    HIGO_BLTOPT2_S blt_opt;
    #if 0
    sample_printf("=======IFrame Picture Info=============\n"
            "u32TopYStartAddr:%x\n"
            "u32TopCStartAddr:%x\n"
            "u32BottomYStartAddr:%x\n"
            "u32BottomCStartAddr:%x\n"
            "u32Width:%d\n"
            "u32Height:%d\n"
            "u32Stride:%d\n"
            "enVideoFormat:%d\n"
            "enFieldMode:%d\n"
            "enSampleType:%d\n",
            cap_pic->u32TopYStartAddr,
            cap_pic->u32TopCStartAddr,
            cap_pic->u32BottomYStartAddr,
            cap_pic->u32BottomCStartAddr,
            cap_pic->u32Width,
            cap_pic->u32Height,
            cap_pic->u32Stride,
            cap_pic->enVideoFormat,
            cap_pic->enFieldMode,
            cap_pic->enSampleType);
    #endif

    memset(&surf_info,0,sizeof(surf_info));
    surf_info.Width       = cap_pic->u32Width;
    surf_info.Height      = cap_pic->u32Height;
    surf_info.PixelFormat = HIGO_PF_YUV420;
    surf_info.pPhyAddr[0] = (hi_char*)((hi_char *)HI_NULL + cap_pic->stVideoFrameAddr[0].u32YAddr);
    surf_info.pPhyAddr[1] = (hi_char*)((hi_char *)HI_NULL + cap_pic->stVideoFrameAddr[0].u32CAddr);
    surf_info.Pitch[0]    = cap_pic->stVideoFrameAddr[0].u32YStride;
    surf_info.Pitch[1]    = cap_pic->stVideoFrameAddr[0].u32YStride;
    surf_info.pVirAddr[0] = (hi_char*)((hi_char *)HI_NULL + cap_pic->stVideoFrameAddr[0].u32YAddr);
    surf_info.pVirAddr[1] = (hi_char*)((hi_char *)HI_NULL + cap_pic->stVideoFrameAddr[0].u32CAddr);

    ret= HI_GO_CreateSurfaceFromMem(&surf_info, &mem_surf);
    if (ret != HI_SUCCESS) {
        sample_printf("\n HI_GO_CreateSurfaceFromMem failed\n");
        return HI_FAILURE;
    }

    ret = HI_GO_GetLayerSurface(layer, &layer_surf);
    if (ret != HI_SUCCESS) {
        sample_printf("-->HI_GO_GetLayerSurface failed ret = 0x%x line %d \n",ret,__LINE__);
        goto OUT;
    }

    memset(&blt_opt,0,sizeof(HIGO_BLTOPT2_S));
    ret = HI_GO_StretchBlit(mem_surf, HI_NULL, layer_surf, HI_NULL, &blt_opt);
    if (ret != HI_SUCCESS) {
        sample_printf("\n HI_GO_StretchBlit failed\n");
        goto OUT;
    }

    ret = HI_GO_RefreshLayer(layer, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("\n HI_GO_FreeSurface failed\n");
        HI_GO_DestroyLayer(layer);
        goto OUT;
    }

OUT:
    ret |= HI_GO_FreeSurface(mem_surf);
    if (ret != HI_SUCCESS) {
        sample_printf("\n HI_GO_FreeSurface failed\n");
    }
#endif
#endif

    return ret;
}

#ifndef ANDROID

hi_s32 save_picture(hi_handle avplay, hi_unf_avplay_iframe_info *iframe, hi_char *file_name)
{
    hi_s32 ret;
    hi_unf_video_frame_info cap_pic;
#ifdef USE_JPEG_SOFT_ENCODE
    SAMPLE_VIDEO_FREMAE_INFO_S frame_info = {0};
#else
#ifdef HI_HIGO_SUPPORT
    hi_handle                 mem_surf;
    HIGO_SURINFO_S            surf_info = {0};
    HIGO_ENC_ATTR_S           enc_attr;
#endif
#endif

    if (!file_name) {
        sample_printf("[%s] filename error\n", __FUNCTION__);
        return HI_FAILURE;
    }

    ret = hi_unf_avplay_decode_iframe(avplay, iframe, &cap_pic);
    if (ret != HI_SUCCESS) {
        sample_printf("call DecodeIFrame failed\n");
        return HI_FAILURE;
    }

#ifdef USE_JPEG_SOFT_ENCODE
    frame_info.Height[0] = cap_pic.u32Height;
    frame_info.Width[0] = cap_pic.u32Width;

    frame_info.yuv_fmt_type = JPGE_INPUT_FMT_YUV420;
    frame_info.yuv_sample_type = JPGE_INPUT_SAMPLER_SEMIPLANNAR;
    frame_info.Stride[0] = cap_pic.stVideoFrameAddr[0].u32YStride;
    frame_info.Stride[1] = cap_pic.stVideoFrameAddr[0].u32CStride;
    frame_info.PhyBuf[0] = cap_pic.stVideoFrameAddr[0].u32YAddr;
    frame_info.PhyBuf[1] = cap_pic.stVideoFrameAddr[0].u32CAddr;

    JPEG_Soft_EncToFile(file_name, &frame_info);
#else
#ifdef HI_HIGO_SUPPORT

    surf_info.Width        = cap_pic.u32Width;
    surf_info.Height       = cap_pic.u32Height;
    surf_info.PixelFormat  = HIGO_PF_BUTT;
    switch (cap_pic.enVideoFormat) {
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_422 :
            surf_info.PixelFormat = HIGO_PF_YUV422;
            break;
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_420 :
            surf_info.PixelFormat = HIGO_PF_YUV420;
            break;
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_400 :
            surf_info.PixelFormat = HIGO_PF_YUV400;
            break;
        default :
            sample_printf("[%s] not support format %d\n", __FUNCTION__, cap_pic.enVideoFormat);
    }

    if (HIGO_PF_BUTT != surf_info.PixelFormat) {
        surf_info.Pitch[0]     = cap_pic.stVideoFrameAddr[0].u32YStride;
        surf_info.Pitch[1]     = cap_pic.stVideoFrameAddr[0].u32CStride;
        surf_info.pPhyAddr[0]  = (hi_char*)HI_NULL + cap_pic.stVideoFrameAddr[0].u32YAddr;
        surf_info.pPhyAddr[1]  = (hi_char*)HI_NULL + cap_pic.stVideoFrameAddr[0].u32CAddr;
        surf_info.pVirAddr[0]  = HI_NULL;
        surf_info.pVirAddr[1]  = HI_NULL;
        surf_info.MemType      = HIGO_MEMTYPE_MMZ;
        surf_info.bPubPalette  = HI_FALSE;

        ret = HI_GO_CreateSurfaceFromMem(&surf_info, &mem_surf);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] HI_GO_CreateSurface failed 0x%x\n", __FUNCTION__, ret);
        }

        enc_attr.ExpectType   = g_expect_type;
        enc_attr.QualityLevel = 99;

        ret = HI_GO_EncodeToFile(mem_surf, file_name, &enc_attr);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] HI_GO_EncodeToFile failed 0x%x\n", __FUNCTION__, ret);
        } else {
            sample_printf("save to %s\n\n", file_name);
        }

        ret = HI_GO_FreeSurface(mem_surf);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] HI_GO_FreeSurface failed 0x%x\n", __FUNCTION__, ret);
        }
    }
#endif
#endif

    ret = hi_unf_avplay_release_iframe(avplay, &cap_pic);
    if (ret != HI_SUCCESS) {
        sample_printf("call ReleaseIFrame failed\n");
    }

    return HI_SUCCESS;
}

hi_s32 update_logo(hi_handle avplay, hi_unf_avplay_iframe_info *iframe)
{
    hi_s32 ret = HI_SUCCESS;
    hi_unf_video_frame_info cap_pic;
    hi_u8                     *buf = HI_NULL;
    hi_u32                    logo_size = 0;;
#ifdef USE_JPEG_SOFT_ENCODE
    SAMPLE_VIDEO_FREMAE_INFO_S frame_info = {0};
#else
#ifdef HI_HIGO_SUPPORT
    hi_handle                 mem_surf;
    HIGO_SURINFO_S            surf_info = {0};
    HIGO_ENC_ATTR_S           enc_attr;
    hi_u32                    max_size = 2 * 1024 * 1024;
#endif
#endif

    ret = hi_unf_avplay_decode_iframe(avplay, iframe, &cap_pic);
    if (ret != HI_SUCCESS) {
        sample_printf("call DecodeIFrame failed\n");
        return HI_FAILURE;
    }

#ifdef USE_JPEG_SOFT_ENCODE
    frame_info.Height[0] = cap_pic.u32Height;
    frame_info.Width[0] = cap_pic.u32Width;

    frame_info.yuv_fmt_type = JPGE_INPUT_FMT_YUV420;
    frame_info.yuv_sample_type = JPGE_INPUT_SAMPLER_SEMIPLANNAR;
    frame_info.Stride[0] = cap_pic.stVideoFrameAddr[0].u32YStride;
    frame_info.Stride[1] = cap_pic.stVideoFrameAddr[0].u32CStride;
    frame_info.PhyBuf[0] = cap_pic.stVideoFrameAddr[0].u32YAddr;
    frame_info.PhyBuf[1] = cap_pic.stVideoFrameAddr[0].u32CAddr;

    ret = JPEG_Soft_EncToMem(&frame_info, &buf, &logo_size);
    if (ret != HI_SUCCESS) {
        sample_printf("call JPEG_Soft_EncToMem failed\n");
        ret = hi_unf_avplay_release_iframe(avplay, &cap_pic);
        if (ret != HI_SUCCESS) {
            sample_printf("call ReleaseIFrame failed\n");
        }

        return ret;
    }

    printf("logo_size:%d\n",logo_size);
#else
#ifdef HI_HIGO_SUPPORT

    surf_info.Width        = cap_pic.u32Width;
    surf_info.Height       = cap_pic.u32Height;
    surf_info.PixelFormat  = HIGO_PF_BUTT;

    switch (cap_pic.enVideoFormat) {
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_422 :
            surf_info.PixelFormat = HIGO_PF_YUV422;
            break;
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_420 :
            surf_info.PixelFormat = HIGO_PF_YUV420;
            break;
        case HI_UNF_FORMAT_YUV_SEMIPLANAR_400 :
            surf_info.PixelFormat = HIGO_PF_YUV400;
            break;
        default :
            sample_printf("[%s] not support format %d\n", __FUNCTION__, cap_pic.enVideoFormat);
    }

    if (HIGO_PF_BUTT == surf_info.PixelFormat) {
        ret = hi_unf_avplay_release_iframe(avplay, &cap_pic);
        if (ret != HI_SUCCESS) {
            sample_printf("call ReleaseIFrame failed\n");
        }

        return ret;
    }

    surf_info.Pitch[0]     = cap_pic.stVideoFrameAddr[0].u32YStride;
    surf_info.Pitch[1]     = cap_pic.stVideoFrameAddr[0].u32CStride;
    surf_info.pPhyAddr[0]  = (hi_char *)HI_NULL + cap_pic.stVideoFrameAddr[0].u32YAddr;
    surf_info.pPhyAddr[1]  = (hi_char *)HI_NULL + cap_pic.stVideoFrameAddr[0].u32CAddr;
    surf_info.pVirAddr[0]  = HI_NULL;
    surf_info.pVirAddr[1]  = HI_NULL;
    surf_info.MemType      = HIGO_MEMTYPE_MMZ;
    surf_info.bPubPalette  = HI_FALSE;

    ret = HI_GO_CreateSurfaceFromMem(&surf_info, &mem_surf);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_CreateSurface failed 0x%x\n", __FUNCTION__, ret);
        ret = hi_unf_avplay_release_iframe(avplay, &cap_pic);
        if (ret != HI_SUCCESS) {
            sample_printf("call ReleaseIFrame failed\n");
        }

        return ret;
    }

    buf = malloc(max_size);
    if (HI_NULL == buf) {
        printf("ERR: malloc !\n");
        goto FREE_SURF;
    }

    memset(buf, 0x00, max_size);

    enc_attr.ExpectType   = g_expect_type;
    enc_attr.QualityLevel = 99;

    ret = HI_GO_EncodeToMem(mem_surf, buf, max_size, &logo_size, &enc_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_EncodeToMem failed 0x%x\n", __FUNCTION__, ret);
    }
#endif
#endif

    ret = hi_unf_pdm_update_logo_content(0, buf, logo_size);
    if (ret != HI_SUCCESS) {
        sample_printf("ERR: hi_unf_pdm_update_logo_content, ret = %#x\n", ret);
        goto FREE_BUF;
    }

FREE_BUF:
    free(buf);
    buf = HI_NULL;

#ifndef USE_JPEG_SOFT_ENCODE
#ifdef HI_HIGO_SUPPORT
FREE_SURF:
    ret = HI_GO_FreeSurface(mem_surf);
    if (ret != HI_SUCCESS) {
        sample_printf("[%s] HI_GO_FreeSurface failed 0x%x\n", __FUNCTION__, ret);
    }
#endif
#endif

    ret = hi_unf_avplay_release_iframe(avplay, &cap_pic);
    if (ret != HI_SUCCESS) {
        sample_printf("call ReleaseIFrame failed\n");
    }

    return ret;
}
#endif
hi_s32 resume_osd(hi_handle layer)
{
    hi_s32 ret = HI_SUCCESS;

#ifdef ANDROID

    ret = HIADP_OSD_ClearSurface(g_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = HIADP_OSD_Refresh(g_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }

#else
#ifdef HI_HIGO_SUPPORT
    hi_handle layer_surf;

    ret = HI_GO_SetLayerAlpha(layer,0);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_GO_SetLayerAlpha failed\n");
        return HI_FAILURE;
    }

    ret = HI_GO_GetLayerSurface(layer,&layer_surf);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_GO_GetLayerSurface failed:%#x\n",ret);
        return HI_FAILURE;
    }

    ret = HI_GO_FillRect(layer_surf,HI_NULL,0xFFFFFF,HIGO_COMPOPT_NONE);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_GO_FillRect failed:%#x\n",ret);
        return HI_FAILURE;
    }

    ret = HI_GO_RefreshLayer(layer,HI_NULL);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }
#endif
#endif

    return ret;
}

hi_s32  release_higo(hi_handle layer)
{
#ifdef ANDROID
    HIADP_OSD_DestroySurface(g_surface);
#else
#ifdef HI_HIGO_SUPPORT
    HI_GO_DestroyLayer(layer);
    HI_GO_Deinit();
#endif
#endif

    return HI_SUCCESS;
}

hi_s32 process_iframe(hi_handle avplay, hi_unf_avplay_iframe_info *iframe, hi_handle layer, hi_s32 proc_type)
{
    hi_s32 ret;
    hi_unf_video_frame_info cap_pic;

    if (0 == proc_type) {
        ret = hi_unf_avplay_decode_iframe(avplay, iframe, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call DecodeIFrame failed\n");
            return HI_FAILURE;
        }
    } else if (1 == proc_type) {
#ifndef ANDROID
#ifdef HI_HIGO_SUPPORT
        ret = HI_GO_SetLayerAlpha(layer, 255);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_GO_SetLayerAlpha failed\n");
            return HI_FAILURE;
        }
#endif
#endif

        ret = hi_unf_avplay_decode_iframe(avplay, iframe, &cap_pic);
        if (ret != HI_SUCCESS) {
            sample_printf("call DecodeIFrame failed\n");
            return HI_FAILURE;
        }

        ret = process_picture(layer, &cap_pic);

        if (ret != HI_SUCCESS) {
            return HI_FAILURE;
        }

        ret = hi_unf_avplay_release_iframe(avplay, &cap_pic);
        if (ret != HI_SUCCESS) {
            sample_printf("call ReleaseIFrame failed\n");
        }
    }

    return HI_SUCCESS;
}

hi_s32 main(hi_s32 argc,hi_char **argv)
{
    hi_s32                      ret = HI_FAILURE;
    hi_handle                   win, avplay, track;
    hi_handle                   layer = HI_INVALID_HANDLE;

#ifdef HI_FRONTEND_SUPPORT

    hi_s32                      tuner_freq = 610;
    hi_s32                      tuner_srate = 6875;
    hi_u32                      third_param = 0;
    pmt_compact_tbl             *prog_tbl = HI_NULL;
    hi_bool                     dvb_play = HI_FALSE;

#endif
    hi_s32                      protocol, proc_type;
    hi_char                     *file_name;
    hi_unf_avplay_iframe_info     iframe_info;
    hi_unf_avplay_stop_opt    stop_opt;
    hi_char                     input_cmd[32];

    if (argc < 4) {
        printf("usage:%s file protocol type [freq] [srate] [qamtype or polarization]\n"
               "protocol 0-mpeg2 1-mpeg4 2-avs 3-h264 4-vc1ap 5-vc1smp5 6-vc1smp8\n"
               "type     0-display on vo 1-display on osd \n"
               "qamtype or polarization: \n"
               "    For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
               "    For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n",
               argv[0]);

        return HI_FAILURE;
    }

    file_name = argv[1];
    protocol = atoi(argv[2]);
    proc_type = atoi(argv[3]);

#ifdef HI_FRONTEND_SUPPORT

    if (argc > 6) {
        dvb_play = HI_TRUE;
        tuner_freq = atoi(argv[4]);
        tuner_srate = atoi(argv[5]);
        third_param = atoi(argv[6]);
    } else if (argc == 6) {
        dvb_play = HI_TRUE;
        tuner_freq = atoi(argv[4]);
        tuner_srate = atoi(argv[5]);
        third_param = (tuner_freq>1000) ? 0 : 64;
    } else if (argc == 5) {
        dvb_play = HI_TRUE;
        tuner_freq = atoi(argv[4]);
        tuner_srate = (tuner_freq>1000) ? 27500 : 6875;
        third_param = (tuner_freq>1000) ? 0 : 64;
    }

#else
    if (argc >= 5 ) {
        printf("cannot support dvbplay,please enable the dvbplay! \n");
        return ret;
    }
#endif

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = prepare_iframe(file_name, protocol,&iframe_info);
    if (ret != HI_SUCCESS) {
        hi_unf_sys_deinit();
        printf("prepare_iframe Failed \n");
        return HI_FAILURE;
    }

    ret = prepare_avplay(&avplay,&win, &track);
    if (ret != HI_SUCCESS) {
        printf("prepare_avplay failed \n");
        goto OUT0;
    }

    ret = prepare_higo(&layer);
    if (ret != HI_SUCCESS) {
        printf("prepare_higo failed \n");
        goto OUT1;
    }

#ifdef HI_FRONTEND_SUPPORT
    /* lock frequency, receive table, play program */
    if (HI_TRUE == dvb_play) {
        ret = receive_prog(tuner_freq,tuner_srate,third_param,&prog_tbl);
        if (ret != HI_SUCCESS) {
            goto OUT2;
        }

        ret = hi_adp_avplay_play_prog(avplay,prog_tbl, 0, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS) {
            sample_printf("call PlayProg failed.\n");
            goto OUT3;
        }

        while(1) {
            printf("please input: q - quit!\n");
            printf("              i - display i frame!\n");
            printf("              d - play dvb!\n");

            SAMPLE_GET_INPUTCMD(input_cmd);

            if ('q' == input_cmd[0]) {
                printf("prepare to quit!\n");
                break;
            }

            if ('i' == input_cmd[0]) {
                stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
                stop_opt.timeout = 0;
                hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
                ret = process_iframe(avplay, &iframe_info, layer, proc_type);
                if (ret != HI_SUCCESS) {
                    continue;
                }

                continue;
            }

            if ('d' == input_cmd[0]) {
                resume_osd(layer);

                ret = hi_unf_avplay_start(avplay,HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
                if (ret != HI_SUCCESS) {
                    sample_printf("call AVPLAY_Start failed\n");
                    continue;
                }

                continue;
            }
        }
    } else

#endif
    {
        stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop_opt.timeout = 0;
        hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);

        ret = process_iframe(avplay, &iframe_info, layer, proc_type);
        if (ret != HI_SUCCESS) {
            goto OUT2;
        }

        while (1) {
            printf("please input: q - quit!\n");

#ifndef ANDROID
            printf("              u - update logo!\n");
            printf("              s - save i frame to picture!\n");
#endif
            SAMPLE_GET_INPUTCMD(input_cmd);

#ifndef ANDROID
            if ('s' == input_cmd[0]) {
                ret = save_picture(avplay, &iframe_info, g_file_name);
                if (ret != HI_SUCCESS) {
                    printf("save Iframe to picture err!\n");
                }
                continue;
            }

            if ('u' == input_cmd[0]) {
                printf("update logo \n");
                ret = update_logo(avplay, &iframe_info);
                if (ret != HI_SUCCESS) {
                    printf("update_logo fail !\n");
                }

                continue;
            }
#endif
            if ('q' == input_cmd[0]) {
                printf("prepare to quit!\n");
                break;
            }
        }
    }

#ifdef HI_FRONTEND_SUPPORT
OUT3:
    if (HI_TRUE == dvb_play) {
        stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop_opt.timeout = 0;
        hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
        release_prog(prog_tbl);
    }

#endif

OUT2:
    release_higo(layer);
OUT1:
    release_avplay(avplay, win, track);
OUT0:
    release_iframe(&iframe_info);

    hi_unf_sys_deinit();
    return ret;
}

