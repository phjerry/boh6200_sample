/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the product pv interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 set_product_pv(hi_s32 argc, hi_char *argv[]);
static hi_s32 get_product_pv(hi_s32 argc, hi_char *argv[]);
static hi_s32 verify_product_pv(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    {
        0, "help", NULL,
        { "Display this help and exit.", "example: ./sample_otp_productpv help" }
    },
    {
        1, "set", set_product_pv,
        { "Set product pv.", "example: ./sample_otp_productpv set" }
    },
    {
        2, "get", get_product_pv,
        { "Get product pv.", "example: ./sample_otp_productpv get" }
    },
    {
        3, "verify", verify_product_pv,
        { "Verify product pv.", "example: ./sample_otp_productpv verify" }
    },
};

static hi_s32 set_product_pv(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_burn_pv_item item[] = {
        { HI_TRUE, "hard_gemac0_disable", 0x1, {0x1}, HI_TRUE },
        { HI_TRUE, "hard_gemac1_disable", 0x1, {0x1}, HI_TRUE },
    };

    ret = hi_unf_otp_burn_product_pv(item, sizeof(item) / sizeof(item[0]));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to burn product pv, ret = 0x%x \n", ret);
    }

    return ret;
}

static hi_s32 get_product_pv(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u32 i, j;
    hi_unf_otp_burn_pv_item read_item[] = {
        { HI_FALSE, "hard_gemac0_disable", 0x1, {0x0}, HI_TRUE },
        { HI_FALSE, "hard_gemac1_disable", 0x1, {0x0}, HI_TRUE },
    };

    ret = hi_unf_otp_read_product_pv(read_item, sizeof(read_item) / sizeof(read_item[0]));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to read product pv, ret = 0x%x \n", ret);
    }

    for (i = 0; i < (sizeof(read_item) / sizeof(read_item[0])); i++) {
        sample_printf("Get %s value: \n", read_item[i].field_name);
        for (j = 0; j < read_item[i].value_len; j++) {
            if ((j % 0x10) == 0 && j != 0) {
                sample_printf("\n");
            }
            sample_printf("0x%x ", read_item[i].value[j]);
        }
        sample_printf("\n");
    }

    return ret;
}

static hi_s32 verify_product_pv(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_burn_pv_item item[] = {
        { HI_TRUE, "hard_gemac0_disable", 0x1, {0x1}, HI_TRUE },
        { HI_TRUE, "hard_gemac1_disable", 0x1, {0x1}, HI_TRUE },
    };

    ret = hi_unf_otp_verify_product_pv(item, sizeof(item) / sizeof(item[0]));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to verify product pv, ret = 0x%x \n", ret);
    }

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
