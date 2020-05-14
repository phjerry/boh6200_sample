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
* @file inv_drv_ipc_master.c
*
* @brief Host IPC Interface
*
*****************************************************************************/
/*#define INV_DEBUG 1*/

/***** #include statements ***************************************************/

#include "inv478x_platform_api.h"
#include "inv_drv_ipc_master_api.h"
#include "inv_sys_obj_api.h"
#include "inv_common.h"
#include "inv_sys_log_api.h"
#include "inv_drv_cra_api.h"
#include "inv478x_hal.h"

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (app_ipc, 6);

/***** local macro definitions ***********************************************/

#define MAX_FIFO_SIZE                     0x40

/***** global functions ****************************************/

void inv_hal_ipc_token_assert(inv_platform_dev_id dev_id, inv_drv_ipc_token_t flags)
{
    /*
     * assert token(s)
     */
    inv_drv_cra_write_8(dev_id, REG08__MCU_REG__MCU__MCU_INT_TRIGGER, flags);
}

uint8_t inv_hal_ipc_fifo_write(inv_platform_dev_id dev_id, uint8_t *p_buf, uint8_t len)
{
    //uint8_t length = len;

    //while (len--) {
    //    inv_drv_cra_write_8(dev_id, REG08__MCU_REG__MCU__RX_FIFO_DATA, *p_buf);
    //    p_buf++;
    //}
    //return length;

    uint8_t length = len;

    inv_drv_cra_fifo_write_8(dev_id, REG08__MCU_REG__MCU__RX_FIFO_DATA, p_buf, len);

    return length;
}

uint8_t inv_hal_ipc_fifo_read(inv_platform_dev_id dev_id, uint8_t *p_buf, uint8_t len)
{
    //uint8_t length;
    //uint8_t received_bytes = MAX_FIFO_SIZE - inv_drv_cra_read_8(dev_id, REG08__MCU_REG__MCU__TX_FIFO_COUNT);

    //if (received_bytes < len)
    //    len = received_bytes;
    //length = len;

    //while (len--) {
    //    *p_buf = inv_drv_cra_read_8(dev_id, REG08__MCU_REG__MCU__TX_FIFO_DATA);
    //    p_buf++;
    //}
    //return length;


    uint8_t length;
    uint8_t received_bytes = MAX_FIFO_SIZE - inv_drv_cra_read_8(dev_id, REG08__MCU_REG__MCU__TX_FIFO_COUNT);

    if (received_bytes < len)
        len = received_bytes;
    length = len;


    inv_drv_cra_fifo_read_8(dev_id, REG08__MCU_REG__MCU__TX_FIFO_DATA, p_buf, len);

    return length;
}

void inv_hal_interrupt_enable(inv_platform_dev_id dev_id)
{
    inv_fifo_init(dev_id);
    inv_drv_cra_write_8(dev_id, REG08__MCU_REG__MCU__MCU_INT_MASK, 0x07);
    inv_fifo_int_clear(dev_id);
}

/***** end of file ***********************************************************/
