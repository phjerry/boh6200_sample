/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the serial_number interface.
 * Author: Linux SDK team
 * Create: 2019-09-19
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_serial_number(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_serial_number(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_serial_number help" } },
    { 1, "set", set_serial_number, { "Set STB SN.", "example: ./sample_otp_serial_number set" } },
    { 2, "get", get_serial_number, { "Get STB SN.", "example: ./sample_otp_serial_number get" } },
};

static hi_s32 get_serial_number(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 serial_number[0x4] = {0};
    hi_u32 len = sizeof(serial_number);

    ret = hi_unf_otp_get_serial_number(serial_number, &len);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get serial_number, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Get serial_number", serial_number, sizeof(serial_number));

out:
    return ret;
}

static hi_s32 set_serial_number(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 serial_number[0x4] = { 0x11, 0x22, 0x33, 0x44 };

    ret = hi_unf_otp_set_serial_number(serial_number, sizeof(serial_number));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set serial_number, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set serial_number", serial_number, sizeof(serial_number));

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
