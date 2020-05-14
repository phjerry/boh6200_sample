/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : dvb_mosaic.c
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
#include "hi_adp_search.h"
#include "hi_adp_frontend.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define PLAY_DEMUX_ID 0
#define PLAY_TUNER_ID 0

#define AVPLAY_NUM  4
pmt_compact_tbl    *g_prog_table = HI_NULL;

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32                   ret,i;
    hi_handle                win_handle[AVPLAY_NUM];
    hi_handle                avplay_handle[AVPLAY_NUM];
    hi_unf_avplay_attr     avplay_attr;
    hi_unf_sync_attr       sync_attr;
    hi_unf_avplay_stop_opt stop;
    hi_char                  input_cmd[32];
    hi_u32                   prog_num = 0;
    hi_unf_video_rect        win_rect;
    hi_u32                   tunner_freq;
    hi_u32                   tunner_srate;
    hi_u32                   third_param;
    hi_bool                  is_audio_play[AVPLAY_NUM];
    hi_handle                avplay_width_snd = HI_INVALID_HANDLE;
    hi_handle                track;
    hi_unf_audio_track_attr track_attr;

    if (argc < 2)
    {
        sample_printf("Usage: %s freq [srate] [qamtype or polarization]\n",argv[0]);
        return HI_FAILURE;
    }

    if (argc >= 2)
    {
        tunner_freq  = strtol(argv[1], NULL, 0);
    }

    tunner_srate = (tunner_freq > 1000) ? 27500 : 6875;
    third_param = (tunner_freq > 1000) ? 0 : 64;

    if (argc >= 3)
    {
        tunner_srate = strtol(argv[2], NULL, 0);
    }

    if (argc >= 4)
    {
        third_param = strtol(argv[3], NULL, 0);
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(HI_UNF_VIDEO_FMT_720P_50);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    for (i=0; i<AVPLAY_NUM; i++)
    {
        win_rect.width = 1280 / (AVPLAY_NUM / 2);
        win_rect.height = 720 / 2;
        win_rect.x = i % (AVPLAY_NUM / 2) * win_rect.width;
        win_rect.y = i / (AVPLAY_NUM / 2) * win_rect.height;

        ret = hi_adp_win_create(&win_rect,&win_handle[i]);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call WinInit failed.\n");
            goto VO_DEINIT;
        }
    }

    ret = hi_unf_dmx_init();
    ret |= hi_adp_dmx_attach_ts_port(PLAY_DEMUX_ID, PLAY_TUNER_ID);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_frontend_connect(PLAY_TUNER_ID, tunner_freq, tunner_srate, third_param);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DEMUX_ID, &g_prog_table);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call HIADP_Search_GetAllPmt failed\n");
        goto PSISI_FREE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    for(i = 0;i < AVPLAY_NUM;i++)
    {
        sample_printf("===============avplay[%d]===============\n",i);
        hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
        avplay_attr.vid_buf_size = 0x200000;
        avplay_attr.demux_id = PLAY_DEMUX_ID;
        ret = hi_unf_avplay_create(&avplay_attr, &avplay_handle[i]);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_avplay_create failed.\n");
            goto AVPLAY_STOP;
        }

        ret = hi_unf_avplay_chan_open(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_STOP;
        }

        ret = hi_unf_win_attach_src(win_handle[i], avplay_handle[i]);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_win_attach_src failed.\n");
            goto AVPLAY_STOP;
        }
        ret = hi_unf_win_set_enable(win_handle[i], HI_TRUE);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_win_set_enable failed.\n");
            goto AVPLAY_STOP;
        }

        is_audio_play[i] = HI_FALSE;
        if (0 == i)
        {
            is_audio_play[i] = HI_TRUE;
            ret = hi_unf_avplay_chan_open(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
                goto AVPLAY_STOP;
            }
            ret |= hi_unf_snd_create_track(HI_UNF_SND_0,&track_attr,&track);
            ret |= hi_unf_snd_attach(track, avplay_handle[i]);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_snd_attach failed.\n");
                goto AVPLAY_STOP;
            }
            avplay_width_snd = avplay_handle[i];
        }

        ret = hi_unf_avplay_get_attr(avplay_handle[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
        ret |= hi_unf_avplay_set_attr(avplay_handle[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_avplay_set_attr failed.\n");
            goto AVPLAY_STOP;
        }

        prog_num = 0;
        ret = hi_adp_avplay_play_prog(avplay_handle[i], g_prog_table, prog_num + i,
            is_audio_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call HIADP_AVPlay_SwitchProg failed\n");
        }
    }

    while(1)
    {
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

    	prog_num = atoi(input_cmd);
        prog_num = prog_num % AVPLAY_NUM;
        hi_unf_avplay_stop(avplay_width_snd, HI_UNF_AVPLAY_MEDIA_CHAN_AUD,HI_NULL);
        hi_unf_snd_detach(track,avplay_width_snd);
        hi_unf_snd_destroy_track(track);
        hi_unf_avplay_chan_close(avplay_width_snd, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        avplay_width_snd = avplay_handle[prog_num];
        hi_unf_avplay_chan_open(avplay_width_snd, HI_UNF_AVPLAY_MEDIA_CHAN_AUD,HI_NULL);

        ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
            goto AVPLAY_STOP;
        }
        hi_unf_snd_create_track(HI_UNF_SND_0,&track_attr,&track);
        hi_unf_snd_attach(track, avplay_width_snd);
        hi_adp_avplay_play_prog(avplay_width_snd, g_prog_table, prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    }

AVPLAY_STOP:
    for(i = 0 ; i < AVPLAY_NUM;i++)
    {
        stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop.timeout = 0;
        if ( prog_num == i )
        {
            hi_unf_avplay_stop(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
            hi_unf_snd_detach(track, avplay_handle[i]);
            hi_unf_snd_destroy_track(track);
        }
        else
        {
            hi_unf_avplay_stop(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop);
        }
        hi_unf_win_set_enable(win_handle[i],HI_FALSE);
        hi_unf_win_detach_src(win_handle[i], avplay_handle[i]);
        if( prog_num == i)
        {
            hi_unf_avplay_chan_close(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }
        else
        {
            hi_unf_avplay_chan_close(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        }

        hi_unf_avplay_destroy(avplay_handle[i]);
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
    for ( i = 0 ; i < AVPLAY_NUM ; i++ )
    {
        hi_unf_win_destroy(win_handle[i]);
    }
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return ret;
}

