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
    printf("inv478x_firmware_version_query  : %ws\n", versionStat);

}

static void inv478x_event_callback_func(inv_inst_t ins, inv478x_event_flags_t event_flags)
{
    ins = ins;
    event_flags = event_flags;
}

void case_flash_device_update(void)
{
    uint8_t key_api_num;
    printf("\n a. Flash Erase");
    printf("\n b. Flash Program");

    printf("\n Press the character key");

    key_api_num = (uint8_t) _getch();
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
    enum inv_sys_boot_stat stat_val = 0;
    stat_val = stat_val;
    inv_inst_t adpt_4796_inst = INV_INST_NULL;
    adpt_4796_inst = adpt_4796_inst;
    printf("Copyright Invecas Inc, 2019\n");
    printf("Build Time: " __DATE__ ", " __TIME__ "\n");

    create_478x_adapter(s_adapter_list_query(), 0x40);

    if (sTest < 0) {
        /*
         * System infinite while loop
         */
        printf("System is running ...\n");
        printf("\n(Press q to abort program, f to flash program)\n");

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
                case 'f':
                    system("CLS");
                    case_flash_device_update();
                    break;
                default:
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
