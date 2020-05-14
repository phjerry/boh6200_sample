/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: aenc sample
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

#include "hi_unf_system.h"
#include "hi_unf_sound.h"
#include "hi_unf_aenc.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_aacenc.h"

#include "hi_adp_mpi.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define CAST_SLEEP_TIME 5000     /* 5ms */
#define INPUT_CMD_LENGTH 32

static FILE *g_audio_cast_file = HI_NULL;
static pthread_t g_cast_thread;
static hi_bool g_stop_thread = HI_FALSE;

static hi_void *cast_thread(hi_void *arg)
{
    hi_s32 ret;
    hi_unf_es_buf es_frame;
    hi_s32 frame = 0;
    hi_handle h_aenc = *(hi_handle *)arg;

    while (!g_stop_thread) {
        ret = hi_unf_aenc_acquire_stream(h_aenc, &es_frame, 0);
        if (ret != HI_SUCCESS) {
            usleep(CAST_SLEEP_TIME);
            continue;
        }

        if (g_audio_cast_file) {
            fwrite(es_frame.buf, 1, es_frame.buf_len, g_audio_cast_file);
        }

        frame++;
        if ((frame % 256 == 0)) { /* per 256 frame print once */
            sample_printf("cast get AAC frame(%d)\n", frame);
        }

        ret = hi_unf_aenc_release_stream(h_aenc, &es_frame);
        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_aenc_release_stream failed(0x%x)\n", ret);
        }
    }

    if (g_audio_cast_file) {
        fclose(g_audio_cast_file);
        g_audio_cast_file = HI_NULL;
    }

    return HI_NULL;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle h_cast = HI_INVALID_HANDLE;
    hi_unf_snd_cast_attr cast_attr;
    hi_bool cast_enable;
    hi_unf_acodec_aac_enc_config private_config;
    hi_unf_aenc_attr aenc_attr;
    hi_handle h_aenc = HI_INVALID_HANDLE;
    hi_char input_cmd[INPUT_CMD_LENGTH];

    if (argc < 2) {
        sample_printf(" usage:    %s filename(*.aac)\n", argv[0]);
        sample_printf(" examples: \n");
        sample_printf("           %s /mnt/cast.aac\n", argv[0]);
        return 0;
    }

    if (strcasecmp("null", argv[1])) {
        g_audio_cast_file = fopen(argv[1], "wb");
        if (!g_audio_cast_file) {
            sample_printf("open file %s error!\n", argv[1]);
            return -1;
        }
    }

    hi_unf_sys_init();
    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_snd_init failed(0x%x)\n", ret);
        goto sys_deinit;
    }

    /* create cast */
    ret = hi_unf_snd_get_default_cast_attr(HI_UNF_SND_0, &cast_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_cast_attr failed(0x%x)\n", ret);
        goto snd_deinit;
    }

    sample_printf("cast attr: pcm_samples = %d, pcm_frame_max_num = %d\n",
        cast_attr.pcm_samples, cast_attr.pcm_frame_max_num);

    ret = hi_unf_snd_create_cast(HI_UNF_SND_0, &cast_attr, &h_cast);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_cast failed(0x%x)\n", ret);
        goto cast_deinit;
    }

    /* create aenc */
    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AAC;
    hi_unf_acodec_aac_get_default_config(&private_config);
    private_config.sample_rate = HI_UNF_AUDIO_SAMPLE_RATE_48K;

    hi_unf_acodec_aac_get_enc_default_open_param(&aenc_attr.param, (hi_void *)&private_config);
    sample_printf("use aac encode\n");

    ret = hi_unf_aenc_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_init failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.AAC.encode.so");
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_register_encoder failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    ret = hi_unf_aenc_create(&aenc_attr, &h_aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_create failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    ret = hi_unf_aenc_attach_input(h_aenc, h_cast);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_attach_input failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    ret = hi_unf_aenc_start(h_aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_start failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    ret = hi_unf_snd_set_cast_enable(h_cast, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_set_cast_enable failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    g_stop_thread = HI_FALSE;
    pthread_create(&g_cast_thread, HI_NULL, cast_thread, &h_aenc);

    while (1) {
        sample_printf("input q to quit! s to disable cast, r to enable cast\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            sample_printf("prepare to quit!\n");
            break;
        }

        if (('s' == input_cmd[0]) || ('S' == input_cmd[0])) {
            ret = hi_unf_snd_set_cast_enable(h_cast, HI_FALSE);
            if (ret != HI_SUCCESS) {
                sample_printf("call hi_unf_snd_set_cast_enable failed(0x%x)\n", ret);
            }
        }

        if (('r' == input_cmd[0]) || ('R' == input_cmd[0])) {
            ret = hi_unf_snd_set_cast_enable(h_cast, HI_TRUE);
            if (ret != HI_SUCCESS) {
                sample_printf("call hi_unf_snd_set_cast_enable failed(0x%x)\n", ret);
            }
        }

        if (('g' == input_cmd[0]) || ('G' == input_cmd[0])) {
            ret = hi_unf_snd_get_cast_enable(h_cast, &cast_enable);
            if (ret != HI_SUCCESS) {
                sample_printf("call hi_unf_snd_get_cast_enable failed(0x%x)\n", ret);
            } else {
                sample_printf("cast handle:0x%x, enable:%d\n", h_cast, cast_enable);
            }
        }
    }

    g_stop_thread = HI_TRUE;
    pthread_join(g_cast_thread, HI_NULL);

    ret = hi_unf_aenc_stop(h_aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_stop failed(0x%x)\n", ret);
    }

    ret = hi_unf_aenc_detach_input(h_aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_detach_input failed(0x%x)\n", ret);
    }

    ret = hi_unf_snd_set_cast_enable(h_cast, HI_FALSE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_set_cast_enable failed(0x%x)\n", ret);
    }

aenc_deinit:
    hi_unf_aenc_destroy(h_aenc);
    hi_unf_aenc_deinit();

cast_deinit:
    hi_unf_snd_destroy_cast(h_cast);
snd_deinit:
    hi_adp_snd_deinit();
sys_deinit:
    hi_unf_sys_deinit();

    if (g_audio_cast_file) {
        fclose(g_audio_cast_file);
        g_audio_cast_file = HI_NULL;
    }

    return 0;
}
