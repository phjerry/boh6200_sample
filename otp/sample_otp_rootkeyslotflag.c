/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the root key slot flag interface.
 * Author: Linux SDK team
 * Create: 2019-09-24
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_root_key_slot_flag(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_root_key_slot_flag(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_rootkeyslotflag help" } },
    {
        1, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_BOOT_ROOTKEY.", "example: ./sample_otp_rootkeyslotflag set boot" }
    },
    {
        2, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_OEM_ROOTKEY.", "example: ./sample_otp_rootkeyslotflag set OEM" }
    },
    {
        3, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY0.", "example: ./sample_otp_rootkeyslotflag set cas0" }
    },
    {
        4, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY1.", "example: ./sample_otp_rootkeyslotflag set cas1" }
    },
    {
        5, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY2.", "example: ./sample_otp_rootkeyslotflag set cas2" }
    },
    {
        6, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY3.", "example: ./sample_otp_rootkeyslotflag set cas3" }
    },
    {
        7, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY4.", "example: ./sample_otp_rootkeyslotflag set cas4" }
    },
    {
        8, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY5.", "example: ./sample_otp_rootkeyslotflag set cas5" }
    },
    {
        9, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY6.", "example: ./sample_otp_rootkeyslotflag set cas6" }
    },
    {
        10, "set", set_root_key_slot_flag,
        { "Set root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY7.", "example: ./sample_otp_rootkeyslotflag set cas7" }
    },

    {
        11, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_BOOT_ROOTKEY.", "example: ./sample_otp_rootkeyslotflag get boot" }
    },
    {
        12, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_OEM_ROOTKEY.", "example: ./sample_otp_rootkeyslotflag get OEM" }
    },
    {
        13, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY0.", "example: ./sample_otp_rootkeyslotflag get cas0" }
    },
    {
        14, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY1.", "example: ./sample_otp_rootkeyslotflag get cas1" }
    },
    {
        15, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY2.", "example: ./sample_otp_rootkeyslotflag get cas2" }
    },
    {
        16, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY3.", "example: ./sample_otp_rootkeyslotflag get cas3" }
    },
    {
        17, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY4.", "example: ./sample_otp_rootkeyslotflag get cas4" }
    },
    {
        18, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY5.", "example: ./sample_otp_rootkeyslotflag get cas5" }
    },
    {
        19, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY6.", "example: ./sample_otp_rootkeyslotflag get cas6" }
    },
    {
        20, "get", get_root_key_slot_flag,
        { "Get root key slot flag --> HI_UNF_OTP_CAS_ROOTKEY7.", "example: ./sample_otp_rootkeyslotflagget cas7" }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_rootkey *key_type)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[0x2] is NULL\n");
        goto out;
    }

    if (key_type == HI_NULL) {
        sample_printf("key_type is NULL\n");
        goto out;
    }

    if (case_strcmp("boot", argv[0x2])) {
        *key_type = HI_UNF_OTP_BOOT_ROOTKEY;
    } else if (case_strcmp("OEM", argv[0x2])) {
        *key_type = HI_UNF_OTP_OEM_ROOTKEY;
    } else if (case_strcmp("cas0", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY0;
    } else if (case_strcmp("cas1", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY1;
    } else if (case_strcmp("cas2", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY2;
    } else if (case_strcmp("cas3", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY3;
    } else if (case_strcmp("cas4", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY4;
    } else if (case_strcmp("cas5", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY5;
    } else if (case_strcmp("cas6", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY6;
    } else if (case_strcmp("cas7", argv[0x2])) {
        *key_type = HI_UNF_OTP_CAS_ROOTKEY7;
    } else {
        *key_type = HI_UNF_OTP_ROOTKEY_MAX;
    }

out:
    return;
}

static hi_s32 get_root_key_slot_flag(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_rootkey key_type = HI_UNF_OTP_ROOTKEY_MAX;
    hi_unf_otp_rootkey_slot_flag flag = {0};

    get_type(argc, argv, &key_type);
    if (key_type == HI_UNF_OTP_ROOTKEY_MAX) {
        sample_printf("Don't have key type\n");
        ret = HI_FAILURE;
        goto out;
    }



    ret = hi_unf_otp_get_root_key_slot_flag(key_type, &flag);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get root key slot flag, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get root key slot flag: 0x%08x\n", flag.slot_flag);

out:
    return ret;
}

static hi_s32 set_root_key_slot_flag(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_rootkey key_type = HI_UNF_OTP_ROOTKEY_MAX;
    hi_unf_otp_rootkey_slot_flag flag = {0};

    get_type(argc, argv, &key_type);
    if (key_type == HI_UNF_OTP_ROOTKEY_MAX) {
        sample_printf("Don't have key type\n");
        ret = HI_FAILURE;
        goto out;
    }

    flag.slot_flag = 0x30234354;

    ret = hi_unf_otp_set_root_key_slot_flag(key_type, &flag);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set root key slot flag, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Set root key slot flag: 0x%08x\n", flag.slot_flag);

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
