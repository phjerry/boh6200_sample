/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description : sample frontend
 * Author        : sdk
 * Create        : 2019-11-01
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_demux.h"
#include "hi_unf_win.h"
#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_unf_frontend.h"

#define MAX_CMD_BUFFER_LEN  256
#define UDP_STR_SPLIT       ' '
#define TEST_ARGS_SPLIT     " "
#define HI_RESULT_SUCCESS   "SUCCESS"
#define HI_RESULT_FAIL      "FAIL"
#define DEFAULT_PORT        1234

#define HI_TUNER_ERR(format, arg...)    printf("[ERR]: %s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#define HI_TUNER_INFO(format, arg...)   printf(format, ##arg)
#define HI_TUNER_WARN(format, arg...)   printf("[WARNING]: %s,%d: " format, __FUNCTION__, __LINE__, ##arg)

typedef struct {
    hi_handle                   win;
    hi_handle                   avplay;
    hi_handle                   track;
} display_handle_t;

typedef hi_void (*test_func_proc)(const char* args);

typedef struct {
    hi_char*         name;
    test_func_proc   proc;
} test_fun_s;

typedef struct {
    const hi_char   *name;
    const hi_char   *help_info;
} cmd_help_s;

static hi_u32 g_frontend_num;
static hi_u32 g_frontend_port;
static hi_bool g_parse_ts = HI_TRUE;
static hi_unf_frontend_connect_para  g_connect_para[HI_FRONTEND_NUMBER];
static pmt_compact_tbl *g_prog_tbl[HI_FRONTEND_NUMBER] = {HI_NULL};
static display_handle_t g_disp_handle;

/* save results, then send client */
char g_test_result[MAX_CMD_BUFFER_LEN];

cmd_help_s g_cmd_help[] = {
    { "help", "help: show help menu, only input 'help' will print command list,\n"
                    "\t 'help cmd' will print the help infomation of the cmd.\n" },
    { "setport", "setport 0(frontend port number,max value is 3).\n"
                  "\t 0 is the first frontend.\n"
                  "\t 1 is the second frontend.\n"
                  "\t 2 is the third frontend.\n"
                  "\t 3 is the fourth frontend.\n"},
    { "setchnl", "setchnl 3840 27500000 0 (frequency/symbolrate/polar) [DVB-S/S2],\n"
                    "\t frequency unit MHz, you should input downlink frequency here,\n"
                    "\t symbolrate unit Hz,\n"
                    "\t polar 0:Horizontal 1:Vertical 2:Left-hand circular 3:Right-hand circular.\n"
                "\tsetchnl 666000 8000 0 [0] [4] [1] (frequency/BandWidth/uncertain/[data plp]/[comm plp]/[combine])"
                    " [DVB-T/T2,ISDB-T,DTMB],\n"
                    "\t frequency unit KHz,\n"
                    "\t bandwidth unit KHz,\n"
                    "\t uncertain 0:HP 1:LP [DVB-T],0:base 1:lite [DVB-T2].\n"
                    "\t data plp:data plp index [DVB-T2],\n"
                    "\t comm plp:common plp index [DVB-T2],\n"
                    "\t combine 0:common plp is exist 1:common plp is not exist [DVB-T2].\n"
                "\tsetchnl 666000 6875 256 (frequency/symbolrate/qam) [DVB-C,J83B],\n"
                    "\t frequency unit KHz,\n"
                    "\t symbolrate unit KHz,\n"
                    "\t qam 16:16qam 32:32qam 64:64qam 128:128qam 256:256qam.\n" },
    { "play", "play [Program_Num]: set program number. The default value is 1.\n" },
    { "getsignalinfo", "getsignalinfo: get detailed infomation of current locked signal."
                    "[FOR satellite and terrestrial signal].\n" },
    { "getoffset", "getoffset : getfreq getsyb. \n" },
    { "parsets", "parsets 0(enable): if prase PMT from TS input.\n" },

#ifdef DVBS_SUPPORT
    { "setlnbpower", "setlnbpower 0: set LNB power.[FOR DVB-S/S2]\n"
                   "\t 0 Power off,  1 Power auto(13V/18V),  2 Enhanced(14V/19V).\n" },
    { "setlnb", "setlnb 1 5150 5750 0: set LNB band and Low/High Local Oscillator.[FOR DVB-S/S2]\n"
                   "\t Param1: LNB type:0 single LO, 1 dual LO.\n"
                   "\t Param2: LNB low LO: MHz, e.g.5150.\n"
                   "\t Param3: LNB high LO: MHz, e.g.5750.\n"
                   "\t Param4: LNB band:0 C, 1 Ku.\n" },
    { "getisi", "getisi \n"
                   "\t when signal mode is acm/vcm/multi stream,\n"
                   "\t print ISI ID of each stream.\n" },
    { "setisi", "setisi 0(ISI ID):set ISI ID \n"
                   "\t when signal mode is acm/vcm/multi stream,\n"
                   "\t ISI ID can be getted from 'getvcm' command\n" },
    { "setscram", "setscram 262140(physical layer scrambling code).\n"
                  "\t the scrambling code can be find out from website,0 is the default value.\n" },
    { "blindscan", "blindscan 0 [0] [0] [950000] [2150000]: Blind scan. [FOR DVB-S/S2]\n"
                  "\t Param1: blind scan type: 0 Auto, 1 Manual.\n"
                  "\t Param2: LNB Polarization: 0 H, 1 V. Only for manual type.\n"
                  "\t Param3: LNB 22K:0 Off, 1 On. Only for manual type.\n"
                  "\t Param4: Start frequency. Only for manual type.\n"
                  "\t Param5: Stop frequency. Only for manual type.\n" },
    { "bsstop", "bsstop: Stop blind scan. [FOR DVB-S/S2]\n" },
    { "cs", "cs [0] [0]: sample data. [FOR HI3136]\n"
           "\t Param1: data source: 0 ADC, 1 EQU.\n"
           "\t Param2: data length: can be 512, 1024, 2048.\n" },
#ifdef DISEQC_SUPPORT
    { "switch", "switch 0 0 [0] [0]: Switch test. [FOR DVB-S/S2]\n"
                  "\t Param1: Switch type:0 0/12V, 1 Tone burst, 2 22K, 3 DiSEqC 1.0, 4 DiSEqC 1.1, "
                  "5 Reset, 6 Standby, 7 WakeUp.\n"
                  "\t 0/12V Switch: param2: Switch port(0 None, 1 0V, 2 12V)\n"
                  "\t Tone burst: Param2: Tone burst port(0 None, 1 Tone burst 0, 1 Tone burst 1)\n"
                  "\t 22K Switch: param2: 22K Switch port(0 None, 1 0KHz, 2 22KHz)\n"
                  "\t DiSEqC 1.0 Switch: Param2: Switch port(0 None, 1-4), Param3: LNB Polarization(0 H, 1 V), "
                  "Param4: LNB 22K(0 Off, 1 On)\n"
                  "\t DiSEqC 1.1 Switch: Param2: Switch port(0 None, 1-16)\n" },
    { "motor", "motor 0 [0]: DiSEqC motor test. [FOR DVB-S/S2]\n"
                  "\t Param1: Control type:0 StorePos, 1 GotoPos, 2 SetLimit, 3 Move, 4 Stop, "
                  "5 USALS, 6 Recalculate, 7 GotoAng.\n"
                  "\t StorePos: Param2:position(0-63).\n"
                  "\t GotoPos: Param2:position(0-63).\n"
                  "\t SetLimit: Param2:limit type(0 off, 1 east, 2 west).\n"
                  "\t Move: Param2:move direction(0 east 1 west ), Param3:move type(0 one step, 1 two step, ..., "
                  "9 nine step, 10 continus).\n"
                  "\t USALS: Param2:local longitude(0-3600), Param3:local latitude(0-1800), "
                  "Param4:satellite longitude(0-3600).\n"
                  "\t Recalculate: Param2/Param3/Param4: parameter1/2/3 for recalculate.\n"
                  "\t GotoAng: Param2:angle.\n" },
    { "unic", "unic 0(function index):select unicable function.[FOR DVB-S/S2]\n"
                  "\t 0: start unicable1 user bands scanning.\n"
                  "\t 1: exit from unicable1 user bands scanning.\n" },
#endif

#endif

#ifdef DVBT_SUPPORT
    { "setplpid",     "select the plp id.[FOR DVB-T/T2]\n" },
    { "ant",          "ant 1: set Antena power on.[FOR DVB-T/T2]\n" },
    { "monitorlayer", "monitorlayer 1: set isdbt mointor layer\n" },
#endif

    { "exit", "exit    : exit the sample\n" }
};

hi_s32 dev_init()
{
    hi_s32 ret;
    hi_s32 i;
    ini_data_section ini_data = { FRONTEND_CONFIG_FILE, "frontendnum" };
    hi_unf_sys_init();
    //hi_adp_mce_exit();

    ret = hi_adp_ini_get_u32(&ini_data, "FrontendNum", &g_frontend_num);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("hi_adp_ini_get_u32 failed.\n");
        return ret;
    }
    if (g_frontend_num > HI_FRONTEND_NUMBER) {
        HI_TUNER_ERR("Cfg FrontendNum[%d] is out of max number[%d].\n", g_frontend_num, HI_FRONTEND_NUMBER);
        return HI_FAILURE;
    }

    if (g_frontend_num > 0) {
        ret = hi_adp_frontend_init();
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("call hi_adp_frontend_init failed.\n");
            return ret;
        }
    }

    for (i = 0; i < g_frontend_num; i++) {
        hi_adp_frontend_get_connect_para(i, &g_connect_para[i]);
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_dmx_init failed.\n");
        hi_adp_frontend_deinit();
        return ret;
    }

    ret = hi_adp_dmx_attach_ts_port(0, 0);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_adp_dmx_attach_ts_port failed 0x%x\n", ret);
        hi_unf_dmx_deinit();
        hi_adp_frontend_deinit();
        return ret;
    }

    return HI_SUCCESS;
}


hi_void dev_deinit()
{
    hi_s32 ret;

    ret = hi_unf_dmx_detach_ts_port(0);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_dmx_detach_ts_port failed.\n");
    }

    ret = hi_unf_dmx_deinit();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_dmx_deinit failed.\n");
    }

    hi_adp_frontend_deinit();
    hi_unf_sys_deinit();
}

static hi_s32 avplay_init(hi_void)
{
    hi_s32 ret;
    hi_unf_avplay_attr avplay_attr;

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_adp_avplay_init failed.\n");
        return ret;
    }

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_get_default_config failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_create(&avplay_attr, &g_disp_handle.avplay);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_create failed.\n");
        goto AVPLAY_DEINIT;
    }

    ret = hi_unf_avplay_chan_open(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY;
    }

    ret = hi_unf_avplay_chan_open(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_chan_open failed.\n");
        goto AVPLAY_DESTROY_VID;
    }

    return HI_SUCCESS;

AVPLAY_DESTROY_VID:
    hi_unf_avplay_chan_close(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
AVPLAY_DESTROY:
    hi_unf_avplay_destroy(g_disp_handle.avplay);
AVPLAY_DEINIT:
    hi_unf_avplay_deinit();
    return ret;
}

static hi_s32 avplay_set_attr(hi_void)
{
    hi_s32 ret;
    hi_unf_sync_attr          sync_attr;
    hi_unf_vdec_attr          vdec_attr;

    ret = hi_adp_avplay_set_adec_attr(g_disp_handle.avplay, HI_UNF_ACODEC_ID_MP3);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call HI_UNF_AVPLAY_SetAttr_ADEC failed.\n");
        return ret;
    }

    ret = hi_unf_avplay_get_attr(g_disp_handle.avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_get_attr failed.\n");
        return ret;
    }

    vdec_attr.work_mode = HI_UNF_VDEC_WORK_MODE_NORMAL;
    vdec_attr.type = HI_UNF_VCODEC_TYPE_MPEG2;
    vdec_attr.error_cover = 80; /* error cover 80 % */
    vdec_attr.priority = HI_UNF_VCODEC_MAX_PRIORITY;
    ret = hi_unf_avplay_set_attr(g_disp_handle.avplay, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_set_attr failed.\n");
        return ret;
    }

    ret = hi_unf_avplay_get_attr(g_disp_handle.avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_get_attr failed.\n");
        return ret;
    }

    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    sync_attr.start_region.vid_plus_time = 100; /* 100: video plus time */
    sync_attr.start_region.vid_negative_time = -100; /* 100: video negative time */
    sync_attr.quick_output_enable = HI_FALSE;
    ret = hi_unf_avplay_set_attr(g_disp_handle.avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_avplay_set_attr failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 vo_init()
{
    hi_s32 ret;
    hi_unf_video_format enc_fmt = HI_UNF_VIDEO_FMT_1080P_50;

    ret = hi_adp_disp_init(enc_fmt);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_adp_disp_init failed.\n");
        return ret;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_adp_win_init failed.\n");
        goto DISP_DEINIT;
    }

    ret = hi_adp_win_create(HI_NULL, &g_disp_handle.win);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_adp_win_create failed.\n");
        goto VO_DEINIT;
    }

    ret = hi_unf_win_attach_src(g_disp_handle.win, g_disp_handle.avplay);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_win_attach_src failed.\n");
        goto VO_DESTROY;
    }

    ret = hi_unf_win_set_enable(g_disp_handle.win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_win_set_enable failed.\n");
        goto VO_DETACH_WINDOW;
    }

    return HI_SUCCESS;

VO_DETACH_WINDOW:
    hi_unf_win_detach_src(g_disp_handle.win, g_disp_handle.avplay);
VO_DESTROY:
    hi_unf_win_destroy(g_disp_handle.win);
VO_DEINIT:
    hi_adp_win_deinit();
DISP_DEINIT:
    hi_adp_disp_deinit();
    return ret;
}

static hi_s32 snd_init(hi_void)
{
    hi_s32 ret;
    hi_unf_audio_track_attr   track_attr;

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call HIADP_Snd_Init failed.\n");
        return ret;
    }

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_snd_get_default_track_attr failed.\n");
        return ret;
    }

    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &g_disp_handle.track);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_snd_create_track failed.\n");
        goto SND_DEINIT;
    }

    ret = hi_unf_snd_attach(g_disp_handle.track, g_disp_handle.avplay);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_unf_snd_attach failed.\n");
        goto SND_DESTORY_TRACK;
    }

    return HI_SUCCESS;

SND_DESTORY_TRACK:
    hi_unf_snd_destroy_track(g_disp_handle.track);
SND_DEINIT:
    hi_adp_snd_deinit();
    return ret;
}

hi_s32 frontend_display_init(hi_void)
{
    hi_s32 ret;
    ret = avplay_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call avplay_init failed.\n");
        return ret;
    }
    ret = vo_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call vo_init failed.\n");
        goto AVPLAY_DEINIT;
    }
    ret = snd_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call snd_init failed.\n");
        goto VO_DEINIT;
    }
    ret = avplay_set_attr();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call avplay_set_attr failed.\n");
        goto SND_DEINIT;
    }

    return HI_SUCCESS;

SND_DEINIT:
    hi_unf_snd_detach(g_disp_handle.track, g_disp_handle.avplay);
    hi_unf_snd_destroy_track(g_disp_handle.track);
    hi_adp_snd_deinit();
VO_DEINIT:
    hi_unf_win_set_enable(g_disp_handle.win, HI_FALSE);
    hi_unf_win_detach_src(g_disp_handle.win, g_disp_handle.avplay);
    hi_unf_win_destroy(g_disp_handle.win);
    hi_adp_win_deinit();
    hi_adp_disp_deinit();
AVPLAY_DEINIT:
    hi_unf_avplay_chan_close(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_chan_close(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    hi_unf_avplay_destroy(g_disp_handle.avplay);
    hi_unf_avplay_deinit();
    return ret;
}

hi_void frontend_display_deinit()
{
    hi_s32 ret;
    hi_unf_avplay_stop_opt stop_opt;

    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop_opt.timeout = 0;

    ret = hi_unf_avplay_stop(g_disp_handle.avplay,
        HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_stop failed.\n");
    }

    ret = hi_unf_snd_detach(g_disp_handle.track, g_disp_handle.avplay);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_snd_detach failed.\n");
    }

    ret = hi_unf_snd_destroy_track(g_disp_handle.track);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_snd_destroy_track failed.\n");
    }

    ret = hi_unf_win_set_enable(g_disp_handle.win, HI_FALSE);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_win_set_enable failed.\n");
    }

    ret = hi_unf_win_detach_src(g_disp_handle.win, g_disp_handle.avplay);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_win_detach_src failed.\n");
    }

    ret = hi_unf_avplay_chan_close(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_chan_close failed.\n");
    }

    ret = hi_unf_avplay_chan_close(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_chan_close failed.\n");
    }

    ret = hi_unf_avplay_destroy(g_disp_handle.avplay);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_destroy failed.\n");
    }

    ret = hi_unf_avplay_deinit();
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_deinit failed.\n");
    }
}

/* help function */
hi_void hi_showhelp(const char *cmd)
{
    hi_u32 i;
    hi_u32 cmd_num = sizeof(g_cmd_help) / sizeof(g_cmd_help[0]);

    if (cmd == HI_NULL_PTR) {
        HI_TUNER_INFO("command list:\n");
        for (i = 0; i < cmd_num; i++) {
            HI_TUNER_INFO("%s:%s\n", g_cmd_help[i].name, g_cmd_help[i].help_info);
        }
        return;
    }

    for (i = 0; i < cmd_num; i++) {
        if (0 == strcmp(cmd, g_cmd_help[i].name)) {
            HI_TUNER_INFO("%s", g_cmd_help[i].help_info);
        }
    }
}

hi_void hi_frontend_select_current_port(const char *args)
{
    hi_s32 ret;

    if (args == HI_NULL_PTR) {
        return;
    }

    if (g_frontend_num <= 1) {
        HI_TUNER_ERR("HI_FRONTEND_NUMBER:%d,so this cmd is invalid.\n", g_frontend_num);
        return;
    }

    g_frontend_port = atoi(args);
    if (g_frontend_port >= g_frontend_num) {
        g_frontend_port %= g_frontend_num;
    }

    ret = hi_adp_dmx_attach_ts_port(0, g_frontend_port);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call hi_adp_dmx_attach_ts_port failed 0x%x\n", ret);
        return;
    }

    HI_TUNER_INFO("switch to port %d\n", g_frontend_port);
}

static hi_s32 hi_frontend_getplpinfo()
{
    hi_s32 ret;
    hi_u8 plp_num = 0;
    hi_unf_frontend_dvbt2_plp_para plp_para[16] = {0}; /* 16: max plp number */
    hi_unf_frontend_dvbt2_plp_info plp_info[16] = {0}; /* 16: max plp number */
    hi_u8 i, j, data_plp_num;

    ret = hi_unf_frontend_get_plp_num(g_frontend_port, &plp_num);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("hi_unf_frontend_get_plp_num error.\n");
        return ret;
    }

    if (plp_num == 0) {
        return HI_SUCCESS;
    }

    for (i = 0; i < plp_num; i++) {
        ret = hi_unf_frontend_get_plp_info(g_frontend_port, i, &plp_info[i]);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("hi_unf_frontend_get_plp_info error.\n");
        }
    }

    /* print all plp */
    HI_TUNER_INFO("plp id       group plp id       plp type\n");
    for (i = 0; i < plp_num; i++) {
        HI_TUNER_INFO("%d              %d                  %d\n",
            plp_info[i].plp_id, plp_info[i].plp_grp_id, plp_info[i].plp_type);
    }
    HI_TUNER_INFO("---------------------------------------------\n");

    /* find all data plp */
    data_plp_num = 0;
    for (i = 0; i < plp_num; i++) {
        if (plp_info[i].plp_type == HI_UNF_FRONTEND_DVBT2_PLP_TYPE_COM) {
            continue;
        }
        plp_para[data_plp_num].plp_id = plp_info[i].plp_id;

        /* find common plp id */
        for (j = 0; j < plp_num; j++) {
            if ((plp_info[i].plp_grp_id == plp_info[j].plp_grp_id) &&
                (plp_info[j].plp_type == HI_UNF_FRONTEND_DVBT2_PLP_TYPE_COM)) {
                plp_para[data_plp_num].comm_plp_id = plp_info[j].plp_id;
                plp_para[data_plp_num].combination = 1;
                break;
            }
        }
        data_plp_num++;
    }

    HI_TUNER_INFO("plp id       common plp id       combination\n");
    for (i = 0; i < data_plp_num; i++) {
        HI_TUNER_INFO("%d              %d                  %d\n",
            plp_para[i].plp_id, plp_para[i].comm_plp_id, plp_para[i].combination);
    }

    return HI_SUCCESS;
}

static hi_void print_connect_result_sat(hi_s32 ret, hi_bool time_out, hi_u32 time_ms)
{
    hi_u32 freq, symbol_rate;
    freq = g_connect_para[g_frontend_port].connect_para.sat.freq;
    symbol_rate = g_connect_para[g_frontend_port].connect_para.sat.symbol_rate;
    if (time_out) {
        HI_TUNER_INFO("Tuner Lock freq %d symb %d  Fail!\n", freq, symbol_rate);
        strcpy(g_test_result, HI_RESULT_FAIL);
        HI_TUNER_INFO("FAIL end\n");
    } else if (ret == HI_SUCCESS) {
        HI_TUNER_INFO("Tuner   Lock freq %d symb %d Success! Use %dms\n", freq, symbol_rate, time_ms);
        strcpy(g_test_result, HI_RESULT_SUCCESS);
        HI_TUNER_INFO("SUCCESS end\n");
    } else {
        HI_TUNER_INFO("Tuner Lock freq %d symb %d Fail!, ret = 0x%x\n", freq, symbol_rate, ret);
        strcpy(g_test_result, HI_RESULT_FAIL);
        HI_TUNER_INFO("FAIL end\n");
    }
}

static hi_void print_connect_result_ter(hi_s32 ret, hi_bool time_out, hi_u32 time_ms)
{
    hi_u32 freq, bw;
    freq = g_connect_para[g_frontend_port].connect_para.ter.freq;
    bw   = g_connect_para[g_frontend_port].connect_para.ter.band_width / 1000; /* 1000:KHz to MHz */
    if (time_out) {
        HI_TUNER_INFO("Tuner Lock freq %d bandwidth %d Fail!\n", freq, bw);
        strcpy(g_test_result, HI_RESULT_FAIL);
        HI_TUNER_INFO("FAIL end\n");
    } else if (ret == HI_SUCCESS) {
        HI_TUNER_INFO("Tuner   Lock freq %d bandwidth %d Success! Use %dms\n", freq, bw, time_ms);
        strcpy(g_test_result, HI_RESULT_SUCCESS);
        HI_TUNER_INFO("SUCCESS end\n");
    } else {
        HI_TUNER_INFO("Tuner Lock freq %d bandwidth %d Fail!, ret = 0x%x\n", freq, bw, ret);
        strcpy(g_test_result, HI_RESULT_FAIL);
        HI_TUNER_INFO("FAIL end\n");
    }
}

static hi_void print_connect_result_cab(hi_s32 ret, hi_bool time_out, hi_u32 time_ms)
{
    hi_u32 freq, symbol_rate, mod_type;
    freq = g_connect_para[g_frontend_port].connect_para.cab.freq;
    symbol_rate = g_connect_para[g_frontend_port].connect_para.cab.symbol_rate;
    if (g_connect_para[g_frontend_port].connect_para.cab.mod_type == HI_UNF_MOD_TYPE_QAM_16) {
        mod_type = 16; /* 16 QAM */
    } else if (g_connect_para[g_frontend_port].connect_para.cab.mod_type == HI_UNF_MOD_TYPE_QAM_32) {
        mod_type = 32; /* 32 QAM */
    } else if (g_connect_para[g_frontend_port].connect_para.cab.mod_type == HI_UNF_MOD_TYPE_QAM_64) {
        mod_type = 64; /* 64 QAM */
    } else if (g_connect_para[g_frontend_port].connect_para.cab.mod_type == HI_UNF_MOD_TYPE_QAM_128) {
        mod_type = 128; /* 128 QAM */
    } else if (g_connect_para[g_frontend_port].connect_para.cab.mod_type == HI_UNF_MOD_TYPE_QAM_256) {
        mod_type = 256; /* 256 QAM */
    } else {
        mod_type = 64; /* 64 QAM */
    }
    if (time_out) {
        HI_TUNER_INFO("Tuner Lock freq %d symb %d qam %d  Fail!\n",
            freq, symbol_rate, mod_type);
        strcpy(g_test_result, HI_RESULT_FAIL);
        HI_TUNER_INFO("FAIL end\n");
    } else if (ret == HI_SUCCESS) {
        HI_TUNER_INFO("Tuner   Lock freq %d symb %d qam %d Success! Use %dms\n",
            freq, symbol_rate, mod_type, time_ms);
        strcpy(g_test_result, HI_RESULT_SUCCESS);
        HI_TUNER_INFO("SUCCESS end\n");
    } else {
        HI_TUNER_INFO("Tuner Lock freq %d symb %d qam %d  Fail!, ret = 0x%x\n",
            freq, symbol_rate, mod_type, ret);
        strcpy(g_test_result, HI_RESULT_FAIL);
        HI_TUNER_INFO("FAIL end\n");
    }
}

static hi_void print_connect_result(hi_s32 ret, hi_bool time_out, hi_u32 time_ms)
{
    switch (g_connect_para[g_frontend_port].sig_type) {
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S:
            print_connect_result_sat(ret, time_out, time_ms);
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_C:
        case HI_UNF_FRONTEND_SIG_TYPE_J83B:
            print_connect_result_cab(ret, time_out, time_ms);
            break;
        default: /* TER signal type */
            print_connect_result_ter(ret, time_out, time_ms);
            break;
    }
}

/* lock freq function */
static hi_s32 hi_frontend_connect(hi_void)
{
    hi_s32 ret;
    hi_u32 time_out = 1000; /* 400:defalue time out */
    hi_u32 start_time, end_time;

    ret = hi_unf_frontend_get_default_time_out(g_frontend_port, &(g_connect_para[g_frontend_port]), &time_out);
    if (ret != HI_SUCCESS) {
        print_connect_result(ret, HI_FALSE, 0);
        return ret;
    }

    hi_unf_sys_get_time_stamp(&start_time);
    ret = hi_unf_frontend_connect(g_frontend_port, &(g_connect_para[g_frontend_port]), time_out);
    if (ret != HI_SUCCESS) {
        print_connect_result(ret, HI_FALSE, 0);
        return ret;
    }

    /* signal lock success */
    hi_unf_sys_get_time_stamp(&end_time);
    print_connect_result(ret, HI_FALSE, end_time - start_time);

    /* Some demodulation automatically lock T and T2. So the PLP info must be obtained for T and T2. */
    if ((g_connect_para[g_frontend_port].sig_type & HI_UNF_FRONTEND_SIG_TYPE_DVB_T2) ==
        HI_UNF_FRONTEND_SIG_TYPE_DVB_T2) {
        ret = hi_frontend_getplpinfo();
        return ret;
    }

    return HI_SUCCESS;
}

static hi_void search_pmt(hi_void)
{
    hi_s32 ret;
    hi_unf_avplay_stop_opt stop_opt;

    if (!g_parse_ts) {
        return;
    }
    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_STILL;
    stop_opt.timeout = 0;
    ret = hi_unf_avplay_stop(g_disp_handle.avplay,
        HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call HI_UNF_AVPLAY_Stop failed.\n");
    }

    hi_adp_search_init();
    hi_adp_search_free_all_pmt(g_prog_tbl[g_frontend_port]);
    g_prog_tbl[g_frontend_port] = HI_NULL;
    ret = hi_adp_search_get_all_pmt(0, &g_prog_tbl[g_frontend_port]);
    if (ret != HI_SUCCESS) {
        if (g_prog_tbl[g_frontend_port] != HI_NULL) {
            hi_adp_search_free_all_pmt(g_prog_tbl[g_frontend_port]);
            g_prog_tbl[g_frontend_port] = HI_NULL;
        }
        HI_TUNER_ERR("call HIADP_Search_GetAllPmt failed\n");
        HI_TUNER_INFO("FAIL end\n");
    } else {
        HI_TUNER_INFO("SUCCESS end\n");
    }

    ret = hi_unf_avplay_start(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, NULL);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_start audio failed.\n");
    }

    ret = hi_unf_avplay_start(g_disp_handle.avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, NULL);
    if (ret != HI_SUCCESS) {
        HI_TUNER_WARN("call hi_unf_avplay_start video failed.\n");
    }
}

static hi_unf_modulation_type convert_qam_type(hi_u32 qam)
{
    switch (qam) {
        case 16: /* 16 QAM */
            return HI_UNF_MOD_TYPE_QAM_16;
        case 32: /* 32 QAM */
            return HI_UNF_MOD_TYPE_QAM_32;
        case 64: /* 64 QAM */
            return HI_UNF_MOD_TYPE_QAM_64;
        case 128: /* 128 QAM */
            return HI_UNF_MOD_TYPE_QAM_128;
        case 256: /* 256 QAM */
            return HI_UNF_MOD_TYPE_QAM_256;
        default:
            return HI_UNF_MOD_TYPE_QAM_64;
    }
}

/* set channel freqency/symbolrate/polar and call hi_frontend_connect */
hi_void hi_frontend_setchannel(const char *channel)
{
    hi_u32 freq;
    hi_u32 para[5] = {0}; /* 5:index */
    hi_s32 ret;

    if (g_frontend_num == 0) {
        search_pmt();
        return;
    } else if (channel == HI_NULL_PTR) {
        return;
    }

    ret = sscanf(channel,  "%d" TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d"
        TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d",
        &freq, &para[0], &para[1], &para[2], &para[3], &para[4]); /* 2,3,4:index */
    if (ret < 2) { /* 2 parameters */
        HI_TUNER_INFO("setchnl needs at least 2 parameters\n");
        return;
    }

    if ((g_connect_para[g_frontend_port].sig_type >= HI_UNF_FRONTEND_SIG_TYPE_DVB_C) &&
        (g_connect_para[g_frontend_port].sig_type <= HI_UNF_FRONTEND_SIG_TYPE_J83B)) {
        g_connect_para[g_frontend_port].connect_para.cab.freq = freq;
        g_connect_para[g_frontend_port].connect_para.cab.symbol_rate = para[0] * 1000; /* 1000:KHz to Hz */
        g_connect_para[g_frontend_port].connect_para.cab.mod_type = convert_qam_type(para[1]);
    } else if ((g_connect_para[g_frontend_port].sig_type >= HI_UNF_FRONTEND_SIG_TYPE_DVB_S) &&
        (g_connect_para[g_frontend_port].sig_type <= HI_UNF_FRONTEND_SIG_TYPE_DVB_S2X)) {
        g_connect_para[g_frontend_port].connect_para.sat.freq = freq * 1000; /* 1000:MHz to KHz */
        g_connect_para[g_frontend_port].connect_para.sat.symbol_rate = para[0];
        g_connect_para[g_frontend_port].connect_para.sat.polar = para[1];
    } else if ((g_connect_para[g_frontend_port].sig_type == HI_UNF_FRONTEND_SIG_TYPE_ISDB_S) ||
        (g_connect_para[g_frontend_port].sig_type == HI_UNF_FRONTEND_SIG_TYPE_ISDB_S3)) {
        g_connect_para[g_frontend_port].connect_para.sat.freq = freq * 1000; /* 1000:MHz to KHz */
        g_connect_para[g_frontend_port].connect_para.sat.polar = para[0];
        g_connect_para[g_frontend_port].connect_para.sat.stream_id = para[1];
        g_connect_para[g_frontend_port].connect_para.sat.stream_id_type = para[2]; /* 2:stream ID type */
    } else {
        g_connect_para[g_frontend_port].connect_para.ter.freq = freq;
        g_connect_para[g_frontend_port].connect_para.ter.band_width = para[0];
        g_connect_para[g_frontend_port].connect_para.ter.channel_mode = para[1];
        g_connect_para[g_frontend_port].connect_para.ter.dvbt_prio = para[1] + 1;

        /*
         * config which data plp and common plp are output after frequency is locked,
         * if common plp is not exist,set u32ComPLPID = 0 and u32PLPComb =0.
         * else set data plp index to u32PLPID,
         * set common plp index to u32ComPLPID,
         * set 1 to u32PLPComb,it means enable data plp and common plp combined.
         */
        g_connect_para[g_frontend_port].connect_para.ter.plp_param.plp_id = para[2]; /* 2:PLP ID */
        g_connect_para[g_frontend_port].connect_para.ter.plp_param.comm_plp_id = para[3]; /* 3:common PLP ID */
        g_connect_para[g_frontend_port].connect_para.ter.plp_param.combination = para[4]; /* 4:combination */
    }

    ret = hi_frontend_connect();
    if (ret != HI_SUCCESS) {
        return;
    }

    search_pmt();
}

static hi_void print_video_scrambled(pmt_compact_prog *prog)
{
    hi_s32 ret;
    hi_handle dmx_vid_chn = HI_INVALID_HANDLE;
    hi_unf_dmx_scrambled_flag scramble_flag;

    if (prog->v_element_pid == 0) {
        /* Don't have video, only audio */
        HI_TUNER_INFO("Only radio program\n");
    } else {
        /* Judge video Scrambled */
        ret = hi_unf_avplay_get_demux_handle(g_disp_handle.avplay, HI_UNF_AVPLAY_DEMUX_HANDLE_VID, &dmx_vid_chn);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Get Dmx Vid channel failed.\n");
            return;
        }

        ret = hi_unf_dmx_get_play_chan_scrambled_flag(dmx_vid_chn, &scramble_flag);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Get srambled flag failed.\n");
            return;
        }

        if ((scramble_flag == HI_UNF_DMX_SCRAMBLED_FLAG_TS) || (scramble_flag == HI_UNF_DMX_SCRAMBLED_FLAG_PES)) {
            HI_TUNER_INFO("\033[1;31;40m" "Video: Scrambled" "\033[0m\n");
        } else {
            HI_TUNER_INFO("Video: Unscrambled\n");
        }
    }
}

static hi_void print_audio_scrambled(pmt_compact_prog *prog)
{
    hi_s32 ret;
    hi_handle dmx_aud_chn = HI_INVALID_HANDLE;
    hi_unf_dmx_scrambled_flag scramble_flag;

    if (prog->a_element_pid == 0) {
        /* Don't have audio, only video */
        HI_TUNER_INFO("Audio: None\n");
    } else {
        /* Judge audio Scrambled */
        ret = hi_unf_avplay_get_demux_handle(g_disp_handle.avplay, HI_UNF_AVPLAY_DEMUX_HANDLE_AUD, &dmx_aud_chn);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Get Dmx Aud channel failed.\n");
            return;
        }

        ret = hi_unf_dmx_get_play_chan_scrambled_flag(dmx_aud_chn, &scramble_flag);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Get srambled flag failed.\n");
            return;
        }

        if ((scramble_flag == HI_UNF_DMX_SCRAMBLED_FLAG_TS) || (scramble_flag == HI_UNF_DMX_SCRAMBLED_FLAG_PES)) {
            HI_TUNER_INFO("\033[1;31;40m" "Audio: Scrambled" "\033[0m\n");
        } else {
            HI_TUNER_INFO("Audio: Unscrambled\n");
        }
    }
}

/* play program */
hi_void hi_frontend_play(const char *args)
{
    hi_s32 ret;
    hi_s32 prog_index = 1;
    pmt_compact_tbl *prog_tbl = g_prog_tbl[g_frontend_port];

    if (prog_tbl == HI_NULL) {
        HI_TUNER_INFO("Can't find a PMT(Program Map Table) yet.\n");
        return;
    }

    if (args == HI_NULL_PTR) {
        /* Select 1 by default */
        prog_index = 1;
    } else {
        ret = sscanf(args, "%d", &prog_index);
        if (prog_index <= 0 || prog_index > prog_tbl->prog_num) {
            HI_TUNER_ERR("Invalid input %d\nMax Program number is %d\n", prog_index, prog_tbl->prog_num);
            return;
        }
    }

    /* Translate the number into the index of s_ProgTbl->proginfo[]. */
    prog_index -= 1;
    ret = hi_adp_avplay_play_prog(g_disp_handle.avplay, prog_tbl, prog_index,
                                  HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call HIADP_AVPlay_PlayProg failed.\n");
        return ;
    }

    HI_TUNER_INFO("Play u32Vpid 0x%x u32Apid 0x%x\n",
        prog_tbl->proginfo[prog_index].v_element_pid,
        prog_tbl->proginfo[prog_index].a_element_pid);

    HI_TUNER_INFO("SUCCESS end\n");
    strcpy(g_test_result, HI_RESULT_SUCCESS);

    HI_TUNER_INFO("\n********************\n");
    print_video_scrambled(&prog_tbl->proginfo[prog_index]);
    print_audio_scrambled(&prog_tbl->proginfo[prog_index]);
    HI_TUNER_INFO("********************\n");
}

static hi_void hi_frontend_getsignalinfo_fec(hi_unf_frontend_sig_type sig_type, hi_unf_frontend_fec_rate fec_rate)
{
    const char *fec_str_table[] = {
        "Unknown",  "1/2",      "2/3",      "3/4",      "4/5",      "5/6",      "6/7",
        "7/8",      "8/9",      "9/10",     "1/4",      "1/3",      "2/5",      "3/5",
        "13/45",    "9/20",     "11/20",    "5/9L",     "26/45L",   "23/36",    "25/36",
        "13/18",    "1/2L",     "8/15L",    "26/45",    "3/5L",     "28/45",    "2/3L",
        "7/9",      "77/90",    "32/45",    "11/15",    "32/45",    "29/45L",   "31/45",
        "11/15L",   "11/45",    "4/15",     "14/45",    "7/15",     "8/15",     "2/9",
        "2/9"
    };

    if (sig_type == HI_UNF_FRONTEND_SIG_TYPE_ISDB_T) {
        HI_TUNER_INFO("\tFEC rate:\t");
    } else {
        HI_TUNER_INFO("FEC rate:\t\t");
    }

    if (sig_type == HI_UNF_FRONTEND_SIG_TYPE_DVB_S &&
        fec_rate == HI_UNF_FRONTEND_FEC_RATE_AUTO) {
            HI_TUNER_INFO("Dummy\n");
    } else if (fec_rate < sizeof(fec_str_table) / sizeof(fec_str_table[0])) {
        HI_TUNER_INFO("%s\n", fec_str_table[fec_rate]);
    } else {
        HI_TUNER_INFO("Unknown\n");
    }
}

static hi_void hi_frontend_getsignalinfo_sat_mod_type(hi_unf_modulation_type mod_type)
{
    switch (mod_type) {
        case HI_UNF_MOD_TYPE_BPSK:
            HI_TUNER_INFO("Modulation type: \tBPSK\n");
            break;
        case HI_UNF_MOD_TYPE_QPSK:
            HI_TUNER_INFO("Modulation type: \tQPSK\n");
            break;
        case HI_UNF_MOD_TYPE_8PSK:
            HI_TUNER_INFO("Modulation type: \t8PSK\n");
            break;
        case HI_UNF_MOD_TYPE_8APSK:
            HI_TUNER_INFO("Modulation type: \t8APSK\n");
            break;
        case HI_UNF_MOD_TYPE_16APSK:
            HI_TUNER_INFO("Modulation type: \t16APSK\n");
            break;
        case HI_UNF_MOD_TYPE_32APSK:
            HI_TUNER_INFO("Modulation type: \t32APSK\n");
            break;
        case HI_UNF_MOD_TYPE_64APSK:
            HI_TUNER_INFO("Modulation type: \t64APSK\n");
            break;
        case HI_UNF_MOD_TYPE_128APSK:
            HI_TUNER_INFO("Modulation type: \t128APSK\n");
            break;
        case HI_UNF_MOD_TYPE_256APSK:
            HI_TUNER_INFO("Modulation type: \t256APSK\n");
            break;
        case HI_UNF_MOD_TYPE_VLSNR_SET1:
            HI_TUNER_INFO("Modulation type: \tVLSNR_SET1\n");
            break;
        case HI_UNF_MOD_TYPE_VLSNR_SET2:
            HI_TUNER_INFO("Modulation type: \tVLSNR_SET2\n");
            break;
        default:
            HI_TUNER_INFO("Modulation type: \tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_sat_frame_mode(hi_unf_frontend_sat_fec_frame frame_mode)
{
    switch (frame_mode) {
        case HI_UNF_FRONTEND_SAT_FEC_FRAME_NORMAL:
            HI_TUNER_INFO("FEC frame:\t\tNormal\n");
            break;
        case HI_UNF_FRONTEND_SAT_FEC_FRAME_SHORT:
            HI_TUNER_INFO("FEC frame:\t\tShort\n");
            break;
        case HI_UNF_FRONTEND_SAT_FEC_FRAME_MEDIUM:
            HI_TUNER_INFO("FEC frame:\t\tMedium\n");
            break;
        default:
            HI_TUNER_INFO("FEC frame:\t\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_sat_roll_off(hi_unf_frontend_roll_off roll_off)
{
    switch (roll_off) {
        case HI_UNF_FRONTEND_ROLL_OFF_35:
            HI_TUNER_INFO("Roll off:\t\t0.35\n");
            break;
        case HI_UNF_FRONTEND_ROLL_OFF_25:
            HI_TUNER_INFO("Roll off:\t\t0.25\n");
            break;
        case HI_UNF_FRONTEND_ROLL_OFF_20:
            HI_TUNER_INFO("Roll off:\t\t0.20\n");
            break;
        case HI_UNF_FRONTEND_ROLL_OFF_15:
            HI_TUNER_INFO("Roll off:\t\t0.15\n");
            break;
        case HI_UNF_FRONTEND_ROLL_OFF_10:
            HI_TUNER_INFO("Roll off:\t\t0.10\n");
            break;
        case HI_UNF_FRONTEND_ROLL_OFF_05:
            HI_TUNER_INFO("Roll off:\t\t0.05\n");
            break;
        default:
            HI_TUNER_INFO("Roll off:\t\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_sat_stream(hi_unf_frontend_sat_stream_type stream)
{
    switch (stream) {
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_GENERIC_PACKETIZED:
            HI_TUNER_INFO("Stream type:\t\tGeneric packetized\n");
            break;
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_GENERIC_CONTINUOUS:
            HI_TUNER_INFO("Stream type:\t\tGeneric continous\n");
            break;
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_GSE_HEM:
            HI_TUNER_INFO("Stream type:\t\tGSE HEM\n");
            break;
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_TRANSPORT:
            HI_TUNER_INFO("Stream type:\t\tTransport\n");
            break;
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_GSE_LITE:
            HI_TUNER_INFO("Stream type:\t\tGSE Lite\n");
            break;
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_GSE_LITE_HEM:
            HI_TUNER_INFO("Stream type:\t\tGSE Lite HEM\n");
            break;
        case HI_UNF_FRONTEND_SAT_STREAM_TYPE_T2MI:
            HI_TUNER_INFO("Stream type:\t\tT2MI\n");
            break;
        default:
            HI_TUNER_INFO("Stream type:\t\tUnknow\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_dvbs2x(const hi_unf_frontend_dvbs2x_signal_info *dvbs2x)
{
    hi_frontend_getsignalinfo_sat_mod_type(dvbs2x->mod_type);
    hi_frontend_getsignalinfo_fec(HI_UNF_FRONTEND_SIG_TYPE_DVB_S, dvbs2x->fec_rate);

    hi_frontend_getsignalinfo_sat_frame_mode(dvbs2x->fec_frame_mode);
    switch (dvbs2x->code_modulation) {
        case HI_UNF_FRONTEND_CODE_MODULATION_VCM_ACM:
            HI_TUNER_INFO("Code and Modulation:\tVCM/ACM.\n");
            break;
        case HI_UNF_FRONTEND_CODE_MODULATION_MULTISTREAM:
            HI_TUNER_INFO("Code and Modulation:\tMulti stream.\n");
            break;
        case HI_UNF_FRONTEND_CODE_MODULATION_CCM:
        default:
            HI_TUNER_INFO("Code and Modulation:\tCCM\n");
            break;
    }
    hi_frontend_getsignalinfo_sat_roll_off(dvbs2x->roll_off);
    switch (dvbs2x->pilot) {
        case HI_UNF_FRONTEND_PILOT_OFF:
            HI_TUNER_INFO("Pilot:\t\t\tOFF\n");
            break;
        case HI_UNF_FRONTEND_PILOT_ON:
            HI_TUNER_INFO("Pilot:\t\t\tON\n");
            break;
        default:
            HI_TUNER_INFO("Pilot:\t\t\tUnknown\n");
            break;
    }
    hi_frontend_getsignalinfo_sat_stream(dvbs2x->stream_type);
}

static hi_void hi_frontend_getsignalinfo_ter_mod_type(hi_unf_modulation_type mode_type)
{
    switch (mode_type) {
        case HI_UNF_MOD_TYPE_QPSK:
            HI_TUNER_INFO("ModType:\t\tQPSK\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_16:
            HI_TUNER_INFO("ModType:\t\tQAM_16\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_32:
            HI_TUNER_INFO("ModType:\t\tQAM_32\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_64:
            HI_TUNER_INFO("ModType:\t\tQAM_64\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_128:
            HI_TUNER_INFO("ModType:\t\tQAM_128\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_256:
            HI_TUNER_INFO("ModType:\t\tQAM_256\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_512:
            HI_TUNER_INFO("ModType:\t\tQAM_512\n");
            break;
        case HI_UNF_MOD_TYPE_BPSK:
            HI_TUNER_INFO("ModType:\t\tBPSK\n");
            break;
        case HI_UNF_MOD_TYPE_DQPSK:
            HI_TUNER_INFO("ModType:\t\tDQPSK\n");
            break;
        case HI_UNF_MOD_TYPE_8PSK:
            HI_TUNER_INFO("ModType:\t\t8PSK\n");
            break;
        case HI_UNF_MOD_TYPE_16APSK:
            HI_TUNER_INFO("ModType:\t\t16APSK\n");
            break;
        case HI_UNF_MOD_TYPE_32APSK:
            HI_TUNER_INFO("ModType:\t\t32APSK\n");
            break;
        default:
            HI_TUNER_INFO("ModType:\t\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_guard_intv(hi_unf_frontend_guard_intv guard_intv)
{
    switch (guard_intv) {
        case HI_UNF_FRONTEND_GUARD_INTV_1_128:
            HI_TUNER_INFO("GI:\t\t\t1/128\n");
            break;
        case HI_UNF_FRONTEND_GUARD_INTV_1_32:
            HI_TUNER_INFO("GI:\t\t\t1/32\n");
            break;
        case HI_UNF_FRONTEND_GUARD_INTV_1_16:
            HI_TUNER_INFO("GI:\t\t\t1/16\n");
            break;
        case HI_UNF_FRONTEND_GUARD_INTV_1_8:
            HI_TUNER_INFO("GI:\t\t\t1/8\n");
            break;
        case HI_UNF_FRONTEND_GUARD_INTV_1_4:
            HI_TUNER_INFO("GI:\t\t\t1/4\n");
            break;
        case HI_UNF_FRONTEND_GUARD_INTV_19_128:
            HI_TUNER_INFO("GI:\t\t\t19/128\n");
            break;
        case HI_UNF_FRONTEND_GUARD_INTV_19_256:
            HI_TUNER_INFO("GI:\t\t\t19/256\n");
            break;
        default:
            HI_TUNER_INFO("GI:\t\t\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_fft(hi_unf_frontend_fft fft)
{
    switch (fft) {
        case HI_UNF_FRONTEND_FFT_1K:
            HI_TUNER_INFO("FFT Mode\t\t1K\n");
            break;
        case HI_UNF_FRONTEND_FFT_2K:
            HI_TUNER_INFO("FFT Mode\t\t2K\n");
            break;
        case HI_UNF_FRONTEND_FFT_4K:
            HI_TUNER_INFO("FFT Mode\t\t4K\n");
            break;
        case HI_UNF_FRONTEND_FFT_8K:
            HI_TUNER_INFO("FFT Mode\t\t8K\n");
            break;
        case HI_UNF_FRONTEND_FFT_16K:
            HI_TUNER_INFO("FFT Mode\t\t16K\n");
            break;
        case HI_UNF_FRONTEND_FFT_32K:
            HI_TUNER_INFO("FFT Mode\t\t32K\n");
            break;
        default:
            HI_TUNER_INFO("FFT Mode\t\tUnkown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_dvbt(const hi_unf_frontend_dvbt_signal_info *dvbt)
{
    hi_frontend_getsignalinfo_guard_intv(dvbt->guard_intv);
    hi_frontend_getsignalinfo_fft(dvbt->fft_mode);
    hi_frontend_getsignalinfo_fec(HI_UNF_FRONTEND_SIG_TYPE_DVB_T, dvbt->fec_rate);
    hi_frontend_getsignalinfo_ter_mod_type(dvbt->mod_type);
    switch (dvbt->hier_mod) {
        case HI_UNF_FRONTEND_DVBT_HIERARCHY_NO:
            HI_TUNER_INFO("HierMod:\t\tNONE\n");
            break;
        case HI_UNF_FRONTEND_DVBT_HIERARCHY_ALHPA1:
            HI_TUNER_INFO("HierMod:\t\tALHPA1\n");
            break;
        case HI_UNF_FRONTEND_DVBT_HIERARCHY_ALHPA2:
            HI_TUNER_INFO("HierMod:\t\tALHPA2\n");
            break;
        case HI_UNF_FRONTEND_DVBT_HIERARCHY_ALHPA4:
            HI_TUNER_INFO("HierMod:\t\tALHPA4\n");
            break;
        default:
            HI_TUNER_INFO("HierMod:\t\tUnknown\n");
            break;
    }

    switch (dvbt->ts_priority) {
        case HI_UNF_FRONTEND_DVBT_TS_PRIORITY_NONE:
            HI_TUNER_INFO("TsPriority:\t\tNONE\n");
            break;
        case HI_UNF_FRONTEND_DVBT_TS_PRIORITY_HP:
            HI_TUNER_INFO("TsPriority:\t\tHP\n");
            break;
        case HI_UNF_FRONTEND_DVBT_TS_PRIORITY_LP:
            HI_TUNER_INFO("TsPriority:\t\tLP\n");
            break;
        default:
            HI_TUNER_INFO("TsPriority:\t\tUnknown\n");
            break;
    }
    HI_TUNER_INFO("Cell ID:\t\t%d\n", dvbt->cell_id);
}

static hi_void hi_frontend_getsignalinfo_dvbt2_stream(hi_unf_frontend_dvbt2_stream_type stream)
{
    switch (stream) {
        case HI_UNF_FRONTEND_DVBT2_STREAM_TYPE_GFPS:
            HI_TUNER_INFO("Stream Type:\t\tGFPS\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_STREAM_TYPE_GCS:
            HI_TUNER_INFO("Stream Type:\t\tGCS\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_STREAM_TYPE_GSE:
            HI_TUNER_INFO("Stream Type:\t\tGSE\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_STREAM_TYPE_TS:
            HI_TUNER_INFO("Stream Type:\t\tTS\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_STREAM_TYPE_GSE_HEM:
            HI_TUNER_INFO("Stream Type:\t\tGSE_HEM\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_STREAM_TYPE_TS_HEM:
            HI_TUNER_INFO("Stream Type:\t\tTS_HEM\n");
            break;
        default:
            HI_TUNER_INFO("Stream Type:\t\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_dvbt2_plp_type(hi_unf_frontend_dvbt2_plp_type plp_type)
{
    switch (plp_type) {
        case HI_UNF_FRONTEND_DVBT2_PLP_TYPE_COM:
            HI_TUNER_INFO("Current PLP type:\tCommon\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_PLP_TYPE_DAT1:
            HI_TUNER_INFO("Current PLP type:\tType1\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_PLP_TYPE_DAT2:
            HI_TUNER_INFO("Current PLP type:\tType2\n");
            break;
        default:
            HI_TUNER_INFO("Current PLP type:\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_dvbt2_issy(hi_unf_frontend_dvbt2_issy issy)
{
    switch (issy) {
        case HI_UNF_FRONTEND_DVBT2_ISSY_NO:
            HI_TUNER_INFO("ISSY:\t\t\tNO\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_ISSY_LONG:
            HI_TUNER_INFO("ISSY:\t\t\tLong\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_ISSY_SHORT:
            HI_TUNER_INFO("ISSY:\t\t\tShort\n");
            break;
        default:
            HI_TUNER_INFO("ISSY:\t\t\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_dvbt2(const hi_unf_frontend_dvbt2_signal_info *dvbt2)
{
    hi_char *fft_mode_str[] = {"1K", "2K", "4K", "8K", "16K", "32K", "64K"};

    hi_frontend_getsignalinfo_guard_intv(dvbt2->guard_intv);

    HI_TUNER_INFO("FFTMode:\t\t");
    if (dvbt2->fft_mode <= 0 || dvbt2->fft_mode >= sizeof(fft_mode_str) / sizeof(fft_mode_str[0])) {
        HI_TUNER_INFO("Unknown\n");
    } else {
        if (dvbt2->carrier_mode == HI_UNF_FRONTEND_DVBT2_CARRIER_EXTEND) {
            HI_TUNER_INFO("%s Ext\n", fft_mode_str[dvbt2->fft_mode - 1]);
        } else {
            HI_TUNER_INFO("%s\n", fft_mode_str[dvbt2->fft_mode - 1]);
        }
    }

    hi_frontend_getsignalinfo_fec(HI_UNF_FRONTEND_SIG_TYPE_DVB_T, dvbt2->fec_rate);
    hi_frontend_getsignalinfo_ter_mod_type(dvbt2->mod_type);
    if (dvbt2->pilot_pattern < HI_UNF_FRONTEND_DVBT2_PILOT_PATTERN_MAX) {
        HI_TUNER_INFO("Pilot Pattern:\t\tPP%d\n", dvbt2->pilot_pattern + 1);
    } else {
        HI_TUNER_INFO("Pilot Pattern:\t\tUnknown\n");
    }
    hi_frontend_getsignalinfo_dvbt2_plp_type(dvbt2->plp_type);
    switch (dvbt2->channel_mode) {
        case HI_UNF_FRONTEND_DVBT2_MODE_BASE:
            HI_TUNER_INFO("BaseLite:\t\tBase\n");
            break;
        case HI_UNF_FRONTEND_DVBT2_MODE_LITE:
            HI_TUNER_INFO("BaseLite:\t\tLite\n");
            break;
        default:
            HI_TUNER_INFO("BaseLite:\t\tUnknown\n");
            break;
    }
    hi_frontend_getsignalinfo_dvbt2_issy(dvbt2->dvbt2_issy);
    hi_frontend_getsignalinfo_dvbt2_stream(dvbt2->stream_type);
    HI_TUNER_INFO("Cell ID:\t\t%d\n", dvbt2->cell_id);
    HI_TUNER_INFO("Network ID:\t\t%d\n", dvbt2->network_id);
    HI_TUNER_INFO("System ID:\t\t%d\n", dvbt2->system_id);
}

static hi_void hi_frontend_getsignalinfo_isdbt_mod_type(hi_unf_modulation_type layer_mod_type)
{
    switch (layer_mod_type) {
        case HI_UNF_MOD_TYPE_QPSK:
            HI_TUNER_INFO("\tModType:\tQPSK\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_16:
            HI_TUNER_INFO("\tModType:\tQAM_16\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_64:
            HI_TUNER_INFO("\tModType:\tQAM_64\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_128:
            HI_TUNER_INFO("\tModType:\tQAM_128\n");
            break;
        case HI_UNF_MOD_TYPE_QAM_256:
            HI_TUNER_INFO("\tModType:\tQAM_256\n");
            break;
        default:
            HI_TUNER_INFO("\tModType:\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_isdbt_time_il(hi_unf_frontend_isdbt_time_interleaver time_il)
{
    switch (time_il) {
        case HI_UNF_FRONTEND_ISDBT_TIME_INTERLEAVER_0:
            HI_TUNER_INFO("\tHierMod:\tNONE\n");
            break;
        case HI_UNF_FRONTEND_ISDBT_TIME_INTERLEAVER_1:
            HI_TUNER_INFO("\tHierMod:\tALHPA1\n");
            break;
        case HI_UNF_FRONTEND_ISDBT_TIME_INTERLEAVER_2:
            HI_TUNER_INFO("\tHierMod:\tALHPA2\n");
            break;
        case HI_UNF_FRONTEND_ISDBT_TIME_INTERLEAVER_4:
            HI_TUNER_INFO("\tHierMod:\tALHPA4\n");
            break;
        case HI_UNF_FRONTEND_ISDBT_TIME_INTERLEAVER_8:
            HI_TUNER_INFO("\tHierMod:\tALHPA4\n");
            break;
        case HI_UNF_FRONTEND_ISDBT_TIME_INTERLEAVER_16:
            HI_TUNER_INFO("\tHierMod:\tALHPA4\n");
            break;
        default:
            HI_TUNER_INFO("\tHierMod:\tUnknown\n");
            break;
    }
}

static hi_void hi_frontend_getsignalinfo_isdbt_layer_a(hi_unf_frontend_isdbt_signal_info *isdbt)
{
    hi_unf_frontend_isdbt_time_interleaver  time_il;
    hi_unf_frontend_fec_rate    layer_fec_rate;
    hi_unf_modulation_type     layer_mod_type;
    if (isdbt->isdbt_layers.bits.layer_a_exist == 0) {
        HI_TUNER_INFO("LayerA is not exist.\n");
        return;
    }

    HI_TUNER_INFO("LayerA info:\n");
    HI_TUNER_INFO("\tLayerSegNum:\t%d\n", isdbt->isdbt_tmcc_info.isdbt_layers_a_info_bits.layer_seg_num);
    layer_mod_type = isdbt->isdbt_tmcc_info.isdbt_layers_a_info_bits.layer_mod_type;
    hi_frontend_getsignalinfo_isdbt_mod_type(layer_mod_type);
    layer_fec_rate = isdbt->isdbt_tmcc_info.isdbt_layers_a_info_bits.layer_fec_rate;
    hi_frontend_getsignalinfo_fec(HI_UNF_FRONTEND_SIG_TYPE_ISDB_T, layer_fec_rate);
    time_il = isdbt->isdbt_tmcc_info.isdbt_layers_a_info_bits.layer_time_interleaver;
    hi_frontend_getsignalinfo_isdbt_time_il(time_il);
}

static hi_void hi_frontend_getsignalinfo_isdbt_layer_b(hi_unf_frontend_isdbt_signal_info *isdbt)
{
    hi_unf_frontend_isdbt_time_interleaver  time_il;
    hi_unf_frontend_fec_rate    layer_fec_rate;
    hi_unf_modulation_type     layer_mod_type;
    if (isdbt->isdbt_layers.bits.layer_b_exist == 0) {
        HI_TUNER_INFO("LayerB is not exist.\n");
        return;
    }

    HI_TUNER_INFO("LayerB info:\n");
    HI_TUNER_INFO("\tLayerSegNum:\t%d\n", isdbt->isdbt_tmcc_info.isdbt_layers_b_info_bits.layer_seg_num);
    layer_mod_type = isdbt->isdbt_tmcc_info.isdbt_layers_b_info_bits.layer_mod_type;
    hi_frontend_getsignalinfo_isdbt_mod_type(layer_mod_type);
    layer_fec_rate = isdbt->isdbt_tmcc_info.isdbt_layers_b_info_bits.layer_fec_rate;
    hi_frontend_getsignalinfo_fec(HI_UNF_FRONTEND_SIG_TYPE_ISDB_T, layer_fec_rate);
    time_il = isdbt->isdbt_tmcc_info.isdbt_layers_b_info_bits.layer_time_interleaver;
    hi_frontend_getsignalinfo_isdbt_time_il(time_il);
}

static hi_void hi_frontend_getsignalinfo_isdbt_layer_c(hi_unf_frontend_isdbt_signal_info *isdbt)
{
    hi_unf_frontend_isdbt_time_interleaver  time_il;
    hi_unf_frontend_fec_rate    layer_fec_rate;
    hi_unf_modulation_type     layer_mod_type;
    if (isdbt->isdbt_layers.bits.layer_c_exist == 0) {
        HI_TUNER_INFO("LayerC is not exist.\n");
        return;
    }

    HI_TUNER_INFO("LayerC info:\n");
    HI_TUNER_INFO("\tLayerSegNum:\t%d\n", isdbt->isdbt_tmcc_info.isdbt_layers_c_info_bits.layer_seg_num);
    layer_mod_type = isdbt->isdbt_tmcc_info.isdbt_layers_c_info_bits.layer_mod_type;
    hi_frontend_getsignalinfo_isdbt_mod_type(layer_mod_type);
    layer_fec_rate = isdbt->isdbt_tmcc_info.isdbt_layers_c_info_bits.layer_fec_rate;
    hi_frontend_getsignalinfo_fec(HI_UNF_FRONTEND_SIG_TYPE_ISDB_T, layer_fec_rate);
    time_il = isdbt->isdbt_tmcc_info.isdbt_layers_c_info_bits.layer_time_interleaver;
    hi_frontend_getsignalinfo_isdbt_time_il(time_il);
}

static hi_void hi_frontend_getsignalinfo_isdbt(hi_unf_frontend_isdbt_signal_info *isdbt)
{
    hi_frontend_getsignalinfo_guard_intv(isdbt->guard_intv);
    hi_frontend_getsignalinfo_fft(isdbt->fft_mode);
    HI_TUNER_INFO("EmergencyFlag:\t\t%d\nPhaseShiftCorr:\t\t%d\nPartialFlag:\t\t%d\n",
        isdbt->isdbt_tmcc_info.emergency_flag,
        isdbt->isdbt_tmcc_info.phase_shift_corr,
        isdbt->isdbt_tmcc_info.partial_flag);

    hi_frontend_getsignalinfo_isdbt_layer_a(isdbt);
    hi_frontend_getsignalinfo_isdbt_layer_b(isdbt);
    hi_frontend_getsignalinfo_isdbt_layer_c(isdbt);
}

static hi_void hi_frontend_getsignalinfo_sig_type(hi_unf_frontend_sig_type sig_type)
{
    switch (sig_type) {
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_C:
            HI_TUNER_INFO("Signal type:\t\tDVB-C\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_J83B:
            HI_TUNER_INFO("Signal type:\t\tJ83B\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_T:
            HI_TUNER_INFO("Signal type:\t\tDVB-T\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_T2:
            HI_TUNER_INFO("Signal type:\t\tDVB-T2\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_T:
            HI_TUNER_INFO("Signal type:\t\tISDB-T\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ATSC_T:
            HI_TUNER_INFO("Signal type:\t\tATSC-T\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DTMB:
            HI_TUNER_INFO("Signal type:\t\tDTMB\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S:
            HI_TUNER_INFO("Signal type:\t\tDVB-S\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S2:
            HI_TUNER_INFO("Signal type:\t\tDVB-S2\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S2X:
            HI_TUNER_INFO("Signal type:\t\tDVB-S2X\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_C:
            HI_TUNER_INFO("Signal type:\t\tISDB-C\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_S:
            HI_TUNER_INFO("Signal type:\t\tISDB-S\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_S3:
            HI_TUNER_INFO("Signal type:\t\tISDB-S3\n");
            break;
        case HI_UNF_FRONTEND_SIG_TYPE_MAX:
        default:
            HI_TUNER_INFO("Signal type:\t\tUnknown\n");
            break;
    }
}

hi_void hi_frontend_getsignalinfo(const char *para)
{
    hi_s32 ret;
    hi_unf_frontend_signal_info info;
    hi_unf_frontend_scientific_num ber;
    hi_u32 signal_strength, signal_quality;
    hi_unf_frontend_integer_decimal snr;

    ret = hi_unf_frontend_get_ber(g_frontend_port, &ber);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("hi_unf_frontend_get_ber failed\n");
    } else {
        HI_TUNER_INFO("BER:\t\t\t%d.%03d*(E%d)\n", ber.integer_val, ber.decimal_val, ber.power);
    }
    ret = hi_unf_frontend_get_snr(g_frontend_port, &snr);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_GetSNR failed\n");
    } else {
        HI_TUNER_INFO("SNR:\t\t\t%d.%02d\n", snr.integer_val, snr.decimal_val);
    }

    ret = hi_unf_frontend_get_signal_strength(g_frontend_port, &signal_strength);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_GetSignalStrength failed\n");
    } else {
        HI_TUNER_INFO("Signal Strength:\t%d%%\n", signal_strength);
    }

    ret = hi_unf_frontend_get_signal_quality(g_frontend_port, &signal_quality);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_GetSignalQuality failed\n");
    } else {
        HI_TUNER_INFO("Signal Quality:\t\t%d%%\n", signal_quality);
    }

    ret = hi_unf_frontend_get_signal_info(g_frontend_port, &info);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("call HI_UNF_TUNER_GetSignalInfo failed.\n");
        HI_TUNER_INFO("End\n");
        return;
    }
    hi_frontend_getsignalinfo_sig_type(info.sig_type);

    if (info.sig_type >= HI_UNF_FRONTEND_SIG_TYPE_DVB_S &&
        info.sig_type <= HI_UNF_FRONTEND_SIG_TYPE_DVB_S2X) {
        hi_frontend_getsignalinfo_dvbs2x(&info.signal_info.dvbs2x);
    } else if (info.sig_type == HI_UNF_FRONTEND_SIG_TYPE_ISDB_T) {
        hi_frontend_getsignalinfo_isdbt(&info.signal_info.isdbt);
    } else if (info.sig_type == HI_UNF_FRONTEND_SIG_TYPE_DVB_T) {
        hi_frontend_getsignalinfo_dvbt(&info.signal_info.dvbt);
    } else if (info.sig_type == HI_UNF_FRONTEND_SIG_TYPE_DVB_T2) {
        hi_frontend_getsignalinfo_dvbt2(&info.signal_info.dvbt2);
    }
    HI_TUNER_INFO("End\n");
}

hi_void hi_frontend_getoffset()
{
    hi_u32 symb;
    hi_u32 freq;
    hi_s32 ret;

    ret = hi_unf_frontend_get_real_freq_symb(g_frontend_port, &freq, &symb);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_GetOffset failed\n");
        return;
    }
    HI_TUNER_INFO("freq = %d kHz, actul_symb = %d symbol/s\n", freq, symb);

    return;
}

hi_void hi_frontend_parse_ts(const char *arg)
{
    hi_s32 ret;

    if (arg == HI_NULL_PTR) {
        g_parse_ts = HI_TRUE;
        return;
    }

    ret = atoi(arg);
    if (ret == 0) {
        g_parse_ts = HI_FALSE;
    } else {
        g_parse_ts = HI_TRUE;
    }
}

hi_void hi_frontend_setlnbpower(const char *power)
{
    if (power == HI_NULL_PTR) {
        return;
    }

    if (HI_SUCCESS != hi_unf_frontend_set_lnb_power(g_frontend_port, atoi(power))) {
        HI_TUNER_ERR("Set LNB power fail:%d\n", atoi(power));
    }
}


/* Configurate LNB parameter */
hi_void hi_frontend_setlnb(const char *args)
{
    hi_unf_frontend_lnb_type lnb_type;
    hi_u32 low_lo_freq  = 0;
    hi_u32 high_lo_freq = 0;
    hi_unf_frontend_lnb_band band;
    hi_u32 src_no = 0;
    hi_u32 if_center_freq_mhz = 0;
    hi_unf_frontend_sat_posn sat_posn = HI_UNF_FRONTEND_SAT_POSN_A;
    hi_unf_frontend_lnb_config lnb_config;

    if (args == HI_NULL_PTR) {
        return;
    }

    sscanf(args, "%d" TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d"
        TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d",
        (hi_u32*)&lnb_type, &low_lo_freq, &high_lo_freq,
        (hi_u32*)&band, &src_no, &if_center_freq_mhz, (hi_u32*)&sat_posn);
    HI_TUNER_INFO("hi_frontend_setlnb Type %d, Low LO:%dMHz, High LO: %dMHz, Band %d \n",
           lnb_type, low_lo_freq, high_lo_freq, band);

    if ((g_connect_para[g_frontend_port].sig_type >= HI_UNF_FRONTEND_SIG_TYPE_DVB_S) &&
        (g_connect_para[g_frontend_port].sig_type <= HI_UNF_FRONTEND_SIG_TYPE_ABSS)) {
        lnb_config.lnb_type = lnb_type;
        lnb_config.low_lo  = low_lo_freq;
        lnb_config.high_lo = high_lo_freq;
        lnb_config.lnb_band = band;
        lnb_config.ub_num = src_no;
        lnb_config.ub_freq = if_center_freq_mhz;
        lnb_config.sat_posn = sat_posn;
    } else {
        HI_TUNER_WARN("Your Signal Type unsupport lnb config.\n");
    }

    if (HI_SUCCESS != hi_unf_frontend_set_lnb_config(g_frontend_port, &lnb_config)) {
        HI_TUNER_ERR("call HI_UNF_TUNER_SetLNBConfig failed.\n");
    }
}

hi_void hi_frontend_setisi(const char *isi)
{
    hi_s32 ret;
    hi_u32 isi_id;
    hi_u8  total_stream = 0;

    if (isi == HI_NULL_PTR) {
        return;
    }

    ret = hi_unf_frontend_get_sat_stream_num(g_frontend_port, &total_stream);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_GetSatTotalStream fail.\n");
    }
    if (total_stream == 0) {
        HI_TUNER_ERR("vcm streams number is 0.\n");
        return;
    }

    isi_id = atoi(isi);
    ret = hi_unf_frontend_set_sat_isi_id(g_frontend_port, (hi_u8)isi_id);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_SetSatIsiID fail.\n");
        return;
    }
    search_pmt();
}

hi_void hi_frontend_getisi(const char *nouse)
{
    hi_s32 ret;
    hi_u8  i;
    hi_u8  isi_id[32] = {0}; /* 32 max isi id number */
    hi_u8  total_stream = 0;

    ret = hi_unf_frontend_get_sat_stream_num(g_frontend_port, &total_stream);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_GetSatTotalStream fail.\n");
    }

    HI_TUNER_INFO("total vcm/acm stream number:%d\n", total_stream);
    HI_TUNER_INFO("STREAM\t\tISI ID\n");

    for (i = 0; i < total_stream; i++) {
        hi_unf_frontend_get_sat_isi_id(g_frontend_port, (hi_u8)i, &isi_id[i]);
        HI_TUNER_INFO("%d\t\t%d\n", i, isi_id[i]);
    }

    return;
}

hi_void hi_frontend_setscramble(const char *args)
{
    hi_u32 scramble;

    if (args == HI_NULL_PTR) {
        HI_TUNER_ERR("Invalid input.\n");
        return;
    }

    scramble = atoi(args);
    g_connect_para[g_frontend_port].connect_para.sat.scramble_value = scramble;
}

/* set plp id */
hi_void hi_frontend_setplpid(const char *plpid)
{
    hi_u32 plp_id = 0;
    hi_u32 com_plp_id = 0;
    hi_u32 combination = 0;
    hi_unf_frontend_dvbt2_plp_para plp_para = {0};

    if (plpid == HI_NULL_PTR) {
        return;
    }

    sscanf(plpid, "%d" TEST_ARGS_SPLIT "%d" TEST_ARGS_SPLIT "%d", &plp_id, &com_plp_id, &combination);

    plp_para.plp_id = plp_id;
    plp_para.comm_plp_id = com_plp_id;
    plp_para.combination = combination;
    if (HI_SUCCESS != hi_unf_frontend_set_plp_para(g_frontend_port, &plp_para)) {
        HI_TUNER_INFO("set plp para %d %d %d failed.\n",
            plp_para.plp_id, plp_para.comm_plp_id, plp_para.combination);
        return;
    }
    search_pmt();
}

hi_void hi_frontend_monitor_layer(const char *args)
{
    hi_unf_frontend_isdbt_receive_config layer_config;
    hi_s32 para;
    hi_s32 ret;

    if (args == HI_NULL_PTR) {
        return;
    }

    sscanf(args, "%d", &para);
    HI_TUNER_INFO("hi_frontend_monitor_ISDBT:%d\n", para);

    memset(&layer_config, 0, sizeof(hi_unf_frontend_isdbt_receive_config));
    layer_config.isdbt_layer = para;

    ret = hi_unf_frontend_isdbt_config_layer_receive(g_frontend_port, &layer_config);
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("HI_UNF_TUNER_MonitorISDBTLayer failed.\n");
    }
}

/* Standby test */
hi_void hi_frontend_standby(const char *args)
{
    hi_u32 para;
    hi_s32 ret = HI_SUCCESS;

    if (args == HI_NULL_PTR) {
        return;
    }

    sscanf(args, "%d", &para);
    HI_TUNER_INFO("hi_frontend_standby %d\n", para);

    if (para == 0) {
        ret = hi_unf_frontend_wake_up(g_frontend_port);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Tuner wake up failed.\n");
        }
    } else {
        ret = hi_unf_frontend_standby(g_frontend_port);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Tuner standby failed.\n");
        }
    }
}

static const hi_char* hi_show_signal_type(hi_unf_frontend_sig_type sig_type)
{
    switch (sig_type) {
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_C:
            return "DVBC ";
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S:
            return "DVBS ";
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S2:
            return "DVBS2 ";
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_S2X:
            return "DVBS2X ";
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_T:
            return "DVBT ";
        case HI_UNF_FRONTEND_SIG_TYPE_DVB_T2:
            return "DVBT2 ";
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_T:
            return "ISDBT ";
        case HI_UNF_FRONTEND_SIG_TYPE_ATSC_T:
            return "ATSCT ";
        case HI_UNF_FRONTEND_SIG_TYPE_DTMB:
            return "DTMB ";
        case HI_UNF_FRONTEND_SIG_TYPE_J83B:
            return "J83B ";
        case HI_UNF_FRONTEND_SIG_TYPE_ABSS:
            return "ABSS ";
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_C:
            return "ISDBC ";
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_S:
            return "ISDBS ";
        case HI_UNF_FRONTEND_SIG_TYPE_ISDB_S3:
            return "ISDBS3 ";
        default:
            return "Unknow ";
    }
    return "Unknow ";
}

static const hi_char* hi_show_demod_type(hi_unf_demod_dev_type demod_dev_type)
{
    hi_s32 demod_index;
    const hi_char* demod_name[] = {
        "3130I",        "3130E",        "J83B",        "AVL6211",
        "MXL101",       "MN88472",      "IT9170",      "IT9133",
        "HI3136",       "INVALID",      "MXL254",      "CXD2837",
        "HI3137",       "MXL214",       "TDA18280",    "INVALID",
        "INVALID",      "MXL251",       "INVALID",     "ATBM888X",
        "MN88473",      "MXL683",       "TP5001",      "HD2501",
        "AVL6381",      "MXL541",       "MXL581",      "MXL582",
        "INTERNAL0",    "CXD2856",      "CXD2857",     "CXD2878",
    };
    demod_index = demod_dev_type - HI_UNF_DEMOD_DEV_TYPE_3130I;
    if (demod_index >= 0 && demod_index < sizeof(demod_name) / sizeof(demod_name[0])) {
        return demod_name[demod_index];
    } else {
        return "Unknown";
    }
}

static const hi_char* hi_show_tuner_type(hi_unf_tuner_dev_type tuner_dev_type)
{
    const hi_char *tuner_name[] = {
        "Unsupport", "Unsupport", "Unsupport", "Unsupport", "TDA18250",
        "Unsupport", "Unsupport", "Unsupport", "R820C",     "MXL203",
        "AV2011",    "Unsupport", "Unsupport", "MXL603",    "Unsupport",
        "Unsupport", "Unsupport", "TDA18250B", "Unsupport", "RDA5815",
        "MXL254",    "Unsupport", "si2147",    "Rafael836", "MXL608",
        "MXL214",    "TDA18280",  "TDA182I5A", "SI2144",    "AV2018",
        "MXL251",    "Unsupport", "MXL601",    "MXL683",    "AV2026",
        "R850",      "R858",      "MXL541",    "MXL581",    "MXL582",
        "MXL661",    "RT720",     "CXD2871",   "SUT_PJ987"};

    hi_u32 tuner_num = sizeof(tuner_name) / sizeof(tuner_name[0]);

    if (tuner_dev_type < tuner_num) {
        return tuner_name[tuner_dev_type];
    } else {
        return "Unknown";
    }
}

hi_void hi_show_dev_info()
{
    hi_s32 i;
    hi_s32 ret;
    hi_unf_frontend_attr tuner_attr;
    const hi_char *signal_type = HI_NULL;
    const hi_char *demod_type = HI_NULL;
    const hi_char *tuner_type = HI_NULL;

    for (i = 0; i < g_frontend_num; i++) {
        ret = hi_unf_frontend_get_attr(i, &tuner_attr);
        if (ret != HI_SUCCESS) {
            HI_TUNER_ERR("Get tuner config failed.\n");
            return;
        }

        signal_type = hi_show_signal_type(tuner_attr.sig_type);
        demod_type = hi_show_demod_type(tuner_attr.demod_dev_type);
        tuner_type = hi_show_tuner_type(tuner_attr.tuner_dev_type);

        HI_TUNER_INFO("******************************************************************************\n");
        HI_TUNER_INFO("          |                      |                   |                       |\n");
        HI_TUNER_INFO("Channel %d |    Demod = %-10s| Tuner = %-10s| Signal Type = %-8s|\n",
            i, demod_type, tuner_type, signal_type);
        HI_TUNER_INFO("          |                      |                   |                       |\n");
        HI_TUNER_INFO("******************************************************************************\n");
    }
}

/* set of received command */
test_fun_s g_test_func[] = {
    { "setport",         hi_frontend_select_current_port },
    { "setchnl",         hi_frontend_setchannel },
    { "play",            hi_frontend_play },
    { "getsignalinfo",   hi_frontend_getsignalinfo },
    { "getoffset",       hi_frontend_getoffset},
    { "parsets",         hi_frontend_parse_ts},

    /* sat */
    { "setlnbpower",     hi_frontend_setlnbpower },
    { "setlnb",          hi_frontend_setlnb },
    { "setisi",          hi_frontend_setisi },
    { "getisi",          hi_frontend_getisi },
    { "setscram",        hi_frontend_setscramble },

    /* dvb-t/t2 */
    { "setplpid",        hi_frontend_setplpid },

    /* isdbt */
    { "monitorlayer",    hi_frontend_monitor_layer },

    { "standby",         hi_frontend_standby },
    { "help",            hi_showhelp }
};

/* search the handle function corresponding with the command in the character string */
test_func_proc get_fun_by_name(const char *name)
{
    hi_u32 i;
    hi_u32 fun_num = sizeof(g_test_func) / sizeof(g_test_func[0]);

    if (name == HI_NULL_PTR) {
        return HI_NULL_PTR;
    }

    for (i = 0; i < fun_num; i++) {
        if (0 == strcmp(name, g_test_func[i].name)) {
            return g_test_func[i].proc;
        }
    }

    return HI_NULL_PTR;
}

/* deals with the universal command */
hi_void procfun(const char *fun_args)
{
    char *arg_str = NULL;
    test_func_proc func = NULL;

    arg_str = strchr(fun_args, UDP_STR_SPLIT);
    if (arg_str != HI_NULL_PTR) {
        *arg_str = 0;
        arg_str += 1;
    }

    func = get_fun_by_name(fun_args);
    if (func != HI_NULL_PTR) {
        func(arg_str);
    } else {
        strcpy(g_test_result, HI_RESULT_FAIL" Can't find the function\n");
        HI_TUNER_ERR("Can't find the function  %s %s\n\n", fun_args, arg_str);
    }
}

hi_s32 main(hi_s32 argc, char *argv[])
{
    hi_s32 ret;
    hi_char recv_buf[MAX_CMD_BUFFER_LEN];
    hi_char *line_end = HI_NULL_PTR;

    /* init device */
    ret = dev_init();
    if (ret != HI_SUCCESS) {
        HI_TUNER_ERR("dev_init failed, ret=0x%08x\n", ret);
        return ret;
    }

    /* init avplay */
    ret = frontend_display_init();
    if (ret != HI_SUCCESS) {
        dev_deinit();
        HI_TUNER_ERR("avplay_init failed ret=0x%08x\n", ret);
        return ret;
    }

    /* print help */
    hi_showhelp(HI_NULL);
    hi_show_dev_info();

    /* recieve command */
    while (1) {
        SAMPLE_GET_INPUTCMD(recv_buf);
        line_end = strchr(recv_buf, '\n');
        if (line_end == recv_buf) {
            continue;
        } else if (line_end != HI_NULL_PTR) {
            *line_end = '\0';
        }

        if (strcmp(recv_buf, "exit") == 0) {
            break;
        }

        procfun(recv_buf);
    }

    /* deinit device */
    frontend_display_deinit();
    dev_deinit();

    return HI_SUCCESS;
}

