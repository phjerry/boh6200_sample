/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: tde sample
 * Create: 2019-03-18
 */

#ifndef __TDE_COMMON_H__
#define __TDE_COMMON_H__

#include <stdio.h>

#include "hi_type.h"
#include "hi_unf_gfx2d.h"

#define GFX2D_SAMPLE_RES "../res/tde/"
#define ARGB8888_1024_683 GFX2D_SAMPLE_RES"fmt_ARGB8888-w_1024-h_683-stride_4096.bits"
#define ARGB8888_1024_684 GFX2D_SAMPLE_RES"fmt_ARGB8888-w_1024-h_684-stride_4096.bits"
#define YUV420MB GFX2D_SAMPLE_RES"fmt_SEMIPLANAR420UV-w_1024-h_684-stride_1024-uvstride_1024.bits"

#define SAMPLE_GFX2D_PRINT  printf
#define TDE_SAMPLE_TIMEOUT 500 /* time out 500ms */

#ifdef __cplusplus
extern "C" {
#endif

hi_s32 gfx2d_create_surface_by_file(const hi_char *file, hi_unf_gfx2d_surface *surface);
hi_s32 gfx2d_dup_surface(hi_unf_gfx2d_surface *in_surface, hi_u32 size, hi_unf_gfx2d_surface *out_surface);
hi_s32 gfx2d_destroy_surface(hi_unf_gfx2d_surface *surface, hi_u32 size);
hi_s32 gfx2d_show_surface(hi_unf_gfx2d_surface *surface, hi_u32 size);
hi_void gfx2d_print_surface(hi_unf_gfx2d_surface *surface, hi_u32 size);
hi_s32 gfx2d_save_surface_to_file(const hi_char *file, hi_unf_gfx2d_surface *surface);

#ifdef __cplusplus
}
#endif
#endif
