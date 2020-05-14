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
* file inv_sys_malloc.c
*
* brief :- Dynamic memory allocation from static memory pool
*
*****************************************************************************/
/*#define INV_DEBUG 3*/

/***** #include statements ***************************************************/

#include "inv_common.h"
#include "inv_sys_malloc_api.h"

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (lib_malloc, 9);

/***** local macro definitions ***********************************************/
#define MEMPOOL_SIZE_IN_BYTES   (0x10000)
#define MEMPOOL_SIZE_IN_WORDS   ((MEMPOOL_SIZE_IN_BYTES>>2)+2)

/***** local prototypes ******************************************************/

static void s_memory_clear(uint16_t size, uint16_t ptr);

/***** local data objects ****************************************************/

static uint32_t s_mem_pool[MEMPOOL_SIZE_IN_WORDS];  /* Ensure memory pool location at 4-byte boundary */
static uint16_t s_ptr;          /* Pointer to next avaialble memory location      */
static bool_t sb_lock;

/***** public functions ******************************************************/

void *inv_sys_malloc_create(uint16_t size)
{
    uint16_t words = ((size >> 2) + 1); /* Round up to nearest number of words(1 word = 4 bytes) */

    /*
     * Check if memory pool is locked
     */
    INV_ASSERT (!sb_lock);
    if (MEMPOOL_SIZE_IN_WORDS > (s_ptr + words)) {
        uint16_t ptr = s_ptr;

        /*
         * Clear memory
         */
        s_memory_clear(words, s_ptr);

        /*
         * Increase pointer to next available memory location
         */
        s_ptr += words;

        return (void *) (&s_mem_pool[ptr]);
    } else {
        INV_ASSERT (0);
        return NULL;
    }
}

void inv_sys_malloc_delete(void *p)
{
    /*
     * Make sure that delete pointer is in allocated memory space
     */
    INV_ASSERT ((uint32_t *) p >= &s_mem_pool[0]);
    INV_ASSERT ((uint32_t *) p < &s_mem_pool[s_ptr]);

    /*
     * Make sure that delete pointer is on a even 4 byte boundary
     */
    INV_ASSERT (!((((uint8_t *) p) - ((uint8_t *) & s_mem_pool[0])) % 4));

    s_ptr = (uint16_t) ((((uint8_t *) p) - ((uint8_t *) & s_mem_pool[0])) >> 2);
}

uint16_t inv_sys_malloc_bytes_allocated_get(void)
{
    /*
     * Return total amound of bytes allocated
     */
    return (uint16_t) (s_ptr << 2);
}

void inv_sys_malloc_lock(void)
{
    sb_lock = true;
}

void inv_sys_malloc_delete_all(void)
{
    sb_lock = false;
    s_ptr = 0;
}

/***** local functions *******************************************************/

static void s_memory_clear(uint16_t size, uint16_t ptr)
{
    uint32_t *p_data = &s_mem_pool[ptr];

    while (size--) {
        *p_data = 0;
        p_data++;
    }
}

/***** end of file ***********************************************************/
