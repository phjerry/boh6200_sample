/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description: hifb sample
 * Create: 2016-01-01
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <assert.h>

#include "hi_type.h"
#include "hifb.h"
#include "hi_unf_disp.h"
#include "hi_adp_mpi.h"

#define HIFB_SAMPLE_PRINT  printf

#define HIFB_WIDTH 1280
#define HIFB_HEIGHT 720

static struct fb_var_screeninfo g_hifb_def_vinfo = {
    HIFB_WIDTH,
    HIFB_HEIGHT,
    HIFB_WIDTH,
    HIFB_HEIGHT,
    0,
    0,
    32,
    0,
    { 16, 8, 0 },
    { 8, 8, 0 },
    { 0, 8, 0 },
    { 24, 8, 0 },
    0,
    FB_ACTIVATE_FORCE,
    0,
    0,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
    HIFB_SAMPLE_PRINT("Printing vinfo:\n");
    HIFB_SAMPLE_PRINT("xres: %d\n", vinfo->xres);
    HIFB_SAMPLE_PRINT("yres: %d\n", vinfo->yres);
    HIFB_SAMPLE_PRINT("xres_virtual: %d\n", vinfo->xres_virtual);
    HIFB_SAMPLE_PRINT("yres_virtual: %d\n", vinfo->yres_virtual);
    HIFB_SAMPLE_PRINT("xoffset: %d\n", vinfo->xoffset);
    HIFB_SAMPLE_PRINT("yoffset: %d\n", vinfo->yoffset);
    HIFB_SAMPLE_PRINT("bits_per_pixel: %d\n", vinfo->bits_per_pixel);
    HIFB_SAMPLE_PRINT("red: %d/%d\n", vinfo->red.length, vinfo->red.offset);
    HIFB_SAMPLE_PRINT("green: %d/%d\n", vinfo->green.length, vinfo->green.offset);
    HIFB_SAMPLE_PRINT("blue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
    HIFB_SAMPLE_PRINT("alpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
}

static void print_finfo(struct fb_fix_screeninfo *finfo)
{
    HIFB_SAMPLE_PRINT("Printing finfo:\n");
    HIFB_SAMPLE_PRINT("smem_start = %p\n", (char *)finfo->smem_start);
    HIFB_SAMPLE_PRINT("smem_len = %d\n", finfo->smem_len);
    HIFB_SAMPLE_PRINT("type = %d\n", finfo->type);
    HIFB_SAMPLE_PRINT("type_aux = %d\n", finfo->type_aux);
    HIFB_SAMPLE_PRINT("visual = %d\n", finfo->visual);
    HIFB_SAMPLE_PRINT("xpanstep = %d\n", finfo->xpanstep);
    HIFB_SAMPLE_PRINT("ypanstep = %d\n", finfo->ypanstep);
    HIFB_SAMPLE_PRINT("ywrapstep = %d\n", finfo->ywrapstep);
    HIFB_SAMPLE_PRINT("line_length = %d\n", finfo->line_length);
    HIFB_SAMPLE_PRINT("mmio_start = %p\n", (char *)finfo->mmio_start);
    HIFB_SAMPLE_PRINT("tmmio_len = %d\n", finfo->mmio_len);
    HIFB_SAMPLE_PRINT("accel = %d\n", finfo->accel);
}

#define BLUE   0xff0000ff
#define RED    0xffff0000
#define GREEN  0xff00ff00
#define WHITE  0xffffffff
#define YELLOW 0xffffff00

hi_void print_help()
{
    HIFB_SAMPLE_PRINT("u can execute this sample like this:\n");
    HIFB_SAMPLE_PRINT("\r\r ./sample_fb /dev/fb0 32\n");
    HIFB_SAMPLE_PRINT("/dev/fb0 --> framebuffer device\n");
    HIFB_SAMPLE_PRINT("32       --> pixel format ARGB8888\n");
    HIFB_SAMPLE_PRINT("for more information, please refer to the document readme.txt\n");
}

static hi_u32 argb8888_to_argb1555(hi_u32 color)
{
    /* 0x80000000 8888 highest bit, 16: 1555 highest bit */
    return (((color & 0x80000000) >> 16) |
    ((color & 0xf80000) >> 9) | /* 0xf80000 8888 5bits r to 9: 1555 r bit */
    ((color & 0xf800) >> 6) | /* 0xf80000 8888 5bits g to 6: b bit */
    ((color & 0xf8) >> 3)); /* 0xf80000 8888 5bits b to 3: 1555 b bit */
}

static hi_void draw_box(hifb_rect *rect, hi_u32 stride, hi_char *mem_start, hi_u32 color, hi_u32 bpp)
{
    hi_char *current_mem = HI_NULL;
    hi_u32 column, row;
    hi_u32 convert_color = color;

    if (bpp != 4 && bpp != 2) { /* 4 is argb8888 2 is argb1555 */
        HIFB_SAMPLE_PRINT("DrawBox just support pixelformat ARGB1555&ARGB8888");
        return;
    }

    if (bpp == 2) { /* 2 is argb1555 */
        convert_color = argb8888_to_argb1555(color);
    }

    for (column = rect->y; column < (rect->y + rect->h); column++) {
        current_mem = mem_start + column * stride;
        for (row = rect->x; row < (rect->x + rect->w); row++) {
            if (bpp == 2) { /* 2 is argb1555 */
                *(hi_u16*)(current_mem + row * bpp) = (hi_u16)convert_color;
            } else {
                *(hi_u32*)(current_mem + row * bpp) = convert_color;
            }
        }
    }
}

/* ARGB1555 */
static struct fb_bitfield g_r16 = { 10, 5, 0}; /* 10, 5: ARGB1555 r part */
static struct fb_bitfield g_g16 = { 5, 5, 0}; /* 5, 5: ARGB1555 g part */
static struct fb_bitfield g_b16 = { 0, 5, 0}; /* 0, 5: ARGB1555 b part */
static struct fb_bitfield g_a16 = { 15, 1, 0}; /* 15, 1: ARGB1555 a part */

static hi_void set_new_color_fmt(int fd, char *cmd)
{
    struct fb_var_screeninfo vinfo;

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        HIFB_SAMPLE_PRINT("fail to get vscreeninfo!\n");
        return;
    }

    if (strncmp("16", cmd, strlen("16")) == 0) { /* 16 is argb1555 */
        vinfo.bits_per_pixel = 16;
        vinfo.blue = g_b16;
        vinfo.red = g_r16;
        vinfo.transp = g_a16;
        vinfo.green = g_g16;

        if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) < 0) {
            HIFB_SAMPLE_PRINT("fail to put vscreeninfo!\n");
            return;
        }
    } else if (strncmp("32", cmd, strlen("32")) != 0) {
        print_help();
    }
}

static hi_void display_init(const hi_char *file)
{
    hi_unf_disp disp_id;

    if (strncmp(file, "/dev/fb3", strlen("/dev/fb3")) == 0) {
        disp_id = HI_UNF_DISPLAY1;
    } else {
        disp_id = HI_UNF_DISPLAY0;
    }

    hi_unf_sys_init();
    hi_adp_disp_init(HI_UNF_VIDEO_FMT_1080P_60);
    hi_unf_disp_set_virtual_screen(disp_id, HIFB_WIDTH, HIFB_HEIGHT);
}

static hi_void display_deinit()
{
    hi_adp_disp_deinit();
    hi_unf_sys_deinit();
}

static hi_void draw_ui(hi_char *mapped_mem, hi_u32 buf_size, hi_u32 line_length, hi_u32 bpp)
{
    hifb_rect rect;
    memset(mapped_mem, 0x0, buf_size);

    rect.x = 0;
    rect.y = 0;
    rect.w = 100; /* box width 100 */
    rect.h = 100; /* box height 100 */

    draw_box(&rect, line_length, mapped_mem, BLUE, bpp);
    rect.x = HIFB_WIDTH - 100 - 1; /* box x pos width - 100 */
    rect.y = 0;
    draw_box(&rect, line_length, mapped_mem, RED, bpp);
    rect.x = 0;
    rect.y = HIFB_HEIGHT - 100 - 1; /* box y pos width - 100 */
    draw_box(&rect, line_length, mapped_mem, GREEN, bpp);
    rect.x = HIFB_WIDTH - 100 - 1; /* box x pos width - 100 */
    rect.y = HIFB_HEIGHT - 100 - 1; /* box y pos width - 100 */
    draw_box(&rect, line_length, mapped_mem, YELLOW, bpp);
    rect.x = HIFB_WIDTH / 2 - 50 - 1;  /* box x pos width / 2 - 100 -50 */
    rect.y = HIFB_HEIGHT / 2 - 50 - 1;  /* box y pos width / 2 - 100 -50 */
    draw_box(&rect, line_length, mapped_mem, WHITE, bpp);
}

static hi_s32 get_finfo_vinfo(hi_s32 fd, struct fb_fix_screeninfo *finfo, struct fb_var_screeninfo *vinfo)
{
    if (ioctl(fd, FBIOGET_FSCREENINFO, finfo) < 0) {
        HIFB_SAMPLE_PRINT("Couldn't get console hardware info\n");
        return HI_FAILURE;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, vinfo) < 0) {
        HIFB_SAMPLE_PRINT("Couldn't get vscreeninfo\n");
        return HI_FAILURE;
    }

    print_vinfo(vinfo);
    print_finfo(finfo);
    return HI_SUCCESS;
}

static hi_char *map_framebuffer(hi_s32 fd, struct fb_fix_screeninfo *finfo)
{
    hi_char *mapped_mem = HI_NULL;

    if (finfo->smem_len != 0) {
        mapped_mem = mmap(HI_NULL, finfo->smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped_mem == MAP_FAILED) {
            HIFB_SAMPLE_PRINT("Unable to memory map the video hardware\n");
            return HI_NULL;
        }
    }
    return mapped_mem;
}

int main(int argc, char* argv[])
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    hi_s32 console_fd;
    hi_s32 ret;
    hi_u32 bpp;
    const hi_char *file = "/dev/fb0";
    hi_char *mapped_mem = HI_NULL;

    if (argc >= 2) { /* more than 2 args */
        file = argv[1];
    }

    display_init(file);

    console_fd = open(file, O_RDWR, 0);
    if (console_fd < 0) {
        print_help();
        return -1;
    }

    if (argc >= 3) { /* 3 args */
        set_new_color_fmt(console_fd, argv[2]); /* arg 2 is new color fmt */
    } else {
        if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &g_hifb_def_vinfo) < 0) {
            HIFB_SAMPLE_PRINT("Unable to set variable screeninfo!\n");
            goto close_fd;
        }
    }

    ret = get_finfo_vinfo(console_fd, &finfo, &vinfo);
    if (ret != HI_SUCCESS) {
        ret = -1;
        goto close_fd;
    }

    bpp = vinfo.bits_per_pixel / 8; /* 8 bits */

    mapped_mem = map_framebuffer(console_fd, &finfo);
    if (mapped_mem == HI_NULL) {
        goto close_fd;
    }

    draw_ui(mapped_mem, finfo.smem_len, finfo.line_length, bpp);

    if (ioctl(console_fd, FBIOPAN_DISPLAY, &vinfo) < 0) {
        HIFB_SAMPLE_PRINT("pan_display failed!\n");
    }

    (hi_void)getchar();

    if (mapped_mem != HI_NULL) {
        munmap(mapped_mem, finfo.smem_len);
    }

close_fd:
    close(console_fd);
    display_deinit();
    return 0;
}
