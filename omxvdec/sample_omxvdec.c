/*--------------------------------------------------------------------------
Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
/*
    An Open max test application ....
*/

#ifdef ANDROID
#include<log/log.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_IVCommon.h"
#include "OMX_VideoExt.h"
#include "OMX_IndexExt.h"
#include "OMX_RoleNames.h"
#include "sample_queue.h"
#include "sample_tidx.h"

#include "sample_omxvdec.h"

#ifdef HI_TVP_SUPPORT
#include "tee_client_api.h"
#endif

/**********************************************************
//#define SupportNative         //是否支持native buffer测试
//#define EnableTest            //是否支持异常退出测试
//#define EnableSetColor        //是否支持输出格式设置
//#define CalcFrameRate         //是否统计帧率
**********************************************************/

OmxTestInfo_S OmxTestInfo[MAX_INST_NUM];

/************************************************************************/
OMX_BOOL g_EnableLog = OMX_TRUE;

#ifdef ANDROID
#undef  LOG_TAG
#define LOG_TAG               "SAMPLE_OMXVDEC"

#define SAMPLE_TRACE()        ALOGE("fun: %s, line: %d\n", __func__, __LINE__)
#define SAMPLE_PRINT_ERROR    ALOGE
#define SAMPLE_PRINT_ALWS     ALOGE
#else
#define SAMPLE_TRACE()        printf("fun: %s, line: %d\n", __func__, __LINE__)
#define SAMPLE_PRINT_ERROR    printf
#define SAMPLE_PRINT_ALWS     printf
#endif

#define SAMPLE_PRINT_INFO(...)         \
do {                                   \
    if (g_EnableLog)                   \
        SAMPLE_PRINT_ALWS(__VA_ARGS__);\
}while(0)


/************************************************************************/

#define FAILED(result) (result != OMX_ErrorNone)
#define SUCCEEDED(result) (result == OMX_ErrorNone)

#define MAX_WIDTH        (4096)
#define MAX_HEIGHT        (2304)
#define DEFAULT_WIDTH    (1920)
#define DEFAULT_HEIGHT    (1088)

#define ALIGN_SIZE        (4096)

#define EnableAndroidNativeBuffers         "OMX.google.android.index.enableAndroidNativeBuffers"
#define GetAndroidNativeBufferUsage        "OMX.google.android.index.getAndroidNativeBufferUsage"
#define UseAndroidNativeBuffer2            "OMX.google.android.index.useAndroidNativeBuffer2"

#define MAX_COMP_NUM                       (40)  // total component num
#define MAX_COMP_NAME_LEN                  (50)  // max len of one component name

/************************************************************************/

#define CONFIG_VERSION_SIZE(param) \
    param.nVersion.nVersion = OMX_VERSION;\
    param.nSize = sizeof(param);

#define SWAPBYTES(ptrA, ptrB) { char t = *ptrA; *ptrA = *ptrB; *ptrB = t;}

#ifdef CalcFrameRate
static bool g_PrintAlready = false;
#endif

static int g_TestInstNum = 0;
static hi_bool g_AutoCrcEnable = HI_FALSE;

OMX_ERRORTYPE error;

#ifdef HI_TVP_SUPPORT
#define TEST_MEMCPY_CMD_ID  50

static TEEC_Context  g_TeeContext;
static TEEC_Session g_teeSession;

#endif

/*---------------------------- LOCAL DEFINITION ------------------------------*/
typedef struct
{
    hi_u32 SendCnt;
    FILE *pTidxFile;
    FRAME_INFO_S stFrameInfotidx[MAX_LOAD_TIDX_NUM];

}TIDX_CONTEXT_S;


/* CodecType Relative */
static char *tComponentName[MAX_COMP_NUM][2] = {

    /*video decoder components */
    {"OMX.hisi.video.decoder.avc",          OMX_ROLE_VIDEO_DECODER_AVC},
    {"OMX.hisi.video.decoder.vc1",          OMX_ROLE_VIDEO_DECODER_VC1},
    {"OMX.hisi.video.decoder.mpeg1",        OMX_ROLE_VIDEO_DECODER_MPEG1},
    {"OMX.hisi.video.decoder.mpeg2",        OMX_ROLE_VIDEO_DECODER_MPEG2},
    {"OMX.hisi.video.decoder.mpeg4",        OMX_ROLE_VIDEO_DECODER_MPEG4},
    {"OMX.hisi.video.decoder.h263",         OMX_ROLE_VIDEO_DECODER_H263},
    {"OMX.hisi.video.decoder.divx3",        OMX_ROLE_VIDEO_DECODER_DIVX3},
    {"OMX.hisi.video.decoder.vp8",          OMX_ROLE_VIDEO_DECODER_VP8},
    {"OMX.hisi.video.decoder.vp9",          OMX_ROLE_VIDEO_DECODER_VP9},
    {"OMX.hisi.video.decoder.vp6",          OMX_ROLE_VIDEO_DECODER_VP6},
    {"OMX.hisi.video.decoder.wmv",          OMX_ROLE_VIDEO_DECODER_WMV},
    {"OMX.hisi.video.decoder.avs",          OMX_ROLE_VIDEO_DECODER_AVS},
    {"OMX.hisi.video.decoder.avs2",         OMX_ROLE_VIDEO_DECODER_AVS2},
    {"OMX.hisi.video.decoder.avs3",         OMX_ROLE_VIDEO_DECODER_AVS3},
    {"OMX.hisi.video.decoder.av1",          OMX_ROLE_VIDEO_DECODER_AV1},
    {"OMX.hisi.video.decoder.sorenson",     OMX_ROLE_VIDEO_DECODER_SORENSON},
    {"OMX.hisi.video.decoder.hevc",         OMX_ROLE_VIDEO_DECODER_HEVC},
    {"OMX.hisi.video.decoder.mvc",          OMX_ROLE_VIDEO_DECODER_MVC},

  #if (defined(REAL8_SUPPORT) || defined(REAL9_SUPPORT))
    /* For copyright, vfmw didn't support std real. */
    {"OMX.hisi.video.decoder.real",         OMX_ROLE_VIDEO_DECODER_RV},
    {"OMX.hisi.video.decoder.real.secure",  OMX_ROLE_VIDEO_DECODER_RV},
  #endif

    /*secure video decoder components */
    {"OMX.hisi.video.decoder.avc.secure",   OMX_ROLE_VIDEO_DECODER_AVC},
    {"OMX.hisi.video.decoder.vc1.secure",   OMX_ROLE_VIDEO_DECODER_VC1},
    {"OMX.hisi.video.decoder.mpeg1.secure", OMX_ROLE_VIDEO_DECODER_MPEG1},
    {"OMX.hisi.video.decoder.mpeg2.secure", OMX_ROLE_VIDEO_DECODER_MPEG2},
    {"OMX.hisi.video.decoder.mpeg4.secure", OMX_ROLE_VIDEO_DECODER_MPEG4},
    {"OMX.hisi.video.decoder.divx3.secure", OMX_ROLE_VIDEO_DECODER_DIVX3},
    {"OMX.hisi.video.decoder.vp8.secure",   OMX_ROLE_VIDEO_DECODER_VP8},
    {"OMX.hisi.video.decoder.vp9.secure",   OMX_ROLE_VIDEO_DECODER_VP9},
    {"OMX.hisi.video.decoder.vp6.secure",   OMX_ROLE_VIDEO_DECODER_VP6},
    {"OMX.hisi.video.decoder.wmv.secure",   OMX_ROLE_VIDEO_DECODER_WMV},
    {"OMX.hisi.video.decoder.avs.secure",   OMX_ROLE_VIDEO_DECODER_AVS},
    {"OMX.hisi.video.decoder.avs2.secure",  OMX_ROLE_VIDEO_DECODER_AVS2},
    {"OMX.hisi.video.decoder.hevc.secure",  OMX_ROLE_VIDEO_DECODER_HEVC},

    /* terminate the table */
    {NULL, NULL},

};


static int get_componet_name_by_role(char *vdecCompNames, const char *compRole)
{
    int i, ret = 0;

    int count = sizeof(tComponentName)/(sizeof(tComponentName[0]));

    for (i = 0; i< count; i++)
    {
        if (!strcmp(compRole, tComponentName[i][1]))
        {
            strcpy(vdecCompNames, tComponentName[i][0]);

            break;
        }
    }

    if (i == count)
    {
        SAMPLE_PRINT_ERROR(" %s can NOT find component name by role:%s\n", __FUNCTION__, compRole);

        vdecCompNames = NULL;
        ret = -1;
    }

    return ret;
}



/****************************************************************************/
/* 获取系统时间  单位：毫秒                                                 */
/****************************************************************************/
#ifdef CalcFrameRate

static int GetTimeInMs(void)
{
    int CurrMs = 0;
    struct timeval CurrentTime;

    gettimeofday(&CurrentTime, 0);
    CurrMs = (int)(CurrentTime.tv_sec*1000 + CurrentTime.tv_usec/1000);

    return CurrMs;
}

static int GetTimeInUs(void)
{
    int CurrUs = 0;
    struct timeval CurrentTime;

    gettimeofday(&CurrentTime, 0);
    CurrUs = (int)(CurrentTime.tv_sec*1000*1000 + CurrentTime.tv_usec);

    return CurrUs;
}
#endif
static void wait_for_event(OmxTestInfo_S * pOmxTestInfo, int cmd)
{
    pthread_mutex_lock(&pOmxTestInfo->event_lock);
    while(1)
    {
        if((pOmxTestInfo->event_is_done == 1) && (pOmxTestInfo->last_cmd == cmd || pOmxTestInfo->last_cmd == -1))
        {
            pOmxTestInfo->event_is_done = 0;
            break;
        }
        pthread_cond_wait(&pOmxTestInfo->event_cond, &pOmxTestInfo->event_lock);

    }
    pthread_mutex_unlock(&pOmxTestInfo->event_lock);
}

static void event_complete(OmxTestInfo_S *pOmxTestInfo, int cmd)
{
    pthread_mutex_lock(&pOmxTestInfo->event_lock);
    if (pOmxTestInfo->event_is_done == 0)
        pOmxTestInfo->event_is_done = 1;

    pthread_cond_broadcast(&pOmxTestInfo->event_cond);
    pOmxTestInfo->last_cmd = cmd;
    pthread_mutex_unlock(&pOmxTestInfo->event_lock);
}


/************************************************************************
                file operation functions
************************************************************************/

static int open_video_file(OmxTestInfo_S *pOmxTestInfo)
{
    int FrameSize;
    int FrameWidth;
    int FrameHeight;
    int error_code = 0;
    char tidxfile[520] = {0};

    pOmxTestInfo->inputBufferFileFd = fopen(pOmxTestInfo->in_filename, "rb");
    if (pOmxTestInfo->inputBufferFileFd == NULL)
    {
        SAMPLE_PRINT_ERROR("Error: i/p file %s open failed, errno = %d\n", pOmxTestInfo->in_filename, errno);
        error_code = -1;
    }

    if (true == pOmxTestInfo->frame_in_packet)
    {
        if (false == pOmxTestInfo->readsize_add_in_stream) // FIX
        {
            memset(&pOmxTestInfo->stContext, 0, sizeof(STR_CONTEXT_S));
            sprintf(tidxfile, "%s.tidx", pOmxTestInfo->in_filename);
            pOmxTestInfo->stContext.fpTidxFile = fopen(tidxfile, "r");
            if (NULL == pOmxTestInfo->stContext.fpTidxFile)
            {
                SAMPLE_PRINT_ERROR("Error: i/p tidx file %s open failed, errno = %d\n",tidxfile, errno);
                error_code = -1;
            }
            TIDX_ReadFrameInfo(&pOmxTestInfo->stContext);
        }
    }

    if (CODEC_FORMAT_DIVX3 == pOmxTestInfo->codec_format_option)
    {
        fread(&FrameSize,   1, 4, pOmxTestInfo->inputBufferFileFd);
        fread(&FrameWidth,  1, 4, pOmxTestInfo->inputBufferFileFd);
        fread(&FrameHeight, 1, 4, pOmxTestInfo->inputBufferFileFd);

        pOmxTestInfo->sliceheight = FrameHeight;
        pOmxTestInfo->height      = FrameHeight;
        pOmxTestInfo->stride      = FrameWidth;
        pOmxTestInfo->width       = FrameWidth;
        fseek(pOmxTestInfo->inputBufferFileFd, 0, 0);
    }

    return error_code;
}


unsigned long char_to_long(char *buf, int len)
{
    int i;
    long frame_len = 0;

    for (i=0; i<len; i++)
    {
        frame_len += (buf[i] << 8*i);
    }

    return frame_len;
}

static int VP9_Read_EsRawStream(OmxTestInfo_S * pOmxTestInfo, OMX_BUFFERHEADERTYPE  *pBufHdr, char* pBuffer)
{
    int bytes_read = 0;
    char tmp_buf[64];
    unsigned long read_len = 0;

    if (0 == pOmxTestInfo->send_cnt)
    {
        bytes_read = fread(tmp_buf, 1, 32, pOmxTestInfo->inputBufferFileFd);
        if (bytes_read < 32)
        {
            SAMPLE_PRINT_INFO("Less than 32 byte(%d), stream send over!\n", bytes_read);
            pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        }
        if (memcmp("DKIF", tmp_buf, 4) != 0)
        {
            SAMPLE_PRINT_ERROR("VP9 file is not IVF file ERROR.\n");
            return 0;
        }
        pBufHdr->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
    }

        if (0 == pOmxTestInfo->frame_flag)
        {
            bytes_read = fread(tmp_buf, 1, 12, pOmxTestInfo->inputBufferFileFd);
            if (bytes_read < 12)
            {
                SAMPLE_PRINT_INFO("Less than 12 byte(%d), stream send over!\n", bytes_read);
                pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
            }
            else
            {
                pOmxTestInfo->frame_len  = tmp_buf[3] << 24;
                pOmxTestInfo->frame_len |= tmp_buf[2] << 16;
                pOmxTestInfo->frame_len |= tmp_buf[1] << 8;
                pOmxTestInfo->frame_len |= tmp_buf[0];

                if (pOmxTestInfo->frame_len > 0)
                {
                    pOmxTestInfo->frame_flag = 1;
                }
                else
                {
                    SAMPLE_PRINT_INFO("Frame Len(%lu) invalid, stream send over!\n", pOmxTestInfo->frame_len);
                    pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                }
            }
        }

       if (1 == pOmxTestInfo->frame_flag)
        {
            if (pOmxTestInfo->frame_len > pBufHdr->nAllocLen)
            {
                read_len =  pBufHdr->nAllocLen;
                pOmxTestInfo->frame_len = pOmxTestInfo->frame_len - read_len;
            }
            else
            {
                read_len = pOmxTestInfo->frame_len;
                pOmxTestInfo->frame_len = 0;
                pOmxTestInfo->frame_flag = 0;
            }

            bytes_read = fread(pBuffer, 1, read_len, pOmxTestInfo->inputBufferFileFd);
            if (bytes_read < read_len)
            {
                SAMPLE_PRINT_INFO("ReadByte(%d) < NeedByte(%lu), stream send over!\n", bytes_read, read_len);
                pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
            }

            if (0 == pOmxTestInfo->frame_flag)
            {
                pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            }
        }

    pOmxTestInfo->send_cnt++;

    return bytes_read;

}

static int DIVX3_Read_EsRawStream(OmxTestInfo_S * pOmxTestInfo, OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    int FrameSize;
    int len;
    int bytes_read = 0;
    char tmp_buf[64];
    OMX_U8* pBuffer = NULL;
    int read_len = 0;

    pBuffer = pBufHdr->pBuffer;

    if (0 == pOmxTestInfo->frame_flag)
    {
        bytes_read = fread(&FrameSize, 1, 4, pOmxTestInfo->inputBufferFileFd);
        if (bytes_read != 4)
        {
            SAMPLE_PRINT_ERROR("%s parse frame size failed, exit!\n", __func__);
            pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        }
        else
        {
            pOmxTestInfo->frame_len = FrameSize; //char_to_long(tmp_buf, 4);
            if (pOmxTestInfo->frame_len > 0)
            {
                pOmxTestInfo->frame_flag = 1;
            }
            else
            {
                SAMPLE_PRINT_INFO("Frame Len(%ld) invalid, stream send over!\n", pOmxTestInfo->frame_len);
                pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
            }
        }
    }

    if (1 == pOmxTestInfo->frame_flag)
    {
        len = fread(tmp_buf, 1, 8, pOmxTestInfo->inputBufferFileFd);
        if(len != 8)
        {
            SAMPLE_PRINT_INFO("Frame width add Frame Hight Len(%d) invalid, stream send over!\n", len);
            pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        }

        if (pOmxTestInfo->frame_len > pBufHdr->nAllocLen)
        {
            read_len =  pBufHdr->nAllocLen;
            pOmxTestInfo->frame_len = pOmxTestInfo->frame_len - read_len;
        }
        else
        {
            read_len = pOmxTestInfo->frame_len - len;
            pOmxTestInfo->frame_len = 0;
            pOmxTestInfo->frame_flag = 0;
        }

        bytes_read = fread(pBuffer, 1, read_len, pOmxTestInfo->inputBufferFileFd);
        if (bytes_read < read_len)
        {
            SAMPLE_PRINT_INFO("ReadByte(%d) < NeedByte(%d), stream send over!\n", bytes_read, read_len);
            pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        }

        if (0 == pOmxTestInfo->frame_flag)
        {
            pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        }
    }

    pOmxTestInfo->send_cnt++;

    return bytes_read;

}


static void HandleEos(OmxTestInfo_S *pOmxTestInfo, OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    if (pOmxTestInfo->rewind_enable != 1)
	{
		pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
	}
	else
	{
		pOmxTestInfo->read_pos = 0;
		rewind(pOmxTestInfo->inputBufferFileFd);
	}

	return;
}

int is_framepacket_standard(codec_format format);
static int Read_Es_WithTidx(OmxTestInfo_S *pOmxTestInfo, OMX_BUFFERHEADERTYPE *pBufHdr, char *pBuffer)
{
    OMX_U8 ReachEOS     = 0;
    int    ReadBytes    = 0;
    int    PreReadBytes = 0;
    int ExpectedReadSize = 0;
    int frame_offset = 0;
    char tmp_buf[64];

    if (0 == is_framepacket_standard(pOmxTestInfo->codec_format_option))
    {
        ExpectedReadSize = pBufHdr->nAllocLen;
    }
    else
    {
        // read frame size in tidx file
        if (0 != pOmxTestInfo->stContext.stFrameInfotidx[pOmxTestInfo->read_pos].FrameSize)
        {
            ExpectedReadSize = pOmxTestInfo->stContext.stFrameInfotidx[pOmxTestInfo->read_pos].FrameSize;
        }

        // read offset in tidx file
        frame_offset = pOmxTestInfo->stContext.stFrameInfotidx[pOmxTestInfo->read_pos].Frameoffset;
        if (0 != frame_offset)
        {
            if(CODEC_FORMAT_DIVX3 == pOmxTestInfo->codec_format_option)
            {
                fread(tmp_buf, 1, 4, pOmxTestInfo->inputBufferFileFd);
                fseek(pOmxTestInfo->inputBufferFileFd, frame_offset, 0);

            }
            else
            {
                fseek(pOmxTestInfo->inputBufferFileFd, frame_offset, 0);
            }
        }

        pOmxTestInfo->read_pos++;

        if (pOmxTestInfo->stContext.stFrameInfotidx[pOmxTestInfo->read_pos].FrameNum <= 0)
        {
            SAMPLE_PRINT_INFO("FrameNum(%d) in FrameInfo invalid, stream send over!\n",
                             pOmxTestInfo->stContext.stFrameInfotidx[pOmxTestInfo->read_pos].FrameNum);
            ReachEOS = 1;
        }
    }

    if (ExpectedReadSize > (pBufHdr->nAllocLen-PreReadBytes))
    {
        SAMPLE_PRINT_ERROR("NeedLen(%d) > AllocLen(%ld)!\n", ExpectedReadSize, pBufHdr->nAllocLen-PreReadBytes);
        return 0;
    }

    ReadBytes = fread(pBuffer, 1, ExpectedReadSize, pOmxTestInfo->inputBufferFileFd);
    if (ReadBytes < (int)ExpectedReadSize)
    {
        SAMPLE_PRINT_INFO("ReadByte(%d) < FrameSize(%d), stream send over!\n", ReadBytes, ExpectedReadSize);
        ReachEOS = 1;
    }
    ReadBytes += PreReadBytes;

    pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

    if (ReachEOS)
    {
        HandleEos(pOmxTestInfo, pBufHdr);
    }

    return ReadBytes;
}


static int Read_Buffer_from_EsRawStream(OmxTestInfo_S * pOmxTestInfo, OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    int bytes_read = 0;
    char tmp_buf[4];
    char *pBuffer = NULL;

    unsigned long read_len = 0;

#ifdef HI_TVP_SUPPORT
    if (1 == pOmxTestInfo->tvp_option)
    {
        pBuffer = (char *)pOmxTestInfo->pCAStreamBuf.user_viraddr;
    }
    else
#endif
    {
        pBuffer = (char *)pBufHdr->pBuffer;
    }

    /* VP9读码流比较特殊，单独走一个分支 */
    if (CODEC_FORMAT_VP9 == pOmxTestInfo->codec_format_option || CODEC_FORMAT_AV1 == pOmxTestInfo->codec_format_option)
    {
        bytes_read = VP9_Read_EsRawStream(pOmxTestInfo, pBufHdr, pBuffer);
        goto EXIT;
    }

    /* DIVX3单独走一个分支 */
    if (CODEC_FORMAT_DIVX3 == pOmxTestInfo->codec_format_option)
    {
        bytes_read = DIVX3_Read_EsRawStream(pOmxTestInfo, pBufHdr);
        goto EXIT;
    }

     /* 读取码流的长度，有些码流的长度在前四个字节 */
    if (true == pOmxTestInfo->readsize_add_in_stream)
    {
       if (0 == pOmxTestInfo->frame_flag)
       {
           bytes_read = fread(tmp_buf, 1, 4, pOmxTestInfo->inputBufferFileFd);
           if (bytes_read < 4)
           {
               SAMPLE_PRINT_INFO("Less than 4 byte(%d), stream send over!\n", bytes_read);
               pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
           }
           else
           {
               pOmxTestInfo->frame_len = char_to_long(tmp_buf, 4);
               if (pOmxTestInfo->frame_len > 0)
               {
                   pOmxTestInfo->frame_flag = 1;
               }
               else
               {
                   SAMPLE_PRINT_INFO("Frame Len(%ld) invalid, stream send over!\n", pOmxTestInfo->frame_len);
                   pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
               }
           }
       }

       if (1 == pOmxTestInfo->frame_flag)
       {
           if (pOmxTestInfo->frame_len > pBufHdr->nAllocLen)
           {
               read_len =  pBufHdr->nAllocLen;
               pOmxTestInfo->frame_len = pOmxTestInfo->frame_len - read_len;
           }
           else
           {
               read_len = pOmxTestInfo->frame_len;
               pOmxTestInfo->frame_len = 0;
               pOmxTestInfo->frame_flag = 0;
           }

           bytes_read = fread(pBuffer, 1, read_len, pOmxTestInfo->inputBufferFileFd);
           if (bytes_read < (int)read_len)
           {
               SAMPLE_PRINT_INFO("ReadByte(%d) < NeedByte(%ld), stream send over!\n", bytes_read, read_len);
               pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
           }

           if (0 == pOmxTestInfo->frame_flag)
           {
               pOmxTestInfo->send_cnt++;
               pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
           }
       }
    }
    else
    {    /* 当按帧大小送流时，从tidx文件读取每次送流的大小 */
        if (true == pOmxTestInfo->frame_in_packet)
        {
            bytes_read = Read_Es_WithTidx(pOmxTestInfo, pBufHdr, pBuffer);
        }
        else
        {
            bytes_read = fread(pBuffer, 1, pBufHdr->nAllocLen, pOmxTestInfo->inputBufferFileFd);
            if (bytes_read < (int)pBufHdr->nAllocLen)
            {
                SAMPLE_PRINT_INFO("ReadByte(%d) < AllocLen(%ld), stream send over!\n", bytes_read, pBufHdr->nAllocLen);
                pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
            }
            pOmxTestInfo->send_cnt++;
        }
    }

#ifdef HI_TVP_SUPPORT
    if (1 == pOmxTestInfo->tvp_option && bytes_read > 0)
    {
    #ifdef HI_SMMU_SUPPORT
        TEEC_Result result;
        TEEC_Operation operation;

        operation.started = 1;
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);
        operation.params[0].value.a = (hi_u32)(long)(pBufHdr->pBuffer);
        operation.params[0].value.b = (hi_u32)(pOmxTestInfo->pCAStreamBuf.phyaddr);
        operation.params[1].value.a = bytes_read;
        operation.params[1].value.b = TEEC_VALUE_UNDEF;

        SAMPLE_PRINT_INFO("\nCA2TA ca phy:%d sec vir:%p len:%d\n",
            (pOmxTestInfo->pCAStreamBuf.phyaddr), pBufHdr->pBuffer, bytes_read);

        result = TEEC_InvokeCommand(&g_teeSession, TEST_MEMCPY_CMD_ID, &operation, HI_NULL);
        if (TEEC_SUCCESS != result)
        {
            SAMPLE_PRINT_ERROR("RawPlay sec memcpy Failed!\n");
            exit(0);
        }

    #else
        HI_SEC_MMZ_CA2TA((unsigned long)(pOmxTestInfo->pCAStreamBuf.phyaddr),(unsigned long)(pBufHdr->pBuffer), bytes_read);
    #endif
    }
#endif

EXIT:
    return bytes_read;
}


/*************************************************************************
                omx interface functions
************************************************************************/

OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                           OMX_IN OMX_PTR pAppData,
                           OMX_IN OMX_EVENTTYPE eEvent,
                           OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{
    OmxTestInfo_S *pOmxTestInfo = NULL;
    int index = 0;

    for (index = 0; index < g_TestInstNum; index++)
    {
        if (OmxTestInfo[index].dec_handle == (OMX_COMPONENTTYPE *)hComponent)
        {
            pOmxTestInfo = &OmxTestInfo[index];
            break;
        }
    }

    if (index == g_TestInstNum)
    {
        SAMPLE_PRINT_ERROR("invalid parameter!\n");
        return OMX_ErrorBadParameter;
    }

    switch(eEvent)
    {
    case OMX_EventCmdComplete:
        SAMPLE_PRINT_INFO("Received OMX_EventCmdComplete\n");

        if(OMX_CommandPortDisable == (OMX_COMMANDTYPE)nData1)
        {
            event_complete(pOmxTestInfo, nData1);
        }
        else if(OMX_CommandPortEnable == (OMX_COMMANDTYPE)nData1)
        {
            pOmxTestInfo->sent_disabled = 0;
            event_complete(pOmxTestInfo, nData1);
        }
        else if(OMX_CommandFlush == (OMX_COMMANDTYPE)nData1)
        {
            if (nData2 == 0) {
                pOmxTestInfo->flush_input_progress = 0;
                sem_post(&pOmxTestInfo->in_flush_sem);
            }
            else if (nData2 == 1) {
                pOmxTestInfo->flush_output_progress = 0;
                sem_post(&pOmxTestInfo->out_flush_sem);
            }

            if (!pOmxTestInfo->flush_input_progress && !pOmxTestInfo->flush_output_progress)
            {
                event_complete(pOmxTestInfo, nData1);
            }
        }
        else
        {
            event_complete(pOmxTestInfo, nData1);
        }
        break;

    case OMX_EventError:
        SAMPLE_PRINT_INFO("Received OMX_EventError\n");

        if ((OMX_ERRORTYPE)nData1 == OMX_ErrorUndefined)
        {
            break;
        }

        pthread_mutex_lock(&pOmxTestInfo->status_lock);
        pOmxTestInfo->currentStatus = ERROR_STATE;
        pthread_mutex_unlock(&pOmxTestInfo->status_lock);

        if ((OMX_ERRORTYPE)nData1 == OMX_ErrorIncorrectStateOperation
         || (OMX_ERRORTYPE)nData1 == OMX_ErrorHardware)
        {
            SAMPLE_PRINT_ERROR("Invalid State or hardware error\n");

            if(pOmxTestInfo->event_is_done == 0)
            {
                SAMPLE_PRINT_INFO("Event error in the middle of Decode\n");
                pOmxTestInfo->bOutputEosReached = true;
            }
        }

        event_complete(pOmxTestInfo, -1);
        break;

    case OMX_EventPortSettingsChanged:
        SAMPLE_PRINT_INFO("Received OMX_EventPortSettingsChanged\n");

        pthread_mutex_lock(&pOmxTestInfo->status_lock);
        pOmxTestInfo->preStatus = pOmxTestInfo->currentStatus;
        pOmxTestInfo->currentStatus = PORT_SETTING_CHANGE_STATE;
        pthread_mutex_unlock(&pOmxTestInfo->status_lock);

        sem_post(&pOmxTestInfo->ftb_sem);
        break;

    case OMX_EventBufferFlag:
        if (nData1 == 1 && (nData2 & OMX_BUFFERFLAG_EOS))
        {
            pOmxTestInfo->bOutputEosReached = true;

        #ifdef CalcFrameRate
            if (!g_PrintAlready)
            {
                int spend_time_ms = 0;

                for (index = 0; index < g_TestInstNum; index++)
                {
                    OmxTestInfo[index].stop_time = GetTimeInMs();

                    if (OmxTestInfo[index].stop_time > OmxTestInfo[index].start_time)
                    {
                        spend_time_ms = OmxTestInfo[index].stop_time - OmxTestInfo[index].start_time;

                        SAMPLE_PRINT_INFO("INSTANCE NO:%d fram rate:%f\n", OmxTestInfo[index].inst_no,\
                                            (float)(OmxTestInfo[index].receive_frame_cnt*1000)/spend_time_ms);
                    }
                }
                g_PrintAlready = true;
            }
        #endif
            SAMPLE_PRINT_INFO("Inst %d receive last frame\n", pOmxTestInfo->inst_no);
        }
        else
        {
            SAMPLE_PRINT_INFO("OMX_EventBufferFlag Event not handled\n");
        }
        break;

    default:
        SAMPLE_PRINT_ERROR("ERROR - Unknown Event\n");
        break;
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    int index     = 0;
    OmxTestInfo_S *pOmxTestInfo = NULL;

    for (index = 0; index < g_TestInstNum; index++)
    {
        if (OmxTestInfo[index].dec_handle == (OMX_COMPONENTTYPE *)hComponent)
        {
            pOmxTestInfo = &OmxTestInfo[index];
            break;
        }
    }

    if (index == g_TestInstNum)
    {
        SAMPLE_PRINT_ERROR("invalid parameter!\n");
        return OMX_ErrorBadParameter;
    }

    pOmxTestInfo->free_ip_buf_cnt++;

    if(pOmxTestInfo->bInputEosReached)
    {
        SAMPLE_PRINT_INFO("EBD: Input EoS Reached.\n");
        return OMX_ErrorNone;
    }

    if(pOmxTestInfo->seeking_progress)
    {
        SAMPLE_PRINT_INFO("EBD: Seeking Pending.\n");
        return OMX_ErrorNone;
    }

    if(pOmxTestInfo->flush_input_progress)
    {
        SAMPLE_PRINT_INFO("EBD: Input Flushing.\n");
        return OMX_ErrorNone;
    }

    pthread_mutex_lock(&pOmxTestInfo->etb_lock);
    if(push(pOmxTestInfo->etb_queue, (void *) pBuffer) < 0)
    {
        SAMPLE_PRINT_ERROR("Error in enqueue  ebd data\n");
        return OMX_ErrorUndefined;
    }

    pthread_mutex_unlock(&pOmxTestInfo->etb_lock);
    sem_post(&pOmxTestInfo->etb_sem);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                             OMX_OUT OMX_PTR pAppData,
                             OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    /* Test app will assume there is a dynamic port setting
    * In case that there is no dynamic port setting, OMX will not call event cb,
    * instead OMX will send empty this buffer directly and we need to clear an event here
    */
    OmxTestInfo_S *pOmxTestInfo = NULL;
    int index = 0;

    for (index = 0; index < g_TestInstNum; index++)
    {
        if (OmxTestInfo[index].dec_handle == (OMX_COMPONENTTYPE *)hComponent)
        {
            pOmxTestInfo = &OmxTestInfo[index];
            break;
        }
    }

    if (index == g_TestInstNum)
    {
        SAMPLE_PRINT_ERROR("invalid parameter!\n");
        return OMX_ErrorBadParameter;
    }

    pOmxTestInfo->free_op_buf_cnt++;

    if (pBuffer->nFilledLen != 0)
    {
        gettimeofday(&pOmxTestInfo->t_cur_get, NULL);

    #ifdef CalcFrameRate
        if (pOmxTestInfo->receive_frame_cnt == 0)
        {
            pOmxTestInfo->start_time = GetTimeInMs();
        }
    #endif

        SAMPLE_PRINT_INFO("Inst %d: Frame %d\n", pOmxTestInfo->inst_no, pOmxTestInfo->receive_frame_cnt);

        pOmxTestInfo->receive_frame_cnt++;
    }

    if (pOmxTestInfo->flush_output_progress)
    {
        return OMX_ErrorNone;
    }

    pthread_mutex_lock(&pOmxTestInfo->ftb_lock);

    if (!pOmxTestInfo->sent_disabled)
    {
        if(push(pOmxTestInfo->ftb_queue, (void *)pBuffer) < 0)
        {
            pthread_mutex_unlock(&pOmxTestInfo->ftb_lock);
            SAMPLE_PRINT_ERROR("Error in enqueueing fbd_data\n");
            return OMX_ErrorUndefined;
        }
        sem_post(&pOmxTestInfo->ftb_sem);
    }
    else
    {
       if (0 == pOmxTestInfo->is_use_buffer)
       {
           OMX_FreeBuffer(pOmxTestInfo->dec_handle, 1, pBuffer);
       }
       else
       {
           OMX_FreeBuffer(pOmxTestInfo->dec_handle, 1, pBuffer);
           hi_unf_mem_free(&pOmxTestInfo->buffer[pBuffer->nTickCount]);
       }
    }

    pthread_mutex_unlock(&pOmxTestInfo->ftb_lock);

    if (pBuffer->nFilledLen != 0)
    {
        gettimeofday(&pOmxTestInfo->t_cur_get, NULL);
        memcpy(&pOmxTestInfo->t_last_get, &pOmxTestInfo->t_cur_get, sizeof(struct timeval));
    }

    return OMX_ErrorNone;
}


static int Init_Decoder(OmxTestInfo_S * pOmxTestInfo)
{
    OMX_ERRORTYPE omxresult;
    OMX_U32 total = 0;
    OMX_U32 i = 0, is_found = 0;

    char vdecCompNames[MAX_COMP_NAME_LEN];// = "OMX.hisi.video.decoder";
    char compRole[OMX_MAX_STRINGNAME_SIZE];

    OMX_CALLBACKTYPE call_back = {
        &EventHandler,
        &EmptyBufferDone,
        &FillBufferDone
    };

    /* Init. the OpenMAX Core */
    omxresult = OMX_Init();
    if(OMX_ErrorNone != omxresult) {
        SAMPLE_PRINT_ERROR("Failed to Init OpenMAX core\n");
        return -1;
    }

    /* CodecType Relative */
    if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_HEVC)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_HEVC, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingHEVC;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_MVC)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_MVC, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMVC;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_H264)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_AVC, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VC1AP)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VC1, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVC1;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_MP4)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_MPEG4, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_MP1)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_MPEG1, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG1;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_MP2)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_MPEG2, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_H263)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_H263, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_DIVX3)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_DIVX3, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingDIVX3;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP8)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VP8, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP6)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VP6, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVP6;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP6F)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VP6, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVP6;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP6A)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VP6, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVP6;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VC1SMP)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VC1, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVC1;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_AVS)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_AVS, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAVS;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_AVS2)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_AVS2, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAVS2;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_SORENSON)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_SORENSON, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingSorenson;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_REAL8)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_RV, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingRV;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_REAL9)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_RV, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingRV;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_MJPEG)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_MJPEG, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMJPEG;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP9)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_VP9, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingVP9;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_AVS3)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_AVS3, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAVS3;
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_AV1)
    {
        strncpy(compRole, OMX_ROLE_VIDEO_DECODER_AV1, OMX_MAX_STRINGNAME_SIZE);
        pOmxTestInfo->portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAV1;
    }
    else
    {
        SAMPLE_PRINT_ERROR("Error: Unsupported codec %d\n", pOmxTestInfo->codec_format_option);
        return -1;
    }

    get_componet_name_by_role(vdecCompNames, compRole);

#ifdef HI_TVP_SUPPORT
    if (1 == pOmxTestInfo->tvp_option)
    {
        strncat(vdecCompNames, ".secure", 10);
    }
#endif

    for( i = 0; ; i++ )
    {
        char enumCompName[OMX_MAX_STRINGNAME_SIZE];
        memset(enumCompName, 0 , OMX_MAX_STRINGNAME_SIZE);

        omxresult = OMX_ComponentNameEnum(enumCompName,
            OMX_MAX_STRINGNAME_SIZE, i);

        if(OMX_ErrorNone != omxresult)
            break;

        if (!strncmp(enumCompName, vdecCompNames, OMX_MAX_STRINGNAME_SIZE))
        {
            is_found = 1;
            break;
        }
    }

    if (!is_found)
    {
        SAMPLE_PRINT_ERROR("Error: cannot find match component!\n");
        return -1;
    }

    /* Query for video decoders*/
    is_found = 0;
    OMX_GetRolesOfComponent(vdecCompNames, &total, NULL);

    if (total)
    {
        /* Allocate memory for pointers to component name */
        char **role = (char**)malloc((sizeof(char*)) * total);

        for (i = 0; i < total; ++i)
        {
            role[i] = (char*)malloc(sizeof(char) * OMX_MAX_STRINGNAME_SIZE);
        }

        OMX_GetRolesOfComponent(vdecCompNames, &total, (OMX_U8 **)role);

        for (i = 0; i < total; ++i)
        {
            if (!strncmp(role[i], compRole, OMX_MAX_STRINGNAME_SIZE))
            {
                is_found = 1;
            }

            free(role[i]);
        }

        free(role);

        if (!is_found)
        {
            SAMPLE_PRINT_ERROR("No Role found \n");
            return -1;
        }
    }
    else
    {
        SAMPLE_PRINT_ERROR("No components found with Role:%s\n", vdecCompNames);
        return -1;
    }

    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&pOmxTestInfo->dec_handle),
                          (OMX_STRING)vdecCompNames, NULL, &call_back);
    if (FAILED(omxresult))
    {
        SAMPLE_PRINT_ERROR("Failed to Load the component:%s\n", vdecCompNames);
        return -1;
    }

    CONFIG_VERSION_SIZE(pOmxTestInfo->role);
    strncpy((char*)pOmxTestInfo->role.cRole, compRole, OMX_MAX_STRINGNAME_SIZE);

    omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle,
            OMX_IndexParamStandardComponentRole, &pOmxTestInfo->role);
    if(FAILED(omxresult))
    {
        SAMPLE_PRINT_ERROR("ERROR - Failed to set param!\n");
        return -1;
    }

    omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle,
            OMX_IndexParamStandardComponentRole, &pOmxTestInfo->role);

    if(FAILED(omxresult))
    {
        SAMPLE_PRINT_ERROR("ERROR - Failed to get role!\n");
        return -1;
    }

    /* CodecType Relative */
    OMX_VIDEO_PARAM_VP6TYPE vp6_param;
    OMX_VIDEO_PARAM_VC1TYPE vc1_param;
    OMX_VIDEO_PARAM_RVTYPE  rv_param;
    if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP6)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVp6, &vp6_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        vp6_param.eFormat = OMX_VIDEO_VP6;
        vp6_param.nSize   = sizeof(OMX_VIDEO_PARAM_VP6TYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVp6, &vp6_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP6F)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVp6, &vp6_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        vp6_param.eFormat = OMX_VIDEO_VP6F;
        vp6_param.nSize   = sizeof(OMX_VIDEO_PARAM_VP6TYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVp6, &vp6_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VP6A)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVp6, &vp6_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        vp6_param.eFormat = OMX_VIDEO_VP6A;
        vp6_param.nSize   = sizeof(OMX_VIDEO_PARAM_VP6TYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVp6, &vp6_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VC1AP)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVC1, &vc1_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        vc1_param.eProfile = OMX_VIDEO_VC1ProfileAdvanced;
        vc1_param.nSize   = sizeof(OMX_VIDEO_PARAM_VC1TYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVC1, &vc1_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_VC1SMP)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVC1, &vc1_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        vc1_param.eProfile = OMX_VIDEO_VC1ProfileMain;
        vc1_param.nSize   = sizeof(OMX_VIDEO_PARAM_VC1TYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoVC1, &vc1_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_REAL8)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoRv, &rv_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        rv_param.eFormat = OMX_VIDEO_RVFormat8;
        rv_param.nSize   = sizeof(OMX_VIDEO_PARAM_RVTYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoRv, &rv_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }
    else if (pOmxTestInfo->codec_format_option == CODEC_FORMAT_REAL9)
    {
        omxresult = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoRv, &rv_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to get Parameter!\n", __LINE__);
            return -1;
        }

        rv_param.eFormat = OMX_VIDEO_RVFormat9;
        rv_param.nSize   = sizeof(OMX_VIDEO_PARAM_RVTYPE);
        omxresult = OMX_SetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoRv, &rv_param);
        if(FAILED(omxresult))
        {
            SAMPLE_PRINT_ERROR("L%d: ERROR - Failed to set Parameter!\n", __LINE__);
            return -1;
        }
    }

    return 0;
}


static int Play_Decoder(OmxTestInfo_S * pOmxTestInfo)
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoportFmt = {0};
    int bufCnt, index = 0;
    int frameSize = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STATETYPE state = OMX_StateMax;

    /* open the i/p and o/p files*/
    if(open_video_file(pOmxTestInfo) < 0)
    {
        SAMPLE_PRINT_ERROR("Error in opening video file");
        return -1;
    }

    /* Get the port information */
    CONFIG_VERSION_SIZE(pOmxTestInfo->portParam);
    ret = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoInit,
                            (OMX_PTR)&pOmxTestInfo->portParam);
    if(FAILED(ret))
    {
        SAMPLE_PRINT_ERROR("ERROR - Failed to get Port Param\n");
        return -1;
    }

    /* Query the decoder input's  buf requirements */
    CONFIG_VERSION_SIZE(pOmxTestInfo->portFmt);
    pOmxTestInfo->portFmt.nPortIndex = pOmxTestInfo->portParam.nStartPortNumber;

    OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamPortDefinition, &pOmxTestInfo->portFmt);

    if(OMX_DirInput != pOmxTestInfo->portFmt.eDir)
    {
        SAMPLE_PRINT_ERROR ("Dec: Expect Input Port\n");
        return -1;
    }

    pOmxTestInfo->portFmt.format.video.nFrameWidth  = pOmxTestInfo->width;
    pOmxTestInfo->portFmt.format.video.nFrameHeight = pOmxTestInfo->height;
    pOmxTestInfo->portFmt.format.video.xFramerate   = 30<<16;

    OMX_SetParameter(pOmxTestInfo->dec_handle,OMX_IndexParamPortDefinition, &pOmxTestInfo->portFmt);

    /* get again to check */
    OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamPortDefinition, &pOmxTestInfo->portFmt);

    /* set component video fmt */
    CONFIG_VERSION_SIZE(videoportFmt);

    pOmxTestInfo->color_fmt = OMX_COLOR_FormatYVU420SemiPlanar;
    while (ret == OMX_ErrorNone)
    {
        videoportFmt.nPortIndex = 1;
        videoportFmt.nIndex = index;
        ret = OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamVideoPortFormat,
                (OMX_PTR)&videoportFmt);

        if((ret == OMX_ErrorNone) && (videoportFmt.eColorFormat ==
            pOmxTestInfo->color_fmt))
        {
            break;
        }
        index++;
    }

    if(ret != OMX_ErrorNone)
    {
        SAMPLE_PRINT_ERROR("Error in retrieving supported color formats\n");
        return -1;
    }

    ret = OMX_SetParameter(pOmxTestInfo->dec_handle,
                           OMX_IndexParamVideoPortFormat, (OMX_PTR)&videoportFmt);
    if (ret != OMX_ErrorNone)
    {
        SAMPLE_PRINT_ERROR("Setting Tile format failed\n");
        return -1;
    }

    OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StateIdle,0);

    // Allocate buffer on decoder's i/p port
    error = Allocate_Buffers(pOmxTestInfo, pOmxTestInfo->dec_handle, &pOmxTestInfo->pInputBufHdrs, pOmxTestInfo->portFmt.nPortIndex, \
                             pOmxTestInfo->portFmt.nBufferCountActual, pOmxTestInfo->portFmt.nBufferSize);
    if (error != OMX_ErrorNone)
    {
        SAMPLE_PRINT_ERROR("Error - OMX_AllocateBuffer Input buffer error\n");
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    SAMPLE_PRINT_ERROR("%s %d alloc in buffer ok\n", __func__, __LINE__);

    pOmxTestInfo->free_ip_buf_cnt = pOmxTestInfo->portFmt.nBufferCountActual;

    //Allocate buffer on decoder's o/p port
    pOmxTestInfo->portFmt.nPortIndex = pOmxTestInfo->portParam.nStartPortNumber + 1;
    OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamPortDefinition, &pOmxTestInfo->portFmt);
    OMX_SetParameter(pOmxTestInfo->dec_handle,OMX_IndexParamPortDefinition, &pOmxTestInfo->portFmt);

    /* get again to check */
    OMX_GetParameter(pOmxTestInfo->dec_handle, OMX_IndexParamPortDefinition, &pOmxTestInfo->portFmt);

    if(OMX_DirOutput != pOmxTestInfo->portFmt.eDir)
    {
        SAMPLE_PRINT_ERROR("Error - Expect Output Port\n");
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    SAMPLE_PRINT_ERROR("%s %d alloc OUT buffer BEFORE is_use_buffer = %d \n", __func__, __LINE__, pOmxTestInfo->is_use_buffer);

    if (0 == pOmxTestInfo->is_use_buffer)
    {
        error = Allocate_Buffers(pOmxTestInfo, pOmxTestInfo->dec_handle, &pOmxTestInfo->pOutYUVBufHdrs,
             pOmxTestInfo->portFmt.nPortIndex, pOmxTestInfo->portFmt.nBufferCountActual, pOmxTestInfo->portFmt.nBufferSize);
        if (error != OMX_ErrorNone)
        {
            SAMPLE_PRINT_ERROR("Error - OMX_AllocateBuffer Output buffer error\n");
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
            return -1;
        }
    }
    else
    {
        error = Use_Buffers(pOmxTestInfo, &pOmxTestInfo->pOutYUVBufHdrs, pOmxTestInfo->portFmt.nPortIndex, \
                    pOmxTestInfo->portFmt.nBufferCountActual, pOmxTestInfo->portFmt.nBufferSize);
        if (error != OMX_ErrorNone)
        {
            SAMPLE_PRINT_ERROR("Error - OMX_UseBuffer Output buffer error\n");
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
            return -1;
        }
    }

    SAMPLE_PRINT_ERROR("%s %d alloc OUT buffer ok\n", __func__, __LINE__);

    pOmxTestInfo->free_op_buf_cnt = pOmxTestInfo->portFmt.nBufferCountActual;

    /* wait component state change from loaded->idle */
    wait_for_event(pOmxTestInfo, OMX_CommandStateSet);
    if (pOmxTestInfo->currentStatus == ERROR_STATE)
    {
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    OMX_GetState(pOmxTestInfo->dec_handle, &state);
    if (state != OMX_StateIdle)
    {
        SAMPLE_PRINT_ERROR("Error - OMX state error\n");
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    if (pOmxTestInfo->freeHandle_option == FREE_HANDLE_AT_IDLE)
    {
        OMX_STATETYPE state = OMX_StateMax;
        OMX_GetState(pOmxTestInfo->dec_handle, &state);

        if (state == OMX_StateIdle)
        {
            do_freeHandle_and_clean_up(pOmxTestInfo, false);
        }
        else
        {
            SAMPLE_PRINT_ERROR("Comp: Decoder in state %d, "
                "trying to call OMX_FreeHandle\n", state);
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
        }
        return -1;
    }

    OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StateExecuting, 0);

    wait_for_event(pOmxTestInfo, OMX_CommandStateSet);

    if (pOmxTestInfo->currentStatus == ERROR_STATE)
    {
        SAMPLE_PRINT_ERROR("OMX_SendCommand Decoder -> Executing\n");
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    OMX_GetState(pOmxTestInfo->dec_handle, &state);

    if (state != OMX_StateExecuting)
    {
        SAMPLE_PRINT_ERROR("Error - OMX state error, state %d\n", state);
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    for(bufCnt = 0; bufCnt < pOmxTestInfo->used_op_buf_cnt; ++bufCnt)
    {
        pOmxTestInfo->pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
        pOmxTestInfo->pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;

        ret = OMX_FillThisBuffer(pOmxTestInfo->dec_handle, pOmxTestInfo->pOutYUVBufHdrs[bufCnt]);

        if (OMX_ErrorNone != ret)
        {
            SAMPLE_PRINT_ERROR("Error - OMX_FillThisBuffer failed!!\n");
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
            return -1;
        }
        else
        {
            pOmxTestInfo->free_op_buf_cnt--;
        }
    }

    for (bufCnt = 0; bufCnt < pOmxTestInfo->used_ip_buf_cnt; bufCnt++)
    {
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nInputPortIndex = 0;
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nOffset = 0;
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nFlags = 0;

        frameSize = Read_Buffer_from_EsRawStream(pOmxTestInfo, pOmxTestInfo->pInputBufHdrs[bufCnt]);
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nFilledLen = frameSize;
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nInputPortIndex = 0;

        ret = OMX_EmptyThisBuffer(pOmxTestInfo->dec_handle, pOmxTestInfo->pInputBufHdrs[bufCnt]);
        if (OMX_ErrorNone != ret)
        {
            SAMPLE_PRINT_ERROR("ERROR: OMX_EmptyThisBuffer failed\n");
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
            return -1;
        }
        else
        {
            if (bufCnt == 0)
            {
                gettimeofday(&pOmxTestInfo->t_first_send, NULL);
            }
            pOmxTestInfo->free_ip_buf_cnt--;
        }

        if (pOmxTestInfo->pInputBufHdrs[bufCnt]->nFlags & OMX_BUFFERFLAG_EOS)
        {
            pOmxTestInfo->bInputEosReached = true;
            break;
        }
    }

    if(0 != pthread_create(&pOmxTestInfo->ebd_thread_id, NULL, ebd_thread, pOmxTestInfo))
    {
        SAMPLE_PRINT_ERROR("Error in Creating fbd_thread\n");
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    if (pOmxTestInfo->freeHandle_option == FREE_HANDLE_AT_EXECUTING)
    {
        OMX_STATETYPE state = OMX_StateMax;
        OMX_GetState(pOmxTestInfo->dec_handle, &state);
        if (state == OMX_StateExecuting)
        {
            do_freeHandle_and_clean_up(pOmxTestInfo, false);
        }
        else
        {
            SAMPLE_PRINT_ERROR("Error: state %d , trying OMX_FreeHandle\n", state);
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
        }
        return -1;
    }
    else if (pOmxTestInfo->freeHandle_option == FREE_HANDLE_AT_PAUSE)
    {
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StatePause,0);
        wait_for_event(pOmxTestInfo, OMX_CommandStateSet);

        OMX_STATETYPE state = OMX_StateMax;
        OMX_GetState(pOmxTestInfo->dec_handle, &state);

        if (state == OMX_StatePause)
        {
            do_freeHandle_and_clean_up(pOmxTestInfo, false);
        }
        else
        {
            SAMPLE_PRINT_ERROR("Error - Decoder is in state %d ,call OMX_FreeHandle\n", state);
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
        }
        return -1;
    }

    return 0;
}

/*
static int get_next_command(OmxTestInfo_S * pOmxTestInfo, FILE *seq_file)
{
    int i = -1;

    do {
        i++;
        if(fread(curr_seq_command[i], 1, 1, seq_file) != 1)
            return -1;
    } while(curr_seq_command[i] != '\n');

    curr_seq_command[i] = 0;

    return 0;
}
*/

static int process_current_command(OmxTestInfo_S * pOmxTestInfo, const char *seq_command)
{
    if (pOmxTestInfo->currentStatus == PORT_SETTING_CHANGE_STATE)
    {
        SAMPLE_PRINT_ERROR("\nCurrent Status = PORT_SETTING_CHANGE_STATE, Command %s Rejected !\n", seq_command);
        return 0;
    }

    if(strcmp(seq_command, "p") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   PAUSE    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending PAUSE cmd to OMX compt\n");
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StatePause,0);
        wait_for_event(pOmxTestInfo, OMX_CommandStateSet);
        SAMPLE_PRINT_INFO("EventHandler for PAUSE DONE\n\n");
    }
    else if(strcmp(seq_command, "r") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   RESUME    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending RESUME cmd to OMX compt\n");
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
        wait_for_event(pOmxTestInfo, OMX_CommandStateSet);
        SAMPLE_PRINT_INFO("EventHandler for RESUME DONE\n\n");
    }
    else if(strcmp(seq_command, "s") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   SEEK    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending SEEK cmd to OMX compt\n");
        pOmxTestInfo->seeking_progress = 1;
        pOmxTestInfo->flush_input_progress = 1;
        pOmxTestInfo->flush_output_progress = 1;
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandFlush, OMX_ALL, 0);
        wait_for_event(pOmxTestInfo, OMX_CommandFlush);

        seek_progress(pOmxTestInfo);
        pOmxTestInfo->seeking_progress = 0;
        sem_post(&pOmxTestInfo->seek_sem);
        SAMPLE_PRINT_INFO("EventHandler for SEEK DONE\n\n");
    }
    else if(strcmp(seq_command, "f") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   FLUSH    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending FLUSH cmd to OMX compt\n");

        pOmxTestInfo->flush_input_progress = 1;
        pOmxTestInfo->flush_output_progress = 1;
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandFlush, OMX_ALL, 0);
        wait_for_event(pOmxTestInfo, OMX_CommandFlush);
        SAMPLE_PRINT_INFO("Sending FLUSH cmd DONE\n\n");
    }
    else if(strcmp(seq_command, "f0") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   FLUSH-IN    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending FLUSH-IN cmd to OMX compt\n");

        pOmxTestInfo->flush_input_progress = 1;
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandFlush, 0, 0);
        wait_for_event(pOmxTestInfo, OMX_CommandFlush);
        SAMPLE_PRINT_INFO("Sending FLUSH-IN cmd DONE\n\n");
    }
    else if(strcmp(seq_command, "f1") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   FLUSH-OUT    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending FLUSH-OUT cmd to OMX compt\n");

        pOmxTestInfo->flush_output_progress = 1;
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandFlush, 1, 0);
        wait_for_event(pOmxTestInfo, OMX_CommandFlush);
        SAMPLE_PRINT_INFO("Sending FLUSH-OUT cmd DONE\n\n");
    }
    /*else if(strcmp(seq_command, "disable-op") == 0)
    {
        SAMPLE_PRINT_INFO("$$$$$   DISABLE OP PORT   $$$$$\n");
        SAMPLE_PRINT_INFO("Sending DISABLE OP cmd to OMX compt\n");

        if (disable_output_port() != 0)
        {
            SAMPLE_PRINT_INFO("ERROR: While DISABLE OP...\n");
            do_freeHandle_and_clean_up(true);
            return -1;
        }
        SAMPLE_PRINT_INFO("DISABLE OP PORT SUCESS!\n\n");
    }
    else if(strstr(seq_command, "enable-op") == seq_command)
    {
        SAMPLE_PRINT_INFO("$$$$$   ENABLE OP PORT    $$$$$\n");
        SAMPLE_PRINT_INFO("Sending ENABLE OP cmd to OMX compt\n");

        if (enable_output_port() != 0)
        {
            SAMPLE_PRINT_INFO("ERROR: While ENABLE OP...\n");
            do_freeHandle_and_clean_up(true);
            return -1;
        }
        SAMPLE_PRINT_INFO("ENABLE OP PORT SUCESS!\n\n");
    }*/
    else
    {
        SAMPLE_PRINT_ERROR("$$$$$   INVALID CMD [%s]   $$$$$\n", seq_command);
    }

    return 0;
}


static void* ebd_thread(void* pArg)
{
    OmxTestInfo_S * pOmxTestInfo = (OmxTestInfo_S *)pArg;

    while(pOmxTestInfo->currentStatus != ERROR_STATE)
    {
        int readBytes =0;
        OMX_BUFFERHEADERTYPE* pBuffer = NULL;

        pOmxTestInfo->EtbStatus = THREAD_WAITING;

        if(pOmxTestInfo->flush_input_progress)
        {
            sem_wait(&pOmxTestInfo->in_flush_sem);
        }

        sem_wait(&pOmxTestInfo->etb_sem);

        if(pOmxTestInfo->seeking_progress)
        {
            sem_wait(&pOmxTestInfo->seek_sem);
        }

        if (pOmxTestInfo->ebd_thread_exit)
            break;

        if (pOmxTestInfo->bInputEosReached)
            continue;

        pthread_mutex_lock(&pOmxTestInfo->etb_lock);
        pBuffer = (OMX_BUFFERHEADERTYPE *) pop(pOmxTestInfo->etb_queue);
        pthread_mutex_unlock(&pOmxTestInfo->etb_lock);

        if(pBuffer == NULL)
        {
            SAMPLE_PRINT_ERROR("Error - No etb pBuffer to dequeue\n");
            continue;
        }

        pOmxTestInfo->EtbStatus = THREAD_RUNNING;

        pBuffer->nOffset = 0;
        pBuffer->nFlags  = 0;

        readBytes = Read_Buffer_from_EsRawStream(pOmxTestInfo, pBuffer);

        if(pOmxTestInfo->seeking_progress)
        {
            SAMPLE_PRINT_INFO("Read es done meet seeking.\n");
            continue;
        }

        if(pOmxTestInfo->flush_input_progress)
        {
            SAMPLE_PRINT_INFO("Read es done meet input flushing.\n");
            continue;
        }

        if (pOmxTestInfo->ebd_thread_exit)
        {
            break;
        }

        if(readBytes > 0)
        {
            if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            {
                pOmxTestInfo->bInputEosReached = true;
            }
        }
        else
        {
            SAMPLE_PRINT_INFO("EBD::Either EOS or Some Error while reading file\n");
            pOmxTestInfo->bInputEosReached = true;
        }

        pBuffer->nFilledLen = readBytes;
        OMX_EmptyThisBuffer(pOmxTestInfo->dec_handle,pBuffer);
        pOmxTestInfo->free_ip_buf_cnt--;
    }

    pOmxTestInfo->EtbStatus = THREAD_INVALID;

    return NULL;
}


static void* fbd_thread(void* pArg)
{
    OmxTestInfo_S * pOmxTestInfo = (OmxTestInfo_S *)pArg;
    OMX_BUFFERHEADERTYPE *pBuffer = NULL;

    while(pOmxTestInfo->currentStatus != ERROR_STATE)
    {
        pOmxTestInfo->FtbStatus = THREAD_WAITING;

        if(pOmxTestInfo->flush_output_progress)
        {
            sem_wait(&pOmxTestInfo->out_flush_sem);
        }

        {
            int ret;
            struct timespec ts;
            ts.tv_sec = time(NULL) + 1;
            ts.tv_nsec = 0;
            //sem_wait(&pOmxTestInfo->ftb_sem);
            ret = sem_timedwait(&pOmxTestInfo->ftb_sem, &ts);
            if (ret == -1)
            {
                if (errno == ETIMEDOUT)
                {
                    //SAMPLE_PRINT_ERROR("\n\n sem_timedwait() timed out\n\n\n");
                }
                continue;
            }
        }

        if (pOmxTestInfo->fbd_thread_exit)
        {
            break;
        }

        pthread_mutex_lock(&pOmxTestInfo->status_lock);

        if (pOmxTestInfo->currentStatus == PORT_SETTING_CHANGE_STATE)
        {
            pOmxTestInfo->currentStatus = PORT_SETTING_RECONFIG_STATE;
            if (output_port_reconfig(pOmxTestInfo) != 0)
            {
                pOmxTestInfo->currentStatus = pOmxTestInfo->preStatus;
                do_freeHandle_and_clean_up(pOmxTestInfo, true);
                break;
            }
            pOmxTestInfo->currentStatus = pOmxTestInfo->preStatus;
        }
        pthread_mutex_unlock(&pOmxTestInfo->status_lock);

        if (pOmxTestInfo->fbd_thread_exit)
        {
            break;
        }

        pthread_mutex_lock(&pOmxTestInfo->ftb_lock);
        pBuffer = (OMX_BUFFERHEADERTYPE *)pop(pOmxTestInfo->ftb_queue);
        pthread_mutex_unlock(&pOmxTestInfo->ftb_lock);

        if (!pBuffer)
        {
            continue;
        }

        pOmxTestInfo->FtbStatus = THREAD_RUNNING;

        /********************************************************************/
        /* De-Initializing the open max and relasing the buffers and */
        /* closing the files.*/
        /********************************************************************/
        if (pOmxTestInfo->flush_output_progress)
        {
        #if 0  // continue to send buffer after output flush

                pBuffer->nFilledLen = 0;
                pBuffer->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;

                pthread_mutex_lock(&pOmxTestInfo->ftb_lock);

                if(push(pOmxTestInfo->fbd_queue, (void *)pBuffer) < 0)
                {
                    SAMPLE_PRINT_ERROR("Error in enqueueing fbd_data\n");
                }
                else
                {
                    sem_post(&pOmxTestInfo->ftb_sem);
                }
                pthread_mutex_unlock(&pOmxTestInfo->ftb_lock);

        #else  // not send buffer after output flush

            SAMPLE_PRINT_INFO("Fill this buffer meet output flushing.\n");
            continue;

        #endif
        }
        else if (pOmxTestInfo->seeking_progress)
        {
            SAMPLE_PRINT_INFO("Fill this buffer meet seek pending.\n");
            continue;
        }
        else
        {
            pBuffer->nFilledLen = 0;
            if ( OMX_FillThisBuffer(pOmxTestInfo->dec_handle, pBuffer) == OMX_ErrorNone )
            {
                pOmxTestInfo->free_op_buf_cnt--;
            }
        }
    }

    pOmxTestInfo->FtbStatus = THREAD_INVALID;

    return NULL;
}

/****************************************************************************/
/* 捕获ctrl + c 信号量                                                      */
/****************************************************************************/
void SignalHandle(int sig)
{
    int i = 0;

    SAMPLE_PRINT_INFO("Signal Handle - I got signal %d\n", sig);
    (void) signal(SIGINT,SIG_IGN);

    for (i=0; i<MAX_INST_NUM; i++)
    {
        if (1 == OmxTestInfo[i].is_open)
        {
            do_freeHandle_and_clean_up(&OmxTestInfo[i], true); //需要增加实例参数，比较麻烦，暂时先屏蔽
        }
    }
    (void) signal(SIGINT, SIG_DFL);
}


static void Init_OmxInst()
{
    int i = 0;

    memset(OmxTestInfo, 0, sizeof(OmxTestInfo));

    for (i = 0; i < MAX_INST_NUM; i++)
    {
        OmxTestInfo[i].timestampInterval = 33333;
        OmxTestInfo[i].currentStatus = GOOD_STATE;

        OmxTestInfo[i].sliceheight  = DEFAULT_HEIGHT;
        OmxTestInfo[i].height       = DEFAULT_HEIGHT;
        OmxTestInfo[i].stride       = DEFAULT_WIDTH;
        OmxTestInfo[i].width        = DEFAULT_WIDTH;
        OmxTestInfo[i].readsize_add_in_stream = false;
        OmxTestInfo[i].is_use_buffer = 0;
    }

    return;
}



int get_resolution(char *pFileName, int *pWidth, int *pHeight)
{
    int   i;
    int   len       = 0;
    int   found_num = 0;
    char  tmpBuf[10];

    if (NULL == pFileName || NULL == pWidth || NULL == pHeight)
    {
        SAMPLE_PRINT_ERROR("get_resolution with invalid param, return error.\n");
        return -1;
    }

    len  = strlen(pFileName);

    for (i=len-1; i>0; i--)
    {
        if ('x' == pFileName[i])
        {
            strncpy(tmpBuf, &pFileName[i+1], 4);
            tmpBuf[4] = '\0';
            *pHeight  = atoi(tmpBuf);
            found_num = 1;
        }

        if ('_' == pFileName[i] && found_num)
        {
            strncpy(tmpBuf, &pFileName[i+1], 4);
            tmpBuf[4] = '\0';
            *pWidth  = atoi(tmpBuf);
            found_num = 2;
            break;
        }
    }

    if (2 == found_num)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int is_framepacket_standard(codec_format format)
{
    if( CODEC_FORMAT_VP8      == format
     || CODEC_FORMAT_VP6      == format
     || CODEC_FORMAT_VP6F     == format
     || CODEC_FORMAT_VP6A     == format
     || CODEC_FORMAT_VC1SMP   == format
     || CODEC_FORMAT_REAL8    == format
     || CODEC_FORMAT_REAL9    == format
     || CODEC_FORMAT_VP9      == format
     || CODEC_FORMAT_AV1      == format)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int is_tidx_file_exist(char *psrc_file)
{
    FILE *fptidxFile = NULL;
    char tidxfile[512] = {0};

    snprintf(tidxfile, sizeof(tidxfile), "%s.tidx", psrc_file);

    fptidxFile = fopen(tidxfile, "r");
    if(NULL == fptidxFile)
    {
        return 0;
    }
    else
    {
        fclose(fptidxFile);
        return 1;
    }
}


#ifdef HI_TVP_SUPPORT
hi_s32 InitSecEnvironment(hi_void)
{
    SAMPLE_PRINT_INFO("%s enter \n", __func__);

#ifdef HI_SMMU_SUPPORT

    TEEC_Result result;
    uint32_t origin;
    TEEC_UUID svc_id = {0x0D0D0D0D, 0x0D0D, 0x0D0D,
                        {0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D}
                       };

    TEEC_Operation session_operation;

    memset(&session_operation, 0, sizeof(TEEC_Operation));

    session_operation.started = 1;
    session_operation.paramTypes = TEEC_PARAM_TYPES(
                               TEEC_NONE,
                               TEEC_NONE,
                               TEEC_MEMREF_TEMP_INPUT,
                               TEEC_MEMREF_TEMP_INPUT);

    if (TEEC_InitializeContext(HI_NULL, &g_TeeContext) != TEEC_SUCCESS)
    {
        SAMPLE_PRINT_ERROR("TEEC_InitializeContext Failed!\n");
        return HI_FAILURE;
    }

    result = TEEC_OpenSession(&g_TeeContext, &g_teeSession, &svc_id, TEEC_LOGIN_IDENTIFY, HI_NULL, &session_operation, &origin);
    if (result != TEEC_SUCCESS)
    {
        SAMPLE_PRINT_ERROR("TEEC_OpenSession Failed!\n");
        TEEC_FinalizeContext(&g_TeeContext);

        exit(0);
    }

#else
    HI_SEC_MMZ_Init();
#endif
    SAMPLE_PRINT_ERROR("%s exit \n", __func__);

    return HI_SUCCESS;
}


hi_void DeInitSecEnvironment(hi_void)
{
#ifdef HI_SMMU_SUPPORT
    TEEC_CloseSession(&g_teeSession);

    TEEC_FinalizeContext(&g_TeeContext);
#else
    HI_SEC_MMZ_DeInit();
#endif
}

#endif

static int OMX_InquireStatus(OmxTestInfo_S *pOmxTestInfo)
{
    static hi_u32 InquireCount = 0;
    static hi_u32 FrameCountRec = 0;
    hi_u32 FrameCountRead;

#define MAX_INQUIRE_COUNT (3000)

    if (pOmxTestInfo->is_open != 1)
    {
        return 0;
    }

    FrameCountRead = pOmxTestInfo->receive_frame_cnt;
    if (FrameCountRead != FrameCountRec)
    {
        InquireCount = 0;
        FrameCountRec = FrameCountRead;
        return 0;
    }

    InquireCount++;
    if (InquireCount > MAX_INQUIRE_COUNT)
    {
        SAMPLE_PRINT_ERROR("[Error]:InquireCount %d > %d\n", InquireCount, MAX_INQUIRE_COUNT);
        return -1;
    }

    return 0;
}

bool IsCrcModeEnable(char *pStr)
{
    if (pStr == HI_NULL)
    {
        return 0;
    }

    if (!strcasecmp("crc_auto", pStr)
     || !strcasecmp("crc_gen",   pStr))
    {
        g_AutoCrcEnable = HI_TRUE;
        return 1;
    }
    else if(!strcasecmp("crc_manu", pStr))
    {
        return 1;
    }

    return 0;
}

int InitCrcEnvironment(OmxTestInfo_S *pCodec, char *pCrcMode, char *pSrcFile)
{
    char EchoStr[512];

    snprintf(EchoStr, sizeof(EchoStr), "echo %s %s >/proc/msp/vfmw_crc", pCrcMode, pSrcFile);
    system(EchoStr);

    system("echo map_frm on >/proc/msp/omxvdec");

    pCodec->bCRCEnable = HI_TRUE;

    return 0;
}

int ExitCrcEnvironment(OmxTestInfo_S *pCodec)
{
    system("echo map_frm off >/proc/msp/omxvdec");
    system("echo crc_off >/proc/msp/vfmw_crc");

    pCodec->bCRCEnable = HI_FALSE;

    return 0;
}

int ParseStandardInfo(char *argv[], OmxTestInfo_S *pTestInfo)
{
    int count = 0;

    if (!strcasecmp("mpeg2", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_MP2;
    }
    else if (!strcasecmp("mpeg1", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_MP1;
    }
    else if (!strcasecmp("mpeg4", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_MP4;
    }
    else if (!strcasecmp("h263", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_H263;
    }
    else if (!strcasecmp("sor", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_SORENSON;
    }
    else if (!strcasecmp("vp6", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VP6;
    }
    else if (!strcasecmp("vp6f", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VP6F;
    }
    else if (!strcasecmp("vp6a", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VP6A;
    }
    else if (!strcasecmp("h264", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_H264;
    }
    else if (!strcasecmp("h265", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_HEVC;
    }
    else if (!strcasecmp("avs", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_AVS;
    }
    else if (!strcasecmp("avs2", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_AVS2;
    }
    else if (!strcasecmp("real8", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_REAL8;
    }
    else if (!strcasecmp("real9", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_REAL9;
    }
    else if (!strcasecmp("vc1ap", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VC1AP;
    }
    else if (!strcasecmp("vc1smp", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VC1SMP;
    }
    else if (!strcasecmp("vp8", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VP8;
    }
    else if (!strcasecmp("vp9", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_VP9;
    }
    else if (!strcasecmp("divx3", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_DIVX3;
    }
    else if (!strcasecmp("mvc", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_MVC;
    }
    else if (!strcasecmp("mjpeg", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_MJPEG;
    }
    else if (!strcasecmp("avs3", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_AVS3;
    }
    else if (!strcasecmp("av1", argv[count]))
    {
        pTestInfo->codec_format_option = CODEC_FORMAT_AV1;
    }
    else
    {
        SAMPLE_PRINT_ERROR("unsupport vid codec type!\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int i = 0;
    int width = 0;
    int height = 0;
    int CurPos = 1;
    int ret;

    if (argc > MAX_INST_NUM + 1)
    {
        SAMPLE_PRINT_ERROR("Command line argument is not available\n");
        SAMPLE_PRINT_ERROR("To use it: ./sample_omxvdec <in-filename 1> <in-filename 2> ... <in-filename n> \n");
        return HI_FAILURE;
    }

    Init_OmxInst();

    for (i = 0; i < MAX_INST_NUM; i++)
    {
        OmxTestInfo[i].inst_no = i;
        strncpy(OmxTestInfo[i].in_filename, argv[CurPos], strlen(argv[CurPos])+1);
        SAMPLE_PRINT_INFO("Input values: inputfilename[%s]\n", OmxTestInfo[i].in_filename);
        CurPos++;

        if (get_resolution(OmxTestInfo[i].in_filename, &width, &height) == 0)
        {
            if (width*height <= MAX_WIDTH*MAX_HEIGHT && width > 0 && height > 0)
            {
                OmxTestInfo[i].width  = width;
                OmxTestInfo[i].height = height;
            }
        }

        ret = ParseStandardInfo(&argv[CurPos], &OmxTestInfo[i]);
        if (ret != 0)
        {
            SAMPLE_PRINT_ERROR("ParseStandardInfo failed!\n");
            return HI_FAILURE;
        }
        CurPos++;

        if (argv[CurPos] != HI_NULL && !strcasecmp("tvpon", argv[CurPos]))
        {
            OmxTestInfo[i].tvp_option = 1;
            CurPos++;
        }

        if (argv[CurPos] != HI_NULL && !strcasecmp("use", argv[CurPos]))
        {
            OmxTestInfo[i].is_use_buffer = 1;
            CurPos++;
        }
        else if (argv[CurPos] != HI_NULL && !strcasecmp("alloc", argv[CurPos]))
        {
            OmxTestInfo[i].is_use_buffer = 0;
            CurPos++;
        }

        if (IsCrcModeEnable(argv[CurPos]))
        {
            hi_char CrcMode[10]; /* 10 is array szie */

            strncpy(CrcMode, argv[CurPos], strlen(argv[CurPos])+1);

            if (InitCrcEnvironment(&OmxTestInfo[i], CrcMode, OmxTestInfo[i].in_filename) != 0)
            {
                SAMPLE_PRINT_ERROR("InitCrcEnvironment failed!\n");
                return HI_FAILURE;
            }
            g_EnableLog = HI_FALSE;
            CurPos++;
        }

        /* 3. Set Extra Option */
        if (is_framepacket_standard(OmxTestInfo[i].codec_format_option))
        {
            if (is_tidx_file_exist(OmxTestInfo[i].in_filename))
            {
                OmxTestInfo[i].frame_in_packet = 1;
            }
            else
            {
                OmxTestInfo[i].frame_in_packet = 0;
            }

            if(CODEC_FORMAT_VP8 == OmxTestInfo[i].codec_format_option)
            {
                OmxTestInfo[i].readsize_add_in_stream = false;
            }
            else
            {
                OmxTestInfo[i].readsize_add_in_stream = true;
            }
        }

        if (OmxTestInfo[i].test_option == 1)
        {
            SAMPLE_PRINT_INFO("*********************************************\n");
            SAMPLE_PRINT_INFO("ENTER THE COMPLIANCE TEST YOU WOULD LIKE TO EXECUTE\n");
            SAMPLE_PRINT_INFO("*********************************************\n");
            SAMPLE_PRINT_INFO("1 --> Call Free Handle at the OMX_StateLoaded\n");
            SAMPLE_PRINT_INFO("2 --> Call Free Handle at the OMX_StateIdle\n");
            SAMPLE_PRINT_INFO("3 --> Call Free Handle at the OMX_StateExecuting\n");
            SAMPLE_PRINT_INFO("4 --> Call Free Handle at the OMX_StatePause\n");

            fflush(stdin);
            scanf("%d", (int *)(&OmxTestInfo[i].freeHandle_option));
            fflush(stdin);
        }
        else
        {
            OmxTestInfo[i].freeHandle_option = (freeHandle_test)0;
        }

        pthread_cond_init(&(OmxTestInfo[i].event_cond), 0);
        pthread_mutex_init(&OmxTestInfo[i].event_lock, 0);
        pthread_mutex_init(&OmxTestInfo[i].etb_lock, 0);
        pthread_mutex_init(&OmxTestInfo[i].ftb_lock, 0);
        pthread_mutex_init(&OmxTestInfo[i].status_lock, 0);

        if (-1 == sem_init(&OmxTestInfo[i].etb_sem, 0, 0))
        {
            SAMPLE_PRINT_ERROR("Error - sem_init failed %d\n", errno);
        }

        if (-1 == sem_init(&OmxTestInfo[i].ftb_sem, 0, 0))
        {
            SAMPLE_PRINT_ERROR("Error - sem_init failed %d\n", errno);
        }

        if (-1 == sem_init(&OmxTestInfo[i].in_flush_sem, 0, 0))
        {
            SAMPLE_PRINT_ERROR("Error - sem_init failed %d\n", errno);
        }

        if (-1 == sem_init(&OmxTestInfo[i].out_flush_sem, 0, 0))
        {
            SAMPLE_PRINT_ERROR("Error - sem_init failed %d\n", errno);
        }

        if (-1 == sem_init(&OmxTestInfo[i].seek_sem, 0, 0))
        {
            SAMPLE_PRINT_ERROR("Error - sem_init failed %d\n", errno);
        }

        (void) signal(SIGINT, SignalHandle);

        OmxTestInfo[i].etb_queue = alloc_queue();
        if (OmxTestInfo[i].etb_queue == NULL)
        {
            SAMPLE_PRINT_ERROR("Error in Creating etb_queue\n");
            return HI_FAILURE;
        }

        OmxTestInfo[i].ftb_queue = alloc_queue();
        if (OmxTestInfo[i].ftb_queue == NULL)
        {
            SAMPLE_PRINT_ERROR("Error in Creating ftb_queue\n");
            free_queue(OmxTestInfo[i].etb_queue);
            return -1;
        }

        g_TestInstNum++;

        if (argv[CurPos] == HI_NULL || strcasecmp("+", argv[CurPos]))
        {
            break;
        }
        CurPos++;
    }

    for (i = 0; i < g_TestInstNum; i++)
    {
        if(0 != pthread_create(&OmxTestInfo[i].fbd_thread_id, NULL, fbd_thread, &OmxTestInfo[i]))
        {
            SAMPLE_PRINT_ERROR("Error in Creating fbd_thread\n");
            free_queue(OmxTestInfo[i].etb_queue);
            free_queue(OmxTestInfo[i].ftb_queue);
            return -1;
        }

        if(Init_Decoder(&OmxTestInfo[i]) < 0)
        {
            SAMPLE_PRINT_ERROR("Error - Decoder Init failed\n");
            return -1;
        }

    #ifdef HI_TVP_SUPPORT
        if (1 == OmxTestInfo[i].tvp_option)
        {
            strncpy(OmxTestInfo[i].pCAStreamBuf.bufname, "OmxSampleEsBuf", sizeof(OmxTestInfo[i].pCAStreamBuf.bufname));
            OmxTestInfo[i].pCAStreamBuf.overflow_threshold = 100;
            OmxTestInfo[i].pCAStreamBuf.underflow_threshold = 0;
            OmxTestInfo[i].pCAStreamBuf.bufsize = 4*1024*1024;
            if (HI_SUCCESS != hi_mpi_mmz_malloc(&OmxTestInfo[i].pCAStreamBuf))
            {
                SAMPLE_PRINT_ERROR("Alloc OmxSampleEsBuf failed\n");

                return -1;
            }

            if (HI_SUCCESS != InitSecEnvironment())
            {
                SAMPLE_PRINT_ERROR("call HI_UNF_AVPLAY_Start failed.\n");
                hi_mpi_mmz_free(&OmxTestInfo[i].pCAStreamBuf);

                return -1;
            }
        }
    #endif

        if (Play_Decoder(&OmxTestInfo[i]) < 0)
        {
            SAMPLE_PRINT_ERROR("Error - Decode Play failed\n");
        #ifdef HI_TVP_SUPPORT
            if (1 == OmxTestInfo[i].tvp_option)
            {
                DeInitSecEnvironment();
            }
        #endif
            return -1;
        }

        OmxTestInfo[i].is_open = 1;
    }

    loop_function();

    return 0;
}

static void exit_loop(OmxTestInfo_S * pOmxTestInfo)
{
    do_freeHandle_and_clean_up(pOmxTestInfo, (pOmxTestInfo->currentStatus == ERROR_STATE));

    return;
}


static void loop_function(void)
{
    int i = 0;
    int cmd_error = 0;

    char curr_seq_command[512];
    int command_inst_no = 0; //命令对应的实例; 等于0表示对所有实例有效

    SAMPLE_PRINT_INFO("\nTest for control, cmds as follows:\n");
    SAMPLE_PRINT_INFO("First input the command type, Then input the instance num.\n");

    SAMPLE_PRINT_INFO("\nPlease input the COMMAND:\n");
    SAMPLE_PRINT_INFO("q (exit), p (pause), r (resume), f (flush all), f0 (flush in), f1 (flush out), s (seek)\n\n");

    while (cmd_error == 0)
    {
        while (g_AutoCrcEnable == HI_TRUE)
        {
            for (i = 0; i < g_TestInstNum; i++)
            {
                usleep(10*1000);
                if (OMX_InquireStatus(&OmxTestInfo[i]) < 0)
                {
                    goto exit1;
                }
                if (OmxTestInfo[i].bOutputEosReached)
                {
                    goto exit1;
                }
            }
        }

        fflush(stdin);
        scanf("%s", curr_seq_command);

        command_inst_no = 0;
        if (g_TestInstNum > 1)
        {
            SAMPLE_PRINT_INFO("\nPlease input the instance No: [0, %d], %d for all instance.\n", g_TestInstNum, g_TestInstNum);
            fflush(stdin);
            scanf("%d", &command_inst_no);
            SAMPLE_PRINT_INFO("command :%s inst:%d\n", curr_seq_command, command_inst_no);

            if (command_inst_no < 0 || command_inst_no > g_TestInstNum)
            {
                SAMPLE_PRINT_ERROR("invalid parameter!\n");
                continue;
            }
        }

        if (!strcmp(curr_seq_command, "q"))
        {
            if (g_TestInstNum > 1)  // multi inst on run
            {
                if (g_TestInstNum == command_inst_no)
                {
                    SAMPLE_PRINT_INFO("All test-case exit!\n");
                    goto exit1;
                }
                else // command on one inst
                {
                    SAMPLE_PRINT_INFO("Test-case %d exit!\n", command_inst_no);
                    if (1 == OmxTestInfo[command_inst_no].is_open)
                    {
                        exit_loop(&OmxTestInfo[command_inst_no]);
                    }
                    for (i = 0; i < MAX_INST_NUM; i++)
                    {
                        if (1 == OmxTestInfo[i].is_open)
                        {
                            break;
                        }
                    }
                    if (i >= MAX_INST_NUM)
                    {
                        SAMPLE_PRINT_INFO("All test-case finish, exit!\n");
                        return;
                    }
                }
            }
            else // single inst on run
            {
                SAMPLE_PRINT_INFO("Test-case exit!\n");
                goto exit1;
            }

        }
        else
        {
            if (g_TestInstNum > 1)  // multi inst on run
            {
                if (g_TestInstNum == command_inst_no)  // command on all valid insts
                {
                    for (i = 0; i < MAX_INST_NUM; i++)
                    {
                        if (1 == OmxTestInfo[i].is_open)
                        {
                            cmd_error = process_current_command(&OmxTestInfo[i], curr_seq_command);
                        }
                    }
                }
                else  // command on one inst
                {
                    cmd_error = process_current_command(&OmxTestInfo[command_inst_no], curr_seq_command);
                }
            }
            else // single inst on run
            {
                cmd_error = process_current_command(&OmxTestInfo[command_inst_no], curr_seq_command);
            }
        }
   }


exit1:

    for (i = 0; i < MAX_INST_NUM; i++)
    {
        if (1 == OmxTestInfo[i].is_open)
        {
            exit_loop(&OmxTestInfo[i]);

            pthread_mutex_destroy(&OmxTestInfo[i].status_lock);
        }
    }

    return;
}


static OMX_ERRORTYPE Allocate_Buffers(OmxTestInfo_S *pOmxTestInfo,
                                       OMX_COMPONENTTYPE *dec_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    long bufCnt=0;

    *pBufHdrs = (OMX_BUFFERHEADERTYPE **)malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

    if (0 == nPortIndex)
    {
        pOmxTestInfo->used_ip_buf_cnt = 0;
    }
    else
    {
        pOmxTestInfo->used_op_buf_cnt = 0;
    }

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt)
    {
        error = OMX_AllocateBuffer(dec_handle, &((*pBufHdrs)[bufCnt]), nPortIndex, NULL, bufSize);

        if (error != OMX_ErrorNone)
        {
            SAMPLE_PRINT_ERROR("OMX_AllocateBuffer No %ld failed!\n", bufCnt);
            break;
        }
        else
        {
            if (0 == nPortIndex)
            {
                pOmxTestInfo->used_ip_buf_cnt++;
            }
            else
            {
                pOmxTestInfo->used_op_buf_cnt++;
            }
        }
    }

    return error;
}


static OMX_ERRORTYPE Use_Buffers(OmxTestInfo_S *pOmxTestInfo,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    long bufCnt=0;

    if (0 == nPortIndex)
    {
        pOmxTestInfo->used_ip_buf_cnt = 0;
    }
    else
    {
        pOmxTestInfo->used_op_buf_cnt = 0;
    }

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt)
    {
        pOmxTestInfo->buffer[bufCnt].buf_size = bufSize;//(bufSize + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);

        hi_unf_mem_malloc(&pOmxTestInfo->buffer[bufCnt]);

        if (0 == nPortIndex)
        {
            strcpy(pOmxTestInfo->buffer[bufCnt].buf_name, "OMX_VDEC_IN");
        }
        else
        {
            strcpy(pOmxTestInfo->buffer[bufCnt].buf_name, "OMX_VDEC_OUT");
        }

        /* error = OMX_UseBuffer(pOmxTestInfo->dec_handle, &((*pBufHdrs)[bufCnt]),
                              nPortIndex, NULL, bufSize, pOmxTestInfo->buffer[bufCnt].user_viraddr); */

        error = OMX_UseBuffer(pOmxTestInfo->dec_handle, &((*pBufHdrs)[bufCnt]),
                              nPortIndex, NULL, bufSize, (hi_u8 *)&pOmxTestInfo->buffer[bufCnt]);
        SAMPLE_PRINT_ERROR("%s buffer = %p \n", __func__, &pOmxTestInfo->buffer[bufCnt]);

        (*pBufHdrs)[bufCnt]->nTickCount = bufCnt;  //暂时找个地方存一下物理地址

        if (error != OMX_ErrorNone)
        {
            SAMPLE_PRINT_ERROR("Use_Buffers No %ld failed!\n", bufCnt);
            //break;
        }
        else
        {
            if (0 == nPortIndex)
            {
                pOmxTestInfo->used_ip_buf_cnt++;
            }
            else
            {
                pOmxTestInfo->used_op_buf_cnt++;
            }
        }
    }

    return error;
}


static int disable_output_port(OmxTestInfo_S * pOmxTestInfo)
{
    int q_flag = 0;
    OMX_BUFFERHEADERTYPE *pBuffer = NULL;

    // Send DISABLE command.
    pOmxTestInfo->sent_disabled = 1;

    OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandPortDisable, 1, 0);

    do
    {
       pthread_mutex_lock(&pOmxTestInfo->ftb_lock);
       pBuffer = (OMX_BUFFERHEADERTYPE *)pop(pOmxTestInfo->ftb_queue);
       pthread_mutex_unlock(&pOmxTestInfo->ftb_lock);

       if (NULL != pBuffer)
       {
           if (0 == pOmxTestInfo->is_use_buffer)
           {
               OMX_FreeBuffer(pOmxTestInfo->dec_handle, 1, pBuffer);
           }
           else
           {
               OMX_FreeBuffer(pOmxTestInfo->dec_handle, 1, pBuffer);
               hi_unf_mem_free(&pOmxTestInfo->buffer[pBuffer->nTickCount]);
           }
       }
       else
       {
           q_flag = 1;
       }
    }while (!q_flag);

    wait_for_event(pOmxTestInfo, OMX_CommandPortDisable);

    if (pOmxTestInfo->currentStatus == ERROR_STATE)
    {
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    return 0;
}


static int enable_output_port(OmxTestInfo_S * pOmxTestInfo)
{
    OMX_U32 bufCnt = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    // Send Enable command
    OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandPortEnable, 1, 0);

    /* Allocate buffer on decoder's o/p port */
    pOmxTestInfo->portFmt.nPortIndex = 1;
    if (0 == pOmxTestInfo->is_use_buffer)
    {
        error = Allocate_Buffers(pOmxTestInfo, pOmxTestInfo->dec_handle, &pOmxTestInfo->pOutYUVBufHdrs, pOmxTestInfo->portFmt.nPortIndex, \
                                 pOmxTestInfo->portFmt.nBufferCountActual, pOmxTestInfo->portFmt.nBufferSize);

        if (error != OMX_ErrorNone)
        {
           SAMPLE_PRINT_ERROR("OMX_AllocateBuffer Output buffer error\n");
           return -1;
        }
        else
        {
           pOmxTestInfo->free_op_buf_cnt = pOmxTestInfo->portFmt.nBufferCountActual;
        }
    }
    else
    {
        error = Use_Buffers(pOmxTestInfo, &pOmxTestInfo->pOutYUVBufHdrs, pOmxTestInfo->portFmt.nPortIndex,
                            pOmxTestInfo->portFmt.nBufferCountActual, pOmxTestInfo->portFmt.nBufferSize);

        if (error != OMX_ErrorNone)
        {
           SAMPLE_PRINT_ERROR("OMX_UseBuffer Output buffer error\n");
           return -1;
        }
        else
        {
           pOmxTestInfo->free_op_buf_cnt = pOmxTestInfo->portFmt.nBufferCountActual;
        }
    }

    if (pOmxTestInfo->fbd_thread_exit)
    {
        return 0;
    }

    // wait for enable event to come back
    wait_for_event(pOmxTestInfo, OMX_CommandPortEnable);

    if (pOmxTestInfo->currentStatus == ERROR_STATE)
    {
        SAMPLE_PRINT_ERROR("start error!\n");
        do_freeHandle_and_clean_up(pOmxTestInfo, true);
        return -1;
    }

    for(bufCnt=0; bufCnt < pOmxTestInfo->portFmt.nBufferCountActual; ++bufCnt)
    {
        pOmxTestInfo->pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
        pOmxTestInfo->pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;

        ret = OMX_FillThisBuffer(pOmxTestInfo->dec_handle, pOmxTestInfo->pOutYUVBufHdrs[bufCnt]);
        if (OMX_ErrorNone != ret)
        {
            SAMPLE_PRINT_ERROR("OMX_FillThisBuffer failed, result %d\n", ret);
        }
        else
        {
            pOmxTestInfo->free_op_buf_cnt--;
        }
    }

    return 0;
}


static int output_port_reconfig(OmxTestInfo_S * pOmxTestInfo)
{
    if (disable_output_port(pOmxTestInfo) != 0)
    {
        SAMPLE_PRINT_ERROR("disable output port failed\n");
        return -1;
    }
    /* Port for which the client needs to obtain info */

    pOmxTestInfo->portFmt.nPortIndex = 1;

    OMX_GetParameter(pOmxTestInfo->dec_handle,OMX_IndexParamPortDefinition,&pOmxTestInfo->portFmt);

    if(OMX_DirOutput != pOmxTestInfo->portFmt.eDir)
    {
        SAMPLE_PRINT_ERROR("Error - Expect Output Port\n");
        return -1;
    }

    if (enable_output_port(pOmxTestInfo) != 0)
    {
        SAMPLE_PRINT_ERROR("enable output port failed\n");
        return -1;
    }

    return 0;
}


static int seek_progress(OmxTestInfo_S * pOmxTestInfo)
{
    int bufCnt = 0;
    int frameSize = 0;
    OMX_BUFFERHEADERTYPE *pBuffer = NULL;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pOmxTestInfo->EtbStatus == THREAD_RUNNING || pOmxTestInfo->FtbStatus == THREAD_RUNNING)
    {
        SAMPLE_PRINT_INFO("EtbStatus/FtbStatus = THREAD_RUNNING, sleep for a while.\n");
        do {
            sleep(1);
        }while(pOmxTestInfo->EtbStatus == THREAD_RUNNING || pOmxTestInfo->FtbStatus == THREAD_RUNNING);
        SAMPLE_PRINT_INFO("EtbStatus&FtbStatus != THREAD_RUNNING, wake up.\n");
    }

    do {
        pthread_mutex_lock(&pOmxTestInfo->etb_lock);
        pBuffer = (OMX_BUFFERHEADERTYPE *) pop(pOmxTestInfo->etb_queue);
        pthread_mutex_unlock(&pOmxTestInfo->etb_lock);
    }while(pBuffer != NULL);

    do {
        pthread_mutex_lock(&pOmxTestInfo->ftb_lock);
        pBuffer = (OMX_BUFFERHEADERTYPE *) pop(pOmxTestInfo->ftb_queue);
        pthread_mutex_unlock(&pOmxTestInfo->ftb_lock);
    }while(pBuffer != NULL);

    rewind(pOmxTestInfo->inputBufferFileFd);
    pOmxTestInfo->frame_flag = 0;
    pOmxTestInfo->send_cnt   = 0;
    pOmxTestInfo->receive_frame_cnt = 0;
    pOmxTestInfo->bInputEosReached = false;

    if (pOmxTestInfo->used_op_buf_cnt != pOmxTestInfo->free_op_buf_cnt)
    {
        SAMPLE_PRINT_ERROR("ERROR: seek_progress used_op_buf_cnt = %d, free_op_buf_cnt = %d!\n", pOmxTestInfo->used_op_buf_cnt, pOmxTestInfo->free_op_buf_cnt);
        sleep(3);
    }

    for(bufCnt = 0; bufCnt < pOmxTestInfo->used_op_buf_cnt; ++bufCnt)
    {
        pOmxTestInfo->pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
        pOmxTestInfo->pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;

        ret = OMX_FillThisBuffer(pOmxTestInfo->dec_handle, pOmxTestInfo->pOutYUVBufHdrs[bufCnt]);

        if (OMX_ErrorNone != ret)
        {
            SAMPLE_PRINT_ERROR("Error - OMX_FillThisBuffer failed!!\n");
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
            return -1;
        }
        else
        {
            pOmxTestInfo->free_op_buf_cnt--;
        }
    }

    if (pOmxTestInfo->used_ip_buf_cnt != pOmxTestInfo->free_ip_buf_cnt)
    {
        SAMPLE_PRINT_ERROR("ERROR: seek_progress used_ip_buf_cnt = %d, free_ip_buf_cnt = %d!\n", pOmxTestInfo->used_ip_buf_cnt, pOmxTestInfo->free_ip_buf_cnt);
        sleep(3);
    }

    for (bufCnt = 0; bufCnt < pOmxTestInfo->used_ip_buf_cnt; bufCnt++)
    {
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nInputPortIndex = 0;
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nOffset = 0;
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nFlags = 0;

        frameSize = Read_Buffer_from_EsRawStream(pOmxTestInfo, pOmxTestInfo->pInputBufHdrs[bufCnt]);

        pOmxTestInfo->pInputBufHdrs[bufCnt]->nFilledLen = frameSize;
        pOmxTestInfo->pInputBufHdrs[bufCnt]->nInputPortIndex = 0;

        ret = OMX_EmptyThisBuffer(pOmxTestInfo->dec_handle, pOmxTestInfo->pInputBufHdrs[bufCnt]);
        if (OMX_ErrorNone != ret)
        {
            SAMPLE_PRINT_ERROR("ERROR: OMX_EmptyThisBuffer failed\n");
            do_freeHandle_and_clean_up(pOmxTestInfo, true);
            return -1;
        }
        else
        {
            pOmxTestInfo->free_ip_buf_cnt--;
        }

        if (pOmxTestInfo->pInputBufHdrs[bufCnt]->nFlags & OMX_BUFFERFLAG_EOS)
        {
            pOmxTestInfo->bInputEosReached = true;
            break;
        }
    }

    return 0;
}

static void do_freeHandle_and_clean_up(OmxTestInfo_S * pOmxTestInfo, int isDueToError)
{
    int bufCnt = 0;
    OMX_STATETYPE state = OMX_StateLoaded;
    OMX_ERRORTYPE result = OMX_ErrorNone;

    pOmxTestInfo->ebd_thread_exit = 1;
    pOmxTestInfo->fbd_thread_exit = 1;
    sem_post(&pOmxTestInfo->etb_sem);
    sem_post(&pOmxTestInfo->ftb_sem);

    OMX_GetState(pOmxTestInfo->dec_handle, &state);

    pOmxTestInfo->flush_input_progress = 1;
    pOmxTestInfo->flush_output_progress = 1;
    OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandFlush, OMX_ALL, 0);
    wait_for_event(pOmxTestInfo, OMX_CommandFlush);

    if (pOmxTestInfo->currentStatus == PORT_SETTING_RECONFIG_STATE)
    {
        SAMPLE_PRINT_ERROR("currentStatus = PORT_SETTING_RECONFIG_STATE, sleep for a while.\n");
        do {
            sleep(1);
        }while(pOmxTestInfo->currentStatus == PORT_SETTING_RECONFIG_STATE);
        SAMPLE_PRINT_ERROR("currentStatus != PORT_SETTING_RECONFIG_STATE, wake up.\n");
    }

    if (state == OMX_StateExecuting || state == OMX_StatePause)
    {
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StateIdle, 0);
        wait_for_event(pOmxTestInfo, OMX_CommandStateSet);

        OMX_GetState(pOmxTestInfo->dec_handle, &state);
        if (state != OMX_StateIdle)
        {
            SAMPLE_PRINT_ERROR("current component state :%d error!\n", state);
        }
    }

    if (state == OMX_StateIdle)
    {
        OMX_SendCommand(pOmxTestInfo->dec_handle, OMX_CommandStateSet, OMX_StateLoaded, 0);

        for(bufCnt=0; bufCnt < pOmxTestInfo->used_ip_buf_cnt; ++bufCnt)
        {
            OMX_FreeBuffer(pOmxTestInfo->dec_handle, 0, pOmxTestInfo->pInputBufHdrs[bufCnt]);
        }

        if (pOmxTestInfo->pInputBufHdrs)
        {
            free(pOmxTestInfo->pInputBufHdrs);
            pOmxTestInfo->pInputBufHdrs = NULL;
        }

        for(bufCnt = 0; bufCnt < pOmxTestInfo->used_op_buf_cnt; bufCnt++)
        {
             OMX_FreeBuffer(pOmxTestInfo->dec_handle, 1, pOmxTestInfo->pOutYUVBufHdrs[bufCnt]);
        }

        if (pOmxTestInfo->pOutYUVBufHdrs)
        {
            free(pOmxTestInfo->pOutYUVBufHdrs);
            pOmxTestInfo->pOutYUVBufHdrs = NULL;
        }

        pOmxTestInfo->free_op_buf_cnt = 0;

        wait_for_event(pOmxTestInfo, OMX_CommandStateSet);

        OMX_GetState(pOmxTestInfo->dec_handle, &state);
        if (state != OMX_StateLoaded)
        {
            SAMPLE_PRINT_ERROR("current component state :%d error!\n", state);
        }

        if (pOmxTestInfo->is_use_buffer)
        {
           for(bufCnt=0; bufCnt < pOmxTestInfo->used_op_buf_cnt; ++bufCnt)
           {
               hi_unf_mem_free(&pOmxTestInfo->buffer[bufCnt]);
           }
        }
    }

    result = OMX_FreeHandle(pOmxTestInfo->dec_handle);
    if (result != OMX_ErrorNone)
    {
        SAMPLE_PRINT_ERROR("OMX_FreeHandle error. Error code: %x\n", result);
    }

    pOmxTestInfo->dec_handle = NULL;

    /* Deinit OpenMAX */
    OMX_Deinit();

    if (pOmxTestInfo->bCRCEnable == HI_TRUE)
    {
        ExitCrcEnvironment(pOmxTestInfo);
    }

    if(pOmxTestInfo->inputBufferFileFd != NULL)
    {
        fclose(pOmxTestInfo->inputBufferFileFd);
        pOmxTestInfo->inputBufferFileFd = NULL;
    }

    if(pOmxTestInfo->etb_queue)
    {
        free_queue(pOmxTestInfo->etb_queue);
        pOmxTestInfo->etb_queue = NULL;
    }

    if(pOmxTestInfo->ftb_queue)
    {
        free_queue(pOmxTestInfo->ftb_queue);
        pOmxTestInfo->ftb_queue = NULL;
    }

#ifdef HI_TVP_SUPPORT
    if (1 == pOmxTestInfo->tvp_option)
    {
        hi_mpi_mmz_free(&pOmxTestInfo->pCAStreamBuf);

        DeInitSecEnvironment();
    }
#endif

    pthread_join(pOmxTestInfo->ebd_thread_id, NULL);
    pthread_join(pOmxTestInfo->fbd_thread_id, NULL);

    pthread_cond_destroy(&pOmxTestInfo->event_cond);

    pthread_mutex_destroy(&pOmxTestInfo->event_lock);
    pthread_mutex_destroy(&pOmxTestInfo->etb_lock);
    pthread_mutex_destroy(&pOmxTestInfo->ftb_lock);

    if (-1 == sem_destroy(&pOmxTestInfo->etb_sem))
    {
        SAMPLE_PRINT_ERROR("L%d Error - sem_destroy failed %d\n", __LINE__, errno);
    }

    if (-1 == sem_destroy(&pOmxTestInfo->ftb_sem))
    {
        SAMPLE_PRINT_ERROR("L%d Error - sem_destroy failed %d\n", __LINE__, errno);
    }

    if (-1 == sem_destroy(&pOmxTestInfo->in_flush_sem))
    {
        SAMPLE_PRINT_ERROR("L%d Error - sem_destroy failed %d\n", __LINE__, errno);
    }

    if (-1 == sem_destroy(&pOmxTestInfo->out_flush_sem))
    {
        SAMPLE_PRINT_ERROR("L%d Error - sem_destroy failed %d\n", __LINE__, errno);
    }

    if (-1 == sem_destroy(&pOmxTestInfo->seek_sem))
    {
        SAMPLE_PRINT_ERROR("L%d Error - sem_destroy failed %d\n", __LINE__, errno);
    }

    close((long)pOmxTestInfo->in_filename);

    pOmxTestInfo->is_open = 0;

    SAMPLE_PRINT_ALWS("Total received frame num %d\n", pOmxTestInfo->receive_frame_cnt);

    SAMPLE_PRINT_INFO("*********************************************\n");

    if (isDueToError)
        SAMPLE_PRINT_INFO("**************...TEST FAILED...**************\n");
    else
        SAMPLE_PRINT_INFO("************...TEST SUCCESSFULL...***********\n");

    SAMPLE_PRINT_INFO("*********************************************\n\n\n");

    return;
}

