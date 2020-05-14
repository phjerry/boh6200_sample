/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : esplay.c
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
#include "hi_unf_acodec.h"

#include "hi_unf_acodec_amrwb.h"
#include "hi_adp_mpi.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define DO_FUNC(func) do { \
    hi_s32 _ret = func; \
    if (_ret != HI_SUCCESS) { \
        sample_printf("Do [%s] failed, errorcode = 0x%x\n", #func, _ret); \
    } \
} while (0)

#define MAX_READ_FRAME_SIZE     (0x200000)
#define DEFAULT_READ_FRAME_SIZE (0x100000)

static FILE* g_vid_es_file = HI_NULL;
static FILE* g_aud_es_file = HI_NULL;

static pthread_t g_es_thread;
static hi_bool g_stop_thread = HI_FALSE;
static hi_bool g_aud_play = HI_FALSE;
static hi_bool g_vid_play = HI_FALSE;
static hi_bool g_play_once = HI_FALSE;

static hi_bool g_is_read_frame_size = HI_FALSE;
static hi_unf_vcodec_type g_vdec_type = HI_UNF_VCODEC_TYPE_MAX;

static hi_s32  g_aud_esfile_offset;

typedef struct {
    const hi_char *type;
    hi_unf_vcodec_type vdec_type;
    hi_bool is_read_frame_size;
} vdec_type_map;

typedef struct {
    const hi_char *type;
    hi_u32 adec_type;
} adec_type_map;

static vdec_type_map g_vdec_type_map[] = {
    {"mpeg2",    HI_UNF_VCODEC_TYPE_MPEG2,    HI_FALSE},
    {"mpeg4",    HI_UNF_VCODEC_TYPE_MPEG4,    HI_FALSE},
    {"h263",     HI_UNF_VCODEC_TYPE_H263,     HI_FALSE},
    {"sor",      HI_UNF_VCODEC_TYPE_SORENSON, HI_FALSE},
    {"vp6",      HI_UNF_VCODEC_TYPE_VP6,      HI_TRUE},
    {"vp6f",     HI_UNF_VCODEC_TYPE_VP6F,     HI_TRUE},
    {"vp6a",     HI_UNF_VCODEC_TYPE_VP6A,     HI_TRUE},
    {"h264",     HI_UNF_VCODEC_TYPE_H264,     HI_FALSE},
    {"h265",     HI_UNF_VCODEC_TYPE_H265,     HI_FALSE},
    {"mvc",      HI_UNF_VCODEC_TYPE_H264_MVC, HI_FALSE},
    {"avs",      HI_UNF_VCODEC_TYPE_AVS,      HI_FALSE},
    {"avs2",     HI_UNF_VCODEC_TYPE_AVS2,     HI_FALSE},
    {"avs3",     HI_UNF_VCODEC_TYPE_AVS3,     HI_FALSE},
    {"real8",    HI_UNF_VCODEC_TYPE_REAL8,    HI_TRUE},
    {"real9",    HI_UNF_VCODEC_TYPE_REAL9,    HI_TRUE},
    {"vc1ap",    HI_UNF_VCODEC_TYPE_VC1,      HI_FALSE},
    {"vc1smp5",  HI_UNF_VCODEC_TYPE_VC1,      HI_TRUE},
    {"vc1smp8",  HI_UNF_VCODEC_TYPE_VC1,      HI_TRUE},
    {"vp8",      HI_UNF_VCODEC_TYPE_VP8,      HI_TRUE},
    {"vp9",      HI_UNF_VCODEC_TYPE_VP9,      HI_TRUE},
    {"av1",      HI_UNF_VCODEC_TYPE_AV1,      HI_TRUE},
    {"divx3",    HI_UNF_VCODEC_TYPE_DIVX3,    HI_TRUE},
    {"mjpeg",    HI_UNF_VCODEC_TYPE_MJPEG,    HI_TRUE},
};

static adec_type_map g_adec_type_map[] = {
    {"aac",     HI_UNF_ACODEC_ID_AAC},
    {"mp3",     HI_UNF_ACODEC_ID_MP3},
    {"truehd",  HI_UNF_ACODEC_ID_DOLBY_TRUEHD},
    {"ac3raw",  HI_UNF_ACODEC_ID_AC3PASSTHROUGH},
    {"dtsraw",  HI_UNF_ACODEC_ID_DTSPASSTHROUGH},
#if defined (DOLBYPLUS_HACODEC_SUPPORT)
    {"ddp",     HI_UNF_ACODEC_ID_DOLBY_PLUS},
#endif
    {"dts",     HI_UNF_ACODEC_ID_DTSHD},
    {"dtsm6",   HI_UNF_ACODEC_ID_DTSM6},
    {"dra",     HI_UNF_ACODEC_ID_DRA},
    {"pcm",     HI_UNF_ACODEC_ID_PCM},
    {"mlp",     HI_UNF_ACODEC_ID_TRUEHD},
    {"amr",     HI_UNF_ACODEC_ID_AMRNB},
    {"g711",    HI_UNF_ACODEC_ID_G711},
    {"g726",    HI_UNF_ACODEC_ID_G726},
    {"vorbis",  HI_UNF_ACODEC_ID_VORBIS},
    {"opus",    HI_UNF_ACODEC_ID_OPUS},
    {"adpcm",   HI_UNF_ACODEC_ID_ADPCM},
    {"amrwb",   HI_UNF_ACODEC_ID_AMRWB},
};

hi_s32 get_framesize_by_vid_type(hi_u32 *frame_size, hi_unf_vcodec_type vid_type, FILE *fp)
{
    hi_u32         read_len;
    hi_u32         frame_size_tmp = 0;
    hi_u8          vp9_file_hdr[32];
    hi_u8          vp9_frm_hdr[12];
    static hi_bool is_vp9_parse_file_hdr = HI_FALSE;
    const hi_char  ivf_signature[] = "DKIF";

    switch (vid_type) {
        case HI_UNF_VCODEC_TYPE_VP9:
        case HI_UNF_VCODEC_TYPE_AV1: {
            if (!is_vp9_parse_file_hdr) {
                if (fread(vp9_file_hdr, 1, 32, fp) != 32) {
                    sample_printf("ERR: read VP9 file header failed.\n");
                    break;
                }

                if (memcmp(ivf_signature, vp9_file_hdr, 4) != 0) {
                    sample_printf("ERR: VP9 file is not IVF file.\n");
                    break;
                }

                is_vp9_parse_file_hdr = HI_TRUE;
            }

            if (fread(vp9_frm_hdr, 1, 12, fp) == 12) {
                frame_size_tmp = (vp9_frm_hdr[3] << 24) + (vp9_frm_hdr[2] << 16) + (vp9_frm_hdr[1] << 8) + vp9_frm_hdr[0];
            } else { /* read vp9 file end*/
                frame_size_tmp = 0;
                is_vp9_parse_file_hdr = HI_FALSE;
                *frame_size = frame_size_tmp;
                return HI_SUCCESS;
            }

            break;
        }

        default: {
            if (g_is_read_frame_size) {
                read_len = fread(&frame_size_tmp, 1, 4, fp);
                frame_size_tmp = (read_len == 4) ? frame_size_tmp : DEFAULT_READ_FRAME_SIZE;
            } else {
                frame_size_tmp = DEFAULT_READ_FRAME_SIZE;
            }

            break;
        }
    }

    *frame_size = frame_size_tmp;

    return frame_size_tmp == 0 ? HI_FAILURE : HI_SUCCESS;
}

static hi_void process_rewind(hi_handle avplay, hi_bool is_vid)
{
    hi_s32 ret = HI_SUCCESS;

    if (g_play_once) {
         hi_bool is_empty = HI_FALSE;
         hi_unf_avplay_flush_stream_opt flush_opt;

         sample_printf("Esplay flush stream.\n");
         hi_unf_avplay_flush_stream(avplay, &flush_opt);

         while (!g_stop_thread) {
             ret = hi_unf_avplay_is_eos(avplay, &is_empty);
             if (ret != HI_SUCCESS) {
                 sample_printf("call hi_unf_avplay_is_eos failed.\n");
                 break;
             } else if (is_empty) {
                 break;
             } else {
                usleep(100 * 1000);
             }
         }

         sleep(1);
         sample_printf("Finish, esplay exit!\n");
         exit(0);
     } else {
         sample_printf("read file end and rewind!\n");
         rewind(is_vid ? g_vid_es_file : g_aud_es_file);

         if (g_aud_esfile_offset) {
            fseek(g_aud_es_file, g_aud_esfile_offset, SEEK_SET);
         }
     }
}

hi_void* es_thread(hi_void* args)
{
    hi_handle            avplay = *((hi_handle*)args);
    hi_u32               frame_size = 0;
    hi_unf_stream_buf    stream_buf;
    hi_u32               read_len;
    hi_s32               ret;
    hi_bool              is_vid_buf_avail = HI_FALSE;
    hi_bool              is_aud_buf_avail = HI_FALSE;

    while (!g_stop_thread) {
        if (g_vid_play) {
            if (frame_size == 0) {
                ret = get_framesize_by_vid_type(&frame_size, g_vdec_type, g_vid_es_file);
                if (ret != HI_SUCCESS) {
                    printf("get frame size error\n");
                    break;
                } else if (frame_size == 0) {
                    process_rewind(avplay, HI_TRUE);
                    continue;
                }
            }

            ret = hi_unf_avplay_get_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, frame_size, &stream_buf, 0);
            if (ret == HI_SUCCESS) {
                is_vid_buf_avail = HI_TRUE;

                read_len = fread(stream_buf.data, sizeof(hi_s8), frame_size, g_vid_es_file);
                if (read_len > 0) {
                    ret = hi_unf_avplay_put_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, read_len, 0, HI_NULL);
                    if (ret != HI_SUCCESS) {
                        sample_printf("call hi_unf_avplay_put_buf failed, ret = 0x%x.\n", ret);
                    }
                } else if (read_len == 0) {
                    process_rewind(avplay, HI_TRUE);
                } else {
                    perror("read vid file error\n");
                }

                frame_size = 0;
            } else {
                is_vid_buf_avail = HI_FALSE;
            }
        }

        if (g_aud_play) {
            ret = hi_unf_avplay_get_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, 0x1000, &stream_buf, 0);

            if (HI_SUCCESS == ret) {
                is_aud_buf_avail = HI_TRUE;
                read_len = fread(stream_buf.data, sizeof(hi_s8), 0x1000, g_aud_es_file);

                if (read_len > 0) {
                    ret = hi_unf_avplay_put_buf(avplay, HI_UNF_AVPLAY_BUF_ID_ES_AUD, read_len, 0, HI_NULL);
                    if (ret != HI_SUCCESS) {
                        sample_printf("call hi_unf_avplay_put_buf failed.\n");
                    }
                } else if (read_len == 0) {
                    process_rewind(avplay, HI_FALSE);
                } else {
                    perror("read aud file error\n");
                }
            } else if (ret != HI_SUCCESS) {
                is_aud_buf_avail = HI_FALSE;
            }
        }

        /* wait for buffer */
        if ((HI_FALSE == is_aud_buf_avail) &&
            (HI_FALSE == is_vid_buf_avail)) {
            usleep(1000 * 10);
        }
    }

    return HI_NULL;
}

hi_s32 get_vdec_type(hi_char *type, hi_unf_vcodec_type *vdec_type, hi_u32 *private1, hi_u32 *private2)
{
    hi_u32 i = 0;
    hi_u32 num = sizeof(g_vdec_type_map) / sizeof(vdec_type_map);

    if (type == HI_NULL) {
        return HI_FAILURE;
    }

    for (; i < num; i++) {
        if (!strcasecmp(type, g_vdec_type_map[i].type)) {
            break;
        }
    }

    if (i >= num) {
        return HI_FAILURE;
    }

    *vdec_type = g_vdec_type_map[i].vdec_type;
    g_is_read_frame_size = g_vdec_type_map[i].is_read_frame_size;

    if ((private1 != HI_NULL) && (private2 != HI_NULL)) {
        if (!strcasecmp(type, "vc1ap")) {
            *private1 = 1;
            *private2 = 8;
        }

        if (!strcasecmp(type, "vc1smp5")) {
            *private1 = 0;
            *private2 = 5; /* WMV3*/
        }

        if (!strcasecmp(type, "vc1smp8")) {
            *private1 = 0;
            *private2 = 8; /* not WMV3 */
        }
    }

    return HI_SUCCESS;
}

hi_s32 get_adec_type(hi_char *type, hi_u32 *adec_type)
{
    hi_u32 adec_type_tmp = 0;
    hi_u32 i = 0;
    hi_u32 num = sizeof(g_adec_type_map) / sizeof(adec_type_map);

    for (; i< num; i++) {
        if (!strcasecmp(type, g_adec_type_map[i].type)) {
            break;
        }
    }

    if (i >= num) {
        return HI_FAILURE;
    }

    adec_type_tmp = g_adec_type_map[i].adec_type;
    *adec_type = adec_type_tmp;

    return HI_SUCCESS;
}


static hi_s32 prepare_fpga_mode()
{
    #define MAX_REPEAT_CNT 3
    static const hi_char *cmd[] = {
        "echo 0x0 0x60 0x2 0xa > /proc/msp/i2c",
        "echo 0x0 0x60 0x5 0x10 > /proc/msp/i2c",
        "echo 0x0 0x60 0x0d 0x01 > /proc/msp/i2c",
        HI_NULL
    };

    hi_u32 i = 0;
    hi_u32 repeat = 0;

    for (; cmd[i] != HI_NULL; ) {
        system(cmd[i]);

        if (i == 0 && (++repeat) < MAX_REPEAT_CNT) {
            continue;
        }
        i++;
    }

    return HI_SUCCESS;
}

static hi_bool process_input_cmd(hi_handle avplay, const hi_char *input_cmd, hi_u32 input_size)
{
    if (input_size < 32) {
        return HI_FALSE;
    }

    if ('q' == input_cmd[0]) {
        sample_printf("prepare to quit!\n");
        return HI_TRUE;
    }

    if ('p' == input_cmd[0] || 'P' == input_cmd[0]) {
        sample_printf("pause playback!\n");
        DO_FUNC(hi_unf_avplay_pause(avplay, HI_NULL));

        return HI_FALSE;
    }

    if ('r' == input_cmd[0] || 'R' == input_cmd[0]) {
        sample_printf("resume playback!\n");
        DO_FUNC(hi_unf_avplay_resume(avplay, HI_NULL));

        return HI_FALSE;
    }

    if (('s' == input_cmd[0]) || ('S' == input_cmd[0])) {
        static int spdif_toggle = 0;
        spdif_toggle++;

        if (spdif_toggle & 1) {
            hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0, HI_UNF_SND_OUTPUT_MODE_RAW);
            sample_printf("spdif pass-through on!\n");
        } else {
            hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_SPDIF0, HI_UNF_SND_OUTPUT_MODE_LPCM);
            sample_printf("spdif pass-through off!\n");
        }

        return HI_FALSE;
    }

    if ('h' == input_cmd[0] || 'H' == input_cmd[0]) {
        static int hdmi_toggle = 0;
        hdmi_toggle++;

        if (hdmi_toggle & 1) {
            hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_AUTO);
            sample_printf("set hdmi output mode to auto!\n");
        } else {
            hi_unf_snd_set_output_mode(HI_UNF_SND_0, HI_UNF_SND_OUT_PORT_HDMITX0, HI_UNF_SND_OUTPUT_MODE_LPCM);
            sample_printf("set hdmi output mode to pcm!\n");
        }

        return HI_FALSE;
    }

    return HI_FALSE;
}

hi_s32 main(hi_s32 argc, hi_char* argv[])
{
    hi_s32                    ret, index;
    hi_handle                 win;
    hi_handle                 track;
    hi_unf_audio_track_attr   track_attr;
    hi_unf_vcodec_type        vdec_type = HI_UNF_VCODEC_TYPE_MAX;
    hi_u32                    adec_type = 0;
    hi_handle                 avplay;
    hi_unf_avplay_attr        avplay_attr;
    hi_unf_sync_attr          sync_attr;
    hi_unf_avplay_stop_opt    stop_opt;
    hi_char                   input_cmd[32] = {0};
    hi_unf_acodec_decode_mode aud_dec_mode = HI_UNF_ACODEC_DEC_MODE_RAWPCM;
    hi_unf_video_format       default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
    hi_bool                   is_advanced_profil = 1;
    hi_u32                    codec_version = 8;
    hi_u32                    frame_rate_val = 0;

    if (argc < 5) {
        printf(" usage: sample_esplay vfile vtype afile atype -[options] \n");
        printf(" vtype: h264/h265/mpeg2/mpeg4/avs/real8/real9/vc1ap/vc1smp5(WMV3)/vc1smp8/vp6/vp6f/vp6a/vp8/divx3/h263/sor\n");
        printf(" atype: aac/mp3/dts/dra/mlp/pcm/ddp(ac3/eac3)\n");
        printf(" options:\n"
               " -mode0,   audio decode mode: pcm decode \n"
               " -mode1,   audio decode mode: raw_stream decode \n"
               " -mode2,   audio decode mode: pcm+raw_stream decode \n"
               " -once,    esplay run one time only \n"
               " -fps 60,  run on 60 fps \n"
               " -fpga,    run on fpga mode \n"
               " -fmt 1080P60/2160P60/4320P60: set video output at 1080P60 or 2160P60 or 4320P60\n");
        printf(" examples: \n");
        printf("   sample_esplay vfile h264 null null\n");
        printf("   sample_esplay null null afile mp3 \n");
        return 0;
    }

    if (strcasecmp("null", argv[1])) {
        g_vid_play = HI_TRUE;
        ret = get_vdec_type(argv[2], &vdec_type, (hi_u32 *)&is_advanced_profil, &codec_version);

        if (ret != HI_SUCCESS) {
            sample_printf("unsupport vid codec type!\n");
            return -1;
        }

        g_vdec_type = vdec_type;
        g_vid_es_file = fopen(argv[1], "rb");

        if (!g_vid_es_file) {
            sample_printf("open file %s error!\n", argv[1]);
            return -1;
        }
    }

    if (strcasecmp("null", argv[3])) {
        g_aud_es_file = fopen(argv[3], "rb");

        if (!g_aud_es_file) {
            sample_printf("open file %s error!\n", argv[3]);
            return -1;
        }

        if (argc > 5) {
            if (!strcasecmp("-mode0", argv[5])) {
                aud_dec_mode = HI_UNF_ACODEC_DEC_MODE_RAWPCM;
            } else if (!strcasecmp("-mode1", argv[5])) {
                aud_dec_mode = HI_UNF_ACODEC_DEC_MODE_THRU;
            } else if (!strcasecmp("-mode2", argv[5])) {
                aud_dec_mode = HI_UNF_ACODEC_DEC_MODE_SIMUL;
            }
            (void)aud_dec_mode;
        }

        g_aud_play = HI_TRUE;
        ret = get_adec_type(argv[4], &adec_type);

        if (ret != HI_SUCCESS) {
            sample_printf("unsupport aud codec type!\n");
            return -1;
        }

        if (!strcasecmp("amrwb", argv[4])) {
            /*read header of file for MIME file format */
            {
                hi_u8 magic_buf[8];

                fread(magic_buf, sizeof(hi_s8), strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER), g_aud_es_file);/* just need by amr file storage format (xxx.amr) */

                if (strncmp((const char*)magic_buf, HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER, strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER))) {
                    sample_printf("%s invalid amr wb file magic number", magic_buf);
                    return HI_FAILURE;
                }
            }

            g_aud_esfile_offset = strlen(HI_UNF_ACODEC_AMRWB_MAGIC_NUMBER);
        }
    }

    for (index = 5; index < argc; index++) {
        if (strcasestr("-fmt", argv[index]) && (++index) < argc) {
            hi_unf_video_format fmt = hi_adp_str2fmt(argv[index]);
            if (fmt != HI_UNF_VIDEO_FMT_MAX) {
                default_fmt = fmt;
            }
        } else if (strcasestr("-once", argv[index])) {
            sample_printf("Play once only.\n");
            g_play_once = HI_TRUE;
        } else if (strcasestr("-fps", argv[index])) {
            if ((frame_rate_val = atoi(argv[index + 1])) < 0) {
                sample_printf("Invalid fps.\n");
                return -1;
            }
        } else if (strcasestr("-fpga", argv[index])) {
            sample_printf("ESPlay use fpga mode\n");
            prepare_fpga_mode();
        }
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_avplay_init();

    if (ret != HI_SUCCESS) {
        sample_printf("call hi_adp_avplay_init failed.\n");
        goto SND_DEINIT;
    }

    if (g_aud_play) {
        ret = hi_adp_snd_init();

        if (ret != HI_SUCCESS) {
            sample_printf("call snd_init failed.\n");
            goto VO_DEINIT;
        }
    }

    ret  = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_ES);
    ret |= hi_unf_avplay_create(&avplay_attr, &avplay);

    if (ret != HI_SUCCESS) {
        sample_printf("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;
    ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);

    if (HI_SUCCESS != ret) {
        sample_printf("call hi_unf_avplay_set_attr failed.\n");
        goto AVPLAY_DEINIT;
    }

    if (g_aud_play) {
        ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_VSTOP;
        }

        ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
            goto ACHN_CLOSE;
        }

        ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &track);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_snd_create_track failed.\n");
            goto ACHN_CLOSE;
        }

        ret = hi_unf_snd_attach(track, avplay);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_snd_attach failed.\n");
            goto TRACK_DESTROY;
        }

        ret = hi_adp_avplay_set_adec_attr(avplay, adec_type);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_set_attr failed.\n");
            goto SND_DETACH;
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_start failed.\n");
        }
    }

    ret = hi_adp_disp_init(default_fmt);

    if (ret != HI_SUCCESS) {
        sample_printf("call DispInit failed.\n");
        goto SYS_DEINIT;
    }

    if (g_vid_play) {
        ret  = hi_adp_win_init();
        ret |= hi_adp_win_create(HI_NULL, &win);

        if (ret != HI_SUCCESS) {
            sample_printf("call VoInit failed.\n");
            hi_adp_win_deinit();
            goto DISP_DEINIT;
        }
    }

    if (g_vid_play) {
        ret = hi_unf_avplay_chan_open(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_chan_open failed.\n");
            goto AVPLAY_DESTROY;
        }

        /*set compress attr*/
        hi_unf_vdec_attr VcodecAttr;
        ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &VcodecAttr);

        if (HI_UNF_VCODEC_TYPE_VC1 == vdec_type) {
            VcodecAttr.ext_attr.vc1.advanced_profile = is_advanced_profil;
            VcodecAttr.ext_attr.vc1.codec_version = codec_version;
        }

        if (HI_UNF_VCODEC_TYPE_VP6 == vdec_type) {
            VcodecAttr.ext_attr.vp6.reverse = 0;
        }

        VcodecAttr.type = vdec_type;

        ret |= hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &VcodecAttr);

        if (HI_SUCCESS != ret) {
            sample_printf("call hi_unf_avplay_set_attr failed.\n");
            goto AVPLAY_DEINIT;
        }

        ret = hi_unf_win_attach_src (win, avplay);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_win_attach_src  failed.\n");
            goto VCHN_CLOSE;
        }

        ret = hi_unf_win_set_enable(win, HI_TRUE);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_win_set_enable failed.\n");
            goto WIN_DETATCH;
        }

        ret = hi_adp_avplay_set_vdec_attr(avplay, vdec_type, HI_UNF_VDEC_WORK_MODE_NORMAL);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_adp_avplay_set_vdec_attr failed.\n");
            goto WIN_DETATCH;
        }

        if (0 != frame_rate_val) {
            hi_unf_avplay_frame_rate_param frame_rate;
            frame_rate.type = HI_UNF_AVPLAY_FRMRATE_TYPE_USER;
            frame_rate.frame_rate = frame_rate_val;
            hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_FRAMERATE, &frame_rate);
        }

        ret = hi_unf_avplay_start(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);

        if (ret != HI_SUCCESS) {
            sample_printf("call hi_unf_avplay_start failed.\n");
            goto WIN_DETATCH;
        }
    }

    /*if (g_vid_es_file == HI_NULL) {
        hi_unf_disp_color BgColor;

        BgColor.red   = 0;
        BgColor.green = 200;
        BgColor.blue  = 200;
        hi_unf_disp_set_bg_color(HI_UNF_DISPLAY1, &BgColor);
    }*/

    g_stop_thread = HI_FALSE;
    pthread_create(&g_es_thread, HI_NULL, es_thread, (hi_void*)&avplay);

    while (1) {
        printf("please input the q to quit!, s to toggle spdif pass-through, h to toggle hdmi pass-through\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        g_stop_thread = process_input_cmd(avplay, input_cmd, sizeof(input_cmd));
        if (g_stop_thread) {
            break;
        }
    }

    g_stop_thread = HI_TRUE;
    pthread_join(g_es_thread, HI_NULL);

    if (g_aud_play) {
        stop_opt.timeout = 0;
        stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    }

SND_DETACH:

    if (g_aud_play) {
        hi_unf_snd_detach(track, avplay);
    }

TRACK_DESTROY:

    if (g_aud_play) {
        hi_unf_snd_destroy_track(track);
    }

ACHN_CLOSE:

    if (g_aud_play) {
        hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    }

AVPLAY_VSTOP:

    if (g_vid_play) {
        stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop_opt.timeout = 0;
        hi_unf_avplay_stop(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop_opt);
    }

WIN_DETATCH:

    if (g_vid_play) {
        hi_unf_win_set_enable(win, HI_FALSE);
        hi_unf_win_detach_src(win, avplay);
    }

VCHN_CLOSE:

    if (g_vid_play) {
        hi_unf_avplay_chan_close(avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    }

AVPLAY_DESTROY:
    hi_unf_avplay_destroy(avplay);

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

SND_DEINIT:

    if (g_aud_play) {
        hi_adp_snd_deinit();
    }

VO_DEINIT:

    if (g_vid_play) {
        hi_unf_win_destroy(win);
        hi_adp_win_deinit();
    }

DISP_DEINIT:
    hi_adp_disp_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();

    if (g_vid_play) {
        fclose(g_vid_es_file);
        g_vid_es_file = HI_NULL;
    }

    if (g_aud_play) {
        fclose(g_aud_es_file);
        g_aud_es_file = HI_NULL;
    }

    return 0;
}
