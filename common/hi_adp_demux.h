/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2006-2019. All rights reserved.
 * Description: adp_demux module external output
 */

#ifndef  _COMMON_DEMUX_H
#define  _COMMON_DEMUX_H

#include "hi_type.h"
#include "hi_unf_demux.h"
#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif  /* __cplusplus */

#define MAX_SECTION_LEN 4096
#define MAX_SECTION_NUM 64
#define INVALID_PORT_DMX_ID 0xFFFF
#define MAX_IP_THREAD_CNT 16
#define MAX_FILE_THREAD_CNT 16

typedef hi_s32 (*t_commsectioncallback)(const hi_u8 *buffer, hi_s32 buffer_length, hi_u8 *section_struct);

typedef struct  hi_dmx_data_filter {
    hi_u32 tspid;                /* TSPID */
    hi_u32 buf_size;             /* hareware BUFFER request */

    hi_u8 section_type;          /* section type, 0-section 1-PES */
    hi_u8 crcflag;               /* channel CRC open flag, 0-not open; 1-open */

    hi_u8  match[DMX_FILTER_MAX_DEPTH];
    hi_u8  mask[DMX_FILTER_MAX_DEPTH];
    hi_u8  negate[DMX_FILTER_MAX_DEPTH];
    hi_u16 filter_depth;         /* filtrate depth, 0xff-data use all the user set, otherwise, use DVB algorithm(fixme) */

    hi_u32 time_out;             /* timeout, in second. 0-permanent wait */

    t_commsectioncallback fun_section_fun_callback;   /* section end callback */
    hi_u8 *section_struct;
} dmx_data_filter;

hi_s32 dmx_section_start_data_filter(hi_u32 dmx_id, dmx_data_filter *data_filter);
hi_s32 dmx_section_start_data_filter_ext(hi_u32 dmx_id, dmx_data_filter *data_filter, hi_bool tee_enable);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif  /* __cplusplus */

#endif /* _SECTIONSVR_PUB_H*/
