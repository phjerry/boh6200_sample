/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: omx decoder sample
 * Author: audio
 * Create: 2019-10-30
 * Notes: NA
 * History: 2019-10-30 for Hi3796CV300
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "securec.h"
#include "omx_audio_base.h"
#include "hi_unf_audio.h"
#include "hi_unf_sound.h"
#include "hi_adp_mpi.h"

#ifdef  ANDROID
#undef  LOG_TAG
#define LOG_TAG    "HIOMX_PLAY_SAMPLE"
#endif

#define FILE_NAME_LEN 256
#define SLEEP_TIME 1000

static hi_bool  g_special_omx_dec = HI_FALSE;

typedef struct {
    hi_u32   riff;
    hi_u32   file_len;
    hi_u32   wave;
    hi_u32   fmt;
    hi_u32   fmt_len;
    hi_u16   data_type;
    hi_u16   channels;
    hi_u32   sample_rate;
    hi_u32   bytes_per_sec;
    hi_u16   alignment;
    hi_u16   bit_depth;
    hi_u32   data;
    hi_u32   data_len;
} wav_header;

#define AMR_TIMING_PROFILE
#ifdef AMR_TIMING_PROFILE
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
static struct timeval g_time_begin;
static struct timeval g_time_end;
#endif

static hi_void get_start_time(hi_void)
{
#ifdef AMR_TIMING_PROFILE
    gettimeofday(&g_time_begin, HI_NULL);
#endif
}

static hi_void get_end_time(hi_char *func_name)
{
#ifdef AMR_TIMING_PROFILE
    hi_u64 time_start;
    hi_u64 time_end;
    gettimeofday(&g_time_end, HI_NULL);
    time_start = g_time_begin.tv_sec * 1000000LLU + g_time_begin.tv_usec;
    time_end = g_time_end.tv_sec * 1000000LLU + g_time_end.tv_usec;
    fprintf(stderr, "%s cosume time: %lld usec\n", func_name, time_end - time_start);
#endif
}

typedef struct {
    hi_char *decoder;
    const hi_char *short_name;
    hi_bool special;
} hi_omx_dec_group;

static const hi_omx_dec_group g_hi_omx_dec_table[] = {
    { "OMX.hisi.audio.decoder.aac", "aac", HI_FALSE },
    { "OMX.hisi.audio.decoder.mp3", "mp3", HI_FALSE },
    { "OMX.hisi.audio.decoder.eac3", "ddp", HI_FALSE },
    { "OMX.hisi.audio.decoder.dra", "dra", HI_FALSE },
    { "OMX.hisi.audio.decoder.dts", "dts", HI_FALSE },
    { "OMX.hisi.audio.decoder.vorbis", "vorbis", HI_FALSE },
    { "OMX.hisi.audio.decoder.cook", "cook", HI_TRUE },
    { "OMX.hisi.audio.decoder.ffmpeg", "ffmpeg", HI_FALSE },
    { "OMX.hisi.audio.decoder.opus", "opus", HI_FALSE },
};

typedef struct {
    OMX_STATETYPE state;
    hi_handle     track;
    hi_bool       audio_play;
    hi_bool       save_out_data;
    hi_char       in_file_name[FILE_NAME_LEN];
    hi_char       out_file_name[FILE_NAME_LEN];
    wav_header    wav_hd;
    OMX_S32       end_playing;
    OMX_S32       end_input_buf;
    OMX_S32       samplerate;
    OMX_S32       channel_out;
    FILE          *file_in;
    FILE          *file_out;
    OMX_STRING    decoder;
} audio_dec_app_data_type;

/* write wav file header structure */
static hi_void write_header(wav_header *h, hi_u32 sample_rate, hi_u32 channels)
{
    h->sample_rate = sample_rate;

    h->riff          = 0x46464952;
    h->wave          = 0x45564157;
    h->fmt           = 0x20746d66;
    h->fmt_len       = 0x10;
    h->data_type     = 0x1;
    h->channels      = channels;
    h->bit_depth     = HI_BIT_DEPTH_16;
    h->data          = 0x61746164;
    h->bytes_per_sec = h->sample_rate * 0x2 * h->channels;
    h->alignment     = 0x4;

    h->file_len = h->data_len + 16 + 20;
}

static hi_u32 fill_data(OMX_BUFFERHEADERTYPE *buf, FILE *file_in)
{
    hi_u32 read = fread(buf->pBuffer, 1, buf->nAllocLen, file_in);

    buf->nFilledLen = read;
    buf->nOffset    = 0;

    return read;
}

static OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE handle,
    OMX_BUFFERHEADERTYPE *buffer, audio_dec_app_data_type *app_priv_data)
{
    hi_u32 read;

    /* don't send more buffers after OMX_BUFFERFLAG_EOS */
    if (app_priv_data->end_input_buf) {
        buffer->nFlags = 0;
        printf("%d : APP:: entering send_input_buffer finish, buffer = %p\n", __LINE__, buffer);
        return OMX_ErrorNone;
    }

    read = fill_data(buffer, app_priv_data->file_in);

    if ((read < buffer->nAllocLen) && (app_priv_data->end_input_buf == 0)) {
        buffer->nFlags = OMX_BUFFERFLAG_EOS;
        app_priv_data->end_input_buf = 1;
        printf("%d : APP:: entering send_input_buffer OMX_BUFFERFLAG_EOS, buffer = %p\n", __LINE__, buffer);
    } else {
        buffer->nFlags = 0;
    }

    /* time stamp & tick count value */
    buffer->nTimeStamp = 0;
    buffer->nTickCount = 0;

    buffer->nFilledLen = read;
    OMX_EmptyThisBuffer(handle, buffer);

    return OMX_ErrorNone;
}

/*
 * this method will wait for the component to get to the state
 * specified by the desired_state input.
 */
static OMX_ERRORTYPE wait_for_state(OMX_HANDLETYPE *handle, OMX_STATETYPE desired_state)
{
    OMX_STATETYPE cur_state = OMX_StateReserved_0x00000000;
    OMX_ERRORTYPE error;
    OMX_COMPONENTTYPE *component = (OMX_COMPONENTTYPE *)handle;

    error = component->GetState(handle, &cur_state);
    if (cur_state == OMX_StateReserved_0x00000000) {
        error = OMX_ErrorReserved_0x8000100A;
    }

    while ((error == OMX_ErrorNone) && (cur_state != desired_state)) {
        sched_yield();
        error = component->GetState(handle, &cur_state);
        if (cur_state == OMX_StateReserved_0x00000000) {
            printf("Invalid State\n");
            error = OMX_ErrorReserved_0x8000100A;
        }
    }

    return error;
}

static hi_void event_cmd_complete_handler(OMX_U32 data1, OMX_U32 data2)
{
    if (data1 == OMX_CommandStateSet) {
        switch (data2) {
            case OMX_StateReserved_0x00000000:
                printf("OMX_StateReserved_0x00000000\n");
                break;
            case OMX_StatePause:
                printf("OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                printf("OMX_StateWaitForResources\n");
                break;
            case OMX_StateLoaded:
                printf("OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                printf("OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                printf("OMX_StateExecuting\n");
                break;
            default:
                printf("OMX_State unknow cmd = 0x%lx\n", data2);
                break;
        }
    } else if (data1 == OMX_CommandPortDisable) {
        printf("OMX_CommandPortDisable\n");
    } else if (data1 == OMX_CommandPortEnable) {
        /* change the state from reconfig to executing on receiving this callback */
        printf("OMX_CommandPortEnable\n");
    }
}

static OMX_ERRORTYPE event_handler(OMX_OUT OMX_HANDLETYPE component,
        OMX_OUT OMX_PTR        app_data,
        OMX_OUT OMX_EVENTTYPE  event,
        OMX_OUT OMX_U32        data1,
        OMX_OUT OMX_U32        data2,
        OMX_OUT OMX_PTR        event_data)
{
    audio_dec_app_data_type *app_priv_data = (audio_dec_app_data_type *)app_data;

    if (event == OMX_EventCmdComplete) {
        event_cmd_complete_handler(data1, data2);
    } else if (event == OMX_EventPortSettingsChanged) {
        printf("OMX_EventPortSettingsChanged\n");
    } else if (event == OMX_EventBufferFlag) {
        /* callback for EOS, change the state on receiving EOS callback, data1 == n_output_port_index */
        if ((data2 == OMX_BUFFERFLAG_EOS) && (data1 == 1)) {
            app_priv_data->end_playing = 1;
        }

        printf("EOS event recieved\n");
    } else if (event == OMX_EventError) {
        if ((OMX_ERRORTYPE)data1 == OMX_ErrorSameState) {
            printf("component reported same_state error, try to proceed\n");
        } else if ((OMX_ERRORTYPE)data1 == OMX_ErrorStreamCorrupt) {
            /* don't do anything right now for the stream corrupt error,
             * just count the number of such callbacks and let the component to proceed
             */
        } else {
            /* do nothing, just try to proceed normally */
        }
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE empty_buffer_done(OMX_OUT OMX_HANDLETYPE component,
        OMX_OUT OMX_PTR               app_data,
        OMX_OUT OMX_BUFFERHEADERTYPE *buffer)
{
    OMX_ERRORTYPE error;

    /* check the validity of buffer */
    if ((buffer == HI_NULL) || (app_data == HI_NULL)) {
        return OMX_ErrorBadParameter;
    }

    error = send_input_buffer(component, buffer, (audio_dec_app_data_type *)app_data);
    if (error != OMX_ErrorNone) {
        printf("[%s]: error %08x calling send_input_buffer\n", __func__, error);
        return error;
    }

    return OMX_ErrorNone;
}

static hi_void omx_play(audio_dec_app_data_type *app_priv_data,
    hi_aud_data_type *ha_data, OMX_BUFFERHEADERTYPE *buffer)
{
    hi_s32        ret;
    hi_unf_audio_frame ao_frame;

    if (app_priv_data->audio_play != HI_TRUE) {
        return;
    }

    ao_frame.bit_depth       = HI_BIT_DEPTH_16;
    ao_frame.interleaved          = HI_TRUE;
    ao_frame.pcm_samples = ha_data->pcm_mode.nSize;
    ao_frame.channels           = ha_data->pcm_mode.nChannels;
    ao_frame.sample_rate         = ha_data->pcm_mode.nSamplingRate;
    ao_frame.pts              = buffer->nTimeStamp;
    ao_frame.pcm_buffer         = (hi_s32 *)(buffer->pBuffer);
    ao_frame.frame_index         = 0;
    ao_frame.iec_data_type   = 0;

    if (ha_data->adec.bitstream_offset != 0) {
        ao_frame.bits_buffer       =
            (hi_s32 *)(buffer->pBuffer + ha_data->adec.bitstream_offset);
        ao_frame.bits_bytes = buffer->nFilledLen -
            ha_data->pcm_mode.nSize * sizeof(hi_s16) * ha_data->pcm_mode.nChannels;
    } else {
        ao_frame.bits_buffer       = HI_NULL;
        ao_frame.bits_bytes = 0;
    }

    while (1) {
        ret = hi_unf_snd_send_track_data(app_priv_data->track, &ao_frame);
        if (ret == HI_SUCCESS) {
            break;
        } else if (ret == HI_FAILURE) {
            printf("hi_unf_snd_send_track_data error\n");
            break;
        } else {
            usleep(SLEEP_TIME);
        }
    }
}

static OMX_ERRORTYPE fill_buffer_done(OMX_OUT OMX_HANDLETYPE h_component,
        OMX_OUT OMX_PTR               app_data,
        OMX_OUT OMX_BUFFERHEADERTYPE *buffer)
{
    OMX_ERRORTYPE error;
    hi_u32        pcm_bytes_per_frame;
    OMX_COMPONENTTYPE *component  = (OMX_COMPONENTTYPE *)h_component;
    audio_dec_app_data_type *app_priv_data = (audio_dec_app_data_type *)app_data;
    hi_aud_data_type *ha_data = (hi_aud_data_type *)(component->pComponentPrivate);

    pcm_bytes_per_frame = ha_data->pcm_mode.nSize * sizeof(hi_s16) * ha_data->pcm_mode.nChannels;

    /* check the validity of buffer */
    if ((buffer == HI_NULL) || (app_priv_data == HI_NULL)) {
        return OMX_ErrorBadParameter;
    }

    if (app_priv_data->save_out_data == HI_TRUE) {
        if ((pcm_bytes_per_frame > 0) && app_priv_data->file_out) {
            fwrite(buffer->pBuffer, 1, pcm_bytes_per_frame, app_priv_data->file_out);
            app_priv_data->wav_hd.data_len += pcm_bytes_per_frame;
        }
    }

    omx_play(app_priv_data, ha_data, buffer);

    buffer->nFilledLen = 0;
    error = OMX_FillThisBuffer(component, buffer);
    if (error != OMX_ErrorNone) {
        printf("in %s error %08x calling OMX_FillThisBuffer\n", __func__, error);
        return error;
    }

    return OMX_ErrorNone;
}

static OMX_CALLBACKTYPE g_hi_omx_dec_callbacks = {
    .EventHandler    = event_handler,
    .EmptyBufferDone = empty_buffer_done,
    .FillBufferDone  = fill_buffer_done,
};

/* show_usage: show the usage message */
static hi_s32 show_usage(hi_void)
{
    puts("\n"
         "usage:  sample_omx_play -i [InputFileName] [OPTIONS]\n"
         "\n");
    puts(
         "         -h     show this usage message and abort\n"
         "         -i     input file name\n");
    puts("         -t     the type of play audio(aac/mp3/ddp/vorbis/dts/dra/cook/ffmpeg)\n"
         "         -p     whether play the audio online\n"
         "                  0 = not  play (default)\n"
         "                  1 = need play\n"
         "         -o     the output file name to save\n"
         "                NOTE:if using the parameter, it would save the WAV file\n");
    puts("\n"
         "for example: ./sample_omx_play -i /mnt/afraic_48k.ac3 -t ddp -p1 -o /mnt/afraic.wav\n"
         "\n");

    return HI_SUCCESS;
}

static hi_s32 parse_opt(hi_s32 opt, hi_char *optarg, audio_dec_app_data_type *app_priv_data)
{
    hi_u32 i;

    if ((opt == 'h') || (opt == 'H')) {
        show_usage();
        return HI_FAILURE;
    }

    if (optarg == HI_NULL) {
        printf("invalid option -- '%c'\n", opt);
        return HI_FAILURE;
    }

    switch (opt) {
        case 'i' | 'I':
            if (strcpy_s(app_priv_data->in_file_name, FILE_NAME_LEN, optarg) != EOK) {
                return HI_FAILURE;
            }
            break;
        case 'o' | 'O':
            app_priv_data->save_out_data = HI_TRUE;
            if (strcpy_s(app_priv_data->out_file_name, FILE_NAME_LEN, optarg) != EOK) {
                return HI_FAILURE;
            }
            break;
        case 'p' | 'P':
            app_priv_data->audio_play = (hi_bool)atoi(optarg);
            break;
        case 't' | 'T':
            app_priv_data->decoder = HI_NULL;
            for (i = 0; i < sizeof(g_hi_omx_dec_table) / sizeof(g_hi_omx_dec_table[0]); i++) {
                if (!strcasecmp(g_hi_omx_dec_table[i].short_name, optarg)) {
                    app_priv_data->decoder = g_hi_omx_dec_table[i].decoder;
                    g_special_omx_dec = g_hi_omx_dec_table[i].special;
                    break;
                }
            }

            if (app_priv_data->decoder == HI_NULL) {
                fprintf(stderr, "unsupported adec type (-%s)\n", optarg);
                return HI_FAILURE;
            }
            break;
        default:
            fprintf(stderr, "invalid command line\n");
            show_usage();
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/* parse_cmd_line: parses the command-line input */
static hi_s32 parse_cmd_line(hi_s32 argc, hi_char *argv[], audio_dec_app_data_type *app_priv_data)
{
    /* declare local variables */
    hi_s32 opt;

    /* check input arguments */
    if ((argv == HI_NULL) || (argc < 0x2)) {
        show_usage();
        return HI_FAILURE;
    }

    while (1) {
        opt = getopt(argc, argv, "i:I:o:O:p:P:t:T:hH");
        if (opt == -1) {
            break;
        }

        if (parse_opt(opt, optarg, app_priv_data) != HI_SUCCESS) {
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

static hi_void omx_proc_special_omx_dec(OMX_HANDLETYPE audio_dec_handle)
{
    OMX_AUDIO_PARAM_RATYPE type;
    OMX_ERRORTYPE  error;
    OMX_VERSIONTYPE version;

    version.nVersion = 0x01000003;

    type.nSamplingRate = HI_SAMPLE_RATE_44K;
    type.nBitsPerFrame = 128;
    type.nChannels = HI_AUDIO_CH_STEREO;
    type.nVersion = version;
    type.nSamplePerFrame = 2048;
    type.nNumRegions = 37;
    type.nCouplingStartRegion = 2;
    type.nCouplingQuantBits = 4;

    error = OMX_SetParameter(audio_dec_handle, OMX_IndexParamAudioRa, (OMX_PTR)(&type));
    if (error != OMX_ErrorNone) {
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
    }
}

static hi_void omx_play_deinit(audio_dec_app_data_type *app_priv_data)
{
    hi_s32 ret;

    ret = hi_adp_disp_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_disp_deinit failed(0x%x)\n", ret);
    }

    ret = hi_unf_snd_destroy_track(app_priv_data->track);
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_snd_destroy_track failed(0x%x)\n", ret);
    }

    ret = hi_adp_snd_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_snd_deinit failed(0x%x)\n", ret);
    }

    ret = hi_unf_sys_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_deinit failed(0x%x)\n", ret);
    }
}

static hi_s32 omx_play_init(audio_dec_app_data_type *app_priv_data)
{
    hi_s32 ret;
    hi_unf_audio_track_attr track_attr;

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        fprintf(stderr, "call hi_unf_sys_init failed\n");
        return ret;
    }

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        fprintf(stderr, "call hi_adp_snd_init failed\n");
        goto SYS_DEINIT;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
    if (ret != HI_SUCCESS) {
        fprintf(stderr, "call hi_unf_snd_get_default_track_attr failed\n");
        goto SND_DEINIT;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &app_priv_data->track);
    if (ret != HI_SUCCESS) {
        fprintf(stderr, "call hi_unf_snd_create_track failed\n");
        goto SND_DEINIT;
    }

    ret = hi_adp_disp_init(HI_UNF_VIDEO_FMT_720P_50);
    if (ret != HI_SUCCESS) {
        fprintf(stderr, "call hi_adp_disp_init failed\n");
        goto TCK_DEINIT;
    }

    return HI_SUCCESS;

TCK_DEINIT:
    ret = hi_unf_snd_destroy_track(app_priv_data->track);
    if (ret != HI_SUCCESS) {
        printf("hi_unf_snd_destroy_track err: 0x%x\n", ret);
    }

SND_DEINIT:
    ret = hi_adp_snd_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_adp_snd_deinit failed(0x%x)\n", ret);
    }

SYS_DEINIT:
    ret = hi_unf_sys_deinit();
    if (ret != HI_SUCCESS) {
        printf("call hi_unf_sys_deinit failed(0x%x)\n", ret);
    }

    return HI_FAILURE;
}

hi_s32 main(hi_s32 argc, hi_char **argv)
{
    OMX_S32 i;
    OMX_S32 j;
    OMX_S32 inport_buf_size;
    OMX_S32 outport_buf_size;
    OMX_ERRORTYPE error;
    OMX_HANDLETYPE audio_dec_handle;
    audio_dec_app_data_type app_priv_data;
    OMX_PARAM_PORTDEFINITIONTYPE *comp_private_struct = HI_NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE  pcm_param;
    OMX_BUFFERHEADERTYPE *input_buffer[NUM_MAX_BUFFERS];
    OMX_BUFFERHEADERTYPE *output_buffer[NUM_MAX_BUFFERS];

    hi_s32 ret;

    ret = memset_s(&app_priv_data, sizeof(app_priv_data), 0, sizeof(app_priv_data));
    if (ret != 0) {
        printf("call memset_s failed(0x%x)\n", ret);
        return -1;
    }

    /* parse the parameters */
    ret = parse_cmd_line(argc, argv, &app_priv_data);
    if (ret != 0) {
        return -1;
    }

    /* open input file */
    if (strncmp(app_priv_data.in_file_name, "\0", 1) != 0) {
        fprintf(stderr, "\e[40m\e[36m\e[1mInput file name: %s\e[0m\n", app_priv_data.in_file_name);
        app_priv_data.file_in = fopen(app_priv_data.in_file_name, "rb");
        if (app_priv_data.file_in == HI_NULL) {
            fprintf(stderr, "open file %s error!\n", app_priv_data.in_file_name);
            return -1;
        }
    } else {
        printf("no input file to open\n");
        return -1;
    }

    /* if save output, open output file */
    if (app_priv_data.save_out_data == HI_TRUE) {
        app_priv_data.file_out = fopen(app_priv_data.out_file_name, "w");
        if (app_priv_data.file_out == HI_NULL) {
            printf("error:  failed to create the output file\n");
            goto OMX_EXIT;
        }

        if (app_priv_data.file_out) {
            /* write default WAV head */
            app_priv_data.wav_hd.data_len = 0;
            write_header(&app_priv_data.wav_hd, HI_SAMPLE_RATE_44K, HI_AUDIO_CH_STEREO);
            fseek(app_priv_data.file_out, 0, SEEK_SET);
            fwrite(&app_priv_data.wav_hd, sizeof(wav_header), 1, app_priv_data.file_out);
        }
    }

    /* if play audio online, do some initialization */
    if (app_priv_data.audio_play == HI_TRUE) {
        ret = omx_play_init(&app_priv_data);
        if (ret != HI_SUCCESS) {
            printf("call omx_play_init failed(0x%x)\n", ret);
            goto OMX_EXIT;
        }
    }

    /* begin to do OMX operation */
    error = OMX_Init();
    if (error != OMX_ErrorNone) {
        printf("%d :: error returned by OMX_Init()\n", __LINE__);
        goto EXIT;
    }

    fprintf(stderr, "\n\e[40m\e[36m\e[1mThis is <%s> test application\e[0m\n\n", app_priv_data.decoder);

    get_start_time();
    error = OMX_GetHandle(&audio_dec_handle, app_priv_data.decoder, &app_priv_data, &g_hi_omx_dec_callbacks);
    get_end_time("OMX_GetHandle");
    if (error != OMX_ErrorNone) {
        printf( "%s component not found\n", app_priv_data.decoder);
        goto EXIT;
    }

    if (g_special_omx_dec == HI_TRUE) {
        omx_proc_special_omx_dec(audio_dec_handle);
    }

    comp_private_struct = malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    if (comp_private_struct == HI_NULL) {
        printf("%d :: app: malloc failed\n", __LINE__);
        goto EXIT;
    }

    comp_private_struct->nPortIndex = OMX_IN_PORT_IDX;
    error = OMX_GetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
        goto EXIT;
    }

    inport_buf_size = comp_private_struct->nBufferSize;
    get_start_time();
    error = OMX_SetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    get_end_time("set parameter OMX_IndexParamPortDefinition");
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
        goto EXIT;
    }

    comp_private_struct->nPortIndex = OMX_OUT_PORT_IDX;
    get_start_time();
    error = OMX_GetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    get_end_time("get parameter OMX_IndexParamPortDefinition");
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
        goto EXIT;
    }

    outport_buf_size = comp_private_struct->nBufferSize;
    get_start_time();
    error = OMX_SetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    get_end_time("set parameter OMX_IndexParamPortDefinition");
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
        goto EXIT;
    }

    for (i = 0; i < NUM_IN_BUFFERS; i++) {
        error = OMX_AllocateBuffer(audio_dec_handle, &input_buffer[i], 0, HI_NULL, inport_buf_size);
        if (error != OMX_ErrorNone) {
            printf("%d :: error returned by OMX_AllocateBuffer()\n", __LINE__);
            goto EXIT;
        }
        printf("%d :: OMX_AllocateBuffer input_buffer[%ld] = %p\n", __LINE__, i, input_buffer[i]);
    }

    for (i = 0; i < NUM_OUT_BUFFERS; i++) {
        error = OMX_AllocateBuffer(audio_dec_handle, &output_buffer[i], 1, HI_NULL, outport_buf_size);
        if (error != OMX_ErrorNone) {
            printf("%d :: error returned by OMX_AllocateBuffer()\n", __LINE__);
            goto EXIT;
        }
        printf("%d :: OMX_AllocateBuffer output_buffer[%ld] = %p\n", __LINE__, i, output_buffer[i]);
    }

    get_start_time();
    error = OMX_SendCommand(audio_dec_handle, OMX_CommandStateSet, OMX_StateIdle, HI_NULL);
    if (error != OMX_ErrorNone) {
        printf("error from OMX_SendCommand <OMX_StateIdle> function - error = 0x%x\n", error);
        goto EXIT;
    }

    error = wait_for_state(audio_dec_handle, OMX_StateIdle);
    get_end_time("call to OMX_SendCommand <OMX_StateIdle>");
    if (error != OMX_ErrorNone) {
        printf("error: wait_for_state reports an error 0x%x\n", error);
        goto EXIT;
    }

    get_start_time();
    error = OMX_SendCommand(audio_dec_handle, OMX_CommandStateSet, OMX_StateExecuting, HI_NULL);
    if (error != OMX_ErrorNone) {
        printf("error from OMX_SendCommand <OMX_StateExecuting> function\n");
        goto EXIT;
    }

    error = wait_for_state(audio_dec_handle, OMX_StateExecuting);
    get_end_time("call to OMX_SendCommand <OMX_StateExecuting>");
    if (error != OMX_ErrorNone) {
        printf("error: wait_for_state reports an error 0x%x\n", error);
        goto EXIT;
    }

    for (j = 0; j < NUM_OUT_BUFFERS; j++) {
        printf("%d APP: before OMX_FillThisBuffer = %ld\n", __LINE__, j);
        OMX_FillThisBuffer(audio_dec_handle, output_buffer[j]);
    }

    for (j = 0; j < NUM_IN_BUFFERS; j++) {
        printf("%d APP: before send_input_buffer = %ld\n", __LINE__, j);
        error = send_input_buffer(audio_dec_handle, input_buffer[j], &app_priv_data);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        }
    }

    /* main process */
    while (error == OMX_ErrorNone) {
        if (app_priv_data.end_playing) {
            get_start_time();
            error = OMX_SendCommand(audio_dec_handle, OMX_CommandStateSet, OMX_StateIdle, HI_NULL);
            if (error != OMX_ErrorNone) {
                printf("error from OMX_SendCommand <OMX_StateIdle> function\n");
                goto EXIT;
            }

            error = wait_for_state(audio_dec_handle, OMX_StateIdle);
            get_end_time("call to OMX_SendCommand <OMX_StateIdle>");
            if (error != OMX_ErrorNone) {
                printf("ERROR: wait_for_state reports an error %X\n", error);
                goto EXIT;
            }

            printf("quit main loop\n");
            app_priv_data.end_playing = 0;
            break;
        }

        usleep(SLEEP_TIME);
    }

    /* free the allocate buffers */
    for (i = 0; i < NUM_IN_BUFFERS; i++) {
        printf("%d :: app: free %p IP buf\n", __LINE__, input_buffer[i]);
        error = OMX_FreeBuffer(audio_dec_handle, OMX_IN_PORT_IDX, input_buffer[i]);
        if ((error != OMX_ErrorNone)) {
            printf("%d:: error in free handle function\n", __LINE__);
            goto EXIT;
        }
    }

    for (i = 0; i < NUM_OUT_BUFFERS; i++) {
        printf("%d :: app: free %p OP buf\n", __LINE__, output_buffer[i]);
        error = OMX_FreeBuffer(audio_dec_handle, OMX_OUT_PORT_IDX, output_buffer[i]);
        if ((error != OMX_ErrorNone)) {
            printf("%d:: error in free handle function\n", __LINE__);
            goto EXIT;
        }
    }

    pcm_param.nPortIndex = OMX_OUT_PORT_IDX;
    /* get samplerate & ch info for wav fileformat after success decoded audiofile */
    error = OMX_GetParameter(audio_dec_handle, OMX_IndexParamAudioPcm, &pcm_param);
    if (error != OMX_ErrorNone) {
        printf("error: OMX_GetParameter reports an error %X\n", error);
        goto EXIT;
    }

    app_priv_data.samplerate = pcm_param.nSamplingRate;
    app_priv_data.channel_out = pcm_param.nChannels;

    get_start_time();
    error = OMX_SendCommand(audio_dec_handle, OMX_CommandStateSet, OMX_StateLoaded, HI_NULL);
    error = wait_for_state(audio_dec_handle, OMX_StateLoaded);
    get_end_time("call to send_command <OMX_StateLoaded>");
    if (error != OMX_ErrorNone) {
        printf ("%d:: error from OMX_SendCommand <OMX_StateLoaded> function\n", __LINE__);
        goto EXIT;
    }

    get_start_time();
    error = OMX_FreeHandle(audio_dec_handle);
    get_end_time("call to OMX_FreeHandle ");
    if (error != OMX_ErrorNone) {
        printf("%s component err in free handle\n", app_priv_data.decoder);
    }

    printf("call to OMX_FreeHandle\n");

EXIT:
    if (app_priv_data.audio_play == HI_TRUE) {
        omx_play_deinit(&app_priv_data);
    }

OMX_EXIT:
    if (comp_private_struct != HI_NULL) {
        free(comp_private_struct);
    }

    if (app_priv_data.file_in != HI_NULL) {
        fclose(app_priv_data.file_in);
        app_priv_data.file_in = HI_NULL;
    }

    if (app_priv_data.save_out_data == HI_TRUE) {
        if (app_priv_data.file_out != HI_NULL) {
            write_header(&app_priv_data.wav_hd, app_priv_data.samplerate, app_priv_data.channel_out);
            fseek(app_priv_data.file_out, 0, SEEK_SET);
            fwrite(&app_priv_data.wav_hd, sizeof(wav_header), 1, app_priv_data.file_out);
            fclose(app_priv_data.file_out);
            app_priv_data.file_out = HI_NULL;
        }
    }

    return -1;
}
