/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the run time check interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_run_time_check_stat(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_enable_run_time_check(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_runtimecheck help" } },
    { 1, "set", set_enable_run_time_check, { "Set enable run time check.", "example: ./sample_otp_runtimecheck set" } },
    { 2, "get", get_run_time_check_stat, { "Get run time check stat.", "example: ./sample_otp_runtimecheck get" } },
};

static hi_s32 get_run_time_check_stat(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_bool stat = HI_FALSE;

    ret = hi_unf_otp_get_runtime_check_stat(&stat);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get run time check stat, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get run time check stat : %d\n", stat);

out:
    return ret;
}

static hi_s32 set_enable_run_time_check(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;

    ret = hi_unf_otp_enable_runtime_check();
    if (ret != HI_SUCCESS) {
        sample_printf("Enable run time check failed, ret = 0x%x \n", ret);
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