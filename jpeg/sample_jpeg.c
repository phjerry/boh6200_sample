/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample jpeg
 * Author: sdk
 * Create: 2019-08-27
 */

#include "sample_disp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <setjmp.h>

#include "jpeglib.h"
#include "jerror.h"

/* define infomation */
#define USE_FIX_JPEG_NAME
#define JPEG_FILE_NAME "./../res/logo.jpg"
#define JPEG_FILE_DIR  "./../res/jpg/"
#define JPEG_CHAR_LEN_MAX 256

/* declare struct */
typedef struct {
    struct jpeg_error_mgr pub;
} jpeg_my_err;

static jmp_buf g_setjmp_buffer;

static void test_dec_err(j_common_ptr cinfo)
{
    (*cinfo->err->output_message)(cinfo);
    longjmp(g_setjmp_buffer, 1);
}

#ifndef USE_FIX_JPEG_NAME
static int jpeg_get_filename(char *file_name, char *decode_file_name)
{
    char *file_pos = NULL;

    file_pos = strrchr(file_name, '.');
    if (file_pos == NULL) {
        return -1;
    }

    file_pos++;
    if (strncasecmp(file_pos, "jpeg", 2) != 0) { /* check 2 bytes */
        return -1;
    }

    memset(decode_file_name, 0, JPEG_CHAR_LEN_MAX);
    strncpy(decode_file_name, JPEG_FILE_DIR, strlen(JPEG_FILE_DIR));
    strncat(decode_file_name, file_name, strlen(file_name));

    return 0;
}
#endif

static void jpeg_decompress(char *file_name, sample_disp_info *disp_info)
{
    struct jpeg_decompress_struct cinfo;
    jpeg_my_err jerr;
    unsigned int stride, mem_size;
    FILE *file_fd = NULL;
    char *out_buf = NULL;
    char *tmp_buf = NULL;

    file_fd = fopen(file_name, "rb");
    if (file_fd == NULL) {
        SAMPLE_JPEG_PRINT("open %s file failure\n", file_name);
        return;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = test_dec_err;

    if (setjmp(g_setjmp_buffer)) {
        goto DEC_FINISH;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, file_fd);

    jpeg_read_header(&cinfo, TRUE);

    cinfo.scale_num   = 1 ;
    cinfo.scale_denom = 1;
    cinfo.out_color_space = JCS_EXT_BGRA;

    jpeg_calc_output_dimensions(&cinfo);

    stride  = (cinfo.output_width * 4 + 16 - 1) & (~(16 - 1)); /* 4 bytes for argb 8888, 16bytes align */
    mem_size = stride * cinfo.output_height;
    if (mem_size == 0) {
        SAMPLE_JPEG_PRINT("mem_size is zero\n");
        goto DEC_FINISH;
    }

    out_buf = (char*)malloc(mem_size);
    if (out_buf == NULL) {
        SAMPLE_JPEG_PRINT("malloc dec mem failure\n");
        goto DEC_FINISH;
    }

    jpeg_start_decompress(&cinfo);

    while (cinfo.output_scanline < cinfo.output_height) {
        tmp_buf = out_buf + stride * cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&tmp_buf, 1);
    }

    jpeg_finish_decompress(&cinfo);

    disp_info->image_info.width = cinfo.output_width;
    disp_info->image_info.height = cinfo.output_height;
    disp_info->image_info.stride = stride;
    disp_info->image_info.buffer = out_buf;
    sample_jpeg_show_image(disp_info);

DEC_FINISH:
    jpeg_destroy_decompress(&cinfo);
    fclose(file_fd);

    if (out_buf != NULL) {
        free(out_buf);
    }

    return;
}

int sample_jpeg_decompress(sample_disp_info *disp_info, char *file_name)
{
#ifndef USE_FIX_JPEG_NAME
    int ret;
    char file_name[JPEG_CHAR_LEN_MAX] = {0};
    DIR *dir = NULL;
    struct dirent *ptr = NULL;

    dir = opendir(JPEG_FILE_DIR);
    if (dir == NULL) {
        SAMPLE_JPEG_PRINT("open jpeg file directory %s failure\n", JPEG_FILE_DIR);
        return -1;
    }

    ptr = readdir(dir);

    while (ptr != NULL) {
        ret = jpeg_get_filename(ptr->d_name, file_name);
        if (ret == -1) {
            ptr = readdir(dir);
        } else {
            SAMPLE_JPEG_PRINT("\n=========================================================================\n");
            SAMPLE_JPEG_PRINT("decode jpeg file is %s\n", file_name);
            SAMPLE_JPEG_PRINT("=========================================================================\n");
            jpeg_decompress(file_name, disp_info);
            ptr = readdir(dir);
        }
    }

    closedir(dir);
#else
    jpeg_decompress(file_name, disp_info);
#endif
    return 0;
}
