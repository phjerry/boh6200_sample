/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample file used to test pid copy.
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

#define SAMPLE_GET_INPUTCMD(InputCmd) fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin)
#define HI_GET_INPUTCMD(InputCmd) fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin)
#define PERF_TIME_DIFF_US(a, b)  (((a.tv_sec) - (b.tv_sec))*1000000 + ((a.tv_usec) - (b.tv_usec)))
#define US_PER_SECOND 1000000

#define MAX_PID_COPY_CNT 34

static FILE *g_ts_in_file = HI_NULL;
static pthread_t g_ts_tx_thread;
static hi_handle g_ts_buf;

static hi_handle g_flt_handle[MAX_PID_COPY_CNT];

typedef struct {
    FILE *ts_out_file;
    hi_handle play_handle;
    pthread_t rx_thread;
    hi_u32 accq_total_cnt;
    hi_u32 accq_success_cnt;
    hi_u64 total_write_size;
    hi_u32 write_file_size;
    hi_char file_name[32];
} pid_copy_thread_info;

static pid_copy_thread_info g_thread_info[MAX_PID_COPY_CNT] = {
    [0 ... (MAX_PID_COPY_CNT - 1)] = {
            .ts_out_file = HI_NULL,
            .play_handle = HI_INVALID_HANDLE,
            .accq_total_cnt = 0,
            .accq_success_cnt = 0,
            .total_write_size = 0,
            .write_file_size = 0,
        }
};

static hi_bool g_stop_tx_thread = HI_FALSE;
static hi_bool g_stop_rx_thread = HI_FALSE;
static struct timeval g_tx_last_time;
static hi_u32 g_tx_rate = 0;

hi_void ts_tx_thread(hi_void *args)
{
    hi_u32 read_len;
    hi_s32 ret;
    dmx_ram_buffer buf;
    static struct timeval cur_time;
    static hi_u32 cur_byte = 0;

    printf("ts send start!\n");
    while (g_stop_tx_thread == HI_FALSE) {
        ret = hi_mpi_dmx_ram_get_buffer(g_ts_buf, 188 * 200, &buf, 1000);
        if (ret != HI_SUCCESS ) {
            printf("ramport get buffer failed, ret(%x)!\n", ret);
            usleep(1000 * 100);
            continue;
        }

        read_len = fread(buf.data, sizeof(hi_s8), buf.length, g_ts_in_file);
        if (read_len <= 0) {
            rewind(g_ts_in_file);
            continue;
        }

        cur_byte += read_len;
        gettimeofday(&cur_time, HI_NULL);
        if (PERF_TIME_DIFF_US(cur_time, g_tx_last_time) >= US_PER_SECOND) {
            g_tx_rate = cur_byte;
            cur_byte = 0;
            g_tx_last_time = cur_time;
        }

        ret = hi_mpi_dmx_ram_put_buffer(g_ts_buf, read_len, 0);
        if (ret != HI_SUCCESS ) {
            printf("call hi_mpi_dmx_ram_put_buffer failed, ret(%x).\n", ret);
        }
    }

    printf("ts send end!\n");
    ret = hi_mpi_dmx_ram_reset_buffer(g_ts_buf);
    if (ret != HI_SUCCESS ) {
        printf("call hi_mpi_dmx_ram_reset_buffer failed, ret(%x).\n", ret);
    }

    return;
}


hi_void ts_rx_thread(hi_void *args)
{
     hi_u32 ret = HI_FAILURE;
     hi_s32 i = 0;
     hi_u32 acquire_num = 10;
     hi_u32 acquired_num;
     dmx_buffer buf[10];
     pid_copy_thread_info *thread_info = (pid_copy_thread_info *)args;

     printf("%x receive start!\n", thread_info->play_handle);

     while (g_stop_rx_thread == HI_FALSE) {
        thread_info->accq_total_cnt++;
        ret = hi_mpi_dmx_play_acquire_buf(thread_info->play_handle, acquire_num, &acquired_num, buf, 5000);
        if(ret != HI_SUCCESS) {
            usleep(100 * 1000);
            continue;
        }
        thread_info->accq_success_cnt++;
        if (acquired_num > 10) {
            printf("acquired_num[0x%x] invalid, handle(%x)!\n", acquired_num, thread_info->play_handle);
            usleep(10 * 1000);
            continue;
        }

        for (i = 0; i < acquired_num; i++) {
            ret = fwrite(buf[i].data, 1, buf[i].length, thread_info->ts_out_file);
            if (buf[i].length != ret) {
                printf("fwrite failed, ret[0x%x], length[0x%x]\n", ret, buf[i].length);
            }
            thread_info->write_file_size += buf[i].length;
            thread_info->total_write_size += buf[i].length;
        }

        if (thread_info->write_file_size >= 60 * 1024 * 1024) {
            fclose(thread_info->ts_out_file);
            thread_info->ts_out_file = fopen(thread_info->file_name, "wb");
            thread_info->write_file_size = 0;
        }

        ret = hi_mpi_dmx_play_release_buf(thread_info->play_handle, acquired_num, buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_play_release_buf failed, ret(%x), handle(%x)!\n", ret, thread_info->play_handle);
        }
     }

     printf("%x receive over!\n", thread_info->play_handle);
     return;
}

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32 ret;
    hi_handle band_handle;
    hi_handle pid_chan_handle;
    hi_char input_cmd[32];
    hi_char out_file_name[32];
    hi_u32 pid = -1;
    dmx_ram_port_attr ram_attr;
    dmx_band_attr band_attr;
    dmx_play_attrs play_attr;
    dmx_port_attr port_attr;
    hi_u32 thread_num;
    hi_u32 i;
    hi_u32 j;
    dmx_play_type play_type = DMX_PLAY_TYPE_MAX;
    dmx_filter_attrs flt_attrs;

    if (argc != 5) {
        printf("\nuseage: ts_file pid_copy_cnt pid\n");
        printf("\ncreate pid_copy_cnt tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    ts_file      : source ts file.\n");
        printf("    pid_copy_cnt : pid copy channel cnt.\n");
        printf("    pid          : the pid of video or audio.\n");
        printf("    pid_tpye     : pid type.\n");
        return -1;
    }

    if (strstr(argv[2], "0x") || strstr(argv[2], "0X")) {
        thread_num = strtoul(argv[2], NULL ,16);
    } else {
        thread_num = strtoul(argv[2], NULL, 10);
    }

    if (strstr(argv[3], "0x") || strstr(argv[3], "0X")) {
        pid = strtoul(argv[3], NULL ,16);
    } else {
        pid = strtoul(argv[3], NULL, 10);
    }

    if (strcmp(argv[4], "sec") == 0) {
        play_type = DMX_PLAY_TYPE_SEC;
    } else if (strcmp(argv[4], "pes") == 0) {
        play_type = DMX_PLAY_TYPE_PES;
    } else if (strcmp(argv[4], "aud") == 0) {
        play_type = DMX_PLAY_TYPE_AUD;
    } else if (strcmp(argv[4], "vid") == 0) {
        play_type = DMX_PLAY_TYPE_VID;
    } else if (strcmp(argv[4], "ts") == 0) {
        play_type = DMX_PLAY_TYPE_TS;
    } else {
        printf("invalid pid type %s.\n", argv[4]);
        return -1;
    }

    if (thread_num > MAX_PID_COPY_CNT) {
        printf("pid_copy_cnt(%d) can't larger than %d.\n", thread_num, MAX_PID_COPY_CNT);
        return -1;
    }

    g_ts_in_file = fopen(argv[1],"rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open file %s failed.\n", argv[1]);
        return -1;
    }

    for (i = 0; i < thread_num; i++) {
        sprintf(out_file_name, "%s_%u", argv[1], i);
        g_thread_info[i].ts_out_file = fopen(out_file_name, "wb");
        if (g_thread_info[i].ts_out_file == HI_NULL) {
            for (j = 0; j < i; j++) {
                fclose(g_thread_info[j].ts_out_file);
            }
            printf("open file %s failed.\n", out_file_name);
            ret = HI_FAILURE;
            goto CLOSE_TS_IN_FILE;
        }
        memcpy(g_thread_info[i].file_name, out_file_name, 32);
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("hi_unf_sys_initfailed, ret(%x).\n", ret);
        goto CLOSE_TS_OUT_FILE;
    }

    ret = hi_mpi_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("hi_mpi_dmx_init faield, ret(%x).\n", ret);
        goto SYS_DEINIT;
    }

    ret = hi_mpi_dmx_ram_get_port_default_attrs(&ram_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_mpi_dmx_ram_get_port_default_attrs failed, ret(%x).\n", ret);
        goto DMX_DEINIT;
    }

    ret = hi_mpi_dmx_ram_open_port(DMX_RAM_PORT_0, &ram_attr, &g_ts_buf);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_mpi_dmx_ram_open_port failed, ret(%x).\n", ret);
        goto DMX_DEINIT;
    }

    ret = hi_mpi_dmx_ram_get_port_attrs(g_ts_buf, &port_attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_get_port_attrs failed, ret(%x).\n", ret);
        goto DMX_ClOSE_RAM;
    }

    ret = hi_mpi_dmx_ram_set_port_attrs(g_ts_buf, &port_attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_ram_set_port_attrs failed, ret(%x).\n", ret);
        goto DMX_ClOSE_RAM;
    }

    band_attr.band_attr = 0;
    ret = hi_mpi_dmx_band_open(DMX_BAND_0, &band_attr, &band_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_band_open failed, ret(%x).\n", ret);
        goto DMX_ClOSE_RAM;
    }

    ret = hi_mpi_dmx_band_attach_port(band_handle, DMX_RAM_PORT_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_band_attach_port failed, ret(%x).\n", ret);
        goto DMX_ClOSE_BAND;
    }


    ret = hi_mpi_dmx_pid_ch_create(band_handle, pid, &pid_chan_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_pid_ch_create failed, ret(%x).\n", ret);
        goto DMX_BAND_DETACH;
    }

    ret = hi_mpi_dmx_play_get_default_attrs(&play_attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_play_get_default_attrs failed, ret(%x).\n", ret);
        goto DMX_PIDCH_DESTROY;
    }

    play_attr.type = play_type;

    for (i = 0; i < thread_num; i++) {
        ret = hi_mpi_dmx_play_create(&play_attr, &g_thread_info[i].play_handle);
        if (ret != HI_SUCCESS) {
            for (j = 0; j < i; j++) {
                hi_mpi_dmx_pid_ch_destroy(g_thread_info[j].play_handle);
            }
            printf("call hi_mpi_dmx_play_create failed, ret(%x), i(%u).\n", ret, i);
            goto DMX_PIDCH_DESTROY;
        }
    }

    for (i = 0; i < thread_num; i++) {
        ret = hi_mpi_dmx_play_attach_pid_ch(g_thread_info[i].play_handle, pid_chan_handle);
        if (ret != HI_SUCCESS) {
            for (j = 0; j < i; j++) {
                if (play_attr.type == DMX_PLAY_TYPE_SEC || play_attr.type == DMX_PLAY_TYPE_PES) {
                    hi_mpi_dmx_play_del_filter(g_thread_info[j].play_handle, g_flt_handle[j]);
                    hi_mpi_dmx_play_destroy_filter(g_flt_handle[j]);
                }
                hi_mpi_dmx_play_detach_pid_ch(g_thread_info[j].play_handle);
            }
            printf("call hi_mpi_dmx_play_attach_pid_ch failed, ret(%x), i(%u), play_handle(%x), pid_chan_handle(%x).\n",
                ret, i, g_thread_info[i].play_handle, pid_chan_handle);
            goto DMX_PLY_DESTROY;
        }

        if (play_attr.type == DMX_PLAY_TYPE_SEC || play_attr.type == DMX_PLAY_TYPE_PES) {
            flt_attrs.depth = 1;
            memset(flt_attrs.mask, 0x0, DMX_FILTER_MAX_DEPTH);
            memset(flt_attrs.match, 0x0, DMX_FILTER_MAX_DEPTH);
            if (play_attr.type == DMX_PLAY_TYPE_PES) {
                memset(flt_attrs.negate, 0x1, DMX_FILTER_MAX_DEPTH);
            } else {
                memset(flt_attrs.negate, 0x0, DMX_FILTER_MAX_DEPTH);
                flt_attrs.match[0] = 2; /* only support pmt table id */
            }

            hi_mpi_dmx_play_create_filter(&flt_attrs, &g_flt_handle[i]);
            hi_mpi_dmx_play_add_filter(g_thread_info[i].play_handle, g_flt_handle[i]);
        }
    }

    for (i = 0; i < thread_num; i++) {
        ret = hi_mpi_dmx_play_open(g_thread_info[i].play_handle);
        if (ret != HI_SUCCESS) {
            for (j = 0; j < i; j++) {
                hi_mpi_dmx_play_close(g_thread_info[j].play_handle);
            }
            printf("call hi_mpi_dmx_play_open failed, ret(%x), i(%u), play_handle(%x).\n", ret, i, g_thread_info[i].play_handle);
            goto DMX_PLY_DETACH;
        }
    }

    g_stop_tx_thread = HI_FALSE;
    g_stop_rx_thread = HI_FALSE;

    gettimeofday(&g_tx_last_time, HI_NULL);
    pthread_create(&g_ts_tx_thread, HI_NULL, (hi_void *)ts_tx_thread, HI_NULL);

    for (i = 0; i < thread_num; i++) {
        pthread_create(&g_thread_info[i].rx_thread, HI_NULL, (hi_void *)ts_rx_thread, (hi_void *)&g_thread_info[i]);
    }

    printf("please input 'q' to quit!\n");
    printf("please input 'p' to display accquire buf info.\n");

    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
           printf("prepare to quit!\n");
           break;
        } else if ('p' == input_cmd[0]) {
            printf("ram port(%x) send rate %u Mbps.\n", g_ts_buf, g_tx_rate * 8 / 1024 / 1024);
            for (i = 0; i < thread_num; i++) {
                printf("play fct(%x) total acquire %u, success %u, write file size %u MB, total write size %llu MB.\n",
                    g_thread_info[i].play_handle, g_thread_info[i].accq_total_cnt, g_thread_info[i].accq_success_cnt,
                    g_thread_info[i].write_file_size / 1024 / 1024, g_thread_info[i].total_write_size / 1024 / 1024);
            }
        }
    }

    printf("begin stop!\n");
    g_stop_tx_thread = HI_TRUE;
    g_stop_rx_thread = HI_TRUE;

    pthread_join(g_ts_tx_thread, HI_NULL);

    for (i = 0; i < thread_num; i++) {
        pthread_join(g_thread_info[i].rx_thread, HI_NULL);
    }

    for (i = 0; i < thread_num; i++) {
        hi_mpi_dmx_play_close(g_thread_info[i].play_handle);
    }

DMX_PLY_DETACH:
    for (i = 0; i < thread_num; i++) {
        if (play_type == DMX_PLAY_TYPE_SEC || play_attr.type == DMX_PLAY_TYPE_PES) {
            hi_mpi_dmx_play_del_filter(g_thread_info[i].play_handle, g_flt_handle[i]);
            hi_mpi_dmx_play_destroy_filter(g_flt_handle[i]);
        }
        hi_mpi_dmx_play_detach_pid_ch(g_thread_info[i].play_handle);
    }

DMX_PLY_DESTROY:
    for (i = 0; i < thread_num; i++) {
        hi_mpi_dmx_play_destroy(g_thread_info[i].play_handle);
    }

DMX_PIDCH_DESTROY:
    hi_mpi_dmx_pid_ch_destroy(pid_chan_handle);

DMX_BAND_DETACH:
    hi_mpi_dmx_band_detach_port(band_handle);

DMX_ClOSE_BAND:
    hi_mpi_dmx_band_close(band_handle);

DMX_ClOSE_RAM:
    hi_mpi_dmx_ram_close_port(g_ts_buf);

DMX_DEINIT:
    hi_mpi_dmx_de_init();
SYS_DEINIT:
    hi_unf_sys_deinit();
CLOSE_TS_OUT_FILE:
    for (i = 0; i < thread_num; i++) {
        fclose(g_thread_info[i].ts_out_file);
    }
CLOSE_TS_IN_FILE:
    fclose(g_ts_in_file);
    return ret;
}


