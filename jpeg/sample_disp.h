/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample disp
 * Author: sdk
 * Create: 2019-08-27
 */

#ifndef  __SAMPLE_DISP__
#define  __SAMPLE_DISP__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define KEY_VALUE 0x0
#define SAMPLE_JPEG_PRINT printf

typedef enum {
    false = 0,
    true = 1,
} bool;

typedef struct {
    int width;
    int height;
    int stride;
    char *buffer;
    int pos_x;
    int pos_y;
} sample_data_info;

typedef struct {
    unsigned int alpha;
    sample_data_info image_info;
    sample_data_info text_info;
} sample_disp_info;


int logo_text_init(int argc, char** argv);
void logo_text_deinit(void);
void logo_png_display(char *file_name);
void logo_jpeg_display(char *file_name);
void text_display(char *file_name);

int sample_png_decompress(sample_disp_info *disp_info, char *file_name);
int sample_jpeg_decompress(sample_disp_info *disp_info, char *file_name);

int sample_jpeg_text_init(void);
void sample_jpeg_text_deinit(void);
int sample_jpeg_text(sample_disp_info *disp_info, char *file_name);

int sample_jpeg_show_image(sample_disp_info *disp_info);
int sample_jpeg_show_text(sample_disp_info *disp_info);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /*__SAMPLE_DISP__ */
