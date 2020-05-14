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
// inv_adapter.c
//

#include <stdio.h>
//#include <conio.h>
//#include <windows.h>
//#include <strsafe.h>
#include "inv_adapter_api.h"

/**************************** local define *******************************************/
#define DLL_HANDLE                HINSTANCE
#define API_NAME                "inv_adapter"
//#define API_NAME                "aardvark"

#define dlopen(name)            LoadLibraryA(name)
#define dlsym(handle, name)        GetProcAddress(handle, name)
#define DLL_NAME                API_NAME ".dll"


/**************************** local functions prototype *******************************************/
static char **(*dll_inv_adapter_list_query) (void) = 0;
static inv_inst_t(*dll_inv_adapter_create) (struct inv_adapter_cfg *, connect_cbf cbf, connect_cbf cbf_interrupt) = 0;
static inv_inst_t(*dll_inv_adapter_delete) (inv_inst_t) = 0;
static bool_t (*dll_inv_adapter_connect_query) (inv_inst_t) = 0;
static int(*dll_inv_adapter_chip_sel_set) (inv_inst_t, uint8_t) = 0;
static int(*dll_inv_adapter_block_read) (inv_inst_t, uint16_t, uint8_t *, uint16_t) = 0;
static int(*dll_inv_adapter_block_write) (inv_inst_t, uint16_t, const uint8_t *, uint16_t) = 0;
static inv_inst_t(*dll_inv_adapter_chip_reset_set) (inv_inst_t, bool_t) = 0;
static bool_t (*dll_inv_adapter_supspense) (char *, bool_t) = 0;


/**************************** local functions *******************************************/
static void *loadFunction(const char *name)
{
    static int handle = 0;
    void *function = 0;

    if (handle == 0) {
       // handle = dlopen(DLL_NAME);
        if (handle == 0) {
            return 0;
        }
    }
   // function = (void *) dlsym(handle, name);
    return function;
}

/**************************** public functions *******************************************/
char **inv_adapter_list_query(void)
{
    if (dll_inv_adapter_list_query == 0) {
        dll_inv_adapter_list_query = loadFunction("inv_adapter_list_query");
        if (!dll_inv_adapter_list_query) {
            return 0;
        }
    }
    return dll_inv_adapter_list_query();
}

inv_inst_t inv_adapter_create(struct inv_adapter_cfg *cfg, connect_cbf cbf, connect_cbf cbf_interrupt)
{

		
	printf("\n333inv_adapter_create 11\n");
	return 0;
   /* if (dll_inv_adapter_create == 0) {
        dll_inv_adapter_create = loadFunction("inv_adapter_create");
        if (!dll_inv_adapter_create) {
            return 0;
        }
    }*/
	printf("\ninv_adapter_create 22\n");
    return dll_inv_adapter_create(cfg, cbf, cbf_interrupt);
}

inv_inst_t inv_adapter_delete(inv_inst_t inst)
{
    if (dll_inv_adapter_delete == 0) {
        dll_inv_adapter_delete = loadFunction("inv_adapter_delete");
        if (!dll_inv_adapter_delete) {
            return 0;
        }
    }
    return dll_inv_adapter_delete(inst);
}

bool_t inv_adapter_connect_query(inv_inst_t inst)
{
    if (dll_inv_adapter_connect_query == 0) {
        dll_inv_adapter_connect_query = loadFunction("inv_adapter_connect_query");
        if (!dll_inv_adapter_connect_query) {
            return 0;
        }
    }
    return dll_inv_adapter_connect_query(inst);
}


int inv_adapter_chip_sel_set(inv_inst_t inst, uint8_t chip_sel)
{
    if (dll_inv_adapter_chip_sel_set == 0) {
        dll_inv_adapter_chip_sel_set = loadFunction("inv_adapter_chip_sel_set");
        if (!dll_inv_adapter_chip_sel_set) {
            return 0;
        }
    }
    return dll_inv_adapter_chip_sel_set(inst, chip_sel);
}

int inv_adapter_block_read(inv_inst_t inst, uint16_t offset, uint8_t *buffer, uint16_t count)
{
    if (dll_inv_adapter_block_read == 0) {
        dll_inv_adapter_block_read = loadFunction("inv_adapter_block_read");
        if (!dll_inv_adapter_block_read) {
            return 0;
        }
    }
    return dll_inv_adapter_block_read(inst, offset, buffer, count);
}

int inv_adapter_block_write(inv_inst_t inst, uint16_t offset, const uint8_t *buffer, uint16_t count)
{
    if (dll_inv_adapter_block_write == 0) {
        dll_inv_adapter_block_write = loadFunction("inv_adapter_block_write");
        if (!dll_inv_adapter_block_write) {
            return 0;
        }
    }
    return dll_inv_adapter_block_write(inst, offset, buffer, count);
}

inv_inst_t inv_adapter_chip_reset_set(inv_inst_t inst, bool_t b_on)
{
    if (dll_inv_adapter_chip_reset_set == 0) {
        dll_inv_adapter_chip_reset_set = loadFunction("dll_inv_adapter_chip_reset_set");
        if (!dll_inv_adapter_chip_reset_set) {
            return 0;
        }
    }
    return dll_inv_adapter_chip_reset_set(inst, b_on);
}

bool_t inv_adapter_supspense(char *p_adapter_id, bool_t b_on)
{
    if (dll_inv_adapter_supspense == 0) {
        dll_inv_adapter_supspense = loadFunction("inv_adapter_supspense");
        if (!dll_inv_adapter_supspense) {
            return 0;
        }
    }
    return dll_inv_adapter_supspense(p_adapter_id, b_on);
}
