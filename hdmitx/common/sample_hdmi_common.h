/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: common function header of hdmi sample.
 * Author: Hisilicon hdmi software group
 * Create: 2019-08-22
 */

#ifndef __SAMPLE_HDMI_COMMON_H__
#define __SAMPLE_HDMI_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hi_unf_hdmitx.h"
#include "hi_unf_disp.h"
#include "hi_unf_sound.h"
#include "hi_svr_dispmng.h"

#define SAMPLE_DEBUG

#define HDMI_MAX_ARGC           10
#define HDMI_MAX_PAMRM_SIZE     30
#define HDMI_MAX_NUM_STRING_LEN 16

#define RAW_EDID_MAX_LEN        1024

#define MAX_AVAIL_MODES_CNT     100
#define HI_SAMPLE_UHD_WIDTH   3840
#define HI_SAMPLE_UHD_HEIGHT  2160
#define HI_SAMPLE_HD_WIDTH    1280
#define HI_SAMPLE_HD_HEIGHT    720


#define HDMI_LOG_INFO(fmt, ...) do { \
    printf("\33[0m"); \
    printf("INFO-%s[%d]: ", __FUNCTION__, __LINE__); \
    printf(fmt, ##__VA_ARGS__); \
} while (0)
#define HDMI_LOG_WARN(fmt, ...) do { \
    printf("\33[33m"); \
    printf("WARN-%s[%d]: ", __FUNCTION__, __LINE__); \
    printf(fmt, ##__VA_ARGS__); \
    printf("\33[0m"); \
} while (0)
#define HDMI_LOG_ERR(fmt, ...) do { \
    printf("\33[31m"); \
    printf("ERROR-%s[%d]: ", __FUNCTION__, __LINE__); \
    printf(fmt, ##__VA_ARGS__); \
    printf("\33[0m"); \
} while (0)

#define CALL_FAILURE_GOTO(func, lable) do{ \
    int ret = func; \
    if (ret != 0) { \
        HDMI_LOG_ERR("call %s failed! ret=%x\n", #func, ret); \
        goto lable;\
    } \
} while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

struct test_cmd {
    char *name;
    void (*func)(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc);
    char *prompt;
};

struct fmt_name {
    hi_unf_video_format fmt;
    char *name;
};

struct sample_context {
    hi_svr_dispmng_hdmitx_id current_hdmi;
    hi_svr_dispmng_display_id disp_id[HI_SVR_DISPMNG_HDMITX_ID_MAX];
    struct {
        hi_unf_snd sound;
        hi_unf_snd_out_port port;
    } sound[HI_SVR_DISPMNG_HDMITX_ID_MAX];
};

#define bool2name(x) (((x) == HI_FALSE) ? "false" : "true")

int string2i(const char *string);
int fmt2index(hi_unf_video_format fmt);
char *fmt2name(hi_unf_video_format fmt);
char *vic2fmt_name(unsigned int vic);
char *colorspace2name(hi_svr_dispmng_pixel_format colorspace);
char *deepcolor2name(hi_svr_dispmng_pixel_bit_depth deepcolor);
char *hotplug2name(hi_unf_hdmitx_hotplug_status hotplug);
char *rsen2name(hi_unf_hdmitx_rsen_status resen);
char *colorcap2name(unsigned int colorcap);
hi_unf_video_format name2fmt(const char *name);
hi_svr_dispmng_pixel_format name2colorspace(const char *name);
hi_svr_dispmng_pixel_bit_depth name2deepcolor(const char *name);
struct sample_context *get_ctx(void);
hi_bool check_hdmi_id(hi_svr_dispmng_hdmitx_id hdmi_id);
hi_bool check_disp_id(hi_svr_dispmng_display_id disp_id);
void show_hdmi_status(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_status *status);
void show_hdcp_status(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_hdcp_status *status);
void show_hdmi_avail_cap(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_avail_cap *cap);
void show_hdmi_avail_mods(hi_svr_dispmng_hdmitx_id hdmi_id, const hi_unf_hdmitx_avail_mode *mode,
    unsigned int mode_cnt);
void show_all_fmt_and_cap(hi_svr_dispmng_hdmitx_id hdmi_id);
void show_sink_base_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_color_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_latency_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_display_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_speaker_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_audio_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_dolby_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_hdr_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
void show_sink_vrr_info(hi_svr_dispmng_hdmitx_id hdmi_id, hi_unf_hdmitx_sink_info *info);
hi_s32 sample_disp_open(hi_u32 id);
hi_void sample_disp_close(hi_u32 id);
hi_s32 sample_disp_init(hi_void);
hi_s32 sample_disp_deinit(hi_void);
hi_s32 sample_snd_open(hi_u32 id);
hi_s32 sample_snd_close(hi_u32 id);
hi_s32 sample_snd_init(hi_void);
hi_s32 sample_snd_deinit(hi_void);

/* inplement in sample_hdmi_tsplay.c */
hi_s32 change_media_file(hi_u32 id, const hi_char *file);

#endif

