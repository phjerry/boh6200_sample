/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : ts_mosaic.c
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf   printf
#endif

#define PLAY_DMX_ID     0
#define AVPLAY_NUM      4

FILE               *g_ts_file = HI_NULL;
static pthread_t   g_ts_thread;
static pthread_mutex_t g_ts_mutex;
static hi_bool     g_stop_ts_thread = HI_FALSE;
hi_handle          g_ts_buffer_handle;
pmt_compact_tbl    *g_grop_table = HI_NULL;

hi_void* ts_thread(hi_void *args)
{
    hi_unf_stream_buf     stream_buf;
    hi_u32                read_len;
    hi_s32                ret;

    while (!g_stop_ts_thread)
    {
        pthread_mutex_lock(&g_ts_mutex);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buffer_handle, 188 * 50, &stream_buf, 1000);
        if (ret != HI_SUCCESS )
        {
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        read_len = fread(stream_buf.data, sizeof(hi_s8), 188 * 50, g_ts_file);
        if(read_len <= 0)
        {
            sample_printf("read ts file end and rewind!\n");
            rewind(g_ts_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_buffer_handle, &stream_buf, 0, read_len);
        if (ret != HI_SUCCESS )
        {
            sample_printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_ts_mutex);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buffer_handle);
    if (ret != HI_SUCCESS )
    {
        sample_printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return HI_NULL;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                      ret;
    hi_u32                      i;
    hi_handle                   win_handle[AVPLAY_NUM];
    hi_handle                   avplay_handle[AVPLAY_NUM];
    hi_unf_sync_attr            sync_attr;
    hi_char                     input_cmd[32];
    hi_unf_video_format         vid_format = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32                      prog_num;
    hi_bool                     is_audio_play[AVPLAY_NUM];
    hi_handle                   track_handle;

    if (argc < 2)
    {
        sample_printf("Usage:\n%s file [vo_format]\n", argv[0]);
        sample_printf("\tvo_format: 2160P60 2160P50 2160P30 2160P25 2160P24 1080P60 1080P50 1080i60 1080i50 720P60 720P50\n\n");
        sample_printf("Example: %s ./test.ts 1080i50\n", argv[0]);
        return 0;
    }

    if (argc >= 3)
    {
        vid_format = hi_adp_str2fmt(argv[2]);
    }

    g_ts_file = fopen(argv[1], "rb");
    if (!g_ts_file)
    {
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    for (i = 0; i < AVPLAY_NUM; i++)
    {
        win_handle[i]     = HI_INVALID_HANDLE;
        avplay_handle[i]  = HI_INVALID_HANDLE;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (HI_SUCCESS != ret)
    {
        sample_printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(vid_format);
    if (HI_SUCCESS != ret)
    {
        sample_printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    if (HI_SUCCESS != ret)
    {
        sample_printf("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    for (i = 0; i < AVPLAY_NUM; i++)
    {
        hi_unf_video_rect WinRect;

        WinRect.width    = 1280 / (AVPLAY_NUM / 2);
        WinRect.height   = 720 / 2;
        WinRect.x = i % (AVPLAY_NUM / 2) * WinRect.width;
        WinRect.y = i / (AVPLAY_NUM / 2) * WinRect.height;

        sample_printf("x=%d, y=%d, w=%d, h=%d\n", WinRect.x, WinRect.y, WinRect.width, WinRect.height);

        ret = hi_adp_win_create(&WinRect, &win_handle[i]);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_adp_win_create failed.\n");
            goto VO_DEINIT;
        }
    }

    ret = hi_unf_dmx_init();
    if (HI_SUCCESS != ret)
    {
        sample_printf("call hi_unf_dmx_init failed.\n");
        goto VO_DEINIT;
    }

    for (i = 0; i < AVPLAY_NUM; i++)
    {
        ret = hi_unf_dmx_attach_ts_port(i, HI_UNF_DMX_PORT_RAM_0);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_unf_dmx_attach_ts_port failed.\n");
            goto DMX_DEINIT;
        }
    }

    hi_unf_dmx_ts_buffer_attr ts_buff_attr;
    ts_buff_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ts_buff_attr.buffer_size = 0x800000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ts_buff_attr, &g_ts_buffer_handle);
    if (HI_SUCCESS != ret)
    {
        sample_printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    pthread_mutex_init(&g_ts_mutex, HI_NULL);

    g_stop_ts_thread = HI_FALSE;
    pthread_create(&g_ts_thread, HI_NULL, ts_thread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_grop_table);
    if (HI_SUCCESS != ret)
    {
        sample_printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    ret = hi_adp_avplay_init();
    if (HI_SUCCESS != ret)
    {
        sample_printf("call hi_adp_avplay_init failed 0x%x\n", ret);
        goto PSISI_FREE;
    }

    for (i = 0; i < AVPLAY_NUM; i++)
    {
        hi_unf_avplay_attr        avplay_attr;
        hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);

        avplay_attr.demux_id = i;
        avplay_attr.vid_buf_size = 8 * 1024 * 1024;

        ret = hi_unf_avplay_create(&avplay_attr, &avplay_handle[i]);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_unf_avplay_create failed 0x%x\n", ret);
            goto AVPLAY_STOP;
        }

        ret = hi_unf_avplay_chan_open(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_unf_avplay_chan_open Vid failed 0x%x\n", ret);
            goto AVPLAY_STOP;
        }

        ret = hi_unf_win_attach_src(win_handle[i], avplay_handle[i]);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_unf_win_attach_src failed 0x%x\n", ret);
            goto AVPLAY_STOP;
        }

        ret = hi_unf_win_set_enable(win_handle[i], HI_TRUE);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_unf_win_set_enable failed 0x%x\n", ret);
            goto AVPLAY_STOP;
        }

        is_audio_play[i] = HI_FALSE;
        if (0 == i)
        {
            hi_unf_audio_track_attr    track_attr;

            is_audio_play[i] = HI_TRUE;

            ret = hi_unf_avplay_chan_open(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_unf_avplay_chan_open Aud failed 0x%x\n", ret);
                goto AVPLAY_STOP;
            }

            track_attr.track_type = HI_UNF_SND_TRACK_TYPE_MASTER;

            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_unf_snd_get_default_track_attr failed 0x%x\n", ret);
                goto AVPLAY_STOP;
            }

            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track_handle);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_unf_snd_create_track failed 0x%x\n", ret);
                goto AVPLAY_STOP;
            }

            ret = hi_unf_snd_attach(track_handle, avplay_handle[i]);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_unf_snd_attach failed 0x%x\n", ret);
                goto AVPLAY_STOP;
            }
        }

        hi_unf_avplay_get_attr(avplay_handle[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);

        sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;

        ret = hi_unf_avplay_set_attr(avplay_handle[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_unf_avplay_set_attr Sync failed 0x%x\n", ret);
            goto AVPLAY_STOP;
        }

        ret = hi_adp_avplay_play_prog(avplay_handle[i], g_grop_table, i,
            is_audio_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        if (HI_SUCCESS != ret)
        {
            sample_printf("call hi_adp_avplay_play_prog failed\n");
        }
    }

    while (1)
    {
        sample_printf("please input 'q' to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0])
        {
            sample_printf("prepare to quit!\n");
            break;
        }

        prog_num = atoi(input_cmd);

        pthread_mutex_lock(&g_ts_mutex);

        rewind(g_ts_file);

        hi_unf_dmx_reset_ts_buffer(g_ts_buffer_handle);

        for (i = 0; i < AVPLAY_NUM; i++)
        {
            ret = hi_adp_avplay_play_prog(avplay_handle[i], g_grop_table, prog_num + i,
                is_audio_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_adp_avplay_play_prog!\n");
                break;
            }
        }

        pthread_mutex_unlock(&g_ts_mutex);
    }

AVPLAY_STOP:
    for (i = 0; i < AVPLAY_NUM; i++)
    {
        hi_unf_avplay_stop_opt stop;

        stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop.timeout = 0;
        hi_unf_avplay_stop(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop);
        if (0 == i)
        {
            hi_unf_avplay_stop(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
            hi_unf_snd_detach(track_handle, avplay_handle[i]);
            hi_unf_snd_destroy_track(track_handle);
            hi_unf_avplay_chan_close(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }

        hi_unf_win_set_enable(win_handle[i],HI_FALSE);
        hi_unf_win_detach_src(win_handle[i], avplay_handle[i]);
        hi_unf_avplay_chan_close(avplay_handle[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(avplay_handle[i]);
    }

    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_grop_table);
    g_grop_table = HI_NULL;
    hi_adp_search_de_init();

    g_stop_ts_thread = HI_TRUE;
    pthread_join(g_ts_thread, HI_NULL);

    pthread_mutex_destroy(&g_ts_mutex);

    hi_unf_dmx_destroy_ts_buffer(g_ts_buffer_handle);

DMX_DEINIT:
    for (i = 0; i < AVPLAY_NUM; i++)
    {
        hi_unf_dmx_detach_ts_port(i);
    }

    hi_unf_dmx_deinit();

VO_DEINIT:
    for (i = 0; i < AVPLAY_NUM; i++)
    {
        if (HI_INVALID_HANDLE != win_handle[i])
        {
            hi_unf_win_destroy(win_handle[i]);
        }
    }

    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

SND_DEINIT:
    hi_adp_snd_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    fclose(g_ts_file);
    g_ts_file = HI_NULL;

    return ret;
}


