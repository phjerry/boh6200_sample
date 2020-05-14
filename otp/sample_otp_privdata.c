/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the privdata interface.
 * Author: Linux SDK team
 * Create: 2019-09-19
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_privdata(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_privdata(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_privdata help" } },
    { 1, "set", set_privdata, { "Set privdata.", "example: ./sample_otp_privdata set" } },
    { 2, "get", get_privdata, { "Get privdata.", "example: ./sample_otp_privdata get" } },
};

static hi_s32 get_privdata(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 privdata[KEY_LEN] = {0};
    hi_u32 i;

    for (i = 0; i < KEY_LEN; i++) {
        ret = hi_unf_otp_get_priv_data(i, &privdata[i], 0x1);
        if (ret != HI_SUCCESS) {
            sample_printf("Failed to get private data, ret = 0x%x \n", ret);
            goto out;
        }
    }

    print_buffer("Get private data", privdata, sizeof(privdata));

out:
    return ret;
}

static hi_s32 set_privdata(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 privdata[KEY_LEN] = {
        0x10, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    hi_u32 i;

    for (i = 0; i < KEY_LEN; i++) {
        ret = hi_unf_otp_set_priv_data(i, &privdata[i], 0x1);
        if (ret != HI_SUCCESS) {
            sample_printf("Failed to set private data, ret = 0x%x \n", ret);
            goto out;
        }
    }

    print_buffer("Set private data", privdata, sizeof(privdata));

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
