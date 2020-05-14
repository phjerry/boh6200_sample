/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_multicipher
 * Author: zhaoguihong
 * Create: 2019-06-18
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_unf_memory.h"
#include "hi_unf_system.h"
#include "hi_unf_cipher.h"
#include "hi_adp.h"
#include "hi_unf_klad.h"

#define SAMPLE_AES_KEY_LEN     16
#define SAMPLE_3DES_KEY_LEN    24
#define SAMPLE_AES_IV_LEN      16
#define SAMPLE_DES_IV_LEN      8
#define SAMPLE_AES_DATA_LEN    16
#define SAMPLE_DES_DATA_LEN    8
#define SAMPLE_MULTI_PACKAGE   3

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define HI_ERR_CIPHER(format, arg...)
#define HI_INFO_CIPHER(format, arg...) printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#else
#define HI_ERR_CIPHER(format, arg...)  printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#define HI_INFO_CIPHER(format, arg...) printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#endif

static hi_u8 aes_key[SAMPLE_AES_KEY_LEN] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
static hi_u8 aes_iv[SAMPLE_AES_IV_LEN]  = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};
static hi_u8 aes_cbc_enc_src_buf[SAMPLE_AES_DATA_LEN] = {
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A
};
static hi_u8 aes_cbc_enc_dst_buf[SAMPLE_AES_DATA_LEN] = {
    0x76, 0x49, 0xAB, 0xAC, 0x81, 0x19, 0xB2, 0x46,
    0xCE, 0xE9, 0x8E, 0x9B, 0x12, 0xE9, 0x19, 0x7D
};
static hi_u8 aes_cbc_dec_src_buf[SAMPLE_AES_DATA_LEN] = {
    0x76, 0x49, 0xAB, 0xAC, 0x81, 0x19, 0xB2, 0x46,
    0xCE, 0xE9, 0x8E, 0x9B, 0x12, 0xE9, 0x19, 0x7D
};
static hi_u8 aes_cbc_dec_dst_buf[SAMPLE_AES_DATA_LEN] = {
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A
};

static hi_void print_buffer(const char *string, const hi_u8 *pu8_input, hi_u32 length)
{
    hi_u32 i = 0;

    if (string != NULL) {
        printf("%s\n", string);
    }

    for (i = 0; i < length; i++) {
        if ((i % 16 == 0) && (i != 0)) {
            printf("\n");
        }
        printf("0x%02x ", pu8_input[i]);
    }
    printf("\n");

    return;
}

hi_s32 cipher_set_clear_key(hi_handle cipher, hi_unf_crypto_alg enEngine, const hi_u8 *key, hi_u32 keylen)
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
    attr_klad.key_cfg.engine = enEngine;
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

hi_s32 cipher_set_config_info(hi_handle cipher,
                     hi_unf_cipher_alg alg,
                     hi_unf_cipher_work_mode mode,
                     hi_unf_cipher_key_length key_len,
                     const hi_u8 *key, hi_u32 klen,
                     const hi_u8 *iv, hi_u32 ivlen,
                     hi_unf_crypto_alg enEngine)
{
    hi_s32 ret;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_config_aes cipher_ctrl_aes;

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    cipher_ctrl.alg = alg;
    cipher_ctrl.work_mode = mode;
    cipher_ctrl_aes.bit_width = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
    cipher_ctrl_aes.key_len = key_len;
    if (cipher_ctrl.work_mode != HI_UNF_CIPHER_WORK_MODE_ECB) {
		cipher_ctrl_aes.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ALL_PKG;
		memcpy(cipher_ctrl_aes.iv, iv, ivlen);
	}
    cipher_ctrl.param = &cipher_ctrl_aes;
    

    ret = hi_unf_cipher_set_config(cipher, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("hi_unf_cipher_set_config failed\n");
        return ret;
    }

    ret = cipher_set_clear_key(cipher, enEngine, key, klen);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 multi_cipher_ex_aes_cbc_enc()
{
    hi_s32 ret;
    hi_u8 *input_addr_vir[SAMPLE_MULTI_PACKAGE] = { 0 };
    hi_u8 *output_addr_vir[SAMPLE_MULTI_PACKAGE] = { 0 };
    hi_handle handle = 0;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_data cipher_data_array[SAMPLE_MULTI_PACKAGE];
    hi_u32 i;

    memset(&cipher_data_array, 0, sizeof(cipher_data_array));

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher init failed.\n");
        (hi_void)hi_unf_sys_deinit();
        return HI_FAILURE;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher create handle failed.\n");
        goto __CIPHER_EXIT__;
    }

    for (i = 0; i < SAMPLE_MULTI_PACKAGE; i++) {
        cipher_data_array[i].src_buf.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in") + 1, SAMPLE_AES_DATA_LEN, 0);
        if (cipher_data_array[i].src_buf.mem_handle == 0) {
            HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
        input_addr_vir[i] = hi_unf_mem_map(cipher_data_array[i].src_buf.mem_handle, SAMPLE_AES_DATA_LEN);

        cipher_data_array[i].dest_buf.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in") + 1, SAMPLE_AES_DATA_LEN, 0);
        if (cipher_data_array[i].dest_buf.mem_handle == 0) {
            HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
        output_addr_vir[i] = hi_unf_mem_map(cipher_data_array[i].dest_buf.mem_handle, SAMPLE_AES_DATA_LEN);

        cipher_data_array[i].byte_length = SAMPLE_AES_DATA_LEN;
        memset(input_addr_vir[i], 0x0, SAMPLE_AES_DATA_LEN);
        memcpy(input_addr_vir[i], aes_cbc_enc_src_buf, SAMPLE_AES_DATA_LEN);
        memset(output_addr_vir[i], 0x0, SAMPLE_AES_DATA_LEN);
    }

    ret = cipher_set_config_info(handle, HI_UNF_CIPHER_ALG_AES, HI_UNF_CIPHER_WORK_MODE_CBC,
        HI_UNF_CIPHER_KEY_AES_128BIT, aes_key, SAMPLE_AES_KEY_LEN, aes_iv, SAMPLE_AES_IV_LEN,
        HI_UNF_CRYPTO_ALG_RAW_AES);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher cipher_set_config_info failed.\n");
        goto __CIPHER_EXIT__;
    }

    ret = hi_unf_cipher_encrypt_multi(handle, cipher_data_array, SAMPLE_MULTI_PACKAGE);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt_multi failed.\n");
        goto __CIPHER_EXIT__;
    }

    for (i = 0; i < SAMPLE_MULTI_PACKAGE; i++) {
        if (memcmp(output_addr_vir[i], aes_cbc_enc_dst_buf, SAMPLE_AES_DATA_LEN != 0)) {
            HI_ERR_CIPHER("multi_cipher AES CBC encryption run failed on array %d!\n", i);
            print_buffer("ENC", output_addr_vir[i], SAMPLE_AES_DATA_LEN);
            print_buffer("GOLD", aes_cbc_enc_dst_buf, SAMPLE_AES_DATA_LEN);
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
    }

    HI_INFO_CIPHER("multi_cipher AES CBC encryption run success!\n");

__CIPHER_EXIT__:

    for (i = 0; i < SAMPLE_MULTI_PACKAGE; i++) {
        if (cipher_data_array[i].src_buf.mem_handle > 0) {
            hi_unf_mem_unmap(input_addr_vir[i], SAMPLE_AES_DATA_LEN);
            hi_unf_mem_delete(cipher_data_array[i].src_buf.mem_handle);
        }
        if (cipher_data_array[i].dest_buf.mem_handle > 0) {
            hi_unf_mem_unmap(output_addr_vir[i], SAMPLE_AES_DATA_LEN);
            hi_unf_mem_delete(cipher_data_array[i].dest_buf.mem_handle);
        }
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return ret;
}

hi_s32 multi_cipher_ex_aes_cbc_dec()
{
    hi_s32 ret;
    hi_u8 *input_addr_vir[SAMPLE_MULTI_PACKAGE] = { 0 };
    hi_u8 *output_addr_vir[SAMPLE_MULTI_PACKAGE] = { 0 };
    hi_handle handle = 0;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_data cipher_data_array[SAMPLE_MULTI_PACKAGE];
    hi_u32 i;

    memset(&cipher_data_array, 0, sizeof(cipher_data_array));

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher init failed.\n");
        (hi_void)hi_unf_sys_deinit();
        return HI_FAILURE;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher Create handle failed.\n");
        goto __CIPHER_EXIT__;
    }

    for (i = 0; i < SAMPLE_MULTI_PACKAGE; i++) {
        cipher_data_array[i].src_buf.mem_handle = hi_unf_mem_new("cipher_in", strlen("cipher_in") + 1, SAMPLE_AES_DATA_LEN, 0);
        if (cipher_data_array[i].src_buf.mem_handle == 0) {
            HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
        input_addr_vir[i] = hi_unf_mem_map(cipher_data_array[i].src_buf.mem_handle, SAMPLE_AES_DATA_LEN);

        cipher_data_array[i].dest_buf.mem_handle = hi_unf_mem_new("cipher_out", strlen("cipher_out") + 1, SAMPLE_AES_DATA_LEN, 0);
        if (cipher_data_array[i].dest_buf.mem_handle == 0) {
            HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
        output_addr_vir[i] = hi_unf_mem_map(cipher_data_array[i].dest_buf.mem_handle, SAMPLE_AES_DATA_LEN);

        cipher_data_array[i].byte_length = SAMPLE_AES_DATA_LEN;
        memset(input_addr_vir[i], 0x0, SAMPLE_AES_DATA_LEN);
        memcpy(input_addr_vir[i], aes_cbc_dec_src_buf, SAMPLE_AES_DATA_LEN);
        memset(output_addr_vir[i], 0x0, SAMPLE_AES_DATA_LEN);
    }

    ret = cipher_set_config_info(handle,
                       HI_UNF_CIPHER_ALG_AES,
                       HI_UNF_CIPHER_WORK_MODE_CBC,
                       HI_UNF_CIPHER_KEY_AES_128BIT,
                       aes_key, SAMPLE_AES_KEY_LEN,
                       aes_iv, SAMPLE_AES_IV_LEN,
                       HI_UNF_CRYPTO_ALG_RAW_AES);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher cipher_set_config_info failed.\n");
        goto __CIPHER_EXIT__;
    }

    ret = hi_unf_cipher_decrypt_multi(handle, cipher_data_array, SAMPLE_MULTI_PACKAGE);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt_multi failed.\n");
        goto __CIPHER_EXIT__;
    }

    for (i = 0; i < SAMPLE_MULTI_PACKAGE; i++) {
        if (memcmp(output_addr_vir[i], aes_cbc_dec_dst_buf, SAMPLE_AES_DATA_LEN != 0)) {
            HI_ERR_CIPHER("multi_cipher AES CBC decryption run failed!\n");
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
    }

    HI_INFO_CIPHER("multi_cipher AES CBC decryption run success!\n");

__CIPHER_EXIT__:

    for (i = 0; i < SAMPLE_MULTI_PACKAGE; i++) {
        if (cipher_data_array[i].src_buf.mem_handle > 0) {
            hi_unf_mem_unmap(input_addr_vir[i], SAMPLE_AES_DATA_LEN);
            hi_unf_mem_delete(cipher_data_array[i].src_buf.mem_handle);
        }
        if (cipher_data_array[i].dest_buf.mem_handle > 0) {
            hi_unf_mem_unmap(output_addr_vir[i], SAMPLE_AES_DATA_LEN);
            hi_unf_mem_delete(cipher_data_array[i].dest_buf.mem_handle);
        }
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return ret;
}

int main(int argc, char *argv[])
{
    hi_s32 ret;

    ret = multi_cipher_ex_aes_cbc_enc();
    if (ret != HI_SUCCESS) {
        HI_INFO_CIPHER("multi_cipher_ex_aes_cbc_enc run failure.\n");
        return HI_FAILURE;
    }

    ret = multi_cipher_ex_aes_cbc_dec();
    if (ret != HI_SUCCESS) {
        HI_INFO_CIPHER("multi_cipher_ex_aes_cbc_dec run failure.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

