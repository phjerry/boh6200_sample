/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample text
 * Author: sdk
 * Create: 2019-08-27
 */

#include "sample_disp.h"

#include <unistd.h>

#include "ft2build.h"
#include FT_TRUETYPE_TABLES_H
#include FT_SYSTEM_H
#include FT_MODULE_H
#include FT_SIZES_H
#include FT_BBOX_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H
#include FT_OUTLINE_H
#include FT_CACHE_H
#include FT_CACHE_MANAGER_H
#include FT_ERRORS_H
#include FT_SYSTEM_H
#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H

/* define infomation */
#define USE_FIX_JPEG_NAME
#define TEXT_SHOW_NAME   "HISILICON-1920-1080-420.jpg"
#define TEXT_FILE_DIR    "./../res/text/"
#define TEXT_FILE_NAME   "./../res/DroidSansFallbackLegacy.ttf"
#define TEXT_CHAR_LEN_MAX 256
#define TEXT_CHAR_NUM     8
#define BYTES 4

typedef struct {
    char *bitmap_buffer;
    int bitmap_top;
    int bitmap_left;
    int bitmap_rows;
    int bitmap_width;
    int bitmap_pitch;
    int advance_x;
} sample_text_info;

static FT_Library g_library = NULL;
static FT_Face g_face = NULL;
static char *g_text_buf = NULL;
static unsigned int g_width = 1920;
static unsigned int g_height = 200;
static unsigned int g_stride = 7680;

char g_text_string[TEXT_CHAR_NUM][TEXT_CHAR_LEN_MAX] = {
    "Ballet_HEVC_AAC_8KP60_HDR_160Mbps.ts",
    "Ballet_HEVC_AAC_8KP120_HDR_255Mbps_NOK.ts",
    "Baseball_HEVC_AAC_8KP60_HDR_160Mbps.ts",
    "Baseball_HEVC_AAC_8KP120_HDR_270Mbps.ts",
    "Embroid_HEVC_AAC_8KP60_SDR_190Mbps.ts",
    "Scenery_HEVC_AAC_8KP60_SDR_250Mbps.ts",
    "Sword_HEVC_AAC_8KP60_HDR_160Mbps.ts",
    "Sword_HEVC_AAC_8KP120_HDR_255Mbps.ts"
};

static void text_render(sample_text_info *text_info, unsigned int offset)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int bitmap_top;
    int base_line_offset;
    char *single_text_buf;
    char *tmp_bitmap_buf = NULL;

    if (text_info->bitmap_buffer == NULL) {
        SAMPLE_JPEG_PRINT("text_info->bitmap_buffer is null\n");
        return;
    }

    /* 计算当前显示字符基于基线的位置 */
    bitmap_top = (text_info->bitmap_top > 0) ? text_info->bitmap_top : (-text_info->bitmap_top);
    if (offset < bitmap_top * g_stride) {
        SAMPLE_JPEG_PRINT("base_line_offset is wrong\n");
        return;
    }

    base_line_offset = offset - bitmap_top * g_stride + text_info->bitmap_left * BYTES;
    if (base_line_offset > g_stride * g_height) {
        SAMPLE_JPEG_PRINT("base_line_offset is to large\n");
        return;
    }

    for (i = 0; i < text_info->bitmap_rows; i++) {
        tmp_bitmap_buf = text_info->bitmap_buffer + text_info->bitmap_pitch * i;
        single_text_buf = g_text_buf + base_line_offset + g_stride * i;
        for (j = 0; j < text_info->bitmap_width; j++) {
            single_text_buf[0] = tmp_bitmap_buf[0]; /* 0 is R */
            single_text_buf[1] = tmp_bitmap_buf[0]; /* 1 is G */
            single_text_buf[2] = tmp_bitmap_buf[0]; /* 2 is B */
            single_text_buf[3] = 0x80; /* 3 is A */
            tmp_bitmap_buf++;
            single_text_buf += BYTES;
#ifdef TEXT_DEBUG
            if (tmp_bitmap_buf[0] == 0xff) {
                SAMPLE_JPEG_PRINT("*");
            } else if (tmp_bitmap_buf[0] == 0) {
                SAMPLE_JPEG_PRINT(" ");
            } else {
                SAMPLE_JPEG_PRINT(".");
            }
#endif
        }
#ifdef TEXT_DEBUG
        SAMPLE_JPEG_PRINT("\n");
#endif
    }

    return;
}

static int sample_text_render(char *file_name, sample_disp_info *disp_info)
{
    unsigned int i;
    unsigned int offset = 0;
    sample_text_info text_info;

    FT_Glyph glyph;
    FT_Error error;
    FT_BitmapGlyph bitmap_glyph;
    FT_Bitmap bitmap;
    FT_UInt glyph_index;

    memset(&text_info, 0x0, sizeof(text_info));
    memset(g_text_buf, KEY_VALUE, g_stride * g_height);

    for (i = 0; i < strlen(file_name); i++) {
        glyph_index = FT_Get_Char_Index(g_face, file_name[i]);
        error = FT_Load_Glyph(g_face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
        if (error != 0) {
            SAMPLE_JPEG_PRINT("FT_Load_Glyph failure\n");
            goto ERROR_EXIT;
        }

        error = FT_Get_Glyph(g_face->glyph, &glyph);
        if (error != 0) {
            SAMPLE_JPEG_PRINT("FT_Get_Glyph failure\n");
            goto ERROR_EXIT;
        }

        FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 0);

        bitmap_glyph = (FT_BitmapGlyph)glyph;

        bitmap = bitmap_glyph->bitmap;

        text_info.bitmap_buffer = (char*)bitmap.buffer;
        text_info.bitmap_top = g_face->glyph->bitmap_top;
        text_info.bitmap_left = g_face->glyph->bitmap_left;
        text_info.bitmap_rows = bitmap.rows;
        text_info.bitmap_width = bitmap.width;
        text_info.bitmap_pitch = bitmap.pitch;
        text_info.advance_x = g_face->glyph->advance.x;
        if (offset == 0) {
            offset = text_info.bitmap_top * g_stride + text_info.bitmap_left * BYTES + g_stride + g_stride + g_stride;
        }
        text_render(&text_info, offset);
        /* render next one char */
        offset += ((g_face->glyph->advance.x >> 6) * BYTES); /* right shift 6 bits */

        FT_Done_Glyph(glyph);
    }

    disp_info->text_info.width = g_width;
    disp_info->text_info.height = g_height;
    disp_info->text_info.stride = g_stride;
    disp_info->text_info.buffer = g_text_buf;
    sample_jpeg_show_text(disp_info);

ERROR_EXIT:
    return 0;
}

int sample_jpeg_text(sample_disp_info *disp_info, char *file_name)
{
#ifndef USE_FIX_JPEG_NAME
    unsigned int len;
    DIR *dir = NULL;
    struct dirent *ptr = NULL;

    dir = opendir(TEXT_FILE_DIR);
    if (dir == NULL) {
        SAMPLE_JPEG_PRINT("open text file directory %s failure\n", TEXT_FILE_DIR);
        return -1;
    }

    ptr = readdir(dir);

    while (ptr != NULL) {
        len = strlen(ptr->d_name);
        if (len < 5) { /* file len need at least 5 bytes */
            break;
        }
        SAMPLE_JPEG_PRINT("\n=========================================================================\n");
        SAMPLE_JPEG_PRINT("text file is %s\n", ptr->d_name);
        SAMPLE_JPEG_PRINT("=========================================================================\n");
        sample_text_render(ptr->d_name, disp_info);
        ptr = readdir(dir);
    }

    closedir(dir);
#else
    sample_text_render(file_name, disp_info);
#endif
    return 0;
}

int sample_jpeg_text_init(void)
{
    FT_Error error;

    error = FT_Init_FreeType(&g_library);
    if (error != 0) {
        SAMPLE_JPEG_PRINT("FT_Init_FreeType failure\n");
        return -1;
    }

    error = FT_New_Face(g_library, TEXT_FILE_NAME, 0, &g_face);
    if (error != 0) {
        FT_Done_FreeType(g_library);
        SAMPLE_JPEG_PRINT("FT_New_Face %s failure\n", TEXT_FILE_NAME);
        return -1;
    }

    error = FT_Set_Char_Size(g_face, 0, 64 << 6, 72, 72); /* 64 6 72 is text width and height */
    if (error != 0) {
        FT_Done_Face(g_face);
        FT_Done_FreeType(g_library);
        SAMPLE_JPEG_PRINT("FT_Set_Char_Size failure\n");
        return -1;
    }

    g_text_buf = (char*)malloc(g_stride * g_height);
    if (g_text_buf == NULL) {
        FT_Done_Face(g_face);
        FT_Done_FreeType(g_library);
        SAMPLE_JPEG_PRINT("malloc text mem failure\n");
        return -1;
    }
    memset(g_text_buf, 0x0, g_stride * g_height);

    sleep(1);
    return 0;
}

void sample_jpeg_text_deinit(void)
{
    if (g_face != NULL) {
        FT_Done_Face(g_face);
    }
    g_face = NULL;

    if (g_library != NULL) {
        FT_Done_FreeType(g_library);
    }
    g_library = NULL;

    if (g_text_buf != NULL) {
        free(g_text_buf);
    }
    g_text_buf = NULL;

    return;
}
