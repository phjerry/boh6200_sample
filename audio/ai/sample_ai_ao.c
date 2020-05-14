/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: ai ao sample
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

#include "hi_unf_system.h"
#include "hi_unf_ai.h"
#include "hi_unf_sound.h"
#include "hi_adp_mpi.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define INPUT_CMD_LENGTH 32

hi_s32 main(hi_void)
{
    hi_s32 ret;
    hi_handle h_ai;
    hi_unf_ai_attr ai_attr;
    hi_handle slave_track;
    hi_unf_audio_track_attr track_attr;
    hi_char input_cmd[INPUT_CMD_LENGTH];

    hi_unf_sys_init();

    ret = hi_unf_ai_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_init failed.\n");
        goto sys_deinit;
    }

    ret = hi_unf_ai_get_default_attr(HI_UNF_AI_PORT_I2S0, &ai_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_get_default_attr failed\n");
        goto ai_deinit;
    }
    ai_attr.pcm_frame_max_num = 0x8;

    ret = hi_unf_ai_create(HI_UNF_AI_PORT_I2S0, &ai_attr, &h_ai);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_ai_get_default_attr failed\n");
        goto ai_deinit;
    }

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_snd_init failed\n");
        goto ai_destroy;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto snd_deinit;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &slave_track);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto snd_deinit;
    }

    ret = hi_unf_snd_attach(slave_track, h_ai);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto track_slave_destroy;
    }

    ret = hi_unf_ai_set_enable(h_ai, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto track_slave_detach;
    }

    while (1) {
        sample_printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            sample_printf("prepare to quit!\n");
            break;
        }
    }

    hi_unf_ai_set_enable(h_ai, HI_FALSE);

track_slave_detach:
    hi_unf_snd_detach(slave_track, h_ai);
track_slave_destroy:
    hi_unf_snd_destroy_track(slave_track);
snd_deinit:
    hi_adp_snd_deinit();
ai_destroy:
    hi_unf_ai_destroy(h_ai);
ai_deinit:
    hi_unf_ai_deinit();
sys_deinit:
    hi_unf_sys_deinit();

    return 0;
}
