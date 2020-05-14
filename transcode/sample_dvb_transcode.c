/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : dvb_transcode.c
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

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_errno.h"
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_aacenc.h"
#include "hi_unf_aenc.h"
#include "hi_unf_venc.h"
#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"

hi_u32 g_tuner_freq;
hi_u32 g_tuner_srate;
hi_u32 g_third_para;
static hi_unf_video_format g_default_fmt = HI_UNF_VIDEO_FMT_1080P_50;

pmt_compact_tbl    *g_prog_table = HI_NULL;


volatile hi_bool g_stop_save_thread = HI_TRUE;
volatile hi_bool g_save_stream = HI_FALSE;

#define VID_FILENAME_PATH  "./vid.h264"
#define AUD_FILENAME_PATH  "./aud.aac"
#define VID_FILE_MAX_LEN   (20 * 1024 * 1024)

typedef struct
{
    hi_handle venc;
    hi_handle aenc;
} thread_args;

hi_void* SaveThread(hi_void *pArgs)
{
    thread_args *thread_para = (thread_args*)pArgs;
    hi_handle venc_chn;
    hi_handle aenc_chn;
    FILE *vid_file = HI_NULL;
    FILE *aud_file = HI_NULL;
    hi_s32 ret;

    venc_chn = thread_para->venc;
    aenc_chn = thread_para->aenc;
    vid_file = fopen(VID_FILENAME_PATH, "w");
    if (vid_file == HI_NULL)
    {
        printf("open file %s failed\n", VID_FILENAME_PATH);
        return HI_NULL;
    }

    aud_file = fopen(AUD_FILENAME_PATH, "w");
    if (aud_file == HI_NULL)
    {
        printf("open file %s failed\n", AUD_FILENAME_PATH);
        fclose(vid_file);
        return HI_NULL;
    }

    while (!g_stop_save_thread)
    {
        hi_bool bGotStream = HI_FALSE;

        if (g_save_stream)
        {
            hi_unf_venc_stream venc_stream;
            hi_unf_es_buf aenc_stream;
#ifdef FILE_LEN_LIMIT
            if (ftell(vid_file) >= VID_FILE_MAX_LEN)
            {
                fclose(vid_file);
                fclose(aud_file);
                vid_file = fopen(VID_FILENAME_PATH, "w");
                aud_file = fopen(AUD_FILENAME_PATH, "w");
                printf("stream files are truncated to zero\n");
            }
#endif

            /*save video stream*/
            ret = hi_unf_venc_acquire_stream(venc_chn, &venc_stream, 0);
            if (ret == HI_SUCCESS)
            {
                fwrite(venc_stream.virt_addr, 1, venc_stream.slc_len, vid_file);
                hi_unf_venc_release_stream(venc_chn, &venc_stream);
                fflush(vid_file);
                bGotStream = HI_TRUE;
            }
            else if (ret != HI_ERR_VENC_BUF_EMPTY)
            {
                printf("hi_unf_venc_acquire_stream failed:%#x\n",ret);
            }

            /*save audio stream*/
            ret = hi_unf_aenc_acquire_stream(aenc_chn, &aenc_stream, 0);
            if (ret == HI_SUCCESS)
            {
                fwrite(aenc_stream.buf, 1, aenc_stream.buf_len, aud_file);
                fflush(aud_file);
                hi_unf_aenc_release_stream(aenc_chn, &aenc_stream);
                bGotStream = HI_TRUE;
            }
            else if (ret != HI_ERR_AENC_OUT_BUF_EMPTY)
            {
                printf("hi_unf_aenc_acquire_stream failed:%#x\n", ret);
            }
        }

        if ( HI_FALSE == bGotStream )
        {
            usleep(10 * 1000);
        }
    }

    fclose(vid_file);
    fclose(aud_file);
    return HI_NULL;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32    ret;
    hi_u32    prog_num;
    hi_char   input_cmd[32];
    hi_handle win;
    hi_handle enc_win;
    hi_handle avplay;
    hi_handle track;
    hi_handle enc_track;
    hi_handle aenc;
    hi_handle venc;
    hi_unf_avplay_attr      avplay_attr;
    hi_unf_sync_attr        sync_attr;
    hi_unf_avplay_stop_opt  stop;
    hi_unf_audio_track_attr track_attr;
    hi_unf_win_attr         virtul_win_attr;
    hi_unf_venc_attr        venc_chan_attr;
    hi_unf_acodec_aac_enc_config  private_config;
    hi_unf_aenc_attr        aenc_attr;
    thread_args             save_args;
    pthread_t               thdSave;


    if (5 == argc)
    {
        g_tuner_freq  = strtol(argv[1], NULL, 0);
        g_tuner_srate = strtol(argv[2], NULL, 0);
        g_third_para = strtol(argv[3], NULL, 0);
        g_default_fmt = hi_adp_str2fmt(argv[4]);
    }
    else if (4 == argc)
    {
        g_tuner_freq  = strtol(argv[1], NULL, 0);
        g_tuner_srate = strtol(argv[2], NULL, 0);
        g_third_para = strtol(argv[3], NULL, 0);
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    }
    else if (3 == argc)
    {
        g_tuner_freq  = strtol(argv[1], NULL, 0);
        g_tuner_srate = strtol(argv[2], NULL, 0);
        g_third_para = (g_tuner_freq > 1000) ? 0 : 64;
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    }
    else if (2 == argc)
    {
        g_tuner_freq  = strtol(argv[1], NULL, 0);
        g_tuner_srate = (g_tuner_freq > 1000) ? 27500 : 6875;
        g_third_para = (g_tuner_freq > 1000) ? 0 : 64;
        g_default_fmt = HI_UNF_VIDEO_FMT_1080P_50;
    }
    else
    {
        printf("Usage: %s freq [srate] [qamtype or polarization] [vo_format]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       vo_format:2160P30|2160P24|1080P60|1080P50|1080i60|[1080i50]|720P60|720P50  default HI_UNF_VIDEO_FMT_1080P_50\n",
                argv[0]);
        printf("Example: %s 610 6875 64 1080P50\n", argv[0]);
        return HI_FAILURE;
    }

    hi_unf_sys_init();
    //hi_adp_mce_exit();
    ret = hi_adp_snd_init();
    if (HI_SUCCESS != ret)
    {
        printf("call hi_adp_snd_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(g_default_fmt);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_adp_disp_deinit failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_win_init();
    ret |= hi_adp_win_create(HI_NULL, &win);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_adp_win_init failed.\n");
        hi_adp_win_deinit();
        goto DISP_DEINIT;
    }

    ret = hi_unf_dmx_init();
    ret |= hi_adp_dmx_attach_ts_port(0, TUNER_USE);
    if (HI_SUCCESS != ret)
    {
        printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_adp_frontend_init();
    if (HI_SUCCESS != ret)
    {
        printf("call HIADP_Demux_Init failed.\n");
        goto DMX_DEINIT;
    }

    ret = hi_adp_frontend_connect(TUNER_USE, g_tuner_freq, g_tuner_srate, g_third_para);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_adp_frontend_connect failed.\n");
        goto TUNER_DEINIT;
    }

    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(0, &g_prog_table);
    if (HI_SUCCESS != ret)
    {
        printf("call HIADP_Search_GetAllPmt failed\n");
        goto PSISI_FREE;
    }

    ret  = hi_adp_avplay_init();
    if (ret != HI_SUCCESS)
    {
        printf("call hi_adp_avplay_init failed.\n");
        goto PSISI_FREE;
    }

    ret  = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    avplay_attr.vid_buf_size = 0x300000;
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret  = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    ret |= hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_avplay_chan_open failed.\n");
        goto CHN_CLOSE;
    }

    ret = hi_unf_win_attach_src(win, avplay);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_win_attach_src failed.\n");
        goto CHN_CLOSE;
    }

    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_win_set_enable failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto WIN_DETACH;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_create_track failed.\n");
        goto WIN_DETACH;
    }

    ret = hi_unf_snd_attach(track, avplay);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_snd_attach failed.\n");
        goto TRACK_DESTROY;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    sync_attr.start_region.vid_plus_time = 60;
    sync_attr.start_region.vid_negative_time = -20;
    sync_attr.quick_output_enable = HI_FALSE;
    ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS)
    {
        printf("call hi_unf_avplay_set_attr failed.\n");
        goto SND_DETACH;
    }

    /*create virtual window */
    virtul_win_attr.disp_id = HI_UNF_DISPLAY0;
    virtul_win_attr.priority = HI_UNF_WIN_WIN_PRIORITY_AUTO;
    virtul_win_attr.is_virtual = HI_TRUE;
    virtul_win_attr.asp_convert_mode = HI_UNF_WIN_ASPECT_CONVERT_FULL;
    virtul_win_attr.video_format = HI_UNF_FORMAT_YUV_SEMIPLANAR_420_UV;

    memset(&virtul_win_attr.video_rect, 0, sizeof(hi_unf_video_rect));
    memset(&virtul_win_attr.src_crop_rect, 0, sizeof(hi_unf_video_crop_rect));
    memset(&virtul_win_attr.output_rect, 0, sizeof(hi_unf_video_crop_rect));
    hi_unf_win_create(&virtul_win_attr, &enc_win);

    /*create video encoder*/
    hi_unf_venc_init();
    hi_unf_venc_get_default_attr(&venc_chan_attr);
    venc_chan_attr.venc_type = HI_UNF_VCODEC_TYPE_H264;
    venc_chan_attr.max_width = 1280;
    venc_chan_attr.max_height = 720;
    venc_chan_attr.stream_buf_size   = 1280 * 720 * 2;
    venc_chan_attr.config.target_bitrate = 4 * 1024 * 1024;
    venc_chan_attr.config.input_frame_rate  = 50;
    venc_chan_attr.config.target_frame_rate = 25;
    venc_chan_attr.config.width = 1280;
    venc_chan_attr.config.height = 720;
    hi_unf_venc_create(&venc, &venc_chan_attr);
    hi_unf_venc_attach_input(venc, enc_win);

    /*create virtual track */
    hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &track_attr);
    track_attr.output_buf_size = 256 * 1024;
    hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &enc_track);

    /*create aenc*/
    hi_unf_aenc_init();
    hi_unf_aenc_register_encoder("libHA.AUDIO.AAC.encode.so");

    /*create audio decoder*/
    aenc_attr.aenc_type = HI_UNF_ACODEC_ID_AAC;
    hi_unf_acodec_aac_get_default_config(&private_config);
    hi_unf_acodec_aac_get_enc_default_open_param(&(aenc_attr.param), (hi_void *)&private_config);
    hi_unf_aenc_create(&aenc_attr, &aenc);
    /*attach audio decoder and virtual track*/
    hi_unf_aenc_attach_input(aenc, enc_track);

    g_stop_save_thread = HI_FALSE;
    save_args.venc = venc;
    save_args.aenc = aenc;
    pthread_create(&thdSave, HI_NULL, (hi_void *)SaveThread, &save_args);

    prog_num = 0;
    ret = hi_adp_avplay_play_prog(avplay, g_prog_table, prog_num, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS)
    {
        printf("call SwitchProg failed.\n");
        goto AVPLAY_STOP;
    }

    printf("please input q to quit\n" \
           "please input r before input s\n" \
           "       input r to start transcode\n" \
           "       input s to stop transcode\n" \
           "       input 0~9 to change channel\n"\
           "       input h to print this message\n");
    while (1)
    {
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        if ('h' == input_cmd[0])
        {
            printf("please input q to quit\n" \
                   "please input r before input s\n" \
                   "       input r to start transcode\n" \
                   "       input s to stop transcode\n" \
                   "       input 0~9 to change channel\n"\
                   "       input h to print this message\n");
            continue;
        }

        if ('r' == input_cmd[0])
        {
            hi_unf_venc_start(venc);
            hi_unf_snd_attach(enc_track, avplay);
            hi_unf_win_attach_src(enc_win, avplay);
            hi_unf_win_set_enable(enc_win, HI_TRUE);
            g_save_stream = HI_TRUE;
            printf("start transcode\n");
            continue;
        }

        if ('s' == input_cmd[0])
        {
            hi_unf_win_set_enable(enc_win, HI_FALSE);
            hi_unf_venc_stop(venc);
            g_save_stream = HI_FALSE;
            hi_unf_snd_detach(enc_track, avplay);
            hi_unf_win_detach_src(enc_win, avplay);
            printf("stop transcode\n");
            continue;
        }

        if (g_save_stream == HI_TRUE)
        {
            g_save_stream = HI_FALSE;
            hi_unf_win_set_enable(enc_win, HI_FALSE);
            hi_unf_venc_stop(venc);
            hi_unf_snd_detach(enc_track, avplay);
            hi_unf_win_detach_src(enc_win, avplay);
            printf("stop transcode\n");
        }

        prog_num = atoi(input_cmd);

        if (prog_num == 0)
        {
            prog_num = 1;
        }

        ret = hi_adp_avplay_play_prog(avplay, g_prog_table, prog_num - 1, HI_UNF_AVPLAY_MEDIA_CHAN_VID|HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        if (ret != HI_SUCCESS)
        {
            printf("call SwitchProgfailed!\n");
            break;
        }
    }

AVPLAY_STOP:
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

//ENC_DESTROY:
    g_stop_save_thread = HI_TRUE;
    pthread_join(thdSave,HI_NULL);

    if (g_save_stream)
    {
        hi_unf_snd_detach(enc_track, avplay);
    }

    hi_unf_aenc_detach_input(aenc);
    hi_unf_aenc_destroy(aenc);
    hi_unf_snd_destroy_track(enc_track);

    hi_unf_win_set_enable(enc_win, HI_FALSE);

    if (g_save_stream)
    {
        hi_unf_venc_stop(venc);
        hi_unf_win_detach_src(enc_win, avplay);
    }

    hi_unf_venc_detach_input(venc);
    hi_unf_venc_destroy(venc);
    hi_unf_win_destroy(enc_win);

SND_DETACH:
    hi_unf_snd_detach(track, avplay);

TRACK_DESTROY:
    hi_unf_snd_destroy_track(track);

WIN_DETACH:
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, avplay);

CHN_CLOSE:
    hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

PSISI_FREE:
    hi_adp_search_free_all_pmt(g_prog_table);
    g_prog_table = HI_NULL;
    hi_adp_search_de_init();

TUNER_DEINIT:
    hi_adp_frontend_deinit();

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

    return ret;
}


