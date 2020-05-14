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

/*disabled warning regarding prinft and scanf as they are required for adjustments*/
#pragma warning (disable : 4477 4459 )

/***** #include statements ***************************************************/

#include <stdio.h>
//#include <conio.h>
//#include <windows.h>
#include "inv478x_api.h"
//#include "inv478x_platform_api.h"
#include "inv478x_app_def.h"
#include "inv_drv_cra_api.h"
#include "inv_common.h"
#include "inv_adapter_api.h"

#include <stdlib.h>
#include <string.h>
#include "hi_unf_i2c.h"
#include <unistd.h>
#define HI_INFO_I2C(format, arg...) printf( format, ## arg)

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
	
	 return 0xFF;
	 
    adapter_list = inv_adapter_list_query();
    p_ptr = p_ptr_cpy = adapter_list;
    if (adapter_list == NULL) {
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
#if 0
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
#endif
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
     printf("\ncreate_478x_adapter\n");
    cfg_478x.chip_sel = addr;
    cfg_478x.mode = INV_ADAPTER_MODE__I2C;
   // cfg_478x.adapt_id = adapter_list[id];   //"Aardvark_2238-031382";
		 cfg_478x.adapt_id = "ff"; 
	//adpt_inst_478x = inv_adapter_create(&cfg_478x, s_adapter_478x_callback, s_adapter_478x_interrupt);
   /* if (!adpt_inst_478x) {
        printf("\nNo adatper found\n");
        exit(0);
    }*/


  //  inv_platform_adapter_select(0, adpt_inst_478x);
    inst = inv478x_create(0, inv478x_event_callback_func, &sInv478xConfig);

   // s_handler_sync(adpt_inst_478x);
	inv478x_handle(inst);

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
    if (event_flags & INV478X_EVENT_FLAG__TX0_RSEN)
        printf("\nbit 12 : Tx Zero RSEN Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX0_HPD)
        printf("\nbit 0  : Tx Zero HPD Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX1_RSEN)
        printf("\nbit 13 : Tx One RSEN Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX1_HPD)
        printf("\nbit 1  : Tx One HPD Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX0_HDCP)
        printf("\nbit 16 : TX0 HDCP State Changed");
    if (event_flags & INV478X_EVENT_FLAG__TX1_HDCP)
        printf("\nbit 17 : TX1 HDCP State Changed");
}





void case_user_notifications(void)
{
    uint8_t key_api_num;
    printf("\n a. Event Flag Mask Set/Get");
    printf("\n b. Event Flag Status Query");
    printf("\n c. Event Flag Status Clear");

    printf("\n Press the character key");
	while(getchar()!='\n');
    key_api_num = (uint8_t) getchar();
	
    switch (key_api_num) {
    case 'a':
        {
            uint32_t ret_val, mask_val;
            uint8_t ch_set_get;

            printf("\n Event Flag Mask : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    
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

void case_pixel_format_conversion(void)
{
    uint8_t key_api_num;
    printf("\n a. Tx CSC Enable");
    printf("\n b. Tx CSC Pixel Format");
    printf("\n c. Tx CSC Failure Query");

    printf("\n Press the character key : ");
	while(getchar()!='\n');
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
			while(getchar()!='\n');
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


void case_transmitter(void)
{
    uint8_t key_api_num;
    printf("\n a. Tx AV Source");
    printf("\n b. Tx Pin Swap");
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
    printf("\n L. Tx Blank Color");
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
    printf("\n X. Tx Blank Enable");

    printf("\n Press the character key");
	while(getchar()!='\n');
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
			while(getchar()!='\n');
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
					while(getchar()!='\n');
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
            enum inv_hdmi_pin_swap val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Pin Swap : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf("%d", &port);

                    printf("\n Pin Swap Value(0/1/2/3) : ");
                    scanf("%d", &val);
                    ret_val = inv478x_tx_pin_swap_set(inst, port, &val);
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

                    ret_val = inv478x_tx_pin_swap_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx SoC Enable Value On %d Port is %d\n", port, val);
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
			while(getchar()!='\n');
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
			while(getchar()!='\n');
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
					while(getchar()!='\n');
                    scanf("%d", &port);

                    ret_val = inv478x_tx_auto_enable_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx Auto Enable Value On %d Port  is %d\n", port, val);
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
			while(getchar()!='\n');
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
			while(getchar()!='\n');
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
			while(getchar()!='\n');
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
					while(getchar()!='\n');
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
					while(getchar()!='\n');
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
			while(getchar()!='\n');
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
					while(getchar()!='\n');
                    scanf("%d", &port);

                    printf("\n Enter 1 to set or 0 to clear the HPD ");
					while(getchar()!='\n');
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
					while(getchar()!='\n');
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
			while(getchar()!='\n');
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
            scanf(" %d", &port);

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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");

                    scanf(" %d", &port);

                    printf("\n Enter 1 to set or 0 to clear ");
					
                    scanf(" %d", &p_val);
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
					
                    scanf(" %d", &port);

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
            scanf(" %d", &inv_port);
            printf("\n **Enter block number 0/1 : ");
            scanf(" %d", &block);
            printf("\n **Enter Offset(0 to 127) : ");
            scanf(" %d", &offset);
            printf("\n **Enter Length(max(128 - offset) : ");
            scanf(" %d", &length);
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enter HVSYNC Plarity Value(0-5) :");
                    scanf(" %d", &val);
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
                    scanf(" %d", &port);

                    ret_val = inv478x_tx_hvsync_pol_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx HVSYNC Value On %d Port is %d\n", port, val);
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n **Choose Source for Tx Audio(0 - 7) :");
                    scanf(" %d", &p_val);

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
                    scanf(" %d", &port);


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
            scanf(" %d", &port);

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
            scanf(" %d", &port);

            ret_val = inv478x_tx_audio_info_query(inst, port, &p_val);
            if (!ret_val) {
                printf("\n The Channel Status Data is :");
                for (i = 0; i < 9; i++)
                    printf(" %d", p_val.cs.b[i]);
                printf("\n The Audio Map Data is :");
                for (i = 0; i < 7; i++)
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n ** Press 1 to Set Audio Mute, 0 to Clear Audio Mute, 0/1 :");
                    scanf(" %d", &p_val);
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
                    scanf("\n %d", &port);
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
            scanf(" %d", &port);

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
                    scanf(" %d", &port);

                    printf("\n ** Press 1 to Set Av Mute, 0 to Clear Av Mute, 0/1 :");
                    scanf(" %d", &p_val);
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
                    scanf(" %d", &port);

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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enable or Disable Tx AVMUTE Auto(0/1) :");
                    scanf(" %d", &p_val);
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
                    scanf(" %d", &port);

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
            scanf(" %d", &port);
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
            scanf(" %d", &port);

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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n **Choose Tx HDCP mode, 0/1/2 :");
                    scanf(" %d", &p_val);
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
                    scanf(" %d", &port);

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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enable or Disable HDCP Auto Approve(0/1) :");
                    scanf(" %d", &p_val);
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
                    scanf(" %d", &port);

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
            scanf(" %d", &port);


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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n **Enter HDCP encrypt value(0/1) :");
                    scanf(" %d", &p_val);

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
                    scanf(" %d", &port);


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
            scanf(" %d", &port);

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
            enum inv_hdcp_type p_val = 0;
            uint32_t ret_val;

            printf("\n **Choose Tx Port 0/1 : ");
            scanf(" %d", &port);


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
            scanf(" %d", &port);

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
            scanf(" %d", &port);
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enter Sequence Number : ");
                    scanf(" %ld", &p_val.seq_num_m);
                    printf("\n Enter Stream ID Type : ");
                    scanf(" %d", &p_val.stream_id_type);

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
                    scanf(" %d", &port);


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
            scanf(" %d", &port);


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
            scanf(" %d", &port);
            printf("\n Enter the length : ");
            scanf(" %d", &len);
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
            scanf(" %d", &port);
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
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Blank Color : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enter Red Color Value : ");
                    scanf(" %d", &val.r);
                    printf("\n Enter Green Color Value : ");
                    scanf(" %d", &val.g);
                    printf("\n Enter Blue Color Value : ");
                    scanf(" %d", &val.b);

                    ret_val = inv478x_tx_blank_color_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx Blank Color Has Been Set On Port %d\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);


                    ret_val = inv478x_tx_blank_color_get(inst, port, &val);
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n **Enter enable/disable, 1/0:");
                    scanf(" %d", &p_val);
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
                    scanf(" %d", &port);

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
            scanf(" %d", &port);

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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);
                    printf("\n SCDC Read Enable/Disable 0/1 :");
                    scanf(" %d", &val);
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
                    scanf(" %d", &port);

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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enter SCDC Poll Period Value : ");
                    scanf(" %d", &val);
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
                    scanf(" %d", &port);

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
            scanf(" %d", &port);

            printf("\n Enter The Offset Value : ");
            scanf(" %d", &offset);
            printf("\n Enter The Value To Write : ");
            scanf(" %d", &val);
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


            printf("\n **Choose Tx Port 0/1 ph : ");
            scanf(" %d", &port);

            printf("\n Enter The Address Value : ");
            scanf(" %d", &offset);
            ret_val = inv478x_tx_scdc_read(inst, port, offset, &val);
            if (!ret_val)
                printf("\n Tx SCDC Value At Address %d On %d Port is : %d", offset, port, val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'S':
        {
            uint8_t port, val[256], slv_addr, size, i, offset;
            uint32_t ret_val;


            printf("\n **Choose Tx Port 0/1 : ");
            scanf(" %d", &port);

            printf("\n Enter The Slave Address Value(In Hexadecimal): ");
            scanf(" %d", &slv_addr);
            printf("\n Enter The Offset Value(In Hexadecimal): ");
            scanf(" %d", &offset);
            printf("\n Enter The Size : ");
            scanf(" %d", &size);
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
            scanf(" %d", &port);

            printf("\n Enter The Address Value(In Hexadecimal) : ");
            scanf(" %X", &slv_addr);
            printf("\n Enter The Offset Value(In Hexadecimal): ");
            scanf(" %X", &offset);
            printf("\n Enter The Size Value : ");
            scanf(" %d", &size);
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
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enter DDC Frequency Value :");
                    scanf(" %d", &val);
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
                    scanf(" %d", &port);

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
            scanf(" %d", &port);

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
            scanf(" %d", &port);
            ret_val = inv478x_tx_tmds2_1over4_query(inst, port, &p_val);
            if (!ret_val)
                printf("\nTx 1OVER4 Div Ratio is = %d\n", p_val);
            else
                printf("\n API Error Code Return Value = %d", ret_val);
            break;
        }
    case 'X':
        {
            uint8_t port;
            bool_t val;
            uint32_t ret_val;

            uint8_t ch_set_get;

            printf("\n  Tx Blank Enable : ");
            printf("\n  a. Set");
            printf("\n  b. Get");
			while(getchar()!='\n');
            ch_set_get = (uint8_t) getchar();
            switch (ch_set_get) {
            case 'a':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    printf("\n Enable or Disable Tx Blank Color(0/1) :");
                    scanf(" %d", &val);
                    ret_val = inv478x_tx_blank_enable_set(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx Blank Enable Value Has Been Set On %d Port\n", port);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            case 'b':
                {
                    printf("\n **Choose Tx Port 0/1 : ");
                    scanf(" %d", &port);

                    ret_val = inv478x_tx_blank_enable_get(inst, port, &val);
                    if (!ret_val)
                        printf("\n Tx Blank Enable Value On %d Port = %d\n", port, val);
                    else
                        printf("\n API Error Code Return Value = %d", ret_val);
                    break;
                }
            }
            break;
        }
    default:
        break;
    }                           /* switch (key_api_num) for Tx ends here */
    /*
     * case 2 ends here
     */
}




void s_cases(void)
{
    char key_module_num;
    printf("\n*************************************");

    printf("\n 1. User Notification");

    printf("\n 2. Pixel Format Conversion");

    printf("\n 3. Transmitter");

    printf("\n *************************************");
    printf("\n Press the number key");
	while(getchar()!='\n');
    key_module_num = getchar();
	printf("\n key_module_num=%c \n",key_module_num);
    switch (key_module_num) {
    case '1':
        //system("CLS");
        printf("User Notification APIs");
        case_user_notifications();
        break;

    case '2':
        //system("CLS");
        printf("Pixel Format Conversion APIs");
        case_pixel_format_conversion();
        break;

    case '3':
        //system("CLS");
        printf("Transmitter APIs");
        case_transmitter();
        break;

    default:
        //system("CLS");
        printf("\n Invalid Character222");
        break;

    }                           /* end of switch(key_api_num) */
}                               /* end of case s */

/*DWORD WINAPI callback_thread_handler(PVOID pParam)
{
    bool_t status = true;
    pParam = pParam;
    while (status) {
        if (inst) {
            inv478x_handle(inst);
        }
    }
    return 0;
}*/

static void s_create_thread(void)
{
    //CreateThread(NULL, 0, callback_thread_handler, NULL, 0, NULL);
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
	
    /*inv_inst_t adpt_4796_inst = INV_INST_NULL;*/
    printf("Copyright Invecas Inc, 2019\n");
    printf("Build Time: " __DATE__ ", " __TIME__ "\n");

	ret = hi_unf_i2c_init();
    if (ret != HI_SUCCESS) {
        HI_INFO_I2C("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }
	usleep(1000000);
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
           /* if (irq_detect || irq_poll) {
                //inv478x_handle(inst);
                irq_detect = false;
            }*/
            /*----------------------------------------------------------- */
            /*
             * Insert here other application handlers
             */
            /*----------------------------------------------------------- */

            if (kbhit()) {
                char key = getchar();
                switch (key) {
                case 'q':
                    sbRunning = false;
                    break;
                case 's':
                   // system("CLS");
                   	printf("capture s button\n");
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
   // inv478x_delete(inst);

    /*
     * API host destructor for Inv478xrx product
     */
   /* if (adpt_inst_478x) {
        inv_adapter_delete(adpt_inst_478x);
    }*/
    //==========i2c=============
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
