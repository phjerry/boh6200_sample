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
* @file inv_drv_isp_api.h
*
* @brief ISP APIs
*
*****************************************************************************/

#ifndef INV_DRV_ISP_H
#define INV_DRV_ISP_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

/***** public constant definitions *******************************************/

#define FLASH_EDID_BASE_ADDR                 0x5000

#define INV_DRV_SPI_BUSRT_WRITE_SIZE         256

/* Flash erase timeouts in milliseconds */

#define FLASH_CHIP_ERASE_TIMEOUT            2000
#define FLASH_SECTOR_ERASE_TIMEOUT          300
#define FLASH_32K_BLOCK_ERASE_TIMEOUT               800
#define FLASH_64K_BLOCK_ERASE_TIMEOUT               1000
#define FLASH_OP_POLL_GRANULARITY           100 /* Poll operation done every 100ms */

/***** public functions ******************************************************/

void inv_drv_isp_event_int_set(inv_platform_dev_id dev_id);
void inv_drv_isp_chip_erase(inv_platform_dev_id dev_id);
void inv_drv_isp_sector_erase(inv_platform_dev_id dev_id, uint32_t addr);
bool_t inv_drv_isp_sector_erase_poll(inv_platform_dev_id dev_id, uint32_t addr);
void inv_drv_isp_block_32k_erase(inv_platform_dev_id dev_id, uint32_t addr);
bool_t inv_drv_isp_block_32k_erase_poll(inv_platform_dev_id dev_id, uint32_t addr);
void inv_drv_isp_block_64k_erase(inv_platform_dev_id dev_id, uint32_t addr);
bool_t inv_drv_isp_block_64k_erase_poll(inv_platform_dev_id dev_id, uint32_t addr);
void inv_drv_isp_burst_write(inv_platform_dev_id dev_id, uint32_t addr, const uint8_t *p_data, uint16_t len);
void inv_drv_isp_burst_read(inv_platform_dev_id dev_id, uint32_t addr, uint8_t *p_data, uint16_t len);
bool_t inv_drv_isp_operation_done(inv_platform_dev_id dev_id);
void inv_drv_isp_flash_protect(inv_platform_dev_id dev_id);
void inv_drv_isp_flash_un_protect(inv_platform_dev_id dev_id);

#endif /* INV_DRV_ISP_H */

/***** end of file ***********************************************************/
