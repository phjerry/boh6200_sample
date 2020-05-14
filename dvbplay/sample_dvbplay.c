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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"

#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

pmt_compact_tbl    *g_prog_tbl = HI_NULL;

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                      ret;
    hi_handle                   win;
    hi_handle                   avplay;
    hi_unf_avplay_attr          avplay_attr;
    hi_unf_sync_attr            sync_attr;
    hi_unf_avplay_stop_opt      stop;
    hi_char                     input_cmd[32];
    hi_u32                      prog_num;
    hi_handle                   track;
    hi_unf_audio_track_attr     track_attr;
    hi_u32                      tuner = 0;
    hi_unf_video_format         format = HI_UNF_VIDEO_FMT_1080P_50;

    if (argc < 2) {
        printf("Usage: %s [freq] [srate] [qamtype or polarization] [vo_format] [tuner]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50  default HI_UNF_VIDEO_FMT_1080P_50\n"
                "       Tuner: value can be 0, 1, 2, 3\n",
                argv[0]);

        printf("Example: %s 618 6875 64 1080i50 0\n", argv[0]);
        return HI_FAILURE;
    }

#ifdef HI_FRONTEND_SUPPORT
    hi_u32  tuner_freq;
    hi_u32  tuner_srate;
    hi_u32  third_para;

    tuner_freq  = strtol(argv[1], NULL, 0);
    tuner_srate = (tuner_freq > 1000) ? 27500 : 6875;
    third_para = (tuner_freq > 1000) ? 0 : 64;

    if (argc >= 3) {
        tuner_srate = strtol(argv[2], NULL, 0);
    }

    if (argc >= 4) {
        third_para = strtol(argv[3], NULL, 0);
    }
#endif

    if (argc >= 5) {
        format = hi_adp_str2fmt(argv[4]);
    }

    if (argc >= 6) {
        tuner = strtol(argv[5], NULL, 0);
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
        sample_printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    ret = hi_adp_win_create(HI_NULL, &win);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_win_create failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_dmx_attach_ts_port failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_adp_dmx_attach_ts_port(0, tuner);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_dmx_attach_ts_port failed.\n");
        goto DMX_DEINIT;
    }

#ifdef HI_FRONTEND_SUPPORT
    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_frontend_init failed.\n");
        goto DMX_DETACH;
    }

    ret = hi_adp_frontend_connect(tuner, tuner_freq, tuner_srate, third_para);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }
#endif

    hi_adp_search_init();

    ret = hi_adp_search_get_all_pmt(0,&g_prog_tbl);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_search_get_all_pmt failed\n");
        goto PSISI_FREE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = 0;
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
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_PCR;
    sync_attr.start_region.vid_plus_time = 100;
    sync_attr.start_region.vid_negative_time = -100;
    sync_attr.quick_output_enable = HI_FALSE;
    ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    prog_num = 0;
    ret = hi_adp_avplay_play_prog(avplay, g_prog_tbl, prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_play_prog failed.\n");
        goto AVPLAY_STOP;
    }

    while(1)  {
#ifdef HI_ADVCA_FUNCTION_RELEASE
        sleep(1);
        continue;
#endif
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }

        if ('s' == input_cmd[0]) {
            static int cnt = 0;

            if (cnt % 2 == 0) {
                printf("prepare to set smart volume true!\n");
                hi_unf_snd_set_avc_enable(HI_UNF_SND_0, HI_TRUE);
            } else {
                printf("prepare to set smart volume false!\n");
                hi_unf_snd_set_avc_enable(HI_UNF_SND_0, HI_FALSE);
            }
            cnt++;
        }

#ifdef HI_FRONTEND_SUPPORT
        hi_u32 signal_quality = 0;
        hi_u32 signal_strength = 0;
        hi_unf_frontend_scientific_num ber = {0};
        hi_unf_frontend_integer_decimal param = {0};

        if ('g' == input_cmd[0]) {
            hi_unf_frontend_get_ber(tuner, &ber);
            printf("\n-->BER: %dE -%d\n", ber.integer_val, ber.power);

            hi_unf_frontend_get_snr(tuner, &param);
            printf("-->SNR: %d \n", param.decimal_val);

            hi_unf_frontend_get_signal_strength(tuner, &signal_strength);
            printf("-->SSI: %d \n", signal_strength);

            hi_unf_frontend_get_signal_quality(tuner, &signal_quality);
            printf("-->SQI: %d \n\n", signal_quality);
            continue;
        }
#endif

        prog_num = atoi(input_cmd);

        if (prog_num == 0)
            prog_num = 1;

        ret = hi_adp_avplay_play_prog(avplay, g_prog_tbl, prog_num - 1, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS) {
            sample_printf("call SwitchProgfailed!\n");
            break;
        }
    }

AVPLAY_STOP:
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

SND_DETACH:
    hi_unf_snd_detach(track, avplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);

WIN_DETACH:
    hi_unf_win_set_enable(win,HI_FALSE);
    hi_unf_win_detach_src(win, avplay);

CHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_tbl);
    hi_adp_search_de_init();

#ifdef HI_FRONTEND_SUPPORT
TUNER_DEINIT:
    hi_adp_frontend_deinit();

DMX_DETACH:
#endif
    hi_unf_dmx_detach_ts_port(0);

DMX_DEINIT:
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

    return ret;
}


