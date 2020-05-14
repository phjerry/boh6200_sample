/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: aenc sample
 * Author: audio
 * Create: 2019-09-17
 * Notes:  NA
 * History: 2019-09-17 Initial version for Hi3796CV300
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_sound.h"

#include "hi_unf_aenc.h"
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_aacenc.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_amrwb.h"
#include "hi_unf_acodec_truehdpassthrough.h"
#include "hi_unf_acodec_dtshddec.h"
#include "hi_unf_acodec_ac3passthrough.h"
#include "hi_unf_acodec_dtspassthrough.h"
#include "hi_adp_mpi.h"
#include "hi_errno.h"

#if defined (DOLBYPLUS_HACODEC_SUPPORT)
#include "hi_unf_acodec_dolbyplusdec.h"
#endif

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif
FILE *g_audio_es_file;
FILE *g_aenc_file;

static pthread_t g_es_thread;
static hi_handle g_avplay;
hi_s32 g_audio_es_file_offset;

static hi_bool g_stop_thread = HI_FALSE;
static hi_bool g_stop_aenc_thread = HI_FALSE;
volatile hi_bool g_save_stream = HI_TRUE;

typedef struct {
    pthread_t thread_send_es;
    hi_handle h_aenc;
    hi_unf_aenc_attr aenc_attr;
} audio_aenc;

static audio_aenc g_audio_aenc;

#define AENC_THREAD_SLEEP_TIME (5 * 1000)     /* 5ms */
#define AENC_SAVE_SLEEP_TIME (10 * 1000)     /* 10ms */
#define INPUT_CMD_LENGTH 32

hi_void *aenc_thread(hi_void *args)
{
    hi_handle h_aenc;
    hi_s32 ret;
    audio_aenc *aenc = (audio_aenc *)args;
    h_aenc = aenc->h_aenc;

    while (!g_stop_aenc_thread) {
        /* save audio stream */
        if (g_save_stream) {
            hi_bool got_stream = HI_FALSE;

            hi_unf_es_buf aenc_stream;

            ret = hi_unf_aenc_acquire_stream(h_aenc, &aenc_stream, 0);
            if (ret == HI_SUCCESS) {
                fwrite(aenc_stream.buf, 1, aenc_stream.buf_len, g_aenc_file);
                hi_unf_aenc_release_stream(h_aenc, &aenc_stream);
                got_stream = HI_TRUE;
            } else if (ret != HI_ERR_AENC_OUT_BUF_EMPTY) {
                printf("hi_unf_aenc_acquire_stream failed:%#x\n", ret);
            }

            if (got_stream == HI_FALSE) {
                usleep(AENC_SAVE_SLEEP_TIME);
            }
        } else {
            usleep(AENC_THREAD_SLEEP_TIME);
        }
    }

    fclose(g_aenc_file);
    return HI_NULL;
}

hi_void audio_thread(hi_void *args)
{
    hi_unf_stream_buf stream_buf;
    hi_u32 read_len;
    hi_s32 ret;
    hi_bool audio_buf_avail;

    audio_buf_avail = HI_FALSE;

    while (!g_stop_thread) {
        ret = hi_unf_avplay_get_buf(g_avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, 0x1000, &stream_buf, 0);
        if (ret == HI_SUCCESS) {
            audio_buf_avail = HI_TRUE;

            read_len = fread(stream_buf.data, sizeof(hi_s8), 0x1000, g_audio_es_file);
            if (read_len > 0) {
                ret = hi_unf_avplay_put_buf(g_avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, read_len, 0, HI_NULL);
                if (ret != HI_SUCCESS) {
                    sample_printf("call hi_unf_avplay_put_buf failed.\n");
                }
            } else if (read_len <= 0) {
                sample_printf("read aud file error!\n");
                rewind(g_audio_es_file);
                if (g_audio_es_file_offset) {
                    fseek(g_audio_es_file, g_audio_es_file_offset, SEEK_SET);
                }
            }
        } else if (ret != HI_SUCCESS) {
            audio_buf_avail = HI_FALSE;
        }

        /* wait for buffer */
        if (audio_buf_avail == HI_FALSE) {
            usleep(AENC_SAVE_SLEEP_TIME);
        }
    }

    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_handle h_track;
    hi_handle h_vir_track;
    hi_unf_audio_track_attr track_attr;
    hi_unf_snd_attr attr;

    hi_u32 adec_type;
    hi_u32 aenc_type;
    hi_unf_avplay_attr avplay_attr;
    hi_unf_sync_attr av_sync_attr;
    hi_unf_avplay_stop_opt stop;
    hi_char input_cmd[INPUT_CMD_LENGTH];
    hi_unf_video_format default_fmt = HI_UNF_VIDEO_FMT_720P_50;

    if (argc != 5) {
        sample_printf(" usage:     %s infile intype outfile outtype\n", argv[0]);
        sample_printf(" type:      aac/mp3/dts/dra/mlp/pcm/ddp(ac3/eac3)\n");
        sample_printf(" examples: \n");
        sample_printf("            %s infile mp3 outfile aac\n", argv[0]);
        return -1;
    }

    g_audio_es_file = fopen(argv[1], "rb");
    if (!g_audio_es_file) {
        sample_printf("open file %s error!\n", argv[1]);
        return -1;
    }

    if (!strcasecmp("aac", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_AAC;
    } else if (!strcasecmp("mp3", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_MP3;
    } else if (!strcasecmp("ac3raw", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_AC3PASSTHROUGH;
    } else if (!strcasecmp("dtsraw", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_DTSPASSTHROUGH;
    }
#if defined (DOLBYPLUS_HACODEC_SUPPORT)
    else if (!strcasecmp("ddp", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_DOLBY_PLUS;
    }
#endif
    else if (!strcasecmp("dts", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_DTSHD;
    } else if (!strcasecmp("dra", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_DRA;
    } else if (!strcasecmp("pcm", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_PCM;
    } else if (!strcasecmp("mlp", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_TRUEHD;
    } else if (!strcasecmp("amr", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_AMRNB;
    } else if (!strcasecmp("amrwb", argv[2])) {
        adec_type = HI_UNF_ACODEC_ID_AMRWB;

        /* read header of file for MIME file format */
        extern hi_u8 dec_open_buf[1024];
        hi_unf_acodec_amrwb_decode_open_config *config = (hi_unf_acodec_amrwb_decode_open_config *)dec_open_buf;
        if (config->format == HI_UNF_ACODEC_AMRWB_FORMAT_MIME) {
            hi_u8 magic_buf[8];

            /* just need by amr file storage format (xxx.amr) */
            fread(magic_buf, sizeof(hi_s8), strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER), g_audio_es_file);
            if (strncmp((const char *)magic_buf, HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER, strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER))) {
                sample_printf("%s invalid amr wb file magic number ", magic_buf);
                return HI_FAILURE;
            }
        }

        g_audio_es_file_offset = strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER);
    } else {
        sample_printf("unsupport aud codec type!\n");
        return -1;
    }

    g_aenc_file = fopen(argv[3], "wb");
    if (!g_aenc_file) {
        sample_printf("open file %s error!\n", argv[3]);
        return -1;
    }

    if (!strcasecmp("aac", argv[4])) {
        aenc_type = HI_UNF_ACODEC_ID_AAC;
    } else {
        sample_printf("unsupport aud encode type!\n");
        return -1;
    }

    hi_unf_sys_init();
    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto sys_deinit;
    }

    ret = hi_unf_snd_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_init failed.\n");
        goto avplay_deinit;
    }

    ret = hi_unf_aenc_init();
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_aenc_init failed.\n");
        goto snd_deinit;
    }

    hi_unf_snd_get_default_open_attr(HI_UNF_SND_0, &attr);
    ret = hi_unf_snd_open(HI_UNF_SND_0, &attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_open failed.\n");
        goto aenc_deinit;
    }

    ret  = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_ES);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_get_default_config failed.\n");
        goto snd_close;
    }

    ret = hi_unf_avplay_create(&avplay_attr, &g_avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto snd_close;
    }
    ret = hi_unf_avplay_get_attr(g_avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &av_sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_get_attr failed.\n");
        goto avplay_destroy;
    }

    av_sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;

    ret = hi_unf_avplay_set_attr(g_avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &av_sync_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto avplay_destroy;
    }

    ret = hi_unf_avplay_chan_open(g_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_chn_open failed.\n");
        goto avplay_destroy;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto achn_close;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &h_track);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto achn_close;
    }

    ret = hi_unf_snd_attach(h_track, g_avplay);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_attach failed.\n");
        goto track_destroy;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &track_attr);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        goto achn_close;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &h_vir_track);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_snd_create_track failed.\n");
        goto snd_stop;
    }

    hi_unf_aenc_register_encoder("libHA.AUDIO.AAC.encode.so");

    g_audio_aenc.aenc_attr.aenc_type = aenc_type;
    hi_unf_acodec_aac_enc_config private_config;
    hi_unf_acodec_aac_get_default_config(&private_config);

    if (g_audio_aenc.aenc_attr.aenc_type == HI_UNF_ACODEC_ID_AAC) {
        hi_unf_acodec_aac_get_enc_default_open_param(&g_audio_aenc.aenc_attr.param, (hi_void *)&private_config);
        sample_printf("use aac encode\n");
    }

    ret = hi_unf_aenc_create(&(g_audio_aenc.aenc_attr), &(g_audio_aenc.h_aenc));

    ret = hi_unf_aenc_attach_input(g_audio_aenc.h_aenc, h_vir_track);

    ret = hi_unf_snd_attach(h_vir_track, g_avplay);

    ret = hi_adp_avplay_set_adec_attr(g_avplay, adec_type);
    if (ret != HI_SUCCESS) {
        sample_printf("call hiadp_av_play_set_adec_attr failed.\n");
        goto aenc_stop;
    }

    ret = hi_unf_avplay_start(g_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_start failed.\n");
    }

    ret = hi_adp_disp_init(default_fmt);
    if (ret != HI_SUCCESS) {
        sample_printf("call disp_init failed.\n");
        goto avplay_stop;
    }

    g_stop_thread = HI_FALSE;
    pthread_create(&g_es_thread, HI_NULL, (hi_void *)audio_thread, (hi_void *)HI_NULL);
    g_save_stream = HI_TRUE;
    g_stop_aenc_thread = HI_FALSE;
    pthread_create(&(g_audio_aenc.thread_send_es), HI_NULL, (hi_void *)aenc_thread, (hi_void *)&(g_audio_aenc));

    while (1) {
        sample_printf("\ninput q to quit!\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            sample_printf("prepare to quit!\n");
            break;
        }
    }

    g_stop_aenc_thread = HI_TRUE;
    pthread_join(g_audio_aenc.thread_send_es, HI_NULL);

    g_stop_thread = HI_TRUE;
    pthread_join(g_es_thread, HI_NULL);

    hi_adp_disp_deinit();

avplay_stop:
    stop.timeout = 0;
    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    hi_unf_avplay_stop(g_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);

aenc_stop:
    hi_unf_snd_detach(h_vir_track, g_avplay);
    hi_unf_aenc_detach_input(g_audio_aenc.h_aenc);
    hi_unf_aenc_destroy(g_audio_aenc.h_aenc);
    hi_unf_snd_destroy_track(h_vir_track);

snd_stop:
    hi_unf_snd_detach(h_track, g_avplay);

track_destroy:
    hi_unf_snd_destroy_track(h_track);

achn_close:
    hi_unf_avplay_chan_close(g_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);

avplay_destroy:
    hi_unf_avplay_destroy(g_avplay);

snd_close:
    hi_unf_snd_close(HI_UNF_SND_0);

aenc_deinit:
    hi_unf_aenc_deinit();

snd_deinit:
    hi_unf_snd_deinit();

avplay_deinit:
    hi_unf_avplay_deinit();

sys_deinit:
    hi_unf_sys_deinit();

    fclose(g_audio_es_file);
    g_audio_es_file = HI_NULL;

    return 0;
}
