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
hi_handle          g_rec_handle;


hi_void ts_tx_thread(hi_void *args)
{
    hi_u32                readlen;
    hi_s32                ret;
    dmx_ram_buffer        buf;
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);

        ret = hi_mpi_dmx_ram_get_buffer(g_ts_buf, 188 * 200, &buf, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("ramport get buffer failed!\n");
            continue;
        }

        readlen = fread(buf.data, sizeof(hi_s8), buf.length, g_ts_in_file);
        if (readlen <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_in_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_mpi_dmx_ram_put_buffer(g_ts_buf, buf.length, 0);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_tsbuffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_mutex);
        usleep(1000 * 10);

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
    hi_s32 ret;
    dmx_buffer st_buf;

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_mpi_dmx_rec_acquire_buf(g_rec_handle, 500 * 188, &st_buf, 5000);
        if (ret != HI_SUCCESS ) {
            usleep(10 * 1000);
            continue;
        }

        //printf("data_addr[0x%x], length[0x%x]\n", (hi_u32)pst_buf.data, pst_buf.length);
        ret = fwrite(st_buf.data, 1, st_buf.length, g_ts_out_file);
        if (st_buf.length != ret) {
            printf("fwrite failed, ret[0x%x], st_buf.length[0x%x]\n", ret, st_buf.length);
        }

        ret = hi_mpi_dmx_rec_release_buf(g_rec_handle, &st_buf);
        if (HI_SUCCESS != ret) {
            printf("call hi_mpi_dmx_rec_release_buf failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;
    hi_handle  band_handle;
    hi_char input_cmd[32];
    hi_s32 pid[5] = {-1};
    hi_handle  pid_ch_handle[5] = {-1};
    hi_s32 i = 0;

    dmx_ram_port_attr attrs = {0};
    dmx_port_attr port_attrs = {0};
    dmx_rec_attrs rec_attrs = {0};
    dmx_band_attr  band_entry = {0};

    if (argc < 3 || argc > 8) {
        printf("\nuseage: record ts_file and save it as rec_ts_file\n");
        printf("\nrecord tsfile as rec_ts_file max support 5 pids.\n");
        printf("\nargument:\n");
        printf("    ts_file : source ts file.\n");
        printf("    rec_ts_file : dest ts file.\n");
        printf("    pid     : the pid of ts file.\n");
        printf("    ...     : the pid of ts file.\n");
        return -1;
    }

    for (i = 0; i < argc - 3; i++) {
        if (strstr(argv[3 + i], "0x") || strstr(argv[3 + i], "0X")) {
            pid[i] = strtoul(argv[3 + i], NULL, 16);
        } else {
            pid[i] = strtoul(argv[3 + i], NULL, 10);
        }
    }

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open input ts file failed!\n");
        goto out;
    }

    g_ts_out_file = fopen(argv[2], "wb");
    if (g_ts_out_file == HI_NULL) {
        printf("open output ts file failed!\n");
        goto DMX_CLOSE_IN_FILE;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_init failed.\n");
        goto DMX_CLOSE_OUT_FILE;
    }

    ret = hi_mpi_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_init failed.\n");
        goto DMX_SYS_DEINIT;
    }

    ret = hi_mpi_dmx_ram_get_port_default_attrs(&attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_get_port_default_attrs failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_mpi_dmx_ram_open_port(DMX_RAM_PORT_0, &attrs, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_open_port failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_mpi_dmx_ram_get_port_attrs(g_ts_buf, &port_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_get_port_attrs failed.\n");
        goto DMX_RAM_CLOSE;
    }

    ret = hi_mpi_dmx_ram_set_port_attrs(g_ts_buf, &port_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_set_port_attrs failed.\n");
        goto DMX_RAM_CLOSE;
    }

    band_entry.band_attr = 0;
    ret = hi_mpi_dmx_band_open(DMX_BAND_0, &band_entry, &band_handle);
    if (ret != HI_SUCCESS) {
        printf("call dmx_api_utils_band_get_handle failed.\n");
        goto DMX_RAM_CLOSE;
    }

    ret = hi_mpi_dmx_band_attach_port(band_handle, DMX_RAM_PORT_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_band_attach_port failed.\n");
        goto DMX_BAND_CLOSE;
    }

    ret = hi_mpi_dmx_rec_get_default_attrs(&rec_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_rec_get_default_attrs failed!\n");
        goto DMX_BAND_DETACH;
    }

    if (argc > 3) {
        printf("start the select pid record!\n");
        rec_attrs.rec_type = DMX_REC_TYPE_SELECT_PID;
    }

    ret = hi_mpi_dmx_rec_create(&rec_attrs, &g_rec_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_rec_create failed.\n");
        goto DMX_BAND_DETACH;
    }

    if (argc > 3 && rec_attrs.rec_type == DMX_REC_TYPE_SELECT_PID) {
        for (i = 0; i < argc - 3; i++) {
            printf("begin call hi_mpi_dmx_pid_ch_create, pid[0x%x].\n", pid[i]);
            ret = hi_mpi_dmx_pid_ch_create(band_handle, pid[i], &pid_ch_handle[i]);
            if (ret != HI_SUCCESS) {
                printf("call hi_mpi_dmx_pid_ch_create failed.\n");
                goto DMX_REC_DESTROY;
            }

            ret = hi_mpi_dmx_rec_add_ch(g_rec_handle, pid_ch_handle[i]);
            if (ret != HI_SUCCESS) {
                printf("call hi_mpi_dmx_rec_add_ch failed.\n");
                goto DMX_REC_DESTROY;
            }
            printf("after call hi_mpi_dmx_rec_add_ch.\n");
        }
    } else if (argc == 3 && rec_attrs.rec_type == DMX_REC_TYPE_ALL_PID) {
        ret = hi_mpi_dmx_rec_add_ch(g_rec_handle, band_handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_rec_add_ch failed.\n");
            goto DMX_REC_DESTROY;
        }
    } else {
        printf("invalid argument argc[0x%x], rec_attrs.rec_type[0x%x].\n", argc, rec_attrs.rec_type);
        goto DMX_REC_DESTROY;
    }

    ret = hi_mpi_dmx_rec_open(g_rec_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_rec_open failed.\n");
        goto DMX_REC_DELCH;
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

    hi_mpi_dmx_rec_close(g_rec_handle);

DMX_REC_DELCH:
    if (argc > 3 && rec_attrs.rec_type == DMX_REC_TYPE_SELECT_PID) {
        hi_mpi_dmx_rec_del_all_ch(g_rec_handle);

        for (i = 0; i < argc - 3; i++) {
            hi_mpi_dmx_pid_ch_destroy(pid_ch_handle[i]);
        }
    } else if (argc == 3 && rec_attrs.rec_type == DMX_REC_TYPE_ALL_PID) {
        hi_mpi_dmx_rec_del_ch(g_rec_handle, band_handle);
    }
DMX_REC_DESTROY:
    hi_mpi_dmx_rec_destroy(g_rec_handle);

DMX_BAND_DETACH:
    hi_mpi_dmx_band_detach_port(band_handle);

DMX_BAND_CLOSE:
    hi_mpi_dmx_band_close(band_handle);

DMX_RAM_CLOSE:
    hi_mpi_dmx_ram_close_port(g_ts_buf);

DMX_DEINIT:
    hi_mpi_dmx_de_init();

DMX_SYS_DEINIT:
    hi_unf_sys_deinit();

DMX_CLOSE_IN_FILE:
    if (g_ts_in_file != HI_NULL) {
        fclose(g_ts_in_file);
        g_ts_in_file = HI_NULL;
    }

DMX_CLOSE_OUT_FILE:
    if (g_ts_out_file != HI_NULL) {
        fclose(g_ts_out_file);
        g_ts_out_file = HI_NULL;
    }

out:
    return 0;
}


