/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample for mono display
 * Author: sdk
 * Create: 2019-07-21
 */

#include "hi_svr_dispmng.h"
#include "hi_unf_video.h"
#include "hi_type.h"
#include "securec.h"
#include "hifb.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define DISPLAY_MODE_AUTO

#define  MAX_FB_DEVICE_NAME_LEN     32

// HDMI output mode 1920x1080p50
#define  HI_DISPLAY_MODE_VIC        31

// Virtual Screen(Frame Buffer) Size
#define  HIFB_WIDTH    1280
#define  HIFB_HEIGHT   720
#define  BITS_PER_PIXEL  4

/* Framebuffer size,  1280x720x4 bytes */
#define  HIFB_FRAME_SIZE  (HIFB_WIDTH*HIFB_HEIGHT*BITS_PER_PIXEL)
#define  BITS_PER_BYTE   8

#ifndef PRINT
#define PRINT    printf
#endif

typedef struct {
    hi_s32 fb_dev;           // fb device handle
    hi_void* fb_mem;           // memory of frame buffer
    hi_slong fb_mem_len;      // memory size of frame buffer
    hi_s32 fb_line_len;
    hi_s32 screen_width;
    hi_s32 screen_height;
    hi_s32 bits_per_pixel;    // for example, ARGB8888 is 4 bytes
} frame_buffer_ctx;
frame_buffer_ctx g_fb_info[HI_SVR_DISPMNG_DISPLAY_ID_MAX] = {{0}, {0}};

static struct fb_var_screeninfo g_fb_screen_info = {
    HIFB_WIDTH,    // visible resolution xres
    HIFB_HEIGHT,   // yres
    HIFB_WIDTH,    // virtual resolution xres_virtual
    HIFB_HEIGHT,   // yres_virtual
    0,   // xoffset
    0,   // yoffset
    32,  // bits per pixel
    0,   // grey levels, 1 means black/white
    {16, 8, 0}, // fb_bitfiled red
    { 8, 8, 0}, // green
    { 0, 8, 0}, // blue
    {24, 8, 0}, // transparency
    0,  // non standard pixel format
    FB_ACTIVATE_FORCE,
    0, // height of picture in mm
    0, // width of picture in mm
    0, // acceleration flags
    -1, // pixclock
    -1, // left margin
    -1, // right margin
    -1, // upper margin
    -1, // lower margin
    -1, // hsync length
    -1, // vsync length
        // sync: FB_SYNC
        // vmod: FB_VMOD
        // reserved[6]: reserved for future use
};

static hi_void print_display_info(hi_svr_dispmng_display_id id);
static hi_void print_display_capability(hi_svr_dispmng_display_id id);
static hi_void print_available_mode(hi_svr_dispmng_display_id id);
hi_void draw_picture(hi_svr_dispmng_display_id id);
hi_s32  mmap_frame_buffer(const hi_svr_dispmng_display_id id);
hi_void unmap_frame_buffer(hi_svr_dispmng_display_id id);
hi_s32  read_picture_from_file(char *file_name, hi_u8 *buffer, hi_u32 size);

hi_s32 main()
{

    hi_s32 disp_cnt;
    hi_svr_dispmng_display_id id;
    hi_s32 ret;

    ret = hi_svr_dispmng_init();
    if (ret != HI_SUCCESS) {
        PRINT("Display Manager initial fail!\n");
        return HI_FAILURE;
    }

    // Get display count
    ret = hi_svr_dispmng_get_display_count(&disp_cnt);
    if (ret != HI_SUCCESS) {
        PRINT("Get display count fail!\n");
        return HI_FAILURE;
    }

    // Get the master display id
    ret = hi_svr_dispmng_get_display_id(HI_SVR_DISPMNG_DISPLAY_INDEX_0, &id);
    if (ret != HI_SUCCESS) {
        PRINT("Can not get the display id.\n");
        return HI_FAILURE;
    }

    // Set Virtual Screen size to 1280x720
    PRINT("Set virtual screen size of display %d to 1280x720.\n", id);
    ret = hi_svr_dispmng_set_virtual_screen(id, HIFB_WIDTH, HIFB_HEIGHT);
    if (ret != HI_SUCCESS) {
        PRINT("Can not set the virtual screen size of display %d.\n", id);
        return HI_FAILURE;
    }

    print_display_info(id);
    print_display_capability(id);
    print_available_mode(id);

    /* Set display mode to 1080p50 or Automatic */
    hi_svr_dispmng_display_mode mode = {0};
#if defined(DISPLAY_MODE_AUTO)
    /* Automatic setting display mode, normally it will use the native mode of the connected TV */
    mode.vic          = HI_SVR_DISPMNG_DISPLAY_MODE_AUTO;
    mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
    mode.bit_depth  = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
#elif defined(DISPLAY_MODE_NORMAL)
    /* Normal setting display mode, use triad (vic/pixel_format/bit_depth) to specify display output mode */
    mode.vic          = HI_DISPLAY_MODE_VIC;
    mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_YUV444;
    mode.bit_depth  = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_8BIT;
#elif defined(DISPLAY_MODE_FORCE)
    /* Force setting display mode, the vic must be HI_SVR_DISPMNG_DISPLAY_MODE_ADVANCED
       And must provide format(mode.format) to specify display format */
    mode.vic          = HI_SVR_DISPMNG_DISPLAY_MODE_ADVANCED;
    mode.format       = HI_UNF_VIDEO_FMT_1080P_50;
    mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_YUV444;
    mode.bit_depth  = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_8BIT;
#endif
    PRINT("Set display mode to vic:%d, pixel format:%d, pixel bitwidth:%d.\n", mode.vic, mode.pixel_format,
        mode.bit_depth);
    ret = hi_svr_dispmng_set_display_mode(id, &mode);
    if (ret != HI_SUCCESS) {
        PRINT("Can not set mode to display %d(ret=0x%X).\n", id, ret);
    }

    // Open and map frame buffer
    ret = mmap_frame_buffer(id);
    if (ret != HI_SUCCESS) {
        PRINT("Can not open and nmap display[%d] framebuffer\n", id);
        return HI_FAILURE;
    }

    // Draw a picture to framebuffer
    draw_picture(id);

    getchar();

    // Unmap the frame buffer
    unmap_frame_buffer(id);

    // DeInit the display manager
    ret = hi_svr_dispmng_deinit();
    if (ret != HI_SUCCESS) {
        PRINT("Display Manager deinit fail!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 mmap_frame_buffer(const hi_svr_dispmng_display_id id)
{

    hi_s32 console_fd = 0;
    hi_s32 ret;
    hi_u8 fb_dev_name[MAX_FB_DEVICE_NAME_LEN];
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    hi_u32 u32DisPlayBufSize;
    hifb_alpha alpha;

#ifdef CFG_HIFB_ANDRIOD
    HI_BOOL bDecmp = HI_FALSE;
#endif

    // Get frame buffer device name
    ret = hi_svr_dispmng_get_device_name(id, fb_dev_name, MAX_FB_DEVICE_NAME_LEN);
    if (ret != HI_SUCCESS) {
        PRINT("Can not get the framebuffer of display %d.\n", id);
        return HI_FAILURE;
    }

    /* Open frame buffer */
    PRINT("Open Framebuffer device(%s).\n", fb_dev_name);
    console_fd = open((const char *)fb_dev_name, O_RDWR, 0);
    if (console_fd < 0) {
        PRINT("Unable to open %s\n", fb_dev_name);
        return HI_FAILURE;
    }

    /* set color format ARGB8888, screen size: 1280*720 */
    if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &g_fb_screen_info) < 0) {
        PRINT("Unable to set variable screeninfo!\n");
        ret = HI_FAILURE;
        goto init_fail;
    }

    /* Get the fix screen info of hardware */
    if (ioctl(console_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        PRINT("Couldn't get console hardware info\n");
        ret = HI_FAILURE;
        goto init_fail;
    }

    g_fb_info[id].fb_mem_len = finfo.smem_len;
    u32DisPlayBufSize = g_fb_screen_info.xres_virtual * g_fb_screen_info.yres_virtual *
        (g_fb_screen_info.bits_per_pixel / BITS_PER_BYTE);
    if (g_fb_info[id].fb_mem_len != 0 && g_fb_info[id].fb_mem_len >= u32DisPlayBufSize) {
        g_fb_info[id].fb_mem = mmap(NULL, g_fb_info[id].fb_mem_len,
            PROT_READ | PROT_WRITE, MAP_SHARED, console_fd, 0);
        if (g_fb_info[id].fb_mem == (char *)-1) {
            PRINT("Unable to memory map the video hardware\n");
            g_fb_info[id].fb_mem = NULL;
            ret = HI_FAILURE;
            goto init_fail;
        }
    }

    /* Determine the current screen depth */
    if (ioctl(console_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        PRINT("Couldn't get vscreeninfo\n");
        ret = HI_FAILURE;
        goto init_fail;
    }

    /* set alpha */
    alpha.pixel_alpha_en  = HI_TRUE;
    alpha.alpha0 = 0xff;
    alpha.alpha1 = 0xff;

    /* set global alpha */
    alpha.global_alpha_en = HI_TRUE;
    alpha.global_alpha = 0xff;

    if (ioctl(console_fd, HIFBIOPUT_ALPHA, &alpha) < 0) {
        PRINT("[ERROR]Couldn't set alpha\n");
        ret = HI_FAILURE;
        goto init_fail;
    }

#ifdef CFG_HIFB_ANDRIOD
    ioctl(console_fd, FBIOPUT_DECOMPRESS_HIFB, &bDecmp);
#endif

    memset_s(g_fb_info[id].fb_mem, g_fb_info[id].fb_mem_len, 0x0, g_fb_info[id].fb_mem_len);

    g_fb_info[id].fb_dev = console_fd;
    g_fb_info[id].screen_width  = g_fb_screen_info.xres;
    g_fb_info[id].screen_height = g_fb_screen_info.yres;
    g_fb_info[id].bits_per_pixel = g_fb_screen_info.bits_per_pixel;
    g_fb_info[id].fb_line_len = finfo.line_length;

    return HI_SUCCESS;

init_fail:
    close(console_fd);
    return ret;
}

hi_void draw_picture(hi_svr_dispmng_display_id id)
{
    char *file_name[] = {"picture01.dat", "picture02.dat"};
    hi_u8  picture_buffer[HIFB_FRAME_SIZE];
    struct fb_var_screeninfo vinfo;
    hifb_flush_info flush_info;
    hi_s32 ret;

    // Read a picture from file
    ret = read_picture_from_file(file_name[id], picture_buffer, HIFB_FRAME_SIZE);
    if (ret != HIFB_FRAME_SIZE) {
        PRINT("Reading file [%s] error(len = %d)!\n", file_name[id], ret);
        return;
    }

    /* Determine the current screen depth */
    if (ioctl(g_fb_info[id].fb_dev, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        PRINT("Couldn't get vscreeninfo\n");
        return;
    }

    // draw the picture data to frame buffer
    PRINT("Rendering picture to framebuffer...\n");
    memcpy_s((hi_u8*)g_fb_info[id].fb_mem, g_fb_info[id].fb_mem_len, picture_buffer, HIFB_FRAME_SIZE);

    // refresh framebuffer to display
    if (g_fb_info[id].fb_mem_len == 0 || g_fb_info[id].fb_mem_len < HIFB_FRAME_SIZE) {
        flush_info.updata_rect.x = 0;
        flush_info.updata_rect.y = 0;
        flush_info.updata_rect.w = HIFB_WIDTH;
        flush_info.updata_rect.h = HIFB_HEIGHT;
        if (ioctl(g_fb_info[id].fb_dev, HIFBIOPUT_REFRESH, &flush_info) < 0) {
            PRINT("[ERROR]refresh buffer info failed!\n");
        }
    } else {
        if (ioctl(g_fb_info[id].fb_dev, FBIOPAN_DISPLAY, &vinfo) < 0) {
            PRINT("[ERROR]pan_display failed!\n");
        }
    }

}

hi_void unmap_frame_buffer(hi_svr_dispmng_display_id id)
{
    if (g_fb_info[id].fb_mem != HI_NULL) {
        munmap(g_fb_info[id].fb_mem, g_fb_info[id].fb_mem_len);
        g_fb_info[id].fb_mem = HI_NULL;
    }

    if (g_fb_info[id].fb_dev != -1) {
        close(g_fb_info[id].fb_dev);
        g_fb_info[id].fb_dev = -1;
    }
}

static hi_void print_display_info(hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_sink_info info;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_sink_info(id, &info);
    if (ret != HI_SUCCESS) {
        PRINT("Can not get the information from display %d\n", id);
        return;
    }

    PRINT("\n====================================== Display Info ========================================\n");
    PRINT("Manufacturor: %s\n", (char *)info.mfrs_name);
    PRINT("Product Code: %d\n", info.product_code);
    PRINT("Serial Number: %d\n", info.serial_number);
    PRINT("Production Year:%d\n", info.year);
    PRINT("Production Week:%d\n", info.week);
}

static hi_void print_display_capability(hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_capability capa;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_display_capabilities(id, &capa);
    if (ret != HI_SUCCESS) {
        PRINT("Can not get capabilities information from display %d\n", id);
        return;
    }

    PRINT("==================================== Display Capabilities ====================================\n");
    PRINT("Colorimetry:\n");
    PRINT("    XVYCC601: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_XVYCC601]);
    PRINT("    XVYCC709: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_XVYCC709]);
    PRINT("    SYCC601: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_SYCC601]);
    PRINT("    ADOBE_YCC601: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_ADOBE_YCC601]);
    PRINT("    ADOBE_RGB: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_ADOBE_RGB]);
    PRINT("    BT2020_CYCC: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_BT2020_CYCC]);
    PRINT("    BT2020_YCC: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_BT2020_YCC]);
    PRINT("    BT2020_RGB: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_BT2020_RGB]);
    PRINT("    DCI_P3: %d\n",
        capa.colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_DCI_P3]);
    PRINT("dolby_vision:\n");
    PRINT("    Dolby Vision V0 support: %d\n", capa.dolby_vision.support_v0);
    PRINT("    Dolby Vision V1 support: %d\n", capa.dolby_vision.support_v1);
    PRINT("    Dolby Vision V2 support: %d\n", capa.dolby_vision.support_v2);
    PRINT("HDR:\n");
    PRINT("    OETF:\n");
    PRINT("        Tranditional SDR:%d\n", capa.hdr.eotf.eotf_sdr);
    PRINT("        Tranditional HDR:%d\n", capa.hdr.eotf.eotf_hdr);
    PRINT("        SMPTE ST 2084:%d\n", capa.hdr.eotf.eotf_smpte_st2084);
    PRINT("        Hybrid Log-Gamma(HLG):%d\n", capa.hdr.eotf.eotf_hlg);
    PRINT("    Static Metadata:\n");
    PRINT("        Static Metadata Type 1 support:%u\n", capa.hdr.static_metadata.static_type_1);
    PRINT("        Max Luminance:%u\n", capa.hdr.static_metadata.max_lum_cv);
    PRINT("        Max Frame-average Luminance:%u\n", capa.hdr.static_metadata.aver_lum_cv);
    PRINT("        Min Luminance Data:%u\n", capa.hdr.static_metadata.min_lum_cv);
    PRINT("    Dynamic Metadata:\n");
    PRINT("        type1 dynamic metadata support:%d\n", capa.hdr.dynamic_metadata.dynamic_type_1_support);
    PRINT("        type2 dynamic metadata support:%d\n", capa.hdr.dynamic_metadata.dynamic_type_2_support);
    PRINT("        type3 dynamic metadata support:%d\n", capa.hdr.dynamic_metadata.dynamic_type_3_support);
    PRINT("        type4 dynamic metadata support:%d\n", capa.hdr.dynamic_metadata.dynamic_type_4_support);
    PRINT("Latency:\n");
    PRINT("    Latency Present:%d\n", capa.latency.latency_present);
    PRINT("    video latency for progressive video:%u\n", capa.latency.pvideo_latency);
    PRINT("    audio latency for progressive video:%u\n", capa.latency.paudio_latency);
    PRINT("    video latency for interlaced video:%u\n", capa.latency.ivideo_latency);
    PRINT("    audio latency for interlaced video:%u\n", capa.latency.iaudio_latency);
}

static hi_void print_display_mode(hi_svr_dispmng_display_mode *mode)
{
    PRINT("vic:%5d, format:%3d,  name:%-25s, width:%4d, height:%4d, field rate:%d\n",
        mode->vic, mode->format, mode->name, mode->width, mode->height, mode->field_rate);
}

static hi_void print_available_mode(hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_available_mode modes;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_supported_mode_list(id, &modes);
    if (ret != HI_SUCCESS) {
        PRINT("Can not get the supported display mode list from display %d[ret=0x%X].\n", id, ret);
        return;
    }

    PRINT("=======================================  display modes  ======================================\n");
    PRINT("Number of display modes:%d\n", modes.number);

    PRINT("========================================  Native Mode  =======================================\n");
    print_display_mode(&modes.native_mode);
    PRINT("==========================================  Max Mode  ========================================\n");
    print_display_mode(&modes.max_mode);
    PRINT("====================================  Available Mode  List ===================================\n");
    for (hi_u32 i = 0; i < modes.number; i++) {
        print_display_mode(&modes.mode[i]);
    }
}

hi_u32 file_size(char* file_name)
{
    struct stat statbuf;
    stat(file_name, &statbuf);
    return(statbuf.st_size);
}

#define READ_BLOCK_SIZE   32768
hi_s32 read_picture_from_file(char *file_name, hi_u8 *buffer, hi_u32 size)
{
    hi_u8 *pos = buffer;
    hi_s32 read_size;
    hi_s32 len = 0;
    FILE *fp = NULL;

    fp = fopen(file_name, "rb+");
    if (fp == NULL) {
        PRINT("File open error[%s]!\n", file_name);
        return HI_FAILURE;
    }

    if (file_size(file_name) > size) {
        PRINT("Buffer size is too small!\n");
        return HI_FAILURE;
    }

    while (!feof(fp)) {
        read_size = fread(pos, 1, READ_BLOCK_SIZE, fp);
        len  += read_size;
        pos += read_size;
    }

    fclose(fp);
    return len;
}