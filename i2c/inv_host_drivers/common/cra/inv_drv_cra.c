/*****************************************************************************
 *                                                                           *
 * INVECAS CONFIDENTIAL                                                      *
 * ____________________                                                      *
 *                                                                           *
 * [2018] - [2023] INVECAS, Incorporated                                     *
 * All Rights Reserved.                                                      *
 *                                                                           *
 * NOTICE:  All information contained herein is, and remains                 *
 * the property of INVECAS, Incorporated, its licensors and suppliers,       *
 * if any.  The intellectual and technical concepts contained                *
 * herein are proprietary to INVECAS, Incorporated, its licensors            *
 * and suppliers and may be covered by U.S. and Foreign Patents,             *
 * patents in process, and are protected by trade secret or copyright law.   *
 * Dissemination of this information or reproduction of this material        *
 * is strictly forbidden unless prior written permission is obtained         *
 * from INVECAS, Incorporated.                                               *
 *                                                                           *
 *****************************************************************************/
/**
* file inv_drv_cra.c
*
* brief :- CRA (common register access) driver
*
*****************************************************************************/
/*#define INV_DEBUG 3*/
/***** #include statements ***************************************************/
#include "inv_common.h"
#include "inv478x_platform_api.h"
#include "inv_drv_cra_api.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hi_unf_i2c.h"

#include "hi_unf_i2c.h"
#include <unistd.h>

//#include <windows.h>
/***** Register Module name **************************************************/
INV_MODULE_NAME_SET (drv_cra, 5);
/***** local macro definitions ***********************************************/
inv_platform_dev_id s_slave_address = 0x40;
typedef void *HANDLE;

#define HI_INFO_I2C(format, arg...) printf( format, ## arg)

#define BITS_SET(inp, mask, bset)   ((b_set) ? (inp | mask) : (inp & (~mask)))
#define BITS_MOD(inp, mask, val)    ((inp & (~mask)) | (val & mask))
/***** local prototypes ******************************************************/
static void s_int32_to_data(uint8_t bytes, uint8_t *p_arr, uint32_t val);
inv_rval_t inv_platform_semaphore_create(const char* p_name, uint32_t max_count, uint32_t initial_value, HANDLE * p_sem_id);
static uint32_t s_data_to_int32(uint8_t bytes, uint8_t *p_arr);
static void s_reg_write(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t bytes, uint32_t val);
static uint32_t s_reg_read(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t bytes);
/***** local data objects ****************************************************/
/***** public functions ******************************************************/
void inv_drv_cra_create(inv_platform_dev_id dev_id)
{
    dev_id = dev_id;
}

void inv_drv_cra_delete(inv_platform_dev_id dev_id)
{
    dev_id = dev_id;
}

void inv_drv_cra_write_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t val)
{
    s_reg_write(dev_id, addr, 1, val);
}

uint8_t inv_drv_cra_read_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr)
{
    return (uint8_t) s_reg_read(dev_id, addr, 1);
}

void inv_drv_cra_bits_set_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t mask, bool_t b_set)
{
    uint8_t temp;
    temp = (uint8_t) s_reg_read(dev_id, addr, 1);
    temp = BITS_SET (temp, mask, b_set);
    s_reg_write(dev_id, addr, 1, temp);
}

void inv_drv_cra_bits_mod_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t mask, uint8_t val)
{
    uint8_t temp;
    temp = (uint8_t) s_reg_read(dev_id, addr, 1);
    temp = BITS_MOD (temp, mask, val);
    s_reg_write(dev_id, addr, 1, temp);
}

void inv_drv_cra_write_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint16_t val)
{
    s_reg_write(dev_id, addr, 2, val);
}

uint16_t inv_drv_cra_read_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr)
{
    return (uint16_t) s_reg_read(dev_id, addr, 2);
}

void inv_drv_cra_bits_set_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint16_t mask, bool_t b_set)
{
    uint16_t temp;
    temp = (uint16_t) s_reg_read(dev_id, addr, 2);
    temp = BITS_SET (temp, mask, b_set);
    s_reg_write(dev_id, addr, 2, temp);
}

void inv_drv_cra_bits_mod_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint16_t mask, uint16_t val)
{
    uint16_t temp;
    temp = (uint16_t) s_reg_read(dev_id, addr, 2);
    temp = BITS_MOD (temp, mask, val);
    s_reg_write(dev_id, addr, 2, temp);
}

void inv_drv_cra_write_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t val)
{
    s_reg_write(dev_id, addr, 3, val);
}

uint32_t inv_drv_cra_read_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr)
{
    return (uint32_t) s_reg_read(dev_id, addr, 3);
}

void inv_drv_cra_bits_set_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, bool_t b_set)
{
    uint32_t temp;
    temp = (uint32_t) s_reg_read(dev_id, addr, 3);
    temp = BITS_SET (temp, mask, b_set);
    s_reg_write(dev_id, addr, 3, temp);
}

void inv_drv_cra_bits_mod_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, uint32_t val)
{
    uint32_t temp;
    temp = (uint32_t) s_reg_read(dev_id, addr, 3);
    temp = BITS_MOD (temp, mask, val);
    s_reg_write(dev_id, addr, 3, temp);
}

void inv_drv_cra_write_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t val)
{
    s_reg_write(dev_id, addr, 4, val);
}

uint32_t inv_drv_cra_read_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr)
{
    return (uint32_t) s_reg_read(dev_id, addr, 4);
}

void inv_drv_cra_bits_set_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, bool_t b_set)
{
    uint32_t temp;
    temp = (uint32_t) s_reg_read(dev_id, addr, 4);
    temp = BITS_SET (temp, mask, b_set);
    s_reg_write(dev_id, addr, 4, temp);
}

void inv_drv_cra_bits_mod_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, uint32_t val)
{
    uint32_t temp;
    temp = (uint32_t) s_reg_read(dev_id, addr, 4);
    temp = BITS_MOD (temp, mask, val);
    s_reg_write(dev_id, addr, 4, temp);
}

void inv_drv_cra_fifo_write_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, const uint8_t *p_data, inv_drv_cra_size size)
{
    //while (size--) {
    //  s_reg_write(dev_id,  addr,  1,  *p_data);
    //  p_data++;
    //}

    inv_drv_cra_bits_set_8(dev_id, 0x7486, 0x01, false);
    inv_drv_cra_block_write_8(dev_id, addr, p_data, size);
    inv_drv_cra_bits_set_8(dev_id, 0x7486, 0x01, true);
}

void inv_drv_cra_fifo_read_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t *p_data, inv_drv_cra_size size)
{
    //while (size--) {
    //  *p_data = (uint8_t)s_reg_read(dev_id,  addr,  1);
    //  p_data++;
    //}

    inv_drv_cra_bits_set_8(dev_id, 0x7486, 0x01, false);
    inv_drv_cra_block_read_8(dev_id, addr, p_data, size);
    inv_drv_cra_bits_set_8(dev_id, 0x7486, 0x01, true);
}

void inv_drv_cra_block_write_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, const uint8_t *p_data, inv_drv_cra_size size)
{
    inv_platform_block_write(dev_id, addr, p_data, size);
    /*
     * (*sp_platform_fns.i2c_write)(s_slave_address, addr, p_data, size);
     */
}

void inv_drv_cra_block_read_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t *p_data, inv_drv_cra_size size)
{
    inv_platform_block_read(dev_id, addr, p_data, size);
    /*
     * (*sp_platform_fns.i2c_read)(s_slave_address, addr, p_data, size);
     */
}

/***** local functions *******************************************************/
static void s_int32_to_data(uint8_t bytes, uint8_t *p_arr, uint32_t val)
{
    while (bytes--) {
        *p_arr = (uint8_t) val;
        p_arr++;
        val >>= 8;
    }
}

static uint32_t s_data_to_int32(uint8_t bytes, uint8_t *p_arr)
{
    uint32_t val = 0;
    p_arr += bytes;
    while (bytes--) {
        val <<= 8;
        p_arr--;
        val += *p_arr;
    }
    return val;
}

static void s_reg_write(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t bytes, uint32_t val)
{
    uint8_t tmp[4];
	hi_s32 ret = HI_FAILURE;
	
	hi_unf_i2c_addr_info i2c_addr_info;
	i2c_addr_info.dev_address = 0x40;
    i2c_addr_info.reg_addr = addr;
    i2c_addr_info.reg_addr_count = bytes;
	
    s_int32_to_data(bytes, tmp, val);
	HI_INFO_I2C("s_reg_write \n");
	/* Read data from Device */
	   ret = hi_unf_i2c_write(0, &i2c_addr_info, tmp, bytes);
	   if (ret != HI_SUCCESS) {
		   HI_INFO_I2C("i2c write failed!\n");
	   } else {
		   HI_INFO_I2C("i2c write success!\n");
	   }
	/*    
	i2c_para.i2c_num = 0;
    i2c_para.dev_address = 0x60;
    i2c_para.i2c_reg_addr = 0x2;
    i2c_para.reg_addr_count = 1;
    data = 0xa;
    hi_unf_i2c_write(i2c_para, &data, 1);*/
    
   // inv_platform_block_write(dev_id, addr, tmp, bytes);
    /*
     * (*sp_platform_fns.i2c_write)(dev_id, addr, tmp, bytes);
     */
}

static uint32_t s_reg_read(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t bytes)
{

	hi_s32 ret = HI_FAILURE;
    uint8_t tmp[4] = { 0, 0, 0, 0 };
	 hi_unf_i2c_addr_info i2c_addr_info;
	i2c_addr_info.dev_address = 0x40;
    i2c_addr_info.reg_addr = addr;
    i2c_addr_info.reg_addr_count = bytes;
	 
	
	 /* Read data from Device */
    ret = hi_unf_i2c_read(0, &i2c_addr_info, tmp, bytes);
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("call HI_I2C_Read failed.\n");
        hi_unf_i2c_deinit();

        return ret;
    }

   // inv_platform_block_read(dev_id, addr, tmp, bytes);
    /*
     * (*sp_platform_fns.i2c_read)(dev_id, addr, tmp, bytes);
     */
    return s_data_to_int32(bytes, tmp);
}

/** END of File *********************************************************/
