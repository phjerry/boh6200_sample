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
/******************************************************************************
* file inv478x_func.h
*
* brief :- Host Driver for inv478x device.
*
*****************************************************************************/
#ifndef INV478X_H
#define INV478X_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "inv_common.h"



/* ISP configuration*/
/*****************************************************************************/
/* macros for the event notification that will be sent to host */
#define INV478X_EVENT_FLAG__TX0_HPD                     ((uint32_t)0x00000001)
#define INV478X_EVENT_FLAG__TX1_HPD                     ((uint32_t)0x00000002)
#define INV478X_EVENT_FLAG__RX0_PLUS5V                  ((uint32_t)0x00000004)
#define INV478X_EVENT_FLAG__RX1_PLUS5V                  ((uint32_t)0x00000008)
#define INV478X_EVENT_FLAG__RX0_AVLINK                  ((uint32_t)0x00000010)
#define INV478X_EVENT_FLAG__RX1_AVLINK                  ((uint32_t)0x00000020)
#define INV478X_EVENT_FLAG__EARC_DISC_TIMEOUT           ((uint32_t)0x00000040)
#define INV478X_EVENT_FLAG__BOOT                        ((uint32_t)0x00000080)
#define INV478X_EVENT_FLAG__EARC_RX_AUDIO               ((uint32_t)0x00000100)
#define INV478X_EVENT_FLAG__EARC_LINK                   ((uint32_t)0x00000200)
#define INV478X_EVENT_FLAG__RX0_HDCP                    ((uint32_t)0x00000400)
#define INV478X_EVENT_FLAG__RX1_HDCP                    ((uint32_t)0x00000800)
#define INV478X_EVENT_FLAG__TX0_RSEN                    ((uint32_t)0x00001000)
#define INV478X_EVENT_FLAG__TX1_RSEN                    ((uint32_t)0x00002000)
#define INV478X_EVENT_FLAG__RX0_AVMUTE                  ((uint32_t)0x00004000)
#define INV478X_EVENT_FLAG__RX1_AVMUTE                  ((uint32_t)0x00008000)
#define INV478X_EVENT_FLAG__TX0_HDCP                    ((uint32_t)0x00010000)
#define INV478X_EVENT_FLAG__TX1_HDCP                    ((uint32_t)0x00020000)
#define INV478X_EVENT_FLAG__RX0_VIDEO                   ((uint32_t)0x00040000)
#define INV478X_EVENT_FLAG__RX1_VIDEO                   ((uint32_t)0x00080000)
#define INV478X_EVENT_FLAG__RX0_AUDIO                   ((uint32_t)0x00100000)
#define INV478X_EVENT_FLAG__RX1_AUDIO                   ((uint32_t)0x00200000)
#define INV478X_EVENT_FLAG__RX0_PACKET                  ((uint32_t)0x00400000)
#define INV478X_EVENT_FLAG__RX1_PACKET                  ((uint32_t)0x00800000)
#define INV478X_EVENT_FLAG__RESERVED_0                  ((uint32_t)0x01000000)
#define INV478X_EVENT_FLAG__RESERVED_1                  ((uint32_t)0x02000000)
#define INV478X_EVENT_FLAG__EARC_ERX_LATENCY_CHANGE     ((uint32_t)0x04000000)
#define INV478X_EVENT_FLAG__EARC_ERX_LATENCY_REQ_CHANGE ((uint32_t)0x08000000)
#define INV478X_EVENT_FLAG__TX0_AVLINK                  ((uint32_t)0x10000000)
#define INV478X_EVENT_FLAG__TX1_AVLINK                  ((uint32_t)0x20000000)
#define INV478X_EVENT_FLAG__GPIO                        ((uint32_t)0x40000000)
#define INV478X_EVENT_FLAG__RESERVED_2                  ((uint32_t)0x80000000)

/*****************************************************************************/
/**
* brief :- Host driver error codes.
*
******************************************************************************/
#define INV_RETVAL__IPC_WRONG_TAG_CODE            ((inv_rval_t)100000)  /* Wrong Tag code received */
#define INV_RETVAL__IPC_WRONG_OP_CODE             ((inv_rval_t)110000)  /* Wrong Tag code received */
#define INV_RETVAL__IPC_NO_RESPONSE               ((inv_rval_t)120000)  /* Wrong Tag code received */

/***** public type definitions ***********************************************/

#define inv478x_event_flags_t uint32_t

/* Error code return value */
#define INV_RVAL__SUCCESS            0x00000000
/***************************************
Parameter error(INV_RVAL__PAR_ERR) is being used for-
1. Null instance error
2. Null pointer error
3. Wrong value entered error
****************************************/
#define INV_RVAL__PAR_ERR            0x00000001
#define INV_RVAL__HOST_ERR           0x00000002
#define INV_RVAL__NOT_IMPL_ERR       0x00000004
#define INV_RVAL__BOARDNOT_CONNECTED 0x00000008
#define INV_RVAL__CHECKSUM_FAILED    0x00000010
#define INV_RVAL__CHECKSUM_PASS      0x00000020
#define INV_RVAL__PROGRAM_FAILED     0x00000040
#define INV_RVAL__ERASE_FAILED       0x00000080

/**
* brief :- Color Specification
*/
enum inv478x_arc_mode {
    INV478X_ARC_MODE__NONE,     /* eARC/ARC-Rx/Tx not supported */
    INV478X_ARC_MODE__RX0,      /* eARC/ARC-Tx supported with Rx0 */
    INV478X_ARC_MODE__TX0,      /* eARC/ARC-Rx supported with Tx0 */
};

enum inv478x_audio_src {
    INV478X_AUDIO_SRC__NONE,    /* No audio */
    INV478X_AUDIO_SRC__EARC,    /* Audio source is eARC Rx */
    INV478X_AUDIO_SRC__RX,      /* this selects either RX0 or RX1 based on ‘inv478x_rx_port_select_set()’ setting */
    INV478X_AUDIO_SRC__RX0,     /* Audio source is HDMI RX0 */
    INV478X_AUDIO_SRC__RX1,     /* Audio source is HDMI RX1 */
    INV478X_AUDIO_SRC__AP0,     /* Audio source is audio port 0 */
    INV478X_AUDIO_SRC__AP1,     /* Audio source is audio port 1 */
    INV478X_AUDIO_SRC__ATPG     /* Audio source is audio pattern generator */
};

/**
* brief :- Configuration structure for driver constructor
*/
struct inv478x_config {
    char p_name_str[8];         /* 8-character UTF-8 null terminated string which is only used for debug logging */
    bool_t b_device_reset;      /* If true, receiver of this data structure will invoke a full device reset */
};

struct inv478x_aif_extract {
    uint8_t ca;
    uint8_t lfe;
    /*
     * The values of attenuation associated with the Level Shift Values
     * * in dB. Refer to "Table 32 Audio InfoFrame Data Byte 5, Level Shift
     * * Value" of CEA-861-G standard
     */
    uint8_t lsv;
    /*
     * Down Mixed Stereo output is permitted or not
     * * Refer to "Table 33 Audio InfoFrame Data Byte 5, Down-mix Inhibit
     * * Flag" of CEA-861-G standard
     */
    bool_t dm_inh;
};

struct inv478x_auto_clr {
    uint8_t freq;
    struct inv_video_rgb_value clr;
};

struct inv478x_rx_eq_cfg {
    uint8_t eq[16];             /* 16 candidates */
    uint8_t ctl;                /* for reg 0x2503; [1:0] power mode; [6:4] for EQ first stage bandwidth control */
};

struct inv478x_rx_equalize {
    struct inv478x_rx_eq_cfg   eq_tmds1;      /* TMDS < 3 Gbps */
    struct inv478x_rx_eq_cfg   eq_tmds2;      /* TMDS 3…6 Gbps */
    struct inv478x_rx_eq_cfg   eq_frl3g;      /* FRL 3G */
    struct inv478x_rx_eq_cfg   eq_frl6g;      /* FRL 6G */
    struct inv478x_rx_eq_cfg   eq_frl8g_a;    /* FRL 8G */
    struct inv478x_rx_eq_cfg   eq_frl8g_b;    /* FRL 8G (backup profile) */
    struct inv478x_rx_eq_cfg   eq_frl10g_a;   /* FRL 10G */
    struct inv478x_rx_eq_cfg   eq_frl10g_b;   /* FRL 10G (backup profile) */
};

enum inv478x_hdcp_type {
    INV478X_HDCP_TYPE__NONE,    /* No HDCP */
    INV478X_HDCP_TYPE__HDCP1,   /* HDCP1.x */
    INV478X_HDCP_TYPE__HDCP2,   /* HDCP2.x */
};

enum inv478x_packet_src {
    INV478X_PACKET_SRC__NONE,   /* No packet insertion */
    INV478X_PACKET_SRC__INSRT,  /* Insertion via ‘inv478x_tx_packet_insrt_set’ */
    INV478X_PACKET_SRC__RX0,    /* Inserted from Rx0 */
    INV478X_PACKET_SRC__RX1,    /* Inserted from Rx1 */
    INV478X_PACKET_SRC__AUTO,   /* Inserted based on 'inv478x_tx_video_src_set' */
};

enum inv478x_video_src {
    INV478X_VIDEO_SRC__NONE,    /* No video source */
    INV478X_VIDEO_SRC__RX,      /* this selects either RX0 or RX1 based on ‘inv478x_rx_port_select_set()’ setting */
    INV478X_VIDEO_SRC__RX0,     /* Video source is RX0 */
    INV478X_VIDEO_SRC__RX1      /* Video source is RX1 */
};

enum inv478x_dl_mode {
    INV478X_DL_MODE__NONE,      /* No Dual Link */
    INV478X_DL_MODE__SPLIT,     /* Split mode */
    INV478X_DL_MODE__MERGE,     /* Merge mode */
    INV478X_DL_MODE__SPLIT_AUTO /* Enable Split based on auto-split character rate. See inv478x_dl_split_auto_set() API */
};

enum inv478x_atpg_ptrn {
    INV478X_ATPG_PTRN__SINE,    /* Audio signal pattern is Sine wave */
    INV478X_ATPG_PTRN__SAWTOOTH,    /* Audio signal pattern is Sawtooth */
    INV478X_ATPG_PTRN__TRIANGLE,    /* Audio signal pattern is Triangle wave */
    INV478X_ATPG_PTRN__BLOCK    /* Audio signal pattern is Block wave */
};

enum inv478x_earc_port_cfg {
    INV478X_EARC_PORT_CFG__NONE,    /* eARC/ARC-Rx/Tx not supported */
    INV478X_EARC_PORT_CFG__TX_RX0,  /* eARC/ARC-Tx configured as Tx in conjunction with Rx0 */
    INV478X_EARC_PORT_CFG__TX_RX1,  /* eARC/ARC-Tx configured as Tx in conjunction with Rx1 */
    INV478X_EARC_PORT_CFG__TX_EXT,  /* eARC/ARC-Tx configured as Tx in conjunction with an external HDMI-Rx */
    INV478X_EARC_PORT_CFG__RX,  /* eARC/ARC-Rx configured as Rx */
};

enum inv478x_ap_port_cfg {
    INV478X_AP_PORT_CFG__NONE,  /* AP not supported */
    INV478X_AP_PORT_CFG__TX,    /* AP configured as Tx */
    INV478X_AP_PORT_CFG__RX,    /* AP configured as Rx */
};




/*****************************************************************************/
/**
* brief Notification callback prototype
*
* This callback is generated from Inv478xHandle() when there is
* any notifications about driver status change.
* Call-back is only generated for the events that have been masked by
* inv478xevent_flags_mask_set().
*
* param[in] inst  Driver instance caused the callback notification
* param[in] flags Bit mask of the events
*
*****************************************************************************/
typedef void(*inv478x_event_callback_func_t) (inv_inst_t inst, inv478x_event_flags_t event_flags);

/***** #include statements ***************************************************/

/***** public functions ******************************************************/

/******************************************************************************
*
* brief :- driver constructor
*
* Allocates resources for Adapter Driver and all child modules.
* This function has to be called before using of any other module function.
* Via this function a callback function can be registered whithat is called
* when any status change event occurs.
*
* input  dev_id        Hardware device instantiation identification number.
*                       Set this to 0 for a single Inv478x. When the system
*                       includes more than one instance of Inv478x.
*                       The user must make sure that each instantiation is
*                       created with a unique dev_id. dev_id is passed into
*                       the platform host interface layer and is used to
*                       select a different I2C device address or a different
*                       SPI chip select signal.
* input  p_callback_func Pointer to a call back function invoked by the driver
*                       when any status change event happens.
*                       If set as NULL, the callback is not invoked.
* input  p_config       Pointer to a static configuration structure containing
*                       static configuration parameters for this driver
*                       instantiation.
*
* return Driver instance handle required by all other functions. If equal to
*         zero then creation of instance has failed.
*
*****************************************************************************/
inv_inst_t inv478x_create(inv_platform_dev_id dev_id, inv478x_event_callback_func_t pCallbackFunc, struct inv478x_config *p_config);

/******************************************************************************
*
* brief :- driver de-constructor
*
* Frees resources allocated by inv478x_create for the driver instance identified
* by inst and shuts down the driver. After this function is called, no other
* module functions can be called for this driver instance until inv478x_create()
* is called again and a new driver handle is received.
*
* input  inst          Driver instance(obtained from inv478x_create).
*                       This is used by Inv478xDelete to identify the driver
*                       instance to delete.
*
*****************************************************************************/
void inv478x_delete(inv_inst_t inst);

/******************************************************************************
*
* brief :- driver handle
*
* This function is expected to be called by the user application  each  time
* the Inv478x chip generates an interrupt. If the interrupt signal(INT) from
* the Inv478x is not connected to application processor, this function must be
* called periodically with an time interval of no more than 100msec.
* This function checks if there are any pending notifications from the Inv478x
* chip, services them and may generate an INV478X_EVENT   ‘notifications’ by
* calling the inv478xeevnt_callback_func_t callback function registered through
* inv478x_create().
*
* input  inst          Driver instance(obtained from inv478x_create).
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_handle(inv_inst_t inst);

/******************************************************************************
*
* brief :- Chip ID status
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Chip ID information.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_chip_id_query(inv_inst_t inst, uint32_t *p_val);

/******************************************************************************
*
* brief :- Chip Revision status
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Chip revision number.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_chip_revision_query(inv_inst_t inst, uint32_t *p_val);

/******************************************************************************
*
* brief :- Firmware Version status
*
* Updates the firmware version string. The maximum string length is 64
* characters. String is terminated by null character if string is less than
* 64 characters.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Pointer to an array of characters in which the
*                       firmware version string is returned.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_firmware_version_query(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_firmware_user_tag_query(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
*
* brief :- To control heart beat LED
*
* This API can be used to make heart beat LED switched off or on permanently.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Pointer to a bool type variable with true/false value
*                      TRUE  : LED is on
*                      FALSE : LED is off
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_heart_beat_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_heart_beat_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- Event flag mask value set
*
* Updates the event flag mask value
*
* input  inst          Driver instance(obtained from inv478x_create).
* input p_flags        Pointer to a 32 bit flag value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_event_flags_mask_set(inv_inst_t inst, uint32_t *p_flags);
inv_rval_t inv478x_event_flags_mask_get(inv_inst_t inst, uint32_t *p_flags);

/******************************************************************************
*
* brief :- Freeze and unfreeze the on-chip firmware. When freezed, on-chip
* firmware will not process any of the interrupts. Host driver can still
* call the APIs.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input p_val          Pointer to a bool type value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_freeze_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_freeze_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- Gets the on-chip MCU boot status.
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to the type of inv_boot_stat
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_boot_stat_query(inv_inst_t inst, enum inv_sys_boot_stat *p_val);

/******************************************************************************
*
* brief :- Event flag status query
*
* This API can be used to query the latest event status
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to the 32-bit integer value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_event_flags_stat_query(inv_inst_t inst, uint32_t *p_val);

/******************************************************************************
*
* brief :- Event flag status clear
*
* This API can be used to clear the latest event status
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to the 32-bit integer value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_event_flag_stat_clear(inv_inst_t inst, uint32_t *p_val);

/******************************************************************************
*
* brief :- Reboots the on-chip firmware.
*
* input  inst          Driver instance(obtained from inv478x_create).
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_reboot_req(inv_inst_t inst);

/******************************************************************************
*
* brief :- Enable/disable left/right swap
*
* input  inst          Driver instance(obtained from inv478x_create).
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_dl_swap_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_dl_swap_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- HDCP2.3 Device ID Query
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Device ID Value
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_chip_serial_query(inv_inst_t inst, uint32_t *p_val);

/******************************************************************************
*
* brief :- HDCP2.3 License Cert Set
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         License Value
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_license_cert_set(inv_inst_t inst, char *p_val);

/******************************************************************************
*
* brief :- HDCP2.3 License ID get
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Cert ID
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_license_id_query(inv_inst_t inst, uint32_t *p_val);

/******************************************************************************
*
* brief :- Prepare for PWD power down.
*
* input  inst          Driver instance(obtained from inv478x_create).
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_pwr_down_req(inv_inst_t inst);

/******************************************************************************
*
* brief :- Erase Atlanta Code
*
* input  inst          Driver instance(obtained from inv478x_create).
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
                        INV_RVAL_ERASE_FAILED      0x00000008
*
*****************************************************************************/
inv_rval_t inv478x_flash_erase(inv_inst_t inst);

/******************************************************************************
*
* brief :- Full Erase Atlanta Code
*
* input  inst          Driver instance(obtained from inv478x_create).
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
                        INV_RVAL_ERASE_FAILED      0x00000008
*
*****************************************************************************/
inv_rval_t inv478x_flash_full_erase(inv_inst_t inst);

/******************************************************************************
*
* brief :- Erase Atlanta Code
*
* input  inst          Driver instance(obtained from inv478x_create).
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_flash_program(inv_inst_t inst, uint8_t *p_val, uint32_t len);

/******************************************************************************
*
* brief :- Configures Dual-Link mode: NONE, SPLIT, MERGE, SPLIT_AUTO and get.
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to enum type
*
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_dl_mode_set(inv_inst_t inst, enum inv478x_dl_mode *p_val);
inv_rval_t inv478x_dl_mode_get(inv_inst_t inst, enum inv478x_dl_mode *p_val);

/******************************************************************************
*
* brief :- Number of overlapping pixels Set/Get
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to 8-bit Unsigned Char
*
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_dl_split_overlap_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_dl_split_overlap_get(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
*
* brief :- Enable/Disable the Big Endian Format For KSV List
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to bool type
*
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_hdcp_ksv_big_endian_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_hdcp_ksv_big_endian_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- Write/Read To a Given Register Address
*
* input  inst          Driver instance(obtained from inv478x_create).
* output p_val         Pointer to bool type
*
* retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                       INV_RVAL__PAR_ERR          0x00000001
*                       INV_RVAL__HOST_ERR         0x00000002
*                       INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_reg_write(inv_inst_t inst, uint16_t addr, uint8_t size, uint8_t *p_val);
inv_rval_t inv478x_reg_read(inv_inst_t inst, uint16_t addr, uint8_t size, uint8_t *p_val);



/****************************************************************************/
/**** EARC Management
/****************************************************************************/

/******************************************************************************
*
* brief :- eARC Tx Audio Source Control
*
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Audio Source enumerator:
*                       inv478x_AUDIO_SRC__NONE    No Audio Source
*                       inv478x_AUDIO_SRC__EARC    INVALID
*                       inv478x_AUDIO_SRC__RX0     Audio Source is RX0
*                       inv478x_AUDIO_SRC__RX1     Audio Source is RX1
*                       inv478x_AUDIO_SRC__AP0     Audio Source is Audio Port 0
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_tx_src_set(inv_inst_t inst, enum inv478x_audio_src *p_val);
inv_rval_t inv478x_earc_tx_src_get(inv_inst_t inst, enum inv478x_audio_src *p_val);

/******************************************************************************
*
* brief :- eARC Audio Format status
* Configuration of incoming audio stream format: NONE, ASP2, ASP8, HBRA, DSD6.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Audio format enumerator:
*                       inv478x_AUDIO_FMT__NONE    no audio
*                       inv478x_AUDIO_FMT__ASP2    2 channel layout
*                       inv478x_AUDIO_FMT__ASP8    8 channel layout
*                       inv478x_AUDIO_FMT__ASP16   16 channel layout
*                       inv478x_AUDIO_FMT__ASP32   32 channel layout
*                       inv478x_AUDIO_FMT__HBRA    High bit rate audio
*                       inv478x_AUDIO_FMT__DSD6    6 channel DSD
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_fmt_query(inv_inst_t inst, enum inv_audio_fmt *p_val);

/******************************************************************************
*
* brief :- eARC Audio Information status
* Configuration of incoming audio stream information(Channel Status, Channel Map).
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Audio info object
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_info_query(inv_inst_t inst, struct inv_audio_info *p_val);

/******************************************************************************
*
* brief :- eARC Link status
* Indicates whether eARC link is established.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         true : link is estableshed
*                       false: link is not established
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_link_query(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- eARC ERX latency control
* Configures audio latency.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Latency in milliseconds
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_erx_latency_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_earc_erx_latency_get(inv_inst_t inst, uint8_t *p_val);


/******************************************************************************
*
* brief :- eARC ERX latency status
* Returns adjusted latency.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Latency in milliseconds
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_erx_latency_query(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
*
* brief :- eARC-Rx Capability Data Structure control
* Configure Rx capability data structure. Note: this API is only functional in eARC-Rx mode.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  offset        Data offset(0..127)
* input  len           Length of data transaction minus 1(0..127)
* object p_val         Array of bytes
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_rxcap_struct_set(inv_inst_t inst, uint8_t offset, uint8_t len, uint8_t *p_val);
inv_rval_t inv478x_earc_rxcap_struct_get(inv_inst_t inst, uint8_t offset, uint8_t len, uint8_t *p_val);

/******************************************************************************
*
* brief :- eARC-Tx Capability Data Structure status
* Return Rx capability data structure. Note: this API is only functional in eARC-Tx mode.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  offset        Data offset(0..127)
* input  len           Length of data transaction minus 1(0..127)
* object p_val         Array of bytes
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_rxcap_struct_query(inv_inst_t inst, uint8_t offset, uint8_t len, uint8_t *p_val);

/******************************************************************************
*
* brief :- eARC-Tx Mute control
* Mute or Unmute the audio at the eARC Tx output.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object mute_val      true=enable, false=disable
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_aud_mute_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_earc_aud_mute_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- Configures content of info packet to transmit. Note: It is functional only in
* earc Tx mode for AUD, ISRC1, ISRC2 packet types only.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  type          Packet Type
* output p_packet      Pointer to Structure of Packet Type
*
* retval inv_rval_t    INV_RVAL__SUCCESS          0x00000000
*                      INV_RVAL__PAR_ERR          0x00000001
*                      INV_RVAL__HOST_ERR         0x00000002
*                      INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_packet_set(inv_inst_t inst, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet);
inv_rval_t inv478x_earc_packet_get(inv_inst_t inst, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet);

/******************************************************************************
*
* brief :- Determines the content of info packet received. Note: It is functional only in
* earc Rx mode for AUD, ISRC1, ISRC2 packet types only.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  type          Packet Type
* output p_packet      Pointer to Structure of Packet Type
*
* retval inv_rval_t    INV_RVAL__SUCCESS          0x00000000
*                      INV_RVAL__PAR_ERR          0x00000001
*                      INV_RVAL__HOST_ERR         0x00000002
*                      INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_packet_query(inv_inst_t inst, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet);

/******************************************************************************
*
* brief :- Configuration of eARC/ARC port(NONE, TX_RX0, TX_RX1, TX_EXT, RX).
*
* input  inst          Driver instance(obtained from inv478x_create).
* in/out p_packet      Pointer to Enum for EARC Port Config
*
* retval inv_rval_t    INV_RVAL__SUCCESS          0x00000000
*                      INV_RVAL__PAR_ERR          0x00000001
*                      INV_RVAL__HOST_ERR         0x00000002
*                      INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_earc_cfg_set(inv_inst_t inst, enum inv478x_earc_port_cfg *p_val);
inv_rval_t inv478x_earc_cfg_get(inv_inst_t inst, enum inv478x_earc_port_cfg *p_val);

/****************************************************************************/
/**** Audio Port Management
/****************************************************************************/

/******************************************************************************
*
* brief :- Audio Port source control
* Selects audio source for AP0 and AP1 output port(NONE, RX, RX0, RX1, ARC).
* If ‘NONE’ AP is configured as an input.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          Audio port(0 or 1)
* object p_val         Enumerator:
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_ap_tx_src_set(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val);
inv_rval_t inv478x_ap_tx_src_get(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val);

/******************************************************************************
*
* brief :- Audio Port Mode control
* Configures interface mode of AP when used as an input(NONE, I2S, SPDIF, TDM).
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          Audio port(0 or 1)
* input  p_val         Audio port mode enumerator:
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_ap_mode_set(inv_inst_t inst, uint8_t port, enum inv_audio_ap_mode *p_val);
inv_rval_t inv478x_ap_mode_get(inv_inst_t inst, uint8_t port, enum inv_audio_ap_mode *p_val);

/****************************************************************************/
/**
* brief :- Selects ARC mode(NONE, ARC, EARC). Note: This API is only functional in
* eARC-Tx mode.
*
*
* param[in]  inst     Driver instance returned by inv478x_create()
* param[out] p_val
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_earc_mode_set(inv_inst_t inst, enum inv_hdmi_earc_mode *p_val);
inv_rval_t inv478x_earc_mode_get(inv_inst_t inst, enum inv_hdmi_earc_mode *p_val);

/****************************************************************************/
/**
* brief :- Configures mode of MCLK output signal(NONE, FS128, FS256, FS384, FS512)
*
* param[in]  inst     Driver instance returned by inv478x_create()
* input port          Audio Port Value
* param[out] p_val
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_mclk_set(inv_inst_t inst, uint8_t port, enum inv_audio_mclk *p_val);
inv_rval_t inv478x_ap_mclk_get(inv_inst_t inst, uint8_t port, enum inv_audio_mclk *p_val);

/****************************************************************************/
/**
* brief :- Select audio format: NONE, ASP2, ASP8, ASP16
*
* param[in]  inst         Driver instance returned by inv478x_create()
* param[in/out] p_val     Pointer to Enum Type
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_atpg_fmt_set(inv_inst_t inst, enum inv_audio_fmt *p_val);
inv_rval_t inv478x_atpg_fmt_get(inv_inst_t inst, enum inv_audio_fmt *p_val);

/****************************************************************************/
/**
* brief :- Audio Amplitude Set/Get
*
* param[in]  inst         Driver instance returned by inv478x_create()
* param[in/out] p_val     Pointer 16-bit Unsigned Int Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_atpg_ampl_set(inv_inst_t inst, uint16_t *p_val);
inv_rval_t inv478x_atpg_ampl_get(inv_inst_t inst, uint16_t *p_val);

/****************************************************************************/
/**
* brief :- Audio Frequency(40 to 10000Hz) Set/Get
*
* param[in]  inst         Driver instance returned by inv478x_create()
* param[in/out] p_val     Pointer 16-bit Unsigned Int Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_atpg_freq_set(inv_inst_t inst, uint16_t *p_val);
inv_rval_t inv478x_atpg_freq_get(inv_inst_t inst, uint16_t *p_val);

/****************************************************************************/
/**
* brief :- Audio pattern: SINE, SAWTOOTH, TRIANGLE, BLOCK Set/Get
*
* param[in]  inst         Driver instance returned by inv478x_create()
* param[in/out] p_val     Pointer Enum Type Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_atpg_ptrn_set(inv_inst_t inst, enum inv478x_atpg_ptrn *p_val);
inv_rval_t inv478x_atpg_ptrn_get(inv_inst_t inst, enum inv478x_atpg_ptrn *p_val);

/****************************************************************************/
/**
* brief :- Set APG sample frequency
* Configures the sampling frequency of audio generated by APG.
*
* param[in]  inst         Driver instance returned by inv478x_create()
* param[in/out] p_val     Pointer Enum Type Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_atpg_fs_set(inv_inst_t inst, enum inv_audio_fs *p_val);
inv_rval_t inv478x_atpg_fs_get(inv_inst_t inst, enum inv_audio_fs *p_val);

/****************************************************************************/
/**
* brief :- Configuration of audio insertion/extraction port(NONE, TX, RX)
*
* param[in]  inst         Driver instance returned by inv478x_create()
* param[in/out] p_val     Pointer Enum Type Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_cfg_set(inv_inst_t inst, uint8_t port, enum inv478x_ap_port_cfg *p_val);
inv_rval_t inv478x_ap_cfg_get(inv_inst_t inst, uint8_t port, enum inv478x_ap_port_cfg *p_val);

/****************************************************************************/
/**** A/V Up-Stream Management
/****************************************************************************/

/******************************************************************************
*
* brief :- Active Input Select control
* Forces both outputs(Tx0 and Tx1) to switch to selected input(Rx0 or Rx1).
* Note: this config function is not supported with a get function. The current
* selected input can be queried using ‘inv478x_tx_av_src_select_set()’.
*
* Controls which Rx input is used for processing.
* Only one input at the time can be selected as the active input.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         HDMI Input port(0 or 1)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_port_select_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_rx_port_select_get(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
*
* brief :- Fast Switching control
* Fast switching mode enable/disable control. When disabled
* ‘inv478x_rx_port_select_set()’ briefly toggles HPD of new selected input port.
* Enable product wide fast switching option
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         true=enable, false=disable
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_fast_switch_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_rx_fast_switch_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx Plus 5V (RPWR) status
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         true=high, false=low
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_plus5v_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx HPD force low control
* Controls the state of the HPD signal on the specified input port.
* Note: Unless EDID modification can be achieved by a single call to
* ‘inv478x_rx_edid_set()’ it is recommended to first force HPD signal low before
* calling ‘inv478x_rx_edid_set()’.

* Controls the desired state of HPD signal of each input port.
* Note: After HPD is requested to go low the Inv478x device will not allow
* the HPD to go high for at least 500msec. even if user application requests
* HPD to go back high within 500msec.
* Note: Inv478x device also forces HPD signal to go low for at least 500msec
* when user application calls inv478x_rx_edid_set() or inv478x_rx_port_select_set().
* It is therefore recommended for the user to call this function first
* to force HPD to go low before requesting(multiple) EDID modifications
* followed by this function call to request HPD become high active again.
* The system makes sure that HPD low time is at least 500ms
* (the standard requires at least 100ms).
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         true  : Allow(release) Hot-Plug signal to go high.
*                       false : Forces Hot-Plug signal to go low.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hpd_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_hpd_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx Physical HPD level status
* Queries the physical level of HPD line.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         true=high, false=low
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hpd_phy_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx Minimum HPD low time control
* Controls the duration of HPD level to be forced low.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* input  p_val         Time in miliseconds(100..3000)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hpd_low_time_set(inv_inst_t inst, uint8_t port, uint16_t *p_val);
inv_rval_t inv478x_rx_hpd_low_time_get(inv_inst_t inst, uint8_t port, uint16_t *p_val);

/******************************************************************************
*
* brief :- Rx Termination enable/disable control
* Controls the termination of Rx RSEN.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         true=enable, false=disable
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_rsen_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_rsen_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx EDID enable/disable control
* Controls whether upstream device is able to read EDID.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         true=enable, false=disable
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_edid_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_edid_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx EDID control
* The set function allows the internal EDID to be updated by the user application.
* The get function allows the previously written internal EDID to be read back.
* Note: Check-sum byte of each EDID block(last byte of block) is not required to
* be configured since it is automatically recalculated after EDID data is modified.
* Note: HPD signal may be briefly forced low when EDID data is changed
*
* EDID configuration control. EDID configuration request is acknowledged
* only if EDID replication is disabled.
* Note: Check-sum is recalculated after EDID data modification.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* input  block         Block number index(0,1,2 or 3)
* input  offset        EDID data offset(0..127)
* input  len           Length of data transaction minus 1(0..127)
* object p_data         Array of bytes.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_edid_set(inv_inst_t inst, uint8_t port, uint8_t block, uint8_t offset, uint8_t len, uint8_t *p_val);
inv_rval_t inv478x_rx_edid_get(inv_inst_t inst, uint8_t port, uint8_t block, uint8_t offset, uint8_t len, uint8_t *p_val);

/******************************************************************************
*
* brief :- Rx Deep Color status
* Query Rx deep color mode.
* Note: Always returns ‘INV478X_DEEP_CLR__NONE’ when input color space is YCbCr 4:2:2
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Enumerator:
*                       inv478x_DEEP_CLR__NONE
*                       inv478x_DEEP_CLR__30BIT
*                       inv478x_DEEP_CLR__36BIT
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_deep_clr_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val);

/******************************************************************************
*
* brief :- Rx Video Timing status
* Updates the video timings for the input port given as an argument.
* Returns the current Rx Video timing.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Pointer to video timing data structure.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_video_tim_query(inv_inst_t inst, uint8_t port, struct inv_video_timing *p_val);

/******************************************************************************
*
* brief :- Rx AV-Link status
* Updates the current Rx AV Link status.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Enumerator indicates any of the following states:
*                       inv478x_AV_LINK__NONE    No video
*                       inv478x_AV_LINK__DVI     DVI
*                       inv478x_AV_LINK__TMDS1   HDMI1.x
*                       inv478x_AV_LINK__TMDS2   HDMI2.x
*                       inv478x_AV_LINK__FRL4X6  HDMI2.1
*                       inv478x_AV_LINK__FRL4X8  HDMI2.1
*                       inv478x_AV_LINK__FRL4X10 HDMI2.1
*                       inv478x_AV_LINK__UNKNOWN HDMI2.1
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_av_link_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val);

/******************************************************************************
*
* brief :- Returns pixel format information(colorimetry/chroma down sampling/bit depth)
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Structure object Pointer

*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_pixel_fmt_query(inv_inst_t inst, uint8_t port, struct inv_video_pixel_fmt *p_val);

/******************************************************************************
*
* brief :- Returns pixel format information(colorimetry/chroma down sampling/bit depth)
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         VIC_ID of Video fmt.(0 - Non stable video, 0xFFFF  - DVI input)

*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_video_fmt_query(inv_inst_t inst, uint8_t port, uint16_t *p_val);

/******************************************************************************
*
* brief :- Rx AVMUTE status
* Queries incoming avmute status
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         true=active, false=inactive
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_avmute_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Rx Audio Format status
* Queries audio format that HDMI-Rx port is receiving.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Audio format enumerator:
*                       inv478x_AUDIO_FMT__NONE    no audio
*                       inv478x_AUDIO_FMT__ASP2    2 channel layout
*                       inv478x_AUDIO_FMT__ASP8    8 channel layout
*                       inv478x_AUDIO_FMT__ASP16   16 channel layout
*                       inv478x_AUDIO_FMT__ASP32   32 channel layout
*                       inv478x_AUDIO_FMT__HBRA    High bit rate audio
*                       inv478x_AUDIO_FMT__DSD6    6 channel DSD
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_audio_fmt_query(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val);

/******************************************************************************
*
* brief :- Rx Audio Information status
* Queries Audio META data of incoming audio stream on HDMI-Rx.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Audio info object
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_audio_info_query(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val);

/******************************************************************************
*
* brief :- Rx Audio CTS/N status
* Queries CTS/N values as received by HDMI-Rx.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         CTS/N value object
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_audio_ctsn_query(inv_inst_t inst, uint8_t port, struct inv_hdmi_audio_ctsn *p_val);

/******************************************************************************
*
* brief :- Rx HDCP1.4 BKSV status
* Queries the local BKSV (HDCP1.4)
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         BKSV value(40 bit)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_bksv_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val);

/******************************************************************************
*
* brief :- Rx HDCP2.2 RxID status
* Queries the local RxID (HDCP2.3).
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         RxID value(40 bit)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_rxid_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val);

/******************************************************************************
*
* brief :- Rx HDCP1.4 AKSV status
* Queries the latest received AKSV from upstream device(HDCP1.4).
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         AKSV value(40 bit)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_aksv_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val);

/******************************************************************************
*
* brief :- Rx HDCP status
* Queries the HDCP receiver state.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         HDCP RX status value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_stat_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_stat *p_val);

/******************************************************************************
*
* brief :- Rx AV-Link status
* Selects the type of Rx link that should be established with the given port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Enumerator:
*                       inv478x_AV_LINK__NONE    No video
*                       inv478x_AV_LINK__DVI     DVI
*                       inv478x_AV_LINK__TMDS1   HDMI1.x
*                       inv478x_AV_LINK__TMDS2   HDMI2.x
*                       inv478x_AV_LINK__FRL4X6  HDMI2.1
*                       inv478x_AV_LINK__FRL4X8  HDMI2.1
*                       inv478x_AV_LINK__FRL4X10 HDMI2.1
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_av_link_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val);
inv_rval_t inv478x_rx_av_link_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val);

/******************************************************************************
*
* brief :- Rx HDCP status
* Queries the HDCP receiver state.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         HDCP RX status value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_ext_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_hdcp_ext_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_hdcp_ext_topology_set(inv_inst_t inst, uint8_t port, struct inv_hdcp_top *p_val);
inv_rval_t inv478x_rx_hdcp_ext_ksvlist_set(inv_inst_t inst, uint8_t port, uint8_t len, struct inv_hdcp_ksv *p_val);
inv_rval_t inv478x_tx_hdcp_start(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_hdcp_encryption_status_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Auto-split character rate
*
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         The TMDS rate value in MHz
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_dl_split_auto_set(inv_inst_t inst, uint16_t *p_val);
inv_rval_t inv478x_dl_split_auto_get(inv_inst_t inst, uint16_t *p_val);

/****************************************************************************/
/**** A/V Down-Stream Management
/****************************************************************************/

/******************************************************************************
*
* brief :- Tx Deep Color control.
* Configures the highest supported deep color mode. E.g. if set to 10bit then
* transmitter will use 10bit even when the upstream video has a bit depth of 12bit.
*
* Controls the maximum video pixel bit depth output by Tx port. With this control
* the user can force the output to use a lower deep color mode than is provided
* on active input port. For instance, when user selects ‘INV478X_BIT_DEPTH__10’
* while upstream device provides a 12-bit deep color mode, the output video bit depth
* is truncated to 10bit(Deep color mode TMDS frequency = pixel frequency x 1.25).
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Enumerator:
*                       inv478x_DEEP_CLR__NONE
*                       inv478x_DEEP_CLR__30BIT
*                       inv478x_DEEP_CLR__36BIT
*                       inv478x_DEEP_CLR__PASSTHRU
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_deep_clr_max_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val);
inv_rval_t inv478x_tx_deep_clr_max_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val);

/******************************************************************************
*
* brief :- Rx AV-Link status
* Selects the type of Tx link that should be established with the given port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Enumerator:
*                       inv478x_AV_LINK__NONE    No video
*                       inv478x_AV_LINK__DVI     DVI
*                       inv478x_AV_LINK__TMDS1   HDMI1.x
*                       inv478x_AV_LINK__TMDS2   HDMI2.x
*                       inv478x_AV_LINK__FRL4X6  HDMI2.1
*                       inv478x_AV_LINK__FRL4X8  HDMI2.1
*                       inv478x_AV_LINK__FRL4X10 HDMI2.1
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_av_link_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val);
inv_rval_t inv478x_tx_av_link_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val);

/******************************************************************************
*
* brief :- Tx Downstream Termination status
* Queries Tx Downstream Termination status
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         true=enabled, false=disabled
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_rsen_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Tx HPD level status
* Queries the HPD level of the output port defined by port.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         true  : Current HPD level is high
*                       false : Current HPD level is low
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hpd_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Tx Downstream EDID status.
* Queries an array of bytes with EDID data that was read from selected downstream port.
* Note: this EDID remains unchanged when HPD becomes low.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* input  block         Block number index(0,1,2 or 3)
* input  offset        EDID data offset(0..127)
* input  len           Length of data transaction minus 1(0..127)
* object p_data         Array of bytes.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_edid_query(inv_inst_t inst, uint8_t port, uint8_t block, uint8_t offset, uint8_t len, uint8_t *p_val);

/******************************************************************************
*
* brief :- Tx Downstream EDID change status
* Queries the downstream EDID change status. The status indicates in which EDID
* block the data was changed from previous EDID read.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Bit0 : if '1' Block 0 was changed
*                      Bit1 : if '1' Block 1 was changed
*                      Bit2 : if '1' Block 2 was changed
*                      Bit3 : if '1' Block 3 was changed
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_edid_stat_query(inv_inst_t inst, uint8_t port, uint8_t *p_val);

/******************************************************************************
*
* brief :- Tx Audio Format status
* Returns audio packet mode: NONE, ASP2, ASP8, HBR, DSD6.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Audio format enumerator:
*                       inv478x_AUDIO_FMT__NONE    no audio
*                       inv478x_AUDIO_FMT__ASP2    2 channel layout
*                       inv478x_AUDIO_FMT__ASP8    8 channel layout
*                       inv478x_AUDIO_FMT__ASP16   16 channel layout
*                       inv478x_AUDIO_FMT__ASP32   32 channel layout
*                       inv478x_AUDIO_FMT__HBRA    High bit rate audio
*                       inv478x_AUDIO_FMT__DSD6    6 channel DSD
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_audio_fmt_query(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val);

/******************************************************************************
*
* brief :- Tx Audio Information status
* Queries Audio META data of expected outgoing audio stream on HDMI-Tx.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Audio info object
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_audio_info_query(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val);

/******************************************************************************
*
* brief :- Tx Audio Information status
* Selects video source: RX, RX0, RX1, TPG. Note: 'RX' must be selected in order
* to support video split.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Audio info object
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_video_src_set(inv_inst_t inst, uint8_t port, enum inv478x_video_src *p_val);
inv_rval_t inv478x_tx_video_src_get(inv_inst_t inst, uint8_t port, enum inv478x_video_src *p_val);

/******************************************************************************
*
* brief :- EDID Replication control
* Enable/Disable EDID replication either from Tx0 or Tx1(applies to all inputs)
* Options: NONE, Tx0, Tx1.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         INV_HDMI_EDID_REPL_NONE      0X00000000
*                      INV_HDMI_EDID_REPL_TX0       0x00000001
*                      INV_HDMI_EDID_REPL_TX1       0x00000002
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_edid_replicate_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_edid_repl *p_val);
inv_rval_t inv478x_rx_edid_replicate_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_edid_repl *p_val);

/******************************************************************************
*
* brief :- HPD Replication control
* Replicates HPD upstream either by edge or level: NONE, LEVEL, EDGE
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Enumerator:
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hpd_replicate_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_hpd_repl *p_val);
inv_rval_t inv478x_rx_hpd_replicate_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_hpd_repl *p_val);

/******************************************************************************
*
* brief :- Query for TMDS Div 1OVER4 Ratio
* Queries TMDS Bit Clock Ratio of expected outgoing audio stream on HDMI-Tx in HDMI2.0 mode.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         To Know whether TMDS 1OVER4 Div is enabled or disabled
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_tmds2_1over4_query(inv_inst_t inst, uint8_t port, uint8_t *p_val);


/*****************************************************************************/
/***** TPG Management */
/*****************************************************************************/

/******************************************************************************
*
* brief :- Video Test Pattern Enable/disable control
* Enables/disables the internal test pattern generator.
*
* User control to enable / disable video test pattern generator.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         true=enable, false=disable
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_vtpg_enable_set(inv_inst_t inst, bool_t *p_val);
inv_rval_t inv478x_vtpg_enable_get(inv_inst_t inst, bool_t *p_val);

/******************************************************************************
*
* brief :- Video Test Pattern format control
* Selects the TPG output video resolution.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_vid_res       Video resolution enumerator(see ‘INV478XVidRes _t’
*                       for all supported formats)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_vtpg_video_fmt_set(inv_inst_t inst, uint16_t *p_val);
inv_rval_t inv478x_vtpg_video_fmt_get(inv_inst_t inst, uint16_t *p_val);

/******************************************************************************
*
* brief :- Video Test Pattern selection control
* Selects TPG pattern.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         Video pattern enumerator:
*                      INV_VTPG_PTRN__RED
*                      INV_VTPG_PTRN__GREEN
*                      INV_VTPG_PTRN__BLUE
*                      INV_VTPG_PTRN__CYAN
*                      INV_VTPG_PTRN__MAGENTA
*                      INV_VTPG_PTRN__YELLOW
*                      INV_VTPG_PTRN__BLACK
*                      INV_VTPG_PTRN__WHITE
*                      INV_VTPG_PTRN__GRAYSCALE
*                      INV_VTPG_PTRN__CHKRBRD
*                      INV_VTPG_PTRN__COLORBAR
*                      INV_VTPG_PTRN__RESERVED
*
* retval inv_rval_t    INV_RVAL__SUCCESS          0x00000000
*                      INV_RVAL__PAR_ERR          0x00000001
*                      INV_RVAL__HOST_ERR         0x00000002
*                      INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_vtpg_ptrn_set(inv_inst_t inst, enum inv_vtpg_ptrn *p_val);
inv_rval_t inv478x_vtpg_ptrn_get(inv_inst_t inst, enum inv_vtpg_ptrn *p_val);



/******************************************************************************

/****************************************************************************/
/**
* brief :- Controls the audio format of the specified Audio Port.
* Configuration of incoming audio stream format: NONE, ASP2, ASP8, HBRA, DSD6.
*
* param[in]  inst     Driver instance returned by inv478x_create()
* param[out] p_val    INV478X_AUDIO_FMT__NONE    : no audio / mute
*                      INV478X_AUDIO_FMT__ASP2    : 2 channel layout
*                      INV478X_AUDIO_FMT__ASP8    : 8 channel layout
*                      INV478X_AUDIO_FMT__ASP16   : 16 channel layout
*                      INV478X_AUDIO_FMT__ASP32   : 32 channel layout
*                      INV478X_AUDIO_FMT__HBRA    : High bit rate audio
*                      INV478X_AUDIO_FMT__DSD6    : 6 channel DSD
*
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_rx_fmt_set(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val);
inv_rval_t inv478x_ap_rx_fmt_get(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val);

/****************************************************************************/
/**
* brief :- Configures the audio info of the specified Audio Port.
* Configuration of incoming audio stream information(Channel Status, Channel Map).
* In case of S/PDIF channel status information is extracted directly from audio stream.
*
* param[in]  inst     Driver instance returned by inv478x_create()
* param[out] p_val    Pointer to audio info structure.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_rx_info_set(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val);
inv_rval_t inv478x_ap_rx_info_get(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val);

/****************************************************************************/
/**
* brief :- Queries the audio format of specified audio port.
* Configuration of incoming audio stream format: NONE, ASP2, ASP8, HBRA, DSD6.
*
* param[in]  inst     Driver instance returned by inv478x_create()
* param[out] p_val    Pointer to audio format.
*                      INV478X_AUDIO_FMT__NONE    : no audio / mute
*                      INV478X_AUDIO_FMT__ASP2    : 2 channel layout
*                      INV478X_AUDIO_FMT__ASP8    : 8 channel layout
*                      INV478X_AUDIO_FMT__ASP16   : 16 channel layout
*                      INV478X_AUDIO_FMT__ASP32   : 32 channel layout
*                      INV478X_AUDIO_FMT__HBRA    : High bit rate audio
*                      INV478X_AUDIO_FMT__DSD6    : 6 channel DSD
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_tx_fmt_query(inv_inst_t inst, uint8_t port, enum inv_audio_fmt *p_val);

/****************************************************************************/
/**
* brief :- Queries the audio info of the specified audio port.
* Configuration of incoming audio stream information(Channel Status, Channel Map).
* In case of S/PDIF channel status information is extracted directly from audio stream.
*
* param[in]  inst     Driver instance returned by inv478x_create()
* param[out] p_val    Audio Port:
*                      0: AP0
*                      1: AP1
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_tx_info_query(inv_inst_t inst, uint8_t port, struct inv_audio_info *p_val);

/****************************************************************************/
/**
* brief :- Mute and Unmute the audio at the specified Audio Port.
* Turns on/off audio clock when configured as output.
*
* param[in]  inst     Driver instance returned by inv478x_create()
* param[out] p_val    Audio Port:
*                      0: AP0
*                      1: AP1
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
******************************************************************************/
inv_rval_t inv478x_ap_tx_mute_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_ap_tx_mute_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Gets the SCDC register value at the requested offset for the specified
* Rx port as received by HDMI-Rx.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* input  offset        SCDC register offset value.
* object p_val         HDCP RX status value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_scdc_register_query(inv_inst_t inst, uint8_t port, uint8_t offset, uint8_t *p_val);

/******************************************************************************
*
* brief :- Rx packet query
* Gets requested packet data for the specified Rx port as received by HDMI-Rx.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* input  len           Length of packet - 1(0..136)
* object p_data        Array of bytes.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_packet_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet);

/******************************************************************************
*
* brief :- Gets status of active packets for the specified Rx port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Status of active events.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_packet_act_stat_query(inv_inst_t inst, uint8_t port, uint32_t *p_val);

/******************************************************************************
*
* brief :- Gets status of masked events of Rx packets for the specified Rx port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Status of unmasked rx packet events.
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_packet_evt_stat_query(inv_inst_t inst, uint8_t port, uint32_t *p_val);

/******************************************************************************
*
* brief :- Controls Rx packet events. Only masked events will be notified.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         Rx packet event mask.
*                       INV_HDMI_PACKET_FLAG__ACG    0x00000001
*                       INV_HDMI_PACKET_FLAG__ACP    0x00000002
*                       INV_HDMI_PACKET_FLAG__ISRC1  0x00000004
*                       INV_HDMI_PACKET_FLAG__ISRC2  0x00000008
*                       INV_HDMI_PACKET_FLAG__GMP    0x00000010
*                       INV_HDMI_PACKET_FLAG__EMP    0x00000020
*                       INV_HDMI_PACKET_FLAG__VS     0x00000040
*                       INV_HDMI_PACKET_FLAG__AVI    0x00000080
*                       INV_HDMI_PACKET_FLAG__SPD    0x00000100
*                       INV_HDMI_PACKET_FLAG__AUD    0x00000200
*                       INV_HDMI_PACKET_FLAG__MPG    0x00000400
*                       INV_HDMI_PACKET_FLAG__HDR    0x00000800
*
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_packet_evt_mask_set(inv_inst_t inst, uint8_t port, uint32_t *p_val);
inv_rval_t inv478x_rx_packet_evt_mask_get(inv_inst_t inst, uint8_t port, uint32_t *p_val);

inv_rval_t inv478x_rx_packet_vs_user_ieee_set(inv_inst_t inst, uint8_t port, uint32_t *p_val);
inv_rval_t inv478x_rx_packet_vs_user_ieee_get(inv_inst_t inst, uint8_t port, uint32_t *p_val);
/******************************************************************************
*
* brief :- Rx HDCP mode set
* To set/get the RX HDCP mode
* Controls the function of HDCP interface at specified Rx port.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         HDCP RX mode value from enum
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_mode_set(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val);
inv_rval_t inv478x_rx_hdcp_mode_get(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val);

/******************************************************************************
*
* brief :- Queries the type of HDCP connection established at the specified Rx port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
* object p_val         INV478X_HDCP_TYPE__NON       : No HDCP
*                       INV478X_HDCP_TYPE__HDCP1    : HDCP1.x
*                       INV478X_HDCP_TYPE__HDCP2    : HDCP2.x
*
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_type_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val);

/******************************************************************************
*
* brief :- Generate re-authentication request to the connected upstream at the specified Rx port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Input port(0 or 1)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_trash_req(inv_inst_t inst, uint8_t port);

/******************************************************************************
*
* brief :- Specific termination value applied when RSEN is high.
*
*  input  inst          Driver Instance(obtained from inv478x_create).
*  input  port          HDMI Input port(0 or 1)
*  object p_val         RSEN Value
*
*  retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_rsen_value_set(inv_inst_t inst, uint8_t port, uint8_t *p_val);
inv_rval_t inv478x_rx_rsen_value_get(inv_inst_t inst, uint8_t port, uint8_t *p_val);

/******************************************************************************
*
* brief :-If set ‘true’ then all existing ADBs(Audio Data Block) are automatically
* replaced with ADBs retrieved from downstream eARC. Note : CEC byte is re-calculated if enabled.
*
*  input  inst          Driver Instance(obtained from inv478x_create).
*  input  port          HDMI Input port(0 or 1)
*  object p_val         Enable/Disable Value
*
*  retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_edid_auto_adb_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_rx_edid_auto_adb_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :-Configures HDCP version capability: NONE, HDCP1, HDCP1_2 Set/Get
*
*  input  inst          Driver Instance(obtained from inv478x_create).
*  input  port          HDMI Input port(0 or 1)
*  object p_val         Enum Value
*
*  retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_cap_set(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val);
inv_rval_t inv478x_rx_hdcp_cap_get(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val);

/******************************************************************************
*
* brief :- Returns topology as exposed to upstream device
*
*  input  inst          Driver Instance(obtained from inv478x_create).
*  input  port          HDMI Input port(0 or 1)
*  object p_val         Pointer to structure variable
*
*  retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_topology_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_top *p_val);

/******************************************************************************
*
* brief :- Returns KSV list as exposed to upstream device
*
*  input  inst          Driver Instance(obtained from inv478x_create).
*  input  port          HDMI Input port(0 or 1)
*  input  len           Length of the list
*  object p_val         Pointer to the enum
*
*  retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_ksvlist_query(inv_inst_t inst, uint8_t port, uint8_t len, struct inv_hdcp_ksv *p_val);

/******************************************************************************
*
* brief :- Returns HDCP2.3 Content Stream Manage message from up-stream device.
*
*  input  inst          Driver Instance(obtained from inv478x_create).
*  input  port          HDMI Input port(0 or 1)
*  object p_val         Pointer to Structure Varible
*
*  retval inv_rval_t     INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_rx_hdcp_csm_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_csm *p_val);

/******************************************************************************
*
* brief :- Query for TMDS Div 1OVER4 Ratio
* Queries TMDS Bit Clock Ratio of expected outgoing audio stream on HDMI-Tx in HDMI2.0 mode.
*
* input  inst          Driver instance(obtained from inv478x_create).
* object p_val         To Know whether TMDS 1OVER4 Div is enabled or disabled
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_tmds2_1over4_query(inv_inst_t inst, uint8_t port, uint8_t *p_val);

/******************************************************************************
*
* brief :- HPD Controls the scrambling for Tx TMDS frequency <= 340 MHz by Tx port.
* With this control, user can enable or disable scrambling on selected Tx port.
* Controls the scrambling for Tx TMDS frequency <= 340 MHz by Tx port. With this
* control, user can enable or disable scrambling on selected Tx port.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         false: Disable scrambling for TMDS frequency <= 340 MHz
*                       true: Enable scrambling for TMDS frequency <= 340 MHz
*
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_tmds2_scramble340_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_tmds2_scramble340_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Tx HDCP mode value set/get
* Controls the HDCP function on selected Tx port.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         HDCP mode value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_mode_set(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val);
inv_rval_t inv478x_tx_hdcp_mode_get(inv_inst_t inst, uint8_t port, enum inv_hdcp_mode *p_val);

/******************************************************************************
*
* brief :- Tx HDCP status query
* Queries HDCP status on Selected Tx port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         HDCP status from enum
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_stat_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_stat *p_val);

/******************************************************************************
*
* brief :- Tx HDCP type query
* Queries HDCP type on Selected Tx port.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         HDCP type value from the enum
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_type_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val);

/******************************************************************************
*
* brief :- CEC ARC Ininit
*
*
* input  inst          Driver instance(obtained from inv478x_create).
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_cec_arc_init(inv_inst_t inst);
inv_rval_t inv478x_cec_arc_term(inv_inst_t inst);

/******************************************************************************
*
* brief :- Tx Av Mute value set/get
* Forces AVMUTE active when set to ‘true’, when set ‘false’ AVMUTE may be released as soon:
* 1) Input signal(audio/video) is valid and stable,
* 2) No AVMUTE is received from upstream device,
* 3) HDCP encryption is enabled(only when HDCP protection is required).
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Av Mute value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_avmute_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_avmute_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Tx Av Mute query
* Returns current applied AVMUTE state.
*
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Av mute Value(Clear(0)/Set(1))
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_avmute_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Forces HDMI output to be disabled if Hot-Plug signal is detected low.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         HPD Value(Clear(0)/Set(1))
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_gated_by_hpd_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_gated_by_hpd_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Read downstream EDID and HDCP capability after a rising edge of HPD is detected.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         EDID Read Value(Clear(0)/Set(1))
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_edid_read_on_hpd_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_edid_read_on_hpd_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Returns list of downstream KSVs or RxIDs. This data is only valid after
* ‘TX_HDCP_DS_READY’ notification. Note: The first KSV/RxID in list is represents key
* of first downstream device.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* input  len           KSV Length
* object p_val         KSV List Value of Five Byte
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_ksvlist_query(inv_inst_t inst, uint8_t port, uint8_t len, struct inv_hdcp_ksv *p_val);

/******************************************************************************
*
* brief :- Configures HDCP2.3 Content Stream Manage message for downstream repeater device.
* This API is only used when HDCP output is an HDCP source or when used with external HDCP
* repeater application(‘INV_HDCP_MODE__SRSK’).
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         CSM Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_csm_set(inv_inst_t inst, uint8_t port, struct inv_hdcp_csm *p_val);
inv_rval_t inv478x_tx_hdcp_csm_get(inv_inst_t inst, uint8_t port, struct inv_hdcp_csm *p_val);

/******************************************************************************
*
* brief :- Returns local AKSV (Only for HDMI1.4).
* Note: The format(little or big endian) is defined by ‘hdcp_ksv_format_set’.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         AKSV Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_aksv_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_ksv *p_val);

/******************************************************************************
*
* brief :- Provides reason of HDCP failure.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Failure Value From Structure
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_failure_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_failure *p_val);

/******************************************************************************
*
* brief :- Information about HDCP downstream topology. Only valid after ‘TX_HDCP_DS_READY’
* notification. Note: this API should always be called before ‘inv478x_tx_hdcp_ksvlist_query’
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Tpology Detail Value From Structure
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_topology_query(inv_inst_t inst, uint8_t port, struct inv_hdcp_top *p_val);

/******************************************************************************
*
* brief :- This control defines whether HDCP encryption is enabled after downstream
* link is authenticated. If disabled then Audio/Video remains muted as long its content is HDCP protected.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Encrypt enable(1) or Disable(0)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_encrypt_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_hdcp_encrypt_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- When ‘auto_approve’ is disabled the user application calls this API
* to unmute audio and video only if it has determined that none of the downstream
* devices requires revocation. This function is only applicable to when in HDCP
* source mode.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Approve enable(1) or Disable(0)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_approve(inv_inst_t inst, uint8_t port);

/******************************************************************************
*
* brief :- Returns current CTS/N value.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Structure Variable of Audio CSTN
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_audio_ctsn_query(inv_inst_t inst, uint8_t port, struct inv_hdmi_audio_ctsn *p_val);

/******************************************************************************
*
* brief :- Mutes audio
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Structure Variable of Audio CSTN
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_audio_mute_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_audio_mute_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Enable/disable pixel format conversion(colorimetry/chroma down sampling/range/bit depth)
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Zero for disable, 1 for enable
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_csc_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_csc_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Indicates whether conversion to requested pixel format is supported
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         False No Failure, 1 for Failure
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_csc_failure_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :-Configure desired pixel format
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Pixel Format Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_csc_pixel_fmt_set(inv_inst_t inst, uint8_t port, struct inv_video_pixel_fmt *p_val);
inv_rval_t inv478x_tx_csc_pixel_fmt_get(inv_inst_t inst, uint8_t port, struct inv_video_pixel_fmt *p_val);

/******************************************************************************
*
* brief :- Initiates a downstream HDCP re-authentication.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_reauth_req(inv_inst_t inst, uint8_t port);

/******************************************************************************
*
* brief :- Indicates downstream HDCP capability: NONE, HDCP1 or HDCP1_2.
* This information is available as soon incoming HPD is sensed high.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         HDCP Type Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_ds_cap_query(inv_inst_t inst, uint8_t port, enum inv_hdcp_type *p_val);

/******************************************************************************
*
* brief :- When set to ‘true’ audio and video are unmuted as soon downstream device is authenticated.
* If set to ‘false’ then user application is expected to call ‘inv478x_tx_hdcp_approve’ to unmute audio/video.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         HDCP Auto Approve Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hdcp_auto_approve_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_hdcp_auto_approve_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Returns current deep color mode.
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Deep Color Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_deep_clr_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_deep_clr *p_val);

/******************************************************************************
*
* brief :- Selects audio source per output  (NONE, RX, RX0, RX1, AP0, AP1)
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Tx Audio Source Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_audio_src_set(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val);
inv_rval_t inv478x_tx_audio_src_get(inv_inst_t inst, uint8_t port, enum inv478x_audio_src *p_val);

/******************************************************************************
*
* brief :- When set ‘true’ AVMUTE state is automatically released when: 1)’ inv478x_tx_avmute_enable_set()’
* is set to ‘false’, 2) Input signal(audio/video) is valid and stable, 3) No AVMUTE is received from upstream
* device, 4) HDCP encryption is enabled(only when HDCP protection is required).
*
* input  inst          Driver Instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Tx Audio Source Value
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_avmute_auto_clear_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_avmute_auto_clear_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Set/get tx pin swap and polarity inversion
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* object p_val          Pin Swap Enum Value(0/1/2/3)
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_pin_swap_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_pin_swap *p_val);
inv_rval_t inv478x_tx_pin_swap_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_pin_swap *p_val);

/******************************************************************************
*
* brief :- Controls Hor/Ver Sync polarity(separately) : PT, PP, PN, NP, NN.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* object p_val          Pointer To The ENUM Type
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_hvsync_pol_set(inv_inst_t inst, uint8_t port, enum inv_video_hvsync_pol *p_val);
inv_rval_t inv478x_tx_hvsync_pol_get(inv_inst_t inst, uint8_t port, enum inv_video_hvsync_pol *p_val);

/******************************************************************************
*
* brief :- If set true then HDMI output is automatically disabled(‘NONE’) as
* soon Rx detects a HDMI mode ‘NONE’ condition. The HDMI output remains disabled
* until user calls ‘tx_hdmi_reenable’.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* object p_val          Enable/Disable Value(0/1)
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_auto_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_auto_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Enable/disable the DDC lines on the requested Tx port.
* Link training is supported only when DDC lines are enabled.
* When set to false , desired av link must be specified using the API
* 'inv478x_tx_av_link_set'
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* object p_val          True for enabled or False for disabled
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_ddc_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_ddc_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Re-enables HDMI output. The new HDMI mode is defined by
* ‘tx_hdmi_mode_mode_set’
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_reenable(inv_inst_t inst, uint8_t port);

/******************************************************************************
*
* brief :- Configures the colour for the blanking video on Tx.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* Object p_val          Structure Variaable
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_blank_color_set(inv_inst_t inst, uint8_t port, struct inv_video_rgb_value *p_val);
inv_rval_t inv478x_tx_blank_color_get(inv_inst_t inst, uint8_t port, struct inv_video_rgb_value *p_val);

/******************************************************************************
*
* brief :- If set to ‘true’ then SCDC-Tx will enable read request only if
* ‘RR_capable’ bit in downstream EDID is set or if ‘inv478x_tx_edid_read_on_hpd_set’
* is configured to ’false’.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* Object p_val          Enable/Disable Value(1/0)
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_scdc_rr_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_scdc_rr_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Polling period in msec. when ‘Update Flag Polling’ is enabled.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* in/out p_val          SCDC Poll Period Value
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_scdc_poll_period_set(inv_inst_t inst, uint8_t port, uint16_t *p_val);
inv_rval_t inv478x_tx_scdc_poll_period_get(inv_inst_t inst, uint8_t port, uint16_t *p_val);

/******************************************************************************
*
* brief :- SCDC control data write to downstream SCDC device.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* in     p_val          8-bit Value to Be Written
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_scdc_write(inv_inst_t inst, uint8_t port, uint8_t offset, uint8_t *p_val);

/******************************************************************************
*
* brief :- SCDC control/status data read from downstream SCDC device.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* in     p_val          8-bit Value to Be Read
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_scdc_read(inv_inst_t inst, uint8_t port, uint8_t offset, uint8_t *p_val);

/******************************************************************************
*
* brief :- Generic Data Burst Write to Downstream Device.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* in     p_val          8-bit Data Array
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_ddc_write(inv_inst_t inst, uint8_t port, uint8_t slv_addr, uint8_t offset, uint8_t size, uint8_t *p_val);

/******************************************************************************
*
* brief :- Generic Data Burst Read From Downstream Device
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* Out    p_val          8-bit Data Array
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_SUPP_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_ddc_read(inv_inst_t inst, uint8_t port, uint8_t slv_addr, uint8_t offset, uint8_t size, uint8_t *p_val);

/******************************************************************************
*
* brief :- Controls DDC Clock Frequency.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* Out    p_val          8-bit Data Array
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_ddc_freq_set(inv_inst_t inst, uint8_t port, uint8_t *p_val);
inv_rval_t inv478x_tx_ddc_freq_get(inv_inst_t inst, uint8_t port, uint8_t *p_val);

/******************************************************************************
*
* brief :- Tx AV-Link status
*
* Returns the current Tx AV Link status.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         Enumerator indicates any of the following states:
*                       inv478x_AV_LINK__NONE    No video
*                       inv478x_AV_LINK__DVI     DVI
*                       inv478x_AV_LINK__TMDS1   HDMI1.x
*                       inv478x_AV_LINK__TMDS2   HDMI2.x
*                       inv478x_AV_LINK__FRL4X6  HDMI2.1
*                       inv478x_AV_LINK__FRL4X8  HDMI2.1
*                       inv478x_AV_LINK__FRL4X10 HDMI2.1
*                       inv478x_AV_LINK__UNKNOWN HDMI2.1
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_av_link_query(inv_inst_t inst, uint8_t port, enum inv_hdmi_av_link *p_val);

/******************************************************************************
*
* brief :- Returns whether or not scrambling is enabled.
*
* input  inst          Driver instance(obtained from inv478x_create).
* input  port          HDMI Output port(0 or 1)
* object p_val         True for enabled or False for disabled
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_tmds2_scramble_query(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Tpg Video Timing Details set/get
*
* input  inst          Driver Instance(obtained from inv478x_create).
* object p_val         Tpg Timing Details
*
* retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
*                        INV_RVAL__PAR_ERR          0x00000001
*                        INV_RVAL__HOST_ERR         0x00000002
*                        INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_vtpg_video_tim_set(inv_inst_t inst, struct inv_video_timing *p_val);
inv_rval_t inv478x_vtpg_video_tim_get(inv_inst_t inst, struct inv_video_timing *p_val);

/******************************************************************************
*
* brief :- Configures blank video transfer on Tx ports.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* object p_val          Enable/Disable Value(0/1)
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_blank_enable_set(inv_inst_t inst, uint8_t port, bool_t *p_val);
inv_rval_t inv478x_tx_blank_enable_get(inv_inst_t inst, uint8_t port, bool_t *p_val);

/******************************************************************************
*
* brief :- Configures different types of packets on Tx.
*
* input  inst           Driver Instance(obtained from inv478x_create).
* input  port           HDMI Output port(0 or 1)
* object p_val          Enable/Disable Value(0/1)
*
* retval inv_rval_t       INV_RVAL__SUCCESS          0x00000000
*                         INV_RVAL__PAR_ERR          0x00000001
*                         INV_RVAL__HOST_ERR         0x00000002
*                         INV_RVAL__NOT_IMPL_ERR     0x00000004
*
*****************************************************************************/
inv_rval_t inv478x_tx_packet_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet);
inv_rval_t inv478x_tx_packet_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, uint8_t len, uint8_t *p_packet);

/******************************************************************************
 *
 * brief :- Selects source per packet type(NONE, RX, RX0, RX1, INSRT) When INSRT
 * is selected the inserted packet content is defined by ‘inv478x_tx_packet_insrt_set’.
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * input  port          HDMI Output port(0 or 1)
 * input  type          Packet type
 * object p_val         Packet Source Value(Clear(0)/Set(1))
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_tx_packet_src_set(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, enum inv478x_packet_src *p_val);
inv_rval_t inv478x_tx_packet_src_get(inv_inst_t inst, uint8_t port, enum inv_hdmi_packet_type type, enum inv478x_packet_src *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_enable_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_gpio_enable_get(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_mode_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_gpio_mode_get(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_gpio_get(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_set_bits(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_clear_bits(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_query(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_evt_stat_query(inv_inst_t inst, uint8_t *p_val);

/******************************************************************************
 *
 * brief :-
 *
 * input  inst          Driver instance(obtained from inv478x_create).
 * object p_val
 *
 * retval inv_rval_t      INV_RVAL__SUCCESS          0x00000000
 *                        INV_RVAL__PAR_ERR          0x00000001
 *                        INV_RVAL__HOST_ERR         0x00000002
 *                        INV_RVAL__NOT_IMPL_ERR     0x00000004
 *
 *****************************************************************************/
inv_rval_t inv478x_gpio_evt_mask_set(inv_inst_t inst, uint8_t *p_val);
inv_rval_t inv478x_gpio_evt_mask_get(inv_inst_t inst, uint8_t *p_val);

uint16_t inv_base64_decode(uint8_t *p, uint16_t len, char *s);

#ifdef __cplusplus
}
#endif
#endif /* INV478X_H */
