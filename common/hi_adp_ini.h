/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description : adp ini head
 * Author        : sdk
 * Create        : 2019-11-01
 */

#ifndef HI_ADP_INI_H
#define HI_ADP_INI_H

#include <stdio.h>
#include <limits.h>
#include "hi_type.h"

#define HI_INI_BUFSIZE 512
#define SECTION_MAX_LENGTH 64

typedef struct hi_ini_section {
    hi_char file[PATH_MAX];
    hi_char section[SECTION_MAX_LENGTH];
} ini_data_section;

hi_s32 hi_adp_ini_get_bool(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_bool *value);
hi_s32 hi_adp_ini_get_long(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], long *value);
hi_s32 hi_adp_ini_get_string(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_char *buffer,
    hi_s32 size);
hi_s32 hi_adp_ini_get_float(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], float *value);
hi_s32 hi_adp_ini_get_s32(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_s32 *value);
hi_s32 hi_adp_ini_get_u32(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_u32 *value);
hi_s32 hi_adp_ini_get_u16(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_u16 *value);
#endif /* HI_ADP_INI_H */
