/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_cbcmac
 * Author: zhaoguihong
 * Create: 2019-06-18
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_cipher.h"
#include "hi_unf_klad.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define HI_ERR_CIPHER(format, arg...)
#define HI_INFO_CIPHER(format, arg...) printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#else
#define HI_ERR_CIPHER(format, arg...)  printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#define HI_INFO_CIPHER(format, arg...) printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#endif

#define SAMPLE_KEY_LEN     16

static hi_void print_buffer(const char *string, const hi_u8 *input, hi_u32 length)
{
    hi_u32 i = 0;

    if (string != NULL) {
        printf("%s\n", string);
    }

    for (i = 0; i < length; i++) {
        if ((i % 16 == 0) && (i != 0)) {
            printf("\n");
        }
        printf("0x%02x ", input[i]);
    }
    printf("\n");

    return;
}

hi_s32 cipher_set_clear_key(hi_handle cipher, hi_unf_crypto_alg engine, const hi_u8 *key, hi_u32 keylen)
{
    hi_s32 ret;
    hi_handle handle_ks = 0;
    hi_handle handle_klad = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};

    ret = hi_unf_klad_init();
    if(ret != HI_SUCCESS){
        HI_ERR_CIPHER("hi_unf_klad_init failed\n");
        return ret;;
    }

    ret = hi_unf_cipher_get_keyslot_handle(cipher, &handle_ks);
    if(ret != HI_SUCCESS){
        HI_ERR_CIPHER("hi_unf_cipher_get_keyslot_handle failed\n");
        goto KLAD_DEINIT;
    }

    ret = hi_unf_klad_create(&handle_klad);
    if(ret != HI_SUCCESS){
        HI_ERR_CIPHER("hi_unf_klad_create failed\n");
        goto KLAD_DEINIT;
    }

    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support= 1;
    attr_klad.key_cfg.encrypt_support= 1;
    attr_klad.key_cfg.engine = engine;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if(ret != HI_SUCCESS){
        HI_ERR_CIPHER("hi_unf_klad_set_attr failed\n");
        goto KLAD_DESTORY;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if(ret != HI_SUCCESS){
        HI_ERR_CIPHER("hi_unf_klad_attach failed\n");
        goto KLAD_DESTORY;
    }

    key_clear.odd = 0;
    key_clear.key_size = keylen;
    memcpy(key_clear.key, key, keylen);
    ret = hi_unf_klad_set_clear_key(handle_klad, &key_clear);
    if(ret != HI_SUCCESS){
        HI_ERR_CIPHER("hi_unf_klad_set_clear_key failed\n");
        goto KLAD_DETACH;
    }

KLAD_DETACH:
    hi_unf_klad_detach(handle_klad, handle_ks);
KLAD_DESTORY:
    hi_unf_klad_destroy(handle_klad);
KLAD_DEINIT:
    hi_unf_klad_deinit();

    return ret;
}

hi_s32 cipher_set_config_info(hi_handle cipher, const hi_u8 key_buf[SAMPLE_KEY_LEN])
{
    hi_s32 ret;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_config_aes cipher_ctrl_aes;

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    memset(&cipher_ctrl_aes, 0, sizeof(hi_unf_cipher_config_aes));
    cipher_ctrl.alg = HI_UNF_CIPHER_ALG_AES;
    cipher_ctrl.work_mode = HI_UNF_CIPHER_WORK_MODE_CBC;
    cipher_ctrl_aes.bit_width = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
    cipher_ctrl_aes.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
    cipher_ctrl_aes.change_flags.iv_change_flag = 1;
    cipher_ctrl.param = &cipher_ctrl_aes;

    ret = hi_unf_cipher_set_config(cipher, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("HI_UNF_CIPHER_ConfigHandle failed\n");
        return ret;
    }

    ret = cipher_set_clear_key(cipher, HI_UNF_CRYPTO_ALG_RAW_AES, key_buf, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 main(hi_s32 argc, char *argv[])
{
    hi_s32 ret;
    hi_u8 mac_value[16];
    hi_u8 M[40] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11
    };

    hi_u8 key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    hi_u8 mac[16] = { 0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30, 0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27 };
    hi_handle cipher_handle = 0;
    hi_unf_cipher_attr cipher_attr = { 0 };
    hi_unf_cipher_calc_mac_data calc_mac_data;

    hi_unf_cipher_init();

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&cipher_handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher Create handle failed!\n");
        (hi_void)hi_unf_sys_deinit();
        return HI_FAILURE;
    }

    ret = cipher_set_config_info(cipher_handle, key);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher setconfiginfo failed!\n");
        hi_unf_cipher_deinit();
        (hi_void)hi_unf_sys_deinit();
        return HI_FAILURE;
    }

    calc_mac_data.input_data = M;
    calc_mac_data.input_data_len = 16;
    calc_mac_data.output_mac = mac_value;
    calc_mac_data.mac_buf_len = sizeof(mac_value);
    calc_mac_data.last_block = HI_FALSE;
    hi_unf_cipher_calc_mac(cipher_handle, &calc_mac_data);
    calc_mac_data.input_data = M + 16;
    hi_unf_cipher_calc_mac(cipher_handle, &calc_mac_data);
    calc_mac_data.input_data = M + 32;
    calc_mac_data.input_data_len = 8;
    calc_mac_data.last_block = HI_TRUE;
    hi_unf_cipher_calc_mac(cipher_handle, &calc_mac_data);

    print_buffer("cbcmac value:", mac_value, sizeof(mac_value));
    ret = hi_unf_cipher_destroy(cipher_handle);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher destroy_handle failed!\n");
        hi_unf_cipher_deinit();
        (hi_void)hi_unf_sys_deinit();
        return HI_FAILURE;
    }
    if (memcmp(mac_value, mac, 16) != 0) {
        HI_ERR_CIPHER("cipher cmac failed!\n");
        hi_unf_cipher_deinit();
        (hi_void)hi_unf_sys_deinit();
        return HI_FAILURE;
    }

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

