/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: tde sample
 * Create: 2019-11-25
 */

#include <string.h>
#include "tde_common.h"

static hi_void set_blend_attr(hi_unf_gfx2d_list *surface_list, hi_unf_gfx2d_attr *attr)
{
    hi_u32 i;

    for (i = 0; i < surface_list->src_surface_cnt; i++) {
        surface_list->src_surfaces[i].attr = attr;
    }
}

static hi_void set_rect(hi_unf_gfx2d_list *surface_list)
{
    hi_u32 i;

    for (i = 0; i < surface_list->src_surface_cnt; i++) {
        surface_list->src_surfaces[i].in_rect.width -= i * 50; /* width -50 in every layer */
        surface_list->src_surfaces[i].in_rect.height -= i * 60; /* height -60 in every layer */

        surface_list->src_surfaces[i].out_rect.x += i * 40; /* start pos x -40 in every layer */
        surface_list->src_surfaces[i].out_rect.y += i * 50; /* start pos y -50 in every layer */

        surface_list->src_surfaces[i].out_rect.width -= i * 50; /* width -50 in every layer */
        surface_list->src_surfaces[i].out_rect.height -= i * 60; /* height -60 in every layer */
    }
}

hi_s32 main(void)
{
    hi_s32 ret;
    hi_unf_gfx2d_surface src_surface[2]; /* 2 src surfaces */
    hi_unf_gfx2d_surface dst_surface;
    hi_unf_gfx2d_attr attr = {0};
    hi_unf_gfx2d_list surface_list = {0};

    ret = gfx2d_create_surface_by_file(ARGB8888_1024_684, &src_surface[0]);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = gfx2d_create_surface_by_file(ARGB8888_1024_684, &src_surface[1]);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = gfx2d_dup_surface(&src_surface[0], 1, &dst_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    surface_list.src_surface_cnt = 2; /* 2 src surfaces */
    surface_list.src_surfaces = src_surface;
    surface_list.dst_surface = &dst_surface;

    set_rect(&surface_list);
    set_blend_attr(&surface_list, &attr);

    hi_unf_gfx2d_open(HI_UNF_GFX2D_DEV_ID_0);

    ret = hi_unf_gfx2d_compose(0, &surface_list, HI_TRUE);
    if (ret != HI_SUCCESS) {
        SAMPLE_GFX2D_PRINT("compose failed 0x%x\n", ret);
        goto free_surface;
    }

    gfx2d_show_surface(&dst_surface, 1);

free_surface:
    gfx2d_destroy_surface(&src_surface[0], 1);
    gfx2d_destroy_surface(&src_surface[1], 1);
    gfx2d_destroy_surface(&dst_surface, 1);

    hi_unf_gfx2d_close(HI_UNF_GFX2D_DEV_ID_0);

    return ret;
}
