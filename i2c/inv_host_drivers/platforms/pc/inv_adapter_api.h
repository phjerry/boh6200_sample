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

#ifndef INV_ADAPTER_API_H
#define INV_ADAPTER_API_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"

typedef void(*connect_cbf) (inv_inst_t inst);

enum inv_adapter_mode {
    INV_ADAPTER_MODE__I2C,
    INV_ADAPTER_MDOE__SPI
};

typedef struct inv_adapter_cfg {
    const char *adapt_id;       /* Adaptor ID string */
    enum inv_adapter_mode mode; /*I2C or SPI */
    uint8_t chip_sel;           /* default ship select */
    //uint8_t                   rst_irq;                    /* rst and irq index*/
} inv_adapter_cfg_t;

/***** local type definitions ************************************************/

char **inv_adapter_list_query(void);   /* Returns a pointer to an array of string pointers with NULL pointer at end of array. Each string pointer indicates an adapter available for use by application. */

/*
 * isr_func     : Is a function pointer to ISR.
 *                Use NULL if Atlanta's IRQ is not connected to adaptor.
 * adapter      : Type of adapter instance.
 * phy_dev_addr : Can either be an index of SPI-select line or an I2C address.
 */
inv_inst_t inv_adapter_create(struct inv_adapter_cfg *cfg, connect_cbf cbf, connect_cbf cbf_interrupt);
inv_inst_t inv_adapter_delete(inv_inst_t inst);

bool_t inv_adapter_connect_query(inv_inst_t inst);

/* The data format of 'chip_sel' is defined per adapter */
/* For I2C mode 'chip_sel' is typically defined as I2C device address */
/* For SPI mode 'chip_sel' defines which GPIO signal is used as 'chip select' signal. */
/* 'chip_sel' value can be changed run-time */
int inv_adapter_chip_sel_set(inv_inst_t inst, uint8_t chip_sel);

int inv_adapter_block_read(inv_inst_t inst, uint16_t offset, uint8_t *buffer, uint16_t count);
int inv_adapter_block_write(inv_inst_t inst, uint16_t offset, const uint8_t *buffer, uint16_t count);

inv_inst_t inv_adapter_chip_reset_set(inv_inst_t inst, bool_t b_on);

bool_t inv_adapter_supspense(char *p_adapter_id, bool_t b_on);

#endif //INV_ADAPTER_API_H

/***** end of file ***********************************************************/
