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
#include "hi_unf_demux.h"

#define PLAY_DMX_ID 0x0

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
    hi_u32                readlen;
    hi_s32                ret;
    hi_unf_stream_buf   buf;
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);

        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, 188 * 200, &buf, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("ramport get buffer failed!\n");
            continue;
        }

        readlen = fread(buf.data, sizeof(hi_s8), buf.size, g_ts_in_file);
        if (readlen <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_in_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_unf_dmx_put_ts_buffer(g_ts_buf, readlen, 0);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_mutex);
        usleep(1000 * 10);

    }

    printf("ts send end!\n");
    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}

hi_void ts_rx_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_s32 i = 0;
    hi_u32 acquire_num = 10;
    hi_u32 acquired_num;
    hi_unf_dmx_data st_buf[10];

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_buffer(g_ply_handle, acquire_num, &acquired_num, st_buf, 5000);
        if (ret != HI_SUCCESS) {
            usleep(10 * 1000);
            continue;
        }

        if (acquired_num > 10) {
            printf("acquired_num[0x%x] invalid!\n", acquired_num);
            usleep(10 * 1000);
            continue;
        }

        for (i = 0; i < acquired_num; i++) {
            ret = fwrite(st_buf[i].data, 1, st_buf[i].size, g_ts_out_file);
            if (st_buf[i].size != ret) {
                printf("fwrite failed, ret[0x%x], st_buf[i].length[0x%x]\n", ret, st_buf[i].size);
            }
        }

        ret = hi_unf_dmx_release_buffer(g_ply_handle, acquired_num, st_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_release_buf failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;
    hi_char input_cmd[32];
    hi_s32 pid = -1;
    hi_unf_dmx_ts_buffer_attr ram_attrs = {0};
    hi_unf_dmx_chan_attr play_attrs = {0};

    if (argc != 4) {
        printf("\nuseage: savets ts_file ts1_file pid\n");
        printf("\ncreate an pid tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    source ts_file : source ts file.\n");
        printf("    dest ts file : dest ts file.\n");
        printf("    pid     : the pid of video or audio.\n");
        return -1;
    }

    if (strstr(argv[3], "0x") || strstr(argv[3], "0X")) {
        pid = strtoul(argv[3], NULL, 16);
    } else {
        pid = strtoul(argv[3], NULL, 10);
    }

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open ts in file failed!\n");
        goto out;
    }

    g_ts_out_file = fopen(argv[2], "wb");
    if (g_ts_out_file == HI_NULL) {
        printf("open ts out file failed!\n");
        goto DMX_CLOSE_IN_FILE;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_init failed.\n");
        goto DMX_CLOSE_OUT_FILE;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_init failed.\n");
        goto DMX_SYS_DEINIT;
    }

    ret = hi_unf_dmx_get_ts_buffer_default_attr(&ram_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_ts_buffer_default_attr failed.\n");
        goto DMX_DEINIT;
    }

    ram_attrs.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ram_attrs.buffer_size = 0x200000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ram_attrs, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto DMX_DESTROY_TSBUF;
    }

    ret = hi_unf_dmx_get_play_chan_default_attr(&play_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_play_chan_default_attr failed.\n");
        goto DMX_RAMPORT_DETACH;
    }

    play_attrs.chan_type = HI_UNF_DMX_CHAN_TYPE_POST;
    ret = hi_unf_dmx_create_play_chan(PLAY_DMX_ID, &play_attrs, &g_ply_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_play_chan failed.\n");
        goto DMX_RAMPORT_DETACH;
    }

    ret = hi_unf_dmx_set_play_chan_pid(g_ply_handle, pid);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_set_play_chan_pid failed.\n");
        goto DMX_PLY_DESTROY;
    }

    ret = hi_unf_dmx_open_play_chan(g_ply_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_open_play_chan failed.\n");
        goto DMX_PLY_DESTROY;
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

    hi_unf_dmx_close_play_chan(g_ply_handle);

DMX_PLY_DESTROY:
    hi_unf_dmx_destroy_play_chan(g_ply_handle);

DMX_RAMPORT_DETACH:
    hi_unf_dmx_detach_ts_port(PLAY_DMX_ID);

DMX_DESTROY_TSBUF:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);

DMX_DEINIT:
    hi_unf_dmx_deinit();

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
