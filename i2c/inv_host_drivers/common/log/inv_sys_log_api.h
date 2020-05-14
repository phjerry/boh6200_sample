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
* @file inv_sys_log_api.h
*
* @brief logging library
*
*****************************************************************************/

#ifndef INV_SYS_LOG_API_H
#define INV_SYS_LOG_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"
#include "inv_sys_obj_api.h"

/***** public macro definitions **********************************************/

#define INV_LOG_ENABLE(lev)                      (lev <= INV_DEBUG)
#define INV_LOG_HEADER(func, obj, str)           { inv_sys_log_time_stamp(s_inv_module_name_str, (char *)func, (void *)(obj)); inv_sys_log_printf str; }
#define INV_LOG_STRING(str)                      { inv_sys_log_printf str; }
#define INV_LOG_ARRAY(p, l)                      { uint16_t _len = l; uint8_t *ptr = (uint8_t *)p; while (_len--) \
                        { inv_sys_log_printf("%02X ", (uint16_t)(*(ptr++) & 0xFF)); }; inv_sys_log_printf("\n"); }
#ifndef INV_DEBUG
#define INV_LOG1A(func, obj, str)
#define INV_LOG2A(func, obj, str)
#define INV_LOG3A(func, obj, str)
#define INV_LOG1B(str)
#define INV_LOG2B(str)
#define INV_LOG3B(str)
#define INV_LOG1B_ARRAY(p, len)
#define INV_LOG2B_ARRAY(p, len)
#define INV_LOG3B_ARRAY(p, len)
#define INV_LOG1_ENABLE                        (false)
#define INV_LOG2_ENABLE                        (false)
#define INV_LOG3_ENABLE                        (false)
#else
#if (0 == INV_DEBUG)
#define INV_LOG1A(func, obj, str)
#define INV_LOG2A(func, obj, str)
#define INV_LOG3A(func, obj, str)
#define INV_LOG1B(str)
#define INV_LOG2B(str)
#define INV_LOG3B(str)
#define INV_LOG1B_ARRAY(p, len)
#define INV_LOG2B_ARRAY(p, len)
#define INV_LOG3B_ARRAY(p, len)
#define INV_LOG1_ENABLE                      (false)
#define INV_LOG2_ENABLE                      (false)
#define INV_LOG3_ENABLE                      (false)
#elif(1 == INV_DEBUG)
#define INV_LOG1A(func, obj, str)            INV_LOG_HEADER(func, obj, str)
#define INV_LOG2A(func, obj, str)
#define INV_LOG3A(func, obj, str)
#define INV_LOG1B(str)                       INV_LOG_STRING(str)
#define INV_LOG2B(str)
#define INV_LOG3B(str)
#define INV_LOG1B_ARRAY(p, len)              INV_LOG_ARRAY(p, len)
#define INV_LOG2B_ARRAY(p, len)
#define INV_LOG3B_ARRAY(p, len)
#define INV_LOG1_ENABLE                      (true)
#define INV_LOG2_ENABLE                      (false)
#define INV_LOG3_ENABLE                      (false)
#elif(2 == INV_DEBUG)
#define INV_LOG1A(func, obj, str)            INV_LOG_HEADER(func, obj, str)
#define INV_LOG2A(func, obj, str)            INV_LOG_HEADER(func, obj, str)
#define INV_LOG3A(func, obj, str)
#define INV_LOG1B(str)                       INV_LOG_STRING(str)
#define INV_LOG2B(str)                       INV_LOG_STRING(str)
#define INV_LOG3B(str)
#define INV_LOG1B_ARRAY(p, len)              INV_LOG_ARRAY(p, len)
#define INV_LOG2B_ARRAY(p, len)              INV_LOG_ARRAY(p, len)
#define INV_LOG3B_ARRAY(p, len)
#define INV_LOG1_ENABLE                      (true)
#define INV_LOG2_ENABLE                      (true)
#define INV_LOG3_ENABLE                      (false)
#else
#define INV_LOG1A(func, obj, str)            INV_LOG_HEADER(func, obj, str)
#define INV_LOG2A(func, obj, str)            INV_LOG_HEADER(func, obj, str)
#define INV_LOG3A(func, obj, str)            INV_LOG_HEADER(func, obj, str)
#define INV_LOG1B(str)                       INV_LOG_STRING(str)
#define INV_LOG2B(str)                       INV_LOG_STRING(str)
#define INV_LOG3B(str)                       INV_LOG_STRING(str)
#define INV_LOG1B_ARRAY(p, len)              INV_LOG_ARRAY(p, len)
#define INV_LOG2B_ARRAY(p, len)              INV_LOG_ARRAY(p, len)
#define INV_LOG3B_ARRAY(p, len)              INV_LOG_ARRAY(p, len)
#define INV_LOG1_ENABLE                      (true)
#define INV_LOG2_ENABLE                      (true)
#define INV_LOG3_ENABLE                      (true)
#endif
#endif
/***** public type definitions ***********************************************/

/***** public functions ******************************************************/

void inv_sys_log_time_stamp(const char *p_class_str, const char *p_func_str, void *p_obj);
void inv_sys_log_printf(const char *p_frm, ...);
void inv_printf(const char *p_frm, ...);
void inv_sys_dump_data_array(char *ptitle, const uint8_t *parray, int length);

#endif /* INV_SYS_LOG_API_H */

/***** end of file ***********************************************************/
