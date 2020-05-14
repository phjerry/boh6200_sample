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
* file inv478x_example_app.c
*
* brief example application for inv478x
*
*****************************************************************************/

/***** #include statements ***************************************************/

#include <stdio.h>
//#include <conio.h>
//#include <windows.h>
#include <stdlib.h>
#include <unistd.h>

#include "inv478x_api.h"
#include "inv478x_app_def.h"
#include "inv_drv_cra_api.h"
#include "inv_common.h"
#include "inv_adapter_api.h"
#include "inv_cec_api.h"


#include <string.h>
#include "hi_unf_i2c.h"

/***** global variables *******************************************************/

/***** local macro definitions ***********************************************/

#define INV_CALLBACK__HANDLER
#define PACKET_SIZE    INV478X_USER_DEF__ISP_CASH_SIZE
#define HI_INFO_I2C(format, arg...) printf( format, ## arg)

/***** local variables *******************************************************/

static inv_inst_t inst = INV_INST_NULL;
static inv_inst_t inst_9612 = INV_INST_NULL;
static inv_inst_t adpt_inst_9612_tx = INV_INST_NULL;
static inv_inst_t adpt_inst_9612_rx = INV_INST_NULL;
static inv_inst_t adpt_inst_478x = INV_INST_NULL;
static struct inv_adapter_cfg cfg_9612_rx;
static struct inv_adapter_cfg cfg_9612_tx;
static struct inv_adapter_cfg cfg_478x;
static char **adapter_list;
static bool_t sbRunning = true;
static int sTest = -1;
static int arc_tx_flag = 0;
static int arc_rx_flag = 1;
static int cec_created = 0;
static bool_t irq_detect = false;
static bool_t irq_poll = false;

/* Static configuratyion object for host driver configuration */
static struct inv478x_config sInv478xConfig = {
    "478x-Rx",                  /* pNameStr[8]         - Name of Object */
    true,                       /* bDeviceReset       - */
};

/***** local functions definitions ***********************************************/

static bool_t s_check_boot_stat(void);
static void inv478x_event_callback_func(inv_inst_t inst1, inv478x_event_flags_t event_flags);
void inv9612_app_rx_notification(uint32_t evt_flgs_notify);
void inv9612_app_tx_notification(uint32_t evt_flgs_notify);
static void s_log_version_information(void);


/***** local functions *******************************************************/
void s_adapter_478x_interrupt(inv_inst_t inst1)
{
    irq_detect = true;
    inst1 = inst1;
    //inv478x_handle(inst);
}

static void s_adapter_478x_callback(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_478x) {
        printf("adapter_478x_callback: %s\n", inv_adapter_connect_query(inst1) ? "adapter is connected" : "adatper is disconnected");

    }
#endif
}

static void s_adapter_9612_tx_callback(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_9612_rx) {
        if (arc_rx_flag) {
            printf("adapter_9612_tx_callback: %s\n", inv_adapter_connect_query(inst1) ? "adapter is connected" : "adatper is disconnected");
        }
    }
#endif
}

static void s_adapter_9612_rx_callback(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_9612_tx) {
        if (arc_rx_flag) {
            printf("adapter_9612_rx_callback: %s\n", inv_adapter_connect_query(inst1) ? "adapter is connected" : "adatper is disconnected");
        }
    }
#endif
}

static void s_adapter_478x_callback_two(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_478x) {
        printf("adapter_478x_callback_two: %s\n", inv_adapter_connect_query(inst1) ? "adapter is connected" : "adapter is disconnected");

    }
#endif
}

static void s_adapter_9612_tx_callback_two(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_9612_rx) {
        if (arc_rx_flag) {
            printf("adapter_9612_tx_callback_tw0: %s\n", inv_adapter_connect_query(inst1) ? "adapter is connected" : "adapter is disconnected");
        }
    }
#endif
}

static void s_adapter_9612_rx_callback_two(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_9612_tx) {
        if (arc_rx_flag) {
            printf("adapter_9612_rx_callback_two: %s\n", inv_adapter_connect_query(inst1) ? "adapter is connected" : "adapter is disconnected");
        }
    }
#endif
}

static uint8_t s_adapter_list_query(void)
{
    uint8_t id = 0;
    char temp;
    char **p_ptr;
    adapter_list = inv_adapter_list_query();
    p_ptr = adapter_list;
    if (adapter_list == NULL) {
      printf("\n------------ adapter_list is null----------------\n");
	  return 0xFF;  //return from here
    }
    printf("\n------------ list of connection----------------\n");
    while (*p_ptr) {
        printf("%d: %s\n", id++, *p_ptr++);
    }
    printf("Please select the connection: ");
    scanf("%d", &temp);
    id = (uint8_t) temp;
    //id = (uint8_t)getche() - '0';
    printf("\n---------------------------------\n");
    return id;
}

static void create_478x_adapter(uint8_t id, uint8_t addr)
{
    /*----------------------------------------------------------- */
    /*
     * System initialization
     */
    /*----------------------------------------------------------- */

    /*
     * Initialize all system resources
     *///
     printf("\ncreate_478x_adapter 11\n");
	  printf("\ncreate_478x_adapter rrrhh\n");
	  
    cfg_478x.chip_sel = addr;
    cfg_478x.mode = INV_ADAPTER_MODE__I2C;
	printf("\ncreate_478x_adapter rrr33\n");
			 
  //  cfg_478x.adapt_id = adapter_list[id];   //"Aardvark_2238-031382";
  // cfg_478x.adapt_id = "23";   //"Aardvark_2238-473723";
     printf("\ncreate_478x_adapter rrr22\n");
    adpt_inst_478x = inv_adapter_create(&cfg_478x, s_adapter_478x_callback, s_adapter_478x_interrupt);
    if (!adpt_inst_478x) {
        printf("\nNo adatper found\n");
		getc(stdin);
        exit(0);
    }

    if (cfg_478x.adapt_id[0] == 'A') {
        irq_poll = true;
    }
	printf("\ncreate_478x_adapter 22\n");
    inv_platform_adapter_select(0, adpt_inst_478x);
    inst = inv478x_create(0, inv478x_event_callback_func, &sInv478xConfig);
    //inv478x_tpg_enable_set(inst, &bEn);
	printf("\ncreate_478x_adapter 33\n");
    if ((inv_inst_t *) INV_RVAL__HOST_ERR == inst) {
        printf("\nERROR: Host connection error\n");
        exit(0);
    }
    if (INV_INST_NULL == inst) {
        printf("\nERROR: Memory problem\n");
        exit(0);
    }

    if (s_check_boot_stat()) {
        /*
         * Retrieve general product/revision info
         */
        s_log_version_information();
    }
}

static void create_tx_9612_adapter(uint8_t id, uint8_t addr)
{
    cfg_9612_tx.chip_sel = addr;
    cfg_9612_tx.mode = INV_ADAPTER_MODE__I2C;
    cfg_9612_tx.adapt_id = adapter_list[id];    //"Aardvark_2238 - 031382";

    inv_cec_tx_delete();
    inv_cec_rx_delete();
    adpt_inst_9612_tx = inv_adapter_create(&cfg_9612_tx, s_adapter_9612_rx_callback, s_adapter_9612_rx_callback_two);
    if (!adpt_inst_9612_tx) {
        printf("\nNo adatper found\n");
        exit(0);
    }
    inv_platform_adapter_select(1, adpt_inst_9612_tx);
    //inst_9612 = inv_cec_tx_create(1, &inv9612_app_tx_notification, adpt_inst_9612_tx);
}

static void create_rx_9612_adapter(uint8_t id, uint8_t addr)
{
    cfg_9612_rx.chip_sel = addr;
    cfg_9612_rx.mode = INV_ADAPTER_MODE__I2C;
    cfg_9612_rx.adapt_id = adapter_list[id];    //"Aardvark_2238 - 031382";
    adpt_inst_9612_rx = inv_adapter_create(&cfg_9612_rx, s_adapter_9612_rx_callback, s_adapter_9612_tx_callback_two);
    inv_platform_adapter_select(2, adpt_inst_9612_rx);
        //inv_cec_rx_create(2, &inv9612_app_rx_notification, adpt_inst_9612_rx);
}

static bool_t s_check_boot_stat(void)
{
    enum inv_sys_boot_stat boot_stat;

    inv478x_boot_stat_query(inst, &boot_stat);
    return (INV_SYS_BOOT_STAT__SUCCESS == boot_stat) ? (true) : (false);
}

static void s_log_version_information(void)
{
    uint32_t chipIdStat = 0;
    uint32_t chipRevStat = 0;
    uint16_t versionStat[10];

    inv478x_chip_id_query(inst, &chipIdStat);
    inv478x_chip_revision_query(inst, &chipRevStat);
    inv478x_firmware_version_query(inst, (uint8_t *) versionStat);



    printf("\n");
    printf("inv478x_chip_id_query           : %0ld\n", chipIdStat);
    printf("inv478x_chip_revision_query     : %02X\n", (int) chipRevStat);
    printf("inv478x_firmware_version_query  : %s\n", versionStat);

}

/* Inv478xrx API Call-Back Handler */
static void inv478x_event_callback_func(inv_inst_t inst, inv478x_event_flags_t event_flags)
{
    int vic, key;
    struct inv_video_pixel_fmt p_val_pixel;
    enum inv_hdmi_packet_type type;
    uint8_t p_packet[31];

    uint8_t port = 0;
    uint32_t ret_val;
    uint16_t p_val;
    enum inv478x_dl_mode dl_val;
    inst = inst;


    if (event_flags & INV478X_EVENT_FLAG__BOOT) {
        uint32_t ret_val;
        enum inv_sys_boot_stat boot_stat_val;
        ret_val = inv478x_boot_stat_query(inst, &boot_stat_val);
        printf("\nbit 7  : Boot Status Changed");
        if (boot_stat_val == INV_SYS_BOOT_STAT__SUCCESS)
            create_tx_9612_adapter(0, 0x30);
    }
    if (event_flags & INV478X_EVENT_FLAG__TX0_RSEN)
        printf("\nbit 12 : Tx Zero RSEN Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX0_HPD)
        printf("\nbit 0  : Tx Zero HPD Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX1_RSEN)
        printf("\nbit 13 : Tx One RSEN Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX1_HPD)
        printf("\nbit 1  : Tx One HPD Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_PLUS5V)
        printf("\nbit 2  : Rx Zero 5V Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_PLUS5V)
        printf("\nbit 3  : Rx One 5V Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_AVLINK)
        printf("\nbit 4  : Rx Zero AV Link Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_AVLINK)
        printf("\nbit 5  : Rx One AV Link Changed");
    if (event_flags & INV478X_EVENT_FLAG__EARC_LINK)
        printf("\nbit 9  : Rx EARC Rx State Changed");
    if (event_flags & INV478X_EVENT_FLAG__EARC_RX_AUDIO)
        printf("\nbit 8  : Rx EARC Rx Audio Stream Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_HDCP)
        printf("\nbit 10 : RX0 HDCP State Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_HDCP)
        printf("\nbit 11 : RX1 HDCP State Changed");
    if (event_flags & INV478X_EVENT_FLAG__EARC_DISC_TIMEOUT) {
        printf("\nbit 14 : EARC DISCOVERY TIMEOUT");
        if (arc_tx_flag) {
            inv_rx_cec_arc_tx_start();
        }
        if (arc_rx_flag) {
            inv_tx_cec_arc_rx_start();
        }
    }
    if (event_flags & INV478X_EVENT_FLAG__TX0_HDCP)
        printf("\nbit 16 : TX0 HDCP State Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX1_HDCP)
        printf("\nbit 17 : TX1 HDCP State Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_VIDEO) {
        printf("\nbit 18 : Rx Zero Video format changed");
        type = INV_HDMI_PACKET_TYPE__AVI;
        ret_val = inv478x_rx_pixel_fmt_query(inst, port, &p_val_pixel);
        ret_val = inv478x_rx_packet_query(inst, port, type, 31, p_packet);
        vic = p_packet[6];
        if (vic == 117 || vic == 118 || vic == 218 || vic == 219 || vic == 202 || vic == 203 || vic == 204) {
            if (p_val_pixel.bitdepth == INV_VIDEO_BITDEPTH__10BIT && p_val_pixel.chroma_smpl == INV_VIDEO_CHROMA_SMPL__444RGB) {
                ret_val = inv478x_dl_mode_get(inst, &dl_val);
                if (dl_val == INV478X_DL_MODE__NONE || dl_val == INV478X_DL_MODE__MERGE) {
                    /*
                     * enable split
                     */
                    dl_val = INV478X_DL_MODE__SPLIT_AUTO;
                    inv478x_dl_mode_set(inst, &dl_val);
                    p_val = (uint16_t)600000;
                    inv478x_dl_split_auto_set(inst, &p_val);

                }
                printf("\n1. Set output pixel format to 422\n 2.set bitdepth to 8 bit");
                key = getchar();
                switch (key) {
                case '1':
                    printf("\nSetting output pixel format to 422");
                    p_val_pixel.chroma_smpl = INV_VIDEO_CHROMA_SMPL__422;
                    inv478x_tx_csc_pixel_fmt_set(inst, port, &p_val_pixel);
                    break;
                case '2':
                    printf("\nSetting  bitdepth to 8 bit");
                    p_val_pixel.chroma_smpl = INV_VIDEO_CHROMA_SMPL__444RGB;
                    p_val_pixel.bitdepth = INV_VIDEO_BITDEPTH__8BIT;
                    inv478x_tx_csc_pixel_fmt_set(inst, port, &p_val_pixel);

                    break;
                default:
                    break;
                }
            }
        }

    }
    if (event_flags & INV478X_EVENT_FLAG__RX1_VIDEO)
        printf("\nbit 19 : Rx One Video format changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_AUDIO)
        printf("\nbit 20 : Rx Zero Audio format changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_AUDIO)
        printf("\nbit 21 : Rx One Audio format changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_PACKET)
        printf("\nbit 22 : Rx Zero Info-packet changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_PACKET)
        printf("\nbit 23 : Rx One Info-packet changed");
    if (event_flags & INV478X_EVENT_FLAG__EARC_ERX_LATENCY_CHANGE)
        printf("\nbit 26 : eARC Latency changed");
    if (event_flags & INV478X_EVENT_FLAG__EARC_ERX_LATENCY_REQ_CHANGE)
        printf("\nbit 27 : eARC Latency Request changed");
}

void inv9612_app_rx_notification(uint32_t evt_flgs_notify)
{
    if (evt_flgs_notify & INV9612_EVENT_FLAG__ARC_INIT) {
        enum inv_hdmi_earc_mode p_val = INV_HDMI_EARC_MODE__ARC;
        inv478x_earc_mode_set(inst, &p_val);
        ;
    }

    if (evt_flgs_notify & INV9612_EVENT_FLAG__ARC_TERM) {
        enum inv_hdmi_earc_mode p_val = INV_HDMI_EARC_MODE__EARC;
        inv478x_earc_mode_set(inst, &p_val);
    }
}

void inv9612_app_tx_notification(uint32_t evt_flgs_notify)
{
    if (evt_flgs_notify & INV9612_EVENT_FLAG__ARC_INIT) {
        enum inv_hdmi_earc_mode p_val = INV_HDMI_EARC_MODE__ARC;
        inv478x_earc_mode_set(inst, &p_val);
    }

    if (evt_flgs_notify & INV9612_EVENT_FLAG__ARC_TERM) {
        enum inv_hdmi_earc_mode p_val = INV_HDMI_EARC_MODE__EARC;
        inv478x_earc_mode_set(inst, &p_val);
    }
}

void case_generic(void)
{
    uint8_t key_api_num;
    printf("\n a. Chip Id Query");
    printf("\n b. Chip Revision Query");
    printf("\n c. Firmware Version Query");
    printf("\n d. Re-boot Request");
    printf("\n f. Freeze Set/Get");
    printf("\n g. Boot Status Query");
    printf("\n h. HDCP-2.3 License Set/Get");
    printf("\n i. HDCP 2.3 Device ID Query");
    printf("\n j. Heart Beat LED ON/OFF");
    printf("\n k. HDCP2.3 Certificate ID Query");

    printf("\n Press the character key");

    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t chip_id, ret_val;

            ret_val = inv478x_chip_id_query(inst, &chip_id);
            if (!ret_val)
                printf("\n The chip id is : 0x%X\n", chip_id);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    case 'b':
        {
            uint32_t chip_rev, ret_val;

            ret_val = inv478x_chip_revision_query(inst, &chip_rev);
            if (!ret_val)
                printf("\n The chip revision is : 0x%X\n", chip_rev);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    case 'c':
        {
            uint32_t ret_val;
            uint16_t versionStat[10];

            ret_val = inv478x_firmware_version_query(inst, (uint8_t *) versionStat);
            if (!ret_val)
                printf("\n The Firmware Version is : %s\n", versionStat);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    case 'd':
        {
            uint32_t ret_val;

            ret_val = inv478x_reboot_req(inst);
            if (ret_val)
                printf("\n Reboot done...");
            else
                printf("\n Return value = %d", ret_val);
            break;
        }

    case 'f':
        {
            uint32_t ret_val;
            uint8_t ch_set_get;
            bool_t val;

            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter 1 to set, 0 to clear");
                    scanf("%d", &val);
                    ret_val = inv478x_freeze_set(inst, &val);
                    if (!ret_val)
                        printf("\n The freeze value has been set");
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_freeze_get(inst, &val);
                    if (!ret_val)
                        printf("\n The freeze value is : 0x%X\n", val);
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'g':
        {
            uint32_t ret_val;
            enum inv_sys_boot_stat boot_stat_val;

            ret_val = inv478x_boot_stat_query(inst, &boot_stat_val);
            if (!ret_val)
                printf("\n Boot status value is = %X\n", boot_stat_val);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    case 'h':
        {
            uint32_t ret_val;
            uint32_t val;

            ret_val = inv478x_chip_serial_query(inst, &val);
            if (!ret_val)
                printf("\n HDCP2.3 Device ID is = %lX\n", val);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    case 'i':
        {
            uint32_t ret_val;
            uint8_t ch_set_get;
            bool_t val;

            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter 1 to make LED ON, 0 to make LED OFF : ");
                    scanf("%d", &val);
                    ret_val = inv478x_heart_beat_set(inst, &val);
                    if (!ret_val)
                        printf("\n The heart beat value has been set");
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_heart_beat_get(inst, &val);
                    if (!ret_val)
                        printf("\n The heart value is : 0x%X\n", val);
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            }
            break;
        }

    }                           /* end of switch */
}

void case_miscellaneous(void)
{
    uint8_t key_api_num;
    printf("\n a. HDCP-2.3 KSV Big  Endian Set/Get");

    printf("\n Press the character key");

    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {

    case 'a':
        {
            uint32_t ret_val;
            uint8_t ch_set_get;
            bool_t val;


            printf("\n HDCP-2.3 KSV Big  Endian : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enable or Disable HDCP Big Endian KSV (0/1) :");
                    scanf("%d", &val);
                    ret_val = inv478x_hdcp_ksv_big_endian_set(inst, &val);
                    if (!ret_val)
                        printf("\n Tx HDCP Sig Endian KSV Has been Set");
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_hdcp_ksv_big_endian_get(inst, &val);
                    if (!ret_val)
                        printf("\n Tx HDCP Sig Endian KSV Value is : %d", val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }

    }
}

void case_user_notifications(void)
{
    uint8_t key_api_num;
    printf("\n a. Event Flag Mask Set/Get");
    printf("\n b. Event Flag Status Query");
    printf("\n c. Event Flag Status Clear");

    printf("\n Press the character key");

    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val, mask_val;
            uint8_t ch_set_get;

            printf("\n Event Flag Mask : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    uint32_t mask_val, ret_val;
                    printf("\n Enter the mask value in hexadecimal : ");
                    scanf("%x", &mask_val);

                    ret_val = inv478x_event_flags_mask_set(inst, &mask_val);
                    if (!ret_val)
                        printf("\n Event flag mask value has been set to 0x%X\n", mask_val);
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_event_flags_mask_get(inst, &mask_val);
                    if (!ret_val)
                        printf("\n Event flag mask value is = 0x%X\n", mask_val);
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'b':
        {
            uint32_t stat_val, ret_val;

            ret_val = inv478x_event_flags_stat_query(inst, &stat_val);
            if (!ret_val)
                printf("\n Event flag status value is = 0x%X\n", stat_val);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    case 'c':
        {
            uint32_t ret_val, p_val;
            printf("\n Enter the status clearing value in hexadecimal");
            scanf("%x", &p_val);

            ret_val = inv478x_event_flag_stat_clear(inst, &p_val);
            if (!ret_val)
                printf("\n The status has been cleared with value equal to %X\n", p_val);
            else
                printf("\n Return value = %d", ret_val);
            break;
        }
    }                           /* API switch closing here */
}

void case_flash_device_update(void)
{
    uint8_t key_api_num;
    printf("\n a. Flash Erase");
    printf("\n b. Flash Program");

    printf("\n Press the character key");

    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val;
            ret_val = inv478x_flash_erase(inst);
            if ((ret_val != INV_RVAL__BOARDNOT_CONNECTED) && (ret_val != INV_RVAL__ERASE_FAILED)) {
                printf("\n Erasing Done\n");
            } else {
                printf("\n Erasing failed\n");
            }
            break;
        }
    case 'b':
        {
            uint32_t ret_val = 0;
            uint32_t len = 0;
            int x = 0;
            uint8_t y = 0;
            uint8_t p_buffer[256];
            FILE *fptr;
            char input[256] = "";
            printf("\nEnter the bin file path to flash:\n");
            scanf("%s", &input);
            fptr = fopen(input, "rb");
            if (fptr == NULL)
                printf("FILE NOT FOUND\n");
            fseek(fptr, 0, SEEK_END);
            x = ftell(fptr);
            y = x % 256;
            while (x / 256 > 0) {
                fseek(fptr, len, SEEK_SET);
                fread(p_buffer, 256, 1, fptr);
                ret_val = inv478x_flash_program(inst, p_buffer, len);
                if (ret_val == INV_RVAL__PROGRAM_FAILED) {
                    printf("Board is in aon domain\n");
                    printf("Turn on the board\n");
                    break;
                }
                len = len + 256;
                x = x - 256;
            }
            fseek(fptr, len, SEEK_SET);
            fread(p_buffer, y, 1, fptr);
            inv478x_flash_program(inst, p_buffer, len);
            ret_val = inv478x_reboot_req(inst);
            if (ret_val == INV_RVAL__CHECKSUM_PASS)
                printf("checksum pass\n");
            if (ret_val == INV_RVAL__CHECKSUM_FAILED)
                printf("checksum fail\n");
            if (ret_val != INV_RVAL__CHECKSUM_FAILED && ret_val != INV_RVAL__PROGRAM_FAILED)
                printf("\nProgramming Done");
            break;
        }

    }                           /* API switch closing here */
}

void case_dual_link_split_merge(void)
{
    uint8_t key_api_num;
    printf("\n DL Mode Set/Get");
    printf("\n DL Split Auto Set/Get");
    printf("\n DL Split Overlap Set/Get");
    printf("\n DL Swap Set/Get");

    printf("\n Press the character key");

    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val;
            enum inv478x_dl_mode val;
            uint8_t ch_set_get;

            printf("\n  DL Mode : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter The DL Mode Value : ");
                    scanf("%d", &val);
                    ret_val = inv478x_dl_mode_set(inst, &val);
                    if (!ret_val) {
                        printf("\n The DL Mode Has Been Set");
                    } else {
                        printf("\n Return value = %d", ret_val);
                    }
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_dl_mode_get(inst, &val);
                    if (!ret_val) {
                        printf("\n The DL Mode Value is : %d", val);
                    } else {
                        printf("\n Return value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'b':
        {
            uint32_t ret_val;
            uint16_t p_val;
            uint8_t ch_set_get;

            printf("\n  DL Auto Split : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter auto split char rate value in MHz:");
                    scanf("%d", &p_val);
                    ret_val = inv478x_dl_split_auto_set(inst, &p_val);
                    if (!ret_val)
                        printf("\n Rx auto split char rate set value is = %d", p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_dl_split_auto_get(inst, &p_val);
                    if (!ret_val)
                        printf("\n Rx auto split char rate get value is = %d", p_val);
                    else
                        printf("\n API Error Code Return Value = %d\n", ret_val);
                    break;
                }
            }
            break;
        }
    case 'c':
        {
            uint32_t ret_val;
            uint8_t val;
            uint8_t ch_set_get;

            printf("\n  DL Split Overlap : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter DL Split Overlap Value : ");
                    scanf("%d", &val);
                    ret_val = inv478x_dl_split_overlap_set(inst, &val);
                    if (!ret_val) {
                        printf("\n The DL Split Overlap Value Has Been Set");
                    } else {
                        printf("\n Return value = %d", ret_val);
                    }
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_dl_split_overlap_get(inst, &val);
                    if (!ret_val) {
                        printf("\n The DL Split Overlap Value is : %d", val);
                    } else {
                        printf("\n Return value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'd':
        {
            uint32_t ret_val;
            bool_t dl_swap_val;
            uint8_t ch_set_get;

            printf("\n  DL Mode : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter the DL Swap value 0/1 ");
                    scanf("%x", &dl_swap_val);
                    ret_val = inv478x_dl_swap_set(inst, &dl_swap_val);
                    if (!ret_val)
                        printf("\n The DL Sweep value has been set to %d\n", dl_swap_val);
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_dl_swap_get(inst, &dl_swap_val);
                    if (!ret_val)
                        printf("\n DL Sweep value is = %X\n", dl_swap_val);
                    else
                        printf("\n Return value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    }                           /* API switch closing here */
}                               /* end of function */

void case_pixel_format_conversion(void)
{
    uint8_t key_api_num;
    printf("\n a. Tx CSC Enable");
    printf("\n b. Tx CSC Pixel Format");
    printf("\n c. Tx CSC Failure Query");

    printf("\n Press the character key : ");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val;
            bool_t val;
            uint8_t port;
            uint8_t ch_set_get;


            printf("\n  Tx CSC Enable : ");
            printf("\n a. Set");
            printf("\n b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);
                    printf("\n **Press 1 to Set, 0 to Clear : ");
                    scanf("%d", &val);
                    printf("\n Port = %d\n", port);
                    ret_val = inv478x_tx_csc_enable_set(inst, port, &val);
                    if (!ret_val) {
                        printf("\n Tx CSC Value on port %d has been set", port);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("\n%d", &port);

                    ret_val = inv478x_tx_csc_enable_get(inst, port, &val);
                    if (!ret_val) {
                        printf("\n Tx CSC Value on port %d is %d", port, val);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'b':
        {
            struct inv_video_pixel_fmt p_val;
            uint8_t port;
            uint32_t ret_val;
            uint8_t ch_set_get;


            printf("\n  Tx CSC Pixel Format : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter Colorimetry(0 to 12) : ");
                    scanf("%d", &p_val.colorimetry);
                    printf("\n Enter Chroma Sample(0/1/2) : ");
                    scanf("%d", &p_val.chroma_smpl);
                    printf("\n Enable/Disable Full Range(0/1) : ");
                    scanf("%d", &p_val.full_range);
                    printf("\n Enter Bit Depth(0/1/2) : ");
                    scanf("%d", &p_val.bitdepth);

                    ret_val = inv478x_tx_csc_pixel_fmt_set(inst, port, &p_val);
                    if (!ret_val) {
                        printf("\n Tx CSC Pixel Format on port %d has been set", port);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_csc_pixel_fmt_get(inst, port, &p_val);
                    if (!ret_val) {
                        printf("\n Tx CSC Pixel Format on port %d is-", (uint8_t) port);
                        printf("\n Pixel Colorimetry is = %d", p_val.colorimetry);
                        printf("\n Pixel Chroma Sample is = %d", p_val.chroma_smpl);
                        printf("\n Pixel Full Range(enable/Disable) is = %d", p_val.full_range);
                        printf("\n Pixel Bitdepth is = %d", p_val.bitdepth);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'c':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;

            printf("\n Enter Tx Port Value(0/1) : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_csc_failure_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n Tx CSC Failure Value on port %d is %d", port, p_val);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    }                           /* switch (key_api_num) ends here */
}                               /* case ends here */

void case_receiver_common(void)
{
    uint8_t key_api_num;
    printf("\n a. Rx Port Select");
    printf("\n b. Rx Fast Switch");

    printf("\n Press the character key");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val = 0;
            uint8_t ch_set_get, port;

            printf("\n  Rx Port Select : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);
                    ret_val = inv478x_rx_port_select_set(inst, &port);
                    if (!ret_val) {
                        printf("\n **Port %d is selected\n", port);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_rx_port_select_get(inst, &port);
                    if (!ret_val) {
                        printf("\n **The current active port number = %d", port);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'b':
        {
            uint8_t ch_set_get;
            bool_t val = true;
            uint32_t ret_val;



            printf("\n  Rx Fast Switch : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Enter 0 to Clear, 1 to Set Fast Switch Mode: ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_fast_switch_set(inst, &val);
                    if (!ret_val) {
                        printf("\n The Fast Switch Value Set is = %d", val);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            case 'b':
                {
                    ret_val = inv478x_rx_fast_switch_get(inst, &val);
                    if (!ret_val) {
                        printf("\n The Fast Switch value = %d", val);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    }                           /* switch (key_api_num) ends here */
}                               /* case ends here */

void case_receiver(void)
{
    uint8_t key_api_num;
    printf("\n a. Rx Plus5V Query");
    printf("\n b. Rx EDID Enable");
    printf("\n c. Rx EDID Value");
    printf("\n d. Rx EDID Replicate");
    printf("\n e. Rx EDID Auto ADB Value");
    printf("\n f. Rx HPD Enable");
    printf("\n g. Rx HPD Replicate");
    printf("\n h. Rx HPD Phy Query");
    printf("\n i. Rx HPD Low Time");
    printf("\n j. Rx RSEN Enable");
    printf("\n k. Rx RSEN Value");
    printf("\n l. Rx AV-Link Query");
    printf("\n m. Rx Deep Color Query");
    printf("\n n. Rx AV Mute Query");
    printf("\n o. Rx Video Timing Query");
    printf("\n p. Rx Pixel Format Query");
    printf("\n q. Rx Audio Format Query");
    printf("\n r. Rx Audio Info Query");
    printf("\n s. Rx Audio CTSN Query");
    printf("\n t. Rx Packet Query");
    printf("\n u. Rx Packet Active Stat Query");
    printf("\n v. Rx Packet Event Stat Query");
    printf("\n w. Rx Packet Event Mask Value");
    printf("\n x. Rx HDCP Capability");
    printf("\n y. Rx HDCP Type Query");
    printf("\n z. Rx HDCP CSM Query");
    printf("\n D. Rx HDCP Topology Query");
    printf("\n E. Rx HDCP KSV List Query");
    printf("\n F. Rx HDCP BKSV Query");
    printf("\n G. Rx HDCP RXID Query");
    printf("\n H. Rx HDCP AKSV Query");
    printf("\n I. Rx HDCP Trash Request");
    printf("\n J. Rx SCDC Register Query");
    printf("\n K. Rx HDCP Stat Query");
    printf("\n L. Rx HDCP Mode Value");

    printf("\n Press the character key");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            bool_t val;
            uint32_t ret_val;
            uint8_t port;

            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_plus5v_query(inst, port, &val);

            if (!ret_val)
                printf("\n Plus 5V value on port %d is = %d", port, val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'b':
        {
            bool_t val = 0;
            uint32_t ret_val;
            uint8_t ch_set_get, port;;


            printf("\n  Rx EDID Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter 1 to enable, 0 to disable ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_edid_enable_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n EDID enable value set on port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_edid_enable_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n EDID enable value on port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    break;
                }
            }
            break;
        }
    case 'c':
        {
            uint8_t length, offset, block, port;
            uint8_t p_val[256];
            uint8_t i;
            inv_rval_t ret_val;
            uint8_t ch_set_get;


            printf("\n  Rx EDID Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);
                    printf("\n **Enter block number 0/1 : ");
                    scanf("%d", &block);
                    printf("\n **Enter Offset(0 to 127) : ");
                    scanf("%d", &offset);
                    printf("\n **Enter Length(max(128 - offset) : ");
                    scanf("%d", &length);
                    printf("\n Enter %d elements of EDID starting from offset %d\n", length, offset);
                    for (i = 0; i < length; i++)
                        scanf("%x", &p_val[i]);
                    printf("\n The Enterd EDID is :\n");
                    for (i = 0; i < length; i++) {
                        printf("%02X ", p_val[i]);
                        if ((i + 1) % 16 == 0)
                            printf("\n");
                    }
                    ret_val = inv478x_rx_edid_set(inst, port, block, offset, length, p_val);
                    if (ret_val == 0)
                        printf("\n The EDID has been set on port %d", port);
                    else
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);
                    printf("\n **Enter block number 0/1 : ");
                    scanf("%d", &block);
                    printf("\n **Enter Offset(0 to 127) : ");
                    scanf("%d", &offset);
                    printf("\n **Enter Length(max(128 - offset) : ");
                    scanf("%d", &length);
                    ret_val = inv478x_rx_edid_get(inst, port, block, offset, length, p_val);

                    if (ret_val == 0) {
                        if (length > offset)
                            length = length - offset;
                        else if ((length + offset) > 128)
                            length = (128 - offset);
                        for (i = 0; i < length; i++) {
                            printf("%02X ", p_val[i]);
                            if ((i + 1) % 16 == 0)
                                printf("\n");
                        }
                    } else {
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'd':
        {
            uint8_t port = 0;
            
            uint32_t ret_val;
            bool_t val;

            uint8_t ch_set_get;

            printf("\n  Rx EDID Replicate : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n **Press 0 to EDID Replicate NONE");
                    printf("\n **Press 1 to EDID Replicate Enable tx0");
                    printf("\n **Press 2 to EDID Replicate Enable tx1");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_edid_replicate_set(inst, port, (enum inv_hdmi_edid_repl*)&val);
                    if (ret_val == 0)
                        printf("\n EDID Replicate Set Value on rx_port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_edid_replicate_get(inst, port, (enum inv_hdmi_edid_repl*) &val);
                    if (ret_val == 0)
                        printf("\n EDID Replicate Set Value on rx_port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    break;
                }
            }
            break;
        }
    case 'e':
        {
            uint8_t port = 0;
            uint32_t ret_val;
            bool_t val;

            uint8_t ch_set_get;

            printf("\n  Rx EDID Auto ADB Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n Enable/Disable EDID Auto ADB (0/1) : ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_edid_auto_adb_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Rx EDID Auto ADB Has Been Set");
                    else
                        printf("\n API Error Code Return Value : %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_edid_auto_adb_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n EDID Auto ADB Value On %d Port Is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value : %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'f':
        {
            bool_t val;
            uint8_t port;

            uint8_t ch_set_get;
            uint32_t ret_val;


            printf("\n  Rx HPD Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n ** Press 0 to Clear HPD, 1 to Set HPD, 0/1 :");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_hpd_enable_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n The HPD value on port %d is %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_hpd_enable_get(inst, port, &val);
                    if (!ret_val) {
                        printf("\n The HPD value on port %d is %d", port, val);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'g':
        {
            uint32_t ret_val;
            uint8_t port;
            uint8_t tx_port;

            enum inv_hdmi_hpd_repl val;
            uint8_t ch_set_get;


            printf("\n  Rx HPD Replicate : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n **Press 0 to HPD Replicate Disable");
                    printf("\n **Press 1 to HPD Replicate Enable Level tx0");
                    printf("\n **Press 2 to HPD Replicate Enable EDGE tx0");
                    printf("\n **Press 1 to HPD Replicate Enable Level tx1");
                    printf("\n **Press 2 to HPD Replicate Enable EDGE tx1");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_hpd_replicate_set(inst, port, &val);
                    if (ret_val == 0)
                        printf("\n HPD replicate Set Value on rx_port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("%d", &port);
                    ret_val = inv478x_rx_hpd_replicate_get(inst, port, &val);
                    if (ret_val == 0)
                        printf("\n Hpd Replicate Get Value on rx_port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'h':
        {
            bool_t val = 0;
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_hpd_phy_query(inst, port, &val);
            if (!ret_val)
                printf("\n HPD Phy on port %d is = %d", port, val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'i':
        {
            uint16_t val;
            uint8_t port;
            uint32_t ret_val;

            uint8_t ch_set_get;


            printf("\n  Rx HPD Low Time Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter HPD Low Time Value To Set(in ms) : ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_hpd_low_time_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n HPD low time set on port %d is = %d ms", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_hpd_low_time_get(inst, port, &val);
                    printf("\n API Error Code Return Value = %d", ret_val);
                    if (ret_val == 0)
                        printf("\n HPD low time set on port %d is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'j':
        {
            bool_t p_val = 0;
            uint8_t port;
            uint32_t ret_val;

            uint8_t ch_set_get;


            printf("\n  Rx RSEN Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter 1 to enable, 0 to disable ");
                    scanf("%d", &p_val);
                    ret_val = inv478x_rx_rsen_enable_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n RSEN value set on port %d is = %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_rsen_enable_get(inst, port, &p_val);

                    if (!ret_val)
                        printf("\n HPD low time value on port %d is = %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'k':
        {
            uint8_t port, val;
            uint32_t ret_val;

            uint8_t ch_set_get;


            printf("\n  Rx RSEN Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n Enter RSEN Value : ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_rsen_value_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n RSEN Value Has Been Set");
                    else
                        printf("\n API Error Code Return Value : %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_rsen_value_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n RSEN Value On %d Port Is = %d", port, val);
                    else
                        printf("\n API Error Code Return Value : %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'l':
        {
            enum inv_hdmi_av_link p_val;
            uint8_t port;

            uint32_t ret_val;

            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_av_link_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n The AV_LINK Query on Port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value : %d", ret_val);
            break;
        }
    case 'm':
        {
            enum inv_hdmi_deep_clr p_val;
            uint8_t port;

            uint32_t ret_val;

            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_deep_clr_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n The Deep Color Value on Port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value : %d", ret_val);
            break;
        }
    case 'n':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_avmute_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Avmute value on port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value : %ld", ret_val);
            break;
        }

    case 'o':
        {
            struct inv_video_timing p_val;
            uint8_t port;
            int in_port;
            uint32_t ret_val;

            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &in_port);
            port = (uint8_t) in_port;
            ret_val = inv478x_rx_video_tim_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The Rx input video timing information :-\n");
                printf("Intl   =  %d\n", p_val.intl);
                printf("Hsync  =  %d\n", p_val.h_syn);
                printf("Vsync  =  %d\n", p_val.v_syn);
                printf("Hact   =  %d\n", p_val.h_act);
                printf("Vact   =  %d\n", p_val.v_act);
                printf("Hfnt   =  %d\n", p_val.h_fnt);
                printf("Vfnt   =  %d\n", p_val.v_fnt);
                printf("Hbck   =  %d\n", p_val.h_bck);
                printf("Vbck   =  %d\n", p_val.v_bck);
                printf("Vpol   =  %d\n", p_val.v_pol);
                printf("Hpol   =  %d\n", p_val.h_pol);
                printf("Pclk   =  %d\n", p_val.pclk);
            } else {
                printf("\n API Error Code Return Value : %ld", ret_val);
            }
            break;
        }
    case 'p':
        {
            uint8_t inv_port;
            struct inv_video_pixel_fmt p_val;
            uint32_t ret_val;

            printf("\n **Choose Rx Port 0/1 :");
            scanf("%d", &inv_port);

            ret_val = inv478x_rx_pixel_fmt_query(inst, inv_port, &p_val);
            if (!ret_val) {
                printf("\n Rx Pixel Format on port %d is-", (uint8_t) inv_port);
                printf("\n Pixel Colorimetry is = %d", p_val.colorimetry);
                printf("\n Pixel Chroma Sample is = %d", p_val.chroma_smpl);
                printf("\n Pixel Full Range(enable/Disable) is = %d", (int) p_val.full_range);
                printf("\n Pixel Bitdepth is = %d", p_val.bitdepth);
            } else {
                printf("\n API Error Code Return Value : %ld", ret_val);
            }
            break;
        }
    case 'q':
        {
            uint8_t port;
            enum inv_audio_fmt p_val;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_audio_fmt_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Rx Audio Format Value on Rx Port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %ld", ret_val);
            break;
        }
    case 'r':
        {
            uint8_t port, i;
            struct inv_audio_info p_val;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_audio_info_query(inst, port, &p_val);

            if (!ret_val) {
                printf("\n The Channel Status Data is :");
                for (i = 0; i < 9; i++)
                    printf(" %d\t", p_val.cs.b[i]);
                printf("\n The Audio Map Data is :");
                for (i = 0; i < 10; i++)
                    printf("%d\t", p_val.map.b[i]);
            } else {
                printf("\n API Error Code Return Value = %ld", ret_val);
            }
            break;
        }
    case 's':
        {
            uint8_t port;
            struct inv_hdmi_audio_ctsn p_val;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_rx_audio_ctsn_query(inst, port, &p_val);

            if (!ret_val) {
                printf(" Rx Audio CST Value on Rx Port %d is = 0x %06x\n", port, p_val.cts);
                printf("\n Rx Audio N Value on Rx Port %d is = 0x %06x\n", port, p_val.n);
            } else {
                printf("\n API Error Code Return Value = %ld", ret_val);
            }
            break;
        }
    case 't':
        {
            uint8_t port, i;
            enum inv_hdmi_packet_type type;
            uint8_t p_packet[31];
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);
            printf("\n Choose the packet type(0 to 16):");
            scanf("%d", &type);
            ret_val = inv478x_rx_packet_query(inst, port, type, 31, p_packet);
            if (!ret_val) {
                printf("\n The packet type number = %d", type);
                printf("\n The packet data is-\n");
                for (i = 0; i < 31; i++) {
                    printf("%02X ", p_packet[i]);
                    if (i % 15 == 0 && i != 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'u':
        {
            uint8_t port;
            uint32_t ret_val, p_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_rx_packet_act_stat_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Rx packet active status value = %lX", p_val);
            else
                printf("\n API Error Code Return Value = %ld", ret_val);
            break;
        }
    case 'v':
        {
            uint8_t port;
            uint32_t ret_val, p_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_rx_packet_evt_stat_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Rx packet event status value = %lX", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'w':
        {
            uint8_t port;
            uint32_t ret_val, p_val;

            uint8_t ch_set_get;


            printf("\n  Rx Packet Event Mask Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);
                    printf("\n Enter 32-bit mask value in hexadecimal:");
                    scanf("%x", &p_val);
                    ret_val = inv478x_rx_packet_evt_mask_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Rx packet event mask value is = %lX", p_val);
                    else
                        printf("\n API Error Code Return Value : %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);
                    ret_val = inv478x_rx_packet_evt_mask_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Rx packet event mask value is = %lX", p_val);
                    else
                        printf("\n API Error Code Return Value : %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'x':
        {
            enum inv_hdcp_type val;
            uint8_t rx_port;
            uint32_t ret_val;

            uint8_t ch_set_get;


            printf("\n  Rx HDCP Capability Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n Enter the Rx Port Value(0/1) : ");
                    scanf("%d", &rx_port);
                    printf("\n HDCP Capability Value(0/1/2) : ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_hdcp_cap_set(inst, rx_port, &val);
                    if (!ret_val)
                        printf("\n Rx HDCP Capability Has Been Set");
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n Enter the Rx Port Value(0/1) : ");
                    scanf("%d", &rx_port);
                    ret_val = inv478x_rx_hdcp_cap_get(inst, rx_port, &val);
                    if (!ret_val)
                        printf("\n Rx HDCP Capability Value On Rx Port %d is : %d", rx_port, val);
                    else
                        printf("\n API Error Code Return Value = %ld", ret_val);
                    break;
                }
            }
            break;
        }
    case 'y':
        {
            uint8_t port;
            uint32_t ret_val;
            enum inv_hdcp_type p_val = 0;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_rx_hdcp_type_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Rx HDCP type = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'z':
        {
            uint8_t port;
            struct inv_hdcp_csm p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_rx_hdcp_csm_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n Rx HDCP CSM Value On Port %d is-\n", port);
                printf("\n seq_num_m = %02X, stream_id_type = %02X\n", p_val.seq_num_m, p_val.stream_id_type);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'D':
        {
            uint8_t port;
            struct inv_hdcp_top p_val;
            uint32_t ret_val;

            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);


            ret_val = inv478x_rx_hdcp_topology_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n Rx HDCP Toplogy Detail On Port %d is\n", port);
                printf("\n hdcp_1device_ds = %d", p_val.hdcp1_dev_ds);
                printf("\n hdcp20_repeater_ds = %d", p_val.hdcp2_rptr_ds);
                printf("\n max_cascade_exceeded = %d", p_val.max_cas_exceed);
                printf("\n max_devs_exceeded = %d", p_val.max_devs_exceed);
                printf("\n device_count = %d", p_val.dev_count);
                printf("\n depth = %d", p_val.rptr_depth);
                printf("\n seq_num_v = 0x%02X", p_val.seq_num_v);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'E':
        {
            uint8_t port, len, i, j;
            struct inv_hdcp_ksv p_val[16];
            uint32_t ret_val;

            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter the length : ");
            scanf("%d", &len);


            ret_val = inv478x_rx_hdcp_ksvlist_query(inst, port, len, p_val);
            if (!ret_val) {
                printf("\n Tx HDCP KSV List on Port %d is-\n", port);
                for (j = 0; j < len; j++) {
                    for (i = 0; i < 5; i++) {
                        printf("%02X ", p_val[j].b[i]);
                    }
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'F':
        {
            struct inv_hdcp_ksv p_val;
            uint8_t port, i;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_hdcp_bksv_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The BKSV Packet Content is-\n");
                for (i = 0; i < 5; i++)
                    printf("%02X ", p_val.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'G':
        {
            struct inv_hdcp_ksv p_val;
            uint8_t port, i;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_hdcp_rxid_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The Rxid Packet Content is-\n");
                for (i = 0; i < 5; i++)
                    printf("%02X ", p_val.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'H':
        {
            struct inv_hdcp_ksv p_val;
            uint8_t port, i;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_hdcp_aksv_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The AKSV Packet Content is-\n");
                for (i = 0; i < 5; i++)
                    printf("%02X ", p_val.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'I':
        {
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_hdcp_trash_req(inst, port);
            if (!ret_val)
                printf("\n Rx HDCP trash request done");
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'J':
        {
            uint8_t p_val;
            uint8_t port, offset;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter offset value in hexadecimal:");
            scanf("%x", &offset);
            ret_val = inv478x_rx_scdc_register_query(inst, port, offset, &p_val);
            if (!ret_val)
                printf("\n The SCDC register value on port %d is %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'K':
        {
            enum inv_hdcp_stat p_val;
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_rx_hdcp_stat_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n The HDCP Status Value on Rx Port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'L':
        {
            uint8_t port;
            enum inv_hdcp_mode p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;


            printf("\n  Rx HDCP Mode Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n **Choose HDCP mode 0/1/2 :");
                    scanf("%d", &p_val);
                    ret_val = inv478x_rx_hdcp_mode_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Rx HDCP mode has been set\n");
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Rx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_rx_hdcp_mode_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Rx HDCP mode is = %d", (int) p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    }                           /* switch (key_api_num) ends here */
}                               /* case ends here */

void case_transmitter(void)
{
    uint8_t key_api_num;
    printf("\n a. Tx AV Source");
    printf("\n b. Tx SoC Enable");
    printf("\n c. Tx Av Link");
    printf("\n d. Tx Auto Enable");
    printf("\n e. Tx Reenable, Re-enable The Output");
    printf("\n f. Tx Av Link Query");
    printf("\n g. Tx Deep Color Max");
    printf("\n h. Tx Deep Color Query");
    printf("\n i. Tx Gated By HPD");
    printf("\n j. Tx HPD Query");
    printf("\n k. Tx RSEN Query");
    printf("\n l. Tx EDID Read On HPD");
    printf("\n m. Tx EDID Query");
    printf("\n n. Tx HVSYNC Polarity");
    printf("\n o. Tx Audio Source");
    printf("\n p. Tx Audio Format Query");
    printf("\n q. Tx Audo Info Query");
    printf("\n r. Tx Audio Mute");
    printf("\n s. Tx Audio ctsN query");
    printf("\n t. Tx Packet Insert");
    printf("\n u. Tx Packet Source");
    printf("\n v. Tx Av Mute");
    printf("\n w. Tx AVMUTE Auto Clear");
    printf("\n x. Tx Av Mute Query");
    printf("\n y. Tx HDCP Downstram Capability Query");
    printf("\n z. Tx HDCP Mode");
    printf("\n A. Tx HDCP Auto Approve");
    printf("\n B. Tx HDCP Approve");
    printf("\n C. Tx HDCP Encryption");
    printf("\n D. Tx HDCP Reauth Request");
    printf("\n E. Tx HDCP type query");
    printf("\n F. Tx HDCP status query");
    printf("\n G. Tx HDCP Failure Query");
    printf("\n H. Tx HDCP CSM Value");
    printf("\n I. Tx HDCP Topology Query");
    printf("\n J. Tx HDCP KSV List Query");
    printf("\n K. Tx HDCP AKSV Query");
    printf("\n L. Tx HDCP Blank Color");
    printf("\n M. Tx TMDS2 Scramble340");
    printf("\n N. Tx TMDS2 Scramble Query");
    printf("\n O. Tx SCDC Read Request Enable");
    printf("\n P. Tx SCDC Poll Period");
    printf("\n Q. Tx SCDC Write");
    printf("\n R. Tx SCDC Read");
    printf("\n S. Tx DDC Write");
    printf("\n T. Tx DDC Read");
    printf("\n U. Tx DDC Frequency");
    printf("\n V. Tx EDID Status Query");
    printf("\n W. Tx tmds2 1_OVER_4  Query");

    printf("\n Press the character key");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint8_t port;
            uint32_t ret_val;
            enum inv478x_video_src p_val;

            uint8_t ch_set_get;


            printf("\n  Tx AV Source : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n **Choose Video Source(0/1/2/3) :");
                    scanf(" %d", &p_val);
                    ret_val = inv478x_tx_video_src_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx AV Source Select Set Value on Tx Port %d is  = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);


                    ret_val = inv478x_tx_video_src_get(inst, port, &p_val);
                    if (!ret_val) {
                        printf("\n Tx AV Source Select Get Value on Tx Port %d is = %d\n", port, p_val);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'b':
        {
            uint8_t port;
            bool_t val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx SoC Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enable or Disable Tx SoC(0/1) :");
                    scanf("%d", &val);
                    ret_val = 0;    //inv478x_tx_soc_enable_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SoC Enable Value Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = 0;    //inv478x_tx_soc_enable_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SoC Enable Value On %d Port\n", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'c':
        {
            uint8_t port;
            enum inv_hdmi_av_link tx_link_mode_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx AV Link : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n **TX AV Link Set:");
                    printf("\n 0. INV_HDMI_AV_LINK__NONE");
                    printf("\n 1. INV_HDMI_AV_LINK__DVI");
                    printf("\n 2. INV_HDMI_AV_LINK__TMDS1");
                    printf("\n 3. INV_HDMI_AV_LINK__TMDS2");
                    printf("\n 4. INV_HDMI_AV_LINK__FRL4x3");
                    printf("\n 5. INV_HDMI_AV_LINK__FRL4x6");
                    printf("\n 6. INV_HDMI_AV_LINK__FRL4x8");
                    printf("\n 7. INV_HDMI_AV_LINK__FRL4x10");
                    printf("\n 8. INV_HDMI_AV_LINK__UNKNOWN");
                    scanf("%d", &tx_link_mode_val);
                    ret_val = inv478x_tx_av_link_set(inst, port, &tx_link_mode_val);
                    if (!ret_val)
                        printf("\n Tx Link Mode on port %d is = %d", port, tx_link_mode_val);
                    else
                        printf("\n Return Value = %d\n", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_av_link_get(inst, port, &tx_link_mode_val);
                    if (!ret_val)
                        printf("\n Tx Av Link Get Value on port %d is = %d", port, tx_link_mode_val);
                    else
                        printf("\n Return Value = %d\n", ret_val);
                    break;
                }
            }
            break;
        }
    case 'd':
        {
            uint8_t port;
            bool_t val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Auto Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Tx Auto Enable or Disable(0/1) :");
                    scanf("%d", &val);
                    ret_val = inv478x_tx_auto_enable_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx Auto Enable Value Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_auto_enable_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx Auto Enable Value On %d Port\n", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'e':
        {
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_reenable(inst, port);
            if (!ret_val)
                printf("\n Tx Re-Enable Has Been Done On %d Port\n", port);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'f':
        {
            enum inv_hdmi_av_link p_val;
            uint8_t port;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_av_link_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n The TX_AV_LINK Query on Port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'g':
        {
            uint8_t port;
            enum inv_hdmi_deep_clr inv_deep_clr_val;
            uint32_t ret_val;
            uint8_t ch_set_get;


            printf("\n  Tx Deep Color Max : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n 0. INV_DEEP_CLR__NONE");
                    printf("\n 1. INV_DEEP_CLR__30BIT");
                    printf("\n 2. INV_DEEP_CLR__36BIT");
                    printf("\n 3. INV_HDMI_DEEP_CLR__PASSTHRU");
                    printf("\n 4. INV478X_DEEP_CLR__UNKNOWN");
                    printf("\n **Enter dep color number 0/1/2/3/4 : ");
                    scanf("%d", &inv_deep_clr_val);
                    ret_val = inv478x_tx_deep_clr_max_set(inst, port, &inv_deep_clr_val);
                    if (!ret_val)
                        printf("\n The Deep Color on port %d is = %d", port, inv_deep_clr_val);
                    else
                        printf("\n Return Value = %d\n", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);
                    ret_val = inv478x_tx_deep_clr_max_get(inst, port, &inv_deep_clr_val);
                    if (!ret_val)
                        printf("\n **The Deep Color on port %d is = %d", port, inv_deep_clr_val);
                    else
                        printf("\n Return Value = %d\n", ret_val);
                    break;
                }
            }
            break;
        }
    case 'h':
        {
            uint8_t port;
            enum inv_hdmi_deep_clr p_val;
            uint32_t ret_val;
            printf("\n **Choose Tx Port 0/1 :");
            scanf("\n%d", &port);
            ret_val = inv478x_tx_deep_clr_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx Deep Color Query On %d Port is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'i':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Gated By HPD : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter 1 to set or 0 to clear the HPD ");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_gated_by_hpd_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx Gated By HPD Set Value at Port %d is = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_gated_by_hpd_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx Gated By HPD Set Value at Port %d is = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'j':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_hpd_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n The HPD on port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'k':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_rsen_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx RSEN Value on Tx Port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'l':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx EDID Read On HPD : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter 1 to set or 0 to clear ");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_edid_read_on_hpd_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx EDID Read On Port %d is = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_edid_read_on_hpd_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx EDID Read On Port %d is = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'm':
        {
            uint8_t length, offset, block, inv_port;
            uint8_t p_val[256] = { 0 };
            uint8_t i;
            inv_rval_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &inv_port);
            printf("\n **Enter block number 0/1 : ");
            scanf("%d", &block);
            printf("\n **Enter Offset(0 to 127) : ");
            scanf("%d", &offset);
            printf("\n **Enter Length(max(128 - offset) : ");
            scanf("%d", &length);
            ret_val = inv478x_tx_edid_query(inst, inv_port, block, offset, length, p_val);
            if (ret_val == 0) {
                if (length > offset)
                    length = length - offset;
                else if ((length + offset) > 128)
                    length = (128 - offset);
                for (i = 0; i < length; i++) {
                    printf("%02X ", p_val[i]);
                    if ((i + 1) % 16 == 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'n':
        {
            uint8_t port;
            enum inv_video_hvsync_pol val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx HVSYNC Polarity : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter HVSYNC Plarity Value(0-5) :");
                    scanf("%d", &val);
                    ret_val = inv478x_tx_hvsync_pol_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx HVSYNC Polarity Value Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_hvsync_pol_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx HVSYNC Value On %d Port\n", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'o':
        {
            enum inv478x_audio_src p_val;
            uint8_t port;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Audio Source : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n **Choose Source for Tx Audio(0 - 7) :");
                    scanf("%d", &p_val);

                    ret_val = inv478x_tx_audio_src_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx Audio Source On Port %d is = %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);


                    ret_val = inv478x_tx_audio_src_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx Audio Source On Port %d is = %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'p':
        {
            uint8_t port;
            enum inv_audio_fmt p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_audio_fmt_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx Audio Format Value on Tx Port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'q':
        {
            uint8_t port, i;
            struct inv_audio_info p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_audio_info_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The Channel Status Data is :");
                for (i = 0; i < 9; i++)
                    printf(" %d", p_val.cs.b[i]);
                printf("\n The Audio Map Data is :");
                for (i = 0; i < 10; i++)
                    printf(" %d", p_val.map.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'r':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Audio Mute : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n ** Press 1 to Set Audio Mute, 0 to Clear Audio Mute, 0/1 :");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_audio_mute_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n The Audio Mute Value on port %d is %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 :");
                    scanf("\n%d", &port);
                    ret_val = inv478x_tx_audio_mute_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n The Audio Mute Value on port %d is %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 's':
        {
            uint8_t port;
            struct inv_hdmi_audio_ctsn p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_audio_ctsn_query(inst, port, &p_val);
            if (!ret_val) {
                printf(" Tx Audio CTS Value on Tx Port %d is = 0x %06x\n", port, p_val.cts);
                printf("\n Tx Audio N Value on Tx Port %d is = 0x %06x\n", port, p_val.n);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }


    case 'v':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx AV Mute : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n ** Press 1 to Set Av Mute, 0 to Clear Av Mute, 0/1 :");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_avmute_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n The Av Mute Value on port %d is %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_avmute_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n The Av Mute Value on port %d is %d", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'w':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx AV Mute Auto Clear : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enable or Disable Tx AVMUTE Auto(0/1) :");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_avmute_auto_clear_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx AVMUTE Auto Clear Value Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_avmute_auto_clear_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx AVMUTE Auto Clear Value On %d Port is %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'x':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;
            printf("\n **Choose Tx Port 0/1 :");
            scanf("%d", &port);
            ret_val = inv478x_tx_avmute_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx Av Mute Value on Tx Port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'y':
        {
            uint8_t port;
            enum inv_hdcp_type p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_hdcp_ds_cap_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx HDCP DS Cap value on port %d is = %02X", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'z':
        {
            uint8_t port;
            enum inv_hdcp_mode p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx HDCP Mode : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n **Choose Tx HDCP mode, 0/1/2 :");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_hdcp_mode_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\nHDCP mode value on Tx port %d is = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_hdcp_mode_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\nHDCP mode value on Tx port %d is = %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'A':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx HDCP Auto Approve : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enable or Disable HDCP Auto Approve(0/1) :");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_hdcp_auto_approve_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx HDCP Auto Approve Value Has been Set on %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_hdcp_auto_approve_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx HDCP Auto Approve Value On %d Port is %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'B':
        {
            uint8_t port;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);


            ret_val = inv478x_tx_hdcp_approve(inst, port);
            if (!ret_val)
                printf("\n Tx HDCP Approve Value Has Been Set On Port %d\n", port);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'C':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx HDCP Encrypt : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n **Enter HDCP encrypt value(0/1) :");
                    scanf("%d", &p_val);

                    ret_val = inv478x_tx_hdcp_encrypt_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx HDCP Encrypt Value Has Been Set On Port %d\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);


                    ret_val = inv478x_tx_hdcp_encrypt_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx HDCP Encrypt Value On Port %d is %d\n", port, p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'D':
        {
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_hdcp_reauth_req(inst, port);
            if (!ret_val)
                printf("\n Tx HDCP Reauth Request Done on Port %d\n", port);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'E':
        {
            uint8_t port;
            enum inv_hdcp_type p_val = INV_HDCP_TYPE__NONE;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);


            ret_val = inv478x_tx_hdcp_type_query(inst, port, &p_val);
            if (!ret_val)
                printf("\nHDCP type value on Tx port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'F':
        {
            uint8_t port;
            enum inv_hdcp_stat p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_hdcp_stat_query(inst, port, &p_val);
            if (!ret_val)
                printf("\nHDCP status value on Tx port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'G':
        {
            uint8_t port;
            enum inv_hdcp_failure p_val;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_tx_hdcp_failure_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx HDCP Failure Value On Port %d is = 0x%02X\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'H':
        {
            uint8_t port;
            struct inv_hdcp_csm p_val;
            uint32_t ret_val;
            uint8_t ch_set_get;

            printf("\n  Tx HDCP CSM : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter Sequence Number : ");
                    scanf("%ld", &p_val.seq_num_m);
                    printf("\n Enter Stream ID Type : ");
                    scanf("%d", &p_val.stream_id_type);

                    ret_val = inv478x_tx_hdcp_csm_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\n Tx HDCP KSV CSM Value Has Been Set On Port %d\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);


                    ret_val = inv478x_tx_hdcp_csm_get(inst, port, &p_val);
                    if (!ret_val) {
                        printf("\n Tx HDCP KSV CSM Value On Port %d is-\n", port);
                        printf("\n seq_num_m = %02X, stream_id_type = %02X\n", p_val.seq_num_m, p_val.stream_id_type);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'I':
        {
            uint8_t port;
            struct inv_hdcp_top p_val;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);


            ret_val = inv478x_tx_hdcp_topology_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n Tx HDCP Toplogy Detail On Port %d is\n", port);
                printf("\n hdcp_1device_ds = %d", p_val.hdcp1_dev_ds);
                printf("\n hdcp20_repeater_ds = %d", p_val.hdcp2_rptr_ds);
                printf("\n max_cascade_exceeded = %d", p_val.max_cas_exceed);
                printf("\n max_devs_exceeded = %d", p_val.max_devs_exceed);
                printf("\n device_count = %d", p_val.dev_count);
                printf("\n depth = %d", p_val.rptr_depth);
                printf("\n seq_num_v = 0x%02X", p_val.seq_num_v);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'J':
        {
            uint8_t port, len, i, j;
            struct inv_hdcp_ksv p_val[127];
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);
            printf("\n Enter the length : ");
            scanf("%d", &len);
            ret_val = inv478x_tx_hdcp_ksvlist_query(inst, port, len, p_val);
            if (!ret_val) {
                printf("\n Tx HDCP KSV List on Port %d is-\n", port);
                for (j = 0; j < len; j++) {
                    for (i = 0; i < 5; i++) {
                        printf("%02X ", p_val[j].b[i]);
                    }
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'K':
        {
            uint8_t port, i;
            struct inv_hdcp_ksv p_val;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_tx_hdcp_aksv_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n Tx HDCP AKSV List on Port %d is-\n", port);
                for (i = 0; i < 5; i++) {
                    printf("0x%02X ", p_val.b[i]);
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'L':
        {
            uint8_t port;
            struct inv_video_rgb_value val;
            uint32_t ret_val = 0;

            uint8_t ch_set_get;

            printf("\n  Tx HDCP Blank Color : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter Red Color Value : ");
                    scanf("%d", &val.r);
                    printf("\n Enter Green Color Value : ");
                    scanf("%d", &val.g);
                    printf("\n Enter Blue Color Value : ");
                    scanf("%d", &val.b);

                    /*
                     * ret_val = inv478x_tx_hdcp_blank_color_set(inst, port, &val);
                     */
                    if (!ret_val)
                        printf("\n Tx HDCP Blank Color Has Been Set On Port %d\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);


                    /*
                     * ret_val = inv478x_tx_hdcp_blank_color_get(inst, port, &val);
                     */
                    if (!ret_val) {
                        printf("\n Red Value is : %d ", val.r);
                        printf("\n Green Value is : %d ", val.g);
                        printf("\n Blue Value is : %d ", val.b);
                    } else {
                        printf("\n API Error Code Return Value = %d", ret_val);
                    }
                    break;
                }
            }
            break;
        }
    case 'M':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx TMDS2 Scramble340 : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n **Enter enable/disable, 1/0:");
                    scanf("%d", &p_val);
                    ret_val = inv478x_tx_tmds2_scramble340_set(inst, port, &p_val);
                    if (!ret_val)
                        printf("\nTMDS2 scramble set value = %d\n", p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_tmds2_scramble340_get(inst, port, &p_val);
                    if (!ret_val)
                        printf("\nTMDS2 scramble set value = %d\n", p_val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'N':
        {
            uint8_t port;
            bool_t p_val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_tmds2_scramble_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx TMDS2 Scramble Value on Tx Port %d is = %d\n", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'O':
        {
            uint8_t port;
            uint32_t ret_val;
            bool_t val;

            uint8_t ch_set_get;

            printf("\n  Tx SCDC Read Request Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);
                    printf("\n SCDC Read Enable/Disable 0/1 :");
                    scanf("%d", &val);
                    ret_val = inv478x_tx_scdc_rr_enable_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SCDC Read Request Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_scdc_rr_enable_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SCDC Read Value On %d Port is %d\n", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'P':
        {
            uint8_t port;
            uint32_t ret_val;
            uint16_t val;

            uint8_t ch_set_get;

            printf("\n  Tx SCDC Poll Period : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter SCDC Poll Period Value : ");
                    scanf("%d", &val);
                    ret_val = inv478x_tx_scdc_poll_period_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SCDC Poll Period Has Been Done On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_scdc_poll_period_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SCDC Poll Period Has Been Done On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'Q':
        {
            uint8_t port, val, offset;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter The Offset Value : ");
            scanf("%d", &offset);
            printf("\n Enter The Value To Write : ");
            scanf("%d", &val);
            ret_val = inv478x_tx_scdc_write(inst, port, offset, &val);
            if (!ret_val)
                printf("\n Tx SCDC Write Has Been Done On %d Port\n", port);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'R':
        {
            uint8_t port, offset, val;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter The Address Value : ");
            scanf("%d", &offset);
            ret_val = inv478x_tx_scdc_read(inst, port, offset, &val);
            if (!ret_val)
                printf("\n Tx SCDC Value At Address %d On %d Port is : ", offset, port, val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'S':
        {
            uint8_t port, val[256], slv_addr, size, i, offset;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter The Slave Address Value(In Hexadecimal): ");
            scanf("%d", &slv_addr);
            printf("\n Enter The Offset Value(In Hexadecimal): ");
            scanf("%d", &offset);
            printf("\n Enter The Size : ");
            scanf("%d", &size);
            printf("\nEnter %d Data Bytes In Hexadecimal : ", size);
            for (i = 0; i < size; i++) {
                scanf("%X", &val[i]);
            }
            ret_val = inv478x_tx_ddc_write(inst, port, slv_addr, offset, size, val);
            if (!ret_val)
                printf("\n Tx DDC Write Has Been Done On %d Port\n", port);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'T':
        {
            uint8_t i, port, slv_addr, val[256], size, offset;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter The Address Value(In Hexadecimal) : ");
            scanf("%X", &slv_addr);
            printf("\n Enter The Offset Value(In Hexadecimal): ");
            scanf("%X", &offset);
            printf("\n Enter The Size Value : ");
            scanf("%d", &size);
            ret_val = inv478x_tx_ddc_read(inst, port, slv_addr, offset, size, val);
            if (!ret_val) {
                printf("\n Tx DDC Value For Slave Address %d With Offset %d On %d Port is : ", slv_addr, offset, port);
                for (i = 0; i < size; i++) {
                    printf("%02X ", val[i]);
                    if (i != 0 && i % 15 == 0) {
                        printf("\n");
                    }
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'U':
        {
            uint8_t port;
            uint32_t ret_val;
            uint8_t val;

            uint8_t ch_set_get;

            printf("\n  Tx DDC Frequency : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Enter DDC Frequency Value :");
                    scanf("%d", &val);
                    ret_val = inv478x_tx_ddc_freq_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx DDC frequency Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    ret_val = inv478x_tx_ddc_freq_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx DDC Frequency Value On %d Port is %d\n", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    case 'V':
        {
            uint8_t p_val, port;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_tx_edid_stat_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Tx EDID Status Value is = 0x%X\n", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'W':
        {
            uint8_t port;
            uint8_t p_val = 0;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_tx_tmds2_1over4_query(inst, port, &p_val);
            if (!ret_val)
                printf("\nTx 1OVER4 Div Ratio is = %d\n", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    default:
        break;
    }                           /* switch (key_api_num) for Tx ends here */
    /*
     * case 2 ends here
     */
}

void case_audio(void)
{
    uint8_t key_api_num;
    printf("\n a. AP Configuration Set");
    printf("\n b. AP Configuration Get");
    printf("\n c. AP Mode Set");
    printf("\n d. AP Mode Get");
    printf("\n e. Ap rx format set");
    printf("\n f. Ap rx format get");
    printf("\n g. Ap rx info set");
    printf("\n h. Ap rx info get");
    printf("\n i. Ap Tx Audio Source Set");
    printf("\n j. Ap Tx Audio Source Get ");
    printf("\n k. Ap Tx fmt query");
    printf("\n l. Ap Tx info query");
    printf("\n m. Ap tx mute set");
    printf("\n n. Ap tx mute get");
    printf("\n o. AP MCLK Set");
    printf("\n p. AP MCLK Get");
    printf("\n Press the character key");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            enum inv478x_ap_port_cfg val;
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter The Audio Port Configuration Value(0/1/2) : ");
            scanf("%d", &val);
            ret_val = inv478x_ap_cfg_set(inst, port, &val);
            if (!ret_val) {
                printf("\n The AP Configuration Set Done On %d Port", port);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'b':
        {
            enum inv478x_ap_port_cfg val;
            uint8_t port;
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_ap_cfg_get(inst, port, &val);
            if (!ret_val) {
                printf("\n The AP Configuration Value On %d Port Is : %d", val);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'c':
        {
            uint8_t port;
            enum inv_audio_ap_mode p_val;
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            printf("\n0.INV_AP_MODE__NONE");
            printf("\n1.INV_AP_MODE__SPDIF");
            printf("\n2.INV_AP_MODE__I2S");
            printf("\n3.INV478X_AP_MODE__TDM");
            printf("\n Choose AP Mode : ");
            scanf("%d", &p_val);

            ret_val = inv478x_ap_mode_set(inst, port, &p_val);
            if (!ret_val)
                printf("\n AP Mode value on port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'd':
        {
            uint8_t port;
            enum inv_audio_ap_mode p_val;
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_ap_mode_get(inst, port, &p_val);
            if (!ret_val)
                printf("\n AP Mode value on port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'e':
        {
            uint8_t port;
            enum inv_audio_fmt p_val;
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            printf("\n0.INV_AUDIO_FMT__NONE");
            printf("\n1.INV_AUDIO_FMT__ASP8");
            printf("\n2.INV_AUDIO_FMT__ASP16");
            printf("\n3.INV_AUDIO_FMT__ASP32");
            printf("\n4.INV_AUDIO_FMT__HBRA");
            printf("\n5.INV_AUDIO_FMT__DSD6");
            printf("\n Choose Audio Format :");
            scanf("%d", &p_val);
            ret_val = inv478x_ap_rx_fmt_set(inst, port, &p_val);
            if (!ret_val)
                printf("\n Audio Format value on rx port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'f':
        {
            uint8_t port;
            enum inv_audio_fmt p_val;
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_ap_rx_fmt_get(inst, port, &p_val);
            if (!ret_val)
                printf("\n Audio Format value on rx port %d is = %02X", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'g':
        {
            uint8_t port, i;
            struct inv_audio_info p_val;
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            printf("\n Enter 9 Bytes of Channel Status :");
            for (i = 0; i < 9; i++)
                scanf("%d", &p_val.cs.b[i]);
            printf("\n Enter 7 Bytes of Audio Map :");
            for (i = 0; i < 10; i++)
                scanf("%d", &p_val.map.b[i]);
            ret_val = inv478x_ap_rx_info_set(inst, port, &p_val);
            if (!ret_val)
                printf("\n Audio info value has been set on port %d is = %d", port);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'h':
        {
            uint8_t port, i;
            struct inv_audio_info p_val;
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_ap_rx_info_get(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The Channel Status Data is :");
                for (i = 0; i < 9; i++)
                    printf(" %d", p_val.cs.b[i]);
                printf("\n The Audio Map Data is :");
                for (i = 0; i < 10; i++)
                    printf("%d", p_val.map.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'i':
        {
            uint8_t val;
            uint32_t ret_val;
            enum inv478x_audio_src p_val = INV478X_AUDIO_SRC__RX0;
            printf("\n  Choose audio src for i2s out, press a number between 0 to 5:\n");
            printf("    0: NONE\n");
            printf("    1: EARC\n");
            printf("    2: RX\n");
            printf("    3: RX0\n");
            printf("    4: RX1\n");
            printf("    5: AP0\n");
            printf("    6: AP1\n");
            printf("    7: APG\n");
            printf("    Audio src for ap tx ");
            val = (uint8_t) getchar();
            switch (val) {
            case '0':
                p_val = INV478X_AUDIO_SRC__NONE;
                break;
            case '1':
                p_val = INV478X_AUDIO_SRC__EARC;
                break;
            case '2':
                p_val = INV478X_AUDIO_SRC__RX;
                break;
            case '3':
                p_val = INV478X_AUDIO_SRC__RX0;
                break;
            case '4':
                p_val = INV478X_AUDIO_SRC__RX1;
                break;
            case '5':
                p_val = INV478X_AUDIO_SRC__AP0;
                break;
            case '6':
                p_val = INV478X_AUDIO_SRC__AP1;
                break;
            case '7':
                p_val = INV478X_AUDIO_SRC__ATPG;
                break;
            }

            ret_val = inv478x_ap_tx_src_set(inst, 0, &p_val);
            if (!ret_val)
                printf("\n Ap Tx Audio Source Set = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'j':
        {
            /*
             * i2s audio src get
             */
            enum inv478x_audio_src p_val;
            uint32_t ret_val;

            ret_val = inv478x_ap_tx_src_get(inst, 0, &p_val);
            if (!ret_val)
                printf("\n The i2s audio src value = %d", (uint8_t) p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'k':
        {
            uint8_t port;
            enum inv_audio_fmt p_val;
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_ap_tx_fmt_query(inst, port, &p_val);
            if (!ret_val)
                printf("\n Audio Format value on port %d is = %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'l':
        {
            uint8_t port, i;
            struct inv_audio_info p_val;
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_ap_tx_info_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The Channel Status Data is :");
                for (i = 0; i < 9; i++)
                    printf(" %d", p_val.cs.b[i]);
                printf("\n The Audio Map Data is :");
                for (i = 0; i < 10; i++)
                    printf("%d", p_val.map.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'm':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            printf("\n ** Press 0 to Mute HPD, 1 to Unmute, 0/1 :");
            scanf("%d", &p_val);
            ret_val = inv478x_ap_tx_mute_set(inst, port, &p_val);
            if (!ret_val)
                printf("\n The mute value on AP Tx port %d is %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'n':
        {
            bool_t p_val;
            uint8_t port;
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            ret_val = inv478x_ap_tx_mute_get(inst, port, &p_val);
            if (!ret_val)
                printf("\n The HPD value on port %d is %d", port, p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'o':
        {
            enum inv_audio_mclk val;
            uint32_t ret_val;
            uint8_t port;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter The MCLK Value(0 - 3) : ");
            scanf("%d", &val);
            ret_val = inv478x_ap_mclk_set(inst, port, &val);
            if (!ret_val) {
                printf("\n The AP MCLK Set Done");
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'p':
        {
            enum inv_audio_mclk val;
            uint32_t ret_val;
            uint8_t port;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            ret_val = inv478x_ap_mclk_get(inst, port, &val);
            if (!ret_val) {
                printf("\n The AP MCLK Value Is : %d", val);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    default:
        break;
    }                           /* switch (key_api_num) for Audio ends here */
    /*
     * case 4 ends here
     */
}

void case_earc(void)
{
    uint8_t key_api_num;;
    printf("\n a. EARC Configuration Set");
    printf("\n b. EARC Configuration Get");
    printf("\n c. EARC Mode Set");
    printf("\n d. EARC Mode Get");
    printf("\n e. Earc Tx Audio Source Set");
    printf("\n f. Earc Tx Audio Source Get");
    printf("\n g. EARC Link Query");
    printf("\n h. EARC Audio Format Query");
    printf("\n i. EARC Audio Info. Query");
    printf("\n j. EARC ERX Latency Set");
    printf("\n k. EARC ERX Latency Get");
    printf("\n l. EARC ERX Latency Query");
    printf("\n m. EARC Rxcap Struct Set");
    printf("\n n. EARC Rxcap Struct Get");
    printf("\n o. EARC Rxcap Struct Query");
    printf("\n v. EARC Audio Mute Set ");
    printf("\n w. EARC Audio Mute Get ");
    printf("\n x. EARC Packet Set");
    printf("\n y. EARC Packet Get");
    printf("\n z. EARC Packet Query");
    printf("\n Press the character key");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            enum inv478x_earc_port_cfg val;
            uint32_t ret_val;

            printf("\n Enter The EARC Configuration Value(0/1/2/3/4) : ");
            scanf("%d", &val);
            ret_val = inv478x_earc_cfg_set(inst, &val);
            if (!ret_val) {
                printf("\n The EARC Configuration Set Done");
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'b':
        {
            enum inv478x_earc_port_cfg val;
            uint32_t ret_val;

            ret_val = inv478x_earc_cfg_get(inst, &val);
            if (!ret_val) {
                printf("\n The EARC Configuration Value Is : %d", val);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'c':
        {
            enum inv_hdmi_earc_mode val;
            uint32_t ret_val;

            printf("\n Enter The EARC Mode(0/1/2) : ");
            scanf("%d", &val);
            ret_val = inv478x_earc_mode_set(inst, &val);
            if (!ret_val)
                printf("\nEARC Mode Has Been Set");
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'd':
        {
            enum inv_hdmi_earc_mode val;
            uint32_t ret_val;

            ret_val = inv478x_earc_mode_get(inst, &val);
            if (!ret_val)
                printf("\nEARC Mode Value is = %d", val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'e':
        {
            enum inv478x_audio_src p_val = INV478X_AUDIO_SRC__RX0;
            uint32_t ret_val;

            printf("\n **Choose Source for EARC Tx(0 - 7) :");
            scanf("%d", &p_val);
            ret_val = inv478x_earc_tx_src_set(inst, &p_val);
            if (!ret_val)
                printf("\n Arc Tx Audio Source Set = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'f':
        {
            enum inv478x_audio_src p_val;
            uint32_t ret_val;

            ret_val = inv478x_earc_tx_src_get(inst, &p_val);
            if (!ret_val)
                printf("\n EARC Tx Audio Source Get Value = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'g':
        {
            bool_t p_val;
            uint32_t ret_val;

            ret_val = inv478x_earc_link_query(inst, &p_val);
            if (!ret_val)
                printf("\n EARC Link Value is = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'h':
        {
            enum inv_audio_fmt p_val;
            uint32_t ret_val;

            ret_val = inv478x_earc_fmt_query(inst, &p_val);
            if (!ret_val)
                printf("\n EARC Mode Value is = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'i':
        {
            uint8_t i;
            struct inv_audio_info p_val;
            uint32_t ret_val;

            ret_val = inv478x_earc_info_query(inst, &p_val);
            if (!ret_val) {
                printf("\n The Channel Status Data is :");
                for (i = 0; i < 9; i++)
                    printf(" %d", p_val.cs.b[i]);
                printf("\n The Audio Map Data is :");
                for (i = 0; i < 10; i++)
                    printf("%d", p_val.map.b[i]);
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'j':
        {
            uint8_t p_val;
            uint32_t ret_val;

            printf("\n Enter EARC Latency Value :");
            scanf("%d", &p_val);

            ret_val = inv478x_earc_erx_latency_set(inst, &p_val);
            if (!ret_val)
                printf("\n EARC Latency Value Set is = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'k':
        {
            uint8_t p_val;
            uint32_t ret_val;

            ret_val = inv478x_earc_erx_latency_get(inst, &p_val);
            if (!ret_val)
                printf("\n EARC Latency Value is = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'l':
        {
            uint8_t p_val;
            uint32_t ret_val;

            ret_val = inv478x_earc_erx_latency_query(inst, &p_val);
            if (!ret_val)
                printf("\n EARC Latency Value is = %d", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'm':
        {
            uint8_t p_val[256], len, offset, i;
            uint32_t ret_val;

            printf("\n Enter Offset Value :");
            scanf("%d", &offset);
            printf("\n Enter Length Value :");
            scanf("%d", &len);
            printf("\n Enter %d elements for P_Val :", len);
            for (i = 0; i < len; i++)
                scanf("%d", &p_val[i]);

            ret_val = inv478x_earc_rxcap_struct_set(inst, offset, len, p_val);
            if (!ret_val) {
                printf("\n Offset Value Set is = %d", offset);
                printf("\n Length Value Set is = %d", len);
                printf("\n P_Val Value Set is-\n");
                for (i = 0; i < len; i++) {
                    printf("%d ", p_val[i]);
                    if (i % 16 == 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'n':
        {
            uint8_t p_val[256], len, offset, i;
            uint32_t ret_val;

            printf("\n Enter Offset Value :");
            scanf("%d", &offset);
            printf("\n Enter Length Value :");
            scanf("%d", &len);
            ret_val = inv478x_earc_rxcap_struct_get(inst, offset, len, p_val);
            if (!ret_val) {
                printf("\n Offset Value Get is = %d", offset);
                printf("\n Length Value Get is = %d", len);
                printf("\n P_Val Value Get is-\n");
                for (i = 0; i < len; i++) {
                    printf("%d ", p_val[i]);
                    if (i % 16 == 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'o':
        {
            uint8_t p_val[256], len, offset, i;
            uint32_t ret_val;


            printf("\n Enter Offset Value :");
            scanf("%d", &offset);
            printf("\n Enter Length Value :");
            scanf("%d", &len);


            ret_val = inv478x_earc_rxcap_struct_query(inst, offset, len, p_val);
            if (!ret_val) {
                printf("\n Offset Value Get is = %d", offset);
                printf("\n Length Value Get is = %d", len);
                printf("\n P_Val Value Get is-\n");
                for (i = 0; i < len; i++) {
                    printf("%02X ", p_val[i]);
                    if (i % 16 == 0 && i != 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'v':
        {
            /*
             * earc aud mute set
             */
            bool_t aud_mute;
            uint32_t ret_val;

            printf("\n Press 0 to unmute earc audio");
            printf("\n Press 1 to mute earc audio");
            printf("\nEnter your choice :");
            scanf("%d", &aud_mute);
            ret_val = inv478x_earc_aud_mute_set(inst, &aud_mute);
            if (!ret_val)
                printf("\n The earc audio mute set to %d", (int) aud_mute);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'w':
        {
            /*
             * earc aud mute get
             */
            bool_t aud_mute;
            uint32_t ret_val;

            ret_val = inv478x_earc_aud_mute_get(inst, &aud_mute);
            if (!ret_val) {
                if (aud_mute)
                    printf("\n The earc audio is MUTED");
                else
                    printf("\n The earc audio is UN-MUTED");
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'x':
        {
            uint8_t port, i, len;
            enum inv_hdmi_packet_type type;
            uint8_t packet[31];
            uint32_t ret_val;

            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);
            printf("\n Enter the packet type(1 to 12) : ");
            scanf("%d", &type);
            printf("\n Enter the packet length : ");
            scanf("%d", &len);
            printf("\n Enter the packet data : ");
            for (i = 0; i < len; i++) {
                scanf("%d", &packet[i]);
            }
            ret_val = inv478x_earc_packet_set(inst, type, len, packet);
            if (!ret_val) {
                printf("\n EARC Packet Set Done");
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'y':
        {
            uint8_t port, i, len;
            enum inv_hdmi_packet_type type;
            uint8_t packet[31];
            uint32_t ret_val;


            printf("\n **Choose AP Port 0/1 : ");
            scanf("%d", &port);

            printf("\n Enter the packet type(1 to 12) : ");
            scanf("%d", &type);
            printf("\n Enter the packet length : ");
            scanf("%d", &len);
            ret_val = inv478x_earc_packet_get(inst, type, len, packet);
            printf("\n API Error Code Return Value = %d", ret_val);
            if (!ret_val) {
                printf("\n The packet data is-\n");
                for (i = 0; i < 31; i++) {
                    printf("%02X ", packet[i]);
                    if (i % 15 == 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'z':
        {
            uint8_t i, len;
            enum inv_hdmi_packet_type type;
            uint8_t packet[31];
            uint32_t ret_val;

            printf("\n Enter the packet type(1 to 12) : ");
            scanf("%d", &type);
            printf("\n Enter the packet length : ");
            scanf("%d", &len);
            ret_val = inv478x_earc_packet_query(inst, type, len, packet);
            printf("\n API Error Code Return Value = %d", ret_val);
            if (!ret_val) {
                printf("\n The packet data is-\n");
                for (i = 0; i < 31; i++) {
                    printf("%02X ", packet[i]);
                    if (i % 15 == 0)
                        printf("\n");
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    default:
        break;
    }                           /* switch (key_api_num) for Audio ends here */
    /*
     * case 4 ends here
     */
}

void case_debug(void)
{
    uint8_t key_api_num;

    printf("\n a. INV478X Register Write");
    printf("\n b. INV478X Register Read");
    printf("\n Press the character key");

    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val;
            uint16_t addr;
            uint8_t size, val[256], i;

            printf("\n Enter Register Address(in Hexadecimal) : ");
            scanf("%X", &addr);
            printf("\n Enter Size : ");
            scanf("%d", &size);
            printf("\n Enter the %d values in hexadecimal : ", size);
            for (i = 0; i < size; i++) {
                scanf("%X", &val[i]);
            }
            ret_val = inv478x_reg_write(inst, addr, size, val);
            if (!ret_val) {
                printf("\n The Values Has Been Written");
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    case 'b':
        {
            uint32_t ret_val;
            uint16_t addr;
            uint8_t size, val[256], i;


            printf("\n Enter Register Address(in Hexadecimal) : ");
            scanf("%X", &addr);
            printf("\n Enter Size : ");
            scanf("%d", &size);

            ret_val = inv478x_reg_read(inst, addr, size, val);
            if (!ret_val) {
                printf("\n The Register Values Are : \n");
                for (i = 0; i < size; i++) {
                    printf("Addr %X : Value %X\n", (addr + i), val[i]);
                }
            } else {
                printf("\n API Error Code Return Value = %d", ret_val);
            }
            break;
        }
    default:
        break;
    }

}

void case_cec(void)
{
    uint8_t key_api_num;
    printf("\n a. CEC Tx Create ");
    printf("\n b. CEC Rx Create ");
    printf("\n Press the character key\n");
    key_api_num = (uint8_t) getchar();
    switch (key_api_num) {
    case 'a':
        {
            inv_cec_tx_delete();
            inv_cec_rx_delete();
            inst_9612 = inv_cec_tx_create(1, &inv9612_app_tx_notification, adpt_inst_9612_tx);
            arc_tx_flag = 0;
            arc_rx_flag = 1;
            printf("\nCEC Tx Create done");
            cec_created = 1;
            break;
        }
    case 'b':
        {
            inv_cec_tx_delete();
            inv_cec_rx_delete();
            inv_cec_rx_create(2, &inv9612_app_rx_notification, adpt_inst_9612_rx);
            arc_tx_flag = 1;
            arc_rx_flag = 0;
            printf("\nCEC Rx Create done");
            cec_created = 1;
            break;
        }
    }
}

void s_cases(void)
{
    uint8_t key_module_num;
    printf("\n*************************************");
    printf("\n Press any key to start CEC initialization");
    printf("\n *************************************");
    key_module_num = (uint8_t) getchar();
    case_cec();
}
int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

int main(void)
{
	int temp;
	hi_s32 ret = HI_FAILURE;	
	hi_u8 *data = HI_NULL;

    hi_u32 device_address = 0;
    hi_u32 i2c_num  = 0;
    hi_u32 reg_addr = 0;
    hi_u32 reg_addr_count = 0;
    hi_u32 read_number = 0;
    hi_unf_i2c_addr_info i2c_addr_info;

	printf("Copyright Invecas Inc, 2019\n");
    printf("Build Time: " __DATE__ ", " __TIME__ "\n");



    ret = hi_unf_i2c_init();
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }
	
/*  create_478x_adapter(s_adapter_list_query(), 0x40);
	//scanf("%d", &temp);
	printf("Build Time test00: " __DATE__ ", " __TIME__ "\n");
    create_tx_9612_adapter(0, 0x30);
	printf("Build Time test11: " __DATE__ ", " __TIME__ "\n");*/
	
    if (sTest < 0) {
        /*
         * System infinite while loop
         */
        printf("System is running ...\n");
        printf("\n(Press q to abort program, s to test the host APIs)\n");

        while (sbRunning) {
            /*
             * Below API has been called to show the boot change notificaton
             */
            //inv478x_boot_stat_query(inst, &stat_val);
            //if (inv_hal_aardvark_interrupt_query())
           /* {
                if (irq_detect || irq_poll) {
                    inv478x_handle(inst);
                    irq_detect = false;
                }
                if (arc_tx_flag & cec_created) {
                    inv_cec_rx_handle();
                }
                if (arc_rx_flag & cec_created) {
                    inv_cec_tx_handle();
                }
            }*/
            /*----------------------------------------------------------- */
            /*
             * Insert here other application handlers
             */
            /*----------------------------------------------------------- */

            if (kbhit()) {
                int key = getchar();
                switch (key) {
                case 'q':
                    sbRunning = false;
                    break;
                case 's':
                    system("CLS");
                    s_cases();
                    break;
                case 't':
                    if (adpt_inst_9612_tx) {
                        inv_adapter_delete(adpt_inst_9612_tx);
                    }
                    s_cases();
                    break;
                case 'r':
                    if (adpt_inst_9612_rx) {
                        inv_adapter_delete(adpt_inst_9612_rx);
                    }
                    s_cases();
                    break;
                case 'd':
                    if (adpt_inst_478x) {
                        inv_adapter_delete(adpt_inst_478x);
                    }
                    s_cases();
                    break;
                case 'c':
                    create_478x_adapter(s_adapter_list_query(), 0x40);
                    //adpt_inst_478x = NULL;
                    s_cases();
                    break;
                }               /* end of switch(key) */

            }

        }                       /* end of while */

    }                           /* end of if (sTest < 0) */
    printf("\nSystem is stopped\n");

    /*
     * API host destructor for Inv478xrx product
     */
    inv478x_delete(inst);

    /*
     * API host destructor for Inv478xrx product
     */
    if (adpt_inst_478x) {
        inv_adapter_delete(adpt_inst_478x);
    }
    if (adpt_inst_9612_tx) {
        inv_adapter_delete(adpt_inst_9612_tx);
    }
    if (adpt_inst_9612_rx) {
        inv_adapter_delete(adpt_inst_9612_rx);
    }
	
	/* Read data from Device */
  /*  ret = hi_unf_i2c_write(i2c_num, &i2c_addr_info, data, 10);
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("i2c write failed!\n");
    } else {
        HI_INFO_I2C("i2c write success!\n");
    }
	ret = hi_unf_i2c_read(i2c_num, &i2c_addr_info, data, read_number);
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("call HI_I2C_Read failed.\n");
        free(data);
        hi_unf_i2c_deinit();

        return ret;
    }*/
    
	hi_unf_i2c_deinit();
    return 0;
}                               /* end of main function */
