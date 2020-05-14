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
* @file inv_sys.h
*
* @brief
*
*****************************************************************************/

#ifndef INV_SYS_H
#define INV_SYS_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

#define INV_INST_NULL                    ((inv_inst_t)0)

/* Construct to stringify defines */
#define INV_STRINGIFY(x)                 #x
#define INV_DEF2STR(x)                   INV_STRINGIFY(x)

#ifdef INV_DEBUG
#if (0 < INV_DEBUG)
#define INV_MODULE_NAME_SET(name, id)    static const char *s_inv_module_name_str = INV_DEF2STR(name); static const uint8_t s_inv_module_id = id
#define INV_MODULE_NAME_CHECK()      (s_inv_module_name_str = s_inv_module_name_str)
#define INV_MODULE_NAME_GET()        s_inv_module_name_str
#else
#define INV_MODULE_NAME_SET(name, id)    static const char *s_inv_module_name_str = INV_DEF2STR(name); static const uint8_t s_inv_module_id = id
#define INV_MODULE_NAME_CHECK()      (s_inv_module_name_str = s_inv_module_name_str)
#define INV_MODULE_NAME_GET()        (NULL)
#endif /* (0 < INV_DEBUG) */
#else
#define INV_MODULE_NAME_SET(name, id)      static const char *s_inv_module_name_str = INV_DEF2STR(name); static const uint8_t s_inv_module_id = id
#define INV_MODULE_NAME_CHECK()        (s_inv_module_name_str = s_inv_module_name_str)
#define INV_MODULE_NAME_GET()          (NULL)
#endif /* INV_DEBUG */

#endif /* INV_SYS_H */
