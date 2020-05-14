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
* file inv478x_example_app_external_rptr.c
*
* brief example application for inv478x
*
*****************************************************************************/

/***** #include statements ***************************************************/

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include "inv478x_api.h"
#include "inv478x_app_def.h"
#include "inv_drv_cra_api.h"
#include "inv_common.h"
#include "inv_adapter_api.h"
#include "inv478x_platform_api.h"


#define INV_CALLBACK__HANDLER

#define NUM_OF_TX_PORTS 2
#define INV_APP_DEF_HDCP14_MAX_DEPTH        7
#define INV_APP_DEF_HDCP14_MAX_DEV_TX       127
#define INV_APP_DEF_HDCP14_MAX_DEV_RPTR     32

#define INV_APP_DEF_HDCP22_MAX_DEV          31
#define INV_APP_DEF_HDCP22_MAX_DEPTH        4

static char **adapter_list;

/*Global Variables for RX*/
static struct inv_adapter_cfg cfg_478x_rx;
static inv_inst_t adpt_inst_478x_rx = INV_INST_NULL;
static inv_inst_t inst_rx = INV_INST_NULL;
static bool_t irq_poll_rx = false;

/*Global variables for TX*/
static struct inv_adapter_cfg cfg_478x_tx;
static inv_inst_t adpt_inst_478x_tx = INV_INST_NULL;
static inv_inst_t inst_tx = INV_INST_NULL;
static bool_t irq_poll_tx = false;
static bool_t ext_hdcp_mode_rptr[2][2];
static bool_t status = 0;
static bool_t rx_port;
static bool_t tx_port;
static enum inv_hdcp_stat prev_val = INV_HDCP_STAT__NONE;
static int fail_tx[2] = { 0, 0 };

static bool_t init_completed = false;
static bool_t tx_vid_src[2];
static bool_t rx_rptr_mode[2];
static bool_t tx_rptr_mode[2];


/* Static configuratyion object for host driver configuration */
static struct inv478x_config sInv478xConfigRx = {
    "478x-Rx",                  /* pNameStr[8]         - Name of Object */
    true,                       /* bDeviceReset       - */
};

/* Static configuratyion object for host driver configuration */
static struct inv478x_config sInv478xConfigTx = {
    "478x-Tx",                  /* pNameStr[8]         - Name of Object */
    true,                       /* bDeviceReset       - */
};

/*******************************************************Start of RX Functions********************************************/
DWORD WINAPI callback_thread_handler_rx(PVOID pParam)
{
    bool_t stat = true;
    pParam = pParam;
    while (stat) {
        if (inst_rx) {
            inv478x_handle(inst_rx);
        }
    }
    return 0;
}

static void s_create_thread_rx()
{
    CreateThread(NULL, 0, callback_thread_handler_rx, NULL, 0, NULL);
}

static void s_handler_sync_rx(inv_inst_t inst1)
{

    if (cfg_478x_rx.adapt_id[0] == 'A') {
        irq_poll_rx = true;
        s_create_thread_rx();
    } else if (cfg_478x_rx.adapt_id[0] == 'I') {
        bool_t query = inv_adapter_connect_query(inst1);
        if (irq_poll_rx) {
            s_create_thread_rx();
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
            inv478x_handle(inst_rx);
        }
    }
}

void s_adapter_478x_interrupt_rx(inv_inst_t inst1)
{
    inst1 = inst1;
    if (!irq_poll_rx) {
        //printf("\n---s_adapter_478x_interrupt--\n");
        inv478x_handle(inst_rx);
    }
}

static void s_adapter_478x_callback_rx(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_478x_rx) {
        s_handler_sync_rx(inst1);
    }
#endif
}

static void s_rx_to_tx_hdcp_update(bool_t port_rx, bool_t port_tx)
{

    bool_t tx_hdcp_start = true;
    bool_t tx_encryption = false;
    struct inv_hdcp_csm CSM_msg;
    enum inv_hdcp_stat p_val;

    inv478x_rx_hdcp_stat_query(inst_rx, port_rx, &p_val);
    if (p_val == INV_HDCP_STAT__AUTH) {
        printf("\n RX%d HDCP auth Start\n", (int)port_rx);
        prev_val = INV_HDCP_STAT__NONE;
        inv478x_tx_hdcp_start(inst_tx, port_tx, &tx_hdcp_start);
    }

    if (p_val == INV_HDCP_STAT__SUCCESS && prev_val != p_val) {
        printf("\n Rx%d CSM change\n", (int)port_rx);
        prev_val = p_val;
        inv478x_rx_hdcp_csm_query(inst_rx, port_rx, &CSM_msg);
        inv478x_tx_hdcp_csm_set(inst_tx, port_tx, &CSM_msg);
    } else if (p_val == INV_HDCP_STAT__SUCCESS) {
        inv478x_rx_hdcp_encryption_status_get(inst_rx, port_rx, &tx_encryption);
        inv478x_tx_hdcp_encrypt_set(inst_tx, port_tx, &tx_encryption);
    }

    if (p_val == INV_HDCP_STAT__NONE) {
        printf("\n Rx%d encryption status change \n", (int)port_rx);
        inv478x_rx_hdcp_encryption_status_get(inst_rx, port_rx, &tx_encryption);
        inv478x_tx_hdcp_encrypt_set(inst_tx, port_tx, &tx_encryption);
        printf("\n tx%d_encryption set = %d\n", (int)port_tx, (int) tx_encryption);
    }
}

/* Inv478xrx API Call-Back Handler */
static void inv478x_event_callback_func_rx(inv_inst_t inst, inv478x_event_flags_t event_flags)
{

    inst = inst;
    /*
     * Do not handle any events until initialization is completed
     */
    if (!init_completed)
        return;

    if ((event_flags & INV478X_EVENT_FLAG__RX0_HDCP) && rx_rptr_mode[0]) {
        rx_port = 0;
        printf("\nbit 10 : RX0 HDCP State Changed");
        for (tx_port = 0; tx_port < 2; tx_port++) {
            if ((rx_port == tx_vid_src[tx_port]) && tx_rptr_mode[tx_port])
                s_rx_to_tx_hdcp_update(rx_port, tx_port);
        }
    }

    if ((event_flags & INV478X_EVENT_FLAG__RX1_HDCP) && rx_rptr_mode[1]) {
        rx_port = 1;
        printf("\nbit 10 : RX1 HDCP State Changed");
        for (tx_port = 0; tx_port < 2; tx_port++) {
            if ((rx_port == tx_vid_src[tx_port]) && tx_rptr_mode[tx_port])
                s_rx_to_tx_hdcp_update(rx_port, tx_port);
        }
    }
}


static bool_t s_check_boot_stat_rx()
{
    enum inv_sys_boot_stat boot_stat;

    inv478x_boot_stat_query(inst_rx, &boot_stat);
    return (INV_SYS_BOOT_STAT__SUCCESS == boot_stat) ? (true) : (false);
}

static void s_log_version_information_rx()
{
    uint32_t chipIdStat = 0;

    printf("\n");
    inv478x_chip_id_query(inst_rx, &chipIdStat);
    printf("inv478x_chip_id_query           : %0ld\n", chipIdStat);

}

void create_adapter_rx(uint8_t id, uint8_t addr)
{
    cfg_478x_rx.chip_sel = addr;
    cfg_478x_rx.mode = INV_ADAPTER_MODE__I2C;
    cfg_478x_rx.adapt_id = adapter_list[id];

    adpt_inst_478x_rx = inv_adapter_create(&cfg_478x_rx, s_adapter_478x_callback_rx, s_adapter_478x_interrupt_rx);

    if (!adpt_inst_478x_rx) {
        printf("\nNo adatper found\n");
        exit(0);
    }

    inv_platform_adapter_select(id, adpt_inst_478x_rx);

    inst_rx = inv478x_create(id, inv478x_event_callback_func_rx, &sInv478xConfigRx);
    s_handler_sync_rx(adpt_inst_478x_rx);


    if ((inv_inst_t *) INV_RVAL__HOST_ERR == inst_rx) {
        printf("\nERROR: Host connection error\n");
        exit(0);
    }
    if (INV_INST_NULL == inst_rx) {
        printf("\nERROR: Memory problem\n");
        exit(0);
    }

    if (s_check_boot_stat_rx()) {
        /*
         * Retrieve general product/revision info
         */
        s_log_version_information_rx();
    }
}

/****************************************************End of RX Functions*****************************************************************/


/*************************************************************Start of TX Functions********************************************/
DWORD WINAPI callback_thread_handler_tx(PVOID pParam)
{
    bool_t stat = true;
    pParam = pParam;
    while (stat) {
        if (inst_tx) {
            inv478x_handle(inst_tx);
        }
    }
    return 0;
}

static void s_create_thread_tx()
{
    CreateThread(NULL, 0, callback_thread_handler_tx, NULL, 0, NULL);
}

static void s_handler_sync_tx(inv_inst_t inst1)
{

    if (cfg_478x_tx.adapt_id[0] == 'A') {
        irq_poll_tx = true;
        s_create_thread_tx();
    } else if (cfg_478x_tx.adapt_id[0] == 'I') {
        bool_t query = inv_adapter_connect_query(inst1);
        if (irq_poll_tx) {
            s_create_thread_tx();
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
            inv478x_handle(inst_tx);
        }
    }
}

void s_adapter_478x_interrupt_tx(inv_inst_t inst1)
{
    inst1 = inst1;
    if (!irq_poll_tx) {
        //printf("\n---s_adapter_478x_interrupt--\n");
        inv478x_handle(inst_tx);
    }
}

static void s_adapter_478x_callback_tx(inv_inst_t inst1)
{
#ifdef INV_CALLBACK__HANDLER
    if (adpt_inst_478x_tx) {
        s_handler_sync_tx(inst1);
    }
#endif
}



static void s_hdcp_common_topology_update(uint8_t port_rx, struct inv_hdcp_top *p_common_topology, struct inv_hdcp_top *p_topology_new)
{
    uint8_t max_device_count = INV_APP_DEF_HDCP14_MAX_DEV_RPTR;
    uint8_t max_depth = INV_APP_DEF_HDCP14_MAX_DEPTH;
    enum inv_hdcp_type rx_hdcp_type;

    inv478x_rx_hdcp_type_query(inst_rx, port_rx, &rx_hdcp_type);

    printf("HDCP type is %d", rx_hdcp_type);
    if (INV_HDCP_TYPE__HDCP2 == rx_hdcp_type) {
        max_device_count = INV_APP_DEF_HDCP22_MAX_DEV;
        max_depth = INV_APP_DEF_HDCP22_MAX_DEPTH;
    }
    p_common_topology->rptr_depth = (p_common_topology->rptr_depth > p_topology_new->rptr_depth) ? p_common_topology->rptr_depth : p_topology_new->rptr_depth;
    p_common_topology->dev_count += p_topology_new->dev_count;
    p_common_topology->max_devs_exceed |= p_topology_new->max_devs_exceed;
    p_common_topology->max_cas_exceed |= p_topology_new->max_cas_exceed;
    p_common_topology->hdcp2_rptr_ds |= p_topology_new->hdcp2_rptr_ds;
    p_common_topology->hdcp1_dev_ds |= p_topology_new->hdcp1_dev_ds;
    if (p_common_topology->dev_count > max_device_count) {
        p_common_topology->max_devs_exceed = true;
        p_common_topology->dev_count = max_device_count;
    }

    if (p_common_topology->rptr_depth > max_depth) {
        p_common_topology->max_cas_exceed = true;
        p_common_topology->rptr_depth = max_depth;
    }
}

void s_ksv_list_consolidate(uint8_t port_rx, struct inv_hdcp_top *tx_topology, struct inv_hdcp_ksv *ksv_list)
{
    int i;
    int device_count;
    for (i = 0; i < NUM_OF_TX_PORTS; i++) {
        struct inv_hdcp_top tx_new_topology[NUM_OF_TX_PORTS];

        inv478x_tx_hdcp_topology_query(inst_tx, (unsigned char)i, &tx_new_topology[i]);
        device_count = i == 0 ? 0 : tx_topology->dev_count;
        s_hdcp_common_topology_update(port_rx, tx_topology, &tx_new_topology[i]);
        inv478x_tx_hdcp_ksvlist_query(inst_tx, (unsigned char)i, tx_new_topology[i].dev_count, ksv_list + device_count);
    }
    inv478x_rx_hdcp_ext_topology_set(inst_rx, port_rx, tx_topology);
    inv478x_rx_hdcp_ext_ksvlist_set(inst_rx, port_rx, tx_topology->dev_count, ksv_list);
    printf("HDCP dev count  is %d", tx_topology->dev_count);
}

static void s_tx_to_rx_hdcp_dual_rptr_update(bool_t port_rx, bool_t port_tx)
{
    enum inv_hdcp_stat val;
    struct inv_hdcp_top tx_topology;
    struct inv_hdcp_ksv ksv_list[32];

    INV_MEMSET (&tx_topology, 0, sizeof(struct inv_hdcp_top));
    inv478x_tx_hdcp_stat_query(inst_tx, port_tx, &val);

    if ((val == INV_HDCP_STAT__SUCCESS) || (val == INV_HDCP_STAT__CSM)) {
        fail_tx[port_tx] = 0;
        inv478x_tx_hdcp_stat_query(inst_tx, !port_tx, &val);
        if ((val == INV_HDCP_STAT__SUCCESS) || (val == INV_HDCP_STAT__CSM) || (val == INV_HDCP_STAT__NONE)) {
            /*
             * update common topology to rx
             */
            s_ksv_list_consolidate(port_rx, &tx_topology, ksv_list);
        }
    }

    if (val == INV_HDCP_STAT__FAILED)
        fail_tx[port_tx]++;

    if (fail_tx[port_tx] == 5) {
        inv478x_tx_hdcp_stat_query(inst_tx, !port_tx, &val);
        if ((val == INV_HDCP_STAT__SUCCESS) || (val == INV_HDCP_STAT__CSM)) {
            inv478x_tx_hdcp_topology_query(inst_tx, !port_tx, &tx_topology);
            inv478x_rx_hdcp_ext_topology_set(inst_rx, port_rx, &tx_topology);
            inv478x_tx_hdcp_ksvlist_query(inst_tx, !port_tx, tx_topology.dev_count, ksv_list);
            inv478x_rx_hdcp_ext_ksvlist_set(inst_rx, port_rx, tx_topology.dev_count, ksv_list);
        }
    }

}

static void s_tx_to_rx_hdcp_update(bool_t port_rx, bool_t port_tx)
{
    enum inv_hdcp_stat val;
    struct inv_hdcp_top tx_topology;
    struct inv_hdcp_ksv ksv_list[32];
    enum inv_hdcp_failure failure_reason;
    enum inv_hdcp_type hdcp_type;

    INV_MEMSET (&tx_topology, 0, sizeof(struct inv_hdcp_top));
    inv478x_tx_hdcp_stat_query(inst_tx, port_tx, &val);
    inv478x_tx_hdcp_type_query(inst_tx, port_tx, &hdcp_type);

    if (((val == INV_HDCP_STAT__SUCCESS) /*&& (hdcp_type == INV_HDCP_TYPE__HDCP1) */ ) || ((val == INV_HDCP_STAT__CSM) /* && hdcp_type == INV_HDCP_TYPE__HDCP2 */ )) {
        printf("\n Tx%d HDCP success\n", (int)port_tx);
        inv478x_tx_hdcp_topology_query(inst_tx, port_tx, &tx_topology);
        inv478x_rx_hdcp_ext_topology_set(inst_rx, port_rx, &tx_topology);
        inv478x_tx_hdcp_ksvlist_query(inst_tx, port_tx, tx_topology.dev_count, ksv_list);
        inv478x_rx_hdcp_ext_ksvlist_set(inst_rx, port_rx, tx_topology.dev_count, ksv_list);
    }

    if (val == INV_HDCP_STAT__FAILED) {
        inv478x_tx_hdcp_failure_query(inst_tx, port_tx, &failure_reason);
        printf("\n Tx%d HDCP failed = %d\n", (int)port_tx, (int) failure_reason);
        if (failure_reason == INV_HDCP_FAILURE__TOPOLOGY) {
            inv478x_tx_hdcp_topology_query(inst_tx, port_tx, &tx_topology);
            inv478x_rx_hdcp_ext_topology_set(inst_rx, port_rx, &tx_topology);
            inv478x_tx_hdcp_ksvlist_query(inst_tx, port_tx, tx_topology.dev_count, ksv_list);
            inv478x_rx_hdcp_ext_ksvlist_set(inst_rx, port_rx, tx_topology.dev_count, ksv_list);
        }
    }
}


/* Inv478xrx API Call-Back Handler */
static void inv478x_event_callback_func_tx(inv_inst_t inst, inv478x_event_flags_t event_flags)
{
    bool_t p_val;
    bool_t b_hpd;
    inst = inst;

    /*
     * Do not handle any events until initialization is completed
     */
    if (!init_completed)
        return;

    /*
     * HPD Handling
     */
    if (event_flags & INV478X_EVENT_FLAG__TX0_HPD) {
        printf("\nbit 0  : Tx Zero HPD Changed");
        inv478x_tx_hpd_query(inst_tx, 0, &p_val);
        if (tx_vid_src[0] == tx_vid_src[1] && tx_rptr_mode[0] && tx_rptr_mode[1]) {
            inv478x_tx_hpd_query(inst_tx, 1, &b_hpd);
            p_val |= b_hpd;
        }
        inv478x_rx_hpd_enable_set(inst_rx, tx_vid_src[0], &p_val);
    }

    if (event_flags & INV478X_EVENT_FLAG__TX1_HPD) {
        printf("\nbit 0  : Tx One HPD Changed");
        inv478x_tx_hpd_query(inst_tx, 1, &p_val);
        if (tx_vid_src[0] == tx_vid_src[1] && tx_rptr_mode[0] && tx_rptr_mode[1]) {
            inv478x_tx_hpd_query(inst_tx, 0, &b_hpd);
            p_val |= b_hpd;
        }
        inv478x_rx_hpd_enable_set(inst_rx, tx_vid_src[1], &p_val);
    }


    if ((event_flags & INV478X_EVENT_FLAG__TX0_HDCP) && tx_rptr_mode[0]) {
        printf("\nbit 0  : Tx0 HDCP Changed");
        tx_port = 0;
    }

    if ((event_flags & INV478X_EVENT_FLAG__TX1_HDCP) && tx_rptr_mode[1]) {
        printf("\nbit 0  : Tx1 HDCP Changed");
        tx_port = 1;
    }

    /*
     * Consolidate KSV list if both TX are in repeater mode and both TX are connected to single
     * RX port
     */
    if (tx_vid_src[0] == tx_vid_src[1] && tx_rptr_mode[0] && tx_rptr_mode[1]) {
        s_tx_to_rx_hdcp_dual_rptr_update(tx_vid_src[tx_port], tx_port);
    } else {
        s_tx_to_rx_hdcp_update(tx_vid_src[tx_port], tx_port);
    }

}

static bool_t s_check_boot_stat_tx()
{
    enum inv_sys_boot_stat boot_stat;

    inv478x_boot_stat_query(inst_tx, &boot_stat);
    return (INV_SYS_BOOT_STAT__SUCCESS == boot_stat) ? (true) : (false);
}

static void s_log_version_information_tx()
{
    uint32_t chipIdStat = 0;

    printf("\n");
    inv478x_chip_id_query(inst_tx, &chipIdStat);
    printf("inv478x_chip_id_query           : %0ld\n", chipIdStat);

}

void create_adapter_tx(uint8_t id, uint8_t addr)
{
    cfg_478x_tx.chip_sel = addr;
    cfg_478x_tx.mode = INV_ADAPTER_MODE__I2C;
    cfg_478x_tx.adapt_id = adapter_list[id];

    adpt_inst_478x_tx = inv_adapter_create(&cfg_478x_tx, s_adapter_478x_callback_tx, s_adapter_478x_interrupt_tx);

    if (!adpt_inst_478x_tx) {
        printf("\nNo adatper found\n");
        exit(0);
    }

    inv_platform_adapter_select(id, adpt_inst_478x_tx);

    inst_tx = inv478x_create(id, inv478x_event_callback_func_tx, &sInv478xConfigTx);
    s_handler_sync_tx(adpt_inst_478x_tx);


    if ((inv_inst_t *) INV_RVAL__HOST_ERR == inst_tx) {
        printf("\nERROR: Host connection error\n");
        exit(0);
    }
    if (INV_INST_NULL == inst_tx) {
        printf("\nERROR: Memory problem\n");
        exit(0);
    }

    if (s_check_boot_stat_tx()) {
        /*
         * Retrieve general product/revision info
         */
        s_log_version_information_tx();
    }
}

/****************************************************End of TX Functions*****************************************************************/

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

int main(void)
{
    uint8_t id;
    char ltr;
    char LTR;
    enum inv_hdcp_mode hdcp_mode = INV_HDCP_MODE__RPTR;
    bool_t val = true;
    ext_hdcp_mode_rptr[0][0] = false;
    ext_hdcp_mode_rptr[0][1] = false;
    ext_hdcp_mode_rptr[1][0] = false;
    ext_hdcp_mode_rptr[1][1] = false;

    printf("Copyright Invecas Inc, 2019\n");
    printf("Build Time: " __DATE__ ", " __TIME__ "\n");

    printf("Select any one combination mentioned below:\n");
    printf(" a) RX0_TX0_RPTR:\n");
    printf(" b) RX0_TX1_RPTR:\n");
    printf(" c) RX1_TX0_RPTR:\n");
    printf(" d) RX1_TX1_RPTR:\n");
    scanf(" %c", &ltr);
    switch (ltr) {
    case 'a':
        ext_hdcp_mode_rptr[0][0] = true;
        printf(" A) RX0_TX1_RPTR:\n");
        printf(" B) RX1_TX1_RPTR:\n");
        printf("press other key to exit\n");
        scanf(" %c", &LTR);
        switch (LTR) {
        case 'A':
            ext_hdcp_mode_rptr[0][1] = true;
            break;
        case 'B':
            ext_hdcp_mode_rptr[1][1] = true;
            break;
        default:
            break;
        }
        break;
    case 'b':
        ext_hdcp_mode_rptr[0][1] = true;
        printf(" A) RX0_TX0_RPTR:\n");
        printf(" B) RX1_TX0_RPTR:\n");
        printf("press other key to exit\n");
        scanf(" %c", &LTR);
        switch (LTR) {
        case 'A':
            ext_hdcp_mode_rptr[0][0] = true;
            break;
        case 'B':
            ext_hdcp_mode_rptr[1][0] = true;
            break;
        default:
            break;
        }
        break;
    case 'c':
        ext_hdcp_mode_rptr[1][0] = true;
        printf(" A) RX0_TX1_RPTR:\n");
        printf(" B) RX1_TX1_RPTR:\n");
        printf("press other key to exit\n");
        scanf(" %c", &LTR);
        switch (LTR) {
        case 'A':
            ext_hdcp_mode_rptr[0][1] = true;
            break;
        case 'B':
            ext_hdcp_mode_rptr[1][1] = true;
            break;
        default:
            break;
        }
        break;
    case 'd':
        ext_hdcp_mode_rptr[1][1] = true;
        printf(" A) RX0_TX0_RPTR:\n");
        printf(" B) RX1_TX0_RPTR:\n");
        printf("press other key to exit\n");
        scanf(" %c", &LTR);
        switch (LTR) {
        case 'A':
            ext_hdcp_mode_rptr[0][0] = true;
            break;
        case 'B':
            ext_hdcp_mode_rptr[1][0] = true;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    printf("\n************Choose Aardvark Of  RX**************\n");
    id = s_adapter_list_query();
    create_adapter_rx(id, 0x40);

    printf("\n************Choose Aardvark Of  TX**************\n");
    id = s_adapter_list_query();
    create_adapter_tx(id, 0x40);

    if (ext_hdcp_mode_rptr[0][0]) {
        inv478x_rx_hdcp_mode_set(inst_rx, 0, &hdcp_mode);
        printf("\nRx0 is set as repeater\n");
        inv478x_tx_hdcp_mode_set(inst_tx, 0, &hdcp_mode);
        printf("\nTx0 is set as repeater\n");
        inv478x_rx_hdcp_ext_enable_set(inst_rx, 0, &val);
        tx_vid_src[0] = 0;
        rx_rptr_mode[0] = true;
        tx_rptr_mode[0] = true;
    }

    if (ext_hdcp_mode_rptr[0][1]) {
        inv478x_rx_hdcp_mode_set(inst_rx, 0, &hdcp_mode);
        printf("\nRx0 is set as repeater\n");
        inv478x_tx_hdcp_mode_set(inst_tx, 1, &hdcp_mode);
        printf("\nTx1 is set as repeater\n");
        inv478x_rx_hdcp_ext_enable_set(inst_rx, 0, &val);
        tx_vid_src[1] = 0;
        rx_rptr_mode[0] = true;
        tx_rptr_mode[1] = true;
    }

    if (ext_hdcp_mode_rptr[1][0]) {
        inv478x_rx_hdcp_mode_set(inst_rx, 1, &hdcp_mode);
        printf("\nRx1 is set as repeater\n");
        inv478x_tx_hdcp_mode_set(inst_tx, 0, &hdcp_mode);
        printf("\nTx0 is set as repeater\n");
        inv478x_rx_hdcp_ext_enable_set(inst_rx, 1, &val);
        tx_vid_src[0] = 1;
        rx_rptr_mode[1] = true;
        tx_rptr_mode[0] = true;
    }

    if (ext_hdcp_mode_rptr[1][1]) {
        inv478x_rx_hdcp_mode_set(inst_rx, 1, &hdcp_mode);
        printf("\nRx1 is set as repeater\n");
        inv478x_tx_hdcp_mode_set(inst_tx, 1, &hdcp_mode);
        printf("\nTx1 is set as repeater\n");
        inv478x_rx_hdcp_ext_enable_set(inst_rx, 1, &val);
        tx_vid_src[1] = 1;
        rx_rptr_mode[1] = true;
        tx_rptr_mode[1] = true;
    }

    init_completed = true;
    while (1);
    /*
     * while (1) {
     * inv478x_handle(inst_rx);
     * inv478x_handle(inst_tx);
     * }
     */
}
