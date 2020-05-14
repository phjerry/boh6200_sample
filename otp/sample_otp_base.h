/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: This is a head file for otp test
 * Author: Linux SDK team
 * Create: 2019-09-19
 */

#ifndef __SAMPLE_OTP_BASE_H__
#define __SAMPLE_OTP_BASE_H__

#include <stdio.h>
#include <string.h>

#include "hi_type.h"
#include "hi_errno.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define sample_printf printf

#define KEY_LEN 16
#define NAME_MAX_SIZE 32

typedef struct otp_sample {
    hi_u32 index;
    hi_char name[NAME_MAX_SIZE];
    hi_s32(*run_sample)(hi_s32, hi_char **);
    hi_u8 help[0x3][0x80];
} otp_sample;

hi_bool case_strcmp(const hi_char *s1, const hi_char *s2)
{
    hi_u32 s1_len = strlen(s1);
    hi_u32 s2_len = strlen(s2);

    if (s1_len == s2_len && s1_len > 0) {
        return (strncasecmp(s1, s2, s1_len) == 0) ? HI_TRUE : HI_FALSE;
    }

    return HI_FALSE;
}

hi_void print_buffer(hi_char *string, hi_u8 *buf, hi_u32 len)
{
    hi_u32 i;

    if (buf == NULL || string == NULL) {
        sample_printf("null pointer input in function print_buf!\n");
        return;
    }

    sample_printf("%s:\n", string);

    for (i = 0; i < len; i++) {
        if ((i != 0) && ((i % 0x10) == 0)) {
            sample_printf("\n");
        }

        sample_printf("0x%02x ", buf[i]);
    }

    sample_printf("\n");

    return;
}

hi_void show_usage(otp_sample sample_table[], hi_u32 len)
{
    hi_u32 i, k;

    sample_printf("Usage:\n");
    sample_printf("----------------------------------------------------\n");

    for (i = 0; i < len; i++) {
        sample_printf("    %10s %-16s %s\n", "name:", sample_table[i].name, sample_table[i].help[0]);
        for (k = 1; k < sizeof(sample_table[i].help) / sizeof(sample_table[i].help[0]); k++) {
            if (sample_table[i].help[k][0] == 0) {
                break;
            }

            sample_printf("    %10s %-16s %s\n", "", "", sample_table[i].help[k]);
        }
    }

    return;
}

void show_returne_msg(otp_sample *sample_table, hi_u32 len, hi_s32 ret)
{
    if (sample_table == HI_NULL) {
        return;
    }
    if (ret == HI_SUCCESS) {
        sample_printf("Sample executed successfully. \n");
        return;
    }

    if (ret == HI_ERR_OTP_NOT_SUPPORT_INTERFACE) {
        sample_printf("This chipset not support this interface. ret:0X%X\n", ret);
    } else {
        sample_printf("Sample executed failed. ret:0X%X\n", ret);
    }

    if (ret == HI_ERR_OTP_INVALID_PARA) {
        show_usage(sample_table, len);
    }

    return;
}

static otp_sample *get_opt(hi_char *argv, otp_sample *otp_table, hi_u32 len)
{
    hi_u32 i;

    for (i = 0; i < len; i++) {
        if (case_strcmp(otp_table[i].name, argv)) {
            return &otp_table[i];
        }
    }
    return NULL;
}

hi_s32 run_cmdline(hi_s32 argc, hi_char **argv, otp_sample *otp_table, hi_u32 len)
{
    hi_s32 ret = HI_FAILURE;
    otp_sample *sample = NULL;

    if (argv[1] == HI_NULL) {
        sample_printf("argv[1] is NULL\n");
        goto out;
    }

    if (otp_table == HI_NULL || len == 0) {
        sample_printf("otp_sample table data is NULL or len == 0\n");
        goto out;
    }

    sample = get_opt(argv[1], otp_table, len);
    if (sample == HI_NULL) {
        sample_printf("failed to get_opt func\n");
        goto out;
    }

    if (sample->run_sample) {
        ret = (*sample->run_sample)(argc, argv);
    } else {
        sample_printf(" otp sample error.\n");
    }

out:
    return ret;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* End of #ifndef __SAMPLE_OTP_BASE_H__ */
