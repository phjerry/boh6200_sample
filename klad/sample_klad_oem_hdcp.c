/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: stbm hdcp keyladder sample.
 * Author: linux SDK team
 * Create: 2019-07-25
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_klad.h"
#include "hi_unf_cipher.h"
#include "hi_mpi_cipher.h"
#include "hi_unf_keyslot.h"

#define klad_err_print_hex(val)   printf("[%-32s][line:%04d]%s = 0x%08x\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_info(val)  printf("[%-32s][line:%04d]%s\n", __FUNCTION__, __LINE__, val)
#define klad_err_print_val(val)   printf("[%-32s][line:%04d]%s = %d\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_point(val) printf("[%-32s][line:%04d]%s = %p\n", __FUNCTION__, __LINE__, #val, val)
#define klad_print_error_code(err_code) \
    printf("[%-32s][line:%04d]return [0x%08x]\n", __FUNCTION__, __LINE__, err_code)
#define klad_print_error_func(func, err_code) \
    printf("[%-32s][line:%04d]call [%s] return [0x%08x]\n", __FUNCTION__, __LINE__, #func, err_code)

#define KEY_LEN 16
#define DATA_LEN 32

hi_u8 g_data_in[DATA_LEN] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32
};

hi_u8 g_content_key[KEY_LEN] = {
    0x67, 0x47, 0x9C, 0x36, 0x6F, 0x2C, 0x19, 0xC7,
    0x2D, 0x3A, 0x12, 0xB6, 0x75, 0x0F, 0x26, 0x98
};

static hi_void print_buffer(hi_char *string, hi_u8 *input, hi_u32 length)
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

hi_s32 set_key(hi_handle klad)
{
    hi_s32 ret;
    hi_unf_klad_content_key content_key = {0};

    content_key.odd = 0;
    content_key.key_size = KEY_LEN;
    content_key.alg = HI_UNF_KLAD_ALG_TYPE_AES;
    memcpy(content_key.key, g_content_key, KEY_LEN);
    ret = hi_unf_klad_set_content_key(klad, &content_key);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_content_key, ret);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle handle_klad = 0;
    hi_handle handle_ks = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_u32 data_len = DATA_LEN;
    hi_cipher_hdcp_attr attr = {0};
    hi_u8 data_out[DATA_LEN] = {0};
    hi_unf_keyslot_attr ks_attr = {0};

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_sys_init, ret);
        return ret;
    }

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_cipher_init, ret);
        goto sys_deinit;
    }

    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_init, ret);
        goto cipher_deinit;
    }

    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_keyslot_init, ret);
        goto klad_deinit;
    }

    ret = hi_unf_klad_create(&handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_create, ret);
        goto ks_deinit;
    }

    ks_attr.type = HI_UNF_KEYSLOT_TYPE_MCIPHER;
    ks_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    ret = hi_unf_keyslot_create(&ks_attr, &handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_keyslot_create, ret);
        goto klad_destroy;
    }

    attr_klad.klad_cfg.owner_id = 0x0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_OEM_HDCP;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_RAW_HDCP;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_attr, ret);
        goto ks_destroy;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_attach, ret);
        goto out;
    }

    ret = set_key(handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(set_key, ret);
        goto out;
    }


    attr.ram_sel = HDMI_RAM_SEL_TX_14;
    attr.ram_num = 0;
    attr.key_sel = HDCP_KEY_SEL_KLAD;
    attr.key_slot = handle_ks;
    attr.alg     = HI_CIPHER_ALG_AES;
    attr.mode    = HI_CIPHER_WORK_MODE_GCM;
    hi_mpi_hdcp_operation(&attr, g_data_in, data_out, data_len, 0);  /* ¼ÓÃÜ */
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_mpi_hdcp_operation, ret);
        goto out;
    }

    print_buffer("clear data:", g_data_in, data_len);
    print_buffer("encrypted data:", data_out, data_len);

out:

ks_destroy:
    hi_unf_keyslot_destroy(handle_ks);
klad_destroy:
    hi_unf_klad_destroy(handle_klad);
ks_deinit:
    hi_unf_keyslot_deinit();
klad_deinit:
    hi_unf_klad_deinit();
cipher_deinit:
    hi_unf_cipher_deinit();
sys_deinit:
    hi_unf_sys_deinit();
    return 0;
}


