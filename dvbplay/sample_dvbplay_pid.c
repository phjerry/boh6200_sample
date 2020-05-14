/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : dvbplay.c
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

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"

#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
//#include "HA.AUDIO.DRA.decode.h"
#include "hi_unf_acodec_pcmdec.h"
//#include "HA.AUDIO.WMA9STD.decode.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_truehdpassthrough.h"
#include "hi_unf_acodec_ac3passthrough.h"
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
#include "hi_unf_acodec_dolbyplusdec.h"
#endif
#include "hi_unf_acodec_dtshddec.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

pmt_compact_tbl    *g_prog_tbl = HI_NULL;

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32                      ret;
    hi_handle                   win_handle;

    hi_handle                   avplay_handle;
    hi_unf_sync_attr          sync_attr;
    hi_unf_avplay_attr          avplay_attr;
    hi_unf_avplay_stop_opt    stop;
    hi_char                     input_cmd[32];
    hi_unf_video_format            vid_format = HI_UNF_VIDEO_FMT_1080P_50;
    hi_unf_vcodec_type        vdec_type = HI_UNF_VCODEC_TYPE_MAX;
    hi_u32                      adec_type = 0;
    hi_u32                      vid_pid = 0;
    hi_u32                      aud_pid = 0;
    hi_bool                     is_aud_play = HI_FALSE;
    hi_bool                     is_vid_play = HI_FALSE;
    hi_u32                      prog_num;
    hi_u32                      tuner_freq = 0;
    hi_u32                      tuner_srate = 0;
    hi_u32                      third_param = 0;
    pmt_compact_tbl             prog_table;
    pmt_compact_prog            prog_info;

    hi_handle track_handle;
    hi_unf_audio_track_attr track_attr;
    hi_u32                      tuner = 0;

    if (argc < 6 || argc > 10) {
        printf("Usage: %s [vpid] [vtype] [apid] [atype] [freq] [srate] [qamtype or polarization] [vo_format] [tuner]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50  default HI_UNF_VIDEO_FMT_1080P_50\n"
                "       vtype: mpeg2/mpeg4/h263/sor/vp6/vp6f/vp6a/h264/avs/real8/real9/vc1\n"
                "       atype: aac/mp3/dts/dra/mlp/pcm/ddp(ac3/eac3)\n"
                "       tuner: value can be 0, 1, 2, 3\n",
                argv[0]);
        printf(" examples: \n");
        printf("   %s vpid h264 \"null\" \"null\" 610 6875 64 1080p50 0\n", argv[0]);
        printf("   %s \"null\" \"null\" apid mp3 610 6875 64 1080p50 0\n", argv[0]);
        printf("   %s vpid h264 apid mp3 610 6875 64 1080p50 0\n", argv[0]);
        return HI_FAILURE;
    }

    if (argc == 10) {
        tuner_freq = strtol(argv[5], NULL, 0);
        tuner_srate = strtol(argv[6], NULL, 0);
        third_param = strtol(argv[7], NULL, 0);
        vid_format = hi_adp_str2fmt(argv[8]);
        tuner = strtol(argv[9], NULL, 0);
    } else if (argc == 9) {
        tuner_freq  = strtol(argv[5], NULL, 0);
        tuner_srate = strtol(argv[6], NULL, 0);
        third_param = strtol(argv[7], NULL, 0);
        vid_format = hi_adp_str2fmt(argv[8]);
        tuner = 0;
    } else if (argc == 8) {
        tuner_freq  = strtol(argv[5], NULL, 0);
        tuner_srate = strtol(argv[6], NULL, 0);
        third_param = strtol(argv[7], NULL, 0);
        vid_format = HI_UNF_VIDEO_FMT_1080P_50;
        tuner = 0;
    } else if(argc == 7) {
        tuner_freq  = strtol(argv[5], NULL, 0);
        tuner_srate = strtol(argv[6], NULL, 0);
        third_param = (tuner_freq > 1000) ? 0 : 64;
        vid_format = HI_UNF_VIDEO_FMT_1080P_50;
        tuner = 0;
    } else if(argc == 6) {
        tuner_freq  = strtol(argv[5], NULL, 0);
        tuner_srate = (tuner_freq > 1000) ? 27500 : 6875;
        third_param = (tuner_freq > 1000) ? 0 : 64;
        vid_format = HI_UNF_VIDEO_FMT_1080P_50;
        tuner = 0;
    }

    if (strcasecmp("null", argv[1])) {
        is_vid_play = HI_TRUE;
        vid_pid = strtol(argv[1], NULL, 0);
        if (!strcasecmp("mpeg2", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_MPEG2;
        } else if (!strcasecmp("mpeg4", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_MPEG4;
        } else if (!strcasecmp("h263", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_H263;
        } else if (!strcasecmp("sor", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_SORENSON;
        } else if (!strcasecmp("vp6", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_VP6;
        } else if (!strcasecmp("vp6f", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_VP6F;
        } else if (!strcasecmp("vp6a", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_VP6A;
        } else if (!strcasecmp("h264", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_H264;
        } else if (!strcasecmp("avs", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_AVS;
        } else if (!strcasecmp("real8", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_REAL8;
        } else if (!strcasecmp("real9", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_REAL9;
        } else if (!strcasecmp("vc1", argv[2])) {
            vdec_type = HI_UNF_VCODEC_TYPE_VC1;
        } else {
            sample_printf("unsupport vid codec type!\n");
            return -1;
        }
    }

    if (strcasecmp("null", argv[3])) {
        is_aud_play = HI_TRUE;
        aud_pid = strtol(argv[3], NULL, 0);
        if (!strcasecmp("aac", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AAC;
        } else if (!strcasecmp("mp3", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_MP3;
        }
        #if defined(DOLBYPLUS_HACODEC_SUPPORT)
        else if (!strcasecmp("ddp", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DOLBY_PLUS;
        }
        #endif
        else if (!strcasecmp("ac3", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AC3PASSTHROUGH;
        } else if (!strcasecmp("dts", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DTSHD;
        } else if (!strcasecmp("dra", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_DRA;
        } else if (!strcasecmp("pcm", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_PCM;
        } else if (!strcasecmp("mlp", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_TRUEHD;
        } else if (!strcasecmp("amr", argv[4])) {
            adec_type = HI_UNF_ACODEC_ID_AMRNB;
        } else {
            sample_printf("unsupport aud codec type!\n");
            return -1;
        }
    }
    if (!(is_vid_play || is_aud_play)) {
        return 0;
    }

    hi_unf_sys_init();
    //hi_adp_mce_exit();
    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }
    /*HI_SYS_GetPlayTime(& u32Playtime);
    sample_printf("u32Playtime = %d\n",u32Playtime);*/
    ret = hi_adp_disp_init(vid_format);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    ret |= hi_adp_dmx_attach_ts_port(0, tuner);
    if (ret != HI_SUCCESS) {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }

    /* to solve compile err if not define HI_FRONTEND_SUPPORT */
    HI_UNUSED(tuner_srate);
    HI_UNUSED(third_param);

#ifdef HI_FRONTEND_SUPPORT
    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_frontend_connect(tuner, tuner_freq, tuner_srate, third_param);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }
#endif

    hi_adp_search_init();

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.vid_buf_size = 0x300000;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    ret |= hi_unf_avplay_chan_open(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto CHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win_handle, avplay_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_attach_src failed.\n");
        goto CHN_CLOSE;
    }
    ret = hi_unf_win_set_enable(win_handle, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track_handle, avplay_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay_handle, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    sync_attr.start_region.vid_plus_time = 100;
    sync_attr.start_region.vid_negative_time = -100;
    sync_attr.quick_output_enable = HI_FALSE;
    ret = hi_unf_avplay_set_attr(avplay_handle, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    memset(&prog_info, 0, sizeof(pmt_compact_prog));

    if (is_vid_play) {
        prog_info.v_element_num = 1;
        prog_info.v_element_pid = vid_pid;
        prog_info.video_type = vdec_type;
    }
    if (is_aud_play) {
        prog_info.a_element_num = 1;
        prog_info.a_element_pid = aud_pid;
        prog_info.audio_type = adec_type;
    }

    prog_table.prog_num = 1;
    prog_table.proginfo = &prog_info;
    g_prog_tbl = &prog_table;

    prog_num = 0;

    ret = hi_adp_avplay_play_prog(avplay_handle, g_prog_tbl, prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        sample_printf("call SwitchProg failed.\n");
        goto AVPLAY_STOP;
    }

    hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_LPCM);

    while (1) {
        printf("please input the q to quit!, s to toggle spdif pass-through, h to toggle hdmi pass-through,v20(set volume 20),vxx(set volume xx)\n");
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            sample_printf("prepare to quit!\n");
            break;
        }
        if ('v' == input_cmd[0]) {
            hi_unf_snd_gain snd_gain;
            snd_gain.linear_mode = HI_TRUE;

            snd_gain.gain = atoi(input_cmd + 1);
            if (snd_gain.gain > 100)
                snd_gain.gain = 100;

            printf("Volume=%d\n", snd_gain.gain);
            hi_unf_snd_set_volume(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_ALL, &snd_gain);
        }

        if ('s' == input_cmd[0] || 'S' == input_cmd[0]) {
            static int spdif_toggle =0;
            spdif_toggle++;
            if (spdif_toggle & 1) {
                hi_unf_snd_set_spdif_scms_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0,  HI_UNF_SND_OUTPUT_MODE_RAW);
                printf("spdif pass-through on!\n");
            }
            else {
                hi_unf_snd_set_spdif_scms_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0, HI_UNF_SND_OUTPUT_MODE_LPCM);
                printf("spdif pass-through off!\n");
            }
            continue;
        }

        if ('h' == input_cmd[0] || 'H' == input_cmd[0]) {
            static int hdmi_toggle =0;
            hdmi_toggle++;
            if (hdmi_toggle & 1) {
                hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_RAW);
                printf("hmdi pass-through on!\n");
            }
            else {
                hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_LPCM);
                printf("hmdi pass-through off!\n");
            }
            continue;
        }
    }

AVPLAY_STOP:
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

SND_DETACH:
    hi_unf_snd_detach(track_handle, avplay_handle);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track_handle);

WIN_DETACH:
    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, avplay_handle);

CHN_CLOSE:
    hi_unf_avplay_chan_close(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

    hi_unf_avplay_destroy(avplay_handle);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_de_init();

#ifdef HI_FRONTEND_SUPPORT
TUNER_DEINIT:
    hi_adp_frontend_deinit();

DMX_DEINIT:
#endif
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

VO_DEINIT:
    hi_unf_win_destroy(win_handle);
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return ret;
}


