/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: descrambler test for clearkey.
 * Author: linux SDK team
 * Create: 2019-07-23
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_keyslot.h"
#include "hi_unf_klad.h"
#include "hi_mpi_demux.h"
#include "hi_unf_demux.h"
#include "hi_unf_descrambler.h"
#include "hi_errno.h"

/* EXTERNAL_KEYSLOT means create keyslot and dsc separately */
#define EXTERNAL_KEYSLOT

#define CALL_FUNC(condition, err_cmd) do{   \
    hi_s32 ret = HI_FAILURE;                \
    ret = condition;                        \
    if (ret != HI_SUCCESS) {                \
        printf("%s() call:%s, error! line:%d, ret:0x%x\n", __FUNCTION__, # condition, __LINE__, ret); \
        err_cmd;                            \
    }                                       \
} while(0)

#define DEMUX_ID      0
#define KEY_LEN       16
#define TS_BUF_LEN    (188 * 200)
#define CMD_LEN       32
#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

FILE *g_ts_in_file = HI_NULL;
FILE *g_ts_out_file = HI_NULL;

static pthread_t g_ts_put_thread;
static pthread_t g_ts_get_thread;
static pthread_mutex_t g_ts_mutex;
static hi_bool g_stop_ts_put_thread = HI_FALSE;
static hi_bool g_stop_es_get_thread = HI_FALSE;

hi_handle g_ts_buf;
hi_handle g_play_handle;

hi_void ts_tx_thread(hi_void *args)
{
    hi_u32 readlen;
    hi_s32 ret;
    hi_unf_stream_buf buf;
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, TS_BUF_LEN, &buf, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("ramport get buffer failed!\n");
            continue;
        }

        readlen = fread(buf.data, sizeof(hi_s8), buf.size, g_ts_in_file);
        if (readlen <= 0) {
            printf("read ts file end and rewind!\n");
            hi_mpi_dmx_ram_put_buffer(g_ts_buf, buf.size, 0);
            rewind(g_ts_in_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_unf_dmx_put_ts_buffer(g_ts_buf, buf.size, 0);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_tsbuffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_mutex);
        usleep(1000 * 10);

    }
    printf("ts send end!\n");
    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_tsbuffer failed.\n");
    }

    return;
}

hi_void ts_rx_thread(hi_void *args)
{
    hi_u32 ret = HI_FAILURE;
    hi_s32 i = 0;
    hi_u32 acquire_num = 10; /* 10 means the number of ts data */
    hi_u32 acquired_num;
    hi_unf_dmx_data st_buf[10];   /* 10 means the number of ts data */

    printf("receive start!\n");

    while (!g_stop_es_get_thread) {
        ret = hi_unf_dmx_acquire_buffer(g_play_handle, acquire_num, &acquired_num, st_buf, 2000);
        if (ret != HI_SUCCESS) {
            if (ret == HI_ERR_DMX_TIMEOUT || ret == HI_ERR_DMX_NOAVAILABLE_DATA){
                usleep(10 * 1000);
                continue;
            }
            printf("acquire buf failed 0x%x, play_handle=0x%x.\n", ret, g_play_handle);
            break;
        }

        if (acquired_num > 10) {
            printf("acquired_num[0x%x] invalid!\n", acquired_num);
            usleep(10 * 1000);
            continue;
        }

        for (i = 0; i < acquired_num; i++) {
            ret = fwrite(st_buf[i].data, 1, st_buf[i].size, g_ts_out_file);
            if (st_buf[i].size != ret) {
                printf("fwrite failed, ret[0x%x], st_buf[i].size[0x%x]\n", ret, st_buf[i].size);
            }
        }

        ret = hi_unf_dmx_release_buffer(g_play_handle, acquired_num, st_buf);
        if (ret != HI_SUCCESS) {
            printf("call hi_mpi_dmx_release_buf failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_handle dsc_handle = HI_INVALID_HANDLE;
    hi_handle ks_handle = HI_INVALID_HANDLE;
    hi_handle klad_handle = HI_INVALID_HANDLE;
    hi_handle pid_ch_handle = HI_INVALID_HANDLE;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    hi_char input_cmd[CMD_LEN];
    hi_s32 pid;
    hi_unf_dmx_ts_buffer_attr tsbuffer_attr = {0};
    hi_unf_dmx_chan_attr play_attrs = {0};
    hi_unf_dmx_desc_attr dsc_attrs = {0};
#ifdef EXTERNAL_KEYSLOT
    hi_unf_keyslot_attr keyslot_attr = {0};
#endif

    /* these keys is for aes128ecb-13.ts */
    static hi_u8 odd_key[KEY_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static hi_u8 even_key[KEY_LEN] = {0xf9,0x33,0xd7,0x83,0xd3,0x15,0xc8,0x5b,0x37,0x78,0xf3,0x60,0x46,0x27,0x12,0x19};

    if (argc != 4) {
        printf("\nuseage: savets ts_file ts1_file pid\n");
        printf("\ncreate an pid tsfile from ts stream through pid.\n");
        printf("\nargument:\n");
        printf(" ts_file : source ts file.\n");
        printf(" es_file : dest ts file.\n");
        printf(" pid : the pid of video or audio.\n");
        goto out;
    }

    pid = (hi_s32)strtoul(argv[3], NULL, 0);

    g_ts_in_file = fopen(argv[1], "rb");
    if (g_ts_in_file == HI_NULL) {
        printf("open input ts file:%s failed!\n", argv[1]);
        goto out;
    }

    g_ts_out_file = fopen(argv[2], "wb");
    if (g_ts_out_file == HI_NULL) {
        printf("open output ts file:%s failed!\n", argv[2]);
        goto dmx_close_in_file;
    }

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_put_thread = HI_FALSE;
    g_stop_es_get_thread = HI_FALSE;

    CALL_FUNC(hi_unf_sys_init(), goto dmx_close_out_file);

    CALL_FUNC(hi_unf_dmx_init(), goto sys_deinit);

    CALL_FUNC(hi_unf_keyslot_init(), goto dmx_deinit);

    CALL_FUNC(hi_unf_klad_init(), goto ks_deinit);
#ifdef EXTERNAL_KEYSLOT
    keyslot_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    keyslot_attr.type = HI_UNF_KEYSLOT_TYPE_TSCIPHER;
    CALL_FUNC(hi_unf_keyslot_create(&keyslot_attr, &ks_handle), goto klad_deinit);

    CALL_FUNC(hi_unf_klad_create(&klad_handle), goto ks_destroy);
#else
    CALL_FUNC(hi_unf_klad_create(&klad_handle), goto klad_deinit);
#endif
    memset(&attr_klad, 0x0, sizeof(hi_unf_klad_attr));
    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support= 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_AES_ECB_T;
    CALL_FUNC(hi_unf_klad_set_attr(klad_handle, &attr_klad), goto klad_destroy);

    CALL_FUNC(hi_unf_dmx_attach_ts_port(DEMUX_ID, DMX_RAM_PORT_0), goto klad_destroy);

    CALL_FUNC(hi_unf_dmx_get_ts_buffer_default_attr(&tsbuffer_attr), goto detach_ts_port);

    CALL_FUNC(hi_unf_dmx_create_ts_buffer(DMX_RAM_PORT_0, &tsbuffer_attr, &g_ts_buf), goto detach_ts_port);

    CALL_FUNC(hi_unf_dmx_get_play_chan_default_attr(&play_attrs), goto desrtoy_ts_buffer);

    play_attrs.chan_type = HI_UNF_DMX_CHAN_TYPE_POST;
    CALL_FUNC(hi_unf_dmx_create_play_chan(DEMUX_ID, &play_attrs, &g_play_handle), goto desrtoy_ts_buffer);

    CALL_FUNC(hi_unf_dmx_set_play_chan_pid(g_play_handle, pid), goto play_destroy);

    CALL_FUNC(hi_unf_dmx_get_pid_chan_handle(DEMUX_ID, pid, &pid_ch_handle), goto play_destroy);

    dsc_attrs.entropy_reduction= DMX_CA_ENTROPY_CLOSE;
    dsc_attrs.alg_type = DMX_DESCRAMBLER_TYPE_AES_ECB;
#ifdef EXTERNAL_KEYSLOT
    dsc_attrs.is_create_keyslot    = HI_FALSE;
#else
    dsc_attrs.is_create_keyslot    = HI_TRUE;
#endif
    CALL_FUNC(hi_unf_dmx_desc_create(DEMUX_ID, &dsc_attrs, &dsc_handle), goto play_destroy);

    CALL_FUNC(hi_unf_dmx_desc_attach_pid_chan(dsc_handle, pid_ch_handle), goto dsc_destroy);

#ifdef EXTERNAL_KEYSLOT
    CALL_FUNC(hi_unf_dmx_desc_attach_key_slot(dsc_handle, ks_handle), goto dsc_detach);

    CALL_FUNC(hi_unf_klad_attach(klad_handle, ks_handle), goto dsc_detach_keyslot);
#else
    CALL_FUNC(hi_unf_dmx_desc_get_key_slot_handle(dsc_handle, &ks_handle), goto dsc_detach);

    CALL_FUNC(hi_unf_klad_attach(klad_handle, ks_handle), goto dsc_detach);
#endif

    key_clear.odd = 1;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, odd_key, KEY_LEN);
    CALL_FUNC(hi_unf_klad_set_clear_key(klad_handle, &key_clear), goto klad_detach);

    key_clear.odd = 0;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, even_key, KEY_LEN);
    CALL_FUNC(hi_unf_klad_set_clear_key(klad_handle, &key_clear), goto klad_detach);

    CALL_FUNC(hi_unf_dmx_open_play_chan(g_play_handle), goto klad_detach);

    /* receive stream */
    pthread_create(&g_ts_get_thread, HI_NULL, (hi_void *)ts_rx_thread, HI_NULL);
    sleep(1);
    /* send stream */
    pthread_create(&g_ts_put_thread, HI_NULL, (hi_void *)ts_tx_thread, HI_NULL);

    printf("please input 'q' to quit!\n");
    SAMPLE_GET_INPUTCMD(input_cmd);
    while (1) {
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_ts_put_thread = HI_TRUE;
    g_stop_es_get_thread = HI_TRUE;
    pthread_join(g_ts_put_thread, HI_NULL);
    pthread_join(g_ts_get_thread, HI_NULL);
    pthread_mutex_destroy(&g_ts_mutex);

    if (!g_ts_in_file || !g_ts_out_file) {
        fclose(g_ts_in_file);
        fclose(g_ts_out_file);
        g_ts_in_file = HI_NULL;
        g_ts_out_file = HI_NULL;
    }

    hi_unf_dmx_close_play_chan(g_play_handle);

klad_detach:
    hi_unf_klad_detach(klad_handle, ks_handle);
#ifdef EXTERNAL_KEYSLOT
dsc_detach_keyslot:
    hi_unf_dmx_desc_detach_key_slot(dsc_handle);
#endif
dsc_detach:
    hi_unf_dmx_desc_detach_pid_chan(dsc_handle, pid_ch_handle);
dsc_destroy:
    hi_unf_dmx_desc_destroy(dsc_handle);
play_destroy:
    hi_unf_dmx_destroy_play_chan(g_play_handle);
desrtoy_ts_buffer:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);
detach_ts_port:
    hi_unf_dmx_detach_ts_port(DEMUX_ID);
klad_destroy:
    hi_unf_klad_destroy(klad_handle);
#ifdef EXTERNAL_KEYSLOT
ks_destroy:
    hi_unf_keyslot_destroy(ks_handle);
#endif
klad_deinit:
    hi_unf_klad_deinit();
ks_deinit:
    hi_unf_keyslot_deinit();
dmx_deinit:
    hi_unf_dmx_deinit();
sys_deinit:
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

