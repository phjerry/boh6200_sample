/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the boot version id interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_boot_version_id(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_boot_version_id(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_bootversionid help" } },
    { 1, "set", set_boot_version_id, { "Set boot version id.", "example: ./sample_otp_bootversionid set" } },
    { 2, "get", get_boot_version_id, { "Get boot version id.", "example: ./sample_otp_bootversionid get" } },
};

static hi_s32 get_boot_version_id(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 version_id[0x4] = {0};
    hi_u32 len = sizeof(version_id);

    ret = hi_unf_otp_get_boot_version_id(version_id, &len);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get boot version id, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Get boot version id", version_id, sizeof(version_id));

out:
    return ret;
}

static hi_s32 set_boot_version_id(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 version_id[0x4] = { 0x21, 0x32, 0x43, 0x54 };

    ret = hi_unf_otp_set_boot_version_id(version_id, sizeof(version_id));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set boot version id, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set boot version id", version_id, sizeof(version_id));

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
