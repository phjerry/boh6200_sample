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

#define unused(x) (x) = (x)

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

#define MAX_THREAD_NUM (32)
#define DMX_ID 0
#define DMX_INVALID_PID 0x1FFFU
#define ACQUIRE_INDEX_NUM 256

FILE               *g_ts_in_file = HI_NULL;
hi_handle          g_ts_buf;
static pthread_t   g_ts_put_thread;
static pthread_mutex_t g_ts_mutex;
static hi_bool     g_stop_ts_put_thread = HI_FALSE;
static hi_bool     g_stop_ts_get_thread = HI_FALSE;

typedef struct {
    FILE *out_file;
    FILE *index_file;
    pthread_t thread;
    pthread_t index_thread;
    hi_handle handle;
} rx_thread_info;

static rx_thread_info g_record_thread[MAX_THREAD_NUM];

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
        usleep(1000 * 10);

    }

    printf("ts send end!\n");

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_ts_buffer failed, ret(%x).\n", ret);
    }
    unused(args);
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

hi_void ts_record_index_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_unf_dmx_rec_index index_data[ACQUIRE_INDEX_NUM];
    hi_u32 received_num;
    rx_thread_info *thread_info = (rx_thread_info *)args;

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_rec_index(thread_info->handle, ACQUIRE_INDEX_NUM, &received_num, index_data, 1000);
        if (ret != HI_SUCCESS ) {
            usleep(10 * 1000);
            continue;
        }

        ret = fwrite(&index_data, 1, received_num * sizeof(hi_unf_dmx_rec_index), thread_info->index_file);
        if (ret != received_num * sizeof(hi_unf_dmx_rec_index)) {
            printf("fwrite failed, ret[0x%x], index_data(%u)\n", ret, (hi_u32)sizeof(index_data));
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
    hi_unf_dmx_rec_attr rec_attrs;
    hi_unf_dmx_rec_index_type index_type;
    hi_unf_vcodec_type vcodec_type;
    hi_char file_name[128];
    hi_u32 i;
    hi_u32 j;
    hi_u32 thread_num;

    if (argc != 6) {
        printf("\nuseage: savets ts_file pid pid_type thread_num\n");
        printf("\ncreate pid play and record tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    ts_file : source ts file.\n");
        printf("    pid     : the pid of video or audio.\n");
        printf("  index_type: none/vid/aud.\n");
        printf(" vcodec_type: mpeg2/mpeg4/avs/avs2/avs3/h263/h264/hevc.\n");
        printf("  thread_num: thread num.\n");
        return -1;
    }

    if (strstr(argv[2], "0x") || strstr(argv[2], "0X")) {
        pid = strtoul(argv[2], NULL, 16);
    } else {
        pid = strtoul(argv[2], NULL, 10);
    }

    if (strcmp(argv[3], "none") == 0) {
        index_type = HI_UNF_DMX_REC_INDEX_TYPE_NONE;
    } else if (strcmp(argv[3], "vid") == 0) {
        index_type = HI_UNF_DMX_REC_INDEX_TYPE_VIDEO;
    } else if (strcmp(argv[3], "aud") == 0) {
        index_type = HI_UNF_DMX_REC_INDEX_TYPE_AUDIO;
    } else {
        printf("invalid index type %s.\n", argv[3]);
        return -1;
    }

    if (strcmp(argv[4], "mpeg2") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_MPEG2;
    } else if (strcmp(argv[4], "mpeg4") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_MPEG4;
    } else if (strcmp(argv[4], "avs") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_AVS;
    } else if (strcmp(argv[4], "avs2") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_AVS2;
    } else if (strcmp(argv[4], "avs3") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_AVS3;
    } else if (strcmp(argv[4], "h263") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_H263;
    } else if (strcmp(argv[4], "h264") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_H264;
    } else if (strcmp(argv[4], "hevc") == 0) {
        vcodec_type = HI_UNF_VCODEC_TYPE_H265;
    } else {
        printf("invalid codec type, %s.\n", argv[4]);
        return -1;
    }

    if (strstr(argv[5], "0x") || strstr(argv[5], "0X")) {
        thread_num = strtoul(argv[5], NULL, 16);
    } else {
        thread_num = strtoul(argv[5], NULL, 10);
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
        sprintf(file_name, "%s_record_%u", argv[1], i);
        g_record_thread[i].out_file = fopen(file_name, "wb");
        if (g_record_thread[i].out_file == HI_NULL) {
            printf("open output(%s) ts file failed!\n", file_name);
            for (j = 0; j < i; j++) {
                fclose(g_record_thread[j].out_file);
                fclose(g_record_thread[j].index_file);
            }
            goto DMX_CLOSE_IN_FILE;
        }
        sprintf(file_name, "%s_index_%u", argv[1], i);
        g_record_thread[i].index_file = fopen(file_name, "wb");
        if (g_record_thread[i].index_file == HI_NULL) {
            printf("open file(%s) failed.\n", file_name);
            fclose(g_record_thread[i].out_file);
            for (j = 0; j < i; j++) {
                fclose(g_record_thread[j].out_file);
                fclose(g_record_thread[j].index_file);
            }
            goto DMX_CLOSE_IN_FILE;
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

    rec_attrs.dmx_id        = DMX_ID;
    rec_attrs.rec_buf_size   = 4 * 1024 * 1024;
    rec_attrs.rec_type       = HI_UNF_DMX_REC_TYPE_SELECT_PID;
    rec_attrs.descramed      = HI_TRUE;
    rec_attrs.index_type     = index_type;
    rec_attrs.index_src_pid  = pid;
    rec_attrs.video_codec_type   = vcodec_type;
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
            goto DMX_RAMPORT_DETACH;
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
            goto DMX_RAMPORT_DETACH;
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
            goto DMX_RAMPORT_DETACH;
        }
    }

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_put_thread = HI_FALSE;
    g_stop_ts_get_thread = HI_FALSE;

    for (i = 0; i < thread_num; i++) {
        pthread_create(&g_record_thread[i].thread, HI_NULL, (hi_void *)ts_record_rx_thread, (hi_void *)&g_record_thread[i]);
        pthread_create(&g_record_thread[i].index_thread, HI_NULL, (hi_void *)ts_record_index_thread, (hi_void *)&g_record_thread[i]);
    }

    usleep(10 * 1000);

    pthread_create(&g_ts_put_thread, HI_NULL, (hi_void *)ts_tx_thread, HI_NULL);


    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_ts_put_thread = HI_TRUE;
    pthread_join(g_ts_put_thread, HI_NULL);
    g_stop_ts_get_thread = HI_TRUE;
    for (i = 0; i < thread_num; i++) {
        pthread_join(g_record_thread[i].thread, HI_NULL);
        pthread_join(g_record_thread[i].index_thread, HI_NULL);
    }

    pthread_mutex_destroy(&g_ts_mutex);

    for (i = 0; i < thread_num; i++) {
        hi_unf_dmx_stop_rec_chan(g_record_thread[i].handle);
        hi_unf_dmx_del_rec_pid(g_record_thread[i].handle, pid_ch_handle[i]);
        hi_unf_dmx_destroy_rec_chan(g_record_thread[i].handle);
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
        fclose(g_record_thread[i].index_file);
    }
DMX_CLOSE_IN_FILE:
    if (g_ts_in_file != HI_NULL) {
        fclose(g_ts_in_file);
        g_ts_in_file = HI_NULL;
    }

out:
    return 0;
}


