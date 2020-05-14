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
*inv_platform_wrapper.c
*
*Platform Wrapper
*
*****************************************************************************/
/***** #include statements ***************************************************/

#include <time.h>
#include <unistd.h>

//#include <windows.h>
#include "inv_adapter_api.h"
#include "inv478x_platform_api.h"

/***** public macro definitions **********************************************/

typedef void *inv_inst_t;
typedef void *HANDLE;


/***** public type definitions ***********************************************/

inv_inst_t adapt_inst_table[255] = { 0, };

/***** Adaptor link control **************************************************/


void inv_platform_adapter_select(uint8_t dev_id, inv_inst_t adapt_inst)
{
    adapt_inst_table[dev_id] = adapt_inst;
}

void inv_platform_adapter_deselect(uint8_t dev_id)
{
    adapt_inst_table[dev_id] = 0;
}


/***** platform wrapper functions ********************************************/

uint16_t inv_platform_block_write(uint8_t dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size)
{
    inv_inst_t adapt_inst = adapt_inst_table[dev_id];
    if (!adapt_inst_table[dev_id])
        return 1;
    if (inv_adapter_block_write(adapt_inst, addr, p_data, size))
        return 2;
    return 0;
}

uint16_t inv_platform_block_read(uint8_t dev_id, uint16_t addr, uint8_t *p_data, uint16_t size)
{
    inv_inst_t adapt_inst = adapt_inst_table[dev_id];

    if (!adapt_inst_table[dev_id])
        return 1;
    if (inv_adapter_block_read(adapt_inst, addr, p_data, size))
        return 2;
    return 0;
}

uint8_t inv_platform_device_reset_set(uint8_t dev_id, bool_t *b_on)
{
    inv_inst_t adapt_inst = adapt_inst_table[dev_id];

    if (!adapt_inst)
        return 1;

    inv_adapter_chip_reset_set(adapt_inst, *b_on);
    return 0;
}

void inv_platform_sleep_msec(uint32_t msec)
{
    usleep(1000*msec);
}

uint32_t inv_platform_time_msec_query(void)
{
    return clock();
}

void inv_platform_log_print(const char *p_str)
{
  //  printf(p_str);
    p_str = p_str;
}

void inv_platform_assert(const char *p_file_str, uint32_t line_no)
{
    char line_no_str[20];
    INV_SPRINTF (line_no_str, "%ld", line_no);
    printf("Assertion Failure in ");
    //printf((char *) p_file_str);
    printf(" at line no ");
    printf("line_no_str=%s\n",line_no_str);
    printf("\n");
    {
        uint8_t i = 1;
        while (i);
    }
}

#if (INV478X_USER_DEF__MULTI_THREAD)
inv_rval_t inv_platform_semaphore_create(const char *p_name, uint32_t max_count, uint32_t initial_value, HANDLE *p_sem_id)
{
   // *p_sem_id
		int p;
	/*p = CreateSemaphore(NULL,  // default security attributes
                                 initial_value, // initial count
                                 max_count, // maximum count
                                 NULL); // unnamed semaphore
   */
   p_sem_id = &p;                             
    p_name = p_name;

    return 0;
}

inv_rval_t inv_platform_semaphore_delete( uint32_t  sem_id)
{
	unsigned long long sem=(unsigned long long) sem_id;
	//CloseHandle( (HANDLE )sem );
    return 0;
}

inv_rval_t inv_platform_semaphore_give(uint32_t sem_id)
{
	unsigned long long sem=(unsigned long long) sem_id;
	//ReleaseSemaphore((HANDLE)sem, 1, NULL);
    return 0;
}

inv_rval_t inv_platform_semaphore_take(uint32_t sem_id, uint32_t time_msec)
{
	unsigned long long sem=(unsigned long long) sem_id;
   // WaitForSingleObject((HANDLE )sem, time_msec);
    return 0;
}
#endif /* INV478x_USER_DEF__MULTI_THREAD */
/***** end of file ***********************************************************/
