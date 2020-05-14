/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:
 * Author: sdk
 * Create: 2019-08-20
 */

#include <stdlib.h>
#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_adp_pvr.h"

#ifdef ENABLE_IR_PRG
#include "hi_unf_ir.h"
#endif

static pthread_t g_TimeThread;
hi_u32 g_play_chn = 0xffffffff;
hi_u32 g_tuner_freq;
hi_u32 g_tuner_srate;
hi_u32 g_third_param;
hi_u32 g_tuner_id = 0;

hi_bool g_play_stop = HI_FALSE;
static hi_unf_video_format g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
pmt_compact_tbl *g_prog_tbl = HI_NULL;

hi_handle layer_pvr = HI_INVALID_HANDLE;
hi_handle layer_surface_pvr;
hi_handle font_pvr = HI_INVALID_HANDLE;

#ifdef ENABLE_IR_PRG
pthread_t g_ir_thread = 0;
hi_bool g_ir_task_runing = HI_FALSE;
hi_handle g_ir_avplay = 0;

hi_s32 ir_process(hi_u32 rec_chn);
#endif

hi_s32 dmx_init_and_search(hi_u32 tuner_id)
{
    hi_s32 ret;

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_init failed.\n");
        return ret;
    }
    ret = hi_adp_dmx_attach_ts_port(PVR_DMX_ID_LIVE, tuner_id);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call HI_UNF_DMX_AttachTSPort failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    ret = hi_adp_dmx_attach_ts_port(PVR_DMX_ID_REC, tuner_id);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call HI_UNF_DMX_AttachTSPort for REC failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PVR_DMX_ID_LIVE, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_search_get_all_pmt failed\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 dmx_deinit(hi_void)
{
    hi_s32 ret;

    hi_adp_search_free_all_pmt(g_prog_tbl);

    ret = hi_unf_dmx_detach_ts_port(PVR_DMX_ID_LIVE);
    ret |= hi_unf_dmx_detach_ts_port(PVR_DMX_ID_REC);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_detach_ts_port failed.\n");
        return ret;
    }

    ret = hi_unf_dmx_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 switch_to_shift_play(const hi_char *file_name, hi_u32 *play_chn, hi_handle av)
{

    sample_pvr_switch_dmx_source(PVR_DMX_ID_LIVE, PVR_DMX_PORT_ID_PLAYBACK);
    sample_pvr_start_playback(file_name, play_chn, av);

    return HI_SUCCESS;
}


hi_s32 switch_to_live_play(hi_handle play_chn, hi_handle av, const pmt_compact_prog *prog_info)
{
    hi_u32 ret;
    sample_pvr_stop_playback(play_chn);

    hi_unf_dmx_detach_ts_port(PVR_DMX_ID_LIVE);
    ret = hi_adp_dmx_attach_ts_port(PVR_DMX_ID_LIVE, g_tuner_id);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_dmx_attach_ts_port for REC failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    sample_pvr_start_live(av, prog_info);
    return HI_SUCCESS;
}

hi_void draw_string(hi_u32 u32TimeType, hi_char *szText)
{
    pvr_sample_printf("%s\n", szText);
    return;
}

hi_void *statu_thread(hi_void *args)
{
    hi_u32 rec_chn = *(hi_u32 *)args;
    hi_s32 ret;
    hi_unf_pvr_rec_attr rec_attr;
    hi_unf_pvr_rec_status rec_status;
    hi_unf_pvr_play_status play_status;
    hi_unf_pvr_file_attr file_status;
    hi_char rec_time_str[20];
    hi_char play_time_str[20];
    hi_char file_time_str[20];
    hi_char stat_str[32];

    while (g_play_stop == HI_FALSE) {
        sleep(1);
        (void)hi_unf_pvr_rec_chan_get_attr(rec_chn, &rec_attr);
#ifdef ANDROID
        pvr_sample_printf("----------------------------------\n");
#endif
        ret = hi_unf_pvr_rec_chan_get_status(rec_chn, &rec_status);
        if (ret == HI_SUCCESS) {
            snprintf(rec_time_str, sizeof(rec_time_str),"Rec time:%ds", rec_status.cur_time_ms / 1000);
            draw_string(1, rec_time_str);
            snprintf(rec_time_str, sizeof(rec_time_str),"Rec len:%ds",
                (rec_status.end_time_ms - rec_status.start_time_ms) / 1000);
            draw_string(2, rec_time_str);
        }

        ret = hi_unf_pvr_get_file_attr_by_file_name(rec_attr.file_name.name, &file_status);
        if (ret == HI_SUCCESS) {
            snprintf(rec_time_str, sizeof(rec_time_str),"Rec size:%lld.%lldM",
                (file_status.valid_size_byte) / 0x100000,
                ((file_status.valid_size_byte) % 0x100000) / 0x400);
            draw_string(3, rec_time_str);
        }

        if (g_play_chn != 0xffffffff) {
            if (ret == HI_SUCCESS) {
                snprintf(file_time_str, sizeof(file_time_str),"File end time:%ds", file_status.end_time_ms / 1000);
                draw_string(4, file_time_str);
            }

            ret = hi_unf_pvr_play_chan_get_status(g_play_chn, &play_status);
            if (ret == HI_SUCCESS) {
                snprintf(play_time_str, sizeof(play_time_str),"Play time:%ds", play_status.cur_play_time_ms / 1000);
                draw_string(5, play_time_str);
                switch(play_status.state) {
                    case HI_UNF_PVR_PLAY_STATE_INIT:
                        snprintf(stat_str, sizeof(stat_str),"Init");
                        break;
                    case HI_UNF_PVR_PLAY_STATE_PLAY:
                        snprintf(stat_str, sizeof(stat_str),"Play");
                        break;
                    case HI_UNF_PVR_PLAY_STATE_PAUSE:
                        snprintf(stat_str, sizeof(stat_str),"Pause");
                        break;
                    case HI_UNF_PVR_PLAY_STATE_FF:
                        snprintf(stat_str, sizeof(stat_str),"FF %dX",
                            play_status.speed / HI_UNF_PVR_PLAY_SPEED_NORMAL);
                        break;
                    case HI_UNF_PVR_PLAY_STATE_FB:
                        snprintf(stat_str, sizeof(stat_str),"FB %dX",
                            play_status.speed / HI_UNF_PVR_PLAY_SPEED_NORMAL);
                        break;
                    case HI_UNF_PVR_PLAY_STATE_SF:
                        snprintf(stat_str, sizeof(stat_str),"SF 1/%dX",
                            HI_UNF_PVR_PLAY_SPEED_NORMAL/play_status.speed);
                        break;
                    case HI_UNF_PVR_PLAY_STATE_SB:
                        snprintf(stat_str, sizeof(stat_str),"SB 1/%dX",
                            HI_UNF_PVR_PLAY_SPEED_NORMAL/play_status.speed);
                        break;
                    case HI_UNF_PVR_PLAY_STATE_STEPF:
                        snprintf(stat_str, sizeof(stat_str),"Step F");
                        break;
                    case HI_UNF_PVR_PLAY_STATE_STEPB:
                        snprintf(stat_str, sizeof(stat_str),"Step B");
                        break;
                    case HI_UNF_PVR_PLAY_STATE_STOP:
                        snprintf(stat_str, sizeof(stat_str),"Stop");
                        break;
                    default:
                        snprintf(stat_str, sizeof(stat_str),"InValid(%d)", play_status.state);
                        break;
                }
                draw_string(0, stat_str);
            }
        }
#ifdef ANDROID
        pvr_sample_printf("----------------------------------\n\n");
#endif
    }

    return HI_NULL;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_u32 i;
    hi_u32 tuner;
    hi_u32 prog_cnt = 0;
    hi_u32 prog_num[8] = {0}; /* one demux can support creating 8 record channel */
    hi_u32 rec_chn[8];
    hi_s32 ret;
    hi_char input_cmd[32];
    hi_bool live = HI_TRUE;
    hi_bool pause = HI_FALSE;
    hi_handle av;
    hi_handle sound_track;
    hi_handle win;
    hi_bool circular_to_linear = HI_FALSE;
    hi_unf_pvr_rec_attr rec_attr;
    pmt_compact_prog *cur_prog_info;
    hi_char cur_path[256] = {0};
#ifdef DISPLAY_JPEG
    hi_handle layer_hd;
    hi_u32 jpeg = 0;
#endif

    if (argc == 8) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = hi_adp_str2fmt(argv[5]);
        tuner = strtol(argv[6],HI_NULL,0);
#ifdef DISPLAY_JPEG
        jpeg = strtol(argv[7],HI_NULL,0);
#endif
    } else if (argc == 7) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = hi_adp_str2fmt(argv[5]);
        tuner = strtol(argv[6],HI_NULL,0);
    } else if (argc == 6) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = hi_adp_str2fmt(argv[5]);
        tuner = 0;
    } else if (argc == 5) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
        tuner = 0;
    } else if(argc == 4) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = (g_tuner_freq > 1000) ? 0 : 64;
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
        tuner = 0;
    } else if(argc == 3) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = (g_tuner_freq > 1000) ? 27500 : 6875;
        g_third_param = (g_tuner_freq > 1000) ? 0 : 64;
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
        tuner = 0;
    } else {
        printf("Usage: %s file_path freq [srate] [qamtype or polarization] [vo_format] [tuner] [JPEG]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n"
                "       tuner: value can be 0, 1, 2, 3\n", argv[0]);
        printf("Example: %s ./ 618 6875 64 1080P50 0 0\n", argv[0]);
        printf("    Satellite : %s ./ 3840 27500 0 1080P50 0 0\n", argv[0]);
        printf("    Cable     : %s ./ 618 6875 64 1080P50 1 0\n", argv[0]);
        printf("    Terestrial: %s ./ 474 8000 64 1080P50 2 0\n", argv[0]);
        return HI_FAILURE;
    }

    g_tuner_id = tuner;
    hi_unf_sys_init();

    //hi_adp_mce_exit();
    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call HIADP_Demux_Init failed.\n");
        return ret;
    }

    pvr_sample_printf("hi_adp_frontend_connect, frequency:%d, Symbol:%d, Qam:%d\n",
        g_tuner_freq, g_tuner_srate, g_third_param);

    ret = hi_adp_frontend_connect(tuner, g_tuner_freq, g_tuner_srate, g_third_param);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_frontend_connect failed. ret = 0x%x\n", ret);
        return ret;
    }

    ret = dmx_init_and_search(tuner);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        return ret;
    }

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_snd_init failed.\n");
        return ret;
    }

    ret = hi_adp_disp_init(g_default_fmt);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_disp_deinit failed.\n");
        return ret;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        return ret;
    }

    ret = sample_pvr_av_init(win, &av, &sound_track);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        hi_adp_win_deinit();
        return ret;
    }

#ifdef DISPLAY_JPEG
    memset(cur_path, 0, sizeof(cur_path));
    memcpy(cur_path, argv[0], (hi_u32)(strstr(argv[0], "sample_pvr_timeshift") - argv[0]));
#endif

    pvr_sample_printf("================================\n");

    ret = hi_unf_pvr_rec_init();
    ret |= hi_unf_pvr_play_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_mpi_pvr_rec_init failed.\n");
        return ret;
    }

    ret = sample_pvr_register_cb();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call sample_pvr_register_cb failed.\n");
        return ret;
    }

    pvr_sample_printf("please input the numbers of program to record, the first program num will timeshift:\n");

    do {
        sleep(1);
        SAMPLE_GET_INPUTCMD(input_cmd);
        hi_u32 cmd_len = strlen(input_cmd);
        hi_char *tmp = input_cmd;
        for (i = 0; i < cmd_len; i++) {
            if (*tmp >= '0' && *tmp <= '7') {
                prog_num[prog_cnt] = atoi(tmp);
                prog_cnt++;
            } else if (*tmp == 'q') {
                return 0;
            }
            tmp++;
        }

        if (prog_cnt == 0) {
            pvr_sample_printf("input invalid! pleasd input again\n");
            continue;
        }
    } while(0);

    if(prog_cnt == 0) {
        prog_num[0] = 0;
        prog_cnt++;
    }

    for (i = 0; i < prog_cnt; i++) {
        cur_prog_info = g_prog_tbl->proginfo + (prog_num[i] % g_prog_tbl->prog_num);
        sample_pvr_rec_start(argv[1], cur_prog_info, PVR_DMX_ID_REC, &rec_chn[i]);
    }

#ifdef ENABLE_IR_PRG
    g_ir_avplay = av;
    if(ir_process(rec_chn[0]) != HI_SUCCESS) {
        pvr_sample_printf("IR Process failed.\n");
        return ret;
    }
#endif

    pthread_create(&g_TimeThread, HI_NULL, statu_thread, (hi_void *)&rec_chn[0]);

    live = HI_TRUE;
    pause = HI_FALSE;
    cur_prog_info = g_prog_tbl->proginfo + prog_num[0] % g_prog_tbl->prog_num;
    ret = sample_pvr_start_live(av, cur_prog_info);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call SwitchProg failed.\n");
        return ret;
    }

    memset(cur_path, 0, sizeof(cur_path));
    memcpy(cur_path, argv[0], (hi_u32)(strstr(argv[0], "sample_pvr_timeshift") - argv[0]));
    draw_string(0, "Live");

    while(1) {
        static hi_u32 r_times = 0;
        static hi_u32 f_times = 0;
        static hi_u32 s_times = 0;

        pvr_sample_printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            g_play_stop = HI_TRUE;
            pvr_sample_printf("prepare to exit!\n");
            break;
        } else if (input_cmd[0] == 'l') {
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (!live) {
                hi_u32 tmp_chn = g_play_chn;
                g_play_chn = 0xffffffff;
                draw_string(0, "Live play");
                switch_to_live_play(tmp_chn, av, cur_prog_info);
                live = HI_TRUE;
                draw_string(0, "Live");
            } else {
                pvr_sample_printf("already live play...\n");
            }
        } else if (input_cmd[0] == 'p') {
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, switch timeshift and pause\n");
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0], &rec_attr);
                pvr_sample_printf("switch to timeshift:%s\n", rec_attr.file_name.name);
                sample_pvr_stop_live(av);
                switch_to_shift_play(rec_attr.file_name.name, &g_play_chn, av);
                live = HI_FALSE;
                draw_string(0, "TimeShift");
                ret = hi_unf_pvr_play_chan_pause(g_play_chn);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
                    return ret;
                }
                pause = HI_TRUE;
            } else {
                if (pause) {
                    pvr_sample_printf("PVR resume now.\n");
                    ret = hi_unf_pvr_play_chan_resume(g_play_chn);
                    if (ret != HI_SUCCESS) {
                        pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed. ret = 0x%08x\n", ret);
                        return ret;
                    }
                    pause = HI_FALSE;
                } else {
                    pvr_sample_printf("PVR pause now.\n");
                    ret = hi_unf_pvr_play_chan_pause(g_play_chn);
                    if (ret != HI_SUCCESS) {
                        pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
                        return ret;
                    }
                    pause = HI_TRUE;
                }
            }
        } else if (input_cmd[0] == 'f') {
            r_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, switch timeshift and ff\n");
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0],&rec_attr);
                pvr_sample_printf("switch to timeshift:%s\n", rec_attr.file_name.name);
                sample_pvr_stop_live(av);
                switch_to_shift_play(rec_attr.file_name.name, &g_play_chn, av);
                live = HI_FALSE;
                pause = HI_FALSE;
                draw_string(0, "TimeShift");
            }
            hi_unf_pvr_play_mode trick_mode;
            f_times = f_times % 6;
            trick_mode.speed = (0x1 << (f_times + 1)) * HI_UNF_PVR_PLAY_SPEED_NORMAL;
            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(g_play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
                return ret;
            }
            pause = HI_FALSE;
            f_times++;
        } else if (input_cmd[0] == 'r') {
            f_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, switch timeshift and fb\n");
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0], &rec_attr);
                pvr_sample_printf("switch to timeshift:%s\n", rec_attr.file_name.name);
                sample_pvr_stop_live(av);
                switch_to_shift_play(rec_attr.file_name.name, &g_play_chn, av);
                live = HI_FALSE;
                pause = HI_FALSE;
                draw_string(0, "TimeShift");
            }
            hi_unf_pvr_play_mode trick_mode;
            r_times = r_times % 6;
            trick_mode.speed = -(0x1 << (r_times+1)) * HI_UNF_PVR_PLAY_SPEED_NORMAL;
            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(g_play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
                return ret;
            }
            pause = HI_FALSE;
            r_times++;
        } else if (input_cmd[0] == 'n') {
            r_times = 0;
            f_times = 0;
            s_times = 0;

            if (live) {
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0], &rec_attr);
                pvr_sample_printf("switch to timeshift:%s\n", rec_attr.file_name.name);
                sample_pvr_stop_live(av);
                switch_to_shift_play(rec_attr.file_name.name, &g_play_chn, av);
                pvr_sample_printf("play_chn ============= %d\n", g_play_chn);
                live = HI_FALSE;
                draw_string(0, "TimeShift");
                pause = HI_FALSE;
            } else {
                pvr_sample_printf("resume to normal play now\n");
                ret = hi_unf_pvr_play_chan_resume(g_play_chn);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                    return ret;
                }
                pause = HI_FALSE;
            }
        } else if (input_cmd[0] == 's') {
            r_times = 0;
            f_times = 0;
            if (live) {
                pvr_sample_printf("now live play, switch timeshift and slow play\n");
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0], &rec_attr);
                pvr_sample_printf("switch to timeshift:%s\n", rec_attr.file_name.name);
                sample_pvr_stop_live(av);
                switch_to_shift_play(rec_attr.file_name.name, &g_play_chn, av);
                live = HI_FALSE;
                pause = HI_FALSE;
                draw_string(0, "TimeShift");
            }
            hi_unf_pvr_play_mode trick_mode;

            s_times = s_times % 6;
            trick_mode.speed = HI_UNF_PVR_PLAY_SPEED_NORMAL / (0x1 << (s_times + 1));
            pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
            ret = hi_unf_pvr_play_chan_tplay(g_play_chn, &trick_mode);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
                return ret;
            }
            pause = HI_FALSE;
            s_times++;
        } else if (input_cmd[0] == 'k') {
            hi_unf_pvr_play_position pos;
            hi_unf_pvr_file_attr f_attr;
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, not support seek\n");

            } else {
                ret = hi_unf_pvr_play_chan_resume(g_play_chn);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                    continue;
                }
                ret = hi_unf_pvr_play_chan_get_file_attr(g_play_chn, &f_attr);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_get_file_attr failed.\n");
                }

                pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
                pos.offset = f_attr.start_time_ms;
                pos.whence = SEEK_SET;
                pvr_sample_printf("seek to start\n");

                ret = hi_unf_pvr_play_chan_seek(g_play_chn, &pos);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_seek failed.\n");
                }
            }
        } else if (input_cmd[0] == 'e') {
            hi_unf_pvr_play_position pos;
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, not support seek\n");
            } else {
                ret = hi_unf_pvr_play_chan_resume(g_play_chn);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                    continue;
                }
                pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
                pos.offset = 0;
                pos.whence = SEEK_END;
                pvr_sample_printf("seek end\n");

                ret = hi_unf_pvr_play_chan_seek(g_play_chn, &pos);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_seek failed.\n");
                }
            }
        } else if (input_cmd[0] == 'a') {
            hi_unf_pvr_play_position pos;
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, not support seek\n");

            } else {
                ret = hi_unf_pvr_play_chan_resume(g_play_chn);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                    continue;
                }
                pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
                pos.offset = -5000;
                pos.whence = SEEK_CUR;
                pvr_sample_printf("seek reward 5 Second\n");

                ret = hi_unf_pvr_play_chan_seek(g_play_chn, &pos);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_seek failed.\n");
                    continue;
                }
            }
        } else if (input_cmd[0] == 'd') {
            hi_unf_pvr_play_position pos;
            r_times = 0;
            f_times = 0;
            s_times = 0;

            if (live) {
                pvr_sample_printf("now live play, not support seek\n");
            } else {
                ret = hi_unf_pvr_play_chan_resume(g_play_chn);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
                    continue;
                }
                pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
                pos.offset = 5000;
                pos.whence = SEEK_CUR;
                pvr_sample_printf("seek forward 5 Second\n");

                ret = hi_unf_pvr_play_chan_seek(g_play_chn, &pos);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_pvr_play_chan_seek failed.\n");
                }
            }
        } else if (input_cmd[0] == 'x') {
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (!live) {
                sample_pvr_stop_playback(g_play_chn);

                ret = sample_pvr_start_playback(rec_attr.file_name.name, &g_play_chn, av);
                if (ret != HI_SUCCESS) {
                    pvr_sample_printf("call sample_pvr_start_playback failed.\n");
                }
            } else {
                pvr_sample_printf("Now in live mode, can't recreate play channel!\n");
            }
        } else if (input_cmd[0] == 'c') {
            if (circular_to_linear == HI_TRUE) {
                pvr_sample_printf("only support one time changes!\n");
                continue;
            }

            ret = hi_unf_pvr_rec_chan_set_attr(rec_chn[0],HI_UNF_PVR_REC_ATTR_ID_REWIND, HI_NULL);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_rec_chan_set_attr failed.\n");
                continue;
            }
            circular_to_linear = HI_TRUE;
            pvr_sample_printf("record is linear\n");
        } else {
            printf("commond:\n");
            printf("    l: switch to live play\n");
            printf("    p: pause/play\n");
            printf("    f: fast forward\n");
            printf("    r: fast backward\n");
            printf("    n: normal play\n");
            printf("    s: slow play\n");
            printf("    k: seek to start\n");
            printf("    e: seek to end\n");
            printf("    a: seek backward 5s\n");
            printf("    d: seek forward 5s\n");
            printf("    c: rewind record switch to linear\n");
            printf("    x: destroy play channel and create again\n");
            printf("    h: help\n");
            printf("    q: quit\n");
            continue;
        }
    }

    pthread_join(g_TimeThread, HI_NULL);

    for (i = 0; i < prog_cnt; i++) {
        sample_pvr_rec_stop(rec_chn[i]);
    }
    if (g_play_chn != 0xffffffff) {
        sample_pvr_stop_playback(g_play_chn);
    }

#ifdef ENABLE_IR_PRG
    g_ir_task_runing = HI_FALSE;
    pthread_join(g_ir_thread, HI_NULL);

    hi_unf_ir_deinit();
#endif
    ret = hi_unf_pvr_play_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("pvr play DeInit failed! ret = 0x%x\n", ret);
    }
    ret = hi_unf_pvr_rec_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("pvr play DeInit failed! ret = 0x%x\n", ret);
    }
    sample_pvr_av_deinit(av, win, sound_track);
    if(dmx_deinit() != HI_SUCCESS) {
        pvr_sample_printf("pvr dmx_deinit failed! ret = 0x%x\n", ret);
    }
    hi_unf_win_destroy(win);
    hi_adp_win_deinit();
    hi_adp_disp_deinit();
    hi_adp_snd_deinit();
    hi_unf_sys_deinit();

    return 0;
}


#ifdef ENABLE_IR_PRG
const hi_char ir_stuts_str[3][10] = {
    "DOWN",
    "HOLD",
    "UP",
};

typedef enum tagkey{
    KEY_FF = 0x29d6ff00,
    KEY_RW = 0x25daff00,
    KEY_STOP = 0x2fd0ff00,
    KEY_PLAY = 0x3cc3ff00,
    KEY_F1 = 0x7b84ff00,
    KEY_F2 = 0x8689ff00,
    KEY_F3 = 0x26d9ff00,
    KEY_F4 = 0x6996ff00,
    KEY_SEEK = 0x7d82ff00
} key_pad;

hi_void ir_task_run_for_press_status(hi_u64 key_id, hi_unf_key_status press_status, hi_u32 rec_chn)
{
    hi_u32 pause_flag = 0xffffffff;
    hi_u32 seek_flag = 0xffffffff;
    hi_u32 rate = 0;

    if(key_id == KEY_PLAY) {
        rate = 0;
        if(pause_flag == 0xffffffff) {
            pvr_sample_printf("PVR pause now.\n");
            if(hi_unf_pvr_play_chan_pause(g_play_chn) != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_pause failed.\n");
            }
            pause_flag = 0;
        } else {
            pvr_sample_printf("PVR resume now.\n");
            if(hi_unf_pvr_play_chan_resume(g_play_chn) != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_play_chan_resume failed.\n");
            }
            pause_flag = 0xffffffff;
        }
    } else if(key_id == KEY_FF || key_id == KEY_RW) {
        hi_unf_pvr_play_mode trick_mode;
        rate = rate % 5;
        trick_mode.speed = (0x1 << (rate + 1)) * HI_UNF_PVR_PLAY_SPEED_NORMAL;

        if(key_id == KEY_RW) {
            trick_mode.speed *= -1;
        }
        pvr_sample_printf("trick mod:%d\n", trick_mode.speed);
        if(hi_unf_pvr_play_chan_tplay(g_play_chn, &trick_mode) != HI_SUCCESS) {
            pvr_sample_printf("call hi_unf_pvr_play_chan_tplay failed.\n");
        }

        rate++;
        pause_flag = 0;
    } else if (key_id == KEY_STOP) {
        pvr_sample_printf("====To stop player channel is %d\n", g_play_chn);
        sample_pvr_stop_playback(g_play_chn);
        pause_flag = 0;
    } else if (key_id == KEY_F1 || key_id == KEY_F2 || key_id == KEY_F3 || key_id == KEY_F4) {
        hi_unf_pvr_rec_attr rec_attr;
        pause_flag = 0xffffffff;

        if(g_play_chn != 0xffffffff) {
            sample_pvr_stop_playback(g_play_chn);
        }

        hi_unf_pvr_rec_chan_get_attr(rec_chn, &rec_attr);
        sample_pvr_start_playback(rec_attr.file_name.name, &g_play_chn, g_ir_avplay);
        pvr_sample_printf("====Created Player channel is %d\n", g_play_chn);
    } else if(key_id == KEY_SEEK) {
        hi_unf_pvr_play_position pos;
        rate = 0;
        pause_flag = 0xffffffff;
        pos.pos_type = HI_UNF_PVR_PLAY_POS_TYPE_TIME;
        pos.offset = 0;

        if(seek_flag == 0xffffffff) {
            pos.whence = SEEK_SET;
            seek_flag = 0;
            pvr_sample_printf("\033[5;31mseek to start\n\033[0m");
        } else {
            pvr_sample_printf("\033[5;31mseek to end\n\033[0m");
            pos.whence = SEEK_END;
            seek_flag = 0xffffffff;
        }

        if(hi_unf_pvr_play_chan_seek(g_play_chn, &pos) != HI_SUCCESS) {
            pvr_sample_printf("call hi_mpi_pvr_play_chan_step_forward failed.\n");
        }
    } else {
        pvr_sample_printf(" IR  KeyId : 0x%llx press_status :%d[%s]\n",
            key_id, press_status, ir_stuts_str[press_status]);
    }
    return;
}

hi_void *ir_receive_task(hi_void *args)
{
    hi_u32 rec_chn = *(hi_u32 *)args;
    hi_s32 ret = 0;
    hi_u64 key_id = 0;
    hi_unf_key_status press_status = HI_UNF_KEY_STATUS_DOWN;
    hi_unf_ir_protocol protocol = HI_UNF_IR_NEC;

    while (g_ir_task_runing) {
        /* get ir press codevalue & press status */
        if(hi_unf_ir_get_value_protocol(&press_status, &key_id,protocol, strlen(protocol), 200) == HI_SUCCESS) {
            ret = hi_unf_ir_get_protocol(&protocol);
            if (press_status == 0) {
                ir_task_run_for_press_status(key_id, rec_chn);
            }
        }
        usleep(10*1000);
    }
    return 0;
}

hi_s32 ir_process(hi_u32 rec_chn)
{
    hi_s32 ret;

    /* open ir device */
    ret = hi_unf_ir_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf(" ErrorCode=0x%x\n", ret);
        return ret;
    }

    ret = hi_unf_ir_enable_keyup(HI_TRUE);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf(" ErrorCode=0x%x\n", ret);
        hi_unf_ir_deinit();
        return ret;
    }

    ret = hi_unf_ir_set_repkey_timeout(108);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf(" ErrorCode=0x%x\n", ret);
        hi_unf_ir_deinit();
        return ret;
    }

    ret = hi_unf_ir_enable_repkey(HI_TRUE);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf(" ErrorCode=0x%x\n", ret);
        hi_unf_ir_deinit();
        return ret;
    }

    ret = hi_unf_ir_enable(HI_TRUE);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf(" ErrorCode=0x%x\n", ret);
        hi_unf_ir_deinit();
        return ret;
    }

    g_ir_task_runing = HI_TRUE;

    /* create a thread for ir receive */
    ret = pthread_create(&g_ir_thread, HI_NULL, ir_receive_task, (hi_void *)&rec_chn);
    if (ret != 0) {
        pvr_sample_printf(" ErrorCode=0x%x\n", ret);
        perror("pthread_create");
    }

    return HI_SUCCESS;
}
#endif

