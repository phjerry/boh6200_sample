/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : tsplay_pid.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2011/05/13
  Description   :
  History       :
  1.Date        : 2011/05/13
    Author      :
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
#include "hi_unf_acodec_truehdpassthrough.h"
#if defined(DOLBYPLUS_HACODEC_SUPPORT)
#include "hi_unf_acodec_dolbyplusdec.h"
#endif
#include "hi_unf_acodec_dtshddec.h"
#include "sample_tsplay_common.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

static FILE            *g_ts_file = HI_NULL;
static pthread_t       g_ts_thread;
static pthread_mutex_t g_ts_mtx;
static hi_bool         g_is_stop_ts_thread = HI_FALSE;
static hi_handle       g_ts_buf;
static pmt_compact_tbl *g_prog_tbl = HI_NULL;

void ts_thread(void *args)
{
    hi_unf_stream_buf stream_buf;
    hi_u32              read_len;
    hi_s32              ret;

    while (!g_is_stop_ts_thread)
    {
        pthread_mutex_lock(&g_ts_mtx);
        ret = hi_unf_dmx_get_ts_buffer(g_ts_buf, 188 * 50, &stream_buf, 1000);
        if (ret != HI_SUCCESS ) {
            pthread_mutex_unlock(&g_ts_mtx);
            continue;
        }

        read_len = fread(stream_buf.data, sizeof(hi_s8), 188 * 50, g_ts_file);
        if(read_len <= 0) {
            sample_printf("read ts file end and rewind!\n");
            rewind(g_ts_file);
            pthread_mutex_unlock(&g_ts_mtx);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(g_ts_buf, &stream_buf, 0, read_len);
        if (ret != HI_SUCCESS ) {
            sample_printf("call hi_unf_dmx_put_ts_buffer failed.\n");
        }

        pthread_mutex_unlock(&g_ts_mtx);
    }

    ret = hi_unf_dmx_reset_ts_buffer(g_ts_buf);
    if (ret != HI_SUCCESS ) {
        sample_printf("call hi_unf_dmx_reset_ts_buffer failed.\n");
    }

    return;
}

hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32                    ret;
    hi_handle                 win;
    hi_handle                 avplay;
    hi_unf_avplay_attr      avplay_attr;
    hi_unf_sync_attr        sync_attr;
    hi_unf_avplay_stop_opt  stop_opt;
    hi_char                   input_cmd[32];
    hi_unf_video_format       enc_fmt;
    hi_u32                    prog_num;
    hi_unf_vcodec_type      vdec_type = HI_UNF_VCODEC_TYPE_MAX;
    hi_u32                    adec_type = 0;
    hi_u32                    vid_pid=0, aud_pid=0;
    hi_bool                   is_aud_play = HI_FALSE;
    hi_bool                   is_vid_play = HI_FALSE;
    pmt_compact_tbl           prog_tbl;
    pmt_compact_prog          prog_info;
    hi_handle                 track;
    hi_unf_audio_track_attr  track_attr;
    hi_unf_dmx_ts_buffer_attr   tsbuf_attr;

    if(argc != 7) {
        printf("Usage: sample_tsplay_pid tsfile vo_format vpid vtype apid atype\n"
                " vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50\n"
                " vtype: mpeg2/mpeg4/h263/sor/vp6/vp6f/vp6a/h264/avs/real8/real9/vc1\n"
                " atype: aac/mp3/dts/dra/mlp/pcm/ddp(ac3/eac3)\n");
        printf(" examples: \n");
        printf("   sample_tsplay_pid tsfile 1080P60 vpid h264 \"null\" \"null\"\n");
        printf("   sample_tsplay_pid tsfile 1080P60 \"null\" \"null\" apid mp3 \n");
        printf("   sample_tsplay_pid tsfile 1080P60 vpid h264   apid mp3 \n");
        return 0;
    }

    g_ts_file = fopen(argv[1], "rb");
    if (!g_ts_file) {
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    enc_fmt = hi_adp_str2fmt(argv[2]);

    if (strcasecmp("null",argv[3])) {
        is_vid_play = HI_TRUE;
        vid_pid = strtol(argv[3], NULL, 0);

        ret = get_vdec_type(argv[4], &vdec_type, HI_NULL, HI_NULL);

        if (ret != HI_SUCCESS) {
            sample_printf("unsupport vid codec type!\n");
            return -1;
        }
    }

    if (strcasecmp("null",argv[5]))
    {
        is_aud_play = HI_TRUE;
        aud_pid = strtol(argv[5],NULL,0);

        ret = get_adec_type(argv[6], &adec_type);

        if (ret != HI_SUCCESS) {
            sample_printf("unsupport aud codec type!\n");
            return -1;
        }
    }

    if (!(is_vid_play || is_aud_play)) {
        return 0;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_snd_init();
    if (HI_SUCCESS != ret) {
        sample_printf("call SndInit failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(enc_fmt);
    if (HI_SUCCESS != ret) {
        sample_printf("call hi_adp_disp_init failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (HI_SUCCESS != ret) {
        sample_printf("call VoInit failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    ret |= hi_unf_dmx_attach_ts_port(0, HI_UNF_DMX_PORT_RAM_0);
    if (HI_SUCCESS != ret) {
        sample_printf("call VoInit failed.\n");
        goto VO_DEINIT;
    }

    tsbuf_attr.buffer_size = 0x200000;
    tsbuf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;

    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &tsbuf_attr, &g_ts_buf);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto TSBUF_FREE;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chan_open failed.\n");
        goto VCHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win, avplay);
    if (HI_SUCCESS != ret) {
        sample_printf("call hi_unf_win_attach_src failed:%#x.\n", ret);
    }
    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_SND_Attach failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call HI_SND_Attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    pthread_mutex_init(&g_ts_mtx,NULL);
    g_is_stop_ts_thread = HI_FALSE;
    pthread_create(&g_ts_thread, HI_NULL, (void *)ts_thread, HI_NULL);


    memset(&prog_info, 0, sizeof(pmt_compact_prog));
    if (is_vid_play) {
         prog_info.v_element_num = 1;
         prog_info.v_element_pid = vid_pid;
         prog_info.video_type = vdec_type;
    }

    if (is_aud_play) {
         prog_info.a_element_num = 1;
         prog_info.a_element_pid = aud_pid;
         prog_info.audio_type = adec_type;
    }


    prog_tbl.prog_num = 1;
    prog_tbl.proginfo = &prog_info;
    g_prog_tbl = &prog_tbl;

    prog_num = 0;

    pthread_mutex_lock(&g_ts_mtx);
    rewind(g_ts_file);
    hi_unf_dmx_reset_ts_buffer(g_ts_buf);

    ret = hi_adp_avplay_play_prog(avplay, g_prog_tbl, prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        sample_printf("call SwitchProg failed.\n");
        goto AVPLAY_STOP;;
    }
    pthread_mutex_unlock(&g_ts_mtx);

    hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_LPCM);

    while(1) {
        static hi_u32 tplay_speed = 2;

        printf("please input the q to quit!, s to toggle spdif pass-through, h to toggle hdmi pass-through,v20(set volume 20),vxx(set volume xx)\n");

        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }

        if ('v' == input_cmd[0]) {
            hi_unf_snd_gain sound_gain;
            sound_gain.linear_mode = HI_TRUE;

            sound_gain.gain = atoi(input_cmd + 1);
            if (sound_gain.gain > 100) {
                sound_gain.gain = 100;
            }

            printf("Volume=%d\n", sound_gain.gain);
            hi_unf_snd_set_volume(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_ALL, &sound_gain);
        }

        if ('s' == input_cmd[0] || 'S' == input_cmd[0]) {
            static int spdif_toggle =0;
            spdif_toggle++;
            if(spdif_toggle & 1) {
                hi_unf_snd_set_spdif_scms_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0,  HI_UNF_SND_OUTPUT_MODE_RAW);
                 printf("spdif pass-through on!\n");
             } else {
                hi_unf_snd_set_spdif_scms_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0, HI_UNF_SND_OUTPUT_MODE_LPCM);
                printf("spdif pass-through off!\n");
            }
            continue;
        }

        if ('h' == input_cmd[0] || 'H' == input_cmd[0]) {
            static int hdmi_toggle =0;
            hdmi_toggle++;
            if(hdmi_toggle & 1) {
                hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_RAW);
                 printf("hmdi pass-through on!\n");
             } else {
                hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_LPCM);
                printf("hmdi pass-through off!\n");
            }
            continue;
        }

        if ('t' == input_cmd[0]) {
            hi_unf_avplay_tplay_opt tplay_opt;
            printf("%dx tplay\n",tplay_speed);

            tplay_opt.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
            tplay_opt.speed_integer = tplay_speed;
            tplay_opt.speed_decimal = 0;

            hi_unf_avplay_tplay(avplay,&tplay_opt);
            tplay_speed = (32 == tplay_speed * 2) ? (2) : (tplay_speed * 2);
            continue;
        }

        if ('r' == input_cmd[0]) {
            printf("resume\n");
            hi_unf_avplay_resume(avplay,HI_NULL);
            tplay_speed = 2;
            continue;
        }
    }

AVPLAY_STOP:
    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop_opt.timeout = 0;
    hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);

//PSISI_FREE:
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


