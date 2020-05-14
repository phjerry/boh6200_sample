/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: Scrambled steam record testing.
 * Author: sdk
 * Create: 2019-9-29
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
#include "hi_unf_descrambler.h"
#include "hi_unf_keyslot.h"
#include "hi_unf_klad.h"

#define REC_DMX_ID      0x0
#define DMX_INVALID_PID 0x1FFFU
#define KEY_LEN         16
#define REQ_BUF_LEN     (188 * 200)
#define TIME_OUT_MS     1000
#define CMD_LEN         32
#define PID_NUM         5
#define MIN_ARGC        3
#define MAX_ARGC        8

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

FILE               *g_ts_in_file  = HI_NULL;
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
    hi_unf_stream_buf   buf = {0};
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);

        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, REQ_BUF_LEN, &buf, TIME_OUT_MS);
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
        usleep(TIME_OUT_MS);

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
    hi_unf_dmx_rec_data st_buf = {0};

    printf("receive start!\n");

    while (!g_stop_ts_get_thread) {
        ret = hi_unf_dmx_acquire_rec_data(g_rec_handle, &st_buf, TIME_OUT_MS);
        if (ret != HI_SUCCESS) {
            usleep(TIME_OUT_MS);
            continue;
        }

        /* printf("data_addr[0x%x], length[0x%x]\n", (hi_u32)pst_buf.data, pst_buf.length); */
        ret = fwrite(st_buf.data_addr, 1, st_buf.len, g_ts_out_file);
        if (st_buf.len != ret) {
            printf("fwrite failed, ret[0x%x], st_buf.len[0x%x]\n", ret, st_buf.len);
        }

        ret = hi_unf_dmx_release_rec_data(g_rec_handle, &st_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_rec_release_buf failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_char input_cmd[CMD_LEN];
    hi_s32 pid[PID_NUM] = {-1};
    hi_handle dsc_handle = HI_INVALID_HANDLE;
    hi_handle ks_handle = HI_INVALID_HANDLE;
    hi_handle klad_handle = HI_INVALID_HANDLE;
    hi_handle  pid_ch_handle[PID_NUM] = {-1};
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    hi_s32 i = 0;
    hi_unf_dmx_ts_buffer_attr  ram_attrs = {0};
    hi_unf_dmx_rec_attr    RecAttr = {0};
    hi_unf_dmx_desc_attr dsc_attrs = {0};
    hi_unf_keyslot_attr keyslot_attr = {0};

    /* these keys is for aes128ecb-13.ts */
    static hi_u8 odd_key[KEY_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static hi_u8 even_key[KEY_LEN] = {0xf9,0x33,0xd7,0x83,0xd3,0x15,0xc8,0x5b,0x37,0x78,0xf3,0x60,0x46,0x27,0x12,0x19};

    if (argc < MIN_ARGC || argc > MAX_ARGC) {
        printf("\nuseage: record ts_file and save it as rec_ts_file\n");
        printf("\nrecord tsfile as rec_ts_file max support 5 pids.\n");
        printf("\nargument:\n");
        printf("    ts_file : source ts file.\n");
        printf("    rec_ts_file : dest ts file.\n");
        printf("    pid     : the pid of ts file.\n");
        printf("    ...     : the pid of ts file.\n");
        return -1;
    }

    for (i = 0; i < argc - MIN_ARGC; i++) {
        if (strstr(argv[MIN_ARGC + i], "0x") || strstr(argv[MIN_ARGC + i], "0X")) {
            pid[i] = strtoul(argv[MIN_ARGC + i], NULL, 16); /* 16 means hex */
        } else {
            pid[i] = strtoul(argv[MIN_ARGC + i], NULL, 10); /* 10 means decimal */
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


    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_keyslot_init failed.\n");
        goto dmx_deinit;
    }

    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_klad_init failed.\n");
        goto ks_deinit;
    }

    keyslot_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    keyslot_attr.type = HI_UNF_KEYSLOT_TYPE_TSCIPHER;
    ret = hi_unf_keyslot_create(&keyslot_attr, &ks_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_keyslot_create failed.\n");
        goto klad_deinit;
    }

    ret = hi_unf_klad_create(&klad_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_klad_create failed.\n");
        goto ks_destroy;
    }

    dsc_attrs.entropy_reduction= HI_UNF_DMX_CA_ENTROPY_REDUCTION_CLOSE;
    dsc_attrs.alg_type = HI_UNF_CRYPTO_ALG_AES_ECB_T;
    dsc_attrs.is_create_keyslot    = HI_FALSE;
    ret = hi_unf_dmx_desc_create(REC_DMX_ID, &dsc_attrs, &dsc_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_desc_create failed.\n");
        goto klad_destroy;
    }

    ret = hi_unf_dmx_desc_attach_key_slot(dsc_handle, ks_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_desc_attach_key_slot failed.\n");
        goto dsc_destroy;
    }

    memset(&attr_klad, 0x0, sizeof(hi_unf_klad_attr));
    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support= 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_AES_ECB_T;
    ret = hi_unf_klad_set_attr(klad_handle, &attr_klad);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_klad_set_attr failed.\n");
        goto dsc_detach_keyslot;
    }

    ret = hi_unf_klad_attach(klad_handle, ks_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_klad_attach failed.\n");
        goto dsc_detach_keyslot;
    }

    ret = hi_unf_dmx_get_ts_buffer_default_attr(&ram_attrs);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_get_ts_buffer_default_attr failed.\n");
        goto klad_detach;
    }

    ram_attrs.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ram_attrs.buffer_size = 0x200000; /* 2MB TS buffer */
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ram_attrs, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto klad_deinit;
    }

    ret = hi_unf_dmx_attach_ts_port(REC_DMX_ID, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto dmx_destroy_ts_buf;
    }

    RecAttr.dmx_id        = REC_DMX_ID;
    RecAttr.rec_buf_size   = 4 * 1024 * 1024; /* 4MB record buffer */
    RecAttr.rec_type       = HI_UNF_DMX_REC_TYPE_ALL_PID;
    RecAttr.descramed      = HI_TRUE;
    RecAttr.index_type     = HI_UNF_DMX_REC_INDEX_TYPE_NONE;
    RecAttr.index_src_pid  = DMX_INVALID_PID;

    if (argc > MIN_ARGC) {
        printf("start the select pid record!\n");
        RecAttr.rec_type   = HI_UNF_DMX_REC_TYPE_SELECT_PID;
    }

    ret = hi_unf_dmx_create_rec_chan(&RecAttr, &g_rec_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_rec_chan failed.\n");
        goto dmx_detach_ts_port;
    }

    if (argc > MIN_ARGC && RecAttr.rec_type == HI_UNF_DMX_REC_TYPE_SELECT_PID) {
        for (i = 0; i < argc - MIN_ARGC; i++) {
            printf("begin call hi_mpi_dmx_pid_ch_create, pid[0x%x].\n", pid[i]);
            ret = hi_unf_dmx_add_rec_pid(g_rec_handle, pid[i], &pid_ch_handle[i]);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_dmx_add_rec_pid failed.\n");
                goto dmx_rec_destroy;
            }


            ret = hi_unf_dmx_desc_attach_pid_chan(dsc_handle, pid_ch_handle[i]);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_dmx_desc_attach_pid_chan failed.\n");
                goto dmx_rec_del_ch;
            }
        }
    }

    key_clear.odd = 1;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, odd_key, KEY_LEN);
    ret = hi_unf_klad_set_clear_key(klad_handle, &key_clear);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_klad_set_clear_key set odd key failed.\n");
        goto dsc_detach;
    }

    key_clear.odd = 0;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, even_key, KEY_LEN);
    ret = hi_unf_klad_set_clear_key(klad_handle, &key_clear);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_klad_set_clear_key set even key failed.\n");
        goto dsc_detach;
    }

    ret = hi_unf_dmx_start_rec_chan(g_rec_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_mpi_dmx_rec_open failed.\n");
        goto dsc_detach;
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

    hi_unf_dmx_stop_rec_chan(g_rec_handle);

dsc_detach:
    if (argc > MIN_ARGC && RecAttr.rec_type == HI_UNF_DMX_REC_TYPE_SELECT_PID) {
        for (i = 0; i < argc - MIN_ARGC; i++) {
            hi_unf_dmx_desc_detach_pid_chan(dsc_handle, pid_ch_handle[i]);
        }
    }

dmx_rec_del_ch:
    if (argc > MIN_ARGC && RecAttr.rec_type == HI_UNF_DMX_REC_TYPE_SELECT_PID) {
        for (i = 0; i < argc - MIN_ARGC; i++) {
            hi_unf_dmx_del_rec_pid(g_rec_handle, pid_ch_handle[i]);
        }
    }

dmx_rec_destroy:
    hi_unf_dmx_destroy_rec_chan(g_rec_handle);

dmx_detach_ts_port:
    hi_unf_dmx_detach_ts_port(REC_DMX_ID);

dmx_destroy_ts_buf:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);

klad_detach:
    hi_unf_klad_detach(klad_handle, ks_handle);

dsc_detach_keyslot:
    hi_unf_dmx_desc_detach_key_slot(dsc_handle);

dsc_destroy:
    hi_unf_dmx_desc_destroy(dsc_handle);

klad_destroy:
    hi_unf_klad_destroy(klad_handle);

ks_destroy:
    hi_unf_keyslot_destroy(ks_handle);

klad_deinit:
    hi_unf_klad_deinit();

ks_deinit:
hi_unf_keyslot_deinit();

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

