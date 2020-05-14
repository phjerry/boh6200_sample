/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <linux/fb.h>
#include <sys/file.h>

#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_venc.h"
#include "hi_adp.h"

#define QFRAME_NUM      (6)
#define VENC_MAX_CHN    (8)
#define ALIGN_UP(val, align) ( (val+((align)-1))&~((align)-1) )
//#define VENC_PROPERTY_TEST

typedef struct {
    hi_u32  real_chn_num;
    hi_handle chan_id[VENC_MAX_CHN];
    FILE*       file_input[VENC_MAX_CHN];
    FILE*       file_output[VENC_MAX_CHN];
    hi_u32    pic_width[VENC_MAX_CHN];
    hi_u32    pic_height[VENC_MAX_CHN];
    hi_u32    time_out[VENC_MAX_CHN];
} sample_thread_info;

typedef struct {
    hi_u32  real_chn_num;
    hi_handle chan_id;
    FILE*       file_enc;
    hi_u32    width;
    hi_u32    height;
    hi_u32    index;
} queue_thread_info;

static pthread_t g_thread_send_frame[VENC_MAX_CHN];
queue_thread_info g_send_frame_info[VENC_MAX_CHN];
static hi_bool g_send_frame_stop = HI_FALSE;

static  hi_char mem_name_table[VENC_MAX_CHN][32] = {"VENC_MMZ_1", "VENC_MMZ_2", "VENC_MMZ_3", "VENC_MMZ_4",
    "VENC_MMZ_5", "VENC_MMZ_6", "VENC_MMZ_7", "VENC_MMZ_8"};

static hi_handle g_venc_chan_id[VENC_MAX_CHN];

static hi_char input_file_name_table[VENC_MAX_CHN][128];
static hi_char output_file_name_table[VENC_MAX_CHN][128];
static hi_u32 g_jpeg_save_type[VENC_MAX_CHN] = {0};

hi_unf_mem_buf g_frame_buf[VENC_MAX_CHN][QFRAME_NUM];

/******************************************************************
*************************** VENC **********************************
******************************************************************/

typedef struct {
    // hi_u32  real_chn_num;
    hi_handle   chan_id;
    FILE*       file_save;
    hi_u32      time_out;
    hi_char     file_name[128];
} save_stream_thread_info;

static pthread_t g_thread_save_stream[VENC_MAX_CHN];
static save_stream_thread_info g_save_stream_info[VENC_MAX_CHN];
static hi_bool g_save_stream_stop = HI_FALSE;
static hi_s32 yuv_store_type = 0;

typedef struct {
    hi_u32    pic_width;
    hi_u32    pic_height;
    hi_u32    enc_width;
    hi_u32    enc_height;
} src_input_info;

static void* sample_save_stream_process(void* args)
{
    save_stream_thread_info* thread_info = (save_stream_thread_info*)args;

    hi_s32 ret = HI_FAILURE;
    //hi_u32 i = 0;
    hi_unf_venc_stream vencStream;

    //hi_handle chan_id = thread_info->chan_id;
    //FILE *file = thread_info->file_save;
    //FILE *file;

    while (!g_save_stream_stop)
    {
        //        for(i = 0;i < thread_info->real_chn_num;i++)
        //        {
        ret = hi_unf_venc_acquire_stream(thread_info->chan_id, &vencStream, thread_info->time_out);
        if (HI_SUCCESS != ret)
        {
            usleep(2 * 1000);
            //sleep(1);
            continue;
        }

        if (thread_info->file_save) /* save to file */
        {
            if (vencStream.slc_len > 0)
            {
                ret = fwrite(vencStream.virt_addr, 1, vencStream.slc_len, thread_info->file_save);
                if (ret != vencStream.slc_len)
                {
                    //         printf("fwrite err, write size=%d, slclen=%d\n",ret, vencStream.slc_len);          //liminqi
                }
                //else  printf("fwrite success, write size=%d, slclen=%d\n",ret, vencStream.slc_len);
            }
        }
        else { printf("ERROR: the pFie is NULL!!\n"); }

        ret = hi_unf_venc_release_stream(thread_info->chan_id, &vencStream);
        if (HI_SUCCESS != ret)
        {
            printf("hi_unf_venc_release_stream failed, u32Ret=%x\n", ret);
        }
        //}
    }
    return NULL;
}

static void* sample_save_stream_process_mjpeg(void* args)
{
    save_stream_thread_info* thread_info = (save_stream_thread_info*)args;

    hi_s32 ret = HI_FAILURE;
    hi_u32 size = 0;
    hi_unf_venc_stream venc_stream;
    hi_s32 time_out = 0;
    hi_handle chan_id = thread_info->chan_id;
    FILE* file = thread_info->file_save;
    //hi_u32 FrameNum = thread_info->FrameNum;
    while (!g_save_stream_stop) //
    {
        memset(&venc_stream, 0, sizeof(hi_unf_venc_stream));
        ret = hi_unf_venc_acquire_stream(chan_id, &venc_stream, time_out);
        if (HI_SUCCESS != ret)
        {
            usleep(3 * 1000);
            //sleep(1);
            continue;
        }

        if (file) /* save to file */
        {
            size = venc_stream.slc_len;

            fwrite(&size, 1, sizeof(hi_u32), thread_info->file_save);

            if (venc_stream.slc_len > 0)
            {
                ret = fwrite(venc_stream.virt_addr, 1, venc_stream.slc_len, thread_info->file_save);
                if (ret != venc_stream.slc_len)
                {
                    printf("fwrite err0, write size=%d, slclen=%d\n", ret, venc_stream.slc_len);         //liminqi
                }
                /*else  printf("fwrite success0, write size=%d, slclen=%d,venc_stream.end = %d,pts = %d\n",
                    ret, venc_stream.slc_len,venc_stream.frame_end,venc_stream.pts_ms);*/
            }
        }
        else { printf("ERROR: the pFie is NULL!!\n"); }

        hi_unf_venc_release_stream(chan_id, &venc_stream);


    }
    return NULL;
}

static hi_u32 g_jpeg_num = 0;
static void* sample_save_stream_process_jpeg(void* args)
{
    save_stream_thread_info* thread_info = (save_stream_thread_info*)args;

    hi_s32 ret = HI_FAILURE;
    hi_unf_venc_stream venc_stream;
    hi_s32 time_out = 0;
    hi_char file_name[128];
    hi_char file_id[32];
    hi_handle chan_id = thread_info->chan_id;
    FILE* file = NULL;

    while (!g_save_stream_stop) //
    {
        memset(&venc_stream, 0, sizeof(hi_unf_venc_stream));
        ret = hi_unf_venc_acquire_stream(chan_id, &venc_stream, time_out);
        if (HI_SUCCESS != ret)
        {
            usleep(3 * 1000);
            continue;
        }

        //strncpy(file_name,thread_info->file_name,strlen(thread_info->file_name));
        memcpy(file_name, thread_info->file_name, sizeof(hi_char) * 128);
        snprintf(file_id, sizeof(file_name), "_%d.jpg", g_jpeg_num);
        strncat(file_name, file_id, strlen(file_id));
        file_name[127] = '\0';

        file = fopen(file_name, "wb+");
        if (file) /* save to file */
        {
            if (venc_stream.slc_len > 0)
            {
                ret = fwrite(venc_stream.virt_addr, 1, venc_stream.slc_len, file);
                if (ret != venc_stream.slc_len)
                {
                    printf("fwrite err0, write size=%d, slclen=%d\n", ret, venc_stream.slc_len);         //liminqi
                }
            }
        }
        else { printf("ERROR: the pFie is NULL!!\n"); }
        g_jpeg_num++;
        if (g_jpeg_num >= 9999) { g_jpeg_num = 0; }
        hi_unf_venc_release_stream(chan_id, &venc_stream);
        fclose(file);
        file = NULL;
    }
    return NULL;
}

enum
{
    YUV_SEMIPLANNER_420_UV  = 0,
    YUV_SEMIPLANNER_420_VU  = 1,
    YUV_PLANNER_420         = 2,
    YUV_PACKAGE_422_YUYV    = 3,
    YUV_PACKAGE_422_YVYU    = 4,
    YUV_PACKAGE_422_UYVY    = 5,
    YUV_SEMIPLANNER_422_UV  = 6, //没调通
    YUV_SEMIPLANNER_422_VU  = 7,//没调通
    YUV_UNKNOW_BUIT
};

enum
{
    V_U             = 0,
    U_V             = 1
};

#define SAMPLE_VEDU_UY0VY1 0x8d
#define SAMPLE_VEDU_Y0UY1V 0xd8
#define SAMPLE_VEDU_Y0VY1U 0x78

#define SAMPLE_VEDU_YUV420       0
#define SAMPLE_VEDU_YUV422       1
#define SAMPLE_VEDU_YUV444       2

#define SAMPLE_VEDU_SEMIPLANNAR  0
#define SAMPLE_VEDU_PACKAGE      1
#define SAMPLE_VEDU_PLANNAR      2

static hi_s32 yuvfile_to_memory( FILE*  yuv_file,
                              hi_u8* y_addr,      /* only by plannar, semiplannar,
package */
                              hi_u8* c_addr,      /* only by plannar, semiplannar */
                              hi_u8* v_addr,      /* only by plannar */
                              hi_s32    sample,  /* 420 422 444 */
                              hi_s32    store,   /* semiplannar, plannar, package */
                              hi_s32    w,
                              hi_s32    h,
                              hi_s32    y_stride,     /* only by semiplannar, plannar,
package */
                              hi_s32    c_stride,     /* only by semiplannar, plannar */
                              hi_s32    sel)
{
#define ERROR_READ { printf("error:read yuv\n"); return HI_FAILURE; }

    hi_s32    y_width = w, c_width, i;
    hi_s32    y_height = h, c_height, j;

    hi_u8* y_ptr = y_addr;
    hi_u8* u_ptr = c_addr;
    hi_u8* v_ptr = v_addr, *uv_ptr;

    if      ( sample == SAMPLE_VEDU_YUV420 ) { c_width = y_width >> 1, c_height = y_height >> 1; }
    else if ( sample == SAMPLE_VEDU_YUV422 ) { c_width = y_width >> 1, c_height = y_height >> 0; }
    else if ( sample == SAMPLE_VEDU_YUV444 ) { c_width = y_width >> 0, c_height = y_height >> 0; }
    else    { return HI_FAILURE; }

    if ( store == SAMPLE_VEDU_PLANNAR )
    {
        for ( j = 0; j < y_height; j++, y_ptr += y_stride )
        {
            if ( 1 != fread(y_ptr, y_width, 1,
                            yuv_file) ) { ERROR_READ; }
        }
        for ( j = 0; j < c_height; j++, u_ptr += c_stride )
        {
            if ( 1 != fread(u_ptr, c_width, 1,
                            yuv_file) ) { ERROR_READ; }
        }
        for ( j = 0; j < c_height; j++, v_ptr += c_stride )
        {
            if ( 1 != fread(v_ptr, c_width, 1,
                            yuv_file) ) { ERROR_READ; }
        }
    }
    else if ( store == SAMPLE_VEDU_SEMIPLANNAR )
    {
        hi_s32 t0 = sel & 1;
        hi_s32 t1 = 1 - t0;

        if ( NULL == (uv_ptr = (hi_u8*)malloc(c_stride)) )
        {
            printf("error: malloc @ read yuv\n");
            return HI_FAILURE;
        }
        v_ptr = c_addr + c_stride / 2;

        for ( j = 0; j < y_height; j++, y_ptr += y_stride )
        {
            if ( 1 != fread(y_ptr, y_width, 1,
                            yuv_file) ) { ERROR_READ; }
        }
        for ( j = 0; j < c_height; j++, u_ptr += c_stride )
        {
            if ( 1 != fread(u_ptr, c_width, 1,
                            yuv_file) ) { ERROR_READ; }
        }
        for ( j = 0; j < c_height; j++, v_ptr += c_stride )
        {
            if ( 1 != fread(v_ptr, c_width, 1,
                            yuv_file) ) { ERROR_READ; }
        }

        u_ptr = c_addr;
        v_ptr = c_addr + c_stride / 2;

        for (j = 0; j < c_height; j++, u_ptr += c_stride, v_ptr += c_stride)
        {
            for ( i = 0; i < c_width; i++ )
            {
                uv_ptr[i * 2 + t0] = *(v_ptr + i);
                uv_ptr[i * 2 + t1] = *(u_ptr + i);
            }
            memcpy(u_ptr, uv_ptr, 2 * c_width);
        }

        free(uv_ptr);
    }
    else if ( store == SAMPLE_VEDU_PACKAGE )
    {
        hi_s32 y0_sel = (sel >> 0) & 3;
        hi_s32 y1_sel = (sel >> 2) & 3;
        hi_s32 u0_sel = (sel >> 4) & 3;
        hi_s32 v0_sel = (sel >> 6) & 3;

        if ( NULL == (uv_ptr = (hi_u8*)malloc(y_width)) )
        {
            printf("error: malloc @ read yuv\n");
            return HI_FAILURE;
        }

        if ( sample == SAMPLE_VEDU_YUV420 )
        {
            for ( j = 0; j < y_height; j++, y_ptr += y_stride )
            {
                if ( 1 != fread(uv_ptr, y_width, 1, yuv_file) ) { ERROR_READ; }

                for ( i = y_width / 2 - 1; i >= 0; i-- )
                {
                    *(y_ptr + i * 4 + y0_sel) = *(uv_ptr + i * 2    );
                    *(y_ptr + i * 4 + y1_sel) = *(uv_ptr + i * 2 + 1);
                }
            }

            u_ptr = y_addr + u0_sel;
            for ( j = 0; j < c_height; j++, u_ptr += y_stride * 2 )
            {
                if ( 1 != fread(uv_ptr, c_width, 1, yuv_file) ) { ERROR_READ; }

                for ( i = c_width - 1; i >= 0; i-- )
                {
                    *(u_ptr + i * 4) = *(uv_ptr + i);
                }
            }

            v_ptr = y_addr + v0_sel;
            for ( j = 0; j < c_height; j++, v_ptr += y_stride * 2 )
            {
                if ( 1 != fread(uv_ptr, c_width, 1, yuv_file) ) { ERROR_READ; }

                for ( i = c_width - 1; i >= 0; i-- )
                {
                    *(v_ptr + i * 4) = *(uv_ptr + i);
                }
            }
        }
        else
        {
            //hi_s32 t = ( sample == VEDU_YUV444 ? 4 : 2 );

            for ( j = 0; j < y_height; j++, y_ptr += y_stride )
            {
                if ( 1 != fread(uv_ptr, y_width, 1, yuv_file) ) { ERROR_READ; }

                if ( sample == SAMPLE_VEDU_YUV422 )
                    for ( i = y_width / 2 - 1; i >= 0; i-- )
                    {
                        *(y_ptr + i * 4 + y0_sel) = *(uv_ptr + i * 2    );
                        *(y_ptr + i * 4 + y1_sel) = *(uv_ptr + i * 2 + 1);
                    }
                else
                    for ( i = y_width - 1; i >= 0; i-- )
                    {
                        *(y_ptr + i * 4 + y0_sel) = *(uv_ptr + i);
                    }
            }

            /*
                        u_ptr = y_addr + u0_sel;
                        for( j = 0; j < c_height; j++, u_ptr += y_stride )
                        {
                            if( 1 != fread(uv_ptr, c_width, 1, yuv_file) ) ERROR_READ;

                            for( i = c_width - 1; i >= 0; i-- )
                            {
                                *(u_ptr + i * 4) = *(uv_ptr + i);
                            }
                        }

                        v_ptr = y_addr + v0_sel;
                        for( j = 0; j < c_height; j++, v_ptr += y_stride )
                        {
                            if( 1 != fread(uv_ptr, c_width, 1, yuv_file) ) ERROR_READ;

                            for( i = c_width - 1; i >= 0; i-- )
                            {
                                *(v_ptr + i * 4) = *(uv_ptr + i);
                            }
                        }
            */
            u_ptr = y_addr + u0_sel;
            for ( j = 0; j < c_height; j++, u_ptr += y_stride )
            {
                if (j % 2 == 0)
                {
                    if ( 1 != fread(uv_ptr, c_width, 1, yuv_file) ) { ERROR_READ; }
                }

                for ( i = c_width - 1; i >= 0; i-- )
                {
                    *(u_ptr + i * 4) = *(uv_ptr + i);
                }
            }

            v_ptr = y_addr + v0_sel;
            for ( j = 0; j < c_height; j++, v_ptr += y_stride )
            {
                if (j % 2 == 0)
                {
                    if ( 1 != fread(uv_ptr, c_width, 1, yuv_file) ) { ERROR_READ; }
                }

                for ( i = c_width - 1; i >= 0; i-- )
                {
                    *(v_ptr + i * 4) = *(uv_ptr + i);
                }
            }
        }


        free(uv_ptr);
    }
    else { return HI_FAILURE; }

    return HI_SUCCESS;
}

static hi_s32 read_buffer_from_yuv_file (queue_thread_info* thread_info, hi_u8* data)
{
    hi_s32 frame_size = 0;
    hi_s32 offset_y_cb = 0;
    hi_s32 offset_Y_cr = 0;
    hi_u8* y_addr = NULL;
    hi_u8* c_addr = NULL;
    hi_u8* v_addr = NULL;
    hi_s32 sample = 0;
    hi_s32 store = 0;
    hi_s32 y_stride = 0;
    hi_s32 c_stride = 0;
    hi_s32 sel = 0;

    switch (yuv_store_type)
    {
        case YUV_SEMIPLANNER_420_UV:
            y_stride   = ALIGN_UP(thread_info->width, 16);
            c_stride   = y_stride;
            offset_y_cb  = y_stride * thread_info->height;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV420;
            store  = SAMPLE_VEDU_SEMIPLANNAR;
            y_addr     = data;
            c_addr     = y_addr + offset_y_cb;
            v_addr     = NULL;
            sel       = U_V;
            frame_size = y_stride * thread_info->height + c_stride * thread_info->height / 2;
            break;
        case YUV_SEMIPLANNER_420_VU:
            y_stride   = ALIGN_UP(thread_info->width, 16);
            c_stride   = y_stride;
            offset_y_cb  = y_stride * thread_info->height;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV420;
            store  = SAMPLE_VEDU_SEMIPLANNAR;
            y_addr     = data;
            c_addr     = y_addr + offset_y_cb;
            v_addr     = NULL;
            sel       = V_U;
            frame_size = y_stride * thread_info->height + c_stride * thread_info->height / 2;
            break;
        case YUV_PLANNER_420:
            y_stride   = ALIGN_UP(thread_info->width, 16);
            c_stride   = ALIGN_UP(thread_info->width / 2, 16);
            offset_y_cb  = y_stride * thread_info->height;
            offset_Y_cr = c_stride * thread_info->height / 2;
            sample = SAMPLE_VEDU_YUV420;
            store  = SAMPLE_VEDU_PLANNAR;
            y_addr     = data;
            c_addr     = y_addr + offset_y_cb;
            v_addr     = c_addr + offset_Y_cr;
            sel       = 0;
            frame_size = y_stride * thread_info->height + c_stride * thread_info->height;
            break;
        case YUV_SEMIPLANNER_422_UV:
            y_stride   = ALIGN_UP(thread_info->width, 16);
            c_stride   = ALIGN_UP(thread_info->width, 16);        //??
            offset_y_cb  = y_stride * thread_info->height;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV422;
            store  = SAMPLE_VEDU_SEMIPLANNAR;
            y_addr     = data;
            c_addr     = y_addr + offset_y_cb;
            v_addr     = NULL;
            sel       = U_V;
            frame_size = y_stride * thread_info->height + c_stride * thread_info->height;
            break;
        case YUV_SEMIPLANNER_422_VU:
            y_stride   = ALIGN_UP(thread_info->width, 16);
            c_stride   = ALIGN_UP(thread_info->width, 16);        //??
            offset_y_cb  = y_stride * thread_info->height;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV422;
            store  = SAMPLE_VEDU_SEMIPLANNAR;
            y_addr     = data;
            c_addr     = y_addr + offset_y_cb;
            v_addr     = NULL;
            sel       = V_U;
            frame_size = y_stride * thread_info->height + c_stride * thread_info->height;
            break;

        case YUV_PACKAGE_422_YUYV:
            y_stride   = ALIGN_UP(thread_info->width * 2, 16);
            c_stride   = 0;        //??
            offset_y_cb  = 0;//y_stride * picHeight;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV422;
            store  = SAMPLE_VEDU_PACKAGE;
            y_addr     = data;
            c_addr     = NULL;
            v_addr     = NULL;
            sel       = SAMPLE_VEDU_Y0UY1V;    //??
            frame_size = y_stride * thread_info->height;
            break;
        case YUV_PACKAGE_422_YVYU:
            y_stride   = ALIGN_UP(thread_info->width * 2, 16);
            c_stride   = 0;        //??
            offset_y_cb  = 0;//y_stride * picHeight;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV422;
            store  = SAMPLE_VEDU_PACKAGE;
            y_addr     = data;
            c_addr     = NULL;
            v_addr     = NULL;
            sel       = SAMPLE_VEDU_Y0VY1U;    //??
            frame_size = y_stride * thread_info->height;
            break;
        case YUV_PACKAGE_422_UYVY:
            y_stride   = ALIGN_UP(thread_info->width * 2, 16);
            c_stride   = 0;        //??
            offset_y_cb  = 0;//y_stride * picHeight;
            offset_Y_cr = 0;
            sample = SAMPLE_VEDU_YUV422;
            store  = SAMPLE_VEDU_PACKAGE;
            y_addr     = data;
            c_addr     = NULL;
            v_addr     = NULL;
            sel       = SAMPLE_VEDU_UY0VY1;    //??
            frame_size = y_stride * thread_info->height;
            break;
    }

    if (HI_SUCCESS != yuvfile_to_memory(thread_info->file_enc, y_addr, c_addr, v_addr,
                                        sample, store, thread_info->width, thread_info->height, y_stride, c_stride, sel))
    {
        return 0;
    }

    return frame_size;
}

hi_u32 calc_per_frame_size(hi_u32 width, hi_u32 height, hi_u32 format)
{
    hi_u32 frame_buf_size = 0;
    if(YUV_PLANNER_420 == format)
    {
        frame_buf_size = (ALIGN_UP(width, 16) + ALIGN_UP(width/2, 16) ) * height;
    }
    else if (YUV_SEMIPLANNER_420_UV == format || YUV_SEMIPLANNER_420_VU == format)
    {
        frame_buf_size =  ALIGN_UP(width, 16) * height * 3 / 2;
    }
    else if (YUV_PACKAGE_422_YUYV == format
          || YUV_PACKAGE_422_YVYU == format
          || YUV_PACKAGE_422_UYVY == format)
    {
        frame_buf_size =  ALIGN_UP(width, 16) * height * 2;
    }
    else if (YUV_SEMIPLANNER_422_UV == format
          || YUV_SEMIPLANNER_422_VU == format)
    {
        frame_buf_size = ALIGN_UP(width, 16) * height * 2;
    }
    else
    {
        printf("not support yuv store type!!!\n");
    }

    return frame_buf_size;
}

hi_s32 alloc_frame_buf(hi_u32 chan_id, hi_u32 width, hi_u32 height)
{
    hi_unf_mem_buf buf;
    hi_u32 i;
    hi_u32 per_size;
    hi_s32 ret;

    per_size = calc_per_frame_size(width, height, yuv_store_type);
    if (per_size == 0) {
        return HI_FAILURE;
    }

    sprintf(buf.buf_name, "%s", mem_name_table[chan_id]);
    buf.buf_size = per_size;

    for (i = 0; i < QFRAME_NUM; i++) {
        ret = hi_unf_mem_malloc(&buf);
        if (ret == HI_FAILURE) {
            printf("~~~~~~ERROR: hi_unf_mem_malloc(%d) Failed!! Ret:%d\n", chan_id, ret);

            return HI_FAILURE;
        }
        memcpy(&g_frame_buf[chan_id][i], &buf, sizeof(hi_unf_mem_buf));
    }

    return HI_SUCCESS;
}


hi_s32 free_frame_buf(hi_u32 chan_id)
{
    hi_s32 ret;
    hi_u32 i;

    for (i = 0; i < QFRAME_NUM; i++) {
        ret = hi_unf_mem_free(&g_frame_buf[chan_id][i]);
        if (ret == HI_FAILURE)
        {
            printf("~~~~~~ERROR: hi_unf_mem_free Failed!! Ret:%d\n", ret);
            return HI_FAILURE;
        }
        memset(&g_frame_buf[chan_id][i], 0, sizeof(hi_unf_mem_buf));
    }

    return HI_SUCCESS;
}

void* sample_send_frame_process(void* args)
{
    hi_s32 ret;
    hi_s64 pts = 1;

    hi_unf_video_frame_info frame_info;
    hi_unf_video_frame_info frame_info_release;
    hi_u32 per_size = 0;

    queue_thread_info* thread_info = (queue_thread_info*)args;
    hi_u32 queue_id  = 0;
    hi_u32 read_len = 0;
    //====================================== MMZ申请内存
    hi_unf_mem_buf buf;

    hi_u8* data = malloc(3840 * 2160 * 3);

    per_size = calc_per_frame_size(thread_info->width, thread_info->height, yuv_store_type);
    if (per_size == 0)
    {
        return HI_NULL;
    }

    //======================================

    for (queue_id = 0; queue_id < QFRAME_NUM; queue_id++)
    {
        memcpy(&buf, &g_frame_buf[thread_info->index][queue_id], sizeof(hi_unf_mem_buf));
        memset(&frame_info, 0, sizeof(hi_unf_video_frame_info));
        memset(&frame_info_release, 0, sizeof(hi_unf_video_frame_info));

        read_len = read_buffer_from_yuv_file(thread_info, data);
        if (read_len > 0)
        {
            memcpy(buf.user_viraddr, data, per_size);

            frame_info.display_width = thread_info->width;
            frame_info.display_height = thread_info->height;
            frame_info.decode_width = thread_info->width;
            frame_info.decode_height = thread_info->height;
            frame_info.scan_type = HI_UNF_VIDEO_SCAN_TYPE_PROGRESSIVE;
            frame_info.field_mode = HI_UNF_VIDEO_FIELD_ALL;
            frame_info.frame_packing_type = HI_UNF_FRAME_PACKING_TYPE_2D;
            if (YUV_SEMIPLANNER_420_UV == yuv_store_type || YUV_SEMIPLANNER_420_VU == yuv_store_type)
            {
                frame_info.frame_addr[0].stride_y = ALIGN_UP(thread_info->width, 16);
                frame_info.frame_addr[0].stride_c = ALIGN_UP(thread_info->width, 16);

                frame_info.frame_addr[0].start_addr = buf.buf_handle;
                frame_info.frame_addr[0].y_offset = 0;
                frame_info.frame_addr[0].c_offset = frame_info.frame_addr[0].stride_y * thread_info->height;

                if (YUV_SEMIPLANNER_420_VU == yuv_store_type)
                {
                    frame_info.video_format = HI_UNF_FORMAT_YUV_SEMIPLANAR_420_VU;
                }
                else
                {
                    frame_info.video_format = HI_UNF_FORMAT_YUV_SEMIPLANAR_420_UV;
                }
            }
            else if (YUV_PLANNER_420 == yuv_store_type)
            {
                frame_info.frame_addr[0].stride_y = ALIGN_UP(thread_info->width, 16);
                frame_info.frame_addr[0].stride_c = ALIGN_UP(thread_info->width/2, 16);
                frame_info.frame_addr[0].stride_cr = ALIGN_UP(thread_info->width/2, 16);

                frame_info.frame_addr[0].start_addr = buf.buf_handle;
                frame_info.frame_addr[0].y_offset = 0;
                frame_info.frame_addr[0].c_offset = frame_info.frame_addr[0].stride_y * thread_info->height;
                frame_info.frame_addr[0].cr_offset = frame_info.frame_addr[0].c_offset +
                                                     frame_info.frame_addr[0].stride_c * thread_info->height / 2;
                frame_info.video_format = HI_UNF_FORMAT_YUV_PLANAR_420_UV;
            }
            else if (YUV_PACKAGE_422_YUYV == yuv_store_type ||
                     YUV_PACKAGE_422_YVYU == yuv_store_type ||
                     YUV_PACKAGE_422_UYVY == yuv_store_type)
            {
                frame_info.frame_addr[0].stride_y = ALIGN_UP(thread_info->width * 2, 16);

                frame_info.frame_addr[0].start_addr = buf.buf_handle;
                frame_info.frame_addr[0].y_offset = 0;

                if (YUV_PACKAGE_422_YUYV == yuv_store_type)
                {
                    frame_info.video_format                  = HI_UNF_FORMAT_YUV_PACKAGE_YUYV;
                }
                else if (YUV_PACKAGE_422_YVYU == yuv_store_type)
                {
                    frame_info.video_format                  = HI_UNF_FORMAT_YUV_PACKAGE_YVYU;
                }
                else
                {
                    frame_info.video_format                  = HI_UNF_FORMAT_YUV_PACKAGE_UYVY;
                }
            }

            frame_info.pts                         = pts++;
        }
        else
        {
            printf("read vid file error!\n");
            rewind(thread_info->file_enc);
            continue;
        }

        ret = hi_unf_venc_queue_frame(thread_info->chan_id, &frame_info);
        if (ret != HI_SUCCESS)
        {
            printf("the venc inter frame buffer is full(frame num =  %d)!\n", queue_id);
            break;
        }
    }

    while (!g_send_frame_stop)
    {
        do
        {
            ret = hi_unf_venc_dequeue_frame(thread_info->chan_id, &frame_info_release);
            usleep(5 * 1000);
        }
        while ((0 != ret) && (!g_send_frame_stop));

        int i;
        for (i = 0; i < QFRAME_NUM; i++) {
            if (g_frame_buf[thread_info->index][i].buf_handle == frame_info_release.frame_addr[0].start_addr) {
                memcpy(&buf, &g_frame_buf[thread_info->index][i], sizeof(hi_unf_mem_buf));
                break;
            }
        }
        if (i == QFRAME_NUM) {
            printf("Can not find corresponding buffer\n");
            continue;
        }

#ifndef VENC_PROPERTY_TEST
        read_len = read_buffer_from_yuv_file(thread_info, data);
        if (read_len > 0)
        {
            memcpy(buf.user_viraddr, data, per_size);
            frame_info_release.pts = pts++;
        }
        else
        {
            printf("read vid file error!\n");
            rewind(thread_info->file_enc);
            read_len = read_buffer_from_yuv_file(thread_info, data);
            if (read_len > 0)
            {
                memcpy(buf.user_viraddr, data, per_size);
                frame_info_release.pts = pts++;
            }
            else
            {
                printf("ERROR :: miss one Framebuffer!!\n");
                continue;
            }
        }
#else
        frame_info_release.pts  = pts++;
#endif
        if(g_send_frame_stop != HI_TRUE)
        {
            ret = hi_unf_venc_queue_frame(thread_info->chan_id, &frame_info_release);
            if (HI_SUCCESS != ret)
            {
                printf("QueueFrame err!ret = %x\n", ret);
                continue;
            }
        }
    }

    free(data);

    return NULL;
}


hi_s32 HI_ADP_VENC_Create(hi_unf_venc_attr* pstVeAttr, hi_handle* phVenc)
{
    hi_s32 ret = HI_SUCCESS;
    hi_handle chan_id;

    if (phVenc == NULL)
    {
        printf("%s,%d,bad phVenc !\n", __func__, __LINE__);
    }

    HIAPI_RUN(hi_unf_venc_create(&chan_id, pstVeAttr), ret);
    if (ret != HI_SUCCESS)
    {
        printf("HI_UNF_VENC_CreateChn failed!!\n");
        hi_unf_venc_deinit();
        return ret;
    }

    *phVenc = chan_id;
    return ret;
}


hi_s32 HI_ADP_VENC_Start(hi_handle chan_id)
{
    hi_s32 ret = HI_SUCCESS;
    HIAPI_RUN(hi_unf_venc_start(chan_id), ret);
    if (ret != HI_SUCCESS)
    {
        printf("hi_unf_venc_start failed!!\n");
        //hi_unf_venc_detach_input(chan_id, hVi);
        hi_unf_venc_destroy(chan_id);
        hi_unf_venc_deinit();
        return ret;
    }
    return ret;
}

hi_s32 HI_ADP_VENC_Thread_Create(sample_thread_info* input)
{
    hi_u32 i = 0;
    if (!input)
    {
        printf("%s,%d,bad input param !\n", __func__, __LINE__);
        return HI_FAILURE;
    }

    for (i = 0; i < input->real_chn_num; i++)
    {
        g_save_stream_info[i].chan_id        = input->chan_id[i];
        g_save_stream_info[i].file_save  = input->file_output[i];
        g_save_stream_info[i].time_out      = input->time_out[i];

        if (g_jpeg_save_type[i] == 2)
        {
            memcpy(g_save_stream_info[i].file_name, output_file_name_table[i], sizeof(g_save_stream_info[i].file_name));
        }
    }
    //g_save_stream_info.real_chn_num = input->real_chn_num;
    g_save_stream_stop = HI_FALSE;

    for (i = 0; i < input->real_chn_num; i++)
    {
        if (g_jpeg_save_type[i] == 1)   /*mjpeg*/
        {
            pthread_create(&g_thread_save_stream[i], HI_NULL, sample_save_stream_process_mjpeg, (void*)(&g_save_stream_info[i]));
        }
        else if ((g_jpeg_save_type[i] == 0)||(g_jpeg_save_type[i] == 3)) /*h264/hevc*/
        {
            pthread_create(&g_thread_save_stream[i], HI_NULL, sample_save_stream_process, (void*)(&g_save_stream_info[i]));
        }
        else if (g_jpeg_save_type[i] == 2) /*jpeg*/
        {
            pthread_create(&g_thread_save_stream[i], HI_NULL, sample_save_stream_process_jpeg, (void*)(&g_save_stream_info[i]));
        }

        usleep(2 * 1000);
    }

    for (i = 0; i < input->real_chn_num; i++)
    {
        g_send_frame_info[i].chan_id       = input->chan_id[i];
        g_send_frame_info[i].width    = input->pic_width[i];
        g_send_frame_info[i].height   = input->pic_height[i];
        g_send_frame_info[i].file_enc  = input->file_input[i];

    }
    //g_send_frame_info.real_chn_num = input->real_chn_num;
    g_send_frame_stop = HI_FALSE;
    for (i = 0; i < input->real_chn_num; i++)
    {
        g_send_frame_info[i].index = i;
        pthread_create(&g_thread_send_frame[i], HI_NULL, sample_send_frame_process, (void*)(&g_send_frame_info[i]));
        usleep(2 * 1000);
    }
    return HI_SUCCESS;
}

hi_void HI_ADP_VENC_Thread_Stop(hi_u32 chnNum)
{
    hi_u32 i = 0;
    if (!g_save_stream_stop)
    {
        g_save_stream_stop = HI_TRUE;
        for (i = 0; i < chnNum; i++)
        {
            pthread_join(g_thread_save_stream[i], NULL);
        }
    }


    if (!g_send_frame_stop)
    {
        g_send_frame_stop = HI_TRUE;
        for (i = 0; i < chnNum; i++)
        {
            pthread_join(g_thread_send_frame[i], NULL);
        }
    }

    printf("Thread stop OK!!\n");

}


hi_s32 HI_ADP_VENC_Stop(hi_handle chan_id)
{
    hi_s32 ret = HI_SUCCESS;
    HIAPI_RUN(hi_unf_venc_stop(chan_id), ret);
    return ret;
}

hi_s32 HI_ADP_VENC_Destroy(hi_handle chan_id)
{
    hi_s32 ret = HI_SUCCESS;
    HIAPI_RUN(hi_unf_venc_destroy(chan_id), ret);
    return ret;
}


/*hi_s32 HI_ADP_VIVENC_DeInit(hi_handle chan_id)
{
    hi_s32 ret = HI_SUCCESS;
    HIAPI_RUN(hi_unf_venc_stop(chan_id), ret);

    HIAPI_RUN(hi_unf_venc_destroy(chan_id), ret);

    //HIAPI_RUN(hi_unf_venc_deinit(), ret);

    if (!g_send_frame_stop)
    {
        g_send_frame_stop = HI_TRUE;
        pthread_join(g_thread_send_frame, NULL);
    }
    return ret;
}*/


static hi_s32 process_current_command(hi_handle chan_id, hi_s32 cmd)
{
    hi_s32 ret = 0;
#if 0
    hi_bool flag = 0;
    hi_unf_venc_attr stAttr;
    ret = hi_unf_venc_get_attr(chan_id, &stAttr);
    if ( HI_SUCCESS != ret )
    {
        printf("ERROR:VENC GetAttr FAILED!(%d)\n", __LINE__);
    }

    switch (cmd)
    {
        case 0:   /*Input FrameRate*/
        {
            hi_u32 inputFrmRate_temp;
            while ( !flag )
            {
                printf("\nCurrent input FrameRate -> %d,target FrameRate -> %d\n", stAttr.input_frame_rate, stAttr.target_frame_rate);
                printf("Active Change the input FrameRate to -> ");
                fflush(stdin);
                scanf("%d", &inputFrmRate_temp);
                if (inputFrmRate_temp < stAttr.target_frame_rate )
                {
                    printf("\nPlease input the input FrameRate again!(should > targetFrmRate(%d))\n", stAttr.target_frame_rate);
                }
                else
                {
                    stAttr.input_frame_rate =  inputFrmRate_temp;
                    printf("\nThe new input FrameRate -> %d\n", stAttr.input_frame_rate);
                    flag = 1;
                }
            }
            break;
        }
        case 1:   /*Target FrameRate*/
        {
            hi_u32 targetFrmRate_temp;
            while ( !flag )
            {
                printf("\nCurrent input FrameRate -> %d,target FrameRate -> %d\n", stAttr.input_frame_rate, stAttr.target_frame_rate);
                printf("Active Change the target FrameRate to -> ");
                fflush(stdin);
                scanf("%d", &targetFrmRate_temp);
                if (targetFrmRate_temp > stAttr.input_frame_rate)
                {
                    printf("\nPlease input the target FrameRate again!(should < inputFrmRate(%d))\n", stAttr.input_frame_rate);
                }
                else
                {
                    stAttr.target_frame_rate =  targetFrmRate_temp;
                    printf("\nThe new target FrameRate -> %d\n", stAttr.target_frame_rate);
                    flag = 1;
                }
            }
            break;
        }
        case 2:   /*Target BitRate*/
        {
            hi_u32 targetBitRate_temp;
            while ( !flag )
            {
                printf("\nCurrent Resolution Ratio  -> %d * %d\n", stAttr.width, stAttr.height);
                printf("Current target BitRate(unit:kbps) -> %d\n", stAttr.target_bitrate / 1000);
                printf("Active Change the target BitRate(unit:kbps) to -> ");
                fflush(stdin);
                scanf("%d", &targetBitRate_temp);
                if ( (stAttr.width >= 1920) && (targetBitRate_temp < 1000))
                {
                    printf("\nWARNING: the new target BitRate(%d) maybe too small!\n", targetBitRate_temp);
                    flag = 1;
                }
                else
                {
                    stAttr.target_bitrate =  targetBitRate_temp * 1000;
                    printf("\nThe new target BitRate(unit:kbps) -> %d\n", targetBitRate_temp);
                    flag = 1;
                }
            }
            break;
        }
        case 3:   /*GOP*/
        {
            hi_u32 GOP_temp;
            printf("\nCurrent GOP  -> %d \n", stAttr.gop);
            printf("Active Change the GOP to -> ");
            fflush(stdin);
            scanf("%d", &GOP_temp);

            stAttr.gop =  GOP_temp;
            printf("\nThe new GOP -> %d\n", stAttr.gop );

            break;
        }
        case 4:   /*MaxQP*/
        {
            printf("This feature is currently not supported");
            break;
        }
        case 5:   /*MinQP*/
        {
            printf("This feature is currently not supported");
            break;
        }
        case 6:   /*channel Priority*/
        {
            hi_u32 Priority_temp;
            while ( !flag )
            {
                printf("\nCurrent channel Priority -> %d\n", stAttr.priority);
                printf("Active Change the channel Priority to(should be 0~%d) -> ", VENC_MAX_CHN - 1);
                fflush(stdin);
                scanf("%d", &Priority_temp);
                if ( Priority_temp > VENC_MAX_CHN - 1)
                {
                    printf("\nPlease input the Priority again!(should be 0~%d)\n", VENC_MAX_CHN - 1);
                }
                else
                {
                    stAttr.priority =  Priority_temp;
                    printf("\nThe new Priority of channel(0x%x) -> %d\n", chan_id, stAttr.priority);
                    flag = 1;
                }
            }
            break;
        }
        default:
            printf("\nUnknow Cmd(%d)\n", cmd);
            break;
    }

    ret = hi_unf_venc_set_attr(chan_id, &stAttr);
    if (HI_SUCCESS == ret)
    {
        printf("\nCmd(%d) change OK!!\n", cmd);
    }
#endif
    return ret;
}

static void loop_function(hi_s32 chnNum)
{
    hi_s32  cmd_error = 0;

    while (cmd_error == 0)
    {
        hi_s32  cmd_id = -1;
        hi_s32  channel_id = -1;
        hi_char inputCmd;

        while ( (cmd_id > 7) || (cmd_id < 0) )
        {
            if (cmd_id > 7)
            {
                printf("\nPlease input the  correct Cmd again!\n\n");
            }
            printf("\n*************  input 'q' to exit!**************\n");
            printf("please choose the Channel ID(0~%d) to control :", chnNum - 1);
            fflush(stdin);

            do
            {
                scanf("%c", &inputCmd);
            } while (inputCmd == 10);

            if (('q' == inputCmd) || ('Q' == inputCmd) )
            {
                printf("test-case exit!!\n");
                //cmd_error = 1;
                return ;
            }

            if ((inputCmd >= '0') && (inputCmd <= '9'))
            {
                channel_id = inputCmd - 48;
            }
            else
            {
                printf("\nPlease input the  Channel ID again!\n\n");
                continue;
            }

            if ((channel_id >= chnNum) || (channel_id < 0))
            {
                printf("\nPlease input the  Channel ID again!\n\n");
                continue;
            }

            printf("*********************************************\n");
            printf("please choose the Cmd as follows :\n");
            printf("*********************************************\n");
            printf("0 --> active change the Input FrameRate\n");
            printf("1 --> active change the Target FrameRate\n");
            printf("2 --> active change the Target BitRate\n");
            printf("3 --> active change the GOP\n");
            printf("4 --> active change the MaxQP\n");
            printf("5 --> active change the MinQP\n");
            printf("6 --> active change the priority\n");
            printf("7 --> exit \n");
            fflush(stdin);
            scanf("%d", &cmd_id);
        }

        if (7 == cmd_id)
        {
            printf("test-case exit!!\n");
            cmd_error = 1;
            break;
        }

        cmd_error = process_current_command(g_venc_chan_id[channel_id], cmd_id);
        cmd_id = -1;
    }
}

hi_s32 main(hi_s32 argc, hi_char* argv[])
{
    hi_s32 ret;
    hi_unf_venc_attr stVeAttr = {0};
    //hi_handle chan_id[VENC_MAX_CHN];
    hi_s32 i = 0;
    hi_char file_name[64] = {""};
    hi_u32 chn_num = 0;
    sample_thread_info thread_arg;
    hi_u32 chn_width[VENC_MAX_CHN] = {0};
    hi_u32 chn_height[VENC_MAX_CHN] = {0};
    hi_u32 chn_priority[VENC_MAX_CHN] = {0};
    hi_unf_vcodec_type venc_type[VENC_MAX_CHN] = {0};

    hi_char inputFileName[VENC_MAX_CHN][20] = {};
    if ( 2 == argc )
    {
        if (!strcmp("user-defined", argv[1]))
        {
            printf("input the num of VENC channel:");
            fflush(stdin);
            scanf("%d", &chn_num);
            if (chn_num > VENC_MAX_CHN)
            {
                printf("ERROR:: the num of VENC channel is invalid !(should < %d)", VENC_MAX_CHN);
                return 0;
            }
            thread_arg.real_chn_num = chn_num;
            for (i = 0; i < chn_num; i++ )
            {
                printf("input the channel of type[%d] (0->h264 1->mjpeg 2->jpeg 3->h265)    :", i);
                fflush(stdin);
                scanf("%d", &g_jpeg_save_type[i]);

                printf("\ninput the Input Filename of channel[%d]:", i);
                fflush(stdin);
                scanf("%s", &(inputFileName[i][0]));
                thread_arg.file_input[i]  = fopen(inputFileName[i], "rb");
                if (NULL == thread_arg.file_input[i])
                {
                    printf("ERROR:open file :%s failed!!\n", inputFileName[i]);               //liminqi
                    return 0;
                }

                strncpy(output_file_name_table[i], inputFileName[i], (strlen(inputFileName[i]) - 4));

                if (g_jpeg_save_type[i] == 1)
                {
                    venc_type[i] = HI_UNF_VCODEC_TYPE_MJPEG;
                    snprintf(file_name, sizeof(file_name), "_chn%d.mjpeg", i);
                    strncat(output_file_name_table[i], file_name, strlen(file_name));
                    output_file_name_table[i][127] = '\0';
                }
                else if (g_jpeg_save_type[i] == 0)
                {
                    venc_type[i] = HI_UNF_VCODEC_TYPE_H264;
                    snprintf(file_name, sizeof(file_name), "_chn%d.h264", i);
                    strncat(output_file_name_table[i], file_name, strlen(file_name));
                    output_file_name_table[i][127] = '\0';
                }
                else if (g_jpeg_save_type[i] == 2)
                {
                    venc_type[i] = HI_UNF_VCODEC_TYPE_MJPEG;
                    snprintf(file_name, sizeof(file_name), "_chn%d", i);
                    strncat(output_file_name_table[i], file_name, strlen(file_name));
                    output_file_name_table[i][127] = '\0';
                }
                else if (g_jpeg_save_type[i] == 3)
                {
                    venc_type[i] = HI_UNF_VCODEC_TYPE_H265;
                    snprintf(file_name, sizeof(file_name), "_chn%d.h265", i);
                    strncat(output_file_name_table[i], file_name, strlen(file_name));
                    output_file_name_table[i][127] = '\0';
                }

                if (g_jpeg_save_type[i] != 2)
                {
                    thread_arg.file_output[i]  = fopen(output_file_name_table[i], "wb+");
                    if (NULL == thread_arg.file_output[i])
                    {
                        printf("ERROR:open file :%s failed!!\n", output_file_name_table[i]);               //liminqi
                        return 0;
                    }
                }

                printf("input the Picture width of channel[%d]:", i);
                fflush(stdin);
                scanf("%d", &thread_arg.pic_width[i]);

                printf("input the Picture height of channel[%d]:", i);
                fflush(stdin);
                scanf("%d", &thread_arg.pic_height[i]);

                printf("\ninput the Encode width of channel[%d]:", i);
                fflush(stdin);
                scanf("%d", &chn_width[i]);

                printf("input the Encode height of channel[%d]:", i);
                fflush(stdin);
                scanf("%d", &chn_height[i]);

                printf("input the priority of channel[%d]:", i);
                fflush(stdin);
                scanf("%d", &chn_priority[i]);
            }
        }
        else
        {
            goto error_0;
        }
    }
    else if (argc == 5)
    {
        chn_num = 1;
        thread_arg.real_chn_num = chn_num;
        thread_arg.pic_width[0] = strtol(argv[2], NULL, 0);
        thread_arg.pic_height[0] = strtol(argv[3], NULL, 0);
        chn_width[0]               = strtol(argv[2], NULL, 0);
        chn_height[0]              = strtol(argv[3], NULL, 0);
        chn_priority[0]            = 0;

        if(!strcmp(argv[4], "H264"))
        {
            venc_type[0] = HI_UNF_VCODEC_TYPE_H264;
            printf("H264 Type is success!\n");
        }
        else if (!strcmp(argv[4], "H265"))
        {
            venc_type[0] = HI_UNF_VCODEC_TYPE_H265;
            printf("H265 Type is success!\n");
        }
        else
        {
            printf("Please input the VencType H264 or H265!\n");

            return 0;
        }

        g_jpeg_save_type[0]              = 0;
        strncpy (input_file_name_table[0], argv[1], 127);
        input_file_name_table[0][127] = '\0';
        strncpy(output_file_name_table[0], argv[1], (strlen(argv[1]) - 4));

        if (venc_type[0] == HI_UNF_VCODEC_TYPE_H264)
        {
            strncat(output_file_name_table[i], "_chn00.h264", strlen("_chn00.h264"));
        }
        else
        {
            strncat(output_file_name_table[i], "_chn00.h265", strlen("_chn00.h265"));  //c00346155
        }

        output_file_name_table[0][127] = '\0';
        thread_arg.file_input[0]   = fopen(input_file_name_table[0], "rb");
        if (NULL == thread_arg.file_input[0])
        {
            printf("ERROR:open file :%s failed!!\n", inputFileName[0]);               //liminqi
            return 0;
        }
        thread_arg.file_output[0]  = fopen(output_file_name_table[0], "wb+");
        if (NULL == thread_arg.file_output[0])
        {
            printf("ERROR:open file :%s failed!!\n", output_file_name_table[0]);               //liminqi
            return 0;
        }
    }
    else
    {
        goto error_0;
    }
    printf("*********************************************\n");
    printf("please enter input YUV store/sample type\n");
    printf("*********************************************\n");
    printf("0 --> SP420_YCbCr\n");
    printf("1 --> SP420_YCrCb\n");
    printf("2 --> Planner 420\n");
    printf("3 --> Package422_YUYV\n");
    printf("4 --> Package422_YVYU\n");
    printf("5 --> Package422_UYVY\n");

    fflush(stdin);
    scanf("%d", &yuv_store_type);
    fflush(stdin);
    if (yuv_store_type >= YUV_UNKNOW_BUIT)
    {
        printf("Wrong option of YUV store/sample type! type(%d) not support!\n", yuv_store_type);
        return -1;
    }

    hi_unf_sys_init();

    HIAPI_RUN_RETURN(hi_unf_venc_init());

    for (i = 0; i < chn_num; i++)
    {
        hi_unf_venc_get_default_attr(&stVeAttr);

        stVeAttr.venc_type = venc_type[i];
        stVeAttr.max_width = chn_width[i];
        stVeAttr.max_height = chn_height[i];
        stVeAttr.stream_buf_size   = stVeAttr.max_width * stVeAttr.max_height * 3 / 2;
        stVeAttr.slice_split = HI_FALSE;
        stVeAttr.split_size = 10;
        stVeAttr.gop_mode = HI_UNF_VENC_GOP_MODE_NORMALP;

        stVeAttr.config.width = chn_width[i];
        stVeAttr.config.height = chn_height[i];
        stVeAttr.config.target_frame_rate = 25 * 1000;
        stVeAttr.config.input_frame_rate = 25 * 1000;
        stVeAttr.config.priority = chn_priority[i];
        stVeAttr.config.quick_encode = HI_FALSE;
        stVeAttr.config.gop          = 100;
		stVeAttr.config.sp_interval = 0;
        stVeAttr.config.input_frame_rate_type = HI_UNF_VENC_FRAME_RATE_TYPE_USER;
        if (stVeAttr.config.width > 2160 || stVeAttr.config.height > 2160)
        {
            stVeAttr.config.target_bitrate = 5 * 1024 * 1024;
        }
        else if (stVeAttr.config.width > 1920  || stVeAttr.config.height > 1088)
        {
            stVeAttr.config.target_bitrate = 5 * 1024 * 1024;
        }
        else if (stVeAttr.config.width > 1280)
        {
            stVeAttr.config.target_bitrate = 5 * 1024 * 1024;
        }
        else if (stVeAttr.config.width > 720)
        {
            stVeAttr.config.target_bitrate = 3 * 1024 * 1024;
        }
        else if (stVeAttr.config.width > 352)
        {
            stVeAttr.config.target_bitrate = 3 / 2 * 1024 * 1024;
        }
        else
        {
            stVeAttr.config.target_bitrate = 800 * 1024;
        }

        if ((g_jpeg_save_type[i] == 2) || (g_jpeg_save_type[i] == 1))
        {
            stVeAttr.config.qfactor    = 70;
        }
        HI_ADP_VENC_Create(&stVeAttr, &g_venc_chan_id[i]);

        thread_arg.chan_id[i] = g_venc_chan_id[i];
        thread_arg.time_out[i] = 0;
        if (HI_SUCCESS != alloc_frame_buf(i, stVeAttr.config.width, stVeAttr.config.height))
        {
            return -1;
        }

        HI_ADP_VENC_Start(g_venc_chan_id[i]);
    }

    HI_ADP_VENC_Thread_Create(&thread_arg);

    sleep(1);

    /////////////////////////////////////////

    loop_function(chn_num);

    /////////////////////////////////////////  收尾工作

    HI_ADP_VENC_Thread_Stop(chn_num);

    for (i = 0; i < chn_num; i++)
    {
        HI_ADP_VENC_Stop(g_venc_chan_id[i]);
        HI_ADP_VENC_Destroy(g_venc_chan_id[i]);
        if (g_jpeg_save_type[i] != 2)
        {
            fclose(thread_arg.file_output[i]);
        }
        fclose(thread_arg.file_input[i]);

        free_frame_buf(i);
    }

    HIAPI_RUN(hi_unf_venc_deinit(), ret);

    ret = hi_unf_sys_deinit();
    if (HI_SUCCESS != ret)
    {
        printf("call HI_SYS_DeInit() failed.\n");
    }

    return ret;

error_0:
    printf("\n      Usage1 : %s filename.yuv  Picture_Width  Picture_Height  Encode_Type\n", argv[0]);
	printf("      Example: %s a_1280x720.yuv  1280  720  H265\n", argv[0]);
    printf("      Usage2: %s user-defined\n\n", argv[0]);
    return 0;

}
