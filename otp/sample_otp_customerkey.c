/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the customer key interface.
 * Author: Linux SDK team
 * Create: 2019-09-19
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_customer_key(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_customer_key(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_customerkey help" } },
    { 1, "set", set_customer_key, { "Set customer key.", "example: ./sample_otp_customerkey set" } },
    { 2, "get", get_customer_key, { "Get customer key.", "example: ./sample_otp_customerkey get" } },
};

static hi_s32 get_customer_key(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 customer_key[KEY_LEN] = {0};

    ret = hi_unf_otp_get_customer_key(customer_key, KEY_LEN);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get customer key, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Get otp customer key", customer_key, sizeof(customer_key));

out:
    return ret;
}

static hi_s32 set_customer_key(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 customer_key[KEY_LEN] = {
        0x12, 0x34, 0x56, 0x78, 0x22, 0x23, 0x24, 0x25,
        0x26, 0x27, 0x28, 0x33, 0x44, 0x55, 0x66, 0x77
    };

    ret = hi_unf_otp_set_customer_key(customer_key, KEY_LEN);
    if (ret != HI_SUCCESS) {
        sample_printf("Set customer key failed, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set otp customer key", customer_key, sizeof(customer_key));

out:
    return ret;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;

    if (argc < 0x2) {
        sample_printf("sample parameter error.\n");
        ret = HI_ERR_OTP_INVALID_PARA;
        goto out1;
    }

    if (case_strcmp("help", argv[1])) {
        show_usage(g_otp_sample_data, sizeof(g_otp_sample_data) / sizeof(g_otp_sample_data[0]));
        ret = HI_SUCCESS;
        goto out0;
    }

    ret = hi_unf_otp_init();
    if (ret != HI_SUCCESS) {
        sample_printf("OTP init failed, ret = 0x%x \n", ret);
        goto out1;
    }

    ret = run_cmdline(argc, argv, g_otp_sample_data, sizeof(g_otp_sample_data) / sizeof(g_otp_sample_data[0]));

    (hi_void)hi_unf_otp_deinit();

out1:
    show_returne_msg(g_otp_sample_data, sizeof(g_otp_sample_data) / sizeof(g_otp_sample_data[0]), ret);
out0:
    return ret;
}
