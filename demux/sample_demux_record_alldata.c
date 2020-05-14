/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Define functions used to test function of demux driver
 * Author: sdk
 * Create: 2019-11-25
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <securec.h>

#include "hi_type.h"
#include "hi_errno.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#define DEMUX_ID        0
#define TUNER_ID        0  /* tunerID */

typedef struct {
    hi_handle   rec_handle;
    hi_char     file_name[256]; /* array 256 bytes len */
    hi_bool     thread_run_flag;
} ts_file_info;

hi_void *tlv_save_rec_data_thread(hi_void *arg)
{
    hi_s32 ret;
    ts_file_info *ts = (ts_file_info *)arg;
    FILE *rec_file = HI_NULL;

    rec_file = fopen(ts->file_name, "w");
    if (rec_file == HI_NULL) {
        perror("fopen error");
        ts->thread_run_flag = HI_FALSE;
        return HI_NULL;
    }

    printf("[%s] open file %s\n", __FUNCTION__, ts->file_name);

    while (ts->thread_run_flag) {
        hi_unf_dmx_rec_data rec_data;

        ret = hi_unf_dmx_acquire_rec_data(ts->rec_handle, &rec_data, 100); /* 100 ms timeout */
        if (ret != HI_SUCCESS) {
            if (ret == HI_ERR_DMX_TIMEOUT || ret == HI_ERR_DMX_NOAVAILABLE_DATA) {
                continue;
            }

            printf("[%s] hi_unf_dmx_acquire_rec_data failed 0x%x\n", __FUNCTION__, ret);
            break;
        }

        if (rec_data.len != fwrite(rec_data.data_addr, 1, rec_data.len, rec_file)) {
            perror("[SaveRecDataThread] fwrite error");
            break;
        }

        ret = hi_unf_dmx_release_rec_data(ts->rec_handle, &rec_data);
        if (ret != HI_SUCCESS) {
            printf("[%s] hi_unf_dmx_release_rec_data failed 0x%x\n", __FUNCTION__, ret);
            break;
        }
    }

    fclose(rec_file);
    ts->thread_run_flag = HI_FALSE;

    return HI_NULL;
}

hi_s32 tlv_dmx_start_record(hi_char *path)
{
    hi_s32                  ret;
    hi_unf_dmx_rec_attr     rec_attr = {0};
    hi_handle               rec_handle;
    hi_bool                 rec_status = HI_FALSE;
    pthread_t               rec_thread_id = -1;
    ts_file_info            ts_rec_info = {0};
    hi_char                 file_name[256]; /* 256 bytes len */

    rec_attr.dmx_id          = DEMUX_ID;
    rec_attr.rec_buf_size    = 16 * 1024 * 1024; /* 16 * 1024 * 1024 means 16 MB */
    /* this is important for recv TVL data */
    rec_attr.rec_type        = HI_UNF_DMX_REC_TYPE_ALL_DATA;
    rec_attr.descramed       = HI_TRUE;
    rec_attr.index_type      = HI_UNF_DMX_REC_INDEX_TYPE_NONE;

    ret = hi_unf_dmx_create_rec_chan(&rec_attr, &rec_handle);
    if (ret != HI_SUCCESS) {
        printf("[%s - %u] hi_unf_dmx_create_rec_chan failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        return ret;;
    }

    ret = snprintf_s(file_name, sizeof(file_name), sizeof(file_name) - 1, "%s/recv_alldata", path);
    if (ret == -1) {
            perror("snprintf_s failed !\n");
            goto exit;
        }

    ret = hi_unf_dmx_start_rec_chan(rec_handle);
    if (ret != HI_SUCCESS) {
        printf("[%s - %u] hi_unf_dmx_create_rec_chan failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    rec_status = HI_TRUE;
    ts_rec_info.rec_handle    = rec_handle;
    ts_rec_info.thread_run_flag = HI_TRUE;

    ret = snprintf_s(ts_rec_info.file_name, sizeof(ts_rec_info.file_name), sizeof(ts_rec_info.file_name) - 1,
        "%s.alldata", file_name);
    if (ret == -1) {
        perror("snprintf_s failed !\n");
        goto exit;
    }

    ret = pthread_create(&rec_thread_id, HI_NULL, tlv_save_rec_data_thread, &ts_rec_info);
    if (ret != 0) {
        perror("[DmxStartRecord] pthread_create record error");
        goto exit;
    }

    sleep(1);  /* delay 1 m */

    while (1) {
        hi_char input_cmd[256] = {0}; /* array 256 bytes len */
        printf("please input the q to quit!\n");
        fgets(input_cmd, sizeof(input_cmd) - 1, stdin);
        if (input_cmd[0] == 'q') {
            break;
        }
    }

exit :
    if (rec_thread_id != -1) {
        ts_rec_info.thread_run_flag = HI_FALSE;
        pthread_join(rec_thread_id, HI_NULL);
    }

    if (rec_status == HI_TRUE) {
        ret = hi_unf_dmx_stop_rec_chan(rec_handle);
        if (ret != HI_SUCCESS) {
            printf("[%s - %u] HI_UNF_DMX_StopRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        }
    }

    ret = hi_unf_dmx_destroy_rec_chan(rec_handle);
    if (ret != HI_SUCCESS) {
        printf("[%s - %u] HI_UNF_DMX_DestroyRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);
    }

    return ret;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32      ret;
    hi_char    *path = HI_NULL;
    hi_u32      freq;
    hi_u32      symbol_rate;
    hi_u32      thrid_param;
    hi_char     section_name[SECTION_MAX_LENGTH] = {0};

    if (argc < 0x3) {
        printf("Usage: %s path freq [srate] [qamtype or polarization]\n"
               "       qamtype or polarization: \n"
               "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
               "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n",
               argv[0]);

        return HI_FAILURE;
    }

    path = argv[1];
    freq  = strtol(argv[0x2], NULL, 0);
    symbol_rate = (freq > 1000) ? 27500 : 6875;  /* if freq > 1000, dvb-s:27500, dvb-c:6875 */
    thrid_param = (freq > 1000) ? 0 : 64; /* if freq > 1000, thrid_param set 0 or 64 */

    if (argc >= 0x4) { /* more than 4 parameters */
        symbol_rate  = strtol(argv[0x3], NULL, 0);
    }

    if (argc >= 0x5) { /* more than 5 parameters */
        thrid_param = strtol(argv[0x4], NULL, 0);
    }

    ret = snprintf_s(section_name, SECTION_MAX_LENGTH, SECTION_MAX_LENGTH - 1, "frontend%dinfo", TUNER_ID);
    if (ret == -1) {
        perror("snprintf_s failed !\n");
        return HI_FAILURE;
    }

    ret = hi_unf_sys_init();
    if (ret == -1) {
        perror("hi_unf_sys_init failed !\n");
        return HI_FAILURE;
    }

    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_frontend_init failed.\n");
        goto tuner_deinit;
    }

    ret = hi_adp_frontend_connect(TUNER_ID, freq, symbol_rate, thrid_param);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_frontend_connect failed.\n");
        goto tuner_deinit;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("hi_unf_dmx_init failed 0x%x\n", ret);
        goto tuner_deinit;
    }

    ret = hi_adp_dmx_attach_ts_port(DEMUX_ID, TUNER_ID);
    if (ret != HI_SUCCESS) {
        printf("hi_adp_dmx_attach_ts_port failed 0x%x\n", ret);
        goto tuner_deinit;
    }

    tlv_dmx_start_record(path);

    hi_unf_dmx_detach_ts_port(DEMUX_ID);
    hi_unf_dmx_deinit();

tuner_deinit:
    hi_adp_frontend_deinit();
    hi_unf_sys_deinit();

    return ret;
}

