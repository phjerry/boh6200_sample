/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the vendor id interface.
 * Author: Linux SDK team
 * Create: 2019-09-24
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_vendor_id(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_vendorid help" } },
    { 1, "get", get_vendor_id, { "Get vendor id.", "example: ./sample_otp_vendorid get" } },
};

static hi_void vendor_id_printf(hi_unf_otp_vendorid *vendor_id)
{
    if (*vendor_id == HI_UNF_OTP_VENDORID_COMMON) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_COMMON\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_NAGRA) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_NAGRA\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_IRDETO) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_IRDETO\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_CONAX) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_CONAX\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_SYNAMEDIA) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_SYNAMEDIA\n");
    } else if(*vendor_id == HI_UNF_OTP_VENDORID_SUMA) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_SUMA\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_NOVEL) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_NOVEL\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_VERIMATRIX) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_VERIMATRIX\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_CTI) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_CTI\n");
    } else if (HI_UNF_OTP_VENDORID_SAFEVIEW) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_SAFEVIEW\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_LATENSE) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_LATENSE\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_SH_TELECOM) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_SH_TELECOM\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_DCAS) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_DCAS\n");
    } else if(*vendor_id == HI_UNF_OTP_VENDORID_VIACCESS) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_VIACCESS\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_PANACCESS) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_PANACCESS\n");
    } else if (*vendor_id == HI_UNF_OTP_VENDORID_ABV) {
        sample_printf("Get vendor id is HI_UNF_OTP_VENDORID_ABV\n");
    } else {
        sample_printf("Get vendor id is failed\n");
    }

    return;
}

static hi_s32 get_vendor_id(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_vendorid vendor_id = HI_UNF_OTP_VENDORID_MAX;

    ret = hi_unf_otp_get_vendor_id(&vendor_id);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get vendor id, ret = 0x%x \n", ret);
        goto out;
    }

    vendor_id_printf(&vendor_id);

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
