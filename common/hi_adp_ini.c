/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description : Read value from ini file.
 * Author        : sdk
 * Create        : 2019-11-01
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "hi_adp_ini.h"
#include "securec.h"

#define hi_adp_ini_open_read(filename, file) ((*(file) = fopen((filename), "rb")) != NULL)
#define hi_adp_ini_close(file) (fclose(*(file)) == 0)
#define hi_adp_ini_read(buffer, size, file) (fgets((buffer), (size), *(file)) != NULL)
#define hi_adp_ini_atof(string) (float)strtod((string), NULL)

#define hi_ini_warn(format, arg...)    printf("[WARN]: %s,%d: " format, __FUNCTION__, __LINE__, ##arg)

typedef struct  hi_ini_string {
    const ini_data_section  *ini_data;
    hi_char  key[SECTION_MAX_LENGTH];
    hi_char  value[SECTION_MAX_LENGTH];
} ini_string;

static hi_char *skip_head(const hi_char *str)
{
    while (*str != '\0' && *str <= ' ') {
        str++;
    }
    return (hi_char *)str;
}

static hi_char *skip_tail(const hi_char *str, const hi_char *base)
{
    while (str > base && *(str - 1) <= ' ') {
        str--;
    }
    return (hi_char *)str;
}

static hi_char *strip_tail(hi_char *str)
{
    hi_char *p = skip_tail(strchr(str, '\0'), str);
    if (p != NULL) {
        *p = '\0';
    }
    return str;
}

static hi_char *copy_string(hi_char *dest, const hi_char *source, hi_u32 len)
{
    hi_u32 i;

    for (i = 0; i < len - 1 && source[i] != '\0'; i++) {
        dest[i] = source[i];
    }

    dest[i] = '\0';

    return dest;
}

static hi_char *clean_string(hi_char *string)
{
    hi_s32 is_string;
    hi_char *p = NULL;

    is_string = 0;
    for (p = string; *p != '\0' && ((*p != ';' && *p != '#') || is_string); p++) {
        if (*p == '"') {
            if (*(p + 1) == '"') {
                p++;
            } else {
                is_string = !is_string;
            }
        } else if (*p == '\\' && *(p + 1) == '"') {
            p++;
        }
    }

    *p = '\0';
    strip_tail(string);
    if (*string == '"' && (p = strchr(string, '\0')) != NULL && *(p - 1) == '"') {
        string++;
        *--p = '\0';
    }

    return string;
}

static hi_s32 get_string(FILE **fp, const ini_string *ini_string_data, hi_char *buffer, hi_s32 size)
{
    hi_char *start = NULL;
    hi_char *end = NULL;
    hi_s32 len;
    hi_char buf[HI_INI_BUFSIZE] = {0};
    if (ini_string_data == NULL || ini_string_data->ini_data == NULL) {
        hi_ini_warn("invalid para!\n");
        return HI_FAILURE;
    }

    len = strlen(ini_string_data->ini_data->section);
    if (len > 0) {
        do {
            if (!hi_adp_ini_read(buf, HI_INI_BUFSIZE, fp)) {
                hi_ini_warn("hi_adp_ini_read fail!\n");
                return HI_FAILURE;
            }
            start = skip_head(buf);
            end = strchr(start, ']');
        } while (*start != '[' || end == NULL || ((hi_s32)(end - start - 1) != len ||
            strncasecmp(start + 1, ini_string_data->ini_data->section, len) != 0));
    }

    len = (hi_s32)strlen(ini_string_data->key);
    do {
        if (!hi_adp_ini_read(buf, HI_INI_BUFSIZE, fp) || *(start = skip_head(buf)) == '[') {
            return HI_FAILURE;
        }
        start = skip_head(buf);
        end = strchr(start, '=');
        if (end == NULL) {
            end = strchr(start, ':');
        }
    } while (*start == ';' || *start == '#' || end == NULL || ((hi_s32)(skip_tail(end, start) - start) != len ||
             strncasecmp(start, ini_string_data->key, len) != 0));

    start = skip_head(end + 1);
    start = clean_string(start);
    copy_string(buffer, start, size);

    return HI_SUCCESS;
}

hi_s32 hi_adp_ini_get_string(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH],
                             hi_char *buffer, hi_s32 size)
{
    FILE *fp = NULL;
    hi_s32 ret = 0;
    char path[PATH_MAX] = {0};
    ini_string ini_string_data = { NULL, "", "" };
    if (ini_data == NULL || buffer == NULL || size <= 0 || key == NULL) {
        return 0;
    }
    ini_string_data.ini_data = ini_data;

    if (memcpy_s(ini_string_data.key, SECTION_MAX_LENGTH, key, SECTION_MAX_LENGTH) != EOK) {
        hi_ini_warn("copy:%s failed!\n", key);
        return 0;
    }
    if (realpath(ini_data->file, path) == NULL) {
        hi_ini_warn("realpath file:%s failed!\n", path);
        return 0;
    }
    if (hi_adp_ini_open_read(path, &fp)) {
        ret = get_string(&fp, &ini_string_data, buffer, size);
        (void)hi_adp_ini_close(&fp);
    } else {
        hi_ini_warn("open file:%s failed!\n", path);
        return 0;
    }

    if (ret != HI_SUCCESS) {
        hi_ini_warn("get string of :%s ,%s failed!\n", ini_string_data.ini_data->section, ini_string_data.key);
        return 0;
    }

    return strlen(buffer);
}

hi_s32 hi_adp_ini_get_long(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], long *value)
{
    hi_char buf[SECTION_MAX_LENGTH] = {0};
    if (ini_data == NULL || value == NULL) {
        return HI_FAILURE;
    }
    hi_s32 len = hi_adp_ini_get_string(ini_data, key, buf, sizeof(buf) / sizeof(buf[0]));
    if (len  == 0) {
        return HI_FAILURE;
    }

    *value = ((len >= 2 && toupper(buf[1]) == 'X') ?  /* 2:byte 2 */
        strtol(buf, NULL, 16) : strtol(buf, NULL, 10)); /* 16:hex, 10:decimal */
    return HI_SUCCESS;
}

hi_s32 hi_adp_ini_get_float(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], float *value)
{
    hi_char buf[64] = {0}; /* 64:buffer size */
    if (ini_data == NULL || value == NULL) {
        return HI_FAILURE;
    }
    hi_s32 len = hi_adp_ini_get_string(ini_data, key, buf, sizeof(buf) / sizeof(buf[0]));
    if (len  == 0) {
        return HI_FAILURE;
    }
    *value = hi_adp_ini_atof(buf);
    return HI_SUCCESS;
}

hi_s32 hi_adp_ini_get_bool(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_bool *value)
{
    hi_char buf[2] = {0}; /* 2:size */
    if (ini_data == NULL || value == NULL) {
        return HI_FAILURE;
    }
    hi_s32 len = hi_adp_ini_get_string(ini_data, key, buf, sizeof(buf) / sizeof(buf[0]));
    if (len == 0) {
        return HI_FAILURE;
    }
    buf[0] = (hi_char)toupper(buf[0]);
    if (buf[0] == 'Y' || buf[0] == '1' || buf[0] == 'T') {
        *value = HI_TRUE;
    } else if (buf[0] == 'N' || buf[0] == '0' || buf[0] == 'F') {
        *value = HI_FALSE;
    } else {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 hi_adp_ini_get_s32(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_s32 *value)
{
    long value_tmp = 0;
    if (ini_data == NULL || value == NULL) {
        return HI_FAILURE;
    }
    hi_s32 ret = hi_adp_ini_get_long(ini_data, key, &value_tmp);
    *value = (hi_s32)value_tmp;
    return ret;
}

hi_s32 hi_adp_ini_get_u32(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_u32 *value)
{
    long value_tmp = 0;
    if (ini_data == NULL) {
        return HI_FAILURE;
    }
    hi_s32 ret = hi_adp_ini_get_long(ini_data, key, &value_tmp);
    *value = (hi_u32)value_tmp;
    return ret;
}

hi_s32 hi_adp_ini_get_u16(const ini_data_section *ini_data, const hi_char key[SECTION_MAX_LENGTH], hi_u16 *value)
{
    long value_tmp = 0;
    if (ini_data == NULL || value == NULL) {
        return HI_FAILURE;
    }
    hi_s32 ret = hi_adp_ini_get_long(ini_data, key, &value_tmp);
    *value = (hi_u16)value_tmp;
    return ret;
}