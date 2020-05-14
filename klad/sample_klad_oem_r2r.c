/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: stbm r2r keyladder sample.
 * Author: linux SDK team
 * Create: 2019-09-19
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "securec.h"
#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_klad.h"
#include "hi_unf_cipher.h"

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

hi_s32 cfg_cipher(hi_handle cipher, hi_unf_cipher_alg alg, hi_unf_cipher_work_mode mode, hi_u8 *iv, hi_u32 iv_len)
{
    hi_s32 ret;
    hi_unf_cipher_config cipher_ctrl = {0};
    hi_unf_cipher_config_aes cipher_ctrl_aes = {0};
    hi_unf_cipher_config_3des cipher_ctrl_3des = {0};
    hi_unf_cipher_config_sm4 cipher_ctrl_sm4 = {0};

    cipher_ctrl.alg = alg;
    cipher_ctrl.work_mode = mode;

    if (cipher_ctrl.alg == HI_UNF_CIPHER_ALG_AES) {
        cipher_ctrl_aes.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
        cipher_ctrl_aes.bit_width = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
        if (cipher_ctrl.work_mode != HI_UNF_CIPHER_WORK_MODE_ECB && iv != HI_NULL) {
            cipher_ctrl_aes.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ONE_PKG;
            memcpy_s(cipher_ctrl_aes.iv, sizeof(cipher_ctrl_aes.iv), iv, iv_len);
        }
        cipher_ctrl.param = &cipher_ctrl_aes;
    } else if (cipher_ctrl.alg == HI_UNF_CIPHER_ALG_3DES) {
        cipher_ctrl_3des.key_len = HI_UNF_CIPHER_KEY_DES_2KEY;
        cipher_ctrl_3des.bit_width = HI_UNF_CIPHER_BIT_WIDTH_64BIT;
        if (cipher_ctrl.work_mode != HI_UNF_CIPHER_WORK_MODE_ECB && iv != HI_NULL) {
            cipher_ctrl_3des.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ONE_PKG;
            memcpy_s(cipher_ctrl_3des.iv, sizeof(cipher_ctrl_3des.iv), iv, iv_len);
        }
        cipher_ctrl.param = &cipher_ctrl_3des;
    } else if (cipher_ctrl.alg == HI_UNF_CIPHER_ALG_SM4) {
        if (cipher_ctrl.work_mode != HI_UNF_CIPHER_WORK_MODE_ECB && iv != HI_NULL) {
            cipher_ctrl_sm4.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ONE_PKG;
            memcpy_s(cipher_ctrl_sm4.iv, sizeof(cipher_ctrl_sm4.iv), iv, iv_len);
        }
        cipher_ctrl.param = &cipher_ctrl_sm4;
    } else {
        return HI_FAILURE;
    }

    ret = hi_unf_cipher_set_config(cipher, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_cipher_set_config, ret);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle handle_klad = 0;
    hi_handle handle_cipher = 0;
    hi_handle handle_ks = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_cipher_attr cipher_attr = {0};
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_mem_handle src_handle;
    hi_mem_handle dst_handle;
    hi_mem_size_t data_len = DATA_LEN;
    hi_u8 iv[KEY_LEN] = {0};

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

    ret = hi_unf_klad_create(&handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_create, ret);
        goto klad_deinit;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle_cipher, &cipher_attr);
    if (ret != HI_SUCCESS) {
        goto klad_destroy;
    }
    src_handle.addr_offset = 0;
    src_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in"), data_len, 0);
    if (src_handle.mem_handle == 0) {
        klad_err_print_info("error: get phyaddr for input failed!\n");
        goto cipher_destroy;
    }
    input_addr_vir = hi_unf_mem_map(src_handle.mem_handle, data_len);

    dst_handle.addr_offset= 0;
    dst_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out"), data_len, 0);
    if (dst_handle.mem_handle == 0) {
        klad_err_print_info("error: get phyaddr for out_put failed!\n");
        goto out;
    }
    output_addr_vir = hi_unf_mem_map(dst_handle.mem_handle, data_len);

    attr_klad.klad_cfg.owner_id = 0x01;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_OEM_R2R;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_RAW_AES;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_attr, ret);
        goto out;
    }

    ret = hi_unf_cipher_get_keyslot_handle(handle_cipher, &handle_ks);
    if(ret != HI_SUCCESS){
        klad_print_error_func(hi_unf_cipher_get_keyslot_handle, ret);
        goto out;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_attach, ret);
        goto out;
    }

    ret = cfg_cipher(handle_cipher, HI_UNF_CIPHER_ALG_AES, HI_UNF_CIPHER_WORK_MODE_CBC, iv, KEY_LEN);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(cfg_cipher, ret);
        goto out;
    }

    ret = set_key(handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(set_key, ret);
        goto out;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, g_data_in, data_len);
    memset(output_addr_vir, 0x0, data_len);
    ret = hi_unf_cipher_encrypt(handle_cipher, src_handle, dst_handle, data_len);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_cipher_encrypt, ret);
        goto out;
    }

    print_buffer("clear data:", g_data_in, data_len);
    print_buffer("encrypted data:", output_addr_vir, data_len);

out:
    if (src_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(src_handle.mem_handle);
    }
    if (dst_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(dst_handle.mem_handle);
    }

cipher_destroy:
    hi_unf_cipher_destroy(handle_cipher);
klad_destroy:
    hi_unf_klad_destroy(handle_klad);
klad_deinit:
    hi_unf_klad_deinit();
cipher_deinit:
    hi_unf_cipher_deinit();
sys_deinit:
    hi_unf_sys_deinit();
    return 0;
}



