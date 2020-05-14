/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: aenc sample
 * Author: audio
 * Create: 2019-09-17
 * Notes:  NA
 * History: 2019-09-17 Initial version for Hi3796CV300
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_aenc.h"

#include "hi_unf_acodec_aacenc.h"
#include "hi_unf_acodec_g711.h"
#include "hi_unf_acodec_g722.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_amrwb.h"

#include "hi_drv_audio.h"
#include "hi_adp_mpi.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

static hi_u32 g_sample_rate_in = 0;
static hi_u32 g_channels_in = 0;

static hi_handle g_aenc = HI_NULL;
static FILE *g_out_file = HI_NULL;
static FILE *g_in_file = HI_NULL;

static pthread_t g_send_thread;
static pthread_t g_acquire_thread;
static hi_bool   g_stop_thread = HI_FALSE;

#define AENC_IN_PACKET_SIZE  (1024 * 3)
#define AENC_SLEEP_TIME (5 * 1000)     /* 5ms */
#define AENC_SEND_SLEEP_TIME (10 * 1000)     /* 10ms */

static hi_void aenc_frame_init(hi_unf_audio_frame *aenc_frame)
{
    memset(aenc_frame, 0, sizeof(hi_unf_audio_frame));

    aenc_frame->sample_rate = g_sample_rate_in;
    aenc_frame->bit_depth = HI_BIT_DEPTH_16;
    aenc_frame->channels = g_channels_in;
    aenc_frame->pcm_samples = AENC_IN_PACKET_SIZE / (aenc_frame->channels * sizeof(hi_s16));
    aenc_frame->interleaved = HI_TRUE;
}

static hi_void *aenc_acquire_frame_thread(hi_void *arg)
{
    hi_s32 ret;
    hi_u32 frames = 0;
    hi_unf_es_buf buf = {0};

    while (g_stop_thread == HI_FALSE) {
        ret = hi_unf_aenc_acquire_stream(g_aenc, &buf, 0);
        if (ret == HI_SUCCESS) {
            if (g_out_file) {
                fwrite(buf.buf, 1, buf.buf_len, g_out_file);
            }

            frames++;
            if (frames % 10240 == 0) { /* 10240 10k print */
                sample_printf("hi_unf_aenc_acquire_stream times = %d\n", frames);
            }

            ret = hi_unf_aenc_release_stream(g_aenc, &buf);
            if (ret != HI_SUCCESS) {
                sample_printf("hi_unf_aenc_release_stream failed(0x%x)\n", ret);
                return HI_NULL;
            }
        } else {
            usleep(AENC_SLEEP_TIME);
        }
    }

    return HI_NULL;
}

static hi_void *aenc_send_frame_thread(hi_void *arg)
{
    hi_s32 ret;
    hi_s32 read_size;
    hi_u32 encode_frame = 0;
    hi_bool send = HI_TRUE;
    hi_void *data = HI_NULL;
    hi_unf_audio_frame ao_frame;

    aenc_frame_init(&ao_frame);

    data = (hi_void *)malloc(AENC_IN_PACKET_SIZE);
    if (data == HI_NULL) {
        sample_printf("malloc buffer failed!\n");
        return HI_NULL;
    }

    ao_frame.pcm_buffer = (hi_s32 *)data;

    while (g_stop_thread == HI_FALSE) {
        if (send == HI_TRUE) {
            read_size = fread(data, 1, AENC_IN_PACKET_SIZE, g_in_file);
            if (read_size != AENC_IN_PACKET_SIZE) {
                sample_printf("read file end and rewind!\n");
                rewind(g_in_file);
                continue;
            }
        }

        ao_frame.frame_index = encode_frame;
        ret = hi_unf_aenc_send_frame(g_aenc, &ao_frame);
        if (ret == HI_SUCCESS) {
            send = HI_TRUE;
            encode_frame++;
        } else {
            send = HI_FALSE;
            usleep(AENC_SEND_SLEEP_TIME);
        }
    }

    return HI_NULL;
}

static hi_s32 aac_aenc_create(hi_handle *aenc)
{
    hi_s32 ret;
    hi_unf_aenc_attr aenc_attr;
    hi_unf_acodec_aac_enc_config private_config;

    if ((g_channels_in != 1) && (g_channels_in != HI_AUDIO_CH_STEREO)) {
        sample_printf("aac encoder only support stero/mono input!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.AAC.encode.so");
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_register_encoder failed(0x%x)\n", ret);
        return ret;
    }

    sample_printf("register \"libHA.AUDIO.AAC.encode.so\" success!\n");

    memset(&aenc_attr, 0, sizeof(hi_unf_aenc_attr));
    memset(&private_config, 0, sizeof(hi_unf_acodec_aac_enc_config));

    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AAC;
    hi_unf_acodec_aac_get_default_config(&private_config);
    private_config.sample_rate = g_sample_rate_in;

    hi_unf_acodec_aac_get_enc_default_open_param(&aenc_attr.param, (hi_void *)&private_config);

    ret = hi_unf_aenc_create(&aenc_attr, aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_create failed(0x%x)\n", ret);
    }

    return ret;
}

static hi_s32 g711_aenc_create(hi_handle *aenc)
{
    hi_s32 ret;
    hi_unf_aenc_attr aenc_attr;
    hi_unf_acodec_g711_encode_open_config private_config;

    /* g711 encoder only support 8k/16bit/mono */
    if ((g_channels_in != 1) || (g_sample_rate_in != HI_SAMPLE_RATE_8K)) {
        sample_printf("g711 encoder only support 8k/16bit/mono input!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.G711.codec.so");
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_register_encoder failed(0x%x)\n", ret);
        return ret;
    }

    sample_printf("register \"libHA.AUDIO.G711.codec.so\" success!\n");

    memset(&aenc_attr, 0, sizeof(hi_unf_aenc_attr));
    memset(&private_config, 0, sizeof(hi_unf_acodec_g711_encode_open_config));

    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_G711;
    private_config.is_alaw = 1;
    private_config.vad = HI_TRUE;

    hi_unf_acodec_g711_get_enc_default_open_param(&aenc_attr.param, (hi_void *)&private_config);
    aenc_attr.param.sample_per_frame = G711_FRAME_LEN;

    ret = hi_unf_aenc_create(&aenc_attr, aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_create failed(0x%x)\n", ret);
    }

    return ret;
}

static hi_s32 g722_aenc_create(hi_handle *aenc)
{
    hi_s32 ret;
    hi_unf_aenc_attr aenc_attr;

    if ((g_channels_in != 1) || (g_sample_rate_in != HI_SAMPLE_RATE_16K)) {
        sample_printf("g722 encoder only support 16k/16bit/mono input!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.G722.codec.so");
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_register_encoder failed(0x%x)\n", ret);
        return ret;
    }

    sample_printf("register \"libHA.AUDIO.G722.codec.so\" success!\n");

    memset(&aenc_attr, 0, sizeof(hi_unf_aenc_attr));

    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_G722;

    hi_unf_acodec_g722_get_enc_default_open_param(&aenc_attr.param);

    ret = hi_unf_aenc_create(&aenc_attr, aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_create failed(0x%x)\n", ret);
    }

    return ret;
}

static hi_s32 amr_wb_aenc_create(hi_handle *aenc)
{
    hi_s32 ret;
    hi_unf_aenc_attr aenc_attr;
    hi_unf_acodec_amrwb_encode_open_config private_config;

    if ((g_channels_in != 1) || (g_sample_rate_in != HI_SAMPLE_RATE_16K)) {
        sample_printf("amw-wb encoder only support 16k/16bit/mono input!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.AMRWB.codec.so");
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_register_encoder failed(0x%x)\n", ret);
        return ret;
    }

    sample_printf("register \"libHA.AUDIO.AMRWB.codec.so\" success!\n");

    memset(&aenc_attr, 0, sizeof(hi_unf_aenc_attr));
    memset(&private_config, 0, sizeof(hi_unf_acodec_amrwb_encode_open_config));

    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AMRWB;
    private_config.format = HI_UNF_ACODEC_AMRWB_FORMAT_MIME;
    private_config.mode = HI_UNF_ACODEC_AMRWB_MR2385;
    private_config.dtx = HI_FALSE;

    hi_unf_acodec_amrwb_get_enc_default_open_param(&aenc_attr.param, (hi_void *)&private_config);

    ret = hi_unf_aenc_create(&aenc_attr, aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_create failed(0x%x)\n", ret);
    }

    return ret;
}

static hi_s32 amr_nb_aenc_create(hi_handle *aenc)
{
    hi_s32 ret;
    hi_unf_aenc_attr aenc_attr;
    hi_unf_acodec_amrnb_encode_open_config private_config;

    if ((g_channels_in != 1) || (g_sample_rate_in != HI_SAMPLE_RATE_8K)) {
        sample_printf("amw-nb encoder only support 8k/16bit/mono input!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_aenc_register_encoder("libHA.AUDIO.AMRNB.codec.so");
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_register_encoder failed(0x%x)\n", ret);
        return ret;
    }

    sample_printf("register \"libHA.AUDIO.AMRNB.codec.so\" success!\n");

    memset(&aenc_attr, 0, sizeof(hi_unf_aenc_attr));
    memset(&private_config, 0, sizeof(hi_unf_acodec_amrnb_encode_open_config));

    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AMRNB;
    private_config.format = HI_UNF_ACODEC_AMRNB_MIME;
    private_config.mode = HI_UNF_ACODEC_AMRNB_MR475;
    private_config.dtx = HI_FALSE;

    hi_unf_acodec_amrwb_get_enc_default_open_param(&aenc_attr.param, (hi_void *)&private_config);

    ret = hi_unf_aenc_create(&aenc_attr, aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_create failed(0x%x)\n", ret);
    }

    return ret;
}

static hi_s32 aenc_create(hi_unf_acodec_id aenc_type)
{
    switch (aenc_type) {
        case HI_UNF_ACODEC_ID_AAC:
            return aac_aenc_create(&g_aenc);

        case HI_UNF_ACODEC_ID_G711:
            return g711_aenc_create(&g_aenc);

        case HI_UNF_ACODEC_ID_G722:
            return g722_aenc_create(&g_aenc);

        case HI_UNF_ACODEC_ID_AMRWB:
            return amr_wb_aenc_create(&g_aenc);

        case HI_UNF_ACODEC_ID_AMRNB:
            return amr_nb_aenc_create(&g_aenc);

        default:
            sample_printf("unsupport aud codec type!\n");
            return HI_FAILURE;
    }
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_unf_acodec_id aenc_type;

    if (argc != 6) {
        printf("usage:   ./sample_aenc infile_name in_channel in_samplerate outfile_name outfile_type\n"
               "         infile_name: input file name\n"
               "         in_channel:1 2\n"
               "         insamplerate:8000 16000\n"
               "         outfile_name: output file name\n"
               "         outfile_type:aac g711 g722 amrwb amrnb\n");
        printf("example: ./sample_aenc ./test.wav 1 8000 ./test.g711 g711\n");

        return HI_FAILURE;
    }

    g_in_file = fopen(argv[1], "rb");
    if (!g_in_file) {
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    g_out_file = fopen(argv[4], "wb");
    if (!g_out_file) {
        sample_printf("open file %s error!\n", argv[4]);
        goto close_files;
    }

    g_channels_in = atoi(argv[2]);
    g_sample_rate_in = atoi(argv[3]);

    if (!strcasecmp("aac", argv[5])) {
        aenc_type = HI_UNF_ACODEC_ID_AAC;
    } else if (!strcasecmp("g711", argv[5])) {
        aenc_type = HI_UNF_ACODEC_ID_G711;
    } else if (!strcasecmp("g722", argv[5])) {
        aenc_type = HI_UNF_ACODEC_ID_G722;
    } else if (!strcasecmp("amrwb", argv[5])) {
        aenc_type = HI_UNF_ACODEC_ID_AMRWB;
        /* write magic number AMR file */
        fwrite(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER, 1, strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER), g_out_file);
    } else if (!strcasecmp("amrnb", argv[5])) {
        aenc_type = HI_UNF_ACODEC_ID_AMRNB;
        /* write magic number AMR file */
        fwrite(HI_UNF_ACODEC_AMRNB_MAGIC_NUMBER, 1, strlen(HI_UNF_ACODEC_AMRNB_MAGIC_NUMBER), g_out_file);
    } else {
        sample_printf("unsupport aud codec type!\n");
        goto close_files;
    }

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_sys_init failed(0x%x)\n", ret);
        goto close_files;
    }

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("hi_adp_snd_init failed(0x%x)\n", ret);
        goto sys_deinit;
    }

    ret = hi_unf_aenc_init();
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_init failed(0x%x)\n", ret);
        goto snd_deinit;
    }

    ret = aenc_create(aenc_type);
    if (ret != HI_SUCCESS) {
        sample_printf("aenc_create failed(0x%x)\n", ret);
        goto aenc_deinit;
    }

    ret = hi_unf_aenc_start(g_aenc);
    if (ret != HI_SUCCESS) {
        sample_printf("hi_unf_aenc_start failed(0x%x)\n", ret);
        goto aenc_destroy;
    }

    ret = pthread_create(&g_acquire_thread, HI_NULL, aenc_acquire_frame_thread, HI_NULL);
    ret |= pthread_create(&g_send_thread, HI_NULL, aenc_send_frame_thread, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("pthread_create failed(0x%x)\n", ret);
        goto aenc_stop;
    }

    printf("press any key to exit\n");
    getchar();

    g_stop_thread = HI_TRUE;
    pthread_join(g_send_thread, HI_NULL);
    pthread_join(g_acquire_thread, HI_NULL);

aenc_stop:
    (hi_void)hi_unf_aenc_stop(g_aenc);

aenc_destroy:
    (hi_void)hi_unf_aenc_destroy(g_aenc);

aenc_deinit:
    (hi_void)hi_unf_aenc_deinit();

snd_deinit:
    (hi_void)hi_adp_snd_deinit();

sys_deinit:
    (hi_void)hi_unf_sys_deinit();

close_files:
    if (g_in_file != HI_NULL) {
        fclose(g_in_file);
    }

    if (g_out_file != HI_NULL) {
        fclose(g_out_file);
    }

    return HI_SUCCESS;
}
