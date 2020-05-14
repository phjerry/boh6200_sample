/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: hdmi test command.
 * Author: Hisilicon hdmi software group
 * Create: 2019-08-22
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <ctype.h>

#include "hi_errno.h"
#include "securec.h"
#include "sample_hdmi_common.h"
#include "hdmi_test_cmd.h"

#define BASE_HEX 16
#define CEC_MSG_STR_MAX_SZ 100

static void select_hdmi(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    struct sample_context *ctx;
    int id;

    ctx = get_ctx();

    if (argc != 1) {
        printf("\33[31mUsage: hdmi hdmi_id\33[0m\n");
        printf("\33[33mExample: hdmi 1\33[0m\n");
        printf("\33[32mcurrent is hdmi%d\33[0m\n", ctx->current_hdmi);
        return;
    }

    id = string2i(argv[0]);
    if (!check_hdmi_id(id)) {
        printf("\33[31minvalid hdmi(%d), usage: 0-hdmi0, 1-hdmi1 \33[0m\n", id);
        return;
    }

    printf("change to hdmi%d(last time is hdmi%d)\n", id, ctx->current_hdmi);
    ctx->current_hdmi = id;

    return;
}

static void set_timing(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int ret;
    hi_bool error = HI_FALSE;
    hi_unf_video_format fmt = HI_UNF_VIDEO_FMT_MAX;
    char *fmt_name = NULL;
    struct sample_context *ctx;
    hi_svr_dispmng_pixel_format pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
    hi_svr_dispmng_pixel_bit_depth pixel_width = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
    hi_svr_dispmng_display_mode mode = {};

    ctx = get_ctx();

    if (argc != 3) {
        printf("\33[31mUsage: timing format colorspace deepcolor\33[0m\n");
        printf("\33[31mUsage: timing h to get help\33[0m\n");
        printf("\33[33mExample: timing h\33[0m\n");
        printf("\33[33mExample: timing 71 0 0\33[0m\n");
        printf("\33[33mExample: timing 3840x2160p60 yuv444 8bit\33[0m\n");
        error = HI_TRUE;

        if ((argc == 1) && !strcmp(argv[0], "h")) {
            printf("\n\33[36mcolorspace\33[0m: 0(auto), 1(rgb444), 2(yuv422), 3(yuv444), 4(yuv420)\n");
            printf("\33[36mdeepcolor \33[0m: 0(auto), 1(8bit), 2(10bit), 3(12bit)\n");
            printf("\33[36mformat:\33[0m\n");
            show_all_fmt_and_cap(ctx->current_hdmi);
            return;
        }
    }

    if (argc == 0) {
        hi_svr_dispmng_get_display_mode(ctx->disp_id[ctx->current_hdmi], &mode);
        printf("\33[32mcurrent mode is (%s %s %s)\33[0m\n", fmt2name(mode.format), colorspace2name(mode.pixel_format),
            deepcolor2name(mode.bit_depth));
        return;
    }

    if (argc >= 1) {
        fmt = string2i(argv[0]);
        /* number of string */
        if (fmt == HI_FAILURE) {
            fmt_name = (char *)argv[0];
            fmt = name2fmt(fmt_name);
            if (fmt == HI_UNF_VIDEO_FMT_MAX) {
                error = HI_TRUE;
                printf("\33[31mformat(%s) not support, list all support format:\33[0m\n", fmt_name);
                show_all_fmt_and_cap(ctx->current_hdmi);
            }
        } else {
            fmt_name = fmt2name(fmt);
            if (fmt_name == NULL) {
                error = HI_TRUE;
                printf("\33[31mformat(%d) not support, list all support format:\33[0m\n", fmt);
                show_all_fmt_and_cap(ctx->current_hdmi);
            }
        }
    }

    if (argc >= 2) {
        if (isdigit(argv[1][0])) {
            pixel_format = string2i(argv[1]);
            if ((pixel_format < HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO) ||
                (pixel_format >= HI_SVR_DISPMNG_PIXEL_FORMAT_MAX)) {
                error = HI_TRUE;
                printf("\33[31minvaild colorspace(%d), usage: 0(auto), 1(rgb444), 2(yuv422), 3(yuv444), 4(yuv420)\33[0m\n",
                    pixel_format);
            }
        } else {
            pixel_format = name2colorspace(argv[1]);
            if (pixel_format == HI_SVR_DISPMNG_PIXEL_FORMAT_MAX) {
                error = HI_TRUE;
                printf("\33[31minvaild colorspace(%s), usage: 0(auto), 1(rgb444), 2(yuv422), 3(yuv444), 4(yuv420)\33[0m\n",
                    argv[1]);
            }
        }
    }

    if (argc >= 3) {
        pixel_width = string2i(argv[2]);
        if (pixel_width == HI_FAILURE) {
            pixel_width = name2deepcolor(argv[2]);
            if (pixel_width == HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_MAX) {
                error = HI_TRUE;
                printf("\33[31minvaild deepcolor(%s), usage: 0(auto), 1(8bit), 2(10bit), 3(12bit)\33[0m\n",
                    argv[2]);
            }
        } else {
            if ((pixel_width < HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO) ||
                (pixel_width >= HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_MAX)) {
                error = HI_TRUE;
                printf("\33[31minvaild deepcolor(%d), usage: 0(auto), 1(8bit), 2(10bit), 3(12bit)\33[0m\n",
                    pixel_width);
            }
        }
    }

    if (error) {
        return;
    }

    /*
     * HI_SVR_DISPMNG_DISPLAY_MODE_ADVANCED is just for test. When you write an appliction, you must use
     * hi_unf_dispmng_get_available_modes() to get the available modes and only set a mode that is available.
     */
    mode.vic = HI_SVR_DISPMNG_DISPLAY_MODE_ADVANCED;
    mode.format = fmt;
    mode.pixel_format = pixel_format;
    mode.bit_depth  = pixel_width;
    ret = hi_svr_dispmng_set_display_mode(ctx->disp_id[ctx->current_hdmi], &mode);
    if (ret != HI_SUCCESS) {
        printf("\33[31mset hdmi%d timing(%s %s %s) fail\33[0m\n", ctx->current_hdmi,
            fmt_name, colorspace2name(pixel_format), deepcolor2name(pixel_width));
        return;
    }

    printf("set hdmi%d timing(%s %s %s) success\n",  ctx->current_hdmi,
        fmt_name, colorspace2name(pixel_format), deepcolor2name(pixel_width));
    return;
}

static void get_status(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int index;
    hi_unf_hdmitx_status hdmi_status;
    struct sample_context *ctx;

    ctx = get_ctx();

    index = hi_unf_hdmitx_get_status(ctx->current_hdmi, &hdmi_status);
    if (index != HI_SUCCESS) {
        HDMI_LOG_ERR("call HI_UNF_HDMI_GetStatus(hdmi%d) failed!\n", ctx->current_hdmi);
        return;
    }
    show_hdmi_status(ctx->current_hdmi, &hdmi_status);

    return;
}

static void get_hdmi_cap(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int ret;
    hi_unf_hdmitx_avail_cap avail_cap;
    hi_unf_hdmitx_avail_mode *avail_modes;
    hi_u32 mode_cnt;
    struct sample_context *ctx;

    ctx = get_ctx();

    ret = hi_unf_hdmitx_get_avail_capability(ctx->current_hdmi, &avail_cap);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call HI_UNF_HDMI_GetAvailCapability(hdmi%d) failed!\n", ctx->current_hdmi);
        return;
    }
    show_hdmi_avail_cap(ctx->current_hdmi, &avail_cap);

    avail_modes = (hi_unf_hdmitx_avail_mode *)malloc(avail_cap.mode_cnt * sizeof(hi_unf_hdmitx_avail_mode));
    if (avail_modes == NULL) {
        HDMI_LOG_ERR("malloc avail_modes failed!\n");
        return;
    }

    ret = hi_unf_hdmitx_get_avail_modes(ctx->current_hdmi, avail_modes, avail_cap.mode_cnt, &mode_cnt);
    if (ret != HI_SUCCESS) {
        free(avail_modes);
        HDMI_LOG_ERR("call HI_UNF_HDMI_GetAvailModes(hdmi%d) failed!\n", ctx->current_hdmi);
        return;
    }
    show_hdmi_avail_mods(ctx->current_hdmi, avail_modes, mode_cnt);

    free(avail_modes);
    return;
}

#define LINE_LEN 16
static void get_edid(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int ret;
    int i;
    unsigned char *edid;
    unsigned int len;
    hi_bool force = HI_FALSE;
    struct sample_context *ctx;

    ctx = get_ctx();

    if (argc == 1) {
        if (!strcmp(argv[0], "-f")) {
            force = HI_TRUE;
            printf("\33[33mforce to read EDID from sink\33[0m\n");
        } else {
            printf("\33[31mUsage: edid [-f]\33[0m\n");
            return;
        }
    }

    edid = (unsigned char *)malloc(RAW_EDID_MAX_LEN);
    if (edid == NULL) {
        HDMI_LOG_ERR("malloc edid failed!\n");
        return;
    }
    ret = hi_unf_hdmitx_read_edid(ctx->current_hdmi, edid, RAW_EDID_MAX_LEN, &len, force);
    if (ret != HI_SUCCESS) {
        free(edid);
        HDMI_LOG_ERR("call HI_UNF_HDMI_ReadEdid(hdmi%d) failed!\n", ctx->current_hdmi);
        return;
    }

    printf("\33[36m\n---------------------- RAW EDID ----------------------\33[0m");
    for (i = 0; i < len; i++) {
        if ((i % LINE_LEN) == 0) {
            printf("\n%04x: ",i);
        }
        printf(" %02X", edid[i]);
    }
    printf("\33[36m\n------------------------ END -------------------------\33[0m\n");

    free(edid);
    return;
}

static void get_sink_info(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int ret;
    hi_unf_hdmitx_sink_info sink_info;
    struct sample_context *ctx;

    ctx = get_ctx();

    ret = hi_unf_hdmitx_get_sink_info(ctx->current_hdmi, &sink_info);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call HI_UNF_HDMI_GetSinkInfo(hdmi%d) failed!\n", ctx->current_hdmi);
        return;
    }

    if (argc == 1) {
        if (!strcmp(argv[0], "base")) {
            show_sink_base_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "color")) {
            show_sink_color_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "latency")) {
            show_sink_latency_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "display")) {
            show_sink_display_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "speaker")) {
            show_sink_speaker_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "audio")) {
            show_sink_audio_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "dolby")) {
            show_sink_dolby_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "hdr")) {
            show_sink_hdr_info(ctx->current_hdmi, &sink_info);
        } else if (!strcmp(argv[0], "vrr")) {
            show_sink_vrr_info(ctx->current_hdmi, &sink_info);
        } else {
            printf("\33[31mUsage: sinkinfo [base|color|latency|display|speaker|audio|dolby|hdr|vrr]\33[0m\n");
        }
        return;
    }
    show_sink_base_info(ctx->current_hdmi, &sink_info);
    show_sink_color_info(ctx->current_hdmi, &sink_info);
    show_sink_latency_info(ctx->current_hdmi, &sink_info);
    show_sink_display_info(ctx->current_hdmi, &sink_info);
    show_sink_speaker_info(ctx->current_hdmi, &sink_info);
    show_sink_audio_info(ctx->current_hdmi, &sink_info);
    show_sink_dolby_info(ctx->current_hdmi, &sink_info);
    show_sink_hdr_info(ctx->current_hdmi, &sink_info);
    show_sink_vrr_info(ctx->current_hdmi, &sink_info);
}

static void change_ts_file(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    struct sample_context *ctx;

    ctx = get_ctx();

    if (argc != 1) {
        printf("\33[31mUsage: ts file\33[0m\n");
        printf("\33[33mExample: ts /tmp/test_stream/8K/Sword_AVS3_AAC_8KP120_HDR.ts\33[0m\n");
        return;
    }
    change_media_file(ctx->current_hdmi, argv[0]);
}

static void change_audio_mode(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int ret;
    hi_unf_snd_output_mode mode;
    struct sample_context *ctx;

    ctx = get_ctx();

    if (argc != 1) {
        printf("\33[31mUsage: audio [pcm|raw|lbr|auto]\33[0m\n");
        printf("\33[33mExample: audio raw\33[0m\n");
        return;
    }

    if (!strcmp(argv[0], "pcm")) {
        mode  = HI_UNF_SND_OUTPUT_MODE_LPCM;
    } else if (!strcmp(argv[0], "raw")) {
        mode  = HI_UNF_SND_OUTPUT_MODE_RAW;
    } else if (!strcmp(argv[0], "lbr")) {
        mode  = HI_UNF_SND_OUTPUT_MODE_HBR2LBR;
    } else if (!strcmp(argv[0], "auto")) {
        mode  = HI_UNF_SND_OUTPUT_MODE_AUTO;
    } else {
        printf("\33[31mUsage: audio [pcm|raw|lbr|auto]\33[0m\n");
        printf("\33[33mExample: audio raw\33[0m\n");
        return;
    }

    ret = hi_unf_snd_set_output_mode(ctx->sound[ctx->current_hdmi].sound,
        ctx->sound[ctx->current_hdmi].port, mode);
    if (ret != HI_SUCCESS) {
        printf("\33[31mset %s failed!\33[0m\n", argv[0]);
        return;
    }

    printf("set %s success\n", argv[0]);
}

static hi_void cec_printf_status(const hi_unf_hdmitx_cec_status *status)
{
    printf("logic addr :%x\n", status->logic_addr);
    printf("physic addr:%x\n", status->physic_addr);
    printf("addr status:%d\n", status->addr_status);
}

void cec_printf_msg(const hi_unf_cec_msg *msg)
{
    int i;

    printf("%02x", (msg->src_addr << 4) | msg->dst_addr); /* bit[7:4] is initiator */
    printf(" %02x", msg->opcode);
    for (i = 0; i < msg->operand.len; i++) {
        printf(" %02x", msg->operand.data[i]);
    }
}

static hi_void cec_event_callback(hi_unf_hdmitx_id id, hi_unf_hdmitx_cec_event_type type,
    const hi_unf_hdmitx_cec_event_data *data, hi_void *private_data)
{
    if (type == HI_UNF_HDMITX_CEC_EVENT_STATUS_CHANGE) {
        HDMI_LOG_INFO("cec%d-status change \n", id);
        cec_printf_status(&data->cec_status);
    } else {
        HDMI_LOG_INFO("cec%d-rx msg:", id);
        cec_printf_msg(&data->rx_msg);
        printf("\n");
    }
}

static hi_void cec_enable(hi_unf_hdmitx_id id, hi_bool enable)
{
    int ret;
    hi_unf_hdmitx_cec_callback callback;

    if (enable) {
        ret = hi_unf_hdmitx_cec_enable(id, HI_TRUE);
        if (ret != HI_SUCCESS) {
            HDMI_LOG_ERR("cec%d enable failed(ret=0x%x)!\n", id, ret);
        }
        callback.func  = cec_event_callback;
        ret = hi_unf_hdmitx_cec_register_callback(id, &callback);
        if (ret != HI_SUCCESS) {
            HDMI_LOG_ERR("cec%d regist callback failed(ret=0x%x)!\n", id, ret);
            return;
        }
    } else {
        hi_unf_hdmitx_cec_unregister_callback(id);
        ret = hi_unf_hdmitx_cec_enable(id, HI_FALSE);
        if (ret != HI_SUCCESS) {
            HDMI_LOG_ERR("cec%d disable failed(ret=0x%x)!\n", id, ret);
            return;
        }
    }
}

static hi_s32 cec_send_msg(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    struct sample_context *ctx = HI_NULL;
    hi_unf_cec_msg msg = {};
    int i;
    int val;
    int ret;

    ctx = get_ctx();

    /*
     * the length of command "send" + header is 2,  the max argc = "send" + header + opcode + operand
     * = 3 + operand
     */
    if ((argc < 2) || (argc > HI_UNF_CEC_MAX_OPERAND_SIZE + 3)) {
        goto err_usage;
    }

    val = strtol(argv[1], NULL, BASE_HEX);
    msg.src_addr = val >> 4; /* bit[7:4] is initiator */
    msg.dst_addr = val & 0xf;
    if (argc >= 3) { /* argc = "send" + heaedr + opcode + operand = 3 + operand */
        val = strtol(argv[2], NULL, BASE_HEX);
        msg.opcode = val;
        msg.operand.len = argc - 3; /* argc = "send" + heaedr + opcode + operand = 3 + operand */
        for (i = 0; i < msg.operand.len; i++) {
            val = strtol(argv[i + 3], NULL, BASE_HEX); /* argc = "cec send" + heaedr + opcode + operand = 3 + operand */
            msg.operand.data[i] = val;
        }
    }
    ret = hi_unf_hdmitx_cec_send_msg(ctx->current_hdmi, &msg);
    if (ret == HI_SUCCESS) {
        HDMI_LOG_INFO("cec%d send msg: ", ctx->current_hdmi);
        cec_printf_msg(&msg);
        printf("(success)\n");
    } else if (ret == HI_ERR_HDMITX_MSG_NACK) {
        HDMI_LOG_INFO("cec%d send msg: ", ctx->current_hdmi);
        cec_printf_msg(&msg);
        printf("(nack)\n");
    } else if (ret == HI_ERR_HDMITX_MSG_FAILED) {
        HDMI_LOG_INFO("cec%d send msg: ", ctx->current_hdmi);
        cec_printf_msg(&msg);
        printf("(failed)\n");
    } else {
        HDMI_LOG_INFO("cec%d send msg: ", ctx->current_hdmi);
        cec_printf_msg(&msg);
        printf("(ret=0x%x)\n", ret);
    }

    return ret;

err_usage:
    printf("\33[31mUsage: cec send data0~data15\33[0m\n");
    printf("\33[33mExample: cec send 4f 84 10 00 04\33[0m\n");
    return HI_FAILURE;
}

static void cec_cmd(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int ret;
    struct sample_context *ctx = HI_NULL;
    hi_unf_hdmitx_cec_status status;

    ctx = get_ctx();

    if (argc < 1) {
        goto err_usage;
    }

    if (!strcmp(argv[0], "en")) {
        cec_enable(ctx->current_hdmi, HI_TRUE);
    } else if (!strcmp(argv[0], "dis")) {
        cec_enable(ctx->current_hdmi, HI_FALSE);
    } else if (!strcmp(argv[0], "dev")) {
        if (argc != 2) {
            printf("\33[31mUsage: cec dev [3-tuner|4-playback]\33[0m\n");
            return;
        }
        ret = hi_unf_hdmitx_cec_set_device_type(ctx->current_hdmi, string2i(argv[1]));
        if (ret != HI_SUCCESS) {
            HDMI_LOG_ERR("cec%d set device type failed(ret=0x%x)!\n", ctx->current_hdmi, ret);
            return;
        }
    } else if (!strcmp(argv[0], "send")) {
        cec_send_msg(argv, argc);
    } else if (!strcmp(argv[0], "status")) {
        ret = hi_unf_hdmitx_cec_get_status(ctx->current_hdmi, &status);
        if (ret != HI_SUCCESS) {
            HDMI_LOG_ERR("cec%d get status failed(ret=0x%x)!\n", ctx->current_hdmi, ret);
            return;
        }
        cec_printf_status(&status);
    } else {
        goto err_usage;
    }
    return;

err_usage:
    printf("\33[31mUsage: cec [command]\33[0m\n");
    printf("[command]: en\t--enable cec.\n"
           "           dis\t--disable cec.\n"
           "           dev\t--set device type(3-tuner, 4-playback).\n"
           "           send\t--send message(data0~data15)\n"
           "           status\t--show cec status.\n");
    printf("\33[33mExample: cec en\33[0m\n");
}

static hi_void hdcp_start_auth(hi_unf_hdmitx_id id, hi_u8 hdcp_mode)
{
    int ret;

    if (hdcp_mode > HI_UNF_HDMITX_HDCP_MODE_23) {
        HDMI_LOG_ERR("hdcp%d mode%d is error!\n", id, hdcp_mode);
        goto err_help;
    }

    switch (hdcp_mode) {
        case 0: /* user input cmd 0 */
            hdcp_mode = HI_UNF_HDMITX_HDCP_MODE_AUTO;
            break;
        case 1: /* user input cmd 1 */
            hdcp_mode = HI_UNF_HDMITX_HDCP_MODE_14;
            break;
        case 2: /* user input cmd 2 */
            hdcp_mode = HI_UNF_HDMITX_HDCP_MODE_22;
            break;
        case 3: /* user input cmd 3 */
            hdcp_mode = HI_UNF_HDMITX_HDCP_MODE_23;
            break;
        default:
            hdcp_mode = HI_UNF_HDMITX_HDCP_MODE_AUTO;
            break;
    }

    ret = hi_unf_hdmitx_hdcp_start_auth(id, hdcp_mode);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("hdcp%d start auth failed(ret=0x%x)!\n", id, ret);
        return;
    }

    return;

err_help:
    printf("[Usage  ]: hdcp 1 [mode]\n");
    printf("[command]: 0\t--start hdcp auth mode auto.\n"
           "           1\t--start hdcp auth mode 1.4.\n"
           "           2\t--start hdcp auth mode 2.2.\n"
           "           3\t--start hdcp auth mode 2.3.\n");
    printf("[example]: hdcp 1 1\t\n");
    return;
}

static hi_void hdcp_stop_auth(hi_unf_hdmitx_id id)
{
    int ret;

    ret = hi_unf_hdmitx_hdcp_stop_auth(id);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("hdcp%d stop auth failed(ret=0x%x)!\n", id, ret);
        return;
    }

    return;
}

static void hdcp_get_sink_caps(hi_unf_hdmitx_id id, hi_unf_hdmitx_hdcp_cap *hdcp_cap)
{
    int ret;

    ret = hi_unf_hdmitx_hdcp_get_cap(id, hdcp_cap);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_hdmitx_hdcp_get_cap(hdmi%d) failed!\n", id);
        return;
    }

    printf("\33[36mHDCP%d capability:\33[0m\n", id);
    printf("support14 : %s\n", (hdcp_cap->support14 ? "YES" : "NO"));
    printf("support22 : %s\n", (hdcp_cap->support22 ? "YES" : "NO"));
    printf("support23 : %s\n", (hdcp_cap->support23 ? "YES" : "NO"));

    return;
}

static void hdcp_get_status(hi_unf_hdmitx_id id, hi_unf_hdmitx_hdcp_status *hdcp_status)
{
    int ret;

    ret = hi_unf_hdmitx_hdcp_get_status(id, hdcp_status);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("call hi_unf_hdmitx_hdcp_get_status(hdmi%d) failed!\n", id);
        return;
    }
    show_hdcp_status(id, hdcp_status);

    return;
}

static hi_void hdcp_set_auth_times(hi_unf_hdmitx_id id, hi_u32 auth_times)
{
    hi_u32 ret;

    ret = hi_unf_hdmitx_hdcp_set_auth_times(id, auth_times);
    if (ret != HI_SUCCESS) {
        HDMI_LOG_ERR("hdcp%d set auth times failed(ret=0x%x)!\n", id, ret);
        return;
    }

    return;
}

static void hdcp_cmd(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    struct sample_context *ctx = HI_NULL;
    hi_unf_hdmitx_hdcp_status hdcp_status;
    hi_unf_hdmitx_hdcp_cap hdcp_cap;

    ctx = get_ctx();

    if (argc < 1) {
        goto err_usage;
    }

    if (!strcmp(argv[0], "0")) {
        hdcp_stop_auth(ctx->current_hdmi);
    } else if (!strcmp(argv[0], "1")) {
        hdcp_start_auth(ctx->current_hdmi, string2i(argv[1]));
    } else if (!strcmp(argv[0], "2")) {
        hdcp_set_auth_times(ctx->current_hdmi, string2i(argv[1]));
    } else if (!strcmp(argv[0], "3")) {
        hdcp_get_status(ctx->current_hdmi, &hdcp_status);
    } else if (!strcmp(argv[0], "4")) {
        hdcp_get_sink_caps(ctx->current_hdmi, &hdcp_cap);
    } else {
        goto err_usage;
    }

    return;

err_usage:
    printf("[Usage  ]: hdcp argv1 [argv2]\n");
    printf("[argv1  ]: 0\t--stop hdcp auth.\n"
           "           1\t--start hdcp auth;[argv2]:0--auto mode, 1--hdcp1.4, 2:hdcp2.2, 3:hdcp2.3.\n"
           "           2\t--set hdcp auth times.\n"
           "           3\t--get hdcp status.\n"
           "           4\t--get sink's HDCP capability.\n");
    printf("[Example]: hdcp 0\n");
    printf("[Example]: hdcp 1 1 --start hdcp auth mode 1.4\n");
    printf("[Example]: hdcp 2 1 --set hdcp reauth 1 times\n");
    return;
}

static void help(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc);
static const struct test_cmd g_hdmitx_test_cmd[] = {
    { "h",          help,                "List all command" },
    { "q",          NULL,                "Exit sample" },
    { "hdmi",       select_hdmi,         "Select the hdmi port to configure" },
    { "timing",     set_timing,          "Change hdmi output timing" },
    { "audio",      change_audio_mode,   "Change the auido output mode" },
    { "ts",         change_ts_file,      "Change the TS file" },
    { "status",     get_status,          "Display hdmi status" },
    { "hdmicap",    get_hdmi_cap,        "Display hdmi available capability and modes" },
    { "edid",       get_edid,            "Display sink EDID" },
    { "sinkinfo",   get_sink_info,       "Display sink information" },
    { "cec",        cec_cmd,             "CEC test command" },
    { "hdcp",       hdcp_cmd,            "HDCP test command" },
};

static void help(const char (*argv)[HDMI_MAX_PAMRM_SIZE], const int argc)
{
    int i;

    printf("\33[33mList all test command support\33[0m\n");
    for (i = 0; i < ARRAY_SIZE(g_hdmitx_test_cmd); i++) {
        if (g_hdmitx_test_cmd[i].name) {
            printf("\33[33m%-16s\33[0m", g_hdmitx_test_cmd[i].name);
            printf("--%s\n", g_hdmitx_test_cmd[i].prompt);
        }
    }

    return;
}

static unsigned char split_cmd(const char *cmd_str, hi_u8 len, char (*args)[HDMI_MAX_PAMRM_SIZE])
{
    unsigned char i;
    char tmp_str[HDMI_INPUT_CMD_MAX] = {};
    char *cmd;

    if (len > HDMI_INPUT_CMD_MAX) {
        return 0;
    }

    memcpy(tmp_str, cmd_str, len);
    cmd = tmp_str;

    /* Search the first charater char */
    while (*cmd == ' ' && *cmd++ != '\0') {
        ;
    }

    for (i = strlen(cmd); i > 0; i--) {
        if (*(cmd + i - 1) == 0x0a || *(cmd + i - 1) == ' ') {
            *(cmd + i - 1) = '\0';
        } else {
            break;
        }
    }

    for (i = 0; i < HDMI_MAX_ARGC; i++) {
        memset(args[i], 0, HDMI_MAX_PAMRM_SIZE);
    }
    /* fill args[][] with input string */
    for (i = 0; i < HDMI_MAX_ARGC; i++) {
        hi_u8 j = 0;
        while (*cmd == ' ' && *cmd++ != '\0') {}

        while ((*cmd != ' ') && (*cmd != '\0')) {
            args[i][j++] = *cmd++;
        }

        args[i][j] = '\0';
        if (*cmd == '\0') {
            i++;
            break;
        }
        args[i][j] = '\0';
    }

    return i;
}

int hdmi_test_cmd(const char *cmd, hi_u8 len)
{
    int i;
    unsigned char argc;
    char args[HDMI_MAX_ARGC][HDMI_MAX_PAMRM_SIZE] = {};

    if (!cmd) {
        HDMI_LOG_ERR("cmd = %p\n", cmd);
        return HI_FAILURE;
    }

    memset(args, 0, sizeof(args));

    argc = split_cmd(cmd, len, args);
    if (argc == 0) {
        return HI_FAILURE;
    }

    for (i = 0; i < ARRAY_SIZE(g_hdmitx_test_cmd); i++) {
        if (g_hdmitx_test_cmd[i].name && g_hdmitx_test_cmd[i].func &&
            !strcmp(g_hdmitx_test_cmd[i].name, args[0])) {
            g_hdmitx_test_cmd[i].func(&args[1], argc - 1);
            return HI_SUCCESS;
        }
    }

    printf("\33[31mcommand:%s not found\33[0m\n", args[0]);

    return HI_FAILURE;
}

