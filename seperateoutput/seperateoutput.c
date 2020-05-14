/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_ts_pip.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/09/23
  Description   :
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_unf_common.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_vo.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#define AVPLAY_NUM  2

FILE               *g_pTsFile = HI_NULL;
static pthread_t   g_TsThd;
static pthread_mutex_t g_TsMutex;
static HI_BOOL     g_bStopTsThread = HI_FALSE;
HI_HANDLE          g_TsBuf;
pmt_compact_tbl    *g_pProgTbl = HI_NULL;

#define  PLAY_DMX_ID  0

HI_VOID TsTthread(HI_VOID *args)
{
    hi_unf_stream_buf     StreamBuf;
    HI_U32                Readlen;
    HI_S32                Ret;

    while (!g_bStopTsThread)
    {
        pthread_mutex_lock(&g_TsMutex);
        Ret = HI_UNF_DMX_GetTSBuffer(g_TsBuf, 188*50, &StreamBuf, 1000);
        if (Ret != HI_SUCCESS )
        {
            pthread_mutex_unlock(&g_TsMutex);
            continue;
        }

        Readlen = fread(StreamBuf.data, sizeof(HI_S8), 188*50, g_pTsFile);
        if(Readlen <= 0)
        {
            printf("read ts file end and rewind!\n");
            rewind(g_pTsFile);
            pthread_mutex_unlock(&g_TsMutex);
            continue;
        }

        Ret = HIADP_DMX_PushTsBuffer(g_TsBuf, &StreamBuf, 0, Readlen);
        if (Ret != HI_SUCCESS )
        {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_TsMutex);
    }

    Ret = HI_UNF_DMX_ResetTSBuffer(g_TsBuf);
    if (Ret != HI_SUCCESS )
    {
        printf("call HI_UNF_DMX_ResetTSBuffer failed.\n");
    }

    return;
}


HI_S32 main(HI_S32 argc, HI_CHAR *argv[])
{
    HI_S32                      Ret,i;
    HI_HANDLE                   hWin[AVPLAY_NUM];
    HI_HANDLE                   hAvplay[AVPLAY_NUM];
    HI_UNF_AVPLAY_ATTR_S        AvplayAttr;
    HI_UNF_SYNC_ATTR_S          SyncAttr;
    HI_CHAR                     InputCmd[32];
    HI_UNF_ENC_FMT_E            VidFormat = HI_UNF_ENC_FMT_1080P_50;
    HI_U32                      ProgNum;
    HI_UNF_AVPLAY_STOP_OPT_S    stStopOpt;
    HI_BOOL                     bAudMix = 0;
    HI_BOOL                     bAudPlay[AVPLAY_NUM];
    HI_HANDLE                   hTrack[AVPLAY_NUM];
    HI_UNF_AUDIOTRACK_ATTR_S    stTrackAttr;
    HI_UNF_SND_GAIN_ATTR_S      stGain;
    HI_U32 u32TSPID = INVALID_TSPID;
    HI_UNF_WINDOW_ATTR_S   WinAttr;

    stGain.bLinearMode = HI_TRUE;

    if (argc < 2)
    {
        printf("Usage:\n%s tsfile [audiomix] [vo_format]\n", argv[0]);
        printf("\taudiomix\t: 0: not mix audio, 1: mix audio\n");
        printf("\tvo_format\t: 2160P60 2160P50 2160P30 2160P25 2160P24 1080P60 1080P50 1080i60 1080i50 720P60 720P50\n\n");
        printf("Example: %s ./test.ts 0 1080P50\n", argv[0]);
        return 0;
    }

    if (argc >= 3)
    {
        bAudMix = strtol(argv[2], NULL, 0);
    }

    if (argc >= 4)
    {
        VidFormat = hi_adp_str2fmt(argv[3]);
    }

    g_pTsFile = fopen(argv[1], "rb");
    if (!g_pTsFile)
    {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    hi_unf_sys_init();

    HIADP_MCE_Exit();

    Ret = HIADP_Snd_Init();
    if (HI_SUCCESS != Ret)
    {
        printf("call HIADP_Snd_Init failed.\n");
        goto SYS_DEINIT;
    }

    Ret = hi_adp_disp_init(VidFormat);
    if (HI_SUCCESS != Ret)
    {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    Ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_NORMAL);
    if (HI_SUCCESS != Ret)
    {
        printf("call HIADP_VO_Init failed.\n");
        goto DISP_DEINIT;
    }

    Ret = HIADP_VO_CreatWin(HI_NULL, &hWin[0]);
    if (HI_SUCCESS != Ret)
    {
        printf("call HIADP_VO_CreatWin 0 failed.\n");
        goto VO_DEINIT;
    }

    memset(&WinAttr, 0, sizeof(HI_UNF_WINDOW_ATTR_S));
    WinAttr.enDisp = HI_UNF_DISPLAY0;
    WinAttr.bVirtual = HI_FALSE;
    WinAttr.stWinAspectAttr.enAspectCvrs = HI_UNF_VO_ASPECT_CVRS_IGNORE;
    WinAttr.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
    WinAttr.stWinAspectAttr.u32UserAspectWidth  = 0;
    WinAttr.stWinAspectAttr.u32UserAspectHeight = 0;
    WinAttr.bUseCropRect = HI_FALSE;
    WinAttr.stInputRect.x = 0;
    WinAttr.stInputRect.y = 0;
    WinAttr.stInputRect.width = 0;
    WinAttr.stInputRect.height = 0;
    memset(&WinAttr.stOutputRect, 0x0, sizeof(hi_unf_rect));


    Ret = HI_UNF_VO_CreateWindow(&WinAttr, &hWin[1]);
    if (Ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call HI_UNF_VO_CreateWindow failed.\n");
        goto VO_DEINIT;
    }


    Ret = hi_unf_dmx_init();
    if (HI_SUCCESS != Ret)
    {
        printf("call hi_unf_dmx_init failed.\n");
        goto DESTROY_WINDOW;
    }

    for (i = 0 ; i < AVPLAY_NUM; i++)
    {
        Ret = hi_unf_dmx_attach_ts_port(i, HI_UNF_DMX_PORT_RAM_0);
        if (HI_SUCCESS != Ret)
        {
            printf("call hi_unf_dmx_attach_ts_port failed.\n");
            goto DMX_DEINIT;
        }
    }

    HI_UNF_DMX_TSBUF_ATTR_S tsbuf_attr;
    tsbuf_attr.enSecureMode = HI_UNF_DMX_SECURE_MODE_NONE;
    tsbuf_attr.u32BufSize = 0x800000;
    Ret = HI_UNF_DMX_CreateTSBuffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_TsBuf);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_CreateTSBuffer failed.\n");
        goto DMX_DEINIT;
    }

    pthread_mutex_init(&g_TsMutex,NULL);
    g_bStopTsThread = HI_FALSE;
    pthread_create(&g_TsThd, HI_NULL, (HI_VOID *)TsTthread, HI_NULL);

    hi_adp_search_init();
    Ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_pProgTbl);
    if (HI_SUCCESS != Ret)
    {
        printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    Ret = HIADP_AVPlay_RegADecLib();
    Ret |= HI_UNF_AVPLAY_Init();
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_Init failed.\n");
        goto PSISI_FREE;
    }

    pthread_mutex_lock(&g_TsMutex);
    rewind(g_pTsFile);
    HI_UNF_DMX_ResetTSBuffer(g_TsBuf);

    for (i = 0; i < AVPLAY_NUM; i++)
    {
        printf("===============avplay[%d]===============\n",i);

        HI_UNF_AVPLAY_GetDefaultConfig(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);

        AvplayAttr.u32DemuxId = i;

        Ret = HI_UNF_AVPLAY_Create(&AvplayAttr, &hAvplay[i]);
        if (Ret != HI_SUCCESS)
        {
            pthread_mutex_unlock(&g_TsMutex);
            printf("call HI_UNF_AVPLAY_Create failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (HI_SUCCESS != Ret)
        {
            pthread_mutex_unlock(&g_TsMutex);
            printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = HI_UNF_VO_AttachWindow(hWin[i], hAvplay[i]);
        if (HI_SUCCESS != Ret)
        {
            pthread_mutex_unlock(&g_TsMutex);
            printf("call HI_UNF_VO_AttachWindow failed.\n");
            goto AVPLAY_STOP;
        }
        Ret = HI_UNF_VO_SetWindowEnable(hWin[i], HI_TRUE);
        if (HI_SUCCESS != Ret)
        {
            pthread_mutex_unlock(&g_TsMutex);
            printf("call HI_UNF_VO_SetWindowEnable failed.\n");
            goto AVPLAY_STOP;
        }

        bAudPlay[i] = HI_FALSE;
        if (0 == i)
        {
            bAudPlay[i] = HI_TRUE;

            Ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
            if (Ret != HI_SUCCESS)
            {
                printf("call HI_UNF_SND_GetDefaultTrackAttr failed.\n");
            }
            Ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hTrack[i]);
            Ret |= HI_UNF_AVPLAY_ChnOpen(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            Ret |= HI_UNF_SND_Attach(hTrack[i], hAvplay[i]);
            if (Ret != HI_SUCCESS)
            {
                pthread_mutex_unlock(&g_TsMutex);
                printf("call HI_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }
        else if(1 == i && HI_TRUE == bAudMix)
        {
            bAudPlay[i] = HI_TRUE;

            stTrackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_SLAVE;
            Ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_SLAVE, &stTrackAttr);
            if (Ret != HI_SUCCESS)
            {
                printf("call HI_UNF_SND_GetDefaultTrackAttr failed.\n");
            }
            Ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hTrack[i]);
            stGain.s32Gain = 60;
            Ret = HI_UNF_SND_SetTrackWeight(hTrack[i],&stGain);
            Ret |= HI_UNF_AVPLAY_ChnOpen(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            Ret |= HI_UNF_SND_Attach( hTrack[i], hAvplay[i]);
            if (Ret != HI_SUCCESS)
            {
                pthread_mutex_unlock(&g_TsMutex);
                printf("call HI_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }

        Ret = HI_UNF_AVPLAY_GetAttr(hAvplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        SyncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
        Ret = HI_UNF_AVPLAY_SetAttr(hAvplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        if (Ret != HI_SUCCESS)
        {
            pthread_mutex_unlock(&g_TsMutex);
            printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
            goto AVPLAY_STOP;
        }

        ProgNum = 0;
        Ret = HIADP_AVPlay_PlayProg(hAvplay[i], g_pProgTbl, ProgNum + i,bAudPlay[i]);
        if (HI_SUCCESS != Ret)
        {
            printf("call HIADP_AVPlay_PlayProg failed\n");
        }
    }

    pthread_mutex_unlock(&g_TsMutex);

    while (1)
    {
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(InputCmd);

        if ('q' == InputCmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        ProgNum = atoi(InputCmd);

        for ( i = 0 ; i < AVPLAY_NUM; i++ )
        {
            stStopOpt.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
            stStopOpt.u32TimeoutMs = 0;
            Ret = HI_UNF_AVPLAY_Stop(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stStopOpt);
            if (HI_SUCCESS != Ret)
            {
                printf("call HI_UNF_AVPLAY_Stop failed.\n");
                break;
            }

            (HI_VOID)HI_UNF_AVPLAY_SetAttr(hAvplay[i],HI_UNF_AVPLAY_ATTR_ID_PCR_PID,&u32TSPID);
            (HI_VOID)HI_UNF_AVPLAY_SetAttr(hAvplay[i],HI_UNF_AVPLAY_ATTR_ID_VID_PID,&u32TSPID);

             if (HI_TRUE == bAudPlay[i])
             {
                (HI_VOID)HI_UNF_AVPLAY_SetAttr(hAvplay[i],HI_UNF_AVPLAY_ATTR_ID_AUD_PID,&u32TSPID);
             }
        }

        pthread_mutex_lock(&g_TsMutex);
        rewind(g_pTsFile);
        HI_UNF_DMX_ResetTSBuffer(g_TsBuf);

        for (i = 0 ; i < AVPLAY_NUM; i++)
        {
            Ret = HIADP_AVPlay_PlayProg(hAvplay[i],g_pProgTbl,ProgNum + i,bAudPlay[i]);
            if (Ret != HI_SUCCESS)
            {
                printf("call SwitchProgfailed!\n");
                break;
            }
        }

        pthread_mutex_unlock(&g_TsMutex);
    }

    g_bStopTsThread = HI_TRUE;
    pthread_join(g_TsThd, HI_NULL);

    pthread_mutex_destroy(&g_TsMutex);

AVPLAY_STOP:
    for(i = 0 ; i < AVPLAY_NUM;i++)
    {
        stStopOpt.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stStopOpt.u32TimeoutMs = 0;
        if (HI_FALSE == bAudMix && 1 == i)
        {
            HI_UNF_AVPLAY_Stop(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stStopOpt);
        }
        else
        {
            HI_UNF_AVPLAY_Stop(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stStopOpt);
            HI_UNF_SND_Detach(hTrack[i], hAvplay[i]);
            HI_UNF_SND_DestroyTrack(hTrack[i]);
        }
        HI_UNF_VO_SetWindowEnable(hWin[i],HI_FALSE);
        HI_UNF_VO_DetachWindow(hWin[i], hAvplay[i]);
        if (HI_FALSE == bAudMix && 1 == i)
        {
            HI_UNF_AVPLAY_ChnClose(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        }
        else
        {
            HI_UNF_AVPLAY_ChnClose(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }
        HI_UNF_AVPLAY_Destroy(hAvplay[i]);
    }
    HI_UNF_AVPLAY_DeInit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_pProgTbl);
    g_pProgTbl = HI_NULL;
    hi_adp_search_de_init();
    HI_UNF_DMX_DestroyTSBuffer(g_TsBuf);

DMX_DEINIT:
    for (i = 0; i < AVPLAY_NUM; i++)
    {
        HI_UNF_DMX_DetachTSPort(i);
    }

    hi_unf_dmx_deinit();

DESTROY_WINDOW:
    HI_UNF_VO_DestroyWindow(hWin[1]);
    HI_UNF_VO_DestroyWindow(hWin[0]);

VO_DEINIT:
    HIADP_VO_DeInit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    HIADP_Snd_DeInit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    fclose(g_pTsFile);
    g_pTsFile = HI_NULL;

    return Ret;
}


