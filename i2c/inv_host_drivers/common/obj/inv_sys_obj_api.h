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
* @file inv_sys_obj_api.h
*
* @brief object base class
*
*****************************************************************************/

#ifndef INV_SYS_OBJ_API_H
#define INV_SYS_OBJ_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

/***** public macro definitions **********************************************/

/*-------------------------------------------------------------------------------------------------
! @brief      Converts external instantiation reference to pointer of instantiation object as
!             defined locally in module. Creates an assertion if user would provide a reference
!             to a different class of instantiation.
!
! @param[in]  inst  External reference to instantiation.
!
! @return     Pointer to instantiation object.
-------------------------------------------------------------------------------------------------*/
#ifdef INV_ENV_BUILD_ASSERT
#if (INV_ENV_BUILD_ASSERT)
#ifndef STM32_WRAPPER
#define INV_SYS_OBJ_CHECK(list_inst, obj_inst)    ((inv_sys_obj_check(list_inst, INV_INST2OBJ(obj_inst))) \
        ? (INV_INST2OBJ(obj_inst)) : ((void xdata*)INV_ASSERT(0)))
#else
#define INV_SYS_OBJ_CHECK(list_inst, obj_inst)    ((inv_sys_obj_check(list_inst, INV_INST2OBJ(obj_inst))) \
        ? (INV_INST2OBJ(obj_inst)) : (void *)0) /*(INV_ASSERT(0))) */
#endif

#else
#define INV_SYS_OBJ_CHECK(list_inst, obj_inst)    (INV_INST2OBJ(obj_inst))
#endif
#else
#define INV_SYS_OBJ_CHECK(list_inst, obj_inst)    (INV_INST2OBJ(obj_inst))
#endif

#define INV_SYS_OBJ_PARENT_INST_GET(child_inst)    (inv_sys_obj_parent_inst_get(INV_INST2OBJ(child_inst)))

/***** public type definitions ***********************************************/

#define inv_sys_obj_size        uint16_t
#define inv_inst_t    inv_inst_t

/***** public functions ******************************************************/

void *inv_sys_obj_singleton_create(const char *p_class_str, inv_inst_t parent_inst, inv_sys_obj_size size);
void inv_sys_obj_singleton_delete(void *p_obj);

inv_inst_t inv_sys_obj_list_create(const char *p_class_str, inv_sys_obj_size size);
void inv_sys_obj_list_delete(inv_inst_t list_inst);

void *inv_sys_obj_instance_create(inv_inst_t list_inst, inv_inst_t parent_inst, const char *p_inst_str);
void inv_sys_obj_instance_delete(void *p_obj);

void *inv_sys_obj_first_get(inv_inst_t list_inst);
bool_t inv_sys_obj_check(inv_inst_t list_inst, void *p_obj);
inv_inst_t inv_sys_obj_parent_inst_get(void *p_obj);

const char *inv_sys_obj_list_name_get(void *p_obj);
const char *inv_sys_obj_name_get(void *p_obj);
void *inv_sys_obj_next_get(void *p_obj);
void inv_sys_obj_move(void *p_obj_des, void *p_obj_src);

#endif /* INV_SYS_OBJ_API_H */

/***** end of file ***********************************************************/
