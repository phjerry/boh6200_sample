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
* file inv_sys_time.c
*
* brief :- Time library
*
*****************************************************************************/
/*#define INV_DEBUG 3*/

/***** #include statements ***************************************************/

#include "inv_common.h"
#include "inv_sys_time_api.h"
#include "inv478x_platform_api.h"
/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (lib_time, 11);

/***** local macro definitions ***********************************************/

#define MILLI_TO_MAX          (((inv_sys_time_milli)~0)>>1) /* Maximum milli out must be set to less than half the range of inv_sys_time_milli */

/***** local type definitions ************************************************/
static unsigned long process_init_time;
/***** local prototypes ******************************************************/

static void s_time_out_milli_set(inv_sys_time_milli * p_milli_to, inv_sys_time_milli time_out);
static bool_t s_time_out_milli_is(const inv_sys_time_milli * p_milli_to);

/***** local data objects ****************************************************/

/***** public functions ******************************************************/

inv_sys_time_milli inv_sys_time_milli_diff_get(inv_sys_time_milli t1, inv_sys_time_milli t2)
{
    if (t2 < t1)
        return ((inv_sys_time_milli) ~ 0) - t1 + t2 + 1;
    else
        return t2 - t1;
}

void inv_sys_time_out_milli_set(inv_sys_time_milli * p_milli_to, inv_sys_time_milli time_out)
{
    s_time_out_milli_set(p_milli_to, time_out);
}

bool_t inv_sys_time_out_milli_is(const inv_sys_time_milli * p_milli_to)
{
    return s_time_out_milli_is(p_milli_to);
}

void inv_sys_time_milli_delay(inv_sys_time_milli mill_delay)
{
    inv_sys_time_milli milli_t_o;

    s_time_out_milli_set(&milli_t_o, mill_delay);

    while (!s_time_out_milli_is(&milli_t_o));
}

/***** local functions *******************************************************/

static void s_time_out_milli_set(inv_sys_time_milli * p_milli_to, inv_sys_time_milli time_out)
{
    INV_ASSERT (MILLI_TO_MAX > time_out);
    *p_milli_to = (inv_sys_time_milli) inv_platform_time_msec_query() + time_out;
}

static bool_t s_time_out_milli_is(const inv_sys_time_milli * p_milli_to)
{
    inv_sys_time_milli milli_new = (inv_sys_time_milli) inv_platform_time_msec_query();
    inv_sys_time_milli milli_dif = (*p_milli_to > milli_new) ? (*p_milli_to - milli_new) : (milli_new - *p_milli_to);

    if (MILLI_TO_MAX < milli_dif)
        return (*p_milli_to > milli_new) ? (true) : (false);
    else
        return (*p_milli_to <= milli_new) ? (true) : (false);
}


/***** end of file ***********************************************************/
