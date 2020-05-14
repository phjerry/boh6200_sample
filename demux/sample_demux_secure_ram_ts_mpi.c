/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Define functions used to test function of demux driver
 * Author: sdk
 * Create: 2019-5-25
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "hi_unf_system.h"

#include "hi_mpi_demux.h"
#include "hi_unf_ssm.h"
#include "hi_unf_demux.h"

#define TS_PKT_LEN 188
#define TS_PKT_NUM 200
#define DEFAULT_TIME_OUT 1000
#define DEFAULT_TIME_OUT_LOOP 10
#define DEFAULT_ACQUIRE_NUM 10
#define DEFAULT_STR_LEN 32
#define BASE_16 16
#define BASE_10 10

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

FILE               *g_ts_in_file = HI_NULL;
FILE               *g_ts_out_file = HI_NULL;

static pthread_t   g_ts_put_thread;
static pthread_t   g_ts_get_thread;
static pthread_mutex_t g_ts_mutex;
static hi_bool     g_stop_ts_put_thread = HI_FALSE;
static hi_bool     g_stop_ts_get_thread = HI_FALSE;

hi_handle          g_ts_buf;
hi_handle          g_ply_handle;


hi_void ts_tx_thread(hi_void *args)
{
    hi_s32                ret;
    dmx_ram_buffer        buf;
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);

        ret = hi_mpi_dmx_ram_get_buffer(g_ts_buf, TS_PKT_LEN * TS_PKT_NUM, &buf, DEFAULT_TIME_OUT);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("ramport get buffer failed!\n");
            continue;
        }

        ret = hi_mpi_dmx_ram_put_buffer(g_ts_buf, buf.length, 0);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_tsbuffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_mutex);
        usleep(DEFAULT_TIME_OUT_LOOP * DEFAULT_TIME_OUT);
    }
    printf("ts send end!\n");
    ret = hi_mpi_dmx_ram_reset_buffer(g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_tsbuffer failed.\n");
    }

    return;
}

hi_void ts_rx_thread(hi_void *args)
{
    hi_u32 ret;
    hi_u32 acquire_num = DEFAULT_ACQUIRE_NUM;
    hi_u32 acquired_num;
    dmx_buffer st_buf[DEFAULT_ACQUIRE_NUM];

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_mpi_dmx_play_acquire_buf(g_ply_handle, acquire_num, &acquired_num, st_buf, DEFAULT_TIME_OUT);
        if (ret != HI_SUCCESS) {
            usleep(DEFAULT_TIME_OUT_LOOP * DEFAULT_TIME_OUT);
            continue;
        }

        if (acquired_num > DEFAULT_ACQUIRE_NUM) {
            printf("acquired_num[0x%x] invalid!\n", acquired_num);
            usleep(DEFAULT_TIME_OUT_LOOP * DEFAULT_TIME_OUT);
            continue;
        }

        ret = hi_mpi_dmx_play_release_buf(g_ply_handle, acquired_num, st_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_release_buf failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle  band_handle;
    hi_handle  pid_ch_handle;
    hi_char input_cmd[DEFAULT_STR_LEN];
    hi_s32 pid = -1;
    dmx_ram_port_attr ram_attrs;
    dmx_port_attr port_attrs;
    dmx_play_attrs play_attrs;
    dmx_band_attr  band_entry;
    hi_unf_ssm_attr ssm_intent;
    hi_handle ssm;
    hi_unf_ssm_module_resource ssm_res;

    if (argc != 0x4) {
        printf("\nuseage: savets ts_file ts1_file pid\n");
        printf("\ncreate an pid tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    ts_file : source ts file.\n");
        printf("    ts1_file : dest ts file.\n");
        printf("    pid     : the pid of video or audio.\n");
        return -1;
    }

    if (strstr(argv[0x3], "0x") || strstr(argv[0x3], "0X")) {
        pid = strtoul(argv[0x3], NULL, BASE_16);
    } else {
        pid = strtoul(argv[0x3], NULL, BASE_10);
    }

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open input ts file failed!\n");
        goto out;
    }

    g_ts_out_file = fopen(argv[0x2], "wb");
    if (g_ts_out_file == HI_NULL) {
        printf("open output ts file failed!\n");
        goto dmx_close_in_file;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call HI_SYS_Init failed.\n");
        goto dmx_close_out_file;
    }

    ret = hi_mpi_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_init failed.\n");
        goto dmx_sys_deinit;
    }

    ret = hi_unf_ssm_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_init failed, ret %x.\n", ret);
        goto dmx_deinit;
    }

    ssm_intent.intent = HI_UNF_SSM_INTENT_WATCH;
    ret = hi_unf_ssm_create(&ssm_intent, &ssm);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_create failed, ret %x.\n", ret);
        goto ssm_deinit;
    }

    ret = hi_mpi_dmx_ram_get_port_default_attrs(&ram_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_get_port_default_attrs failed.\n");
        goto ssm_destroy;
    }

    ram_attrs.secure_mode = DMX_SECURE_TEE;
    ret = hi_mpi_dmx_ram_open_port(DMX_RAM_PORT_0, &ram_attrs, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_open_port failed.\n");
        goto ssm_destroy;
    }

    ret = hi_mpi_dmx_ram_get_port_attrs(g_ts_buf, &port_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_get_port_attrs failed.\n");
        goto dmx_ram_close;
    }

    ret = hi_mpi_dmx_ram_set_port_attrs(g_ts_buf, &port_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_set_port_attrs failed.\n");
        goto dmx_ram_close;
    }

    band_entry.band_attr = 0;
    ret = hi_mpi_dmx_band_open(DMX_BAND_0, &band_entry, &band_handle);
    if (ret != HI_SUCCESS) {
        printf("call dmx_api_utils_band_get_handle failed.\n");
        goto dmx_ram_close;
    }

    ret = hi_mpi_dmx_band_attach_port(band_handle, DMX_RAM_PORT_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_band_attach_port failed.\n");
        goto dmx_band_close;
    }

    ret = hi_mpi_dmx_pid_ch_create(band_handle, pid, &pid_ch_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_pid_ch_create failed.\n");
        goto dmx_band_detach;
    }

    ret = hi_mpi_dmx_play_get_default_attrs(&play_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_play_get_default_attrs failed.\n");
        goto dmx_pidch_destroy;
    }
    play_attrs.secure_mode = DMX_SECURE_TEE;
    ret = hi_mpi_dmx_play_create(&play_attrs, &g_ply_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_play_create failed.\n");
        goto dmx_pidch_destroy;
    }

    ret = hi_mpi_dmx_play_attach_pid_ch(g_ply_handle, pid_ch_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_play_attach_pid_ch failed.\n");
        goto dmx_ply_destroy;
    }

    ssm_res.module_handle = g_ts_buf;
    ret = hi_unf_ssm_add_resource(ssm, &ssm_res);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_add_resource failed, ret %x.\n", ret);
        goto dmx_ply_detach;
    }

    ssm_res.module_handle = g_ply_handle;
    ret = hi_unf_ssm_add_resource(ssm, &ssm_res);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_add_resource failed, ret %x.\n", ret);
        goto dmx_ply_detach;
    }

    ret = hi_unf_dmx_ts_buffer_attach_ssm(g_ts_buf, ssm, HI_UNF_DMX_BUFFER_ATTACH_TYPE_TSR2RCIPHER);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_ts_buffer_attach_ssm failed, ret %x.\n", ret);
        goto dmx_ply_detach;
    }

    ret = hi_unf_dmx_play_chan_attach_ssm(g_ply_handle, ssm);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_play_chan_attach_ssm failed, ret %x.\n", ret);
        goto dmx_ply_detach;
    }

    ret = hi_mpi_dmx_play_open(g_ply_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_play_open failed.\n");
        goto dmx_ply_detach;
    }

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_put_thread = HI_FALSE;
    g_stop_ts_get_thread = HI_FALSE;

    pthread_create(&g_ts_put_thread, HI_NULL, (hi_void *)ts_tx_thread, HI_NULL);
    pthread_create(&g_ts_get_thread, HI_NULL, (hi_void *)ts_rx_thread, HI_NULL);

    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_ts_put_thread = HI_TRUE;
    g_stop_ts_get_thread = HI_TRUE;
    pthread_join(g_ts_put_thread, HI_NULL);
    pthread_join(g_ts_get_thread, HI_NULL);
    pthread_mutex_destroy(&g_ts_mutex);

    hi_mpi_dmx_play_close(g_ply_handle);

dmx_ply_detach:
    hi_mpi_dmx_play_detach_pid_ch(g_ply_handle);

dmx_ply_destroy:
    hi_mpi_dmx_play_destroy(g_ply_handle);

dmx_pidch_destroy:
    hi_mpi_dmx_pid_ch_destroy(pid_ch_handle);

dmx_band_detach:
    hi_mpi_dmx_band_detach_port(band_handle);

dmx_band_close:
    hi_mpi_dmx_band_close(band_handle);

dmx_ram_close:
    hi_mpi_dmx_ram_close_port(g_ts_buf);

ssm_destroy:
    hi_unf_ssm_destroy(ssm);

ssm_deinit:
    hi_unf_ssm_deinit();

dmx_deinit:
    hi_mpi_dmx_de_init();

dmx_sys_deinit:
    hi_unf_sys_deinit();

dmx_close_in_file:
    if (g_ts_in_file != HI_NULL) {
        fclose(g_ts_in_file);
        g_ts_in_file = HI_NULL;
    }

dmx_close_out_file:
    if (g_ts_out_file != HI_NULL) {
        fclose(g_ts_out_file);
        g_ts_out_file = HI_NULL;
    }

out:
    return 0;
}


