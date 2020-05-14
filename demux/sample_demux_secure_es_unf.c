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
#include "hi_unf_ssm.h"
#include "hi_adp_search.h"

#define PLAY_DMX_ID 0x0

#define TS_PKT_LEN 188
#define TS_PKT_NUM 200
#define DEFAULT_TS_BUF_SIZE 0x200000
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
pmt_compact_tbl    *g_prog_table = HI_NULL;

hi_handle          g_ts_buf;
hi_handle          g_ply_handle;


hi_void ts_tx_thread(hi_void *args)
{
    hi_u32                readlen;
    hi_s32                ret;
    hi_unf_stream_buf     buf;
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);

        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, TS_PKT_LEN * TS_PKT_NUM, &buf, DEFAULT_TIME_OUT);
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
        usleep(DEFAULT_TIME_OUT * DEFAULT_TIME_OUT_LOOP);
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
    hi_u32 ret;
    hi_unf_es_buf es_buf;

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_es(g_ply_handle, &es_buf);
        if (ret != HI_SUCCESS) {
            usleep(DEFAULT_TIME_OUT * DEFAULT_TIME_OUT_LOOP);
            continue;
        }

        ret = hi_unf_dmx_release_es(g_ply_handle, &es_buf);
        if (ret != HI_SUCCESS) {
            printf("call HI_UNF_DMX_ReleaseEs failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_char input_cmd[DEFAULT_STR_LEN];
    hi_unf_dmx_ts_buffer_attr ram_attrs = {0};
    hi_unf_dmx_chan_attr play_attrs = {0};
    hi_unf_ssm_attr ssm_intent;
    hi_handle ssm;
    hi_unf_ssm_module_resource ssm_res;
    dmx_play_status play_status;

    if (argc != 0x3) {
        printf("\nuseage: savets ts_file ts1_file\n");
        printf("\ncreate an pid tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf("    source ts_file : source ts file.\n");
        printf("    dest ts file : dest ts file.\n");
        return -1;
    }

    g_ts_in_file = fopen(argv[0x1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open ts file failed!\n");
        goto out;
    }

    g_ts_out_file = fopen(argv[0x2], "wb");
    if (g_ts_out_file == HI_NULL) {
        printf("open ts file failed!\n");
        goto dmx_close_in_file;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_init failed.\n");
        goto dmx_close_out_file;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_init failed.\n");
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

    ret = hi_unf_dmx_get_ts_buffer_default_attr(&ram_attrs);
    if (ret != HI_SUCCESS) {
        printf("call HI_UNF_DMX_GetTSBufferDefaultAttr failed.\n");
        goto ssm_destroy;
    }

    ram_attrs.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ram_attrs.buffer_size = DEFAULT_TS_BUF_SIZE;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ram_attrs, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto ssm_destroy;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto dmx_destroy_tsbuf;
    }

    pthread_create(&g_ts_put_thread, HI_NULL, (hi_void *)ts_tx_thread, HI_NULL);

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_put_thread = HI_FALSE;

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_table);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_search_get_all_pmt failed.\n");
        goto dmx_ramport_detach;
    }

    printf("video pid is %d\n", g_prog_table->proginfo->v_element_pid);

    ret = hi_unf_dmx_get_play_chan_default_attr(&play_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_play_chan_default_attr failed.\n");
        goto dmx_pmt_deinit;
    }

    play_attrs.chan_type = HI_UNF_DMX_CHAN_TYPE_VID;
    play_attrs.secure_mode = HI_UNF_DMX_SECURE_MODE_TEE;
    ret = hi_unf_dmx_create_play_chan(PLAY_DMX_ID, &play_attrs, &g_ply_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_play_chan failed.\n");
        goto dmx_pmt_free;
    }
    ret = hi_mpi_dmx_play_get_status(g_ply_handle, &play_status);
    if (ret != HI_SUCCESS) {
        printf("get play status error!\n");
    }
    printf("play_buf handle[0x%llx]\n", play_status.buf_handle);

    ret = hi_unf_dmx_set_play_chan_pid(g_ply_handle, g_prog_table->proginfo->v_element_pid);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_set_play_chan_pid failed.\n");
        goto dmx_ply_destroy;
    }

    ssm_res.module_handle = g_ply_handle;
    ret = hi_unf_ssm_add_resource(ssm, &ssm_res);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_add_resource failed, ret %x.\n", ret);
        goto dmx_ply_destroy;
    }

    ret = hi_unf_dmx_play_chan_attach_ssm(g_ply_handle, ssm);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_play_chan_attach_ssm failed, ret %x.\n", ret);
        goto dmx_ply_destroy;
    }

    pthread_mutex_lock(&g_ts_mutex);
    rewind(g_ts_in_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buf);

    ret = hi_unf_dmx_open_play_chan(g_ply_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_open_play_chan failed.\n");
        goto dmx_ply_destroy;
    }

    pthread_create(&g_ts_get_thread, HI_NULL, (hi_void *)ts_rx_thread, HI_NULL);
    g_stop_ts_get_thread = HI_FALSE;
    pthread_mutex_unlock(&g_ts_mutex);

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

dmx_ply_destroy:
    hi_unf_dmx_destroy_play_chan(g_ply_handle);

dmx_pmt_free:
    hi_adp_search_free_all_pmt(g_prog_table);

dmx_pmt_deinit:
    hi_adp_search_de_init();

dmx_ramport_detach:
    hi_unf_dmx_detach_ts_port(PLAY_DMX_ID);

dmx_destroy_tsbuf:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);

ssm_destroy:
    hi_unf_ssm_destroy(ssm);

ssm_deinit:
    hi_unf_ssm_deinit();

dmx_deinit:
    hi_unf_dmx_deinit();

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


