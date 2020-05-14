/******************************************************************************

Copyright (C), 2004-2007, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_psi_si.h
Version       : Initial
Author        : Hisilicon multimedia software group
Created       : 2007-04-10
Last Modified :
Description   : Hi3560 PSI/SI function define
Function List :
History       :
* Version   Date         Author     DefectNum    Description
* main\1    2007-04-10   Hs         NULL         Create this file.
******************************************************************************/
#ifndef __HI_PSI_SI_H__
#define __HI_PSI_SI_H__

#include "hi_type.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* __cplusplus */

#define PRINT_PSISI(fmt...)     SAMPLE_COMMON_PRINTF(fmt)

typedef struct hiPROG_INFO_S
{
    hi_u16 ProgID;         /*Progam ID*/
    hi_u16 ProgPID;        /*PMT ID*/
} PROG_INFO;

typedef struct hiPAT_TBL_S
{
    hi_u32 ProgNum;
    hi_u32 TsID;
    hi_u8 *pat_buf;         /*PROG_INFO*/
} PAT_TBL;

typedef struct hiCAT_TBL_S
{
    hi_u32 len;
    hi_u8 *cat_buf;
} CAT_TBL;

typedef struct hiNIT_TSDESC_S
{
    hi_u16 TsID;            /*TS ID*/
    hi_u16 NetID;           /*net work ID*/
    hi_u16 TsDescLen;       /*TS desc len*/
    hi_u16 Reserved;
    struct hiNIT_TSDESC_S *Next;
    hi_u8 *TsDesc;          /*TS Desc*/
} NIT_TSDESC;

typedef struct hiNIT_TBL_S
{
    hi_u32 NetworkID;       /*Network ID*/
    hi_u32 NetDescLen;
    hi_u8 *NetDesc;
    hi_u32 StreamDescLen;
    hi_u8 *StreamDesc;
} NIT_TBL;

typedef struct hiPMT_Stream_S
{
    hi_u32 StreamType;
    hi_u32 ElementPid;
    hi_u32 ESInfoLen;
    struct hiPMT_Stream_S *Next;
    hi_u8 *ESDesc;
} PMT_Stream;

typedef struct hiPMT_Prog_S
{
    hi_u32 ProgID;
    hi_u32 PCR_PID;
    hi_u32 ProgDescLen;     /*Program Desc Len*/
    hi_u8 *ProgDescPtr;     /*Program Infor*/
    hi_u8 *ESDescPtr;       /*PMT_Stream*/
} PMT_Prog;

typedef struct hiPMT_TBL_S
{
    hi_u8 *pmt_buf;         /*PMT_Prog*/
} PMT_TBL;

typedef enum hiRUN_STATE_E
{
    UnDefined = 0,
    NotRun,
    StartInSeconds,
    Pause,
    Running,
    Run_Reserved1,
    Run_Reserved2,
    Run_Reserved3
} RUN_STATE_E;

typedef enum hiCA_MODE_E
{
    CA_NotNeed = 0,
    CA_Need
} CA_MODE_E;

typedef enum hiDESC_TAG_E
{
    Ser_Desc = 0x48,
    Short_evt_Desc = 0x4D,
    Ext_evt_Desc = 0x4E,
    TmShft_evt_Desc = 0x4F
} DESC_TAG_E;

typedef enum hiSERVICE_TYPE_E
{
    SvcType_Reserved1 = 0,
    Digital_TV,
    Digital_Radio,
    TeleText,
    Nvod_Refer,
    Nvod_TimeShift,
    Mosaic,
    PAL_Encode,
    SECAM_Encode,
    D2_MAC,
    ACodec_Digital_Radio,
    ACodec_Mosaic,
    Data_Broadcast,
    SvcType_Reserved5,
    RcsMap,
    RcsFls,
    DvbMhp,
    MPEG2_HD_Digital_TV,
    ACodec_SD_Digital_TV = 0x16,
    ACodec_SD_Nvod_TmShft,
    ACodec_SD_Nvod_Refer,
    ACodec_HD_Digital_TV,
    ACodec_HD_Nvod_TmShft,
    ACodec_HD_Nvod_Refer,
    INVALID_SER_TYPE = 0x100
} SERVICE_TYPE_E;

typedef struct hiSER_DESC_S
{
    SERVICE_TYPE_E SerType;
    hi_u32         PrvdNameLen;
    hi_u32         SerNameLen;
    hi_u8 *        PrvdName;
    hi_u8 *        SerName;
} SER_DESC_S;

typedef struct hiDESC_STRU_S
{
    DESC_TAG_E DescTag;
    hi_u32     DescLen;
    struct hiDESC_STRU_S *Next;
    hi_u8 *Desc;
} DESC_STRU_S;

typedef struct hiSER_INFO_S
{
    hi_u32      SerID;     /*Service ID*/
    hi_u32      SchdFlag;
    hi_u32      PrsntFlwFlag;
    RUN_STATE_E RunState;
    CA_MODE_E   CAMode;
    struct hiSER_INFO_S *Next;
    DESC_STRU_S *DescPtr;
} SER_INFO;

typedef struct hiSDT_TBL_S
{
    hi_u32 TsID;
    hi_u32 NetID;
    hi_u8 *ser_buf;         /*SER_INFO*/
} SDT_TBL;

typedef struct hiSHOTR_ETV_DESC_S
{
    hi_u8  Lang[4];
    hi_u32 EvtNameLen;
    hi_u32 TextLen;
    hi_u8 *EvtName;
    hi_u8 *Text;
} SHOTR_ETV_DESC_S;

typedef struct hiEVT_INFO_S
{
    hi_u32 EvtID;
    hi_u8  StartTime[4];
    hi_u8  Duration[4];
    hi_u16 RunState;        //program run state
    hi_u16 CAMode;
    struct hiEVT_INFO_S *Next;
    DESC_STRU_S *DescPtr;
} EVT_INFO;

typedef struct hiEIT_TBL_S
{
    hi_u32 TsID;
    hi_u32 NetID;
    hi_u8 *evt_buf;         /*EVT_INFO*/
} EIT_TBL;

hi_s32	HI_API_PSISI_GetPcrPid(hi_u32 u32DmxId, hi_u32 ProgID, hi_u32 *pPCR_PID);
hi_s32	HI_API_PSISI_GetPatTbl(hi_u32 u32DmxId, PAT_TBL **tbladdr, hi_u32 u32TimeOutMs);
hi_void HI_API_PSISI_FreePatTbl(hi_void);
hi_s32	HI_API_PSISI_GetCatTbl(hi_u32 u32DmxId, CAT_TBL **tbladdr, hi_u32 u32TimeOutms);
hi_void HI_API_PSISI_FreeCatTbl(hi_void);
hi_s32	HI_API_PSISI_GetNitTbl(hi_u32 u32DmxId, NIT_TBL **tbladdr, hi_u32 CurrFlag, hi_u32 u32TimeOutms);
hi_void HI_API_PSISI_FreeNitTbl(hi_void);
hi_s32	HI_API_PSISI_GetPmtTbl(hi_u32 u32DmxId, PMT_TBL **tbladdr, hi_u32 ProgID, hi_u32 u32TimeOutms);
hi_void HI_API_PSISI_FreePmtTbl(hi_void);
hi_s32	HI_API_PSISI_GetSdtTbl(hi_u32 u32DmxId, SDT_TBL **tbladdr, hi_u32 CurrFlag, hi_u32 TsID, hi_u32 NetID, hi_u32 u32TimeOutms);
hi_void HI_API_PSISI_FreeSdtTbl(hi_void);
hi_s32	HI_API_PSISI_GetEitTbl(hi_u32 u32DmxId, EIT_TBL **tbladdr, hi_u32 CurrFlag, hi_u32 SerID, hi_u32 TsID, hi_u32 NetID,
                               hi_u32 u32TimeOutms);
hi_void HI_API_PSISI_FreeEitTbl(hi_void);
hi_void HI_API_PSISI_Init(hi_void);
hi_void HI_API_PSISI_Destroy(hi_void);


#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* __cplusplus */

#endif /* __HI_PSI_SI_H__ */
