/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:
 * Author: sdk
 * Create: 2019-08-21
 */

#include <stdlib.h>
#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_adp_pvr.h"

static pthread_t g_status_thread;
static hi_char *g_multi_addr;
static hi_u16 g_udp_port;
static hi_unf_video_format g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
pmt_compact_tbl *g_prog_tbl = HI_NULL;
hi_bool g_play_stop = HI_FALSE;
hi_handle g_play_chn = 0xffffffff;
static hi_bool g_stop_socket_thread = HI_FALSE;
static hi_handle g_ip_ts_buf;
static pthread_t g_socket_thd;

#define MODULE_MEM_TEST_INFO _IOWR(0, 8, int)

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

static hi_void socket_thread(hi_void *args)
{
    hi_s32 socket_fd;
    struct sockaddr_in server_addr;
    in_addr_t ip_addr;
    struct ip_mreq m_req;
    hi_u32 addr_len;

    hi_unf_stream_buf stream_buf;
    hi_u32 read_len;
    hi_u32 get_buf_cnt = 0;
    hi_u32 receive_cnt = 0;
    hi_s32 ret;

    socket_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (socket_fd < 0) {
        pvr_sample_printf("create socket error [%d].\n", errno);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(g_udp_port);

    if (bind(socket_fd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr_in)) < 0) {
        pvr_sample_printf("socket bind error [%d].\n", errno);
        close(socket_fd);
        return;
    }

    ip_addr = inet_addr(g_multi_addr);
    if (ip_addr) {
        m_req.imr_multiaddr.s_addr = ip_addr;
        m_req.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m_req, sizeof(struct ip_mreq))) {
            pvr_sample_printf("Socket setsockopt ADD_MEMBERSHIP error [%d].\n", errno);
            close(socket_fd);
            return;
        }
    }

    addr_len = sizeof(server_addr);

    while (!g_stop_socket_thread) {
        ret = hi_unf_dmx_get_ts_buffer(g_ip_ts_buf, 188*50, &stream_buf, 0);
        if (ret != HI_SUCCESS) {
            get_buf_cnt++;
            if(get_buf_cnt >= 10) {
                pvr_sample_printf("########## TS come too fast! #########, ret=%d\n", ret);
                get_buf_cnt = 0;
            }

            usleep(10000) ;
            continue;
        }
        get_buf_cnt = 0;

        read_len = recvfrom(socket_fd, stream_buf.data, stream_buf.size, 0,
            (struct sockaddr *)&server_addr, &addr_len);
        if (read_len <= 0) {
            receive_cnt++;
            if (receive_cnt >= 50) {
                pvr_sample_printf("########## TS come too slow or net error! #########\n");
                receive_cnt = 0;
            }
        } else {
            receive_cnt = 0;
            ret = hi_adp_dmx_push_ts_buf(g_ip_ts_buf, &stream_buf, 0, read_len);
            if (ret != HI_SUCCESS ) {
                pvr_sample_printf("call HI_UNF_DMX_PutTSBuffer failed.\n");
            }
        }
    }

    close(socket_fd);
    return;
}

hi_s32 switch_to_shift_play(const hi_char *file_name, hi_handle *play_chn, hi_handle av)
{
    sample_pvr_switch_dmx_source(PVR_DMX_ID_LIVE, PVR_DMX_PORT_ID_PLAYBACK);
    sample_pvr_start_playback(file_name, play_chn, av);

    return HI_SUCCESS;
}


hi_s32 switch_to_live_play(hi_handle play_chn, hi_handle av, const pmt_compact_prog *prog_info)
{
    sample_pvr_stop_playback(play_chn);
    sample_pvr_switch_dmx_source(PVR_DMX_ID_LIVE, HI_UNF_DMX_PORT_RAM_0);
    sample_pvr_start_live(av, prog_info);
    return HI_SUCCESS;
}

hi_void draw_string(hi_u32 time_type, hi_char *text)
{
    pvr_sample_printf("%s\n", text);

    return;
}

hi_void draw_play_status_string(hi_unf_pvr_play_status play_status)
{
    hi_char play_time_str[20];
    hi_char stat_str[32];

    snprintf(play_time_str, sizeof(play_time_str),"Play time:%d", play_status.cur_play_time_ms / 1000);
    draw_string(3, play_time_str);
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
            snprintf(stat_str, sizeof(stat_str),"FF %dX", play_status.speed / HI_UNF_PVR_PLAY_SPEED_NORMAL);
            break;
        case HI_UNF_PVR_PLAY_STATE_FB:
            snprintf(stat_str, sizeof(stat_str),"FB %dX", play_status.speed / HI_UNF_PVR_PLAY_SPEED_NORMAL);
            break;
        case HI_UNF_PVR_PLAY_STATE_SF:
            snprintf(stat_str, sizeof(stat_str),"SF 1/%dX", HI_UNF_PVR_PLAY_SPEED_NORMAL / play_status.speed);
            break;
        case HI_UNF_PVR_PLAY_STATE_SB:
            snprintf(stat_str, sizeof(stat_str),"SB 1/%dX", HI_UNF_PVR_PLAY_SPEED_NORMAL / play_status.speed);
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
    return;
}

hi_void *statu_thread(hi_void *args)
{
    hi_u32 rec_chn = *(hi_u32 *)args;
    hi_s32 ret;
    hi_unf_pvr_rec_status rec_status;
    hi_unf_pvr_play_status play_status;
    hi_unf_pvr_file_attr play_file_status;
    hi_char rec_time_str[20];
    hi_char file_time_str[20];

    while (g_play_stop == HI_FALSE) {
        sleep(1);
#ifdef ANDROID
        pvr_sample_printf("----------------------------------\n");
#endif
        ret = hi_unf_pvr_rec_chan_get_status(rec_chn, &rec_status);
        if (ret == HI_SUCCESS) {
            snprintf(rec_time_str, sizeof(rec_time_str),"Rec time:%d", rec_status.cur_time_ms / 1000);
            draw_string(1, rec_time_str);
        }

        if (g_play_chn != 0xffffffff) {
            ret = hi_unf_pvr_play_chan_get_file_attr(g_play_chn, &play_file_status);
            if (ret == HI_SUCCESS) {
                snprintf(file_time_str, sizeof(file_time_str),"File end time:%d", play_file_status.end_time_ms / 1000);
                draw_string(2, file_time_str);
            }

            ret = hi_unf_pvr_play_chan_get_status(g_play_chn, &play_status);
            if (ret == HI_SUCCESS) {
                draw_play_status_string(play_status);
            }
        }
#ifdef ANDROID
        pvr_sample_printf("----------------------------------\n\n");
#endif
    }

    return HI_NULL;
}

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_u32 i;
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
    hi_unf_pvr_rec_attr rec_attr;
    pmt_compact_prog *cur_prog_info;
    hi_char cur_path[256] = {0};
    hi_unf_dmx_ts_buffer_attr ts_buf_attr;

    HI_DBG_PRINT(HI_ID_PVR, "%s... enter and argc is %d\n", __func__, argc);

    if(argc == 5) {
        g_multi_addr = argv[2];
        g_udp_port = strtol(argv[3],HI_NULL,0);
        g_default_fmt = hi_adp_str2fmt(argv[4]);
    } else if(argc == 4) {
        g_multi_addr = argv[2];
        g_udp_port = strtol(argv[3],HI_NULL,0);
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
    } else {
        printf("Usage: %s file_path multiaddr portnum [vo_format]\n"
                "       multicast address: \n"
                "           UDP port \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n", argv[0]);
        return HI_FAILURE;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_unf_dmx_init();
    ret |= hi_unf_dmx_attach_ts_port(PVR_DMX_ID_LIVE, PVR_DMX_PORT_ID_IP);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        return HI_FAILURE;
    }

    ts_buf_attr.secure_mode = 0;
    ts_buf_attr.buffer_size = 0x200000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ts_buf_attr, &g_ip_ts_buf);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_attach_ts_port(PVR_DMX_ID_REC, PVR_DMX_PORT_ID_IP);

    g_stop_socket_thread = HI_FALSE;
    pthread_create(&g_socket_thd, HI_NULL, (hi_void *)socket_thread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PVR_DMX_ID_LIVE, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_search_get_all_pmt failed\n");
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

    pvr_sample_printf("please input the number of program to record, the first program num can timeshift:\n");

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

    for (i = 0; i < prog_cnt; i++) {
        cur_prog_info = g_prog_tbl->proginfo + (prog_num[i]%g_prog_tbl->prog_num);
        sample_pvr_rec_start(argv[1], cur_prog_info, PVR_DMX_ID_REC, &rec_chn[i]);
    }

    pthread_create(&g_status_thread, HI_NULL, statu_thread, (hi_void *)&rec_chn[0]);

    live = HI_TRUE;
    pause = HI_FALSE;
    cur_prog_info = g_prog_tbl->proginfo + prog_num[0]%g_prog_tbl->prog_num;
    ret = sample_pvr_start_live(av, cur_prog_info);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call SwitchProg failed.\n");
        return ret;
    }

    memset(cur_path, 0, sizeof(cur_path));
    memcpy(cur_path, argv[0], (hi_u32)(strstr(argv[0], "sample_pvr_eth_timeshift") - argv[0]));
    draw_string(0, "Live");

    while(1) {
        static hi_u32 r_times = 0;
        static hi_u32 f_times = 0;
        static hi_u32 s_times = 0;

        pvr_sample_printf("please input the q to quit!\n");

        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            g_play_stop = HI_TRUE;
            g_stop_socket_thread = HI_TRUE;
            pvr_sample_printf("prepare to exit!\n");
            break;
        } else if (input_cmd[0] == 'l') {
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (!live) {
                draw_string(0, "Live play");
                switch_to_live_play(g_play_chn, av, cur_prog_info);
                live = HI_TRUE;
                g_play_chn = 0xffffffff;
                draw_string(0, "Live");
            } else {
                pvr_sample_printf("already live play...\n");
            }
        } else if (input_cmd[0]  == 'p') {
            r_times = 0;
            f_times = 0;
            s_times = 0;
            if (live) {
                pvr_sample_printf("now live play, switch timeshift and pause\n");
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0],&rec_attr);
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
            trick_mode.speed = (0x1 << (f_times+1)) * HI_UNF_PVR_PLAY_SPEED_NORMAL;
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
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0],&rec_attr);
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
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0],&rec_attr);
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
                hi_unf_pvr_rec_chan_get_attr(rec_chn[0],&rec_attr);
                pvr_sample_printf("switch to timeshift:%s\n", rec_attr.file_name.name);
                sample_pvr_stop_live(av);
                switch_to_shift_play(rec_attr.file_name.name, &g_play_chn, av);
                live = HI_FALSE;
                pause = HI_FALSE;
                draw_string(0, "TimeShift");
            }
            hi_unf_pvr_play_mode trick_mode;
            s_times = s_times % 6;
            trick_mode.speed = HI_UNF_PVR_PLAY_SPEED_NORMAL/(0x1 << (s_times + 1));
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
                ret = hi_unf_pvr_play_chan_get_file_attr(g_play_chn,&f_attr);
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
                    pvr_sample_printf("call hi_mpi_pvr_play_chan_step_forward failed.\n");
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
        } else if (input_cmd[0] == 'x' ) {
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
        } else {
            printf("commond:\n");
            printf("    l: live play\n");
            printf("    p: pause/play\n");
            printf("    f: fast foreword\n");
            printf("    r: fast reword\n");
            printf("    n: normal play\n");
            printf("    s: slow play\n");
            printf("    k: seek to start\n");
            printf("    e: seek to end\n");
            printf("    a: seek backward 5s\n");
            printf("    d: seek forward 5s\n");
            printf("    x: destroy play channel and create again\n");
            printf("    h: help\n");
            printf("    q: quit\n");
            continue;
        }
    }

    pthread_join(g_socket_thd, HI_NULL);
    pthread_join(g_status_thread, HI_NULL);

    for (i = 0; i < prog_cnt; i++) {
        sample_pvr_rec_stop(rec_chn[i]);
    }

    if (g_play_chn != 0xffffffff) {
        sample_pvr_stop_playback(g_play_chn);
    }

    ret = hi_unf_pvr_play_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("pvr play DeInit failed! ret = 0x%x\n", ret);
    }
    ret = hi_unf_pvr_rec_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("pvr play DeInit failed! ret = 0x%x\n", ret);
    }

    sample_pvr_av_deinit(av, win, sound_track);
    dmx_deinit();
    hi_unf_win_destroy(win);
    hi_adp_win_deinit();
    hi_adp_disp_deinit();
    hi_adp_snd_deinit();
    hi_unf_sys_deinit();

    return 0;
}


