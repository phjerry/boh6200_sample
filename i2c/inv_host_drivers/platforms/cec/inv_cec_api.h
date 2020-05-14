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
 * @file inv_adaptor_api.h
 *
 * @brief register access APIs
 *
 *****************************************************************************/

#ifndef INV_CEC_API_H
#define INV_CEC_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

#define INV9789_EVENT_EARC_TIMEOUT    0x00000030

#define INV9612_EVENT_FLAG__CLK_CHANGE        0x000000000000001
#define INV9612_EVENT_FLAG__HPD_CHANGE        0x000000000000002
#define INV9612_EVENT_FLAG__EDID_CHANGE        0x000000000000004
#define INV9612_EVENT_FLAG__AUD_CHANGE        0x000000000000008
#define INV9612_EVENT_FLAG__AIF_CHANGE        0x000000000000010
#define INV9612_EVENT_FLAG__ARC_INIT        0x000000000000020
#define INV9612_EVENT_FLAG__ARC_TERM        0x000000000000040

#define   inv_cec_event    uint32_t
typedef void(*inv_cec_call_back) (inv_cec_event event);
typedef bool_t(*inv_cec_adapter_call_back) (uint8_t dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size, uint8_t read_enable);

bool_t inv_cec_adapter_callback(uint8_t dev_id, uint16_t addr, const uint8_t *p_data, uint16_t size, uint8_t read_enable);

inv_inst_t inv_cec_tx_create(uint8_t devid, inv_cec_call_back p_call_back, inv_inst_t adapt_inst);
void inv_cec_tx_delete(void);
void inv_cec_tx_handle(void);
uint32_t inv_tx_cec_arc_rx_start(void);
uint32_t inv_tx_cec_arc_rx_stop(void);

inv_inst_t inv_cec_rx_create(uint8_t devid, inv_cec_call_back p_call_back, inv_inst_t adapt_inst);
void inv_cec_rx_delete(void);
void inv_cec_rx_handle(void);
uint32_t inv_rx_cec_arc_tx_start(void);
uint32_t inv_rx_cec_arc_tx_stop(void);

#endif //INV_CEC_API_H

/***** end of file ***********************************************************/
