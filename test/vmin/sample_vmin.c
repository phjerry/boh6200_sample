/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_vmin.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2019/09/26
  Description   :
  History       :
  1.Date        : 2019/09/26
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

#include "hi_unf_common.h"
#include "hi_errno.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_vo.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"

#include "HA.AUDIO.MP3.decode.h"
#include "HA.AUDIO.MP2.decode.h"
#include "HA.AUDIO.AAC.decode.h"
#include "HA.AUDIO.PCM.decode.h"
#include "HA.AUDIO.AMRNB.codec.h"
#include "HA.AUDIO.AAC.encode.h"
#include "hi_unf_aenc.h"
#include "hi_unf_venc.h"
#include "hi_type.h"

#define AENC_SUPPORT
//#define PLAY_AND_TRANSCODE_SUPPORT
#define HIFB_SUPPORT
#define HI_RECORD_OPEN
#define PLAY_DMX_ID  0
#define PLAY_DMX_ID_1 1
#define AUTO_TRANS
#ifdef HIFB_SUPPORT
#include "hi_fb_ui.h"
#endif
#ifdef HI_RECORD_OPEN
#include "hi_demux_record.h"
#endif


FILE               *g_pTsFile = HI_NULL;
FILE               *g_ts_play_file = HI_NULL;
static pthread_t   g_TsThd;
static pthread_mutex_t g_TsMutex;
static HI_BOOL     g_bStopTsThread = HI_FALSE;
HI_HANDLE          g_TsBuf;
pmt_compact_tbl    *g_pProgTbl = HI_NULL;

static pthread_t g_ts_play_thd;
static pthread_mutex_t g_ts_play_mtx;
static HI_HANDLE g_ts_play_buf = HI_FALSE;
static HI_BOOL g_ts_play_stop = HI_FALSE;


HI_VOID TsTthread(HI_VOID *args)
{
    hi_unf_stream_buf   StreamBuf;
    HI_U32                Readlen;
    HI_S32                Ret;

    while (!g_bStopTsThread)
    {
        pthread_mutex_lock(&g_TsMutex);
        Ret = HI_UNF_DMX_GetTSBuffer(g_TsBuf, 188*2000, &StreamBuf, 1000);
        if (Ret != HI_SUCCESS )
        {
            pthread_mutex_unlock(&g_TsMutex);
            continue;
        }

        Readlen = fread(StreamBuf.pu8Data, sizeof(HI_S8), 188*2000, g_pTsFile);
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

HI_VOID ts_play_thread(HI_VOID *args)
{
    hi_unf_stream_buf   StreamBuf;
    HI_U32                Readlen;
    HI_S32                Ret;

    while (!g_ts_play_stop)
    {
        pthread_mutex_lock(&g_ts_play_mtx);
        Ret = HI_UNF_DMX_GetTSBuffer(g_ts_play_buf, 188*2000, &StreamBuf, 1000);
        if (Ret != HI_SUCCESS )
        {
            pthread_mutex_unlock(&g_ts_play_mtx);
            continue;
        }

        Readlen = fread(StreamBuf.pu8Data, sizeof(HI_S8), 188*2000, g_ts_play_file);
        if(Readlen <= 0)
        {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_play_file);
            pthread_mutex_unlock(&g_ts_play_mtx);
            continue;
        }

        Ret = HIADP_DMX_PushTsBuffer(g_ts_play_buf, &StreamBuf, 0, Readlen);
        if (Ret != HI_SUCCESS )
        {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_ts_play_mtx);
    }

    Ret = HI_UNF_DMX_ResetTSBuffer(g_ts_play_buf);
    if (Ret != HI_SUCCESS )
    {
        printf("call HI_UNF_DMX_ResetTSBuffer in ts_play_thread failed.\n");
    }

    return;
}


volatile HI_BOOL g_bStopSaveThread = HI_TRUE;
volatile HI_BOOL g_bSaveStream = HI_FALSE;

//#define FILE_LEN_LIMIT

typedef struct
{
    HI_HANDLE hVenc;
    HI_HANDLE hAenc;
}THREAD_ARGS_S;

HI_VOID* SaveThread(HI_VOID *pArgs)
{
    THREAD_ARGS_S *pThreadArgs = (THREAD_ARGS_S*)pArgs;
    HI_HANDLE hVencChn;
    HI_S32 Ret;
#ifdef AENC_SUPPORT
    HI_UNF_ES_BUF_S stAencStream;
    HI_HANDLE hAencChn;
    hAencChn = pThreadArgs->hAenc;
#endif
    hVencChn = pThreadArgs->hVenc;

    while(!g_bStopSaveThread)
    {
        HI_BOOL bGotStream = HI_FALSE;

        if (g_bSaveStream)
        {
            HI_UNF_VENC_STREAM_S stVencStream;
            /*save video stream*/
            Ret = HI_UNF_VENC_AcquireStream(hVencChn,&stVencStream,0);
            if ( HI_SUCCESS == Ret )
            {
                HI_UNF_VENC_ReleaseStream(hVencChn,&stVencStream);
                bGotStream = HI_TRUE;
            }
            else if ( HI_ERR_VENC_BUF_EMPTY != Ret)
            {
                printf("HI_UNF_VENC_AcquireStream failed:%#x\n",Ret);
            }

#ifdef AENC_SUPPORT
            /*save audio stream*/
            Ret = HI_UNF_AENC_AcquireStream(hAencChn,&stAencStream,0);
            if ( HI_SUCCESS == Ret )
            {
                HI_UNF_AENC_ReleaseStream(hAencChn,&stAencStream);
                bGotStream = HI_TRUE;
            }
            else if ( HI_ERR_AENC_OUT_BUF_EMPTY != Ret)
            {
                printf("HI_UNF_AENC_AcquireStream failed:%#x\n",Ret);
            }
#endif
        }
        usleep(10 * 1000);
        if ( HI_FALSE == bGotStream )
        {
            usleep(20 * 1000);
        }
    }
    return HI_NULL;
}

typedef struct {
    hi_handle window;
    hi_handle venc;
    hi_bool   pause;
} transcode_thread_ctx;

static hi_bool g_stop_transcode_thread = HI_FALSE;
static void *transcode_thread(void *arg)
{
    hi_s32 ret;
    transcode_thread_ctx *thread_ctx = arg;
    HI_UNF_VIDEO_FRAME_INFO_S a_frame;
    HI_UNF_VIDEO_FRAME_INFO_S d_frame;

    hi_bool acquired = HI_FALSE;
    hi_bool dequeued = HI_FALSE;
    hi_bool wait = HI_FALSE;

    while (!g_stop_transcode_thread) {
        if (wait) {
            usleep(10 * 1000);
        }

        wait = HI_TRUE;
        if (thread_ctx->pause) {
            continue;
        }

        /* WINDOW->VENC */
        if (!acquired) {
            ret = HI_UNF_VO_AcquireFrame(thread_ctx->window, &a_frame, 20);

            if (ret == HI_SUCCESS) {
                acquired = HI_TRUE;
            }
        }

        if (acquired) {
            ret = HI_UNF_VENC_QueueFrame(thread_ctx->venc, &a_frame);
            if (ret == HI_SUCCESS) {
                acquired = HI_FALSE;
                wait = HI_FALSE;
            }
        }

        /* VENC->WINDOW */
        if (!dequeued) {
            ret = HI_UNF_VENC_DequeueFrame(thread_ctx->venc, &d_frame);
            if (ret == HI_SUCCESS) {
                dequeued = HI_TRUE;
            }
        }

        if (dequeued) {
            ret = HI_UNF_VO_ReleaseFrame(thread_ctx->window, &d_frame);
            if (ret == HI_SUCCESS) {
                dequeued = HI_FALSE;
                wait = HI_FALSE;
            }
        }
    }

    return HI_NULL;
}

HI_S32 avplay_play(HI_HANDLE hAvplay,pmt_compact_tbl *pProgTbl,HI_U32 ProgNum,HI_BOOL bAudPlay)
{
    HI_UNF_AVPLAY_STOP_OPT_S stop_opt;
    HI_U32                  VidPid;
    HI_UNF_VCODEC_TYPE_E    enVidType;
    HI_S32                  Ret;

    stop_opt.enMode = HI_UNF_AVPLAY_STOP_MODE_STILL;
    stop_opt.u32TimeoutMs = 0;

    Ret = HI_UNF_AVPLAY_Stop(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    if (Ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call HI_UNF_AVPLAY_Stop failed.\n");
        return Ret;
    }

    ProgNum = ProgNum % pProgTbl->prog_num;
    if (pProgTbl->proginfo[ProgNum].v_element_num > 0 )
    {
        VidPid = pProgTbl->proginfo[ProgNum].v_element_pid;
        enVidType = pProgTbl->proginfo[ProgNum].video_type;
    }
    else
    {
        VidPid = INVALID_TSPID;
        enVidType = HI_UNF_VCODEC_TYPE_BUTT;
    }

    if (VidPid != INVALID_TSPID)
    {
        Ret = HIADP_AVPlay_SetVdecAttr(hAvplay,enVidType,HI_UNF_VCODEC_MODE_NORMAL);
        Ret |= HI_UNF_AVPLAY_SetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_VID_PID,&VidPid);
        if (Ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call HIADP_AVPlay_SetVdecAttr failed.\n");
            return Ret;
        }

        Ret = HI_UNF_AVPLAY_Start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (Ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call HI_UNF_AVPLAY_Start failed.\n");
            return Ret;
        }
    }

    HI_U32                  AudPid;
    HI_U32                  u32AudType;

    if (pProgTbl->proginfo[ProgNum].a_element_num > 0)
    {
        AudPid  = pProgTbl->proginfo[ProgNum].a_element_pid;
        u32AudType = pProgTbl->proginfo[ProgNum].audio_type;
    }
    else
    {
        AudPid = INVALID_TSPID;
        u32AudType = 0xffffffff;
    }

    if (HI_TRUE == bAudPlay && AudPid != INVALID_TSPID)
    {

        Ret  = HIADP_AVPlay_SetAdecAttr(hAvplay, u32AudType, HD_DEC_MODE_RAWPCM, 1);

        Ret |= HI_UNF_AVPLAY_SetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &AudPid);
        if (HI_SUCCESS != Ret)
        {
            SAMPLE_COMMON_PRINTF("HIADP_AVPlay_SetAdecAttr failed:%#x\n",Ret);
            return Ret;
        }

        Ret = HI_UNF_AVPLAY_Start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (Ret != HI_SUCCESS)
        {
            printf("call HI_UNF_AVPLAY_Start to start audio failed.\n");
            //return Ret;
        }
    }

    return HI_SUCCESS;
}

static HI_S32 ts_play_prepare(HI_HANDLE *win, HI_HANDLE *track, HI_HANDLE *avplay)
{
    HI_S32 Ret;
    HI_HANDLE               hAvplay, hWin, hTrack;
    HI_UNF_AVPLAY_ATTR_S        AvplayAttr;
    HI_UNF_AUDIOTRACK_ATTR_S    stTrackAttr;
    HI_UNF_SYNC_ATTR_S          SyncAttr;

    Ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (HI_SUCCESS != Ret)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        return HI_FAILURE;
    }

    HI_UNF_DMX_TSBUF_ATTR_S tsbuf_attr;
    tsbuf_attr.enSecureMode = HI_UNF_DMX_SECURE_MODE_NONE;
    tsbuf_attr.u32BufSize = 0x1000000;
    Ret = HI_UNF_DMX_CreateTSBuffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_play_buf);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_CreateTSBuffer failed.\n");
        goto DMX_DETACH;
    }

    Ret = HI_UNF_AVPLAY_GetDefaultConfig(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    AvplayAttr.u32DemuxId = PLAY_DMX_ID;
    //AvplayAttr.stStreamAttr.u32VidBufSize = (3*1024*1024);
    Ret |= HI_UNF_AVPLAY_Create(&AvplayAttr, &hAvplay);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_Create failed.\n");
        goto TSBUF_FREE;
    }

    Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
        goto AVPLAY_DESTROY;
    }

    Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
        goto VCHN_CLOSE;
    }

    Ret |= HIADP_VO_CreatWin(HI_NULL, &hWin);
    if (HI_SUCCESS != Ret)
    {
        printf("call VoInit failed.\n");
        HIADP_VO_DeInit();
        goto ACHN_CLOSE;
    }

    Ret = HI_UNF_VO_AttachWindow(hWin, hAvplay);
    if (HI_SUCCESS != Ret)
    {
        printf("call HI_UNF_VO_AttachWindow failed:%#x.\n",Ret);
    }
    Ret = HI_UNF_VO_SetWindowEnable(hWin, HI_TRUE);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_VO_SetWindowEnable failed.\n");
        goto WIN_DETACH;
    }

    Ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_SND_GetDefaultTrackAttr failed.\n");
        goto WIN_DETACH;
    }
    Ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hTrack);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto WIN_DETACH;
    }

    Ret = HI_UNF_SND_Attach(hTrack, hAvplay);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }

    Ret = HI_UNF_AVPLAY_GetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
    SyncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
    Ret |= HI_UNF_AVPLAY_SetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
        goto TRACK_DETACH;
    }

    *win = hWin;
    *track = hTrack;
    *avplay = hAvplay;

    return HI_SUCCESS;

TRACK_DETACH:
    HI_UNF_SND_Detach(hTrack, hAvplay);

TRACK_DESTROY:
    HI_UNF_SND_DestroyTrack(hTrack);

WIN_DETACH:
    HI_UNF_VO_SetWindowEnable(hWin, HI_FALSE);
    HI_UNF_VO_DetachWindow(hWin, hAvplay);
    HI_UNF_VO_DestroyWindow(hWin);

ACHN_CLOSE:
    HI_UNF_AVPLAY_ChnClose(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    HI_UNF_AVPLAY_ChnClose(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    HI_UNF_AVPLAY_Destroy(hAvplay);

TSBUF_FREE:
    HI_UNF_DMX_DestroyTSBuffer(g_ts_play_buf);

DMX_DETACH:
    HI_UNF_DMX_DetachTSPort(PLAY_DMX_ID_1);

    return HI_FAILURE;
}


static HI_S32 ts_play_exit(HI_HANDLE *win, HI_HANDLE *track, HI_HANDLE *avplay)
{
    HI_UNF_SND_Detach(*track, *avplay);
    HI_UNF_SND_DestroyTrack(*track);

    HI_UNF_VO_SetWindowEnable(*win, HI_FALSE);
    HI_UNF_VO_DetachWindow(*win, *avplay);
    HI_UNF_VO_DestroyWindow(*win);

    HI_UNF_AVPLAY_ChnClose(*avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    HI_UNF_AVPLAY_Destroy(*avplay);

    HI_UNF_DMX_DestroyTSBuffer(g_ts_play_buf);
    HI_UNF_DMX_DetachTSPort(PLAY_DMX_ID);

    return HI_SUCCESS;
}


HI_S32 main(HI_S32 argc,HI_CHAR *argv[])
{
    HI_S32                  Ret;
    HI_HANDLE               hEncWin;
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    HI_HANDLE             hWin;
#endif
    HI_HANDLE               hAvplay = HI_INVALID_HANDLE;
    HI_UNF_AVPLAY_STOP_OPT_S    Stop;
    HI_CHAR                 InputCmd[32];
    HI_UNF_ENC_FMT_E   enFormat = HI_UNF_ENC_FMT_1080P_60; //HI_UNF_ENC_FMT_7680X4320_30
    HI_U32             ProgNum;
    THREAD_ARGS_S           stSaveArgs;
    pthread_t               thdSave;
#ifdef HI_RECORD_OPEN
    hi_u32 j;
#endif
    HI_HANDLE hVenc;
    HI_UNF_VENC_CHN_ATTR_S stVencChnAttr;
#ifdef AENC_SUPPORT
    HI_UNF_AUDIOTRACK_ATTR_S    stTrackAttr;
    HI_HANDLE          hEncTrack;
    HI_HANDLE          hAenc;
    AAC_ENC_CONFIG     stPrivateConfig;
    HI_UNF_AENC_ATTR_S stAencAttr;
#endif
    hi_u32 width;
    hi_u32 height;
#ifdef HIFB_SUPPORT
    HI_S32 k;
#endif

    if (argc < 4)
    {
        printf("Usage: %s file vo_fomat [width] [height]\n", argv[0]);
        printf("Example: %s ./test.ts 1080P60 1280 720\n", argv[0]);
        printf("Note: if not set width and height, set them to default value: 1280 720");
        return 0;
    }

    g_pTsFile = fopen(argv[1], "rb");
    if (!g_pTsFile)
    {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }
    g_ts_play_file = fopen(argv[1], "rb");
    if (!g_ts_play_file)
    {
        fclose(g_pTsFile);
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    if (argv[2] != NULL) {
        enFormat = hi_adp_str2fmt(argv[2]);
    }

    if (argv[3] != NULL && argv[4] != NULL) {
        width = atoi(argv[3]);
        height = atoi(argv[4]);
    } else {
        width = 1280;
        height = 720;
    }


    hi_unf_sys_init();

    HIADP_MCE_Exit();

    Ret = HIADP_Snd_Init();
    if (HI_SUCCESS != Ret)
    {
        printf("call SndInit failed.\n");
        goto SYS_DEINIT;
    }

    Ret = hi_adp_disp_init(enFormat);
    if (HI_SUCCESS != Ret)
    {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    Ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_NORMAL);

#ifdef PLAY_AND_TRANSCODE_SUPPORT
    Ret |= HIADP_VO_CreatWin(HI_NULL, &hWin);
    if (HI_SUCCESS != Ret)
    {
        printf("call VoInit failed.\n");
        HIADP_VO_DeInit();
        goto DISP_DEINIT;
    }
#endif

    Ret = hi_unf_dmx_init();
    if (HI_SUCCESS != Ret)
    {
        printf("call hi_unf_dmx_init failed.\n");
        goto DISP_DEINIT;
    }

    Ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID_1, HI_UNF_DMX_PORT_RAM_1);
    if (HI_SUCCESS != Ret)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto VO_DEINIT;
    }

    HI_UNF_DMX_TSBUF_ATTR_S tsbuf_attr;
    tsbuf_attr.enSecureMode = HI_UNF_DMX_SECURE_MODE_NONE;
    tsbuf_attr.u32BufSize = 0x1000000;
    Ret = HI_UNF_DMX_CreateTSBuffer(HI_UNF_DMX_PORT_RAM_1, &tsbuf_attr, &g_TsBuf);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_CreateTSBuffer failed.\n");
        goto DMX_DEINIT;
    }

    Ret = HIADP_AVPlay_RegADecLib();
    if (HI_SUCCESS != Ret)
    {
        printf("call RegADecLib failed.\n");
        goto TSBUF_FREE;
    }

    Ret = HI_UNF_AVPLAY_Init();
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_Init failed.\n");
        goto TSBUF_FREE;
    }

    HI_UNF_AVPLAY_ATTR_S        AvplayAttr;
    Ret = HI_UNF_AVPLAY_GetDefaultConfig(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    AvplayAttr.u32DemuxId = PLAY_DMX_ID_1;
    //AvplayAttr.stStreamAttr.u32VidBufSize = (3*1024*1024);
    Ret |= HI_UNF_AVPLAY_Create(&AvplayAttr, &hAvplay);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_Create failed.\n");
        goto AVPLAY_DEINIT;
    }

    Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
        goto AVPLAY_DESTROY;
    }

    Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
        goto VCHN_CLOSE;
    }

#ifdef PLAY_AND_TRANSCODE_SUPPORT
    Ret = HI_UNF_VO_AttachWindow(hWin, hAvplay);
    if (HI_SUCCESS != Ret)
    {
        printf("call HI_UNF_VO_AttachWindow failed:%#x.\n",Ret);
    }
    Ret = HI_UNF_VO_SetWindowEnable(hWin, HI_TRUE);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_VO_SetWindowEnable failed.\n");
        goto WIN_DETACH;
    }
#endif

#ifdef PLAY_AND_TRANSCODE_SUPPORT
#ifdef AENC_SUPPORT
    HI_HANDLE                   hTrack;

    //stTrackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_MASTER;
    Ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_SND_GetDefaultTrackAttr failed.\n");
        goto WIN_DETACH;
    }
    Ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hTrack);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto WIN_DETACH;
    }

    Ret = HI_UNF_SND_Attach(hTrack, hAvplay);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }
#endif
#endif

    HI_UNF_SYNC_ATTR_S          SyncAttr;
    Ret = HI_UNF_AVPLAY_GetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
    SyncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
    Ret |= HI_UNF_AVPLAY_SetAttr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
        goto WIN_DETACH;
    }

    /*create virtual window */
    HI_UNF_WINDOW_ATTR_S stWinAttrVoVirtual;
    stWinAttrVoVirtual.enDisp = HI_UNF_DISPLAY0;
    stWinAttrVoVirtual.bVirtual = HI_TRUE;
    stWinAttrVoVirtual.enVideoFormat = HI_UNF_FORMAT_YUV_SEMIPLANAR_420;
    stWinAttrVoVirtual.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
    stWinAttrVoVirtual.stWinAspectAttr.enAspectCvrs = HI_UNF_VO_ASPECT_CVRS_IGNORE;
    stWinAttrVoVirtual.bUseCropRect = HI_FALSE;
    memset(&stWinAttrVoVirtual.stInputRect,0,sizeof(hi_unf_rect));
    memset(&stWinAttrVoVirtual.stOutputRect,0,sizeof(hi_unf_rect));
    stWinAttrVoVirtual.stOutputRect.s32Width = width;
    stWinAttrVoVirtual.stOutputRect.s32Height = height;
    HI_UNF_VO_CreateWindow(&stWinAttrVoVirtual, &hEncWin);

    HI_UNF_VO_AttachWindow(hEncWin, hAvplay);
    HI_UNF_VO_SetWindowEnable(hEncWin, HI_TRUE);

    /*create video encoder*/
    HI_UNF_VENC_Init();
    HI_UNF_VENC_GetDefaultAttr(&stVencChnAttr);
    stVencChnAttr.stGopAttr.enGopMode = HI_UNF_VENC_GOP_MODE_NORMALP;
    stVencChnAttr.stGopAttr.u32SPInterval = 0;
    stVencChnAttr.enVencType = HI_UNF_VCODEC_TYPE_H264;
    stVencChnAttr.enCapLevel = HI_UNF_VCODEC_CAP_LEVEL_4096x2160;
    stVencChnAttr.u32Width = width;
    stVencChnAttr.u32Height = height;
    stVencChnAttr.u32StrmBufSize   = width * height * 2;
    stVencChnAttr.u32TargetBitRate = 5 * 1024 * 1024;
    /*?????*/
    stVencChnAttr.u32InputFrmRate  = 60;
    stVencChnAttr.u32TargetFrmRate = 60;
    HI_UNF_VENC_Create(&hVenc, &stVencChnAttr);
    //HI_UNF_VENC_AttachInput(hVenc, hEncWin);

#ifdef AENC_SUPPORT
    /*create virtual track */
    HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &stTrackAttr);
    stTrackAttr.u32OutputBufSize = 256 * 1024;
    HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hEncTrack);
    //HI_UNF_SND_Attach(hEncTrack, hAvplay);

    /*create aenc*/
    HI_UNF_AENC_Init();
    HI_UNF_AENC_RegisterEncoder("libHA.AUDIO.AAC.encode.so");

    /*create audio decoder*/
    stAencAttr.enAencType = HA_AUDIO_ID_AAC;
    HA_AAC_GetDefaultConfig(&stPrivateConfig);
    HA_AAC_GetEncDefaultOpenParam(&(stAencAttr.sOpenParam),(HI_VOID *)&stPrivateConfig);
    HI_UNF_AENC_Create(&stAencAttr,&hAenc);
    /*attach audio decoder and virtual track*/
    HI_UNF_AENC_AttachInput(hAenc, hEncTrack);
#endif

#ifdef HIFB_SUPPORT
        for (k=0; k<FB_NUM; k++) {
            hi_fb_ui(k);
        }
#endif

    HI_HANDLE av_play, win_play, track_play;

    Ret = ts_play_prepare(&win_play, &track_play, &av_play);
    if (Ret != HI_SUCCESS) {
        printf("call ts_play_prepare failed: 0x%x.\n", Ret);
        goto WIN_DETACH;
    }

    g_bStopSaveThread = HI_FALSE;
    stSaveArgs.hVenc = hVenc;
#ifdef AENC_SUPPORT
    stSaveArgs.hAenc = hAenc;
#endif
    pthread_create(&thdSave, HI_NULL,(HI_VOID *)SaveThread,&stSaveArgs);

    pthread_mutex_init(&g_TsMutex,NULL);
    g_bStopTsThread = HI_FALSE;
    pthread_create(&g_TsThd, HI_NULL, (HI_VOID *)TsTthread, HI_NULL);


#ifdef HI_RECORD_OPEN
    for (j = 0; j < RECORD_COUNT; j++) {
        hi_demux_record_ts(j, RECORD_DMX_ID);
    }
#endif

    pthread_mutex_init(&g_ts_play_mtx, NULL);
    g_ts_play_stop = HI_FALSE;
    pthread_create(&g_ts_play_thd, HI_NULL, (HI_VOID *)ts_play_thread, HI_NULL);

    g_stop_transcode_thread = HI_FALSE;
    pthread_t trans_thread_id;
    transcode_thread_ctx trans_thread_arg;
    trans_thread_arg.window = hEncWin;
    trans_thread_arg.venc = hVenc;
    trans_thread_arg.pause = HI_TRUE;
    pthread_create(&trans_thread_id, HI_NULL, (HI_VOID *)transcode_thread, &trans_thread_arg);

    hi_adp_search_init();
    Ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_pProgTbl);
    if (Ret != HI_SUCCESS)
    {
        printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    ProgNum = 0;

    pthread_mutex_lock(&g_TsMutex);
    rewind(g_pTsFile);
    HI_UNF_DMX_ResetTSBuffer(g_TsBuf);

    Ret = avplay_play(hAvplay, g_pProgTbl, ProgNum, HI_FALSE);
    if (Ret != HI_SUCCESS)
    {
        printf("call avplay_play failed.\n");
        pthread_mutex_unlock(&g_TsMutex);
        goto AVPLAY_STOP;
    }

    pthread_mutex_unlock(&g_TsMutex);


    pthread_mutex_lock(&g_ts_play_mtx);
    rewind(g_ts_play_file);
    HI_UNF_DMX_ResetTSBuffer(g_ts_play_buf);
    Ret = avplay_play(av_play, g_pProgTbl, ProgNum, HI_TRUE);
    if (Ret != HI_SUCCESS)
    {
        printf("call play failed.\n");
        pthread_mutex_unlock(&g_ts_play_mtx);
        goto AVPLAY_STOP;
    }
    pthread_mutex_unlock(&g_ts_play_mtx);

    printf("please input q to quit\n" \
           "       input r to start transcode\n" \
           "       input s to stop transcode\n" \
           "       input 0~9 to change channel\n"\
           "       input h to print this message\n");

#ifdef AUTO_TRANS
    HI_UNF_VENC_Start(hVenc);
#ifdef AENC_SUPPORT
    HI_UNF_SND_Attach(hEncTrack, av_play);
#endif
    trans_thread_arg.pause = HI_FALSE;
    g_bSaveStream = HI_TRUE;
    printf("start transcode\n");
#endif

    while (1)
    {
        SAMPLE_GET_INPUTCMD(InputCmd);

        if ('q' == InputCmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        if ('h' == InputCmd[0])
        {
            printf("please input q to quit\n" \
                   "       input r to start transcode\n" \
                   "       input s to stop transcode\n" \
                   "       input 0~9 to change channel\n"\
                   "       input h to print this message\n");
            continue;
        }
#ifndef AUTO_TRANS
        if ('r' == InputCmd[0])
        {
            HI_UNF_VENC_Start(hVenc);
#ifdef AENC_SUPPORT
            HI_UNF_SND_Attach(hEncTrack, av_play);
#endif
            trans_thread_arg.pause = HI_FALSE;
            g_bSaveStream = HI_TRUE;
            printf("start transcode\n");
            continue;
        }
#endif

        if ('s' == InputCmd[0])
        {
            trans_thread_arg.pause = HI_TRUE;
            HI_UNF_VENC_Stop(hVenc);
#ifdef AENC_SUPPORT
            HI_UNF_SND_Detach(hEncTrack, av_play);
#endif
            g_bSaveStream = HI_FALSE;
            printf("stop transcode\n");
            continue;
        }

        if ( HI_TRUE == g_bSaveStream )
        {
            g_bSaveStream = HI_FALSE;
            trans_thread_arg.pause = HI_TRUE;
            HI_UNF_VO_SetWindowEnable(hEncWin, HI_FALSE);
            HI_UNF_VENC_Stop(hVenc);
#ifdef AENC_SUPPORT
            HI_UNF_SND_Detach(hEncTrack, av_play);
#endif
            HI_UNF_VO_DetachWindow(hEncWin, hAvplay);
            printf("stop transcode\n");
        }

        ProgNum = atoi(InputCmd);
        if (ProgNum == 0)
        {
            ProgNum = 1;
        }
        pthread_mutex_lock(&g_TsMutex);
        rewind(g_pTsFile);
        HI_UNF_DMX_ResetTSBuffer(g_TsBuf);
        Ret = avplay_play(hAvplay, g_pProgTbl, ProgNum - 1, HI_FALSE);
        if (Ret != HI_SUCCESS)
        {
            printf("call SwitchProgfailed!\n");
            pthread_mutex_unlock(&g_TsMutex);
            break;
        }
        pthread_mutex_unlock(&g_TsMutex);
        pthread_mutex_lock(&g_ts_play_mtx);
        rewind(g_ts_play_file);
        HI_UNF_DMX_ResetTSBuffer(g_ts_play_buf);
        Ret = avplay_play(av_play, g_pProgTbl, ProgNum - 1, HI_TRUE);
        if (Ret != HI_SUCCESS)
        {
            printf("call play failed.\n");
            pthread_mutex_unlock(&g_ts_play_mtx);
            goto AVPLAY_STOP;
        }
        pthread_mutex_unlock(&g_ts_play_mtx);
    }
#ifdef HI_RECORD_OPEN
    for (j = 0; j < RECORD_COUNT; j++) {
        hi_demux_record_ts_stop(j);
    }
#endif


AVPLAY_STOP:
    Stop.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    Stop.u32TimeoutMs = 0;
    HI_UNF_AVPLAY_Stop(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &Stop);
    HI_UNF_AVPLAY_Stop(av_play, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_pProgTbl);
    g_pProgTbl = HI_NULL;
    hi_adp_search_de_init();

    g_stop_transcode_thread = HI_TRUE;
    pthread_join(trans_thread_id, HI_NULL);

    g_bStopTsThread = HI_TRUE;
    pthread_join(g_TsThd, HI_NULL);
    pthread_mutex_destroy(&g_TsMutex);

    g_ts_play_stop = HI_TRUE;
    pthread_join(g_ts_play_thd, HI_NULL);
    pthread_mutex_destroy(&g_ts_play_mtx);

    g_bStopSaveThread = HI_TRUE;
    pthread_join(thdSave,HI_NULL);

#ifdef AENC_SUPPORT
    if (g_bSaveStream)
    {
        HI_UNF_SND_Detach(hEncTrack, av_play);
    }
    HI_UNF_AENC_DetachInput(hAenc);
    HI_UNF_AENC_Destroy(hAenc);
    HI_UNF_SND_DestroyTrack(hEncTrack);
#endif

    HI_UNF_VO_SetWindowEnable(hEncWin, HI_FALSE);

    if (g_bSaveStream)
    {
        printf("stop venc.");
        HI_UNF_VENC_Stop(hVenc);
    }

    HI_UNF_VO_DetachWindow(hEncWin, hAvplay);
    HI_UNF_VENC_DetachInput(hVenc);
    HI_UNF_VENC_Destroy(hVenc);
    HI_UNF_VO_DestroyWindow(hEncWin);

    (void)ts_play_exit(&win_play, &track_play, &av_play);


#ifdef PLAY_AND_TRANSCODE_SUPPORT
#ifdef AENC_SUPPORT
TRACK_DESTROY:
    HI_UNF_SND_DestroyTrack(hTrack);
#endif
#endif

WIN_DETACH:
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    HI_UNF_VO_SetWindowEnable(hWin, HI_FALSE);
    HI_UNF_VO_DetachWindow(hWin, hAvplay);
#endif

    HI_UNF_AVPLAY_ChnClose(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    HI_UNF_AVPLAY_ChnClose(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    HI_UNF_AVPLAY_Destroy(hAvplay);

AVPLAY_DEINIT:
    HI_UNF_AVPLAY_DeInit();

TSBUF_FREE:
    HI_UNF_DMX_DestroyTSBuffer(g_TsBuf);

DMX_DEINIT:
#ifndef HI_RECORD_OPEN
    HI_UNF_DMX_DetachTSPort(PLAY_DMX_ID_1);
#endif
    hi_unf_dmx_deinit();

VO_DEINIT:
#ifdef PLAY_AND_TRANSCODE_SUPPORT
    HI_UNF_VO_DestroyWindow(hWin);
#endif
    HIADP_VO_DeInit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    HIADP_Snd_DeInit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    fclose(g_pTsFile);
    g_pTsFile = HI_NULL;
    fclose(g_ts_play_file);
    g_ts_play_file = HI_NULL;

    return Ret;
}


