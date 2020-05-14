/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description: pq sample
 * Author: pq
 * Create: 2016-01-1
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hi_unf_pq.h"

#define PQ_BASEPARA_NUM         5
#define PQ_COLOR_TEMP_NUM       6
#define PQ_SIX_BASE__NUM        6

#define pq_printf(fmt...)   printf(fmt)

#define RANGECHECK(x, min, max)                          \
    do {                                                 \
        if (((x) < (min)) || ((max) < (x))) {                    \
            pq_printf("%d not in [%d, %d]\n", x, min, max); \
            pq_help_msg();                                   \
            return HI_FAILURE;                           \
        }                                               \
    } while (0)

static hi_s32 g_set_type = 0; /* 0:get;  1:set ; default: get */
static hi_s32 g_chan = 1;    /* 0:chan 0;  1: chan 1; default: chan 1 */
static hi_s32 g_image_mode = -1;
static hi_s32 g_bright = -1;
static hi_s32 g_contrast = -1;
static hi_s32 g_hue = -1;
static hi_s32 g_csc_color_temperation[PQ_COLOR_TEMP_NUM];
static hi_s32 g_saturation = -1;
static hi_s32 g_pq_six_base_color[PQ_SIX_BASE__NUM];
static hi_s32 g_fleshtone = -1;
static hi_s32 g_color_space_mode = -1;
static hi_s32 g_set_bypass = -1;
static hi_s32 g_pq_bypass = -1;
static hi_s32 g_set_demo = -1;
static hi_s32 g_set_demo_disp_mode = -1;
static hi_s32 g_demo_disp_mode = -1;
static hi_s32 g_demo_coordinate = -1;
static hi_s32 g_demo_en = -1;
static hi_s32 g_set_pq_module = -1;
static hi_s32 g_pq_module = -1;
static hi_s32 g_module_en = 1;
static hi_s32 g_strength = -1;
static hi_s32 g_image_type = -1;

static char *g_mdule_str[HI_UNF_PQ_MODULE_MAX] = {
    "HI_UNF_PQ_MODULE_ALL",
    "HI_UNF_PQ_MODULE_DEI",
    "HI_UNF_PQ_MODULE_FMD",
    "HI_UNF_PQ_MODULE_TNR",
    "HI_UNF_PQ_MODULE_SNR",
    "HI_UNF_PQ_MODULE_DB",
    "HI_UNF_PQ_MODULE_DM",
    "HI_UNF_PQ_MODULE_DS",
    "HI_UNF_PQ_MODULE_DC",
    "HI_UNF_PQ_MODULE_HSHARPNESS",
    "HI_UNF_PQ_MODULE_SHARPNESS",
    "HI_UNF_PQ_MODULE_CCCL",
    "HI_UNF_PQ_MODULE_COLOR_CORING",
    "HI_UNF_PQ_MODULE_BLUE_STRETCH",
    "HI_UNF_PQ_MODULE_GAMMA",
    "HI_UNF_PQ_MODULE_DCI",
    "HI_UNF_PQ_MODULE_COLOR",
    "HI_UNF_PQ_MODULE_IQTM",
    "HI_UNF_PQ_MODULE_SR",
    "HI_UNF_PQ_MODULE_WCG",
    "HI_UNF_PQ_MODULE_HDR",
    "HI_UNF_PQ_MODULE_HDR10PLUS",
    "HI_UNF_PQ_MODULE_SDR2HDR",
    "HI_UNF_PQ_MODULE_3DLUT",
    "HI_UNF_PQ_MODULE_COLOR_TEMP",
};

const char *g_pq_get_option = NULL;
const char *g_pq_set_option = NULL;
hi_unf_pq_image_mode g_pq_image_mode;

hi_s32 pq_sample_get_image_type(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_brightness(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_contrast(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_saturation(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_hue(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_csc_color_temperation(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_imagemode(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_fleshtone(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_six_base(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_color_mode(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_module_enable(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_module_strength(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_by_pass(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_demo_enable(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_demo_mode(hi_u32 index, int argc, char *arg[]);
hi_s32 pq_sample_get_demo_coor(hi_u32 index, int argc, char *arg[]);

typedef hi_s32 (*pfunproc)(hi_u32 index, int argc, char *arg[]);

typedef struct {
    hi_char* func_name;
    pfunproc proc;
} pq_sample_fun;

static pq_sample_fun g_pq_sample_info[] = {
    { "-imagetype", pq_sample_get_image_type },
    { "-brightness", pq_sample_get_brightness },
    { "-contrast", pq_sample_get_contrast },
    { "-saturation", pq_sample_get_saturation },
    { "-hue", pq_sample_get_hue },
    { "-csccolortemperation", pq_sample_get_csc_color_temperation },
    { "-imagemode", pq_sample_get_imagemode },
    { "-fleshtone", pq_sample_get_fleshtone },
    { "-sixbasecolor", pq_sample_get_six_base },
    { "-colormode", pq_sample_get_color_mode },
    { "-moduleenable", pq_sample_get_module_enable },
    { "-modulestrength", pq_sample_get_module_strength },
    { "-bypass", pq_sample_get_by_pass },
    { "-demoenable", pq_sample_get_demo_enable },
    { "-demomode", pq_sample_get_demo_mode },
    { "-dmcoor", pq_sample_get_demo_coor },
};

static void pq_help_msg(void)
{
    pq_printf("\n");
    pq_printf("PQ sample options:\n");
    pq_printf("-help                          Show this help message\n");
    pq_printf("-set             <option>      Set the PQ options\n");
    pq_printf("-get             <option>      Get the PQ options\n");
    pq_printf("-chan            <0/1>         Set the chan(0:SD/1:HD)\n");
    pq_printf("-imagetype      <0/1/2>         Set the chan(0:video and graph/1:graph/2:video)\n");
    pq_printf("-brightness      <0~1023>       Change the brightness\n");
    pq_printf("-contrast        <0~1023>       Change the contrast\n");
    pq_printf("-hue             <0~1023>       Change the hue\n");
    pq_printf("-saturation      <0~1023>       Change the saturation (0~1023)\n");
    pq_printf("-csc colortemperation       <0~1023> \
    Change the csc temperation (red_gain, green_gain, blue_gain, red_offset, green_offset, blue_offset)\n");
    pq_printf("-imagemode      <0/1/2>       Set Image Mode (0:fleshtone/1:sixbasecolor/3/colorspace)\n");
    pq_printf("-sixbasecolor    <0~1023> \
              Change the 6basecolorenhance (Red, Green, Blue, Cyan, Magenta, Yellow)\n");
    pq_printf("-fleshtone       <0/1/2/3>     Change the flesh tone (0:off/1:low/2:mid/3:high)\n");
    pq_printf("-colormode       <0/1/2/3>     Change the color mode (0:recommend/1:blue/2:green/3:BG)\n");

    pq_printf("-moduleenable <type> <enable> \
             Enable the pq module,     type(0:all/1:dei/2:fmd/3:tnr/4:snr/5:db/6:dm/7:ds/8:dc/9:hsharpen/10:sharpen/ \
    11:cccl/12:coco/13:bluestretch/14:gamma/15:dci/16:acm/17:iptm/18:sr/19:wcg/ \
    20:hdr/21:hdr10plus/22:sdr2hdr/23:3dlut/24:colortemp), enable(0:off/1:on)\n");

    pq_printf("-modulestrength     <type> <strength> \
    type(0:all/1:dei/2:fmd/3:tnr/4:snr/5:db/6:dm/7:ds/8:dc/9:hsharpen/10:sharpen/ \
    11:cccl/12:coco/13:bluestretch/14:gamma/15:dci/16:acm/17:iptm/18:sr/19:wcg/ \
    20:hdr/21:hdr10plus/22:sdr2hdr/23:3dlut/24:colortemp), strength(0~1023)\n");

    pq_printf("-demoenable     <type> <enable> \
    type(0:all/1:dei/2:fmd/3:tnr/4:snr/5:db/6:dm/7:ds/8:dc/9:hsharpen/10:sharpen/ \
    11:cccl/12:coco/13:bluestretch/14:gamma/15:dci/16:acm/17:iptm/18:sr/19:wcg/ \
    20:hdr/21:hdr10plus/22:sdr2hdr/23:3dlut/24:colortemp), enable(0:off/1:on)\n");

    pq_printf("-demomode     <type> <mode> type(0:all/1:dei/2:fmd/3:tnr/4:snr/ \
    5:db/6:dm/7:ds/8:dc/9:hsharpen/10:sharpen/11:cccl/12:coco/13:bluestretch/14:gamma/ \
    15:dci/16:acm/17:iptm/18:sr/19:wcg/20:hdr/21:hdr10plus/22:sdr2hdr/23:3dlut/24:colortemp), \
    mode(0:fixed right/1:fixed left/2:scroll right/3:scroll left)\n");

    pq_printf("-dmcoor     <type> <coor> \
    type(0:all/1:dei/2:fmd/3:tnr/4:snr/5:db/6:dm/7:ds/8:dc/9:hsharpen/10:sharpen/ \
    11:cccl/12:coco/13:bluestretch/14:gamma/15:dci/16:acm/17:iptm/18:sr/19:wcg/ \
    20:hdr/21:hdr10plus/22:sdr2hdr/23:3dlut/24:colortemp), coor(0~1023)\n");

    pq_printf("-bypass                  Set the default PQ configuration for video parameter test\n");
    pq_printf("-all                        Show all PQ options in current image mode\n");
    pq_printf("\n");
}

hi_s32 pq_set_image_mode(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_image_mode mode = (hi_unf_pq_image_mode)val;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    /* HD  high definition */
    ret = hi_unf_pq_set_image_mode(disp_chan, pq_type, mode);
    if (ret != HI_SUCCESS) {
        pq_printf("SetImageMode fail\n");
        return HI_FAILURE;
    }
    pq_printf("Chan %d  Type %d Set Image Mode:%d (0:natural/1:person/2:film/3:UD)\n", chan, type, val);
    pq_printf("Chan %d  Type %d Set Image Mode:%d (0:natural/1:person/2:film/3:UD)\n", disp_chan, pq_type, mode);
    return ret;
}

hi_s32 pq_set_pq_bypass(hi_bool enable)
{
    hi_s32 ret = hi_unf_pq_enable_bypass(enable);
    if (ret != HI_SUCCESS) {
        pq_printf("SetDefaultParam fail\n");
        return HI_FAILURE;
    }

    pq_printf("Set PQ default param %d success\n", enable);
    return ret;
}

hi_s32 pq_set_bright(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_set_brightness(disp_chan, pq_type, val);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_set_bright fail\n");
        return HI_FAILURE;
    }

    pq_printf("Chan %d Type %d set Brightness:%d\n", chan, type, val);
    pq_printf("Chan %d Type %d set Brightness:%d\n", disp_chan, pq_type, val);
    return ret;
}

hi_s32 pq_set_contrast(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_set_contrast(disp_chan, pq_type, val);
    if (ret != HI_SUCCESS) {
        pq_printf("set Contrast fail\n");
        return HI_FAILURE;
    }

    pq_printf("Chan %d Type %d set Contrast:%d\n", chan, type, val);
    pq_printf("Chan %d Type %d set Contrast:%d\n", disp_chan, pq_type, val);
    return ret;
}

hi_s32 pq_set_hue(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_set_hue(disp_chan, pq_type, val);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_set_hue fail\n");
        return HI_FAILURE;
    }

    pq_printf("Chan %d Type %d set Hue:%d\n", chan, type, val);
    pq_printf("Chan %d Type %d set Hue:%d\n", disp_chan, pq_type, val);
    return ret;
}

hi_s32 pq_set_saturation(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_set_saturation(disp_chan, pq_type, val);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_set_saturation fail\n");
        return HI_FAILURE;
    }

    pq_printf("Chan %d Type %d set Saturation:%d\n", chan, type, val);
    pq_printf("Chan %d Type %d set Saturation:%d\n", disp_chan, pq_type, val);
    return ret;
}

hi_s32 pq_set_six_base_color_enhance(hi_u32 chan, hi_u32 type, hi_s32 val[])
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_color_enhance color_enhance_param;
    color_enhance_param.mode = HI_UNF_PQ_COLOR_ENHANCE_SIX_BASE;
    color_enhance_param.color_gain.six_base_color.red = val[0];
    color_enhance_param.color_gain.six_base_color.green = val[1];
    color_enhance_param.color_gain.six_base_color.blue = val[2]; /* 2: index */
    color_enhance_param.color_gain.six_base_color.cyan = val[3]; /* 3: index */
    color_enhance_param.color_gain.six_base_color.magenta = val[4]; /* 4: index */
    color_enhance_param.color_gain.six_base_color.yellow = val[5]; /* 5: index */

    ret = hi_unf_pq_set_color_mode(disp_chan, pq_type, color_enhance_param);
    if (ret != HI_SUCCESS) {
        pq_printf("set 6BaseColorEnhance fail!\n");
        return HI_FAILURE;
    }

    pq_printf(" Chan %d Type %d set 6BaseColorEnhance:\nRed:%d \nGreen:%d \nBlue:%d \
    \nCyan:%d \nMagenta:%d \nYellow:%d\n",
        chan, type, val[0], val[1], val[2], val[3], val[4], val[5]); /* 2/3/4/5: index */

    pq_printf(" Chan %d Type %d set 6BaseColorEnhance:\nRed:%d \nGreen:%d \nBlue:%d \
    \nCyan:%d \nMagenta:%d \nYellow:%d\n",
        disp_chan, pq_type, val[0], val[1], val[2], val[3], val[4], val[5]); /* 2/3/4/5: index */

    return ret;
}

hi_s32 pq_set_fleshtone(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_color_enhance color_enhance_param;
    color_enhance_param.mode = HI_UNF_PQ_COLOR_ENHANCE_FLESHTONE;
    color_enhance_param.color_gain.fleshtone_gain = (hi_unf_pq_fleshtone_gain)val;

    ret = hi_unf_pq_set_color_mode(disp_chan, pq_type, color_enhance_param);
    if (ret != HI_SUCCESS) {
        pq_printf("set FleshTone fail!\n");
        return HI_FAILURE;
    }

    pq_printf("set FleshTone:%d (0:off/1:low/2:mid/3:high)\n", val);
    return ret;
}

hi_s32 pq_set_color_space_mode(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_color_enhance color_enhance_param;
    color_enhance_param.mode = HI_UNF_PQ_COLOR_ENHANCE_SPEC_MODE;
    color_enhance_param.color_gain.color_space_mode = (hi_unf_pq_color_space_mode)val;

    ret = hi_unf_pq_set_color_mode(disp_chan, pq_type, color_enhance_param);
    if (ret != HI_SUCCESS) {
        pq_printf("set ColorMode fail!\n");
        return HI_FAILURE;
    }

    pq_printf("set ColorMode:%d (0:recommend/1:blue/2:green/3:BG)\n", val);
    return ret;
}

hi_s32 pq_set_module_strength(hi_u32 chan, hi_u32 type, hi_u32 module, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_module pq_module = (hi_unf_pq_module)module;

    ret = hi_unf_pq_set_module_strength(disp_chan, pq_type, pq_module, val);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_set_module_strength fail\n");
        return HI_FAILURE;
    }

    pq_printf("set strength:%d\n", val);
    return HI_SUCCESS;
}

hi_s32 pq_set_demo_enable(hi_u32 chan, hi_u32 type, hi_u32 val, hi_bool enable)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_demo_param demo_param = {0};
    hi_unf_pq_module module;
    module = (hi_unf_pq_module)val;

    ret = hi_unf_pq_get_demo(disp_chan, pq_type, module, &demo_param);

    if (enable == HI_FALSE) {
        demo_param.mode = HI_UNF_PQ_DEMO_MODE_CLOSE;
    } else {
        demo_param.mode = HI_UNF_PQ_DEMO_MODE_FIXED_L;
    }

    ret = hi_unf_pq_set_demo(disp_chan, pq_type, module, demo_param);
    if (ret != HI_SUCCESS) {
        pq_printf("SetDemo fail\n");
        return HI_FAILURE;
    }

    pq_printf("Set Demo %d :%s (0:sharpness/1:dci/2:color/3:sr/4:tnr/5:dei/6:dbm/7:snr/8:all)\n", val,
        enable ? "on" : "off");
    return ret;
}

hi_s32 pq_set_demo_disp_mode(hi_u32 chan, hi_u32 type, hi_u32 val, hi_u32 mode)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_demo_mode pq_mode = (hi_unf_pq_demo_mode)mode;
    hi_unf_pq_demo_param demo_param = {0};
    hi_unf_pq_module module = (hi_unf_pq_module)val;

    ret = hi_unf_pq_get_demo(disp_chan, pq_type, module, &demo_param);

    demo_param.mode = pq_mode;
    ret = hi_unf_pq_set_demo(disp_chan, pq_type, module, demo_param);
    if (ret != HI_SUCCESS) {
        pq_printf("SetDemoMode fail\n");
        return HI_FAILURE;
    }

    pq_printf("Set Demo Disp Mode:%d (0:fixed right/1:fixed left/2:scroll right/3:scroll left)\n", val);
    return ret;
}

hi_s32 pq_set_demo_coordinate(hi_u32 chan, hi_u32 type, hi_u32 val, hi_u32 x)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_demo_param demo_param = {0};
    hi_unf_pq_module module = (hi_unf_pq_module)val;

    ret = hi_unf_pq_get_demo(disp_chan, pq_type, module, &demo_param);

    demo_param.x_position = x;
    ret = hi_unf_pq_set_demo(disp_chan, pq_type, module, demo_param);
    if (ret != HI_SUCCESS) {
        pq_printf("SetDemoCoordinate fail\n");
        return HI_FAILURE;
    }

    pq_printf("Set Demo Coordinate: %d \n", x);
    return ret;
}

hi_s32 pq_set_module_enable(hi_u32 chan, hi_u32 type, hi_u32 val, hi_u32 enable)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_module module;
    module = (hi_unf_pq_module)val;

    if (module >= HI_UNF_PQ_MODULE_MAX) {
        pq_printf("PQModule error\n");
        return HI_FAILURE;
    }

    ret = hi_unf_pq_set_module_enable(disp_chan, pq_type, module, enable);
    if (ret != HI_SUCCESS) {
        pq_printf("SetPQModule fail\n");
        return HI_FAILURE;
    }

    pq_printf("Set PQ Module %d :%s (0:all/1:dei/2:fmd/3:tnr/4:snr/5:db/6:dm/7:ds/8:dc/9:hsharpen/10:sharpen/ \
    11:cccl/12:coco/13:bluestretch/14:gamma/15:dci/16:acm/17:iptm/18:sr/19:wcg/ \
    20:hdr/21:hdr10plus/22:sdr2hdr/23:3dlut/24:colortemp)\n", val,
        enable ? "on" : "off");
    return ret;
}

hi_s32 pq_get_module_enable(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_module module;
    hi_bool enable;
    module = (hi_unf_pq_module)val;

    if (module  >= HI_UNF_PQ_MODULE_MAX) {
        pq_printf("PQModule error\n");
        return HI_FAILURE;
    }

    ret = hi_unf_pq_get_module_enable(disp_chan, pq_type, module, &enable);
    if (ret != HI_SUCCESS) {
        pq_printf("GetPQModule fail\n");
        return HI_FAILURE;
    }

    return enable;
}

/* --------------- get --------------------- */
static hi_s32 pq_get_image_mode(hi_u32 chan, hi_u32 type)
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_image_mode image_mode;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_get_image_mode(disp_chan, pq_type, &image_mode);

    return (hi_u32)image_mode;
}

static hi_s32 pq_get_bright(hi_u32 chan, hi_u32 type)
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_u32 brightness = 0;
    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_get_brightness(disp_chan, pq_type, &brightness);
    return brightness;
}

static hi_s32 pq_get_contrast(hi_u32 chan, hi_u32 type)
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_u32 contrast = 0;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_get_contrast(disp_chan, pq_type, &contrast);
    return contrast;
}

static hi_s32 pq_get_hue(hi_u32 chan, hi_u32 type)
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_u32 hue = 0;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_get_hue(disp_chan, pq_type, &hue);
    return hue;
}

static hi_s32 pq_get_saturation(hi_u32 chan, hi_u32 type)
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_u32 saturation = 0;
    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    hi_unf_pq_get_saturation(disp_chan, pq_type, &saturation);
    return saturation;
}

static hi_s32 pq_get_six_base_color_enhance(hi_u32 chan, hi_u32 type, hi_s32 val[])
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_s32 ret;
    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    if (val == HI_NULL) {
        return HI_FAILURE;
    }

    hi_unf_pq_color_enhance color_mode_param;
    color_mode_param.mode = HI_UNF_PQ_COLOR_ENHANCE_SIX_BASE;

    ret = hi_unf_pq_get_color_mode(disp_chan, pq_type, &color_mode_param);
    if (ret != HI_SUCCESS) {
        pq_printf("get 6BaseColorEnhance fail!\n");
        return HI_FAILURE;
    }

    val[0] = color_mode_param.color_gain.six_base_color.red;
    val[1] = color_mode_param.color_gain.six_base_color.green;
    val[2] = color_mode_param.color_gain.six_base_color.blue; /* 2: index */
    val[3] = color_mode_param.color_gain.six_base_color.cyan; /* 3: index */
    val[4] = color_mode_param.color_gain.six_base_color.magenta; /* 4: index */
    val[5] = color_mode_param.color_gain.six_base_color.yellow; /* 5: index */

    pq_printf("get 6BaseColorEnhance:\nRed:%d \nGreen:%d \nBlue:%d \nCyan:%d \nMagenta:%d \nYellow:%d\n",
        val[0], val[1], val[2], val[3], val[4], val[5]); /* 2/3/4/5: index */

    return ret;
}

static hi_s32 pq_get_csc_color_temperation(hi_u32 chan, hi_u32 type, hi_s32 val[])
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_color_temp color_temperature = { 512, 512, 512, 512, 512, 512}; /* 512: index */
    hi_s32 ret;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    if (val == HI_NULL) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_get_color_temperature(disp_chan, pq_type, &color_temperature);
    if (ret != HI_SUCCESS) {
        pq_printf("sample pq_get_csc_color_temperation fail!\n");
        return HI_FAILURE;
    }

    val[0] = color_temperature.red_gain;
    val[1] = color_temperature.green_gain;
    val[2] = color_temperature.blue_gain; /* 2: index */
    val[3] = color_temperature.red_offset; /* 3: index */
    val[4] = color_temperature.green_offset; /* 4: index */
    val[5] = color_temperature.blue_offset; /* 5: index */

    pq_printf("sample pq_set_csc_color_temperation:\nRed_Gain:%d \nGreen_Gain:%d \nBlue_Gain:%d \
    \nRed_Offset:%d \nGreen_Offset:%d \nBlue_Offset:%d\n",
        val[0], val[1], val[2], val[3], val[4], val[5]); /* 2/3/4/5: index */

    return ret;
}

hi_s32 pq_set_csc_color_temperation(hi_u32 chan, hi_u32 type, hi_s32 val[])
{
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_s32 ret;

    hi_unf_pq_color_temp color_temperature = { 512, 512, 512, 512, 512, 512}; /* 512: index */
    color_temperature.red_gain = val[0];
    color_temperature.green_gain = val[1];
    color_temperature.blue_gain = val[2]; /* 2: index */
    color_temperature.red_offset = val[3]; /* 3: index */
    color_temperature.green_offset = val[4]; /* 4: index */
    color_temperature.blue_offset = val[5]; /* 5: index */

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_set_color_temperature(disp_chan, pq_type, color_temperature);
    if (ret != HI_SUCCESS) {
        pq_printf("sample pq_set_csc_color_temperation fail!\n");
        return HI_FAILURE;
    }

    pq_printf("sample pq_set_csc_color_temperation:\nRed_Gain:%d \nGreen_Gain:%d \nBlue_Gain:%d \
    \nRed_Offset:%d \nGreen_Offset:%d \nBlue_Offset:%d\n",
        val[0], val[1], val[2], val[3], val[4], val[5]); /* 2/3/4/5: index */
    return ret;
}

static hi_s32 pq_get_fleshtone(hi_u32 chan, hi_u32 type)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_color_enhance color_mode_param = {0};

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    color_mode_param.mode = HI_UNF_PQ_COLOR_ENHANCE_FLESHTONE;

    ret = hi_unf_pq_get_color_mode(disp_chan, pq_type, &color_mode_param);
    if (ret != HI_SUCCESS) {
        pq_printf("get fleshtone fail!\n");
        return HI_FAILURE;
    }

    return color_mode_param.color_gain.fleshtone_gain;
}

static hi_s32 pq_get_color_space_mode(hi_u32 chan, hi_u32 type)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_color_enhance color_mode_param = {0};

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    color_mode_param.mode = HI_UNF_PQ_COLOR_ENHANCE_SPEC_MODE;

    ret = hi_unf_pq_get_color_mode(disp_chan, pq_type, &color_mode_param);
    if (ret != HI_SUCCESS) {
        pq_printf("get colormode fail!\n");
        return HI_FAILURE;
    }

    return color_mode_param.color_gain.color_space_mode;
}

hi_s32 pq_get_module_strength(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_u32 strength = 0;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_module pq_module = (hi_unf_pq_module)val;

    ret = hi_unf_pq_get_module_strength(disp_chan, pq_type, pq_module, &strength);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_get_module_strength fail\n");
        return HI_FAILURE;
    }

    pq_printf("pq_get_module_strength:%d\n", strength);
    return strength;
}

static hi_s32 pq_get_demo_enable(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_demo_param demo_param;
    hi_unf_pq_module pq_module = (hi_unf_pq_module)val;
    hi_bool enable;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_get_demo(disp_chan, pq_type, pq_module, &demo_param);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_get_demo_mode fail\n");
        return HI_FAILURE;
    }

    if (demo_param.mode == HI_UNF_PQ_DEMO_MODE_CLOSE) {
        enable = HI_FALSE;
    } else {
        enable = HI_TRUE;
    }

    return enable;
}

static hi_s32 pq_get_demo_mode(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_demo_param demo_param;
    hi_unf_pq_module pq_module = (hi_unf_pq_module)val;

    if (chan > HI_UNF_DISPLAY1) {
        return HI_FAILURE;
    }

    ret = hi_unf_pq_get_demo(disp_chan, pq_type, pq_module, &demo_param);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_get_demo_mode fail\n");
        return HI_FAILURE;
    }

    return demo_param.mode;
}

static hi_s32 pq_get_demo_coordinate(hi_u32 chan, hi_u32 type, hi_u32 val)
{
    hi_s32 ret;
    hi_unf_disp disp_chan = (hi_unf_disp)chan;
    hi_unf_pq_image_type pq_type = (hi_unf_pq_image_type)type;
    hi_unf_pq_demo_param demo_param;
    hi_unf_pq_module pq_module = (hi_unf_pq_module)val;

    ret = hi_unf_pq_get_demo(disp_chan, pq_type, pq_module, &demo_param);
    if (ret != HI_SUCCESS) {
        pq_printf("pq_get_demo_coordinate fail\n");
        return HI_FAILURE;
    }

    return demo_param.x_position;
}

hi_s32 pq_sample_get_image_type(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
        if (strncmp(arg[n], "-imagetype", strlen(arg[n])) == 0) {
            if (++n == argc) {
                return HI_FAILURE;
            }
            g_image_type = atoi(arg[n]);
            RANGECHECK(g_image_type, 0, 2);
        }

    return HI_SUCCESS;
}

hi_s32 pq_sample_get_brightness(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-brightness", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_bright = atoi(arg[n]);
        RANGECHECK(g_bright, 0, 1023); /* 1023: param max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_contrast(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-contrast", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_contrast = atoi(arg[n]);
        RANGECHECK(g_contrast, 0, 1023); /* 1023: param max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_saturation(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-saturation", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_saturation = atoi(arg[n]);
        RANGECHECK(g_saturation, 0, 1023); /* 1023: param max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_hue(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-hue", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_hue = atoi(arg[n]);
        RANGECHECK(g_hue, 0, 1023); /* 1023: param max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_csc_color_temperation(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;

    if (strncmp(arg[n], "-csccolortemperation", strlen(arg[n])) == 0) {
        hi_s32 i;

        if (g_set_type == 0) {
            return HI_SUCCESS;
        }

        for (i = 0; i < PQ_COLOR_TEMP_NUM; i++) {
            if (++n == argc) {
                return HI_FAILURE;
            }
            g_csc_color_temperation[i] = atoi(arg[n]);
            RANGECHECK(g_csc_color_temperation[i], 0, 1023); /* 1023: param max */
        }
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_imagemode(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-imagemode", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_image_mode = atoi(arg[n]);
        RANGECHECK(g_image_mode, 0, 2); /* 3: mode max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_fleshtone(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-fleshtone", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_fleshtone = atoi(arg[n]);
        RANGECHECK(g_fleshtone, 0, 3); /* 3: mode max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_six_base(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;

    if (strncmp(arg[n], "-sixbasecolor", strlen(arg[n])) == 0) {
        hi_s32 i;
        if (g_set_type == 0) {
            return HI_SUCCESS;
        }

        for (i = 0; i < PQ_SIX_BASE__NUM; i++) { /* 6: index */
            if (++n == argc) {
                return HI_FAILURE;
            }
            g_pq_six_base_color[i] = atoi(arg[n]);
            RANGECHECK(g_pq_six_base_color[i], 0, 1023); /* 1023: param max */
        }
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_color_mode(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-colormode", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_color_space_mode = atoi(arg[n]);
        RANGECHECK(g_color_space_mode, 0, 3); /* 3: mode max */
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_module_enable(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-moduleenable", strlen(arg[n])) == 0) {
        g_set_pq_module = 1;

        if (++n == argc) {
            return HI_FAILURE;
        }

        g_pq_module = atoi(arg[n]);
        RANGECHECK(g_pq_module, 0, HI_UNF_PQ_MODULE_MAX); /* 8: mode max */

        if (++n == argc) {
            return HI_FAILURE;
        }
        g_module_en = atoi(arg[n]);
        RANGECHECK(g_module_en, 0, 1);
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_module_strength(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-modulestrength", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_pq_module = atoi(arg[n]);
        RANGECHECK(g_pq_module, 0, HI_UNF_PQ_MODULE_MAX); /* 8: mode max */

        if (++n == argc) {
            return HI_FAILURE;
        }
        g_strength = atoi(arg[n]);
        RANGECHECK(g_strength, 0, 1023);
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_by_pass(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-bypass", strlen(arg[n])) == 0) {
        g_set_bypass = 1;
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_pq_bypass = atoi(arg[n]);
        RANGECHECK(g_pq_bypass, 0, 1);
    }
    return HI_SUCCESS;
}

hi_s32 pq_sample_get_demo_enable(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-demoenable", strlen(arg[n])) == 0) {
        g_set_demo = 1;

        if (++n == argc) {
            return HI_FAILURE;
        }
        g_pq_module = atoi(arg[n]);
        RANGECHECK(g_pq_module, 0, HI_UNF_PQ_MODULE_MAX); /* 8: mode max */

        if (++n == argc) {
            return HI_FAILURE;
        }
        g_demo_en = atoi(arg[n]);
        RANGECHECK(g_demo_en, 0, 1);
    }

    return HI_SUCCESS;
}

hi_s32 pq_sample_get_demo_mode(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-demomode", strlen(arg[n])) == 0) {
        g_set_demo_disp_mode = 1;

        if (++n == argc) {
            return HI_FAILURE;
        }
        g_pq_module = atoi(arg[n]);
        RANGECHECK(g_pq_module, 0, HI_UNF_PQ_MODULE_MAX); /* 8: mode max */

        if (++n == argc) {
            return HI_FAILURE;
        }

        g_demo_disp_mode = atoi(arg[n]);
        RANGECHECK(g_demo_disp_mode, 0, 3); /* 3: mode max */
    }

    return HI_SUCCESS;
}

hi_s32 pq_sample_get_demo_coor(hi_u32 index, int argc, char *arg[])
{
    hi_u32 n = index;
    if (strncmp(arg[n], "-dmcoor", strlen(arg[n])) == 0) {
        if (++n == argc) {
            return HI_FAILURE;
        }
        g_pq_module = atoi(arg[n]);
        RANGECHECK(g_pq_module, 0, HI_UNF_PQ_MODULE_MAX); /* 8: mode max */

        if (++n == argc) {
            return HI_FAILURE;
        }
        g_demo_coordinate = atoi(arg[n]);
        RANGECHECK(g_demo_coordinate, 0, 1023); /* 1023: param max */
    }

    return HI_SUCCESS;
}

static hi_s32 pq_parse_command_line(int argc, char *argv[])
{
    hi_s32 n, ret;
    hi_u32 index;
    hi_u32 max_func_index = sizeof(g_pq_sample_info) / sizeof(pq_sample_fun);

    g_pq_six_base_color[PQ_SIX_BASE__NUM - 1] = -1; /* 5: index */
    g_csc_color_temperation[PQ_COLOR_TEMP_NUM - 1] = -1;

    for (n = 1; n < argc; n++) {
        const char *arg = argv[n];
        if (strncmp(arg, "-set", strlen(arg)) == 0) {
            g_set_type = 1;
            continue;
        }

        if (strncmp(arg, "-get", strlen(arg)) == 0) {
            g_set_type = 0;
            if (n + 1 == argc) {
                return HI_FAILURE;
            }

            g_pq_get_option = argv[n + 1];

            continue;
        }

        if (strncmp(arg, "-help", strlen(arg)) == 0) {
            pq_help_msg();
            hi_unf_pq_deinit();
            exit(0);
        }

        if (strncmp(arg, "-chan", strlen(arg)) == 0) {
            if (++n == argc) {
                return HI_FAILURE;
            }
            g_chan = atoi(argv[n]);
            RANGECHECK(g_chan, 0, 1);
            continue;
        }

        for (index = 0; index < max_func_index; index++) {
            if (strncmp(arg, g_pq_sample_info[index].func_name, strlen(g_pq_sample_info[index].func_name)) == 0) {
                ret = g_pq_sample_info[index].proc(n, argc, argv);
                break;
            }
        }

        if (ret == HI_SUCCESS) {
            continue;
        } else {
            break;
        }

        pq_help_msg();
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 pq_get_csc_info(hi_void)
{
    if (strncmp(g_pq_get_option, "-brightness", strlen(g_pq_get_option)) == 0) {
        pq_printf("bright ness: %d\n", pq_get_bright((hi_unf_disp)g_chan, g_image_type));
    } else if (strncmp(g_pq_get_option, "-contrast", strlen(g_pq_get_option)) == 0) {
        pq_printf("contrast: %d\n", pq_get_contrast((hi_unf_disp)g_chan, g_image_type));
    } else if (strncmp(g_pq_get_option, "-hue", strlen(g_pq_get_option)) == 0) {
        pq_printf("hue: %d\n", pq_get_hue((hi_unf_disp)g_chan, g_image_type));
    } else if (strncmp(g_pq_get_option, "-saturation", strlen(g_pq_get_option)) == 0) {
        pq_printf("saturation: %d\n", pq_get_saturation((hi_unf_disp)g_chan, g_image_type));
    } else if (strncmp(g_pq_get_option, "-csccolortemperation", strlen(g_pq_get_option)) == 0) {
        pq_get_csc_color_temperation((hi_unf_disp)g_chan, g_image_type, g_csc_color_temperation);
    }
    return HI_SUCCESS;
}

hi_s32 pq_get_color_enhance_info(hi_void)
{
    if (strncmp(g_pq_get_option, "-imagemode", strlen(g_pq_get_option)) == 0) {
        pq_printf("image mode: %d (0:natural/1:person/2:film/3:UD)\n",
            pq_get_image_mode((hi_unf_disp)g_chan, g_image_type));
    } else if (strncmp(g_pq_get_option, "-sixbasecolor", strlen(g_pq_get_option)) == 0) {
        pq_get_six_base_color_enhance((hi_unf_disp)g_chan, g_image_type, g_pq_six_base_color);
    } else if (strncmp(g_pq_get_option, "-fleshtone", strlen(g_pq_get_option)) == 0) {
        pq_printf("fleshtone: %d (0:off/1:low/2:mid/3:high)\n",
            pq_get_fleshtone((hi_unf_disp)g_chan, g_image_type));
    } else if (strncmp(g_pq_get_option, "-colormode", strlen(g_pq_get_option)) == 0) {
        pq_printf("colormode: %d (0:recommend/1:blue/2:green/3:BG)\n",
            pq_get_color_space_mode((hi_unf_disp)g_chan, g_image_type));
    }
    return HI_SUCCESS;
}

hi_s32 pq_get_module_info(hi_void)
{
    if (strncmp(g_pq_get_option, "-moduleenable", strlen(g_pq_get_option)) == 0) {
        pq_printf("Module %d enable: %s\n", g_pq_module, pq_get_module_enable((hi_unf_disp)g_chan,
                  g_image_type, g_pq_module) ? "on" : "off");
    } else if (strncmp(g_pq_get_option, "-modulestrength", strlen(g_pq_get_option)) == 0) {
        pq_printf("Module %s strength: %d\n", g_mdule_str[g_pq_module], pq_get_module_strength((hi_unf_disp)g_chan,
                  g_image_type, g_pq_module));
    } else if (strncmp(g_pq_get_option, "-demoenable", strlen(g_pq_get_option)) == 0) {
        pq_printf("Module %s demo enable: %d\n", g_mdule_str[g_pq_module], pq_get_demo_enable((hi_unf_disp)g_chan,
                  g_image_type, g_pq_module));
    } else if (strncmp(g_pq_get_option, "-demomode", strlen(g_pq_get_option)) == 0) {
        pq_printf("Module %s demomode: %d\n", g_mdule_str[g_pq_module], pq_get_demo_mode((hi_unf_disp)g_chan,
                  g_image_type, g_pq_module));
    } else if (strncmp(g_pq_get_option, "-dmcoor", strlen(g_pq_get_option)) == 0) {
        pq_printf("Module %s demo coor: %d\n", g_mdule_str[g_pq_module], pq_get_demo_coordinate((hi_unf_disp)g_chan,
                  g_image_type, g_pq_module));
    }
    return HI_SUCCESS;
}

hi_s32 pq_get_all_info(hi_void)
{
    hi_u32 i = 0;

    pq_printf("brightness:%d\n", pq_get_bright((hi_unf_disp)g_chan, g_image_type));
    pq_printf("contrast:%d\n", pq_get_contrast((hi_unf_disp)g_chan, g_image_type));
    pq_printf("hue:%d\n", pq_get_hue((hi_unf_disp)g_chan, g_image_type));
    pq_printf("saturation:%d\n", pq_get_saturation((hi_unf_disp)g_chan, g_image_type));
    pq_printf("csc color temperation:%d\n", pq_get_csc_color_temperation((hi_unf_disp)g_chan,
        g_image_type, g_csc_color_temperation));
    pq_printf("fleshtone:    %d (0:off/1:low/2:mid/3:high)\n",
        pq_get_fleshtone((hi_unf_disp)g_chan, g_image_type));
    pq_printf("colormode:   %d (0:recommend/1:blue/2:green/3:BG)\n",
        pq_get_color_space_mode((hi_unf_disp)g_chan, g_image_type));

    pq_printf("\n");
    for (i  = HI_UNF_PQ_MODULE_DEI; i < HI_UNF_PQ_MODULE_MAX; i++) {
        pq_printf("Module %s enable: %s\n", g_mdule_str[i], pq_get_module_enable((hi_unf_disp)g_chan,
            g_image_type, i) ? "on" : "off");
    }
    pq_printf("\n");
    for (i  = HI_UNF_PQ_MODULE_DEI; i < HI_UNF_PQ_MODULE_MAX; i++) {
        pq_printf("Module %s strength: %d\n", g_mdule_str[i], pq_get_module_strength((hi_unf_disp)g_chan,
            g_image_type, i));
    }
    pq_printf("\n");
    for (i  = HI_UNF_PQ_MODULE_DEI; i < HI_UNF_PQ_MODULE_MAX; i++) {
        pq_printf("Module %s demo enable: %d\n", g_mdule_str[i], pq_get_demo_enable((hi_unf_disp)g_chan,
            g_image_type, i));
    }
    for (i  = HI_UNF_PQ_MODULE_DEI; i < HI_UNF_PQ_MODULE_MAX; i++) {
        pq_printf("Module %s demomode: %d\n", g_mdule_str[i], pq_get_demo_mode((hi_unf_disp)g_chan,
            g_image_type, i));
    }
    for (i  = HI_UNF_PQ_MODULE_DEI; i < HI_UNF_PQ_MODULE_MAX; i++) {
        pq_printf("Module %s demo coor: %d\n", g_mdule_str[i], pq_get_demo_coordinate((hi_unf_disp)g_chan,
            g_image_type, i));
    }
    return HI_SUCCESS;
}

hi_s32 pq_set_module_info(hi_void)
{
    if (g_set_pq_module > -1) {
        pq_set_module_enable((hi_unf_disp)g_chan, g_image_type, g_pq_module, g_module_en);
    }
    if (g_strength > -1) {
        pq_set_module_strength((hi_unf_disp)g_chan, g_image_type, g_pq_module, g_strength);
    }
    if (g_set_demo > -1) {
        pq_set_demo_enable((hi_unf_disp)g_chan, g_image_type, g_pq_module, g_demo_en);
    }
    if (g_set_demo_disp_mode > -1) {
        pq_set_demo_disp_mode((hi_unf_disp)g_chan, g_image_type, g_pq_module, g_demo_disp_mode);
    }
    if (g_demo_coordinate > -1) {
        pq_set_demo_coordinate((hi_unf_disp)g_chan, g_image_type, g_pq_module, g_demo_coordinate);
    }

    return HI_SUCCESS;
}

hi_s32 pq_set_color_enhance_info(hi_void)
{
    if (g_image_mode > -1) {
        pq_set_image_mode((hi_unf_disp)g_chan, g_image_type, g_image_mode);
    }

    if (g_pq_six_base_color[PQ_SIX_BASE__NUM - 1] > -1) { /* 5: index */
        pq_set_six_base_color_enhance((hi_unf_disp)g_chan, g_image_type, g_pq_six_base_color);
    }

    if (g_fleshtone > -1) {
        pq_set_fleshtone((hi_unf_disp)g_chan, g_image_type, g_fleshtone);
    }
    if (g_color_space_mode > -1) {
        pq_set_color_space_mode((hi_unf_disp)g_chan, g_image_type, g_color_space_mode);
    }

    return HI_SUCCESS;
}

hi_s32 pq_set_csc_info(hi_void)
{
    if (g_bright > -1) {
        pq_set_bright((hi_unf_disp)g_chan, g_image_type, g_bright);
    }
    if (g_contrast > -1) {
        pq_set_contrast((hi_unf_disp)g_chan, g_image_type, g_contrast);
    }
    if (g_hue > -1) {
        pq_set_hue((hi_unf_disp)g_chan, g_image_type, g_hue);
    }
    if (g_saturation > -1) {
        pq_set_saturation((hi_unf_disp)g_chan, g_image_type, g_saturation);
    }

    if (g_csc_color_temperation[PQ_COLOR_TEMP_NUM - 1] > -1) { /* 2: index */
        pq_set_csc_color_temperation((hi_unf_disp)g_chan, g_image_type, g_csc_color_temperation);
    }

    return HI_SUCCESS;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;

    hi_unf_pq_init(HI_NULL);

    ret = pq_parse_command_line(argc, argv);
    if (ret != HI_SUCCESS) {
        pq_printf("Parse command line error!\n");
        hi_unf_pq_deinit();
        return HI_FAILURE;
    }

    /* get option */
    if (g_set_type == 0) {
        if (g_pq_get_option == NULL) {
            pq_printf("option is null\n");
            hi_unf_pq_deinit();
            pq_help_msg();
            return HI_FAILURE;
        } else {
            pq_printf("Chan : %d (0: disp0/1:disp1), Type: %d(0: ALL/1:graph/2:video), %s\n", g_chan,
                g_image_type, g_pq_get_option);
            pq_get_module_info();
            pq_get_color_enhance_info();
            pq_get_csc_info();
            if (strncmp(g_pq_get_option, "-all", strlen(g_pq_get_option)) == 0) {
                pq_get_all_info();
            }
        }
    } else if (g_set_type == 1) {    /* set option */
        pq_printf("Chan : %d (0: disp0/1:disp1), Type: %d(0: ALL/1:graph/2:video), %s\n", g_chan,
            g_image_type, g_pq_get_option);
        pq_set_csc_info();
        pq_set_color_enhance_info();
        pq_set_module_info();

        if (g_set_bypass > -1) {
            pq_set_pq_bypass(g_pq_bypass);
        }
    }

    hi_unf_pq_deinit();

    return ret;
}



