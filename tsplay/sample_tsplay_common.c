#include <stdio.h>
#include <string.h>
#include "sample_tsplay_common.h"

typedef struct {
    const hi_char *type;
    hi_unf_vcodec_type vdec_type;
    hi_bool is_read_frame_size;
} vdec_type_map;

typedef struct {
    const hi_char *type;
    hi_u32 adec_type;
} adec_type_map;

static vdec_type_map g_vdec_type_map[] = {
    {"mpeg2",    HI_UNF_VCODEC_TYPE_MPEG2,    HI_FALSE},
    {"mpeg4",    HI_UNF_VCODEC_TYPE_MPEG4,    HI_FALSE},
    {"h263",     HI_UNF_VCODEC_TYPE_H263,     HI_FALSE},
    {"sor",      HI_UNF_VCODEC_TYPE_SORENSON, HI_FALSE},
    {"vp6",      HI_UNF_VCODEC_TYPE_VP6,      HI_TRUE},
    {"vp6f",     HI_UNF_VCODEC_TYPE_VP6F,     HI_TRUE},
    {"vp6a",     HI_UNF_VCODEC_TYPE_VP6A,     HI_TRUE},
    {"h264",     HI_UNF_VCODEC_TYPE_H264,     HI_FALSE},
    {"h265",     HI_UNF_VCODEC_TYPE_H265,     HI_FALSE},
    {"mvc",      HI_UNF_VCODEC_TYPE_H264_MVC,      HI_FALSE},
    {"avs",      HI_UNF_VCODEC_TYPE_AVS,      HI_FALSE},
    {"avs2",     HI_UNF_VCODEC_TYPE_AVS2,     HI_FALSE},
    {"avs3",     HI_UNF_VCODEC_TYPE_AVS3,     HI_FALSE},
    {"real8",    HI_UNF_VCODEC_TYPE_REAL8,    HI_TRUE},
    {"real9",    HI_UNF_VCODEC_TYPE_REAL9,    HI_TRUE},
    {"vc1ap",    HI_UNF_VCODEC_TYPE_VC1,      HI_FALSE},
    {"vc1smp5",  HI_UNF_VCODEC_TYPE_VC1,      HI_TRUE},
    {"vc1smp8",  HI_UNF_VCODEC_TYPE_VC1,      HI_TRUE},
    {"vp8",      HI_UNF_VCODEC_TYPE_VP8,      HI_TRUE},
    {"vp9",      HI_UNF_VCODEC_TYPE_VP9,      HI_TRUE},
    {"av1",      HI_UNF_VCODEC_TYPE_AV1,      HI_TRUE},
    {"divx3",    HI_UNF_VCODEC_TYPE_DIVX3,    HI_TRUE},
    {"mjpeg",    HI_UNF_VCODEC_TYPE_MJPEG,    HI_TRUE},
};

static adec_type_map g_adec_type_map[] = {
    {"aac",     HI_UNF_ACODEC_ID_AAC},
    {"mp3",     HI_UNF_ACODEC_ID_MP3},
    {"truehd",  HI_UNF_ACODEC_ID_DOLBY_TRUEHD},
    {"ac3raw",  HI_UNF_ACODEC_ID_AC3PASSTHROUGH},
    {"dtsraw",  HI_UNF_ACODEC_ID_DTSPASSTHROUGH},
#if defined (DOLBYPLUS_HACODEC_SUPPORT)
    {"ddp",     HI_UNF_ACODEC_ID_DOLBY_PLUS},
#endif
    {"dts",     HI_UNF_ACODEC_ID_DTSHD},
    {"dtsm6",   HI_UNF_ACODEC_ID_DTSM6},
    {"dra",     HI_UNF_ACODEC_ID_DRA},
    {"pcm",     HI_UNF_ACODEC_ID_PCM},
    {"mlp",     HI_UNF_ACODEC_ID_TRUEHD},
    {"amr",     HI_UNF_ACODEC_ID_AMRNB},
    {"g711",    HI_UNF_ACODEC_ID_G711},
    {"g726",    HI_UNF_ACODEC_ID_G726},
    {"vorbis",  HI_UNF_ACODEC_ID_VORBIS},
    {"opus",    HI_UNF_ACODEC_ID_OPUS},
    {"adpcm",   HI_UNF_ACODEC_ID_ADPCM},
    {"amrwb",   HI_UNF_ACODEC_ID_AMRWB},
};

hi_s32 get_vdec_type(hi_char *type, hi_unf_vcodec_type *vdec_type, hi_u32 *private1, hi_u32 *private2)
{
    hi_u32 i = 0;
    hi_u32 num = sizeof(g_vdec_type_map) / sizeof(vdec_type_map);

    if (type == HI_NULL) {
        return HI_FAILURE;
    }

    for (; i < num; i++) {
        if (!strcasecmp(type, g_vdec_type_map[i].type)) {
            break;
        }
    }

    if (i >= num) {
        return HI_FAILURE;
    }

    *vdec_type = g_vdec_type_map[i].vdec_type;

    if ((private1 != HI_NULL) && (private2 != HI_NULL)) {
        if (!strcasecmp(type, "vc1ap")) {
            *private1 = 1;
            *private2 = 8;
        }

        if (!strcasecmp(type, "vc1smp5")) {
            *private1 = 0;
            *private2 = 5; /* WMV3*/
        }

        if (!strcasecmp(type, "vc1smp8")) {
            *private1 = 0;
            *private2 = 8; /* not WMV3 */
        }
    }

    return HI_SUCCESS;
}

hi_s32 get_adec_type(hi_char *type, hi_u32 *adec_type)
{
    hi_u32 adec_type_tmp = 0;
    hi_u32 i = 0;
    hi_u32 num = sizeof(g_adec_type_map) / sizeof(adec_type_map);

    for (; i< num; i++) {
        if (!strcasecmp(type, g_adec_type_map[i].type)) {
            break;
        }
    }

    if (i >= num) {
        return HI_FAILURE;
    }

    adec_type_tmp = g_adec_type_map[i].adec_type;
    *adec_type = adec_type_tmp;

    return HI_SUCCESS;
}
