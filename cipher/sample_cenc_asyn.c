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
#include <semaphore.h>
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

static sem_t  g_cenc_sem;
static hi_u32 g_count = 0;

typedef struct hicenc_user_data {
    hi_mem_handle input_addr_phy;
    hi_mem_handle output_addr_phy;
    hi_u8 *input_addr_vir;
    hi_u8 *output_addr_vir;
    hi_u8 *gold_data;
    hi_u32 dat_len;
    hi_u32 id;
    hi_u32 total;
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

    key_clear.odd = 1;
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
    user_data->dat_len = length;

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

hi_void cenc_done_notify_func(hi_handle cipher, hi_s32 result,
                              hi_void *user_data, hi_u32 user_data_len)
{
    hi_s32 ret;
    cenc_user_data *cenc_data = HI_NULL;
    hi_u32 id;
    hi_u32 total;

    if ((user_data == HI_NULL) || (user_data_len != sizeof(cenc_user_data))) {
        HI_ERR_CIPHER("cenc dec done, invalid user data\n");
        return;
    }

    cenc_data = user_data;
    id = cenc_data->id;
    total = cenc_data->total;

    HI_PRINT("cenc done(%d/%d) length 0x%x, result 0x%x, in_handle 0x%llu, out_handle 0x%llu\n",
             id, total, cenc_data->dat_len, result, cenc_data->input_addr_phy.mem_handle, cenc_data->output_addr_phy.mem_handle);

    if (result == HI_SUCCESS) {
        g_count++;
    }

    if (cenc_data->dat_len <= MAX_SUBSAMPLE_DATA_LEN) {
        if (memcmp(cenc_data->output_addr_vir, cenc_data->gold_data, cenc_data->dat_len) == 0) {
            TEST_END_PASS(id);
        } else {
            print_buffer("CENC-DEC:", cenc_data->output_addr_vir, cenc_data->dat_len);
            HI_ERR_CIPHER("cipher cenc decrypt, memcmp failed!\n");
        }
    } else {
        TEST_END_PASS(id);
    }

    user_data_deinit(cenc_data, cenc_data->dat_len);
    free(user_data);
    user_data = HI_NULL;

    ret = sem_post(&g_cenc_sem);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("sem_post failed, ret 0x%x!\n", ret);
        return;
    }

    return;
}

/* encrypt data using special chn */
hi_s32 sample_cenc(hi_u32 id, hi_handle handle, cenc_gold_s *sample_data)
{
    hi_s32 ret;
    hi_unf_cipher_cenc_param cenc;
    hi_unf_cipher_done_callback symc_done;
    cenc_user_data *user_data = HI_NULL;
    hi_unf_cenc_decrypt_data cenc_decrypt_data;

    cenc.use_odd_key = 1;
    cenc.first_encrypt_offset = sample_data->first_encrypt_offset;
    cenc.subsample = sample_data->subsample;
    cenc.subsample_num = sample_data->subsample_num;
    user_data = malloc(sizeof(cenc_user_data));
    if (user_data == HI_NULL) {
        HI_ERR_CIPHER("error: malloc failed!\n");
        return HI_FAILURE;
    }

    ret = user_data_init(user_data, sample_data->length);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    ret = cipher_set_config_info(handle, sample_data->alg, sample_data->mode,
                                 sample_data->key, sample_data->iv);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    if (sample_data->length <= MAX_SUBSAMPLE_DATA_LEN) {
        memcpy(user_data->input_addr_vir, sample_data->enc, sample_data->length);
    }

    symc_done.symc_done_func = cenc_done_notify_func;
    symc_done.user_data_len = sizeof(cenc_user_data);
    symc_done.user_data = user_data;
    user_data->id = id;
    user_data->gold_data = sample_data->dec;
    user_data->total = sizeof(cenc_sample_data) / sizeof(cenc_sample_data[0]);

    cenc_decrypt_data.src_buf = user_data->input_addr_phy;
    cenc_decrypt_data.dest_buf = user_data->output_addr_phy;
    cenc_decrypt_data.byte_length = sample_data->length;
    cenc_decrypt_data.symc_done = &symc_done;
    ret = hi_unf_cipher_cenc_decrypt(handle, &cenc, &cenc_decrypt_data);
    if (ret != HI_SUCCESS) {
        goto __CIPHER_EXIT__;
    }

    return HI_SUCCESS;

__CIPHER_EXIT__:

    user_data_deinit(user_data, sample_data->length);
    free(user_data);
    user_data = HI_NULL;

    return ret;
}

hi_s32 cipher_cenc_trywait(hi_u32 id, cenc_gold_s *sample_data)
{
    hi_s32 ret;
    static hi_u8 key[SAMPLE_KEY_LEN] = { 0 };
    static hi_unf_cipher_alg alg = HI_UNF_CIPHER_ALG_AES;
    static hi_unf_cipher_work_mode mode = HI_UNF_CIPHER_WORK_MODE_CTR;
    hi_s32 value = 0;
    hi_u32 wait = HI_FALSE;

    if ((sample_data->alg != alg) || (sample_data->mode != mode)) {
        alg = sample_data->alg;
        mode = sample_data->mode;
        wait = HI_TRUE;
    }
    if (memcmp(sample_data->key, key, SAMPLE_KEY_LEN) != 0) {
        memcpy(key, sample_data->key, SAMPLE_KEY_LEN);
        wait = HI_TRUE;
    }

    /* wait all sample finished */
    if (wait == HI_TRUE) {
        while (value != id) {
            ret = sem_getvalue(&g_cenc_sem, &value);
            if (ret != HI_SUCCESS) {
                HI_ERR_CIPHER("cipher sem_getvalue failed.\n");
                return ret;
                ;
            }
            usleep(1);
        }
    }

    return HI_SUCCESS;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_handle handle;
    hi_unf_cipher_attr cipher_attr;
    hi_u32 id = 0;

    ret = sem_init(&g_cenc_sem, 0, 0);
    if (ret != HI_SUCCESS) {
        HI_ERR_CIPHER("sem_init failed, ret 0x%x!\n", ret);
        return ret;
    }

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        sem_destroy(&g_cenc_sem);
        return ret;
    }

    cipher_attr.cipher_type = HI_UNF_CIPHER_TYPE_NORMAL;
    ret = hi_unf_cipher_create(&handle, &cipher_attr);
    if (ret != HI_SUCCESS) {
        sem_destroy(&g_cenc_sem);
        hi_unf_cipher_deinit();
        return ret;
    }

    for (id = 0; id < sizeof(cenc_sample_data) / sizeof(cenc_sample_data[0]); id++) {
        HI_PRINT("\n**************** sample %d start ********************\n", id);
        ret = cipher_cenc_trywait(id, &cenc_sample_data[id]);
        if (ret != HI_SUCCESS) {
            goto __CIPHER_EXIT__;
        }

        ret = sample_cenc(id, handle, &cenc_sample_data[id]);
        if (ret != HI_SUCCESS) {
            HI_ERR_CIPHER("sem_wait failed.\n");
            goto __CIPHER_EXIT__;
        }
    }

    /* wait all sample finished */
    while (id--) {
        ret = sem_wait(&g_cenc_sem);
        if (ret != HI_SUCCESS) {
            HI_ERR_CIPHER("cipher sem_wait failed.\n");
            goto __CIPHER_EXIT__;
        }
    }
    id = sizeof(cenc_sample_data) / sizeof(cenc_sample_data[0]);

    HI_PRINT("\n**********************************************************\n");
    HI_PRINT("             CENC test end, success %d, faile %d\n", g_count, id - g_count);
    HI_PRINT("\n**********************************************************\n");

__CIPHER_EXIT__:

    hi_unf_cipher_destroy(handle);
    hi_unf_cipher_deinit();
    sem_destroy(&g_cenc_sem);

    return ret;
}

