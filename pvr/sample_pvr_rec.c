/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:record streams by using PVR
 * Author: sdk
 * Create: 2019-08-20
 */

#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_adp_pvr.h"

/* when recording, playing the live program */
#define PVR_WITH_LIVE

hi_u32 g_tuner_id = 0;
hi_u32 g_tuner_freq;
hi_u32 g_tuner_rate;
hi_u32 g_third_param;
extern hi_bool g_is_rec_stop;
pthread_t g_status_thread;
hi_unf_video_format g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
pmt_compact_tbl *g_prog_tbl = HI_NULL;

hi_handle layer_pvr = HI_INVALID_HANDLE;
hi_handle layer_surface_pvr;
hi_handle font_pvr = HI_INVALID_HANDLE;

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
        pvr_sample_printf("call hi_adp_dmx_attach_ts_port failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    ret = hi_adp_dmx_attach_ts_port(PVR_DMX_ID_REC, tuner_id);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_dmx_attach_ts_port for REC failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    hi_adp_search_init();
    if (g_prog_tbl != HI_NULL) {
        hi_adp_search_free_all_pmt(g_prog_tbl);
        g_prog_tbl = HI_NULL;
    }

    ret = hi_adp_search_get_all_pmt(PVR_DMX_ID_REC, &g_prog_tbl);
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
    g_prog_tbl = HI_NULL;

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
hi_s32 avplay_open_chn_and_set_win(hi_handle win, hi_handle av)
{
    hi_s32 ret;

    ret = hi_unf_avplay_chan_open(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_chan_open failed.\n");
        hi_unf_avplay_destroy(av);
        hi_unf_avplay_deinit();
        return ret;
    }

    ret = hi_unf_avplay_chan_open(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_chan_open failed.\n");
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(av);
        return ret;
    }

    ret = hi_unf_win_attach_src(win, av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_win_attach_src failed.\n");
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(av);
        hi_unf_avplay_deinit();
        return ret;
    }

    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_win_set_enable failed.\n");
        hi_unf_win_detach_src(win, av);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(av);
        hi_unf_avplay_deinit();
        return ret;
    }
    return ret;
}

hi_s32 avply_snd_creat_and_attach(hi_handle av, hi_handle win, hi_handle *tra)
{
    hi_s32 ret;
    hi_unf_audio_track_attr track_attr;

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        return ret;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, tra);
    if (ret != HI_SUCCESS) {
        hi_unf_win_detach_src(win, av);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(av);
        hi_unf_avplay_deinit();
        return ret;
    }

    ret = hi_unf_snd_attach(*tra, av);
    if (ret != HI_SUCCESS) {
        hi_unf_snd_destroy_track(*tra);
        pvr_sample_printf("call HI_SND_Attach failed.\n");
        hi_unf_win_set_enable(win, HI_FALSE);
        hi_unf_win_detach_src(win, av);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(av);
        hi_unf_avplay_deinit();
        return ret;
    }
    return HI_SUCCESS;
}

hi_s32 avplay_init(hi_handle win, hi_handle *avplay, hi_handle *track)
{
    hi_s32 ret;
    hi_handle av;
    hi_handle tra;
    hi_unf_avplay_attr av_attr;

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_avplay_init failed.\n");
        return ret;
    }

    ret = hi_unf_avplay_get_default_config(&av_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_get_default_config failed.\n");
        hi_unf_avplay_deinit();
        return ret;
    }

    av_attr.demux_id = PVR_DMX_ID_LIVE;
    av_attr.vid_buf_size = 0x300000;
    ret = hi_unf_avplay_create(&av_attr, &av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_create failed.\n");
        hi_unf_avplay_deinit();
        return ret;
    }

    ret= avplay_open_chn_and_set_win(win, av);
    if(ret != HI_SUCCESS) {
        pvr_sample_printf("avplay_open_chn_and_set_win failed\n");
    }
    avply_snd_creat_and_attach(av, win, &tra);

    *avplay = av;
    *track = tra;
    return HI_SUCCESS;
}

hi_s32 av_deinit(hi_handle av, hi_handle win, hi_handle track)
{
    hi_s32 ret;
    hi_unf_avplay_stop_opt stop;

    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;

    ret = hi_unf_avplay_stop(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
    if (ret != HI_SUCCESS ) {
        pvr_sample_printf("call hi_unf_avplay_stop failed.\n");
    }

    ret = hi_unf_snd_detach(track, av);
    if (ret != HI_SUCCESS ) {
        pvr_sample_printf("call hi_unf_snd_detach failed.\n");
    }

    hi_unf_snd_destroy_track(track);

    hi_unf_win_set_enable(win, HI_FALSE);

    ret = hi_unf_win_detach_src(win, av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_win_detach_src failed.ret = %#x\n", ret);
    }

    ret = hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    if (ret != HI_SUCCESS ) {
        pvr_sample_printf("call hi_unf_avplay_chan_close failed.\n");
    }

    ret = hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS ) {
        pvr_sample_printf("call hi_unf_avplay_chan_close failed.\n");
    }

    ret = hi_unf_avplay_destroy(av);
    if (ret != HI_SUCCESS ) {
        pvr_sample_printf("call hi_unf_avplay_destroy failed.\n");
    }

    ret = hi_unf_avplay_deinit();
    if (ret != HI_SUCCESS ) {
        pvr_sample_printf("call hi_unf_avplay_deinit failed.\n");
    }

    return HI_SUCCESS;
}

hi_s32 draw_string(hi_u32 time_type, hi_char *text)
{
    pvr_sample_printf("current time:%s\n", text);
    return HI_SUCCESS;
}

hi_void conver_time_string(hi_char* buf, hi_u32 buf_len, hi_u32 time_in_ms)
{
    hi_u32 hour;
    hi_u32 minute;
    hi_u32 second;
    hi_u32 msecond;

    hour = time_in_ms / (60 * 60 * 1000);

    minute = (time_in_ms - hour * 60 * 60 * 1000)/(60 * 1000);
    second = (time_in_ms - hour* 60 * 60 * 1000 - minute * 60 * 1000) / 1000;

    msecond = (time_in_ms - hour * 60 * 60 * 1000 - minute * 60 * 1000 - second * 1000);

    snprintf(buf, buf_len, "%d:%02d:%02d:%04d", hour, minute, second, msecond);

    return;
}

hi_void *statu_thread(hi_void *args)
{
    hi_u32 rec_chn = *(hi_u32 *)args;
    hi_s32 ret;
    hi_unf_pvr_rec_attr rec_attr;
    hi_unf_pvr_rec_status rec_status;
    hi_unf_pvr_file_attr file_status;
    hi_char rec_time[40];

    while (sample_pvr_check_rec_stop() == HI_FALSE) {
        sleep(1);
        (hi_void)hi_unf_pvr_rec_chan_get_attr(rec_chn, &rec_attr);
#ifdef ANDROID
        pvr_sample_printf("----------------------------------\n");
#endif
        ret = hi_unf_pvr_rec_chan_get_status(rec_chn, &rec_status);
        if (ret == HI_SUCCESS) {
            snprintf(rec_time, sizeof(rec_time),"Rec time:%ds", rec_status.cur_time_ms / 1000);
            draw_string(1, rec_time);
            snprintf(rec_time, sizeof(rec_time),"Rec len:%ds",
                (rec_status.end_time_ms - rec_status.start_time_ms) / 1000);
            draw_string(2, rec_time);
        } else {
            pvr_sample_printf("hi_unf_pvr_rec_chan_get_status failed %x\n", ret);
        }

        if (rec_attr.file_name.name == HI_NULL) {
            pvr_sample_printf("null pointer for filename(%p)\n", rec_attr.file_name.name);
            return HI_NULL;
        }

        ret = hi_unf_pvr_get_file_attr_by_file_name(rec_attr.file_name.name, &file_status);

        if (ret == HI_SUCCESS) {
            snprintf(rec_time, sizeof(rec_time),"Rec size:%lld.%lldM", (file_status.valid_size_byte) / 0x100000,
                ((file_status.valid_size_byte) % 0x100000) / 0x400);
            draw_string(3, rec_time);
        } else {
            pvr_sample_printf("hi_unf_pvr_get_file_attr_by_file_name failed %x\n", ret);
        }

#ifdef ANDROID
        pvr_sample_printf("----------------------------------\n\n");
#endif
    }

    return HI_NULL;
}


hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_u32 i = 0;
    hi_u32 tuner;
    hi_u32 prog_cnt = 0;
    hi_u32 prog_num[8] = {0};  /* one demux can support  creating 8 record channel */
    hi_u32 rec_chn[8];
    hi_s32 ret;
    hi_char input_cmd[32];
    hi_handle win;
    pmt_compact_prog *cur_prog_info = HI_NULL;
    hi_char cur_path[256] = {0};
    hi_u32 cmd_len;
#ifdef PVR_WITH_LIVE
    hi_handle av;
    hi_handle track;
#endif

    if (argc == 7) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_rate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = hi_adp_str2fmt(argv[5]);
        tuner = strtol(argv[6], HI_NULL, 0);
    } else if (argc == 6) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_rate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = hi_adp_str2fmt(argv[5]);
        tuner = 0;
    } else if (argc == 5) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_rate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
        tuner = 0;
    } else if(argc == 4) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_rate = strtol(argv[3], HI_NULL, 0);
        g_third_param = (g_tuner_freq > 1000) ? 0 : 64;
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
        tuner = 0;
    } else if(argc == 3) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_rate = (g_tuner_freq > 1000) ? 27500 : 6875;
        g_third_param = (g_tuner_freq > 1000) ? 0 : 64;
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
        tuner = 0;
    } else {
        printf("Usage: %s file_path freq [srate] [qamtype or polarization] [vo_format] [tuner]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n"
                "       tuner: value can be 0, 1, 2, 3\n", argv[0]);
        printf("Example: %s ./ 618 6875 64 1080P50 0 0\n", argv[0]);
        printf("    Satellite : %s ./ 3840 27500 0 1080P50 0 0\n", argv[0]);
        printf("    Cable     : %s ./ 618 6875 64 1080P50 0 0\n", argv[0]);
        printf("    Terestrial: %s ./ 474 8000 64 1080P50 0 0\n", argv[0]);
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

    ret = hi_adp_frontend_connect(tuner, g_tuner_freq, g_tuner_rate, g_third_param);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_frontend_connect failed.\n");
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

    memset(cur_path, 0, sizeof(cur_path));
    memcpy(cur_path, argv[0], (hi_u32)(strstr(argv[0], "sample_pvr_rec") - argv[0]));

    ret = hi_unf_pvr_rec_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_pvr_rec_init failed.\n");
        return ret;
    }

    ret = sample_pvr_register_cb();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call sample_pvr_register_cb failed.\n");
        return ret;
    }

    pvr_sample_printf("\nPlease input the number of program to record, each num is separated by blank:\n");

    do {
        sleep(1);
        SAMPLE_GET_INPUTCMD(input_cmd);
        cmd_len = strlen(input_cmd);
        hi_char *cmd_tmp = input_cmd;
        for (i = 0; i < cmd_len; i++) {
            if (*cmd_tmp >= '0' && *cmd_tmp <= '7') {
                prog_num[prog_cnt] = atoi(cmd_tmp);
                prog_cnt++;
            } else if (*cmd_tmp == 'q') {
                return 0;
            }
            cmd_tmp++;
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
        pvr_sample_printf("create recording for %d\n", i);
        cur_prog_info = g_prog_tbl->proginfo + (prog_num[i] % g_prog_tbl->prog_num);
        sample_pvr_rec_start(argv[1], cur_prog_info, PVR_DMX_ID_REC, &rec_chn[i]);
    }

#ifdef PVR_WITH_LIVE
    ret = avplay_init(win, &av, &track);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        return ret;
    }

    ret = hi_adp_avplay_play_prog(av, g_prog_tbl, prog_num[0], HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call SwitchProgfailed!\n");
        return 0;
    }
#endif

    pthread_create(&g_status_thread, HI_NULL, statu_thread, (hi_void *)&rec_chn[0]);

    while (sample_pvr_check_rec_stop() == HI_FALSE) {
        pvr_sample_printf("please input the 'q' to quit. \n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            pvr_sample_printf("now exit!\n");
            sample_pvr_inform_rec_stop(HI_TRUE);
        } else {
            sleep(1);
            continue;
        }
    }

    pthread_join(g_status_thread, HI_NULL);

    for (i = 0; i < prog_cnt; i++) {
        sample_pvr_rec_stop(rec_chn[i]);
    }

    hi_unf_pvr_rec_deinit();

#ifdef PVR_WITH_LIVE
    av_deinit(av, win, track);
#endif

    ret = dmx_deinit();
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


