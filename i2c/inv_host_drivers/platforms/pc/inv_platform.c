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
 * file inv478x_platform.c
 *
 * @brief INV9679 platform implementation example
 *
 *****************************************************************************/

/***** #include statements ***************************************************/

#include <time.h>
#include <windows.h>
#include "inv_datatypes.h"
#include "inv478x_platform_api.h"
#include "inv_hal_aardvark_api.h"

/***** local macro definitions ***********************************************/

/***** local static variable ******************************************************/

static bool_t restrt_val;

/***** public functions ******************************************************/

void inv_platform_sleep_msec(uint16_t msec)
{
    Sleep(msec);
}

uint32_t inv_platform_time_msec_query(void)
{
    return clock();
}

uint16_t inv_platform_host_block_write(inv_platform_dev_id dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size)
{
    switch (dev_id) {
    case 0x30:
        inv_hal_aardvark_i2c_slave_address_set(0x30);
        break;
    default:
        inv_hal_aardvark_i2c_slave_address_set(0x40);
        break;
    }
    return (uint16_t) inv_hal_aardvark_block_write(addr, p_data, size);
}

uint16_t inv_platform_host_block_read(inv_platform_dev_id dev_id, uint16_t addr, uint8_t *p_data, uint16_t size)
{
    switch (dev_id) {
    case 0x30:
        inv_hal_aardvark_i2c_slave_address_set(0x30);
        break;
    default:
        inv_hal_aardvark_i2c_slave_address_set(0x40);
        break;
    }
    return (uint16_t) inv_hal_aardvark_block_read(addr, p_data, size);
}

void inv_platform_device_reset_set(bool_t *b_on)
{
    restrt_val = *b_on;
    /*
     * Note for supporting multiple Inv478x devives through one Aardvark interface:
     */
    /*
     * Since Aardvark supports only one reset signal all Inv478x devices must be connected to this reset signal
     */
    inv_hal_aardvark_device_reset_set(*b_on);
}

void inv_platform_device_reset_get(inv_platform_dev_id dev_id, bool_t *b_on)
{
    dev_id = dev_id;
    *b_on = restrt_val;
}

enum inv_platform_status inv_platform_ipc_semaphore_create(const char *p_name, uint32_t max_count, int32_t initial_value, inv_platform_semaphore *p_sem_id)
{
    p_name = p_name;
    max_count = max_count;
    initial_value = initial_value;
    p_sem_id = p_sem_id;
    return INV478x_PLATFORM_STATUS__SUCCESS;
}

enum inv_platform_status inv_platform_ipc_semaphore_delete(inv_platform_semaphore sem_id)
{
    sem_id = sem_id;
    return INV478x_PLATFORM_STATUS__SUCCESS;
}

enum inv_platform_status inv_platform_ipc_semaphore_give(inv_platform_semaphore sem_id)
{
    sem_id = sem_id;
    return INV478x_PLATFORM_STATUS__SUCCESS;
}

enum inv_platform_status inv_platform_ipc_semaphore_take(inv_platform_semaphore sem_id, int32_t time_msec)
{
    sem_id = sem_id;
    time_msec = time_msec;
    return INV478x_PLATFORM_STATUS__SUCCESS;
}

void inv_platform_log_print(const char *p_str)
{
    /*
     * p_str = p_str;
     */
    printf(p_str);
}

void inv_platform_assert(const char *p_file_str, uint32_t line_no)
{
    char line_no_str[20];
    INV_SPRINTF (line_no_str, "%ld", line_no);
    printf("Assertion Failure in ", 0);
    printf((char *) p_file_str, 0);
    printf(" at line no ", 0);
    printf(line_no_str, 0);
    printf("\n", 0);
    {
        uint8_t i = 1;
        while (i);
    }
}

/***** end of file ***********************************************************/
