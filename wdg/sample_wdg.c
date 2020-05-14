/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2014-2019. All rights reserved.
 * Description: sample test for gpio function.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "hi_unf_wdg.h"

#define TEST_WDG_NO    0

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_u32 loop;
    hi_u32 value;

    /* Open WDG */
    ret = hi_unf_wdg_init();
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    /* Set WDG TimeOut */
    value = 2000; /* 2000 ms */
    ret = hi_unf_wdg_set_timeout(TEST_WDG_NO, value);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    /* Enable WDG */
    ret = hi_unf_wdg_enable(TEST_WDG_NO);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    sleep(1);
    for (loop = 0; loop < 5; loop++) { /* for 5 times */
        /* Clear WDG during timeout, can not reset system */
        ret = hi_unf_wdg_clear(TEST_WDG_NO);
        if (ret != HI_SUCCESS) {
            printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
            goto ERR1;
        }

        printf("Clear wdg Success\n");
        sleep(1);
    }

    printf("wdg disabled\n");

    /* Disable WDG, cat not reset system */
    ret = hi_unf_wdg_disable(TEST_WDG_NO);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    sleep(5); /* sleep 5 secends */
    printf("\nwdg doesn't reset board,demo passed\n");
    printf("now enable wdg and wait reset boardaaaa\n\n");

    /* Enable WDG */
    ret = hi_unf_wdg_enable(TEST_WDG_NO);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    printf("system reset at onceaa\n");
    /* After timeout, system reset at once */
    sleep(5); /* sleep 5 secends */
    printf("system reset at once\n");

ERR1:
    hi_unf_wdg_deinit();
    printf("run wdg sample failed\n");
    return ret;
}
