/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
 * Description: sample_lasdc.c
 * Author: BSP
 * Create: 2011-11-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include "securec.h"
#include "hi_unf_lsadc.h"

static hi_u32 g_channel = 0;
static int g_thread_should_stop = 0;
static hi_u32 g_cur_value;

#define MAX_CHANNEL     3

static int check_params(void)
{
    if (g_channel > MAX_CHANNEL) {
        printf("channel error!\n");
        return -1;
    }
    return 0;
}


/*lint -save -e715 */
hi_void *lsadc_sample_thread(hi_void *arg)
{
    hi_u32 value;

    while (!g_thread_should_stop) {
        hi_unf_lsadc_get_value(g_channel, &value);
        if (g_cur_value != value) {
            printf("Channel %d Value 0x%08x\n", g_channel, value);
            g_cur_value = value;
        }
    }

    return 0;
}

void set_lsadc_config(void)
{
    hi_unf_lsadc_config lsadc_config;

    memset_s(&lsadc_config, sizeof(lsadc_config), 0, sizeof(hi_unf_lsadc_config));
    hi_unf_lsadc_get_config(&lsadc_config);
    lsadc_config.channel_mask |= (0x1 << g_channel);
    lsadc_config.active_bit = 0xFC; /* 6bit 0xF8; */
    lsadc_config.data_delta = 0x0;
    lsadc_config.deglitch_bypass  = 0x1;
    lsadc_config.glitch_sample = 0x0;
    lsadc_config.lsadc_reset = 0x0;
    lsadc_config.lsadc_zero = 0x0;
    lsadc_config.model_sel = 0x1;
    lsadc_config.power_down_mod = 0x1;
    lsadc_config.time_scan = 600; /* scan 600 times */

    hi_unf_lsadc_set_config(&lsadc_config);
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    int ret;
    pthread_t thread;
    unsigned char c;

    /* Get param */
    if (argc == 2) { /* input 2 param */
        g_channel  = strtol(argv[1], NULL, 0);
    } else {
        printf("Usage: %s [Channel]\n", argv[0]);
        printf("Example: %s 0\n", argv[0]);
        return HI_FAILURE;
    }

    /* Chech param */
    if (check_params() < 0) {
        return HI_FAILURE;
    }

    /* Open LSADC */
    ret = hi_unf_lsadc_init();
    if (ret != HI_SUCCESS) {
        printf("hi_unf_lsadc_init err, 0x%08x !\n", ret);
        return ret;
    }

    set_lsadc_config();

    printf("Create lsadc sample thread, press q to exit!\n");
    ret = pthread_create(&thread, NULL, lsadc_sample_thread, NULL);
    if (ret < 0) {
        printf("Failt to create thread!");
        return HI_FAILURE;
    }
    while ((c = getchar()) != 'q') {
        sleep(3); /* sleep 3s */
    }

    g_thread_should_stop = 1;
    pthread_join(thread, NULL);
    printf("LSADC sample thread exit! Bye!\n");

    hi_unf_lsadc_deinit();

    return HI_SUCCESS;
}
