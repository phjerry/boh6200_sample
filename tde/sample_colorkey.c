/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: tde sample
 * Create: 2019-03-18
 */

#include <string.h>
#include "tde_common.h"

static hi_void set_color_key_attr(hi_unf_gfx2d_list *surface_list, hi_unf_gfx2d_attr *attr)
{
    attr->colorkey_attr.colorkey_enable = HI_TRUE;
    attr->colorkey_attr.colorkey_mode = HI_UNF_GFX2D_COLORKEY_FOREGROUND;
    attr->blend_attr.out_alpha_mode = HI_UNF_GFX2D_OUTALPHA_FROM_FOREGROUND;
    attr->colorkey_attr.colorkey_value.colorkey_argb.blue.is_component_ignore = HI_FALSE;
    attr->colorkey_attr.colorkey_value.colorkey_argb.blue.is_component_out = HI_FALSE;
     /* 0x80: component min */
    attr->colorkey_attr.colorkey_value.colorkey_argb.blue.component_min = 0x80;
    /* 0xff: component max */
    attr->colorkey_attr.colorkey_value.colorkey_argb.blue.component_max = 0xff;
    /* 0xff: component mask */
    attr->colorkey_attr.colorkey_value.colorkey_argb.blue.component_mask = 0xff;

    attr->colorkey_attr.colorkey_value.colorkey_argb.alpha.is_component_ignore = HI_TRUE;
    attr->colorkey_attr.colorkey_value.colorkey_argb.red.is_component_ignore = HI_TRUE;
    attr->colorkey_attr.colorkey_value.colorkey_argb.green.is_component_ignore = HI_TRUE;

    surface_list->src_surfaces[0].attr = attr;
}

hi_s32  main(void)
{
    hi_s32 ret;
    hi_handle handle;
    hi_unf_gfx2d_surface src_surface[2]; /* 2: two src surfaces */
    hi_unf_gfx2d_surface dst_surface;
    hi_unf_gfx2d_attr attr;
    hi_unf_gfx2d_list surface_list = {0};

    memset(&attr, 0, sizeof(hi_unf_gfx2d_attr));

    ret = gfx2d_create_surface_by_file(ARGB8888_1024_684, &src_surface[1]);
    if (ret != HI_SUCCESS) {
        return ret;
    }
    ret = gfx2d_create_surface_by_file(ARGB8888_1024_683, &src_surface[0]);
    if (ret != HI_SUCCESS) {
        return ret;
    }
    ret = gfx2d_dup_surface(&src_surface[0], 1, &dst_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    gfx2d_show_surface(&src_surface[0], 1);
    gfx2d_show_surface(&src_surface[1], 1);

    surface_list.src_surface_cnt = 2; /* 2: two src surfaces */
    surface_list.src_surfaces = src_surface;
    surface_list.dst_surface = &dst_surface;
    surface_list.ops_mode = HI_UNF_GFX2D_OPS_BIT_BLIT;

    set_color_key_attr(&surface_list, &attr);

    hi_unf_gfx2d_open(HI_UNF_GFX2D_DEV_ID_0);

    handle = hi_unf_gfx2d_create(HI_UNF_GFX2D_DEV_ID_0);

    hi_unf_gfx2d_bitblit(handle, &surface_list);
    if (ret != HI_SUCCESS) {
        SAMPLE_GFX2D_PRINT("blit failed 0x%x\n", ret);
        return ret;
    }

    hi_unf_gfx2d_submit(handle, HI_TRUE, TDE_SAMPLE_TIMEOUT);

    gfx2d_show_surface(&dst_surface, 1);

    gfx2d_destroy_surface(&src_surface[0], 1);
    gfx2d_destroy_surface(&src_surface[1], 1);
    gfx2d_destroy_surface(&dst_surface, 1);

    hi_unf_gfx2d_close(HI_UNF_GFX2D_DEV_ID_0);

    return HI_SUCCESS;
}

