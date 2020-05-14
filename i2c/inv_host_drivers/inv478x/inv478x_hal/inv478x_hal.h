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
* @file inv_hal_fifo_api.h
*
* @brief Fifo HAL APIs
*
*****************************************************************************/

#ifndef HAL_FIFO__T
#define HAL_FIFO__T
#include "inv_datatypes.h"

#define REG08__MCU_REG__MCU__EXT_INT_STATUS            0x7132
#define REG08__MCU_REG__MCU__MCU_INT_MASK            0x71C1
#define REG08__MCU_REG__MCU__EXT_INT_TRIGGER                0x7130
#define REG08__MCU_REG__MCU__MCU_STATUS                0x7114
#define REG08__MCU_REG__MCU__MCU_INT_TRIGGER                0x71C0
#define REG08__MCU_REG__MCU__MCU_CONFIG                0x7110
#define REG_MCU_OFFSSET                        0x7100
#define REG_ADDR__MODE_SELECT                    0x7050

#define INV478X_SLAVE_ADDRESS                                   0x40
#define REG08_MCU_STATUS                                        REG08__MCU_REG__MCU__MCU_STATUS
#define REG08_MCU_INT_TRIGGER                                   REG08__MCU_REG__MCU__MCU_INT_TRIGGER
#define INV478X_FLAG_NACK                                       0x4

/* bootloader status*/
#define REG08__BOOT_REG__BT__BOOT_STATUS                        0x7020
/* (read_only, bits 4)*/
/* section data checksum error*/
#define BIT__BOOT_REG__BT__BOOT_STATUS__CHECKSUM_ERROR          0x10
/* (read_only, bits 3)*/
/* section header checksum error*/
#define BIT__BOOT_REG__BT__BOOT_STATUS__SECTION_ERROR           0x08
/* (read_only, bits 2)*/
/* boot sequence opcode error*/
#define BIT__BOOT_REG__BT__BOOT_STATUS__OPCODE_ERROR            0x04
/* (read_only, bits 1)*/
/* header read failed after multiple retries*/
#define BIT__BOOT_REG__BT__BOOT_STATUS__HEADER_ERROR            0x02
/* (read_only, bits 0)*/
/* boot sequence is done with no errors*/
#define BIT__BOOT_REG__BT__BOOT_STATUS__BOOT_DONE               0x01

/* AON clock select register*/
#define REG08__AON_TOP_REG__AON_CLK_SEL_REG 0x7408
/* PCM config register*/
#define REG08__AON_TOP_REG__PCM_CONFIG 0x741B
/* boot configuration*/
#define REG08__BOOT_REG__BT__BOOT_CONFIG 0x7010
/* MCU configuration*/
#define REG08__MCU_REG__MCU__MCU_CONFIG 0x7110
/* reset MCU*/
#define BIT__MCU_REG__MCU__MCU_CONFIG__MCU_RESET 0x01
/* ISP configuration*/
#define REG08__ISP_REG__ISP__ISP_CONFIG 0x7210
/* enable ISP function*/
#define BIT__ISP_REG__ISP__ISP_CONFIG__ENABLE 0x01
/* enable SPI access mode*/
#define BIT__ISP_REG__ISP__ISP_CONFIG__SPI_ACCESS_MODE 0x03
/* ISP opcode register*/
#define REG08__ISP_REG__ISP__ISP_OPCODE 0x7240
/* ISP address register*/
#define REG24__ISP_REG__ISP__ISP_ADDR 0x7244
/* AONDIG configuration0 register*/
#define REG08__AON_DIG_REG__DIG_CONFIG0 0x7486
/* ISP data register*/
#define REG08__ISP_REG__ISP__ISP_DATA 0x7248
/* bootloader status*/
#define REG08__BOOT_REG__BT__BOOT_STATUS 0x7020
/* boot sequence is done with no errors*/
#define BIT__BOOT_REG__BT__BOOT_STATUS__BOOT_DONE 0x01

enum fifo_status {
    INV__FIFO_SUCCESS,
    INV__FIFO_NOT_SUCCESS,
    INV__FIFO_EMPTY,
    INV__FIFO_FULL,
    INV__FIFO_OVERRUN,
    INV__FIFO_UNDERRUN
};
void inv_fifo_init(inv_platform_dev_id dev_id);
enum fifo_status inv_fifo_write(uint8_t *ptr, uint8_t size);
uint8_t inv_fifo_read(uint8_t *ptr);
void inv_fifo_int_clear(inv_platform_dev_id dev_id);

void inv_mode_set(uint8_t a);

#endif /*HAL_FIFO__T */
