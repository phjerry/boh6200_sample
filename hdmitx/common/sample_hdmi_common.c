/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: common function of hdmi sample.
 * Author: Hisilicon hdmi software group
 * Create: 2019-08-22
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "sample_hdmi_common.h"
#include "hi_adp.h"

#define get_width(x)  (x) ? HI_SAMPLE_HD_WIDTH : HI_SAMPLE_UHD_WIDTH
#define get_height(x) (x) ? HI_SAMPLE_HD_HEIGHT : HI_SAMPLE_UHD_HEIGHT

static struct sample_context g_ctx;

static const struct fmt_name g_fmt_name[] = {
    { HI_UNF_VIDEO_FMT_NTSC, "ntsc" },
    { HI_UNF_VIDEO_FMT_PAL,  "pal" },

    { HI_UNF_VIDEO_FMT_480P_60,     "480p60" },
    { HI_UNF_VIDEO_FMT_576P_50,     "576p50" },
    { HI_UNF_VIDEO_FMT_720P_50,     "720p50" },
    { HI_UNF_VIDEO_FMT_720P_59_94,  "720p59.94" },
    { HI_UNF_VIDEO_FMT_720P_60,     "720p60" },
    { HI_UNF_VIDEO_FMT_1080I_50,    "1080i50" },
    { HI_UNF_VIDEO_FMT_1080I_59_94, "1080i59.94" },
    { HI_UNF_VIDEO_FMT_1080I_60,    "1080i60" },

    { HI_UNF_VIDEO_FMT_1080P_23_976, "1080p23.976" },
    { HI_UNF_VIDEO_FMT_1080P_24,     "1080p24" },
    { HI_UNF_VIDEO_FMT_1080P_25,     "1080p25" },
    { HI_UNF_VIDEO_FMT_1080P_29_97,  "1080p29.97" },
    { HI_UNF_VIDEO_FMT_1080P_30,     "1080p30" },
    { HI_UNF_VIDEO_FMT_1080P_50,     "1080p50" },
    { HI_UNF_VIDEO_FMT_1080P_59_94,  "1080p59.94" },
    { HI_UNF_VIDEO_FMT_1080P_60,     "1080p60" },
    { HI_UNF_VIDEO_FMT_1080P_100,    "1080p100" },
    { HI_UNF_VIDEO_FMT_1080P_119_88, "1080p119.88" },
    { HI_UNF_VIDEO_FMT_1080P_120,    "1080p120" },

    { HI_UNF_VIDEO_FMT_3840X2160_23_976, "3840x2160p23.976" },
    { HI_UNF_VIDEO_FMT_3840X2160_24,     "3840x2160p24" },
    { HI_UNF_VIDEO_FMT_3840X2160_25,     "3840x2160p25" },
    { HI_UNF_VIDEO_FMT_3840X2160_29_97,  "3840x2160p29.97" },
    { HI_UNF_VIDEO_FMT_3840X2160_30,     "3840x2160p30" },
    { HI_UNF_VIDEO_FMT_3840X2160_50,     "3840x2160p50" },
    { HI_UNF_VIDEO_FMT_3840X2160_59_94,  "3840x2160p59.94" },
    { HI_UNF_VIDEO_FMT_3840X2160_60,     "3840x2160p60" },
    { HI_UNF_VIDEO_FMT_3840X2160_100,    "3840x2160p100" },
    { HI_UNF_VIDEO_FMT_3840X2160_119_88, "3840x2160p119.88" },
    { HI_UNF_VIDEO_FMT_3840X2160_120,    "3840x2160p120" },

    { HI_UNF_VIDEO_FMT_4096X2160_23_976, "4096x2160p23.976" },
    { HI_UNF_VIDEO_FMT_4096X2160_24,     "4096x2160p24" },
    { HI_UNF_VIDEO_FMT_4096X2160_25,     "4096x2160p25" },
    { HI_UNF_VIDEO_FMT_4096X2160_29_97,  "4096x2160p29.97" },
    { HI_UNF_VIDEO_FMT_4096X2160_30,     "4096x2160p30" },
    { HI_UNF_VIDEO_FMT_4096X2160_50,     "4096x2160p50" },
    { HI_UNF_VIDEO_FMT_4096X2160_59_94,  "4096x2160p59.94" },
    { HI_UNF_VIDEO_FMT_4096X2160_60,     "4096x2160p60" },
    { HI_UNF_VIDEO_FMT_4096X2160_100,    "4096x2160p100" },
    { HI_UNF_VIDEO_FMT_4096X2160_119_88, "4096x2160p119.88" },
    { HI_UNF_VIDEO_FMT_4096X2160_120,    "4096x2160p120" },

    { HI_UNF_VIDEO_FMT_7680X4320_23_976, "7680x4320p23.976" },
    { HI_UNF_VIDEO_FMT_7680X4320_24,     "7680x4320p24" },
    { HI_UNF_VIDEO_FMT_7680X4320_25,     "7680x4320p25" },
    { HI_UNF_VIDEO_FMT_7680X4320_29_97,  "7680x4320p29.97" },
    { HI_UNF_VIDEO_FMT_7680X4320_30,     "7680x4320p30" },
    { HI_UNF_VIDEO_FMT_7680X4320_50,     "7680x4320p50" },
    { HI_UNF_VIDEO_FMT_7680X4320_59_94,  "7680x4320p59.94" },
    { HI_UNF_VIDEO_FMT_7680X4320_60,     "7680x4320p60" },
    { HI_UNF_VIDEO_FMT_7680X4320_100,    "7680x4320p100" },
    { HI_UNF_VIDEO_FMT_7680X4320_119_88, "7680x4320p119.88" },
    { HI_UNF_VIDEO_FMT_7680X4320_120,    "7680x4320p120" },

    { HI_UNF_VIDEO_FMT_861D_640X480_60,      "640x480" },
    { HI_UNF_VIDEO_FMT_VESA_800X600_60,      "800x600" },
    { HI_UNF_VIDEO_FMT_VESA_1024X768_60,     "1024x768" },
    { HI_UNF_VIDEO_FMT_VESA_1280X720_60,     "1280x720" },
    { HI_UNF_VIDEO_FMT_VESA_1280X800_60,     "1280x800" },
    { HI_UNF_VIDEO_FMT_VESA_1280X1024_60,    "1280x1024" },
    { HI_UNF_VIDEO_FMT_VESA_1360X768_60,     "1360x768" },
    { HI_UNF_VIDEO_FMT_VESA_1366X768_60,     "1366x768" },
    { HI_UNF_VIDEO_FMT_VESA_1400X1050_60,    "1400x1050" },
    { HI_UNF_VIDEO_FMT_VESA_1440X900_60,     "1440x900" },
    { HI_UNF_VIDEO_FMT_VESA_1440X900_60_RB,  "1440x900rb" },
    { HI_UNF_VIDEO_FMT_VESA_1600X900_60_RB,  "1600x900rb" },
    { HI_UNF_VIDEO_FMT_VESA_1600X1200_60,    "1600x1200" },
    { HI_UNF_VIDEO_FMT_VESA_1680X1050_60,    "1680x1050" },
    { HI_UNF_VIDEO_FMT_VESA_1680X1050_60_RB, "1680x1050rb" },
    { HI_UNF_VIDEO_FMT_VESA_1920X1080_60,    "1920x1080" },
    { HI_UNF_VIDEO_FMT_VESA_1920X1200_60,    "1920x1200" },
    { HI_UNF_VIDEO_FMT_VESA_1920X1440_60,    "1920x1440" },
    { HI_UNF_VIDEO_FMT_VESA_2048X1152_60,    "2048x1152" },
    { HI_UNF_VIDEO_FMT_VESA_2560X1440_60_RB, "2560x1440rb" },
    { HI_UNF_VIDEO_FMT_VESA_2560X1600_60_RB, "2560x1600rb" },
};

/* EIA-CEA-861-F Table 29 */
const static hi_char *g_audio_ext_code_type_str[] = {
    "Refer",
    "Not in use",
    "Not in use",
    "Not in use",
    "4 HE AAC", /* MPEG4 */
    "4 HE AACv2",
    "4 AAC LC",
    "DRA",
    "4 HE AAC+",
    "MPEG-H 3D",
    "AC-4",
    "L-PCM 3D",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

/* EIA-CEA-861-F Table 27 */
const static hi_char *g_audio_code_type_str[] = {
    "STREAM",
    "L-PCM",
    "AC-3",
    "MPEG-1",
    "MP3",
    "MPEG2",
    "AAC LC",
    "DTS",
    "ATRAC",
    "OneBitAo",
    "EAC-3",
    "DTS-HD",
    "MAT",
    "DST",
    "WMA Pro",
    "ERROR",
};

int string2i(const char *string)
{
    int index;

    if (!string) {
        return HI_FAILURE;
    }

    if (strlen(string) > HDMI_MAX_NUM_STRING_LEN) {
        return HI_FAILURE;
    }

    for (index = 0; index < strlen(string); index++) {
        if (isdigit(string[index])) {
            continue;
        }
        return HI_FAILURE;
    }

    return (atoi(string));
}

int fmt2index(hi_unf_video_format fmt)
{
    int index;
    for (index = 0; index < ARRAY_SIZE(g_fmt_name); index++) {
        if (g_fmt_name[index].fmt == fmt) {
            return index;
        }
    }
    return HI_FAILURE;
}

char *fmt2name(hi_unf_video_format fmt)
{
    int index;

    for (index = 0; index < ARRAY_SIZE(g_fmt_name); index++) {
        if (g_fmt_name[index].fmt == fmt) {
            return g_fmt_name[index].name;
        }
    }
    return NULL;
}

char *vic2fmt_name(unsigned int vic)
{
    char *name;
    hi_unf_video_format fmt;
    hi_svr_dispmng_get_format_from_vic(vic, &fmt);
    name = fmt2name(fmt);
    if (name == NULL) {
        return "not support";
    }
    return name;
}

char *colorspace2name(hi_svr_dispmng_pixel_format colorspace)
{
    return (colorspace == HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO) ? "auto" : \
        (colorspace == HI_SVR_DISPMNG_PIXEL_FORMAT_RGB) ? "rgb444" : \
        (colorspace == HI_SVR_DISPMNG_PIXEL_FORMAT_YUV422) ? "yuv422" : \
        (colorspace == HI_SVR_DISPMNG_PIXEL_FORMAT_YUV444) ? "yuv444" : \
        (colorspace == HI_SVR_DISPMNG_PIXEL_FORMAT_YUV420) ? "yuv420" : "error";
}

char *deepcolor2name(hi_svr_dispmng_pixel_bit_depth deepcolor)
{
    return (deepcolor == HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO) ? "auto" : \
        (deepcolor == HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_8BIT) ? "8bit" : \
        (deepcolor == HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_10BIT) ? "10bit" : \
        (deepcolor == HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_12BIT) ? "12bit" : "error";
}

char *hotplug2name(hi_unf_hdmitx_hotplug_status hotplug)
{
    return (hotplug == HI_UNF_HDMITX_HOTPLUG_DETECTING) ? "detecting" : \
        (hotplug == HI_UNF_HDMITX_HOTPLUG_IN) ? "hotplug in" : \
        (hotplug == HI_UNF_HDMITX_HOTPLUG_OUT) ? "hotplug out" : \
        (hotplug == HI_UNF_HDMITX_HOTPLUG_DET_FAIL) ? "detect fail" : "error";
}

char *rsen2name(hi_unf_hdmitx_rsen_status resen)
{
    return (resen == HI_UNF_HDMITX_RSEN_DISCONNECT) ? "disconnect" : \
        (resen == HI_UNF_HDMITX_RSEN_CONNECT) ? "connect" : \
        (resen == HI_UNF_HDMITX_RSEN_DET_FAIL) ? "detect fail" : "error";
}

#define MAX_COLORCAP_NAME_LEN 100
char *colorcap2name(unsigned int colorcap)
{
    static char name[MAX_COLORCAP_NAME_LEN];
    memset(name, 0, sizeof(name));
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_RGB_MASK) ? "rgb444" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_RGB_24) ? "-8" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_RGB_30) ? "-10" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_RGB_36) ? "-12" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_RGB_48) ? "-16" : "");

    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y444_MASK) ? "|yuv444" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y444_24) ? "-8" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y444_30) ? "-10" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y444_36) ? "-12" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y444_48) ? "-16" : "");

    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y420_MASK) ? "|yuv420" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y420_24) ? "-8" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y420_30) ? "-10" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y420_36) ? "-12" : "");
    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y420_48) ? "-16" : "");

    strcat(name, (colorcap & HI_UNF_HDMITX_COLOR_Y422) ? "|yuv422" : "");
    return name;
}

hi_unf_video_format name2fmt(const char *name)
{
    int index;

    for (index = 0; index < ARRAY_SIZE(g_fmt_name); index++) {
        if (!strcmp(name, g_fmt_name[index].name)) {
            return g_fmt_name[index].fmt;
        }
    }
    return HI_UNF_VIDEO_FMT_MAX;
}

hi_svr_dispmng_pixel_format name2colorspace(const char *name)
{
    if (!strcmp(name, "auto")) {
        return HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
    } else if (!strcmp(name, "rgb444")) {
        return HI_SVR_DISPMNG_PIXEL_FORMAT_RGB;
    } else if (!strcmp(name, "yuv422")) {
        return HI_SVR_DISPMNG_PIXEL_FORMAT_YUV422;
    } else if (!strcmp(name, "yuv444")) {
        return HI_SVR_DISPMNG_PIXEL_FORMAT_YUV444;
    } else if (!strcmp(name, "yuv420")) {
        return HI_SVR_DISPMNG_PIXEL_FORMAT_YUV420;
    }
    return HI_SVR_DISPMNG_PIXEL_FORMAT_MAX;
}

hi_svr_dispmng_pixel_bit_depth name2deepcolor(const char *name)
{
    if (!strcmp(name, "auto")) {
        return HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
    } else if (!strcmp(name, "8bit")) {
        return HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_8BIT;
    } else if (!strcmp(name, "10bit")) {
        return HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_10BIT;
    } else if (!strcmp(name, "12bit")) {
        return HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_12BIT;
    }
    return HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_MAX;
}

static const char *hdcp_ver_to_name(hi_unf_hdmitx_hdcp_ver version)
{
    return (version == HI_UNF_HDMITX_HDCP_VER_NONE) ? "don't start hdcp" : \
           (version == HI_UNF_HDMITX_HDCP_VER_14) ? "HDCP1.4" : \
           (version == HI_UNF_HDMITX_HDCP_VER_22) ? "HDCP2.2" : \
           (version == HI_UNF_HDMITX_HDCP_VER_23) ? "HDCP2.3" : "error";
}

static const char *hdcp_err_code_to_name(hi_unf_hdmitx_hdcp_err err_code)
{
    return (err_code == HI_UNF_HDMITX_HDCP_ERR_UNDO) ? "don't start hdcp" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_NONE) ? "no auth error" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_PLUG_OUT) ? "cable plug out" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_NO_SIGNAL) ? "signal output disable" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_NO_KEY) ? "do not load key" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_INVALID_KEY) ? "key is invalid" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_DDC) ? "DDC link error" : \
           (err_code == HI_UNF_HDMITX_HDCP_ERR_ON_SRM) ? "on revocation list" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_BCAP_FAIL) ? "read BCAP failed" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_BSKV_FAIL) ? "read BKSV failed" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_INTEGRITY_FAIL_R0) ? "R0(R0') integrety failure" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_WATCHDOG_TIMEOUT) ? "Repeater watch dog timeout" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_VI_CHCECK_FAIL) ? "Vi(Vi') check failed" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_EXCEEDED_TOPOLOGY) ? "exceeded toplogy" : \
           (err_code == HI_UNF_HDMITX_HDCP_1X_INTEGRITY_FAIL_RI) ? "Ri(Ri') integrety failure" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_SIGNATURE_FAIL) ? "signature error" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_MISMATCH_H) ? "mismatch  H(H')" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_AKE_TIMEOUT) ? "AKE timeout" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_LOCALITY_FAIL) ? "locality check failed" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_REAUTH_REQ) ? "reauth request" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_WATCHDOG_TIMEOUT) ? "watchdog timeout" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_V_MISMATCH) ? "mismatch  V(V')" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_ROLL_OVER) ? "no roll-over of seq_num_V" : \
           (err_code == HI_UNF_HDMITX_HDCP_2X_EXCEEDED_TOPOLOGY) ? "exceeded toplogy" : "error";
}

struct sample_context *get_ctx(void)
{
    return &g_ctx;
}

hi_bool check_hdmi_id(hi_svr_dispmng_hdmitx_id hdmi_id)
{
    if ((hdmi_id == HI_SVR_DISPMNG_HDMITX_ID_0) ||
        (hdmi_id == HI_SVR_DISPMNG_HDMITX_ID_1)) {
        return HI_TRUE;
    }
    return HI_FALSE;
}

hi_bool check_disp_id(hi_svr_dispmng_display_id disp_id)
{
    if ((disp_id == HI_SVR_DISPMNG_DISPLAY_ID_0) ||
        (disp_id == HI_SVR_DISPMNG_DISPLAY_ID_1)) {
        return HI_TRUE;
    }
    return HI_FALSE;
}

void show_hdmi_status(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_status *status)
{
    printf("\33[36mHDMI%d status:\33[0m\n", hdmi_id);
    printf("hotplug : %s\n"
           "resen   : %s\n"
           "output  : %s\n",
           hotplug2name(status->hotplug),
           rsen2name(status->rsen),
           bool2name(status->output_enable));
}

void show_hdcp_status(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_hdcp_status *status)
{
    printf("\33[36mHDCP%d status:\33[0m\n", hdmi_id);
    printf("auth_start : %s\n"
           "auth_success   : %s\n"
           "cur_reauth_times  : %d\n"
           "version  : %s\n"
           "err_code  : %s\n",
           status->auth_start ? "YES" : "NO",
           status->auth_success ? "YES" : "NO",
           status->cur_reauth_times,
           hdcp_ver_to_name(status->version),
           hdcp_err_code_to_name(status->err_code));
}

void show_hdmi_avail_cap(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_avail_cap *cap)
{
    printf("\33[36mHDMI%d available capability:\33[0m\n", hdmi_id);

    printf("%03d|%s(native) %s\n",cap->native_mode.vic, vic2fmt_name(cap->native_mode.vic),
           colorcap2name(cap->native_mode.color_cap));
    printf("%03d|%s(max)    %s\n",cap->max_mode.vic, vic2fmt_name(cap->max_mode.vic),
           colorcap2name(cap->max_mode.color_cap));
    printf("available modes count : %d\n"
           "sink maximum TMDSCLK  : %d\n"
           "DVI only : %s\n",
           cap->mode_cnt,
           cap->max_tmds_clock,
           bool2name(cap->dvi_only));
}

#define MAX_FMT_NAME_LEN 20
void show_hdmi_avail_mods(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_avail_mode *mode,
    unsigned int mode_cnt)
{
    int i;
    char fmt_name[MAX_FMT_NAME_LEN] = {};

    printf("\33[36mHDMI%d available modes:\33[0m\n", hdmi_id);
    printf("\33[35mindex  vic       timing          pixel clk   colorspace&deepcolor\33[0m\n");
    for (i = 0; i < mode_cnt; i++) {
        sprintf(fmt_name, "%dx%d", mode[i].detail_timing.h_active, mode[i].detail_timing.v_active);
        strcat(fmt_name, mode[i].detail_timing.progressive ? "p" : "i");
        sprintf(fmt_name, "%s%d", fmt_name, mode[i].detail_timing.field_rate);
        printf("%-7d%-10d%-16s%-12d%s\n",i, mode[i].vic, fmt_name,
            mode[i].detail_timing.pixel_clock, colorcap2name(mode[i].color_cap));
    }
}

void show_all_fmt_and_cap(hi_svr_dispmng_hdmitx_id hdmi_id)
{
    int ret;
    int i, j;
    unsigned int mode_cnt;
    hi_unf_hdmitx_avail_cap avail_cap = {};
    hi_unf_hdmitx_avail_mode *avail_modes;
    unsigned int vic;

    if (!check_hdmi_id(hdmi_id)) {
        HDMI_LOG_ERR("invalid hdmi_id = %d\n", hdmi_id);
        return;
    }
    if (!check_disp_id(g_ctx.disp_id[hdmi_id])) {
        HDMI_LOG_ERR("hdmi%d not attach to a display\n", hdmi_id);
        return;
    }

    ret = hi_unf_hdmitx_get_avail_capability((hi_unf_hdmitx_id)hdmi_id, &avail_cap);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call HI_UNF_HDMI_GetAvailCapability(hdmi%d) failed\n", hdmi_id);
        avail_cap.mode_cnt = MAX_AVAIL_MODES_CNT;
    }

    avail_modes = (hi_unf_hdmitx_avail_mode *)malloc(avail_cap.mode_cnt * sizeof(hi_unf_hdmitx_avail_mode));
    if (!avail_modes) {
        HDMI_LOG_ERR("malloc failed!\n");
        return;
    }

    ret = hi_unf_hdmitx_get_avail_modes((hi_unf_hdmitx_id)hdmi_id, avail_modes, avail_cap.mode_cnt, &mode_cnt);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call HI_UNF_HDMI_GetAvailModes(hdmi%d) failed!\n", hdmi_id);
        mode_cnt = 0;
    }

    for (i = 0; i < ARRAY_SIZE(g_fmt_name); i++) {
        hi_svr_dispmng_get_vic_from_format(g_fmt_name[i].fmt, &vic);
        for (j = 0; j < mode_cnt; j++) {
            if (avail_modes[j].vic == vic) {
                printf("%03d|%-17s-%s\n", g_fmt_name[i].fmt, g_fmt_name[i].name,
                    colorcap2name(avail_modes[j].color_cap));
                break;
            }
        }
        if (j >= mode_cnt) {
            printf("%03d|%-17s\33[31m-not support\33[0m\n",
                g_fmt_name[i].fmt, g_fmt_name[i].name);
        }
    }

    free(avail_modes);
    return;
}

void show_sink_base_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d base info:\33[0m\n", hdmi_id);
    printf("Version: %d.%d\n", info->edid_version.version, info->edid_version.revision);
    printf("manufacture name : %s\n", info->manufacture.mfrs_name);
    printf("manufacture time : %04d-%02d\n", info->manufacture.year, info->manufacture.week);
    printf("product code     : %d\n", info->manufacture.product_code);
    printf("serial numeber   : %d\n", info->manufacture.serial_number);
    printf("\n");
}

void show_sink_color_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d color info\33[0m\n", hdmi_id);
    printf("xvYCC601         : %s\n", bool2name(info->colorimetry.xvycc601));
    printf("xvYCC709         : %s\n", bool2name(info->colorimetry.xvycc709));
    printf("sYCC601          : %s\n", bool2name(info->colorimetry.sycc601));
    printf("AdobleYCC601     : %s\n", bool2name(info->colorimetry.adobe_ycc601));
    printf("AdobleRGB        : %s\n", bool2name(info->colorimetry.adobe_rgb));
    printf("BT2020CYCC       : %s\n", bool2name(info->colorimetry.bt2020_c_ycc));
    printf("BT2020YCC        : %s\n", bool2name(info->colorimetry.bt2020_ycc));
    printf("BT2020RGB        : %s\n", bool2name(info->colorimetry.bt2020_rgb));
    printf("\n");
}

void show_sink_latency_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d latency info\33[0m\n", hdmi_id);
    printf("valid            : %s\n", bool2name(info->latency.present));
    printf("pvideo           : %d\n", info->latency.p_video);
    printf("paudio           : %d\n", info->latency.p_audio);
    printf("ivideo           : %d\n", info->latency.i_video);
    printf("iaudio           : %d\n", info->latency.i_audio);
    printf("\n");
}

void show_sink_display_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d display info\33[0m\n", hdmi_id);
    printf("MaxDispWidth     : %d\n", info->disp_para.max_image_width);
    printf("MaxDispHeight    : %d\n", info->disp_para.max_image_height);
    printf("\n");
}

void show_sink_speaker_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d speaker info\33[0m\n", hdmi_id);
    printf("front center(FC) : %s\n", bool2name(info->speaker.fc));
    printf("rear center(RC)  : %s\n", bool2name(info->speaker.rc));
    printf("top center(TC)   : %s\n", bool2name(info->speaker.tc));
    printf("front left&right(FL/FR)  : %s\n", bool2name(info->speaker.fl_fr));
    printf("rear left&right(RL/RR)   : %s\n", bool2name(info->speaker.rl_rr));
    printf("low frequency effect(LFE): %s\n", bool2name(info->speaker.lfe));
    printf("front center high(FCH)   : %s\n", bool2name(info->speaker.fch));
    printf("front left&right center(FLC/FRC) : %s\n", bool2name(info->speaker.flc_frc));
    printf("rear left&right center(RLC/RRC)  : %s\n", bool2name(info->speaker.rlc_rrc));
    printf("front left&right wide(FLW/FRW)   : %s\n", bool2name(info->speaker.flw_frw));
    printf("front left&right high(FLH/FRH)   : %s\n", bool2name(info->speaker.flh_frh));
    printf("\n");
}

#define MAX_SAMPLE_RATE_NAME_LEN 50
void show_sink_audio_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    int i;
    char name[MAX_SAMPLE_RATE_NAME_LEN] = {};

    printf("\33[36mHDMI%d audio info\33[0m\n", hdmi_id);
    printf("basic audio      : %s\n", bool2name(info->audio.basic_support));
    printf("total            : %d\n", info->audio.audio_fmt_cnt);
    for (i = 0; i < info->audio.audio_fmt_cnt; i++) {
        printf("\33[36mnum.%02d\33[0m\n", i);
        printf("code type        : %s\n", info->audio.sad[i].fmt_code < ARRAY_SIZE(g_audio_code_type_str) ?
            g_audio_code_type_str[info->audio.sad[i].fmt_code] : "error");
        printf("extern code type : %s\n", info->audio.sad[i].ext_code < ARRAY_SIZE(g_audio_ext_code_type_str) ?
            g_audio_ext_code_type_str[info->audio.sad[i].ext_code] : "error");
        printf("maximum channel  : %d\n", info->audio.sad[i].max_channel);
        printf("maximum bit rate : %d\n", info->audio.sad[i].max_bit_rate);
        memset(name, 0, sizeof(name));
        strcat(name, info->audio.sad[i].samp_32k ? "32k " : "");
        strcat(name, info->audio.sad[i].samp_44p1k ? "44.1k " : "");
        strcat(name, info->audio.sad[i].samp_48k ? "48k " : "");
        strcat(name, info->audio.sad[i].samp_88p2k ? "88.2k " : "");
        strcat(name, info->audio.sad[i].samp_96k ? "96k " : "");
        strcat(name, info->audio.sad[i].samp_176p4k ? "176.4k " : "");
        strcat(name, info->audio.sad[i].samp_192k ? "192k " : "");
        printf("sample rate      : %s\n", name);
        memset(name, 0, sizeof(name));
        strcat(name, info->audio.sad[i].width_16 ? "16 " : "");
        strcat(name, info->audio.sad[i].width_20 ? "20 " : "");
        strcat(name, info->audio.sad[i].width_24 ? "24 " : "");
        printf("bit width        : %s\n", name);
        printf("dependent        : %d\n", info->audio.sad[i].dependent);
        printf("profile          : %d\n", info->audio.sad[i].profile);
        printf("1024_TL          : %s\n", bool2name(info->audio.sad[i].len_1024tl));
        printf("960_TL           : %s\n", bool2name(info->audio.sad[i].len_960tl));
        printf("MPS_L            : %s\n", bool2name(info->audio.sad[i].mps_l));
    }
    printf("\n");
}

static void show_dolby0_info(hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mdolby version0\33[0m: \n");
    printf("support          : %s\n", bool2name(info->dolby.support_v0));
    printf("Y422_12bit       : %s\n", bool2name(info->dolby.v0.ycbcr422_12bit));
    printf("2160p60          : %s\n", bool2name(info->dolby.v0.cap_2160p60));
    printf("global dimming   : %s\n", bool2name(info->dolby.v0.global_dimming));
    printf("DM major version : %d\n", info->dolby.v0.dm_major_ver);
    printf("DM minor version : %d\n", info->dolby.v0.dm_minor_ver);
    printf("target min PQ    : %d\n", info->dolby.v0.target_min_pq);
    printf("target max PQ    : %d\n", info->dolby.v0.target_max_pq);
    printf("whiteX           : %d\n", info->dolby.v0.white_x);
    printf("whiteY           : %d\n", info->dolby.v0.white_y);
    printf("redX             : %d\n", info->dolby.v0.red_x);
    printf("redY             : %d\n", info->dolby.v0.red_y);
    printf("greenX           : %d\n", info->dolby.v0.green_x);
    printf("greenY           : %d\n", info->dolby.v0.green_y);
    printf("blueX            : %d\n", info->dolby.v0.blue_x);
    printf("blueY            : %d\n", info->dolby.v0.blue_y);
}

static void show_dolby1_info(hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mdolby version1\33[0m: \n");
    printf("support          : %s\n", bool2name(info->dolby.support_v1));
    printf("Y422_12bit       : %s\n", bool2name(info->dolby.v1.ycbcr422_12bit));
    printf("2160p60          : %s\n", bool2name(info->dolby.v1.cap_2160p60));
    printf("global dimming   : %s\n", bool2name(info->dolby.v1.global_dimming));
    printf("colorimetry      : %s\n", bool2name(info->dolby.v1.colorimetry));
    printf("DM version       : %d\n", info->dolby.v1.dm_version);
    printf("low latency      : %d\n", info->dolby.v1.low_latency);
    printf("target min lum   : %d\n", info->dolby.v1.target_min_lum);
    printf("target max lum   : %d\n", info->dolby.v1.target_max_lum);
    printf("redX             : %d\n", info->dolby.v1.red_x);
    printf("redY             : %d\n", info->dolby.v1.red_y);
    printf("greenX           : %d\n", info->dolby.v1.green_x);
    printf("greenY           : %d\n", info->dolby.v1.green_y);
    printf("blueX            : %d\n", info->dolby.v1.blue_x);
    printf("blueY            : %d\n", info->dolby.v1.blue_y);
}

static void show_dolby2_info(hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mdolby version2\33[0m: \n");
    printf("support          : %s\n", bool2name(info->dolby.support_v2));
    printf("Y422_12bit       : %s\n", bool2name(info->dolby.v2.ycbcr422_12bit));
    printf("backlight ctrl   : %s\n", bool2name(info->dolby.v2.back_light_ctrl));
    printf("global dimming   : %s\n", bool2name(info->dolby.v2.global_dimming));
    printf("DM version       : %d\n", info->dolby.v2.dm_version);
    printf("backlight min lum: %d\n", info->dolby.v2.back_lt_min_lum);
    printf("interface        : %d\n", info->dolby.v2.interface);
    printf("Y444Rgb30b36b    : %d\n", info->dolby.v2.y444_rgb_10b12b);
    printf("target min PQ2   : %d\n", info->dolby.v2.target_min_pq_v2);
    printf("target max PQ2   : %d\n", info->dolby.v2.target_max_pq_v2);
    printf("redX             : %d\n", info->dolby.v2.red_x);
    printf("redY             : %d\n", info->dolby.v2.red_y);
    printf("greenX           : %d\n", info->dolby.v2.green_x);
    printf("greenY           : %d\n", info->dolby.v2.green_y);
    printf("blueX            : %d\n", info->dolby.v2.blue_x);
    printf("blueY            : %d\n", info->dolby.v2.blue_y);
}

void show_sink_dolby_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d dolby info:\33[0m\n", hdmi_id);
    show_dolby0_info(info);
    show_dolby1_info(info);
    show_dolby2_info(info);
    printf("\n");
}

void show_sink_hdr_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d HDR info:\33[0m\n", hdmi_id);
    printf("SDR              : %s\n", bool2name(info->hdr.eotf.traditional_sdr));
    printf("HDR              : %s\n", bool2name(info->hdr.eotf.traditional_hdr));
    printf("SMPTE ST2084     : %s\n", bool2name(info->hdr.eotf.hdr10));
    printf("HLG              : %s\n", bool2name(info->hdr.eotf.hlg));

    printf("static type1     : %s\n", bool2name(info->hdr.static_metadata.stype1));
    printf("max lum          : %d\n", info->hdr.static_metadata.max_lum_cv);
    printf("min lum          : %d\n", info->hdr.static_metadata.aver_lum_cv);
    printf("average lum      : %d\n", info->hdr.static_metadata.min_lum_cv);

    printf("dynamic type1    : %s   version: %d\n",
        bool2name(info->hdr.dynamic_metadata.d_type1_support),
        info->hdr.dynamic_metadata.d_type1_version);
    printf("dynamic type2    : %s   version: %d\n",
        bool2name(info->hdr.dynamic_metadata.d_type2_support),
        info->hdr.dynamic_metadata.d_type2_version);
    printf("dynamic type3    : %s\n",
        bool2name(info->hdr.dynamic_metadata.d_type3_support));
    printf("dynamic type4    : %s   version: %d\n",
        bool2name(info->hdr.dynamic_metadata.d_type4_support),
        info->hdr.dynamic_metadata.d_type4_version);
    printf("\n");
}

void show_sink_vrr_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info)
{
    printf("\33[36mHDMI%d VRR info:\33[0m\n", hdmi_id);
    printf("FVA          : %s\n", bool2name(info->vrr.fva));
    printf("CNM VRR      : %s\n", bool2name(info->vrr.cnm_vrr));
    printf("cinema VRR   : %s\n", bool2name(info->vrr.cinema_vrr));
    printf("mdelta       : %s\n", bool2name(info->vrr.m_delta));
    printf("FAPA         : %s\n", bool2name(info->vrr.fapa_start_locat));
    printf("ALLM         : %s\n", bool2name(info->vrr.allm));
    printf("VRR min      : %d\n", info->vrr.vrr_min);
    printf("VRR max      : %d\n", info->vrr.vrr_max);
}

void init_intf_and_output_timing(hi_svr_dispmng_display_id disp_id)
{
    hi_svr_dispmng_display_mode mode = {};

    /* Automatic setting display mode, normally it will use the native mode of the connected TV */
    mode.vic          = HI_SVR_DISPMNG_DISPLAY_MODE_AUTO;
    mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
    mode.bit_depth    = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
    hi_svr_dispmng_set_display_mode(disp_id, &mode);
}

hi_s32 sample_disp_init(hi_void)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_init();
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("hi_unf_dispmng_init failed 0x%x\n", ret);
        return ret;
    }

    return ret;
}

hi_s32 sample_disp_deinit(hi_void)
{
    hi_s32 ret;

    ret = hi_svr_dispmng_deinit();
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("hi_svr_dispmng_deinit failed 0x%x\n", ret);
        return ret;
    }

    return ret;
}


hi_s32 sample_disp_open(hi_u32 id)
{
    hi_s32 display_count;
    hi_s32 index;
    hi_svr_dispmng_display_id disp_id;
    hi_svr_dispmng_interface_group intf;
    hi_svr_dispmng_screen_offset offset;

    if (hi_svr_dispmng_get_display_count(&display_count) != HI_SUCCESS) {
        return HI_FAILURE;
    }

    for (index = 0; index < display_count; index++) {
        if (hi_svr_dispmng_get_display_id(index, &disp_id) != HI_SUCCESS) {
            return HI_FAILURE;
        }

        if (disp_id == id) {
            break;
        }
    }

    if (index >= display_count) {
        printf("not find this disp%d.", id);
        return HI_FAILURE;
    }
    /*
     * Attach interface, enable interface and set display mode has been initialized in hi_unf_dispmng_init
     * when official release. So please delete the debug code when release.
     */
#ifdef SAMPLE_DEBUG
    init_intf_and_output_timing(disp_id);
#endif

    if (hi_svr_dispmng_set_virtual_screen(disp_id, get_width(disp_id), get_height(disp_id)) != HI_SUCCESS) {
        printf("hi_svr_dispmng_set_virtual_screen%d.", id);
        return HI_FAILURE;
    }

    offset.left = 0;
    offset.top = 0;
    offset.right = 0;
    offset.bottom = 0;
    if (hi_svr_dispmng_set_screen_offset(disp_id, (const hi_svr_dispmng_screen_offset *)&offset) != HI_SUCCESS) {
        printf("hi_svr_dispmng_set_screen_offset%d.", id);
        return HI_FAILURE;
    }

    if (hi_unf_disp_open((hi_unf_disp)disp_id) != HI_SUCCESS) {
        printf("hi_unf_disp_open%d.", id);
        return HI_FAILURE;
    }

    /* Initialize the global context */
    hi_svr_dispmng_get_attached_interface(disp_id, &intf);
    if (intf.intf[0].intf_type == HI_SVR_DISPMNG_INTERFACE_HDMITX) {
        g_ctx.disp_id[intf.intf[0].intf.hdmitx_id] = disp_id;
    }

    if (disp_id == HI_SVR_DISPMNG_DISPLAY_ID_MASTER) {
        g_ctx.current_hdmi = intf.intf[0].intf.hdmitx_id;
    }

    return HI_SUCCESS;
}

hi_void sample_disp_close(hi_u32 id)
{
    hi_s32 ret;
    hi_s32 display_count;
    hi_s32 index;
    hi_svr_dispmng_display_id disp_id;

    ret = hi_svr_dispmng_get_display_count(&display_count);
    if (ret != HI_SUCCESS) {
        display_count = HI_SVR_DISPMNG_DISPLAY_INDEX_MAX;
    }

    for (index = 0; index < display_count; index++) {
        hi_svr_dispmng_get_display_id(index, &disp_id);
        if (disp_id == id) {
            break;
        }
    }

    if (index >= display_count) {
        printf("not find this disp%d.", id);
        return;
    }

    hi_unf_disp_close((hi_unf_disp)disp_id);

    hi_unf_hdmitx_cec_unregister_callback(id);
    hi_unf_hdmitx_cec_unregister_callback(id);
    hi_unf_hdmitx_cec_enable(id, HI_FALSE);
    hi_unf_hdmitx_cec_enable(id, HI_FALSE);

    return;
}

hi_s32 sample_snd_open(hi_u32 id)
{
    hi_s32 ret;
    hi_unf_snd_attr attr;

    ret = hi_unf_snd_get_default_open_attr(id, &attr);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_snd_get_default_open_attr(snd%d) failed.\n", id);
        return ret;
    }

    attr.port_num = 1;
    attr.port_attr[0].out_port = id ? HI_UNF_SND_OUT_PORT_HDMITX1 : HI_UNF_SND_OUT_PORT_HDMITX0;
    ret = hi_unf_snd_open(id, &attr);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_snd_open(snd%d) failed.\n", id);
        return ret;
    }

    g_ctx.sound[id].sound = id;
    g_ctx.sound[id].port  = id ? HI_UNF_SND_OUT_PORT_HDMITX1 : HI_UNF_SND_OUT_PORT_HDMITX0;

    return ret;
}

hi_s32 sample_snd_close(hi_u32 id)
{
    hi_s32 ret;

    ret = hi_unf_snd_close(id);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_snd_close(snd%d) failed.\n", id);
        return ret;
    }

    g_ctx.sound[id].sound = HI_UNF_SND_MAX;
    g_ctx.sound[id].port  = HI_UNF_SND_OUT_PORT_MAX;

    return ret;
}

hi_s32 sample_snd_init(hi_void)
{
    hi_s32 ret;

    ret = hi_unf_snd_init();
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_snd_init failed.\n");
    }

    return ret;
}

hi_s32 sample_snd_deinit(hi_void)
{
    hi_s32 ret;

    ret = hi_unf_snd_deinit();
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_snd_deinit failed.\n");
        return ret;
    }

    return ret;
}

