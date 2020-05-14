/**
 \file
 \brief dmx adaption
 \copyright Shenzhen Hisilicon Co., Ltd.
 \version draft
 \author
 \date 2011-8-8
 */

#ifndef __HI_FILTER_H__
#define __HI_FILTER_H__

/* add include here */
#include "hi_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DMX_FILTER_COUNT                96
#define DMX_CHANNEL_COUNT               96

/***************************** Macro Definition ******************************/

/*************************** Structure Definition ****************************/
/*call back envent type*/ /*CNcomment:回调函数事件类型 */
typedef enum hiFILTER_CALLBACK_TYPE_E
{
    HI_FILTER_CALLBACK_TYPE_DATA_COME,       /**< 数据到达内部buffer    */
    HI_FILTER_CALLBACK_TYPE_DATA_IN_BUFFER,  /**< 数据已拷贝到外部buffer   */
    HI_FILTER_CALLBACK_TYPE_TIMEOUT,         /**< 等待数据超时 */
    HI_FILTER_CALLBACK_TYPE_BUTT
} HI_FILTER_CALLBACK_TYPE_E;

/*Transparent transmission mode, only the data to reach the internal buffer of events and timeout events, not a copy of the data does not appear to copy data to an external buffer event
    In copy mode, in addition to the data reaches the internal buffer of events and timeout events, there will be a copy of the data to the external buffer events.
      However, in the copy mode, when the reach the data of the external buffer event, the user via the callback function's return value to confirm whether to continue copying, in order to discard the unwanted duplicate data.
      At this point, the return value of 0 means copy a copy, and direct release.*/
/*CNcomment:
    在透传模式下，只有数据到达内部buffer的事件和超时事件，不会进行数据的拷贝也不会出现数据拷贝到外部buffer的事件
    在拷贝模式下，除了数据到达内部buffer的事件和超时事件外，还会有一个数据拷贝到外部buffer的事件。
      不过在拷贝模式下，当发生数据到达外部buffer事件时，用户可以通过回调函数的返回值确认是否继续进行拷贝，以便直接丢弃不需要的重复数据。
      此时，返回值为0表示进行拷贝，为1表示不拷贝，直接释放。

 */
typedef hi_s32 (*HI_FILTER_CALLBACK)(hi_s32 s32Filterid, HI_FILTER_CALLBACK_TYPE_E enCallbackType,
                                     hi_unf_dmx_data_type eDataType, hi_u8 *pu8Buffer, hi_u32 u32BufferLength);

typedef struct  hiFILTER_ATTR_S
{
    hi_u32  u32DMXID;              /* DMX ID ,0-4*/
    hi_u32  u32PID;                 /* TS PID */
    hi_u8  *pu8BufAddr;             /*Transparent transmission mode set to NULL,otherwise you need to get a buffer and record here*//*CNcomment:在非透传模式下，保存接收数据的buffer地址，需要先申请一段内存，然后传入指针,透传模式设置为NULL */
    hi_u32  u32BufSize;             /* buffer size,Transparent transmission mode set to 0 */

    hi_u32 u32DirTransFlag;        /* if Transparent transmission mode，0 -Not Transparent transmission mode 1 - Transparent transmission mode */
    hi_u32 u32FilterType;          /* section type 0 - SECTION  1- PES */
    hi_u32 u32CrcFlag;             /* crc check flag,0 - close crc check 1-force crc check 2-crc check by sytax*/
    hi_u32 u32TimeOutMs;           /*time out in ms,0 stand for not timeout,otherwise when timeout,the user can receive the envent*//*CNcomment: 超时时间(ms)，超过此时间filter仍然没有数据，则进行事件回调 如果为0则表示不需要超时事件*/

    hi_u32 u32FilterDepth;          /*Filter Depth*/
    hi_u8  u8Match[DMX_FILTER_MAX_DEPTH];
    hi_u8  u8Mask[DMX_FILTER_MAX_DEPTH];
    hi_u8  u8Negate[DMX_FILTER_MAX_DEPTH];

    HI_FILTER_CALLBACK funCallback;         /* data comes callback */
} HI_FILTER_ATTR_S;

/********************** Global Variable declaration **************************/

/******************************* API declaration *****************************/
hi_s32   HI_FILTER_Init(hi_void);
hi_s32   HI_FILTER_DeInit(hi_void);
hi_s32   HI_FILTER_Creat(HI_FILTER_ATTR_S *pstFilterAttr, hi_s32 *ps32FilterID);
hi_s32   HI_FILTER_Destroy(hi_s32 s32FilterID);
hi_s32   HI_FILTER_SetAttr(hi_s32 s32FilterID, HI_FILTER_ATTR_S *pstFilterAttr);
hi_s32   HI_FILTER_GetAttr (hi_s32 s32FilterID, HI_FILTER_ATTR_S *pstFilterAttr);
hi_s32   HI_FILTER_Start(hi_s32 s32FilterID);
hi_s32   HI_FILTER_Stop(hi_s32 s32FilterID);

#ifdef __cplusplus
}
#endif
#endif /* __HI_FILTER_H__ */
