/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_cenc
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
#include "hi_unf_klad.h"
#include "hi_adp.h"
#include "sample_cenc.h"

#define HI_ERR_CIPHER(format, arg...)  HI_PRINT("\033[0;1;31m" format "\033[0m", ##arg)
#define HI_INFO_CIPHER(format, arg...) HI_PRINT("\033[0;1;32m" format "\033[0m", ##arg)
#define TEST_END_PASS(id)              \
    HI_INFO_CIPHER("****************** sample %d test PASS !!! ******************\n", id)
#define TEST_END_FAIL(id) \
    HI_ERR_CIPHER("****************** sample %d test FAIL !!! ******************\n", id)
#define TEST_RESULT_PRINT(id)  {                          \
        if (ret) { \
            TEST_END_FAIL(id); \
        else                   \
            TEST_END_PASS(id); \
    }
#define U32_TO_POINT(addr) ((hi_void *)((HI_SIZE_T)(addr)))
#define POINT_TO_U32(addr) ((hi_u32)((HI_SIZE_T)(addr)))

#define SAMPLE_KEY_LEN   16
#define SAMPLE_IV_LEN    16
#define SAMPLE_LINE_LEN  16

typedef struct hicenc_user_data {
    hi_mem_handle input_addr_phy;
    hi_mem_handle output_addr_phy;
    hi_u8 *input_addr_vir;
    hi_u8 *output_addr_vir;
} cenc_user_data;

static hi_void print_buffer(char *string, hi_u8 *input, hi_u32 length)
{
    hi_u32 i = 0;

    if (string != NULL) {
        printf("%s\n", string);
    }

    for (i = 0; i < length; i++) {
        if ((i % SAMPLE_LINE_LEN == 0) && (i != 0)) {
            printf("\n");
        }
        printf("0x%02x ", input[i]);
    }
    printf("\n");

    return;
}

static hi_void clear_key_init(hi_unf_klad_attr *attr_klad, hi_unf_crypto_alg engine)
{
    attr_klad->klad_cfg.owner_id = 0;
    attr_klad->klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad->key_cfg.decrypt_support = 1;
    attr_klad->key_cfg.encrypt_support = 1;
    attr_klad->key_cfg.engine = engine;
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

    clear_key_init(&attr_klad, engine);
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
                              const hi_u8 key[SAMPLE_KEY_LEN],
                              const hi_u8 iv[SAMPLE_IV_LEN])
{
    hi_s32 ret;
    hi_unf_cipher_config cipher_ctrl;
    hi_unf_cipher_config_aes cipher_ctrl_aes;
    hi_unf_cipher_alg engine = HI_UNF_CRYPTO_ALG_RAW_AES;

    memset(&cipher_ctrl, 0, sizeof(hi_unf_cipher_config));
    memset(&cipher_ctrl_aes, 0, sizeof(hi_unf_cipher_config_aes));
    cipher_ctrl.alg = alg;
    cipher_ctrl.work_mode = mode;
    cipher_ctrl_aes.bit_width = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
    cipher_ctrl_aes.key_len = HI_UNF_CIPHER_KEY_AES_128BIT;
    cipher_ctrl_aes.change_flags.iv_change_flag = HI_UNF_CIPHER_IV_CHANGE_ONE_PKG;
    memcpy(cipher_ctrl_aes.iv, iv, SAMPLE_IV_LEN);
    cipher_ctrl.param = &cipher_ctrl_aes;

    ret = hi_unf_cipher_set_config(cipher, &cipher_ctrl);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("hi_unf_cipher_set_config failed\n");
        return ret;
    }

    if (alg == HI_UNF_CIPHER_ALG_SM4) {
        engine = HI_UNF_CRYPTO_ALG_RAW_SM4;
    }

    ret = cipher_set_clear_key(cipher, engine, key, SAMPLE_KEY_LEN);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("cipher_set_clear_key failed\n");
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 user_data_init(cenc_user_data *user_data, hi_u32 length)
{
    user_data->input_addr_phy.mem_handle = hi_unf_mem_new("cipher_buf_in", strlen("cipher_buf_in") + 1, length, 0);
    if (user_data->input_addr_phy.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for input failed!\n");
        return HI_FAILURE;
    }
    user_data->input_addr_vir = hi_unf_mem_map(user_data->input_addr_phy.mem_handle, length);
    user_data->output_addr_phy.mem_handle = hi_unf_mem_new("cipher_buf_out", strlen("cipher_buf_out") + 1, length, 0);
    if (user_data->output_addr_phy.mem_handle == 0) {
        HI_ERR_CIPHER("error: get bufhandle for out_put failed!\n");
        hi_unf_mem_unmap(user_data->input_addr_vir, length);
        hi_unf_mem_delete(user_data->input_addr_phy.mem_handle);
        return HI_FAILURE;
    }
    user_data->output_addr_vir = hi_unf_mem_map(user_data->output_addr_phy.mem_handle, length);

    return HI_SUCCESS;
}

static hi_void user_data_deinit(cenc_user_data *user_data, hi_u32 length)
{

    if (user_data->input_addr_phy.mem_handle > 0) {
        hi_unf_mem_unmap(user_data->input_addr_vir, length);
        hi_unf_mem_delete(user_data->input_addr_phy.mem_handle);
    }

    if (user_data->output_addr_phy.mem_handle > 0) {
        hi_unf_mem_unmap(user_data->output_addr_vir, length);
        hi_unf_mem_delete(user_data->output_addr_phy.mem_handle);
    }

    return;
}

/* encrypt data using special chn */
hi_s32 sample_cenc(hi_u32 id, hi_handle handle, cenc_gold_s *sample_data)
{
    hi_s32 ret;
    hi_unf_cipher_cenc_param cenc;
    cenc_user_data user_data;
    hi_unf_cenc_decrypt_data cenc_decrypt_data;

    cenc.use_odd_key = 0;
    cenc.first_encrypt_offset = sample_data->first_encrypt_offset;
    cenc.subsample = sample_data->subsample;
    cenc.subsample_num = sample_data->subsample_num;

    //user_data.input_addr_phy  = 0;
    //user_data.output_addr_phy = 0;
    memset(&user_data.input_addr_phy, 0, sizeof(hi_mem_handle));
    memset(&user_data.output_addr_phy, 0, sizeof(hi_mem_handle));
    ret = user_data_init(&user_data, sample_data->length);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    ret = cipher_set_config_info(handle, sample_data->alg, sample_data->mode,
                                 sample_data->key, sample_data->iv);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    if (sample_data->length <= MAX_SUBSAMPLE_DATA_LEN) {
        memcpy(user_data.input_addr_vir, sample_data->enc, sample_data->length);
    }

    cenc_decrypt_data.src_buf = user_data.input_addr_phy;
    cenc_decrypt_data.dest_buf = user_data.output_addr_phy;
    cenc_decrypt_data.byte_length = sample_data->length;
    cenc_decrypt_data.symc_done = HI_NULL;
    ret = hi_unf_cipher_cenc_decrypt(handle, &cenc, &cenc_decrypt_data);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    if (sample_data->length <= MAX_SUBSAMPLE_DATA_LEN) {
        if (memcmp(user_data.output_addr_vir, sample_data->dec, sample_data->length) != 0) {
            print_buffer("CENC-ORI:", sample_data->enc, sample_data->length);
            print_buffer("CENC-DEC:", user_data.output_addr_vir, sample_data->length);
            HI_ERR_CIPHER("cipher cenc decrypt, memcmp failed!\n");
            ret = HI_FAILURE;
            goto __CIPHER_EXIT__;
        }
    }

    TEST_END_PASS(id);

__CIPHER_EXIT__:

    user_data_deinit(&user_data, sample_data->length);

    return ret;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_handle handle;
    hi_unf_cipher_attr cipher_attr;
    hi_u32 id = 0;
    hi_u32 count = 0;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        return ret;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        hi_unf_cipher_deinit();
        return ret;
    }

    for (id = 0; id < sizeof(cenc_sample_data) / sizeof(cenc_sample_data[0]); id++) {
        HI_PRINT("\n**************** sample %d start ********************\n", id);
        ret = sample_cenc(id, handle, &cenc_sample_data[id]);
        if (ret == HI_SUCCESS) {
            count++;
        }
    }

    HI_PRINT("\n**********************************************************\n");
    HI_PRINT("             CENC test end, success %d, faile %d\n", count, id - count);
    HI_PRINT("\n**********************************************************\n");

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

