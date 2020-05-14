/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : sample_macrovision.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   :
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#if CONFING_SUPPORT_AUDIO
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "HA.AUDIO.DRA.decode.h"
#include "hi_unf_acodec_pcmdec.h"
#include "HA.AUDIO.WMA9STD.decode.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_amrwb.h"
#include "hi_codec_dolbytruehddec.h"
#include "hi_unf_acodec_truehdpassthrough.h"
#include "hi_unf_acodec_dtshddec.h"
#include "hi_unf_acodec_ac3passthrough.h"
#include "hi_unf_acodec_dtspassthrough.h"
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
#include "hi_codec_dolbyplusdec.h"
#endif
#endif

#include "hi_adp_mpi.h"

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

static hi_bool g_read_frame_size = HI_FALSE;

hi_s32 g_aud_es_file_offest;

hi_void cgms_put_vid_stream(hi_handle avplay, hi_bool *vid_buf_avail)
{
    hi_u32 frame_size = 0;
    hi_unf_stream_buf stream_buf;
    hi_u32 read_len;
    hi_s32 ret;

    if (g_read_frame_size != HI_FALSE) {
        ret = hi_unf_avplay_get_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, 0x100000, &stream_buf, 0);
    } else {
        ret = hi_unf_avplay_get_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, 0x4000, &stream_buf, 0);
    }

    if (ret == HI_SUCCESS) {
        *vid_buf_avail = HI_TRUE;

        if (g_read_frame_size != HI_FALSE) {
            read_len = fread(&frame_size, 1, 4, g_vid_es_file);
            if (read_len == 4) {
            } else {
                frame_size = 0x4000;
            }
        } else {
            frame_size = 0x4000;
        }

        read_len = fread(stream_buf.data, sizeof(hi_s8), frame_size, g_vid_es_file);

        if (read_len > 0) {
            ret = hi_unf_avplay_put_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, read_len, 0, HI_NULL);
            if (ret != HI_SUCCESS) {
                sample_printf("call hi_unf_avplay_put_buf failed.\n");
            }

            frame_size = 0;
        } else if (read_len == 0) {
            sample_printf("read vid file end and rewind!\n");
            rewind(g_vid_es_file);
        } else {
            perror("read vid file error\n");
        }
    } else {
        *vid_buf_avail = HI_FALSE;
    }
}

hi_void cgms_put_audio_stream(hi_handle avplay, hi_bool *aud_buf_avail)
{
    static int counter = 0;
    hi_unf_stream_buf stream_buf;
    hi_u32 read_len;
    hi_s32 ret;

    ret = hi_unf_avplay_get_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, 0x1000, &stream_buf, 0);
    if (ret == HI_SUCCESS) {
        *aud_buf_avail = HI_TRUE;
        read_len = fread(stream_buf.data, sizeof(hi_s8), 0x1000, g_aud_es_file);
        if (read_len > 0) {
            counter++;
            ret = hi_unf_avplay_put_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, read_len, 0, HI_NULL);
            if (ret != HI_SUCCESS) {
                sample_printf("call hi_unf_avplay_put_buf failed.\n");
            }
        } else if (read_len == 0) {
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


hi_void es_thread(hi_void *args)
{
    hi_handle avplay;
    hi_bool vid_buf_avail = HI_FALSE;
    hi_bool aud_buf_avail = HI_FALSE;

    avplay = *((hi_handle *)args);


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

    return;
}

static void set_disp_format(hi_unf_video_format video_disp0_format, hi_unf_video_format video_disp1_format)
{
    const hi_unf_disp disp_id[2] = {HI_UNF_DISPLAY0, HI_UNF_DISPLAY1};
    hi_unf_disp_format format[2] = {0};

    hi_unf_disp_format *disp0_format = &format[0];
    (void)hi_unf_disp_get_disp_format(HI_UNF_DISPLAY0, disp0_format);
    hi_unf_disp_format *disp1_format = &format[1];
    (void)hi_unf_disp_get_disp_format(HI_UNF_DISPLAY1, disp1_format);

    disp0_format->video_format = video_disp0_format;
    disp1_format->video_format = video_disp1_format;
    hi_unf_disp_set_disp_format(disp_id, format, 2);
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret, index;
    hi_handle win;
    hi_handle track;
    hi_unf_audio_track_attr track_attr;

    hi_unf_vcodec_type vdec_type = HI_UNF_VCODEC_TYPE_MAX;
    hi_u32 adec_type = 0;
    hi_handle avplay;
    hi_unf_avplay_attr avplay_attr;
    hi_unf_sync_attr av_sync_attr;
    hi_unf_avplay_stop_opt stop;
    hi_char input_cmd[32];
    hi_unf_acodec_decode_mode audio_dec_mode = HI_UNF_ACODEC_DEC_MODE_RAWPCM;
    hi_unf_video_format g_default_fmt = HI_UNF_VIDEO_FMT_720P_50;
    hi_bool advanced_profil = 1;
    hi_u32 codec_version = 8;

    if (argc < 5) {
#if 1
        printf(" usage: sample_esplay vfile vtype afile atype -[options] \n");
        printf(" vtype: h264/mpeg2/mpeg4/avs/real8/real9/vc1ap/vc1smp5(WMV3)/vc1smp8/vp6/vp6f/vp6a/vp8/divx3/h263/sor\n");
        printf(" atype: aac/mp3/dts/dra/mlp/pcm/ddp(ac3/eac3)\n");
        printf(" options:\n"
               " -mode0,   audio decode mode: pcm decode \n"
               " -mode1,   audio decode mode: raw_stream decode \n"
               " -mode2,   audio decode mode: pcm+raw_stream decode \n"
               " 1080p50 or 1080p60  video output at 1080p50 or 1080p60\n");
        printf(" examples: \n");
        printf("   sample_esplay vfile h264 null null\n");
        printf("   sample_esplay null null afile mp3 \n");
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
        } else if (!strcasecmp("h264", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_H264;
        } else if (!strcasecmp("h265", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_H265;
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
        } else if (!strcasecmp("mvc", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_H264_MVC;
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
            vdec_type = HI_UNF_VCODEC_TYPE_H264_MVC;
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
                audio_dec_mode = HI_UNF_ACODEC_DEC_MODE_RAWPCM;
            } else if (!strcasecmp("-mode1", argv[5])) {
                audio_dec_mode = HI_UNF_ACODEC_DEC_MODE_THRU;
            } else if (!strcasecmp("-mode2", argv[5])) {
                audio_dec_mode = HI_UNF_ACODEC_DEC_MODE_SIMUL;
            }
            (void)audio_dec_mode;
        }

        g_aud_play = HI_TRUE;
        if (!strcasecmp("aac", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AAC;
        } else if (!strcasecmp("mp3", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_MP3;
        } else if (!strcasecmp("truehd", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DOLBY_TRUEHD;
        } else if (!strcasecmp("ac3raw", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AC3PASSTHROUGH;
        } else if (!strcasecmp("dtsraw", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DTSPASSTHROUGH;
        }
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
        else if (!strcasecmp("ddp", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DOLBY_PLUS;
        }
#endif
        else if (!strcasecmp("dts", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DTSHD;
        } else if (!strcasecmp("dra", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DRA;
        } else if (!strcasecmp("pcm", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_PCM;
        } else if (!strcasecmp("mlp", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_TRUEHD;
        } else if (!strcasecmp("amr", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AMRNB;
        } else if (!strcasecmp("amrwb", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AMRWB;
        } else {
            sample_printf("unsupport aud codec type!\n");
            return -1;
        }
    }

    for (index = 0; index < argc; index++) {
        if (strcasestr("1080p50", argv[index])) {
            sample_printf("ESPlay use 1080p50 output\n");
            g_default_fmt = HI_UNF_VIDEO_FMT_1080P_50;
            break;
        } else if (strcasestr("1080p60", argv[index])) {
            sample_printf("ESPlay use 1080p60 output\n");
            g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
            break;
        }
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto SND_DEINIT;
    }

    if (g_aud_play) {
        ret = hi_adp_snd_init();
        if (ret != HI_SUCCESS) {
            sample_printf("call SndInit failed.\n");
            goto VO_DEINIT;
        }
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_ES);
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &av_sync_attr);
    av_sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &av_sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto AVPLAY_DEINIT;
    }

    if (g_aud_play) {
        ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_VSTOP;
        }

        ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
            goto ACHN_CLOSE;
        }
        ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_snd_create_track failed.\n");
            goto ACHN_CLOSE;
        }

        ret = hi_unf_snd_attach(track, avplay);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_snd_attach failed.\n");
            goto TRACK_DESTROY;
        }
        ret = hi_adp_avplay_set_adec_attr(avplay, adec_type);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_set_attr failed.\n");
            goto SND_DETACH;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_start failed.\n");
        }
    }

    ret = hi_adp_disp_init(g_default_fmt);
    if (ret != HI_SUCCESS) {
        sample_printf("call DispInit failed.\n");
        goto SYS_DEINIT;
    }

    if (g_vid_play) {
        ret = hi_adp_win_init();
        ret |= hi_adp_win_create(HI_NULL, &win);
        if (ret != HI_SUCCESS) {
            sample_printf("call VoInit failed.\n");
            hi_adp_win_deinit();
            goto DISP_DEINIT;
        }
    }

    if (g_vid_play) {
        ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_DESTROY;
        }

        /* set compress attr */
        hi_unf_vdec_attr vcodec_attr;
        ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vcodec_attr);

        if (HI_UNF_VCODEC_TYPE_VC1 == vdec_type) {
            vcodec_attr.ext_attr.vc1.advanced_profile = advanced_profil;
            vcodec_attr.ext_attr.vc1.codec_version = codec_version;
        }

        if (HI_UNF_VCODEC_TYPE_VP6 == vdec_type) {
            vcodec_attr.ext_attr.vp6.reverse = 0;
        }

        vcodec_attr.type = vdec_type;

        ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vcodec_attr);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_set_attr failed.\n");
            goto AVPLAY_DEINIT;
        }
        ret = hi_unf_win_attach_src(win, avplay);
        if (ret != HI_SUCCESS) {
            sample_printf("call HI_UNF_VO_Attacwindow failed.\n");
            goto VCHN_CLOSE;
        }

        ret = hi_unf_win_set_enable(win, HI_TRUE);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_win_set_enable failed.\n");
            goto WIN_DETATCH;
        }

        ret = hi_adp_avplay_set_vdec_attr(avplay, vdec_type, HI_UNF_VDEC_WORK_MODE_NORMAL);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_adp_avplay_set_vdec_attr failed.\n");
            goto WIN_DETATCH;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_start failed.\n");
            goto WIN_DETATCH;
        }
    }

    if (g_vid_es_file == NULL) {
        hi_unf_disp_color bg_color;

        bg_color.red = 0;
        bg_color.green = 200;
        bg_color.blue = 200;
        hi_unf_disp_set_bg_color(HI_UNF_DISPLAY1, &bg_color);
    }

    g_stop_thread = HI_FALSE;
    pthread_create(&g_es_thd, HI_NULL, (hi_void *)es_thread, (hi_void *)&avplay);

    while (1) {
        printf("please input the q to quit!, s to toggle spdif pass-through, h to toggle hdmi pass-through\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        if (input_cmd[0] == 'a') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_50);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE0, HI_NULL);
            printf("Format PAL  Macvision Type 0\n");
            continue;
        }

        if (input_cmd[0] == 'b') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_50);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE1, HI_NULL);
            printf("Format PAL  Macvision Type 1\n");
            continue;
        }

        if (input_cmd[0] == 'c') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_50);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE2, HI_NULL);
            printf("Format PAL  Macvision Type 2\n");
            continue;
        }

        if (input_cmd[0] == 'd') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_50);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE3, HI_NULL);

            printf("Format PAL  Macvision Type 3\n");
            continue;
        }

        if (input_cmd[0] == 'e') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_60);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE0, HI_NULL);

            printf("Format NTSC  Macvision Type 0\n");
            continue;
        }
        if (input_cmd[0] == 'f') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_60);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE1, HI_NULL);
            printf("Format NTSC  Macvision Type 1\n");
            continue;
        }
        if (input_cmd[0] == 'g') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_60);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE2, HI_NULL);
            printf("Format NTSC  Macvision Type 2\n");
            continue;
        }
        if (input_cmd[0] == 'h') {
            set_disp_format(HI_UNF_VIDEO_FMT_PAL, HI_UNF_VIDEO_FMT_720P_60);
            hi_unf_disp_set_macrovision(HI_UNF_DISPLAY0, HI_UNF_DISP_MACROVISION_MODE_TYPE3, HI_NULL);
            printf("Format NTSC  Macvision Type 3\n");
            continue;
        }
        /*
        if ('3' == input_cmd[0] && 'd' == input_cmd[1]  )
        {
            HI_UNF_DISP_Set3DMode(HI_UNF_DISPLAY1,
            HI_UNF_DISP_3D_FRAME_PACKING,
            HI_UNF_ENC_FMT_1080P_24_FRAME_PACKING);
        }
 */
        if ((input_cmd[0] == 's') || (input_cmd[0] == 'S')) {
            static int spdif_toggle = 0;
            spdif_toggle++;
            if (spdif_toggle & 1) {
                hi_unf_snd_set_spdif_scms_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0,  HI_UNF_SND_OUTPUT_MODE_RAW);
                sample_printf("spdif pass-through on!\n");
            } else {
                hi_unf_snd_set_spdif_scms_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0, HI_UNF_SND_OUTPUT_MODE_LPCM);
                sample_printf("spdif pass-through off!\n");
            }
        }

#if 0
        if (input_cmd[0] == 'h' || input_cmd[0] == 'H') {
            static int hdmi_toggle =0;
            hdmi_toggle++;
            if (hdmi_toggle&1) {
                HI_UNF_SND_SetHdmiMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_HDMI0, HI_UNF_SND_HDMI_MODE_RAW);
                printf("hmdi pass-through on!\n");
            }
            else {
                HI_UNF_SND_SetHdmiMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_HDMI0, HI_UNF_SND_HDMI_MODE_LPCM);
                printf("hmdi pass-through off!\n");
            }
        }
#endif
    }

    g_stop_thread = HI_TRUE;
    pthread_join(g_es_thd, HI_NULL);

    if (g_aud_play) {
        stop.timeout = 0;
        stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
    }

SND_DETACH:
    if (g_aud_play) {
        hi_unf_snd_detach(track, avplay);
    }

TRACK_DESTROY:
    if (g_aud_play) {
        hi_unf_snd_destroy_track(track);
    }

ACHN_CLOSE:
    if (g_aud_play) {
        hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    }

AVPLAY_VSTOP:
    if (g_vid_play) {
        stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop.timeout = 0;
        hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop);
    }

WIN_DETATCH:
    if (g_vid_play) {
        hi_unf_win_set_enable(win, HI_FALSE);
        hi_unf_win_detach_src(win, avplay);
    }

VCHN_CLOSE:
    if (g_vid_play) {
        hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    }

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

SND_DEINIT:
    if (g_aud_play) {
        hi_adp_snd_deinit();
    }

VO_DEINIT:
    if (g_vid_play) {
        hi_unf_win_destroy(win);
        hi_adp_win_deinit();
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
