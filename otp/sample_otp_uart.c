/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: Test the file of the uart interface.
 * Author: Linux SDK team
 * Create: 2019-09-23
 */

#include "hi_unf_otp.h"
#include "sample_otp_base.h"

static hi_s32 get_uart_stat(hi_s32 argc, hi_char *argv[]);
static hi_s32 set_disable_uart(hi_s32 argc, hi_char *argv[]);

static otp_sample g_otp_sample_data[] = {
    { 0, "help", NULL, { "Display this help and exit.", "example: ./sample_otp_uart help" } },
    {
        1, "set", set_disable_uart,
        { "Set disable uart --> HI_UNF_OTP_UART0.", "example: ./sample_otp_uart set uart_0" }
    },
    {
        2, "set", set_disable_uart,
        { "Set disable uart --> HI_UNF_OTP_UART2.", "example: ./sample_otp_uart set uart_2" }
    },
    {
        3, "set", set_disable_uart,
        { "Set disable uart --> HI_UNF_OTP_UART3.", "example: ./sample_otp_uart set uart_3" }
    },
    {
        4, "set", set_disable_uart,
        { "Set disable uart --> HI_UNF_OTP_UART4.", "example: ./sample_otp_uart set uart_4" }
    },
    {
        5, "set", set_disable_uart,
        { "Set disable uart --> HI_UNF_OTP_UART5.", "example: ./sample_otp_uart set uart_5" }
    },
    {
        6, "get", get_uart_stat,
        { "Get uart stat --> HI_UNF_OTP_UART0.", "example: ./sample_otp_uart get uart_0" }
    },
    {
        7, "get", get_uart_stat,
        { "Get uart stat --> HI_UNF_OTP_UART2.", "example: ./sample_otp_uart get uart_2" }
    },
    {
        8, "get", get_uart_stat,
        { "Get uart stat --> HI_UNF_OTP_UART3.", "example: ./sample_otp_uart get uart_3" }
    },
    {
        9, "get", get_uart_stat,
        { "Get uart stat --> HI_UNF_OTP_UART4.", "example: ./sample_otp_uart get uart_4" }
    },
    {
        10, "get", get_uart_stat,
        { "Get uart stat --> HI_UNF_OTP_UART5.", "example: ./sample_otp_uart get uart_5" }
    },
};

static hi_void get_type(hi_s32 argc, hi_char *argv[], hi_unf_otp_uart_type *uart_type)
{
    if (argv[0x2] == HI_NULL) {
        sample_printf("argv[0x2] is NULL\n");
        goto out;
    }

    if (uart_type == HI_NULL) {
        sample_printf("uart_type is NULL\n");
        goto out;
    }

    if (case_strcmp("uart_0", argv[0x2])) {
        *uart_type = HI_UNF_OTP_UART0;
    } else if (case_strcmp("uart_2", argv[0x2])) {
        *uart_type = HI_UNF_OTP_UART2;
    } else if (case_strcmp("uart_3", argv[0x2])) {
        *uart_type = HI_UNF_OTP_UART3;
    } else if (case_strcmp("uart_4", argv[0x2])) {
        *uart_type = HI_UNF_OTP_UART4;
    } else if (case_strcmp("uart_5", argv[0x2])) {
        *uart_type = HI_UNF_OTP_UART5;
    } else {
        *uart_type = HI_UNF_OTP_MAX;
    }

out:
    return;
}

static hi_s32 get_uart_stat(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_bool stat = HI_FAILURE;
    hi_unf_otp_uart_type uart_type = HI_UNF_OTP_MAX;

    get_type(argc, argv, &uart_type);
    if (uart_type == HI_UNF_OTP_MAX) {
        sample_printf("Don't have uart type\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_get_uart_stat(uart_type, &stat);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to get uart stat, ret = 0x%x \n", ret);
        goto out;
    }

    sample_printf("Get uart stat: %d\n", stat);

out:
    return ret;
}

static hi_s32 set_disable_uart(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_unf_otp_uart_type uart_type = HI_UNF_OTP_MAX;

    get_type(argc, argv, &uart_type);
    if (uart_type == HI_UNF_OTP_MAX) {
        sample_printf("Don't have uart\n");
        ret = HI_FAILURE;
        goto out;
    }

    ret = hi_unf_otp_disable_uart(uart_type);
    if (ret != HI_SUCCESS) {
        sample_printf("Failed to disable uart, ret = 0x%x \n", ret);
        goto out;
    }

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
