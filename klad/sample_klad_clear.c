/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: clear keyladder test.
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
#include "hi_unf_keyslot.h"
#include "hi_unf_klad.h"

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

#define klad_mark()               printf("[%-32s][line:%04d]mark\n", __FUNCTION__, __LINE__)
#define klad_err_print_hex(val)   printf("[%-32s][line:%04d]%s = 0x%08x\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_info(val)  printf("[%-32s][line:%04d]%s\n", __FUNCTION__, __LINE__, val)
#define klad_err_print_val(val)   printf("[%-32s][line:%04d]%s = %d\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_point(val) printf("[%-32s][line:%04d]%s = %p\n", __FUNCTION__, __LINE__, #val, val)
#define klad_print_error_code(err_code) \
    printf("[%-32s][line:%04d]return [0x%08x]\n", __FUNCTION__, __LINE__, err_code)
#define klad_print_error_func(func, err_code) \
    printf("[%-32s][line:%04d]call [%s] return [0x%08x]\n", __FUNCTION__, __LINE__, #func, err_code)

#define KEY_LEN 16

hi_u8 g_test_key[KEY_LEN] = {
    0x67, 0x47, 0x9C, 0x36, 0x6F, 0x2C, 0x19, 0xC7,
    0x2D, 0x3A, 0x12, 0xB6, 0x75, 0x0F, 0x26, 0x98
};

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle handle_ks = 0;
    hi_handle handle_klad = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    hi_unf_keyslot_attr ks_attr = {0};
    hi_char input_cmd[0x20] = {0};

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_sys_init, ret);
        return ret;
    }
    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_keyslot_init, ret);
        goto SYS_DEINIT;
    }

    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_init, ret);
        goto KS_DEINIT;
    }
    ks_attr.type = HI_UNF_KEYSLOT_TYPE_MCIPHER;
    ks_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    ret = hi_unf_keyslot_create(&ks_attr, &handle_ks);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_keyslot_create, ret);
        goto KLAD_DEINIT;
    }

    ret = hi_unf_klad_create(&handle_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_create, ret);
        goto KS_DESTROY;
    }

    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = HI_UNF_CRYPTO_ALG_RAW_AES;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        klad_print_error_func(hi_unf_klad_set_attr, ret);
        goto KLAD_DESTROY;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if (ret != HI_SUCCESS) {
      klad_print_error_func(hi_unf_klad_attach, ret);
      goto KLAD_DESTROY;
    }

    key_clear.odd = 0;
    key_clear.key_size = KEY_LEN;
    memcpy(key_clear.key, g_test_key, KEY_LEN);
    ret = hi_unf_klad_set_clear_key(handle_klad, &key_clear);
    if (ret != HI_SUCCESS) {
      klad_print_error_func(hi_unf_klad_set_clear_key, ret);
      goto KLAD_DESTROY;
    }
    printf("please input 'q' to quit!\n");
    while (1) {
      SAMPLE_GET_INPUTCMD(input_cmd);
      if ('q' == input_cmd[0]) {
          printf("prepare to quit!\n");
          break;
      }
    }

    hi_unf_klad_detach(handle_klad, handle_ks);
KLAD_DESTROY:
    hi_unf_klad_destroy(handle_klad);
KS_DESTROY:
    hi_unf_keyslot_destroy(handle_ks);
KLAD_DEINIT:
    hi_unf_klad_deinit();
KS_DEINIT:
    hi_unf_keyslot_deinit();
SYS_DEINIT:
    hi_unf_sys_deinit();
    return 0;

}

