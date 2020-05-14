/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tsplay.c
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

//#include "hi_unf_ecs.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_unf_ssm.h"
#include "hi_adp_mpi.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
//#include "HA.AUDIO.DRA.decode.h"
#include "hi_unf_acodec_pcmdec.h"
//#include "HA.AUDIO.WMA9STD.decode.h"
#include "hi_unf_acodec_amrnb.h"

#define TS_READ_SIZE    (188 * 1000)
#define  PLAY_DMX_ID  0

FILE               *g_pTsFile = HI_NULL;
static pthread_t   g_TsThd;
static pthread_mutex_t g_TsMutex;
static hi_bool     g_bStopTsThread = HI_FALSE;
hi_handle          g_TsBuf;
pmt_compact_tbl    *g_pProgTbl = HI_NULL;

hi_void TsTthread(hi_void *args)
{
    hi_unf_stream_buf   StreamBuf;
    hi_u32                Readlen;
    hi_s32                ret;

    while (!g_bStopTsThread)
    {
        pthread_mutex_lock(&g_TsMutex);
        ret = hi_unf_dmx_get_ts_buffer(g_TsBuf, TS_READ_SIZE, &StreamBuf, 100);
        if (ret != HI_SUCCESS)
        {
            pthread_mutex_unlock(&g_TsMutex);
            usleep(1000);
            continue;
        }

        Readlen = fread(StreamBuf.data, sizeof(hi_s8), TS_READ_SIZE, g_pTsFile);
        if(Readlen <= 0)
        {
            printf("read ts file end and rewind!\n");
            rewind(g_pTsFile);
            pthread_mutex_unlock(&g_TsMutex);
            continue;
        }

        ret = hi_unf_dmx_put_ts_buffer(g_TsBuf, Readlen, 0);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_TsMutex);

        usleep(1000);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_TsBuf);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}


hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                            ret;
    hi_handle                         hWin;
    hi_handle                         hAvplay;
    hi_unf_avplay_attr              AvplayAttr;
    hi_unf_sync_attr                SyncAttr;
    hi_unf_avplay_stop_opt          Stop;
    hi_char                           InputCmd[32];
    hi_unf_video_format                  enFormat = HI_UNF_VIDEO_FMT_1080P_24;
    hi_u32                            ProgNum;
    hi_handle                         hTrack;
    hi_unf_audio_track_attr          stTrackAttr;
    hi_unf_avplay_smp_attr          stAvTVPAttr;
    hi_unf_avplay_smp_attr stAudSecAttr;
    hi_unf_dmx_ts_buffer_attr           tsbuf_attr;
    hi_handle                         ssm;

    if (3 == argc)
    {
        enFormat = hi_adp_str2fmt(argv[2]);
    }
    else if (2 == argc)
    {
        enFormat = HI_UNF_VIDEO_FMT_1080P_24;
    }
    else
    {
        printf("Usage: sample_tsplay file [vo_format]\n"
               "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n");
        printf("Example:./sample_tsplay ./test.ts 1080P50\n");
        return 0;
    }

    g_pTsFile = fopen(argv[1], "rb");
    if (!g_pTsFile)
    {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(enFormat);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &hWin);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_init failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto DMX_DEINIT;
    }

    tsbuf_attr.buffer_size  = 0x200000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_TsBuf);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_ssm_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_init failed.\n");
        goto TSBUF_FREE;
    }

    hi_unf_ssm_attr ssm_attr = {0};
    ssm_attr.intent = HI_UNF_SSM_INTENT_WATCH;
    ret = hi_unf_ssm_create(&ssm_attr, &ssm);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_ssm_create failed.\n");
        goto SSM_DEINIT;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_init failed.\n");
        goto SSM_DESTROY;
    }

    ret = hi_unf_avplay_get_default_config(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    AvplayAttr.demux_id = PLAY_DMX_ID;
    ret |= hi_unf_avplay_create(&AvplayAttr, &hAvplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    hi_unf_avplay_session_info session_info;
    session_info.ssm = ssm;
    ret = hi_unf_avplay_set_session_info(hAvplay, &session_info);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_avplay_set_session_info failed.\n");
        goto AVPLAY_DESTROY;
    }

    stAvTVPAttr.enable = HI_TRUE;
    stAvTVPAttr.chan = HI_UNF_AVPLAY_MEDIA_CHAN_VID;
    ret = hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SMP, &stAvTVPAttr);
    if (ret != HI_SUCCESS)
    {
        printf("set tvp error\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    stAudSecAttr.enable = HI_FALSE;
    stAvTVPAttr.chan = HI_UNF_AVPLAY_MEDIA_CHAN_AUD;
    ret = hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SMP, &stAudSecAttr);
    if (ret != HI_SUCCESS)
    {
        printf("set security aud dmx chn fail.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_avplay_chan_open(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(hWin, hAvplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
        goto ACHN_CLOSE;
    }

    ret = hi_unf_win_set_enable(hWin, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &stTrackAttr, &hTrack);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(hTrack, hAvplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
    SyncAttr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    pthread_mutex_init(&g_TsMutex,NULL);
    g_bStopTsThread = HI_FALSE;
    pthread_create(&g_TsThd, HI_NULL, (hi_void *)TsTthread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_pProgTbl);
    if (ret != HI_SUCCESS)
    {
        printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    ProgNum = 0;

    pthread_mutex_lock(&g_TsMutex);
    rewind(g_pTsFile);
    hi_unf_dmx_reset_ts_buffer(g_TsBuf);

    ret = hi_adp_avplay_play_prog(hAvplay, g_pProgTbl, ProgNum, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS)
    {
        pthread_mutex_unlock(&g_TsMutex);
        printf("call hi_adp_avplay_play_prog failed.\n");
        goto AVPLAY_STOP;
    }

    pthread_mutex_unlock(&g_TsMutex);

    while (1)
    {
        static hi_u32 u32TplaySpeed = 2;
        printf("please input 'q' to quit!\n");
        SAMPLE_GET_INPUTCMD(InputCmd);

        if ('q' == InputCmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        if ('t' == InputCmd[0])
        {
            hi_unf_avplay_tplay_opt stTplayOpt;
            printf("%dx tplay\n",u32TplaySpeed);

            stTplayOpt.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
            stTplayOpt.speed_integer = u32TplaySpeed;
            stTplayOpt.speed_decimal = 0;

            hi_unf_avplay_tplay(hAvplay,&stTplayOpt);
            u32TplaySpeed = (32 == u32TplaySpeed * 2) ? (2) : (u32TplaySpeed * 2) ;
            continue;
        }

        if ('r' == InputCmd[0])
        {
            printf("resume\n");
            hi_unf_avplay_resume(hAvplay,HI_NULL);
            u32TplaySpeed = 2;
            continue;
        }

        if ('g' == InputCmd[0])
        {
            hi_unf_sync_status status_info = {0};
            hi_unf_avplay_get_status_info(hAvplay, HI_UNF_AVPLAY_STATUS_TYPE_SYNC, &status_info);
            printf("localtime %lld playtime %lld\n", status_info.local_time, status_info.play_time);
            continue;
        }

        ProgNum = atoi(InputCmd);
        if (ProgNum == 0)
            ProgNum = 1;

        pthread_mutex_lock(&g_TsMutex);
        rewind(g_pTsFile);
        hi_unf_dmx_reset_ts_buffer(g_TsBuf);

        ret = hi_adp_avplay_play_prog(hAvplay, g_pProgTbl, ProgNum - 1, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_adp_avplay_play_prog failed!\n");
            break;
        }

        pthread_mutex_unlock(&g_TsMutex);
    }

AVPLAY_STOP:
    Stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    Stop.timeout = 0;
    hi_unf_avplay_stop(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &Stop);

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_pProgTbl);
    g_pProgTbl = HI_NULL;
    hi_adp_search_de_init();

    g_bStopTsThread = HI_TRUE;
    pthread_join(g_TsThd, HI_NULL);
    pthread_mutex_destroy(&g_TsMutex);

SND_DETACH:
    hi_unf_snd_detach(hTrack, hAvplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(hTrack);

WIN_DETACH:
    hi_unf_win_set_enable(hWin, HI_FALSE);
    hi_unf_win_detach_src(hWin, hAvplay);

ACHN_CLOSE:
    hi_unf_avplay_chan_close(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(hAvplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

SSM_DESTROY:
    hi_unf_ssm_destroy(ssm);

SSM_DEINIT:
    hi_unf_ssm_deinit();

TSBUF_FREE:
    hi_unf_dmx_destroy_ts_buffer(g_TsBuf);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

VO_DEINIT:
    hi_unf_win_destroy(hWin);
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    fclose(g_pTsFile);
    g_pTsFile = HI_NULL;

    return ret;
}


