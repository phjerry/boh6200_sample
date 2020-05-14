/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: tde sample
 * Create: 2019-03-18
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>


#include "hi_errno.h"
#include "tde_common.h"
#include "hi_unf_memory.h"
#include "hifb.h"
#ifdef SAMPLE_GFX2D_INIT_DISP
#include "hi_adp_mpi.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif  /* __cplusplus */
#endif  /* __cplusplus */

typedef struct {
    hi_unf_gfx2d_color_fmt fmt_index;
    char *fmt_name;
} tde_fmt_index_name_conv;

typedef struct {
    hi_char fmt_name[32]; /* 32 buffer size */
    hi_unf_gfx2d_color_fmt fmt_index;
    hi_u32 width;
    hi_u32 height;
    hi_u32 stride;
    hi_u32 uv_stride;
} gfx2d_resource_file_info;

const static tde_fmt_index_name_conv g_gfx2d_fmt_index_name[] = {
    { HI_UNF_GFX2D_FMT_RGB565, "RGB565" },
    { HI_UNF_GFX2D_FMT_BGR565, "BGR565" },
    { HI_UNF_GFX2D_FMT_RGB888, "RGB888" },
    { HI_UNF_GFX2D_FMT_BGR888, "BGR888" },

    { HI_UNF_GFX2D_FMT_ARGB4444, "ARGB4444" },
    { HI_UNF_GFX2D_FMT_ABGR4444, "ABGR4444" },
    { HI_UNF_GFX2D_FMT_RGBA4444, "RGBA4444" },
    { HI_UNF_GFX2D_FMT_BGRA4444, "BGRA4444" },
    { HI_UNF_GFX2D_FMT_ARGB1555, "ARGB1555" },
    { HI_UNF_GFX2D_FMT_ABGR1555, "ABGR1555" },
    { HI_UNF_GFX2D_FMT_RGBA5551, "RGBA5551" },
    { HI_UNF_GFX2D_FMT_BGRA5551, "BGRA5551" },

    { HI_UNF_GFX2D_FMT_ARGB8888, "ARGB8888" },
    { HI_UNF_GFX2D_FMT_ABGR8888, "ABGR8888" },
    { HI_UNF_GFX2D_FMT_RGBA8888, "RGBA8888" },
    { HI_UNF_GFX2D_FMT_BGRA8888, "BGRA8888" },
    { HI_UNF_GFX2D_FMT_ARGB2101010, "ARGB2101010" },
    { HI_UNF_GFX2D_FMT_ARGB10101010, "ARGB10101010" },
    { HI_UNF_GFX2D_FMT_FP16, "FP16" },

    { HI_UNF_GFX2D_FMT_CLUT1, "CLUT1" },
    { HI_UNF_GFX2D_FMT_CLUT2, "CLUT2" },
    { HI_UNF_GFX2D_FMT_CLUT4, "CLUT4" },
    { HI_UNF_GFX2D_FMT_CLUT8, "CLUT8" },
    { HI_UNF_GFX2D_FMT_ACLUT44, "ACLUT44" },
    { HI_UNF_GFX2D_FMT_ACLUT88, "ACLUT88" },
    { HI_UNF_GFX2D_FMT_A1, "A1" },
    { HI_UNF_GFX2D_FMT_A8, "A8" },

    { HI_UNF_GFX2D_FMT_YUV888, "YUV888" },
    { HI_UNF_GFX2D_FMT_AYUV8888, "AYUV8888" },
    { HI_UNF_GFX2D_FMT_YUYV422, "YUYV422" },
    { HI_UNF_GFX2D_FMT_YVYU422, "YVYU422" },
    { HI_UNF_GFX2D_FMT_UYVY422, "UYVY422" },
    { HI_UNF_GFX2D_FMT_YYUV422, "YYUV422" },
    { HI_UNF_GFX2D_FMT_VYUY422, "VYUY422" },
    { HI_UNF_GFX2D_FMT_VUYY422, "VUYY422" },

    { HI_UNF_GFX2D_FMT_SEMIPLANAR400, "SEMIPLANAR400" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR420UV, "SEMIPLANAR420UV" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR420VU, "SEMIPLANAR420VU" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR422UV_H, "SEMIPLANAR422UV_H" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR422VU_H, "SEMIPLANAR422VU_H" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR422UV_V, "SEMIPLANAR422UV_V" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR422VU_V, "SEMIPLANAR422VU_V" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR444UV, "SEMIPLANAR444UV" },
    { HI_UNF_GFX2D_FMT_SEMIPLANAR444VU, "SEMIPLANAR444VU" },

    { HI_UNF_GFX2D_FMT_PLANAR400, "PLANAR400" },
    { HI_UNF_GFX2D_FMT_PLANAR420, "PLANAR420" },
    { HI_UNF_GFX2D_FMT_PLANAR411, "PLANAR411" },
    { HI_UNF_GFX2D_FMT_PLANAR410, "PLANAR410" },
    { HI_UNF_GFX2D_FMT_PLANAR422H, "PLANAR422H" },
    { HI_UNF_GFX2D_FMT_PLANAR422V, "PLANAR422V" },
    { HI_UNF_GFX2D_FMT_PLANAR444, "PLANAR444" },

    { HI_UNF_GFX2D_FMT_MAX, "ERROR" },
};

static hi_s32 get_fmt_index_from_name(char *fmt_name)
{
    int fmt_index;

    for (fmt_index = 0; fmt_index < sizeof(g_gfx2d_fmt_index_name) / sizeof(tde_fmt_index_name_conv); fmt_index++) {
        if (!strncasecmp(fmt_name, g_gfx2d_fmt_index_name[fmt_index].fmt_name,
            strlen(g_gfx2d_fmt_index_name[fmt_index].fmt_name))) {
            return g_gfx2d_fmt_index_name[fmt_index].fmt_index;
        }
    }
    return HI_UNF_GFX2D_FMT_MAX;
}

static hi_s32 tde_get_resource_file_info(const hi_char *file, hi_unf_gfx2d_surface *surface)
{
    hi_u32 ret;
    gfx2d_resource_file_info file_info = {0};
    hi_u32 args;
    if ((file == HI_NULL) || (surface == HI_NULL)) {
        return -1;
    }

    if (strstr(file, "uvstride") == HI_NULL) {
        args = 4; /* 4 args */
        ret = sscanf(file, GFX2D_SAMPLE_RES"fmt_%32[^-]-w_%u-h_%u-stride_%u",
            file_info.fmt_name, &file_info.width, &file_info.height, &file_info.stride);
    } else {
        args = 5; /* 5 args with uv stride */
        ret = sscanf(file, GFX2D_SAMPLE_RES"fmt_%32[^-]-w_%u-h_%u-stride_%u-uvstride_%u",
            file_info.fmt_name, &file_info.width, &file_info.height, &file_info.stride, &file_info.uv_stride);
    }
    if (ret != args) {
        printf("parse_file_name_to_tde_surface failed %s\n", file);
        return -1;
    }
    file_info.fmt_index = get_fmt_index_from_name(file_info.fmt_name);
    if (file_info.fmt_index >= HI_UNF_GFX2D_FMT_MAX) {
        printf("parse_fmt failed\n");
        return -1;
    }
    surface->format = file_info.fmt_index;
    surface->width = file_info.width;
    surface->height = file_info.height;
    surface->stride[0] = file_info.stride;
    surface->stride[1] = file_info.uv_stride;
    surface->size[0] = surface->stride[0] * surface->height;
    surface->size[1] = surface->stride[1] * surface->height;
    return 0;
}

static hi_void display_init()
{
#ifdef SAMPLE_GFX2D_INIT_DISP
    hi_unf_sys_init();
    hi_adp_disp_init(HI_UNF_VIDEO_FMT_1080P_60);
#endif
}

static hi_void display_deinit()
{
#ifdef SAMPLE_GFX2D_INIT_DISP
    hi_adp_disp_deinit();
    hi_unf_sys_deinit();
#endif
}

static hi_s32 tde_read_clut_data(hi_unf_gfx2d_surface *surface, FILE *fp)
{
    hi_u8 *virt = HI_NULL;
    if ((surface->format >= HI_UNF_GFX2D_FMT_CLUT1) && (surface->format <= HI_UNF_GFX2D_FMT_ACLUT88)) {
        surface->palette_mem_handle.mem_handle =
            hi_unf_mem_new("sample-tde-clut", strlen("sample-tde-clut"), 256 * 4, HI_FALSE); /* 256 * 4: clut size */
        if (surface->palette_mem_handle.mem_handle <= 0) {
            SAMPLE_GFX2D_PRINT("error when call hi_unf_mem_new, line:%d\n", __LINE__);
            return -1;
        }

        virt = (hi_u8 *)hi_unf_mem_map(surface->palette_mem_handle.mem_handle, 256 * 4); /* 256 * 4: clut size */
        if (virt == HI_NULL) {
            hi_unf_mem_delete(surface->palette_mem_handle.mem_handle);
            surface->palette_mem_handle.mem_handle = -1;
            return -1;
        }
        fread(virt, 1, 256 * 4, fp); /* 256 * 4: clut size */
        hi_unf_mem_unmap(virt, 256 * 4); /* 256 * 4: clut size */
    } else if (surface->format  == HI_UNF_GFX2D_FMT_ARGB1555) {
        surface->alpha_ext.alpha_ext_enable = HI_TRUE;
        surface->alpha_ext.alpha0 = 0;
        surface->alpha_ext.alpha1 = 0xff;
    }
    return 0;
}

static hi_s32 tde_read_y_data(hi_unf_gfx2d_surface *surface, FILE *fp)
{
    hi_u8 *virt = HI_NULL;
    hi_u32 size;

    size = surface->stride[0] * surface->height;
    surface->mem_handle[0].mem_handle = hi_unf_mem_new("sample-tde", strlen("sample-tde"), size, HI_FALSE);
    if (surface->mem_handle[0].mem_handle <= 0) {
        return -1;
    }
    virt = (hi_u8 *)hi_unf_mem_map(surface->mem_handle[0].mem_handle, size);
    if (virt == HI_NULL) {
        hi_unf_mem_delete(surface->mem_handle[0].mem_handle);
        surface->mem_handle[0].mem_handle = -1;
        return -1;
    }
    fread(virt, 1, size, fp);
    hi_unf_mem_unmap(virt, size);
    return 0;
}

static hi_s32 tde_read_uv_data(hi_unf_gfx2d_surface *surface, FILE *fp)
{
    hi_u8 *virt = HI_NULL;
    hi_u32 size;

    size = surface->stride[1] * surface->height;
    if (size != 0) {
        surface->mem_handle[1].mem_handle = hi_unf_mem_new("sample-tde", strlen("sample-tde"), size, HI_FALSE);
        if (surface->mem_handle[1].mem_handle <= 0) {
            return -1;
        }
        virt = (hi_u8 *)hi_unf_mem_map(surface->mem_handle[1].mem_handle, size);
        if (virt == HI_NULL) {
            hi_unf_mem_delete(surface->mem_handle[1].mem_handle);
            surface->mem_handle[1].mem_handle = -1;
            return -1;
        }
        fread(virt, 1, size, fp);
        hi_unf_mem_unmap(virt, size);
    }
    return 0;
}

static hi_void tde_set_rect(hi_unf_gfx2d_surface *surface)
{
    /* only use in src_suface */
    surface->in_rect.x = 0;
    surface->in_rect.y = 0;
    surface->in_rect.width = surface->width;
    surface->in_rect.height = surface->height;

    /* only use in dst_suface */
    surface->out_rect.x = 0;
    surface->out_rect.y = 0;
    surface->out_rect.width = surface->width;
    surface->out_rect.height = surface->height;
}
hi_s32 gfx2d_create_surface_by_file(const hi_char *file, hi_unf_gfx2d_surface *surface)
{
    FILE *fp = HI_NULL;
    hi_s32 ret;

    memset(surface, 0, sizeof(hi_unf_gfx2d_surface));

    ret = tde_get_resource_file_info(file, surface);
    if (ret != 0) {
        return ret;
    }

    fp = fopen(file, "rb");
    if (fp == HI_NULL) {
        SAMPLE_GFX2D_PRINT("error when open file %s, line:%d\n", file, __LINE__);
        return -1;
    }

    surface->alpha_ext.alpha0 = 0;
    surface->alpha_ext.alpha1 = 0xff;
    surface->is_alpha_ext_1555 = HI_TRUE;
    surface->is_alpha_max_255 = HI_TRUE;
    tde_set_rect(surface);

    ret = tde_read_clut_data(surface, fp);
    if (ret < 0) {
        goto err;
    }
    ret = tde_read_y_data(surface, fp);
    if (ret < 0) {
        goto err;
    }
    ret = tde_read_uv_data(surface, fp);
    if (ret < 0) {
        goto err;
    }
    fclose(fp);
    hi_unf_mem_flush(surface->mem_handle[0].mem_handle);
    return 0;
err:
    if (fp != HI_NULL) {
        fclose(fp);
    }
    gfx2d_destroy_surface(surface, 1);
    return -1;
}

hi_s32 gfx2d_dup_surface(hi_unf_gfx2d_surface *in_surface, hi_u32 size, hi_unf_gfx2d_surface *out_surface)
{
    hi_u8 *virt = HI_NULL;

    memcpy(out_surface, in_surface, sizeof(hi_unf_gfx2d_surface));
    out_surface->palette_mem_handle.mem_handle = -1;
    out_surface->format = HI_UNF_GFX2D_FMT_ARGB8888;
    out_surface->stride[0] = out_surface->width * 4; /* 4: ARGB8888 stride */
    out_surface->mem_handle[0].mem_handle = hi_unf_mem_new("gfx2d-dst", strlen("gfx2d-dst"),
        out_surface->stride[0] * out_surface->height, HI_FALSE);
    if (out_surface->mem_handle[0].mem_handle <= 0) {
        SAMPLE_GFX2D_PRINT("error when call hi_unf_mem_new, line:%d\n", __LINE__);
        return -1;
    }

    virt = (hi_u8 *)hi_unf_mem_map(out_surface->mem_handle[0].mem_handle,
        out_surface->stride[0] * out_surface->height);
    memset(virt, 0, out_surface->stride[0] * out_surface->height);
    hi_unf_mem_unmap(virt, out_surface->stride[0] * out_surface->height);

    return 0;
}

hi_s32 gfx2d_destroy_surface(hi_unf_gfx2d_surface *surface, hi_u32 size)
{
    hi_u32 i;

    for (i = 0; i < HI_UNF_GFX2D_COMPONENT; i++) {
        if (surface->mem_handle[i].mem_handle > 0) {
            hi_unf_mem_delete(surface->mem_handle[i].mem_handle);
            surface->mem_handle[i].mem_handle = -1;
        }
    }
    if (surface->palette_mem_handle.mem_handle > 0) {
        hi_unf_mem_delete(surface->palette_mem_handle.mem_handle);
        surface->palette_mem_handle.mem_handle = -1;
    }

    return 0;
}

struct fb_dmabuf_export {
    __u32 fd;
    __u32 flags;
};
#define FBIOGET_DMABUF         _IOR('F', 0x21, struct fb_dmabuf_export)

static hi_s32 gfx2d_set_vinfo(int fd, struct fb_var_screeninfo *var, hi_unf_gfx2d_surface *surface,
    struct fb_fix_screeninfo *fix, struct fb_dmabuf_export *fb_dma_buf)
{
    struct fb_bitfield r32 = {16, 8, 0}; /* 16 8 0: ARGB8888 */
    struct fb_bitfield g32 = {8, 8, 0}; /* 8 8 0: ARGB8888 */
    struct fb_bitfield b32 = {0, 8, 0}; /* 0 8 0: ARGB8888 */
    struct fb_bitfield a32 = {24, 8, 0}; /* 24 8 0: ARGB8888 */

    if (ioctl(fd, FBIOGET_VSCREENINFO, var) < 0) {
        SAMPLE_GFX2D_PRINT("Get variable screen info failed!\n");
        return -1;
    }

    var->xres = var->xres_virtual = surface->width;
    var->yres = surface->height;
    var->yres_virtual = surface->height;

    var->transp = a32;
    var->red = r32;
    var->green = g32;
    var->blue = b32;
    var->bits_per_pixel = 32; /* 32 ARGB8888 */

    if (ioctl(fd, FBIOPUT_VSCREENINFO, var) < 0) {
        SAMPLE_GFX2D_PRINT("Put variable screen info failed!\n");
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, var) < 0) {
        SAMPLE_GFX2D_PRINT("Put variable screen info failed!\n");
        return -1;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, fix) < 0) {
        SAMPLE_GFX2D_PRINT("Get fix screen info failed!\n");
        return -1;
    }

    if (ioctl(fd, FBIOGET_DMABUF, fb_dma_buf) < 0) {
        SAMPLE_GFX2D_PRINT ("Couldn't get FBIOGET_DMABUF\n");
        return -1;
    }
    return 0;
}

static hi_s32 gfx2d_begin_job(int fd, struct fb_var_screeninfo *var, hi_handle *handle)
{
    hi_s32 ret;
    if (ioctl(fd, FBIOPAN_DISPLAY, var) < 0) {
        printf("pan_display failed!\n");
        return -1;
    }

    ret = hi_unf_gfx2d_open(HI_UNF_GFX2D_DEV_ID_0);
    if (ret < 0) {
        SAMPLE_GFX2D_PRINT("%s, line %d, ret = 0x%x",  __FUNCTION__, __LINE__, ret);
        return -1;
    }

    *handle = hi_unf_gfx2d_create(HI_UNF_GFX2D_DEV_ID_0);
    if (*handle == HI_ERR_TDE_INVALID_HANDLE) {
        SAMPLE_GFX2D_PRINT("%s, line %d, handle = 0x%x",  __FUNCTION__, __LINE__, *handle);
        return -1;
    }
    return 0;
}

static hi_void gfx2d_set_fb_surface(hi_unf_gfx2d_surface *fb_surface, hi_u32 fd, hi_u32 line_length,
    hi_unf_gfx2d_surface *surface)
{
    fb_surface->mem_handle[0].mem_handle = fd;
    fb_surface->is_alpha_max_255 = HI_TRUE;
    fb_surface->is_ycbcr_clut = 0;
    fb_surface->format = HI_UNF_GFX2D_FMT_ARGB8888;
    fb_surface->height = surface->height;
    fb_surface->width = surface->width;
    fb_surface->stride[0] = line_length;
    fb_surface->out_rect.x = 0;
    fb_surface->out_rect.y = 0;
    fb_surface->out_rect.width = surface->in_rect.width;
    fb_surface->out_rect.height = surface->in_rect.height;
}

hi_void gfx2d_print_surface(hi_unf_gfx2d_surface *surface, hi_u32 size)
{
    hi_u32 i;

    SAMPLE_GFX2D_PRINT("===================================\n");
    SAMPLE_GFX2D_PRINT("format: %u\n", surface->format);
    SAMPLE_GFX2D_PRINT("width: %u\n", surface->width);
    SAMPLE_GFX2D_PRINT("height: %u\n", surface->height);

    for (i = 0; i < HI_UNF_GFX2D_COMPONENT; i++) {
        SAMPLE_GFX2D_PRINT("stride[%u]: %u\n", i, surface->stride[i]);
        SAMPLE_GFX2D_PRINT("mem_handle[%u]: %lld\n", i, surface->mem_handle[i].mem_handle);
    }
    SAMPLE_GFX2D_PRINT("in [%d %d %u %u] out[%d %d %u %u]\n",
        surface->in_rect.x, surface->in_rect.y, surface->in_rect.width, surface->in_rect.height,
        surface->out_rect.x, surface->out_rect.y, surface->out_rect.width, surface->out_rect.height);
    SAMPLE_GFX2D_PRINT("===================================\n");
}

hi_s32 gfx2d_show_surface(hi_unf_gfx2d_surface *surface, hi_u32 size)
{
    hi_s32 ret;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    struct fb_dmabuf_export fb_dma_buf;
    hi_unf_gfx2d_surface fb_surface = {0};
    hi_handle handle;
    hi_unf_gfx2d_list surface_list = {0};
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        return -1;
    }

    display_init();

    surface->in_rect.x = 0;
    surface->in_rect.y = 0;
    surface->in_rect.width = surface->width;
    surface->in_rect.height = surface->height;

    ret = gfx2d_set_vinfo(fd, &var, surface, &fix, &fb_dma_buf);
    if (ret < 0) {
        goto ERR;
    }
    gfx2d_set_fb_surface(&fb_surface, fb_dma_buf.fd, fix.line_length, surface);

    ret = gfx2d_begin_job(fd, &var, &handle);
    if (ret < 0) {
        goto ERR;
    }

    surface_list.src_surface_cnt = 1;
    surface_list.src_surfaces = surface;
    surface_list.dst_surface = &fb_surface;
    ret = hi_unf_gfx2d_bitblit(handle, &surface_list);
    if (ret < 0) {
        SAMPLE_GFX2D_PRINT("%s, line %d, ret = 0x%x\n",  __FUNCTION__, __LINE__, ret);
        goto ERR;
    }

    ret = hi_unf_gfx2d_submit(handle, HI_TRUE, TDE_SAMPLE_TIMEOUT);
    if (ret < 0) {
        SAMPLE_GFX2D_PRINT("%s, line %d, ret = 0x%x\n",  __FUNCTION__, __LINE__, ret);
        goto ERR;
    }

    SAMPLE_GFX2D_PRINT("Showing Bitmap\n");
    getchar();
    close(fd);
    display_deinit();
    return 0;

ERR:
    close(fd);
    return -1;
}

hi_s32 gfx2d_save_surface_to_file(const hi_char *file, hi_unf_gfx2d_surface *surface)
{
    hi_u8 *vir = HI_NULL;
    FILE *fp = HI_NULL;
    hi_u32 i;

    fp = fopen(file, "wb");
    if (fp == NULL) {
        printf("%s %d open file failed\n", __func__, __LINE__);
        return HI_FAILURE;
    }

    for (i = 0; i < HI_UNF_GFX2D_COMPONENT; i++) {

        if (surface->mem_handle[i].mem_handle > 0) {
            vir = (hi_u8 *)hi_unf_mem_map(surface->mem_handle[i].mem_handle, surface->size[i]);
            if (vir == NULL) {
                printf("%s %d map failed\n", __func__, __LINE__);
                return HI_FAILURE;
            }
            fwrite(vir, 1, surface->size[i], fp);
            hi_unf_mem_unmap(vir, surface->size[i]);
        }
    }
    fclose(fp);

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __cplusplus */

