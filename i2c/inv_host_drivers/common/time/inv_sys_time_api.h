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
* @file inv_sys_time_api.h
*
* @brief Time Library
*
*****************************************************************************/
#ifndef INV_SYS_TIME_API_H
#define INV_SYS_TIME_API_H
/***** #include statements ***************************************************/
#include "inv_datatypes.h"
/***** public macro definitions **********************************************/
/***** public type definitions ***********************************************/
#define  inv_sys_time_micro           uint32_t
#define  inv_sys_time_milli         uint32_t
#define  inv_sys_seq_time_ms         uint16_t
/***** public functions ******************************************************/
/*-------------------------------------------------------------------------------------------------
! @brief      Calculates time difference between t1 and t2 and prevent result from roll-over
!             corruption(t1<t2).
!
! @param[in]  t1 - Number of milli seconds.
! @param[in]  t2 - Number of milli seconds.
!
! @return     Number of milli seconds.
-------------------------------------------------------------------------------------------------*/
inv_sys_time_milli inv_sys_time_milli_diff_get(inv_sys_time_milli t1, inv_sys_time_milli t2);
/*-------------------------------------------------------------------------------------------------
 ! @brief      Configures a milli time-out object. This object can be used with 'inv_sys_time_out_milli_is'
 !             to find out if 'milli_t_o' object has been expired.
 !             example:
 !             {
 !                 inv_sys_time_milli_t milli_t_o;
 !
 !                 inv_sys_time_out_milli_set(&milli_t_o, 100);
 !                 while(!inv_sys_time_out_milli_is(milli_t_o))
 !                 { .... }
 !             }
 !
 ! @param[in]  p_milli_t_o - pointer to 'milli_t_o' object.
 ! @param[in]  time_out  - Number of milli seconds.
 -------------------------------------------------------------------------------------------------*/
void inv_sys_time_out_milli_set(inv_sys_time_milli * p_milli_to, inv_sys_time_milli time_out);
/*-------------------------------------------------------------------------------------------------
 ! @brief      Finds out if 'mill_t_o' object has been expired.
 !
 ! @param[in]  p_milli_t_o - pointer to 'milli_t_o' object.
 !
 ! @return     true if 'milli_t_o' object has been expired.
 -------------------------------------------------------------------------------------------------*/
bool_t inv_sys_time_out_milli_is(const inv_sys_time_milli * p_milli_to);
/*-------------------------------------------------------------------------------------------------
 ! @brief      Blocks execution for x number of milli seconds.
 !
 ! @param[in]  milli_delay - Number of milli seconds.
 -------------------------------------------------------------------------------------------------*/
void inv_sys_time_milli_delay(inv_sys_time_milli milli_delay);
/*-------------------------------------------------------------------------------------------------
 ! @brief      Returns number of passed milli seconds since time_init().
!
! @return     Number of milli seconds.
-------------------------------------------------------------------------------------------------*/
inv_sys_seq_time_ms inv_time_milli_get(void);
/*-------------------------------------------------------------------------------------------------
! @brief
!
! @return
-------------------------------------------------------------------------------------------------*/
void inv_init_process_time(void);
#endif /* INV_SYS_TIME_API_H */
/***** end of file ***********************************************************/
