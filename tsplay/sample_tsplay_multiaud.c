/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tsplay_multiaud.c
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
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

//#include "hi_unf_ecs.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"

#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
//#include "HA.AUDIO.DRA.decode.h"
#include "hi_unf_acodec_pcmdec.h"
//#include "HA.AUDIO.WMA9STD.decode.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_dtshddec.h"
#ifdef DOLBYPLUS_HACODEC_SUPPORT
#include "hi_unf_acodec_dolbyplusdec.h"
#endif
#include "hi_unf_acodec_dolbytruehddec.h"
#include "hi_unf_acodec_truehdpassthrough.h"

#define  PLAY_DMX_ID  0

static FILE                          *g_ts_file = HI_NULL;
static pthread_t                     g_ts_thread;
static pthread_mutex_t               g_ts_mtx;
static hi_bool                       g_is_stop_ts_thread = HI_FALSE;
static hi_handle                     g_ts_buf;
static pmt_compact_tbl               *g_prog_tbl = HI_NULL;
static hi_unf_avplay_multi_audio_attr g_multi_aud_attr;
static hi_u32                        g_aud_pid[32] = {0};

hi_s32 avplay_reset_multi_aud_attr(hi_handle avplay,pmt_compact_tbl *prog_tbl,hi_u32 prog_num)
{
    hi_s32                  j, ret;
    hi_unf_audio_decode_attr    adec_attr[32];

    g_multi_aud_attr.pid_num = prog_tbl->proginfo[prog_num].a_element_num ;
    for (j = 0; j < g_multi_aud_attr.pid_num; j++) {
        adec_attr[j].id = prog_tbl->proginfo[prog_num].audio_info[j].audio_enc_type;

        if (adec_attr[j].id == HI_UNF_ACODEC_ID_MP2) {
            hi_unf_acodec_mp2_dec_get_default_open_param(&(adec_attr[j].param));
        } else if (adec_attr[j].id == HI_UNF_ACODEC_ID_AAC) {
            hi_unf_acodec_aac_dec_get_default_open_param(&(adec_attr[j].param));
        } else if (adec_attr[j].id == HI_UNF_ACODEC_ID_MP3) {
            hi_unf_acodec_mp3_dec_get_default_open_param(&(adec_attr[j].param));
        } else if (adec_attr[j].id == HI_UNF_ACODEC_ID_DTSHD) {
            static hi_unf_acodec_dtshd_decode_open_config dts_conf = {0};

            hi_unf_acodec_dtshd_dec_get_default_open_config(&dts_conf);
            hi_unf_acodec_dtshd_dec_get_default_open_param(&(adec_attr[j].param), &dts_conf);

            adec_attr[j].param.dec_mode = HI_UNF_ACODEC_DEC_MODE_SIMUL;
        }
#ifdef DOLBYPLUS_HACODEC_SUPPORT
        else if (HI_UNF_ACODEC_ID_DOLBY_PLUS == adec_attr[j].id) {
            static hi_unf_acodec_dolbyplus_decode_openconfig dolby_conf = {0};

            hi_unf_acodec_dolbyplus_dec_get_default_open_config(&dolby_conf);

            /* Dolby DVB Broadcast default settings */
            dolby_conf.drc_mode = DOLBYPLUS_DRC_RF;
            dolby_conf.dmx_mode = DOLBYPLUS_DMX_SRND;

            hi_unf_acodec_dolbyplus_dec_get_default_open_param(&(adec_attr[j].param), &dolby_conf);

            adec_attr[j].param.dec_mode = HI_UNF_ACODEC_DEC_MODE_SIMUL;
        }
#endif
        else if (adec_attr[j].id == HI_UNF_ACODEC_ID_TRUEHD) {
            hi_unf_acodec_truehd_dec_get_default_open_param(&(adec_attr[j].param));
            adec_attr[j].param.dec_mode = HI_UNF_ACODEC_DEC_MODE_THRU;        /* truehd just support pass-through */
            SAMPLE_COMMON_PRINTF(" TrueHD decoder(HBR Pass-through only).\n");
        } else if (adec_attr[j].id == HI_UNF_ACODEC_ID_DOLBY_TRUEHD) {
            static hi_unf_acodec_truehd_decode_open_config dolby_truehd_conf = {0};
            hi_unf_acodec_dolby_truehd_dec_get_default_open_config(&dolby_truehd_conf);
            hi_unf_acodec_dolby_truehd_dec_get_default_open_param(&(adec_attr[j].param), &dolby_truehd_conf);
        }

        g_aud_pid[j] = (hi_u32)(g_prog_tbl->proginfo[prog_num].audio_info[j].audio_pid);
        //printf("dai %u, aud_pid[%u] : 0x%x\n", g_multi_aud_attr.u32PidNum, j,  g_aud_pid[j]);
    }

    g_multi_aud_attr.pid = g_aud_pid;
    g_multi_aud_attr.attr = adec_attr;

    ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_AUD_MULTI_TRACK, &g_multi_aud_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("set multi audio attr to avplay failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 avplay_play_prog(hi_handle avplay,pmt_compact_tbl *prog_tbl,hi_u32 prog_num,hi_bool is_aud_play)
{
    hi_unf_avplay_stop_opt stop_opt;
    hi_u32                   vid_pid;
    hi_u32                   aud_pid;
    hi_u32                   pcr_pid;
    hi_unf_vcodec_type     vid_type;
    hi_u32                   aud_type;
    hi_s32                   ret;

    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop_opt.timeout = 0;
    ret = hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_stop failed.\n");
        return ret;
    }

    ret = avplay_reset_multi_aud_attr(avplay, prog_tbl, prog_num);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("avplay_reset_multi_aud_attr failed 0x%x\n", ret);
        return ret;
    }

    prog_num = prog_num % prog_tbl->prog_num;
    if (prog_tbl->proginfo[prog_num].v_element_num > 0 ) {
        vid_pid = prog_tbl->proginfo[prog_num].v_element_pid;
        vid_type = prog_tbl->proginfo[prog_num].video_type;
    } else {
        vid_pid = INVALID_TSPID;
        vid_type = HI_UNF_VCODEC_TYPE_MAX;
    }

    if (prog_tbl->proginfo[prog_num].a_element_num > 0) {
        aud_pid  = prog_tbl->proginfo[prog_num].a_element_pid;
        aud_type = prog_tbl->proginfo[prog_num].audio_type;
    } else {
        aud_pid = INVALID_TSPID;
        aud_type = 0xffffffff;
    }

    pcr_pid = prog_tbl->proginfo[prog_num].pcr_pid;
    if (INVALID_TSPID != pcr_pid) {
        hi_unf_sync_attr  sync_attr;

        ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("hi_unf_avplay_get_attr Sync failed 0x%x\n", ret);
            return ret;
        }

        if (sync_attr.ref_mode == HI_UNF_SYNC_REF_MODE_PCR) {
            ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_PCR_PID, &pcr_pid);
            if (ret != HI_SUCCESS) {
                SAMPLE_COMMON_PRINTF("hi_unf_avplay_set_attr Sync failed 0x%x\n", ret);
                return ret;
            }
        }
    }

    if (vid_pid != INVALID_TSPID) {
        ret = hi_adp_avplay_set_vdec_attr(avplay, vid_type, HI_UNF_VDEC_WORK_MODE_NORMAL);
        ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VID_PID,&vid_pid);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("call hi_adp_avplay_set_vdec_attr failed.\n");
            return ret;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("call hi_unf_avplay_start failed.\n");
            return ret;
        }
    }

    if (HI_TRUE == is_aud_play && aud_pid != INVALID_TSPID) {
        //aud_type = HI_UNF_ACODEC_ID_DTSHD;
        //printf("aud_type = %#x\n",aud_type);
        ret  = hi_adp_avplay_set_adec_attr(avplay, aud_type);

        ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &aud_pid);
        if (ret != HI_SUCCESS) {
            SAMPLE_COMMON_PRINTF("hi_adp_avplay_set_adec_attr failed:%#x\n",ret);
            return ret;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_avplay_start to start audio failed.\n");
            //return ret;
        }
    }

    return HI_SUCCESS;
}

void ts_thread(void *args)
{
    hi_unf_stream_buf   stream_buf;
    hi_u32                read_len;
    hi_s32                ret;

    while (!g_is_stop_ts_thread) {
        pthread_mutex_lock(&g_ts_mtx);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, 188 * 50, &stream_buf, 1000);
        if (ret != HI_SUCCESS ) {
            pthread_mutex_unlock(&g_ts_mtx);
            continue;
        }

        read_len = fread(stream_buf.data, sizeof(hi_s8), 188 * 50, g_ts_file);
        if(read_len <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(g_ts_file);
            pthread_mutex_unlock(&g_ts_mtx);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_buf, &stream_buf, 0, read_len);
        if (ret != HI_SUCCESS) {
            printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_mtx);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS ) {
        printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32                      ret;
    hi_handle                   win;
    hi_handle                   avplay;
    hi_unf_avplay_attr        avplay_attr;
    hi_unf_sync_attr          sync_attr;
    hi_unf_avplay_stop_opt    stop_opt;
    hi_char                     input_cmd[32];
    hi_unf_video_format            enc_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    hi_u32                      prog_num;
    hi_s32                      i;
    hi_handle                   track;
    hi_unf_audio_track_attr    track_attr;
    hi_unf_dmx_ts_buffer_attr tsbuf_attr;

    if (3 == argc) {
        enc_fmt = hi_adp_str2fmt(argv[2]);
    } else if (2 == argc) {
        enc_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    } else {
        printf("Usage: %s file [vo_format]\n", argv[0]);
        printf("       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n");
        printf("Example: %s ./test.ts 1080P50\n", argv[0]);
        return 0;
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
        printf("call SndInit failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(enc_fmt);
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (ret != HI_SUCCESS) {
        printf("call VoInit failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_init failed.\n");
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_attach_ts_port(PLAY_DMX_ID,HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_attach_ts_port failed.\n");
        goto VO_DEINIT;
    }

    tsbuf_attr.buffer_size = 0x200000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;

    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_avplay_init failed.\n");
        goto TSBUF_FREE;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.demux_id = PLAY_DMX_ID;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win, avplay);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
    }
    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay);
    if (ret != HI_SUCCESS) {
        printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    pthread_mutex_init(&g_ts_mtx,NULL);
    g_is_stop_ts_thread = HI_FALSE;
    pthread_create(&g_ts_thread, HI_NULL, (void *)ts_thread, HI_NULL);

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(PLAY_DMX_ID, &g_prog_tbl);
    if (ret != HI_SUCCESS) {
        printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }

    prog_num = 0;

    pthread_mutex_lock(&g_ts_mtx);
    rewind(g_ts_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buf);

    ret = avplay_play_prog(avplay, g_prog_tbl, prog_num, HI_TRUE);
    if (ret != HI_SUCCESS) {
        printf("call SwitchProg failed.\n");
        goto AVPLAY_STOP;;
    }

    pthread_mutex_unlock(&g_ts_mtx);

    i = prog_num + 1;

    while (1) {
        printf("please input 'q' to quit, 'c' to switch the aud!\n");

        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }

        if ('c' == input_cmd[0]) {
            if (g_multi_aud_attr.pid_num <= 1) {
                printf("only one audio track exists, can not switch!\n");
                continue;
            }

            i = (i >= g_multi_aud_attr.pid_num) ? 0 : i;

            printf("now switch the aud, pid = %#x!\n", g_aud_pid[i]);

            ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &g_aud_pid[i++]);
            if (ret != HI_SUCCESS) {
                printf("hi_unf_avplay_set_attr failed 0x%x\n", ret);
            }

            continue;
        }

        prog_num = atoi(input_cmd);
        if (prog_num == 0) {
            prog_num = 1;
        }

        pthread_mutex_lock(&g_ts_mtx);
        rewind(g_ts_file);
        hi_unf_dmx_reset_ts_buffer(g_ts_buf);

        ret = avplay_play_prog(avplay,g_prog_tbl,prog_num-1,HI_TRUE);
        if (ret != HI_SUCCESS) {
            printf("call SwitchProgfailed!\n");
            break;
        }
        pthread_mutex_unlock(&g_ts_mtx);
    }

AVPLAY_STOP:
    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop_opt.timeout = 0;
    hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_tbl);
    g_prog_tbl = HI_NULL;
    hi_adp_search_de_init();

    g_is_stop_ts_thread = HI_TRUE;
    pthread_join(g_ts_thread, HI_NULL);
    pthread_mutex_destroy(&g_ts_mtx);

SND_DETACH:
    hi_unf_snd_detach(track, avplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);

WIN_DETACH:
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);

    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

VCHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

TSBUF_FREE:
    hi_unf_dmx_destroy_ts_buffer(g_ts_buf);

DMX_DEINIT:
    hi_unf_dmx_detach_ts_port(0);
    hi_unf_dmx_deinit();

VO_DEINIT:
    hi_unf_win_destroy(win);
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


