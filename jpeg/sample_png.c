/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample png
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

#include "png.h"

typedef struct {
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int interlace_type;
    unsigned int row_bytes;
    char *out_buf;
    unsigned int channel_num;
} sample_png_info;

static void png_get_info(png_structp png_ptr, png_infop info_ptr, sample_png_info *png_info)
{
    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &png_info->width, &png_info->height, &png_info->bit_depth,
                 &png_info->color_type, &png_info->interlace_type, NULL, NULL);

    png_set_interlace_handling(png_ptr);

    if (png_info->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_expand(png_ptr);
    }

    if ((png_info->color_type == PNG_COLOR_TYPE_GRAY) && (png_info->bit_depth < 8)) { /* litter 8 bits is clut */
        png_set_expand(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    } else {
        png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    }

    if (png_info->bit_depth == 16) { /* 16 bits is argb 1555 */
        png_set_strip_16(png_ptr);
    }

    /* GRAY->RGB */
    if ((png_info->color_type == PNG_COLOR_TYPE_GRAY) || (png_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
        png_set_gray_to_rgb(png_ptr);
    }

    /* RGBA->BGRA */
    png_set_bgr(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &png_info->width,
                 &png_info->height, &png_info->bit_depth,
                 &png_info->color_type, &png_info->interlace_type, NULL, NULL);
    return;
}

static void png_decompress(char *file_name, sample_disp_info *disp_info)
{
    FILE *fp = NULL;
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int i;
    sample_png_info png_info = {0};

    fp = fopen(file_name, "rb");

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    info_ptr = png_create_info_struct(png_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);

    png_get_info(png_ptr, info_ptr, &png_info);

    png_bytep row_pointers[png_info.height];

    png_info.channel_num = png_get_channels(png_ptr, info_ptr);
    png_info.row_bytes = (png_info.channel_num * png_info.bit_depth * png_info.width) >> 3; /* 3 is 8 bytes align */
    if (png_info.row_bytes == 0) {
        png_info.row_bytes = 1;
    }

    png_info.out_buf = (char*)malloc(png_info.row_bytes * png_info.height);
    if (png_info.out_buf == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;
    }

    for (i = 0; i < png_info.height; i++) {
        row_pointers[i] = (unsigned char *)png_info.out_buf + i * png_info.row_bytes;
    }

    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, info_ptr);

    disp_info->image_info.width = png_info.width;
    disp_info->image_info.height = png_info.height;
    disp_info->image_info.stride = png_info.row_bytes;
    disp_info->image_info.buffer = png_info.out_buf;
    sample_jpeg_show_image(disp_info);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    free(png_info.out_buf);
    fclose(fp);

    return;
}

int sample_png_decompress(sample_disp_info *disp_info, char *file_name)
{
    png_decompress(file_name, disp_info);
    return 0;
}
