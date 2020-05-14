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
#include "hi_unf_tsio.h"

#define PLAY_DMX_ID 0x0

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

static FILE *g_ts_in_file = HI_NULL;
static pthread_t g_ts_put_thread;

static hi_bool g_stop_ts_put_thread;
static hi_bool g_stop_ts_get_thread;

typedef struct {
    FILE *ts_out_file;
    pthread_t g_ts_get_thread;
    hi_handle dmx_play_handle;
} thread_info;

#define MAX_THREAD_NUM 32

static thread_info g_thread_info[MAX_THREAD_NUM];

hi_void ts_tx_thread(hi_void *args)
{
    hi_u32                readlen;
    hi_s32                ret;
    hi_unf_tsio_buffer        buf;
    hi_handle tsio_buffer_handle = *(hi_handle *)args;
    hi_u8 temp_buffer[188 * 200];
    hi_bool read_new_data = HI_TRUE;

    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        if (read_new_data == HI_TRUE) {
            readlen = fread(temp_buffer, sizeof(hi_s8), 188 * 200, g_ts_in_file);
            if (readlen <= 0) {
                printf("read ts file end and rewind!\n");
                rewind(g_ts_in_file);
                continue;
            }
        }

        ret = hi_unf_tsio_get_buffer(tsio_buffer_handle, readlen, &buf, 1000);
        if (ret != HI_SUCCESS) {
            printf("tsio ramport get buffer failed, ret(%x)!\n", ret);
            usleep(10 * 1000);
            read_new_data = HI_FALSE;
            continue;
        }

        memcpy(buf.data, temp_buffer, readlen);

        ret = hi_unf_tsio_put_buffer(tsio_buffer_handle, &buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_tsbuffer failed, ret(%x).\n", ret);
        }
        read_new_data = HI_TRUE;
    }
    printf("ts send end!\n");
    ret = hi_unf_tsio_reset_buffer(tsio_buffer_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_tsbuffer failed, ret(%x).\n", ret);
    }

    return;
}

hi_void ts_rx_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_unf_es_buf es_buf;
    thread_info *info = (thread_info *)args;

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_es(info->dmx_play_handle, &es_buf);
        if (ret != HI_SUCCESS) {
            usleep(10 * 1000);
            continue;
        }

        ret = fwrite(es_buf.buf, 1, es_buf.buf_len, info->ts_out_file);
        if (ret != es_buf.buf_len) {
            printf("fwrite failed, ret[0x%x], es_buf.buf_len[0x%x]\n", ret, es_buf.buf_len);
        }

        ret = hi_unf_dmx_release_es(info->dmx_play_handle, &es_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_release_es failed!\n");
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
    hi_unf_tsio_config tsio_config;
    hi_unf_tsio_buffer_attr tsio_buffer_attr;
    hi_handle tsio_buffer_handle;
    hi_unf_tsio_attr tsio_attr;
    hi_handle tsio_handle;
    hi_unf_dmx_chan_attr play_attrs;
    hi_u32 thread_num;
    hi_u32 i;
    hi_u32 j;
    hi_char temp_buf[64];
    hi_u32 buffer_size;

    if (argc != 6) {
        printf("\nuseage: savets ts_file ts1_file pid thread_num buf_size\n");
        printf("\ncreate an pid tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    source ts_file : source ts file.\n");
        printf("    dest ts file : dest ts file.\n");
        printf("    pid     : the pid of video or audio.\n");
        printf("  thread_num: dmx get data thread num.\n");
        printf("    buf_size: dmx output buffer size.\n");
        return -1;
    }

    if (strstr(argv[3], "0x") || strstr(argv[3], "0X")) {
        pid = strtoul(argv[3], NULL, 16);
    } else {
        pid = strtoul(argv[3], NULL, 10);
    }

    if (strstr(argv[4], "0x") || strstr(argv[4], "0X")) {
        thread_num = strtoul(argv[4], NULL, 16);
    } else {
        thread_num = strtoul(argv[4], NULL, 10);
    }

    if (strstr(argv[5], "0x") || strstr(argv[5], "0X")) {
        buffer_size = strtoul(argv[5], NULL, 16);
    } else {
        buffer_size = strtoul(argv[5], NULL, 10);
    }

    if (thread_num > MAX_THREAD_NUM) {
        printf("thread_num beyond %d, valid thread_num 1~%d.\n", MAX_THREAD_NUM, MAX_THREAD_NUM);
        return -1;
    }

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open ts in file failed!\n");
        goto out;
    }

    for (i = 0; i < thread_num; i++) {
        sprintf(temp_buf, "%s_%u", argv[2], i);
        g_thread_info[i].ts_out_file = fopen(temp_buf, "wb");
        if (g_thread_info[i].ts_out_file == HI_NULL) {
            printf("open ts out file %s failed!\n", temp_buf);
            for (j = 0; j < i; j++) {
                fclose(g_thread_info[j].ts_out_file);
                g_thread_info[j].ts_out_file = HI_NULL;
            }
            goto DMX_CLOSE_IN_FILE;
        }
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

    tsio_config.band_width = HI_UNF_TSIO_BW_400M;
    tsio_config.stuff_sid = 0x3f;
    tsio_config.sync_thres = 8;
    ret = hi_unf_tsio_init(&tsio_config);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_tsio_init failed, ret(%x).\n", ret);
        goto DMX_DEINIT;
    }

    tsio_buffer_attr.stream_type = HI_UNF_TSIO_RAM_TS;
    tsio_buffer_attr.buf_size = 2 * 1024 * 1024;
    tsio_buffer_attr.max_data_rate = 0;
    ret = hi_unf_tsio_create_buffer(HI_UNF_TSIO_ANY_RAM_PORT, &tsio_buffer_attr, &tsio_buffer_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_tsio_create_buffer failed, ret(%x).\n", ret);
        goto TSIO_DEINIT;
    }

    tsio_attr.out_put.out_put_type = HI_UNF_TSIO_OUTPUT_DMX;
    tsio_attr.out_put.param.dmx.port = HI_UNF_DMX_PORT_TSIO_0;
    tsio_attr.sid = 0;

    ret = hi_unf_tsio_create(&tsio_attr, &tsio_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_tsio_create failed, ret(%x).\n", ret);
        goto TSIO_DESTROY_BUFFER;
    }

    ret = hi_unf_tsio_attach_ram_input(tsio_handle, tsio_buffer_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_tsio_attach_ram_input failed, ret(%x).\n", ret);
        goto TSIO_DESTROY;
    }

    ret = hi_unf_tsio_add_pid(tsio_handle, pid);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_tsio_add_pid faild, ret(%x).\n", ret);
        goto TSIO_DETACH;
    }

    ret = hi_unf_tsio_start(tsio_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_tsio_start failed, ret(%x).\n", ret);
        goto TSIO_DELPID;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_TSIO_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_attach_ts_port failed, ret(%x).\n", ret);
        goto TSIO_STOP;
    }

    ret = hi_unf_dmx_get_play_chan_default_attr(&play_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_play_chan_default_attr failed, ret(%x).\n", ret);
        goto DMX_DETACH;
    }

    play_attrs.chan_type = HI_UNF_DMX_CHAN_TYPE_VID;
    play_attrs.buffer_size = buffer_size;

    for (i = 0; i < thread_num; i++) {
        ret = hi_unf_dmx_create_play_chan(PLAY_DMX_ID, &play_attrs, &g_thread_info[i].dmx_play_handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_create_play_chan failed, ret(%x).\n", ret);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_thread_info[j].dmx_play_handle);
                hi_unf_dmx_destroy_play_chan(g_thread_info[j].dmx_play_handle);
            }
            goto DMX_DETACH;
        }

        ret = hi_unf_dmx_set_play_chan_pid(g_thread_info[i].dmx_play_handle, pid);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_set_play_chan_pid failed, ret(%x).\n", ret);
            hi_unf_dmx_destroy_play_chan(g_thread_info[i].dmx_play_handle);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_thread_info[j].dmx_play_handle);
                hi_unf_dmx_destroy_play_chan(g_thread_info[j].dmx_play_handle);
            }
            goto DMX_DETACH;
        }

        ret = hi_unf_dmx_open_play_chan(g_thread_info[i].dmx_play_handle);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_open_play_chan failed.\n");
            hi_unf_dmx_destroy_play_chan(g_thread_info[i].dmx_play_handle);
            for (j = 0; j < i; j++) {
                hi_unf_dmx_close_play_chan(g_thread_info[j].dmx_play_handle);
                hi_unf_dmx_destroy_play_chan(g_thread_info[j].dmx_play_handle);
            }
            goto DMX_DETACH;
        }
    }

    g_stop_ts_put_thread = HI_FALSE;
    g_stop_ts_get_thread = HI_FALSE;

    pthread_create(&g_ts_put_thread, HI_NULL, (hi_void *)ts_tx_thread, &tsio_buffer_handle);

    for (i = 0; i < thread_num; i++) {
        pthread_create(&g_thread_info[i].g_ts_get_thread, HI_NULL, (hi_void *)ts_rx_thread, &g_thread_info[i]);
    }


    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_ts_get_thread = HI_TRUE;
    for (i = 0; i < thread_num; i++) {
        pthread_join(g_thread_info[i].g_ts_get_thread, HI_NULL);
    }

    g_stop_ts_put_thread = HI_TRUE;
    pthread_join(g_ts_put_thread, HI_NULL);

    for (i = 0; i < thread_num; i++) {
        hi_unf_dmx_close_play_chan(g_thread_info[i].dmx_play_handle);
        hi_unf_dmx_destroy_play_chan(g_thread_info[i].dmx_play_handle);
    }

DMX_DETACH:
    hi_unf_dmx_detach_ts_port(PLAY_DMX_ID);

TSIO_STOP:
    hi_unf_tsio_stop(tsio_handle);

TSIO_DELPID:
    hi_unf_tsio_del_all_pid(tsio_handle);

TSIO_DETACH:
    hi_unf_tsio_detach_input(tsio_handle);

TSIO_DESTROY:
    hi_unf_tsio_destroy(tsio_handle);

TSIO_DESTROY_BUFFER:
    hi_unf_tsio_destroy_buffer(tsio_buffer_handle);

TSIO_DEINIT:
    hi_unf_tsio_deinit();

DMX_DEINIT:
    hi_unf_dmx_deinit();

DMX_SYS_DEINIT:
    hi_unf_sys_deinit();

DMX_CLOSE_OUT_FILE:
    for (i = 0; i < thread_num; i++) {
        fclose(g_thread_info[i].ts_out_file);
        g_thread_info[i].ts_out_file = HI_NULL;
    }

DMX_CLOSE_IN_FILE:
    if (g_ts_in_file != HI_NULL) {
        fclose(g_ts_in_file);
        g_ts_in_file = HI_NULL;
    }

out:
    return 0;
}
