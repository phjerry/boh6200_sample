/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the trust zone interface.
 * Author: Linux SDK team
 * Create: 2019-09-19
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_trust_zone_stat(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_enable_trust_zone(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_trustzone help" } },
    { 1, "set", set_enable_trust_zone, { "Set enable trust zone.", "example: ./sample_otp_trustzone set" } },
    { 2, "get", get_trust_zone_stat, { "Get trust zone stat.", "example: ./sample_otp_trustzone get" } },
};

static hi_s32 get_trust_zone_stat(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_bool stat = HI_FALSE;

    ret = hi_unf_otp_get_trust_zone_stat(&stat);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get trust zone stat, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get trust zone stat : %d\n", stat);

out:
    return ret;
}

static hi_s32 set_enable_trust_zone(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;

    ret = hi_unf_otp_enable_trust_zone();
    if (ret != HI_SUCCESS) {
        sample_printf("Enable trust zone failed, ret = 0x%x \n", ret);
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