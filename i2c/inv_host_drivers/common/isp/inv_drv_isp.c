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
* file inv_drv_isp.c
*
* brief :- ISP APIs
*
*****************************************************************************/
/*
  This compile time enables debug logging. For release version and check-in
  keep this undefined.
*/
/*#define INV_DEBUG 2*/
/*
  This compile time flag controls if flash WP should protect Status Register itself or not.
  SRP bit(bit 7) in Flash Status Register is set if the following is defined.
*/
#define ISP_USE_WP_PROT

/***** #include statements ***************************************************/

#include "inv_common.h"
#include "inv_drv_cra_api.h"
#include "inv_drv_isp_api.h"
#include "inv_sys_log_api.h"

/***** local macro definitions ***********************************************/

#define ISP_OPCODE__WREN                    0x06
#define ISP_OPCODE__WREN_FOR_WRITE_PROT     0x50    /* Write enable for SRP write */
#define ISP_OPCODE__CHIP_ERASE              0x60
#define ISP_OPCODE__SECTOR_ERASE            0x20
#define ISP_OPCODE__BLOCK32K_ERASE          0x52
#define ISP_OPCODE__BLOCK64K_ERASE          0xD8
#define ISP_OPCODE__WRITE_STATUS            0x01    /* Write status is used for setting SRP = Status Register Protection and BP */
#define ISP_OPCODE__PROG                    0x02
#define ISP_OPCODE__DATA_RD                 0x03
#define ISP_OPCODE__STATUS                  0x05

#ifdef ISP_USE_WP_PROT
#define ISP_SREG_PROT                     0x98  /* SRP = 1. BP2 = 1, BP1 = 1 */
#define ISP_SREG_UNPROT                   0x80  /* SRP = 1. BP2 = 0, BP1 = 0 */
#else
#define ISP_SREG_PROT                     0x18  /* SRP = 0. BP2 = 1, BP1 = 1 */
#define ISP_SREG_UNPROT                   0x00  /* SRP = 0. BP2 = 0, BP1 = 0 */
#endif

#define REG_ADDR_AUTO_INC_DISABLE()         inv_drv_cra_write_8(dev_id, 0x000C, 0x02)   /* Auto increment mode disabled */
#define REG_ADDR_AUTO_INC_ENABLE()          inv_drv_cra_write_8(dev_id, 0x000C, 0x03)   /* Auto increment mode enabled  */

#define REG_ISP_DISABLE()                   inv_drv_cra_write_8(dev_id, 0x7030, 0x00)   /* Disable isp */
#define REG_ISP_ENABLE()                    inv_drv_cra_write_8(dev_id, 0x7030, 0x07)   /* Enable isp in page mode, MCU Clock/2 */
#define REG_ISP_BURST_LEN(len)              inv_drv_cra_write_8(dev_id, 0x708c, (uint8_t)(len-1))   /* Length of burst write */
#define REG_ISP_OPCODE_SET(opcode)          inv_drv_cra_write_8(dev_id, 0x7080, opcode) /* Enable for write access */
#define REG_ISP_ADDRESS_SET(addr)           inv_drv_cra_write_24(dev_id, 0x7084, addr)  /* Set flash address */
#define REG_ISP_DATA_WRITE(pdata, len)      inv_drv_cra_block_write_8(dev_id, 0x7088, pdata, len)   /* Burst Write data */
#define REG_ISP_DATA_READ(pdata, len)       inv_drv_cra_block_read_8(dev_id, 0x7088, pdata, len)    /* Burst Read data */
#define REG_ISP_STATUS_GET()                inv_drv_cra_read_8(dev_id, 0x708a)  /* Read status */
#define REG_ISP_STATUS_SET(pdata)           inv_drv_cra_write_8(dev_id, 0x708a, pdata)  /* Write status for Write Protection */

#define REG_TIMER_CONFIG()                  inv_drv_cra_write_8(dev_id, 0x00F0, 0x01)
#define REG_TIMER_INT_MASK_ENABLE1()        inv_drv_cra_write_16(dev_id, 0x0072, 0x0C00);
#define REG_TIMER_INT_MASK_ENABLE2()        inv_drv_cra_write_8(dev_id, 0x00F2, 0x01)
#define REG_TIMER_INT_STATUS_CLEAR()        inv_drv_cra_write_8(dev_id, 0x00F3, 0x01)
#define REG_TIMER_SET(val)                  inv_drv_cra_write_32(dev_id, 0x00F4, val*2000)


/***** local type definitions ************************************************/

/***** local prototypes ******************************************************/

static void s_timer_init(inv_platform_dev_id dev_id);
static void s_isp_enable(inv_platform_dev_id dev_id);
static bool_t s_flash_op_wait(inv_platform_dev_id dev_id, int timeout_ms, int to_granularity_ms);
static void s_wait_for_flash_until_busy(inv_platform_dev_id dev_id);
static void s_flash_protect(inv_platform_dev_id dev_id);
static void s_ensure_write_protect(inv_platform_dev_id dev_id);

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (inv_drv_isp, 13);

/***** local data objects ****************************************************/

/***** public functions ******************************************************/

void inv_drv_isp_event_int_set(inv_platform_dev_id dev_id)
{
    s_timer_init(dev_id);
    REG_TIMER_SET (1);
}

void inv_drv_isp_chip_erase(inv_platform_dev_id dev_id)
{
    s_isp_enable(dev_id);
    s_timer_init(dev_id);

    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__CHIP_ERASE);
    REG_TIMER_SET (FLASH_CHIP_ERASE_TIMEOUT);
}

bool_t inv_drv_isp_sector_erase_poll(inv_platform_dev_id dev_id, uint32_t addr)
{
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__SECTOR_ERASE);
    REG_ISP_ADDRESS_SET (addr);
    return s_flash_op_wait(dev_id, FLASH_SECTOR_ERASE_TIMEOUT, FLASH_OP_POLL_GRANULARITY);
}

void inv_drv_isp_sector_erase(inv_platform_dev_id dev_id, uint32_t addr)
{
    s_isp_enable(dev_id);
    s_timer_init(dev_id);
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__SECTOR_ERASE);
    REG_ISP_ADDRESS_SET (addr);
    REG_TIMER_SET (FLASH_SECTOR_ERASE_TIMEOUT);
}

bool_t inv_drv_isp_block_32k_erase_poll(inv_platform_dev_id dev_id, uint32_t addr)
{
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__BLOCK32K_ERASE);
    REG_ISP_ADDRESS_SET (addr);
    return s_flash_op_wait(dev_id, FLASH_32K_BLOCK_ERASE_TIMEOUT, FLASH_OP_POLL_GRANULARITY);
}

void inv_drv_isp_block_32k_erase(inv_platform_dev_id dev_id, uint32_t addr)
{
    s_isp_enable(dev_id);
    s_timer_init(dev_id);
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__BLOCK32K_ERASE);
    REG_ISP_ADDRESS_SET (addr);
    REG_TIMER_SET (FLASH_32K_BLOCK_ERASE_TIMEOUT);
}

bool_t inv_drv_isp_block_64k_erase_poll(inv_platform_dev_id dev_id, uint32_t addr)
{
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__BLOCK64K_ERASE);
    REG_ISP_ADDRESS_SET (addr);
    return s_flash_op_wait(dev_id, FLASH_64K_BLOCK_ERASE_TIMEOUT, FLASH_OP_POLL_GRANULARITY);
}

void inv_drv_isp_block_64k_erase(inv_platform_dev_id dev_id, uint32_t addr)
{
    s_isp_enable(dev_id);
    s_timer_init(dev_id);
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__BLOCK64K_ERASE);
    REG_ISP_ADDRESS_SET (addr);
    REG_TIMER_SET (FLASH_64K_BLOCK_ERASE_TIMEOUT);
}

void inv_drv_isp_burst_write(inv_platform_dev_id dev_id, uint32_t addr, const uint8_t *p_data, uint16_t len)
{
    REG_ISP_BURST_LEN (len);
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__PROG);
    REG_ISP_ADDRESS_SET (addr);
    REG_ADDR_AUTO_INC_DISABLE ();
    REG_ISP_DATA_WRITE (p_data, len);
    REG_ADDR_AUTO_INC_ENABLE ();
    REG_TIMER_SET (1);
}

void inv_drv_isp_burst_read(inv_platform_dev_id dev_id, uint32_t addr, uint8_t *p_data, uint16_t len)
{
    REG_ISP_OPCODE_SET (ISP_OPCODE__DATA_RD);
    REG_ISP_ADDRESS_SET (addr);
    REG_ADDR_AUTO_INC_DISABLE ();
    REG_ISP_DATA_READ (p_data, len);
    REG_ADDR_AUTO_INC_ENABLE ();
}

bool_t inv_drv_isp_operation_done(inv_platform_dev_id dev_id)
{
    REG_ISP_OPCODE_SET (ISP_OPCODE__STATUS);
    return ((REG_ISP_STATUS_GET () & 0x01) == 0x00) ? true : false;
}

void inv_drv_isp_flash_protect(inv_platform_dev_id dev_id)
{
    //INV_LOG1A("inv_drv_isp_flash_protect", p_obj, ("Write Protecting Flash\n"));
    s_flash_protect(dev_id);
}

void inv_drv_isp_flash_un_protect(inv_platform_dev_id dev_id)
{
    //INV_LOG1A("inv_drv_isp_flash_un_protect", p_obj, ("Un-Protect Flash\n"));

    s_ensure_write_protect(dev_id);

    s_isp_enable(dev_id);

    /*
     * WP_ pin to control flash write protection
     */
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN_FOR_WRITE_PROT);

    s_wait_for_flash_until_busy(dev_id);

    /*
     * Set SRP bit7 = 0 for protection
     */
    REG_ISP_OPCODE_SET (ISP_OPCODE__WRITE_STATUS);
    REG_ISP_STATUS_SET (ISP_SREG_UNPROT);

    s_wait_for_flash_until_busy(dev_id);
}

/***** local functions *******************************************************/

void s_timer_init(inv_platform_dev_id dev_id)
{
    REG_TIMER_INT_STATUS_CLEAR ();
    REG_TIMER_INT_MASK_ENABLE1();
    REG_TIMER_INT_MASK_ENABLE2();
    REG_TIMER_CONFIG ();
}

static void s_isp_enable(inv_platform_dev_id dev_id)
{
    REG_ISP_DISABLE ();
    REG_ISP_ENABLE ();
}

static bool_t s_flash_op_wait(inv_platform_dev_id dev_id, int timeout_ms, int to_granularity_ms)
{
    while (timeout_ms > 0) {
        inv_platform_sleep_msec((uint16_t) to_granularity_ms);
        if (inv_drv_isp_operation_done(dev_id))
            break;
        timeout_ms -= to_granularity_ms;
    }

    return (timeout_ms > 0) ? true : false;
}

static void s_wait_for_flash_until_busy(inv_platform_dev_id dev_id)
{
    uint8_t status;

    do {
        REG_ISP_OPCODE_SET (ISP_OPCODE__STATUS);
        status = REG_ISP_STATUS_GET ();
        INV_LOG2A ("invDrvIspWaitForFlashUntilBusy", p_obj, ("Status register = %02X\n", status));
    } while (status & 1);

}

static void s_flash_protect(inv_platform_dev_id dev_id)
{
    s_isp_enable(dev_id);

    /*
     * WP_ pin to control flash write protection
     */
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN);
    REG_ISP_OPCODE_SET (ISP_OPCODE__WREN_FOR_WRITE_PROT);

    /*
     * Set SRP bit7 = 1 for protection
     */
    REG_ISP_OPCODE_SET (ISP_OPCODE__WRITE_STATUS);
    REG_ISP_STATUS_SET (ISP_SREG_PROT);

    s_wait_for_flash_until_busy(dev_id);
}

static void s_ensure_write_protect(inv_platform_dev_id dev_id)
{
    uint8_t status;

    REG_ISP_OPCODE_SET (ISP_OPCODE__STATUS);
    status = REG_ISP_STATUS_GET ();
    if (status != ISP_SREG_PROT) {
        INV_LOG2A ("invDrvIspEnsureWriteProtect", p_obj, ("Flash was already unprotected(%02X). Must protect first.\n", status));
        s_flash_protect(dev_id);
    }
}

/***** end of file ***********************************************************/
