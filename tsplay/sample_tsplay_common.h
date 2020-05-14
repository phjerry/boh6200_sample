/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: avplay mpi interface
 * Author: Hisilicon multimedia software group
 * Create: 2012-12-22
*/

#include "hi_unf_acodec.h"
#include "hi_unf_video.h"

#ifndef SAMPLE_TSPLAY_COMMON_H_
#define SAMPLE_TSPLAY_COMMON_H_

hi_s32 get_vdec_type(hi_char *type, hi_unf_vcodec_type *vdec_type, hi_u32 *private1, hi_u32 *private2);
hi_s32 get_adec_type(hi_char *type, hi_u32 *adec_type);

#endif
