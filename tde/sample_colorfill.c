/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: tde sample
 * Create: 2019-03-18
 */

#include <string.h>
#include "tde_common.h"

hi_s32  main(void)
{
    hi_s32 ret;
    hi_handle handle;
    hi_unf_gfx2d_surface dst_surface = {0};
    hi_unf_gfx2d_list surface_list = {0};

    ret = gfx2d_create_surface_by_file(ARGB8888_1024_684, &dst_surface);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    dst_surface.out_rect.width = dst_surface.width / 2; /* 2: fill half width */
    dst_surface.out_rect.height = dst_surface.height / 2; /* 2: fill half height */

    gfx2d_show_surface(&dst_surface, 1);

    surface_list.src_surface_cnt = 0;
    surface_list.src_surfaces = HI_NULL;
    surface_list.dst_surface = &dst_surface;
    surface_list.ops_mode = HI_UNF_GFX2D_OPS_QUICK_FILL;
    surface_list.dst_surface->background_color = 0xff0000ff;

    hi_unf_gfx2d_open(HI_UNF_GFX2D_DEV_ID_0);

    handle = hi_unf_gfx2d_create(HI_UNF_GFX2D_DEV_ID_0);

    hi_unf_gfx2d_bitblit(handle, &surface_list);
    if (ret != HI_SUCCESS) {
        SAMPLE_GFX2D_PRINT("blit failed 0x%x\n", ret);
        return ret;
    }

    hi_unf_gfx2d_submit(handle, HI_TRUE, TDE_SAMPLE_TIMEOUT);

    gfx2d_show_surface(&dst_surface, 1);

    gfx2d_destroy_surface(&dst_surface, 1);

    hi_unf_gfx2d_close(HI_UNF_GFX2D_DEV_ID_0);

    return HI_SUCCESS;
}

