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
* @file inv_drv_ipc_api.h
*
* @brief IPC message protocol
*
*****************************************************************************/
#ifndef INV_DRV_IPC_API_H
#define INV_DRV_IPC_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

/***** public macro definitions **********************************************/

#define INV_DRV_IPC_EVENT__MESSAGE_SENT          0x01
#define INV_DRV_IPC_EVENT__MESSAGE_RECEIVED      0x02

#define INV_DRV_IPC_TOKEN__PACKET_RDY            (1<<0)
#define INV_DRV_IPC_TOKEN__ACK                   (1<<1)
#define INV_DRV_IPC_TOKEN__NACK                  (1<<2)
#define INV_DRV_IPC_TOKEN__ALL                   (INV_DRV_IPC_TOKEN__PACKET_RDY | INV_DRV_IPC_TOKEN__ACK | INV_DRV_IPC_TOKEN__NACK)

/***** public type definitions ***********************************************/

#define inv_drv_ipc_inst          inv_inst_t
#define inv_drv_ipc_mes_inst      inv_inst_t
#define inv_drv_ipc_event         uint32_t
#define inv_drv_ipc_op_code       uint16_t
#define inv_drv_ipc_tag_code      uint8_t
#define inv_drv_ipc_token_t       uint8_t

enum inv_drv_ipc_type {
    INV_DRV_IPC_TYPE__NONE,
    INV_DRV_IPC_TYPE__REQUEST,
    INV_DRV_IPC_TYPE__RESPONSE,
    INV_DRV_IPC_TYPE__NOTIFY
};

struct inv_drv_ipc_config {
    inv_platform_dev_id dev_id;
};
/***** call-back functions ********************************************** *****/

void inv_drv_ipc_call_back(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_event event_flags);

/***** public functions ******************************************************/

inv_drv_ipc_inst inv_drv_ipc_create(const char *p_name_str, inv_inst_t parent_inst, struct inv_drv_ipc_config *p_config);
void inv_drv_ipc_delete(inv_drv_ipc_inst ipc_inst);
void inv_drv_ipc_handler(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_token_t token);
void inv_drv_ipc_message_send(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_mes_inst mes_inst);
void inv_drv_ipc_message_receive(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_mes_inst mes_inst);
bool_t inv_drv_ipc_message_sent(inv_drv_ipc_inst ipc_inst);
bool_t inv_drv_ipc_message_received(inv_drv_ipc_inst ipc_inst);
void inv_drv_ipc_tx_reset(inv_drv_ipc_inst ipc_inst);
inv_drv_ipc_mes_inst inv_drv_ipc_message_create(void);
void inv_drv_ipc_message_delete(inv_drv_ipc_mes_inst ipc_inst);
void inv_drv_ipc_message_clear(inv_drv_ipc_mes_inst mes_inst);
void inv_drv_ipc_message_copy(inv_drv_ipc_mes_inst mes_des_inst, inv_drv_ipc_mes_inst mes_src_inst);

void inv_drv_ipc_type_set(inv_drv_ipc_mes_inst mes_inst, enum inv_drv_ipc_type type);
enum inv_drv_ipc_type inv_drv_ipc_type_get(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_op_code_set(inv_drv_ipc_mes_inst mes_inst, inv_drv_ipc_op_code op_code);
inv_drv_ipc_op_code inv_drv_ipc_op_code_get(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_tag_code_set(inv_drv_ipc_mes_inst mes_inst, inv_drv_ipc_tag_code tag_code);
inv_drv_ipc_tag_code inv_drv_ipc_tag_code_get(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_bool_push(inv_drv_ipc_mes_inst mes_inst, bool_t par);
bool_t inv_drv_ipc_bool_pop(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_byte_push(inv_drv_ipc_mes_inst mes_inst, uint8_t par);
uint8_t inv_drv_ipc_byte_pop(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_word_push(inv_drv_ipc_mes_inst mes_inst, uint16_t par);
uint16_t inv_drv_ipc_word_pop(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_long_push(inv_drv_ipc_mes_inst mes_inst, uint32_t par);
uint32_t inv_drv_ipc_long_pop(inv_drv_ipc_mes_inst mes_inst);

void inv_drv_ipc_array_push(inv_drv_ipc_mes_inst mes_inst, const uint8_t *p_par, uint16_t size);
void inv_drv_ipc_array_pop(inv_drv_ipc_mes_inst mes_inst, uint8_t *p_par, uint16_t size);

#endif /* INV_DRV_IPC_API_H */

/***** end of file ***********************************************************/
