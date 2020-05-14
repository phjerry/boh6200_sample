/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Define functions used to test function of demux driver
 * Author: sdk
 * Create: 2019-8-22
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_mpi_memory.h"
#include "hi_common.h"
#include "hi_unf_demux.h"
#include "hi_demux_record.h"


typedef struct {
    HI_U32      dmx_id;
    HI_HANDLE   record_handle;
    HI_BOOL     record_status;
    pthread_t   record_thread_id;
    HI_BOOL     thread_run_flag;
} hi_demux_record_info;

static hi_demux_record_info    g_demux_rcord_info[RECORD_COUNT];


hi_void* ts_record_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    HI_UNF_DMX_REC_DATA_S st_buf = {0};
    hi_demux_record_info *rcord_info = (hi_demux_record_info*)args;
    HI_U32 addrOffset = 0;
    HI_U8* recTmpData = hi_mpi_malloc(HI_ID_DEMUX, MEMORY_SIZE);
    if (recTmpData == HI_NULL) {
        printf("malloc failed\n");
    }
    printf("record start!\n");
    while (rcord_info->thread_run_flag == HI_TRUE) {

        ret = HI_UNF_DMX_AcquireRecData(rcord_info->record_handle, &st_buf, 1000);
        if (ret != HI_SUCCESS) {
            usleep(10 * 1000);
            continue;
        }

        if (addrOffset>MEMORY_SIZE-(3840*2160)) {
            addrOffset=0;
        } else {
            memcpy(recTmpData+addrOffset, st_buf.pDataAddr, st_buf.u32Len);
            addrOffset += st_buf.u32Len;
        }

        ret = HI_UNF_DMX_ReleaseRecData(rcord_info->record_handle, &st_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_rec_release_buf failed!\n");
        }

    }
    free(recTmpData);
    recTmpData = HI_NULL;
    printf("record over!\n");
    return HI_NULL;
}

HI_S32 hi_demux_record_ts(HI_U32 record_id, HI_U32 dmx_id)
{
    hi_s32 ret;
    HI_UNF_DMX_REC_ATTR_S    recAttr = {0};
    hi_demux_record_info      *rcord_info;

    if (record_id >= RECORD_COUNT) {
        printf("[%s - %u] record_id %u error\n", __FUNCTION__, __LINE__, record_id);
        return HI_FAILURE;
    }

    rcord_info = &g_demux_rcord_info[record_id];
    rcord_info->record_status     = HI_FALSE;
    rcord_info->record_thread_id  = -1;
    rcord_info->thread_run_flag   = HI_FALSE;

    recAttr.u32DmxId        = dmx_id + record_id;
    recAttr.u32RecBufSize   = 5 * 1024 * 1024;
    recAttr.enRecType       = HI_UNF_DMX_REC_TYPE_ALL_PID;
    recAttr.bDescramed      = HI_TRUE;
    recAttr.enIndexType     = HI_UNF_DMX_REC_INDEX_TYPE_NONE;

    ret = hi_unf_dmx_attach_ts_port(recAttr.u32DmxId, HI_UNF_DMX_PORT_RAM_0);
    if (HI_SUCCESS != ret) {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        return ret;
    }
    rcord_info->dmx_id = recAttr.u32DmxId;
    ret = HI_UNF_DMX_CreateRecChn(&recAttr, &rcord_info->record_handle);
    if (ret != HI_SUCCESS) {
        printf("call HI_UNF_DMX_CreateRecChn failed.\n");
        goto DETACH_PORT;
    }

    rcord_info->thread_run_flag = HI_TRUE;
    ret = pthread_create(&rcord_info->record_thread_id, HI_NULL,
            (hi_void *)ts_record_thread, (hi_void*)rcord_info);
    if (ret != 0) {
        perror("[DmxStartRecord] pthread_create record error");
        goto STOP_CHN;
    }

    ret = HI_UNF_DMX_StartRecChn(rcord_info->record_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_rec_open failed.\n");
        goto DESTROY_CHN;
    }

    rcord_info->record_status = HI_TRUE;

    return ret;


STOP_CHN:
    HI_UNF_DMX_StopRecChn(rcord_info->record_handle);
DESTROY_CHN:
    HI_UNF_DMX_DestroyRecChn(rcord_info->record_handle);
DETACH_PORT:
    HI_UNF_DMX_DetachTSPort(recAttr.u32DmxId);

    return ret;
}


HI_S32 hi_demux_record_ts_stop(HI_U32 record_id)
{
    HI_S32                  ret;
    hi_demux_record_info      *rcord_info;

    rcord_info = &g_demux_rcord_info[record_id];
    if (rcord_info->record_thread_id != -1) {
        rcord_info->thread_run_flag = HI_FALSE;
        pthread_join(rcord_info->record_thread_id, HI_NULL);
        rcord_info->record_thread_id = -1;
    }
    if (rcord_info->record_status == HI_TRUE) {
        ret = HI_UNF_DMX_StopRecChn(rcord_info->record_handle);
        if (ret != HI_SUCCESS) {
            printf("[%s - %u] HI_UNF_DMX_StopRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        }
    }

    ret = HI_UNF_DMX_DestroyRecChn(rcord_info->record_handle);
    if (ret != HI_SUCCESS) {
        printf("[%s - %u] HI_UNF_DMX_DestroyRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);
    }
    ret = HI_UNF_DMX_DetachTSPort(rcord_info->dmx_id);
    if (ret != HI_SUCCESS) {
        printf("[%s - %u] HI_UNF_DMX_DetachTSPort failed 0x%x\n", __FUNCTION__, __LINE__, ret);
    }

    return ret;
}
