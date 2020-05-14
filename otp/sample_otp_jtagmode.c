/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the jtag mode interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_jtag_mode(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_jtag_mode(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_jtagmode help" } },
    {
        1, "set", set_jtag_mode,
        { "Set jtag mode --> HI_UNF_OTP_JTAG_MODE_OPEN.", "example: ./sample_otp_jtagmode set open" }
    },
    {
        2, "set", set_jtag_mode,
        { "Set jtag mode --> HI_UNF_OTP_JTAG_MODE_PROTECT.", "example: ./sample_otp_jtagmode set protect" }
    },
    {
        3, "set", set_jtag_mode,
        { "Set jtag mode --> HI_UNF_OTP_JTAG_MODE_CLOSED.", "example: ./sample_otp_jtagmode set close" }
    },
    { 4, "get", get_jtag_mode, { "Get jtag mode.", "example: ./sample_otp_jtagmode get" } },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_jtag_mode *jtag_mode)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (jtag_mode == HI_NULL) {
        sample_printf("jtag_mode is NULL\n");
        goto out;
    }

    if (case_strcmp("open", argv[0x2])) {
        *jtag_mode = HI_UNF_OTP_JTAG_MODE_OPEN;
    } else if (case_strcmp("protect", argv[0x2])) {
        *jtag_mode = HI_UNF_OTP_JTAG_MODE_PROTECT;
    } else if (case_strcmp("close", argv[0x2])) {
        *jtag_mode = HI_UNF_OTP_JTAG_MODE_CLOSED;
    } else {
        *jtag_mode = HI_UNF_OTP_JTAG_MODE_MAX;
    }

out:
    return;
}

static hi_void jtag_mode_printf(hi_char *string, hi_unf_otp_jtag_mode jtag_mode)
{
    if (string == HI_NULL) {
        sample_printf("null pointer input in function print_buf!\n");
        return;
    }

    switch (jtag_mode) {
        case HI_UNF_OTP_JTAG_MODE_OPEN:
            sample_printf("%s: HI_UNF_OTP_JTAG_MODE_OPEN\n", string);
            break;
        case HI_UNF_OTP_JTAG_MODE_PROTECT:
            sample_printf("%s: HI_UNF_OTP_JTAG_MODE_PROTECT\n", string);
            break;
        case HI_UNF_OTP_JTAG_MODE_CLOSED:
            sample_printf("%s: HI_UNF_OTP_JTAG_MODE_CLOSED\n", string);
            break;
        default:
            sample_printf("%s: NOT FOUND\n", string);
            break;
    }

    return;
}

static hi_s32 get_jtag_mode(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_jtag_mode jtag_mode = HI_UNF_OTP_JTAG_MODE_MAX;

    ret = hi_unf_otp_get_jtag_mode(&jtag_mode);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get jtag mode, ret = 0x%x \n", ret);
        goto out;
    }

    jtag_mode_printf("Get jtag mode", jtag_mode);

out:
    return ret;
}

static hi_s32 set_jtag_mode(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_jtag_mode jtag_mode = HI_UNF_OTP_JTAG_MODE_MAX;

    get_type(argc, argv, &jtag_mode);
    if (jtag_mode == HI_UNF_OTP_JTAG_MODE_MAX) {
        sample_printf("Don't have jtag mode\n");
        ret = HI_FAILURE;
        goto out;
    }
    ret = hi_unf_otp_set_jtag_mode(jtag_mode);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set jtag mode, ret = 0x%x \n", ret);
        goto out;
    }

    jtag_mode_printf("Set jtag mode", jtag_mode);

out:
    return ret;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret ;

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
