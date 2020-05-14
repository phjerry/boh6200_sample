/******************************************************************************

  Copyright (C), 2014-2015, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : rawplay.c
  Version       : Initial Draft
  Description   : video raw data player, multi channel support
  History       :
  1.Date        : 2014/11/13
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "hi_unf_disp.h"

#ifndef sample_printf
#define sample_printf  printf
#endif

hi_s32 common_get_disp_fmt(const hi_char *str)
{
    if (str == HI_NULL) {
        return HI_UNF_VIDEO_FMT_MAX;
    }

    if (strcasecmp(str, "1080P60") == 0) {
        sample_printf("Use 1080P60 output\n");
        return HI_UNF_VIDEO_FMT_1080P_60;
    } else if (strcasecmp(str, "1080P50") == 0) {
        sample_printf("Use 1080P50 output\n");
        return HI_UNF_VIDEO_FMT_1080P_50;
    } else if (strcasecmp(str, "1080P30") == 0) {
        sample_printf("Use 1080P30 output\n");
        return HI_UNF_VIDEO_FMT_1080P_30;
    } else if (strcasecmp(str, "1080P25") == 0) {
        sample_printf("Use 1080P25 output\n");
        return HI_UNF_VIDEO_FMT_1080P_25;
    } else if (strcasecmp(str, "1080P24") == 0) {
        sample_printf("Use 1080P24 output\n");
        return HI_UNF_VIDEO_FMT_1080P_24;
    } else if (strcasecmp(str, "1080i60") == 0) {
        sample_printf("Use 1080i60 output\n");
        return HI_UNF_VIDEO_FMT_1080I_60;
    } else if (strcasecmp(str, "1080i50") == 0) {
        sample_printf("Use 1080i50 output\n");
        return HI_UNF_VIDEO_FMT_1080I_50;
    } else if (strcasecmp(str, "720P60") == 0) {
        sample_printf("Use 720P60 output\n");
        return HI_UNF_VIDEO_FMT_720P_60;
    } else if (strcasecmp(str, "720P50") == 0) {
        sample_printf("Use 720P50 output\n");
        return HI_UNF_VIDEO_FMT_720P_50;
    }

    return HI_UNF_VIDEO_FMT_MAX;
}

