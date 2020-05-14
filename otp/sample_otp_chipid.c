/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the chip id interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_chip_id(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_chip_id(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_chipid help" } },
    {
        1, "set", set_chip_id,
        { "Set chip id --> HI_UNF_OTP_CHIPID0.", "example: ./sample_otp_chipid set chip_id0" }
    },
    {
        2, "set", set_chip_id,
        { "Set chip id --> HI_UNF_OTP_CHIPID1.", "example: ./sample_otp_chipid set chip_id1" }
    },
    {
        3, "set", set_chip_id,
        { "Set chip id --> HI_UNF_OTP_CHIPID2.", "example: ./sample_otp_chipid set chip_id2" }
    },
    {
        4, "get", get_chip_id,
        { "Get chip id --> HI_UNF_OTP_CHIPID0.", "example: ./sample_otp_chipid get chip_id0" }
    },
    {
        5, "get", get_chip_id,
        { "Get chip id --> HI_UNF_OTP_CHIPID1.", "example: ./sample_otp_chipid get chip_id1" }
    },
    {
        6, "get", get_chip_id,
        { "Get chip id --> HI_UNF_OTP_CHIPID2.", "example: ./sample_otp_chipid get chip_id2" }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_chipid_sel *chip_id_sel)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (chip_id_sel == HI_NULL) {
        sample_printf("chip_id_sel is NULL\n");
        goto out;
    }

    if (case_strcmp("chip_id0", argv[0x2])) {
        *chip_id_sel = HI_UNF_OTP_CHIPID0;
    } else if (case_strcmp("chip_id1", argv[0x2])) {
        *chip_id_sel = HI_UNF_OTP_CHIPID1;
    } else if (case_strcmp("chip_id2", argv[0x2])) {
        *chip_id_sel = HI_UNF_OTP_CHIPID2;
    } else {
        *chip_id_sel = HI_UNF_OTP_CHIPID_MAX;
    }

out:
    return;
}

static hi_s32 get_chip_id(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 chip_id[0x8] = {0};
    hi_u32 len = sizeof(chip_id);
    hi_unf_otp_chipid_sel chip_id_sel = HI_UNF_OTP_CHIPID_MAX;

    get_type(argc, argv, &chip_id_sel);
    if (chip_id_sel == HI_UNF_OTP_CHIPID_MAX) {
        sample_printf("Don't have chip id sel\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_get_chip_id(chip_id_sel, chip_id, &len);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get chip id, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Get chip id", chip_id, sizeof(chip_id));

out:
    return ret;
}

static hi_s32 set_chip_id(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 chip_id[0x8] = { 0x21, 0x32, 0x43, 0x54, 0x44, 0x55, 0x66, 0x77 };
    hi_unf_otp_chipid_sel chip_id_sel = HI_UNF_OTP_CHIPID_MAX;

    get_type(argc, argv, &chip_id_sel);
    if (chip_id_sel == HI_UNF_OTP_CHIPID_MAX) {
        sample_printf("Don't have chip id sel\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_set_chip_id(chip_id_sel, chip_id, sizeof(chip_id));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set chip id, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set chip id", chip_id, sizeof(chip_id));

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
