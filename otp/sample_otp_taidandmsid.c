/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the taid and msid interface.
 * Author: Linux SDK team
 * Create: 2019-09-23
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_taid_and_msid(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_taid_and_msid(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_taidandmsid help" } },
    {
        1, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_1.", "example: ./sample_otp_taidandmsid set index_1" }
    },
    {
        2, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_2.", "example: ./sample_otp_taidandmsid set index_2" }
    },
    {
        3, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_3.", "example: ./sample_otp_taidandmsid set index_3" }
    },
    {
        4, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_4.", "example: ./sample_otp_taidandmsid set index_4" }
    },
    {
        5, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_5.", "example: ./sample_otp_taidandmsid set index_5" }
    },
    {
        6, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_6.", "example: ./sample_otp_taidandmsid set index_6" }
    },
    {
        7, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_7.", "example: ./sample_otp_taidandmsid set index_7" }
    },
    {
        8, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_8.", "example: ./sample_otp_taidandmsid set index_8" }
    },
    {
        9, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_9.", "example: ./sample_otp_taidandmsid set index_9" }
    },
    {
        10, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_10.", "example: ./sample_otp_taidandmsid set index_10" }
    },
    {
        11, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_11.", "example: ./sample_otp_taidandmsid set index_11" }
    },
    {
        12, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_12.", "example: ./sample_otp_taidandmsid set index_12" }
    },
    {
        13, "set", set_taid_and_msid,
        { "Set taid and msid --> HI_UNF_OTP_TA_INDEX_13.", "example: ./sample_otp_taidandmsid set index_13" }
    },
    {
        14, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_1.", "example: ./sample_otp_taidandmsid get index_1" }
    },
    {
        15, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_2.", "example: ./sample_otp_taidandmsid get index_2" }
    },
    {
        16, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_3.", "example: ./sample_otp_taidandmsid get index_3" }
    },
    {
        17, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_4.", "example: ./sample_otp_taidandmsid get index_4" }
    },
    {
        18, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_5.", "example: ./sample_otp_taidandmsid get index_5" }
    },
    {
        19, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_6.", "example: ./sample_otp_taidandmsid get index_6" }
    },
    {
        20, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_7.", "example: ./sample_otp_taidandmsid get index_7" }
    },
    {
        21, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_8.", "example: ./sample_otp_taidandmsid get index_8" }
    },
    {
        22, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_9.", "example: ./sample_otp_taidandmsid get index_9" }
    },
    {
        23, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_10.", "example: ./sample_otp_taidandmsid get index_10" }
    },
    {
        24, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_11.", "example: ./sample_otp_taidandmsid get index_11" }
    },
    {
        25, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_12.", "example: ./sample_otp_taidandmsid get index_12" }
    },
    {
        27, "get", get_taid_and_msid,
        { "Get taid and msid --> HI_UNF_OTP_TA_INDEX_13.", "example: ./sample_otp_taidandmsid get index_13" }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_ta_index *ta_index)
{
    if (ta_index == HI_NULL) {
        sample_printf("argv[0x2] is NULL\n");
        goto out;
    }

    if (case_strcmp("index_1", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_1;
    } else if (case_strcmp("index_2", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_2;
    } else if (case_strcmp("index_3", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_3;
    } else if (case_strcmp("index_4", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_4;
    } else if (case_strcmp("index_5", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_5;
    } else if (case_strcmp("index_6", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_6;
    } else if (case_strcmp("index_7", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_7;
    } else if (case_strcmp("index_8", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_8;
    } else if (case_strcmp("index_9", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_9;
    } else if (case_strcmp("index_10", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_10;
    } else if (case_strcmp("index_11", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_11;
    } else if (case_strcmp("index_12", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_12;
    } else if (case_strcmp("index_13", argv[0x2])) {
        *ta_index = HI_UNF_OTP_TA_INDEX_13;
    } else {
        *ta_index = HI_UNF_OTP_TA_INDEX_MAX;
    }

out:
    return;
}

static hi_s32 get_taid_and_msid(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u32 msid = 0;
    hi_u32 taid = 0;
    hi_unf_otp_ta_index ta_index = HI_UNF_OTP_TA_INDEX_MAX;

    get_type(argc, argv, &ta_index);
    if (ta_index == HI_UNF_OTP_TA_INDEX_MAX) {
        sample_printf("Don't have index of taid and msid\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_get_taid_and_msid(ta_index, &taid, &msid);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get taid and msid, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get taid : 0x%08x, msid : 0x%08x\n", taid, msid);

out:
    return ret;
}

static hi_s32 set_taid_and_msid(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u32 msid = 0xad3d4d5f;
    hi_u32 taid = 0x2343655f;
    hi_unf_otp_ta_index ta_index = HI_UNF_OTP_TA_INDEX_MAX;

    get_type(argc, argv, &ta_index);
    if (ta_index == HI_UNF_OTP_TA_INDEX_MAX) {
        sample_printf("Don't have index of taid and msid \n");
        ret = HI_FAILURE;
        goto out;
    }

    msid += ta_index;
    taid += ta_index;

    ret = hi_unf_otp_set_taid_and_msid(ta_index, taid, msid);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to set taid and msid, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Set taid :0x%08x; msid :0x%08x\n", taid, msid);

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
