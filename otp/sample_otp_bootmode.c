/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the boot mode interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 set_boot_mode(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_bootmode help" } },
    {
        1, "set", set_boot_mode,
        { "Set boot mode --> HI_UNF_OTP_FLASH_TYPE_SPI_NOR.", "example: ./sample_otp_bootmode set spi_nor" }
    },
    {
        2, "set", set_boot_mode,
        { "Set boot mode --> HI_UNF_OTP_FLASH_TYPE_SPI_NAND.", "example: ./sample_otp_bootmode set spi_nand" }
    },
    {
        3, "set", set_boot_mode,
        { "Set boot mode --> HI_UNF_OTP_FLASH_TYPE_NAND.", "example: ./sample_otp_bootmode set nand" }
    },
    {
        4, "set", set_boot_mode,
        { "Set boot mode --> HI_UNF_OTP_FLASH_TYPE_EMMC_INNER_PHY.", "example: ./sample_otp_bootmode set emmc_phy" }
    },
    {
        5, "set", set_boot_mode,
        { "Set boot mode --> HI_UNF_OTP_FLASH_TYPE_EMMC_OUTER_PHY.", "example: ./sample_otp_bootmode set emmc" }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_flash_type *boot_mode)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (boot_mode == HI_NULL) {
        sample_printf("boot_mode is NULL\n");
        goto out;
    }

    if (case_strcmp("spi", argv[0x2])) {
        *boot_mode = HI_UNF_OTP_FLASH_TYPE_SPI_NOR;
    } else if (case_strcmp("spi_nand", argv[0x2])) {
        *boot_mode = HI_UNF_OTP_FLASH_TYPE_SPI_NAND;
    } else if (case_strcmp("nand", argv[0x2])) {
        *boot_mode = HI_UNF_OTP_FLASH_TYPE_NAND;
    } else if (case_strcmp("emmc_phy", argv[0x2])) {
        *boot_mode = HI_UNF_OTP_FLASH_TYPE_EMMC_INNER_PHY;
    } else if (case_strcmp("emmc", argv[0x2])) {
        *boot_mode = HI_UNF_OTP_FLASH_TYPE_EMMC_OUTER_PHY;
    } else {
        *boot_mode = HI_UNF_OTP_FLASH_TYPE_MAX;
    }

out:
    return;
}

static hi_void boot_mode_printf(hi_unf_otp_flash_type boot_mode)
{
    switch (boot_mode) {
        case HI_UNF_OTP_FLASH_TYPE_SPI_NOR:
            sample_printf("Set boot mode: HI_UNF_OTP_FLASH_TYPE_SPI_NOR\n");
            break;
        case HI_UNF_OTP_FLASH_TYPE_SPI_NAND:
            sample_printf("Set boot mode: HI_UNF_OTP_FLASH_TYPE_SPI_NAND\n");
            break;
        case HI_UNF_OTP_FLASH_TYPE_NAND:
            sample_printf("Set boot mode: HI_UNF_OTP_FLASH_TYPE_NAND\n");
            break;
        case HI_UNF_OTP_FLASH_TYPE_EMMC_INNER_PHY:
            sample_printf("Set boot mode: HI_UNF_OTP_FLASH_TYPE_EMMC_INNER_PHY\n");
            break;
        case HI_UNF_OTP_FLASH_TYPE_EMMC_OUTER_PHY:
            sample_printf("Set boot mode: HI_UNF_OTP_FLASH_TYPE_EMMC_OUTER_PHY\n");
            break;
        default:
            break;
    }

    return;
}

static hi_s32 set_boot_mode(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_flash_type boot_mode = HI_UNF_OTP_FLASH_TYPE_MAX;

    get_type(argc, argv, &boot_mode);
    if (boot_mode == HI_UNF_OTP_FLASH_TYPE_MAX) {
        sample_printf("NO chip id sel\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_set_boot_mode(boot_mode);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set boot mode, ret = 0x%x \n", ret);
        goto out;
    }

    boot_mode_printf(boot_mode);

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
