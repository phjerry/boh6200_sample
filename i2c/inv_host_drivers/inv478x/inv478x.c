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
 * file inv478x.c
 *
 * brief Defines the exported functions for the DLL application.
 *
 *****************************************************************************/
#define INV_DEBUG 0             /*DO NOT comment out.  Set value appropriately */

#pragma warning (disable : 4477 )
/***** #include statements ***************************************************/

#include "inv_sys.h"
#include "inv_common.h"
#include "inv478x_api.h"
#include "inv_drv_cra_api.h"
#include "inv_drv_isp_api.h"
#include "inv_drv_ipc_api.h"
#include "inv478x_platform_api.h"
#include "inv478x_ipc_def.h"
#include "inv478x_app_def.h"
#include "inv_sys_time_api.h"
#include "inv_sys_obj_api.h"
#include "inv478x_hal.h"
#include "inv_drv_ipc_master_api.h"

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (common_api, 1);

/***** local macro definitions ***********************************************/

#ifndef INV478X_USER_DEF__MULTI_THREAD
#define INV478X_USER_DEF__MULTI_THREAD        0
#endif

#define RESULT_CODE__CMD_SUCCESS                ((uint8_t)0x00)
#define RESULT_CODE__CMD_ERR_FAIL               ((uint8_t)0xFF)
#define EVENT_MASK_INTERNAL                     (0x02)
#define EVENT_MASK_EXTERNAL                     (~EVENT_MASK_INTERNAL)
#define IPC_COMPLETION_TIMEOUT                  3200    /* milliseconds */
#define IPC_COMPLETION_TIMEOUT_LICENSE_CERT_SET 7000    /* milliseconds, License certificate Set takes about 6 sec. */
#define BOOT_TIME_MAX                           5000    /* milliseconds */
#define EDID_MAX_BLOCK_LENGTH                   128
#define EDID_MAX_BLOCK_ID                       3
#define EDID_MAX_OFFSET_VALUE                   127
#define RETURN_API(retval)                      {if (retval == 0xFF) retval = INV_RVAL__NOT_IMPL_ERR; return retval; }

#if (INV478X_USER_DEF__MULTI_THREAD)
#if (INV478X_USER_DEF__IPC_SEMAPHORE)
#define MUTEX_HANDLE_TAKE(p_obj)            s_mutex_handle_take(p_obj)
#define MUTEX_HANDLE_GIVE(p_obj)            s_mutex_handle_give(p_obj)
#else
#define MUTEX_HANDLE_TAKE(p_obj)            s_mutex_api_take(p_obj)
#define MUTEX_HANDLE_GIVE(p_obj)            s_mutex_api_give(p_obj)
#endif
#define MUTEX_API_TAKE(p_obj)                 s_mutex_api_take(p_obj)
#define MUTEX_API_GIVE(p_obj)                 s_mutex_api_give(p_obj)
#else
#define MUTEX_HANDLE_TAKE(p_obj)              (0)
#define MUTEX_HANDLE_GIVE(p_obj)
#define MUTEX_API_TAKE(p_obj)                 (0)
#define MUTEX_API_GIVE(p_obj)
#endif

#define INV_LIC_CERT_SIZE                       64

#define mutex_t                                 uint8_t

/***** local type definitions ************************************************/

enum flashing_state {
    FLASHING_STATE_PROGRAMMING,
    FLASHING_STATE_DONE
};

struct obj {
    struct inv478x_config config;
    inv478x_event_flags_t event_flags_stat;
    inv478x_event_flags_t event_flags_mask;
    inv478x_event_flags_t event_flags_user;
    uint32_t h_inst;
    inv478x_event_callback_func_t inv478x_lib_evt_cb_func;
    inv_platform_dev_id dev_id;
#if (INV478X_USER_DEF__MULTI_THREAD)
    inv_platform_semaphore semaphore_api;
#endif
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
    inv_platform_semaphore semaphore_ipc;
    inv_platform_semaphore semaphore_handle;
    bool_t b_ipc_given;
    bool_t b_from_user_call_back;
#endif
    uint32_t file_pntr;
    uint32_t file_size;
    bool_t b_flash_read;
    uint8_t *p_flash_packet;
#if (INV478X_FLASH_BUFFER_SIZE)
    uint8_t *p_packet_pntr;
    uint16_t packet_size;
#endif
    inv_drv_ipc_tag_code tag_code;
    inv_drv_ipc_inst ipc_inst;
    inv_drv_ipc_mes_inst mes_req_inst;
    inv_drv_ipc_mes_inst mes_ack_inst;
    uint8_t reg0085;
    uint8_t reg334F;
    char p_license[90];
};

/***** local data objects ****************************************************/

static inv_inst_t s_obj_list_inst = INV_INST_NULL;
static struct obj *sp_obj;
inv_rval_t inv_platform_semaphore_create(const char* p_name, uint32_t max_count, uint32_t initial_value, uint32_t* p_sem_id);
inv_rval_t inv_platform_semaphore_delete(uint32_t sem_id);

/***** local functions ******************************************************/

#include "inv478x_comlib.c"

inv_rval_t inv_connection_test(inv_platform_dev_id dev_id)
{
    return ((inv_drv_cra_read_8(dev_id, REG08_MCU_STATUS) & 0x1) == 1) ? (INV_RVAL__SUCCESS) : (INV_RVAL__HOST_ERR);
}

/***** public functions ******************************************************/

inv_inst_t inv478x_create(inv_platform_dev_id dev_id, inv478x_event_callback_func_t p_callback, struct inv478x_config *p_config)
{
    inv_inst_t parent_inst = INV_INST_NULL;
    struct obj *p_obj = NULL;
    inv_rval_t ret_val = 0;
	printf("enter inv478x_create\n");
	usleep(1000000);
    if (!ret_val) {
        if (!s_obj_list_inst) {
            s_obj_list_inst = inv_sys_obj_list_create(INV_MODULE_NAME_GET (), sizeof(struct obj));
            if (!s_obj_list_inst)
                ret_val = 3;
        }
        if (!ret_val) {
            /*
             * Allocate memory for object
             */
            p_obj = (struct obj *) inv_sys_obj_instance_create(s_obj_list_inst, parent_inst, "RX");
            if (!p_obj) {
                /*
                 * Check for empty list
                 */
                if (!inv_sys_obj_first_get(s_obj_list_inst)) {
                    /*
                     * If empty then delete list of objects
                     */
                    inv_sys_obj_list_delete(s_obj_list_inst);
                    s_obj_list_inst = INV_INST_NULL;
                }
                ret_val = 5;
            }
        }
        if (!ret_val) {
            sp_obj = p_obj;
            p_obj->dev_id = dev_id, p_obj->inv478x_lib_evt_cb_func = p_callback;    /* allocate DrvAdapt handle */
            p_obj->config = *p_config;
            p_obj->event_flags_stat = 0x00000000;
            p_obj->event_flags_mask = 0xFFFFFFFF;
            p_obj->event_flags_user = 0x00000000;
            p_obj->file_pntr = 0;
            p_obj->file_size = 0;
            p_obj->p_flash_packet = NULL;
#if (INV478X_FLASH_BUFFER_SIZE)
            p_obj->p_packet_pntr = NULL;
            p_obj->packet_size = 0;
#endif
            p_obj->tag_code = 0;
            p_obj->ipc_inst = INV_INST_NULL;
            p_obj->mes_req_inst = INV_INST_NULL;
            p_obj->mes_ack_inst = INV_INST_NULL;
            p_obj->reg0085 = 0;
            p_obj->reg334F = 0;
#if (INV478X_USER_DEF__MULTI_THREAD)
            p_obj->semaphore_api = 0;
#endif
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
            p_obj->semaphore_ipc = 0;
            p_obj->semaphore_handle = 0;
            p_obj->b_ipc_given = false;
            p_obj->b_from_user_call_back = false;
#endif
        }
#if (INV478X_USER_DEF__MULTI_THREAD)
        /*
         * Create semaphore that functions as Mutex for making API multi-thread safe
         */
        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_create("SemaphoreApi", 1, 1, &p_obj->semaphore_api))) {
            INV_ASSERT (0);
            s_resources_delete(p_obj);
            ret_val = 3;
        }
#endif
#if (INV478X_USER_DEF__MULTI_THREAD && INV478X_USER_DEF__IPC_SEMAPHORE)
        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_create("SemaphoreIpc", 1, 0, &p_obj->semaphore_ipc))) {
            INV_ASSERT (0);
            s_resources_delete(p_obj);
            ret_val = 3;
        }
        if (!ret_val && (INV478X_PLATFORM_STATUS__SUCCESS != inv_platform_semaphore_create("SemaphoreHandle", 1, 1, &p_obj->semaphore_handle))) {
            INV_ASSERT (0);
            s_resources_delete(p_obj);
            ret_val = 4;
        }
#endif
        if (!ret_val) {
            struct inv_drv_ipc_config ipc_cfg;
            ipc_cfg.dev_id = dev_id;
            /*
             * initialize IPC module
             */
            p_obj->ipc_inst = inv_drv_ipc_create(p_obj->config.p_name_str, INV_OBJ2INST (p_obj), &ipc_cfg);
            inv_hal_interrupt_enable(dev_id);
            if (!p_obj->ipc_inst) {
                INV_ASSERT (0);
                s_resources_delete(p_obj);
                ret_val = 5;
            }
        }
        if (!ret_val) {
            /*
             * initialize IPC Request message
             */
            p_obj->mes_req_inst = inv_drv_ipc_message_create();
            if (!p_obj->mes_req_inst) {
                INV_ASSERT (0);
                s_resources_delete(p_obj);
                ret_val = 6;
            }
        }
        if (!ret_val) {
            /*
             * initialize IPC Acknowledge message
             */
            p_obj->mes_ack_inst = inv_drv_ipc_message_create();
            if (!p_obj->mes_ack_inst) {
                INV_ASSERT (0);
                s_resources_delete(p_obj);
                ret_val = 7;
            }
        }
        if (!ret_val) {
            /*
             * Enable INT signal activation
             */
            s_interrupts_enable_set(p_obj, true);
            if (p_config->b_device_reset)
                /*
                 * Apply device reset
                 */
                p_config->b_device_reset = p_config->b_device_reset;
        }
    }
    /*
     * After connection done, send a NACK to clear earlier notification sent
     */
     printf("enter inv478x_create 22\n");
	usleep(1000000);
    inv_drv_cra_write_8(dev_id, REG08_MCU_INT_TRIGGER, INV478X_FLAG_NACK);
    inv_drv_cra_write_8(dev_id, REG08__MCU_REG__MCU__TX_FIFO_CONTROL, BIT__MCU_REG__MCU__TX_FIFO_CONTROL__RESET);
    inv_drv_cra_write_8(dev_id, REG08__MCU_REG__MCU__RX_FIFO_CONTROL, BIT__MCU_REG__MCU__RX_FIFO_CONTROL__RESET);
    return (ret_val) ? (INV_INST_NULL) : (INV_OBJ2INST (p_obj));
}

void inv478x_delete(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t ret_val = 0;

    if (!ret_val) {
        /*
         * Disable INT signal activation
         */
        s_interrupts_enable_set(p_obj, false);
        s_resources_delete(p_obj);
    }
}

inv_rval_t inv478x_handle(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t ret_val = 0;

    if (INV_INST_NULL == inst) {
        /*
         * Handle multi host driver instantiations
         */
        p_obj = (struct obj *) inv_sys_obj_first_get(s_obj_list_inst);
        while (p_obj && !ret_val) {
            ret_val = s_handle(p_obj);
            /*
             * Go to next timer instance
             */
            p_obj = (struct obj *) inv_sys_obj_next_get(p_obj);
        }
    } else {
        /*
         * Handle single host driver instantiation
         */
        p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
        ret_val = s_handle(p_obj);
    }

    RETURN_API (ret_val);
}

/*
 * Driver General APIs
 */
inv_rval_t inv478x_chip_id_query(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_CHIPID_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_CHIPID_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_chip_revision_query(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_CHIPREVISION_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_CHIPREVISION_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_firmware_version_query(inv_inst_t inst, uint8_t *p_val)
{
    uint16_t length;
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_FIRMWAREVERSION_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FIRMWAREVERSION_QUERY);
    if (!rval) {
        length = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, length);
        p_val[length] = '\0';
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_heart_beat_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_HEART_BEAT_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, (*p_val));
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HEART_BEAT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_heart_beat_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_HEART_BEAT_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HEART_BEAT_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_event_flags_mask_set(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EVENTFLAGSMASK_SET);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, (*p_val));
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EVENTFLAGSMASK_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_event_flags_mask_get(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EVENTFLAGSMASK_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EVENTFLAGSMASK_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_obj->event_flags_mask = *p_val;
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_freeze_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_FREEZE_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, (*p_val));
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FREEZE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_freeze_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_FREEZE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FREEZE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_boot_stat_query(inv_inst_t inst, enum inv_sys_boot_stat *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    uint8_t boot1 = inv_drv_cra_read_8(p_obj->dev_id, 0x7020) & 0x1e;
    uint8_t boot2 = inv_drv_cra_read_8(p_obj->dev_id, 0x71F3) & 0x01;
    uint8_t boot3 = inv_drv_cra_read_8(p_obj->dev_id, 0x740E) & 0x08;
    uint8_t boot_flag = ((inv_drv_cra_read_8(p_obj->dev_id, 0x7020) || inv_drv_cra_read_8(p_obj->dev_id, 0x71F3) || inv_drv_cra_read_8(p_obj->dev_id, 0x740E)) == 0x00) ? 0x1 : 0x00;

    if (boot1 || boot2 || boot_flag)
        *p_val = INV_SYS_BOOT_STAT__FAILURE;
    else if (boot3)
        *p_val = INV_SYS_BOOT_STAT__SUCCESS;
    else
        *p_val = INV_SYS_BOOT_STAT__IN_PROGRESS;

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_event_flags_stat_query(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EVENT_FLAG_STAT_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EVENT_FLAG_STAT_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_event_flag_stat_clear(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EVENT_FLAG_STAT_CLEAR);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EVENT_FLAG_STAT_CLEAR);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_reboot_req(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    /*
     * software reset for everything, so software can reset the chip
     */
    inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_TOP_REG__PCM_CONFIG, 0x09);
    inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_TOP_REG__PCM_CONFIG, 0x01);
    inv_sys_time_milli_delay(500);

    RETURN_API (rval);
}

inv_rval_t inv478x_dl_swap_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_DL_SWEEP_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, (*p_val));
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_DL_SWEEP_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_swap_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_DL_SWEEP_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_DL_SWEEP_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_chip_serial_query(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_CHIP_SERIAL_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_CHIP_SERIAL_QUERY);
    if (!rval) {
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_license_cert_set(inv_inst_t inst, char *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    uint8_t cert_hex[64] = { 0 };

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    inv_base64_decode(cert_hex, INV_LIC_CERT_SIZE, p_val);
    INV_MEMCPY (p_obj->p_license, p_val, 89);

    s_request_message_init(p_obj, INV478X_LICENSE_CERT_SET);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, (uint8_t *) cert_hex, 64);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_LICENSE_CERT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_license_id_query(inv_inst_t inst, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_LICENSE_ID_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_LICENSE_ID_QUERY);
    if (!rval) {
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_pwr_down_req(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);

    rval = inv_connection_test(p_obj->dev_id);
    if (!rval) {
        s_request_message_init(p_obj, INV478X_PWR_DOWN_REQ);
        s_request_message_send(p_obj);
        rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_PWR_DOWN_REQ);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_flash_erase(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    uint8_t i;
    uint8_t val;
    uint32_t addr = 0;

    inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__RX_FIFO_CONTROL, BIT__MCU_REG__MCU__RX_FIFO_CONTROL__RESET);
    if (inv_drv_cra_read_8(p_obj->dev_id, 0x0000) == 0) {
        rval = INV_RVAL__BOARDNOT_CONNECTED;
    }
    if (rval != INV_RVAL__BOARDNOT_CONNECTED) {
        for (i = 0; i < 5; i++) {
            inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_TOP_REG__AON_CLK_SEL_REG, 0x00);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_TOP_REG__PCM_CONFIG, 0x08);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__BOOT_REG__BT__BOOT_CONFIG, 0x00);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__MCU_CONFIG, BIT__MCU_REG__MCU__MCU_CONFIG__MCU_RESET);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, BIT__ISP_REG__ISP__ISP_CONFIG__ENABLE);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x06);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0xd8);
            inv_drv_cra_write_24(p_obj->dev_id, REG24__ISP_REG__ISP__ISP_ADDR, addr);
            inv_sys_time_milli_delay(1000);
            addr = addr + 65536;
        }
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, 0x01);

        for (i = 0; i < 4; i++) {
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x03);
            inv_drv_cra_write_24(p_obj->dev_id, REG24__ISP_REG__ISP__ISP_ADDR, i);
            val = inv_drv_cra_read_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_DATA);
            if (val != 0xff)
                rval = INV_RVAL__ERASE_FAILED;
        }
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, 0x00);
    }
    RETURN_API (rval);
}

inv_rval_t inv478x_flash_full_erase(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    uint8_t i;
    uint8_t val;
    uint32_t addr = 0;
    addr = addr;

    inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__RX_FIFO_CONTROL, BIT__MCU_REG__MCU__RX_FIFO_CONTROL__RESET);
    if (inv_drv_cra_read_8(p_obj->dev_id, 0x0000) == 0) {
        rval = INV_RVAL__BOARDNOT_CONNECTED;
    }
    if (rval != INV_RVAL__BOARDNOT_CONNECTED) {

        uint8_t stat_reg = 0;
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, BIT__ISP_REG__ISP__ISP_CONFIG__SPI_ACCESS_MODE);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x06);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x01);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_DATA, 0x80);
        inv_sys_time_milli_delay(1000);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x03);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x05);
        stat_reg = inv_drv_cra_read_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_DATA);
        if (stat_reg == 0x80)
        {
            inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_TOP_REG__AON_CLK_SEL_REG, 0x00);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_TOP_REG__PCM_CONFIG, 0x08);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__BOOT_REG__BT__BOOT_CONFIG, 0x00);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__MCU_CONFIG, BIT__MCU_REG__MCU__MCU_CONFIG__MCU_RESET);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, BIT__ISP_REG__ISP__ISP_CONFIG__SPI_ACCESS_MODE);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x06);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x60);
            inv_sys_time_milli_delay(2500);

            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, 0x01);

            for (i = 0; i < 4; i++) {
                inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x03);
                inv_drv_cra_write_24(p_obj->dev_id, REG24__ISP_REG__ISP__ISP_ADDR, i);
                val = inv_drv_cra_read_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_DATA);
                if (val != 0xff)
                    rval = INV_RVAL__ERASE_FAILED;
            }
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x06);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x01);
            inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_DATA, 0x84);
        }
        else
            rval = INV_RVAL__ERASE_FAILED;
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, 0x00);
    }
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_mode_set(inv_inst_t inst, enum inv478x_dl_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_DL_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_DL_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_mode_get(inv_inst_t inst, enum inv478x_dl_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_DL_MODE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_DL_MODE_GET);
    if (!rval) {
        *p_val = (enum inv478x_dl_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_split_overlap_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_DL_SPLIT_OVERLAP_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_DL_SPLIT_OVERLAP_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_split_overlap_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_DL_SPLIT_OVERLAP_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_DL_SPLIT_OVERLAP_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_hdcp_ksv_big_endian_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_HDCP_KSV_BIG_ENDIAN_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HDCP_KSV_BIG_ENDIAN_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_hdcp_ksv_big_endian_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_HDCP_KSV_BIG_ENDIAN_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HDCP_KSV_BIG_ENDIAN_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_reg_write(inv_inst_t inst, uint16_t addr, uint8_t size, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    inv_drv_cra_block_write_8(p_obj->dev_id, addr, p_val, size);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_reg_read(inv_inst_t inst, uint16_t addr, uint8_t size, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    inv_drv_cra_block_read_8(p_obj->dev_id, addr, p_val, size);
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_firmware_user_tag_query(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
	uint8_t length;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_FIRMWAREUSERTAG_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FIRMWAREUSERTAG_QUERY);
    if (!rval) {
        length = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, length);
        p_val[length] = '\0';
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}






/*
 * Audio Port
 */
inv_rval_t inv478x_ap_rx_fmt_set(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_RX_FMT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_RX_FMT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_rx_fmt_get(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_RX_FMT_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_RX_FMT_GET);
    if (!rval)
        *p_val = (enum inv_audio_fmt) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_rx_info_set(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_RX_INFO_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_val->cs.b, 9);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_val->map.b, 10);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_RX_INFO_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_rx_info_get(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_RX_INFO_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_RX_INFO_GET);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->cs.b, 9);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->map.b, 10);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_tx_fmt_query(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_TX_FMT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_TX_FMT_QUERY);
    if (!rval)
        *p_val = (enum inv_audio_fmt) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_tx_info_query(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_TX_INFO_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_TX_INFO_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->cs.b, 9);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->map.b, 10);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_tx_mute_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_TX_MUTE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_TX_MUTE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_tx_mute_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_TX_MUTE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_TX_MUTE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_fmt_set(inv_inst_t inst, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_ATPG_FMT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_FMT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_fmt_get(inv_inst_t inst, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_ATPG_FMT_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_FMT_GET);
    if (!rval)
        *p_val = (enum inv_audio_fmt) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_ampl_set(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_ATPG_AMPLITUDE_SET);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_AMPLITUDE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_ampl_get(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_ATPG_AMPLITUDE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_AMPLITUDE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_freq_set(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_ATPG_FREQUENCY_SET);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_FREQUENCY_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_freq_get(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_ATPG_FREQUENCY_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_FREQUENCY_GET);
    if (!rval)
        *p_val = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_ptrn_set(inv_inst_t inst, enum inv478x_atpg_ptrn *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_ATPG_PATTERN_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_PATTERN_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_ptrn_get(inv_inst_t inst, enum inv478x_atpg_ptrn *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_ATPG_PATTERN_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_PATTERN_GET);
    if (!rval)
        *p_val = (enum inv478x_atpg_ptrn) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

/*
 * A/V Up-Stream Management
 */

inv_rval_t inv478x_rx_port_select_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_INPUTSELECT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_INPUTSELECT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_port_select_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_INPUTSELECT_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_INPUTSELECT_GET);
    if (!rval)
        *p_val = (uint8_t) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_fast_switch_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_FASTSWITCH_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FASTSWITCH_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_fast_switch_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_FASTSWITCH_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_FASTSWITCH_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_set(inv_inst_t inst, uint8_t port, uint8_t block, uint8_t offset, uint8_t len, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    if ((len + offset) > EDID_MAX_BLOCK_LENGTH)
        len = (EDID_MAX_BLOCK_LENGTH - offset);

    s_request_message_init(p_obj, INV478X_EDIDRX_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, block);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_val, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EDIDRX_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_get(inv_inst_t inst, uint8_t port, uint8_t block, uint8_t offset, uint8_t len, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    if ((len + offset) > EDID_MAX_BLOCK_LENGTH)
        len = (EDID_MAX_BLOCK_LENGTH - offset);

    s_request_message_init(p_obj, INV478X_EDIDRX_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, block);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EDIDRX_GET);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, len);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_video_tim_query(inv_inst_t inst, uint8_t port, struct inv_video_timing *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VIDEOTIMING_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VIDEOTIMING_QUERY);
    if (!rval) {
        p_val->v_act = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->v_syn = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->v_fnt = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->v_bck = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_act = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_syn = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_fnt = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_bck = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->pclk = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_val->v_pol = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->h_pol = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->intl = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_av_link_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AVLINKRX_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AVLINKRX_QUERY);
    if (!rval)
        *p_val = (enum inv_hdmi_av_link) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HPD_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HPD_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HPD_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HPD_ENABLE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_deep_clr_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_DEEPCOLOR_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_DEEPCOLOR_QUERY);
    if (!rval)
        *p_val = (enum inv_hdmi_deep_clr) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_pixel_fmt_query(inv_inst_t inst, uint8_t port, struct inv_video_pixel_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PIXEL_FMT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PIXEL_FMT_QUERY);
    if (!rval) {
        p_val->colorimetry = (enum inv_video_colorimetry) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->chroma_smpl = (enum inv_video_chroma_smpl) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->full_range = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->bitdepth = (enum inv_video_bitdepth) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_arc_support_mode_set(inv_inst_t inst, enum inv_hdmi_earc_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    if (*p_val > INV_HDMI_EARC_MODE__EARC) {
        rval = INV_RVAL__PAR_ERR;
        RETURN_API (rval);
    }

    s_request_message_init(p_obj, INV478X_ARC_SUPPORT_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ARC_SUPPORT_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_arc_support_mode_get(inv_inst_t inst, enum inv_hdmi_earc_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_ARC_SUPPORT_MODE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ARC_SUPPORT_MODE_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_earc_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ext_aud_info_get(inv_inst_t inst, struct inv_audio_cs *channel_status, struct inv478x_aif_extract *aif)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (channel_status);
    INV_ASSERT (aif);

    s_request_message_init(p_obj, INV478X_CHANNEL_STATUS_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_CHANNEL_STATUS_GET);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, channel_status->b, 7);
    s_request_message_init(p_obj, INV478X_AIF_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AIF_GET);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, (uint8_t *) aif, 4);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_phy_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HPD_PHY_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HPD_PHY_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_low_time_set(inv_inst_t inst, uint8_t port, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HPD_LOW_TIME_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HPD_LOW_TIME_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_low_time_get(inv_inst_t inst, uint8_t port, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HPD_LOW_TIME_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HPD_LOW_TIME_GET);
    if (!rval)
        *p_val = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_rsen_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_RSEN_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_RSEN_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_rsen_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_RSEN_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_RSEN_ENABLE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_EDID_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_EDID_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_EDID_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_EDID_ENABLE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}


inv_rval_t inv478x_rx_plus5v_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PLUS5V_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PLUS5V_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_avmute_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_AVMUTE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_AVMUTE_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_bksv_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_BKSV_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_BKSV_QUERY);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->b, 5);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_rxid_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_RXID_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_RXID_QUERY);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->b, 5);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_aksv_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_AKSV_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_AKSV_QUERY);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->b, 5);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_stat_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_stat *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_STAT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_STAT_QUERY);
    if (!rval)
        *p_val = (enum inv_hdcp_stat) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_ext_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_EXT_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_EXT_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);

}

inv_rval_t inv478x_rx_hdcp_ext_topology_set(inv_inst_t inst, uint8_t port, struct inv_hdcp_top *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_EXT_TOPOLOGY_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, p_val->hdcp1_dev_ds);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, p_val->hdcp2_rptr_ds);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, p_val->max_cas_exceed);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, p_val->max_devs_exceed);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->rptr_depth);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->dev_count);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, p_val->seq_num_v);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_EXT_TOPOLOGY_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_ext_ksvlist_set(inv_inst_t inst, uint8_t port, uint8_t len, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_EXT_KSV_LIST_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_val->b, 5 * len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_EXT_KSV_LIST_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_encryption_status_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_ENCRYPTION_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_ENCRYPTION_GET);
    if (!rval)
        *p_val = (bool_t) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_video_fmt_query(inv_inst_t inst, uint8_t port, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_VIDEO_FMT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_VIDEO_FMT_QUERY);
    if (!rval) {
        *p_val = (uint16_t) inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_audio_fmt_query(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_AUDIO_FMT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_AUDIO_FMT_QUERY);
    if (!rval)
        *p_val = (enum inv_audio_fmt) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_audio_info_query(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_AUDIO_INFO_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_AUDIO_INFO_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->cs.b, 9);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->map.b, 10);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_audio_ctsn_query(inv_inst_t inst, uint8_t port, struct inv_hdmi_audio_ctsn *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_AUDIO_CSTN_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_AUDIO_CSTN_QUERY);
    if (!rval) {
        p_val->cts = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_val->n = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_split_auto_set(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AUTOSPLITCHRRATE_SET);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AUTOSPLITCHRRATE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_dl_split_auto_get(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AUTOSPLITCHRRATE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AUTOSPLITCHRRATE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_scdc_register_query(inv_inst_t inst, uint8_t port, uint8_t offset, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_SCDC_REGISTER_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_SCDC_REGISTER_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_packet, len);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_mode_set(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_mode_get(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_MODE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_MODE_GET);
    if (!rval)
        *p_val = (enum inv_hdcp_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_trash_req(inv_inst_t inst, uint8_t port)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    s_request_message_init(p_obj, INV478X_RX_HDCP_TRASH_REQ);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_TRASH_REQ);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_type_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_TYPE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_TYPE_QUERY);
    if (!rval)
        *p_val = (enum inv_hdcp_type) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_act_stat_query(inv_inst_t inst, uint8_t port, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_ACT_STAT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_ACT_STAT_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_evt_stat_query(inv_inst_t inst, uint8_t port, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_EVT_STAT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_EVT_STAT_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_evt_mask_set(inv_inst_t inst, uint8_t port, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_EVT_MASK_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_EVT_MASK_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_evt_mask_get(inv_inst_t inst, uint8_t port, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_EVT_MASK_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_EVT_MASK_GET);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_csm_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_csm *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_CSM_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_CSM_QUERY);
    if (!rval) {
        p_val->seq_num_m = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_val->stream_id_type = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
    }


    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_rsen_value_set(inv_inst_t inst, uint8_t port, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_RSEN_VALUE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_RSEN_VALUE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_rsen_value_get(inv_inst_t inst, uint8_t port, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_RSEN_VALUE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_RSEN_VALUE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_auto_adb_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_EDID_AUTO_ADB_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_EDID_AUTO_ADB_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_auto_adb_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_EDID_AUTO_ADB_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_EDID_AUTO_ADB_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_vs_user_ieee_set(inv_inst_t inst, uint8_t port, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_VS_USER_IEEE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_VS_USER_IEEE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_packet_vs_user_ieee_get(inv_inst_t inst, uint8_t port, uint32_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_PACKET_VS_USER_IEEE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_PACKET_VS_USER_IEEE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);

}

inv_rval_t inv478x_rx_hdcp_cap_set(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_CAP_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_CAP_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_cap_get(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_CAP_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_CAP_GET);
    if (!rval) {
        *p_val = (enum inv_hdcp_type) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_topology_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_top *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_TOPOLOGY_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_TOPOLOGY_QUERY);
    if (!rval) {
        p_val->hdcp1_dev_ds = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->hdcp2_rptr_ds = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->max_cas_exceed = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->max_devs_exceed = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->dev_count = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->rptr_depth = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->seq_num_v = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hdcp_ksvlist_query(inv_inst_t inst, uint8_t port, uint8_t len, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    uint8_t i;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_HDCP_KSVLIST_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_HDCP_KSVLIST_QUERY);
    if (!rval) {
        for (i = 0; i < len; i++) {
            inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val[i].b, 5);
        }
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

/****************************************************************************/
/**** EARC Management
/****************************************************************************/
inv_rval_t inv478x_earc_aud_mute_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_AUDIO_MUTE_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_AUDIO_MUTE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_aud_mute_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_AUDIO_MUTE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_AUDIO_MUTE_GET);
    if (!rval)
        *p_val = (bool_t) inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_tx_src_set(inv_inst_t inst, enum inv478x_audio_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_TX_AUDIO_SRC_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_TX_AUDIO_SRC_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_tx_src_get(inv_inst_t inst, enum inv478x_audio_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_TX_AUDIO_SRC_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_TX_AUDIO_SRC_GET);
    if (!rval)
        *p_val = (enum inv478x_audio_src) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_mode_set(inv_inst_t inst, enum inv_hdmi_earc_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    if (*p_val > INV_HDMI_EARC_MODE__EARC) {
        rval = INV_RVAL__PAR_ERR;
        RETURN_API (rval);
    }

    s_request_message_init(p_obj, INV478X_EARC_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_mode_get(inv_inst_t inst, enum inv_hdmi_earc_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_MODE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_MODE_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_earc_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_fmt_query(inv_inst_t inst, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_AUD_FMT_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_AUD_FMT_QUERY);
    if (!rval)
        *p_val = (enum inv_audio_fmt) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_info_query(inv_inst_t inst, struct inv_audio_info *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_AUD_INFO_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_AUD_INFO_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->cs.b, 9);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->map.b, 10);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_link_query(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_LINK_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_LINK_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_erx_latency_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_EARC_LATENCY_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_LATENCY_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_erx_latency_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_LATENCY_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_LATENCY_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_earc_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_erx_latency_query(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_LATENCY_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_LATENCY_QUERY);
    if (!rval)
        *p_val = (enum inv_hdmi_earc_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_rxcap_struct_set(inv_inst_t inst, uint8_t offset, uint8_t len, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_RX_RXCAP_STRUCT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_val, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_RX_RXCAP_STRUCT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_rxcap_struct_get(inv_inst_t inst, uint8_t offset, uint8_t len, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_RX_RXCAP_STRUCT_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_RX_RXCAP_STRUCT_GET);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, len);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_rxcap_struct_query(inv_inst_t inst, uint8_t offset, uint8_t len, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_TX_RX_CAP_STRUCT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_TX_RX_CAP_STRUCT_QUERY);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, len);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_packet_set(inv_inst_t inst, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_PACKET_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) len);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_packet, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_PACKET_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_packet_get(inv_inst_t inst, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_PACKET_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_PACKET_GET);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_packet, len);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_packet_query(inv_inst_t inst, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_PACKET_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_PACKET_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_packet, len);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_cfg_set(inv_inst_t inst, enum inv478x_earc_port_cfg *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_CONFIG_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_CONFIG_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_earc_cfg_get(inv_inst_t inst, enum inv478x_earc_port_cfg *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EARC_CONFIG_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EARC_CONFIG_GET);
    if (!rval)
        *p_val = (enum inv478x_earc_port_cfg) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

/*
 * Audio Port APIs
 */

inv_rval_t inv478x_ap_tx_src_set(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_TX_SRC_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_TX_SRC_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_tx_src_get(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_TX_SRC_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_TX_SRC_GET);
    if (!rval)
        *p_val = (enum inv478x_audio_src) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_mode_set(inv_inst_t inst, uint8_t port, enum inv_audio_ap_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);

}

inv_rval_t inv478x_ap_mode_get(inv_inst_t inst, uint8_t port, enum inv_audio_ap_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_MODE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_MODE_GET);
    if (!rval)
        *p_val = (enum inv_audio_ap_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_mclk_set(inv_inst_t inst, uint8_t port, enum inv_audio_mclk *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_MCLK_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_MCLK_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_mclk_get(inv_inst_t inst, uint8_t port, enum inv_audio_mclk *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_MCLK_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_MCLK_GET);
    if (!rval)
        *p_val = (enum inv_audio_mclk) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_fs_set(inv_inst_t inst, enum inv_audio_fs *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_ATPG_FS_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_FS_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_atpg_fs_get(inv_inst_t inst, enum inv_audio_fs *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_ATPG_FS_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_ATPG_FS_GET);
    if (!rval)
        *p_val = (enum inv_audio_fs) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_cfg_set(inv_inst_t inst, uint8_t port, enum inv478x_ap_port_cfg *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_CONFIG_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_CONFIG_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_ap_cfg_get(inv_inst_t inst, uint8_t port, enum inv478x_ap_port_cfg *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_AP_CONFIG_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_AP_CONFIG_GET);
    if (!rval)
        *p_val = (enum inv478x_ap_port_cfg) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

/*
 * TPG Management
 */

inv_rval_t inv478x_vtpg_enable_set(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPGENABLE_SET);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPGENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_enable_get(inv_inst_t inst, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPGENABLE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPGENABLE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_ptrn_set(inv_inst_t inst, enum inv_vtpg_ptrn *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPGPATTERN_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPGPATTERN_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_ptrn_get(inv_inst_t inst, enum inv_vtpg_ptrn *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPGPATTERN_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPGPATTERN_GET);
    if (!rval)
        *p_val = (enum inv_vtpg_ptrn) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_video_fmt_set(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPGVIDFRM_SET);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, (uint16_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPGVIDFRM_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_video_fmt_get(inv_inst_t inst, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPGVIDFRM_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPGVIDFRM_GET);
    if (!rval)
        *p_val = (uint16_t) inv_drv_ipc_word_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}





/*
 * A/V Down-Stream APIs
 */

inv_rval_t inv478x_tx_audio_info_query(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_INFO_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_INFO_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->cs.b, 9);
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->map.b, 10);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_replicate_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_hpd_repl *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    /*
     * Presently port zero replication has higher preference,
     * * if user sets HPD replication for both the Tx port then,
     * * HPD replication will work for Tx port zero
     */
    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_HPD_REPLICATE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HPD_REPLICATE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_hpd_replicate_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_hpd_repl *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_HPD_REPLICATE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HPD_REPLICATE_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_hpd_repl) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_replicate_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_edid_repl *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    /*
     * As of now implementing for TX port zero only,
     * * if user enters port value greater than one then
     * * host driver returns non-zero value(an error case)
     */
    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EDID_REPLICATE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EDID_REPLICATE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_edid_replicate_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_edid_repl *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    /*
     * As of now implementing for TX port zero only,
     * * if user enters port value greater than one then
     * * host driver returns non-zero value(an error case)
     */
    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_EDID_REPLICATE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EDID_REPLICATE_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_edid_repl) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_av_link_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_RX_AV_LINK_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_AV_LINK_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_rx_av_link_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_RX_AV_LINK_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_RX_AV_LINK_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_av_link) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}



inv_rval_t inv478x_tx_rsen_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_RSEN_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_RSEN_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}



inv_rval_t inv478x_tx_edid_stat_query(inv_inst_t inst, uint8_t port, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_EDID_STATUS_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_EDID_STATUS_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_audio_fmt_query(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_FMT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_FMT_QUERY);
    if (!rval)
        *p_val = (enum inv_audio_fmt) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_av_link_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    /*
     * Tx0 and Tx1 use the same setting for now
     */
    s_request_message_init(p_obj, INV478X_TX_AV_LINK_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AV_LINK_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_av_link_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AV_LINK_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AV_LINK_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_av_link) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_ddc_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_TX_DDC_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DDC_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_ddc_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_DDC_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DDC_ENABLE_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_av_link) inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}


inv_rval_t inv478x_tx_deep_clr_max_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_BITDEPTH_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_BITDEPTH_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_deep_clr_max_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_BITDEPTH_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_BITDEPTH_GET);
    if (!rval)
        *p_val = (enum inv_hdmi_deep_clr) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hpd_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_HPDTX_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_HPDTX_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_edid_query(inv_inst_t inst, uint8_t port, uint8_t block, uint8_t offset, uint8_t len, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    if (len > offset)
        len = len - offset;
    else if ((len + offset) > EDID_MAX_BLOCK_LENGTH)
        len = (EDID_MAX_BLOCK_LENGTH - offset);
    s_request_message_init(p_obj, INV478X_EDIDTX_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, block);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_EDIDTX_QUERY);
    if (!rval)
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, len);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}


inv_rval_t inv478x_tx_video_src_set(inv_inst_t inst, uint8_t port, enum inv478x_video_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AV_SRC_SELECT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AV_SRC_SELECT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_video_src_get(inv_inst_t inst, uint8_t port, enum inv478x_video_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AV_SRC_SELECT_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AV_SRC_SELECT_GET);
    if (!rval)
        *p_val = (enum inv478x_video_src) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_tmds2_1over4_query(inv_inst_t inst, uint8_t port, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_TMDS2_1OVER4_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_TMDS2_1OVER4_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_tmds2_scramble340_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_TMDS2_SCRAMBLE340_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_TMDS2_SCRAMBLE340_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_tmds2_scramble340_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_TMDS2_SCRAMBLE340_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_TMDS2_SCRAMBLE340_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_mode_set(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_mode_get(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_MODE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_MODE_GET);
    if (!rval)
        *p_val = (enum inv_hdcp_mode) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_start(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_START);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_START);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_stat_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_stat *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_STAT_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_STAT_QUERY);
    if (!rval)
        *p_val = (enum inv_hdcp_stat) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_type_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_TYPE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_TYPE_QUERY);
    if (!rval)
        *p_val = (enum inv_hdcp_type) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);

}

inv_rval_t inv478x_cec_arc_init(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (sObjListInst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    s_request_message_init(p_obj, INV478X_CEC_ARC_INIT_NOTFY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_CEC_ARC_INIT_NOTFY);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_cec_arc_term(inv_inst_t inst)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (sObjListInst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    s_request_message_init(p_obj, INV478X_CEC_ARC_TERM_NOTFY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_CEC_ARC_TERM_NOTFY);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_avmute_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AVMUTE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AVMUTE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_avmute_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AVMUTE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AVMUTE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_avmute_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AVMUTE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AVMUTE_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_gated_by_hpd_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_GATED_BY_HPD_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_GATED_BY_HPD_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_gated_by_hpd_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_GATED_BY_HPD_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_GATED_BY_HPD_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_edid_read_on_hpd_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_EDID_READ_ON_HPD_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_EDID_READ_ON_HPD_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_edid_read_on_hpd_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_EDID_READ_ON_HPD_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_EDID_READ_ON_HPD_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_topology_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_top *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_TOPOLOGY_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_TOPOLOGY_QUERY);
    if (!rval) {
        p_val->hdcp1_dev_ds = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->hdcp2_rptr_ds = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->max_cas_exceed = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->max_devs_exceed = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->dev_count = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->rptr_depth = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->seq_num_v = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_ksvlist_query(inv_inst_t inst, uint8_t port, uint8_t len, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    uint8_t i;
    uint8_t j;
    uint8_t t_len;
    uint8_t st_source;
    uint8_t loop_count;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    loop_count = len / 52;
    st_source = 0;
    t_len = 52;
    for (j = 0; j <= loop_count; j++) {
        if (j == loop_count)
            t_len = len - (t_len * loop_count);
        s_request_message_init(p_obj, INV478X_TX_HDCP_KSV_LIST_QUERY);
        inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
        inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) t_len);
        s_request_message_send(p_obj);
        rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_KSV_LIST_QUERY);
        if (!rval) {
            for (i = st_source; i < st_source + t_len; i++) {
                inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val[i].b, 5);
            }
        }
        st_source = st_source + t_len;
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_csm_set(inv_inst_t inst, uint8_t port, struct inv_hdcp_csm *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_CSM_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, p_val->seq_num_m);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->stream_id_type);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_CSM_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_csm_get(inv_inst_t inst, uint8_t port, struct inv_hdcp_csm *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_CSM_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_CSM_GET);
    if (!rval) {
        p_val->seq_num_m = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_val->stream_id_type = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_aksv_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_AKSV_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_AKSV_QUERY);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val->b, 5);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_failure_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_failure *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_FAILURE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_FAILURE_QUERY);
    if (!rval) {
        *p_val = (enum inv_hdcp_failure) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_audio_ctsn_query(inv_inst_t inst, uint8_t port, struct inv_hdmi_audio_ctsn *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_CTSN_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_CTSN_QUERY);
    if (!rval) {
        p_val->cts = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_val->n = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_audio_mute_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_MUTE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_MUTE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_audio_mute_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_MUTE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_MUTE_GET);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_encrypt_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_ENCRYPT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_ENCRYPT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_encrypt_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_ENCRYPT_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_ENCRYPT_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_approve(inv_inst_t inst, uint8_t port)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    s_request_message_init(p_obj, INV478X_TX_HDCP_APPROVE);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_APPROVE);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_csc_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_CSC_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_CSC_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_csc_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_CSC_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_CSC_ENABLE_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_csc_failure_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_CSC_FAILURE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_CSC_FAILURE_QUERY);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_csc_pixel_fmt_set(inv_inst_t inst, uint8_t port, struct inv_video_pixel_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_CSC_PIXEL_FMT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->colorimetry);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->chroma_smpl);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, p_val->full_range);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->bitdepth);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_CSC_PIXEL_FMT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_csc_pixel_fmt_get(inv_inst_t inst, uint8_t port, struct inv_video_pixel_fmt *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_CSC_PIXEL_FMT_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_CSC_PIXEL_FMT_GET);
    if (!rval) {
        p_val->colorimetry = (enum inv_video_colorimetry) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->chroma_smpl = (enum inv_video_chroma_smpl) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->full_range = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
        p_val->bitdepth = (enum inv_video_bitdepth) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_reauth_req(inv_inst_t inst, uint8_t port)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    s_request_message_init(p_obj, INV478X_TX_HDCP_REAUTH_REQ);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_REAUTH_REQ);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_ds_cap_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_DS_CAP_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_DS_CAP_QUERY);
    if (!rval) {
        *p_val = (enum inv_hdcp_type) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_auto_approve_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_AUTO_APPROVE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_AUTO_APPROVE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hdcp_auto_approve_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HDCP_AUTO_APPROVE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HDCP_AUTO_APPROVE_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_flash_program(inv_inst_t inst, uint8_t *p_val, uint32_t len)
{

    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    inv_drv_cra_write_8(p_obj->dev_id, REG08__MCU_REG__MCU__RX_FIFO_CONTROL, BIT__MCU_REG__MCU__RX_FIFO_CONTROL__RESET);
    if (inv_drv_cra_read_8(p_obj->dev_id, 0x0000) == 0) {
        rval = INV_RVAL__PROGRAM_FAILED;
    }
    if (rval != INV_RVAL__PROGRAM_FAILED) {
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_CONFIG, 0x09);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x06);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_OPCODE, 0x02);
        inv_drv_cra_write_24(p_obj->dev_id, REG24__ISP_REG__ISP__ISP_ADDR, len);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_DIG_REG__DIG_CONFIG0, 0x02);
        inv_drv_cra_block_write_8(p_obj->dev_id, REG08__ISP_REG__ISP__ISP_DATA, p_val, 256);
        inv_drv_cra_write_8(p_obj->dev_id, REG08__AON_DIG_REG__DIG_CONFIG0, 0x03);
        inv_sys_time_milli_delay(3);
    }
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_deep_clr_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_DEEP_CLR_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DEEP_CLR_QUERY);
    if (!rval) {
        *p_val = (enum inv_hdmi_deep_clr) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_audio_src_set(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_SRC_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_SRC_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_audio_src_get(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUDIO_SRC_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUDIO_SRC_GET);
    if (!rval)
        *p_val = (enum inv478x_audio_src) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_avmute_auto_clear_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AVMUTE_AUTO_CLEAR_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AVMUTE_AUTO_CLEAR_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_avmute_auto_clear_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AVMUTE_AUTO_CLEAR_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AVMUTE_AUTO_CLEAR_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_pin_swap_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_pin_swap *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_PIN_SWAP_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_PIN_SWAP_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_pin_swap_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_pin_swap *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_PIN_SWAP_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_PIN_SWAP_GET);
    if (!rval) {
        *p_val = (enum inv_hdmi_pin_swap) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hvsync_pol_set(inv_inst_t inst, uint8_t port, enum inv_video_hvsync_pol *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HVSYNC_POL_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HVSYNC_POL_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_hvsync_pol_get(inv_inst_t inst, uint8_t port, enum inv_video_hvsync_pol *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_HVSYNC_POL_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_HVSYNC_POL_GET);
    if (!rval) {
        *p_val = (enum inv_video_hvsync_pol) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_auto_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUTO_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUTO_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_auto_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_AUTO_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AUTO_ENABLE_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_reenable(inv_inst_t inst, uint8_t port)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    s_request_message_init(p_obj, INV478X_TX_REENABLE);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_REENABLE);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_blank_color_set(inv_inst_t inst, uint8_t port, struct inv_video_rgb_value *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_BLANK_COLOR_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->r);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->g);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->b);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_BLANK_COLOR_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_blank_color_get(inv_inst_t inst, uint8_t port, struct inv_video_rgb_value *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_BLANK_COLOR_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_BLANK_COLOR_GET);
    if (!rval) {
        p_val->r = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->g = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->b = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_scdc_rr_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_SCDC_READ_REQ_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_SCDC_READ_REQ_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_scdc_rr_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_SCDC_READ_REQ_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_SCDC_READ_REQ_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_scdc_poll_period_set(inv_inst_t inst, uint8_t port, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_SCDC_POLL_PERIOD_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_SCDC_POLL_PERIOD_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_scdc_poll_period_get(inv_inst_t inst, uint8_t port, uint16_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_SCDC_POLL_PERIOD_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_SCDC_POLL_PERIOD_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_scdc_write(inv_inst_t inst, uint8_t port, uint8_t offset, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_SCDC_WRITE);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_SCDC_WRITE);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_scdc_read(inv_inst_t inst, uint8_t port, uint8_t offset, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;
    printf("inv478x_tx_scdc_read \n");
    rval = MUTEX_API_TAKE (p_obj);
	printf("inv478x_tx_scdc_read22 \n");
    INV_ASSERT (p_val);
	printf("inv478x_tx_scdc_read 333\n");
    s_request_message_init(p_obj, INV478X_TX_SCDC_READ);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_SCDC_READ);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_ddc_write(inv_inst_t inst, uint8_t port, uint8_t slv_addr, uint8_t offset, uint8_t size, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_TX_DDC_WRITE);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, slv_addr);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, size);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_val, size);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DDC_WRITE);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_ddc_read(inv_inst_t inst, uint8_t port, uint8_t slv_addr, uint8_t offset, uint8_t size, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_TX_DDC_READ);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, slv_addr);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, offset);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, size);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DDC_READ);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_val, size);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_ddc_freq_set(inv_inst_t inst, uint8_t port, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_TX_DDC_FREQENCY_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DDC_FREQENCY_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_ddc_freq_get(inv_inst_t inst, uint8_t port, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_TX_DDC_FREQENCY_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_DDC_FREQENCY_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_av_link_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);

    s_request_message_init(p_obj, INV478X_TX_AV_LINK_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_AV_LINK_QUERY);
    if (!rval)
        *p_val = (enum inv_hdmi_av_link) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_tmds2_scramble_query(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_TMDS2_SCRAMBLE_QUERY);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_TMDS2_SCRAMBLE_QUERY);
    if (!rval)
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_video_tim_set(inv_inst_t inst, struct inv_video_timing *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPG_VIDEO_TIMING_SET);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->v_act);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->v_syn);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->v_fnt);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->v_bck);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->h_act);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->h_syn);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->h_fnt);
    inv_drv_ipc_word_push(p_obj->mes_req_inst, p_val->h_bck);
    inv_drv_ipc_long_push(p_obj->mes_req_inst, p_val->pclk);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->v_pol);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->h_pol);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, p_val->intl);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPG_VIDEO_TIMING_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_vtpg_video_tim_get(inv_inst_t inst, struct inv_video_timing *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_VTPG_VIDEO_TIMING_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_VTPG_VIDEO_TIMING_GET);
    if (!rval) {
        p_val->v_act = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->v_syn = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->v_fnt = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->v_bck = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_act = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_syn = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_fnt = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->h_bck = inv_drv_ipc_word_pop(p_obj->mes_ack_inst);
        p_val->pclk = inv_drv_ipc_long_pop(p_obj->mes_ack_inst);
        p_val->v_pol = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->h_pol = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
        p_val->intl = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_blank_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_BLANK_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_bool_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_BLANK_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_blank_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_BLANK_ENABLE_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_BLANK_ENABLE_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_bool_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_packet_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_PACKET_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) len);
    inv_drv_ipc_array_push(p_obj->mes_req_inst, p_packet, len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_PACKET_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_packet_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_PACKET_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) len);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_PACKET_GET);
    if (!rval) {
        inv_drv_ipc_array_pop(p_obj->mes_ack_inst, p_packet, len);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_packet_src_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, enum inv478x_packet_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_PACKET_SRC_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) * p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_PACKET_SRC_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_tx_packet_src_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, enum inv478x_packet_src *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_TX_PACKET_SRC_GET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) port);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, (uint8_t) type);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_TX_PACKET_SRC_GET);
    if (!rval) {
        *p_val = (enum inv478x_packet_src) inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

uint16_t inv_base64_decode(uint8_t *p, uint16_t len, char *s)
{
    uint16_t i = 0;
    uint16_t j = 0;
    uint16_t val = 0;
    bool_t stat = true;

    INV_ASSERT (p);
    INV_ASSERT (s);

    while (stat) {
        val <<= 6;
        if (('A' <= *s) && (*s <= 'Z')) {
            val += (*s - 'A');
        } else if (('a' <= *s) && (*s <= 'z')) {
            val += (*s - 'a' + 26);
        } else if (('0' <= *s) && (*s <= '9')) {
            val += (*s - '0' + (26 * 2));
        } else if ('+' == *s) {
            val += ((26 * 2) + 10);
        } else if ('/' == *s) {
            val += ((26 * 2) + 11);
        } else if ('=' == *s) {
        } else {
        }

        if (1 == (i % 4)) {
            *p = (uint8_t) ((val >> 4) & 0xFF);
            p++;
            j++;
        } else if (2 == (i % 4)) {
            *p = (uint8_t) ((val >> 2) & 0xFF);
            p++;
            j++;
        } else if (3 == (i % 4)) {
            *p = (uint8_t) (val & 0xFF);
            p++;
            j++;
        }
        i++;
        if (j == len)
            break;
        s++;
    }
    return j;
}

inv_rval_t inv478x_gpio_enable_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_ENABLE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_ENABLE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_enable_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_ENABLE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_ENABLE_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_mode_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_MODE_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_MODE_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_mode_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_MODE_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_MODE_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_set_bits(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_BIT_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_BIT_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_bit_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_BIT_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_BIT_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_clear_bits(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_BIT_CLR);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_BIT_CLR);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_query(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_QUERY);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_evt_stat_query(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_EVT_STAT_QUERY);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_EVT_STAT_QUERY);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_evt_mask_set(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_EVT_MASK_SET);
    inv_drv_ipc_byte_push(p_obj->mes_req_inst, *p_val);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_EVT_MASK_SET);

    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

inv_rval_t inv478x_gpio_evt_mask_get(inv_inst_t inst, uint8_t *p_val)
{
    struct obj *p_obj = (struct obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, inst);
    inv_rval_t rval = 0;

    rval = MUTEX_API_TAKE (p_obj);
    INV_ASSERT (p_val);
    s_request_message_init(p_obj, INV478X_GPIO_EVT_MASK_GET);
    s_request_message_send(p_obj);
    rval = s_wait_for_valid_ipc_response_message(p_obj, INV478X_GPIO_EVT_MASK_GET);
    if (!rval) {
        *p_val = inv_drv_ipc_byte_pop(p_obj->mes_ack_inst);
    }
    MUTEX_API_GIVE (p_obj);
    RETURN_API (rval);
}

/***** end of file ***********************************************************/
