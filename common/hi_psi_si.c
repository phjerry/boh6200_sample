/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName   : hi_api_psi_si.c
* Description: application program interface implementation
*
* History    :
* Version   Date         Author     DefectNum    Description
* main\1    2006-09-20              NULL         Create this file.
***********************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <malloc.h>

#ifdef SFP
#include <linux/wait.h>
#else
#include <sys/wait.h>
#endif

#include <string.h>

#include "hi_adp.h"
#include "hi_unf_demux.h"
#include "hi_psi_si.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* __cplusplus */

#define VTOP_MemMalloc malloc
#define VTOP_MemFree free

/*PSI Table PID*/
#define MAX_SECTION_LEN 4096
#define INVALID_PID 0xFFFF
#define PAT_PID 0
#define CAT_PID 1
#define NIT_PID 0x10
#define SDT_PID 0x11
#define EIT_PID 0x12

hi_u16 PMT_PID = INVALID_PID;
hi_u32 pat_ch, cat_ch, nit_ch, pmt_ch, sdt_ch, eit_ch, PSISI_InitFlag = 0;
PAT_TBL PatTbl;
CAT_TBL CatTbl;
NIT_TBL NitTbl;
PMT_TBL PmtTbl;
SDT_TBL SdtTbl;
EIT_TBL EitTbl;
hi_u32 pat_filter_id, cat_filter_id, nit_filter_id, pmt_filter_id, sdt_filter_id, eit_filter_id;
hi_u32 gPSISIInitFlag = 0;

hi_u8 filter_match[DMX_FILTER_MAX_DEPTH];
hi_u8 filter_mask[DMX_FILTER_MAX_DEPTH];
hi_u8 filter_negate[DMX_FILTER_MAX_DEPTH];

hi_u32 PatSize = 32768;             //32K
hi_u32 CatSize = 32768;             //32K
hi_u32 NitNetSize = 32768;          //32K
hi_u32 NitStreamSize = 32768;       //32K
hi_u32 PmtSize = 4096;
hi_u32 SdtSize = 102400;            //100K
hi_u32 EitSize = 65536;             //64K
hi_u32 CpLen;

hi_void PSI_Filter_Reset()
{
    hi_u32 i;

    for (i = 0; i < DMX_FILTER_MAX_DEPTH; i++)
    {
        filter_match[i] = 0x00;
        filter_mask[i]   = 0xff;
        filter_negate[i] = 0x00;
    }
}

/*
    Timeout:in ms
 */
hi_s32 PSI_DataRead(hi_u32 ChannelId, hi_u32 u32TimeOutms, hi_u8 *pBuf,
                    hi_u32 Len, hi_u32* pCopyLen)
{
    hi_unf_dmx_data sSection;
    hi_u32 num = 0;

    *pCopyLen = 0;

    if (HI_SUCCESS != hi_unf_dmx_acquire_buffer(ChannelId, 1, &num, &sSection, u32TimeOutms))
    {
        PRINT_PSISI("hi_unf_dmx_acquire_buffer error\n");
        return HI_FAILURE;
    }

    PRINT_PSISI("read date num:%d len :0x%x\n", num, sSection.size);

    if (sSection.size > Len)
    {
        PRINT_PSISI("read date len error:0x%x, should small than 0x%x\n",
                sSection.size, Len);
    }

    memcpy(pBuf, sSection.data, sSection.size);
    *pCopyLen = sSection.size;

    return hi_unf_dmx_release_buffer(ChannelId, num, &sSection);
}

hi_s32 HI_API_PSISI_GetPcrPid(hi_u32 u32DmxId, hi_u32 ProgID, hi_u32 *pPCR_PID)
{
    PMT_TBL *pmt_tbl;
    PMT_Prog *pmt_prog;
    hi_s32 s32ret;

    if (pPCR_PID == HI_NULL_PTR)
    {
        return HI_FAILURE;
    }

    s32ret = HI_API_PSISI_GetPmtTbl(u32DmxId, &pmt_tbl, ProgID, 6000);
    if (s32ret == HI_SUCCESS)
    {
        pmt_prog  = (PMT_Prog *)(pmt_tbl->pmt_buf);
        *pPCR_PID = pmt_prog->PCR_PID;
        HI_API_PSISI_FreePmtTbl();
    }

    return s32ret;
}

hi_s32 HI_API_PSISI_GetPatTbl(hi_u32 u32DmxId, PAT_TBL **tbladdr, hi_u32 u32TimeOutms)
{
    hi_s32 s32ret, FirstFlag = 1;
    hi_u32 cur_section = 0, last_section = 0, prev_section = 0, bytescopied = 0, section_len = 0, N, i, CurNextInd;
    hi_u8  *bufptr, *curbufptr;
    hi_u8  *byteptr, *ptr_prog;
    hi_u16 ProgID, ProgPID;
    hi_s32 ReadResult;
    hi_unf_dmx_chan_attr stChAttr;
    hi_unf_dmx_filter_attr stFilterAttr;

    if (tbladdr == HI_NULL_PTR)
    {
        return HI_FAILURE;
    }

    if (PSISI_InitFlag == 0)
    {
        PRINT_PSISI("PSISI Error: Fail to get PAT table, PSISI has not initialized!\n");
        return HI_FAILURE;
    }

    if (PatTbl.pat_buf != HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get PAT table, PAT Table has not free!\n");
        return HI_FAILURE;
    }

    PatTbl.pat_buf = VTOP_MemMalloc(PatSize);
    if (PatTbl.pat_buf == HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get PAT table, PAT Table alloc fail!\n");
        return HI_FAILURE;
    }

    bufptr = malloc(MAX_SECTION_LEN);

    if (bufptr == HI_NULL_PTR)
    {
        VTOP_MemFree(PatTbl.pat_buf);
        PatTbl.pat_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get PAT table, section memory alloc fail!\n");
        return HI_FAILURE;
    }

    //set filter paramter
    PSI_Filter_Reset();
    filter_match[0] = 0x00;
    filter_mask[0]   = 0x00;
    filter_negate[0] = 0x00;

    hi_unf_dmx_get_play_chan_default_attr(&stChAttr);

    /*create channel of pat table */
    stChAttr.buffer_size = 16*1024;
    stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
    s32ret = hi_unf_dmx_create_play_chan(u32DmxId, &stChAttr, &pat_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_play_chan Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    //HI_WARN_DEMUX("pat channel: %d\n", pat_ch);
    s32ret = hi_unf_dmx_set_play_chan_pid(pat_ch, PAT_PID);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_set_play_chan_pid Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    stFilterAttr.filter_depth = 0;
    memcpy(stFilterAttr.match, filter_match, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.mask, filter_mask, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.negate, filter_negate, DMX_FILTER_MAX_DEPTH);
    s32ret = hi_unf_dmx_create_filter(u32DmxId, &stFilterAttr, &pat_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_set_filter_attr(pat_filter_id, &stFilterAttr);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_attach_filter(pat_filter_id, pat_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_attach_filter Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_open_play_chan(pat_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_open_play_chan Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    /*read first section*/
    ReadResult = PSI_DataRead(pat_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
    while (ReadResult == HI_SUCCESS)
    {
        cur_section = *(bufptr + 6);
        CurNextInd = (*(bufptr + 5)) & 0x01;

        //if setction is invalid,read next
        if ((cur_section != 0) || (CurNextInd == 0))
        {
            ReadResult = PSI_DataRead(pat_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
        }
        else
        {
            last_section = *(bufptr + 7);
            PatTbl.TsID = (((hi_u16)(*(bufptr + 3))) << 8) + *(bufptr + 4);
            break;
        }
    }
    if (ReadResult == HI_SUCCESS)
    {
        FirstFlag = 1;
        PatTbl.ProgNum = 0;
        curbufptr = bufptr;
        do
        {
            if (FirstFlag == 1)
            {
                FirstFlag = 0;
            }
            else
            {
                /*read setion again*/
                s32ret = PSI_DataRead(pat_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
                if (s32ret != HI_SUCCESS)
                {
                    s32ret = HI_SUCCESS;
                    break;
                }
                else
                {
                    curbufptr = bufptr;

                    /*get current section number*/
                    cur_section = *(curbufptr + 6);
                }
            }

            /*if section number is 0*/
            if (cur_section == 0)
            {
                last_section = *(curbufptr + 7);
                bytescopied = 0;
            }
            else
            {
                if (cur_section != prev_section + 1)
                {
                    continue;
                }
            }

            //record pre section number
            prev_section = cur_section;

            /*get section length*/
            section_len = (((hi_u16)((*(curbufptr + 1)) & 0x0F)) << 8) + *(curbufptr + 2);

            byteptr = PatTbl.pat_buf + bytescopied;

            if (bytescopied + section_len + 256 > PatSize)
            {
                break;
            }

            /*N stand for program number*/
            ptr_prog = curbufptr + 8;
            N = (section_len - 9) >> 2;
            for (i = 0; i < N; i++)
            {
                ProgID    = (((hi_u16)(*ptr_prog)) << 8) + *(ptr_prog + 1);
                ProgPID   = ((((hi_u16)(*(ptr_prog + 2))) << 8) + *(ptr_prog + 3)) & 0x1FFF;
                ptr_prog += 4;

                /*check if it NIT table,record to standard 300468£¬NIT_PID=0x10
                if(ProgID == 0)
                    NIT_PID = ProgPID;
                 */

                ((PROG_INFO *)byteptr)->ProgID  = ProgID;
                ((PROG_INFO *)byteptr)->ProgPID = ProgPID;
                byteptr += 4;
                bytescopied += 4;
            }
        } while (cur_section != last_section);               /*section end*/

        /*record program number*/
        PatTbl.ProgNum = bytescopied >> 2;
    }
    else
    {
        VTOP_MemFree(PatTbl.pat_buf);
        PatTbl.pat_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get PAT table, section read fail!\n");
    }

    PRINT_PSISI("ok\n");

    VTOP_MemFree(bufptr);

    *tbladdr = &PatTbl;

    s32ret = hi_unf_dmx_close_play_chan(pat_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_close_play_chan Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_detach_filter(pat_filter_id, pat_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_detach_filter Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_destroy_filter(pat_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_destroy_filter Error:0x%x.\n", s32ret);
    }

    s32ret = hi_unf_dmx_destroy_play_chan(pat_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_destroy_play_chan Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    return ReadResult;
}

hi_void HI_API_PSISI_FreePatTbl(hi_void)
{
    PatTbl.ProgNum = 0;
    VTOP_MemFree(PatTbl.pat_buf);
    PatTbl.pat_buf = HI_NULL_PTR;
}

hi_s32 HI_API_PSISI_GetCatTbl(hi_u32 u32DmxId, CAT_TBL **tbladdr, hi_u32 u32TimeOutms)
{
    hi_s32 s32ret, FirstFlag = 1;
    hi_u32 cur_section = 0, last_section = 0, prev_section = 0, bytescopied = 0, section_len = 0, N, i, CurNextInd;
    hi_u8  *bufptr, *curbufptr, *byteptr;
    hi_unf_dmx_chan_attr stChAttr;
    hi_unf_dmx_filter_attr stFilterAttr;

    if (tbladdr == HI_NULL_PTR)
    {
        return HI_FAILURE;
    }

    if (PSISI_InitFlag == 0)
    {
        PRINT_PSISI("PSISI Error: Fail to get CAT table, PSISI has not initialized!\n");
        return HI_FAILURE;
    }

    if (CatTbl.cat_buf != HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get CAT table, CAT Table has not free!\n");
        return HI_FAILURE;
    }

    CatTbl.cat_buf = VTOP_MemMalloc(CatSize);
    if (CatTbl.cat_buf == HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get CAT table, CAT Table alloc fail!\n");
        return HI_FAILURE;
    }

    bufptr = VTOP_MemMalloc(MAX_SECTION_LEN);
    if (bufptr == HI_NULL_PTR)
    {
        VTOP_MemFree(CatTbl.cat_buf);
        CatTbl.cat_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get CAT table, section memory alloc fail!\n");
        return HI_FAILURE;
    }

    //set filter paramter
    PSI_Filter_Reset();
    filter_match[0] = 0x01;
    filter_mask[0]   = 0x00;
    filter_negate[0] = 0x00;

    hi_unf_dmx_get_play_chan_default_attr(&stChAttr);

    /*create cat channel*/
    stChAttr.buffer_size = 16*1024;
    stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
    hi_unf_dmx_create_play_chan(u32DmxId, &stChAttr, &cat_ch);

    //HI_WARN_DEMUX("pat channel: %d\n", pat_ch);
    hi_unf_dmx_set_play_chan_pid(cat_ch, CAT_PID);

    stFilterAttr.filter_depth = 1;
    memcpy(stFilterAttr.match, filter_match, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.mask, filter_mask, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.negate, filter_negate, DMX_FILTER_MAX_DEPTH);
    s32ret = hi_unf_dmx_create_filter(u32DmxId, &stFilterAttr, &cat_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
    }

    hi_unf_dmx_attach_filter(cat_filter_id, cat_ch);
    hi_unf_dmx_open_play_chan(cat_ch);

    /*read first section*/
    s32ret = PSI_DataRead(cat_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
    while (s32ret == HI_SUCCESS)
    {
        /*get current section number*/
        cur_section = *(bufptr + 6);
        CurNextInd = (*(bufptr + 5)) & 0x01;

        //if not first section,drop it
        if ((cur_section != 0) || (CurNextInd == 0))
        {
            s32ret = PSI_DataRead(cat_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
        }
        else
        {
            last_section = *(bufptr + 7);
            break;
        }
    }

    if (s32ret == HI_SUCCESS)
    {
        FirstFlag = 1;
        curbufptr = bufptr;
        do
        {
            if (FirstFlag == 1)
            {
                FirstFlag = 0;
            }
            else
            {
                /*continue read section*/
                s32ret = PSI_DataRead(cat_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
                if (s32ret != HI_SUCCESS)
                {
                    s32ret = HI_SUCCESS;
                    break;
                }
                else
                {
                    curbufptr = bufptr;

                    /*get section number*/
                    cur_section = *(curbufptr + 6);
                }
            }

            if (cur_section == 0)
            {
                last_section = *(curbufptr + 7);
                bytescopied = 0;
            }
            else
            {
                if (cur_section != prev_section + 1)
                {
                    continue;
                }
            }

            //record last section number
            prev_section = cur_section;

            /*get section length*/
            section_len = (((hi_u16)((*(curbufptr + 1)) & 0x0F)) << 8) + *(curbufptr + 2);

            byteptr = CatTbl.cat_buf + bytescopied;

            if (bytescopied + section_len + 256 > CatSize)
            {
                break;
            }

            N = section_len - 9;
            for (i = 0; i < N; i++)
            {
                *byteptr++ = *(curbufptr + 8 + i);
            }

            bytescopied += N;
        } while (cur_section != last_section);

        /*record CAT table len*/
        CatTbl.len = bytescopied;
    }
    else
    {
        VTOP_MemFree(CatTbl.cat_buf);
        CatTbl.cat_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get CAT table, section read fail!\n");
    }

    VTOP_MemFree(bufptr);

    *tbladdr = &CatTbl;

    hi_unf_dmx_close_play_chan(cat_ch);
    hi_unf_dmx_detach_filter(cat_filter_id, cat_ch);
    hi_unf_dmx_destroy_filter(cat_filter_id);
    hi_unf_dmx_destroy_play_chan(cat_ch);
    return s32ret;
}

hi_void HI_API_PSISI_FreeCatTbl(hi_void)
{
    CatTbl.len = 0;
    VTOP_MemFree(CatTbl.cat_buf);
    CatTbl.cat_buf = HI_NULL_PTR;
}

/*record to standard ETS300468*/
hi_s32 HI_API_PSISI_GetNitTbl(hi_u32 u32DmxId, NIT_TBL **tbladdr, hi_u32 CurrFlag, hi_u32 u32TimeOutms)
{
    hi_s32 s32ret, FirstFlag = 1;
    hi_u32 len = 0, cur_section = 0, last_section = 0, prev_section = 0, bytescopied = 0, TSDescbytescopied = 0;
    hi_u32 i, j, CurNextInd;
    hi_u32 netdesc_len, streamdesc_len;
    hi_u8  *bufptr, *curbufptr, *byteptr = HI_NULL_PTR, *byteptr1;
    hi_unf_dmx_chan_attr stChAttr;
    hi_unf_dmx_filter_attr stFilterAttr;

    if ((tbladdr == HI_NULL_PTR) || (CurrFlag > 1))
    {
        return HI_FAILURE;
    }

    if (PSISI_InitFlag == 0)
    {
        PRINT_PSISI("PSISI Error: Fail to get NIT table, PSISI has not initialized!\n");
        return HI_FAILURE;
    }

    if ((NitTbl.NetDesc != HI_NULL_PTR) || (NitTbl.StreamDesc != HI_NULL_PTR))
    {
        PRINT_PSISI("PSISI Error: Fail to get NIT table, NIT Table has not free!\n");
        return HI_FAILURE;
    }



    NitTbl.NetDesc = VTOP_MemMalloc(NitNetSize);
    if (NitTbl.NetDesc == HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get NIT table, NIT table alloc fail!\n");
        return HI_FAILURE;
    }

    NitTbl.StreamDesc = VTOP_MemMalloc(NitStreamSize);
    if (NitTbl.StreamDesc == HI_NULL_PTR)
    {
        VTOP_MemFree(NitTbl.NetDesc);
        NitTbl.NetDesc = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get NIT table, NIT table alloc fail!\n");
        return HI_FAILURE;
    }

    bufptr = VTOP_MemMalloc(MAX_SECTION_LEN);
    if (bufptr == HI_NULL_PTR)
    {
        VTOP_MemFree(NitTbl.NetDesc);
        NitTbl.NetDesc = HI_NULL_PTR;
        VTOP_MemFree(NitTbl.StreamDesc);
        NitTbl.StreamDesc = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get NIT table, section alloc fail!\n");
        return HI_FAILURE;
    }

    //set filter paramter
    PSI_Filter_Reset();
    if (CurrFlag == 1)
    {
        //current network nit table
        filter_match[0] = 0x40;
    }
    else
    {
        //other network nit table
        filter_match[0] = 0x41;
    }

    filter_mask[0]   = 0x00;
    filter_negate[0] = 0x00;

    hi_unf_dmx_get_play_chan_default_attr(&stChAttr);

    stChAttr.buffer_size = 16*1024;
    stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
    hi_unf_dmx_create_play_chan(u32DmxId, &stChAttr, &nit_ch);

    //HI_WARN_DEMUX("pat channel: %d\n", pat_ch);
    hi_unf_dmx_set_play_chan_pid(nit_ch, NIT_PID);

    stFilterAttr.filter_depth = 1;
    memcpy(stFilterAttr.match, filter_match, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.mask, filter_mask, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.negate, filter_negate, DMX_FILTER_MAX_DEPTH);
    s32ret = hi_unf_dmx_create_filter(u32DmxId, &stFilterAttr, &nit_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
    }

    hi_unf_dmx_attach_filter(nit_filter_id, nit_ch);
    hi_unf_dmx_open_play_chan(nit_ch);

    /*read first section*/
    s32ret = PSI_DataRead(nit_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
    while (s32ret == HI_SUCCESS)
    {
        /*get curent section number*/
        cur_section = *(bufptr + 6);
        CurNextInd = (*(bufptr + 5)) & 0x01;

        //if the section is not the first one,drop it
        if ((cur_section != 0) || (CurNextInd == 0))
        {
            s32ret = PSI_DataRead(nit_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
        }
        else
        {
            last_section = *(bufptr + 7);
            NitTbl.NetworkID = (((hi_u16)(*(bufptr + 3))) << 8) + *(bufptr + 4);
            break;
        }
    }

    if (s32ret == HI_SUCCESS)
    {
        curbufptr = bufptr;
        FirstFlag = 1;
        do
        {
            if (FirstFlag == 1)
            {
                FirstFlag = 0;
            }
            else
            {
                /*continue read section*/
                s32ret = PSI_DataRead(nit_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
                if (s32ret != HI_SUCCESS)
                {
                    s32ret = HI_SUCCESS;
                    break;
                }
                else
                {
                    curbufptr = bufptr;

                    /*get current section number*/
                    cur_section = *(curbufptr + 6);
                }
            }

            if (cur_section == 0)
            {
                last_section = *(curbufptr + 7);
                bytescopied = 0;
                TSDescbytescopied = 0;
            }
            else
            {
                if (cur_section != prev_section + 1)
                {
                    continue;
                }
            }

            //record last section
            prev_section = cur_section;

            /*get section Len and NetDesc len*/
            netdesc_len = (((hi_u16)((*(curbufptr + 8)) & 0x0F)) << 8) + *(curbufptr + 9);
            streamdesc_len = (((hi_u16)((*(curbufptr + 10
                                           + netdesc_len)) & 0x0F)) << 8) + *(curbufptr + 11 + netdesc_len);

            byteptr = NitTbl.NetDesc + bytescopied;

            /*reach NIT table size,stop*/
            if (bytescopied + netdesc_len <= NitNetSize - 256)
            {
                for (i = 0; i < netdesc_len; i++)
                {
                    *byteptr++ = *(curbufptr + 10 + i);
                }

                bytescopied += netdesc_len;
            }

            byteptr = NitTbl.StreamDesc + TSDescbytescopied;

            /*reach NIT table size,stop*/
            if (TSDescbytescopied + sizeof(NIT_TSDESC) + 256 > NitStreamSize)
            {
                break;
            }

            curbufptr += 12 + netdesc_len;
            i = 0;
            while (1)
            {
                /*TS Stream Infor*/
                ((NIT_TSDESC *)byteptr)->TsID  = (((hi_u16)(*(curbufptr))) << 8) + *(curbufptr + 1);
                ((NIT_TSDESC *)byteptr)->NetID = (((hi_u16)(*(curbufptr + 2))) << 8) + *(curbufptr + 3);
                ((NIT_TSDESC *)byteptr)->TsDescLen = (((hi_u16)((*(curbufptr + 4)) & 0x0F)) << 8) + *(curbufptr + 5);
                len = ((NIT_TSDESC *)byteptr)->TsDescLen;
                ((NIT_TSDESC *)byteptr)->TsDesc = byteptr + sizeof(NIT_TSDESC);

                /*reach Section size,stop*/
                if (TSDescbytescopied + sizeof(NIT_TSDESC) + len + 256 > NitStreamSize)
                {
                    break;
                }

                /*TS steam infor*/
                curbufptr += 6;
                byteptr1 = ((NIT_TSDESC *)byteptr)->TsDesc;
                for (j = 0; j < len; j++)
                {
                    *byteptr1++ = *(curbufptr + j);
                }

                curbufptr += len;

                TSDescbytescopied += len + sizeof(NIT_TSDESC);

                /*jump to next stream infor*/
                if (((hi_u32)byteptr1 & 0x03) != 0)
                {
                    TSDescbytescopied += 4 - ((hi_u32)byteptr1 & 0x03);
                    byteptr1 += 4 - ((hi_u32)byteptr1 & 0x03);
                }

                ((NIT_TSDESC *)byteptr)->Next = (struct hiNIT_TSDESC_S *)byteptr1;

                i += len + 6;

                if (i >= streamdesc_len)
                {
                    break;
                }
                else
                {
                    byteptr = byteptr1;
                }
            }
        } while (cur_section != last_section);

        ((NIT_TSDESC *)byteptr)->Next = HI_NULL_PTR;

        /*flush length*/
        NitTbl.NetDescLen = bytescopied;
        NitTbl.StreamDescLen = TSDescbytescopied;
    }
    else
    {
        VTOP_MemFree(NitTbl.NetDesc);
        NitTbl.NetDesc = HI_NULL_PTR;
        VTOP_MemFree(NitTbl.StreamDesc);
        NitTbl.StreamDesc = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get NIT table, section read fail!\n");
    }

    VTOP_MemFree(bufptr);

    *tbladdr = &NitTbl;

    hi_unf_dmx_close_play_chan(nit_ch);
    hi_unf_dmx_detach_filter(nit_filter_id, nit_ch);
    hi_unf_dmx_destroy_filter(nit_filter_id);
    hi_unf_dmx_destroy_play_chan(nit_ch);

    return s32ret;
}

hi_void HI_API_PSISI_FreeNitTbl(hi_void)
{
    NitTbl.NetDescLen = 0;
    NitTbl.StreamDescLen = 0;

    if (NitTbl.NetDesc != HI_NULL_PTR)
    {
        VTOP_MemFree(NitTbl.NetDesc);
    }

    if (NitTbl.StreamDesc != HI_NULL_PTR)
    {
        VTOP_MemFree(NitTbl.StreamDesc);
    }

    NitTbl.NetDesc = HI_NULL_PTR;
    NitTbl.StreamDesc = HI_NULL_PTR;
}

hi_s32 HI_API_PSISI_GetPmtTbl(hi_u32 u32DmxId, PMT_TBL **tbladdr, hi_u32 ProgID, hi_u32 u32TimeOutms)
{
    hi_s32 s32ret, PatKeep = 0;
    hi_u32 len = 0, section_len = 0, i, CurNextInd;
    hi_u32 proginfolen, esinfolen, esbytecopied = 0, tblbytecopied = 0;
    hi_u32 pcr_pid;
    hi_u8 *bufptr, *curbufptr, *esptr;
    hi_u8 *byteptr0, *byteptr1, *byteptr2, *byteptr3;
    hi_u8 *progptr;
    PAT_TBL *pat_tbl;
    hi_s32 ReadResult;
    hi_unf_dmx_chan_attr stChAttr;
    hi_unf_dmx_filter_attr stFilterAttr;

    if (tbladdr == HI_NULL_PTR)
    {
        return HI_FAILURE;
    }

    if (PSISI_InitFlag == 0)
    {
        PRINT_PSISI("PSISI Error: Fail to get PMT table, PSISI has not initialized!\n");
        return HI_FAILURE;
    }

    if (PmtTbl.pmt_buf != HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get PMT table, PMT Table has not free!\n");
        return HI_FAILURE;
    }

    /*PAT table invalid*/
    if (PatTbl.pat_buf == HI_NULL_PTR)
    {
        PatKeep = 0;

        /*read pat first*/
        s32ret = HI_API_PSISI_GetPatTbl(u32DmxId, &pat_tbl, 3000);

        if (s32ret != HI_SUCCESS)
        {
            PRINT_PSISI("PSISI Error: Fail to get PMT table, get PAT table fail!\n");
            return s32ret;
        }
    }
    /*need to record pat*/
    else
    {
        PatKeep = 1;
    }

    /*Find program PID*/
    PMT_PID = INVALID_PID;
    progptr = PatTbl.pat_buf;
    for (i = 0; i < PatTbl.ProgNum; i++)
    {
        len = ((PROG_INFO*)progptr)->ProgID;
        if (ProgID == len)
        {
            PMT_PID = ((PROG_INFO*)progptr)->ProgPID;

            //PRINT_PSISI("PMT_PID = %x\n", PMT_PID);
            break;
        }
        else
        {
            progptr += 4;
        }
    }

    /*release pat table*/
    if (PatKeep == 0)
    {
        HI_API_PSISI_FreePatTbl();
    }

    if (PMT_PID != INVALID_PID)
    {
        //set filter paramter
        PSI_Filter_Reset();
        filter_match[0] = 0x02;
        filter_mask[0]   = 0x00;
        filter_negate[0] = 0x00;

        hi_unf_dmx_get_play_chan_default_attr(&stChAttr);

        stChAttr.buffer_size = 16*1024;
        stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
        stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
        s32ret = hi_unf_dmx_create_play_chan(u32DmxId, &stChAttr, &pmt_ch);
        if (s32ret != HI_SUCCESS)
        {
            PRINT_PSISI("hi_unf_dmx_create_play_chan Error:0x%x.\n", s32ret);
            return HI_FAILURE;
        }

        //HI_WARN_DEMUX("pat channel: %d\n", pat_ch);
        s32ret = hi_unf_dmx_set_play_chan_pid(pmt_ch, PMT_PID);
        if (s32ret != HI_SUCCESS)
        {
            PRINT_PSISI("hi_unf_dmx_set_play_chan_pid Error:0x%x.\n", s32ret);
            return HI_FAILURE;
        }

        stFilterAttr.filter_depth = 0;
        memcpy(stFilterAttr.match, filter_match, DMX_FILTER_MAX_DEPTH);
        memcpy(stFilterAttr.mask, filter_mask, DMX_FILTER_MAX_DEPTH);
        memcpy(stFilterAttr.negate, filter_negate, DMX_FILTER_MAX_DEPTH);
        s32ret = hi_unf_dmx_create_filter(u32DmxId, &stFilterAttr, &pmt_filter_id);
        if (s32ret != HI_SUCCESS)
        {
            PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
            return HI_FAILURE;
        }

        s32ret = hi_unf_dmx_attach_filter(pmt_filter_id, pmt_ch);
        if (s32ret != HI_SUCCESS)
        {
            PRINT_PSISI("hi_unf_dmx_attach_filter Error:0x%x.\n", s32ret);
            return HI_FAILURE;
        }

        s32ret = hi_unf_dmx_open_play_chan(pmt_ch);
        if (s32ret != HI_SUCCESS)
        {
            PRINT_PSISI("hi_unf_dmx_open_play_chan Error:0x%x.\n", s32ret);
            return HI_FAILURE;
        }
    }
    else
    {
        PRINT_PSISI("PSISI Error: Fail to get PMT table, PMT's pid is invalid!\n");
        return HI_FAILURE;
    }

    PmtTbl.pmt_buf = VTOP_MemMalloc(PmtSize);
    if (PmtTbl.pmt_buf == HI_NULL_PTR)
    {
        hi_unf_dmx_close_play_chan(pmt_ch);
        hi_unf_dmx_detach_filter(pmt_filter_id, pmt_ch);
        hi_unf_dmx_destroy_filter(pmt_filter_id);
        hi_unf_dmx_destroy_play_chan(pmt_ch);
        PRINT_PSISI("PSISI Error: Fail to get PMT table, PMT table alloc fail!\n");
        return HI_FAILURE;
    }

    bufptr = VTOP_MemMalloc(MAX_SECTION_LEN);
    if (bufptr == HI_NULL_PTR)
    {
        VTOP_MemFree(PmtTbl.pmt_buf);
        PmtTbl.pmt_buf = HI_NULL_PTR;
        hi_unf_dmx_close_play_chan(pmt_ch);
        hi_unf_dmx_detach_filter(pmt_filter_id, pmt_ch);
        hi_unf_dmx_destroy_filter(pmt_filter_id);
        hi_unf_dmx_destroy_play_chan(pmt_ch);
        PRINT_PSISI("PSISI Error: Fail to get PMT table, section alloc fail!\n");
        return HI_FAILURE;
    }

    /*read PMT table*/
    ReadResult = PSI_DataRead(pmt_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
    while (ReadResult == HI_SUCCESS)
    {
        CurNextInd = (*(bufptr + 5)) & 0x01;

        //if current not valid,read next
        if (CurNextInd == 0)
        {
            ReadResult = PSI_DataRead(pmt_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
        }
        else
        {
            break;
        }
    }

    if (ReadResult == HI_SUCCESS)
    {
        byteptr0 = PmtTbl.pmt_buf;
        ((PMT_Prog *)(PmtTbl.pmt_buf))->ProgDescPtr = PmtTbl.pmt_buf + sizeof(PMT_Prog);
        byteptr1  = ((PMT_Prog *)(PmtTbl.pmt_buf))->ProgDescPtr;
        curbufptr = bufptr;

        /*get section length*/
        section_len = (((hi_u16)((*(curbufptr + 1)) & 0x0F)) << 8) + *(curbufptr + 2);
        pcr_pid = (((hi_u16)((*(curbufptr + 8)) & 0x1F)) << 8) + *(curbufptr + 9);
        proginfolen = (((hi_u16)((*(curbufptr + 10)) & 0x0F)) << 8) + *(curbufptr + 11);
        esinfolen = (((hi_u16)((*(curbufptr + 15 + proginfolen)) & 0x0F)) << 8) + *(curbufptr + 16 + proginfolen);

        if (proginfolen + sizeof(PMT_Prog) <= PmtSize - 256)
        {
            /*get program desc data*/
            ((PMT_Prog *)byteptr0)->ProgID = (((hi_u16)((*(curbufptr + 3)))) << 8) + *(curbufptr + 4);

            for (i = 0; i < proginfolen; i++)
            {
                *byteptr1++ = *(curbufptr + 12 + i);
            }

            tblbytecopied += proginfolen + sizeof(PMT_Prog);
            ((PMT_Prog *)byteptr0)->ProgDescLen = proginfolen;
            ((PMT_Prog *)byteptr0)->PCR_PID = pcr_pid;

            if (((hi_u32)byteptr1 & 0x03) != 0)
            {
                tblbytecopied += 4 - ((hi_u32)byteptr1 & 0x03);
                byteptr1 += 4 - ((hi_u32)byteptr1 & 0x03);
            }

            if (tblbytecopied + esinfolen + sizeof(PMT_Stream) <= PmtSize - 256)
            {
                ((PMT_Prog *)byteptr0)->ESDescPtr = byteptr1;

                byteptr2 = ((PMT_Prog *)byteptr0)->ESDescPtr;
                esbytecopied = 0;
                esptr = curbufptr + 12 + proginfolen;

                while (1)
                {
                    /*get element infor*/
                    ((PMT_Stream *)byteptr2)->StreamType = *(esptr);
                    ((PMT_Stream *)byteptr2)->ElementPid = (((hi_u16)((*(esptr + 1)) & 0x1f)) << 8) +
                                                           *(esptr + 2);
                    ((PMT_Stream *)byteptr2)->ESInfoLen = esinfolen;
                    ((PMT_Stream *)byteptr2)->ESDesc = byteptr2 + sizeof(PMT_Stream);

                    byteptr3 = ((PMT_Stream *)byteptr2)->ESDesc;
                    for (i = 0; i < esinfolen; i++)
                    {
                        *byteptr3++ = *(esptr + 5 + i);
                    }

                    esptr += esinfolen + 5;
                    esbytecopied  += esinfolen + 5;
                    tblbytecopied += esinfolen + sizeof(PMT_Stream);

                    /*jump to next element infor*/
                    if (((hi_u32)byteptr3 & 0x03) != 0)
                    {
                        tblbytecopied += 4 - ((hi_u32)byteptr3 & 0x03);
                        byteptr3 += 4 - ((hi_u32)byteptr3 & 0x03);
                    }

                    if (proginfolen + esbytecopied + 13 <= section_len - 5)
                    {
                         esinfolen = (((hi_u16)((*(esptr + 3)) & 0x0F)) << 8) + *(esptr + 4);

                         if (tblbytecopied + esinfolen + sizeof(PMT_Stream) + 256 > PmtSize)
                        {
                            ((PMT_Stream *)byteptr2)->Next = HI_NULL_PTR;
                            break;
                        }
                        else
                        {
                            ((PMT_Stream *)byteptr2)->Next = (PMT_Stream *)byteptr3;
                            byteptr2 = byteptr3;
                        }
                    }
                     else
                    {
                        ((PMT_Stream *)byteptr2)->Next = HI_NULL_PTR;
                        break;
                    }
                }
            }
            else
            {
                ((PMT_Prog *)byteptr0)->ESDescPtr = HI_NULL_PTR;
            }
        }
        else
        {
            ((PMT_Prog *)byteptr0)->ESDescPtr   = HI_NULL_PTR;
            ((PMT_Prog *)byteptr0)->ProgDescPtr = HI_NULL_PTR;
        }
    }
    else
    {
        VTOP_MemFree(PmtTbl.pmt_buf);
        PmtTbl.pmt_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get PMT table, section read fail!\n");
    }

    VTOP_MemFree(bufptr);

    *tbladdr = &PmtTbl;

    s32ret = hi_unf_dmx_close_play_chan(pmt_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_close_play_chan Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_detach_filter(pmt_filter_id, pmt_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_detach_filter Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_destroy_filter(pmt_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_destroy_filter Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    s32ret = hi_unf_dmx_destroy_play_chan(pmt_ch);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_destroy_play_chan Error:0x%x.\n", s32ret);
        return HI_FAILURE;
    }

    return ReadResult;
}

hi_void HI_API_PSISI_FreePmtTbl(hi_void)
{
    VTOP_MemFree(PmtTbl.pmt_buf);
    PmtTbl.pmt_buf = HI_NULL_PTR;
}

hi_s32 HI_API_PSISI_GetSdtTbl(hi_u32 u32DmxId, SDT_TBL **tbladdr, hi_u32 CurrFlag, hi_u32 TsID, hi_u32 NetID, hi_u32 u32TimeOutms)
{
    hi_s32 s32ret, FirstFlag = 1;
    hi_u32 cur_section = 0, last_section = 0, prev_section = 0, bytescopied = 0, section_len = 0, N, i, j, CurNextInd;
    hi_u32 SerDescLen = 0, PrvdNameLen, SerNameLen, DescLenCopied;
    hi_u8  *bufptr, *curbufptr;
    hi_u8  *byteptr = HI_NULL_PTR, *ptr_ser, *byteptr1, *TmpDesc, *TmpDesc1;
    hi_unf_dmx_chan_attr stChAttr;
    hi_unf_dmx_filter_attr stFilterAttr;

    if ((tbladdr == HI_NULL_PTR) || (CurrFlag > 1))
    {
        return HI_FAILURE;
    }

    if (PSISI_InitFlag == 0)
    {
        PRINT_PSISI("PSISI Error: Fail to get SDT table, PSISI has not initialized!\n");
        return HI_FAILURE;
    }

    if (SdtTbl.ser_buf != HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get SDT table, SDT Table has not free!\n");
        return HI_FAILURE;
    }

     SdtTbl.ser_buf = VTOP_MemMalloc(SdtSize);
    if (SdtTbl.ser_buf == HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get SDT table, SDT Table alloc fail!\n");
        return HI_FAILURE;
    }

     bufptr = VTOP_MemMalloc(MAX_SECTION_LEN);
    if (bufptr == HI_NULL_PTR)
    {
        VTOP_MemFree(SdtTbl.ser_buf);
        SdtTbl.ser_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get SDT table, section alloc fail!\n");
        return HI_FAILURE;
    }

    //set filter paramter
    PSI_Filter_Reset();
    if (CurrFlag == 1)
    {
        //get table from current ts
        filter_match[0] = 0x42;
        filter_mask[0]   = 0x00;
        filter_negate[0] = 0x00;
    }
    else
    {
        //get table from other ts
        filter_match[0] = 0x46;
        filter_mask[0]   = 0x00;
        filter_negate[0] = 0x00;

        filter_match[1] = TsID >> 8;
        filter_mask[1]   = 0x00;
        filter_negate[1] = 0x00;

        filter_match[2] = TsID & 0x000000ff;
        filter_mask[2]   = 0x00;
        filter_negate[2] = 0x00;

        filter_match[6] = NetID >> 8;
        filter_mask[6]   = 0x00;
        filter_negate[6] = 0x00;

        filter_match[7] = NetID & 0x000000ff;
        filter_mask[7]   = 0x00;
        filter_negate[7] = 0x00;
    }

    hi_unf_dmx_get_play_chan_default_attr(&stChAttr);

    stChAttr.buffer_size = 16*1024;
    stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
    hi_unf_dmx_create_play_chan(u32DmxId, &stChAttr, &sdt_ch);

    //HI_WARN_DEMUX("pat channel: %d\n", pat_ch);
    hi_unf_dmx_set_play_chan_pid(sdt_ch, sdt_filter_id);

    if (CurrFlag == 1)
    {
        stFilterAttr.filter_depth = 1;
    }
    else
    {
        stFilterAttr.filter_depth = 8;
    }

    memcpy(stFilterAttr.match, filter_match, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.mask, filter_mask, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.negate, filter_negate, DMX_FILTER_MAX_DEPTH);
    s32ret = hi_unf_dmx_create_filter(u32DmxId, &stFilterAttr, &sdt_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
    }

    hi_unf_dmx_attach_filter(sdt_filter_id, sdt_ch);
    hi_unf_dmx_open_play_chan(sdt_ch);

     s32ret = PSI_DataRead(sdt_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
    while (s32ret == HI_SUCCESS)
    {
         cur_section = *(bufptr + 6);
        CurNextInd = (*(bufptr + 5)) & 0x01;

        //HI_WARN_DEMUX("Secnum = %d, ind = %d\n", cur_section, CurNextInd);

         if ((cur_section != 0) || (CurNextInd == 0))
        {
            s32ret = PSI_DataRead(sdt_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
        }
        else
        {
            last_section = *(bufptr + 7);
            SdtTbl.TsID  = (((hi_u16)(*(bufptr + 3))) << 8) + *(bufptr + 4);
            SdtTbl.NetID = (((hi_u16)(*(bufptr + 8))) << 8) + *(bufptr + 9);
            break;
        }
    }

    if (s32ret == HI_SUCCESS)
    {
        FirstFlag = 1;
        curbufptr = bufptr;
        do
        {
            if (FirstFlag == 1)
            {
                FirstFlag = 0;
            }
            else
            {
                s32ret = PSI_DataRead(sdt_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
                if (s32ret != HI_SUCCESS)
                {
                    s32ret = HI_SUCCESS;
                    break;
                }
                else
                {
                    curbufptr = bufptr;

                     cur_section = *(curbufptr + 6);
                }
            }

             if (cur_section == 0)
            {
                last_section = *(curbufptr + 7);
                bytescopied = 0;
            }
            else
            {
                if (cur_section != prev_section + 1)
                {
                    continue;
                }
            }

             prev_section = cur_section;

             section_len = (((hi_u16)((*(curbufptr + 1)) & 0x0F)) << 8) + *(curbufptr + 2);

             byteptr = SdtTbl.ser_buf + bytescopied;

             if (bytescopied + section_len + 256 > SdtSize)
            {
                break;
            }

             ptr_ser = curbufptr + 11;
            N = section_len - 12;
            i = 0;
            while (1)
            {
                DescLenCopied = 0;
                ((SER_INFO *)byteptr)->SerID = (((hi_u16)(*ptr_ser)) << 8) + *(ptr_ser + 1);
                ((SER_INFO *)byteptr)->SchdFlag = (*(ptr_ser + 2) & 0x02) >> 1;
                ((SER_INFO *)byteptr)->PrsntFlwFlag = *(ptr_ser + 2) & 0x01;
                ((SER_INFO *)byteptr)->RunState = (*(ptr_ser + 3) & 0xe0) >> 5;
                ((SER_INFO *)byteptr)->CAMode = (*(ptr_ser + 3) & 0x10) >> 4;
                SerDescLen = (((hi_u16)(*(ptr_ser + 3) & 0x0F)) << 8) + *(ptr_ser + 4);
                ptr_ser += 5;
                bytescopied += sizeof(SER_INFO);
                ((SER_INFO *)byteptr)->DescPtr = (DESC_STRU_S *)(byteptr + sizeof(SER_INFO));
                TmpDesc = (hi_u8 *)((SER_INFO *)byteptr)->DescPtr;

                //get all desc
                while (1)
                {
                    //get desc tag and len
                    ((DESC_STRU_S *)TmpDesc)->DescTag = *ptr_ser;
                    ((DESC_STRU_S *)TmpDesc)->DescLen = *(ptr_ser + 1);

                    if (((DESC_STRU_S *)TmpDesc)->DescTag == Ser_Desc)
                    {
                        ((DESC_STRU_S *)TmpDesc)->Desc = TmpDesc + sizeof(DESC_STRU_S);
                        TmpDesc1 = ((DESC_STRU_S *)TmpDesc)->Desc;
                        ((SER_DESC_S *)TmpDesc1)->SerType = *(ptr_ser + 2);
                        ((SER_DESC_S *)TmpDesc1)->PrvdNameLen = *(ptr_ser + 3);
                        PrvdNameLen = ((SER_DESC_S *)TmpDesc1)->PrvdNameLen;
                        ((SER_DESC_S *)TmpDesc1)->PrvdName = TmpDesc1 + sizeof(SER_DESC_S);
                        byteptr1 = ((SER_DESC_S *)TmpDesc1)->PrvdName;

                        //copy provider name
                        for (j = 0; j < PrvdNameLen; j++)
                        {
                            *byteptr1++ = *(ptr_ser + 4 + j);
                        }

                        ptr_ser += PrvdNameLen + 4;
                        bytescopied += PrvdNameLen + sizeof(DESC_STRU_S) + sizeof(SER_DESC_S);

                        if (((hi_u32)byteptr1 & 0x03) != 0)
                        {
                            bytescopied += 4 - ((hi_u32)byteptr1 & 0x03);
                            byteptr1 += 4 - ((hi_u32)byteptr1 & 0x03);
                        }

                        //copy sevice name
                        ((SER_DESC_S *)TmpDesc1)->SerName = byteptr1;
                        ((SER_DESC_S *)TmpDesc1)->SerNameLen = *ptr_ser;
                        SerNameLen = ((SER_DESC_S *)TmpDesc1)->SerNameLen;

                        for (j = 0; j < SerNameLen; j++)
                        {
                            *byteptr1++ = *(ptr_ser + 1 + j);
                        }

                        ptr_ser += SerNameLen + 1;
                        bytescopied += SerNameLen;

                        if (((hi_u32)byteptr1 & 0x03) != 0)
                        {
                            bytescopied += 4 - ((hi_u32)byteptr1 & 0x03);
                            byteptr1 += 4 - ((hi_u32)byteptr1 & 0x03);
                        }
                    }
                    else
                    {
                        ((DESC_STRU_S *)TmpDesc)->Desc = HI_NULL_PTR;
                        byteptr1 = TmpDesc + sizeof(DESC_STRU_S);
                        bytescopied += sizeof(DESC_STRU_S);
                        ptr_ser += ((DESC_STRU_S *)TmpDesc)->DescLen + 2;
                    }

                    DescLenCopied += ((DESC_STRU_S *)TmpDesc)->DescLen + 2;

                    ((DESC_STRU_S *)TmpDesc)->Next = (DESC_STRU_S *)byteptr1;

                    if (DescLenCopied >= SerDescLen)
                    {
                        ((DESC_STRU_S *)TmpDesc)->Next = HI_NULL_PTR;
                        break;
                    }
                    else
                    {
                        TmpDesc = (hi_u8 *)((DESC_STRU_S *)TmpDesc)->Next;
                    }
                }

                i += 5 + SerDescLen;

                ((SER_INFO *)byteptr)->Next = (SER_INFO *)byteptr1;

                if (i >= N)
                {
                    //current section is completed
                    break;
                }
                else
                {
                    byteptr = byteptr1;
                }
            }
        } while (cur_section != last_section);

        ((SER_INFO *)byteptr)->Next = HI_NULL_PTR;
    }
    else
    {
        VTOP_MemFree(SdtTbl.ser_buf);
        SdtTbl.ser_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get SDT table, section read fail!\n");
    }

    VTOP_MemFree(bufptr);

    *tbladdr = &SdtTbl;

    hi_unf_dmx_close_play_chan(sdt_ch);
    hi_unf_dmx_detach_filter(sdt_filter_id, sdt_ch);
    hi_unf_dmx_destroy_filter(sdt_filter_id);
    hi_unf_dmx_destroy_play_chan(sdt_ch);

    return s32ret;
}

hi_void HI_API_PSISI_FreeSdtTbl(hi_void)
{
    VTOP_MemFree(SdtTbl.ser_buf);
    SdtTbl.ser_buf = HI_NULL_PTR;
}

hi_s32 HI_API_PSISI_GetEitTbl(hi_u32 u32DmxId, EIT_TBL **tbladdr, hi_u32 CurrFlag, hi_u32 SerID, hi_u32 TsID, hi_u32 NetID,
                              hi_u32 u32TimeOutms)
{
    hi_s32 s32ret, FirstFlag = 1;
    hi_u32 cur_section = 0, last_section = 0, prev_section = 0, bytescopied = 0, section_len = 0, N, i, j, CurNextInd;
    hi_u32 EvtDescLen = 0, DescLenCopied, EvtNameLen, TextLen, Duration;
    hi_u8  *bufptr, *curbufptr;
    hi_u8  *byteptr = HI_NULL_PTR, *ptr_evt, *byteptr1, *TmpDesc, *TmpDesc1;
    hi_u64 StartTime;
    hi_unf_dmx_chan_attr stChAttr;
    hi_unf_dmx_filter_attr stFilterAttr;

    if ((tbladdr == HI_NULL_PTR) || (CurrFlag > 1))
    {
        return HI_FAILURE;
    }

    if (PSISI_InitFlag == 0)
    {
        PRINT_PSISI("PSISI Error: Fail to get EIT table, PSISI has not initialized!\n");
        return HI_FAILURE;
    }

    if (EitTbl.evt_buf != HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get EIT table, EIT Table has not free!\n");
        return HI_FAILURE;
    }

    EitTbl.evt_buf = VTOP_MemMalloc(EitSize);
    if (EitTbl.evt_buf == HI_NULL_PTR)
    {
        PRINT_PSISI("PSISI Error: Fail to get EIT table, EIT Table alloc fail!\n");
        return HI_FAILURE;
    }

    bufptr = VTOP_MemMalloc(MAX_SECTION_LEN);
    if (bufptr == HI_NULL_PTR)
    {
        VTOP_MemFree(EitTbl.evt_buf);
        EitTbl.evt_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get EIT table, section alloc fail!\n");
        return HI_FAILURE;
    }

    //set filter paramter
    PSI_Filter_Reset();
    if (CurrFlag == 1)
    {
        //get table from current ts
        filter_match[0] = 0x4e;
        filter_mask[0]   = 0x00;
        filter_negate[0] = 0x00;
    }
    else
    {
        //get table from other ts
        filter_match[0] = 0x4f;
        filter_mask[0]   = 0x00;
        filter_negate[0] = 0x00;

        filter_match[6] = TsID >> 8;
        filter_mask[6]   = 0x00;
        filter_negate[6] = 0x00;

        filter_match[7] = TsID & 0x000000ff;
        filter_mask[7]   = 0x00;
        filter_negate[7] = 0x00;

        filter_match[8] = NetID >> 8;
        filter_mask[8]   = 0x00;
        filter_negate[8] = 0x00;

        filter_match[9] = NetID & 0x000000ff;
        filter_mask[9]   = 0x00;
        filter_negate[9] = 0x00;
    }

    filter_match[1] = SerID >> 8;
    filter_mask[1]   = 0x00;
    filter_negate[1] = 0x00;

    filter_match[2] = SerID & 0x000000ff;
    filter_mask[2]   = 0x00;
    filter_negate[2] = 0x00;

    hi_unf_dmx_get_play_chan_default_attr(&stChAttr);

    stChAttr.buffer_size = 16*1024;
    stChAttr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    stChAttr.crc_mode = HI_UNF_DMX_CHAN_CRC_MODE_FORBID;
    hi_unf_dmx_create_play_chan(u32DmxId, &stChAttr, &eit_ch);

    //HI_WARN_DEMUX("pat channel: %d\n", pat_ch);
    hi_unf_dmx_set_play_chan_pid(eit_ch, EIT_PID);

    if (CurrFlag == 1)
    {
        stFilterAttr.filter_depth = 3;
    }
    else
    {
        stFilterAttr.filter_depth = 10;
    }

    memcpy(stFilterAttr.match, filter_match, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.mask, filter_mask, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.negate, filter_negate, DMX_FILTER_MAX_DEPTH);
    s32ret = hi_unf_dmx_create_filter(u32DmxId, &stFilterAttr, &eit_filter_id);
    if (s32ret != HI_SUCCESS)
    {
        PRINT_PSISI("hi_unf_dmx_create_filter Error:0x%x.\n", s32ret);
    }

    hi_unf_dmx_attach_filter(eit_filter_id, eit_ch);
    hi_unf_dmx_open_play_chan(eit_ch);

     s32ret = PSI_DataRead(eit_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
    while (s32ret == HI_SUCCESS)
    {
         cur_section = *(bufptr + 6);
        CurNextInd = (*(bufptr + 5)) & 0x01;

        //HI_WARN_DEMUX("Secnum = %d, ind = %d\n", cur_section, CurNextInd);

        if ((cur_section != 0) || (CurNextInd == 0))
        {
            s32ret = PSI_DataRead(eit_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
        }
        else
        {
            last_section = *(bufptr + 7);
            EitTbl.TsID  = (((hi_u16)(*(bufptr + 8))) << 8) + *(bufptr + 9);
            EitTbl.NetID = (((hi_u16)(*(bufptr + 10))) << 8) + *(bufptr + 11);
            break;
        }
    }

    if (s32ret == HI_SUCCESS)
    {
        FirstFlag = 1;
        curbufptr = bufptr;
        do
        {
            if (FirstFlag == 1)
            {
                FirstFlag = 0;
            }
            else
            {
                s32ret = PSI_DataRead(eit_ch, u32TimeOutms, bufptr, MAX_SECTION_LEN, &CpLen);
                if (s32ret != HI_SUCCESS)
                {
                    s32ret = HI_SUCCESS;
                    break;
                }
                else
                {
                    curbufptr = bufptr;

                    cur_section = *(curbufptr + 6);
                }
            }

            if (cur_section == 0)
            {
                last_section = *(curbufptr + 7);
                bytescopied = 0;
            }
            else
            {
                if (cur_section != prev_section + 1)
                {
                    continue;
                }
            }

             prev_section = cur_section;

             section_len = (((hi_u16)((*(curbufptr + 1)) & 0x0F)) << 8) + *(curbufptr + 2);

             byteptr = EitTbl.evt_buf + bytescopied;

             if (bytescopied + section_len + 256 > EitSize)
            {
                break;
            }

             ptr_evt = curbufptr + 14;
            N = section_len - 15;
            i = 0;
            while (1)
            {
                DescLenCopied = 0;
                ((EVT_INFO *)byteptr)->EvtID = (((hi_u16)(*ptr_evt)) << 8) + *(ptr_evt + 1);

                //get start time
                StartTime = (((hi_u64)(*(ptr_evt + 2))) << 32) + (((hi_u64)(*(ptr_evt + 3))) << 24) +
                            (((hi_u64)(*(ptr_evt + 4))) << 16) + (((hi_u64)(*(ptr_evt + 5))) << 8) +
                            *(ptr_evt + 6);
                ((EVT_INFO *)byteptr)->StartTime[0] = ((StartTime
                                                        & 0x0000000000F00000LL)
                                                       >> 20) * 10 + ((StartTime & 0x00000000000F0000LL) >> 16);
                ((EVT_INFO *)byteptr)->StartTime[1] = ((StartTime
                                                        & 0x000000000000F000LL)
                                                       >> 12) * 10 + ((StartTime & 0x0000000000000F00LL) >> 8);
                ((EVT_INFO *)byteptr)->StartTime[2] = ((StartTime
                                                        & 0x00000000000000F0LL)
                                                       >> 4) * 10 + (StartTime & 0x000000000000000FLL);
                ((EVT_INFO *)byteptr)->StartTime[3] = 0;

                //get duration time
                Duration = (((hi_u32)(*(ptr_evt + 7))) << 16) + (((hi_u32)(*(ptr_evt + 8))) << 8) +
                           *(ptr_evt + 9);
                ((EVT_INFO *)byteptr)->Duration[0] = ((Duration
                                                       & 0x00F00000) >> 20) * 10 + ((Duration & 0x000F0000) >> 16);
                ((EVT_INFO *)byteptr)->Duration[1] = ((Duration
                                                       & 0x0000F000) >> 12) * 10 + ((Duration & 0x00000F00) >> 8);
                ((EVT_INFO *)byteptr)->Duration[2] = ((Duration & 0x000000F0) >> 4) * 10 + (Duration & 0x0000000F);
                ((EVT_INFO *)byteptr)->Duration[3] = 0;

                ((EVT_INFO *)byteptr)->RunState = (*(ptr_evt + 10) & 0xe0) >> 5;
                ((EVT_INFO *)byteptr)->CAMode = (*(ptr_evt + 10) & 0x10) >> 4;
                EvtDescLen = (((hi_u16)(*(ptr_evt + 10) & 0x0F)) << 8) + *(ptr_evt + 11);

                ptr_evt += 12;
                bytescopied += sizeof(EVT_INFO);
                ((EVT_INFO *)byteptr)->DescPtr = (DESC_STRU_S *)(byteptr + sizeof(EVT_INFO));
                TmpDesc = (hi_u8 *)((EVT_INFO *)byteptr)->DescPtr;

                //get all desc
                while (1)
                {
                    //get desc tag and len
                    ((DESC_STRU_S *)TmpDesc)->DescTag = *(ptr_evt);
                    ((DESC_STRU_S *)TmpDesc)->DescLen = *(ptr_evt + 1);

                    if (((DESC_STRU_S *)TmpDesc)->DescTag == Short_evt_Desc)
                    {
                        ((DESC_STRU_S *)TmpDesc)->Desc = TmpDesc + sizeof(DESC_STRU_S);
                        TmpDesc1 = ((DESC_STRU_S *)TmpDesc)->Desc;

                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->Lang[0] = *(ptr_evt + 2);
                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->Lang[1] = *(ptr_evt + 3);
                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->Lang[2] = *(ptr_evt + 4);
                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->Lang[3] = 0;
                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->EvtNameLen = *(ptr_evt + 5);
                        EvtNameLen = ((SHOTR_ETV_DESC_S *)TmpDesc1)->EvtNameLen;
                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->EvtName = TmpDesc1 + sizeof(SHOTR_ETV_DESC_S);
                        byteptr1 = ((SHOTR_ETV_DESC_S *)TmpDesc1)->EvtName;

                        //copy event name
                        for (j = 0; j < EvtNameLen; j++)
                        {
                            *byteptr1++ = *(ptr_evt + 6 + j);
                        }

                        ptr_evt += EvtNameLen + 6;
                        bytescopied += EvtNameLen + sizeof(DESC_STRU_S) + sizeof(SHOTR_ETV_DESC_S);

                        if (((hi_u32)byteptr1 & 0x03) != 0)
                        {
                            bytescopied += 4 - ((hi_u32)byteptr1 & 0x03);
                            byteptr1 += 4 - ((hi_u32)byteptr1 & 0x03);
                        }

                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->TextLen = *ptr_evt;
                        TextLen = ((SHOTR_ETV_DESC_S *)TmpDesc1)->TextLen;
                        ((SHOTR_ETV_DESC_S *)TmpDesc1)->Text = byteptr1;

                        //copy text
                        for (j = 0; j < TextLen; j++)
                        {
                            *byteptr1++ = *(ptr_evt + 1 + j);
                        }

                        ptr_evt += TextLen + 1;
                        bytescopied += TextLen;

                        if (((hi_u32)byteptr1 & 0x03) != 0)
                        {
                            bytescopied += 4 - ((hi_u32)byteptr1 & 0x03);
                            byteptr1 += 4 - ((hi_u32)byteptr1 & 0x03);
                        }
                    }
                    else
                    {
                        ((DESC_STRU_S *)TmpDesc)->Desc = HI_NULL_PTR;
                        byteptr1 = TmpDesc + sizeof(DESC_STRU_S);
                        bytescopied += sizeof(DESC_STRU_S);
                        ptr_evt += ((DESC_STRU_S *)TmpDesc)->DescLen + 2;
                    }

                    DescLenCopied += ((DESC_STRU_S *)TmpDesc)->DescLen + 2;

                    ((DESC_STRU_S *)TmpDesc)->Next = (DESC_STRU_S *)byteptr1;

                    if (DescLenCopied >= EvtDescLen)
                    {
                        ((DESC_STRU_S *)TmpDesc)->Next = HI_NULL_PTR;
                        break;
                    }
                    else
                    {
                        TmpDesc = (hi_u8 *)((DESC_STRU_S *)TmpDesc)->Next;
                    }
                }

                i += 12 + EvtDescLen;

                //HI_WARN_DEMUX("N=%d, i=%d, byteptr1=%x, EvtDescLen=%d\n", N, i, byteptr1, EvtDescLen);

                ((EVT_INFO *)byteptr)->Next = (EVT_INFO *)byteptr1;

                if (i >= N)
                {
                    //current section is completed
                    break;
                }
                else
                {
                    byteptr = byteptr1;
                }
            }
        } while (cur_section != last_section);

        ((EVT_INFO *)byteptr)->Next = HI_NULL_PTR;
    }
    else
    {
        VTOP_MemFree(EitTbl.evt_buf);
        EitTbl.evt_buf = HI_NULL_PTR;
        PRINT_PSISI("PSISI Error: Fail to get EIT table, section read fail!\n");
    }

    VTOP_MemFree(bufptr);

    *tbladdr = &EitTbl;

    hi_unf_dmx_close_play_chan(eit_ch);
    hi_unf_dmx_detach_filter(eit_filter_id, eit_ch);
    hi_unf_dmx_destroy_filter(eit_filter_id);
    hi_unf_dmx_destroy_play_chan(eit_ch);

    return s32ret;
}

hi_void HI_API_PSISI_FreeEitTbl(hi_void)
{
    VTOP_MemFree(EitTbl.evt_buf);
    EitTbl.evt_buf = HI_NULL_PTR;
}

hi_void HI_API_PSISI_Init(hi_void)
{
    hi_u32 i;

    if (PSISI_InitFlag == 1)
    {
        return;
    }

    /*init filter*/
    for (i = 0; i < DMX_FILTER_MAX_DEPTH; i++)
    {
        filter_match[i] = 0x00;
        filter_mask[i]   = 0xff;
        filter_negate[i] = 0x00;
    }

    PatTbl.ProgNum = 0;
    PatTbl.pat_buf = HI_NULL_PTR;

    CatTbl.len = 0;
    CatTbl.cat_buf = HI_NULL_PTR;

    NitTbl.NetDescLen = 0;
    NitTbl.NetDesc = HI_NULL_PTR;
    NitTbl.StreamDescLen = 0;
    NitTbl.StreamDesc = HI_NULL_PTR;

    PmtTbl.pmt_buf = HI_NULL_PTR;

#if 0
    prog_tbl.prog_num = 0;
    prog_tbl.proginfo = HI_NULL_PTR;
#endif

    PSISI_InitFlag = 1;
}

hi_void HI_API_PSISI_Destroy(hi_void)
{
    if (PSISI_InitFlag == 0)
    {
        return;
    }

    PSISI_InitFlag = 0;

    if (PatTbl.pat_buf != HI_NULL_PTR)
    {
        HI_API_PSISI_FreePatTbl();
    }

    if (CatTbl.cat_buf != HI_NULL_PTR)
    {
        HI_API_PSISI_FreeCatTbl();
    }

    if ((NitTbl.NetDesc != HI_NULL_PTR) || (NitTbl.StreamDesc != HI_NULL_PTR))
    {
        HI_API_PSISI_FreeNitTbl();
    }

    if (PmtTbl.pmt_buf != HI_NULL_PTR)
    {
        HI_API_PSISI_FreePmtTbl();
    }
}

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* __cplusplus */
