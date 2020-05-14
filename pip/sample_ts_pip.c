/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_ts_pip.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/09/23
  Description   :
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
#include "hi_adp_frontend.h"

#define AVPLAY_NUM  2

FILE               *g_ts_file = HI_NULL;
static pthread_t   g_ts_thd;
static pthread_mutex_t g_ts_mutex;
static hi_bool     g_stop_ts_thread = HI_FALSE;
hi_handle          g_ts_buffer;
pmt_compact_tbl    *g_prog_table = HI_NULL;

#define  PLAY_DMX_ID  0

static hi_s32 stop_ts_pip_play(hi_handle avplay, hi_bool is_aud_play)
{
    hi_unf_avplay_stop_opt stop_opt;
    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop_opt.timeout = 0;
    hi_s32 ret = HI_FAILURE;

    if (is_aud_play == HI_TRUE) {
        ret = hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    } else {
        ret = hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop_opt);
    }

    if (ret != HI_SUCCESS) {
        printf("call hi_unf_avplay_stop failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_void TsTthread(hi_void *args)
{
    hi_unf_stream_buf     StreamBuf;
    hi_u32                Readlen;
    hi_s32                ret;

    while (!g_stop_ts_thread) {
        pthread_mutex_lock(&g_ts_mutex);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buffer, 188 * 50, &StreamBuf, 1000);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        Readlen = fread(StreamBuf.data, sizeof(hi_s8), 188 * 50, g_ts_file);
        if (Readlen <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_file);
            pthread_mutex_unlock(&g_ts_mutex);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_buffer, &StreamBuf, 0, Readlen);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }
        pthread_mutex_unlock(&g_ts_mutex);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buffer);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}


hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32                      ret;
    hi_s32                      i;
    hi_handle                   win[AVPLAY_NUM];
    hi_handle                   avplay[AVPLAY_NUM];
    hi_unf_avplay_attr          avplay_attr;
    hi_unf_sync_attr            sync_attr;
    hi_char                     input_cmd[32];
    hi_unf_video_format         video_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32                      prog_num;
    hi_unf_video_rect           child_rect;
    hi_unf_avplay_stop_opt      stop_opt;
    hi_bool                     is_aud_mix = 0;
    hi_bool                     is_aud_play[AVPLAY_NUM];
    hi_handle                   track[AVPLAY_NUM];
    hi_unf_audio_track_attr     track_attr;
    hi_unf_snd_gain             sound_gain;
    hi_u32 ts_pid = INVALID_TSPID;

    sound_gain.linear_mode = HI_TRUE;

    if (argc < 2) {
        printf("Usage:\n%s tsfile [audiomix] [vo_format]\n", argv[0]);
        printf("\taudiomix\t: 0: not mix audio, 1: mix audio\n");
        printf("\tvo_format\t: 2160P60 2160P50 2160P30 2160P25 2160P24 1080P60 1080P50 1080i60 1080i50 720P60 720P50\n\n");
        printf("Example: %s ./test.ts 0 1080P50\n", argv[0]);
        return 0;
    }

    if (argc >= 3) {
        is_aud_mix = strtol(argv[2], NULL, 0);
    }

    if (argc >= 4) {
        video_fmt = hi_adp_str2fmt(argv[3]);
    }

    g_ts_file = fopen(argv[1], "rb");
    if (!g_ts_file) {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(video_fmt);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    hi_handle *win_handle = &win[0];
    ret = hi_adp_win_create(HI_NULL, win_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_win_create 0 failed.\n");
        goto VO_DEINIT;
    }

    child_rect.width = 720;
    child_rect.height = 576;
    child_rect.x = 0;
    child_rect.y = 0;
    win_handle = &win[1];
    ret = hi_adp_win_create(&child_rect, win_handle);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_win_create 1 failed.\n");
        hi_unf_win_destroy(win[0]);
        goto VO_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_init failed.\n");
        goto DESTROY_WINDOW;
    }

    for (i = 0 ; i < AVPLAY_NUM; i++) {
        ret = hi_unf_dmx_attach_ts_port(i, HI_UNF_DMX_PORT_RAM_0);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_dmx_attach_ts_port failed.\n");
            goto DMX_DEINIT;
        }
    }

    hi_unf_dmx_ts_buffer_attr tsbuf_attr;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    tsbuf_attr.buffer_size = 0x800000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_buffer);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    pthread_mutex_init(&g_ts_mutex, NULL);
    g_stop_ts_thread = HI_FALSE;
    pthread_create(&g_ts_thd, HI_NULL, (hi_void *)TsTthread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_table);
    if (ret != HI_SUCCESS) {
        printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    pthread_mutex_lock(&g_ts_mutex);
    rewind(g_ts_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buffer);

    for (i = 0; i < AVPLAY_NUM; i++) {
        printf("===============avplay[%d]===============\n",i);

        hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);

        avplay_attr.demux_id = i;

        ret = hi_unf_avplay_create(&avplay_attr, &avplay[i]);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("call hi_unf_avplay_create failed.\n");
            goto AVPLAY_STOP;
        }

        ret = hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_STOP;
        }

        ret = hi_unf_win_attach_src(win[i], avplay[i]);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("call hi_unf_win_attach_src failed.\n");
            goto AVPLAY_STOP;
        }
        ret = hi_unf_win_set_enable(win[i], HI_TRUE);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("call hi_unf_win_set_enable failed.\n");
            goto AVPLAY_STOP;
        }

        is_aud_play[i] = HI_FALSE;
        if (i == 0) {
            is_aud_play[i] = HI_TRUE;

            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_snd_get_default_track_attr failed.\n");
            }
            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track[i]);
            ret |= hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            ret |= hi_unf_snd_attach(track[i], avplay[i]);
            if (ret != HI_SUCCESS) {
                pthread_mutex_unlock(&g_ts_mutex);
                printf("call HI_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }
        else if(i == 1 && is_aud_mix == HI_TRUE) {
            is_aud_play[i] = HI_TRUE;

            track_attr.track_type = HI_UNF_SND_TRACK_TYPE_SLAVE;
            ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_snd_get_default_track_attr failed.\n");
            }
            ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track[i]);
            sound_gain.gain = 60;
            ret = hi_unf_snd_set_track_weight(track[i], &sound_gain);
            ret |= hi_unf_avplay_chan_open(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            ret |= hi_unf_snd_attach(track[i], avplay[i]);
            if (ret != HI_SUCCESS) {
                pthread_mutex_unlock(&g_ts_mutex);
                printf("call HI_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }

        ret = hi_unf_avplay_get_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
        ret = hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&g_ts_mutex);
            printf("call hi_unf_avplay_set_attr failed.\n");
            goto AVPLAY_STOP;
        }

        prog_num = 0;
        ret = hi_adp_avplay_play_prog(avplay[i], g_prog_table, prog_num + i,
            is_aud_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        if (ret != HI_SUCCESS) {
            printf("call hi_adp_avplay_play_prog failed\n");
        }
    }

    pthread_mutex_unlock(&g_ts_mutex);

    while (1) {
        printf("please input the q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        prog_num = atoi(input_cmd);

        for (i = 0; i < AVPLAY_NUM; i++)
        {
            ret = stop_ts_pip_play(avplay[i], is_aud_play[i]);
            if (ret != HI_SUCCESS) {
                printf("call hi_unf_avplay_stop failed.\n");
                break;
            }

            (hi_void)hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_PCR_PID, &ts_pid);
            (hi_void)hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_VID_PID, &ts_pid);

            if (is_aud_play[i] == HI_TRUE) {
                (hi_void)hi_unf_avplay_set_attr(avplay[i], HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &ts_pid);
            }
        }

        pthread_mutex_lock(&g_ts_mutex);
        rewind(g_ts_file);
        hi_unf_dmx_reset_ts_buffer(g_ts_buffer);

        for (i = 0 ; i < AVPLAY_NUM; i++) {
            ret = hi_adp_avplay_play_prog(avplay[i], g_prog_table, prog_num + i,
                is_aud_play[i] ? HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD : HI_UNF_AVPLAY_MEDIA_CHAN_VID);
            if (ret != HI_SUCCESS) {
                printf("call SwitchProgfailed!\n");
                break;
            }
        }

        pthread_mutex_unlock(&g_ts_mutex);
    }

    g_stop_ts_thread = HI_TRUE;
    pthread_join(g_ts_thd, HI_NULL);

    pthread_mutex_destroy(&g_ts_mutex);

AVPLAY_STOP:
    for(i = 0; i < AVPLAY_NUM; i++) {
        stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop_opt.timeout = 0;
        if (is_aud_mix == HI_FALSE && i == 1) {
            hi_unf_avplay_stop(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop_opt);
        } else {
            hi_unf_avplay_stop(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
            hi_unf_snd_detach(track[i], avplay[i]);
            hi_unf_snd_destroy_track(track[i]);
        }
        hi_unf_win_set_enable(win[i], HI_FALSE);
        hi_unf_win_detach_src(win[i], avplay[i]);
        if (is_aud_mix == HI_FALSE && i == 1) {
            hi_unf_avplay_chan_close(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        } else {
            hi_unf_avplay_chan_close(avplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        }
        hi_unf_avplay_destroy(avplay[i]);
    }
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_table);
    g_prog_table = HI_NULL;
    hi_adp_search_de_init();
    hi_unf_dmx_destroy_ts_buffer(g_ts_buffer);

DMX_DEINIT:
    for (i = 0; i < AVPLAY_NUM; i++) {
        hi_unf_dmx_detach_ts_port(i);
    }

    hi_unf_dmx_deinit();

DESTROY_WINDOW:
    hi_unf_win_destroy(win[1]);
    hi_unf_win_destroy(win[0]);

VO_DEINIT:
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


