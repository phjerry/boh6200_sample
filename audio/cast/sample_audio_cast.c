/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: cast sample
 * Author: audio
 * Create: 2019-09-17
 * Notes:  NA
 * History: 2019-09-17 Initial version for Hi3796CV300
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_sound.h"
#include "hi_adp_mpi.h"
#include "hi_adp.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define CAST_FILE_SAVE_MAX_SIZE  (500 * 1024 * 1024)    /* 500MB */
#define CAST_SLEEP_TIME  (5 * 1000)     /* 5ms */
#define INPUT_CMD_LENGTH 32
#define BYTE_TO_BIT 8
#define MB_TO_BIT (1024 * 1024)

hi_u32 g_file_limit_size;
FILE *g_aud_cast_file = HI_NULL;
static pthread_t g_cast_thread;
static hi_bool g_stop_thread = HI_FALSE;

hi_void cast_thread(hi_void *args)
{
    hi_s32 ret;
    hi_unf_audio_frame ao_frame;
    hi_u32 frame_size = 0;
    hi_u32 total_counter = 0;
    hi_u32 frame = 0;
    hi_handle h_cast = *(hi_handle *)args;

    while (!g_stop_thread) {
        ret = hi_unf_snd_acquire_cast_frame(h_cast, &ao_frame, 0);
        if (ret != HI_SUCCESS) {
            usleep(CAST_SLEEP_TIME);
            continue;
        }

        frame_size = ao_frame.pcm_samples * ao_frame.channels * sizeof(hi_u16);
        total_counter += frame_size;
        frame++;
        if (!(frame & 0x3f) || frame < 0x4) {
            printf("frame = 0x%x\n", frame);
        }

        if (total_counter <= g_file_limit_size * MB_TO_BIT) {
            fwrite(ao_frame.pcm_buffer, 1, frame_size, g_aud_cast_file);
        }
        hi_unf_snd_release_cast_frame(h_cast, &ao_frame);
        usleep(CAST_SLEEP_TIME);
    }

    if (g_aud_cast_file) {
        fclose(g_aud_cast_file);
        g_aud_cast_file = HI_NULL;
    }

    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle h_cast;
    hi_unf_snd_cast_attr cast_attr;
    hi_bool cast_enable;

    hi_char input_cmd[INPUT_CMD_LENGTH];

    if (argc < 3) {
        printf(" usage:    ./sample_audio_cast size(MB) file(*.pcm)\n");
        printf(" examples: \n");
        printf("           ./sample_audio_cast 100 /mnt/cast.pcm \n");
        return 0;
    }

    g_file_limit_size = strtol(argv[1], NULL, 0);
    if ((g_file_limit_size <= 0) || (g_file_limit_size > 500)) { /* 500 max file size */
        printf("file save length limit %d is not set or larger than 500MB\n", g_file_limit_size);
        return 0;
    } else {
        printf("file save length limit %d(MB)\n", g_file_limit_size);
    }

    if (strcasecmp("null", argv[2])) {
        g_aud_cast_file = fopen(argv[2], "wb");
        if (!g_aud_cast_file) {
            sample_printf("open file %s error!\n", argv[2]);
            return -1;
        }
    }

    hi_unf_sys_init();
    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call snd_init failed.\n");
        goto sys_deinit;
    }

    ret = hi_unf_snd_get_default_cast_attr(HI_UNF_SND_0, &cast_attr);
    if (ret != HI_SUCCESS) {
        printf("get default cast attr failed \n");
        goto snd_deinit;
    }

    ret = hi_unf_snd_create_cast(HI_UNF_SND_0,  &cast_attr, &h_cast);
    if (ret != HI_SUCCESS) {
        printf("cast create failed \n");
        goto snd_deinit;
    }

    printf("hi_unf_snd_set_cast_enable   HI_TRUE\n");
    ret = hi_unf_snd_set_cast_enable(h_cast, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("cast enable failed \n");
        goto cast_destroy;
    }

    g_stop_thread = HI_FALSE;
    pthread_create(&g_cast_thread, HI_NULL, (hi_void *)cast_thread, &h_cast);
    while (1) {
        printf("please input the q to quit!, s to disable cast channel , r to resable cast channel\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        if (input_cmd[0] == 's' || input_cmd[0] == 'S') {
            ret = hi_unf_snd_set_cast_enable(h_cast, HI_FALSE);
            if (ret != HI_SUCCESS) {
                printf("cast disable failed \n");
            }
        }

        if (input_cmd[0] == 'r' || input_cmd[0] == 'R') {
            ret = hi_unf_snd_set_cast_enable(h_cast, HI_TRUE);
            if (ret != HI_SUCCESS) {
                printf("cast enable failed \n");
            }
        }

        if (('g' == input_cmd[0]) || ('G' == input_cmd[0])) {
            ret = hi_unf_snd_get_cast_enable(h_cast, &cast_enable);
            if (ret != HI_SUCCESS) {
                printf("cast get_enable failed \n");
            } else
                printf("cast ID=0x%x enable=%d\n", h_cast, cast_enable);
        }
    }

    g_stop_thread = HI_TRUE;
    pthread_join(g_cast_thread, HI_NULL);

    ret = hi_unf_snd_set_cast_enable(h_cast, HI_FALSE);
    if (ret != HI_SUCCESS) {
        printf("cast enable failed \n");
    }

cast_destroy:
    hi_unf_snd_destroy_cast(h_cast);
snd_deinit:
    hi_adp_snd_deinit();
sys_deinit:
    hi_unf_sys_deinit();

    if (g_aud_cast_file) {
        fclose(g_aud_cast_file);
        g_aud_cast_file = HI_NULL;
    }

    return 0;
}
