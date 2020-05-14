/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_dvb_pip.c
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
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#define AVPLAY_NUM  2
pmt_compact_tbl    *g_prog_table = HI_NULL;

#define PLAY_DEMUX_ID 0
#define PLAY_TUNER_ID 0

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                      ret;
    hi_s32                      i;
    hi_handle                   win[AVPLAY_NUM];
    hi_handle                   avplay[AVPLAY_NUM];
    hi_unf_avplay_attr          avplay_attr;
    hi_unf_sync_attr            sync_attr;
    hi_char                     input_cmd[32];
    hi_unf_video_format         video_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32                      prog_num;
    hi_unf_video_rect           child_rect;
    hi_u32                      tuner_freq;
    hi_u32                      tuner_srate;
    hi_u32                      third_param;
    hi_unf_avplay_stop_opt      stop_opt;
    hi_bool                     is_aud_mix = 0;
    hi_bool                     is_aud_play[AVPLAY_NUM];
    hi_handle                   track[AVPLAY_NUM];
    hi_unf_audio_track_attr     track_attr;
    hi_unf_snd_gain             sound_gain;
    hi_u32                      ts_pid = INVALID_TSPID;

    sound_gain.linear_mode = HI_TRUE;

    if (argc < 2) {
        printf("Usage:\n%s freq [srate] [qamtype or polarization] [audiomix] [vo_format]\n", argv[0]);
        printf("\tqamtype or polarization:\n");
        printf("\t\tFor cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64]\n");
        printf("\t\tFor satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0]\n");
        printf("\taudiomix\t: 0: not mix audio, 1: mix audio\n");
        printf("\tvo_format\t: 2160P60 2160P50 2160P30 2160P25 2160P24 1080P60 1080P50 1080i60 1080i50 720P60 720P50\n\n");
        printf("Example:%s 610 6875 64 0\n", argv[0]);
        return HI_FAILURE;
    }

    tuner_freq   = strtol(argv[1], NULL, 0);
    tuner_srate  = (tuner_freq > 1000) ? 27500 : 6875;
    third_param  = (tuner_freq > 1000) ? 0 : 64;

    if (argc > 2) {
        tuner_srate = strtol(argv[2], NULL, 0);
    }

    if (argc > 3) {
        third_param = strtol(argv[3], NULL, 0);
    }

    if (argc > 4) {
        is_aud_mix = strtol(argv[4], NULL, 0);
    }

    if (argc > 5) {
        video_fmt = hi_adp_str2fmt(argv[5]);
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(video_fmt);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    hi_handle *win_handle = &win[0];
    ret = hi_adp_win_create(HI_NULL, win_handle);
    child_rect.width = 720;
    child_rect.height = 576;
    child_rect.x = 0;
    child_rect.y = 0;
    win_handle = &win[1];
    ret |= hi_adp_win_create(&child_rect, win_handle);
    if (ret != HI_SUCCESS) {
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_init();
    ret |= hi_adp_dmx_attach_ts_port(PLAY_DEMUX_ID, PLAY_TUNER_ID);
    if (ret != HI_SUCCESS) {
        printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        printf("call HIADP_Demux_Init failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_frontend_connect(PLAY_TUNER_ID, tuner_freq, tuner_srate, third_param);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DEMUX_ID, &g_prog_table);
    if (ret != HI_SUCCESS) {
        printf("call HIADP_Search_GetAllPmt failed\n");
        goto PSISI_FREE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    for (i = 0; i < AVPLAY_NUM; i++) {
        printf("===============avplay[%d]===============\n",i);

        hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
        avplay_attr.demux_id = PLAY_DEMUX_ID;
        ret = hi_unf_avplay_create(&avplay_attr, &avplay[i]);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_avplay_create failed.\n");
            goto AVPLAY_STOP;
        }

        ret = hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_STOP;
        }

        ret = hi_unf_win_attach_src(win[i], avplay[i]);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_win_attach_src failed.\n");
            goto AVPLAY_STOP;
        }
        ret = hi_unf_win_set_enable(win[i], HI_TRUE);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_win_set_enable failed.\n");
            goto AVPLAY_STOP;
        }

        is_aud_play[i] = HI_FALSE;
        if (i == 0) {
            is_aud_play[i] = HI_TRUE;

            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_snd_get_default_track_attr failed.\n");
            }
            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track[i]);
            ret |= hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            ret |= hi_unf_snd_attach(track[i], avplay[i]);
            if (ret != HI_SUCCESS) {
                printf("call HI_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }
        else if(i == 1 && is_aud_mix == HI_TRUE) {
            is_aud_play[i] = HI_TRUE;

            track_attr.track_type = HI_UNF_SND_TRACK_TYPE_SLAVE;
            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_snd_get_default_track_attr failed.\n");
            }
            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track[i]);
            sound_gain.gain = 60;
            ret = hi_unf_snd_set_track_weight(track[i], &sound_gain);
            ret |= hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            ret |= hi_unf_snd_attach(track[i], avplay[i]);
            if (ret != HI_SUCCESS) {
                printf("call HI_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }

        ret = hi_unf_avplay_get_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
        ret = hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_avplay_set_attr failed.\n");
            goto AVPLAY_STOP;
        }

        prog_num = 0;
        ret = hi_adp_avplay_play_prog(avplay[i], g_prog_table, prog_num + i,
            is_aud_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        if (ret != HI_SUCCESS) {
            printf("call HIADP_AVPlay_SwitchProg failed\n");
        }
    }

    while (1) {
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        prog_num = atoi(input_cmd);

        for (i = 0; i < AVPLAY_NUM; i++)
        {
            stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
            stop_opt.timeout = 0;
            ret = hi_unf_avplay_stop(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_avplay_stop failed.\n");
                break;
            }

            (hi_void)hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_PCR_PID, &ts_pid);
            (hi_void)hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_VID_PID, &ts_pid);

            if (HI_TRUE == is_aud_play[i]) {
                (hi_void)hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &ts_pid);
            }
        }

        for (i = 0; i < AVPLAY_NUM; i++) {
            ret = hi_adp_avplay_play_prog(avplay[i], g_prog_table, prog_num + i,
                is_aud_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
            if (ret != HI_SUCCESS) {
                printf("call SwitchProgfailed!\n");
                break;
            }
        }
    }

AVPLAY_STOP:
    for(i = 0; i < AVPLAY_NUM; i++) {
        stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop_opt.timeout = 0;
        if (is_aud_mix == HI_FALSE && i == 1) {
            hi_unf_avplay_stop(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop_opt);
        } else {
            hi_unf_avplay_stop(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
            hi_unf_snd_detach(track[i], avplay[i]);
            hi_unf_snd_destroy_track(track[i]);
        }
        hi_unf_win_set_enable(win[i], HI_FALSE);
        hi_unf_win_detach_src(win[i], avplay[i]);
        if (is_aud_mix == HI_FALSE && i == 1) {
            hi_unf_avplay_chan_close(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        } else {
            hi_unf_avplay_chan_close(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }
        hi_unf_avplay_destroy(avplay[i]);
    }
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_table);
    g_prog_table = HI_NULL;
    hi_adp_search_de_init();

TUNER_DEINIT:
    hi_adp_frontend_deinit();

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(PLAY_DEMUX_ID);
    hi_unf_dmx_deinit();

VO_DEINIT:
    hi_unf_win_destroy(win[1]);
    hi_unf_win_destroy(win[0]);
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return ret;
}

