/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: tde sample
 * Create: 2019-03-18
 */

#include <string.h>
#include "tde_common.h"

static hi_void set_blend_attr(hi_unf_gfx2d_list *surface_list, hi_unf_gfx2d_attr *attr)
{
    attr->blend_attr.blend_enable = HI_TRUE;
    attr->blend_attr.blend_cmd = HI_UNF_GFX2D_BLENDCMD_SRCOVER;
    attr->blend_attr.global_alpha = 0x80;
    attr->blend_attr.global_alpha_en = HI_TRUE;
    attr->blend_attr.pixel_alpha_en = HI_TRUE;

    surface_list->src_surfaces[0].attr = attr;
    surface_list->src_surfaces[1].attr = attr;
}

hi_s32 main(void)
{
    hi_s32 ret;
    hi_handle handle;
    hi_unf_gfx2d_surface src_surface[2]; /* 2: two src surfaces */
    hi_unf_gfx2d_surface dst_surface;
    hi_unf_gfx2d_attr attr = {0};
    hi_unf_gfx2d_list surface_list = {0};

    memset(&attr, 0, sizeof(hi_unf_gfx2d_attr));

    ret = gfx2d_create_surface_by_file(ARGB8888_1024_683, &src_surface[1]);
    if (ret != HI_SUCCESS) {
        return ret;
    }
    ret = gfx2d_create_surface_by_file(ARGB8888_1024_684, &src_surface[0]);
    if (ret != HI_SUCCESS) {
        return ret;
    }
    ret = gfx2d_dup_surface(&src_surface[0], 1, &dst_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    surface_list.src_surface_cnt = 2; /* 2: two src surfaces */
    surface_list.src_surfaces = src_surface;
    surface_list.dst_surface = &dst_surface;
    surface_list.ops_mode = HI_UNF_GFX2D_OPS_BIT_BLIT;

    set_blend_attr(&surface_list, &attr);

    hi_unf_gfx2d_open(HI_UNF_GFX2D_DEV_ID_0);

    gfx2d_show_surface(&src_surface[0], 1);
    gfx2d_show_surface(&src_surface[1], 1);

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
