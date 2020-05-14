/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: ai sample
 * Author: audio
 * Create: 2019-09-17
 * Notes:  NA
 * History: 2019-09-17 Initial version for Hi3796CV300
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_ai.h"
#include "hi_unf_sound.h"
#include "hi_adp.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define CAST_FILE_SAVE_MAX_SIZE  (500 * 1024 * 1024)

static hi_u32 g_file_limit_size;
static FILE *g_ai_file = HI_NULL;
static pthread_t g_ai_thread;
static hi_bool g_stop_thread = HI_FALSE;

#define AI_THREAD_SLEEP_TIME (10 * 1000)     /* 10ms */
#define INPUT_CMD_LENGTH 32
#define BYTE_TO_BIT 8
#define MB_TO_BIT (1024 * 1024)

static hi_void ai_thread(hi_void *args)
{
    hi_s32 ret;
    hi_unf_audio_frame ai_frame;
    hi_u32 frame_size = 0;
    hi_u32 total_counter = 0;
    hi_handle ai = *(hi_handle *)args;

    while (!g_stop_thread) {
        ret = hi_unf_ai_acquire_frame(ai, &ai_frame, 0);
        if (ret != HI_SUCCESS) {
            usleep(AI_THREAD_SLEEP_TIME);
            continue;
        }

        frame_size = ai_frame.pcm_samples * ai_frame.channels * ai_frame.bit_depth / BYTE_TO_BIT;
        total_counter += frame_size;

        if (total_counter <= g_file_limit_size * MB_TO_BIT) {
            fwrite(ai_frame.pcm_buffer, 1, frame_size, g_ai_file);
        }

        ret = hi_unf_ai_release_frame(ai, &ai_frame);
        if (ret != HI_SUCCESS) {
            sample_printf("[%s] hi_unf_ai_release_frame failed 0x%x\n", __FUNCTION__, ret);
        }
    }
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle ai;
    hi_unf_ai_attr ai_attr;

    hi_char input_cmd[INPUT_CMD_LENGTH];

    if (argc < 3) {
        printf(" usage: %s size(MB) file(*.pcm) \n", argv[0]);
        printf(" examples: \n");
        printf(" %s 100 /mnt/ai.pcm\n", argv[0]);
        return 0;
    }

    g_file_limit_size = strtol(argv[1], HI_NULL, 0);
    if ((g_file_limit_size <= 0) || (g_file_limit_size > 500)) { /* 500 max file size */
        printf("file save length limit %d is not set or larger than 500MB\n", g_file_limit_size);
        return 0;
    } else {
        printf("file save length limit %d(MB)\n", g_file_limit_size);
    }

    if (strcasecmp("null", argv[2])) {
        g_ai_file = fopen(argv[2], "wb");
        if (!g_ai_file) {
            sample_printf("open file %s error!\n", argv[2]);
            return -1;
        }
    }

    hi_unf_sys_init();

    ret = hi_unf_ai_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_init failed\n");
        goto sys_deinit;
    }

    ret = hi_unf_ai_get_default_attr(HI_UNF_AI_PORT_I2S0, &ai_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_get_default_attr failed\n");
        goto ai_deinit;
    }

    ret = hi_unf_ai_create(HI_UNF_AI_PORT_I2S0, &ai_attr, &ai);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_create failed\n");
        goto ai_deinit;
    }

    ret = hi_unf_ai_set_enable(ai, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_set_enable failed\n");
        goto ai_destroy;
    }

    g_stop_thread = HI_FALSE;
    pthread_create(&g_ai_thread, HI_NULL, (hi_void *)ai_thread, &ai);

    while (1) {
        sample_printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            sample_printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_thread = HI_TRUE;
    pthread_join(g_ai_thread, HI_NULL);

    ret = hi_unf_ai_set_enable(ai, HI_FALSE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_set_enable failed\n");
    }

ai_destroy:
    hi_unf_ai_destroy(ai);
ai_deinit:
    hi_unf_ai_deinit();
sys_deinit:
    hi_unf_sys_deinit();

    if (g_ai_file) {
        fclose(g_ai_file);
        g_ai_file = HI_NULL;
    }

    return 0;
}
