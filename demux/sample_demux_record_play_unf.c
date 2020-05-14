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
#include "hi_unf_demux.h"

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

#define MAX_THREAD_NUM (32)
#define DMX_ID 0
#define DMX_INVALID_PID 0x1FFFU

FILE               *g_ts_in_file = HI_NULL;
hi_handle          g_ts_buf;
static pthread_t   g_ts_put_thread;
static pthread_mutex_t g_ts_mutex;
static hi_bool     g_stop_ts_put_thread = HI_FALSE;
static hi_bool     g_stop_ts_get_thread = HI_FALSE;

typedef struct {
    FILE *out_file;
    pthread_t thread;
    hi_handle handle;
} rx_thread_info;

static rx_thread_info g_play_thread[MAX_THREAD_NUM];
static rx_thread_info g_record_thread[MAX_THREAD_NUM];
static hi_unf_dmx_chan_type g_play_type = HI_UNF_DMX_CHAN_TYPE_MAX;
static hi_handle g_flt_handle[MAX_THREAD_NUM];

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
            printf("ramport get buffer failed, ret(%x)!\n", ret);
            continue;
        }

        readlen = fread(buf.data, sizeof(hi_s8), buf.size, g_ts_in_file);
        if (readlen <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_in_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_unf_dmx_put_ts_buffer(g_ts_buf, buf.size, 0);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_ts_buffer failed, ret(%x).\n", ret);
        }

        pthread_mutex_unlock(&g_ts_mutex);

    }

    printf("ts send end!\n");

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_ts_buffer failed, ret(%x).\n", ret);
    }

    return;
}

hi_void ts_play_rx_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_s32 i = 0;
    hi_u32 acquire_num = 10;
    hi_u32 acquired_num;
    hi_unf_dmx_data st_buf[10];
    hi_unf_es_buf es_buf;
    rx_thread_info *thread_info = (rx_thread_info *)args;

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        if (g_play_type == HI_UNF_DMX_CHAN_TYPE_AUD || g_play_type == HI_UNF_DMX_CHAN_TYPE_VID) {
            ret = hi_unf_dmx_acquire_es(thread_info->handle, &es_buf);
            if (ret != HI_SUCCESS) {
                usleep(10 * 1000);
                continue;
            }

            ret = fwrite(es_buf.buf, 1, es_buf.buf_len, thread_info->out_file);
            if (ret != es_buf.buf_len) {
                printf("fwrite failed, ret[0x%x], es_buf.buf_len[0x%x]\n", ret, es_buf.buf_len);
            }

            ret = hi_unf_dmx_release_es(thread_info->handle, &es_buf);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_dmx_release_es failed, handle(%x).!\n", thread_info->handle);
            }
        } else {
            ret = hi_unf_dmx_acquire_buffer(thread_info->handle, acquire_num, &acquired_num, st_buf, 5000);
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
                ret = fwrite(st_buf[i].data, 1, st_buf[i].size, thread_info->out_file);
                if (st_buf[i].size != ret) {
                    printf("fwrite failed, ret[0x%x], st_buf[i].length[0x%x]\n", ret, st_buf[i].size);
                }
            }

            ret = hi_unf_dmx_release_buffer(thread_info->handle, acquired_num, st_buf);
            if (HI_SUCCESS != ret) {
                printf("call hi_unf_dmx_release_buffer failed, ret(%x), handle(%x), acquired_num(%u)!\n", ret, thread_info->handle, acquired_num);
            }
        }
    }

    printf("receive over!\n");

    return;
}

hi_void ts_record_rx_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_unf_dmx_rec_data st_buf;
    rx_thread_info *thread_info = (rx_thread_info *)args;

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_rec_data(thread_info->handle, &st_buf, 1000);
        if (ret != HI_SUCCESS ) {
            usleep(10 * 1000);
            continue;
        }

        ret = fwrite(st_buf.data_addr, 1, st_buf.len, thread_info->out_file);
        if (st_buf.len != ret) {
            printf("fwrite failed, ret[0x%x], st_buf.length[0x%x]\n", ret, st_buf.len);
        }

        ret = hi_unf_dmx_release_rec_data(thread_info->handle, &st_buf);
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
    hi_handle  pid_ch_handle[MAX_THREAD_NUM];
    hi_char input_cmd[32];
    hi_s32 pid = -1;
    hi_unf_dmx_ts_buffer_attr ram_attrs;
    hi_unf_dmx_chan_attr play_attrs;
    hi_unf_dmx_rec_attr rec_attrs;
    hi_char file_name[128];
    hi_u32 i;
    hi_u32 j;
    hi_u32 thread_num;
    hi_unf_dmx_filter_attr flt_attrs = {0};

    if (argc != 5) {
        printf("\nuseage: savets ts_file pid pid_type thread_num\n");
        printf("\ncreate pid play and record tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    ts_file : source ts file.\n");
        printf("    pid     : the pid of video or audio.\n");
        printf("    pid_type: sec/pes/aud/vid.\n");
        printf("  thread_num: thread num.\n");
        return -1;
    }

    if (strstr(argv[2], "0x") || strstr(argv[2], "0X")) {
        pid = strtoul(argv[2], NULL, 16);
    } else {
        pid = strtoul(argv[2], NULL, 10);
    }

    if (strcmp(argv[3], "sec") == 0) {
        g_play_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    } else if (strcmp(argv[3], "pes") == 0) {
        g_play_type = HI_UNF_DMX_CHAN_TYPE_PES;
    } else if (strcmp(argv[3], "aud") == 0) {
        g_play_type = HI_UNF_DMX_CHAN_TYPE_AUD;
    } else if (strcmp(argv[3], "vid") == 0) {
        g_play_type = HI_UNF_DMX_CHAN_TYPE_VID;
    } else {
        printf("invalid pid type %s.\n", argv[3]);
        return -1;
    }

    if (strstr(argv[4], "0x") || strstr(argv[4], "0X")) {
        thread_num = strtoul(argv[4], NULL, 16);
    } else {
        thread_num = strtoul(argv[4], NULL, 10);
    }

    if (thread_num > MAX_THREAD_NUM) {
        printf("invalid thread num %u.\n", thread_num);
        return -1;
    }

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open input ts file failed!\n");
        goto out;
    }

    for (i = 0; i < thread_num; i++) {
        sprintf(file_name, "%s_play_%u", argv[1], i);
        g_play_thread[i].out_file = fopen(file_name, "wb");
        if (g_play_thread[i].out_file == HI_NULL) {
            printf("open output(%s) ts file failed!\n", file_name);
            for (j = 0; j < i; j++) {
                fclose(g_play_thread[j].out_file);
            }
            goto DMX_CLOSE_IN_FILE;
        }
    }

    for (i = 0; i < thread_num; i++) {
        sprintf(file_name, "%s_record_%u", argv[1], i);
        g_record_thread[i].out_file = fopen(file_name, "wb");
        if (g_record_thread[i].out_file == HI_NULL) {
            printf("open ouput(%s) ts file failed!\n", file_name);
            for (j = 0; j < i; j++) {
                fclose(g_record_thread[j].out_file);
            }
            goto DMX_CLOSE_PLAY_FILE;
        }
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_init failed.\n");
        goto DMX_CLOSE_REC_FILE;
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

    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ram_attrs, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto DMX_DESTROY_TSBUFFER;
    }

    ret = hi_unf_dmx_get_play_chan_default_attr(&play_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_play_chan_default_attr failed.\n");
        goto DMX_RAMPORT_DETACH;
    }

    play_attrs.chan_type = g_play_type;

    for (i = 0; i < thread_num; i++) {
        ret = hi_unf_dmx_create_play_chan(DMX_ID, &play_attrs, &g_play_thread[i].handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_create_play_chan failed, ret(%x).\n", ret);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_play_thread[j].handle);
                hi_unf_dmx_destroy_play_chan(g_play_thread[j].handle);
            }
            goto DMX_RAMPORT_DETACH;
        }

        ret = hi_unf_dmx_set_play_chan_pid(g_play_thread[i].handle, pid);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_set_play_chan_pid failed, ret(%x).\n", ret);
            hi_unf_dmx_destroy_play_chan(g_play_thread[i].handle);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_play_thread[j].handle);
                hi_unf_dmx_destroy_play_chan(g_play_thread[j].handle);
            }
            goto DMX_RAMPORT_DETACH;
        }

        if (play_attrs.chan_type == HI_UNF_DMX_CHAN_TYPE_SEC || play_attrs.chan_type == HI_UNF_DMX_CHAN_TYPE_PES) {
            flt_attrs.filter_depth = 1;
            memset(flt_attrs.mask, 0x0, DMX_FILTER_MAX_DEPTH);
            memset(flt_attrs.match, 0x0, DMX_FILTER_MAX_DEPTH);
            if (play_attrs.chan_type == HI_UNF_DMX_CHAN_TYPE_PES) {
                memset(flt_attrs.negate, 0x1, DMX_FILTER_MAX_DEPTH);
            } else {
                memset(flt_attrs.negate, 0x0, DMX_FILTER_MAX_DEPTH);
                flt_attrs.match[0] = 2; /* only support pmt table id */
            }

            hi_unf_dmx_create_filter(DMX_ID, &flt_attrs, &g_flt_handle[i]);
            hi_unf_dmx_attach_filter(g_flt_handle[i], g_play_thread[i].handle);
        }

        ret = hi_unf_dmx_open_play_chan(g_play_thread[i].handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_open_play_chan failed, ret(%x).\n", ret);
            hi_unf_dmx_destroy_play_chan(g_play_thread[i].handle);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_play_thread[j].handle);
                hi_unf_dmx_destroy_play_chan(g_play_thread[j].handle);
            }
            goto DMX_RAMPORT_DETACH;
        }
    }

    rec_attrs.dmx_id        = DMX_ID;
    rec_attrs.rec_buf_size   = 4 * 1024 * 1024;
    rec_attrs.rec_type       = HI_UNF_DMX_REC_TYPE_SELECT_PID;
    rec_attrs.descramed      = HI_TRUE;
    rec_attrs.index_type     = HI_UNF_DMX_REC_INDEX_TYPE_NONE;
    rec_attrs.index_src_pid  = DMX_INVALID_PID;
    rec_attrs.video_codec_type   = HI_UNF_VCODEC_TYPE_MPEG2;
    rec_attrs.secure_mode    = HI_UNF_DMX_SECURE_MODE_REE;
    rec_attrs.ts_packet_type  = HI_UNF_DMX_TS_PACKET_188;

    for (i = 0; i < thread_num; i++) {
        ret = hi_unf_dmx_create_rec_chan(&rec_attrs, &g_record_thread[i].handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_create_rec_chan failed, ret(%x).\n", ret);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_stop_rec_chan(g_record_thread[j].handle);
                hi_unf_dmx_del_rec_pid(g_record_thread[j].handle, pid_ch_handle[j]);
                hi_unf_dmx_destroy_rec_chan(g_record_thread[j].handle);
            }
            goto DMX_PLY_EXIT;
        }

        ret = hi_unf_dmx_add_rec_pid(g_record_thread[i].handle, pid, &pid_ch_handle[i]);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_add_rec_pid failed, ret(%x).\n", ret);
            hi_unf_dmx_destroy_rec_chan(g_record_thread[i].handle);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_stop_rec_chan(g_record_thread[j].handle);
                hi_unf_dmx_del_rec_pid(g_record_thread[j].handle, pid_ch_handle[j]);
                hi_unf_dmx_destroy_rec_chan(g_record_thread[j].handle);
            }
            goto DMX_PLY_EXIT;
        }

        ret = hi_unf_dmx_start_rec_chan(g_record_thread[i].handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_start_rec_chan failed, ret(%x).\n", ret);
            hi_unf_dmx_del_rec_pid(g_record_thread[i].handle, pid_ch_handle[i]);
            hi_unf_dmx_destroy_rec_chan(g_record_thread[i].handle);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_stop_rec_chan(g_record_thread[j].handle);
                hi_unf_dmx_del_rec_pid(g_record_thread[j].handle, pid_ch_handle[j]);
                hi_unf_dmx_destroy_rec_chan(g_record_thread[j].handle);
            }
            goto DMX_PLY_EXIT;
        }
    }

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_put_thread = HI_FALSE;
    g_stop_ts_get_thread = HI_FALSE;

    pthread_create(&g_ts_put_thread, HI_NULL, (hi_void *)ts_tx_thread, HI_NULL);

    for (i = 0; i < thread_num; i++) {
        pthread_create(&g_play_thread[i].thread, HI_NULL, (hi_void *)ts_play_rx_thread, (hi_void *)&g_play_thread[i]);
        pthread_create(&g_record_thread[i].thread, HI_NULL, (hi_void *)ts_record_rx_thread, (hi_void *)&g_record_thread[i]);
    }

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
    for (i = 0; i < thread_num; i++) {
        pthread_join(g_play_thread[i].thread, HI_NULL);
        pthread_join(g_record_thread[i].thread, HI_NULL);
    }

    pthread_mutex_destroy(&g_ts_mutex);

    for (i = 0; i < thread_num; i++) {
        hi_unf_dmx_stop_rec_chan(g_record_thread[i].handle);
        hi_unf_dmx_del_rec_pid(g_record_thread[i].handle, pid_ch_handle[i]);
        hi_unf_dmx_destroy_rec_chan(g_record_thread[i].handle);
    }

DMX_PLY_EXIT:
    for (i = 0; i < thread_num; i++) {
        hi_unf_dmx_close_play_chan(g_play_thread[i].handle);
        hi_unf_dmx_detach_filter(g_flt_handle[i], g_play_thread[i].handle);
        hi_unf_dmx_destroy_filter(g_flt_handle[i]);
        hi_unf_dmx_destroy_play_chan(g_play_thread[i].handle);
    }

DMX_RAMPORT_DETACH:
    hi_unf_dmx_detach_ts_port(DMX_ID);

DMX_DESTROY_TSBUFFER:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);

DMX_DEINIT:
    hi_unf_dmx_deinit();

DMX_SYS_DEINIT:
    hi_unf_sys_deinit();

DMX_CLOSE_REC_FILE:
    for (i = 0; i < thread_num; i++) {
        fclose(g_record_thread[i].out_file);
    }

DMX_CLOSE_PLAY_FILE:
    for (i = 0; i < thread_num; i++) {
        fclose(g_play_thread[i].out_file);
    }
DMX_CLOSE_IN_FILE:
    if (g_ts_in_file != HI_NULL) {
        fclose(g_ts_in_file);
        g_ts_in_file = HI_NULL;
    }

out:
    return 0;
}


