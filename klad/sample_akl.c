/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: akl test.
 * Author: linux SDK team
 * Create: 2019-11-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hi_unf_cert.h"

#define HI_ID_KLAD HI_ID_USR_START

#define HI_DBG_KLAD(fmt...)              HI_DBG_PRINT(HI_ID_KLAD, fmt)
#define HI_FATAL_KLAD(fmt...)            HI_FATAL_PRINT(HI_ID_KLAD, fmt)
#define HI_ERR_KLAD(fmt...)              HI_ERR_PRINT(HI_ID_KLAD, fmt)
#define HI_WARN_KLAD(fmt...)             HI_WARN_PRINT(HI_ID_KLAD, fmt)
#define HI_INFO_KLAD(fmt...)             HI_INFO_PRINT(HI_ID_KLAD, fmt)

#define print_err(val)                   HI_ERR_KLAD("%s\n", val)

#define print_dbg_hex(val)               HI_INFO_KLAD("%s = 0x%08x\n", #val, val)
#define print_dbg_hex2(x, y)             HI_INFO_KLAD("%s = 0x%08x %s = 0x%08x\n", #x, x, #y, y)
#define print_dbg_hex3(x, y, z)          HI_INFO_KLAD("%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #x, x, #y, y, #z, z)

#define print_err_hex(val)               HI_ERR_KLAD("%s = 0x%08x\n", #val, val)
#define print_err_hex2(x, y)             HI_ERR_KLAD("%s = 0x%08x %s = 0x%08x\n", #x, x, #y, y)
#define print_err_hex3(x, y, z)          HI_ERR_KLAD("%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #x, x, #y, y, #z, z)
#define print_err_hex4(w, x, y, z)       HI_ERR_KLAD("%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #w, \
                                                     w, #x, x, #y, y, #z, z)

#define print_dbg_func_hex(func, val)    HI_INFO_KLAD("call [%s]%s = 0x%08x\n", #func, #val, val)
#define print_dbg_func_hex2(func, x, y)  HI_INFO_KLAD("call [%s]%s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y)
#define print_dbg_func_hex3(func, x, y, z) \
    HI_INFO_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y, #z, z)
#define print_dbg_func_hex4(func, w, x, y, z) \
    HI_INFO_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #w,  w, #x, x, #y, y, #z, z)

#define print_err_func_hex(func, val)    HI_ERR_KLAD("call [%s]%s = 0x%08x\n", #func, #val, val)
#define print_err_func_hex2(func, x, y)  HI_ERR_KLAD("call [%s]%s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y)
#define print_err_func_hex3(func, x, y, z) \
    HI_ERR_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y, #z, z)
#define print_err_func_hex4(func, w, x, y, z) \
    HI_ERR_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #w,  w, #x, x, #y, y, #z, z)

#define dbg_print_dbg_hex(val)           HI_DBG_KLAD("%s = 0x%08x\n", #val, val)
#define print_err_val(val)               HI_ERR_KLAD("%s = %d\n", #val, val)
#define print_err_point(val)             HI_ERR_KLAD("%s = %p\n", #val, val)
#define print_err_code(err_code)         HI_ERR_KLAD("return [0x%08x]\n", err_code)
#define print_warn_code(err_code)        HI_WARN_KLAD("return [0x%08x]\n", err_code)
#define print_err_func(func, err_code)   HI_ERR_KLAD("call [%s] return [0x%08x]\n", #func, err_code)

#define CMD_NUM 2

int main(int argc, char *argv[])
{
    int ret;
    hi_unf_cert_res_handle *handle = HI_NULL;
    hi_size_t num_of_proccessed = 0;
    hi_unf_cert_command command[CMD_NUM] = {
        { {0}, {0}, {0}, { 0x04, 0x01, 0x00, 0x01 }, HI_UNF_CERT_TIMEOUT_DEFAULT },
        { {0}, {0}, {0}, { 0x04, 0x04, 0x00, 0x01 }, HI_UNF_CERT_TIMEOUT_DEFAULT } };

    ret = hi_unf_cert_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_cert_init, ret);
        return ret;
    }

    ret = hi_unf_cert_reset();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_cert_reset, ret);
        goto out;
    }

    ret = hi_unf_cert_lock(&handle);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_cert_lock, ret);
        goto out;
    }
    ret = hi_unf_cert_exchange(handle, CMD_NUM, command, &num_of_proccessed);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_cert_unlock, ret);
        goto unlock;
    }

unlock:
    ret = hi_unf_cert_unlock(handle);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_cert_unlock, ret);
        goto out;
    }
out:
    hi_unf_cert_deinit();
    return ret;
}

