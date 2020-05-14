/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: TEE keyladder test sample.
 * Author: linux SDK team
 * Create: 2019-07-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tee_client_api.h"

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

#define HI_DBG_OTP(format, arg...)       printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)
#define HI_FATAL_OTP(format, arg...)     printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)
#define HI_ERR_OTP(format, arg...)       printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)
#define HI_WARN_OTP(format, arg...)      printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)
#define HI_INFO_OTP(format, arg...)      printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)

#define print_err(val)                   HI_ERR_OTP("%s\n", val)

#define print_dbg_hex(val)               HI_INFO_OTP("%s = 0x%08x\n", #val, val)
#define print_dbg_hex2(x, y)             HI_INFO_OTP("%s = 0x%08x %s = 0x%08x\n", #x, x, #y, y)
#define print_dbg_hex3(x, y, z)          HI_INFO_OTP("%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #x, x, #y, y, #z, z)

#define print_err_hex(val)               HI_ERR_OTP("%s = 0x%08x\n", #val, val)
#define print_err_hex2(x, y)             HI_ERR_OTP("%s = 0x%08x %s = 0x%08x\n", #x, x, #y, y)
#define print_err_hex3(x, y, z)          HI_ERR_OTP("%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #x, x, #y, y, #z, z)
#define print_err_hex4(w, x, y, z)       HI_ERR_OTP("%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #w, \
                                                     w, #x, x, #y, y, #z, z)

#define print_dbg_func_hex(func, val)    HI_INFO_OTP("call [%s]%s = 0x%08x\n", #func, #val, val)
#define print_dbg_func_hex2(func, x, y)  HI_INFO_OTP("call [%s]%s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y)
#define print_dbg_func_hex3(func, x, y, z) \
    HI_INFO_OTP("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y, #z, z)
#define print_dbg_func_hex4(func, w, x, y, z) \
    HI_INFO_OTP("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #w,  w, #x, x, #y, y, #z, z)

#define print_err_func_hex(func, val)    HI_ERR_OTP("call [%s]%s = 0x%08x\n", #func, #val, val)
#define print_err_func_hex2(func, x, y)  HI_ERR_OTP("call [%s]%s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y)
#define print_err_func_hex3(func, x, y, z) \
    HI_ERR_OTP("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y, #z, z)
#define print_err_func_hex4(func, w, x, y, z) \
    HI_ERR_OTP("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #w,  w, #x, x, #y, y, #z, z)

#define dbg_print_dbg_hex(val)           HI_DBG_OTP("%s = 0x%08x\n", #val, val)
#define print_err_val(val)               HI_ERR_OTP("%s = %d\n", #val, val)
#define print_err_point(val)             HI_ERR_OTP("%s = %p\n", #val, val)
#define print_err_code(err_code)         HI_ERR_OTP("return [0x%08x]\n", err_code)
#define print_warn_code(err_code)        HI_WARN_OTP("return [0x%08x]\n", err_code)
#define print_err_func(func, err_code)   HI_ERR_OTP("call [%s] return [0x%08x]\n", #func, err_code)

/*
 * *********************************OTP****************************************
 */
typedef unsigned char           hi_uchar;
typedef unsigned char           hi_u8;
typedef unsigned int            hi_u32;
typedef char                    hi_char;
typedef int                     hi_s32;
typedef void                    hi_void;
typedef hi_u32                  hi_handle;
typedef enum {
    HI_FALSE = 0,
    HI_TRUE  = 1
} hi_bool;

#define OTP_CMD_READ_WORD      0x01
#define OTP_CMD_READ_BYTE      0x02
#define OTP_CMD_WRITE_BYTE     0x03
#define OTP_CMD_LOG_LEVEL      0xff

static int g_session_open = -1;
static TEEC_Context g_teec_context = {0};
static TEEC_Session g_teec_session = {0};
static TEEC_UUID g_teec_uuid = { 0x7ece101c, 0xe197, 0x11e8, { 0x9f, 0x32, 0xf2, 0x80, 0x1f, 0x1b, 0x9f, 0xd1 } } ;

int tee_otp_ta_init()
{
    TEEC_Result teec_rst;
    TEEC_Operation sess_op = {0};
    uint32_t origin = 0;

    if (g_session_open > 0) {
        g_session_open++;
        return 0;
    }

    teec_rst = TEEC_InitializeContext(NULL, &g_teec_context);
    if (teec_rst != TEEC_SUCCESS) {
        print_err_func(TEEC_InitializeContext, teec_rst);
        return teec_rst;
    }
    sess_op.started = 1;
    sess_op.paramTypes = TEEC_PARAM_TYPES(
                             TEEC_NONE,
                             TEEC_NONE,
                             TEEC_MEMREF_TEMP_INPUT,
                             TEEC_MEMREF_TEMP_INPUT);
    teec_rst = TEEC_OpenSession(&g_teec_context,
                                &g_teec_session,
                                &g_teec_uuid,
                                TEEC_LOGIN_IDENTIFY,
                                NULL,
                                &sess_op,
                                &origin);
    if (teec_rst != TEEC_SUCCESS) {
        (void)TEEC_FinalizeContext(&g_teec_context);
        print_err_func(TEEC_OpenSession, teec_rst);
        return (int)teec_rst;
    }

    g_session_open = 1;
    return 0;
}

int tee_otp_ta_deinit()
{
    if (g_session_open > 0) {
        g_session_open--;
    }

    if (g_session_open != 0) {
        return 0;
    }
    (void)TEEC_CloseSession(&g_teec_session);
    (void)TEEC_FinalizeContext(&g_teec_context);

    g_session_open = -1;

    return 0;
}

int tee_otp_log_level(hi_u32 level)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)level;

    teec_result = TEEC_InvokeCommand(&g_teec_session, OTP_CMD_LOG_LEVEL, &teec_operation, NULL);
    if (teec_result != 0) {
        print_err_func_hex2(OTP_CMD_LOG_LEVEL, teec_result, level);
        return (int)teec_result;
    }

    return 0;
}

int tee_otp_ta_read_word(hi_u32 addr, hi_u32 *value)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INOUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)addr;

    teec_result = TEEC_InvokeCommand(&g_teec_session, OTP_CMD_READ_WORD, &teec_operation, NULL);
    if (teec_result != 0) {
        print_err_func_hex2(OTP_CMD_READ_WORD, teec_result, addr);
        return (int)teec_result;
    }
    *value = teec_operation.params[0].value.b;
    return 0;
}

int tee_otp_ta_read_byte(hi_u32 addr, hi_u8 *value)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INOUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)addr;

    teec_result = TEEC_InvokeCommand(&g_teec_session, OTP_CMD_READ_BYTE, &teec_operation, NULL);
    if (teec_result != 0) {
        print_err_func_hex2(OTP_CMD_READ_BYTE, teec_result, addr);
        return (int)teec_result;
    }

    *value = (hi_u8)teec_operation.params[0].value.b;
    return 0;
}

int tee_otp_ta_write_byte(hi_u32 addr, hi_u8 value)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INOUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);

    teec_operation.params[0].value.a = addr;
    teec_operation.params[0].value.b = value;
    teec_result = TEEC_InvokeCommand(&g_teec_session, OTP_CMD_WRITE_BYTE, &teec_operation, NULL);
    if (teec_result != 0) {
        print_err_func_hex3(OTP_CMD_WRITE_BYTE, teec_result, addr, value);
        return (int)teec_result;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_u8 otp_buf[0x1000] = {0};
    hi_u32 i;
    hi_u32 level;

    if (argc < 2) { /* Check the value of argv is less than 2. */
        printf("%s level\n", argv[0]);
        return -1;
    }
    level = strtol(argv[1], NULL, 0);

    ret = tee_otp_ta_init();
    if (ret != 0) {
        print_err_func(tee_otp_ta_init, ret);
        return ret;
    }
    tee_otp_log_level(level);

    for (i = 0; i < 0x1000; i++) {
        ret = tee_otp_ta_read_byte(i, &otp_buf[i]);
        if (ret != 0) {
            print_err_func_hex2(tee_otp_ta_read_byte, i, ret);
        }
    }

    for(i = 0; i < 0x1000; i += 0x4) {
        if(i % 0x10 == 0) {
            printf("%04x:", i);
        }
            printf(" %08x", *((unsigned int*)&otp_buf[i]));
        if(i % 0x10 == 0x0c) {
            printf("\n");
        }
    }
    tee_otp_ta_deinit();
    return 0;
}

