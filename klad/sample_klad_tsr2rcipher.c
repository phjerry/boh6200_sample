/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: keyladder tsr2rcipher sample.
 * Author: linux SDK team
 * Create: 2019-09-19
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

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define klad_err_print_hex(val)   printf("[%-32s][line:%04d]%s = 0x%08x\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_info(val)  printf("[%-32s][line:%04d]%s\n", __FUNCTION__, __LINE__, val)
#define klad_err_print_val(val)   printf("[%-32s][line:%04d]%s = %d\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_point(val) printf("[%-32s][line:%04d]%s = %p\n", __FUNCTION__, __LINE__, #val, val)
#define klad_print_error_code(err_code) \
    printf("[%-32s][line:%04d]return [0x%08x]\n", __FUNCTION__, __LINE__, err_code)
#define klad_print_error_func(func, err_code) \
    printf("[%-32s][line:%04d]call [%s] return [0x%08x]\n", __FUNCTION__, __LINE__, #func, err_code)

#define KEY_LEN 16
#define DATA_LEN 188

const hi_u8 g_input_pkt[DATA_LEN] = {
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

hi_u8 g_session_key1[KEY_LEN] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};

hi_u8 g_session_key2[KEY_LEN] = {
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
};

hi_u8 g_content_key[KEY_LEN] = {
    0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32
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

hi_s32 set_key(hi_handle klad)
{
    hi_s32 ret;
    hi_unf_klad_session_key session_key = {0};
    hi_unf_klad_content_key content_key = {0};

    session_key.level = HI_UNF_KLAD_LEVEL1;
    session_key.alg = HI_UNF_KLAD_ALG_TYPE_AES;
    session_key.key_size = KEY_LEN;
    memcpy(content_key.key, g_session_key1, KEY_LEN);
    ret = hi_unf_klad_set_session_key(klad, &session_key);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_session_key, ret);
        return ret;
    }

    session_key.level = HI_UNF_KLAD_LEVEL2;
    session_key.alg = HI_UNF_KLAD_ALG_TYPE_AES;
    session_key.key_size = KEY_LEN;
    memcpy(content_key.key, g_session_key2, KEY_LEN);
    ret = hi_unf_klad_set_session_key(klad, &session_key);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_session_key, ret);
        return ret;
    }

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
    hi_handle handle_ks = 0;
    hi_handle handle_klad = 0;
    hi_handle handle_tsc = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_char input_cmd[32] = {0};
    hi_unf_tsr2rcipher_attr tsc_attr;
    hi_unf_keyslot_attr keyslot_attr;
    hi_u8 *src_virt = HI_NULL;
    hi_u8 *dst_virt = HI_NULL;
    hi_unf_tsr2rcipher_mem_handle src_mem_handle = {0};
    hi_unf_tsr2rcipher_mem_handle dst_mem_handle = {0};
    hi_u32 data_len = DATA_LEN;

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_sys_init, ret);
        return ret;
    }

    ret = hi_unf_tsr2rcipher_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_tsr2rcipher_init, ret);
        goto sys_deinit;
    }

    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_keyslot_init, ret);
        goto tsc_deinit;
    }

    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_init, ret);
        goto ks_deinit;
    }

    ret = hi_unf_tsr2rcipher_get_default_attr(&tsc_attr);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_tsr2rcipher_get_default_attr, ret);
        goto klad_deinit;
    }

    tsc_attr.alg = HI_UNF_TSR2RCIPHER_ALG_AES_ECB;
    tsc_attr.is_create_keyslot = HI_FALSE;
    ret = hi_unf_tsr2rcipher_create(&tsc_attr, &handle_tsc);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_tsr2rcipher_create, ret);
        goto klad_deinit;
    }

    keyslot_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    keyslot_attr.type = HI_UNF_KEYSLOT_TYPE_TSCIPHER;
    ret = hi_unf_keyslot_create(&keyslot_attr, &handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_keyslot_create, ret);
        goto tsc_destroy;
    }

    ret = hi_unf_klad_create(&handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_create, ret);
        goto ks_destroy;
    }

    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CSA2;
    attr_klad.key_cfg.decrypt_support= 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_CSA2;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_attr, ret);
        goto klad_destroy;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_attach, ret);
        goto klad_destroy;
    }

    ret = hi_unf_tsr2rcipher_attach_keyslot(handle_tsc, handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_tsr2rcipher_attach_keyslot, ret);
        goto klad_detach;
    }

    ret = set_key(handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_tsr2rcipher_attach_keyslot, ret);
        goto tsc_detach;
    }

    src_mem_handle.mem_handle = hi_unf_mem_new("src_buf", strlen("src_buf") + 1, data_len, HI_FALSE);
    if (src_mem_handle.mem_handle < 0) {
        klad_print_error_func(hi_unf_mem_new, HI_FAILURE);
        goto tsc_detach;
    }
    src_mem_handle.addr_offset = 0;
    printf("src_phy: 0x%llx!\n", src_mem_handle.mem_handle);

    src_virt = hi_unf_mem_map(src_mem_handle.mem_handle, data_len);
    if (src_virt == HI_NULL) {
        klad_print_error_func(hi_unf_mem_map, HI_FAILURE);
        goto src_del;
    }

    dst_mem_handle.mem_handle = hi_unf_mem_new("dst_buf", strlen("dst_buf") + 1, data_len, HI_FALSE);
    if (dst_mem_handle.mem_handle < 0) {
        klad_print_error_func(hi_unf_mem_new, HI_FAILURE);
        goto src_unmap;
    }
    dst_mem_handle.addr_offset = 0;
    printf("dst_phy: 0x%llx!\n", dst_mem_handle.mem_handle);

    dst_virt = hi_unf_mem_map(dst_mem_handle.mem_handle, data_len);
    if (dst_virt == HI_NULL) {
        klad_print_error_func(hi_unf_mem_map, HI_FAILURE);
        goto dst_del;
    }

    memset(src_virt, 0x0, data_len);
    memset(dst_virt, 0x0, data_len);
    memcpy(src_virt, g_input_pkt, data_len);

    ret = hi_unf_tsr2rcipher_encrypt(handle_tsc, src_mem_handle, dst_mem_handle, data_len);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_tsr2rcipher_encrypt, ret);
        dump_data("dst_buf", dst_virt, data_len);
        goto dst_unmap;
    }

    dump_data("src_buf", src_virt, data_len);
    dump_data("dst_buf", dst_virt, data_len);

    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

dst_unmap:
    hi_unf_mem_unmap(dst_virt, DATA_LEN);
dst_del:
    hi_unf_mem_delete(dst_mem_handle.mem_handle);
src_unmap:
    hi_unf_mem_unmap(src_virt, DATA_LEN);
src_del:
    hi_unf_mem_delete(src_mem_handle.mem_handle);
tsc_detach:
    hi_unf_tsr2rcipher_detach_keyslot(handle_tsc, handle_ks);
klad_detach:
    hi_unf_klad_detach(handle_klad, handle_ks);
klad_destroy:
    hi_unf_klad_destroy(handle_klad);
ks_destroy:
    hi_unf_keyslot_destroy(handle_ks);
tsc_destroy:
    hi_unf_tsr2rcipher_destroy(handle_tsc);
klad_deinit:
    hi_unf_klad_deinit();
ks_deinit:
    hi_unf_keyslot_deinit();
tsc_deinit:
    hi_unf_tsr2rcipher_deinit();
sys_deinit:
    hi_unf_sys_deinit();

    return 0;
}

