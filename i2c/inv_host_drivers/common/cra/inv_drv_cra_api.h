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
* @file inv_drv_cra_api.h
*
* @brief Chip Register Access Driver
*
*****************************************************************************/

#ifndef INV_DRV_CRA_API_H
#define INV_DRV_CRA_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"
#include "inv478x_platform_api.h"
#include "inv478x_hal.h"
#include "inv_sys_obj_api.h"
#include "inv_sys_malloc_api.h"
#include "inv_sys_log_api.h"

/***** public type definitions ***********************************************/

#define   inv_drv_cra_addr         uint16_t
#define   inv_drv_cra_size       uint16_t
#define   inv_drv_cra_idx        uint8_t

#ifdef STM32_WRAPPER
struct platform_ptrs {
    enum inv_platform_status(*i2c_write) (uint8_t, uint16_t, const uint8_t *, uint16_t);
    enum inv_platform_status(*i2c_read) (uint8_t, uint16_t, uint8_t *, uint16_t);
};
#endif
/***** public functions ******************************************************/
#ifdef STM32_WRAPPER
void inv_drv_cra_create(uint8_t dev_id);
void inv_drv_cra_delete(uint8_t dev_id);

void inv_drv_cra_write_8(uint8_t dev_id, inv_drv_cra_addr addr, uint8_t val);
uint8_t inv_drv_cra_read_8(uint8_t dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_8(uint8_t dev_id, inv_drv_cra_addr addr, uint8_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_8(uint8_t dev_id, inv_drv_cra_addr addr, uint8_t mask, uint8_t val);
void inv_drv_cra_write_16(uint8_t dev_id, inv_drv_cra_addr addr, uint16_t val);
uint16_t inv_drv_cra_read_16(uint8_t dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_16(uint8_t dev_id, inv_drv_cra_addr addr, uint16_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_16(uint8_t dev_id, inv_drv_cra_addr addr, uint16_t mask, uint16_t val);
void inv_drv_cra_write_24(uint8_t dev_id, inv_drv_cra_addr addr, uint32_t val);
uint32_t inv_drv_cra_read_24(uint8_t dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_24(uint8_t dev_id, inv_drv_cra_addr addr, uint32_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_24(uint8_t dev_id, inv_drv_cra_addr addr, uint32_t mask, uint32_t val);
void inv_drv_cra_write_32(uint8_t dev_id, inv_drv_cra_addr addr, uint32_t val);
uint32_t inv_drv_cra_read_32(uint8_t dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_32(uint8_t dev_id, inv_drv_cra_addr addr, uint32_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_32(uint8_t dev_id, inv_drv_cra_addr addr, uint32_t mask, uint32_t val);
void inv_drv_cra_fifo_write_8(uint8_t dev_id, inv_drv_cra_addr addr, const uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_fifo_read_8(uint8_t dev_id, inv_drv_cra_addr addr, uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_block_write_8(uint8_t dev_id, inv_drv_cra_addr addr, const uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_block_read_8(uint8_t dev_id, inv_drv_cra_addr addr, uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_platform_register(struct platform_ptrs *fn_ptrs);
bool inv_cra_interrupt_query(void);
#else
/*bool                inv_drv_cra_create(inv_platform_interface_t *p_interface_info);*/
/*inv_platform_status_t inv_drv_cra_delete(inv_platform_interface_t *p_interface_info);*/

void inv_drv_cra_write_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t val);
uint8_t inv_drv_cra_read_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t mask, uint8_t val);

void inv_drv_cra_write_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint16_t val);
uint16_t inv_drv_cra_read_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint16_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_16(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint16_t mask, uint16_t val);

void inv_drv_cra_write_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t val);
uint32_t inv_drv_cra_read_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_24(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, uint32_t val);

void inv_drv_cra_write_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t val);
uint32_t inv_drv_cra_read_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr);
void inv_drv_cra_bits_set_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, bool_t b_set);
void inv_drv_cra_bits_mod_32(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint32_t mask, uint32_t val);

void inv_drv_cra_fifo_write_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, const uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_fifo_read_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_block_write_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, const uint8_t *p_data, inv_drv_cra_size size);
void inv_drv_cra_block_read_8(inv_platform_dev_id dev_id, inv_drv_cra_addr addr, uint8_t *p_data, inv_drv_cra_size size);

#endif
#endif /* INV_DRV_CRA_API_H */
