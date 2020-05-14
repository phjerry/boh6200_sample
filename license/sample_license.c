/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: license sample.
 */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_unf_system.h"
#include "hi_unf_pdm.h"

#define LICENSE_FILEL_SIZE  1024
#define FILE_W_R            0755
#define CMD_MIN_LEN         2
#define CMD_MAX_LEN         3
#define CMD_INDEX_1         1
#define CMD_INDEX_2         2

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_s32 bin_fd;
    hi_u8  buf[LICENSE_FILEL_SIZE];
    hi_u32 read_len;
    hi_unf_sys_chip_id chip_info;

    if (argc < CMD_MIN_LEN) {
        printf("usage: --read/-r                     :read die id\n");
        printf("usage: --set/-s  input_license_name  :set license file\n");
        printf("usage: --get/-g  output_license_name :get license file\n");
        printf("usage: --help/-h                     :show the information and exit\n");
        return HI_SUCCESS;
    }

    if (argc > CMD_MAX_LEN) {
        printf("param exceed!\n");
        return HI_SUCCESS;
    }

    hi_unf_sys_init();
    if ((strcmp(argv[CMD_INDEX_1], "--help") == 0) || (strcmp(argv[CMD_INDEX_1], "-h") == 0)) {
        printf("usage: --read/-r                     :read die id\n");
        printf("usage: --set/-s  input_license_name  :set license file\n");
        printf("usage: --get/-g  output_license_name :get license file\n");
        printf("usage: --help/-h                     :show the information and exit\n");

        return HI_SUCCESS;
    } else if ((strcmp(argv[CMD_INDEX_1], "--read") == 0) || (strcmp(argv[CMD_INDEX_1], "-r") == 0)) {
        ret = hi_unf_sys_get_chip_id(&chip_info);
        if (ret != HI_SUCCESS) {
            printf("chip attr read fail\n");
            return HI_FAILURE;
        }
        printf("read die id:%llx\n", chip_info.chip_id_64);

        return HI_SUCCESS;
    } else if ((strcmp(argv[CMD_INDEX_1], "--set") == 0) || (strcmp(argv[CMD_INDEX_1], "-s") == 0)) {
        bin_fd = open(argv[CMD_INDEX_2], O_RDWR | O_NONBLOCK, FILE_W_R);
        if (bin_fd < 0) {
            printf("cannot open file! \n");
            return HI_FAILURE;
        }

        read_len = read(bin_fd, buf, LICENSE_FILEL_SIZE);
        if (read_len < 0) {
            printf("cannot read fd:%x\n", read_len);
            return HI_FAILURE;
        }
        ret = hi_unf_pdm_set_feature_license(buf, read_len);
        if (ret != HI_SUCCESS) {
            printf("call HI_UNF_PDM_SetFeatureLicense fail: %x\n", ret);
            return HI_FAILURE;
        }

        return ret;
    } else if ((strcmp(argv[CMD_INDEX_1], "-get") == 0) || (strcmp(argv[CMD_INDEX_1], "-g") == 0)) {
        bin_fd = open(argv[CMD_INDEX_2], O_RDWR | O_CREAT | O_TRUNC, FILE_W_R);
        if (bin_fd < 0) {
            printf("open %s file fail\n", argv[CMD_INDEX_2]);
            return HI_FAILURE;
        }
        ret = hi_unf_pdm_get_feature_license(buf, LICENSE_FILEL_SIZE, &read_len);
        if (ret != HI_SUCCESS) {
            printf("HI_UNF_PDM_GetFeatureLicense fail\n");
            return HI_FAILURE;
        }
        if (read_len != LICENSE_FILEL_SIZE) {
            printf("lost data in license from flash\n");
            return HI_FAILURE;
        }
        ret = write(bin_fd, buf, LICENSE_FILEL_SIZE);
        if (ret < 0) {
            printf("write data failed\n");
            return HI_FAILURE;
        }
        return HI_SUCCESS;
    }

    return HI_FAILURE;
}
