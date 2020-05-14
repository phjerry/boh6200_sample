/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: The sample for keyled
 */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hi_unf_keyled.h"
#include "hi_unf_system.h"

#define LED_DELAY_TIME 1000000
static hi_s32 g_task_running = 0;

hi_u8 g_dig_display_code_ct1642[10] = { /* array length 10 */
    0xfc, 0x60, 0xda, 0xf2, 0x66,
    0xb6, 0xbe, 0xe0, 0xfe, 0xf6
};
hi_u8 g_dig_display_code_fd650[10]  = { /* array length 10 */
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

const hi_char g_keyled_name[3][16] = { /* 3: line; 16: column */
    "CT1642",
    "FD650",
    "GPIOKEY"
};

hi_u8 g_dig_display_code[10] = {0}; /* array length 10 */

hi_void *led_display_task(hi_void *args)
{
    hi_u32 loop = 0;
    hi_s32 ret;
    hi_unf_keyled_time led_time;
    led_time.hour = 12; /* 12 hour */
    led_time.minute = 25; /* 25 minute */

    while (g_task_running == 1) {
        for (loop = 0; loop < sizeof(g_dig_display_code) && g_task_running == 1; loop++) {
            /* LED display 0~9 or letters */
            ret = hi_unf_led_display((g_dig_display_code[(loop + 3) % /* 3: 3 loops */
                                      sizeof(g_dig_display_code)] << 24) | /* 24: left shift 24bits */
                                     (g_dig_display_code[(loop + 2) % /* 2: 2 loops */
                                      sizeof(g_dig_display_code)] << 16) | /* 16: left shift 16bits */
                                     (g_dig_display_code[(loop + 1) % /* 1: 1 loop */
                                      sizeof(g_dig_display_code)] << 8) | /* 8: left shift 8bits */
                                     (g_dig_display_code[loop]));
            if (ret != HI_SUCCESS) {
                printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
                break;
            }
            ret = hi_unf_led_set_flash_pin(loop % 5 + 1); /* mod 5 */

            usleep(LED_DELAY_TIME);
        }

        (hi_void)hi_unf_led_set_flash_pin(HI_UNF_KEYLED_LIGHT_NONE);

        ret = hi_unf_led_display_time(led_time);
        usleep(LED_DELAY_TIME * 5); /* 5 times LED_DELAY_TIME */
    }
    g_task_running = 0;
    return 0;
}

hi_void *key_receive_task(hi_void *args)
{
    hi_s32 ret;
    hi_u32 press_status, key_id;

    while (g_task_running == 1) {
        /* get KEY press value press status */
        ret = hi_unf_key_get_value(&press_status, &key_id);
        if (ret == HI_SUCCESS) {
            printf("KEY  KeyId : 0x%x    PressStatus :%d[%s]\n", key_id, press_status,
                (press_status == 0) ? "DOWN":(press_status == 1) ? "HOLD" : "UP");
        } else {
            usleep(50000); /* 50000 us */
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    hi_s32 ret;
    pthread_t key_task_id;
    pthread_t led_task_id;
    hi_unf_keyled_type keyled_type;

    printf("Show led and wait key press\n");

    if (argc != 2) { /* para number 2 */
        printf("usage:sample_keyled [keyled_type]\n"
               "keyled_type = 0: CT1642 \n"
               "keyled_type = 1: FD650 \n"
               "keyled_type = 2: GPIOKEY \n");
        return HI_FAILURE;
    } else {
        keyled_type = atoi(argv[1]);
    }

    if (keyled_type >= HI_UNF_KEYLED_TYPE_MAX || keyled_type < HI_UNF_KEYLED_TYPE_CT1642) {
        printf("usage:sample_keyled [keyled_type]\n"
               "keyled_type = 0: CT1642 \n"
               "keyled_type = 1: FD650 \n");
        return HI_FAILURE;
    }
    printf("Test keyled_type %s\n", g_keyled_name[keyled_type]);

    if (keyled_type == 0) { /* 0 is ct1642 */
        memcpy(g_dig_display_code, g_dig_display_code_ct1642, sizeof(g_dig_display_code_ct1642));
    }
    else if(keyled_type == 1) { /* 1 is fd650 */
        memcpy(g_dig_display_code, g_dig_display_code_fd650, sizeof(g_dig_display_code_fd650));
    }else {
        printf(" Do not support this type keyled! \n ");
        return HI_FAILURE;
    }


    hi_unf_sys_init();
    hi_unf_keyled_init();

    ret = hi_unf_keyled_select_type(keyled_type);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    /* open LED device */
    ret = hi_unf_led_open();
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    /* enable flash */
    ret = hi_unf_led_set_flash_freq(HI_UNF_KEYLED_LEVEL_1);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }


    /* config LED flash or not */
    ret = hi_unf_led_set_flash_pin(HI_UNF_KEYLED_LIGHT_NONE);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    /* open KEY device */
    ret = hi_unf_key_open();
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR1;
    }

    /* config keyup is valid */
    ret = hi_unf_key_is_keyup(1);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR2;
    }

    /* config keyhold is valid */
    ret = hi_unf_key_is_repkey(1);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto ERR2;
    }

    g_task_running = 1;

    /* create a thread for receive */
    ret = pthread_create(&key_task_id, NULL, key_receive_task, NULL);
    if (ret != 0) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        perror("pthread_create");
        goto ERR3;
    }

    /* create a thread for led display */
    ret = pthread_create(&led_task_id, NULL, led_display_task, NULL);
    if (ret != 0) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        perror("pthread_create");
        goto ERR3;
    }

    printf("Press any key to exit demo\n");
    getchar();
    g_task_running = 0;

    /* exit the two thread that led_task_id and key_task_id */
    pthread_join(led_task_id, 0);
    pthread_join(key_task_id, 0);

    hi_unf_key_close();
    hi_unf_led_close();
    hi_unf_keyled_deinit();

    hi_unf_sys_deinit();

    printf("\nrun keyled demo success\n");
    return HI_SUCCESS;

ERR3:
    g_task_running = 0;
ERR2:
    hi_unf_key_close();
ERR1:
    hi_unf_led_close();

    hi_unf_keyled_deinit();
    hi_unf_sys_deinit();

    printf("run keyled demo failed\n");
    return ret;
}