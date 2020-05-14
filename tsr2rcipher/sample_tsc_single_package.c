/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: tsr2rcipher encrypt test.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_tsr2rcipher.h"
#include "hi_unf_keyslot.h"
#include "hi_unf_klad.h"

#define SAMPLE_GET_INPUTCMD(input_cmd)     fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define TSC_MARK()                         printf("[%-4s][line:%04d]mark\n", __FUNCTION__, __LINE__)
#define TSC_ERR_PRINT_HEX(val)             printf("[%-4s][line:%04d]%s = 0x%08x\n", __FUNCTION__, __LINE__, #val, val)
#define TSC_ERR_PRINT_INFO(val)            printf("[%-4s][line:%04d]%s\n", __FUNCTION__, __LINE__, val)
#define TSC_ERR_PRINT_VAL(val)             printf("[%-4s][line:%04d]%s = %d\n", __FUNCTION__, __LINE__, #val, val)
#define TSC_ERR_PRINT_POINT(val)           printf("[%-4s][line:%04d]%s = %p\n", __FUNCTION__, __LINE__, #val, val)
#define TSC_PRINT_ERR_CODE(err_code)       printf("[%-4s][line:%04d]return [0x%08x]\n", __FUNCTION__, \
                                                   __LINE__, err_code)
#define TSC_PRINT_ERR_FUNC(func, err_code) printf("[%-4s][line:%04d]call [%s] return [0x%08x]\n", __FUNCTION__, \
                                                   __LINE__, #func, err_code)

#define DATA_LEN 188

const hi_u8 g_input_pkt[188] = {
0x47, 0x00, 0x1c, 0x18, 0x61, 0xb4, 0x68, 0xd1, 0xa3, 0x36, 0x1b, 0x33, 0x06, 0x2c, 0xcd, 0x4c,
0x36, 0x56, 0x8c, 0x18, 0xb0, 0xc3, 0x68, 0xd1, 0xa3, 0x46, 0x18, 0x6d, 0x1a, 0x34, 0x65, 0x69,
0x61, 0xb1, 0x60, 0xce, 0xd1, 0xa9, 0x86, 0xc8, 0xd1, 0xa3, 0x46, 0x18, 0x6d, 0x1a, 0x33, 0x30,
0x61, 0x86, 0xc1, 0x99, 0x95, 0x99, 0x86, 0x1b, 0x2b, 0x33, 0x16, 0x56, 0xa6, 0x1b, 0x06, 0x2c,
0x58, 0xb0, 0xc3, 0x64, 0x66, 0x62, 0xca, 0xd2, 0xc3, 0x68, 0xc1, 0x8b, 0x16, 0x18, 0x6c, 0xad,
0x1a, 0x34, 0x61, 0x86, 0xd1, 0x8b, 0x46, 0x0c, 0x30, 0xda, 0x34, 0x68, 0xd1, 0xa5, 0x86, 0xd1,
0xa3, 0x46, 0x66, 0x6c, 0x36, 0x56, 0x8c, 0xcd, 0x19, 0xb0, 0xd8, 0x31, 0x68, 0xca, 0xc3, 0x0d,
0x8b, 0x06, 0x66, 0x8c, 0x30, 0xd9, 0x5a, 0x31, 0x60, 0xc3, 0x0d, 0xa3, 0x33, 0x06, 0x0c, 0x30,
0xda, 0x34, 0x66, 0x62, 0xc3, 0x0d, 0x91, 0xa3, 0x33, 0x06, 0xa6, 0x1b, 0x46, 0x2c, 0x18, 0xb4,
0xb0, 0xda, 0x32, 0xb3, 0x34, 0x6a, 0x61, 0xb0, 0x62, 0xd1, 0xa3, 0x4b, 0x0d, 0x95, 0xa3, 0x46,
0x8c, 0x30, 0xda, 0x34, 0x68, 0xc1, 0xa5, 0x86, 0xce, 0xca, 0xca, 0xcc, 0xcd, 0x86, 0xcc, 0xc1,
0x99, 0xa3, 0x36, 0x18, 0x00, 0x00, 0x01, 0x07, 0x6b, 0x7f, 0x11, 0x58
};

hi_u8 g_test_key[16] = {
    0x67, 0x47, 0x9C, 0x36, 0x6F, 0x2C, 0x19, 0xC7,
    0x2D, 0x3A, 0x12, 0xB6, 0x75, 0x0F, 0x26, 0x98
};

hi_void dump_data(const hi_char *name, hi_u8 *data, hi_u32 len)
{
    hi_u32 index = 0;

    printf("begin dump %s\n", name);
    printf("data len = %d\n", len);
    printf("data:\n");
    for(index = 0; index < len; index ++) {
        printf("%02X ", data[index]);
        if((index + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\ndump end\n");
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_s32 i = 0;
    hi_handle handle_ks = 0;
    hi_handle handle_klad = 0;
    hi_handle handle_tsc = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    hi_char input_cmd[32] = {0};
    hi_unf_tsr2rcipher_attr tsc_attr;
    hi_unf_keyslot_attr keyslot_attr;
    hi_unf_tsr2rcipher_mem_handle src_mem_handle = {0};
    hi_unf_tsr2rcipher_mem_handle dst_mem_handle = {0};
    hi_u8 *src_virt = HI_NULL;
    hi_u8 *dst_virt = HI_NULL;

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_sys_init, ret);
        return ret;
    }

    ret = hi_unf_tsr2rcipher_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_init, ret);
        goto SYS_DEINIT;
    }

    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_keyslot_init, ret);
        goto TSC_DEINIT;
    }

    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_init, ret);
        goto KS_DEINIT;
    }

    ret = hi_unf_tsr2rcipher_get_default_attr(&tsc_attr);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_get_default_attr, ret);
        goto KLAD_DEINIT;
    }

    tsc_attr.alg = HI_UNF_TSR2RCIPHER_ALG_AES_ECB;
    tsc_attr.is_create_keyslot = HI_FALSE;
    ret = hi_unf_tsr2rcipher_create(&tsc_attr, &handle_tsc);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_create, ret);
        goto KLAD_DEINIT;
    }

    keyslot_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    keyslot_attr.type = HI_UNF_KEYSLOT_TYPE_TSCIPHER;
    ret = hi_unf_keyslot_create(&keyslot_attr, &handle_ks);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_keyslot_create, ret);
        goto TSC_DESTROY;
    }

    ret = hi_unf_klad_create(&handle_klad);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_create, ret);
        goto KS_DESTORY;
    }

    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_AES_ECB_T;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_set_attr, ret);
        goto KLAD_DESTORY;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_attach, ret);
        goto KLAD_DESTORY;
    }

    ret = hi_unf_tsr2rcipher_attach_keyslot(handle_tsc, handle_ks);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_attach_keyslot, ret);
        goto KLAD_DETACH;
    }

    key_clear.odd = 0;
    key_clear.key_size = 16;
    memcpy(key_clear.key, g_test_key, 16);
    ret = hi_unf_klad_set_clear_key(handle_klad, &key_clear);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_set_clear_key, ret);
        goto TSC_DETACH;
    }

    src_mem_handle.mem_handle = hi_unf_mem_new("src_buf", strlen("src_buf") + 1, DATA_LEN, HI_FALSE);
    if (src_mem_handle.mem_handle < 0) {
        TSC_PRINT_ERR_FUNC(hi_mpi_mmz_new, HI_FAILURE);
        goto TSC_DETACH;
    }
    src_mem_handle.addr_offset = 0;
    printf("src_handle: %lld!\n", src_mem_handle.mem_handle);

    src_virt = hi_unf_mem_map(src_mem_handle.mem_handle, DATA_LEN);
    if (src_virt == HI_NULL) {
        TSC_PRINT_ERR_FUNC(hi_mpi_mmz_map, HI_FAILURE);
        goto SRC_DEL;
    }

    dst_mem_handle.mem_handle = hi_unf_mem_new("dst_buf", strlen("dst_buf") + 1, DATA_LEN, HI_FALSE);
    if (dst_mem_handle.mem_handle < 0) {
        TSC_PRINT_ERR_FUNC(hi_mpi_mmz_new, HI_FAILURE);
        goto SRC_UNMAP;
    }
    dst_mem_handle.addr_offset = 0;
    printf("dst_handle: %lld!\n", dst_mem_handle.mem_handle);

    dst_virt = hi_unf_mem_map(dst_mem_handle.mem_handle, DATA_LEN);
    if (dst_virt == HI_NULL) {
        TSC_PRINT_ERR_FUNC(hi_unf_mem_map, HI_FAILURE);
        goto DST_DEL;
    }

    memset(src_virt, 0, DATA_LEN);
    memset(dst_virt, 0, DATA_LEN);
    for (i = 0; i < (DATA_LEN / 188); i++) {
        memcpy(src_virt + (i * 188), g_input_pkt, 188);
    }

    dump_data("src_buf", src_virt, 188);

    ret = hi_unf_tsr2rcipher_encrypt(handle_tsc, src_mem_handle, dst_mem_handle, DATA_LEN);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_encrypt, ret);
        dump_data("dst_buf", dst_virt, 188);
        goto DST_UNMAP;
    }

    dump_data("dst_buf", dst_virt, 188);

    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

DST_UNMAP:
    hi_unf_mem_unmap(dst_virt, DATA_LEN);
DST_DEL:
    hi_unf_mem_delete(dst_mem_handle.mem_handle);
SRC_UNMAP:
    hi_unf_mem_unmap(src_virt, DATA_LEN);
SRC_DEL:
    hi_unf_mem_delete(src_mem_handle.mem_handle);
TSC_DETACH:
    hi_unf_tsr2rcipher_detach_keyslot(handle_tsc, handle_ks);
KLAD_DETACH:
    hi_unf_klad_detach(handle_klad, handle_ks);
KLAD_DESTORY:
    hi_unf_klad_destroy(handle_klad);
KS_DESTORY:
    hi_unf_keyslot_destroy(handle_ks);
TSC_DESTROY:
    hi_unf_tsr2rcipher_destroy(handle_tsc);
KLAD_DEINIT:
    hi_unf_klad_deinit();
KS_DEINIT:
    hi_unf_keyslot_deinit();
TSC_DEINIT:
    hi_unf_tsr2rcipher_deinit();
SYS_DEINIT:
    hi_unf_sys_deinit();
    return 0;
}

