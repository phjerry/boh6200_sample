#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <assert.h>
#include <pthread.h>
#include "hi_type.h"
#include "hi_adp_mpi.h"
#include "hifb.h"
#include "hi_fb_ui.h"

#define HIFB_WIDTH 3840
#define HIFB_HEIGHT 2160
#define HIFB_WIDTH_LAST 1920
#define HIFB_HEIGHT_LAST 1080

#define BLUE   0xff0000ff
#define RED    0xffff0000
#define GREEN  0xff00ff00
#define WHITE  0xffffffff
#define YELLOW 0xffffff00
#define BLACK 0x00000000
#define ARGB8888TOARGB1555(color) (((color & 0x80000000) >> 16) | ((color & 0xf80000) >> 9) | ((color & 0xf800) >> 6) | ((color & 0xf8) >> 3))


#ifndef CONFIG_SUPPORT_CA_RELEASE
#define Printf  printf
#else
#define Printf(x...)
#endif

typedef struct {
    HI_S32      fb_index;
    pthread_t   fb_thread_id;
    HI_BOOL is_stop_fb_thread;
} hi_fb_ui_info;


static hi_fb_ui_info g_fb_ui_info[FB_NUM];

static HI_VOID* g_mapped_mem[FB_NUM] = {HI_NULL};
static HI_ULONG g_mapped_offset[FB_NUM] = {0};
static HI_ULONG g_mapped_memlen[FB_NUM] = {0};
static HI_BOOL PRINTCMDINFO = HI_FALSE;


static struct fb_var_screeninfo g_hifb_st_def_vinfo =
{
    HIFB_WIDTH,    //visible resolution xres
    HIFB_HEIGHT, // yres
    HIFB_WIDTH, //virtual resolution xres_virtual
    HIFB_HEIGHT, //yres_virtual
    0, //xoffset
    0, //yoffset
    32, //bits per pixel
    0, //grey levels, 1 means black/white
    {16, 8, 0}, //fb_bitfiled red
    { 8, 8, 0}, // green
    { 0, 8, 0}, //blue
    {24, 8, 0}, // transparency
    0,  //non standard pixel format
    FB_ACTIVATE_FORCE,
    0, //height of picture in mm
    0, //width of picture in mm
    0, //acceleration flags
    -1, //pixclock
    -1, //left margin
    -1, //right margin
    -1, //upper margin
    -1, //lower margin
    -1, //hsync length
    -1, //vsync length
};

static struct fb_var_screeninfo g_hifb_st_def_vinfo_last =
{
    HIFB_WIDTH_LAST,    //visible resolution xres
    HIFB_HEIGHT_LAST, // yres
    HIFB_WIDTH_LAST, //virtual resolution xres_virtual
    HIFB_HEIGHT_LAST, //yres_virtual
    0, //xoffset
    0, //yoffset
    32, //bits per pixel
    0, //grey levels, 1 means black/white
    {16, 8, 0}, //fb_bitfiled red
    { 8, 8, 0}, // green
    { 0, 8, 0}, //blue
    {24, 8, 0}, // transparency
    0,  //non standard pixel format
    FB_ACTIVATE_FORCE,
    0, //height of picture in mm
    0, //width of picture in mm
    0, //acceleration flags
    -1, //pixclock
    -1, //left margin
    -1, //right margin
    -1, //upper margin
    -1, //lower margin
    -1, //hsync length
    -1, //vsync length
};


static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
    Printf( "Printing vinfo:\n");
    Printf("txres: %d\n", vinfo->xres);
    Printf( "tyres: %d\n", vinfo->yres);
    Printf( "txres_virtual: %d\n", vinfo->xres_virtual);
    Printf( "tyres_virtual: %d\n", vinfo->yres_virtual);
    Printf( "txoffset: %d\n", vinfo->xoffset);
    Printf( "tyoffset: %d\n", vinfo->yoffset);
    Printf( "tbits_per_pixel: %d\n", vinfo->bits_per_pixel);
    Printf( "tgrayscale: %d\n", vinfo->grayscale);
    Printf( "tnonstd: %d\n", vinfo->nonstd);
    Printf( "tactivate: %d\n", vinfo->activate);
    Printf( "theight: %d\n", vinfo->height);
    Printf( "twidth: %d\n", vinfo->width);
    Printf( "taccel_flags: %d\n", vinfo->accel_flags);
    Printf( "tpixclock: %d\n", vinfo->pixclock);
    Printf( "tleft_margin: %d\n", vinfo->left_margin);
    Printf( "tright_margin: %d\n", vinfo->right_margin);
    Printf( "tupper_margin: %d\n", vinfo->upper_margin);
    Printf( "tlower_margin: %d\n", vinfo->lower_margin);
    Printf( "thsync_len: %d\n", vinfo->hsync_len);
    Printf( "tvsync_len: %d\n", vinfo->vsync_len);
    Printf( "tsync: %d\n", vinfo->sync);
    Printf( "tvmode: %d\n", vinfo->vmode);
    Printf( "tred: %d/%d\n", vinfo->red.length, vinfo->red.offset);
    Printf( "tgreen: %d/%d\n", vinfo->green.length, vinfo->green.offset);
    Printf( "tblue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
    Printf( "talpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
}

static void print_finfo(struct fb_fix_screeninfo *finfo)
{
    Printf( "Printing finfo:\n");
    Printf( "tsmem_start = %p\n", (char *)finfo->smem_start);
    Printf( "tsmem_len = %d\n", finfo->smem_len);
    Printf( "ttype = %d\n", finfo->type);
    Printf( "ttype_aux = %d\n", finfo->type_aux);
    Printf( "tvisual = %d\n", finfo->visual);
    Printf( "txpanstep = %d\n", finfo->xpanstep);
    Printf( "typanstep = %d\n", finfo->ypanstep);
    Printf( "tywrapstep = %d\n", finfo->ywrapstep);
    Printf( "tline_length = %d\n", finfo->line_length);
    Printf( "tmmio_start = %p\n", (char *)finfo->mmio_start);
    Printf( "tmmio_len = %d\n", finfo->mmio_len);
    Printf( "taccel = %d\n", finfo->accel);
}

void PrintfCMDInfo()
{
    if (PRINTCMDINFO)
    {
        return;
    }

    Printf("u can execute this sample like this:\n");
    Printf("\r\r ./sample_fb /dev/fb0 TAB 32\n");
    Printf("/dev/fb0 --> framebuffer device\n");
    Printf("TAB      --> stereo mode top and bottom\n");
    Printf("32       --> pixel format ARGB8888\n");
    Printf("for more information, please refer to the document readme.txt\n");

    PRINTCMDINFO = HI_TRUE;
}

void DrawBox(HIFB_RECT *pstRect, HI_U32 stride,
             HI_U8* pmapped_mem, HI_U32 color, HI_U32 u32Bpp)
{
    HI_U8 *pMem;
    HI_U32 column,row;
    HI_U32 u32Color = color;

    if (u32Bpp != 4 && u32Bpp != 2)
    {
        Printf("DrawBox just support pixelformat ARGB1555&ARGB8888");
        return;
    }

    if (2 == u32Bpp)
    {
        u32Color = ARGB8888TOARGB1555(color);
    }

    for (column = pstRect->y;column < (pstRect->y+pstRect->h);column++)
    {
        pMem =     pmapped_mem + column*stride;
        for (row = pstRect->x;row < (pstRect->x+pstRect->w);row++)
        {
            if (2 == u32Bpp)
            {
                *(HI_U16*)(pMem + row*u32Bpp) = (HI_U16)u32Color;
            }
            else if (4 == u32Bpp)
            {
                *(HI_U32*)(pMem + row*u32Bpp) = u32Color;
            }
        }
    }

}


hi_void* fb_thread(void *args)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    HIFB_BUFFER_S CanvasBuf;
    HI_U32 u32BufSize = 0;
    HI_U32 u32DisPlayBufSize = 0;
    HI_U32 u32LineLen = 0;
    HIFB_ALPHA_S stAlpha;
    HI_S32 console_fd;
    HIFB_RECT stRect;
    hi_fb_ui_info *fb_ui_info = (hi_fb_ui_info*)args;
    HI_VOID* mapped_mem = g_mapped_mem[fb_ui_info->fb_index];
    HI_ULONG mapped_offset = g_mapped_offset[fb_ui_info->fb_index];
    HI_ULONG mapped_memlen = g_mapped_memlen[fb_ui_info->fb_index];
    HI_U32 i, j = 1;
    HI_U32 u32Bpp;
    HI_U32 u32Color[4] = {RED,BLUE,WHITE,BLACK};
    HI_U32 u32Color1[4] = {0};
    Printf("------------------------------------------index %d be created!\n\n", fb_ui_info->fb_index);

    const char *file;
    switch (fb_ui_info->fb_index)
    {
    case 0:
        file = "/dev/fb0";
        break;
    case 1:
        file = "/dev/fb1";
        break;
    case 2:
        file = "/dev/fb2";
        break;
    case 3:
        file = "/dev/fb3";
        break;
    default:
        file = "/dev/fb0";
        break;
    }

    console_fd = open(file, O_RDWR, 0);
    if (console_fd < 0)
    {
        Printf ( "Unable to open %s\n", file);
        PrintfCMDInfo();
        return HI_NULL;
    }

    if (fb_ui_info->fb_index == 3) {
         if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &g_hifb_st_def_vinfo_last) < 0)
        {
            Printf ( "Unable to set variable screeninfo!\n");
            goto CLOSEFD;
        }
    } else {
    /* set color format ARGB8888, screen size: 3980*2160 */
        if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &g_hifb_st_def_vinfo) < 0)
        {
            Printf ( "Unable to set variable screeninfo!\n");
            goto CLOSEFD;
        }
    }
    /* Get the fix screen info of hardware */
    if (ioctl(console_fd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        Printf ( "Couldn't get console hardware info\n");
        goto CLOSEFD;
    }

    print_finfo(&finfo);

    mapped_memlen = finfo.smem_len + mapped_offset;
    if (fb_ui_info->fb_index == 3) {
        u32DisPlayBufSize = g_hifb_st_def_vinfo_last.xres_virtual*g_hifb_st_def_vinfo_last.yres_virtual*(g_hifb_st_def_vinfo_last.bits_per_pixel/8);
    } else {
        u32DisPlayBufSize = g_hifb_st_def_vinfo.xres_virtual*g_hifb_st_def_vinfo.yres_virtual*(g_hifb_st_def_vinfo.bits_per_pixel/8);
    }
    if (mapped_memlen != 0 && mapped_memlen >= u32DisPlayBufSize)
    {
        u32BufSize = finfo.smem_len;
        u32LineLen = finfo.line_length;
        mapped_mem = mmap(NULL, mapped_memlen,
                          PROT_READ | PROT_WRITE, MAP_SHARED, console_fd, 0);
        if (mapped_mem == (char *)-1)
        {
            Printf ( "Unable to memory map the video hardware\n");
            mapped_mem = NULL;
            goto CLOSEFD;
        }
    }

    /* Determine the current screen depth */
    if (ioctl(console_fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        Printf ( "Couldn't get vscreeninfo\n");
        goto UNMAP;
    }

    u32Bpp = vinfo.bits_per_pixel/8;
    print_vinfo(&vinfo);

    /* set alpha */
    stAlpha.bAlphaEnable  = HI_TRUE;
    stAlpha.u8Alpha0 = 0xa0;
    stAlpha.u8Alpha1 = 0xa0;

    /* set global alpha */
    stAlpha.bAlphaChannel = HI_TRUE;
    stAlpha.u8GlobalAlpha = 0x1a;//0xf0,0x80..

    if (ioctl(console_fd, FBIOPUT_ALPHA_HIFB, &stAlpha) < 0)
    {
        Printf ( "Couldn't set alpha\n");
        goto UNMAP;
    }

    memset(mapped_mem, 0x0, u32BufSize);
        stRect.x = 0;
        stRect.y = 0;
        stRect.w = 500;
        stRect.h = 500;
    while (fb_ui_info->is_stop_fb_thread == HI_FALSE) {

        DrawBox(&stRect, u32LineLen, mapped_mem, u32Color[0], u32Bpp);
        stRect.x += 500;
        DrawBox(&stRect, u32LineLen, mapped_mem, u32Color[1], u32Bpp);
        stRect.y += 500;
        DrawBox(&stRect, u32LineLen, mapped_mem, u32Color[2], u32Bpp);
        stRect.x -= 500;
        DrawBox(&stRect, u32LineLen, mapped_mem, u32Color[3], u32Bpp);
        stRect.y -= 500;
        if (mapped_memlen == 0 ||mapped_memlen < u32DisPlayBufSize) {
            CanvasBuf.UpdateRect.x = 0;
            CanvasBuf.UpdateRect.y = 0;
            CanvasBuf.UpdateRect.w = CanvasBuf.stCanvas.u32Width;
            CanvasBuf.UpdateRect.h = CanvasBuf.stCanvas.u32Height;
            if (ioctl(console_fd, FBIO_REFRESH, &CanvasBuf) < 0) {
                Printf("refresh buffer info failed!\n");
            }
        } else {
            if (ioctl(console_fd, FBIOPAN_DISPLAY, &vinfo) < 0) {
                Printf("pan_display failed!\n");
            }
        }
        usleep(20*1000);
        for(i=0;i<4;i++) {
            u32Color1[j] = u32Color[i];
            j++;
            if (j == 4) {
                j=0;
            }
        }
        for(i=0;i<4;i++) {
            u32Color[i] = u32Color1[i];
        }
    }

    return HI_NULL;

UNMAP:
    if (mapped_memlen != 0)
    {
        munmap(mapped_mem, mapped_memlen);
    }
CLOSEFD:
    close(console_fd);

    return HI_NULL;
}


HI_S32 hi_fb_ui(HI_S32 k)
{
    HI_S32 Ret;
    hi_fb_ui_info *fb_info = &g_fb_ui_info[k];
    fb_info->fb_index = k;
    fb_info->fb_thread_id = -1;
    fb_info->is_stop_fb_thread = HI_FALSE;
    Ret = pthread_create(&fb_info->fb_thread_id, HI_NULL, (hi_void *)fb_thread, (hi_void *)fb_info);
    if (Ret != HI_SUCCESS) {
        perror("[fb_ui] pthread_create fb error");
        return Ret;
    }
    return Ret;
}
