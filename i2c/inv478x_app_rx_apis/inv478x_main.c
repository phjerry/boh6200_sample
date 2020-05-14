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
#include <conio.h>
#include <windows.h>
#include "inv478x_api.h"
//#include "inv478x_platform_api.h"
#include "inv478x_app_def.h"
#include "inv_drv_cra_api.h"
#include "inv_common.h"
#include "inv_adapter_api.h"

/***** global variables *******************************************************/

/***** local macro definitions ***********************************************/

#define INV_CALLBACK__HANDLER
#define PACKET_SIZE    INV478X_USER_DEF__ISP_CASH_SIZE

/***** local variables *******************************************************/

static inv_inst_t inst = INV_INST_NULL;

static inv_inst_t adpt_inst_478x = INV_INST_NULL;

static struct inv_adapter_cfg cfg_478x;
static char **adapter_list;
static bool_t sbRunning = true;
static int sTest = -1;


/* Static configuratyion object for host driver configuration */
static struct inv478x_config sInv478xConfig = {
    "478x-Rx",                  /* pNameStr[8]         - Name of Object */
    true,                       /* bDeviceReset       - */
};

/***** local functions definitions ***********************************************/

static bool_t s_check_boot_stat(void);
static void inv478x_event_callback_func(inv_inst_t inst1, inv478x_event_flags_t event_flags);
static void s_create_thread(void);

static void s_log_version_information(void);
static bool_t irq_detect = false;
static bool_t irq_poll = false;

/***** local functions *******************************************************/
void s_adapter_478x_interrupt(inv_inst_t inst1)
{
    inst1 = inst1;
    if (!irq_poll) {
        //printf("\n---s_adapter_478x_interrupt--\n");
        inv478x_handle(inst);
    }
}

static void s_adapter_478x_callback(inv_inst_t inst1)
{
    inst1 = inst1;
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_478x) {
        printf("adapter_478x_callback: %s\n", inv_adapter_connect_query(adpt_inst_478x) ? "adapter is connected" : "adapter is disconnected");

    }
#endif
}

static void s_adapter_478x_callback_two(void)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_478x) {
        printf("adapter_478x_callback_two: %s\n", inv_adapter_connect_query(adpt_inst_478x) ? "adapter is connected" : "adapter is disconnected");

    }
#endif
}

static uint8_t s_adapter_list_query(void)
{
    uint8_t id = 0;
    int temp;
    char **p_ptr, **p_ptr_cpy;
    adapter_list = inv_adapter_list_query();
    p_ptr = p_ptr_cpy = adapter_list;
    if (adapter_list == NULL) {
		 printf("\n------------ adapter_list null----------------\n");
        return 0xFF;
    }
    printf("\n------------ list of connection----------------\n");
    while (*p_ptr) {
        printf("%d: %s\n", id++, *p_ptr++);
    }
    if (id == 1)
        id--;
    else {
        printf("\nPlease select the connection : ");
        scanf("%d", &temp);
        id = (uint8_t) temp;
    }
    printf("Connecting to %s ...", p_ptr_cpy[id]);
    printf("\n-----------------------------------------------\n");
    return id;
}

static void s_handler_sync(inv_inst_t inst1)
{

    if (cfg_478x.adapt_id[0] == 'A') {
        irq_poll = true;
        s_create_thread();
    } else if (cfg_478x.adapt_id[0] == 'I') {
        bool_t query = inv_adapter_connect_query(inst1);
        if (irq_poll) {
            s_create_thread();
        }
        if (query) {
            printf("adapter is connected\n");
        } else {
            printf("adapter is disconnected\n");
        }
        if (query) {
            /*
             * check for any pending interrupts
             */
            printf("inv478x_handle is called\n");
            inv478x_handle(inst);
        }
    }
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
    cfg_478x.chip_sel = addr;
    cfg_478x.mode = INV_ADAPTER_MODE__I2C;
    cfg_478x.adapt_id = adapter_list[id];   //"Aardvark_2238-031382";
    adpt_inst_478x = inv_adapter_create(&cfg_478x, s_adapter_478x_callback, s_adapter_478x_interrupt);
    if (!adpt_inst_478x) {
        printf("\nNo adatper found\n");
        exit(0);
    }


    inv_platform_adapter_select(0, adpt_inst_478x);
    inst = inv478x_create(0, inv478x_event_callback_func, &sInv478xConfig);

    s_handler_sync(adpt_inst_478x);

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
    inst = inst;

    if (event_flags & INV478X_EVENT_FLAG__BOOT)
        printf("\nbit 7  : Boot Status Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_PLUS5V)
        printf("\nbit 2  : Rx Zero 5V Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_PLUS5V)
        printf("\nbit 3  : Rx One 5V Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_AVLINK)
        printf("\nbit 4  : Rx Zero AV Link Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_AVLINK)
        printf("\nbit 5  : Rx One AV Link Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_HDCP)
        printf("\nbit 10 : RX0 HDCP State Changed");
    if (event_flags & INV478X_EVENT_FLAG__RX1_HDCP)
        printf("\nbit 11 : RX1 HDCP State Changed");
	if (event_flags & INV478X_EVENT_FLAG__RX0_AVMUTE)
        printf("\nbit 14 : RX0 Av Mute changed");
	if (event_flags & INV478X_EVENT_FLAG__RX1_AVMUTE)
        printf("\nbit 15 : RX1 Av Mute changed");
    if (event_flags & INV478X_EVENT_FLAG__RX0_VIDEO)
        printf("\nbit 18 : Rx Zero Video format changed");
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

}




void case_user_notifications(void)
{
    uint8_t key_api_num;
    printf("\n a Event Flag Mask Set/Get");
    printf("\n b Event Flag Status Query");
    printf("\n c Event Flag Status Clear");

    printf("\n Press the character key");

    key_api_num = (uint8_t) _getch();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val, mask_val;
            uint8_t ch_set_get;

            printf("\n Event Flag Mask : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) _getch();
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





void case_receiver_common(void)
{
    uint8_t key_api_num;
    printf("\n a. Rx Port Select");
    printf("\n b. Rx Fast Switch");

    printf("\n Press the character key");
    key_api_num = (uint8_t) _getch();
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val = 0;
            uint8_t ch_set_get, port;

            printf("\n  Rx Port Select : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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
    key_api_num = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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
            uint32_t length, offset, block, port;
            uint8_t p_val[256];
            uint8_t i;
            inv_rval_t ret_val;
            uint8_t ch_set_get;


            printf("\n  Rx EDID Value : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) _getch();
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
                    ret_val = inv478x_rx_edid_set(inst, (uint8_t) port, (uint8_t) block, (uint8_t) offset, (uint8_t) length, p_val);
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
                    ret_val = inv478x_rx_edid_get(inst, (uint8_t) port, (uint8_t) block, (uint8_t) offset, (uint8_t) length, p_val);

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
            enum inv_hdmi_edid_repl val;

            uint8_t ch_set_get;

            printf("\n  Rx EDID Replicate : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) _getch();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n **Press 0 to EDID Replicate none");
                    printf("\n **Press 1 to EDID Replicate tx0 ");
                    printf("\n **Press 2 to EDID Replicate tx1 ");
                    scanf("%d", &val);
                    ret_val = inv478x_rx_edid_replicate_set(inst, port, &val);
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

                    ret_val = inv478x_rx_edid_replicate_get(inst, port, &val);
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
            ch_set_get = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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

            enum inv_hdmi_hpd_repl val;
            uint8_t ch_set_get;


            printf("\n  Rx HPD Replicate : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
            ch_set_get = (uint8_t) _getch();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Rx Port 0/1 :");
                    scanf("%d", &port);

                    printf("\n **Press 0 to HPD Replicate Disable");
                    printf("\n **Press 1 to HPD Replicate Enable Level tx0 ");
                    printf("\n **Press 2 to HPD Replicate Enable EDGE tx0");
                    printf("\n **Press 3 to HPD Replicate Enable Level tx1 ");
                    printf("\n **Press 4 to HPD Replicate Enable EDGE tx1");
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
                    printf("\n **Choose Rx Port 0/1 :");
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
            ch_set_get = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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
                printf("\n Pixel Full Range(enable/Disable) is = %d", p_val.full_range);
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
                for (i = 0; i < 7; i++)
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
            uint8_t packet[136];
            uint8_t len;
            uint32_t ret_val;


            printf("\n **Choose Rx Port 0/1 : ");
            scanf("%d", &port);
            printf("\n Choose the packet type(0 to 14):");
            scanf("%d", &type);
            printf("\n Enter the Packet length :");
            scanf("%d", &len);
            ret_val = inv478x_rx_packet_query(inst, port, type, len, packet);
            if (!ret_val) {
                printf("\n The packet type number = %d", type);
                printf("\n The packet data is-\n");
                for (i = 0; i < 31; i++) {
                    printf("%2X ", packet[i]);
                    if (i % 15 == 0)
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
            ch_set_get = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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
            ch_set_get = (uint8_t) _getch();
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

void s_cases(void)
{
    uint8_t key_module_num;
    printf("\n*************************************");

    printf("\n 1. User Notification");
    printf("\n 2. Receiver Common");
    printf("\n 3. Receiver");

    printf("\n *************************************");
    printf("\n Press the number key");
    key_module_num = (uint8_t) _getch();
    switch (key_module_num) {

    case '1':
        system("CLS");
        printf("User Notification APIs");
        case_user_notifications();
        break;

    case '2':
        system("CLS");
        printf("Receiver(Common) APIs");
        case_receiver_common();
        break;
    case '3':
        system("CLS");
        printf("Receiver APIs");
        case_receiver();
        break;

    default:
        system("CLS");
        printf("\n Invalid Character");
        break;

    }                           /* end of switch(key_api_num) */
}                               /* end of case s */

DWORD WINAPI callback_thread_handler(PVOID pParam)
{
    bool_t status = true;
    pParam = pParam;
    while (status) {
        if (inst) {
            inv478x_handle(inst);
        }
    }
    return 0;
}

static void s_create_thread(void)
{
    CreateThread(NULL, 0, callback_thread_handler, NULL, 0, NULL);
}


int main(void)
{
    printf("Copyright Invecas Inc, 2019\n");
    printf("Build Time: " __DATE__ ", " __TIME__ "\n");

    create_478x_adapter(s_adapter_list_query(), 0x40);

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
            if (irq_detect || irq_poll) {
                //inv478x_handle(inst);
                irq_detect = false;
            }
            /*----------------------------------------------------------- */
            /*
             * Insert here other application handlers
             */
            /*----------------------------------------------------------- */

            if (_kbhit()) {
                int key = _getch();
                switch (key) {
                case 'q':
                    sbRunning = false;
                    break;
                case 's':
                    system("CLS");
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

    return 0;
}                               /* end of main function */
