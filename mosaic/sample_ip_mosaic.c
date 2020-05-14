/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mplayer.c
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
//#include "hi_unf_ecs.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define PLAY_DMX_ID 0
#define AVPLAY_NUM  4
#define AUD_MIX

static pthread_t   g_SocketThd;
hi_char            *g_pMultiAddr;
hi_u8             g_UdpPort;
hi_handle          g_TsBuf;


pmt_compact_tbl    *g_pProgTbl = HI_NULL;
static hi_bool g_StopSocketThread;

hi_void SocketThread(hi_void *args)
{
    hi_s32              SocketFd;
    struct sockaddr_in  ServerAddr;
    in_addr_t           IpAddr;
    struct ip_mreq      Mreq;
    hi_u32              AddrLen;

    hi_unf_stream_buf     StreamBuf;
    hi_u32              ReadLen;
    hi_u32              GetBufCount=0;
    hi_u32              ReceiveCount=0;
    hi_s32              Ret;

    SocketFd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (SocketFd < 0)
    {
        sample_printf("create socket error [%d].\n", errno);
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ServerAddr.sin_port = htons(g_UdpPort);

    if (bind(SocketFd,(struct sockaddr *)(&ServerAddr),sizeof(struct sockaddr_in)) < 0)
    {
        sample_printf("socket bind error [%d].\n", errno);
        close(SocketFd);
        return;
    }

    IpAddr = inet_addr(g_pMultiAddr);
    if (IpAddr)
    {
        Mreq.imr_multiaddr.s_addr = IpAddr;
        Mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(SocketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &Mreq, sizeof(struct ip_mreq)))
        {
            sample_printf("Socket setsockopt ADD_MEMBERSHIP error [%d].\n", errno);
            close(SocketFd);
            return;
        }
    }

    AddrLen = sizeof(ServerAddr);

    while (!g_StopSocketThread)
    {
        Ret = hi_unf_dmx_get_ts_buffer(g_TsBuf, 188*50, &StreamBuf, 0);
        if (Ret != HI_SUCCESS)
        {
            GetBufCount++;
            if(GetBufCount >= 10)
            {
                sample_printf("########## TS come too fast! #########, Ret=%d\n", Ret);
                GetBufCount=0;
            }

            usleep(10000) ;
            continue;
        }
        GetBufCount=0;

        ReadLen = recvfrom(SocketFd, StreamBuf.data, 1316, 0,
                           (struct sockaddr *)&ServerAddr, &AddrLen);
        if (ReadLen <= 0)
        {
            ReceiveCount++;
            if (ReceiveCount >= 50)
            {
                sample_printf("########## TS come too slow or net error! #########\n");
                ReceiveCount = 0;
            }
        }
        else
        {
            ReceiveCount = 0;
            Ret = hi_adp_dmx_push_ts_buf(g_TsBuf, &StreamBuf, 0, ReadLen);
            if (Ret != HI_SUCCESS )
            {
                sample_printf("call hi_unf_dmx_put_ts_buffer failed.\n");
            }
        }
    }

    close(SocketFd);
    return;
}

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32                  Ret,i;
    hi_handle               hWin[AVPLAY_NUM];

    hi_handle               hAvplay[AVPLAY_NUM];
    hi_unf_avplay_attr        AvplayAttr;
    hi_unf_sync_attr          SyncAttr;
    hi_unf_avplay_stop_opt    Stop;
    hi_char                 InputCmd[32];
    hi_u32                  ProgNum;
    hi_unf_video_rect               stWinRect;
    hi_bool                  bAudPlay[AVPLAY_NUM];

    hi_handle                hTrack;
    hi_unf_audio_track_attr stTrackAttr;

    if (argc < 3)
    {
        sample_printf("Usage: %s MultiAddr UdpPort\n",argv[0]);
        return HI_FAILURE;
    }

    g_pMultiAddr = argv[1];
    g_UdpPort = atoi(argv[2]);

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    Ret = hi_adp_snd_init();
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    Ret = hi_adp_disp_init(HI_UNF_VIDEO_FMT_720P_50);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    Ret = hi_adp_win_init();
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    for (i=0; i < AVPLAY_NUM; i++)
    {
        stWinRect.width = 1280 / (AVPLAY_NUM / 2);
        stWinRect.height = 720 / 2;
        stWinRect.x = i % (AVPLAY_NUM / 2) * stWinRect.width;
        stWinRect.y = i / (AVPLAY_NUM / 2) * stWinRect.height;
        Ret = hi_adp_win_create(&stWinRect,&hWin[i]);
        if (Ret != HI_SUCCESS)
        {
            sample_printf("call WinInit failed.\n");
            goto VO_DEINIT;
        }
    }
    Ret = hi_unf_dmx_init();
    Ret |= hi_unf_dmx_attach_ts_port(0,HI_UNF_DMX_PORT_RAM_0);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    tsbuf_attr.buffer_size = 0x200000;
    Ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_TsBuf);
    if (Ret != HI_SUCCESS)
    {
        sample_printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    g_StopSocketThread = HI_FALSE;
    pthread_create(&g_SocketThd, HI_NULL, (hi_void *)SocketThread, HI_NULL);

    hi_adp_search_init();
    Ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_pProgTbl);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    Ret = hi_adp_avplay_init();
    if (Ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    for ( i = 0 ; i < AVPLAY_NUM ; i++ )
    {
        sample_printf("===============avplay[%d]===============\n",i);
        hi_unf_avplay_get_default_config(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
        AvplayAttr.vid_buf_size = 0x300000;
        Ret = hi_unf_avplay_create(&AvplayAttr, &hAvplay[i]);
        if (Ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_avplay_create failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = hi_unf_avplay_chan_open(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = hi_unf_win_attach_src(hWin[i], hAvplay[i]);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call hi_unf_win_attach_src failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = hi_unf_win_set_enable(hWin[i], HI_TRUE);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call hi_unf_win_set_enable failed.\n");
            goto AVPLAY_STOP;
        }

        bAudPlay[i] = HI_FALSE;
        if (0 == i)
        {
            bAudPlay[i] = HI_TRUE;
            Ret = hi_unf_avplay_chan_open(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            stTrackAttr.track_type = HI_UNF_SND_TRACK_TYPE_MASTER;
            Ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
            if (Ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
                goto AVPLAY_STOP;
            }
            Ret |= hi_unf_snd_create_track(HI_UNF_SND_0,&stTrackAttr,&hTrack);
            Ret |= hi_unf_snd_attach(hTrack, hAvplay[i]);
            if (Ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_snd_attach failed.\n");
                goto AVPLAY_STOP;
            }
        }

        Ret = hi_unf_avplay_get_attr(hAvplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        SyncAttr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
        Ret |= hi_unf_avplay_set_attr(hAvplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        if (Ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_avplay_set_attr failed.\n");
            goto AVPLAY_STOP;
        }

        ProgNum = 0;
        Ret = hi_adp_avplay_play_prog(hAvplay[i], g_pProgTbl, ProgNum + i,
            bAudPlay[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call HIADP_AVPlay_SwitchProg failed\n");
        }
    }

    while(1)
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
            Ret = hi_adp_avplay_play_prog(hAvplay[i],g_pProgTbl,ProgNum + i,
                bAudPlay[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
            if (Ret != HI_SUCCESS)
            {
                sample_printf("call SwitchProgfailed!\n");
                break;
            }
        }
    }

AVPLAY_STOP:
    for(i = 0 ; i < AVPLAY_NUM;i++)
    {
        Stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        Stop.timeout = 0;
        hi_unf_avplay_stop(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &Stop);
        if (0 == i)
        {
            hi_unf_snd_detach(hTrack, hAvplay[i]);
            hi_unf_snd_destroy_track(hTrack);
        }
        hi_unf_win_set_enable(hWin[i],HI_FALSE);
        hi_unf_win_detach_src(hWin[i], hAvplay[i]);
        hi_unf_avplay_chan_close(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        hi_unf_avplay_destroy(hAvplay[i]);
    }
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_pProgTbl);
    g_pProgTbl = HI_NULL;
    hi_adp_search_de_init();

    g_StopSocketThread = HI_TRUE;
    pthread_join(g_SocketThd, HI_NULL);

    hi_unf_dmx_destroy_ts_buffer(g_TsBuf);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

VO_DEINIT:
    for ( i = 0 ; i < AVPLAY_NUM ; i++ )
    {
        hi_unf_win_destroy(hWin[i]);
    }
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return Ret;
}


