/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the msid interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_msid(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_msid(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_msid help" } },
    {
        1, "set", set_msid,
        { "Set msid --> HI_UNF_OTP_MSID_BOOT.", "example: ./sample_otp_msid set boot" }
    },
    {
        2, "set", set_msid,
        { "Set msid --> HI_UNF_OTP_MSID_TEE.", "example: ./sample_otp_msid set tee" }
    },
    {
        3, "set", set_msid,
        { "Set msid --> HI_UNF_OTP_MSID_HRF.", "example: ./sample_otp_msid set hrf" }
    },
    {
        4, "set", set_msid,
        { "Set msid --> HI_UNF_OTP_MSID_ADSP.", "example: ./sample_otp_msid set adsp" }
    },
    {
        5, "set", set_msid,
        { "Set msid --> HI_UNF_OTP_MSID_VMCU.", "example: ./sample_otp_msid set vmcu" }
    },
    {
        6, "get", get_msid,
        { "Get msid --> HI_UNF_OTP_MSID_BOOT.", "example: ./sample_otp_msid get boot" }
    },
    {
        7, "get", get_msid,
        { "Get msid --> HI_UNF_OTP_MSID_TEE.", "example: ./sample_otp_msid get tee" }
    },
    {
        8, "get", get_msid,
        { "Get msid --> HI_UNF_OTP_MSID_HRF.", "example: ./sample_otp_msid get hrf" }
    },
    {
        9, "get", get_msid,
        { "Get msid --> HI_UNF_OTP_MSID_ADSP.", "example: ./sample_otp_msid get adsp" }
    },
    {
        10, "get", get_msid,
        { "Get msid --> HI_UNF_OTP_MSID_VMCU.", "example: ./sample_otp_msid get vmcu" }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_msid_type *msid_type)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (msid_type == HI_NULL) {
        sample_printf("msid_type is NULL\n");
        goto out;
    }

    if (case_strcmp("boot", argv[0x2])) {
        *msid_type = HI_UNF_OTP_MSID_BOOT;
    } else if (case_strcmp("tee", argv[0x2])) {
        *msid_type = HI_UNF_OTP_MSID_TEE;
    } else if (case_strcmp("hrf", argv[0x2])) {
        *msid_type = HI_UNF_OTP_MSID_HRF;
    } else if (case_strcmp("adsp", argv[0x2])) {
        *msid_type = HI_UNF_OTP_MSID_ADSP;
    } else if (case_strcmp("vmcu", argv[0x2])) {
        *msid_type = HI_UNF_OTP_MSID_VMCU;
    } else {
        *msid_type = HI_UNF_OTP_MSID_MAX;
    }

out:
    return;
}

static hi_s32 get_msid(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 msid[0x4] = {0};
    hi_u32 len = sizeof(msid);
    hi_unf_otp_msid_type msid_type = HI_UNF_OTP_MSID_MAX;

    get_type(argc, argv, &msid_type);
    if (msid_type == HI_UNF_OTP_MSID_MAX) {
        sample_printf("Don't have msid type\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_get_msid(msid_type, msid, &len);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get msid, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Get msid", msid, sizeof(msid));

out:
    return ret;
}

static hi_s32 set_msid(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 msid[0x4] = { 0x44, 0x55, 0x66, 0x77 };
    hi_unf_otp_msid_type msid_type = HI_UNF_OTP_MSID_MAX;

    get_type(argc, argv, &msid_type);
    if (msid_type == HI_UNF_OTP_MSID_MAX) {
        sample_printf("Don't have msid\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_set_msid(msid_type, msid, sizeof(msid));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set chip id, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set msid", msid, sizeof(msid));

out:
    return ret;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;

    if (argc < 0x3) {
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
