/******************************************************************************

  Copyright (C), 2014-2015, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : rawplay.c
  Version       : Initial Draft
  Description   : video raw data player, multi channel support
  History       :
  1.Date        : 2014/11/13
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

#include "hi_unf_avplay.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_adp_mpi.h"
//#include "hi_common.h"

#ifdef HI_TEE_SUPPORT
//#define RAWP_TEE_SUPPORT
#endif

#ifdef RAWP_TEE_SUPPORT
#include "tee_client_api.h"
#include "mpi_mmz.h"
#ifndef HI_SMMU_SUPPORT
#include "sec_mmz.h"
#endif
#endif

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define MAX_CODEC_NUM         (16)

#define MAX_LOAD_TIDX_NUM     (50*1024)       /*一次性从.tidx文件中读取的帧信息数*/

static hi_u32 g_RectWinTable[MAX_CODEC_NUM][3] =
{
    {1, 1, 1},  {2, 2, 1},  {3, 2, 2},  {4, 2, 2},
    {5, 3, 2},  {6, 3, 2},  {7, 3, 3},  {8, 3, 3},
    {9, 3, 3},  {10, 4, 3}, {11, 4, 3}, {12, 4, 3},
    {13, 4, 4}, {14, 4, 4}, {15, 4, 4}, {16, 4, 4},
};

typedef enum
{
    VIDEO_MODE_RANDOM = 0,     /*send by random size*/
    VIDEO_MODE_STRM_SIZE,      /*send by stream info size*/
    VIDEO_MODE_EXT_SIZE,       /*send by external info size*/
    VIDEO_MODE_BUTT
}stream_mode;

/*图像帧信息*/
typedef struct
{
    hi_u32 frame_num;
    hi_u32 frame_offset;
    hi_u32 frame_size;
    hi_u32 frame_type;              // frame coding type
    hi_u32 top_field_type;           // top field coding type
    hi_u32 btm_field_type;           // buttom field coding type
    long   frame_pts;
    hi_u32 frame_structure;         // 0-frame ; 1-fieldpair
    hi_u32 top_field_crc;
    hi_u32 btm_field_crc;
    hi_u32 frame_crc;
    hi_u32 nvop_flag;               // NVOP输不输出的标志
    hi_u32 already_output_flag;      // 帧已经输出标志

}frame_info;

typedef struct
{
    hi_u32 SendCnt;
    FILE *pTidxFile;
    frame_info stFrameInfotidx[MAX_LOAD_TIDX_NUM];

}tidx_context;

typedef struct hicodec_param
{
    hi_u32  codec_id;
    hi_bool is_vid_paly;
    hi_bool is_smp_enable;
    hi_bool rand_err_enable;
    stream_mode stream_mode;
    FILE   *vid_es_file;
    pthread_t es_thread;
    hi_unf_vcodec_type vdec_type;
    hi_bool advance_profile;
    hi_bool crc_enable;
    hi_u32  codec_version;
    hi_handle avplay;
    hi_handle win;
    hi_unf_mem_buf tidx_buffer;
    tidx_context *tidx_ctx;
    hi_u32 inquire_count;
    hi_u32 frame_count;
    hi_u32 need_parse_hdr;
}codec_param;


codec_param g_codec[MAX_CODEC_NUM];

#define FIXED_FRAME_SIZE          (0x200000)
#define CONFIG_ES_BUFFER_SIZE     (16*1024*1024)

static hi_bool g_stop_thread = HI_FALSE;
static hi_bool g_play_once = HI_FALSE;
static hi_bool g_inquire_off = HI_FALSE;
static hi_bool g_auto_crc_enable = HI_FALSE;
static hi_bool g_es_thread_exit[MAX_CODEC_NUM] = {HI_FALSE};

#ifdef RAWP_TEE_SUPPORT
typedef enum
{
    CMD_TZ_EMPTY_DRM_INITIALIZE = 1,
    CMD_TZ_EMPTY_DRM_TERMINATE,
    CMD_TZ_EMPTY_DRM_CA2TA,
}Hi_TzEmptyDrmCommandId_E;

#ifdef HI_SMMU_SUPPORT
static TEEC_Context   g_tee_context;
#endif

static TEEC_Session   g_tee_session;

hi_s32 InitSecEnvironment(hi_void)
{
#ifdef HI_SMMU_SUPPORT
    TEEC_Result result;
    uint32_t origin;
    TEEC_UUID svc_id = {0x79b77788, 0x9789, 0x4a7a, {0xa2, 0xbe, 0xb6, 0x01, 0x55, 0x11, 0x11, 0x11}};

    TEEC_Operation session_operation;

    memset(&session_operation, 0, sizeof(TEEC_Operation));

    session_operation.started = 1;
    session_operation.paramTypes = TEEC_PARAM_TYPES(
                               TEEC_NONE,
                               TEEC_NONE,
                               TEEC_MEMREF_TEMP_INPUT,
                               TEEC_MEMREF_TEMP_INPUT);

    if (TEEC_InitializeContext(HI_NULL, &g_tee_context) != TEEC_SUCCESS)
    {
        sample_printf("TEEC_InitializeContext Failed!\n");
        return HI_FAILURE;
    }

    result = TEEC_OpenSession(&g_tee_context, &g_tee_session, &svc_id, TEEC_LOGIN_IDENTIFY, HI_NULL, &session_operation, &origin);
    if (result != TEEC_SUCCESS)
    {
        sample_printf("TEEC_OpenSession Failed!\n");
        TEEC_FinalizeContext(&g_tee_context);
        return HI_FAILURE;
    }

    memset(&session_operation, 0, sizeof(TEEC_Operation));

    session_operation.started = 1;
    session_operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    result = TEEC_InvokeCommand(&g_tee_session, CMD_TZ_EMPTY_DRM_INITIALIZE, &session_operation, HI_NULL);
    if (TEEC_SUCCESS != result)
    {
        sample_printf("init empty drm failed\n");

        TEEC_CloseSession(&g_tee_session);
        TEEC_FinalizeContext(&g_tee_context);

        return HI_FAILURE;
    }

    return HI_SUCCESS;

#else
    HI_SEC_MMZ_Init();

    return HI_SUCCESS;
#endif
}


hi_void DeInitSecEnvironment(hi_void)
{
#ifdef HI_SMMU_SUPPORT
    TEEC_Result result;
    TEEC_Operation session_operation;

    memset(&session_operation, 0, sizeof(TEEC_Operation));

    session_operation.started = 1;
    session_operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    result = TEEC_InvokeCommand(&g_tee_session, CMD_TZ_EMPTY_DRM_TERMINATE, &session_operation, HI_NULL);
    if (result != TEEC_SUCCESS)
    {
        sample_printf("deinit empty drm failed\n");
    }

    TEEC_CloseSession(&g_tee_session);

    TEEC_FinalizeContext(&g_tee_context);
#else
    HI_SEC_MMZ_DeInit();
#endif
}

hi_s32 HI_SEC_CA2TA(hi_u32 SrcAddr, hi_u32 DstAddr, hi_u32 read_len)
{
    TEEC_Result result;
    TEEC_Operation operation;

    operation.started = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = (hi_u32)SrcAddr;
    operation.params[0].value.b = read_len;
    operation.params[1].value.a = (hi_u32)DstAddr;
    operation.params[1].value.b = TEEC_VALUE_UNDEF;

    result = TEEC_InvokeCommand(&g_tee_session, CMD_TZ_EMPTY_DRM_CA2TA, &operation, HI_NULL);
    if (TEEC_SUCCESS != result)
    {
        sample_printf("%s failed, SrcAddr 0x%x, DstAddr 0x%x, read_len 0x%x\n", __func__, SrcAddr, DstAddr, Readlen);
        return HI_FAILURE;
    }

    return HI_SUCCESS; //(operation.params[1].value.b == 0) ? HI_SUCCESS : HI_FAILURE;
}
#endif

int frame_typeChartoSINT32(char type)
{
    int frame_type;

    if(type == 'I')
    {
        frame_type = 0;    // 0 表示帧的类型为 I
    }
    else if(type == 'P')
    {
        frame_type = 1;    // 1 表示帧的类型为 P
    }
    else if(type == 'B')
    {
        frame_type = 2;    // 2 表示帧的类型为 B
    }
    else
    {
        frame_type = -1;
    }
    return frame_type;
}

/***********************************************************/
/*              复位存放理论输出帧信息数组                 */
/***********************************************************/
void ResetFrameInfoTidx(frame_info *info, hi_u32 len)
{
    int i;

    /*将上次文件信息中的信息清空*/
    for(i = 0 ; i < len ; i++)
    {
        memset(&(info[i]) , -1 , sizeof(frame_info));
    }
}

/***********************************************************/
/*             从.tidx文件中读取一组帧信息                 */
/***********************************************************/
hi_s32 ReadFrameTidxInfo(tidx_context *tidx_ctx)
{
    hi_u32 i;
    hi_char *pFrameInfoBuf,*pFrameInfo;
    hi_char Type;
    hi_char FieldType[2];

    ResetFrameInfoTidx(tidx_ctx->stFrameInfotidx, MAX_LOAD_TIDX_NUM);

    if (tidx_ctx->pTidxFile == HI_NULL)
    {
        return HI_FAILURE;
    }

    if(!feof(tidx_ctx->pTidxFile))
    {
        pFrameInfoBuf = (char *)malloc(1024);

        for(i = 0 ; i < MAX_LOAD_TIDX_NUM ; i++)
        {
            fgets(pFrameInfoBuf, 1024, tidx_ctx->pTidxFile);

            pFrameInfo = pFrameInfoBuf;
            sscanf(pFrameInfo, "%d", &(tidx_ctx->stFrameInfotidx[i].frame_num));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo, "%x", &(tidx_ctx->stFrameInfotidx[i].frame_offset));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo, "%d", &(tidx_ctx->stFrameInfotidx[i].frame_size));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%ld", (long *)&(tidx_ctx->stFrameInfotidx[i].frame_pts));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%c", &Type);
            tidx_ctx->stFrameInfotidx[i].frame_type = frame_typeChartoSINT32(Type);
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%x", &(tidx_ctx->stFrameInfotidx[i].frame_structure));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%s", FieldType);
            tidx_ctx->stFrameInfotidx[i].top_field_type = frame_typeChartoSINT32(FieldType[0]);
            tidx_ctx->stFrameInfotidx[i].btm_field_type = frame_typeChartoSINT32(FieldType[1]);
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%x", &(tidx_ctx->stFrameInfotidx[i].top_field_crc));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%x", &(tidx_ctx->stFrameInfotidx[i].btm_field_crc));
            while(*pFrameInfo != ' ')pFrameInfo++;
            while(*pFrameInfo == ' ')pFrameInfo++;
            sscanf(pFrameInfo,"%d", &(tidx_ctx->stFrameInfotidx[i].nvop_flag));

            if( feof(tidx_ctx->pTidxFile))
            {
                break;
            }
        }

        free(pFrameInfoBuf);
    }
    return HI_SUCCESS;
}

int is_framepacket_standard(hi_unf_vcodec_type format)
{
    if( HI_UNF_VCODEC_TYPE_H263     == format
     || HI_UNF_VCODEC_TYPE_DIVX3    == format
     || HI_UNF_VCODEC_TYPE_VP8      == format
     || HI_UNF_VCODEC_TYPE_VP6      == format
     || HI_UNF_VCODEC_TYPE_VP6F     == format
     || HI_UNF_VCODEC_TYPE_VP6A     == format
     || HI_UNF_VCODEC_TYPE_SORENSON == format
     || HI_UNF_VCODEC_TYPE_REAL8    == format
     || HI_UNF_VCODEC_TYPE_REAL9    == format
     || HI_UNF_VCODEC_TYPE_VP9      == format
     || HI_UNF_VCODEC_TYPE_AV1      == format)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

hi_s32 TryOpenTidxFile(tidx_context *pCtx, hi_char *pSrcFile)
{
    hi_s32 ret;
    hi_char TidxFile[512];

    snprintf(TidxFile, sizeof(TidxFile), "%s.tidx", pSrcFile);

    pCtx->pTidxFile = fopen(TidxFile, "r");
    if (pCtx->pTidxFile == HI_NULL)
    {
        return HI_FAILURE;
    }

    ret = ReadFrameTidxInfo(pCtx);
    fclose(pCtx->pTidxFile);

    return ret;
}

hi_s32 ParseFrameSizeFromExtInfo(codec_param *codec, hi_u32 *pFrameSize)
{
    hi_u32 ReadFrameSize = 0;
    hi_u32 FrameOffset = 0;
    tidx_context *pCtx = codec->tidx_ctx;

    if (pCtx->SendCnt > 0 && pCtx->stFrameInfotidx[pCtx->SendCnt].frame_num <= 0)
    {
       sample_printf("Frame[%d] FrameNum(%d) in FrameInfo invalid, stream send over!\n", pCtx->SendCnt, pCtx->stFrameInfotidx[pCtx->SendCnt].frame_num);
       return HI_FAILURE;
    }

    if (pCtx->stFrameInfotidx[pCtx->SendCnt].frame_size != 0)
    {
       ReadFrameSize = pCtx->stFrameInfotidx[pCtx->SendCnt].frame_size;
    }

    FrameOffset = pCtx->stFrameInfotidx[pCtx->SendCnt].frame_offset;
    if (FrameOffset != 0)
    {
       fseek(codec->vid_es_file, FrameOffset, 0);
    }

    *pFrameSize = ReadFrameSize;
    if (*pFrameSize == 0 || *pFrameSize > FIXED_FRAME_SIZE)
    {
        sample_printf("%s parse frame size exceed %d\n", __func__, FIXED_FRAME_SIZE);
        return HI_FAILURE;
    }

    pCtx->SendCnt++;

    return HI_SUCCESS;
}

hi_s32 InquireStatus(hi_handle avplay, hi_u32 codec_id)
{
    hi_s32    ret;
    hi_u32 frame_count_read = 0;
    hi_unf_avplay_vid_chan_status status_info = {0};

#define MAX_INQUIRE_COUNT (1000)

    ret = hi_unf_avplay_get_status_info(avplay, HI_UNF_AVPLAY_STATUS_TYPE_VID_CHAN, &status_info);
    if (ret == HI_FAILURE) {
        return HI_FAILURE;
    }

    frame_count_read = status_info.played_frm_cnt;
    if (frame_count_read != g_codec[codec_id].frame_count) {
        g_codec[codec_id].inquire_count = 0;;
        g_codec[codec_id].frame_count = frame_count_read;
        return HI_SUCCESS;
    }

    g_codec[codec_id].inquire_count++;
    if (g_codec[codec_id].inquire_count > MAX_INQUIRE_COUNT) {
        sample_printf("[Error]:inquire_count > %d\n", MAX_INQUIRE_COUNT);
        return HI_FAILURE;
    }

    if (g_inquire_off == HI_TRUE) {
        sample_printf("close inquire\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_void NotifyEOS(hi_handle avplay, hi_u32 codec_id)
{
    hi_s32 ret = HI_FAILURE;
    hi_bool bIsEmpty = HI_FALSE;
    hi_unf_avplay_flush_stream_opt stFlushOpt;

    sample_printf("Sample send stream finish.\n");

    hi_unf_avplay_flush_stream(avplay, &stFlushOpt);
    do
    {
        ret = hi_unf_avplay_is_eos(avplay, &bIsEmpty);
        if (ret != HI_SUCCESS)
        {
            sample_printf("call hi_unf_avplay_is_eos failed.\n");
            break;
        }

        usleep(10*1000);
        if (InquireStatus(avplay, codec_id) != HI_SUCCESS)
        {
            break;
        }
    }while(bIsEmpty != HI_TRUE);

    sleep(1);
    return;
}

hi_s32 ParseVP9FrameSize(codec_param *codec, hi_u32 *pFrameSize)
{
    hi_char Vp9FileHdr[32];
    hi_char Vp9FrmHdr[12];
    char *const g_IVFSignature = "DKIF";
    static hi_u32 FrameCnt = 0;

    if (g_codec[codec->codec_id].need_parse_hdr == HI_TRUE)
    {
        if (fread(Vp9FileHdr, 1, 32, codec->vid_es_file) != 32)
        {
            sample_printf("Read vp9 file header failed.\n");

            return HI_FAILURE;
        }

        if (memcmp(g_IVFSignature, Vp9FileHdr, 4) != 0)
        {
            sample_printf("This VP9 file is not IVF file (Format ERROR).\n");

            return HI_FAILURE;
        }

        g_codec[codec->codec_id].need_parse_hdr = HI_FALSE;
        FrameCnt = 0;
    }

    if (fread(Vp9FrmHdr, 1, 12, codec->vid_es_file) != 12)
    {
        sample_printf("No enough stream to parse frame size!\n");

        return HI_FAILURE;
    }

    *pFrameSize  = Vp9FrmHdr[3] << 24;
    *pFrameSize |= Vp9FrmHdr[2] << 16;
    *pFrameSize |= Vp9FrmHdr[1] << 8;
    *pFrameSize |= Vp9FrmHdr[0];

    FrameCnt++;

    if (*pFrameSize == 0 || *pFrameSize > FIXED_FRAME_SIZE)
    {
        sample_printf("Parse frame size %d exceed %d\n", *pFrameSize, FIXED_FRAME_SIZE);

        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 ParseOtherFrameSize(codec_param *codec, hi_u32 *pFrameSize)
{
    hi_u32 read_len = 0;

    read_len = fread(pFrameSize, 1, 4, codec->vid_es_file);
    if (read_len != 4)
    {
        sample_printf("%s parse frame size failed, exit!\n", __func__);

        return HI_FAILURE;
    }

    if (*pFrameSize == 0 || *pFrameSize > FIXED_FRAME_SIZE)
    {
        sample_printf("%s parse frame size exceed %d\n", __func__, FIXED_FRAME_SIZE);

        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 ParseFrameSize(codec_param *codec, hi_u32 *pFrameSize)
{
    hi_s32 ret = HI_SUCCESS;

    if(codec->stream_mode == VIDEO_MODE_EXT_SIZE)
    {
        ret = ParseFrameSizeFromExtInfo(codec, pFrameSize);

    }
    else if (codec->stream_mode == VIDEO_MODE_STRM_SIZE)
    {
        if (codec->vdec_type == HI_UNF_VCODEC_TYPE_VP9 ||
            codec->vdec_type == HI_UNF_VCODEC_TYPE_AV1)
        {
            ret = ParseVP9FrameSize(codec, pFrameSize);
        }
        else
        {
            ret = ParseOtherFrameSize(codec, pFrameSize);
        }
    }
    else
    {
        *pFrameSize = FIXED_FRAME_SIZE;
    }

    return ret;
}

hi_void RandomAlterEsData(codec_param *codec, hi_u8 *pEsBuf, hi_u32 EsSize)
{
    hi_u32 pos;
    hi_u32 RandStep = 10;
    hi_u32 RandStart = pEsBuf[EsSize-1]%RandStep;

    for (pos=RandStart; pos<EsSize; pos++)
    {
        if (pos%RandStep == 1)
        {
            pEsBuf[pos] = pEsBuf[pos-1];
        }
    }

    return;
}

hi_void EsTthread(hi_void *args)
{
    hi_s32 ret = HI_FAILURE;
    hi_s32 read_len   = 0;
    hi_u32 frame_size = 0;
    hi_u8 *buffer = HI_NULL;
    codec_param *codec = (codec_param *)args;
    hi_unf_stream_buf stream_buf;
    hi_unf_avplay_put_buf_opt ext_opt;
    ext_opt.is_continue = HI_TRUE;
    ext_opt.end_of_frame = HI_FALSE;

    if (codec->stream_mode != VIDEO_MODE_RANDOM)
    {
        ext_opt.end_of_frame = HI_TRUE;
    }

#ifdef RAWP_TEE_SUPPORT
    hi_unf_mem_buf stCAStreamBuf;
    hi_unf_avplay_smp_attr AvTVPAttr;

    memset(&stCAStreamBuf, 0, sizeof(hi_unf_mem_buf));
    memset(&AvTVPAttr, 0, sizeof(hi_unf_avplay_smp_attr));

    hi_unf_avplay_get_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_SMP, &AvTVPAttr);

    if (AvTVPAttr.bEnable == HI_TRUE)
    {
    #ifdef HI_SMMU_SUPPORT
        strncpy(stCAStreamBuf.bufname, "CAStreamBuf", sizeof(stCAStreamBuf.bufname));
        stCAStreamBuf.bufsize = 4*1024*1024;

        stCAStreamBuf.phyaddr = HI_MPI_SMMU_New(stCAStreamBuf.bufname, stCAStreamBuf.bufsize);
        if (stCAStreamBuf.phyaddr != 0)
        {
            stCAStreamBuf.user_viraddr = HI_MPI_SMMU_Map(stCAStreamBuf.phyaddr, HI_FALSE);
            if (stCAStreamBuf.user_viraddr == 0)
            {
                sample_printf("%s: map smmu error phy:0x%x\n", __func__, stCAStreamBuf.phyaddr);

                HI_MPI_SMMU_Delete(stCAStreamBuf.phyaddr);

                return;
            }
        }
        else
        {
            sample_printf("%s: map smmu error size:0x%x\n", __func__, stCAStreamBuf.bufsize);
            return;
        }
    #else
        strncpy(stCAStreamBuf.bufname, "CAStreamBuf", sizeof(stCAStreamBuf.bufname));
                stCAStreamBuf.overflow_threshold  = 100;
                stCAStreamBuf.underflow_threshold = 0;
                stCAStreamBuf.bufsize = FIXED_FRAME_SIZE;
                ret = HI_MPI_MMZ_Malloc(&stCAStreamBuf);
        if (ret != HI_SUCCESS)
        {
           sample_printf("Alloc tee temp es buffer %d failed!\n", stCAStreamBuf.bufsize);

           return;
        }

    #endif
    }
#endif

    while (!g_stop_thread)
    {
        if (!codec->is_vid_paly)
        {
            break;
        }

        if (frame_size == 0)
        {
            ret = ParseFrameSize(codec, &frame_size);
            if (ret != HI_SUCCESS)
            {
                goto END_OF_FILE;
            }
            //sample_printf("frame_size = %d\n", FrameSize);
        }

        ret = hi_unf_avplay_get_buf(codec->avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, frame_size, &stream_buf, 0);
        if (ret != HI_SUCCESS)
        {
            usleep(10*1000);
            if(InquireStatus(codec->avplay, codec->codec_id) != HI_SUCCESS)
            {
                break;
            }
            else
            {
                continue;
            }
        }

#ifdef RAWP_TEE_SUPPORT
        if(HI_TRUE == AvTVPAttr.bEnable)
        {
            buffer = (hi_u8 *)stCAStreamBuf.user_viraddr;
        }
        else
#endif
        {
            buffer = (hi_u8 *)stream_buf.data;
        }

        read_len = fread(buffer, sizeof(hi_s8), frame_size, codec->vid_es_file);
        if (read_len < 0)
        {
            sample_printf("Sample read vid file error!\n");

            goto END_OF_THREAD;
        }
        else if (read_len == 0)
        {
            goto END_OF_FILE;
        }
        else
        {
            if (codec->rand_err_enable == HI_TRUE)
            {
                RandomAlterEsData(codec, buffer, read_len);
            }

        #ifdef RAWP_TEE_SUPPORT
            if(AvTVPAttr.bEnable == HI_TRUE)
            {
                ret = HI_SEC_CA2TA((hi_u32)stCAStreamBuf.phyaddr, (hi_u32)(long)stream_buf.data, read_len);
                if (ret != HI_SUCCESS)
                {
                    sample_printf("Tee call CA2TA failed!\n");

                    goto END_OF_THREAD;
                }
            }
        #endif

            ret = hi_unf_avplay_put_buf(codec->avplay, HI_UNF_AVPLAY_BUF_ID_ES_VID, read_len, 0, &ext_opt);
            if (ret != HI_SUCCESS)
            {
                sample_printf("Sample put es buffer failed!\n");

                goto END_OF_THREAD;
            }

            goto END_OF_RUN;
        }

END_OF_FILE:
        if (HI_TRUE == g_play_once)
        {
            NotifyEOS(codec->avplay, codec->codec_id);

            goto END_OF_THREAD;
        }
        else
        {
            sample_printf("Sample rewind!\n");
            g_codec[codec->codec_id].need_parse_hdr = HI_TRUE;

            rewind(codec->vid_es_file);
        }

END_OF_RUN:
        usleep(5 * 1000);
        frame_size = 0;

        continue;
    }

END_OF_THREAD:
#ifdef RAWP_TEE_SUPPORT
    if(AvTVPAttr.bEnable == HI_TRUE)
    {
    #ifdef HI_SMMU_SUPPORT
        HI_MPI_SMMU_Unmap(stCAStreamBuf.phyaddr);
        HI_MPI_SMMU_Delete(stCAStreamBuf.phyaddr);
    #else
         HI_MPI_MMZ_Free(&stCAStreamBuf);
    #endif
    }
#endif

    g_es_thread_exit[codec->codec_id] = HI_TRUE;

    return;
}

hi_bool IsCrcModeEnable(hi_char *pStr)
{
    hi_bool IsCrcMode = HI_FALSE;

    if (pStr == HI_NULL)
    {
        return HI_FALSE;
    }

    if (strstr(pStr, "/") == HI_NULL &&
        strstr(pStr, "crc") != HI_NULL)
    {
        IsCrcMode = HI_TRUE;
    }

    if (!strcasecmp("crc_auto", pStr))
    {
        g_auto_crc_enable = HI_TRUE;
    }

    return IsCrcMode;
}

hi_s32 InitCrcEnvironment(codec_param *codec, hi_char *pCrcMode, hi_char *pSrcFile)
{
    hi_s32 ret;
    hi_char EchoStr[512];
    hi_unf_mem_buf *ptidx_buffer = &(codec->tidx_buffer);

    snprintf(ptidx_buffer->buf_name, sizeof(ptidx_buffer->buf_name), "Sample");
    ptidx_buffer->buf_size = sizeof(tidx_context);

    ret = hi_unf_mem_malloc(ptidx_buffer);
    if (ret != HI_SUCCESS)
    {
        sample_printf("Alloc buffer %s size %lld failed!\n", ptidx_buffer->buf_name, ptidx_buffer->buf_size);
        return HI_FAILURE;
    }
    codec->tidx_ctx = (tidx_context *)(ptidx_buffer->user_viraddr);
    memset(codec->tidx_ctx, 0, sizeof(tidx_context));

    if (codec->vdec_type == HI_UNF_VCODEC_TYPE_VP8)
    {
        if (TryOpenTidxFile(codec->tidx_ctx, pSrcFile) == HI_SUCCESS)
        {
            codec->stream_mode = VIDEO_MODE_EXT_SIZE;
        }
        else
        {
            sample_printf("Open VP8 TidxFile failed!\n");
            return HI_FAILURE;
        }
    }

    snprintf(EchoStr, sizeof(EchoStr), "echo %s %s > /proc/msp/vfmw_crc", pCrcMode, pSrcFile);
    system(EchoStr);

    system("echo map_frm on > /proc/msp/vdec");

    g_play_once = HI_TRUE;

    codec->crc_enable = HI_TRUE;

    return HI_SUCCESS;
}

hi_s32 ExitCrcEnvironment(codec_param *codec)
{
    system("echo map_frm off > /proc/msp/vdec");
    system("echo crc_off > /proc/msp/vfmw_crc");
    hi_unf_mem_free(&(codec->tidx_buffer));
    codec->crc_enable = HI_FALSE;

    return HI_SUCCESS;
}

hi_s32 GetCodecParam(hi_char *argv[], codec_param *codec, hi_u32 *pUsedNum)
{
    hi_u32 CurPos = 0;
    hi_char *pSrcFile = HI_NULL;

    pSrcFile = argv[CurPos];
    codec->vid_es_file = fopen(pSrcFile, "rb");
    if (codec->vid_es_file == HI_NULL)
    {
        sample_printf("open file %s error!\n", argv[CurPos]);
        return HI_FAILURE;
    }
    CurPos++;

    if (!strcasecmp("mpeg2", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_MPEG2;
    }
    else if (!strcasecmp("mpeg4", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_MPEG4;
    }
    else if (!strcasecmp("h263", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_H263;
    }
    else if (!strcasecmp("sor", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_SORENSON;
    }
    else if (!strcasecmp("vp6", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VP6;
    }
    else if (!strcasecmp("vp6f", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VP6F;
    }
    else if (!strcasecmp("vp6a", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VP6A;
    }
    else if (!strcasecmp("h264", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_H264;
    }
    else if (!strcasecmp("h265", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_H265;
    }
    else if (!strcasecmp("mvc", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_H264_MVC;
    }
    else if (!strcasecmp("avs", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_AVS;
    }
    else if (!strcasecmp("avs2", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_AVS2;
    }
    else if (!strcasecmp("avs3", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_AVS3;
    }
    else if (!strcasecmp("real8", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_REAL8;
    }
    else if (!strcasecmp("real9", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_REAL9;
    }
    else if (!strcasecmp("vc1ap", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VC1;
        codec->advance_profile = 1;
        codec->codec_version     = 8;
    }
    else if (!strcasecmp("vc1smp5", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VC1;
        codec->advance_profile = 0;
        codec->codec_version     = 5;  //WMV3
    }
    else if (!strcasecmp("vc1smp8", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VC1;
        codec->advance_profile = 0;
        codec->codec_version     = 8;  //not WMV3
    }
    else if (!strcasecmp("vp8", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VP8;
    }
    else if (!strcasecmp("vp9", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_VP9;
    }
    else if (!strcasecmp("av1", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_AV1;
    }
    else if (!strcasecmp("divx3", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_DIVX3;
    }
    else if (!strcasecmp("mvc", argv[CurPos]))
    {
        codec->vdec_type = HI_UNF_VCODEC_TYPE_H264_MVC;
    }
    else if (!strcasecmp("mjpeg", argv[CurPos]))
    {
        codec->stream_mode = VIDEO_MODE_STRM_SIZE;
        codec->vdec_type = HI_UNF_VCODEC_TYPE_MJPEG;
    }
    else
    {
        sample_printf("unsupport vid codec type!\n");
        return HI_FAILURE;
    }
    CurPos++;

#ifdef RAWP_TEE_SUPPORT
    if (argv[CurPos] != HI_NULL)
    {
        if (!strcasecmp("tvpon", argv[CurPos]))
        {
            codec->is_smp_enable = HI_TRUE;
            CurPos++;
        }
        else if (!strcasecmp("tvpoff", argv[CurPos]))
        {
            codec->is_smp_enable = HI_FALSE;
            CurPos++;
        }
    }
#endif

    if (argv[CurPos] != HI_NULL)
    {
        if (!strcasecmp("randerr", argv[CurPos]))
        {
            codec->rand_err_enable = HI_TRUE;
            CurPos++;
            sample_printf("Enable random error test.\n");
        }
    }

    if (argv[CurPos] != HI_NULL)
    {
        if (IsCrcModeEnable(argv[CurPos]) == HI_TRUE)
        {
            hi_char CrcMode[10];
            strncpy(CrcMode, argv[CurPos], strlen(argv[CurPos])+1);
            CurPos++;

            if (InitCrcEnvironment(codec, CrcMode, pSrcFile) != HI_SUCCESS)
            {
                sample_printf("InitCrcEnvironment failed!\n");
                return HI_FAILURE;
            }
        }
    }

    (*pUsedNum) += CurPos;

    return HI_SUCCESS;
}

hi_void GetWinParam(hi_u32 TotalWinNum, hi_u32 Index, hi_unf_video_rect *rect_win)
{
    hi_u32 avr_width, avr_height;
    hi_u32 disp_width, disp_height;
    hi_u32 align_x_num, align_y_num;

    align_x_num = g_RectWinTable[TotalWinNum-1][1];
    align_y_num = g_RectWinTable[TotalWinNum-1][2];

    hi_unf_disp_get_virtual_screen(HI_UNF_DISPLAY0, &disp_width, &disp_height);

    avr_width  = disp_width/align_x_num;
    avr_height = disp_height/align_y_num;

    rect_win->x = (Index%align_x_num)*avr_width;
    rect_win->y = (Index/align_x_num)*avr_height;;
    rect_win->width = avr_width;
    rect_win->height = avr_height;

    return;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32  ret = HI_FAILURE;
    hi_bool have_tvp_inst = HI_FALSE;
    hi_s32  pos, index;
    hi_u32  used_num = 0;
    hi_bool is_low_delay = HI_FALSE;
    hi_s32  cur_codec_num  = 0;
    hi_s32  framerate = 0;
    hi_char input_cmd[32];
    hi_unf_video_format default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
    codec_param   *codec       = HI_NULL;
    hi_unf_video_rect                     rect_win;
    hi_unf_avplay_attr          avplay_attr;
    hi_unf_sync_attr            sync_attr;
    hi_unf_avplay_stop_opt      stop_attr;
    hi_unf_vdec_attr          vdec_attr;
    hi_unf_avplay_frame_rate_param frame_rate_param;
#ifdef RAWP_TEE_SUPPORT
    hi_unf_avplay_smp_attr      av_tvp_attr;
#endif

    if (argc < 3)
    {
        printf(" usage: sample_rawplay vfile vtype -[options] \n");
        printf(" vtype: h264/h265/mpeg2/mpeg4/avs/avs2/avs3/real8/real9\n");
        printf("        vc1ap/vc1smp5/vc1smp8/vp6/vp6f/vp6a/vp8/vp9/av1/divx3/h263/sor\n");
        printf(" options:\n"
               " -once,       run one time only \n"
               " -fps 60,     run on on 60 fps \n"
               " -ldy,        run on on lowdelay mode \n"
               " -crc_auto, enable crc auto check mode \n"
               " -crc_manu, enable crc manu check mode \n"
               " -crc_gen,   enable crc gen_crc mode \n"
               " 1080p50/1080p60  video output at 1080p50 or 1080p60\n");
#ifdef RAWP_TEE_SUPPORT
        printf(" -tvpon,   turn on tvp only the first channel run on tvp \n");
        printf(" -tvpoff,  turn off tvp \n");
#endif
        printf(" examples: \n");
        printf("   sample_rawplay vfile0 h264 vfile1 h265 ...\n");

        return HI_FAILURE;
    }

    memset(g_codec, 0, MAX_CODEC_NUM*sizeof(codec_param));

    for (index = 0; index < MAX_CODEC_NUM; index++)
    {
        pos = 1 + used_num;
        if (pos >= argc-1)
        {
            break;
        }

        ret = GetCodecParam(&argv[pos], &g_codec[index], &used_num);
        if (ret == HI_SUCCESS)
        {
            cur_codec_num++;
            g_codec[index].is_vid_paly = HI_TRUE;
            g_codec[index].codec_id = index;
            g_codec[index].need_parse_hdr = HI_TRUE;
            if (have_tvp_inst == HI_FALSE)
            {
                have_tvp_inst = g_codec[index].is_smp_enable;
            }
        }
        else if (0 == cur_codec_num)
        {
            sample_printf("Get Codec param failed!\n");
            return HI_FAILURE;
        }
        else
        {
            break;
        }
    }
    sample_printf("%d source to play.\n", cur_codec_num);

    for (index = 0; index < argc; index++)
    {
        if (!strcasecmp("1080p50", argv[index]))
        {
            sample_printf("ESPlay use 1080p50 output\n");
            default_fmt = HI_UNF_VIDEO_FMT_1080P_50;
            break;
        }
        else if (!strcasecmp("1080p60", argv[index]))
        {
            sample_printf("ESPlay use 1080p60 output\n");
            default_fmt = HI_UNF_VIDEO_FMT_1080P_60;
            break;
        }
        else if (!strcasecmp("-once", argv[index]))
        {
            sample_printf("Play once only.\n");
            g_play_once = HI_TRUE;
        }
        else if (!strcasecmp("-inquire_off", argv[index]))
        {
            sample_printf("InquireStatus off.\n");
            g_inquire_off = HI_TRUE;
        }
        else if (!strcasecmp("-ldy", argv[index]))
        {
            is_low_delay = HI_TRUE;
        }
        else if (!strcasecmp("-fps", argv[index]))
        {
            framerate = atoi(argv[index+1]);
            if(framerate < 0)
            {
                sample_printf("Invalid fps.\n");
                return HI_FAILURE;
            }
        }
    }

#ifdef RAWP_TEE_SUPPORT
    if (have_tvp_inst == HI_TRUE)
    {
        ret = InitSecEnvironment();
        if (ret != HI_SUCCESS)
        {
            sample_printf("call InitSecEnvironment failed.\n");
            goto WIN_DETATCH;
        }
    }
#endif

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_unf_avplay_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call hi_unf_avplay_init failed.\n");
        goto SYS_DEINIT;
    }

    ret = hi_adp_disp_init(default_fmt);
    if (ret != HI_SUCCESS)
    {
        sample_printf("call DispInit failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS)
    {
        sample_printf("call DispInit failed.\n");
        goto DISP_DEINIT;
    }

    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        if (codec->is_vid_paly)
        {
            ret  = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_ES);
            avplay_attr.vid_buf_size = CONFIG_ES_BUFFER_SIZE;
            ret |= hi_unf_avplay_create(&avplay_attr, &codec->avplay);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_avplay_create failed.\n");
                goto VO_DEINIT;
            }

            ret = hi_unf_avplay_get_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
            sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;
            ret |= hi_unf_avplay_set_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_unf_avplay_set_attr failed.\n");
                goto AVPLAY_DESTROY;
            }

            GetWinParam(cur_codec_num, index, &rect_win);
            ret |= hi_adp_win_create(&rect_win, &codec->win);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call VoInit failed.\n");
                hi_adp_win_deinit();
                goto AVPLAY_DESTROY;
            }

#ifdef RAWP_TEE_SUPPORT
            av_tvp_attr.bEnable = codec->is_smp_enable;
            ret = hi_unf_avplay_set_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_SMP, &av_tvp_attr);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_avplay_set_attr TVP failed.\n");
                goto VO_DESTROY;
            }
#endif
            ret = hi_unf_avplay_chan_open(codec->avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_avplay_chan_open failed.\n");
                goto VO_DESTROY;
            }

            /*set compress attr*/
            ret = hi_unf_avplay_get_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);

            if (HI_UNF_VCODEC_TYPE_VC1 == codec->vdec_type)
            {
                vdec_attr.ext_attr.vc1.advanced_profile = codec->advance_profile;
                vdec_attr.ext_attr.vc1.codec_version  = codec->codec_version;
            }
            else if (HI_UNF_VCODEC_TYPE_VP6 == codec->vdec_type)
            {
                vdec_attr.ext_attr.vp6.reverse = 0;
            }

            vdec_attr.type = codec->vdec_type;

            ret |= hi_unf_avplay_set_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
            if (HI_SUCCESS != ret)
            {
                sample_printf("call hi_unf_avplay_set_attr failed.\n");
                goto VCHN_CLOSE;
            }

            ret = hi_unf_win_attach_src(codec->win, codec->avplay);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_win_attach_src failed.\n");
                goto VCHN_CLOSE;
            }

            ret = hi_unf_win_set_enable(codec->win, HI_TRUE);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_win_set_enable failed.\n");
                goto WIN_DETATCH;
            }

            ret = hi_adp_avplay_set_vdec_attr(codec->avplay, codec->vdec_type, HI_UNF_VDEC_WORK_MODE_NORMAL);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_adp_avplay_set_vdec_attr failed.\n");
                goto WIN_DETATCH;
            }

            if (g_auto_crc_enable == HI_TRUE)
            {
                framerate = 60;
            }

            if (0 != framerate)
            {
                frame_rate_param.type = HI_UNF_AVPLAY_FRMRATE_TYPE_USER;
                frame_rate_param.frame_rate = framerate;
                hi_unf_avplay_set_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_FRAMERATE, &frame_rate_param);
                if (ret != HI_SUCCESS)
                {
                    sample_printf("call hi_adp_avplay_set_vdec_attr failed.\n");
                    goto WIN_DETATCH;
                }
            }

            if (is_low_delay)
            {
                hi_unf_avplay_low_delay_attr low_delay_attr;

                hi_unf_avplay_get_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_LOW_DELAY, &low_delay_attr);

                low_delay_attr.enable = HI_TRUE;
                ret = hi_unf_avplay_set_attr(codec->avplay, HI_UNF_AVPLAY_ATTR_ID_LOW_DELAY, &low_delay_attr);
                if (ret != HI_SUCCESS)
                {
                    sample_printf("call hi_unf_avplay_set_attr HI_UNF_AVPLAY_ATTR_ID_LOW_DELAY failed.\n");
                    goto WIN_DETATCH;
                }
            }

            ret = hi_unf_avplay_start(codec->avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_avplay_start failed.\n");
                goto WIN_DETATCH;
            }
        }
    }

    g_stop_thread = HI_FALSE;
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        pthread_create(&codec->es_thread, HI_NULL, (hi_void *)EsTthread, (hi_void *)codec);
    }

    while (1)
    {
        if (g_auto_crc_enable == HI_TRUE)
        {
cont:
            usleep(30*1000);
            for (index = 0; index < cur_codec_num; index++) {
                if (g_es_thread_exit[index] == HI_FALSE) {
                    goto cont;
                }
            }
            break;
        }

        printf("please input the q to quit!, s to toggle spdif pass-through, h to toggle hdmi pass-through\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if ('q' == input_cmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }
#if 0
        //FOR MVC TEST
        if ('3' == input_cmd[0] && 'd' == InputCmd[1]  )
        {
            HI_UNF_DISP_Set3DMode(HI_UNF_DISPLAY1,
                HI_UNF_DISP_3D_FRAME_PACKING,
                HI_UNF_ENC_FMT_1080P_24_FRAME_PACKING);
        }
#endif
#ifdef RAWP_TEE_SUPPORT
        if ('t' == input_cmd[0] || 'T' == InputCmd[0])
        {
            if(av_tvp_attr.bEnable == HI_TRUE)
            {
                sample_printf("call hi_unf_avplay_set_attr TVP false.\n");
                av_tvp_attr.bEnable = HI_FALSE;
            }
            else
            {
                sample_printf("call hi_unf_avplay_set_attr TVP true.\n");
                av_tvp_attr.bEnable = HI_TRUE;
            }

            stop_attr.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
            stop_attr.u32TimeoutMs = 0;
            hi_unf_avplay_stop(g_codec[0].avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop_attr);

            ret = hi_unf_avplay_set_attr(g_codec[0].avplay, HI_UNF_AVPLAY_ATTR_ID_SMP, &av_tvp_attr);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_avplay_set_attr TVP failed.\n");
                break;
            }

            ret = hi_unf_avplay_start(g_codec[0].avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
            if (ret != HI_SUCCESS)
            {
                sample_printf("call hi_unf_avplay_start failed.\n");
                break;
            }
        }
#endif
    }

    g_stop_thread = HI_TRUE;
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        pthread_join(codec->es_thread, HI_NULL);
        stop_attr.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        stop_attr.timeout = 0;
        hi_unf_avplay_stop(codec->avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stop_attr);
    }

WIN_DETATCH:
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        hi_unf_win_set_enable(codec->win, HI_FALSE);
        hi_unf_win_detach_src(codec->win, codec->avplay);
    }

VCHN_CLOSE:
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        hi_unf_avplay_chan_close(codec->avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    }

VO_DESTROY:
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        hi_unf_win_destroy(codec->win);
    }

AVPLAY_DESTROY:
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        hi_unf_avplay_destroy(codec->avplay);
    }

VO_DEINIT:
    hi_adp_win_deinit();

DISP_DEINIT:
    hi_adp_disp_deinit();

AVPLAY_DEINIT:
    hi_unf_avplay_deinit();

SYS_DEINIT:
    hi_unf_sys_deinit();
    for (index = 0; index < cur_codec_num; index++)
    {
        codec = &g_codec[index];
        fclose(codec->vid_es_file);
        codec->vid_es_file = HI_NULL;

        if (codec->crc_enable == HI_TRUE)
        {
            ExitCrcEnvironment(codec);
        }
    }

#ifdef RAWP_TEE_SUPPORT
    if (have_tvp_inst == HI_TRUE)
    {
        DeInitSecEnvironment();
    }
#endif

    sample_printf("Finish, sample exit!\n");

    return HI_SUCCESS;
}


