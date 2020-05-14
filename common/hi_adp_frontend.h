/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description : adp frontend head
 * Author        : sdk
 * Create        : 2019-11-01
 */
#ifndef     HI_ADP_FRONTEND_H
#define     HI_ADP_FRONTEND_H

#include "hi_type.h"
#include "hi_unf_frontend.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

hi_s32 hi_adp_frontend_init(hi_void);
hi_s32 hi_adp_frontend_connect(hi_u32 port, hi_u32 freq, hi_u32 symbol_rate, hi_u32 third_param);
hi_s32 hi_adp_frontend_get_connect_para(hi_u32 port, hi_unf_frontend_connect_para *connect_para);
hi_void hi_adp_frontend_deinit(hi_void);

#endif

