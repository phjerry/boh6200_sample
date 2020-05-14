/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:play streams by using PVR
 * Author: sdk
 * Create: 2019-08-20
 */

#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_adp_pvr.h"

#define SAMPLE_PVR_TRICK_WITH_AUDIO_ENABLE_MASK 0x0F000000

static hi_unf_video_format g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;

hi_bool g_play_stop = HI_FALSE;
hi_u32 play_chn;
static pthread_t g_sta_thd;

hi_s32 pvr_play_dmx_init(hi_void)
{
    hi_s32   ret;

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_init failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 pvr_play_dmx_deinit(hi_void)
{
    hi_s32 ret;

    SAMPLE_RUN(hi_unf_dmx_detach_ts_port(PVR_DMX_ID_LIVE), ret);
    SAMPLE_RUN(hi_unf_dmx_deinit(), ret);
    return ret;
}

hi_s32 draw_time_string(hi_char *text)
{
    pvr_sample_printf("current Time:%s\n", text);
    return HI_SUCCESS;
}

hi_s32 draw_status_str(hi_char *text)
{
    pvr_sample_printf("STATUS:%s\n", text);
    return 0;
}

hi_void status_trick_with_audio_enable(hi_unf_pvr_play_speed speed)
{
    hi_u32 ratio = 1000;
    hi_char stat_str[32];

    if ((speed & SAMPLE_PVR_TRICK_WITH_AUDIO_ENABLE_MASK) == HI_UNF_PVR_TRICK_WITH_AUDIO_ENABLE) {
        switch(speed) {
            case HI_UNF_PVR_PLAY_SPEED_0_5X_WITH_AUDIO:
                ratio = 500;
                break;
            case HI_UNF_PVR_PLAY_SPEED_0_8X_WITH_AUDIO:
                ratio = 800;
                break;
            case HI_UNF_PVR_PLAY_SPEED_0_9X_WITH_AUDIO:
                ratio = 900;
                break;
            case HI_UNF_PVR_PLAY_SPEED_1_1X_WITH_AUDIO:
                ratio = 1100;
                break;
            case HI_UNF_PVR_PLAY_SPEED_1_2X_WITH_AUDIO:
                ratio = 1200;
                break;
            case HI_UNF_PVR_PLAY_SPEED_1_5X_WITH_AUDIO:
                ratio = 1500;
                break;
            default:
                break;
        }
        snprintf(stat_str, sizeof(stat_str),"Play %01d.%01d", ratio / 1000, (ratio % 1000) / 100);
    }
    return;
}

hi_void status_for_state_sf(hi_unf_pvr_play_speed speed)
{
    hi_char stat_str[32];
    if (HI_UNF_PVR_PLAY_SPEED_NORMAL % speed == 0) {
        snprintf(stat_str, sizeof(stat_str),"SF 1/%dX", HI_UNF_PVR_PLAY_SPEED_NORMAL / speed);
    } else {
        snprintf(stat_str, sizeof(stat_str),"SF 3/4X");
    }
    return;
}

hi_void status_for_state_sb(hi_unf_pvr_play_speed speed)
{
    hi_char stat_str[32];

    if (HI_UNF_PVR_PLAY_SPEED_NORMAL % speed == 0) {
        snprintf(stat_str, sizeof(stat_str),"SB -1/%dX", HI_UNF_PVR_PLAY_SPEED_NORMAL / abs(speed));
    } else {
        snprintf(stat_str, sizeof(stat_str),"SB -3/4X");
    }
    return;
}

hi_void *statu_thread(hi_void *args)
{
    hi_s32 ret;
    hi_unf_pvr_play_status status;
    hi_char timestr[20];
    hi_char stat_str[32];

    while (g_play_stop == HI_FALSE) {
        sleep(1);

        ret = hi_unf_pvr_play_chan_get_status(play_chn, &status);
        if (ret == HI_SUCCESS) {
            snprintf(timestr, sizeof(timestr),"%d", status.cur_play_time_ms / 1000);
            draw_time_string(timestr);
            if(status.state == HI_UNF_PVR_PLAY_STATE_INIT) {
                snprintf(stat_str, sizeof(stat_str),"Init");
            } else if (status.state == HI_UNF_PVR_PLAY_STATE_PLAY) {
                snprintf(stat_str, sizeof(stat_str), "Play");
                status_trick_with_audio_enable(status.speed);
            } else if(status.state == HI_UNF_PVR_PLAY_STATE_PAUSE) {
                snprintf(stat_str, sizeof(stat_str),"Pause");
            } else if(status.state == HI_UNF_PVR_PLAY_STATE_FF) {
                snprintf(stat_str, sizeof(stat_str),"FF %dX", status.speed / HI_UNF_PVR_PLAY_SPEED_NORMAL);
            } else if (status.state == HI_UNF_PVR_PLAY_STATE_FB) {
                snprintf(stat_str, sizeof(stat_str),"FB %dX", status.speed / HI_UNF_PVR_PLAY_SPEED_NORMAL);
            } else if (status.state == HI_UNF_PVR_PLAY_STATE_SF) {
                status_for_state_sf(status.speed);
            } else if(status.state == HI_UNF_PVR_PLAY_STATE_SB) {
                status_for_state_sb(status.speed);
            } else if (status.state == HI_UNF_PVR_PLAY_STATE_STEPF) {
                snprintf(stat_str, sizeof(stat_str),"Step F");
            } else if (status.state == HI_UNF_PVR_PLAY_STATE_STEPB) {
                snprintf(stat_str, sizeof(stat_str),"Step B");
            } else if (status.state == HI_UNF_PVR_PLAY_STATE_STOP) {
                snprintf(stat_str, sizeof(stat_str),"Stop");
            } else {
                snprintf(stat_str, sizeof(stat_str),"InValid(%d)", status.state);
            }
            draw_status_str(stat_str);
        }
    }

    return HI_NULL;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_handle win;
    hi_handle av;
    hi_handle sound_track;
    hi_char input_cmd[32];
    hi_char cur_path[256] = {0};

    if (argc < 2) {
        printf("Usage: %s recTsFile [vo_format]\n", argv[0]);
        printf("       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n");
        return 0;
    } else if(argc == 3) {
        g_default_fmt = hi_adp_str2fmt(argv[2]);
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();
    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_snd_init failed.\n");
        return 0;
    }

    ret = hi_adp_disp_init(g_default_fmt);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_disp_init failed.\n");
        return 0;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        return 0;
    }

    ret = pvr_play_dmx_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        return ret;
    }

    ret = sample_pvr_av_init(win, &av, &sound_track);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        return ret;
    }


    ret = hi_unf_pvr_play_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_mpi_pvr_play_init failed.\n");
        return ret;
    }

    ret = sample_pvr_register_cb();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call PVR_RegisterCallBacks failed.\n");
        return ret;
    }

    ret = sample_pvr_start_playback(argv[1], &play_chn, av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call sample_pvr_start_playback failed.\n");
        return ret;
    }

    memset(cur_path, 0, sizeof(cur_path));
    memcpy(cur_path, argv[0], (hi_u32)(strstr(argv[0], "sample_pvr_play") - argv[0]));
    pthread_create(&g_sta_thd, HI_NULL, statu_thread, HI_NULL);

    while (g_play_stop == HI_FALSE) {
        static hi_u32 f_times = 0;
        static hi_u32 s_times = 0;
        static hi_u32 r_times = 0;
        static hi_u32 audio_times = 0;
        static hi_u32 b_times = 0;

        pvr_sample_printf("Press q to exit, h for help...\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            pvr_sample_printf("prepare to exit!\n");
            g_play_stop = HI_TRUE;
        }

        if (input_cmd[0] == 'h') {
            printf("commond:\n");
            printf("    n: normal play\n");
            printf("    p: pause\n");
            printf("    f: fast forward(2x/4x/8x/12x/16x/24x/32x/64x)\n");
            printf("    s: slow forward(0.75x/2x/4x/8x/16x/32x/64x)\n");
            printf("    r: fast rewind(1x/2x/4x/8x/12x/16x/24x/32x/64x)\n");
            printf("    b: slow rewind(0.75x/2x/4x)\n");
            printf("    t: step forward one frame\n");
            printf("    k: seek to start\n");
            printf("    e: seek to end\n");
            printf("    l: seek forward 5 second\n");
            printf("    j: seek rewwind 5 second\n");
            printf("    a: 0.5x, 0.8x, 0.9x, 1.1x, 1.2x, 1.5x play with audio enabled\n");
            printf("    x: destroy play channel and create again\n");
            printf("    h: help\n");
            printf("    q: quit\n");
            continue;
        }

        if (input_cmd[0] == 'f') {
            hi_unf_pvr_play_mode trick_mode;
            hi_unf_pvr_play_speed ff_speed[8] = {
                HI_UNF_PVR_PLAY_SPEED_2X_FAST_FORWARD, HI_UNF_PVR_PLAY_SPEED_4X_FAST_FORWARD,
                HI_UNF_PVR_PLAY_SPEED_8X_FAST_FORWARD, HI_UNF_PVR_PLAY_SPEED_12X_FAST_FORWARD,
                HI_UNF_PVR_PLAY_SPEED_16X_FAST_FORWARD, HI_UNF_PVR_PLAY_SPEED_24X_FAST_FORWARD,
                HI_UNF_PVR_PLAY_SPEED_32X_FAST_FORWARD, HI_UNF_PVR_PLAY_SPEED_64X_FAST_FORWARD
            };
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            trick_mode.speed = ff_speed[f_times % 8];
            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_tplay failed.\n");
                continue;
            }

            f_times++;
            continue;
        }
        if (input_cmd[0] == 's') {
            hi_unf_pvr_play_mode trick_mode;
            hi_unf_pvr_play_speed sf_speed[7] = {
                HI_UNF_PVR_PLAY_SPEED_0_75X_SLOW_FORWARD, HI_UNF_PVR_PLAY_SPEED_2X_SLOW_FORWARD,
                HI_UNF_PVR_PLAY_SPEED_4X_SLOW_FORWARD, HI_UNF_PVR_PLAY_SPEED_8X_SLOW_FORWARD,
                HI_UNF_PVR_PLAY_SPEED_16X_SLOW_FORWARD, HI_UNF_PVR_PLAY_SPEED_32X_SLOW_FORWARD,
                HI_UNF_PVR_PLAY_SPEED_64X_SLOW_FORWARD
            };
            f_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            trick_mode.speed = sf_speed[s_times % 7];

            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_tplay failed.\n");
                continue;
            }

            s_times++;
            continue;
        }
        if (input_cmd[0] == 'a') {
            hi_unf_pvr_play_mode audio_trick_mode;
            hi_unf_pvr_play_position pos;
            hi_unf_pvr_play_speed audio_speed[6] = {
                HI_UNF_PVR_PLAY_SPEED_0_5X_WITH_AUDIO, HI_UNF_PVR_PLAY_SPEED_0_8X_WITH_AUDIO,
                HI_UNF_PVR_PLAY_SPEED_0_9X_WITH_AUDIO, HI_UNF_PVR_PLAY_SPEED_1_1X_WITH_AUDIO,
                HI_UNF_PVR_PLAY_SPEED_1_2X_WITH_AUDIO, HI_UNF_PVR_PLAY_SPEED_1_5X_WITH_AUDIO
            };

            f_times = 0;
            s_times = 0;
            r_times = 0;
            b_times = 0;

            pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
            pos.whence = SEEK_SET;
            pos.offset = 0;
            ret = hi_unf_pvr_play_chan_seek(play_chn, &pos);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_seek failed. ret = 0x%x\n", ret);
                continue;
            }

            audio_trick_mode.speed = audio_speed[audio_times % 6];
            ret = hi_unf_pvr_play_chan_tplay(play_chn, &audio_trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_tplay failed. ret = 0x%x\n", ret);
                continue;
            }
            audio_times++;
        }
        if (input_cmd[0] == 'r') {
            hi_unf_pvr_play_mode trick_mode;
            hi_unf_pvr_play_speed fb_speed[9] = {
                HI_UNF_PVR_PLAY_SPEED_1X_FAST_BACKWARD, HI_UNF_PVR_PLAY_SPEED_2X_FAST_BACKWARD,
                HI_UNF_PVR_PLAY_SPEED_4X_FAST_BACKWARD, HI_UNF_PVR_PLAY_SPEED_8X_FAST_BACKWARD,
                HI_UNF_PVR_PLAY_SPEED_12X_FAST_BACKWARD, HI_UNF_PVR_PLAY_SPEED_16X_FAST_BACKWARD,
                HI_UNF_PVR_PLAY_SPEED_24X_FAST_BACKWARD, HI_UNF_PVR_PLAY_SPEED_32X_FAST_BACKWARD,
                HI_UNF_PVR_PLAY_SPEED_64X_FAST_BACKWARD
            };

            f_times = 0;
            s_times = 0;
            audio_times = 0;
            b_times = 0;

            trick_mode.speed = fb_speed[r_times % 9];
            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_tplay failed.\n");
                continue;
            }

            r_times++;
            continue;
        }
        if (input_cmd[0] == 'b') {
            hi_unf_pvr_play_mode trick_mode;
            hi_unf_pvr_play_speed sb_speed[3] = {
                HI_UNF_PVR_PLAY_SPEED_0_75X_SLOW_BACKWARD, HI_UNF_PVR_PLAY_SPEED_2X_SLOW_BACKWARD,
                HI_UNF_PVR_PLAY_SPEED_4X_SLOW_BACKWARD
            };
            f_times = 0;
            s_times = 0;
            audio_times = 0;
            r_times = 0;

            trick_mode.speed = sb_speed[b_times % 3];
            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_tplay failed.\n");
                continue;
            }

            b_times++;
            continue;
        }

        if (input_cmd[0] == 'n') {
            f_times = 0;
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            pvr_sample_printf("PVR normal play now.\n");
            ret = hi_unf_pvr_play_chan_resume(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                continue;
            }

            continue;
        }

        if (input_cmd[0] == 'p') {
            f_times = 0;
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            pvr_sample_printf("PVR pause now.\n");
            ret = hi_unf_pvr_play_chan_pause(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
                continue;
            }

            continue;
        }

        if (input_cmd[0] == 't') {
            f_times = 0;
            s_times = 0;
            audio_times = 0;
            r_times = 0;
            b_times = 0;

            pvr_sample_printf("PVR play step now.\n");
            ret = hi_unf_pvr_play_chan_step_forward(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_step_forward failed.\n");
                continue;
            }

            continue;
        }

        if (input_cmd[0] == 'k') {
            hi_unf_pvr_play_position pos;
            hi_unf_pvr_file_attr f_attr;
            f_times = 0;
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            ret = hi_unf_pvr_play_chan_get_file_attr(play_chn, &f_attr);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_get_file_attr failed.\n");
                continue;
            }

            ret = hi_unf_pvr_play_chan_resume(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                continue;
            }

            pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
            pos.offset = f_attr.start_time_ms;
            pos.whence = SEEK_SET;
            pvr_sample_printf("seek to start\n");


            ret = hi_unf_pvr_play_chan_seek(play_chn, &pos);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_seek failed.\n");
                continue;
            }
            continue;
        }

        if (input_cmd[0] == 'l') {
            hi_unf_pvr_play_position pos;
            f_times = 0;
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            ret = hi_unf_pvr_play_chan_resume(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                continue;
            }

            pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
            pos.offset = 5000;
            pos.whence = SEEK_CUR;
            pvr_sample_printf("seek forward 5 Second\n");

            ret = hi_unf_pvr_play_chan_seek(play_chn, &pos);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_step_forward failed.\n");
                continue;
            }
            continue;
        }
        if (input_cmd[0] == 'j') {
            hi_unf_pvr_play_position pos;
            f_times = 0;
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            ret = hi_unf_pvr_play_chan_resume(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                continue;
            }

            pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
            pos.offset = -5000;
            pos.whence = SEEK_CUR;
            pvr_sample_printf("seek reward 5 Second\n");

            ret = hi_unf_pvr_play_chan_seek(play_chn, &pos);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_step_forward failed.\n");
                continue;
            }
            continue;
        }

        if (input_cmd[0] == 'e') {
            hi_unf_pvr_play_position pos;
            f_times = 0;
            s_times = 0;
            r_times = 0;
            audio_times = 0;
            b_times = 0;

            ret = hi_unf_pvr_play_chan_resume(play_chn);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                continue;
            }

            pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
            pos.offset = 0;
            pos.whence = SEEK_END;
            pvr_sample_printf("seek end\n");

            ret = hi_unf_pvr_play_chan_seek(play_chn, &pos);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_step_forward failed.\n");
                continue;
            }
            continue;
        }

        if (input_cmd[0] == 'x') {
            sample_pvr_stop_playback(play_chn);

            ret = sample_pvr_start_playback(argv[1], &play_chn, av);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call sample_pvr_start_playback failed.\n");
                continue;
            }
            continue;
        }
    }

    if(g_sta_thd != 0) {
        pthread_join(g_sta_thd, HI_NULL);
    }

    sample_pvr_stop_playback(play_chn);

    sample_pvr_av_deinit(av, win, sound_track);

    PRINT_SMP(">>==dmx_deinit.\n");
    ret = pvr_play_dmx_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call dmx_deinit failed.\n");
        return ret;
    }

    hi_unf_win_destroy(win);
    hi_adp_win_deinit();
    hi_adp_disp_deinit();
    hi_adp_snd_deinit();
    hi_unf_sys_deinit();

    return 0;
}


