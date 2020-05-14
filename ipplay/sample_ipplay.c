/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : ipplay.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/01/26
  Description   :
  History       :
  1.Date        : 2010/01/26
    Author      : sdk
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define UDP_RECV_MEM_MAX (1 * 1024 * 1024)
#define TS_READ_SIZE     (188 * 1000)


static pthread_t    g_socket_thread;
static hi_char     *g_multi_attr;
static hi_u16       g_udp_port;

static hi_bool          g_stop_socket_thread = HI_FALSE;
static hi_handle        g_ip_ts_buf;
static pmt_compact_tbl *g_ip_prog_table = HI_NULL;

static hi_void socket_thread(hi_void *args)
{
    hi_s32              socket_fd;
    struct sockaddr_in  server_attr;
    in_addr_t           ip_attr;
    struct ip_mreq      mreq;
    hi_u32              addr_len;
    hi_unf_stream_buf   stream_buff;
    hi_s32              read_len;
    hi_u32              get_buf_count = 0;
    hi_s32              ret;
    hi_s32              optVal = UDP_RECV_MEM_MAX;
    socklen_t           opt_len = sizeof(hi_s32);
    struct timeval      timeout_val;


    socket_fd = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
    if (socket_fd < 0)
    {
        sample_printf("create socket error [%d].\n", errno);
        return;
    }

    server_attr.sin_family = AF_INET;
    server_attr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_attr.sin_port = htons(g_udp_port);

    if (bind(socket_fd, (struct sockaddr *)(&server_attr), sizeof(struct sockaddr_in)) < 0)
    {
        sample_printf("socket bind error [%d].\n", errno);
        close(socket_fd);
        return;
    }

    ip_attr = inet_addr(g_multi_attr);
    if (ip_attr)
    {
        mreq.imr_multiaddr.s_addr = ip_attr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)))
        {
            sample_printf("Socket setsockopt ADD_MEMBERSHIP error [%d].\n", errno);
            close(socket_fd);
            return;
        }
    }

    timeout_val.tv_sec  = 3;
    timeout_val.tv_usec = 0;
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_val, sizeof(timeout_val));
    if (ret != 0)
    {
        sample_printf("setsockopt timeout error 0x%x", ret);
    }

    ret = setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (hi_char*)&optVal, opt_len);
    if (ret != 0)
    {
        sample_printf("setsockopt error 0x%x", ret);
    }

    addr_len = sizeof(server_attr);

    while (!g_stop_socket_thread)
    {
        ret = hi_unf_dmx_get_ts_buffer(g_ip_ts_buf, TS_READ_SIZE, &stream_buff, 0);
        if (ret != HI_SUCCESS)
        {
            if (get_buf_count++ >= 10)
            {
                sample_printf("########## TS come too fast! #########, ret=0x%x\n", ret);
                get_buf_count = 0;
            }

            usleep(10 * 1000);
            continue;
        }

        get_buf_count = 0;

        read_len = recvfrom(socket_fd, stream_buff.data, stream_buff.size, 0,
            (struct sockaddr *)&server_attr, &addr_len);

        if (read_len <= 0)
        {
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ip_ts_buf, &stream_buff, 0, read_len);
        if (ret != HI_SUCCESS)
        {
            sample_printf("hi_unf_dmx_put_ts_buffer failed 0x%x\n", ret);
        }

    }

    close(socket_fd);
    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                      ret;
    hi_handle                   win_handle;
    hi_u32                      dmx_id = 4;
    hi_unf_dmx_port             ram_port = HI_UNF_DMX_PORT_RAM_0;
    hi_handle                   avplay_handle;
    hi_unf_avplay_attr          avplay_attr;
    hi_unf_avplay_stop_opt      stop;

    hi_unf_sync_attr            sync_attr;
    hi_handle                   track;
    hi_unf_audio_track_attr     track_attr;
    hi_char                     InputCmd[32];
    hi_u32                      ProgNum;
    hi_unf_video_format  format = HI_UNF_VIDEO_FMT_1080P_50;

    if (argc < 3)
    {
        printf("Usage: sample_ipplay MultiAddr UdpPort [vo_format]\n"
               "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n"
           "Example:./sample_ipplay 224.0.0.1 1234 1080p50\n");
        return 0;
    }

    g_multi_attr = argv[1];
    g_udp_port = atoi(argv[2]);

    if (argc >= 4)
    {
        format = hi_adp_str2fmt(argv[3]);
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call SndInit failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(format);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call DispInit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win_handle);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_win_init failed\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_dmx_init failed 0x%x\n", ret);
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(dmx_id, ram_port);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_dmx_attach_ts_port failed 0x%x\n", ret);
        goto VO_DEINIT;
    }

    hi_unf_dmx_ts_buffer_attr buff_attr;
    buff_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    buff_attr.buffer_size = 0x1000000;
    ret = hi_unf_dmx_create_ts_buffer(ram_port, &buff_attr, &g_ip_ts_buf);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_dmx_create_ts_buffer failed 0x%x\n", ret);
        goto DMX_DEINIT;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_adp_avplay_init failed 0x%x\n", ret);
        goto TSBUF_DESTROY;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = dmx_id;
    avplay_attr.vid_buf_size = 0x300000;

    ret = hi_unf_avplay_create(&avplay_attr, &avplay_handle);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_avplay_create failed 0x%x\n", ret);
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_avplay_chan_open failed 0x%x\n", ret);
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_win_attach_src(win_handle, avplay_handle);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_win_attach_src failed 0x%x\n", ret);
        goto AVCHN_CLOSE;
    }

    ret = hi_unf_win_set_enable(win_handle, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_win_set_enable failed 0x%x\n", ret);
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_snd_get_default_track_attr failed 0x%x\n", ret);
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_snd_create_track failed 0x%x\n", ret);
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay_handle);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_snd_attach failed 0x%x\n", ret);
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay_handle, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;

    ret = hi_unf_avplay_set_attr(avplay_handle, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_avplay_set_attr failed 0x%x\n", ret);
        goto SND_DETATCH;
    }

    g_stop_socket_thread = HI_FALSE;
    pthread_create(&g_socket_thread, HI_NULL, (hi_void *)socket_thread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(dmx_id, &g_ip_prog_table);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_adp_search_get_all_pmt failed.\n");
        goto THD_STOP;
    }

    ret = hi_adp_avplay_play_prog(avplay_handle, g_ip_prog_table, 0, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call SwitchProg failed.\n");
        goto PSISI_FREE;
    }

    while(1)
    {
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(InputCmd);

        if ('q' == InputCmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        ProgNum = atoi(InputCmd);
        ret = hi_adp_avplay_play_prog(avplay_handle, g_ip_prog_table, ProgNum, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call SwitchProgfailed!\n");
            break;
        }
    }

    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    ret = hi_unf_avplay_stop(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
    if (ret != HI_SUCCESS)
    {
        sample_printf("hi_unf_avplay_stop failed 0x%x\n", ret);
    }

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_ip_prog_table);
    hi_adp_search_de_init();

THD_STOP:
    g_stop_socket_thread = HI_TRUE;
    pthread_join(g_socket_thread, HI_NULL);

SND_DETATCH:
    hi_unf_snd_detach(track, avplay_handle);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);

WIN_DETACH:
    hi_unf_win_set_enable(win_handle, HI_FALSE);
    hi_unf_win_detach_src(win_handle, avplay_handle);

AVCHN_CLOSE:
    hi_unf_avplay_chan_close(avplay_handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay_handle);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

TSBUF_DESTROY:
    hi_unf_dmx_destroy_ts_buffer(g_ip_ts_buf);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(dmx_id);
    hi_unf_dmx_deinit();

VO_DEINIT:
    hi_unf_win_destroy(win_handle);
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    return 0;
}


