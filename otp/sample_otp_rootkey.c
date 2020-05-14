/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the root key interface.
 * Author: Linux SDK team
 * Create: 2019-09-20
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_root_key_lock_stat(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_root_key(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_rootkey help" } },
    {
        1, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_BOOT_ROOTKEY.", "example: ./sample_otp_rootkey set boot" }
    },
    {
        2, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_OEM_ROOTKEY.", "example: ./sample_otp_rootkey set OEM" }
    },
    {
        3, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY0.", "example: ./sample_otp_rootkey set cas0" }
    },
    {
        4, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY1.", "example: ./sample_otp_rootkey set cas1" }
    },
    {
        5, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY2.", "example: ./sample_otp_rootkey set cas2" }
    },
    {
        6, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY3.", "example: ./sample_otp_rootkey set cas3" }
    },
    {
        7, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY4.", "example: ./sample_otp_rootkey set cas4" }
    },
    {
        8, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY5.", "example: ./sample_otp_rootkey set cas5" }
    },
    {
        9, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY6.", "example: ./sample_otp_rootkey set cas6" }
    },
    {
        10, "set", set_root_key,
        { "Set root key --> HI_UNF_OTP_CAS_ROOTKEY7.", "example: ./sample_otp_rootkey set cas7" }
    },

    {
        11, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_BOOT_ROOTKEY.", "example: ./sample_otp_rootkey get boot" }
    },
    {
        12, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_OEM_ROOTKEY.", "example: ./sample_otp_rootkey get OEM" }
    },
    {
        13, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY0.", "example: ./sample_otp_rootkey get cas0" }
    },
    {
        14, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY1.", "example: ./sample_otp_rootkey get cas1" }
    },
    {
        15, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY2.", "example: ./sample_otp_rootkey get cas2" }
    },
    {
        16, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY3.", "example: ./sample_otp_rootkey get cas3" }
    },
    {
        17, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY4.", "example: ./sample_otp_rootkey get cas4" }
    },
    {
        18, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY5.", "example: ./sample_otp_rootkey get cas5" }
    },
    {
        19, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY6.", "example: ./sample_otp_rootkey get cas6" }
    },
    {
        20, "get", get_root_key_lock_stat,
        { "Get root key --> HI_UNF_OTP_CAS_ROOTKEY7.", "example: ./sample_otp_rootkey get cas7" }
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

static hi_void set_key_printf(hi_unf_otp_rootkey key_type, hi_u8 *key, hi_u32 len)
{
    hi_char *string = HI_NULL;

    if (key == HI_NULL) {
        goto out;
    }

    switch (key_type) {
        case HI_UNF_OTP_BOOT_ROOTKEY:
            string = "Set root key(HI_UNF_OTP_BOOT_ROOTKEY)";
            break;
        case HI_UNF_OTP_OEM_ROOTKEY:
            string = "Set root key(HI_UNF_OTP_OEM_ROOTKEY)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY0:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY0)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY1:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY1)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY2:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY2)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY3:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY3)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY4:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY4)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY5:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY5)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY6:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY6)";
            break;
        case HI_UNF_OTP_CAS_ROOTKEY7:
            string = "Set root key(HI_UNF_OTP_CAS_ROOTKEY7)";
            break;
        default:
            goto out;
    }

    print_buffer(string, key, len);

out:
    return;
}

static hi_void get_key_lock_stat_printf(hi_unf_otp_rootkey key_type, hi_bool status)
{
    switch (key_type) {
        case HI_UNF_OTP_BOOT_ROOTKEY:
            sample_printf("Get root key(HI_UNF_OTP_BOOT_ROOTKEY) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_OEM_ROOTKEY:
            sample_printf("Get root key(HI_UNF_OTP_OEM_ROOTKEY) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY0:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY0) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY1:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY1) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY2:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY2) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY3:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY3) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY4:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY4) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY5:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY5) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY6:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY6) lock stat: %d\n", status);
            break;
        case HI_UNF_OTP_CAS_ROOTKEY7:
            sample_printf("Get root key(HI_UNF_OTP_CAS_ROOTKEY7) lock stat: %d\n", status);
            break;
        default:
            break;
    }

    return;
}

static hi_s32 get_root_key_lock_stat(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_bool status = HI_FALSE;
    hi_unf_otp_rootkey key_type = HI_UNF_OTP_ROOTKEY_MAX;

    get_type(argc, argv, &key_type);
    if (key_type == HI_UNF_OTP_ROOTKEY_MAX) {
        sample_printf("Don't have root key type\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_get_root_key_lock_stat(key_type, &status);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get root key lock stat, ret = 0x%x \n", ret);
        goto out;
    }

    get_key_lock_stat_printf(key_type, status);

out:
    return ret;
}

static hi_s32 set_root_key(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u8 key[KEY_LEN] = {
        0x10, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    hi_unf_otp_rootkey key_type = HI_UNF_OTP_ROOTKEY_MAX;

    get_type(argc, argv, &key_type);
    if (key_type == HI_UNF_OTP_ROOTKEY_MAX) {
        sample_printf("Don't have root key type\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_set_root_key(key_type, key, sizeof(key));
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set root key, ret = 0x%x \n", ret);
        goto out;
    }

    set_key_printf(key_type, key, sizeof(key));

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
