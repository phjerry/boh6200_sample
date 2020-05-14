/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tsplay.c
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

//#include "hi_unf_ecs.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_unf_ssm.h"
#include "hi_adp_mpi.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"

typedef struct
{
    hi_handle   avplay;
    FILE       *ts_file;
    hi_handle   ts_buffer_handle;
    pthread_mutex_t  *ts_mutex;
}tee_ts_param;

static pthread_t   g_ts_thread[2] = {-1};
static hi_bool     g_stop_ts_thread = HI_FALSE;
pmt_compact_tbl   *g_prog_table[2] = {HI_NULL};

#define  PLAY_DMX_ID_0  0
#define  PLAY_DMX_ID_1  1
#define  TS_READ_SIZE   (188 * 1000)

hi_void ts_thread(hi_void *args)
{
    hi_unf_stream_buf   stream_buffer;
    hi_u32                read_len;
    hi_s32                ret;
    tee_ts_param           *ts_param;

    ts_param = (tee_ts_param *)args;

    while (!g_stop_ts_thread)
    {
        pthread_mutex_lock(ts_param->ts_mutex);
        ret = hi_unf_dmx_get_ts_buffer(ts_param->ts_buffer_handle, TS_READ_SIZE, &stream_buffer, 1000);
        if (ret != HI_SUCCESS)
        {
            pthread_mutex_unlock(ts_param->ts_mutex);
            continue;
        }

        read_len = fread(stream_buffer.data, sizeof(hi_s8), TS_READ_SIZE, ts_param->ts_file);
        if (read_len <= 0)
        {
            printf("read ts file end and rewind!\n");
            rewind(ts_param->ts_file);
            pthread_mutex_unlock(ts_param->ts_mutex);
            continue;
        }

        ret = hi_unf_dmx_put_ts_buffer(ts_param->ts_buffer_handle, read_len, 0);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(ts_param->ts_mutex);
    }

    pthread_mutex_lock(ts_param->ts_mutex);
    ret = hi_unf_dmx_reset_ts_buffer(ts_param->ts_buffer_handle);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }
    pthread_mutex_unlock(ts_param->ts_mutex);

    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                             ret;
    hi_handle                          win_handle[2] = {HI_INVALID_HANDLE};
    hi_handle                          avplay[2] = {HI_INVALID_HANDLE};
    hi_unf_avplay_attr                 avplay_attr;
    hi_unf_sync_attr                   sync_attr;
    hi_unf_avplay_stop_opt             stop;
    hi_char                            input_cmd[32];
    hi_unf_video_format                vid_format = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32                             prog_num;
    hi_handle                          track_handle[2] = {HI_INVALID_HANDLE};
    hi_unf_audio_track_attr            track_attr;
    tee_ts_param                       ts_param[2];
    FILE                              *ts_file[2];
    hi_handle                          ts_buffer_handle[2] = {-1};
    hi_unf_video_rect                  master_rect;
    hi_unf_video_rect                  child_rect;
    hi_unf_avplay_smp_attr             smp_attr;
    hi_u32                             i;
    hi_u32                             avplay_num = 2;
    hi_unf_dmx_ts_buffer_attr          ts_buffer_attr;
    hi_u32                             ssm[2];

    pthread_mutex_t ts_mutex[2];

    if (4 == argc)
    {
        vid_format = hi_adp_str2fmt(argv[3]);
    }
    else if (3 == argc)
    {
        vid_format = HI_UNF_VIDEO_FMT_1080P_50;

    }
    else
    {
        printf("Usage: sample_tee_tsplay_pip file file [vo_format]\n"
               "       vo_format:1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n");
        printf("Example:./sample_tee_tsplay_pip ./[HD]test.ts ./[SD]test.ts 1080P50\n");
        return 0;
    }

    ts_file[0] = fopen(argv[1], "rb");
    if (!ts_file[0])
    {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    ts_file[1] = fopen(argv[2], "rb");
    if (!ts_file[1])
    {
        printf("open file %s error!\n", argv[2]);
        return -1;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(vid_format);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    master_rect.width  = 0;
    master_rect.height = 0;
    master_rect.x      = 0;
    master_rect.y      = 0;
    ret = hi_adp_win_create(&master_rect, &win_handle[0]);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    child_rect.width = 964;
    child_rect.height = 544;
    child_rect.x = 2850 ;
    child_rect.y = 1600;
    ret = hi_adp_win_create(&child_rect, &win_handle[1]);
    if (ret != HI_SUCCESS)
    {
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_init failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID_0, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID_1, HI_UNF_DMX_PORT_RAM_1);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto DMX_DEINIT;
    }

    ts_buffer_attr.buffer_size = 0x200000;
    ts_buffer_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    hi_handle *buffer_handle = &ts_buffer_handle[0];
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &ts_buffer_attr, buffer_handle);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ts_buffer_attr.buffer_size = 0x200000;
    ts_buffer_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    buffer_handle = &ts_buffer_handle[1];
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_1, &ts_buffer_attr, buffer_handle);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_unf_ssm_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_ssm_init failed.\n");
        goto TSBUF_FREE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_avplay_init failed.\n");
        goto SSM_DEINIT;
    }

    pthread_mutex_t *thread_mutex = &ts_mutex[0];
    pthread_mutex_init(thread_mutex, NULL);
    thread_mutex = &ts_mutex[1];
    pthread_mutex_init(thread_mutex, NULL);

    for (i = 0; i < avplay_num; i++)
    {
        ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);

        switch (i)
        {
            case 0 :
            {
                avplay_attr.demux_id = PLAY_DMX_ID_0;
                break;
            }

            case 1 :
            {
                avplay_attr.demux_id = PLAY_DMX_ID_1;
                break;
            }

            default:
                return HI_FAILURE;
        }

        ret = hi_unf_avplay_create(&avplay_attr, &avplay[i]);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_create failed.\n");
            goto SSM_DESTROY;
        }

        hi_unf_ssm_attr ssm_attr = {0};
        ssm_attr.intent = HI_UNF_SSM_INTENT_WATCH;
        ret = hi_unf_ssm_create(&ssm_attr, &ssm[i]);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_ssm_create failed.\n");
            goto AVPLAY_DESTROY;
        }

        hi_unf_avplay_session_info session_info;
        session_info.ssm = ssm[i];
        ret = hi_unf_avplay_set_session_info(avplay[i], &session_info);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_avplay_set_session_info failed.\n");
            goto SSM_DESTROY;
        }

        if (1 == i)
        {
            smp_attr.enable = HI_TRUE;
            smp_attr.chan = HI_UNF_AVPLAY_MEDIA_CHAN_VID;
            hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SMP, &smp_attr);
        }

        ret = hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_chan_open failed.\n");
            goto SSM_DESTROY;
        }

        if (1 == i)
        {
            smp_attr.enable = HI_FALSE;
            smp_attr.chan = HI_UNF_AVPLAY_MEDIA_CHAN_AUD;
            hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SMP, &smp_attr);
        }

        ret = hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_chan_open failed.\n");
            goto VCHN_CLOSE;
        }

        ret = hi_unf_win_attach_src(win_handle[i], avplay[i]);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
            goto ACHN_CLOSE;
        }

        ret = hi_unf_win_set_enable(win_handle[i], HI_TRUE);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_win_set_enable failed.\n");
            goto WIN_DETACH;
        }

        if (0 == i)
        {
            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
            if (ret != HI_SUCCESS)
            {
                printf("call hi_unf_snd_get_default_track_attr failed.\n");
                goto WIN_DETACH;
            }
            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track_handle[i]);
            if (ret != HI_SUCCESS)
            {
                printf("call hi_unf_snd_create_track failed.\n");
                goto WIN_DETACH;
            }

            ret = hi_unf_snd_attach(track_handle[i], avplay[i]);
            if (ret != HI_SUCCESS)
            {
                printf("call hi_unf_snd_attach failed.\n");
                goto TRACK_DESTROY;
            }
        }

        if (1 == i)
        {
            track_attr.track_type = HI_UNF_SND_TRACK_TYPE_SLAVE;
            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
            if (ret != HI_SUCCESS)
            {
                printf("call hi_unf_snd_get_default_track_attr failed.\n");
                goto WIN_DETACH;
            }
            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track_handle[i]);
            if (ret != HI_SUCCESS)
            {
                printf("call hi_unf_snd_create_track failed.\n");
                goto WIN_DETACH;
            }

            ret = hi_unf_snd_attach(track_handle[i], avplay[i]);
            if (ret != HI_SUCCESS)
            {
                printf("call hi_unf_snd_attach failed.\n");
                goto TRACK_DESTROY;
            }
        }

        ret = hi_unf_avplay_get_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
        ret |= hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_set_attr failed.\n");
            goto SND_DETACH;
        }

        ts_param[i].avplay = avplay[i];
        ts_param[i].ts_file = ts_file[i];
        ts_param[i].ts_buffer_handle = ts_buffer_handle[i];
        ts_param[i].ts_mutex = &ts_mutex[i];
        printf("ts_buffer_handle[%d] = %d\n", i, ts_buffer_handle[i]);

        g_stop_ts_thread = HI_FALSE;
        pthread_create(&g_ts_thread[i], HI_NULL, (hi_void *)ts_thread, &ts_param[i]);

        hi_adp_search_init();
        if (0 == i)
        {
            ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID_0, &g_prog_table[0]);
            if (ret != HI_SUCCESS)
            {
                printf("call HIADP_Search_GetAllPmt failed.\n");
                goto SND_DETACH;
            }
        }

        if (1 == i)
        {
            ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID_1, &g_prog_table[1]);
            if (ret != HI_SUCCESS)
            {
                printf("call HIADP_Search_GetAllPmt failed.\n");
                goto PSISI_FREE;
            }
        }

        prog_num = 0;

        pthread_mutex_lock(&ts_mutex[i]);
        rewind(ts_file[i]);
        hi_unf_dmx_reset_ts_buffer(ts_buffer_handle[i]);

        ret = hi_adp_avplay_play_prog(avplay[i], g_prog_table[i], prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_adp_avplay_play_prog failed.\n");
            goto AVPLAY_STOP;
        }
        pthread_mutex_unlock(&ts_mutex[i]);

    }

    while (1)
    {
        printf("please input 'q' to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        if ('t' == input_cmd[0])
        {
            hi_unf_avplay_stop(avplay[0], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            hi_unf_win_set_freeze_mode(win_handle[0], HI_UNF_WIN_FREEZE_MODE_BLACK);
            hi_unf_win_set_zorder(win_handle[0], HI_UNF_WIN_ZORDER_MOVEBOTTOM);
            hi_unf_avplay_start(avplay[0], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
            hi_unf_avplay_start(avplay[0], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

            hi_unf_win_set_freeze_mode(win_handle[1], HI_UNF_WIN_FREEZE_MODE_BLACK);
        }

        if ('b' == input_cmd[0])
        {
            hi_unf_avplay_stop(avplay[1], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            hi_unf_win_set_freeze_mode(win_handle[1], HI_UNF_WIN_FREEZE_MODE_BLACK);
            hi_unf_win_set_zorder(win_handle[1], HI_UNF_WIN_ZORDER_MOVEBOTTOM);
            hi_unf_avplay_start(avplay[1], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
            hi_unf_avplay_start(avplay[1], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

            hi_unf_win_set_freeze_mode(win_handle[0], HI_UNF_WIN_FREEZE_MODE_BLACK);
        }
    }

AVPLAY_STOP:
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    for (i = 0; i < avplay_num; i++)
    {
        if (HI_INVALID_HANDLE != avplay[i])
        {
            hi_unf_avplay_stop(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
        }
    }
PSISI_FREE:
    for (i = 0; i < avplay_num; i++)
    {
        if (g_prog_table[i] != HI_NULL)
        {
            hi_adp_search_free_all_pmt(g_prog_table[i]);
            g_prog_table[i] = HI_NULL;
        }
    }
    hi_adp_search_de_init();

    g_stop_ts_thread = HI_TRUE;

    for (i = 0; i < avplay_num; i++)
    {
        if (-1 != g_ts_thread[i])
        {
            pthread_join(g_ts_thread[i], HI_NULL);
        }
    }

SND_DETACH:
    for (i = 0; i < avplay_num; i++)
    {
        if ((HI_INVALID_HANDLE != track_handle[i]) && (HI_INVALID_HANDLE != avplay[i]))
        {
            hi_unf_snd_detach(track_handle[i], avplay[i]);
        }
    }

TRACK_DESTROY:
    for (i = 0; i < avplay_num; i++)
    {
        if (HI_INVALID_HANDLE != track_handle[i])
        {
            hi_unf_snd_destroy_track(track_handle[i]);
        }
    }

WIN_DETACH:
    for (i = 0; i < avplay_num; i++)
    {
        if ((HI_INVALID_HANDLE != win_handle[i]) && (HI_INVALID_HANDLE != avplay[i]))
        {
            hi_unf_win_set_enable(win_handle[i], HI_FALSE);
            hi_unf_win_detach_src(win_handle[i], avplay[i]);
        }
    }

ACHN_CLOSE:
    for (i = 0; i < avplay_num; i++)
    {
        if (HI_INVALID_HANDLE != avplay[i])
        {
            hi_unf_avplay_chan_close(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }
    }

VCHN_CLOSE:
    for (i = 0; i < avplay_num; i++)
    {
        if (HI_INVALID_HANDLE != avplay[i])
        {
            hi_unf_avplay_chan_close(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        }
    }

SSM_DESTROY:
    for (i = 0; i < avplay_num; i++) {
        if (ssm[i] != HI_INVALID_HANDLE) {
            hi_unf_ssm_destroy(ssm[i]);
        }
    }

AVPLAY_DESTROY:
    for (i = 0; i < avplay_num; i++)
    {
        if (HI_INVALID_HANDLE != avplay[i])
        {
            hi_unf_avplay_destroy(avplay[i]);
        }
    }

//AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

SSM_DEINIT:
    hi_unf_ssm_deinit();

TSBUF_FREE:
    for (i = 0; i < avplay_num; i++)
    {
        if (-1 != ts_buffer_handle[i])
        {
            hi_unf_dmx_destroy_ts_buffer(ts_buffer_handle[i]);
        }
    }
    pthread_mutex_destroy(&ts_mutex[0]);
    pthread_mutex_destroy(&ts_mutex[1]);


DMX_DEINIT:
    for (i = 0; i < avplay_num; i++)
    {
        hi_unf_dmx_detach_ts_port(i);
    }

    hi_unf_dmx_deinit();

VO_DEINIT:
    for (i = 0; i < avplay_num; i++)
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

    fclose(ts_file[0]);
    fclose(ts_file[1]);

    return ret;
}


