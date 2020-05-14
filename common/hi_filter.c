/**
 \file
 \brief aa
 \copyright Shenzhen Hisilicon Co., Ltd.
 \version draft
 \author
 \date 2011-8-8
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "hi_debug.h"
#include "hi_adp.h"
#include "hi_errno.h"
#include "hi_unf_demux.h"
#include "hi_filter.h"

/***************************** Macro Definition ******************************/
#define HI_FLT_REGION_NUM       3
#define HI_FLT_NUM_PERREGION    32
#define HI_MAX_CHN_NUM          DMX_CHANNEL_COUNT
#define HI_MAX_MSG_NUM          64

#define HI_FATAL_FLT(fmt...)    SAMPLE_COMMON_PRINTF(fmt)
#define HI_ERR_FLT(fmt...)      SAMPLE_COMMON_PRINTF(fmt)
#define HI_WARN_FLT(fmt...)     SAMPLE_COMMON_PRINTF(fmt)
#define HI_INFO_FLT(fmt...)     SAMPLE_COMMON_PRINTF(fmt)

#define HI_FLT_LOCK() \
    do {                                                    \
        (void)pthread_mutex_lock(&(g_FltMutex));           \
    } while (0)

#define HI_FLT_UNLOCK() \
    do {                                                    \
        (void)pthread_mutex_unlock(&(g_FltMutex));         \
    } while (0)

#define IsFltInit() \
    if (g_bFltInit == HI_FALSE) { \
        HI_FATAL_FLT("filter is not exist!\n"); \
        return HI_FAILURE; \
    } \

#define CheckFltID(id) \
    if ((id) >= DMX_CHANNEL_COUNT) { \
        HI_FATAL_FLT("invalid fltid:%d!\n", (id)); \
        return HI_FAILURE; \
    } \


#define DMX_GET_DEV_REGION(dmxid, region) \
    if ((dmxid) == 0) { \
        (region) = 0; \
    } \
    else if ((dmxid) == 4) { \
        (region) = 2; \
    } \
    else {\
        (region) = 1; \
    } \

#define FLT_REVERSE_MASK(mask) (hi_u8)(~(mask))

/*************************** Structure Definition ****************************/
typedef struct
{
    hi_u32 u32UseFlag;             /*use flag :0- available,1-busy*//*CNcomment:使用标记:0 - 未使用，1 - 使用*/
    hi_u32 u32EnableFlag;          /*enable flag :0- enable,1-disable*//*CNcomment:使能标记:0 - 未使能，1 - 使能*/

    hi_handle hChannel;            /*corresponding  channel of filter*//*CNcomment:filter对应的通道号*/
    hi_handle hFilter;             /*corresponding  hander of filter*//*CNcomment:filter对应的句柄*/

    hi_u32 u32TimerCount;          /*current value in timer of (ms)filter *//*CNcomment:  当前filter定时器计数值,ms*/

    HI_FILTER_ATTR_S stFltAttr;
} FILTER_ATTR_S;

/********************** Global Variable declaration **************************/
static FILTER_ATTR_S g_FltAttrArr[DMX_FILTER_COUNT];
static hi_bool g_bFltInit = HI_FALSE;
static hi_bool g_bFltRunFlag = HI_TRUE;
static pthread_mutex_t g_FltMutex;
static pthread_t g_FltThreadID;
static pthread_t g_FltTimerThread;

//feedback data to filter by callback
// CNcomment:将数据回调给每个filter
static hi_void Flt_DataCallBack(hi_handle Chan, hi_unf_dmx_data_type eDataType, hi_u8 *pData, hi_u32 u32SecLen )
{
    hi_u32 i = 0, k = 0;
    HI_FILTER_ATTR_S *pFilter;
    FILTER_ATTR_S    *pFltArrItem;

    if (HI_NULL == pData)
    {
        return;
    }

    for (i = 0; i < DMX_FILTER_COUNT; i++)
    {
        pFilter = &(g_FltAttrArr[i].stFltAttr);
        pFltArrItem = &(g_FltAttrArr[i]);

        if ((pFltArrItem->hChannel != Chan) || (0 == pFltArrItem->u32UseFlag) || (0 == pFltArrItem->u32EnableFlag))
        {
            continue;
        }

        if (0 == pFilter->u32FilterType && pFilter->u32FilterDepth != 0) /*section data,filter data by software*//*CNcomment:section 数据,对数据进行软过滤*/
        {
            if (pFilter->u8Negate[0])
            {
                if (((pFilter->u8Match[0]) & FLT_REVERSE_MASK(pFilter->u8Mask[0])) ==
                    ((*pData) & FLT_REVERSE_MASK(pFilter->u8Mask[0])))
                {
                    continue;
                }
            }
            else
            {
                if (((pFilter->u8Match[0]) & FLT_REVERSE_MASK(pFilter->u8Mask[0])) !=
                    ((*pData) & FLT_REVERSE_MASK(pFilter->u8Mask[0])))
                {
                    continue;
                }
            }

            for (k = 1; k < pFilter->u32FilterDepth; k++)
            {
                if (pFilter->u8Negate[k])
                {
                    if (((pFilter->u8Match[k]) & FLT_REVERSE_MASK(pFilter->u8Mask[k])) ==
                        ((*(pData + k + 2)) & FLT_REVERSE_MASK(pFilter->u8Mask[k])))
                    {
                        break;
                    }
                }
                else
                {
                    if (((pFilter->u8Match[k]) & FLT_REVERSE_MASK(pFilter->u8Mask[k])) !=
                        ((*(pData + k + 2)) & FLT_REVERSE_MASK(pFilter->u8Mask[k])))
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            k = pFilter->u32FilterDepth; /*do not filter pes*//*CNcomment:pes没有filter过滤*/
        }

        if (k == pFilter->u32FilterDepth) /*get matched filter condition *//*CNcomment: 得到匹配的过滤条件 */
        {
            if (pFilter->funCallback)
            {
                if (HI_FALSE == pFltArrItem->u32EnableFlag)
                {
                    continue;
                }

                pFltArrItem->u32TimerCount = 0;
                if ((pFilter->u32DirTransFlag == 0) && (pFilter->pu8BufAddr != HI_NULL)
                    && (pFilter->u32BufSize >= u32SecLen))
                {
                    hi_s32 ret;
                    ret = pFilter->funCallback(i, HI_FILTER_CALLBACK_TYPE_DATA_COME, eDataType, (void *)pData,
                                               u32SecLen);
                    if (0 == ret)
                    {
                        memcpy(pFilter->pu8BufAddr, pData, u32SecLen); /*copy data and return to caller,newer data will cover older data *//*CNcomment:拷贝数据并返回给上层,后面来的数据会覆盖前面的数据*/
                        pFilter->funCallback(i, HI_FILTER_CALLBACK_TYPE_DATA_IN_BUFFER, eDataType, pFilter->pu8BufAddr,
                                             u32SecLen);
                    }
                }
                else
                {
                    /*under transfer mode,data will give caller directly*//*CNcomment:透传模式下，数据直接给上层 */
                    pFilter->funCallback(i, HI_FILTER_CALLBACK_TYPE_DATA_COME, eDataType, (void *)pData, u32SecLen);
                }
            }
            else
            {
                HI_ERR_FLT("filter have no callback\n");
            }

            return; /*if return once find data that need to think over *//*CNcomment:是否找到后就return，需要斟*/
        }
    }

    return;
}

static hi_void* dmx_timer_task(hi_void *param)
{
    hi_u32 i;
    FILTER_ATTR_S    *pFlt;

    while (g_bFltRunFlag)
    {
        usleep(40 * 1000);

        for (i = 0; i < DMX_FILTER_COUNT; i++)
        {
            pFlt = &(g_FltAttrArr[i]);
            if (pFlt->u32UseFlag && pFlt->u32EnableFlag && pFlt->stFltAttr.u32TimeOutMs)
            {
                pFlt->u32TimerCount += 50;
                if (pFlt->u32TimerCount >= pFlt->stFltAttr.u32TimeOutMs)
                {
                    pFlt->stFltAttr.funCallback(i, HI_FILTER_CALLBACK_TYPE_TIMEOUT, 0, 0, 0);
                    pFlt->u32TimerCount = 0;
                }
            }
        }
    }

    return NULL;
}

static hi_void* flt_task( hi_void *param)
{
    hi_s32 ret = HI_FAILURE;
    hi_u32 u32MsgNum = 0;
    hi_u32 u32BufLen = 0;
    hi_u8  *pTmpBuff = 0;
    hi_u32 u32HandleNum = HI_MAX_CHN_NUM;
    hi_handle u32ChHandle[HI_MAX_CHN_NUM];
    hi_unf_dmx_data sRcvBuf[HI_MAX_MSG_NUM];
    hi_unf_dmx_chan_status channelstatus;
    hi_unf_dmx_chan_attr pstChAttr;
    hi_unf_dmx_data_type enDataType;
    hi_u32 i = 0, j = 0;

    while (g_bFltRunFlag)
    {
        memset((void *)u32ChHandle, 0, sizeof(hi_handle) * HI_MAX_CHN_NUM);

        u32HandleNum = HI_MAX_CHN_NUM;
        ret = hi_unf_dmx_get_data_handle(u32ChHandle, &u32HandleNum, 1000);  /* timeout 1000ms */
        if ((HI_SUCCESS != ret) || (u32HandleNum == 0))
        {
            continue;
        }

        for (i = 0; i < u32HandleNum; i++) /*amount of channel which has new data*//*CNcomment:有新数据的通道数*/
        {
            ret = hi_unf_dmx_get_play_chan_status(u32ChHandle[i], &channelstatus);
            if ((HI_SUCCESS != ret) || (channelstatus.is_opened == HI_FALSE))
            {
                continue;
            }

            u32MsgNum = 0;
            ret = hi_unf_dmx_acquire_buffer(u32ChHandle[i], HI_MAX_MSG_NUM, &u32MsgNum, sRcvBuf, 1000);
            if (HI_SUCCESS != ret)
            {
                HI_ERR_FLT("hi_unf_dmx_acquire_buffer error:%x\n", ret);
                continue;
            }

            ret = hi_unf_dmx_get_play_chan_attr(u32ChHandle[i], &pstChAttr);
            if (HI_SUCCESS != ret)
            {
                HI_ERR_FLT("hi_unf_dmx_get_play_chan_attr error:%x\n", ret);
                continue;
            }

            for (j = 0; j < u32MsgNum; j++) /*process data package gradually*//*CNcomment:逐个处理数据包*/
            {
                HI_FLT_LOCK();
                ret = hi_unf_dmx_get_play_chan_status(u32ChHandle[i], &channelstatus);
                if ((HI_SUCCESS != ret) || ((channelstatus.is_opened == HI_FALSE) && (HI_SUCCESS == ret)))
                {
                    HI_FLT_UNLOCK(); /*avoid reference sRcvBuf after closing channel *//*CNcomment:避免关闭通道后，又访问sRcvBuf*/
                    break;
                }

                pTmpBuff   = sRcvBuf[j].data;
                u32BufLen  = sRcvBuf[j].size;
                enDataType = sRcvBuf[j].data_type;

                if (HI_UNF_DMX_CHAN_TYPE_PES == pstChAttr.chan_type)
                {
                    Flt_DataCallBack(u32ChHandle[i], enDataType, pTmpBuff, u32BufLen);
                }
                else if (HI_UNF_DMX_CHAN_TYPE_SEC == pstChAttr.chan_type)
                {
                    Flt_DataCallBack(u32ChHandle[i], HI_UNF_DMX_DATA_TYPE_WHOLE, pTmpBuff, u32BufLen);
                }

                HI_FLT_UNLOCK();
            }

            /*release message*//*CNcomment: 释放消息 */
            HI_FLT_LOCK();
            ret = hi_unf_dmx_get_play_chan_status(u32ChHandle[i], &channelstatus);
            if ((HI_SUCCESS != ret) || (channelstatus.is_opened == HI_FALSE))
            {
                HI_FLT_UNLOCK();
                continue;
            }
            else
            {
                ret = hi_unf_dmx_release_buffer(u32ChHandle[i], u32MsgNum, sRcvBuf);
                if (HI_SUCCESS != ret)
                {
                    HI_ERR_FLT("hi_unf_dmx_release_buffer error:%x\n", ret);
                }

                HI_FLT_UNLOCK();
            }
        }
    }

    return NULL;
}

static hi_s32 CheckFltAttr(HI_FILTER_ATTR_S *pstFilterAttr)
{
    if (!pstFilterAttr)
    {
        return -1;
    }

    if ((pstFilterAttr->u32DirTransFlag == 0) && (!pstFilterAttr->pu8BufAddr
                                                  || (pstFilterAttr->u32BufSize == 0))) /*did not send available buffer under un-transfer mode *//*CNcomment:非透传模式没有传入合法的buffer */
    {
        HI_ERR_FLT("error buffer\n");
        return -1;
    }

    if ((pstFilterAttr->u32FilterType != 0) && (pstFilterAttr->u32FilterType != 1))
    {
        HI_ERR_FLT("error filter type:%x!\n", pstFilterAttr->u32FilterType);
        return -1;
    }

    if (pstFilterAttr->u32CrcFlag > 2)
    {
        HI_ERR_FLT("error u32CrcFlag flag:%x\n", pstFilterAttr->u32CrcFlag);
        return -1;
    }

    if (pstFilterAttr->u32FilterDepth > DMX_FILTER_MAX_DEPTH)
    {
        HI_ERR_FLT("error filter depth:%x\n", pstFilterAttr->u32FilterDepth);
        return -1;
    }

    return 0;
}

/*pu32FltId's value scope is 0-95 *//*CNcomment:pu32FltId获取到的范围为0-95*/
static hi_s32 GetFreeFltId(hi_u32 u32DmxID, hi_u32 *pu32FltId)
{
    hi_u32 i;
    hi_u32 u32RegionNum;
    FILTER_ATTR_S *pFilter;

    DMX_GET_DEV_REGION(u32DmxID, u32RegionNum);
    for (i = 0; i < HI_FLT_NUM_PERREGION; i++)
    {
        pFilter = &(g_FltAttrArr[u32RegionNum * HI_FLT_NUM_PERREGION + i]);
        if (pFilter->u32UseFlag == 0)
        {
            *pu32FltId = u32RegionNum * HI_FLT_NUM_PERREGION + i;
            return 0;
        }
    }

    return -1;
}

static hi_u32 GetChnFltNum(hi_s32 u32DmxID, hi_handle Chan)
{
    hi_u32 i = 0;
    hi_u32 u32RegionNum;
    hi_u32 u32FltNum = 0;
    FILTER_ATTR_S *pFilter;

    DMX_GET_DEV_REGION(u32DmxID, u32RegionNum);
    for (i = 0; i < HI_FLT_NUM_PERREGION; i++)
    {
        pFilter = &(g_FltAttrArr[u32RegionNum * HI_FLT_NUM_PERREGION + i]);
        if (!pFilter->u32UseFlag)
        {
            continue;
        }

        if (pFilter->hChannel == Chan)
        {
            u32FltNum++;
        }
    }

    return u32FltNum;
}

/*get filter amount which channel is enable *//*CNcomment:获取通道使能的filter数目*/
static hi_u32 GetChnEnFltNum(hi_s32 u32DmxID, hi_handle Chan)
{
    hi_u32 i = 0;
    hi_u32 u32RegionNum;
    hi_u32 u32FltNum = 0;
    FILTER_ATTR_S *pFilter;

    DMX_GET_DEV_REGION(u32DmxID, u32RegionNum);
    for (i = 0; i < HI_FLT_NUM_PERREGION; i++)
    {
        pFilter = &(g_FltAttrArr[u32RegionNum * HI_FLT_NUM_PERREGION + i]);
        if (!pFilter->u32UseFlag || !pFilter->u32EnableFlag)
        {
            continue;
        }

        if (pFilter->hChannel == Chan)
        {
            u32FltNum++;
        }
    }

    return u32FltNum;
}

hi_s32 HI_FILTER_Init(hi_void)
{
    hi_s32 ret;

    if (g_bFltInit == HI_TRUE)
    {
        return HI_SUCCESS;
    }

    HI_FLT_LOCK();
    ret = hi_unf_dmx_init();
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_init error:%x\n", ret);
        return HI_FAILURE;
    }

    memset(g_FltAttrArr, 0, sizeof(FILTER_ATTR_S) * DMX_FILTER_COUNT);
    pthread_mutex_init(&g_FltMutex, NULL);
    g_bFltRunFlag = HI_TRUE;
    ret  = pthread_create(&g_FltThreadID, NULL, flt_task, NULL);
    ret |= pthread_create(&g_FltTimerThread, NULL, dmx_timer_task, NULL);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("create flt_task error:%x\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    g_bFltInit = HI_TRUE;
    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_DeInit(hi_void)
{
    hi_s32 ret;

    if (g_bFltInit == HI_FALSE)
    {
        return HI_SUCCESS;
    }

    HI_FLT_LOCK();
    g_bFltRunFlag = HI_FALSE;
    usleep(100 * 1000);
    pthread_join(g_FltThreadID, 0);
    pthread_join(g_FltTimerThread, 0);
    ret = hi_unf_dmx_deinit();
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_init error:%x\n", ret);
        return HI_FAILURE;
    }

    g_bFltInit = HI_FALSE;
    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_Creat(HI_FILTER_ATTR_S *pstFilterAttr, hi_s32 *ps32FilterID)
{
    hi_u32 u32FltID;
    hi_unf_dmx_filter_attr sFilterAttr;
    hi_unf_dmx_chan_attr stChAttr;
    hi_handle hFilter;
    hi_handle hChannel;
    FILTER_ATTR_S* ps_FltAttr;
    hi_s32 ret;

    IsFltInit();
    if (CheckFltAttr(pstFilterAttr) != 0)
    {
        HI_ERR_FLT("invalid param!\n");
        return HI_FAILURE;
    }

    HI_FLT_LOCK();
    if (GetFreeFltId(pstFilterAttr->u32DMXID, &u32FltID) != 0)
    {
        HI_ERR_FLT("no free filter!\n");
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    memset(&sFilterAttr, 0, sizeof(hi_unf_dmx_filter_attr));
    sFilterAttr.filter_depth = pstFilterAttr->u32FilterDepth;
    memcpy(sFilterAttr.mask, pstFilterAttr->u8Mask, pstFilterAttr->u32FilterDepth);
    memcpy(sFilterAttr.match, pstFilterAttr->u8Match, pstFilterAttr->u32FilterDepth);
    memcpy(sFilterAttr.negate, pstFilterAttr->u8Negate, pstFilterAttr->u32FilterDepth);
    ret = hi_unf_dmx_create_filter(pstFilterAttr->u32DMXID, &sFilterAttr, &hFilter);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_create_filter error:%x !\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_get_pid_chan_handle(pstFilterAttr->u32DMXID, pstFilterAttr->u32PID, &hChannel);
    if ((HI_SUCCESS != ret) && (HI_ERR_DMX_UNMATCH_CHAN != ret))
    {
        HI_ERR_FLT("HI_UNF_DMX_GetChannelHandle error:%x !\n", ret);
        hi_unf_dmx_destroy_filter(hFilter);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if ((HI_ERR_DMX_UNMATCH_CHAN == ret) || (HI_INVALID_HANDLE == hChannel))
    {
        /*do have channel for this PID,it need to create new channel *//*CNcomment:没有该pid的通道，需要创建新的通道 */
        ret = hi_unf_dmx_get_play_chan_default_attr(&stChAttr);
        if (HI_SUCCESS != ret)
        {
            HI_ERR_FLT("hi_unf_dmx_get_play_chan_default_attr error:%x !\n", ret);
            hi_unf_dmx_destroy_filter(hFilter);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }

        if (pstFilterAttr->u32CrcFlag == 1)
        {
            stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORCE_AND_DISCARD;
        }
        else if (pstFilterAttr->u32CrcFlag == 2)
        {
            stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
        }
        else
        {
            stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
        }

        if (pstFilterAttr->u32FilterType == 1)
        {
            stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_PES;
        }
        else
        {
            stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
        }


        stChAttr.buffer_size = 512 * 1024; //not used,don't care

        ret = hi_unf_dmx_create_play_chan(pstFilterAttr->u32DMXID, &stChAttr, &hChannel);
        if (HI_SUCCESS != ret)
        {
            hi_unf_dmx_destroy_filter(hFilter);
            HI_ERR_FLT("hi_unf_dmx_create_play_chan error:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }

        ret = hi_unf_dmx_set_play_chan_pid(hChannel, pstFilterAttr->u32PID);
        if (HI_SUCCESS != ret)
        {
            hi_unf_dmx_destroy_filter(hFilter);
            hi_unf_dmx_destroy_play_chan(hChannel);
            HI_ERR_FLT("hi_unf_dmx_set_play_chan_pid error:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }
    }

    ps_FltAttr = &g_FltAttrArr[u32FltID];
    memset(ps_FltAttr, 0, sizeof(FILTER_ATTR_S));
    ps_FltAttr->u32EnableFlag = 0;
    ps_FltAttr->u32TimerCount = 0;
    ps_FltAttr->hFilter  = hFilter;
    ps_FltAttr->hChannel = hChannel;
    memcpy(&ps_FltAttr->stFltAttr, pstFilterAttr, sizeof(HI_FILTER_ATTR_S));
    ps_FltAttr->u32UseFlag = 1;
    *ps32FilterID = u32FltID;

    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_Destroy(hi_s32 s32FilterID)
{
    FILTER_ATTR_S* ps_FltAttr;
    hi_s32 ret;

    IsFltInit();
    CheckFltID(s32FilterID);
    HI_FLT_LOCK();
    ps_FltAttr = &g_FltAttrArr[s32FilterID];
    if ((HI_INVALID_HANDLE == ps_FltAttr->hFilter) || (HI_INVALID_HANDLE == ps_FltAttr->hChannel))
    {
        HI_ERR_FLT("filter handle error !\n");
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if (ps_FltAttr->u32EnableFlag)
    {
        ret = hi_unf_dmx_detach_filter(ps_FltAttr->hFilter, ps_FltAttr->hChannel);
        if (HI_SUCCESS != ret)
        {
            HI_ERR_FLT("hi_unf_dmx_detach_filter error:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }

        ps_FltAttr->u32EnableFlag = 0;
    }

    ret = hi_unf_dmx_destroy_filter(ps_FltAttr->hFilter);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_destroy_filter error:%x !\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    ps_FltAttr->u32UseFlag = 0;
    if (!GetChnFltNum(ps_FltAttr->stFltAttr.u32DMXID, ps_FltAttr->hChannel)) /*if the channel did not attach filter,destroy the channel*//*CNcomment:通道上没有绑定filter，则销毁通道*/
    {
        ret = hi_unf_dmx_destroy_play_chan(ps_FltAttr->hChannel);
        if (HI_SUCCESS != ret)
        {
            HI_ERR_FLT("hi_unf_dmx_destroy_play_chan error:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }
    }

    ps_FltAttr->hChannel = HI_INVALID_HANDLE;
    ps_FltAttr->hFilter = HI_INVALID_HANDLE;
    ps_FltAttr->u32TimerCount = 0;

    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_SetAttr(hi_s32 s32FilterID, HI_FILTER_ATTR_S *pstFilterAttr)
{
    FILTER_ATTR_S* ps_FltAttr;
    hi_unf_dmx_filter_attr sFilterAttr;
    hi_unf_dmx_chan_attr sChanAttr;
    hi_s32 ret;

    IsFltInit();
    CheckFltID(s32FilterID);
    if (CheckFltAttr(pstFilterAttr) != 0)
    {
        HI_ERR_FLT("invalid param!\n");
        return HI_FAILURE;
    }

    HI_FLT_LOCK();
    ps_FltAttr = &g_FltAttrArr[s32FilterID];
    if (pstFilterAttr->u32FilterType != ps_FltAttr->stFltAttr.u32FilterType)
    {
        HI_ERR_FLT("can not change the channel type!\n");
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if ((HI_INVALID_HANDLE == ps_FltAttr->hFilter) || (HI_INVALID_HANDLE == ps_FltAttr->hChannel))
    {
        HI_ERR_FLT("filter handle error !\n");
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    memset(&sFilterAttr, 0, sizeof(hi_unf_dmx_filter_attr));
    sFilterAttr.filter_depth = pstFilterAttr->u32FilterDepth;
    memcpy(sFilterAttr.mask, pstFilterAttr->u8Mask, pstFilterAttr->u32FilterDepth);
    memcpy(sFilterAttr.match, pstFilterAttr->u8Match, pstFilterAttr->u32FilterDepth);
    memcpy(sFilterAttr.negate, pstFilterAttr->u8Negate, pstFilterAttr->u32FilterDepth);
    ret = hi_unf_dmx_set_filter_attr(ps_FltAttr->hFilter, &sFilterAttr);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_set_filter_attr error:%x !\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if (ps_FltAttr->stFltAttr.u32CrcFlag != pstFilterAttr->u32CrcFlag)
    {
        ret = hi_unf_dmx_get_play_chan_attr(ps_FltAttr->hChannel, &sChanAttr);
        if (HI_SUCCESS != ret)
        {
            HI_ERR_FLT("hi_unf_dmx_get_play_chan_attr error:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }

        if (sChanAttr.chan_type == HI_UNF_DMX_CHAN_TYPE_SEC)
        {
            if ((ps_FltAttr->stFltAttr.u32CrcFlag == 1)
                && (sChanAttr.crc_mode != HI_UNF_DMX_CHAN_CRC_MODE_FORCE_AND_DISCARD))
            {
                sChanAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORCE_AND_DISCARD;
                ret  = hi_unf_dmx_close_play_chan(ps_FltAttr->hChannel);
                ret |= hi_unf_dmx_set_play_chan_attr(ps_FltAttr->hChannel, &sChanAttr);
                ret |= hi_unf_dmx_open_play_chan(ps_FltAttr->hChannel);
            }
            else if ((ps_FltAttr->stFltAttr.u32CrcFlag == 2) && (sChanAttr.crc_mode != HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD))
            {
                sChanAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
                ret  = hi_unf_dmx_close_play_chan(ps_FltAttr->hChannel);
                ret |= hi_unf_dmx_set_play_chan_attr(ps_FltAttr->hChannel, &sChanAttr);
                ret |= hi_unf_dmx_open_play_chan(ps_FltAttr->hChannel);
            }
            else if ((ps_FltAttr->stFltAttr.u32CrcFlag == 0) && (sChanAttr.crc_mode != HI_UNF_DMX_CHAN_CRC_MODE_FORBID))
            {
                sChanAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
                ret  = hi_unf_dmx_close_play_chan(ps_FltAttr->hChannel);
                ret |= hi_unf_dmx_set_play_chan_attr(ps_FltAttr->hChannel, &sChanAttr);
                ret |= hi_unf_dmx_open_play_chan(ps_FltAttr->hChannel);
            }

            if (HI_SUCCESS != ret)
            {
                HI_ERR_FLT("set crc mode error:%x !\n", ret);
                HI_FLT_UNLOCK();
                return HI_FAILURE;
            }
        }
    }

    memcpy(&ps_FltAttr->stFltAttr, pstFilterAttr, sizeof(HI_FILTER_ATTR_S));
    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_GetAttr (hi_s32 s32FilterID, HI_FILTER_ATTR_S *pstFilterAttr)
{
    FILTER_ATTR_S* ps_FltAttr;

    IsFltInit();
    CheckFltID(s32FilterID);
    if (!pstFilterAttr)
    {
        HI_ERR_FLT("null pointer!\n");
        return HI_FAILURE;
    }

    HI_FLT_LOCK();
    ps_FltAttr = &g_FltAttrArr[s32FilterID];
    memcpy(pstFilterAttr, &ps_FltAttr->stFltAttr, sizeof(HI_FILTER_ATTR_S));
    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_Start(hi_s32 s32FilterID)
{
    FILTER_ATTR_S* ps_FltAttr;
    hi_unf_dmx_chan_status stStatus;
    hi_s32 ret;

    IsFltInit();
    CheckFltID(s32FilterID);
    HI_FLT_LOCK();
    ps_FltAttr = &g_FltAttrArr[s32FilterID];
    if ((HI_INVALID_HANDLE == ps_FltAttr->hFilter) || (HI_INVALID_HANDLE == ps_FltAttr->hChannel))
    {
        HI_ERR_FLT("filter handle error !\n");
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if (ps_FltAttr->u32EnableFlag)
    {
        HI_ERR_FLT("filter already started!\n");
        HI_FLT_UNLOCK();
        return HI_SUCCESS;
    }

    ret = hi_unf_dmx_attach_filter(ps_FltAttr->hFilter, ps_FltAttr->hChannel);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_attach_filter error:%x !\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    ps_FltAttr->u32EnableFlag = 1;
    ps_FltAttr->u32TimerCount = 0;
    ret = hi_unf_dmx_get_play_chan_status(ps_FltAttr->hChannel, &stStatus);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_get_play_chan_status failed:%x !\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if (stStatus.is_opened == HI_FALSE)
    {
        ret = hi_unf_dmx_open_play_chan(ps_FltAttr->hChannel);
        if (HI_SUCCESS != ret)
        {
            HI_ERR_FLT("hi_unf_dmx_open_play_chan failed:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }
    }

    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}

hi_s32 HI_FILTER_Stop(hi_s32 s32FilterID)
{
    FILTER_ATTR_S* ps_FltAttr;
    hi_unf_dmx_chan_status stStatus;
    hi_s32 ret;

    IsFltInit();
    CheckFltID(s32FilterID);
    HI_FLT_LOCK();
    ps_FltAttr = &g_FltAttrArr[s32FilterID];
    if ((HI_INVALID_HANDLE == ps_FltAttr->hFilter) || (HI_INVALID_HANDLE == ps_FltAttr->hChannel))
    {
        HI_ERR_FLT("filter handle error !\n");
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    if (!ps_FltAttr->u32EnableFlag)
    {
        HI_ERR_FLT("filter already stoped!\n");
        HI_FLT_UNLOCK();
        return HI_SUCCESS;
    }

    ret = hi_unf_dmx_detach_filter(ps_FltAttr->hFilter, ps_FltAttr->hChannel);
    if (HI_SUCCESS != ret)
    {
        HI_ERR_FLT("hi_unf_dmx_detach_filter error:%x !\n", ret);
        HI_FLT_UNLOCK();
        return HI_FAILURE;
    }

    ps_FltAttr->u32EnableFlag = 0;
    ps_FltAttr->u32TimerCount = 0;

    if (!GetChnEnFltNum(ps_FltAttr->stFltAttr.u32DMXID, ps_FltAttr->hChannel)) /*if the channel did not attach filter,close the channel*//*CNcomment:通道上没有绑定filter，则关闭通道*/
    {
        ret = hi_unf_dmx_get_play_chan_status(ps_FltAttr->hChannel, &stStatus);
        if (HI_SUCCESS != ret)
        {
            HI_ERR_FLT("hi_unf_dmx_get_play_chan_status failed:%x !\n", ret);
            HI_FLT_UNLOCK();
            return HI_FAILURE;
        }

        if (stStatus.is_opened != HI_FALSE)
        {
            ret = hi_unf_dmx_close_play_chan(ps_FltAttr->hChannel);
            if (HI_SUCCESS != ret)
            {
                HI_ERR_FLT("hi_unf_dmx_close_play_chan error:%x !\n", ret);
                HI_FLT_UNLOCK();
                return HI_FAILURE;
            }
        }
    }

    HI_FLT_UNLOCK();
    return HI_SUCCESS;
}
