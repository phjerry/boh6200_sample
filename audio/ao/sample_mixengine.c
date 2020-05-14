/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: mix engine sample
 * Author: audio
 * Create: 2019-09-17
 * Notes:  NA
 * History: 2019-09-17 Initial version for Hi3796CV300
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_sound.h"
#include "hi_unf_system.h"

#ifndef SAMPLE_GET_INPUTCMD
#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#endif

typedef struct {
    hi_u32              port_id;

    hi_handle           h_track;
    hi_u32              sample_rate;
    hi_u32              channels;
    hi_u32              weight;

    FILE               *audio_mix_file;
    pthread_t           mix_thread;
    hi_bool             stop_thread;
} audio_mixer;

#define HI_SND_MIXER_NUMBER 8
#define SLAVE_OUTPUT_BUF_SIZE (1024 * 16)
#define MIX_THREAD_SLEEP_TIME (5 * 1000)     /* 5ms */
#define INPUT_CMD_LENGTH 32

static audio_mixer g_mixer[HI_SND_MIXER_NUMBER];

static hi_s32 mix_engine_snd_init(hi_void)
{
    hi_s32 ret;
    hi_unf_snd_attr attr;

    ret = hi_unf_snd_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_init failed.\n");
        return ret;
    }

    ret = hi_unf_snd_get_default_open_attr(HI_UNF_SND_0, &attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_get_default_open_attr failed.\n");
        return ret;
    }

    /* in order to increase the reaction of stop/start, the buf cannot too big */
    attr.slave_output_buf_size = SLAVE_OUTPUT_BUF_SIZE;

    ret = hi_unf_snd_open(HI_UNF_SND_0, &attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_open failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 mix_engine_snd_deinit(hi_void)
{
    hi_s32 ret;

    ret = hi_unf_snd_close(HI_UNF_SND_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_close failed.\n");
        return ret;
    }

    ret = hi_unf_snd_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 audio_mixer_thread(hi_void *args)
{
    hi_void *pcm_buf = HI_NULL;
    hi_s32 ret;
    hi_unf_audio_frame ao_frame;
    hi_bool interleaved = HI_TRUE;
    hi_s32 bit_per_sample = 16;
    hi_s32 pcm_samples = 1024;
    audio_mixer* mixer;
    hi_unf_audio_track_attr  track_attr;
    hi_unf_snd_gain mix_weight;

    hi_bool send_pending = HI_FALSE;
    hi_u32 need_size;
    hi_u32 read_len = 0;

    mixer = (audio_mixer *)args;

    if (mixer->port_id == 0) {
        ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    } else {
        ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
    }

    if (ret != HI_SUCCESS) {
        printf("hi_unf_snd_get_default_track_attr failed(0x%x)!\n", ret);
        return ret;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &mixer->h_track);
    if (ret != HI_SUCCESS) {
        printf("hi_unf_snd_create_track failed(0x%x)!\n", ret);
        return ret;
    }

    mix_weight.linear_mode = HI_TRUE;
    mix_weight.gain = (hi_s32)mixer->weight;
    ret = hi_unf_snd_set_track_weight(mixer->h_track, &mix_weight);
    if (ret != HI_SUCCESS) {
        printf("hi_unf_snd_create_track failed(0x%x)!\n", ret);
        goto track_deinit;
    }

    ao_frame.bit_depth = bit_per_sample;
    ao_frame.channels   = mixer->channels;
    ao_frame.interleaved  = interleaved;
    ao_frame.sample_rate = (hi_u32)(mixer->sample_rate);
    ao_frame.pts = 0xffffffff;
    ao_frame.bits_buffer = HI_NULL;
    ao_frame.bits_bytes = 0;
    ao_frame.frame_index = 0;
    ao_frame.pcm_samples = pcm_samples;

    need_size = pcm_samples * mixer->channels * sizeof(hi_s16);

    pcm_buf = (hi_void*)malloc(need_size);
    if (pcm_buf == HI_NULL) {
        printf("malloc pcm buffer failed!\n");
        goto track_deinit;
    }
    ao_frame.pcm_buffer = (hi_s32*)(pcm_buf);

    while (mixer->stop_thread != HI_TRUE) {
        if (send_pending == HI_FALSE) {
            read_len = fread(pcm_buf, 1, need_size, mixer->audio_mix_file);
            if (read_len != need_size) {
                printf("mixer(%d) reach end of mix file\n", mixer->port_id);
                rewind(mixer->audio_mix_file);
                continue;
            }
        }

        ret = hi_unf_snd_send_track_data(mixer->h_track, &ao_frame);
        if (ret == HI_SUCCESS) {
            send_pending = HI_FALSE;
            continue;
        } else if (ret == HI_FAILURE) {
            printf("hi_unf_snd_send_track_data failed(0x%x)!\n", ret);
            break;
        } else {
            usleep(MIX_THREAD_SLEEP_TIME);
            send_pending = HI_TRUE;
            continue;
        }
    }

    if (pcm_buf != HI_NULL) {
        free(pcm_buf);
        pcm_buf = HI_NULL;
    }
track_deinit:
    ret = hi_unf_snd_destroy_track(mixer->h_track);
    if (ret != HI_SUCCESS) {
        printf("hi_unf_snd_destroy_track failed(0x%x)!\n", ret);
        return ret;
    }

    return ret;
}

hi_s32 main(int argc, char *argv[])
{
    hi_u32 port_id;
    hi_char input_cmd[INPUT_CMD_LENGTH];
    hi_u32 mix_number;
    hi_s32 ret;

    /* sample_mixengine only support pcm stream */
    if (argc < 5) {
        printf("usage:    %s inputfile0 trackweight0 samplerate0 inchannels0 inputfile1 "
                "trackweight1 samplerate1 inchannels1 ...\n", argv[0]);
        printf("examples:\n");
        printf("          %s inputfile0 100 48000 2 inputfile1 100 44100 1\n", argv[0]);
        return HI_FAILURE;
    }

    hi_unf_sys_init();

    ret = mix_engine_snd_init();
    if (ret != HI_SUCCESS) {
        printf("mix_engine_snd_init failed(0x%x)!\n", ret);
        goto sys_deinit;
    }

    mix_number  = argc - 1;
    mix_number /= 4;

    for (port_id = 0; port_id < mix_number; port_id++) {
        g_mixer[port_id].audio_mix_file = fopen(argv[4 * port_id + 0 + 1], "rb");
        if (g_mixer[port_id].audio_mix_file == HI_NULL) {
            printf("open file %s error!\n", argv[port_id + 1]);
            ret = HI_FAILURE;
            goto snd_deinit;
        }

        g_mixer[port_id].weight = atoi(argv[4 * port_id + 1 + 1]);
        g_mixer[port_id].sample_rate = atoi(argv[4 * port_id + 2 + 1]);
        g_mixer[port_id].channels = atoi(argv[4 * port_id + 3 + 1]);
        g_mixer[port_id].port_id = port_id;
        g_mixer[port_id].stop_thread = HI_FALSE;
        g_mixer[port_id].mix_thread = (pthread_t)NULL;
        printf("\n create mixer(%d) , sample_rate(%d) ch(%d)\n", port_id,
               g_mixer[port_id].sample_rate, g_mixer[port_id].channels);

        pthread_create(&(g_mixer[port_id].mix_thread), (const pthread_attr_t*)HI_NULL, (hi_void *)audio_mixer_thread,
                       (hi_void*)(&g_mixer[port_id]));
    }

    while (1) {
        printf("\n please input the q to quit!  v to toggle precision_volume\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            for (port_id = 0; port_id < mix_number; port_id++) {
                g_mixer[port_id].stop_thread = 0;
            }

            break;
        }

        if ('v' == input_cmd[0] || 'V' == input_cmd[0]) {
            hi_unf_snd_preci_gain preci_gain  = {-20, -125};
            hi_unf_snd_preci_gain preci_gain1 = {10, 125};

            static int toggle = 0;
            toggle++;

            if (toggle & 1) {
                hi_unf_snd_set_precision_volume(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, &preci_gain);
                printf("precision_volume -125 on!\n");
            } else {
                hi_unf_snd_set_precision_volume(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, &preci_gain1);
                printf("precision_volume +125 on!\n");
            }
        }
    }

    for (port_id = 0; port_id < mix_number; port_id++) {
        g_mixer[port_id].stop_thread = HI_TRUE;
        pthread_join(g_mixer[port_id].mix_thread, HI_NULL);
        fclose(g_mixer[port_id].audio_mix_file);
    }

snd_deinit:
    ret = mix_engine_snd_deinit();
    if (ret != HI_SUCCESS) {
        printf("mix_engine_snd_init failed(0x%x)!\n", ret);
        return ret;
    }

sys_deinit:
    hi_unf_sys_deinit();

    return ret;
}
