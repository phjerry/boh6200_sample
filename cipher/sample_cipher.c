/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_cipher
 * Author: zhaoguihong
 * Create: 2019-06-18
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_cipher.h"
#include "hi_unf_klad.h"
#include "hi_adp.h"

#define HI_ERR_CIPHER(format, arg...)  HI_PRINT("\033[0;1;31m" format "\033[0m", ##arg)
#define HI_INFO_CIPHER(format, arg...) HI_PRINT("\033[0;1;32m" format "\033[0m", ##arg)
#define TEST_END_PASS()                \
    HI_INFO_CIPHER("****************** %s test PASS !!! ******************\n", __FUNCTION__)
#define TEST_END_FAIL() \
    HI_ERR_CIPHER("****************** %s test FAIL !!! ******************\n", __FUNCTION__)
#define TEST_RESULT_PRINT() do { \
        if (ret) {            \
            TEST_END_FAIL();  \
        }                     \
        else {                \
            TEST_END_PASS();  \
        } \
    } while (0)

#define SAMPLE_KEY_LEN     16
#define SAMPLE_IV_LEN      16
#define SMMU_PAGE_SIZE     (4 * 1024)

static hi_void print_buffer(char *string, hi_u8 *input, hi_u32 length)
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

hi_s32 cipher_set_config_info(hi_handle cipher, hi_unf_cipher_alg alg,
                     hi_unf_cipher_work_mode mode, hi_unf_cipher_key_length key_len,
                     const hi_u8 key_buf[SAMPLE_KEY_LEN], const hi_u8 iv_buf[SAMPLE_IV_LEN])
{
    hi_s32 ret;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_config_aes cipher_ctrl_aes;

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    memset(&cipher_ctrl_aes, 0, sizeof(hi_unf_cipher_config_aes));
    cipher_ctrl.alg = alg;
    cipher_ctrl.work_mode = mode;
    cipher_ctrl_aes.bit_width = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
    cipher_ctrl_aes.key_len = key_len;
    if (cipher_ctrl.work_mode != HI_UNF_CIPHER_WORK_MODE_ECB) {
        cipher_ctrl_aes.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ONE_PKG;
        memcpy(cipher_ctrl_aes.iv, iv_buf, SAMPLE_IV_LEN);
    }
    cipher_ctrl.param = &cipher_ctrl_aes;

    ret = hi_unf_cipher_set_config(cipher, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("hi_unf_cipher_set_config failed\n");
        return ret;
    }

    ret = cipher_set_clear_key(cipher, HI_UNF_CRYPTO_ALG_RAW_AES, key_buf, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return ret;
    }

    return HI_SUCCESS;
}

/* encrypt data using special chn */
hi_void aes_cbc_128(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 16;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_u32 testcached = 0;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_unf_cipher_attr cipher_attr;

    hi_u8 aes_key[16] = {0x2B,0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
    hi_u8 aes_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    hi_u8 aes_src[16] ={0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96, 0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A};
    hi_u8 aes_dst[16] ={0x76, 0x49, 0xAB, 0xAC, 0x81, 0x19, 0xB2, 0x46, 0xCE, 0xE9, 0x8E, 0x9B, 0x12, 0xE9, 0x19, 0x7D};

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }

    input_buf_handle.addr_offset = SMMU_PAGE_SIZE;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in")+1, SMMU_PAGE_SIZE + data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, SMMU_PAGE_SIZE + data_len);
    output_buf_handle.addr_offset = SMMU_PAGE_SIZE;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, SMMU_PAGE_SIZE + data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, SMMU_PAGE_SIZE + data_len);

    /* for encrypt */
    ret = cipher_set_config_info(handle,
                        HI_UNF_CIPHER_ALG_AES,
                        HI_UNF_CIPHER_WORK_MODE_CBC,
                        HI_UNF_CIPHER_KEY_AES_128BIT,
                        aes_key,
                        aes_iv);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    memset(input_addr_vir, 0x0, SMMU_PAGE_SIZE + data_len);
    memcpy(input_addr_vir + SMMU_PAGE_SIZE, aes_src, data_len);
    print_buffer("CBC-AES-128-ORI:", aes_src, sizeof(aes_src));

    memset(output_addr_vir, 0x0, SMMU_PAGE_SIZE + data_len);
    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CBC-AES-128-ENC:", output_addr_vir + SMMU_PAGE_SIZE, sizeof(aes_dst));

    /* compare */
    if (0 != memcmp(output_addr_vir + SMMU_PAGE_SIZE, aes_dst, data_len)) {
        HI_ERR_CIPHER("cipher encrypt, memcmp failed!\n");
        ret = HI_FAILURE;

        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    memcpy(input_addr_vir + SMMU_PAGE_SIZE, aes_dst, data_len);
    memset(output_addr_vir, 0x0, SMMU_PAGE_SIZE + data_len);

    ret = cipher_set_config_info(handle,
                        HI_UNF_CIPHER_ALG_AES,
                        HI_UNF_CIPHER_WORK_MODE_CBC,
                        HI_UNF_CIPHER_KEY_AES_128BIT,
                        aes_key,
                        aes_iv);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }
    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CBC-AES-128-DEC:", output_addr_vir + SMMU_PAGE_SIZE, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir + SMMU_PAGE_SIZE, aes_src, data_len)) {
        HI_ERR_CIPHER("cipher decrypt, memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, SMMU_PAGE_SIZE + data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, SMMU_PAGE_SIZE + data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return;
}

hi_void aes_cfb_128(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 32;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_u32 testcached = 0;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_unf_cipher_attr cipher_attr;
    hi_u8 aes_key[16] = { "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c" };
    hi_u8 aes_iv[16] =  { "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f" };
    hi_u8 aes_src[32] = { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a"
                          "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51" };
    hi_u8 aes_dst[32] = { "\x3b\x3f\xd9\x2e\xb7\x2d\xad\x20\x33\x34\x49\xf8\xe8\x3c\xfb\x4a"
                          "\xc8\xa6\x45\x37\xa0\xb3\xa9\x3f\xcd\xe3\xcd\xad\x9f\x1c\xe5\x8b" };

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }
    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }
    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in")+1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);
    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    /* for encrypt */
    ret = cipher_set_config_info(handle,
                        HI_UNF_CIPHER_ALG_AES,
                        HI_UNF_CIPHER_WORK_MODE_CFB,
                        HI_UNF_CIPHER_KEY_AES_128BIT,
                        aes_key,
                        aes_iv);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, aes_src, data_len);
    print_buffer("CFB-AES-128-ORI:", aes_src, data_len);

    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CFB-AES-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, aes_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    memcpy(input_addr_vir, aes_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = cipher_set_config_info(handle,
                       HI_UNF_CIPHER_ALG_AES,
                       HI_UNF_CIPHER_WORK_MODE_CFB,
                       HI_UNF_CIPHER_KEY_AES_128BIT,
                       aes_key,
                       aes_iv);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CFB-AES-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, aes_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return;
}

hi_void aes_ctr_128(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 32;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_u32 testcached = 0;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_unf_cipher_attr cipher_attr;
    hi_u8 aes_key[16] = { "\x7e\x24\x06\x78\x17\xfa\xe0\xd7\x43\xd6\xce\x1f\x32\x53\x91\x63" };
    hi_u8 aes_iv[16] =  { "\x00\x6c\xb6\xdb\xc0\x54\x3b\x59\xda\x48\xd9\x0b\x00\x00\x00\x01" };
    hi_u8 aes_src[32] = { "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
                          "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f" };
    hi_u8 aes_dst[32] = {  "\x51\x04\xa1\x06\x16\x8a\x72\xd9\x79\x0d\x41\xee\x8e\xda\xd3\x88"
                          "\xeb\x2e\x1e\xfc\x46\xda\x57\xc8\xfc\xe6\x30\xdf\x91\x41\xbe\x28" };

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }
    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }
    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in")+1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);
    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    /* for encrypt */
    ret = cipher_set_config_info(handle,
                       HI_UNF_CIPHER_ALG_AES,
                       HI_UNF_CIPHER_WORK_MODE_CTR,
                       HI_UNF_CIPHER_KEY_AES_128BIT,
                       aes_key,
                       aes_iv);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, aes_src, data_len);
    print_buffer("CTR-AES-128-ORI:", aes_src, data_len);

    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CTR-AES-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, aes_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    memcpy(input_addr_vir, aes_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = cipher_set_config_info(handle,
                       HI_UNF_CIPHER_ALG_AES,
                       HI_UNF_CIPHER_WORK_MODE_CTR,
                       HI_UNF_CIPHER_KEY_AES_128BIT,
                       aes_key,
                       aes_iv);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CTR-AES-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, aes_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return;
}

hi_void sm4_cbc_128(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 32;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_u32 testcached = 0;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_config ctrl;
    hi_unf_cipher_config_sm4 sm4_param;
    hi_u8 sm4_key[16] = { "\x20\xee\xdd\xc8\x4b\x52\xad\x17\xbf\x85\x65\x3b\x33\xc3\xa3\xe5" };
    hi_u8 sm4_iv[16] =  { "\x29\xa3\x6c\xbd\x4d\x43\x94\xf9\x2a\xce\xa6\x71\xc1\x00\xbd\x1b" };
    hi_u8 sm4_src[32] = { "\xd6\xec\x40\x8d\x54\x27\xc4\x0e\x21\xd9\x91\xad\x5a\xa3\xc7\x71"
                          "\x2c\x85\x59\x5d\x30\x72\xde\x74\xf5\x6c\x1b\x4f\xe0\x2f\xae\xca" };
    hi_u8 sm4_dst[32] = {  "\xc3\x34\x6b\xaa\x1d\x5d\xd1\x70\xaa\xd7\x67\x95\xdc\xb1\x99\x90"
                          "\xb9\x69\xd3\xf3\x3d\xba\xb6\xbe\x85\xe9\x1c\x8c\x7d\xb3\x2c\xb9" };

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }
    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }
    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in")+1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);
    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    ctrl.alg = HI_UNF_CIPHER_ALG_SM4;
    ctrl.work_mode = HI_UNF_CIPHER_WORK_MODE_CBC;
    ctrl.param = &sm4_param;
    sm4_param.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ONE_PKG;
    memcpy(sm4_param.iv, sm4_iv, 16);

    /* for encrypt */
    ret = hi_unf_cipher_set_config(handle, &ctrl);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    ret = cipher_set_clear_key(handle, HI_UNF_CRYPTO_ALG_RAW_SM4, sm4_key, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, sm4_src, data_len);
    print_buffer("CBC-SM4-128-ORI:", sm4_src, data_len);

    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CBC-SM4-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, sm4_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    memcpy(input_addr_vir, sm4_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_set_config(handle, &ctrl);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CBC-SM4-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, sm4_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return;
}

/* *** klen  = 128, tlen =32, nlen  = 56,  alen  = 64, and  plen  = 32 *** */
/* *** t = 4, q=8, p=4 *** */
hi_void aes_ccm_128(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 4;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_mem_handle aad_buf_handle;
    hi_u32 testcached = 0;
    hi_u32 tag_len;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_u8 *aad_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_config_aes_ccm_gcm ctrl;
    hi_u8 aes_key[16] = { "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f" };
    hi_u8 aes_n[7] = { "\x10\x11\x12\x13\x14\x15\x16" };
    hi_u8 aes_a[8] = { "\x00\x01\x02\x03\x04\x05\x06\x07" };
    hi_u8 aes_src[4] = { "\x20\x21\x22\x23" };
    hi_u8 aes_dst[4] = { "\x71\x62\x01\x5b" };
    hi_u8 aes_tag[4] = { "\x4d\xac\x25\x5d" };
    hi_u8 out_tag[4];

    printf("\n--------------------------%s-----------------------\n", __FUNCTION__);

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }

    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in") + 1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);
    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    aad_buf_handle.addr_offset = 0;
    aad_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_aad", strlen("cipher_buf_aad") + 1, sizeof(aes_a), testcached);
    if (aad_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for AAD failed!\n");
        goto __CIPHER_EXIT__;
    }
    aad_addr_vir = hi_unf_mem_map(aad_buf_handle.mem_handle, sizeof(aes_a));

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    cipher_ctrl.alg = HI_UNF_CIPHER_ALG_AES;
    cipher_ctrl.work_mode = HI_UNF_CIPHER_WORK_MODE_CCM;
    ctrl.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
    ctrl.iv_len = sizeof(aes_n);
    ctrl.tag_len = sizeof(aes_tag);
    ctrl.a_len = sizeof(aes_a);
    ctrl.a_buf_handle = aad_buf_handle;
    memcpy(ctrl.iv, aes_n, sizeof(aes_n));
    memcpy(aad_addr_vir, aes_a, sizeof(aes_a));
    cipher_ctrl.param = &ctrl;
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    ret = cipher_set_clear_key(handle, HI_UNF_CRYPTO_ALG_RAW_AES, aes_key, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, aes_src, data_len);
    print_buffer("CCM-AES-128-ORI:", aes_src, data_len);
    memset(output_addr_vir, 0x0, data_len);
    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    print_buffer("CCM-AES-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, aes_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("CCM-AES-128-TAG", out_tag, sizeof(aes_tag));
    if (0 != memcmp(out_tag, aes_tag, sizeof(aes_tag))) {
        HI_ERR_CIPHER("tag compare failed!\n");
        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    memcpy(input_addr_vir, aes_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("CCM-AES-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, aes_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("CCM-AES-128-TAG", out_tag, sizeof(aes_tag));
    if (0 != memcmp(out_tag, aes_tag, sizeof(aes_tag))) {
        HI_ERR_CIPHER("tag compare failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }
    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }
    if (aad_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(aad_addr_vir, sizeof(aes_a));
        hi_unf_mem_delete(aad_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return ;
}

/******************************************************************************
 * in the following example,  klen  = 128, tlen =112,  nlen  = 104,
 * alen  = 524288, and  plen  = 256.
 ******************************************************************************/
hi_void aes_ccm_128_2(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 32;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_mem_handle aad_buf_handle;
    hi_u32 testcached = 0;
    hi_u32 tag_len, i;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_u8 *aad_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_config_aes_ccm_gcm ctrl;
    hi_u8 aes_key[16] = { "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f" };
    hi_u8 aes_n[13] = { "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c" };
    static hi_u8 aes_a[65536];
    hi_u8 aes_src[32] = { "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
                          "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
                        };
    hi_u8 aes_dst[32] = { "\x69\x91\x5d\xad\x1e\x84\xc6\x37\x6a\x68\xc2\x96\x7e\x4d\xab\x61"
                          "\x5a\xe0\xfd\x1f\xae\xc4\x4c\xc4\x84\x82\x85\x29\x46\x3c\xcf\x72"
                        };
    hi_u8 aes_tag[14] = { "\xb4\xac\x6b\xec\x93\xe8\x59\x8e\x7f\x0d\xad\xbc\xea\x5b" };
    hi_u8 out_tag[14];

    printf("\n--------------------------%s-----------------------\n", __FUNCTION__);

    for (i = 0; i < 65536; i++) {
        aes_a[i] = i;
    }
    printf("[DEBUG] cipher init .\n");
    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }
    printf("[DEBUG] create handle .\n");
    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }
    printf("[DEBUG] alloc mem .\n");
    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in") + 1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);

    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    aad_buf_handle.addr_offset = 0;
    aad_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_aad", strlen("cipher_buf_aad") + 1, sizeof(aes_a), testcached);
    if (aad_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for AAD failed!\n");
        goto __CIPHER_EXIT__;
    }
    aad_addr_vir = hi_unf_mem_map(aad_buf_handle.mem_handle, sizeof(aes_a));

    printf("[DEBUG] set config .\n");
    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    cipher_ctrl.alg = HI_UNF_CIPHER_ALG_AES;
    cipher_ctrl.work_mode = HI_UNF_CIPHER_WORK_MODE_CCM;
    ctrl.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
    ctrl.iv_len = sizeof(aes_n);
    ctrl.tag_len = sizeof(aes_tag);
    ctrl.a_len = sizeof(aes_a);
    ctrl.a_buf_handle = aad_buf_handle;
    memcpy(ctrl.iv, aes_n, sizeof(aes_n));
    memcpy(aad_addr_vir, aes_a, sizeof(aes_a));
    cipher_ctrl.param = &ctrl;
    printf("[DEBUG] config handle ex .\n");
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }
    printf("[DEBUG] set clear key .\n");
    ret = cipher_set_clear_key(handle, HI_UNF_CRYPTO_ALG_RAW_AES, aes_key, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return;
    }
    printf("[DEBUG] begin encrypt .\n");
    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, aes_src, data_len);
    print_buffer("CCM-AES-128-ORI:", aes_src, data_len);
    memset(output_addr_vir, 0x0, data_len);
    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("CCM-AES-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, aes_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("CCM-AES-128-TAG", out_tag, sizeof(aes_tag));
    if (0 != memcmp(out_tag, aes_tag, sizeof(aes_tag))) {
        HI_ERR_CIPHER("tag compare failed!\n");
        goto __CIPHER_EXIT__;
    }

    printf("[DEBUG] begin decrypt .\n");
    /* for decrypt */
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    memcpy(input_addr_vir, aes_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("CCM-AES-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, aes_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("CCM-AES-128-TAG", out_tag, sizeof(aes_tag));
    if (0 != memcmp(out_tag, aes_tag, sizeof(aes_tag))) {
        HI_ERR_CIPHER("tag compare failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }
    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }
    if (aad_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(aad_addr_vir, sizeof(aes_a));
        hi_unf_mem_delete(aad_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return ;
}

/* klen = 128, tlen=64, nlen = 96, alen = 160, and plen = 192 */
hi_void aes_ccm_128_3(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 24;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_mem_handle aad_buf_handle;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_u8 *aad_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_u32 testcached = 0;
    hi_u32 tag_len;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_config_aes_ccm_gcm ctrl;
    hi_u8 aes_key[16] = { "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f" };
    hi_u8 aes_n[12] = { "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b" };
    hi_u8 aes_a[20] = { "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13" };
    hi_u8 aes_src[24] = { "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
                          "\x30\x31\x32\x33\x34\x35\x36\x37"
                        };
    hi_u8 aes_dst[24] = { "\xe3\xb2\x01\xa9\xf5\xb7\x1a\x7a\x9b\x1c\xea\xec\xcd\x97\xe7\x0b"
                          "\x61\x76\xaa\xd9\xa4\x42\x8a\xa5"
                        };
    hi_u8 aes_tag[8] = { "\x48\x43\x92\xfb\xc1\xb0\x99\x51" };
    hi_u8 out_tag[8];

    printf("\n--------------------------%s-----------------------\n", __FUNCTION__);

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return ;
    }

    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in")+1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);

    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    aad_buf_handle.addr_offset = 0;
    aad_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_aad", strlen("cipher_buf_aad")+1, sizeof(aes_a), testcached);
    if (aad_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for AAD failed!\n");
        goto __CIPHER_EXIT__;
    }
    aad_addr_vir = hi_unf_mem_map(aad_buf_handle.mem_handle, sizeof(aes_a));

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    cipher_ctrl.alg = HI_UNF_CIPHER_ALG_AES;
    cipher_ctrl.work_mode = HI_UNF_CIPHER_WORK_MODE_CCM;
    ctrl.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
    ctrl.iv_len= sizeof(aes_n);
    ctrl.tag_len = sizeof(aes_tag);
    ctrl.a_len = sizeof(aes_a);
    ctrl.a_buf_handle = aad_buf_handle;
    memcpy(ctrl.iv, aes_n, sizeof(aes_n));
    memcpy(aad_addr_vir, aes_a, sizeof(aes_a));
    cipher_ctrl.param = &ctrl;
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    ret = cipher_set_clear_key(handle, HI_UNF_CRYPTO_ALG_RAW_AES, aes_key, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, aes_src, data_len);
    print_buffer("CCM-AES-128-ORI:", aes_src, data_len);
    memset(output_addr_vir, 0x0, data_len);
    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("CCM-AES-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, aes_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("CCM-AES-128-TAG", out_tag, sizeof(aes_tag));
    if (0 != memcmp(out_tag, aes_tag, sizeof(aes_tag))) {
        HI_ERR_CIPHER("tag compare failed!\n");
        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    memcpy(input_addr_vir, aes_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("CCM-AES-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, aes_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("CCM-AES-128-TAG", out_tag, sizeof(aes_tag));
    if (0 != memcmp(out_tag, aes_tag, sizeof(aes_tag))) {
        HI_ERR_CIPHER("tag compare failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }
    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }
    if (aad_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(aad_addr_vir, sizeof(aes_a));
        hi_unf_mem_delete(aad_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return;
}

hi_void aes_gcm_128(hi_void)
{
    hi_s32 ret;
    hi_u32 data_len = 60;
    hi_mem_handle input_buf_handle;
    hi_mem_handle output_buf_handle;
    hi_mem_handle aad_buf_handle;
    hi_u32 testcached = 0;
    hi_u8 *input_addr_vir = HI_NULL;
    hi_u8 *output_addr_vir = HI_NULL;
    hi_u8 *aad_addr_vir = HI_NULL;
    hi_handle handle = 0;
    hi_u32 tag_len;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_attr cipher_attr;
    hi_unf_cipher_config_aes_ccm_gcm ctrl;
    hi_u8 aes_key[32] = { "\xfe\xff\xe9\x92\x86\x65\x73\x1c\x6d\x6a\x8f\x94\x67\x30\x83\x08" };
    hi_u8 aes_iv[12] = { "\xca\xfe\xba\xbe\xfa\xce\xdb\xad\xde\xca\xf8\x88" };
    hi_u8 aes_a[20] = { "\xfe\xed\xfa\xce\xde\xad\xbe\xef\xfe\xed\xfa\xce\xde\xad\xbe\xef\xab\xad\xda\xd2" };
    hi_u8 aes_src[60] = { "\xd9\x31\x32\x25\xf8\x84\x06\xe5\xa5\x59\x09\xc5\xaf\xf5\x26\x9a"
                          "\x86\xa7\xa9\x53\x15\x34\xf7\xda\x2e\x4c\x30\x3d\x8a\x31\x8a\x72"
                          "\x1c\x3c\x0c\x95\x95\x68\x09\x53\x2f\xcf\x0e\x24\x49\xa6\xb5\x25"
                          "\xb1\x6a\xed\xf5\xaa\x0d\xe6\x57\xba\x63\x7b\x39"
                        };
    hi_u8 aes_dst[60] = { "\x42\x83\x1e\xc2\x21\x77\x74\x24\x4b\x72\x21\xb7\x84\xd0\xd4\x9c"
                          "\xe3\xaa\x21\x2f\x2c\x02\xa4\xe0\x35\xc1\x7e\x23\x29\xac\xa1\x2e"
                          "\x21\xd5\x14\xb2\x54\x66\x93\x1c\x7d\x8f\x6a\x5a\xac\x84\xaa\x05"
                          "\x1b\xa3\x0b\x39\x6a\x0a\xac\x97\x3d\x58\xe0\x91"
                        };
    hi_u8 aes_tag[16] = { "\x5b\xc9\x4f\xbc\x32\x21\xa5\xdb\x94\xfa\xe9\x5a\xe7\x12\x1a\x47" };
    hi_u8 out_tag[16];

    printf("\n--------------------------%s-----------------------\n", __FUNCTION__);

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return;
    }

    input_buf_handle.addr_offset = 0;
    input_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in")+1, data_len, testcached);
    if (input_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        goto __CIPHER_EXIT__;
    }
    input_addr_vir = hi_unf_mem_map(input_buf_handle.mem_handle, data_len);

    output_buf_handle.addr_offset = 0;
    output_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, data_len, testcached);
    if (output_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        goto __CIPHER_EXIT__;
    }
    output_addr_vir = hi_unf_mem_map(output_buf_handle.mem_handle, data_len);

    aad_buf_handle.addr_offset = 0;
    aad_buf_handle.mem_handle = hi_unf_mem_new("cipher_buf_aad", strlen("cipher_buf_aad")+1, sizeof(aes_a), testcached);
    if (aad_buf_handle.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for AAD failed!\n");
        goto __CIPHER_EXIT__;
    }
    aad_addr_vir = hi_unf_mem_map(aad_buf_handle.mem_handle, sizeof(aes_a));

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    cipher_ctrl.alg = HI_UNF_CIPHER_ALG_AES;
    cipher_ctrl.work_mode = HI_UNF_CIPHER_WORK_MODE_GCM;
    ctrl.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
    ctrl.iv_len = sizeof(aes_iv);
    ctrl.tag_len = sizeof(aes_tag);
    ctrl.a_len = sizeof(aes_a);
    ctrl.a_buf_handle = aad_buf_handle;
    memcpy(ctrl.iv, aes_iv, sizeof(aes_iv));
    memcpy(aad_addr_vir, aes_a, sizeof(aes_a));
    cipher_ctrl.param = &ctrl;
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    ret = cipher_set_clear_key(handle, HI_UNF_CRYPTO_ALG_RAW_AES, aes_key, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return;
    }

    memset(input_addr_vir, 0x0, data_len);
    memcpy(input_addr_vir, aes_src, data_len);
    print_buffer("GCM-AES-128-ORI:", aes_src, data_len);
    memset(output_addr_vir, 0x0, data_len);
    ret = hi_unf_cipher_encrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher encrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("GCM-AES-128-ENC:", output_addr_vir, data_len);

    /* compare */
    if (0 != memcmp(output_addr_vir, aes_dst, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("GCM-AES-128-TAG", out_tag, 16);
    if (0 != memcmp(out_tag, aes_tag, 16)) {
        HI_ERR_CIPHER("tag compare failed!\n");
        goto __CIPHER_EXIT__;
    }

    /* for decrypt */
    ret = hi_unf_cipher_set_config(handle, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    memcpy(input_addr_vir, aes_dst, data_len);
    memset(output_addr_vir, 0x0, data_len);

    ret = hi_unf_cipher_decrypt(handle, input_buf_handle, output_buf_handle, data_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher decrypt failed.\n");
        goto __CIPHER_EXIT__;
    }

    print_buffer("GCM-AES-128-DEC", output_addr_vir, data_len);
    /* compare */
    if (0 != memcmp(output_addr_vir, aes_src, data_len)) {
        HI_ERR_CIPHER("memcmp failed!\n");
        goto __CIPHER_EXIT__;
    }

    tag_len = 16;
    ret = hi_unf_cipher_get_tag(handle, out_tag, &tag_len);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("get tag failed.\n");
        goto __CIPHER_EXIT__;
    }
    print_buffer("GCM-AES-128-TAG", out_tag, 16);
    if (0 != memcmp(out_tag, aes_tag, 16)) {
        HI_ERR_CIPHER("tag compare failed!\n");
        ret = HI_FAILURE;
        goto __CIPHER_EXIT__;
    }

    TEST_END_PASS();

__CIPHER_EXIT__:

    if (input_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(input_addr_vir, data_len);
        hi_unf_mem_delete(input_buf_handle.mem_handle);
    }
    if (output_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(output_addr_vir, data_len);
        hi_unf_mem_delete(output_buf_handle.mem_handle);
    }
    if (aad_buf_handle.mem_handle > 0) {
        hi_unf_mem_unmap(aad_addr_vir, sizeof(aes_a));
        hi_unf_mem_delete(aad_buf_handle.mem_handle);
    }

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return;
}

hi_s32 main(int argc, char *argv[])
{
    aes_cbc_128();
    aes_cfb_128();
    aes_ctr_128();
    aes_ccm_128();
    aes_ccm_128_2();
    aes_ccm_128_3();
    aes_gcm_128();
    sm4_cbc_128();
    return HI_SUCCESS;
}

