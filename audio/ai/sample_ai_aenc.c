/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: ai aenc sample
 * Author: audio
 * Create: 2019-09-17
 * Notes:  NA
 * History: 2019-09-17 Initial version for Hi3796CV300
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_ai.h"
#include "hi_unf_aenc.h"
#include "hi_unf_acodec_aacenc.h"
#include "hi_unf_system.h"
#include "hi_adp.h"
#include "hi_errno.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define AI_THREAD_SLEEP_TIME (5 * 1000)     /* 5ms */
#define INPUT_CMD_LENGTH 32
#define TIMEOUT_MS 10


static FILE *g_aenc_file;
static hi_bool g_stop_aenc_thread = HI_FALSE;
static hi_bool g_save_stream = HI_TRUE;
static pthread_t g_aenc_ai_thread;

static hi_void *aenc_ai_thread(hi_void *args)
{
    hi_s32 ret;
    hi_unf_es_buf aenc_stream;
    hi_handle h_aenc = *(hi_handle *)args;

    while (!g_stop_aenc_thread) {
        if (g_save_stream) {
            ret = hi_unf_aenc_acquire_stream(h_aenc, &aenc_stream, TIMEOUT_MS);
            if (ret == HI_SUCCESS) {
                fwrite(aenc_stream.buf, 1, aenc_stream.buf_len, g_aenc_file);
                ret = hi_unf_aenc_release_stream(h_aenc, &aenc_stream);
                if (ret != HI_SUCCESS) {
                    sample_printf("call hi_unf_aenc_release_stream failed(0x%x)\n", ret);
                    break;
                }
            } else if (ret != HI_ERR_AENC_OUT_BUF_EMPTY) {
                sample_printf("call hi_unf_aenc_acquire_stream failed(0x%x)\n", ret);
                break;
            }
        } else {
            usleep(AI_THREAD_SLEEP_TIME);
        }
    }

    return HI_NULL;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle h_aenc;
    hi_handle h_ai;
    hi_unf_aenc_attr aenc_attr;
    hi_unf_ai_attr ai_attr;
    hi_char input_cmd[INPUT_CMD_LENGTH];

    hi_unf_acodec_aac_enc_config private_config;

    if (argc != 3) {
        sample_printf(" usage:     %s aencfile aenctype\n", argv[0]);
        sample_printf(" aenctype: aac\n");
        sample_printf(" examples: \n");
        sample_printf("            %s aencfile aac\n", argv[0]);
        return -1;
    }

    g_aenc_file = fopen(argv[1], "wb");
    if (!g_aenc_file) {
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    if (!strcasecmp("aac", argv[2])) {
        aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AAC;
    } else {
        sample_printf("unsupport aud encode type!\n");
        return -1;
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

    ret = hi_unf_ai_create(HI_UNF_AI_PORT_I2S0, &ai_attr, &h_ai);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_create failed\n");
        goto ai_deinit;
    }

    ret = hi_unf_aenc_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_init failed\n");
        goto ai_destroy;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.AAC.encode.so");
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_init failed\n");
        goto aenc_deinit;
    }

    sample_printf("use aac encode\n");

    hi_unf_acodec_aac_get_default_config(&private_config);
    hi_unf_acodec_aac_get_enc_default_open_param(&(aenc_attr.param), &private_config);

    ret = hi_unf_aenc_create(&aenc_attr, &h_aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_create failed\n");
        goto aenc_deinit;
    }

    ret = hi_unf_aenc_attach_input(h_aenc, h_ai);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_attach_input failed\n");
        goto aenc_destroy;
    }

    ret = hi_unf_ai_set_enable(h_ai, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_set_enable failed\n");
        goto aenc_detach;
    }

    g_stop_aenc_thread = HI_FALSE;
    g_save_stream = HI_TRUE;
    pthread_create(&g_aenc_ai_thread, HI_NULL, aenc_ai_thread, (hi_void *)(&h_aenc));

    while (1) {
        sample_printf("\ninput q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            sample_printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_aenc_thread = HI_TRUE;
    pthread_join(g_aenc_ai_thread, HI_NULL);

    hi_unf_ai_set_enable(h_ai, HI_FALSE);

aenc_detach:
    hi_unf_aenc_detach_input(h_aenc);
aenc_destroy:
    hi_unf_aenc_destroy(h_aenc);
aenc_deinit:
    hi_unf_aenc_deinit();
ai_destroy:
    hi_unf_ai_destroy(h_ai);
ai_deinit:
    hi_unf_ai_deinit();
sys_deinit:
    hi_unf_sys_deinit();

    if (g_aenc_file) {
        fclose(g_aenc_file);
        g_aenc_file = HI_NULL;
    }

    return 0;
}
