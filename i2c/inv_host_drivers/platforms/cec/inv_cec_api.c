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
// inv_cec_api.c
//

#include <stdio.h>
//#include <conio.h>
//#include <windows.h>
//#include <strsafe.h>
#include "inv_cec_api.h"
#include "inv478x_platform_api.h"

/**************************** local define *******************************************/
#define DLL_HANDLE                HINSTANCE
#define API_NAME                "inv478x_cec_dll"
#define dlopen(name)            LoadLibraryA(name)
#define dlsym(handle, name)        GetProcAddress(handle, name)
#define DLL_NAME                API_NAME ".dll"


/**************************** local functions prototype *******************************************/
static inv_inst_t(*dll_inv_cec_tx_create) (uint8_t devid, inv_cec_call_back p_call_back, inv_cec_adapter_call_back p_adapter_call_back) = 0;
static void(*dll_inv_cec_tx_delete) (void) = 0;
static void(*dll_inv_cec_tx_handle) (void) = 0;
static uint32_t(*dll_inv_tx_cec_arc_rx_start) (void) = 0;
static uint32_t(*dll_inv_tx_cec_arc_rx_stop) (void) = 0;

static inv_inst_t(*dll_inv_cec_rx_create) (uint8_t devid, inv_cec_call_back p_call_back, inv_cec_adapter_call_back p_adapter_call_back) = 0;
static void(*dll_inv_cec_rx_delete) (void) = 0;
static void(*dll_inv_cec_rx_handle) (void) = 0;
static uint32_t(*dll_inv_rx_cec_arc_tx_start) (void) = 0;
static uint32_t(*dll_inv_rx_cec_arc_tx_stop) (void) = 0;

static void(*dll_inv_platform_adapter_select) (uint8_t dev_id, inv_inst_t adapt_inst) = 0;
/**************************** local functions *******************************************/
static void *loadDllFunction(const char *name)
{
    static int handle = 0;
    void *function = 0;

    if (handle == 0) {
       // handle = dlopen(DLL_NAME);
        if (handle == 0) {
            return 0;
        }
    }
    function = (void *) dlsym(handle, name);
    return function;
}

/**************************** public functions *******************************************/
bool_t inv_cec_adapter_callback(uint8_t dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size, uint8_t read_enable)
{
    if(read_enable)
    {
        inv_platform_block_read(dev_id, addr, (uint8_t *)p_data, size);
        return false;
    }
    else
    {
        inv_platform_block_write(dev_id, addr, p_data, size);
        return false;
    }
}

void inv_cec_tx_delete(void)
{
    if (dll_inv_cec_tx_delete == 0) {
        dll_inv_cec_tx_delete = loadDllFunction("inv9612_tx_delete");
        if (!dll_inv_cec_tx_delete) {
            return;
        }
    }
    dll_inv_cec_tx_delete();
}

void inv_cec_rx_delete(void)
{
    if (dll_inv_cec_rx_delete == 0) {
        dll_inv_cec_rx_delete = loadDllFunction("inv9612_rx_delete");
        if (!dll_inv_cec_rx_delete) {
            return;
        }
    }
    dll_inv_cec_rx_delete();
}

inv_inst_t inv_cec_tx_create(uint8_t devid, inv_cec_call_back p_call_back, inv_inst_t adapt_inst)
{
    if (dll_inv_platform_adapter_select == 0) {
        dll_inv_platform_adapter_select = loadDllFunction("inv_platform_adapter_select");
        if (!dll_inv_platform_adapter_select) {
            return 0;
        }
		dll_inv_platform_adapter_select(devid, adapt_inst);

		if (dll_inv_cec_tx_create == 0) {
				dll_inv_cec_tx_create = loadDllFunction("inv9612_tx_create");
				if (!dll_inv_cec_tx_create) {
					return 0;
				}
			}
		return dll_inv_cec_tx_create(devid, p_call_back, &inv_cec_adapter_callback);
    }
	return 0;
}

inv_inst_t inv_cec_rx_create(uint8_t devid, inv_cec_call_back p_call_back, inv_inst_t adapt_inst)
{
    if (dll_inv_platform_adapter_select == 0) {
        dll_inv_platform_adapter_select = loadDllFunction("inv_platform_adapter_select");
        if (!dll_inv_platform_adapter_select) {
            return 0;
        }

		dll_inv_platform_adapter_select(devid, adapt_inst);
		if (dll_inv_cec_rx_create == 0) {
				dll_inv_cec_rx_create = loadDllFunction("inv9612_rx_create");
				if (!dll_inv_cec_rx_create) {
					return 0;
				}
			}
		return dll_inv_cec_rx_create(devid, p_call_back, &inv_cec_adapter_callback);
    }
	return 0;
}

void inv_cec_tx_handle(void)
{
    if (dll_inv_cec_tx_handle == 0) {
        dll_inv_cec_tx_handle = loadDllFunction("inv9612_tx_cec_handle");
        if (!dll_inv_cec_tx_handle) {
            return;
        }
    }
    dll_inv_cec_tx_handle();
}

void inv_cec_rx_handle(void)
{
    if (dll_inv_cec_rx_handle == 0) {
        dll_inv_cec_rx_handle = loadDllFunction("inv9612_rx_cec_handle");
        if (!dll_inv_cec_rx_handle) {
            return;
        }
    }
    dll_inv_cec_rx_handle();
}

uint32_t inv_tx_cec_arc_rx_start(void)
{
    if (dll_inv_tx_cec_arc_rx_start == 0) {
        dll_inv_tx_cec_arc_rx_start = loadDllFunction("inv9612_tx_cec_arc_rx_start");
        if (!dll_inv_tx_cec_arc_rx_start) {
            return -1;
        }
    }
    return dll_inv_tx_cec_arc_rx_start();
}

uint32_t inv_tx_cec_arc_rx_stop(void)
{
    if (dll_inv_tx_cec_arc_rx_stop == 0) {
        dll_inv_tx_cec_arc_rx_stop = loadDllFunction("inv9612_tx_cec_arc_rx_stop");
        if (!dll_inv_tx_cec_arc_rx_stop) {
            return -1;
        }
    }
    return dll_inv_tx_cec_arc_rx_stop();
}

uint32_t inv_rx_cec_arc_tx_start(void)
{
    if (dll_inv_rx_cec_arc_tx_start == 0) {
        dll_inv_rx_cec_arc_tx_start = loadDllFunction("inv9612_rx_cec_arc_tx_start");
        if (!dll_inv_rx_cec_arc_tx_start) {
            return -1;
        }
    }
    return dll_inv_rx_cec_arc_tx_start();
}

uint32_t inv_rx_cec_arc_tx_stop(void)
{
    if (dll_inv_rx_cec_arc_tx_stop == 0) {
        dll_inv_rx_cec_arc_tx_stop = loadDllFunction("inv9612_rx_cec_arc_tx_stop");
        if (!dll_inv_rx_cec_arc_tx_stop) {
            return -1;
        }
    }
    return dll_inv_rx_cec_arc_tx_stop();
}

