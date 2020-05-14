/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: omx aac encoder sample
 * Author: audio
 * Create: 2019-10-30
 * Notes: NA
 * History: 2019-10-30 for Hi3796CV300
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "omx_audio_base.h"

#ifdef  ANDROID
#undef  LOG_TAG
#define LOG_TAG    "HIOMX_AACENC_SAMPLE"
#endif

#define SLEEP_TIME 1000

static hi_s32 g_play_completed = 0;
static hi_s32 g_last_input_buffer_sent   = 0;

static FILE *g_file_input = HI_NULL;
static FILE *g_file_output = HI_NULL;

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

static OMX_STRING g_aac_encoder = "OMX.hisi.audio.encoder.aac";

typedef struct {
    OMX_STATETYPE state;
} audio_dec_app_data_type;

static hi_u32 fill_data(OMX_BUFFERHEADERTYPE *buf, FILE *file_in)
{
    hi_u32 read = fread(buf->pBuffer, 1, buf->nAllocLen, file_in);

    buf->nFilledLen = read;
    buf->nOffset    = 0;

    return read;
}

static OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE handle,
    OMX_BUFFERHEADERTYPE *buffer, FILE *file_in)
{
    hi_u32 read;

    /* don't send more buffers after OMX_BUFFERFLAG_EOS */
    if (g_last_input_buffer_sent) {
        buffer->nFlags = 0;
        printf("%d : APP:: entering send_input_buffer finish, buffer = %p\n", __LINE__, buffer);
        return OMX_ErrorNone;
    }

    read = fill_data(buffer, file_in);

    if ((read < buffer->nAllocLen) && (g_last_input_buffer_sent == 0)) {
        buffer->nFlags = OMX_BUFFERFLAG_EOS;
        g_last_input_buffer_sent = 1;
        printf("%d :APP:: entering send_input_buffer OMX_BUFFERFLAG_EOS, buffer = %p\n", __LINE__, buffer);
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
    if (event == OMX_EventCmdComplete) {
        event_cmd_complete_handler(data1, data2);
    } else if (event == OMX_EventPortSettingsChanged) {
        printf("OMX_EventPortSettingsChanged\n");
    } else if (event == OMX_EventBufferFlag) {
        /* callback for EOS, change the state on receiving EOS callback, data1 == n_output_port_index */
        if ((data2 == OMX_BUFFERFLAG_EOS) && (data1 == 1)) {
            g_play_completed = 1;
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
    if (buffer == HI_NULL) {
        return OMX_ErrorBadParameter;
    }

    error = send_input_buffer(component, buffer, g_file_input);
    if (error != OMX_ErrorNone) {
        printf("[%s]: error %08x calling send_input_buffer\n", __func__, error);
        return error;
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE fill_buffer_done(OMX_OUT OMX_HANDLETYPE component,
        OMX_OUT OMX_PTR               app_data,
        OMX_OUT OMX_BUFFERHEADERTYPE *buffer)
{
    OMX_ERRORTYPE error;

    /* check the validity of buffer */
    if (buffer == HI_NULL) {
        return OMX_ErrorBadParameter;
    }

    if (buffer->nFilledLen > 0) {
        fwrite(buffer->pBuffer, 1, buffer->nFilledLen, g_file_output);
    }

    buffer->nFilledLen = 0;
    error = OMX_FillThisBuffer(component, buffer);
    if (error != OMX_ErrorNone) {
        printf("[%s]: error %08x calling OMX_FillThisBuffer\n", __func__, error);
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
static hi_void show_usage(hi_void)
{
    puts("\n"
         "usage:  sample_omxaenc_aac [InputFileName] [OutputFileName]\n"
         "NOTE: sample_omxaenc_aac now only support 48k/2ch/16bit PCM input\n"
         "\n");
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
    OMX_BUFFERHEADERTYPE *input_buffer[NUM_MAX_BUFFERS] = { HI_NULL };
    OMX_BUFFERHEADERTYPE *output_buffer[NUM_MAX_BUFFERS] = { HI_NULL };

    if (argc < 2) {
        show_usage();
        return 0;
    }

    g_file_input = fopen(argv[1], "rb");
    if (g_file_input == HI_NULL) {
        printf("error:  failed to open the file %s for readonly\n", argv[1]);
        goto EXIT;
    }

    g_file_output = fopen(argv[2], "w");
    if (g_file_output == HI_NULL) {
        printf("error:  failed to create the output file \n");
        goto EXIT;
    }

    error = OMX_Init();
    if (error != OMX_ErrorNone) {
        printf("%d :: error returned by OMX_Init()\n", __LINE__);
        goto EXIT;
    }

    printf("------------------------------------------------------\n");
    printf("this is main thread in AAC ENCODER test application:\n");

    get_start_time();
    error = OMX_GetHandle(&audio_dec_handle, g_aac_encoder, &app_priv_data, &g_hi_omx_dec_callbacks);
    get_end_time("OMX_GetHandle");

    if (error != OMX_ErrorNone) {
        printf("%s component not found\n", g_aac_encoder);
        goto EXIT;
    }

    comp_private_struct = malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    if (comp_private_struct == HI_NULL) {
        printf("%d :: app: malloc failed\n", __LINE__);
        goto EXIT;
    }

    comp_private_struct->nPortIndex = OMX_IN_PORT_IDX;
    error = OMX_GetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    if (error != OMX_ErrorNone) {
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
        goto EXIT;
    }

    inport_buf_size = comp_private_struct->nBufferSize;
    get_start_time();
    error = OMX_SetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    get_end_time("set parameter OMX_IndexParamPortDefinition");
    if (error != OMX_ErrorNone) {
        printf("%d :: OMX_ErrorBadParameter\n", __LINE__);
        goto EXIT;
    }

    comp_private_struct->nPortIndex = OMX_OUT_PORT_IDX;
    get_start_time();
    error = OMX_GetParameter(audio_dec_handle, OMX_IndexParamPortDefinition, comp_private_struct);
    get_end_time("get parameter OMX_IndexParamPortDefinition");
    if (error != OMX_ErrorNone) {
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
        error = send_input_buffer(audio_dec_handle, input_buffer[j], g_file_input);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        }
    }

    pcm_param.nPortIndex = OMX_IN_PORT_IDX;
    error = OMX_GetParameter(audio_dec_handle, OMX_IndexParamAudioPcm, &pcm_param);
    if (error != OMX_ErrorNone) {
        printf("error: OMX_GetParameter reports an error 0x%x\n", error);
        goto EXIT;
    } else {
        printf("AAC encode parameters: samplerate = %ld, channel = %ld\n",
            pcm_param.nSamplingRate, pcm_param.nChannels);
    }

    /* main process */
    while (error == OMX_ErrorNone) {
        if (g_play_completed) {
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
            g_play_completed = 0;
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
        printf("%s component err in free handle\n", g_aac_encoder);
    }

    printf("call to OMX_FreeHandle\n");

EXIT:
    if (comp_private_struct != HI_NULL) {
        free(comp_private_struct);
    }

    if (g_file_input != HI_NULL) {
        fclose(g_file_input);
        g_file_input = HI_NULL;
    }

    if (g_file_output != HI_NULL) {
        fclose(g_file_output);
        g_file_output = HI_NULL;
    }

    return -1;
}
