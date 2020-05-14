/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the long data interface.
 * Author: Linux SDK team
 * Create: 2019-09-23
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 set_long_data(hi_s32 argc, hi_char *argv[]);
static hi_s32 get_long_data(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    {
        0, "help", NULL,
        { "Display this help and exit.", "example: ./sample_otp_longdata help" }
    },
    {
        1, "set", set_long_data,
        { "Set long data.", "example: ./sample_otp_longdata set data" }
    },
    {
        2, "set", set_long_data,
        { "Set long data lock.", "example: ./sample_otp_longdata set lock" }
    },
    {
        3, "get", get_long_data,
        { "Get long data.", "example: ./sample_otp_longdata get data" }
    },
    {
        4, "get", get_long_data,
        { "Get long data lock.", "example: ./sample_otp_longdata get lock" }
    },
};

static hi_s32 _set_long_data(hi_void)
{
    hi_s32 ret;
    hi_char *fuse_name = "reserved_long_0";
    hi_u8 data[0x10] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
    };

    ret = hi_unf_otp_set_long_data(fuse_name, 0, data, sizeof(data));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set long data, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set long data", data, sizeof(data));

out:
    return ret;
}

static hi_s32 set_long_data_lock(hi_void)
{
    hi_s32 ret;
    hi_char *fuse_name = "reserved_long_0";

    ret = hi_unf_otp_set_long_data_lock(fuse_name, 0, 0x10);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set long data lock, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Set long data lock success;\n");

out:
    return ret;
}

static hi_s32 set_long_data(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;

    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (case_strcmp("data", argv[0x2])) {
        ret = _set_long_data();
    } else if (case_strcmp("lock", argv[0x2])) {
        ret = set_long_data_lock();
    } else {
        show_usage(g_otp_sample_data, sizeof(g_otp_sample_data) / sizeof(g_otp_sample_data[0]));
    }

out:
    return ret;
}

static hi_s32 _get_long_data(hi_void)
{
    hi_s32 ret;
    hi_u8 data[0x10] = {0};
    hi_char *fuse_name = "reserved_long_0";

    ret = hi_unf_otp_get_long_data(fuse_name, 0, data, sizeof(data));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get long data, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Get long data", data, sizeof(data));

out:
    return ret;
}

static hi_s32 get_long_data_lock_stat(hi_void)
{
    hi_s32 ret;
    hi_u32 lock = 0;
    hi_char *fuse_name = "reserved_long_0";

    ret = hi_unf_otp_get_long_data_lock(fuse_name, 0, 0x10, &lock);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get long data lock stat, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get long data lock data : %d\n", lock);

out:
    return ret;
}

static hi_s32 get_long_data(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;

    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (case_strcmp("data", argv[0x2])) {
        ret = _get_long_data();
    } else if (case_strcmp("lock", argv[0x2])) {
        ret = get_long_data_lock_stat();
    } else {
        show_usage(g_otp_sample_data, sizeof(g_otp_sample_data) / sizeof(g_otp_sample_data[0]));
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
