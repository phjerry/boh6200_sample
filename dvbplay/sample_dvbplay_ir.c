/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : dvbplay_ir.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   :
  History       :
  1.Date        : 2010/01/26
    Author      : sdk
    Modification: Created file

******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"

#include "hi_unf_keyled.h"
#include "hi_unf_ir.h"
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
//#include "HA.AUDIO.DRA.decode.h"
#include "hi_unf_acodec_pcmdec.h"
//#include "HA.AUDIO.WMA9STD.decode.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

hi_u32 g_tunner_freq;
hi_u32 g_tunner_srate;
hi_u32 g_third_param;

static hi_s32 g_track_running = 1;
static hi_s32 g_chn_num  = 0;
static hi_s32 g_prog_num = 0;
static hi_s32 g_vol = 100;

hi_unf_video_format g_default_fromat  = HI_UNF_VIDEO_FMT_1080P_24;

#ifdef HI_KEYLED_SUPPORT
hi_unf_keyled_type g_KeyledType = 1;
#endif

hi_handle g_avplay_handle;
pmt_compact_tbl              *g_prog_tbl = HI_NULL;
hi_unf_avplay_stop_opt g_stop;

#define IR_CHN_UP 0x35caff00
#define IR_CHN_DOWN 0x2dd2ff00
#define IR_VOL_UP 0X3ec1ff00
#define IR_VOL_DOWN 0x6699ff00

#define KEY_CHN_UP 0x6
#define KEY_CHN_DOWN 0x1
#define KEY_VOL_UP 0x2
#define KEY_VOL_DOWN 0x4

#define THREAD_DELAY_TIME 50 * 1000 //50ms
#define VOL_MAX 100

hi_u8 g_dig_discode_std[10] = {0x03, 0x9F, 0x25, 0x0d, 0x99, 0x49, 0x41, 0x1f, 0x01, 0x09};

hi_u8 g_dig_discode[10] = {0};

#ifdef HI_KEYLED_SUPPORT
void *led_display_task(void *args)
{
    hi_u32 loop = 0;
    hi_s32 ret;

    while (g_track_running) {
        ret = hi_unf_led_display((g_dig_discode[loop + g_prog_num + 1] << 24) | (g_dig_discode[loop] << 16) |
            (g_dig_discode[loop] << 8) | g_dig_discode[loop]);

        if (ret != HI_SUCCESS) {
            sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
            break;
        }

        usleep(THREAD_DELAY_TIME);
    }

    return 0;
}
#endif

static hi_s32 dvb_prog_up_down(hi_s32 flag)
{
    if (flag) {         // program up
        ++g_prog_num;
        if (g_chn_num == g_prog_num) {
            g_prog_num = 0;
        }
    } else {            // program down
        g_prog_num--;
        if (-1 == g_prog_num) {
            g_prog_num = g_chn_num - 1;
        }
    }

    return 0;
}

static hi_s32 dvb_vol_set(hi_s32 flag)
{
    hi_unf_snd_gain snd_gain;
    snd_gain.linear_mode = HI_TRUE;

    if (flag) {          // add up vol
        g_vol += 5;
        if (g_vol > VOL_MAX) {
            g_vol = VOL_MAX;
        }
    } else {              //Vol down
        g_vol -= 5;
        if (g_vol < 0) {
            g_vol = 0;
        }
    }
    snd_gain.gain = g_vol;
    return hi_unf_snd_set_volume(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_ALL, &snd_gain);
}

#ifdef HI_KEYLED_SUPPORT
static void *key_receive_task(void *args)
{
    hi_s32 ret;
    hi_s32 loop_flag;
    hi_u32 press_status, key_id;

    while (g_track_running) {
        /*get KEY press value & press status*/
        ret = hi_unf_key_get_value(&press_status, &key_id);
        if ((ret != HI_SUCCESS) || (press_status == HI_UNF_KEY_STATUS_UP)) {
            continue;
        }

        loop_flag = 0;
        switch (key_id) {
        case KEY_CHN_UP:
            dvb_prog_up_down(1);
            loop_flag = 1;
            break;
        case KEY_CHN_DOWN:
            dvb_prog_up_down(0);
            loop_flag = 1;
            break;
        case KEY_VOL_UP:
            dvb_vol_set(1);
            break;
        case KEY_VOL_DOWN:
            dvb_vol_set(0);
            break;
        default:
            break;
        }

        if (loop_flag == 1) {
            hi_adp_avplay_play_prog(g_avplay_handle, g_prog_tbl, g_prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }

        usleep(THREAD_DELAY_TIME);
    }
    return 0;
}
#endif

static void *ir_receive_task(void *args)
{
    hi_s32 ret;
    hi_s32 loop_flag = 0;
    hi_u64 key_id;
    hi_unf_key_status press_status;
    char name[64] = "";   /* 64:Protocol name max len */

    while (g_track_running) {
        /* get ir press codevalue & press status*/
        ret = hi_unf_ir_get_value_protocol(&press_status, &key_id, name, sizeof(name), 0);
        if ((ret != HI_SUCCESS) || (press_status == HI_UNF_KEY_STATUS_UP)) {
            continue;
        }

        loop_flag = 0;
        if (ret == HI_SUCCESS) {
            switch (key_id) {
            case IR_CHN_UP:
                dvb_prog_up_down(1);
                loop_flag = 1;
                break;

            case IR_CHN_DOWN:
                dvb_prog_up_down(0);
                loop_flag = 1;
                break;

            case IR_VOL_UP:
                dvb_vol_set(1);
                break;

            case IR_VOL_DOWN:
                dvb_vol_set(0);
                break;

            default:
                break;
            }

            if (loop_flag == 1) {
                hi_adp_avplay_play_prog(g_avplay_handle, g_prog_tbl, g_prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
            }

            ret = HI_SUCCESS;
            usleep(THREAD_DELAY_TIME);
        }
    }
    return 0;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle win_handle;
    hi_unf_avplay_attr avplay_attr;
    hi_unf_sync_attr sync_attr;
    hi_char input_cmd[32];

    hi_handle track_handle;
    hi_unf_audio_track_attr track_attr;
    hi_u32                      tuner;
    pthread_t ir_thread;

#ifdef HI_KEYLED_SUPPORT
    pthread_t key_task_id;
    pthread_t ledTaskid;
#endif

    if (argc == 7) {
        g_tunner_freq  = strtol(argv[2], NULL, 0);
        g_tunner_srate = strtol(argv[3], NULL, 0);
        g_third_param = strtol(argv[4], NULL, 0);
        g_default_fromat = hi_adp_str2fmt(argv[5]);
        tuner = strtol(argv[6], NULL, 0);
    } else if (argc == 6) {
        g_tunner_freq  = strtol(argv[2], NULL, 0);
        g_tunner_srate = strtol(argv[3], NULL, 0);
        g_third_param = strtol(argv[4], NULL, 0);
        g_default_fromat = hi_adp_str2fmt(argv[5]);
        tuner = 0;
    } else if (argc == 5) {
        g_tunner_freq  = strtol(argv[2], NULL, 0);
        g_tunner_srate = strtol(argv[3], NULL, 0);
        g_third_param = strtol(argv[4], NULL, 0);
        g_default_fromat = HI_UNF_VIDEO_FMT_1080P_50;
        tuner = 0;
    } else if (argc == 4) {
        g_tunner_freq  = strtol(argv[2], NULL, 0);
        g_tunner_srate = strtol(argv[3], NULL, 0);
        g_third_param = (g_tunner_freq > 1000) ? 0 : 64;
        g_default_fromat = HI_UNF_VIDEO_FMT_1080P_50;
        tuner = 0;
    } else if (argc == 3) {
        g_tunner_freq  = strtol(argv[2], NULL, 0);
        g_tunner_srate = (g_tunner_freq > 1000) ? 27500 : 6875;
        g_third_param = (g_tunner_freq > 1000) ? 0 : 64;
        g_default_fromat = HI_UNF_VIDEO_FMT_1080P_50;
        tuner = 0;
    } else {
        printf("Usage: %s [keyled_type] [freq] [srate] [qamtype or polarization] [vo_format] [tuner]\n"
                "       keyled_type: 0: STD, 1: PT6961, 2: CT1642, 3: PT6964, 4: FD650, 5: GPIOKEY\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50  default HI_UNF_VIDEO_FMT_1080P_50\n"
                "       tuner: value can be 0, 1, 2, 3\n",
                argv[0]);
        printf("Example:%s 1 610 6875 64 1080i50 0\n", argv[0]);
        return HI_FAILURE;
    }

#ifdef HI_KEYLED_SUPPORT
    g_KeyledType  = strtol(argv[1], NULL, 0);
#endif

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    //hi_adp_mce_exit();

    ret = hi_unf_ir_init();
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto SYS_DEINIT;
    }

    ret = hi_unf_ir_enable_keyup(HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto IR_CLOSE;
    }

    ret = hi_unf_ir_set_repkey_timeout(300);   //???  108< parament   <65536
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto IR_CLOSE;
    }

    ret = hi_unf_ir_enable_repkey(HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto IR_CLOSE;
    }

    ret = hi_unf_ir_enable(HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto IR_CLOSE;
    }

#ifdef HI_KEYLED_SUPPORT
    ret = hi_unf_keyled_init();
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto IR_CLOSE;
    }

    ret = hi_unf_keyled_select_type(g_KeyledType);  /*standard hisi keyboard*//*CNcomment:标准海思键盘 0*/
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto KEYLED_DEINIT;
    }

    /*open LED device*/
    ret = hi_unf_led_open();
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto KEYLED_DEINIT;
    }

    /*enable flash*/
    ret = hi_unf_led_set_flash_freq(HI_UNF_KEYLED_LEVEL_1);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto LED_CLOSE;
    }

    /*config LED flash or not*/
    ret = hi_unf_led_set_flash_pin(HI_UNF_KEYLED_LIGHT_NONE);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto LED_CLOSE;
    }

    /*open KEY device*/
    ret = hi_unf_key_open();   /*block mode*/
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto LED_CLOSE;
    }

    /*config keyup is valid*/
    ret = hi_unf_key_is_keyup(1);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto KEY_CLOSE;
    }

    /*config keyhold is valid*/
    ret = hi_unf_key_is_repkey(1);
    if (ret != HI_SUCCESS) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto KEY_CLOSE;
    }
#endif
    /******************************************************************************
                                 HI_AVPALY_NINT

    ******************************************************************************/

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_snd_init failed.\n");
        goto KEY_CLOSE;
    }

    /*HI_SYS_GetPlayTime(&u32Playtime);
    sample_printf("u32Playtime = %d\n", u32Playtime);*/
    ret = hi_adp_disp_init(g_default_fromat);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call  hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    ret = hi_adp_win_create(HI_NULL, &win_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_win_create failed.\n");
        goto  VO_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_dmx_init failed.\n");
        goto  VO_DESTROYWIN;
    }

    ret = hi_adp_dmx_attach_ts_port(0, tuner);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_dmx_attach_ts_port failed.\n");
        goto DMX_DEINIT;
    }

#ifdef HI_FRONTEND_SUPPORT
    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto DMX_DETACH;
    }

    ret = hi_adp_frontend_connect(tuner, g_tunner_freq, g_tunner_srate, g_third_param);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }
#endif

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(0, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_search_get_all_pmt failed\n");
        goto hi_adp_search_de_init;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto  PSISI_FREE;
    }

    ret  = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.vid_buf_size = 0x300000;
    ret |= hi_unf_avplay_create(&avplay_attr, &g_avplay_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret  = hi_unf_avplay_chan_open(g_avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    ret |= hi_unf_avplay_chan_open(g_avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_win_attach_src(win_handle, g_avplay_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_attach_src failed.\n");
        goto CHN_CLOSE;
    }

    ret = hi_unf_win_set_enable(win_handle, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track_handle, g_avplay_handle);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(g_avplay_handle, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    sync_attr.start_region.vid_plus_time = 100;
    sync_attr.start_region.vid_negative_time = -100;
    sync_attr.quick_output_enable = HI_FALSE;
    ret = hi_unf_avplay_set_attr(g_avplay_handle, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    memcpy(g_dig_discode, g_dig_discode_std, sizeof(g_dig_discode_std));

    ret = hi_adp_avplay_play_prog(g_avplay_handle, g_prog_tbl, g_prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        sample_printf("call SwitchProg failed.\n");
        goto AVPLAY_STOP;
    }

    g_chn_num = g_prog_tbl->prog_num;

    /******************************************************************************
                                  create three threads.

    ******************************************************************************/

    ret = pthread_create(&ir_thread, NULL, ir_receive_task, NULL);
    if (ret != 0) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        perror("pthread_create");
        goto AVPLAY_STOP;
    }
#ifdef HI_KEYLED_SUPPORT
    ret = pthread_create(&key_task_id, NULL, key_receive_task, NULL);
    if (ret != 0) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        perror("pthread_create");
        goto ERR1;
    }

    ret = pthread_create(&ledTaskid, NULL, led_display_task, NULL);
    if (ret != 0) {
        sample_printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        perror("pthread_create");
        goto ERR2;
    }
#endif
    while (1) {
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

    /*exit the three threads that ledTaskid and key_task_id ir_thread */
    g_track_running = 0;
#ifdef HI_KEYLED_SUPPORT
    pthread_join(ledTaskid, 0);
ERR2:
    pthread_join(key_task_id, 0);
ERR1:
#endif
    pthread_join(ir_thread, 0);

AVPLAY_STOP:
    g_stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    g_stop.timeout = 0;
    hi_unf_avplay_stop(g_avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &g_stop);

SND_DETACH:
    hi_unf_snd_detach(track_handle, g_avplay_handle);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track_handle);

WIN_DETACH:
    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, g_avplay_handle);

CHN_CLOSE:
    hi_unf_avplay_chan_close(g_avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(g_avplay_handle);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_tbl);

hi_adp_search_de_init:
    hi_adp_search_de_init();

#ifdef HI_FRONTEND_SUPPORT
TUNER_DEINIT:
    hi_adp_frontend_deinit();

DMX_DETACH:
#endif
    hi_unf_dmx_detach_ts_port(0);

DMX_DEINIT:
    hi_unf_dmx_deinit();

VO_DESTROYWIN:
    hi_unf_win_destroy(win_handle);

VO_DEINIT:
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

KEY_CLOSE:
#ifdef HI_KEYLED_SUPPORT

    hi_unf_key_close();

LED_CLOSE:
    hi_unf_led_close();

KEYLED_DEINIT:
    hi_unf_keyled_deinit();
#endif
IR_CLOSE:
    hi_unf_ir_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return ret;
}
