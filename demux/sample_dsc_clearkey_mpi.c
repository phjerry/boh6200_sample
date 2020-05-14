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
#include "hi_mpi_keyslot.h"
#include "hi_unf_klad.h"
#include "hi_mpi_demux.h"
#include "hi_unf_demux.h"
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
    dmx_ram_buffer buf;
    printf("ts send start!\n");
    while (!g_stop_ts_put_thread) {
        pthread_mutex_lock(&g_ts_mutex);

        ret = hi_mpi_dmx_ram_get_buffer(g_ts_buf, TS_BUF_LEN, &buf, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("ramport get buffer failed!\n");
            continue;
        }

        readlen = fread(buf.data, sizeof(hi_s8), buf.length, g_ts_in_file);
        if (readlen <= 0) {
            printf("read ts file end and rewind!\n");
            hi_mpi_dmx_ram_put_buffer(g_ts_buf, buf.length, 0);
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
    hi_s32 i;
    hi_u32 acquire_num = 10; /* 10 means the number of ts data */
    hi_u32 acquired_num;
    dmx_buffer st_buf[10];   /* 10 means the number of ts data */

    printf("receive start!\n");

    while (!g_stop_es_get_thread) {
        ret = hi_mpi_dmx_play_acquire_buf(g_play_handle, acquire_num, &acquired_num, st_buf, 5000);
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
            ret = fwrite(st_buf[i].data, 1, st_buf[i].length, g_ts_out_file);
            if (st_buf[i].length != ret) {
                printf("fwrite failed, ret[0x%x], st_buf[i].length[0x%x]\n", ret, st_buf[i].length);
            }
        }

        ret = hi_mpi_dmx_play_release_buf(g_play_handle, acquired_num, st_buf);
        if (HI_SUCCESS != ret) {
            printf("call hi_mpi_dmx_release_buf failed!\n");
        }
    }
    printf("receive over!\n");
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_handle band_handle;
    hi_handle dsc_handle;
    hi_handle ks_handle = 0;
    hi_handle klad_handle = 0;
    hi_handle pid_ch_handle;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    hi_char input_cmd[CMD_LEN];
    hi_s32 pid;
    dmx_ram_port_attr ram_attrs;
    dmx_port_attr port_attrs;
    dmx_band_attr band_attrs;
    dmx_play_attrs play_attrs;
    dmx_dsc_attrs dsc_attrs;
    hi_keyslot_attr keyslot_attr;

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

    CALL_FUNC(hi_mpi_dmx_init(), goto sys_deinit);

    CALL_FUNC(hi_mpi_keyslot_init(), goto dmx_deinit);

    CALL_FUNC(hi_unf_klad_init(), goto ks_deinit);
#ifdef EXTERNAL_KEYSLOT
    keyslot_attr.secure_mode = HI_KEYSLOT_SECURE_MODE_NONE;
    keyslot_attr.type = HI_KEYSLOT_TYPE_TSCIPHER;
    CALL_FUNC(hi_mpi_keyslot_create(&keyslot_attr, &ks_handle),
        goto klad_deinit);
    printf("hi_mpi_key_slot_create success. ks_handle = %#x\n", ks_handle);

    CALL_FUNC(hi_unf_klad_create(&klad_handle), goto ks_destroy);
    printf("hi_unf_klad_create success.\n");
#else
    CALL_FUNC(hi_unf_klad_create(&klad_handle), goto klad_deinit);
    printf("hi_unf_klad_create success.\n");
#endif
    memset(&attr_klad, 0x0, sizeof(hi_unf_klad_attr));
    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support= 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_AES_ECB_T;
    CALL_FUNC(hi_unf_klad_set_attr(klad_handle, &attr_klad), goto klad_destroy);
    printf("hi_unf_klad_set_attr success.\n");

    CALL_FUNC(hi_mpi_dmx_ram_get_port_default_attrs(&ram_attrs), goto klad_destroy);

    CALL_FUNC(hi_mpi_dmx_ram_open_port(DMX_RAM_PORT_0, &ram_attrs, &g_ts_buf), goto klad_destroy);
    printf("hi_mpi_dmx_ram_open_port success.\n");

    CALL_FUNC(hi_mpi_dmx_ram_get_port_attrs(g_ts_buf, &port_attrs), goto dmx_close_ram);

    CALL_FUNC(hi_mpi_dmx_ram_set_port_attrs(g_ts_buf, &port_attrs), goto dmx_close_ram);

    band_attrs.band_attr = 0;
    CALL_FUNC(hi_mpi_dmx_band_open(DMX_BAND_0, &band_attrs, &band_handle), goto dmx_close_ram);
    printf("hi_mpi_dmx_band_open success.\n");

    CALL_FUNC(hi_mpi_dmx_band_attach_port(band_handle, DMX_RAM_PORT_0), goto dmx_close_band);
    printf("hi_mpi_dmx_band_attach_port success.\n");

    CALL_FUNC(hi_mpi_dmx_pid_ch_create(band_handle, pid, &pid_ch_handle), goto dmx_band_detach);
    printf("hi_mpi_dmx_pid_ch_create success.\n");

    CALL_FUNC(hi_mpi_dmx_play_get_default_attrs(&play_attrs), goto dmx_pidch_destroy);

    CALL_FUNC(hi_mpi_dmx_play_create(&play_attrs, &g_play_handle), goto dmx_pidch_destroy);
    printf("call hi_mpi_dmx_play_create success.\n");

    CALL_FUNC(hi_mpi_dmx_play_attach_pid_ch(g_play_handle, pid_ch_handle), goto play_destroy);
    printf("call hi_mpi_dmx_play_attach_pid_ch success.\n");

    CALL_FUNC(hi_mpi_dmx_dsc_get_default_attrs(&dsc_attrs), goto play_detach);
#ifdef EXTERNAL_KEYSLOT
    dsc_attrs.keyslot_create_en = HI_FALSE;
#else
    dsc_attrs.keyslot_create_en = HI_TRUE;
#endif
    dsc_attrs.alg = DMX_DESCRAMBLER_TYPE_AES_ECB;
    CALL_FUNC(hi_mpi_dmx_dsc_create(&dsc_attrs, &dsc_handle), goto play_detach);
    printf("hi_mpi_dmx_dsc_create success.\n");
#ifdef EXTERNAL_KEYSLOT
    CALL_FUNC(hi_mpi_dmx_dsc_attach_keyslot(dsc_handle, ks_handle), goto dsc_detach);
    printf("hi_mpi_dmx_dsc_attach_keyslot success.ks_handle=%#x\n", ks_handle);
    sleep(1);

    CALL_FUNC(hi_mpi_dmx_dsc_attach(dsc_handle, pid_ch_handle), goto dsc_destroy);
    printf("hi_mpi_dmx_dsc_attach success.\n");
    sleep(1);

    CALL_FUNC(hi_unf_klad_attach(klad_handle, ks_handle), goto dsc_detach_keyslot);
    printf("hi_unf_klad_attach success.\n");
    sleep(1);
#else
    CALL_FUNC(hi_mpi_dmx_dsc_attach(dsc_handle, pid_ch_handle), goto dsc_destroy);
    printf("hi_mpi_dmx_dsc_attach success.\n");
    sleep(1);

    CALL_FUNC(hi_mpi_dmx_dsc_get_keyslot_handle(dsc_handle, &ks_handle), goto dsc_detach);

    CALL_FUNC(hi_unf_klad_attach(klad_handle, ks_handle), goto dsc_detach);
    printf("hi_unf_klad_attach success.\n");
    sleep(1);
#endif

    key_clear.odd = 1;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, odd_key, KEY_LEN);
    CALL_FUNC(hi_unf_klad_set_clear_key(klad_handle, &key_clear), goto klad_detach);

    key_clear.odd = 0;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, even_key, KEY_LEN);
    CALL_FUNC(hi_unf_klad_set_clear_key(klad_handle, &key_clear), goto klad_detach);
    printf("hi_unf_klad_set_clear_key success.\n");
    sleep(1);

    CALL_FUNC(hi_mpi_dmx_play_open(g_play_handle), goto klad_detach);
    printf("call hi_mpi_dmx_play_open success.\n");

    /* receive stream */
    pthread_create(&g_ts_get_thread, HI_NULL, (hi_void *)ts_rx_thread, HI_NULL);
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

    hi_mpi_dmx_play_close(g_play_handle);
    printf("hi_mpi_dmx_play_close success!\n");

klad_detach:
    hi_unf_klad_detach(klad_handle, ks_handle);
    printf("hi_unf_klad_detach success!\n");

#ifdef EXTERNAL_KEYSLOT
dsc_detach_keyslot:
    hi_mpi_dmx_dsc_detach_keyslot(dsc_handle);
    printf("hi_mpi_dmx_dsc_detach_keyslot success!\n");
dsc_detach:
    hi_mpi_dmx_dsc_detach(dsc_handle, pid_ch_handle);
    printf("hi_mpi_dmx_dsc_detach success!\n");
dsc_destroy:
    hi_mpi_dmx_dsc_destroy(dsc_handle);
    printf("hi_mpi_dmx_dsc_destroy success!\n");
#else
dsc_detach:
    hi_mpi_dmx_dsc_detach(dsc_handle, pid_ch_handle);
    printf("hi_mpi_dmx_dsc_detach success!\n");
dsc_destroy:
    hi_mpi_dmx_dsc_destroy(dsc_handle);
    printf("hi_mpi_dmx_dsc_destroy success!\n");
#endif
play_detach:
    hi_mpi_dmx_play_detach_pid_ch(g_play_handle);
    printf("hi_mpi_dmx_play_detach_pid_ch success!\n");
play_destroy:
    hi_mpi_dmx_play_destroy(g_play_handle);
    printf("hi_mpi_dmx_play_destroy success!\n");
dmx_pidch_destroy:
    hi_mpi_dmx_pid_ch_destroy(pid_ch_handle);
    printf("hi_mpi_dmx_pid_ch_destroy success!\n");
dmx_band_detach:
    hi_mpi_dmx_band_detach_port(band_handle);
    printf("hi_mpi_dmx_band_detach_port success!\n");
dmx_close_band:
    hi_mpi_dmx_band_close(band_handle);
    printf("hi_mpi_dmx_play_close success!\n");
dmx_close_ram:
    hi_mpi_dmx_ram_close_port(g_ts_buf);
    printf("hi_mpi_dmx_ram_close_port success!\n");
klad_destroy:
    hi_unf_klad_destroy(klad_handle);
    printf("hi_unf_klad_destroy success!\n");
#ifdef EXTERNAL_KEYSLOT
ks_destroy:
    hi_mpi_keyslot_destroy(ks_handle);
    printf("hi_mpi_key_slot_destroy success!\n");
#endif
klad_deinit:
    hi_unf_klad_deinit();
    printf("hi_unf_klad_deinit success!\n");
ks_deinit:
    hi_mpi_keyslot_deinit();
    printf("hi_mpi_key_slot_deinit success!\n");
dmx_deinit:
    hi_mpi_dmx_de_init();
    printf("hi_mpi_dmx_de_init success!\n");
sys_deinit:
    hi_unf_sys_deinit();
    printf("hi_unf_sys_deinit success!\n");

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

