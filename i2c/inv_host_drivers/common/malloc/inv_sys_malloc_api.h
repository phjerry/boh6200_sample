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
* @file inv_sys_malloc_api.h
*
* @brief Dynamic memory allocation from static memory pool
*
*****************************************************************************/

#ifndef INV_SYS_MALLOC_API_H
#define INV_SYS_MALLOC_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

/***** public functions ******************************************************/

void *inv_sys_malloc_create(uint16_t size);
void inv_sys_malloc_delete(void *p);
uint16_t inv_sys_malloc_bytes_allocated_get(void);
void inv_sys_malloc_lock(void);
void inv_sys_malloc_delete_all(void);

#endif /* INV_SYS_MALLOC_API_H */

/***** end of file ***********************************************************/
