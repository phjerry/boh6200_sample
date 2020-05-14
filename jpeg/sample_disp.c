/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample disp
 * Author: sdk
 * Create: 2019-08-27
 */

#include "sample_disp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "securec.h"
#ifdef SAMPLE_JPEG_DISP_INIT
#include "hi_adp_mpi.h"
#endif
#include "hifb.h"

/* define infomation */
#define UI_WIDTH  3840
#define UI_HEIGHT 2160
#define BYTES 4
#define DISP_FMT HI_UNF_ENC_FMT_1080P_60

static struct fb_var_screeninfo g_fb_vinfo_argb8888 = {
    UI_WIDTH, UI_HEIGHT, UI_WIDTH, UI_HEIGHT, 0, 0,
    32,
    0,
    {16, 8, 0},
    { 8, 8, 0},
    { 0, 8, 0},
    {24, 8, 0},
    0, FB_ACTIVATE_FORCE, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1,
};

static int g_fb_fd;
static void *g_mapped_mem = NULL;
static sample_disp_info g_disp_info = {0};

/* declare function */
#ifdef SAMPLE_JPEG_DISP_INIT
static int disp_init(void);
static void disp_deinit(void);
#endif
static int gfx_init(void);
static void gfx_deinit(void);

/* release function */
static int sample_jpeg_parse_argc(int argc, char** argv, sample_disp_info *disp_info)
{
    int i;
    for (i = 1; i < argc; i++) {
        if (!strncmp("pic_x:", argv[i], strlen("pic_x:") > strlen(argv[i]) ? strlen("pic_x:") : strlen(argv[i]))) {
            disp_info->image_info.pos_x = atoi(argv[i + 1]);
        }

        if (!strncmp("pic_y:", argv[i], strlen("pic_y:") > strlen(argv[i]) ? strlen("pic_y:") : strlen(argv[i]))) {
            disp_info->image_info.pos_y = atoi(argv[i + 1]);
        }

        if (!strncmp("text_x:", argv[i], strlen("text_x:") > strlen(argv[i]) ? strlen("text_x:") : strlen(argv[i]))) {
            disp_info->text_info.pos_x = atoi(argv[i + 1]);
        }

        if (!strncmp("text_y:", argv[i], strlen("text_y:") > strlen(argv[i]) ? strlen("text_y:") : strlen(argv[i]))) {
            disp_info->text_info.pos_y = atoi(argv[i + 1]);
        }

        if (!strncmp("alpha:", argv[i], strlen("alpha:") > strlen(argv[i]) ? strlen("alpha:") : strlen(argv[i]))) {
            disp_info->alpha = atoi(argv[i + 1]);
        }

        if (!strncmp("--help", argv[i], strlen("--help") > strlen(argv[i]) ? strlen("--help") : strlen(argv[i]))) {
            SAMPLE_JPEG_PRINT("use example:\n");
            SAMPLE_JPEG_PRINT("./sample_disp pic_x: 50 pic_y: 100 text_x: 1500 text_y: 1800 alpha: 125\n");
            return -1;
        }
    }

    return 0;
}

#ifdef CONFIG_DISP_CALL
int main(int argc, char** argv)
{
    int ret;

#ifdef SAMPLE_JPEG_DISP_INIT
    ret = disp_init();
    if (ret != 0) {
        SAMPLE_JPEG_PRINT("call disp_init failure\n");
        return -1;
    }
    sleep(1);
#endif

    ret = logo_text_init(argc, argv);
    if (ret != 0) {
#ifdef SAMPLE_JPEG_DISP_INIT
        disp_deinit();
#endif
        SAMPLE_JPEG_PRINT("call logo_text_init failure\n");
        return -1;
    }

    logo_png_display("./../res/logo.png");

    text_display("Ballet_HEVC_AAC_8KP120_HDR_255Mbps_NOK.ts");

    SAMPLE_JPEG_PRINT("enter to exit\n");
    getchar();

    logo_text_deinit();

#ifdef SAMPLE_JPEG_DISP_INIT
    disp_deinit();
#endif

    return 0;
}
#endif

static int gfx_init(void)
{
    int ret;
    struct fb_fix_screeninfo finfo;
    hifb_colorkey color_key = {0, 0};

    g_fb_fd = open("/dev/fb0", O_RDWR, 0);
    if (g_fb_fd < 0) {
        SAMPLE_JPEG_PRINT("unable to open /dev/fb0\n");
        return -1;
    }

    ret = ioctl(g_fb_fd, FBIOPUT_VSCREENINFO, &g_fb_vinfo_argb8888);
    if (ret < 0) {
        ret = -1;
        SAMPLE_JPEG_PRINT("unable to set variable screeninfo!\n");
        goto CLOSEFD;
    }

    ret = ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &finfo);
    if (ret < 0) {
        ret = -1;
        SAMPLE_JPEG_PRINT("couldn't get fscreen info info\n");
        goto CLOSEFD;
    }

    color_key.enable = HI_TRUE;
    color_key.key_value = KEY_VALUE;
    ret = ioctl(g_fb_fd, HIFBIOPUT_COLORKEY, &color_key);
    if (ret < 0) {
        ret = -1;
        SAMPLE_JPEG_PRINT("couldn't set color key\n");
        goto CLOSEFD;
    }

    g_mapped_mem = mmap(NULL, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, g_fb_fd, 0);
    if (g_mapped_mem == NULL) {
        SAMPLE_JPEG_PRINT("unable to map the video mem\n");
        g_mapped_mem = NULL;
        ret = -1;
        goto CLOSEFD;
    }
    memset(g_mapped_mem, KEY_VALUE, finfo.smem_len);

    return 0;

CLOSEFD:
    if (g_mapped_mem != NULL) {
        munmap(g_mapped_mem, finfo.smem_len);
    }

    if (g_fb_fd > 0) {
        close(g_fb_fd);
    }
    g_fb_fd = -1;

    return ret;
}

static void gfx_deinit(void)
{
    struct fb_fix_screeninfo finfo;

    ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &finfo);

    if ((g_mapped_mem != NULL) && (finfo.smem_len != 0)) {
        munmap(g_mapped_mem, finfo.smem_len);
    }

    if (g_fb_fd > 0) {
        close(g_fb_fd);
    }
    g_fb_fd = -1;

    return;
}

int sample_jpeg_show_image(sample_disp_info *disp_info)
{
    int ret;
    int row, offset;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    char *tmp_src_buf = NULL;
    char *tmp_disp_buf = NULL;
    hifb_alpha alpha_info = {1, 1, 0, 255, 255};

    if (disp_info == NULL) {
        SAMPLE_JPEG_PRINT("disp_info is null\n");
        return -1;
    }

    ret = ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &finfo);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("couldn't get fscreen info info\n");
        return -1;
    }

    ret = ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("couldn't get vscreen info info\n");
        return -1;
    }

    alpha_info.global_alpha = disp_info->alpha;
    ret = ioctl(g_fb_fd, HIFBIOPUT_ALPHA, &alpha_info);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("couldn't set alpha\n");
        return -1;
    }

    tmp_src_buf = disp_info->image_info.buffer;
    offset = disp_info->image_info.pos_x * BYTES + disp_info->image_info.pos_y * finfo.line_length;
    tmp_disp_buf = g_mapped_mem + offset;

    if ((finfo.smem_len - offset) < (disp_info->image_info.stride * disp_info->image_info.height) ||
        (finfo.smem_len - offset) < (finfo.line_length * disp_info->image_info.height)) {
        SAMPLE_JPEG_PRINT("dst mem is not enough\n");
        return -1;
    }

    for (row = 0; row < disp_info->image_info.height; row++) {
        memcpy_s(tmp_disp_buf, disp_info->image_info.width * BYTES, tmp_src_buf, disp_info->image_info.width * BYTES);
        tmp_disp_buf += finfo.line_length;
        tmp_src_buf += disp_info->image_info.stride;
    }

    ret = ioctl(g_fb_fd, FBIOPAN_DISPLAY, &vinfo);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("pan_display failed\n");
        return -1;
    }

    return 0;
}

int sample_jpeg_show_text(sample_disp_info *disp_info)
{
    int ret;
    int row, offset;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    char *tmp_src_buf = NULL;
    char *tmp_disp_buf = NULL;
    hifb_alpha alpha_info = {1, 1, 0, 255, 255};

    if (disp_info == NULL) {
        SAMPLE_JPEG_PRINT("disp_info is null\n");
        return -1;
    }

    ret = ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &finfo);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("couldn't get fscreen info info\n");
        return -1;
    }

    ret = ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("couldn't get vscreen info info\n");
        return -1;
    }

    alpha_info.global_alpha = disp_info->alpha;
    ret = ioctl(g_fb_fd, HIFBIOPUT_ALPHA, &alpha_info);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("couldn't set alpha\n");
        return -1;
    }

    tmp_src_buf = disp_info->text_info.buffer;
    offset = disp_info->text_info.pos_x * BYTES + disp_info->text_info.pos_y * finfo.line_length;
    tmp_disp_buf = g_mapped_mem + offset;

    if ((finfo.smem_len - offset) < (disp_info->text_info.stride * disp_info->text_info.height) ||
        (finfo.smem_len - offset) < (finfo.line_length * disp_info->text_info.height)) {
        SAMPLE_JPEG_PRINT("dst mem is not enough\n");
        return -1;
    }

    for (row = 0; row < disp_info->text_info.height; row++) {
        memcpy_s(tmp_disp_buf, disp_info->text_info.width * BYTES, tmp_src_buf, disp_info->text_info.width * BYTES);
        tmp_disp_buf += finfo.line_length;
        tmp_src_buf += disp_info->text_info.stride;
    }

    ret = ioctl(g_fb_fd, FBIOPAN_DISPLAY, &vinfo);
    if (ret < 0) {
        SAMPLE_JPEG_PRINT("pan_display failed\n");
        return -1;
    }

    return 0;
}

#ifdef SAMPLE_JPEG_DISP_INIT
static int disp_init(void)
{
    int ret;

    hi_unf_sys_init();

    HIADP_MCE_Exit();

    ret = hi_adp_disp_init(DISP_FMT);
    if (ret != 0) {
        return -1;
    }

    HI_UNF_DISP_SetVirtualScreen(HI_UNF_DISPLAY1, UI_WIDTH, UI_HEIGHT);

    sleep(1);

    return 0;
}

static void disp_deinit(void)
{
    hi_adp_disp_deinit();
    hi_unf_sys_deinit();
    return;
}
#endif

int logo_text_init(int argc, char** argv)
{
    int ret;

    ret = sample_jpeg_parse_argc(argc, argv, &g_disp_info);
    if (ret != 0) {
        SAMPLE_JPEG_PRINT("sample_jpeg_parse_argc failure\n");
        return -1;
    }

    ret = gfx_init();
    if (ret != 0) {
        SAMPLE_JPEG_PRINT("call gfx_init failure\n");
        return -1;
    }

    ret = sample_jpeg_text_init();
    if (ret != 0) {
        gfx_deinit();
        SAMPLE_JPEG_PRINT("call gfx_init failure\n");
        return -1;
    }

    return 0;
}

void logo_text_deinit(void)
{
    sample_jpeg_text_deinit();
    gfx_deinit();
    return;
}


void logo_jpeg_display(char *file_name)
{
    sample_jpeg_decompress(&g_disp_info, file_name);
    return;
}

void logo_png_display(char *file_name)
{
    sample_png_decompress(&g_disp_info, file_name);
    return;
}

void text_display(char *file_name)
{
    sample_jpeg_text(&g_disp_info, file_name);
    return;
}
