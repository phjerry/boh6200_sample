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
    hi_u32 write_number = 0;
    hi_unf_i2c_addr_info i2c_addr_info;

    hi_u32 loop;

    if (argc < 6) { /* para number 6 */
        HI_INFO_I2C("Usage: i2c_write  i2c_channel  device_addr  register_addr  ");
        HI_INFO_I2C("register_addr_len  write_bytes_number  byte0 [... byten]\n");

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

    write_number = strtol(argv[5], NULL, 0);

    if (write_number + 6 > argc) { /* para number 6 */
        HI_INFO_I2C("input error!\n");
        HI_INFO_I2C("Usage: i2c_write  i2c_channel  device_addr  register_addr  ");
        HI_INFO_I2C("register_addr_len  write_bytes_number  byte0 [... byten]\n");
        return -1;
    }

    ret = hi_unf_i2c_init();
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    data = (hi_u8 *)malloc(write_number);
    if (data == HI_NULL) {
        HI_INFO_I2C("\n malloc() error!\n");
        hi_unf_i2c_deinit();

        return HI_FAILURE;
    }

    for (loop = 0; loop < write_number; loop++) {
        data[loop] = strtol(argv[loop + 6], NULL, 0); /* 6 para */
    }

    i2c_addr_info.dev_address = device_address;
    i2c_addr_info.reg_addr = reg_addr;
    i2c_addr_info.reg_addr_count = reg_addr_count;

    /* Read data from Device */
    ret = hi_unf_i2c_write(i2c_num, &i2c_addr_info, data, write_number);
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("i2c write failed!\n");
    } else {
        HI_INFO_I2C("i2c write success!\n");
    }

    free(data);

    hi_unf_i2c_deinit();

    return ret;
}
