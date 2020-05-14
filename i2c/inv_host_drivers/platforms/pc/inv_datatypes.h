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
 * @file inv_datatypes.h
 *
 * @brief Common data types
 *
 *****************************************************************************/

#ifndef INV_DATATYPES_H
#define INV_DATATYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/***** #include statements ***************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/*#include <stdint.h>*/

#ifndef NULL
#define NULL                                 ((void *)0)
#endif                          /* NULL */

#ifndef INST_NULL
#define INST_NULL                            ((inv_inst_t)0)
#endif                          /* INST_NULL */

#define INV_INST2OBJ(inst)                   ((void *)inst)
#define INV_OBJ2INST(pObj)                   ((inv_inst_t)pObj)

/***** public type definitions ***********************************************/

#define     int_t     signed int
#define  struct_mes_t struct mes *
#define inv_platform_dev_id    uint8_t

#if (__STDC_VERSION__ >= 199901L)
#include <stdint.h>
#else
#define   uint8_t    unsigned char
#define   uint16_t   unsigned short
#define   uint32_t   unsigned long int
#define   int8_t     signed char
#define   int16_t    signed short
#define   int32_t    signed long int
#endif


/**
 * @brief Instance type
 */
    typedef void *inv_inst_t;
    typedef uint32_t inv_rval_t;

/**
 * @brief C++ -like Boolean type
 */
#ifdef __cplusplus
#define bool_t  char
#else
    typedef enum {
        false = 0,
        true = !(false)
    } bool_t;
#endif                          /* __cplusplus */

/**
 * @brief Type to use with bitfields
 */
#define   bit_fld     int        /* bit field type used in structures */
#define bool_t  char
/***** standard functions ****************************************************/
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define SET_BITS                         (0xFF)
#define CLEAR_BITS                       (0x00)
/*typedef uint16_t                       inv_inst_t;*/
    extern void inv_platform_assert(const char *pFileStr, uint32_t lineNo);

#define INV_MEMCPY(pdes, psrc, size)    memcpy(pdes, psrc, size)
#define INV_MEMSET(pdes, value, size)   memset(pdes, value, size)
#define INV_STRLEN(str)                 strlen(str)
#define INV_ASSERT(expr)                INV_PLATFORM_DEBUG_ASSERT(expr)
#define INV_PLATFORM_DEBUG_ASSERT(expr) ((void)0)
/*#define INV_ASSERT(expr) ((expr)?((void)0):(inv_platform_assert(__FILE__, __LINE__)))*/
#if defined(_WIN16) || (_WIN32)
#define INV_VSPRINTF(dstr, fstr, arg) vsprintf_s(dstr, sizeof(dstr), fstr, arg)
#define INV_SPRINTF(str, frm, arg)   sprintf_s(str, sizeof(str), frm, arg)
#else
#define INV_VSPRINTF(dstr, fstr, arg) vsnprintf(dstr, sizeof(dstr), fstr, arg)
#define INV_SPRINTF(str, frm, arg)    snprintf(str, sizeof(str), frm, arg)
#endif
#ifdef __cplusplus
}
#endif
#endif                          /* INV_DATATYPES_H */
