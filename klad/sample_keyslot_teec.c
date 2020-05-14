/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: tee keyslot test.
 * Author: linux SDK team
 * Create: 2019-07-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tee_client_api.h"


#define HI_ERR_DFT(format, arg...)     printf( "%s,%d: " format , __FUNCTION__, __LINE__, ## arg)
#define HI_INFO_DFT(format, arg...)    printf( "%s,%d: " format , __FUNCTION__, __LINE__, ## arg)

typedef enum {
    HI_KEYSLOT_TYPE_TSCIPHER = 0x00,
    HI_KEYSLOT_TYPE_MCIPHER,
    HI_KEYSLOT_TYPE_HMAC,
    HI_KEYSLOT_TYPE_MAX
} hi_keyslot_type;

#define KEYSLOT_CMD_CREATE         0
#define KEYSLOT_CMD_DESTROY        1

static int g_session_open = -1;
static TEEC_Context g_teec_context = {0};
static TEEC_Session g_teec_session = {0};
static TEEC_UUID g_teec_uuid = { 0x59e80d08, 0xad42, 0x11e9, { 0xa2, 0xa3, 0x2a, 0x2a, 0xe2, 0xdb, 0xcc, 0xe4 } };

int tee_sec_init()
{
    TEEC_Result teec_rst;
    TEEC_Operation sess_op = {0};
    uint32_t origin = 0;

    if (g_session_open > 0) {
        g_session_open++;
        return 0;
    }
    HI_ERR_DFT("===================init=======================\n");

    teec_rst = TEEC_InitializeContext(NULL, &g_teec_context);
    if (teec_rst != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec Initialize context failed!\n");
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
        HI_ERR_DFT("Teec open session failed!\n");
        (void)TEEC_FinalizeContext(&g_teec_context);
        return (int)teec_rst;
    }
    g_session_open = 1;
    return 0;
}

int tee_sec_deinit()
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

int tee_sec_key_slot_create(hi_keyslot_type keyslot_type, unsigned int *handle)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INOUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)keyslot_type;
    teec_result = TEEC_InvokeCommand(&g_teec_session, KEYSLOT_CMD_CREATE, &teec_operation, NULL);
    if (teec_result != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec invoke command failed, cmd = 0x%08x\n", KEYSLOT_CMD_CREATE);
    }
    *handle = teec_operation.params[0].value.b;
    return (int)teec_result;
}

int tee_sec_key_slot_destroy(unsigned int handle)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = handle;
    teec_result = TEEC_InvokeCommand(&g_teec_session, KEYSLOT_CMD_DESTROY, &teec_operation, NULL);
    if (teec_result != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec invoke command failed, cmd = 0x%08x\n", KEYSLOT_CMD_DESTROY);
    }
    return (int)teec_result;
}

int main(int argc, char *argv[])
{
    unsigned int handle[4] = {0};
    int i;
    int ret;

    tee_sec_init();

    sleep(1);
    for (i = 0; i < 4; i ++) {
        ret = tee_sec_key_slot_create(HI_KEYSLOT_TYPE_MCIPHER, &handle[i]);
        HI_ERR_DFT("============>  0x%08x= 0x%08x\n", ret, handle[i]);
        sleep(1);
    }
    for (i = 0; i < 4; i ++) {
        ret = tee_sec_key_slot_destroy(handle[i]);
        HI_ERR_DFT("************>  0x%08x= 0x%08x\n", ret, handle[i]);
        sleep(1);
    }
    tee_sec_deinit();
    return 0;
}

