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
* @file inv478x_platform_api.h
*
* @brief Minimum Platform API function set for inv478x Driver
*
*****************************************************************************/

#ifndef INV478X_PLATFORM_API_H
#define INV478X_PLATFORM_API_H

#ifdef __cplusplus
extern "C" {
#endif

/***** #include statements ***************************************************/

#include "inv_datatypes.h"
#include "inv478x_app_def.h"

/***** public macro definitions **********************************************/

#define  inv_platform_semaphore    uint32_t
#define  inv_platform_dev_id    uint8_t

#define INV_SYS_TIME_MILLI_T    uint32_t
#define INV478X_RETVAL__SEMAPHORE_IPC_EXPIRE          ((inv_rval_t) 70000)  // IPC semaphore error
#define INV478X_RETVAL__SEMAPHORE_HANDLE_EXPIRE       ((inv_rval_t) 80000)  // IPC semaphore error
#define INV478X_RETVAL__SEMAPHORE_API_EXPIRE          ((inv_rval_t) 90000)  // IPC semaphore error

/***** public type definitions ***********************************************/

    enum inv_platform_status {
        INV478X_PLATFORM_STATUS__SUCCESS,
        INV478X_PLATFORM_STATUS__ERR_INVALID_PARAM,
        INV478X_PLATFORM_STATUS__ERR_NOT_AVAIL,
        INV478X_PLATFORM_STATUS__ERR_FAILED
    };

/***** public functions ******************************************************/

    inv_rval_t inv_platform_semaphore_take(uint32_t sem_id, uint32_t time_msec);
    inv_rval_t inv_platform_semaphore_give(inv_platform_semaphore sem_id);
    void inv_platform_adapter_select(uint8_t dev_id, inv_inst_t adapt_inst);
    uint16_t inv_platform_block_read(uint8_t dev_id, uint16_t addr, uint8_t *p_data, uint16_t size);
    uint16_t inv_platform_block_write(uint8_t dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size);

/*****************************************************************************/
/**
* @brief Blocking sleep function
*
* This function should not exit until ’msec’ milliseconds of time.
* The Inv478x host driver uses this function for brief delays(less than 100 milliseconds).
*
* @param[in]  msec            Number of milliseconds that this function should wait before it returns.
*
******************************************************************************/
    void inv_platform_sleep_msec(uint32_t msec);

/*****************************************************************************/
/**
* @brief Time inquiry in milliseconds
*
* Provides time in milliseconds passed since system was powered up.
*
* @retval       Absolute time in milliseconds started from system power up.
*
******************************************************************************/
    extern uint32_t inv_platform_time_msec_query(void);

/*****************************************************************************/
/**
* @brief I2C/SPI Data write function
*
* Implements either I2C or SPI write access to the Inv478x by writing ‘size’ bytes to memory
* starting at ‘addr’. If multiple devices are present on the platform ‘dev_id’ is used by this
* function to determine which device to access. This function should return ‘0’ if all bytes
* are successfully written and should return ‘-1’ if the write fails.
*
* @param[in]  dev_id           Hardware device ID passed to ‘inv478xcreate()’.
*                             This ID is associated with a physical hardware device
*                             and is allocated by the user when this function is implemented.
* @param[in]  addr            16 bit address of register.
* @param[in]  p_data           Pointer to array of bytes that is required to be written to register ‘addr’.
* @param[in]  size            Number of bytes in array that is required to be written to device.
*
* @retval                      0 : Write was successful
*                             -1 : Write has failed
*
******************************************************************************/
    extern uint16_t inv_platform_host_block_write(inv_platform_dev_id dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size);

/*****************************************************************************/
/**
* @brief I2C/SPI Data read function
* Implements either I2C or SPI read access to the Inv478x by reading ‘size’ bytes from memory
* starting at ‘addr’. If multiple devices are present on the platform ‘dev_id’ is used by this
* function to determine which device to access. This function should return ‘0’ if all bytes
* are successfully written and should return ‘-1’ if the write fails.
*
* @param[in]  dev_id           Hardware device ID passed to ‘inv478xcreate()’.
*                             This ID is associated with a physical hardware device
*                             and is allocated by the user when this function is implemented.
* @param[in]  addr            16 bit address of register.
* @param[in]  p_data           Pointer to array of bytes that is required to be read from register ‘addr’.
* @param[in]  size            Number of bytes in array that is required to be read from device.
*
* @retval                      0 : Read was successful
*                             -1 : Read has failed
*
******************************************************************************/
    extern uint16_t inv_platform_host_block_read(inv_platform_dev_id dev_id, uint16_t addr, uint8_t *p_data, uint16_t size);

/*****************************************************************************/
/**
* @brief Device Hardware Reset
*
* Controls level of reset signal connected to device's RST pin.
*
* @param[in]  dev_id           Hardware device ID passed to ‘inv478xcreate()’.
*                             This ID is associated with a physical hardware device
*                             and is allocated by the user when this function is implemented.
* @param[in]  b_on             Requested level of reset signal:
*                             false : No reset
*                             true  : Active reset
*
******************************************************************************/
    uint8_t inv_platform_device_reset_set(uint8_t dev_id, bool_t *b_on);

#if (INV478X_USER_DEF__MULTI_THREAD)
/*****************************************************************************/
/**
* @brief Semaphore-Take wrapper(Obsoleted. Replaced by inv_platform_semaphore_take())
*
* Optional function for implementing a semaphore function that blocks until either
* ‘inv_platform_ipc_semaphore_give()’ is called by the Inv478x host driver or when
* ‘max_block_time’ expires. This function is called by Inv478x host driver directly
* after host driver has sent a request to Inv478x device and is waiting for a
* response from the inv478x firmware.
* This function is only called if ‘b_ipc_semaphore’ flag in the inv_config_t
* structure is is set to ‘true’. If ‘b_ipc_semaphore’ is set to ‘false’ an empty
* implementation of this function must be provided to avoid linkage errors.
* Please refer ‘inv478xrx_porting_guide’ for guidance on how to implement.
*
* @param[in]  max_block_time    Maximum time in milliseconds that the semaphore remains blocking
*
* @retval                     Describes reason that semaphore stops blocking:
*                             0 : Released by inv_platform_ipc_semaphore_give
*                             1 : Released when max_block_time expires
*
******************************************************************************/
//extern int_t inv_platform_ipc_semaphore_take(uint32_t max_block_time);

/*****************************************************************************/
/**
* @brief Semaphore-Give wrapper(Obsoleted. Replaced by inv_platform_semaphore_give())
*
* Optional function that releases ‘inv_platform_ipc_semaphore_take()’ from being blocked.
* This function is called by Inv478x host driver directly after host driver has
* received an acknowledgement to a message from Inv478x device.
* This function is only called if ‘b_ipc_semaphore’ flag in the inv_config_t
* structure is is set to ‘true’. If ‘b_ipc_semaphore’ is set to ‘false’ an empty
* implementation of this function must be provided to avoid linkage errors. Please
* refer ‘inv478xrx_porting_guide’ for guidance on how to implement.
*
******************************************************************************/
//extern void inv_platform_ipc_semaphore_give(void);
    extern enum inv_platform_status inv_platform_ipc_semaphore_create(const char *p_name, uint32_t max_count, uint32_t initial_value, inv_platform_semaphore *p_sem_id);
    extern enum inv_platform_status inv_platform_ipc_semaphore_delete(inv_platform_semaphore sem_id);

/*****************************************************************************/
/**
* @brief Semaphore-Give wrapper
*
* Optional function that releases ‘inv_platform_ipc_semaphore_take()’ from being blocked.
* This function is called by Inv478x host driver directly after host driver has
* received an acknowledgement to a message from Inv478x device.
* This function is only called if ‘b_ipc_semaphore’ flag in the inv_config_t
* structure is is set to ‘true’. If ‘b_ipc_semaphore’ is set to ‘false’ an empty
* implementation of this function must be provided to avoid linkage errors. Please
* refer ‘inv478xrx_porting_guide’ for guidance on how to implement.
*
******************************************************************************/
    extern enum inv_platform_status inv_platform_ipc_semaphore_give(inv_platform_semaphore sem_id);

/*****************************************************************************/
/**
* @brief Semaphore-Take wrapper
*
* Optional function for implementing a semaphore function that blocks until either
* ‘inv_platform_ipc_semaphore_give()’ is called by the Inv478x host driver or when
* ‘max_block_time’ expires. This function is called by Inv478x host driver directly
* after host driver has sent a request to Inv478x device and is waiting for a
* response from the inv478x firmware.
* This function is only called if ‘b_ipc_semaphore’ flag in the inv_config_t
* structure is is set to ‘true’. If ‘b_ipc_semaphore’ is set to ‘false’ an empty
* implementation of this function must be provided to avoid linkage errors.
* Please refer ‘inv478xrx_porting_guide’ for guidance on how to implement.
*
* @param[in]  max_block_time    Maximum time in milliseconds that the semaphore remains blocking
*
* @retval                     Describes reason that semaphore stops blocking:
*                             0 : Released by inv_platform_ipc_semaphore_give
*                             1 : Released when max_block_time expires
*
******************************************************************************/
    extern enum inv_platform_status inv_platform_ipc_semaphore_take(inv_platform_semaphore sem_id, int32_t time_msec);
#endif                          /* INV478X_USER_DEF__MULTI_THREAD */

/*****************************************************************************/
/**
* @brief ASCII string logger
*
* Function for outputting NULL terminated log strings to a platform specific
* logging mechanism such as a UART or log file.
*
* @param[in]  p_str    Pointer to string to be logged.
*
******************************************************************************/
    extern void inv_platform_log_print(const char *p_str);

#ifdef __cplusplus
}
#endif
#endif                          /* INV478X_PLATFORM_API_H */
