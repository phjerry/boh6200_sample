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
* file inv478x_hal.c
*
* brief :- inv478x hal
*
*****************************************************************************/
/***** #include statements ***************************************************/
#include "inv_drv_cra_api.h"
#include "inv478x_hal.h"
#include "inv_common.h"
#include "inv_drv_ipc_api.h"
/***** global variables *******************************************************/

/***** local macro definitions ***********************************************/


/*#define INV__FIFO_MCU_ENABLE*/
#define I2C_ADDR                                0x40
#define FIFO_MAX_SIZE                           64

#define REG_ADDR__MCU__TX_FIFO_DATA             (REG_MCU_OFFSSET | 0x90)
#define REG_ADDR__MCU__TX_FIFO_COUNT            (REG_MCU_OFFSSET | 0x91)
#define REG_ADDR__MCU__RX_FIFO_DATA             (REG_MCU_OFFSSET | 0xA0)
#define REG_ADDR__MCU__RX_FIFO_CONTROL          (REG_MCU_OFFSSET | 0xA4)
#define REG_ADDR__MCU__EXT_INT_STATUS           (REG_MCU_OFFSSET | 0x32)
#define REG_ADDR__MCU__MCU_INT_TRIGGER          (REG_MCU_OFFSSET | 0xC0)
#define REG_ADDR__MCU__MCU_INT_MASK             (REG_MCU_OFFSSET | 0xC1)
#define BIT_MSK__MCU__MCU_INT_MASK              0x01
#define BIT_MSK__MCU__MCU_INT_CLEAR             0x01

/*
* MCU write to TX FIFO, but read from RX FIFO
* HOST write to RX FIFO, but read from TX FIFO
*/
/* Interrupt registers */
#define REG_ADDR__INT_MASK                      REG_ADDR__MCU__MCU_INT_MASK /*host enable mcu fifo interrupt */
#define REG_ADDR__INT_TRIGGER                   REG_ADDR__MCU__MCU_INT_TRIGGER  /*host trigger mcu interrupt */
#define REG_ADDR__INT_STATUS                    REG_ADDR__MCU__EXT_INT_STATUS   /*host accept interrupt from mcu */
#define REG__BIT_MASK                           BIT_MSK__MCU__MCU_INT_MASK
#define REG__BIT_CLEAR                          BIT_MSK__MCU__MCU_INT_CLEAR
/* FIFO registers */
#define REG_ADDR__FIFO_CNT                      REG_ADDR__MCU__TX_FIFO_COUNT    /*host check if any bytes in tx fifo */
#define REG_ADDR__FIFO_READ_DATA                REG_ADDR__MCU__TX_FIFO_DATA /*host read from tx fifo */
#define REG_ADDR__FIFO_WRITE_DATA               REG_ADDR__MCU__RX_FIFO_DATA /*host write to rx fifo */
#define REG_ADDR__FIFO_STATUS                   REG_ADDR__MCU__RX_FIFO_STATUS
#define REG_ADDR__FIFO_CONTROL                  REG_ADDR__MCU__RX_FIFO_CONTROL

#define REG__INT_CLEAR()   (inv_drv_cra_write_8(dev_id, REG_ADDR__INT_STATUS, REG__BIT_CLEAR))
#define REG__FIFO_RESET()  (inv_drv_cra_write_8(dev_id, REG_ADDR__FIFO_CONTROL, 0x01))
#define REG__FIFO_DATA()   (inv_drv_cra_read_8(dev_id, REG_ADDR__FIFO_READ_DATA))
#define REG__FIFO_WRITE(a) (inv_drv_cra_write_8(dev_id, REG_ADDR__FIFO_WRITE_DATA, a))
#define REG__MODE_SET(a)   (inv_drv_cra_write_24(dev_id, REG_ADDR__MODE_SELECT, a))

void inv_fifo_init(inv_platform_dev_id dev_id)
{
    REG__FIFO_RESET ();
    inv_drv_cra_write_8(dev_id, REG_ADDR__INT_MASK, 0x07);

}

void inv_fifo_int_clear(inv_platform_dev_id dev_id)
{
    REG__INT_CLEAR ();
}
