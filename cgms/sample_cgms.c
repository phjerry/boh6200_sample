/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : sample_cgms.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   :
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_common.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_vo.h"
#include "hi_unf_demux.h"
#if CONFING_SUPPORT_AUDIO
#include "HA.AUDIO.MP3.decode.h"
#include "HA.AUDIO.MP2.decode.h"
#include "HA.AUDIO.AAC.decode.h"
#include "HA.AUDIO.DRA.decode.h"
#include "HA.AUDIO.PCM.decode.h"
#include "HA.AUDIO.WMA9STD.decode.h"
#include "HA.AUDIO.AMRNB.codec.h"
#include "HA.AUDIO.AMRWB.codec.h"
#include "HA.AUDIO.DOLBYTRUEHD.decode.h"
#include "HA.AUDIO.TRUEHDPASSTHROUGH.decode.h"
#include "HA.AUDIO.DTSHD.decode.h"
#include "HA.AUDIO.AC3PASSTHROUGH.decode.h"
#include "HA.AUDIO.DTSPASSTHROUGH.decode.h"
#include "HA.AUDIO.DTSM6.decode.h"
#endif
#include "hi_adp_mpi.h"
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
#include "HA.AUDIO.DOLBYPLUS.decode.h"
#endif

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif
FILE *g_vid_es_file = HI_NULL;
FILE *g_aud_es_file = HI_NULL;

static pthread_t g_es_thd;
static hi_bool g_stop_thread = HI_FALSE;
static hi_bool g_aud_play = HI_FALSE;
static hi_bool g_vid_play = HI_FALSE;
static hi_bool g_play_once = HI_FALSE;

static hi_bool g_read_frame_size = HI_FALSE;

hi_s32 g_aud_es_file_offest;

hi_void cgms_put_vid_stream(HI_HANDLE avplay, hi_bool *vid_buf_avail)
{
    hi_u32 frame_size = 0;
    hi_unf_stream_buf stream_buf;
    hi_u32 readl_len;
    hi_s32 ret;

    if (g_read_frame_size != HI_FALSE) {
        ret = HI_UNF_AVPLAY_GetBuf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, 0x100000, &stream_buf, 0);
    } else {
        ret = HI_UNF_AVPLAY_GetBuf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, 0x4000, &stream_buf, 0);
    }

    if (ret == HI_SUCCESS) {
        *vid_buf_avail = HI_TRUE;

        if (g_read_frame_size != HI_FALSE) {
            readl_len = fread(&frame_size, 1, 4, g_vid_es_file);
            if (readl_len == 4) {
            } else {
                frame_size = 0x4000;
            }
        } else {
            frame_size = 0x4000;
        }

        readl_len = fread(stream_buf.data, sizeof(HI_S8), frame_size, g_vid_es_file);

        if (readl_len > 0) {
            ret = HI_UNF_AVPLAY_PutBuf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, readl_len, 0);
            if (ret != HI_SUCCESS) {
                sample_printf("call HI_UNF_AVPLAY_PutBuf failed.\n");
            }

            frame_size = 0;
        } else if (readl_len == 0) {
            if (g_play_once == HI_TRUE) {
                hi_bool is_empty = HI_FALSE;
                HI_UNF_AVPLAY_FLUSH_STREAM_OPT_S stFlushOpt;

                sample_printf("Esplay flush stream.\n");
                HI_UNF_AVPLAY_FlushStream(avplay, &stFlushOpt);
                do {
                    ret = HI_UNF_AVPLAY_IsBuffEmpty(avplay, &is_empty);
                    if (ret != HI_SUCCESS) {
                        sample_printf("call HI_UNF_AVPLAY_IsBuffEmpty failed.\n");
                        break;
                    }
                } while (is_empty != HI_TRUE);
                sleep(5);
                sample_printf("Finish, esplay exit!\n");
                exit(0);
            } else {
                sample_printf("read vid file end and rewind!\n");
                rewind(g_vid_es_file);
            }
        } else {
            perror("read vid file error\n");
        }
    } else {
        *vid_buf_avail = HI_FALSE;
    }
}

hi_void cgms_put_audio_stream(HI_HANDLE avplay, hi_bool *aud_buf_avail)
{
    static int counter = 0;
    hi_unf_stream_buf stream_buf;
    hi_u32 readl_len;
    hi_s32 ret;

    ret = HI_UNF_AVPLAY_GetBuf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, 0x1000, &stream_buf, 0);
    if (ret == HI_SUCCESS) {
        *aud_buf_avail = HI_TRUE;
        readl_len = fread(stream_buf.data, sizeof(HI_S8), 0x1000, g_aud_es_file);
        if (readl_len > 0) {
            counter++;
            ret = HI_UNF_AVPLAY_PutBuf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, readl_len, 0);
            if (ret != HI_SUCCESS) {
                sample_printf("call HI_UNF_AVPLAY_PutBuf failed.\n");
            }
        } else if (readl_len == 0) {
            sample_printf("read aud file end and rewind!\n");
            rewind(g_aud_es_file);
            if (g_aud_es_file_offest) {
                fseek(g_aud_es_file, g_aud_es_file_offest, SEEK_SET);
            }
        } else {
            perror("read aud file error\n");
        }
    } else if (ret != HI_SUCCESS) {
        *aud_buf_avail = HI_FALSE;
    }
}

hi_void *es_thread(hi_void *args)
{
    HI_HANDLE avplay = *((HI_HANDLE *)args);
    hi_bool vid_buf_avail = HI_FALSE;
    hi_bool aud_buf_avail = HI_FALSE;

    while (!g_stop_thread) {
        if (g_vid_play) {
            cgms_put_vid_stream(avplay, &vid_buf_avail);
        }

        if (g_aud_play) {
            cgms_put_audio_stream(avplay, &aud_buf_avail);
        }

        /* wait for buffer */
        if ((aud_buf_avail == HI_FALSE) && (vid_buf_avail == HI_FALSE)) {
            usleep(1000 * 10);
        }
    }

    return HI_NULL;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret, index;
    HI_HANDLE win;
    HI_HANDLE track;
    HI_UNF_AUDIOTRACK_ATTR_S track_attr;

    HI_UNF_VCODEC_TYPE_E vdec_type = HI_UNF_VCODEC_TYPE_BUTT;
    hi_u32 adec_type = 0;
    HI_HANDLE avplay;
    HI_UNF_AVPLAY_ATTR_S avplay_attr;
    HI_UNF_SYNC_ATTR_S av_sync_attr;
    HI_UNF_AVPLAY_STOP_OPT_S stop;
    hi_char input_cmd[32];
    HI_HA_DECODEMODE_E audio_dec_mode = HD_DEC_MODE_RAWPCM;
    hi_s32 dts_core_only = 0;
    HI_UNF_ENC_FMT_E g_default_fmt = HI_UNF_ENC_FMT_720P_50;
    hi_bool advanced_profil = 1;
    hi_u32 codec_version = 8;
    hi_u32 frame_rate = 0;

    if (argc < 5) {
#if 1
        printf(" usage: sample_cgms vfile vtype null null [fmt] \n");
        printf(" vtype: h264/h265/mpeg2/mpeg4/avs/real8/real9/vc1ap/vc1smp5(WMV3)/vc1smp8/vp6/vp6f/vp6a/vp8/divx3/h263/sor\n");
        printf(" fmt: 1080p50/1080p60/1080i60/1080i50/720p50/720p60/480p60/576p50/ntsc/pal_n\n");
        printf(" examples: \n");
        printf("   sample_cgms vfile h264 null null\n");
#endif
        return 0;
    }

    if (strcasecmp("null", argv[1])) {
        g_vid_play = HI_TRUE;
        if (!strcasecmp("mpeg2", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_MPEG2;
        } else if (!strcasecmp("mpeg4", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_MPEG4;
        } else if (!strcasecmp("h263", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_H263;
        } else if (!strcasecmp("sor", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_SORENSON;
        } else if (!strcasecmp("vp6", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_VP6;
        } else if (!strcasecmp("vp6f", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_VP6F;
        } else if (!strcasecmp("vp6a", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_VP6A;
        } else if (!strcasecmp("h264", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_H264;
        } else if (!strcasecmp("h265", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_HEVC;
        } else if (!strcasecmp("mvc", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_MVC;
        } else if (!strcasecmp("avs", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_AVS;
        } else if (!strcasecmp("real8", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_REAL8;
        } else if (!strcasecmp("real9", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_REAL9;
        } else if (!strcasecmp("vc1ap", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_VC1;
            advanced_profil = 1;
            codec_version = 8;
        } else if (!strcasecmp("vc1smp5", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_VC1;
            advanced_profil = 0;
            codec_version = 5;  // WMV3
        } else if (!strcasecmp("vc1smp8", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_VC1;
            advanced_profil = 0;
            codec_version = 8;  // not WMV3
        } else if (!strcasecmp("vp8", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_VP8;
        } else if (!strcasecmp("divx3", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_DIVX3;
        } else if (!strcasecmp("mvc", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_MVC;
        } else if (!strcasecmp("mjpeg", argv[2])) {
            g_read_frame_size = HI_TRUE;
            vdec_type = HI_UNF_VCODEC_TYPE_MJPEG;
        } else {
            sample_printf("unsupport vid codec type!\n");
            return -1;
        }

        g_vid_es_file = fopen(argv[1], "rb");
        if (!g_vid_es_file) {
            sample_printf("open file %s error!\n", argv[1]);
            return -1;
        }
    }

    if (strcasecmp("null", argv[3])) {
        g_aud_es_file = fopen(argv[3], "rb");
        if (!g_aud_es_file) {
            sample_printf("open file %s error!\n", argv[3]);
            return -1;
        }

        if (argc > 5) {
            if (!strcasecmp("-mode0", argv[5])) {
                audio_dec_mode = HD_DEC_MODE_RAWPCM;
            } else if (!strcasecmp("-mode1", argv[5])) {
                audio_dec_mode = HD_DEC_MODE_THRU;
            } else if (!strcasecmp("-mode2", argv[5])) {
                audio_dec_mode = HD_DEC_MODE_SIMUL;
            }
        }

        g_aud_play = HI_TRUE;
        if (!strcasecmp("aac", argv[4])) {
            adec_type = HA_AUDIO_ID_AAC;
        } else if (!strcasecmp("mp3", argv[4])) {
            adec_type = HA_AUDIO_ID_MP3;
        } else if (!strcasecmp("truehd", argv[4])) {
            adec_type = HA_AUDIO_ID_DOLBY_TRUEHD;
        } else if (!strcasecmp("ac3raw", argv[4])) {
            adec_type = HA_AUDIO_ID_AC3PASSTHROUGH;
        } else if (!strcasecmp("dtsraw", argv[4])) {
            adec_type = HA_AUDIO_ID_DTSPASSTHROUGH;
        }
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
        else if (!strcasecmp("ddp", argv[4])) {
            adec_type = HA_AUDIO_ID_DOLBY_PLUS;
        }
#endif
        else if (!strcasecmp("dts", argv[4])) {
            adec_type = HA_AUDIO_ID_DTSHD;
        } else if (!strcasecmp("dtsm6", argv[4])) {
            adec_type = HA_AUDIO_ID_DTSM6;
        } else if (!strcasecmp("dra", argv[4])) {
            adec_type = HA_AUDIO_ID_DRA;
        } else if (!strcasecmp("pcm", argv[4])) {
            adec_type = HA_AUDIO_ID_PCM;
        } else if (!strcasecmp("mlp", argv[4])) {
            adec_type = HA_AUDIO_ID_TRUEHD;
        } else if (!strcasecmp("amr", argv[4])) {
            adec_type = HA_AUDIO_ID_AMRNB;
        } else if (!strcasecmp("amrwb", argv[4])) {
            adec_type = HA_AUDIO_ID_AMRWB;
        } else {
            sample_printf("unsupport aud codec type!\n");
            return -1;
        }
    }

    for (index = 0; index < argc; index++) {
        if (strcasestr("1080p50", argv[index])) {
            sample_printf("sample use 1080p50 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_1080P_50;
            break;
        } else if (strcasestr("1080p60", argv[index])) {
            sample_printf("sample use 1080p60 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_1080P_60;
            break;
        } else if (strcasestr("1080i60", argv[index])) {
            sample_printf("sample use 1080i60 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_1080i_60;
            break;
        } else if (strcasestr("1080i50", argv[index])) {
            sample_printf("sample use 1080i50 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_1080i_50;
            break;
        } else if (strcasestr("720p50", argv[index])) {
            sample_printf("sample use 720p50 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_720P_50;
            break;
        } else if (strcasestr("720p60", argv[index])) {
            sample_printf("sample use 720p60 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_720P_60;
            break;
        } else if (strcasestr("480p60", argv[index])) {
            sample_printf("sample use 480p60 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_480P_60;
            break;
        } else if (strcasestr("576p50", argv[index])) {
            sample_printf("sample use 576p60 output\n");
            g_default_fmt = HI_UNF_ENC_FMT_576P_50;
            break;
        } else if (strcasestr("ntsc", argv[index])) {
            sample_printf("sample use ntsc output\n");
            g_default_fmt = HI_UNF_ENC_FMT_NTSC;
            break;
        } else if (strcasestr("pal_n", argv[index])) {
            sample_printf("sample use pal_n output\n");
            g_default_fmt = HI_UNF_ENC_FMT_PAL_N;
            break;
        } else if (strcasestr("-once", argv[index])) {
            sample_printf("Play once only.\n");
            g_play_once = HI_TRUE;
        } else if (strcasestr("-fps", argv[index])) {
            frame_rate = atoi(argv[index + 1]);
            if (frame_rate < 0) {
                sample_printf("Invalid fps.\n");
                return -1;
            }
        }
    }

    hi_unf_sys_init();

    HIADP_MCE_Exit();

    ret = HI_UNF_AVPLAY_Init();
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_UNF_AVPLAY_Init failed.\n");
        goto SND_DEINIT;
    }

    if (g_aud_play) {
        ret = HIADP_Snd_Init();
        if (ret != HI_SUCCESS) {
            sample_printf("call SndInit failed.\n");
            goto VO_DEINIT;
        }

        ret = HIADP_AVPlay_RegADecLib();
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_RegisterAcodecLib failed.\n");
            goto SND_DEINIT;
        }
    }

    ret = HI_UNF_AVPLAY_GetDefaultConfig(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_ES);

    ret |= HI_UNF_AVPLAY_Create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_UNF_AVPLAY_Create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = HI_UNF_AVPLAY_GetAttr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &av_sync_attr);
    av_sync_attr.enSyncRef = HI_UNF_SYNC_REF_NONE;
    ret |= HI_UNF_AVPLAY_SetAttr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &av_sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
        goto AVPLAY_DEINIT;
    }

    if (g_aud_play) {
        ret = HI_UNF_AVPLAY_ChnOpen(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
            goto AVPLAY_VSTOP;
        }

        ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_SND_GetDefaultTrackAttr failed.\n");
            goto ACHN_CLOSE;
        }
        ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &track_attr, &track);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_SND_CreateTrack failed.\n");
            goto ACHN_CLOSE;
        }

        ret = HI_UNF_SND_Attach(track, avplay);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_SND_Attach failed.\n");
            goto TRACK_DESTROY;
        }
        ret = HIADP_AVPlay_SetAdecAttr(avplay, adec_type, audio_dec_mode, dts_core_only);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
            goto SND_DETACH;
        }

        ret = HI_UNF_AVPLAY_Start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_Start failed.\n");
        }
    }

    ret = hi_adp_disp_init(g_default_fmt);
    if (ret != HI_SUCCESS) {
        sample_printf("call DispInit failed.\n");
        goto SYS_DEINIT;
    }

    if (g_vid_play) {
        ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_NORMAL);
        ret |= HIADP_VO_CreatWin(HI_NULL, &win);
        if (ret != HI_SUCCESS) {
            sample_printf("call VoInit failed.\n");
            HIADP_VO_DeInit();
            goto DISP_DEINIT;
        }
    }

    if (g_vid_play) {
        HI_UNF_AVPLAY_OPEN_OPT_S *pMaxCapbility = HI_NULL;
        HI_UNF_AVPLAY_OPEN_OPT_S stMaxCapbility;

        if (vdec_type == HI_UNF_VCODEC_TYPE_MVC) {
            stMaxCapbility.enCapLevel = HI_UNF_VCODEC_CAP_LEVEL_FULLHD;
            stMaxCapbility.enDecType = HI_UNF_VCODEC_DEC_TYPE_BUTT;
            stMaxCapbility.enProtocolLevel = HI_UNF_VCODEC_PRTCL_LEVEL_MVC;

            pMaxCapbility = &stMaxCapbility;
        }

        ret = HI_UNF_AVPLAY_ChnOpen(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, pMaxCapbility);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
            goto AVPLAY_DESTROY;
        }

        /* set compress attr */
        HI_UNF_VCODEC_ATTR_S vcodec_attr;
        ret = HI_UNF_AVPLAY_GetAttr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vcodec_attr);

        if (vdec_type == HI_UNF_VCODEC_TYPE_VC1) {
            vcodec_attr.unExtAttr.stVC1Attr.bAdvancedProfile = advanced_profil;
            vcodec_attr.unExtAttr.stVC1Attr.u32CodecVersion = codec_version;
        }

        if (vdec_type == HI_UNF_VCODEC_TYPE_VP6) {
            vcodec_attr.unExtAttr.stVP6Attr.bReversed = 0;
        }

        vcodec_attr.enType = vdec_type;

        ret |= HI_UNF_AVPLAY_SetAttr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vcodec_attr);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
            goto AVPLAY_DEINIT;
        }
        ret = HI_UNF_VO_AttachWindow(win, avplay);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_VO_Attacwindow failed.\n");
            goto VCHN_CLOSE;
        }

        ret = HI_UNF_VO_SetWindowEnable(win, HI_TRUE);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_VO_SetWindowEnable failed.\n");
            goto WIN_DETATCH;
        }

        ret = HIADP_AVPlay_SetVdecAttr(avplay, vdec_type, HI_UNF_VCODEC_MODE_NORMAL);
        if (ret != HI_SUCCESS) {
            sample_printf("call HIADP_AVPlay_SetVdecAttr failed.\n");
            goto WIN_DETATCH;
        }

        if (frame_rate != 0) {
            HI_UNF_AVPLAY_FRMRATE_PARAM_S frame_param;
            frame_param.enFrmRateType = HI_UNF_AVPLAY_FRMRATE_TYPE_USER;
            frame_param.stSetFrmRate.u32fpsInteger = frame_rate;
            frame_param.stSetFrmRate.u32fpsDecimal = 0;
            HI_UNF_AVPLAY_SetAttr(avplay, HI_UNF_AVPLAY_ATTR_ID_FRMRATE_PARAM, &frame_param);
        }

        ret = HI_UNF_AVPLAY_Start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_AVPLAY_Start failed.\n");
            goto WIN_DETATCH;
        }
    }

    if (g_vid_es_file == NULL) {
        HI_UNF_DISP_BG_COLOR_S bg_color;

        bg_color.u8Red = 0;
        bg_color.u8Green = 200;
        bg_color.u8Blue = 200;
        HI_UNF_DISP_SetBgColor(HI_UNF_DISPLAY1, &bg_color);
    }

    g_stop_thread = HI_FALSE;
    pthread_create(&g_es_thd, HI_NULL, es_thread, (hi_void *)&avplay);

    while (1) {
        printf("please input the q to quit!, s to toggle spdif pass-through, h to toggle hdmi pass-through\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        if (input_cmd[0] == 'a' && input_cmd[1] == '1') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_FREELY;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY0, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'a' && input_cmd[1] == '2') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_NO_MORE;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY0, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'a' && input_cmd[1] == '3') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_ONCE;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY0, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'a' && input_cmd[1] == '4') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_NEVER;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY0, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'a' && input_cmd[1] == '0') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_FALSE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_NEVER;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY0, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'b' && input_cmd[1] == '1') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_FREELY;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY1, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'b' && input_cmd[1] == '2') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_NO_MORE;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY1, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'b' && input_cmd[1] == '3') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_ONCE;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY1, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'b' && input_cmd[1] == '4') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_TRUE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_NEVER;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY1, &cgms_cfg);

            continue;
        }

        if (input_cmd[0] == 'b' && input_cmd[1] == '0') {
            HI_UNF_DISP_CGMS_CFG_S cgms_cfg;

            cgms_cfg.bEnable = HI_FALSE;
            cgms_cfg.enType = HI_UNF_DISP_CGMS_TYPE_A;
            cgms_cfg.enMode = HI_UNF_DISP_CGMS_MODE_COPY_NEVER;

            HI_UNF_DISP_SetCgms(HI_UNF_DISPLAY1, &cgms_cfg);

            continue;
        }

        if ((input_cmd[0] == 's') || (input_cmd[0] == 'S')) {
            static int spdif_toggle = 0;
            spdif_toggle++;
            if (spdif_toggle & 1) {
                HI_UNF_SND_SetSpdifMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_SPDIF0, HI_UNF_SND_SPDIF_MODE_RAW);
                sample_printf("spdif pass-through on!\n");
            } else {
                HI_UNF_SND_SetSpdifMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_SPDIF0, HI_UNF_SND_SPDIF_MODE_LPCM);
                sample_printf("spdif pass-through off!\n");
            }
        }

        if (input_cmd[0] == 'h' || input_cmd[0] == 'H') {
            static int hdmi_toggle = 0;
            hdmi_toggle++;
            if (hdmi_toggle & 1) {
                HI_UNF_SND_SetHdmiMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_HDMI0, HI_UNF_SND_HDMI_MODE_RAW);
                printf("hmdi pass-through on!\n");
            } else {
                HI_UNF_SND_SetHdmiMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_HDMI0, HI_UNF_SND_HDMI_MODE_LPCM);
                printf("hmdi pass-through off!\n");
            }
        }
    }

    g_stop_thread = HI_TRUE;
    pthread_join(g_es_thd, HI_NULL);

    if (g_aud_play) {
        stop.u32TimeoutMs = 0;
        stop.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        HI_UNF_AVPLAY_Stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
    }

SND_DETACH:
    if (g_aud_play) {
        HI_UNF_SND_Detach(track, avplay);
    }

TRACK_DESTROY:
    if (g_aud_play) {
        HI_UNF_SND_DestroyTrack(track);
    }

ACHN_CLOSE:
    if (g_aud_play) {
        HI_UNF_AVPLAY_ChnClose(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    }

AVPLAY_VSTOP:
    if (g_vid_play) {
        stop.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop.u32TimeoutMs = 0;
        HI_UNF_AVPLAY_Stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop);
    }

WIN_DETATCH:
    if (g_vid_play) {
        HI_UNF_VO_SetWindowEnable(win, HI_FALSE);
        HI_UNF_VO_DetachWindow(win, avplay);
    }

VCHN_CLOSE:
    if (g_vid_play) {
        HI_UNF_AVPLAY_ChnClose(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    }

AVPLAY_DESTROY:
    HI_UNF_AVPLAY_Destroy(avplay);

AVPLAY_DEINIT:
    HI_UNF_AVPLAY_DeInit();

SND_DEINIT:
    if (g_aud_play) {
        HIADP_Snd_DeInit();
    }

VO_DEINIT:
    if (g_vid_play) {
        HI_UNF_VO_DestroyWindow(win);
        HIADP_VO_DeInit();
    }

DISP_DEINIT:
    hi_adp_disp_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();
    if (g_vid_play) {
        fclose(g_vid_es_file);
        g_vid_es_file = HI_NULL;
    }

    if (g_aud_play) {
        fclose(g_aud_es_file);
        g_aud_es_file = HI_NULL;
    }

    return 0;
}
