/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the asymmetric key hash interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_asymmetric_key_hash_lock_stat(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_asymmetric_key_hash(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_asymmetrickeyhash help" } },
    {
        1, "set", set_asymmetric_key_hash,
        {
            "Set asymmetric key hash ==> HI_UNF_OTP_ASYMMETRIC_KEY_RSA.",
            "example: ./sample_otp_asymmetrickeyhash set rsa"
        }
    },
    {
        2, "set", set_asymmetric_key_hash,
        {
            "Set asymmetric key hash ==> HI_UNF_OTP_ASYMMETRIC_KEY_SM2.",
            "example: ./sample_otp_asymmetrickeyhash set sm2"
        }
    },
    {
        3, "get", get_asymmetric_key_hash_lock_stat,
        {
            "Get asymmetric key hash lock stat ==> HI_UNF_OTP_ASYMMETRIC_KEY_RSA.",
            "example: ./sample_otp_asymmetrickeyhash get rsa"
        }
    },
    {
        4, "get", get_asymmetric_key_hash_lock_stat,
        {
            "Get asymmetric key hash lock stat ==> HI_UNF_OTP_ASYMMETRIC_KEY_SM2.",
            "example: ./sample_otp_asymmetrickeyhash get sm2"
        }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_asymmetric_key_type *key_type)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[2] is NULL\n");
        goto out;
    }

    if (key_type == HI_NULL) {
        sample_printf("key_type is NULL\n");
        goto out;
    }

    if (case_strcmp("rsa", argv[0x2])) {
        *key_type = HI_UNF_OTP_ASYMMETRIC_KEY_RSA;
    } else if (case_strcmp("sm2", argv[0x2])) {
        *key_type = HI_UNF_OTP_ASYMMETRIC_KEY_SM2;
    } else {
        *key_type = HI_UNF_OTP_ASYMMETRIC_KEY_MAX;
    }

out:
    return;
}

static hi_s32 get_asymmetric_key_hash_lock_stat(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_bool status = HI_FALSE;
    hi_unf_otp_asymmetric_key_type key_type = HI_UNF_OTP_ASYMMETRIC_KEY_MAX;

    get_type(argc, argv, &key_type);
    if (key_type == HI_UNF_OTP_ASYMMETRIC_KEY_MAX) {
        sample_printf("Don't have key type\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_get_asymmetric_key_hash_lock_stat(key_type, &status);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get asymmetric key hash lock stat, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get asymmetric key hash lock stat: %d\n", status);

out:
    return ret;
}

static hi_s32 set_asymmetric_key_hash(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 hash[0x20] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x16, 0x17,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26, 0x27,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x36, 0x37,
    };
    hi_unf_otp_asymmetric_key_type key_type = HI_UNF_OTP_ASYMMETRIC_KEY_MAX;

    get_type(argc, argv, &key_type);
    if (key_type == HI_UNF_OTP_ASYMMETRIC_KEY_MAX) {
        sample_printf("NO chip id sel\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_set_asymmetric_key_hash(key_type, hash, sizeof(hash));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set asymmetric key hash, ret = 0x%x \n", ret);
        goto out;
    }

    print_buffer("Set asymmetric key hash", hash, sizeof(hash));

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
