/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the global OTP interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_global_otp_lock_stat(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_lock_global_otp(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_globalotp help" } },
    { 1, "set", set_lock_global_otp, { "Set lock global OTP.", "example: ./sample_otp_globalotp set" } },
    { 2, "get", get_global_otp_lock_stat, { "Get global OTP lock stat.", "example: ./sample_otp_globalotp get" } },
};

static hi_s32 get_global_otp_lock_stat(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_bool stat = HI_FALSE;

    ret = hi_unf_otp_get_global_otp_lock_stat(&stat);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get global OTP lock stat, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get global OTP lock stat : %d\n", stat);

out:
    return ret;
}

static hi_s32 set_lock_global_otp(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;

    ret = hi_unf_otp_lock_global_otp();
    if (ret != HI_SUCCESS) {
        sample_printf("Lock global OTP failed, ret = 0x%x \n", ret);
        goto out;
    }

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