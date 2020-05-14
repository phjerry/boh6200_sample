/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: sample for display mode
 * Author: sdk
 * Create: 2019-07-21
 */

#include "hi_svr_dispmng.h"
#include "hi_unf_video.h"
#include "hi_type.h"
#include "securec.h"
#include "hifb.h"
#include "hi_adp_mpi.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define  MAX_FB_DEVICE_NAME_LEN     32

/* Virtual Screen(Frame Buffer) Size */
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

hi_void print_usage(hi_s32 argc, hi_char* argv[]);
hi_void set_display_mode();
hi_void display_mode(const hi_svr_dispmng_display_id id);
hi_void draw_picture(const hi_svr_dispmng_display_id id);
hi_s32  mmap_frame_buffer(const hi_svr_dispmng_display_id id);
hi_void unmap_frame_buffer(const hi_svr_dispmng_display_id id);
hi_s32  read_picture_from_file(const char* file_name, hi_u8* buffer, hi_u32 size);
static hi_void process_input_cmd();

hi_svr_dispmng_display_mode g_disp_mode = {0};
hi_svr_dispmng_display_id   g_disp_id = HI_SVR_DISPMNG_DISPLAY_ID_MASTER;


hi_s32 main(hi_s32 argc, hi_char* argv[])
{

    hi_s32 ret;

    if (argc < 2) {
        print_usage(argc, argv);
        return HI_SUCCESS;
    }

    if (argc >= 2) {
        if (argv[1][0] == '0') {
            g_disp_id = HI_SVR_DISPMNG_DISPLAY_ID_MASTER;
        } else {
            g_disp_id = HI_SVR_DISPMNG_DISPLAY_ID_SLAVE;
        }
    }

    PRINT("[INFO]Display Manager init.\n");
    ret = hi_svr_dispmng_init();
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Display Manager initial fail(ret=0x%x)!\n", ret);
        return HI_FAILURE;
    }

    hi_s32 disp_num;
    ret = hi_svr_dispmng_get_display_count(&disp_num);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_get_display_count fail(ret = 0x%X).\n", ret);
        return HI_FAILURE;
    }
    PRINT("[INFO]Display count:%d\n", disp_num);

    /* Set Virtual Screen size to 1280x720 */
    PRINT("[INFO]Set virtual screen size of display %d to 1280x720.\n", g_disp_id);
    ret = hi_svr_dispmng_set_virtual_screen(g_disp_id, HIFB_WIDTH, HIFB_HEIGHT);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not set the virtual screen size of display %d(ret = 0x%X).\n", g_disp_id, ret);
        return HI_FAILURE;
    }

    /* Get and print the current display mode */
    display_mode(g_disp_id);

    /* Open and map frame buffer */
    ret = mmap_frame_buffer(g_disp_id);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not open and nmap display[%d] framebuffer\n", g_disp_id);
        return HI_FAILURE;
    }

    /* Draw a picture to framebuffer */
    draw_picture(g_disp_id);

    /* Process command line */
    process_input_cmd();

    /* Unmap the frame buffer */
    unmap_frame_buffer(g_disp_id);

    /* DeInit the display manager */
    ret = hi_svr_dispmng_deinit();
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Display Manager deinit fail(ret = 0x%X)!\n", ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_void print_usage(hi_s32 argc, hi_char* argv[])
{
    PRINT("Usage: %s [display]\n", argv[0]);
    PRINT("\tdisplay: 0 -- MASTER display | 1 -- SLAVE display\n");
    PRINT("Example:\n\t%s 0\n", argv[0]);
}

hi_void set_display_mode()
{
    hi_s32 ret;
    ret = hi_svr_dispmng_get_vic_from_format(g_disp_mode.format, &g_disp_mode.vic);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_get_vic_from_format fail(ret=0x%X).\n", ret);
    }
    ret = hi_svr_dispmng_set_display_mode(g_disp_id, &g_disp_mode);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_display_mode fail(ret=0x%X).\n", ret);
    }
}

hi_s32 mmap_frame_buffer(const hi_svr_dispmng_display_id id)
{
    hi_s32 console_fd = 0;
    hi_s32 ret;
    hi_u8 fb_dev_name[MAX_FB_DEVICE_NAME_LEN];
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    hi_u32 display_buff_size;
    hifb_alpha alpha;

#ifdef CFG_HIFB_ANDRIOD
    hi_bool decompress = HI_FALSE;
#endif

    // Get frame buffer device name
    ret = hi_svr_dispmng_get_device_name(id, fb_dev_name, MAX_FB_DEVICE_NAME_LEN);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not get the framebuffer of display %d.\n", id);
        return HI_FAILURE;
    }

    /* Open frame buffer */
    PRINT("[INFO]Open Framebuffer device(%s).\n", fb_dev_name);
    console_fd = open((const char *)fb_dev_name, O_RDWR, 0);
    if (console_fd < 0) {
        PRINT("[ERROR]Unable to open %s\n", fb_dev_name);
        return HI_FAILURE;
    }

    /* set color format ARGB8888, screen size: 1280*720 */
    if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &g_fb_screen_info) < 0) {
        PRINT("[ERROR]Unable to set variable screeninfo!\n");
        ret = HI_FAILURE;
        goto INIT_FAIL;
    }

    /* Get the fix screen info of hardware */
    if (ioctl(console_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        PRINT("[ERROR]Couldn't get console hardware info\n");
        ret = HI_FAILURE;
        goto INIT_FAIL;
    }

    g_fb_info[id].fb_mem_len = finfo.smem_len;
    display_buff_size = g_fb_screen_info.xres_virtual * g_fb_screen_info.yres_virtual *
        (g_fb_screen_info.bits_per_pixel / BITS_PER_BYTE);
    if (g_fb_info[id].fb_mem_len != 0 && g_fb_info[id].fb_mem_len >= display_buff_size) {
        g_fb_info[id].fb_mem = mmap(NULL, g_fb_info[id].fb_mem_len,
            PROT_READ | PROT_WRITE, MAP_SHARED, console_fd, 0);
        if (g_fb_info[id].fb_mem == (char *)-1) {
            PRINT("[ERROR]Unable to memory map the video hardware\n");
            g_fb_info[id].fb_mem = NULL;
            ret = HI_FAILURE;
            goto INIT_FAIL;
        }
    }

    /* Determine the current screen depth */
    if (ioctl(console_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        PRINT("[ERROR]Couldn't get vscreeninfo\n");
        ret = HI_FAILURE;
        goto INIT_FAIL;
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
        goto INIT_FAIL;
    }

#ifdef CFG_HIFB_ANDRIOD
    ioctl(console_fd, HIFBIOPUT_DECOMPRESS, &decompress);
#endif

    memset_s(g_fb_info[id].fb_mem, g_fb_info[id].fb_mem_len, 0x0, g_fb_info[id].fb_mem_len);
    g_fb_info[id].fb_dev = console_fd;
    g_fb_info[id].screen_width  = g_fb_screen_info.xres;
    g_fb_info[id].screen_height = g_fb_screen_info.yres;
    g_fb_info[id].bits_per_pixel = g_fb_screen_info.bits_per_pixel;
    g_fb_info[id].fb_line_len = finfo.line_length;

    return HI_SUCCESS;

INIT_FAIL:
    close(console_fd);
    return ret;
}

hi_void draw_picture(const hi_svr_dispmng_display_id id)
{
    char *file_name[] = {"picture01.dat", "picture02.dat"};
    hi_u8  picture_buffer[HIFB_FRAME_SIZE];
    struct fb_var_screeninfo vinfo;
    hifb_flush_info flush_info;
    hi_s32 ret;

    // Read a picture from file
    ret = read_picture_from_file(file_name[id], picture_buffer, HIFB_FRAME_SIZE);
    if (ret != HIFB_FRAME_SIZE) {
        PRINT("[ERROR]Reading file [%s] error(len = %d)!\n", file_name[id], ret);
        return;
    }

    /* Determine the current screen depth */
    if (ioctl(g_fb_info[id].fb_dev, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        PRINT("[ERROR]Couldn't get vscreeninfo\n");
        return;
    }

    // draw the picture data to frame buffer
    PRINT("[INFO]Rendering picture to framebuffer...\n");
    ret = memcpy_s((hi_u8*)g_fb_info[id].fb_mem, g_fb_info[id].fb_mem_len, picture_buffer, HIFB_FRAME_SIZE);
    if (ret != EOK) {
        PRINT("[ERROR]memcpy_s fail(ret=%d).\n", ret);
        return;
    }

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

hi_void unmap_frame_buffer(const hi_svr_dispmng_display_id id)
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

hi_u32 file_size(const char* file_name)
{
    struct stat statbuf;
    stat(file_name, &statbuf);
    return(statbuf.st_size);
}

#define READ_BLOCK_SIZE   32768
hi_s32 read_picture_from_file(const char* file_name, hi_u8* buffer, hi_u32 size)
{
    hi_u8 *pos = buffer;
    hi_s32 read_size;
    hi_s32 len = 0;
    FILE *fp = NULL;

    fp = fopen(file_name, "rb+");
    if (fp == NULL) {
        PRINT("[ERROR]File open error[%s]!\n", file_name);
        return HI_FAILURE;
    }

    if (file_size(file_name) > size) {
        PRINT("[ERROR]Buffer size is too small!\n");
        goto close_file;
    }

    while (!feof(fp)) {
        read_size = fread(pos, 1, READ_BLOCK_SIZE, fp);
        len  += read_size;
        pos += read_size;
    }

close_file:
    fclose(fp);
    return len;
}

static hi_void print_display_mode(const hi_svr_dispmng_display_mode* mode)
{
    PRINT("vic:%5d, format:%3d,  name:%-24s, width:%4d, height:%4d, field rate:%3d,"\
        " pixel_format:%d, bit_depth:%d\n",
        mode->vic, mode->format, mode->name, mode->width, mode->height, mode->field_rate,
        mode->pixel_format, mode->bit_depth);
}

static hi_void print_available_mode(const hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_available_mode modes;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_supported_mode_list(id, &modes);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not get the supported display modes from display %d[ret=%d].\n", id, ret);
        return;
    }

    PRINT("==================================  display modes  =================================\n");
    PRINT("Number of display modes:%d\n", modes.number);

    PRINT("===================================  Native Mode  ==================================\n");
    print_display_mode(&modes.native_mode);
    PRINT("=====================================  Max Mode  ===================================\n");
    print_display_mode(&modes.max_mode);
    PRINT("===============================  Available Mode  List ==============================\n");
    for (hi_u32 i = 0; i < modes.number; i++) {
        print_display_mode(&modes.mode[i]);
    }
}

static hi_void print_all_display_mode(const hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_available_mode modes;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_all_mode_list(id, &modes);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not get the unsupported display modes from display %d[ret=%d].\n", id, ret);
        return;
    }

    PRINT("===============================  All Display Mode List ==============================\n");
    PRINT("Number of display modes:%d\n", modes.number);
    for (hi_u32 i = 0; i < modes.number; i++) {
        print_display_mode(&modes.mode[i]);
    }
}

static hi_void print_display_info(const hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_sink_info info;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_sink_info(id, &info);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not get the information from display %d\n", id);
        return;
    }

    PRINT("================================== Display Info ====================================\n");
    PRINT("Manufacturor: %s\n", (char *)info.mfrs_name);
    PRINT("Product Code: %d\n", info.product_code);
    PRINT("Serial Number: %d\n", info.serial_number);
    PRINT("Production Year:%d\n", info.year);
    PRINT("Production Week:%d\n", info.week);
}

hi_void print_display_colorimetry(const hi_svr_dispmng_display_capability *capa)
{
    PRINT("Colorimetry:\n");
    PRINT("\tXVYCC601: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_XVYCC601]);
    PRINT("\tXVYCC709: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_XVYCC709]);
    PRINT("\tSYCC601: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_SYCC601]);
    PRINT("\tADOBE_YCC601: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_ADOBE_YCC601]);
    PRINT("\tADOBE_RGB: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_ADOBE_RGB]);
    PRINT("\tBT2020_CYCC: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_BT2020_CYCC]);
    PRINT("\tBT2020_YCC: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_BT2020_YCC]);
    PRINT("\tBT2020_RGB: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_BT2020_RGB]);
    PRINT("\tDCI_P3: %d\n",
        capa->colorimetry.colorimetry[HI_SVR_DISPMNG_COLORIMETRY_DCI_P3]);
}

hi_void print_display_dv_capa(const hi_svr_dispmng_display_capability *capa)
{
    PRINT("dolby_vision:\n");
    PRINT("\tDolby Vision V0 support: %d\n", capa->dolby_vision.support_v0);
    PRINT("\tDolby Vision V1 support: %d\n", capa->dolby_vision.support_v1);
    PRINT("\tDolby Vision V2 support: %d\n", capa->dolby_vision.support_v2);
}

hi_void print_display_hdr_capa(const hi_svr_dispmng_display_capability *capa)
{
    PRINT("HDR:\n");
    PRINT("\tOETF:\n");
    PRINT("\t\tTranditional SDR:%d\n", capa->hdr.eotf.eotf_sdr);
    PRINT("\t\tTranditional HDR:%d\n", capa->hdr.eotf.eotf_hdr);
    PRINT("\t\tSMPTE ST 2084:%d\n", capa->hdr.eotf.eotf_smpte_st2084);
    PRINT("\t\tHybrid Log-Gamma(HLG):%d\n", capa->hdr.eotf.eotf_hlg);
    PRINT("\tStatic Metadata:\n");
    PRINT("\t\tStatic Metadata Type 1 support:%u\n", capa->hdr.static_metadata.static_type_1);
    PRINT("\t\tMax Luminance:%u\n", capa->hdr.static_metadata.max_lum_cv);
    PRINT("\t\tMax Frame-average Luminance:%u\n", capa->hdr.static_metadata.aver_lum_cv);
    PRINT("\t\tMin Luminance Data:%u\n", capa->hdr.static_metadata.min_lum_cv);
    PRINT("\tDynamic Metadata:\n");
    PRINT("\t\ttype1 dynamic metadata support:%d\n", capa->hdr.dynamic_metadata.dynamic_type_1_support);
    PRINT("\t\ttype2 dynamic metadata support:%d\n", capa->hdr.dynamic_metadata.dynamic_type_2_support);
    PRINT("\t\ttype3 dynamic metadata support:%d\n", capa->hdr.dynamic_metadata.dynamic_type_3_support);
    PRINT("\t\ttype4 dynamic metadata support:%d\n", capa->hdr.dynamic_metadata.dynamic_type_4_support);
}

hi_void print_display_latency_capa(const hi_svr_dispmng_display_capability *capa)
{
    PRINT("Latency:\n");
    PRINT("\tLatency Present:%d\n", capa->latency.latency_present);
    PRINT("\tvideo latency for progressive video:%u\n", capa->latency.pvideo_latency);
    PRINT("\taudio latency for progressive video:%u\n", capa->latency.paudio_latency);
    PRINT("\tvideo latency for interlaced video:%u\n", capa->latency.ivideo_latency);
    PRINT("\taudio latency for interlaced video:%u\n", capa->latency.iaudio_latency);
}

static hi_void print_display_capability(const hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_capability capa;
    hi_s32 ret;

    ret = hi_svr_dispmng_get_display_capabilities(id, &capa);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]Can not get capabilities information from display %d\n", id);
        return;
    }

    PRINT("============================== Display Capabilities ==============================\n");

    print_display_colorimetry(&capa);
    print_display_dv_capa(&capa);
    print_display_hdr_capa(&capa);
    print_display_latency_capa(&capa);

}

hi_void print_display_status(const hi_svr_dispmng_display_id id)
{
    hi_svr_dispmng_display_status status;
    hi_s32 ret, i;

    ret = hi_svr_dispmng_get_display_status(id, &status);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_get_display_status fail(disp:%d, ret=0x%X).\n", id, ret);
    }
    PRINT("Attached interface number:%d\n", status.number);
    for (i = 0; i < status.number; i++) {
        if (status.intf[i].intf_type == HI_SVR_DISPMNG_INTERFACE_HDMITX) {
            PRINT("=====================HDMI status===================\n");
            PRINT("HDMI ID: %d\n", status.intf[i].intf.hdmi.hdmitx_id);
            PRINT("Connect Status: %d\n", status.intf[i].intf.hdmi.conn_status);
            PRINT("RSEN Status: %d\n", status.intf[i].intf.hdmi.rsen_status);
            PRINT("Enabled: %d\n", status.intf[i].intf.hdmi.enabled);
        } else if (status.intf[i].intf_type == HI_SVR_DISPMNG_INTERFACE_PANEL) {
            PRINT("=====================MIPI status===================\n");
            PRINT("PANEL ID: %d\n", status.intf[i].intf.panel.panel_id);
            PRINT("Power On: %d\n", status.intf[i].intf.panel.power_on);
            PRINT("Dynamic Backlight: %d\n", status.intf[i].intf.panel.dynamic_backlight_enabled);
            PRINT("Backlight Level: %d\n", status.intf[i].intf.panel.backlight_level);
        } else {
            PRINT("Unkown interface intf_type=%u\n", status.intf[i].intf_type);
        }
    }
}

hi_void display_mode(const hi_svr_dispmng_display_id id)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_get_display_mode(id, &g_disp_mode);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_get_display_mode fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }

    PRINT("Display mode:\n");
    PRINT("\tvic:%d\n", g_disp_mode.vic);
    PRINT("\tformat:%d\n", g_disp_mode.format);
    PRINT("\tpixel format:%d\n", g_disp_mode.pixel_format);
    PRINT("\tpixel width:%d\n", g_disp_mode.bit_depth);
    PRINT("\tname:%s\n", g_disp_mode.name);
    PRINT("\twidth:%d\n", g_disp_mode.width);
    PRINT("\theight:%d\n", g_disp_mode.height);
    PRINT("\tframe rate:%d\n", g_disp_mode.field_rate);
    PRINT("\tprogressive:%d\n", g_disp_mode.progressive);
}

hi_void display_enable(const hi_svr_dispmng_display_id id, const hi_bool enable)
{
    hi_svr_dispmng_interface_group attach;
    hi_svr_dispmng_interface_enable interface_enable;
    hi_s32 ret;

    // Get the attached interface of current display
    ret = hi_svr_dispmng_get_attached_interface(id, &attach);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_get_attached_interface fail(disp:%d, ret=0x%X).\n", id, ret);
    }

    // Enable attached interface
    interface_enable.intf[0] = attach.intf[0];
    interface_enable.enable[0] = enable;
    interface_enable.number = 1;
    ret = hi_svr_dispmng_set_intf_enable(id, &interface_enable);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_intf_enable failed 0x%X\n", ret);
    } else if (enable) {
        PRINT("Enable attached interface.\n");
    } else {
        PRINT("Disable attached interface.\n");
    }
}

hi_void display_brightness(const hi_svr_dispmng_display_id id, const hi_u32 brightness)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_set_brightness(id, brightness);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_brightness fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }
    PRINT("set display brightness to %d.\n", brightness);
}

hi_void display_contrast(const hi_svr_dispmng_display_id id, const hi_u32 contrast)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_set_contrast(id, contrast);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_contrast fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }
    PRINT("set display contrast to %d.\n", contrast);
}

hi_void display_hue(const hi_svr_dispmng_display_id id, const hi_u32 hue)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_set_hue(id, hue);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_hue fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }
    PRINT("set display hue to %d.\n", hue);
}

hi_void display_saturation(const hi_svr_dispmng_display_id id, const hi_u32 saturation)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_set_saturation(id, saturation);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_saturation fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }
    PRINT("set display saturation to %d.\n", saturation);
}

hi_void display_offset(const hi_svr_dispmng_display_id id, const hi_svr_dispmng_screen_offset offset)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_set_screen_offset(id, &offset);
    if (ret != HI_SUCCESS) {
        PRINT("[ERROR]hi_svr_dispmng_set_screen_offset fail(disp:%d, ret=0x%X).\n", id, ret);
        return;
    }
    PRINT("set display offset to [%d, %d, %d, %d].\n", offset.left, offset.top,
        offset.right, offset.bottom);
}

hi_void print_help()
{
    PRINT("\ninput  q  to quit\n" \
           "input  help   to display this message\n" \
           "input  info   to display info of sink device\n" \
           "input  capa   to display capability of sink device\n" \
           "input  status to display status of interface\n" \
           "input  modes  to display available display modes\n" \
           "input  all_modes  to display all display modes\n" \
           "input  mode   to display current display mode\n" \
           "input  enable to enable attached interface\n" \
           "input  disable to disable attached interface\n" \
           "input  offset [left] [top] [right] [bottom]  to set screen offset\n" \
           "input  brightness [0~1023] to set display brightness\n" \
           "input  contrast   [0~1023] to set display contrast\n" \
           "input  hue        [0~1023] to set display hue\n" \
           "input  saturation [0~1023] to set display saturation\n" \
           "input  auto   to set display mode to auto mode\n" \
           "input  format [format] to set display fomat, such as 1080P60\n" \
           "input  force  [format] to force set display fomat\n" \
           "input  rgb  to set pixel format to RGB\n" \
           "input  444  to set pixel format to YUV444\n" \
           "input  420  to set pixel format to YUV420\n" \
           "input  422  to set pixel format to YUV422\n" \
           "input  8    to set pixel width  to 8 bit\n" \
           "input  10   to set pixel width  to 10 bit\n" \
           "input  12   to set pixel width  to 12 bit\n");
}

#define MAX_BUFFER_SIZE     32
#define SLEEPING_TIME       500000
static hi_void process_input_cmd()
{
    hi_s32 ret;
    hi_char input_cmd[MAX_BUFFER_SIZE] = {0};
    hi_char fmt_str[MAX_BUFFER_SIZE] = {0};
    hi_unf_video_format out_fmt;

    print_help();

    while (1) {
        PRINT("\nplease input the q to quit!\n");
        PRINT("dispmng cmd>");
        fflush(stdout);
        memset_s(input_cmd, sizeof(input_cmd), 0, sizeof(input_cmd));
        ret = read(STDIN_FILENO, input_cmd, sizeof(input_cmd));
        if (ret <= 0) {
            usleep(SLEEPING_TIME);
            continue;
        }

        for (hi_s32 i = 0; i < sizeof(input_cmd); i++) {
            if (input_cmd[i] == '\n') {
                input_cmd[i] = '\0';
            }
        }

        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        if (!strcmp(input_cmd, "help")) {
            print_help();
            continue;
        }

        if (!strcmp(input_cmd, "info")) {
            print_display_info(g_disp_id);
            continue;
        }

        if (!strcmp(input_cmd, "capa")) {
            print_display_capability(g_disp_id);
            continue;
        }

        if (!strcmp(input_cmd, "status")) {
            print_display_status(g_disp_id);
            continue;
        }

        if (!strcmp(input_cmd, "modes")) {
            print_available_mode(g_disp_id);
            continue;
        }

        if (!strcmp(input_cmd, "all_modes")) {
            print_all_display_mode(g_disp_id);
            continue;
        }

        if (!strcmp(input_cmd, "mode")) {
            display_mode(g_disp_id);
            continue;
        }

        if (!strcmp(input_cmd, "enable")) {
            display_enable(g_disp_id, HI_TRUE);
            continue;
        }

        if (!strcmp(input_cmd, "disable")) {
            display_enable(g_disp_id, HI_FALSE);
            continue;
        }

        if (!strncmp(input_cmd, "brightness", strlen("brightness"))) {
            hi_u32 brightness;
            ret = sscanf_s(input_cmd, "brightness %d", &brightness);
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
                continue;
            }
            display_brightness(g_disp_id, brightness);
            continue;
        }

        if (!strncmp(input_cmd, "contrast", strlen("contrast"))) {
            hi_u32 contrast;
            ret = sscanf_s(input_cmd, "contrast %d", &contrast);
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
                continue;
            }
            display_contrast(g_disp_id, contrast);
            continue;
        }

        if (!strncmp(input_cmd, "hue", strlen("hue"))) {
            hi_u32 hue;
            ret = sscanf_s(input_cmd, "hue %d", &hue);
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
                continue;
            }
            display_hue(g_disp_id, hue);
            continue;
        }

        if (!strncmp(input_cmd, "saturation", strlen("saturation"))) {
            hi_u32 saturation;
            ret = sscanf_s(input_cmd, "saturation %d", &saturation);
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
                continue;
            }
            display_saturation(g_disp_id, saturation);
            continue;
        }

        if (!strncmp(input_cmd, "offset", strlen("offset"))) {
            hi_svr_dispmng_screen_offset offset;
            ret = sscanf_s(input_cmd, "offset %d %d %d %d", &offset.left, &offset.top,
                &offset.right, &offset.bottom);
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
                continue;
            }
            display_offset(g_disp_id, offset);
            continue;
        }

        if (!strcmp(input_cmd, "auto")) {
            g_disp_mode.vic = HI_SVR_DISPMNG_DISPLAY_MODE_AUTO;
            g_disp_mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
            g_disp_mode.bit_depth = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
            ret = hi_svr_dispmng_set_display_mode(g_disp_id, &g_disp_mode);
            if (ret != HI_SUCCESS) {
                PRINT("[ERROR]hi_svr_dispmng_set_display_mode fail(ret=0x%X).\n", ret);
            }
        }

        if (!strncmp(input_cmd, "format", strlen("format"))) {
            ret = sscanf_s(input_cmd, "format %s", fmt_str, sizeof(fmt_str));
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
            }
            out_fmt = hi_adp_str2fmt(fmt_str);
            ret = hi_svr_dispmng_get_vic_from_format(out_fmt, &g_disp_mode.vic);
            if (ret != HI_SUCCESS) {
                PRINT("[ERROR]Invalid display output format(format=%d).\n", out_fmt);
                continue;
            }
            g_disp_mode.format = out_fmt;
            ret = hi_svr_dispmng_set_display_mode(g_disp_id, &g_disp_mode);
            if (ret != HI_SUCCESS) {
                PRINT("[ERROR]Can not set mode to display %d(ret=0x%X).\n", g_disp_id, ret);
            }
        }

        if (!strncmp(input_cmd, "force", strlen("force"))) {
            ret = sscanf_s(input_cmd, "force %s", fmt_str, sizeof(fmt_str));
            if (ret == HI_FAILURE) {
                PRINT("[ERROR]invalid input string(ret=%d).\n", ret);
                continue;
            }
            out_fmt = hi_adp_str2fmt(fmt_str);
            g_disp_mode.format = out_fmt;
            g_disp_mode.vic    = HI_SVR_DISPMNG_DISPLAY_MODE_ADVANCED;
            ret = hi_svr_dispmng_set_display_mode(g_disp_id, &g_disp_mode);
            if (ret != HI_SUCCESS) {
                PRINT("[ERROR]Can not set mode to display %d(ret=0x%X).\n", g_disp_id, ret);
            }
        }

        if (!strcmp(input_cmd, "rgb")) {
            g_disp_mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_RGB;
            set_display_mode();
        }

        if (!strcmp(input_cmd, "444")) {
            g_disp_mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_YUV444;
            set_display_mode();
        }

        if (!strcmp(input_cmd, "420")) {
            g_disp_mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_YUV420;
            set_display_mode();
        }

        if (!strcmp(input_cmd, "422")) {
            g_disp_mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_YUV422;
            set_display_mode();
        }

        if (!strcmp(input_cmd, "8")) {
            g_disp_mode.bit_depth = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_8BIT;
            set_display_mode();
        }

        if (!strcmp(input_cmd, "10")) {
            g_disp_mode.bit_depth = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_10BIT;
            set_display_mode();
        }

        if (!strcmp(input_cmd, "12")) {
            g_disp_mode.bit_depth = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_12BIT;
            set_display_mode();
        }

        PRINT("display mode[vic:%d, format:%d, pixel format:%d, pixel bitwidth:%d].\n",
            g_disp_mode.vic, g_disp_mode.format, g_disp_mode.pixel_format, g_disp_mode.bit_depth);
    }
}