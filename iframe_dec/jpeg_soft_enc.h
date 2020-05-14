/*
 *
 * Copyright (C) 2018 Hisilicon Technologies Co., Ltd.  All rights reserved.
 *
 * This program is confidential and proprietary to Hisilicon  Technologies Co., Ltd. (Hisilicon),
 * and may not be copied, reproduced, modified, disclosed to others, published or used, in
 * whole or in part, without the express prior written permission of Hisilicon.
 *
 */

#ifndef __SAMPLE_ENC_API_H__
#define __SAMPLE_ENC_API_H__


/*********************************add include here***********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <setjmp.h>
#include <sys/time.h>
#include "hi_type.h"

/***********************************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/***************************** Macro Definition ***********************************************/
#define JPGE_INPUT_FMT_YUV420               0
#define JPGE_INPUT_SAMPLER_SEMIPLANNAR      0
#define JPGE_INPUT_COMPONENT_NUM            3

/*************************** Structure Definition *********************************************/
typedef struct tagSAMPLE_VIDEO_FREMAE_INFO_S
{
    hi_u32  YuvSampleType;
    hi_u32  YuvFmtType;
    hi_u32  Qlevel;

    hi_u32  Width[JPGE_INPUT_COMPONENT_NUM];
    hi_u32  Height[JPGE_INPUT_COMPONENT_NUM];
    hi_u32  Stride[JPGE_INPUT_COMPONENT_NUM];
    hi_u32  PhyBuf[JPGE_INPUT_COMPONENT_NUM];
}SAMPLE_VIDEO_FREMAE_INFO_S;

/******************************* API declaration **********************************************/
/****************************************************************************
* func          : JPEG_Soft_EncToFile
* description   : CNcomment: 编码成jpeg图片到文件 CNend\n
* param[in]     : 只支持YUV420 SP格式的码流
* retval        : NA
* others:       : NA
******************************************************************************/
hi_s32 JPEG_Soft_EncToFile(hi_char *pOutFileName, SAMPLE_VIDEO_FREMAE_INFO_S *pstVideoFrameInfo);

/****************************************************************************
* func          : JPEG_Soft_EncToMem
* description   : CNcomment: 编码成jpeg图片到内存 CNend\n
* param[in]     : 只支持YUV420 SP格式的码流
* retval        : NA
* others:       : NA
******************************************************************************/
hi_s32 JPEG_Soft_EncToMem(SAMPLE_VIDEO_FREMAE_INFO_S *pstVideoFrameInfo, hi_u8 **ppOutBuffer, hi_u32 *pBufferSize);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __SAMPLE_ENC_API_H__*/
