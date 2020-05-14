/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2014-2019. All rights reserved.
 * Description:
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hi_unf_i2c.h"

#define HI_INFO_I2C(format, arg...) printf( format, ## arg)


hi_s32 main(hi_s32 argc, hi_char **argv)
{
    hi_s32 ret = HI_FAILURE;

    hi_u8 *data = HI_NULL;

    hi_u32 device_address = 0;
    hi_u32 i2c_num  = 0;
    hi_u32 reg_addr = 0;
    hi_u32 reg_addr_count = 0;
    hi_u32 read_number = 0;
    hi_unf_i2c_addr_info i2c_addr_info;

    hi_u32 loop;

    if (argc != 6) { /* para number 6 */
        HI_INFO_I2C("Usage: i2c_read  i2c_channel  device_addr  register_addr  ");
        HI_INFO_I2C("register_addr_len  read_bytes_number\n");

        return -1;
    }

    i2c_num = strtol(argv[1], NULL, 0);
    device_address = strtol(argv[2], NULL, 0);
    reg_addr = strtol(argv[3], NULL, 0);
    reg_addr_count = strtol(argv[4], NULL, 0);
    if (reg_addr_count > 4) { /* 4 byte */
        HI_INFO_I2C("register address length is error!\n");
        return -1;
    }

    read_number = strtol(argv[5], NULL, 0);

    ret = hi_unf_i2c_init();
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    data = (hi_u8 *)malloc(read_number);
    if (data == HI_NULL) {
        HI_INFO_I2C("\n pReadData malloc() error!\n");
        hi_unf_i2c_deinit();

        return HI_FAILURE;
    }

    i2c_addr_info.dev_address = device_address;
    i2c_addr_info.reg_addr = reg_addr;
    i2c_addr_info.reg_addr_count = reg_addr_count;
    /* Read data from Device */
    ret = hi_unf_i2c_read(i2c_num, &i2c_addr_info, data, read_number);
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("call HI_I2C_Read failed.\n");
        free(data);
        hi_unf_i2c_deinit();

        return ret;
    }

    HI_INFO_I2C("\ndata read:\n");

    for(loop = 0; loop < read_number; loop++) {
        HI_INFO_I2C("0x%02x ", data[loop]);
    }

    HI_INFO_I2C("\n\n");

    free(data);

    hi_unf_i2c_deinit();

    return HI_SUCCESS;
}
