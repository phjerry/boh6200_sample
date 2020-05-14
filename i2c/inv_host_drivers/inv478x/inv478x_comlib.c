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
* @file inv478x_comlib.c
*
* @brief Defines the exported functions for the DLL application.
*
*****************************************************************************/

/***** #include statements ***************************************************/
#include "inv_common.h"
#include "inv478x_api.h"
#include "inv_sys_time_api.h"
#include "inv_sys_log_api.h"
#include "inv478x_platform_api.h"

/***** Register Module name **************************************************/

/***** local macro definitions ***********************************************/

/***** local type definitions ************************************************/

/***** local prototypes ******************************************************/

static void s_resources_delete(struct obj *p_obj);
static inv_rval_t s_handle(struct obj *p_obj);
static void s_handle_flash_prog(struct obj *p_obj);
static void s_handle_ipc_interrupts(struct obj *p_obj);
static void s_handle_isp_interrupts(struct obj *p_obj);
static void s_request_message_init(struct obj *p_obj, inv_drv_ipc_op_code op_code);
static void s_request_message_send(struct obj *p_obj);
static inv_rval_t s_wait_for_valid_ipc_response_message(struct obj *p_obj, inv_drv_ipc_op_code op_code);
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
static inv_rval_t s_semaphore_ipc_take(struct obj *p_obj);
static void s_semaphore_ipc_give(struct obj *p_obj);
static inv_rval_t s_mutex_handle_take(struct obj *p_obj);
static void s_mutex_handle_give(struct obj *p_obj);
#endif
#if (INV478X_USER_DEF__MULTI_THREAD)
static inv_rval_t s_mutex_api_take(struct obj *p_obj);
static void s_mutex_api_give(struct obj *p_obj);
#endif
static void s_device_restart(struct obj *p_obj);
static bool_t s_device_ping(struct obj *p_obj);
static void s_interrupts_enable_set(struct obj *p_obj, bool_t b_enable);

/* These APIs are for internal use for API GUI tool and host app pc */

/***** local data objects ****************************************************/


/***** call-back functions ***************************************************/
void inv_drv_ipc_call_back(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_event ipc_event)
{
    inv_inst_t inst = INV_SYS_OBJ_PARENT_INST_GET (ipc_inst);
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);

    if (ipc_event & INV_DRV_IPC_EVENT__MESSAGE_RECEIVED) {
        inv_drv_ipc_mes_inst mes_inst = inv_drv_ipc_message_create();

        inv_drv_ipc_message_receive(p_obj->ipc_inst, mes_inst);
        if (INV_DRV_IPC_TYPE__NOTIFY == inv_drv_ipc_type_get(mes_inst)) {
            inv478x_event_flags_t event_flags = (inv478x_event_flags_t) inv_drv_ipc_long_pop(mes_inst);
            p_obj->event_flags_user |= event_flags;
            INV_LOG1A ("INV_DRV_IPC_TYPE__NOTIFY", p_obj, ("\n"));
        } else if (INV_DRV_IPC_TYPE__RESPONSE == inv_drv_ipc_type_get(mes_inst)) {
            INV_LOG1A ("INV_DRV_IPC_TYPE__RESPONSE", p_obj, ("\n"));
            inv_drv_ipc_message_copy(p_obj->mes_ack_inst, mes_inst);

#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
            /*
             * Don't give IPC semaphore if IPC request was initiated through Inv478xHandle
             */
            if (!p_obj->b_from_user_call_back)
                s_semaphore_ipc_give(p_obj);
#endif
        }
        inv_drv_ipc_message_delete(mes_inst);
    }
}

/*************************************************************************************************/
#if (INV_DEBUG > 0)
static char *p_info_frame_names[] = {
    "AVI",
    "AUDIO",
    "VENDOR SPECIFIC",
    "SPD",
    "GBD",
    "MPEG",
    "ISRC1",
    "ISRC2",
    "ACP",
    "GCP",
    "HDR"
};
#endif

/***** local functions *******************************************************/

enum inv_platform_status inv_platform_ipc_semaphore_delete(inv_platform_semaphore sem_id)
{
    sem_id = sem_id;
    return INV478X_PLATFORM_STATUS__SUCCESS;
}

static void s_resources_delete(struct obj *p_obj)
{
    if (p_obj->mes_ack_inst)
        inv_drv_ipc_message_delete(p_obj->mes_ack_inst);
    if (p_obj->mes_req_inst)
        inv_drv_ipc_message_delete(p_obj->mes_req_inst);

    if (p_obj->ipc_inst)
        inv_drv_ipc_delete(p_obj->ipc_inst);

#if (INV478X_USER_DEF__MULTI_THREAD)
#if (INV478X_USER_DEF__IPC_SEMAPHORE)
    if (p_obj->semaphore_handle)
        inv_platform_semaphore_delete(p_obj->semaphore_handle);

    if (p_obj->semaphore_ipc)
        inv_platform_semaphore_delete(p_obj->semaphore_ipc);
#endif
    if (p_obj->semaphore_api)
        inv_platform_semaphore_delete(p_obj->semaphore_api);
#endif

    inv_sys_obj_instance_delete(p_obj);
    if (!inv_sys_obj_first_get(s_obj_list_inst)) {
        /*
         * Delete list of objects
         */
        inv_sys_obj_list_delete(s_obj_list_inst);
        s_obj_list_inst = INV_INST_NULL;
    }
}

static inv_rval_t s_handle(struct obj *p_obj)
{
    inv_rval_t ret_val = 0;
    inv478x_event_flags_t flags = 0;

    ret_val = MUTEX_HANDLE_TAKE (p_obj);
    if (!ret_val) {

        /*
         * Temperarily disable all interrupts to force INT signal to go low
         */
        s_interrupts_enable_set(p_obj, false);

        /*
         * Handle IPC interrupts
         */
        s_handle_ipc_interrupts(p_obj);

        /*
         * Handle ISP interrupts
         */
        s_handle_isp_interrupts(p_obj);

        /*
         * Release INT force low condition to allow level triggered ISR to be invoked in case some interrupt bits still were set
         */
        s_interrupts_enable_set(p_obj, true);

        /*
         * Update Event status
         */
        p_obj->event_flags_stat |= p_obj->event_flags_user;
        p_obj->event_flags_user = 0x00000000;
        flags = p_obj->event_flags_stat & p_obj->event_flags_mask;
        p_obj->event_flags_stat &= (~p_obj->event_flags_mask);
    }

    /*
     * Release Mutex before calling user call-back handler so that user application can call ap_is from call-back handler
     */
    MUTEX_HANDLE_GIVE (p_obj);

    if (!ret_val) {
        if (flags) {
            if (p_obj->inv478x_lib_evt_cb_func) {
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
                /*
                 * Set flag to allow API calls indentify if they were called from user call-back
                 */
                p_obj->b_from_user_call_back = true;
#endif
                /*
                 * Call notification handler
                 */
                p_obj->inv478x_lib_evt_cb_func(INV_OBJ2INST (p_obj), flags);
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
                p_obj->b_from_user_call_back = false;
#endif
            }
        }
    }
    return ret_val;
}

static void s_handle_flash_prog(struct obj *p_obj)
{
    /*
     * Make sure previous flashing operation is done
     */
    INV_ASSERT (inv_drv_isp_operation_done(p_obj->dev_id));

    if (p_obj->p_flash_packet) {
        uint16_t page_size = 256;
#if (INV478X_FLASH_BUFFER_SIZE)
        /*
         * Detemine maximum page size based on absolute flashing address
         */
        /*
         * Maximum page size is 256 bytes if LSByte of absolute flashing address is zero
         */
        page_size = 256 - (p_obj->file_pntr % 256);
        /*
         * Check if packet size is smaller than maximum page size
         */
        if (p_obj->packet_size < page_size)
            page_size = p_obj->packet_size;
#endif
        INV_LOG2A ("s_handle_flash_prog", p_obj, ("Offset=0x%lX, length=0x%x\n", p_obj->file_pntr, page_size));
#if (INV478X_FLASH_BUFFER_SIZE)
        /*
         * Program page of bytes
         */
        inv_drv_isp_burst_write(p_obj->file_pntr, p_obj->p_packet_pntr, page_size);

        p_obj->p_packet_pntr += page_size;
        p_obj->packet_size -= page_size;

        /*
         * Check if entire packet has been programmed
         */
        if (0 == p_obj->packet_size) {
            /*
             * Delete flash packet
             */
            inv_sys_malloc_delete(p_obj->p_flash_packet);
            p_obj->p_flash_packet = NULL;
        }
#else
        /*
         * Program page of bytes
         */
        inv_drv_isp_burst_write(p_obj->dev_id, p_obj->file_pntr, p_obj->p_flash_packet, page_size);

        /*
         * Clear packet pointer to indicate packet is programmed
         */
        p_obj->p_flash_packet = NULL;
#endif
        p_obj->file_pntr += page_size;
    }
}

static void s_handle_ipc_interrupts(struct obj *p_obj)
{
    uint8_t ipc_flags = 0;

    /*
     * Read IPC interrupt status flags
     */
    ipc_flags = inv_drv_cra_read_8(p_obj->dev_id, REG08__MCU_REG__MCU__EXT_INT_STATUS);

    if (ipc_flags) {
        /*
         * Clear IPC interrupt status flags
         */
        inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__EXT_INT_STATUS, ipc_flags);

        if (ipc_flags & 0x7)
            /*
             * Handle IPC interrupts
             */
            inv_drv_ipc_handler(p_obj->ipc_inst, (inv_drv_ipc_token_t) ipc_flags);
    }

    INV_LOG2A ("s_handle_ipc_interrupts", p_obj, ("%02X\n", (uint16_t) ipc_flags));
}

static void s_handle_isp_interrupts(struct obj *p_obj)
{
    uint8_t isp_flags = 0;

    /*
     * Read ISP interrupt status flags
     */
    isp_flags = inv_drv_cra_read_8(p_obj->dev_id, 0x00f3);
    if (isp_flags) {
        /*
         * Clear ISP interrupt status flag
         */
        inv_drv_cra_write_8(p_obj->dev_id, 0x00f3, isp_flags);

        /*
         * Handle ISP interrupts
         */
        s_handle_flash_prog(p_obj);    /* Flash program handler */
    }

    if (isp_flags != 0)
        INV_LOG2A ("s_handle_isp_interrupts", p_obj, ("%02X\n", (uint16_t) isp_flags));
}

static void s_request_message_init(struct obj *p_obj, inv_drv_ipc_op_code op_code)
{
	printf("s_request_message_init\n");
    inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__RX_FIFO_CONTROL, BIT__MCU_REG__MCU__RX_FIFO_CONTROL__RESET);
    inv_drv_ipc_message_clear(p_obj->mes_req_inst);
    inv_drv_ipc_type_set(p_obj->mes_req_inst, INV_DRV_IPC_TYPE__REQUEST);
    inv_drv_ipc_op_code_set(p_obj->mes_req_inst, op_code);
    p_obj->tag_code++;
    inv_drv_ipc_tag_code_set(p_obj->mes_req_inst, p_obj->tag_code);

    /*
     * Clear previous acknowledge message before new request message is initiated
     */
    inv_drv_ipc_message_clear(p_obj->mes_ack_inst);
}

static void s_request_message_send(struct obj *p_obj)
{
    inv_drv_ipc_message_send(p_obj->ipc_inst, p_obj->mes_req_inst);
}

static inv_rval_t s_wait_for_valid_ipc_response_message(struct obj *p_obj, inv_drv_ipc_op_code op_code)
{
    inv_rval_t ret_val = 0;
    inv_sys_time_milli time_out;
    inv_sys_time_milli ipc_time_out = IPC_COMPLETION_TIMEOUT;
    if (op_code == INV478X_LICENSE_CERT_SET)
        ipc_time_out = IPC_COMPLETION_TIMEOUT_LICENSE_CERT_SET;
    INV_LOG2A ("s_wait_for_valid_ipc_response_message", p_obj, (""));
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
    /*
     * First check if API call was initiated through user call back.
     */
    /*
     * Response message polling is required when called through user call-back, since Inv478xHandle is still blocked
     */
    if (p_obj->b_from_user_call_back)
#endif
    {

        /*
         * Poll and wait until response message is received
         */
        /*
         * Wait for respondse message from slave
         */
        inv_sys_time_out_milli_set(&time_out, ipc_time_out);
        while (op_code != inv_drv_ipc_op_code_get(p_obj->mes_ack_inst)) {  /* Check if responding message has been received. */
            inv_platform_sleep_msec(10);
            s_handle_ipc_interrupts(p_obj);
            if (inv_sys_time_out_milli_is(&time_out)) {    /* Check for time-out */
                ret_val = INV_RETVAL__IPC_NO_RESPONSE;
                printf("\n Time Out !!!\n");
                break;
            }
        }
    }
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
    else {
        /*
         * Wait for Inv478xHandle() to receive response message
         */
        /*
         * s_semaphore_ipc_take returns false if released through timeout
         */
        ret_val = s_semaphore_ipc_take(p_obj);
        if (!ret_val) {
            if (op_code == INV478X_LICENSE_CERT_SET) {
                inv_sys_time_milli_delay(7000);
            }
            if (op_code != inv_drv_ipc_op_code_get(p_obj->mes_ack_inst)) { /* Check if responding message has been received. */
                ret_val = INV_RETVAL__IPC_NO_RESPONSE;
                printf("\n Time Out !!!\n");
            }
        }
	}
#endif
        if (p_obj->event_flags_user) {
            /*
             * bit 3 is used to invoke user application to call Inv478xHandle() to ensure handling of remaining event flags
             */
            inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__EXT_INT_TRIGGER, 0x08);
        }
        /*
         * Clear IPC Tx-queue in case it wasn't cleared due to time out or latency of inv_drv_ipc_handler() call.
         */
        inv_drv_ipc_tx_reset(p_obj->ipc_inst);

        if (!ret_val) {
            /*
             * Check Tag Code
             */
            if (p_obj->tag_code != inv_drv_ipc_tag_code_get(p_obj->mes_ack_inst))
                ret_val = INV_RETVAL__IPC_WRONG_TAG_CODE;
        }

        if (!ret_val)
            /*
             * Read result code
             */
            ret_val = (inv_rval_t) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

        return ret_val;
    }

/*static void s_device_restart(struct obj *p_obj)
{
    bool_t reset;
    /* Re-fetch firmware from flash and restart firmware
    p_obj->boot_stat = INV_SYS_BOOT_STAT__IN_PROGRESS;

    /* SW reset Inv478x device
    reset = true;
    inv_platform_device_reset_set(&reset);
    reset = false;
    inv_platform_device_reset_set(&reset);

    /* Set boot timeout timer
    inv_sys_time_out_milli_set(&p_obj->boot_time_out, BOOT_TIME_MAX);
}*/

    static bool_t s_device_ping(struct obj *p_obj) {
        inv_rval_t ret_val = 0;

        /*
         * Ping device
         */
        s_request_message_init(p_obj, INV478X_FIRMWAREVERSION_QUERY);
        s_request_message_send(p_obj);
        /*
         * wait for response from 8051 firmware and check result
         */
        ret_val = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FIRMWAREVERSION_QUERY);

        return (ret_val) ? (false) : (true);
    }

    static void s_interrupts_enable_set(struct obj *p_obj, bool_t b_enable) {
        if (b_enable)
            /*
             * Set interrupt mask bits to allow invokation of INT signal
             */
            inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__MCU_INT_MASK, 0x07);
        /*
         * inv_drv_cra_write8(p_obj->dev_id, 0x00f2, 0x01);
         */
        else
            /*
             * Clear interrupt mask bits to avoid invokation of INT signal
             */
            inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__MCU_INT_MASK, 0x00);
        /*
         * inv_drv_cra_write8(p_obj->dev_id, 0x00f2, 0x00);
         */
    }

#if (INV478X_USER_DEF__MULTI_THREAD)
    static inv_rval_t s_mutex_api_take(struct obj *p_obj) {
        inv_rval_t ret_val = 0;

        INV_ASSERT (p_obj->semaphore_api);
        INV_LOG2A ("s_mutex_api_take", p_obj, (""));
        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_take(p_obj->semaphore_api, IPC_COMPLETION_TIMEOUT))) {
            ret_val = INV478X_RETVAL__SEMAPHORE_API_EXPIRE;
        }
#if (INV478X_USER_DEF__IPC_SEMAPHORE)
        INV_ASSERT (p_obj->semaphore_handle);
        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_take(p_obj->semaphore_handle, IPC_COMPLETION_TIMEOUT))) {
            ret_val = INV478X_RETVAL__SEMAPHORE_HANDLE_EXPIRE;
        }
#endif
        return ret_val;
    }


    static void s_mutex_api_give(struct obj *p_obj) {
        INV_ASSERT (p_obj->semaphore_api);
        INV_LOG2A ("s_mutex_api_give", p_obj, (""));
#if ( INV478X_USER_DEF__IPC_SEMAPHORE )
        INV_ASSERT (p_obj->semaphore_handle);
        inv_platform_semaphore_give(p_obj->semaphore_handle);
#endif
        inv_platform_semaphore_give(p_obj->semaphore_api);
    }

    static inv_rval_t s_semaphore_ipc_take(struct obj *p_obj) {
        inv_rval_t ret_val = 0;

        INV_ASSERT (p_obj->semaphore_handle);
        INV_ASSERT (p_obj->semaphore_ipc);
        p_obj = p_obj;
#if ( INV478X_USER_DEF__IPC_SEMAPHORE )
        INV_LOG2A ("s_ipc_semaphore_give", p_obj, (""));

        /*
         * Ipc semaphore should not be given at this point. If still given then take ipc semaphore to clear
         */
        if (p_obj->b_ipc_given) {
            /*
             * Take IPC Semaphore
             */
            if (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_take(p_obj->semaphore_ipc, 1)) {
                INV_ASSERT (0);
            }

            p_obj->b_ipc_given = false;
        }

        /*
         * Give Handle Semaphore
         */
        inv_platform_semaphore_give(p_obj->semaphore_handle);

        if (!ret_val) {
            if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_take(p_obj->semaphore_ipc, IPC_COMPLETION_TIMEOUT))) {
                ret_val = INV478X_RETVAL__SEMAPHORE_IPC_EXPIRE;
            } else {
                p_obj->b_ipc_given = false;
            }
        }
        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_take(p_obj->semaphore_handle, IPC_COMPLETION_TIMEOUT))) {
            ret_val = INV478X_RETVAL__SEMAPHORE_HANDLE_EXPIRE;
        }
#endif
        return ret_val;
    }

    static void s_semaphore_ipc_give(struct obj *p_obj) {

        INV_ASSERT (p_obj->semaphore_ipc);
        p_obj = p_obj;
#if ( INV478X_USER_DEF__IPC_SEMAPHORE )
        INV_LOG2A ("s_semaphore_ipc_give", p_obj, (""));
        /*
         * Set Ipc given flag so that later we can still determine if IPC semaphore was still given after it was released through expiration
         */
        p_obj->b_ipc_given = true;

        inv_platform_semaphore_give(p_obj->semaphore_ipc);
#endif
    }

    static inv_rval_t s_mutex_handle_take(struct obj *p_obj) {
        inv_rval_t ret_val = 0;

        INV_ASSERT (p_obj->semaphore_handle);
        p_obj = p_obj;
#if ( INV478X_USER_DEF__IPC_SEMAPHORE )
        INV_LOG2A ("s_mutex_handle_take", p_obj, (""));

        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_take(p_obj->semaphore_handle, IPC_COMPLETION_TIMEOUT))) {
            ret_val = INV478X_RETVAL__SEMAPHORE_HANDLE_EXPIRE;
        }
#endif
        return ret_val;
    }

    static void s_mutex_handle_give(struct obj *p_obj) {
        INV_ASSERT (p_obj->semaphore_handle);
        p_obj = p_obj;
#if ( INV478X_USER_DEF__IPC_SEMAPHORE )
        INV_LOG2A ("s_mutex_handle_give", p_obj, (""));

        inv_platform_semaphore_give(p_obj->semaphore_handle);
#endif
    }
#endif
/***** end of file ***********************************************************/
