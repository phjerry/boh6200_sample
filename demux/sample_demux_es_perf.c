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
#include <sys/time.h>

#include "hi_unf_system.h"
#include "hi_mpi_demux.h"
#include "hi_unf_demux.h"
#include "hi_adp_search.h"

#define PLAY_DMX_ID 0x0
#define INPUT_BUFFER_SIZE 188 * 200
#define PERF_THREAD_NUM 5
#define US_PER_SECOND 1000000
#define PERF_TIME_DIFF_US(a, b)  (((a.tv_sec) - (b.tv_sec))*1000000 + ((a.tv_usec) - (b.tv_usec)))

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

FILE               *g_ts_in_file = HI_NULL;

static pthread_t   g_ts_put_thread[PERF_THREAD_NUM];
static pthread_t   g_ts_get_thread[PERF_THREAD_NUM];
static hi_bool     g_stop_ts_put_thread = HI_FALSE;
static hi_bool     g_stop_ts_get_thread = HI_FALSE;

hi_handle          g_ts_buf[PERF_THREAD_NUM];
hi_handle          g_ply_handle[PERF_THREAD_NUM];

hi_u8 g_ts_data[INPUT_BUFFER_SIZE];

typedef struct {
    struct timeval last_time;
    hi_u32 cur_size;
    hi_u32 data_rate;
    hi_handle handle;
} perf_thread_info;

perf_thread_info g_perf_put_thread_info[PERF_THREAD_NUM];
perf_thread_info g_perf_get_thread_info[PERF_THREAD_NUM];

hi_void ts_tx_thread(hi_void *args)
{
    hi_s32                ret;
    hi_unf_stream_buf   buf;
    perf_thread_info *thread_info = (perf_thread_info *)args;
    hi_handle ts_buf = thread_info->handle;
    struct timeval cur_time;

    printf("ts(%x) send start!\n", ts_buf);

    while (!g_stop_ts_put_thread) {
        ret = hi_unf_dmx_get_ts_buffer(ts_buf, INPUT_BUFFER_SIZE, &buf, 1000);
        if (ret != HI_SUCCESS) {
            continue;
        }

        memcpy(buf.data, g_ts_data, INPUT_BUFFER_SIZE);

        ret = hi_unf_dmx_put_ts_buffer(ts_buf, INPUT_BUFFER_SIZE, 0);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        } else {
            thread_info->cur_size += INPUT_BUFFER_SIZE;
            gettimeofday(&cur_time, HI_NULL);
            if (PERF_TIME_DIFF_US(cur_time, thread_info->last_time) >= US_PER_SECOND) {
                thread_info->data_rate = thread_info->cur_size;
                thread_info->cur_size = 0;
                thread_info->last_time = cur_time;
            }
        }
    }

    printf("ts(%x) send end!\n", ts_buf);
    ret = hi_unf_dmx_reset_ts_buffer(ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}

hi_void ts_rx_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_unf_es_buf es_buf;
    perf_thread_info *thread_info = (perf_thread_info *)args;
    hi_handle play_handle = thread_info->handle;
    struct timeval cur_time;

    printf("play(%x) receive start!\n", play_handle);

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_es(play_handle, &es_buf);
        if (ret != HI_SUCCESS) {
            continue;
        }

        thread_info->cur_size += es_buf.buf_len;
        gettimeofday(&cur_time, HI_NULL);
        if (PERF_TIME_DIFF_US(cur_time, thread_info->last_time) >= US_PER_SECOND) {
            thread_info->data_rate = thread_info->cur_size;
            thread_info->cur_size = 0;
            thread_info->last_time = cur_time;
        }

        ret = hi_unf_dmx_release_es(play_handle, &es_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_release_es failed!\n");
        }
    }

    printf("play(%x) receive over!\n", play_handle);
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;
    hi_char input_cmd[32];
    hi_u32 pid = -1;
    hi_u32 thread_num;
    hi_u32 i;
    hi_u32 j;
    hi_unf_dmx_ts_buffer_attr ram_attrs = {0};
    hi_unf_dmx_chan_attr play_attrs = {0};

    if (argc != 4) {
        printf("\nuseage: sample_demux_es_perf ts_file pid\n");
        printf("\ndemux perf test.\n");
        printf("\nargument:\n");
        printf("    source ts_file : source ts file.\n");
        printf("               pid : pid of ts file.\n");
        printf("       thread_num  : thread_num.\n");
        return -1;
    }

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open ts file failed!\n");
        goto out;
    }

    if (fread(g_ts_data, sizeof(hi_u8), INPUT_BUFFER_SIZE, g_ts_in_file) <= 0) {
        printf("read ts file failed!\n");
        goto out;
    }

    if (argv[2] == HI_NULL) {
        printf("please input pid.\n");
        goto out;
    } else if (strstr(argv[2], "0x") || strstr(argv[2], "0X")) {
        pid = strtoul(argv[2], NULL, 16);
    } else {
        pid = strtoul(argv[2], NULL, 10);
    }

    if (argv[3] == HI_NULL) {
        printf("please input thread_num.\n");
        goto out;
    } else if (strstr(argv[3], "0x") || strstr(argv[3], "0X")) {
        thread_num = strtoul(argv[3], NULL, 16);
    } else {
        thread_num = strtoul(argv[3], NULL, 10);
    }

    if (thread_num > PERF_THREAD_NUM) {
        printf("thread_num can't larger than %d.\n", PERF_THREAD_NUM);
        goto out;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_init failed.\n");
        goto DMX_CLOSE_IN_FILE;
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

    for (i = 0; i < thread_num; i++) {
        ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0 + i, &ram_attrs, &g_ts_buf[i]);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_create_ts_buffer failed.\n");
            for (j = 0; j < i; j++) {
                hi_unf_dmx_detach_ts_port(PLAY_DMX_ID + j);
                hi_unf_dmx_destroy_ts_buffer(g_ts_buf[j]);
            }
            goto DMX_DEINIT;
        }

        ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID + i, HI_UNF_DMX_PORT_RAM_0 + i);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_attach_ts_port failed.\n");
            hi_unf_dmx_destroy_ts_buffer(g_ts_buf[i]);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_detach_ts_port(PLAY_DMX_ID + j);
                hi_unf_dmx_destroy_ts_buffer(g_ts_buf[j]);
            }
            goto DMX_DEINIT;
        }
    }

    ret = hi_unf_dmx_get_play_chan_default_attr(&play_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_play_chan_default_attr failed.\n");
        goto DMX_RAMPORT_DETACH_DESTROY;
    }

    play_attrs.chan_type = HI_UNF_DMX_CHAN_TYPE_VID;

    for (i = 0; i < thread_num; i++) {
        ret = hi_unf_dmx_create_play_chan(PLAY_DMX_ID + i, &play_attrs, &g_ply_handle[i]);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_create_play_chan failed.\n");
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_ply_handle[j]);
                hi_unf_dmx_destroy_play_chan(g_ply_handle[j]);
            }
            goto DMX_RAMPORT_DETACH_DESTROY;
        }

        ret = hi_unf_dmx_set_play_chan_pid(g_ply_handle[i], pid);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_set_play_chan_pid failed.\n");
            hi_unf_dmx_destroy_play_chan(g_ply_handle[i]);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_ply_handle[j]);
                hi_unf_dmx_destroy_play_chan(g_ply_handle[j]);
            }
            goto DMX_RAMPORT_DETACH_DESTROY;
        }

        hi_unf_dmx_reset_ts_buffer(g_ts_buf[i]);

        ret = hi_unf_dmx_open_play_chan(g_ply_handle[i]);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_open_play_chan failed.\n");
            hi_unf_dmx_destroy_play_chan(g_ply_handle[i]);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_ply_handle[j]);
                hi_unf_dmx_destroy_play_chan(g_ply_handle[j]);
            }
            goto DMX_RAMPORT_DETACH_DESTROY;
        }
    }

    for (i = 0; i < thread_num; i++) {

        g_stop_ts_put_thread = HI_FALSE;
        g_stop_ts_get_thread = HI_FALSE;

        g_perf_put_thread_info[i].cur_size = 0;
        g_perf_put_thread_info[i].data_rate = 0;
        g_perf_put_thread_info[i].handle = g_ts_buf[i];
        gettimeofday(&g_perf_put_thread_info[i].last_time, HI_NULL);
        pthread_create(&g_ts_put_thread[i], HI_NULL, (hi_void *)ts_tx_thread, &g_perf_put_thread_info[i]);

        g_perf_get_thread_info[i].cur_size = 0;
        g_perf_get_thread_info[i].data_rate = 0;
        g_perf_get_thread_info[i].handle = g_ply_handle[i];
        gettimeofday(&g_perf_get_thread_info[i].last_time, HI_NULL);
        pthread_create(&g_ts_get_thread[i], HI_NULL, (hi_void *)ts_rx_thread, &g_perf_get_thread_info[i]);
    }

    printf("please input 'q' to quit!\n");
    printf("please input 'p' display perf info.\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }

        if ('p' == input_cmd[0]) {
            for (i = 0; i < thread_num; i++) {
                printf("ts buffer(%x) input speed:%f Mbps;es channel(%x) ouput speed:%f Mbps.\n",
                    g_perf_put_thread_info[i].handle, ((hi_float)g_perf_put_thread_info[i].data_rate * 8) / 1024 / 1024,
                    g_perf_get_thread_info[i].handle, ((hi_float)g_perf_get_thread_info[i].data_rate * 8) / 1024 / 1024);
            }
        }
    }

    g_stop_ts_put_thread = HI_TRUE;
    g_stop_ts_get_thread = HI_TRUE;

    for (i = 0; i < thread_num; i++) {
        pthread_join(g_ts_put_thread[i], HI_NULL);
        pthread_join(g_ts_get_thread[i], HI_NULL);
    }

    for (i = 0; i < thread_num; i++) {
        hi_unf_dmx_close_play_chan(g_ply_handle[i]);
        hi_unf_dmx_destroy_play_chan(g_ply_handle[i]);
    }

DMX_RAMPORT_DETACH_DESTROY:
    for (i = 0; i < thread_num; i++) {
        hi_unf_dmx_detach_ts_port(PLAY_DMX_ID + i);

        hi_unf_dmx_destroy_ts_buffer(g_ts_buf[i]);
    }

DMX_DEINIT:
    hi_unf_dmx_deinit();

DMX_SYS_DEINIT:
    hi_unf_sys_deinit();

DMX_CLOSE_IN_FILE:
    if (g_ts_in_file != HI_NULL) {
        fclose(g_ts_in_file);
        g_ts_in_file = HI_NULL;
    }

out:
    return 0;
}

