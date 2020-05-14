/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2008-2019. All rights reserved.
 * Description: sample test for gpio function.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#include <assert.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_unf_gpio.h"
#include "hi_errno.h"
#include "hi_adp_boardcfg.h"
#include "hi_unf_system.h"

static hi_u32 g_multi_use_reg_addr = 0;
static hi_u32 g_multi_use_value;

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u32 interrupt_wait_time = 5000;
    hi_u32 interrupt_group = 0;
    hi_u32 interrupt_bit = 0;
    hi_bool read_bit_val;
    hi_u32 value = 0;
    hi_u32 group;
    hi_u32 bit;
    hi_char str[32]; /* array length 32 */

    printf("Attention:\n"
           "This sample will change the multiplex function of GPIO pin.\n"
           "Please DO NOT press Ctrl+C to quit!\n");

    (hi_void)hi_unf_sys_init();

    printf("Please input the GPIO port you want to test:\n");
    scanf("%s", str);
    value = (hi_u32)strtoul(str, 0, 0);
    group = value / 8; /* 8 bit */
    bit = value % 8; /* 8 bit */

    printf("Please input multi-use register address of the GPIO port:\n");
    scanf("%s", str);
    value = (hi_u32)strtoul(str, 0, 0);
    g_multi_use_reg_addr = value;

    printf("Please input multi-use value used for the GPIO port:\n");
    scanf("%s", str);
    value = (hi_u32)strtoul(str, 0, 0);

    hi_unf_sys_read_register(g_multi_use_reg_addr, &g_multi_use_value);
    hi_unf_sys_write_register(g_multi_use_reg_addr, value);

    ret = hi_unf_gpio_init();
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR0;
    }

    getchar();

    printf("Press any key to set GPIO%d_%d to output mode and output high level\n", group, bit);
    getchar();
    ret = hi_unf_gpio_set_direction(group, bit, HI_FALSE);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }
    ret = hi_unf_gpio_write(group, bit, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    printf("Press any key to set GPIO%d_%d to input mode and get the input level\n", group, bit);
    getchar();
    ret = hi_unf_gpio_set_direction(group, bit, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    ret = hi_unf_gpio_read(group, bit, &read_bit_val);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    printf("GPIO%d_%d Input: %d\n", group, bit, read_bit_val);

    printf("Press any key to set GPIO%d_%d to up-interrupt mode.\n", group, bit);
    getchar();
    ret = hi_unf_gpio_set_interrupt_type(group, bit, HI_UNF_GPIO_INTTYPE_UP);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    ret = hi_unf_gpio_set_interrupt_enable(group, bit, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    printf("Please wait for %d seconds to query interrupt on GPIO%d_%d\n",
               interrupt_wait_time / 1000, group, bit); /* 1000 ms */
    ret = hi_unf_gpio_query_interrupt(&interrupt_group, &interrupt_bit, interrupt_wait_time);
    if ((ret == HI_SUCCESS) && (interrupt_group == group) && (interrupt_bit == bit)) {
        printf("GPIO%d_%d get an up edge interrupt\n", group, bit);
    }else if (ret == HI_ERR_GPIO_GETINT_TIMEOUT) {
        printf("GPIO%d_%d has not got an up edge interrupt in %d seconds!\n",
            group, bit, interrupt_wait_time / 1000); /* 1000 ms */
    }

    ret = hi_unf_gpio_set_interrupt_enable(group, bit, HI_FALSE);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    printf("Press any key to quit normally.");
    getchar();
ERR1:

    ret = hi_unf_gpio_deinit();
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
    }

ERR0:
    hi_unf_sys_write_register(g_multi_use_reg_addr, g_multi_use_value);

    (hi_void)hi_unf_sys_deinit();

    return ret;
}
