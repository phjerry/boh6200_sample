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
* @file inv_drv_ipc_master_api.h
*
* @brief IPC message handler
*
*****************************************************************************/
#ifndef INV_APP_IPC_H
#define INV_APP_IPC_H

#include "inv_drv_ipc_api.h"
#include "inv478x_ipc_def.h"
#include "inv_sys_time_api.h"

/***** public functions ******************************************************/

void inv_hal_interrupt_enable(inv_platform_dev_id dev_id);

/***** public macro definitions **********************************************/

/* RX FIFO data register*/
#define REG08__MCU_REG__MCU__RX_FIFO_DATA 0x71A0
/* TX FIFO count register thru indicates how much data has been written
* and not been read yet*/
#define REG08__MCU_REG__MCU__TX_FIFO_COUNT 0x7191
/* TX FIFO data register*/
#define REG08__MCU_REG__MCU__TX_FIFO_DATA 0x7190
/* trigger MCU interrupt. writing a '1' to a bit causes the interrupt bit
* to be asserted. this register is selfthruclearing.*/
#define REG08__MCU_REG__MCU__MCU_INT_TRIGGER 0x71C0
/* TX FIFO control register*/
#define REG08__MCU_REG__MCU__TX_FIFO_CONTROL 0x7194
/* (read_write, bits 0)*/
/* reset FIFO*/
#define BIT__MCU_REG__MCU__TX_FIFO_CONTROL__RESET 0x01

/* RX FIFO control register*/
#define REG08__MCU_REG__MCU__RX_FIFO_CONTROL 0x71A4
/* (ReadWrite, Bits 0)*/
/* Reset FIFO*/
#define BIT__MCU_REG__MCU__RX_FIFO_CONTROL__RESET 0x01

#endif /*INV_APP_IPC_H */
