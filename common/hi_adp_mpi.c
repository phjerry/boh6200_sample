#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "securec.h"
#include "hi_unf_demux.h"
#include "hi_unf_avplay.h"
#include "hi_unf_win.h"
#include "hi_unf_sound.h"
#include "hi_unf_ai.h"
#include "hi_unf_disp.h"
#include "hi_unf_hdmitx.h"
#include "hi_unf_acodec.h"
#include "hi_errno.h"
//#include "hi_unf_pdm.h"
#include "hi_svr_dispmng.h"
#include "hi_unf_video.h"

#include "hi_adp_mpi.h"
#include "hi_adp.h"
#include "hi_adp_data.h"
#include "hi_adp_boardcfg.h"

#include "hi_unf_acodec_g711.h"
#include "hi_unf_acodec_mp3dec.h"
#include "hi_unf_acodec_mp2dec.h"
#include "hi_unf_acodec_aacdec.h"
#include "hi_unf_acodec_pcmdec.h"
#include "hi_unf_acodec_amrnb.h"
#include "hi_unf_acodec_amrwb.h"
#include "hi_unf_acodec_truehdpassthrough.h"
#include "hi_unf_acodec_dolbytruehddec.h"
#include "hi_unf_acodec_dtshddec.h"
#if defined (DOLBYPLUS_HACODEC_SUPPORT)
 #include "hi_unf_acodec_dolbyplusdec.h"
#endif
#include "hi_unf_acodec_ac3passthrough.h"
#include "hi_unf_acodec_dtspassthrough.h"
#include "hi_unf_acodec_aacenc.h"
#include "hi_unf_acodec_dolbyms12dec.h"
#include "hi_unf_acodec_voice.h"
#include "hi_unf_acodec_opus.h"
#include "hi_unf_acodec_vorbis.h"
#include "hi_unf_acodec_dtsm6dec.h"
#include "hi_unf_acodec_dradec.h"

#ifdef ANDROID
#include <utils/Log.h>
#endif

#define MPI_DEMUX_NUM 5
#define MPI_DEMUX_PLAY 0
#define MPI_DEMUX_REC_0 1
#define MPI_DEMUX_REC_1 2
#define MPI_DEMUX_TIMETHIFT 3
#define MPI_DEMUX_PLAYBACK 4
#define HI_ADP_UHD_WIDTH   3840
#define HI_ADP_UHD_HEIGHT  2160
#define HI_ADP_HD_WIDTH    1280
#define HI_ADP_HD_HEIGHT    720

/*
big-endian pcm output format, if extword is 1, choose normal pcm decoder,
                                            if extword is 2, choose wifidsp_lpcm decoder(Frame Header:0xA0,0x06)
                                            if others, fail to decode.
*/
#define NORMAL_PCM_EXTWORD    1
#define WIFIDSP_LPCM_EXTWORD  2


hi_u8 dec_open_buf[1024];
hi_u8 enc_open_buf[1024];

#if defined (DOLBYPLUS_HACODEC_SUPPORT)

hi_unf_acodec_dolbyplus_stream_info g_ddp_stream_info;

/*dolby Dual Mono type control*/
hi_u32  g_dolby_acmod = 0;
hi_bool g_draw_chn_bar = HI_TRUE;

#endif

#ifdef ANDROID

#define LOG_MAX_LEN 1024

void LogPrint(const char *format, ...)
{
    char LogStr[LOG_MAX_LEN];
    va_list args = {0};

    va_start(args, format);
    vsnprintf(LogStr, LOG_MAX_LEN, format, args);
    va_end(args);
    android_printLog(ANDROID_LOG_ERROR, "SAMPLE", "%s", LogStr);
}
#endif

void stdin_constructor(void) __attribute__ ((constructor));
void stdin_constructor(void)
{
    hi_s32 flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags < 0) {
        printf("[%s:%d][ERROR] get flags failed.\n", __FUNCTION__, __LINE__);
    } else if (flags & O_NONBLOCK){
        flags &= ~O_NONBLOCK;
        flags = fcntl(STDIN_FILENO, F_SETFL, flags);
        if (flags < 0) {
            printf("[%s:%d][ERROR] set flags failed.\n", __FUNCTION__, __LINE__ );
        }
    }
}

/************************************DISPLAY Common Interface*******************************/
hi_unf_video_format hi_adp_str2fmt(const hi_char* format_str)
{
    hi_unf_video_format format = HI_UNF_VIDEO_FMT_MAX;

    if (format_str == NULL) {
        return HI_UNF_VIDEO_FMT_MAX;
    }

    if (0 == strcasecmp(format_str, "1080P60")) {
        format = HI_UNF_VIDEO_FMT_1080P_60;
    } else if (0 == strcasecmp(format_str, "1080P50")) {
        format = HI_UNF_VIDEO_FMT_1080P_50;
    } else if (0 == strcasecmp(format_str, "1080P30")) {
        format = HI_UNF_VIDEO_FMT_1080P_30;
    } else if (0 == strcasecmp(format_str, "1080P25")) {
        format = HI_UNF_VIDEO_FMT_1080P_25;
    } else if (0 == strcasecmp(format_str, "1080P24")) {
        format = HI_UNF_VIDEO_FMT_1080P_24;
    } else if (0 == strcasecmp(format_str, "1080i60")) {
        format = HI_UNF_VIDEO_FMT_1080I_60;
    } else if (0 == strcasecmp(format_str, "1080i50")) {
        format = HI_UNF_VIDEO_FMT_1080I_50;
    } else if (0 == strcasecmp(format_str, "720P60")) {
        format = HI_UNF_VIDEO_FMT_720P_60;
    } else if (0 == strcasecmp(format_str, "720P50")) {
        format = HI_UNF_VIDEO_FMT_720P_50;
    } else if (0 == strcasecmp(format_str, "576P50")) {
        format = HI_UNF_VIDEO_FMT_576P_50;
    } else if (0 == strcasecmp(format_str, "480P60")) {
        format = HI_UNF_VIDEO_FMT_480P_60;
    } else if (0 == strcasecmp(format_str, "PAL")) {
        format = HI_UNF_VIDEO_FMT_PAL;
    } else if (0 == strcasecmp(format_str, "NTSC")) {
        format = HI_UNF_VIDEO_FMT_NTSC;
    } else if (0 == strcasecmp(format_str, "1080P24_FP")) {
        format = HI_UNF_VIDEO_FMT_1080P_24_FRAME_PACKING;
    } else if (0 == strcasecmp(format_str, "720P60_FP")) {
        format = HI_UNF_VIDEO_FMT_720P_60_FRAME_PACKING;
    } else if (0 == strcasecmp(format_str, "720P50_FP")) {
        format = HI_UNF_VIDEO_FMT_720P_50_FRAME_PACKING;
    } else if (0 == strcasecmp(format_str, "2160P24")) {
        format = HI_UNF_VIDEO_FMT_3840X2160_24;
    } else if (0 == strcasecmp(format_str, "2160P25")) {
        format = HI_UNF_VIDEO_FMT_3840X2160_25;
    } else if (0 == strcasecmp(format_str, "2160P30")) {
        format = HI_UNF_VIDEO_FMT_3840X2160_30;
    } else if (0 == strcasecmp(format_str, "2160P50")) {
        format = HI_UNF_VIDEO_FMT_3840X2160_50;
    } else if (0 == strcasecmp(format_str, "2160P60")) {
        format = HI_UNF_VIDEO_FMT_3840X2160_60;
    } else if (0 == strcasecmp(format_str, "4320P30")) {
        format = HI_UNF_VIDEO_FMT_7680X4320_30;
    } else if (0 == strcasecmp(format_str, "4320P50")) {
        format = HI_UNF_VIDEO_FMT_7680X4320_50;
    } else if (0 == strcasecmp(format_str, "4320P60")) {
        format = HI_UNF_VIDEO_FMT_7680X4320_60;
    } else if (0 == strcasecmp(format_str, "2160P120")) {
        format = HI_UNF_VIDEO_FMT_3840X2160_120;
    } else if (0 == strcasecmp(format_str, "4320P120")) {
        format = HI_UNF_VIDEO_FMT_7680X4320_120;
    } else {
        format = HI_UNF_VIDEO_FMT_720P_50;
        SAMPLE_COMMON_PRINTF("\n!!! Can NOT match format, set format to is '720P50'/%d.\n\n",
            HI_UNF_VIDEO_FMT_720P_50);
    }

    return format;
}

hi_unf_video_format get_sd_format(const hi_unf_video_format format_hd)
{
    hi_unf_video_format  format_sd = HI_UNF_VIDEO_FMT_PAL;
    switch (format_hd) {
        case HI_UNF_VIDEO_FMT_4096X2160_60:
        case HI_UNF_VIDEO_FMT_4096X2160_30:
        case HI_UNF_VIDEO_FMT_4096X2160_24:
        case HI_UNF_VIDEO_FMT_3840X2160_60:
        case HI_UNF_VIDEO_FMT_3840X2160_30:
        case HI_UNF_VIDEO_FMT_3840X2160_24:
        case HI_UNF_VIDEO_FMT_1080P_60:
        case HI_UNF_VIDEO_FMT_1080P_30:
        case HI_UNF_VIDEO_FMT_1080I_60:
        case HI_UNF_VIDEO_FMT_720P_60:
        case HI_UNF_VIDEO_FMT_480P_60:
        case HI_UNF_VIDEO_FMT_NTSC:
            format_sd = HI_UNF_VIDEO_FMT_NTSC;
            break;

        case HI_UNF_VIDEO_FMT_4096X2160_50:
        case HI_UNF_VIDEO_FMT_4096X2160_25:
        case HI_UNF_VIDEO_FMT_3840X2160_50:
        case HI_UNF_VIDEO_FMT_3840X2160_25:
        case HI_UNF_VIDEO_FMT_1080P_50:
        case HI_UNF_VIDEO_FMT_1080P_25:
        case HI_UNF_VIDEO_FMT_1080I_50:
        case HI_UNF_VIDEO_FMT_720P_50:
        case HI_UNF_VIDEO_FMT_576P_50:
        case HI_UNF_VIDEO_FMT_PAL:
            format_sd = HI_UNF_VIDEO_FMT_PAL;
            break;

        default:
            format_sd = HI_UNF_VIDEO_FMT_PAL;
    }
    return format_sd;
}

hi_s32 hi_adp_disp_set_format(const hi_svr_dispmng_display_id id, const hi_unf_video_format format)
{
    hi_s32 ret;
    hi_svr_dispmng_display_mode mode;

    if (id >= HI_SVR_DISPMNG_DISPLAY_ID_MAX) {
        SAMPLE_COMMON_PRINTF("Invalid display id %d!\n", id);
        return HI_FAILURE;
    }
    if (format >= HI_UNF_VIDEO_FMT_MAX) {
        SAMPLE_COMMON_PRINTF("Invalid format %d!\n", format);
        return HI_FAILURE;
    }

    ret = hi_svr_dispmng_get_vic_from_format(format, &mode.vic);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_get_vic_from_format failed 0x%x\n", ret);
        return ret;
    }
    mode.format = format;
    mode.name[0] = '\0';
    mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
    mode.bit_depth  = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
    ret = hi_svr_dispmng_set_display_mode(id, &mode);
    SAMPLE_COMMON_PRINTF("set display mode (vic:%d, format:%d, pixel_format:%d, pixel_bitwidth:%d).\n",
        mode.vic, mode.format, mode.pixel_format, mode.bit_depth);
    return ret;
}

#ifdef HI_BOOT_HOMOLOGOUS_SUPPORT
hi_s32 hi_adp_disp_init_mutex(const hi_unf_video_format input_format)
{
    hi_s32                         ret;
    hi_svr_dispmng_interface_group attach;
    hi_svr_dispmng_screen_offset   offset;
    hi_unf_video_format            format_sd;

    if (input_format >= HI_UNF_VIDEO_FMT_MAX) {
        SAMPLE_COMMON_PRINTF("Invalid format %d!\n", input_format);
        return HI_FAILURE;
    }

    ret = hi_svr_dispmng_init();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_init failed 0x%x\n", ret);
        return ret;
    }

    attach.intf[0].intf_type = HI_SVR_DISPMNG_INTERFACE_HDMITX;
    attach.intf[0].intf.cvbs_dac = HI_SVR_DISPMNG_HDMITX_ID_0;
    attach.number = 1;
    ret = hi_svr_dispmng_attach_interface(HI_SVR_DISPMNG_DISPLAY_ID_MASTER, &attach)
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_attach_interface failed 0x%x\n", ret);
        hi_svr_dispmng_deinit();
        return ret;
    }

    format_sd = get_sd_format(input_format);
    ret = hi_adp_disp_set_format(HI_SVR_DISPMNG_DISPLAY_ID_SLAVE, format_sd);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_adp_disp_set_format DISPLAY1 0x%x failed 0x%x\n", input_format, ret);
        hi_svr_dispmng_deinit();
        return ret;
    }

    ret = hi_svr_dispmng_set_virtual_screen(HI_SVR_DISPMNG_DISPLAY_ID_MASTER, HI_ADP_HD_WIDTH, HI_ADP_HD_HEIGHT);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_set_virtual_screen failed 0x%x\n", ret);
        hi_svr_dispmng_deinit();
        return ret;
    }

    offset.left      = 0;
    offset.top       = 0;
    offset.right     = 0;
    offset.bottom    = 0;
    ret = hi_svr_dispmng_set_screen_offset(HI_SVR_DISPMNG_DISPLAY_ID_SLAVE, &offset);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_set_screen_offset DISPLAY1 failed 0x%x\n", ret);
        hi_svr_dispmng_deinit();
        return ret;
    }

    ret = HI_UNF_DISP_Open(HI_SVR_DISPMNG_DISPLAY_ID_SLAVE);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("HI_UNF_DISP_Open DISPLAY1 failed 0x%x\n", ret);
        hi_svr_dispmng_deinit();
        return ret;
    }
    return HI_SUCCESS;
}
#endif

hi_s32 hi_adp_disp_init(const hi_unf_video_format format)
{
    hi_s32                       ret;
    hi_svr_dispmng_screen_offset offset;
    hi_svr_dispmng_display_id    id;
    hi_svr_dispmng_display_mode  mode;

    if (format >= HI_UNF_VIDEO_FMT_MAX) {
        SAMPLE_COMMON_PRINTF("Invalid format %d!\n", format);
        return HI_FAILURE;
    }

    ret = hi_svr_dispmng_init();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_init failed 0x%x\n", ret);
        return ret;
    }

    ret = hi_svr_dispmng_get_display_id(HI_SVR_DISPMNG_DISPLAY_INDEX_0, &id);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_get_display_id failed 0x%x.\n", ret);
        return ret;
    }

    ret = hi_svr_dispmng_get_vic_from_format(format, &mode.vic);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_get_vic_from_format failed 0x%x\n", ret);
        return ret;
    }
    mode.format = format;
    mode.name[0] = '\0';
    mode.pixel_format = HI_SVR_DISPMNG_PIXEL_FORMAT_AUTO;
    mode.bit_depth  = HI_SVR_DISPMNG_PIXEL_BIT_DEPTH_AUTO;
    ret = hi_svr_dispmng_set_display_mode(id, &mode);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_set_display_mode failed 0x%x.\n", ret);
        return ret;
    }

#ifdef ANDROID
    ret = hi_svr_dispmng_set_virtual_screen(HI_SVR_DISPMNG_DISPLAY_ID_MASTER, HI_ADP_UHD_WIDTH, HI_ADP_UHD_HEIGHT);
#else
    ret = hi_svr_dispmng_set_virtual_screen(HI_SVR_DISPMNG_DISPLAY_ID_MASTER, HI_ADP_HD_WIDTH, HI_ADP_HD_HEIGHT);
#endif
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_set_virtual_screen failed 0x%x\n", ret);
        hi_svr_dispmng_deinit();
        return ret;
    }

    offset.left      = 0;
    offset.top       = 0;
    offset.right     = 0;
    offset.bottom    = 0;
    ret = hi_svr_dispmng_set_screen_offset(id, (const hi_svr_dispmng_screen_offset *)&offset);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("hi_svr_dispmng_set_screen_offset DISPLAY1 failed 0x%x\n", ret);
        hi_svr_dispmng_deinit();
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 hi_adp_disp_deinit(hi_void)
{
    hi_s32                      ret;

    ret = hi_svr_dispmng_deinit();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_svr_dispmng_deinit failed, ret=%#x.\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

/****************************VO Common Interface********************************************/
hi_s32 hi_adp_win_init(hi_void)
{
    hi_s32             ret;
    ret = hi_unf_win_init();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_win_init failed.\n");
        return ret;
    }
    return HI_SUCCESS;
}

hi_s32 hi_adp_win_create(const hi_unf_video_rect *rect, hi_handle *win)
{
    hi_s32 ret;
    hi_unf_win_attr   win_attr;

    if (win == HI_NULL) {
        SAMPLE_COMMON_PRINTF("Invalid parameter(win==NULL).\n");
        return HI_FAILURE;
    }

    memset_s(&win_attr, sizeof(win_attr), 0, sizeof(hi_unf_win_attr));
    win_attr.disp_id = HI_SVR_DISPMNG_DISPLAY_ID_MASTER;
    win_attr.priority = HI_UNF_WIN_WIN_PRIORITY_AUTO;
    win_attr.is_virtual = HI_FALSE;
    win_attr.asp_convert_mode = HI_UNF_WIN_ASPECT_CONVERT_FULL;
    win_attr.video_rect.x = 0;
    win_attr.video_rect.y = 0;
    win_attr.video_rect.width  = 0;
    win_attr.video_rect.height = 0;

    if (rect == HI_NULL) {
        memset_s(&win_attr.output_rect, sizeof(win_attr.output_rect), 0x0, sizeof(hi_unf_video_rect));
    } else {
        memcpy_s(&win_attr.output_rect, sizeof(win_attr.output_rect), rect, sizeof(hi_unf_video_rect));
    }

    ret = hi_unf_win_create(&win_attr, win);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_win_create failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_win_create_ext(hi_unf_video_rect *rect, hi_handle *win, hi_svr_dispmng_display_id id)
{
    hi_s32 ret;
    hi_unf_win_attr   win_attr;

    if (win == HI_NULL) {
        SAMPLE_COMMON_PRINTF("Invalid parameter(win==NULL).\n");
        return HI_FAILURE;
    }
    if (id >= HI_SVR_DISPMNG_DISPLAY_ID_MAX) {
        SAMPLE_COMMON_PRINTF("Invalid display id.\n");
        return HI_FAILURE;
    }

    memset_s(&win_attr, sizeof(win_attr), 0, sizeof(hi_unf_win_attr));
    win_attr.disp_id = id;
    win_attr.priority = HI_UNF_WIN_WIN_PRIORITY_AUTO;
    win_attr.is_virtual = HI_FALSE;
    win_attr.asp_convert_mode = HI_UNF_WIN_ASPECT_CONVERT_FULL;
    win_attr.video_rect.x = 0;
    win_attr.video_rect.y = 0;
    win_attr.video_rect.width  = 0;
    win_attr.video_rect.height = 0;

    if (rect == HI_NULL) {
        memset_s(&win_attr.output_rect, sizeof(win_attr.output_rect), 0x0, sizeof(hi_unf_video_rect));
    } else {
        memcpy_s(&win_attr.output_rect, sizeof(win_attr.output_rect), rect, sizeof(hi_unf_video_rect));
    }

    ret = hi_unf_win_create(&win_attr, win);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_win_create failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_win_deinit(hi_void)
{
    hi_s32         ret;

    ret = hi_unf_win_deinit();
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_win_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

/*****************************************SOUND common interface************************************/
hi_s32 hi_adp_snd_init(hi_void)
{
    hi_s32                  ret;
    hi_unf_snd_attr       attr;

    ret = hi_unf_snd_init();
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_init failed.\n");
        return ret;
    }
    ret = hi_unf_snd_get_default_open_attr(HI_UNF_SND_0, &attr);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_get_default_open_attr failed.\n");
        return ret;
    }

    ret = hi_unf_snd_open(HI_UNF_SND_0, &attr);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_open failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_snd_deinit(hi_void)
{
    hi_s32                  ret;

    ret = hi_unf_snd_close(HI_UNF_SND_0);
    if (ret != HI_SUCCESS )
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_close failed.\n");
        return ret;
    }

    ret = hi_unf_snd_deinit();
    if (ret != HI_SUCCESS )
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

#ifdef HI_AUDIO_AI_SUPPORT
/*****************************************AI common interface************************************/
hi_s32 hi_adp_ai_init(hi_unf_ai_port ai_src, hi_handle *ai_handle, hi_handle *track_slave, hi_handle *a_track_vir)
{
    hi_s32                  ret;
    hi_unf_ai_attr        ait_attr = {0};
    hi_unf_audio_track_attr  track_attr;

    ret = hi_unf_ai_init();
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_init failed.\n");
    }

    ret = hi_unf_ai_get_default_attr(ai_src,&ait_attr);
    ait_attr.pcm_frame_max_num = 8;
    if(HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_get_default_attr failed \n");
        return ret;
    }

    ret = hi_unf_ai_create(ai_src, &ait_attr, ai_handle);
    if(HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_create failed \n");
        return ret;
    }


    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_SLAVE, &track_attr);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_get_default_track_attr failed.\n");
        return ret;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0,&track_attr, track_slave);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_create_track failed.\n");
        return ret;
    }

    ret = hi_unf_snd_attach(*track_slave, *ai_handle);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_attach failed.\n");
        return ret;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &track_attr);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_get_default_track_attr failed.\n");
        return ret;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0,&track_attr,a_track_vir);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_create_track failed.\n");
        return ret;
    }

    ret = hi_unf_snd_attach(*a_track_vir, *ai_handle);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_snd_attach failed.\n");
        return ret;
    }

    ret = hi_unf_ai_set_enable(*ai_handle, HI_TRUE);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_set_enable failed.\n");
        return ret;
    }


    return HI_SUCCESS;
}

hi_s32 hiadp_ai_de_init(hi_handle h_ai, hi_handle h_ai_slave, hi_handle h_ai_vir)
{
    hi_s32                  ret;

    ret = hi_unf_ai_set_enable(h_ai, HI_FALSE);
    if (ret != HI_SUCCESS )
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_set_enable failed.\n");
        return ret;
    }

    hi_unf_snd_detach(h_ai_vir, h_ai);
    hi_unf_snd_destroy_track(h_ai_vir);
    hi_unf_snd_detach(h_ai_slave, h_ai);
    hi_unf_snd_destroy_track(h_ai_slave);

    ret = hi_unf_ai_destroy(h_ai);
    if (ret != HI_SUCCESS )
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_destroy failed.\n");
        return ret;
    }

    ret = hi_unf_ai_deinit();
    if (ret != HI_SUCCESS )
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_ai_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}
#endif

static hi_s32 hi_adp_avplay_reg_adec_lib()
{
    hi_s32 ret = HI_SUCCESS;

    ret = hi_unf_avplay_register_acodec_lib("libHA.AUDIO.AMRWB.codec.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.MP3.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.MP2.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.AAC.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.DOLBYTRUEHD.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.TRUEHDPASSTHROUGH.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.AMRNB.codec.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.COOK.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.VOICE.codec.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.G711.codec.so");
#ifdef DOLBYPLUS_HACODEC_SUPPORT
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.DOLBYPLUS.decode.so");
#endif
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.DTSHD.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.DTSM6.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.DTSPASSTHROUGH.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.AC3PASSTHROUGH.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.PCM.decode.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.OPUS.codec.so");
    ret |= hi_unf_avplay_register_acodec_lib("libHA.AUDIO.VORBIS.codec.so");

    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("\n\n!!! some audio codec NOT found. you may NOT able to decode some audio type.\n\n");
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_avplay_init()
{
    hi_s32 ret;
    ret = hi_adp_avplay_reg_adec_lib();
    ret |= hi_unf_avplay_init();
    return ret;
}

hi_s32 hi_adp_avplay_create(hi_handle *avplay,
                                 hi_u32 u32DemuxId,
                                 hi_unf_avplay_stream_type streamtype,
                                 hi_u32 channelflag)
{
    hi_unf_avplay_attr attr;
    hi_handle avhandle;

    if(avplay == HI_NULL)
        return HI_FAILURE;

    if ((u32DemuxId != MPI_DEMUX_PLAY) && (u32DemuxId != MPI_DEMUX_PLAYBACK))
    {
        SAMPLE_COMMON_PRINTF("%d is not a play demux , please select play demux \n", u32DemuxId);
        return HI_FAILURE;
    }

    if(streamtype >= HI_UNF_AVPLAY_STREAM_TYPE_MAX)
        return HI_FAILURE;

    HIAPI_RUN_RETURN(hi_unf_avplay_get_default_config(&attr, streamtype));

    attr.demux_id = u32DemuxId;
    attr.vid_buf_size = 0x300000;
    HIAPI_RUN_RETURN(hi_unf_avplay_create(&attr, &avhandle));

    if(channelflag&HI_UNF_AVPLAY_MEDIA_CHAN_AUD)
        HIAPI_RUN_RETURN(hi_unf_avplay_chan_open(avhandle, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, NULL));

    if(channelflag&HI_UNF_AVPLAY_MEDIA_CHAN_VID)
        HIAPI_RUN_RETURN(hi_unf_avplay_chan_open(avhandle, HI_UNF_AVPLAY_MEDIA_CHAN_VID, NULL));

    *avplay = avhandle;

    SAMPLE_COMMON_PRINTF("demux %d create avplay 0x%x  \n", u32DemuxId, avhandle);

    return HI_SUCCESS;
}

hi_s32 hi_adp_avplay_set_vdec_attr(hi_handle avplay,hi_unf_vcodec_type type,hi_unf_vdec_work_mode enMode)
{
    hi_s32 ret;
    hi_unf_vdec_attr  vdec_attr;

    ret = hi_unf_avplay_get_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("hi_unf_avplay_get_attr failed:%#x\n",ret);
        return ret;
    }

    vdec_attr.type = type;
    vdec_attr.work_mode = enMode;
    vdec_attr.error_cover = 100;
    vdec_attr.priority = 3;

    ret = hi_unf_avplay_set_attr(avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_set_attr failed.\n");
        return ret;
    }

    return ret;
}

#if defined (DOLBYPLUS_HACODEC_SUPPORT)
static hi_void dd_plus_call_back(hi_unf_acodec_dolbyplus_event event, hi_void *user_data)
{
    hi_unf_acodec_dolbyplus_stream_info *info = (hi_unf_acodec_dolbyplus_stream_info *)user_data;
#if 0
    SAMPLE_COMMON_PRINTF( "dd_plus_call_back show info:\n \
                stream_type          = %d\n \
                acmod               = %d\n \
                bit_rate             = %d\n \
                sample_rate_rate      = %d\n \
                event                  = %d\n",
                info->stream_type, info->acmod, info->bit_rate, info->sample_rate_rate,event);
#endif
    g_dolby_acmod = info->acmod;

    if (HI_DOLBYPLUS_EVENT_SOURCE_CHANGE == event)
    {
        g_draw_chn_bar = HI_TRUE;
        //printf("dd_plus_call_back enent !\n");
    }
    return;
}
#endif

hi_s32 hi_adp_avplay_set_adec_attr(hi_handle h_avplay, hi_u32 a_dec_type)
{
    hi_unf_audio_decode_attr adec_attr;
     hi_unf_acodec_wav_format wav_format;
     hi_unf_acodec_dolbyms12_open_config ms12_cfg; //NOTE static mem when being called
     hi_unf_acodec_opus_head_config opus_head_cfg;

    HIAPI_RUN_RETURN(hi_unf_avplay_get_attr(h_avplay, HI_UNF_AVPLAY_ATTR_ID_ADEC, &adec_attr));
    adec_attr.id = a_dec_type;

    if (HI_UNF_ACODEC_ID_PCM == adec_attr.id)
    {
        hi_bool is_big_endian;

        /* if big-endian pcm */
        is_big_endian = HI_FALSE;
        if(HI_TRUE == is_big_endian)
        {
            wav_format.cb_size = 4;
            wav_format.cb_ext_word[0] = NORMAL_PCM_EXTWORD; //choose normal pcm decoder
            //wav_format.cb_ext_word[0] = WIFIDSP_LPCM_EXTWORD; //choose wifi_dsp_lpcm decoder
        }

        if(wav_format.cb_ext_word[0] == NORMAL_PCM_EXTWORD || HI_FALSE == is_big_endian)
        {
        /*
            if choose normal pcm decoder, set attribute
            if choose wifi_dsp_lpcm decoder, need not to set attribute by follows, ignore it
        */
            /* set pcm wav format here base on pcm file */
            wav_format.channels = 1;
            wav_format.samples_per_sec = 48000;
            wav_format.bits_per_sample = 16;
        }
        hi_unf_acodec_pcm_dec_get_default_open_param(&(adec_attr.param),&wav_format);
        SAMPLE_COMMON_PRINTF("please make sure the attributes of PCM stream is tme same as defined in function of \"hi_adp_avplay_set_adec_attr\"? \n");
        SAMPLE_COMMON_PRINTF("(n_channels = 1, w_bits_per_sample = 16, n_samples_per_sec = 48000, is_big_endian = HI_FALSE) \n");
    }
    else if (HI_UNF_ACODEC_ID_MP2 == adec_attr.id)
    {
         hi_unf_acodec_mp2_dec_get_default_open_param(&(adec_attr.param));
    }
    else if (HI_UNF_ACODEC_ID_AAC == adec_attr.id)
    {
         hi_unf_acodec_aac_dec_get_default_open_param(&(adec_attr.param));
    }
    else if (HI_UNF_ACODEC_ID_MP3 == adec_attr.id)
    {
         hi_unf_acodec_mp3_dec_get_default_open_param(&(adec_attr.param));
    }
    else if (HI_UNF_ACODEC_ID_AMRNB == adec_attr.id)
    {
        hi_unf_acodec_amrnb_decode_open_config *config = (hi_unf_acodec_amrnb_decode_open_config *)dec_open_buf;
        hi_amrnb_get_dec_default_open_param(&(adec_attr.param), config);
        config->format = HI_UNF_ACODEC_AMRNB_MIME;
    }
    else if (HI_UNF_ACODEC_ID_AMRWB == adec_attr.id)
    {
        hi_unf_acodec_amrwb_decode_open_config *config = (hi_unf_acodec_amrwb_decode_open_config *)dec_open_buf;
        hi_unf_acodec_amrwb_get_dec_default_open_param(&(adec_attr.param), config);
        config->format = HI_UNF_ACODEC_AMRWB_FORMAT_MIME;
    }

    else if (HI_UNF_ACODEC_ID_G711 == adec_attr.id)
    {
        hi_unf_acodec_voice_open_config *config = (hi_unf_acodec_voice_open_config *)dec_open_buf;
        adec_attr.id = HI_UNF_ACODEC_ID_VOICE;
        config->voice_format = HI_UNF_ACODEC_VOICE_G711_A;
        config->sample_per_frame = 320;
        hi_unf_acodec_voice_get_dec_default_open_param(&(adec_attr.param), config);
    }
    else if (HI_UNF_ACODEC_ID_G726 == adec_attr.id)
    {
        hi_unf_acodec_voice_open_config *config = (hi_unf_acodec_voice_open_config *)dec_open_buf;
        adec_attr.id = HI_UNF_ACODEC_ID_VOICE;
        config->voice_format = HI_UNF_ACODEC_VOICE_G726_40KBPS;
        config->sample_per_frame = 320;
        hi_unf_acodec_voice_get_dec_default_open_param(&(adec_attr.param), config);
    }
    else if (HI_UNF_ACODEC_ID_ADPCM == adec_attr.id)
    {
        hi_unf_acodec_voice_open_config *config = (hi_unf_acodec_voice_open_config *)dec_open_buf;
        adec_attr.id = HI_UNF_ACODEC_ID_VOICE;
        config->voice_format = HI_UNF_ACODEC_VOICE_ADPCM_DVI4;
        config->sample_per_frame = 320;
        hi_unf_acodec_voice_get_dec_default_open_param(&(adec_attr.param), config);
    }
    else if (HI_UNF_ACODEC_ID_AC3PASSTHROUGH == adec_attr.id)
    {
        hi_unf_acodec_ac3_passthrough_dec_get_default_open_param(&(adec_attr.param));
        adec_attr.param.dec_mode = HI_UNF_ACODEC_DEC_MODE_THRU;
    }
    else if(HI_UNF_ACODEC_ID_DTSPASSTHROUGH == adec_attr.id)
    {
        hi_unf_acodec_dtspassthrough_dec_get_default_open_param(&(adec_attr.param));
        adec_attr.param.dec_mode = HI_UNF_ACODEC_DEC_MODE_THRU;
    }
    else if (HI_UNF_ACODEC_ID_TRUEHD == adec_attr.id)
    {
        hi_unf_acodec_truehd_dec_get_default_open_param(&(adec_attr.param));
        adec_attr.param.dec_mode = HI_UNF_ACODEC_DEC_MODE_THRU;        /* truehd just support pass-through */
        SAMPLE_COMMON_PRINTF(" true_hd decoder(HBR pass-through only).\n");
    }
    else if (HI_UNF_ACODEC_ID_DOLBY_TRUEHD == adec_attr.id)
    {
        hi_unf_acodec_truehd_decode_open_config *config = (hi_unf_acodec_truehd_decode_open_config *)dec_open_buf;
        hi_unf_acodec_dolby_truehd_dec_get_default_open_config(config);
        hi_unf_acodec_dolby_truehd_dec_get_default_open_param(&(adec_attr.param), config);
    }
    else if (HI_UNF_ACODEC_ID_DTSHD == adec_attr.id)
    {
        hi_unf_acodec_dtshd_decode_open_config *config = (hi_unf_acodec_dtshd_decode_open_config *)dec_open_buf;
        hi_unf_acodec_dtshd_dec_get_default_open_config(config);
        hi_unf_acodec_dtshd_dec_get_default_open_param(&(adec_attr.param), config);
        adec_attr.param.dec_mode = HI_UNF_ACODEC_DEC_MODE_SIMUL;
    }
    else if (HI_UNF_ACODEC_ID_DTSM6 == adec_attr.id)
    {
        hi_unf_acodec_dtsm6_decode_open_config *config = (hi_unf_acodec_dtsm6_decode_open_config *)dec_open_buf;
        hi_unf_acodec_dtsm6_dec_get_default_open_config(config);
        hi_unf_acodec_dtsm6_dec_get_default_open_param(&(adec_attr.param), config);
    }
#if defined (DOLBYPLUS_HACODEC_SUPPORT)
    else if (HI_UNF_ACODEC_ID_DOLBY_PLUS == adec_attr.id)
    {
        hi_unf_acodec_dolbyplus_decode_openconfig *config = (hi_unf_acodec_dolbyplus_decode_openconfig *)dec_open_buf;
        hi_unf_acodec_dolbyplus_dec_get_default_open_config(config);
        config->pfn_evt_cb_func[HI_DOLBYPLUS_EVENT_SOURCE_CHANGE] = dd_plus_call_back;
        config->app_data[HI_DOLBYPLUS_EVENT_SOURCE_CHANGE] = &g_ddp_stream_info;
        /* dolby DVB broadcast default settings */
        config->drc_mode = DOLBYPLUS_DRC_RF;
        config->dmx_mode = DOLBYPLUS_DMX_SRND;
        hi_unf_acodec_dolbyplus_dec_get_default_open_param(&(adec_attr.param), config);
        adec_attr.param.dec_mode = HI_UNF_ACODEC_DEC_MODE_SIMUL;
    }
#endif
    else if (HI_UNF_ACODEC_ID_MS12_DDP == adec_attr.id)
    {
        hi_unf_acodec_dolbyms12_get_default_open_config(&ms12_cfg);
        hi_unf_acodec_dolbyms12_get_default_open_param(&adec_attr.param, &ms12_cfg);
    }
    else if (HI_UNF_ACODEC_ID_MS12_AAC == adec_attr.id)
    {
        hi_unf_acodec_dolbyms12_get_default_open_config(&ms12_cfg);
        ms12_cfg.input_type = MS12_HEAAC;
        hi_unf_acodec_dolbyms12_get_default_open_param(&adec_attr.param, &ms12_cfg);
    }
    else if (HI_UNF_ACODEC_ID_VORBIS == adec_attr.id)
    {
        hi_unf_acodec_vorbis_dec_get_defalut_open_param(&adec_attr.param);
    }
    else if (HI_UNF_ACODEC_ID_OPUS == adec_attr.id)
    {
        /*
        opus decoder support 1 2 6 8 channels 8 16 24 32 bits
        opus stream with sample rate not less than 8000 and not more than 48000
        after the upper de-encapsulation need to provide code stream information this information can be found in the front page
        the format is as follows:
              0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      'O'           |      'p'             |      'u'          |      's'         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      'H'           |      'e'             |      'a'          |      'd'         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |  version = 1  |channel count |           pre-skip                |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                     input sample rate (hz)                               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |   output gain (Q7.8 in d_b)      | mapping family|               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                                                                                      |
        :               optional channel mapping table...
        |                                                                                      |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        the parsed out stream information needs to be stored in the opus_header structure
        like this:
        */
        hi_unf_acodec_opus_dec_get_default_head_config(&opus_head_cfg);

#if 0
        //test for 2 channel
        opus_head_cfg.version = 1;
        opus_head_cfg.chan = 2;
        opus_head_cfg.preskip = 0;
        opus_head_cfg.sample_rate = 48000;
        opus_head_cfg.channel_map = 0;
#else
        //test for 5.1 channel
        opus_head_cfg.version = 1;
        opus_head_cfg.chan = 6;
        opus_head_cfg.preskip = 312;
        opus_head_cfg.gain = 0;
        opus_head_cfg.sample_rate = 48000;
        opus_head_cfg.nb_streams = 4;
        opus_head_cfg.nb_coupled = 2;
        opus_head_cfg.channel_map = 1;

        opus_head_cfg.stream_map[0] = 0;
        opus_head_cfg.stream_map[1] = 4;
        opus_head_cfg.stream_map[2] = 1;
        opus_head_cfg.stream_map[3] = 2;
        opus_head_cfg.stream_map[4] = 3;
        opus_head_cfg.stream_map[5] = 5;
#endif

        hi_unf_acodec_opus_dec_get_default_open_param(&adec_attr.param, &opus_head_cfg);
    }
    else
    {
         hi_unf_acodec_dra_dec_get_open_param_multich_pcm(&(adec_attr.param));
    }

    HIAPI_RUN_RETURN(hi_unf_avplay_set_attr(h_avplay, HI_UNF_AVPLAY_ATTR_ID_ADEC, &adec_attr));

    return HI_SUCCESS;
}

hi_s32 hi_adp_avplay_play_prog(hi_handle hAvplay,pmt_compact_tbl *pProgTbl,hi_u32 ProgNum,hi_u32 channel_flag)
{
    hi_unf_avplay_stop_opt    Stop;
    hi_u32                  VidPid;
    hi_u32                  AudPid;
    hi_u32                  PcrPid;
    hi_unf_vcodec_type    enVidType;
    hi_u32                  u32AudType;
    hi_s32                  ret;

    Stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    Stop.timeout = 0;

    ret = hi_unf_avplay_stop(hAvplay, (hi_unf_avplay_media_chan)channel_flag, &Stop);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_stop failed.\n");
        return ret;
    }

    ProgNum = ProgNum % pProgTbl->prog_num;
    if (pProgTbl->proginfo[ProgNum].v_element_num > 0 )
    {
        VidPid = pProgTbl->proginfo[ProgNum].v_element_pid;
        enVidType = pProgTbl->proginfo[ProgNum].video_type;
    }
    else
    {
        VidPid = INVALID_TSPID;
        enVidType = HI_UNF_VCODEC_TYPE_MAX;
    }

    if (pProgTbl->proginfo[ProgNum].a_element_num > 0)
    {
        AudPid  = pProgTbl->proginfo[ProgNum].a_element_pid;
        u32AudType = pProgTbl->proginfo[ProgNum].audio_type;
    }
    else
    {
        AudPid = INVALID_TSPID;
        u32AudType = 0xffffffff;
    }

    PcrPid = pProgTbl->proginfo[ProgNum].pcr_pid;
    if (INVALID_TSPID != PcrPid)
    {
        hi_unf_sync_attr  SyncAttr;

        ret = hi_unf_avplay_get_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        if (HI_SUCCESS != ret)
        {
            SAMPLE_COMMON_PRINTF("hi_unf_avplay_get_attr Sync failed 0x%x\n", ret);
            return ret;
        }

        if (HI_UNF_SYNC_REF_MODE_PCR == SyncAttr.ref_mode)
        {
            ret = hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_PCR_PID, &PcrPid);
            if (HI_SUCCESS != ret)
            {
                SAMPLE_COMMON_PRINTF("hi_unf_avplay_set_attr Sync failed 0x%x\n", ret);
                return ret;
            }
        }
    }

#ifdef TSPLAY_SUPPORT_VID_CHAN
    if (VidPid != INVALID_TSPID && channel_flag & HI_UNF_AVPLAY_MEDIA_CHAN_VID)
    {
        ret = hi_adp_avplay_set_vdec_attr(hAvplay,enVidType, HI_UNF_VDEC_WORK_MODE_NORMAL);
        ret |= hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_VID_PID,&VidPid);
        if (ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call hi_adp_avplay_set_vdec_attr failed.\n");
            return ret;
        }

        ret = hi_unf_avplay_start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call hi_unf_avplay_start failed.\n");
            return ret;
        }
    }
#else
    (void)VidPid;
    (void)enVidType;
#endif

#ifdef TSPLAY_SUPPORT_AUD_CHAN
    if (AudPid != INVALID_TSPID && channel_flag & HI_UNF_AVPLAY_MEDIA_CHAN_AUD)
    {
        //u32AudType = HI_UNF_ACODEC_ID_DTSHD;
        //printf("u32AudType = %#x\n",u32AudType);
        ret  = hi_adp_avplay_set_adec_attr(hAvplay, u32AudType);

        ret |= hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &AudPid);
        if (HI_SUCCESS != ret)
        {
            SAMPLE_COMMON_PRINTF("hi_adp_avplay_set_adec_attr failed:%#x\n",ret);
            return ret;
        }

        ret = hi_unf_avplay_start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_start to start audio failed.\n");
            //return ret;
        }
    }
#else
    (void)AudPid;
    (void)u32AudType;
#endif
    return HI_SUCCESS;
}

hi_s32 hi_adp_avplay_play_prog_ms12(hi_handle hAvplay,pmt_compact_tbl *pProgTbl,hi_u32 ProgNum)
{
    hi_unf_avplay_stop_opt    Stop;
    hi_u32                  AudPid;
    hi_u32                  u32AudType;
    hi_s32                  ret;
    hi_s32                  MainProgNum = 0;
    hi_s32                  PIDNum = 0;

    Stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    Stop.timeout = 0;
    ret = hi_unf_avplay_stop(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &Stop);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_stop failed.\n");
        return ret;
    }

    PIDNum = ProgNum % pProgTbl->proginfo[MainProgNum].a_element_num;

    if (pProgTbl->proginfo[MainProgNum].a_element_num > 0)
    {
        AudPid  = pProgTbl->proginfo[MainProgNum].audio_info[PIDNum].audio_pid;
        u32AudType = pProgTbl->proginfo[MainProgNum].audio_info[PIDNum].audio_enc_type;
    }
    else
    {
        AudPid = INVALID_TSPID;
        u32AudType = 0xffffffff;
    }

    if (AudPid != INVALID_TSPID)
    {
        ret  = hi_adp_avplay_set_adec_attr(hAvplay, u32AudType);
        ret |= hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &AudPid);
        if (HI_SUCCESS != ret)
        {
            SAMPLE_COMMON_PRINTF("hi_adp_avplay_set_adec_attr failed:%#x\n",ret);
            return ret;
        }

        ret = hi_unf_avplay_start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            printf("call hi_unf_avplay_start to start audio failed.\n");
        }
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_avplay_play_aud(hi_handle hAvplay,pmt_compact_tbl *pProgTbl,hi_u32 ProgNum)
{
    hi_u32                  AudPid;
    hi_u32                  u32AudType;
    hi_s32                  ret;

    ret = hi_unf_avplay_stop(hAvplay,HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_stop failed.\n");
        return ret;
    }

    ProgNum = ProgNum % pProgTbl->prog_num;
    if (pProgTbl->proginfo[ProgNum].a_element_num > 0)
    {
        AudPid  = pProgTbl->proginfo[ProgNum].a_element_pid;
        u32AudType = pProgTbl->proginfo[ProgNum].audio_type;
    }
    else
    {
        AudPid = INVALID_TSPID;
        u32AudType = 0xffffffff;
    }

    if (AudPid != INVALID_TSPID)
    {
        ret  = hi_adp_avplay_set_adec_attr(hAvplay, u32AudType);
        ret |= hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &AudPid);
        if (HI_SUCCESS != ret)
        {
            SAMPLE_COMMON_PRINTF("hi_adp_avplay_set_adec_attr failed:%#x\n",ret);
            return ret;
        }

        ret = hi_unf_avplay_start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
        if (ret != HI_SUCCESS)
        {
            SAMPLE_COMMON_PRINTF("call hi_unf_avplay_start failed.\n");
            return ret;
        }
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_avplay_switch_aud(hi_handle hAvplay,hi_u32 AudPid, hi_u32 u32AudType)
{
    hi_s32 ret = HI_SUCCESS;

    if (AudPid == INVALID_TSPID)
    {
        SAMPLE_COMMON_PRINTF("%s, audio pid is invalid!\n", __func__);
        return HI_FAILURE;
    }

    ret = hi_unf_avplay_stop(hAvplay,HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_stop failed.\n");
        return ret;
    }


    ret  = hi_adp_avplay_set_adec_attr(hAvplay, u32AudType);
    ret |= hi_unf_avplay_set_attr(hAvplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &AudPid);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("hi_adp_avplay_set_adec_attr failed:%#x\n",ret);
        return ret;
    }

    ret = hi_unf_avplay_start(hAvplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_COMMON_PRINTF("call hi_unf_avplay_start failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}


hi_s32 hi_adp_mce_exit(hi_void)
{
#if 0
#ifndef ANDROID
#ifdef MCE_SUPPORTED
    hi_s32                  ret;
    HI_UNF_MCE_STOPPARM_S   stStop;

    ret = HI_UNF_MCE_Init(HI_NULL);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call HI_UNF_MCE_Init failed, ret=%#x!\n", ret);
        return ret;
    }

    ret = HI_UNF_MCE_ClearLogo();
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call HI_UNF_MCE_ClearLogo failed, ret=%#x!\n", ret);
        return ret;
    }

    stStop.enStopMode = HI_UNF_AVPLAY_STOP_MODE_STILL;
    stStop.enCtrlMode = HI_UNF_MCE_PLAYCTRL_BY_TIME;
    stStop.u32PlayTimeMs = 0;
    ret = HI_UNF_MCE_Stop(&stStop);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call HI_UNF_MCE_Stop failed, ret=%#x!\n", ret);
        return ret;
    }

    ret = HI_UNF_MCE_Exit(HI_NULL);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_COMMON_PRINTF("call HI_UNF_MCE_Exit failed, ret=%#x!\n", ret);
        return ret;
    }

    HI_UNF_MCE_DeInit();
#endif
#endif
#endif
    return HI_SUCCESS;
}


hi_s32 hi_adp_dmx_attach_ts_port(hi_u32 dmx_id, hi_u32 tuner_id)
{
    hi_s32                      ret, ini_ret;
    hi_unf_dmx_port           dmx_attach_port;
    hi_unf_dmx_port_attr      dmx_attach_port_attr;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "" };
    snprintf(ini_data.section, SECTION_MAX_LENGTH, "frontend%dinfo", tuner_id);

    ini_ret = hi_adp_ini_get_u32(&ini_data, "DemuxPort", &dmx_attach_port);
    if (ini_ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_get_ts_port_attr(dmx_attach_port, &dmx_attach_port_attr);

    ini_ret = hi_adp_ini_get_u32(&ini_data, "DemuxPortType", &(dmx_attach_port_attr.port_type));
    if (ini_ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    ini_ret = hi_adp_ini_get_u32(&ini_data, "DemuxBitSel", &(dmx_attach_port_attr.serial_bit_selector));
    if (ini_ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    ini_ret = hi_adp_ini_get_u32(&ini_data, "DemuxInClk", &(dmx_attach_port_attr.frontend_clk_inverse));
    if (ini_ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    ini_ret = hi_adp_ini_get_u32(&ini_data, "DemuxPortShareClk", &(dmx_attach_port_attr.serial_port_share_clk));
        if (ini_ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    ini_ret = hi_adp_ini_get_u32(&ini_data, "DemuxClkMode", &(dmx_attach_port_attr.frontend_clk_mode));
    if (ini_ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    ret |= hi_unf_dmx_set_ts_port_attr(dmx_attach_port, &dmx_attach_port_attr);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_dmx_set_ts_port_attr failed.\n");
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_attach_ts_port(dmx_id, dmx_attach_port);
    if (HI_SUCCESS != ret)
    {
        printf("call hi_unf_dmx_attach_ts_port.\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 hi_adp_dmx_push_ts_buf(hi_handle hTsBuf, hi_unf_stream_buf *buf, hi_u32 start_pos, hi_u32 valid_len)
{
    hi_s32 ret = HI_SUCCESS;

#ifdef HI_DMX_TSBUF_MULTI_THREAD_SUPPORT
    hi_unf_stream_buf data;

    if ((HI_NULL == buf) || (HI_NULL == buf->data) || (buf->size < start_pos + valid_len))
    {
        printf("invalided parameter!\n");
        return HI_FAILURE;
    }

    data.data = buf->data + start_pos;
    data.size = valid_len;
    ret = hi_unf_dmx_push_ts_buffer(hTsBuf, &data);
    if (HI_SUCCESS != ret)
    {
        printf("[%s_%d]calling hi_unf_dmx_push_ts_buffer failed! ret = 0x%08x\n", __FILE__, __LINE__, ret);
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_release_ts_buffer(hTsBuf, buf);
#else
    if ((HI_NULL == buf) || (HI_NULL == buf->data) || (buf->size < start_pos + valid_len))
    {
        printf("invalided parameter!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_dmx_put_ts_buffer(hTsBuf, valid_len, start_pos);
#endif

    if (HI_SUCCESS != ret)
    {
        printf("[pu8Data, start_pos, valid_len, ret] = [%p, %u, %u, 0x%x]\n", buf->data, start_pos, valid_len, ret);
    }

    return ret;
}
