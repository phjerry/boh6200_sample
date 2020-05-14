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
* @file inv_drv_ipc.c
*
* @brief Host Interface handler
*
*****************************************************************************/
#define INV_DEBUG 1

/***** #include statements ***************************************************/

#include "inv_common.h"
#include "inv_drv_cra_api.h"
#include "inv_drv_ipc_api.h"

/***** Register Module name **************************************************/

INV_MODULE_NAME_SET (inv_drv_ipc, 28);

/***** local macro definitions ***********************************************/

#define USE_LAST_MESSAGE_ACK_LOGIC
#define INV_IPC_TIMER_TASK_EXECUTION
#define MESSAGE_SIZE                        (268)

#define INV_IPC_PACKET_PAYLOAD_ADDR_SIZE    (4)
#define BUFFER_TOTAL_SIZE                   (63)
#define BUFFER_PAYLOAD_SIZE                 (BUFFER_TOTAL_SIZE-INV_IPC_PACKET_PAYLOAD_ADDR_SIZE)

#define PACKET_LAST_SEGMENT_FLAG            0x80

#define IPC_NOTIFY_MESSAGE_BUFFER_MAX       0x3000

/***** local type definitions ************************************************/

struct message {
    uint16_t size;
    uint8_t pay_load[MESSAGE_SIZE];
};

struct mes {
    enum inv_drv_ipc_type type;
    inv_drv_ipc_op_code op_code;
    inv_drv_ipc_tag_code tag_code;
    uint16_t par_size;
    uint16_t par_pntr;
    uint8_t par_data[MESSAGE_SIZE];
};

struct ipc_obj {
    struct inv_drv_ipc_config config;
    uint8_t rx_seq_num;
    uint8_t rx_frg_num;
    uint16_t rx_chr_cnt;
    uint16_t rx_msg_siz;
    struct message rx_message;
    uint8_t tx_seq_num;
    uint8_t tx_frg_num;
    uint16_t tx_chr_cnt1;
    uint16_t tx_chr_cnt2;
    uint16_t tx_msg_siz;
    uint8_t tx_retry;
    struct message tx_message;
};

/***** externally defined hal methods ****************************************/

void inv_hal_ipc_token_assert(inv_platform_dev_id dev_id, inv_drv_ipc_token_t flags);
uint8_t inv_hal_ipc_fifo_write(inv_platform_dev_id dev_id, uint8_t *p_buf, uint8_t len);
uint8_t inv_hal_ipc_fifo_read(inv_platform_dev_id dev_id, uint8_t *p_buf, uint8_t len);

/***** local prototypes ******************************************************/

static void s_message_clear(struct mes *p_mes);
static void s_message_copy(struct mes *p_mes_des, struct mes *p_mes_src);
static void s_type_set(struct message *p_message, enum inv_drv_ipc_type type);
static enum inv_drv_ipc_type s_type_get(struct message *p_message);
static void s_op_code_set(struct message *p_message, inv_drv_ipc_op_code op_code);
static inv_drv_ipc_op_code s_op_code_get(struct message *p_message);
static void s_tag_code_set(struct message *p_message, inv_drv_ipc_tag_code tag_code);
static inv_drv_ipc_tag_code s_tag_code_get(struct message *p_message);
static uint16_t s_par_size_get(struct message *p_message);
static void s_par_data_set(struct message *p_message, uint8_t *p_data, uint16_t par_size);
static void s_par_data_get(struct message *p_message, uint8_t *p_data);
static void s_check_sum_update(struct message *p_message);
static bool_t s_message_valid_is(struct message *p_message);

/* Packet layer functions */
static void s_raw_message_clear(struct message *p_message);
static uint8_t s_check_sum_get(uint8_t *p_buf, uint16_t len);
static bool_t s_check_sum_failed(uint8_t *p_buf, uint16_t len);
static void s_send_message_handler(struct ipc_obj *p_obj);
static void s_receive_message_handler(struct ipc_obj *p_obj);
static void s_token_handler(struct ipc_obj *p_obj, inv_drv_ipc_token_t flags);
static uint16_t s_packet_message_send(struct ipc_obj *p_obj, const struct message *ip_message);
static uint16_t s_packet_message_receive(struct ipc_obj *p_obj, struct message *op_message);

/***** local data objects ****************************************************/

static inv_inst_t s_obj_list_inst = INV_INST_NULL;

/***** public functions ******************************************************/

inv_drv_ipc_inst inv_drv_ipc_create(const char *p_name_str, inv_inst_t parent_inst, struct inv_drv_ipc_config *p_config)
{
    struct ipc_obj *p_obj = NULL;

    if (!s_obj_list_inst) {
        INV_ASSERT (!s_obj_list_inst);
        s_obj_list_inst = inv_sys_obj_list_create(INV_MODULE_NAME_GET (), sizeof(struct ipc_obj));
        INV_ASSERT (s_obj_list_inst);
    }

    /*
     * allocate memory for object
     */
    p_obj = (struct ipc_obj *) inv_sys_obj_instance_create(s_obj_list_inst, parent_inst, p_name_str);
    INV_ASSERT (p_obj);
    INV_LOG2A ("create", p_obj, (""));

    /*
     * DBG_LOG0(DBG_ERR, NEW, inv_sys_obj_name_get(p_obj), "create");
     */

    p_obj->config = *p_config;

   /*--------------------------------*/
    /*
     * Initialize internal states
     */
   /*--------------------------------*/
    p_obj->rx_seq_num = 0;
    p_obj->rx_frg_num = 0;
    p_obj->rx_chr_cnt = 0;
    p_obj->rx_msg_siz = 0;
    s_raw_message_clear(&p_obj->rx_message);

    p_obj->tx_seq_num = 0;
    p_obj->tx_frg_num = 0;
    p_obj->tx_chr_cnt1 = 0;
    p_obj->tx_chr_cnt2 = 0;
    p_obj->tx_msg_siz = 0;
    p_obj->tx_retry = 0;
    s_raw_message_clear(&p_obj->tx_message);

    return INV_OBJ2INST (p_obj);
}


void inv_drv_ipc_delete(inv_drv_ipc_inst ipc_inst)
{
    struct ipc_obj *p_obj = (struct ipc_obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, ipc_inst);

    inv_sys_obj_instance_delete(p_obj);
    if (!inv_sys_obj_first_get(s_obj_list_inst)) {

        /*
         * Delete list of objects
         */
        inv_sys_obj_list_delete(s_obj_list_inst);
        s_obj_list_inst = INV_INST_NULL;
    }
}

void inv_drv_ipc_handler(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_token_t token)
{
    struct ipc_obj *p_obj = (struct ipc_obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, ipc_inst);

    s_token_handler(p_obj, token);
}

void inv_drv_ipc_message_send(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_mes_inst mes_inst)
{
    struct ipc_obj *p_obj = (struct ipc_obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, ipc_inst);
    struct mes *p_mes = (struct_mes_t) mes_inst;
    struct message pack_mes;

    s_type_set(&pack_mes, p_mes->type);
    s_op_code_set(&pack_mes, p_mes->op_code);
    s_tag_code_set(&pack_mes, p_mes->tag_code);
    p_mes->par_size = p_mes->par_pntr;
    s_par_data_set(&pack_mes, p_mes->par_data, p_mes->par_size);
    s_check_sum_update(&pack_mes);
    s_packet_message_send(p_obj, &pack_mes);
}

void inv_drv_ipc_message_receive(inv_drv_ipc_inst ipc_inst, inv_drv_ipc_mes_inst mes_inst)
{
    struct ipc_obj *p_obj = (struct ipc_obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, ipc_inst);
    struct mes *p_mes = (struct_mes_t) mes_inst;
    struct message pack_mes;

    s_packet_message_receive(p_obj, &pack_mes);
    p_mes->type = s_type_get(&pack_mes);
    p_mes->op_code = s_op_code_get(&pack_mes);
    p_mes->tag_code = s_tag_code_get(&pack_mes);
    p_mes->par_size = s_par_size_get(&pack_mes);
    p_mes->par_pntr = 0;
    s_par_data_get(&pack_mes, p_mes->par_data);
}

bool_t inv_drv_ipc_message_sent(inv_drv_ipc_inst ipc_inst)
{
    struct ipc_obj *p_obj = (struct ipc_obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, ipc_inst);

    return (p_obj->tx_message.size) ? (false) : (true);
}

/*#ifdef UNUSED */
/*
bool_t inv_drv_ipc_message_received(inv_drv_ipc_inst ipc_inst)
{
   struct ipc_obj *p_obj = (struct ipc_obj *)INV_SYS_OBJ_CHECK(s_obj_list_inst, ipc_inst);

   return (p_obj->rx_message.size) ? (false) : (true);
}
*/


void inv_drv_ipc_tx_reset(inv_drv_ipc_inst ipc_inst)
{
    struct ipc_obj *p_obj = (struct ipc_obj *) INV_SYS_OBJ_CHECK (s_obj_list_inst, ipc_inst);

    /*
     * reset IPC itself
     */
    p_obj->tx_message.size = 0;
    p_obj->tx_chr_cnt1 = 0;
    p_obj->tx_seq_num++;        /*  Increase sequence number for next message */
}

inv_drv_ipc_mes_inst inv_drv_ipc_message_create(void)
{

    /*
     * Allocate memory for object
     */
    struct mes *p_mes = (struct_mes_t) inv_sys_malloc_create(sizeof(struct mes));

    INV_ASSERT (p_mes);

    s_message_clear(p_mes);
    return (inv_drv_ipc_mes_inst) p_mes;
}


void inv_drv_ipc_message_delete(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    inv_sys_malloc_delete(p_mes);
}

void inv_drv_ipc_message_clear(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    s_message_clear(p_mes);
}


void inv_drv_ipc_message_copy(inv_drv_ipc_mes_inst mes_des_inst, inv_drv_ipc_mes_inst mes_src_inst)
{
    struct mes *p_mes_des = (struct_mes_t) mes_des_inst;
    struct mes *p_mes_src = (struct_mes_t) mes_src_inst;

    s_message_copy(p_mes_des, p_mes_src);
}

/*#endif */

void inv_drv_ipc_type_set(inv_drv_ipc_mes_inst mes_inst, enum inv_drv_ipc_type type)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    p_mes->type = type;
}

enum inv_drv_ipc_type inv_drv_ipc_type_get(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    return p_mes->type;
}

void inv_drv_ipc_op_code_set(inv_drv_ipc_mes_inst mes_inst, inv_drv_ipc_op_code op_code)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    p_mes->op_code = op_code;
}

inv_drv_ipc_op_code inv_drv_ipc_op_code_get(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    return p_mes->op_code;
}

void inv_drv_ipc_tag_code_set(inv_drv_ipc_mes_inst mes_inst, inv_drv_ipc_tag_code tag_code)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    p_mes->tag_code = tag_code;
}

inv_drv_ipc_tag_code inv_drv_ipc_tag_code_get(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    return p_mes->tag_code;
}

void inv_drv_ipc_bool_push(inv_drv_ipc_mes_inst mes_inst, bool_t par)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    INV_ASSERT (MESSAGE_SIZE >= (p_mes->par_pntr + 1));
    if (MESSAGE_SIZE >= (p_mes->par_pntr + 1))
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) par;
}

bool_t inv_drv_ipc_bool_pop(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;
    bool_t par = false;

    INV_ASSERT (p_mes->par_size >= (p_mes->par_pntr + 1));
    if (p_mes->par_size >= (p_mes->par_pntr + 1))
        par = (p_mes->par_data[p_mes->par_pntr++] != 0);
    return par;
}

void inv_drv_ipc_byte_push(inv_drv_ipc_mes_inst mes_inst, uint8_t par)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    INV_ASSERT (MESSAGE_SIZE >= (p_mes->par_pntr + 1));
    if (MESSAGE_SIZE >= (p_mes->par_pntr + 1))
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 0);
}

uint8_t inv_drv_ipc_byte_pop(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;
    uint8_t par = 0;

    INV_ASSERT (p_mes->par_size >= (p_mes->par_pntr + 1));
    if (p_mes->par_size >= (p_mes->par_pntr + 1))
        par = (p_mes->par_data[p_mes->par_pntr++] << 0);
    return par;
}

void inv_drv_ipc_word_push(inv_drv_ipc_mes_inst mes_inst, uint16_t par)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    INV_ASSERT (MESSAGE_SIZE >= (p_mes->par_pntr + 2));
    if (MESSAGE_SIZE >= (p_mes->par_pntr + 2)) {
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 0);
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 8);
    }
}

uint16_t inv_drv_ipc_word_pop(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;
    uint16_t par = 0;

    INV_ASSERT (p_mes->par_size >= (p_mes->par_pntr + 2));
    if (p_mes->par_size >= (p_mes->par_pntr + 2)) {
        par |= (p_mes->par_data[p_mes->par_pntr++] << 0);
        par |= (p_mes->par_data[p_mes->par_pntr++] << 8);
    }
    return par;
}

void inv_drv_ipc_long_push(inv_drv_ipc_mes_inst mes_inst, uint32_t par)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    INV_ASSERT (MESSAGE_SIZE >= (p_mes->par_pntr + 4));
    if (MESSAGE_SIZE >= (p_mes->par_pntr + 4)) {
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 0);
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 8);
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 16);
        p_mes->par_data[p_mes->par_pntr++] = (uint8_t) (par >> 24);
    }
}

uint32_t inv_drv_ipc_long_pop(inv_drv_ipc_mes_inst mes_inst)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;
    uint32_t par = 0;

    INV_ASSERT (p_mes->par_size >= (p_mes->par_pntr + 4));
    if (p_mes->par_size >= (p_mes->par_pntr + 4)) {
        par |= (((uint32_t) p_mes->par_data[p_mes->par_pntr++]) << 0);
        par |= (((uint32_t) p_mes->par_data[p_mes->par_pntr++]) << 8);
        par |= (((uint32_t) p_mes->par_data[p_mes->par_pntr++]) << 16);
        par |= (((uint32_t) p_mes->par_data[p_mes->par_pntr++]) << 24);
    }
    return par;
}

void inv_drv_ipc_array_push(inv_drv_ipc_mes_inst mes_inst, const uint8_t *p_par, uint16_t size)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    INV_ASSERT (MESSAGE_SIZE >= (p_mes->par_pntr + size));
    if (MESSAGE_SIZE >= (p_mes->par_pntr + size)) {
        INV_MEMCPY (&p_mes->par_data[p_mes->par_pntr], p_par, size);
        p_mes->par_pntr += size;
    }
}

void inv_drv_ipc_array_pop(inv_drv_ipc_mes_inst mes_inst, uint8_t *p_par, uint16_t size)
{
    struct mes *p_mes = (struct_mes_t) mes_inst;

    INV_ASSERT (p_mes->par_size >= (p_mes->par_pntr + size));
    if (p_mes->par_size >= (p_mes->par_pntr + size)) {
        INV_MEMCPY (p_par, &p_mes->par_data[p_mes->par_pntr], size);
        p_mes->par_pntr += size;
    }
}

/***** local functions *******************************************************/

static void s_message_clear(struct mes *p_mes)
{
    p_mes->type = INV_DRV_IPC_TYPE__NONE;
    p_mes->op_code = 0;
    p_mes->tag_code = 0;
    p_mes->par_size = 0;
    p_mes->par_pntr = 0;
    INV_MEMSET (p_mes->par_data, 0, MESSAGE_SIZE);
}


static void s_message_copy(struct mes *p_mes_des, struct mes *p_mes_src)
{
    p_mes_des->type = p_mes_src->type;
    p_mes_des->op_code = p_mes_src->op_code;
    p_mes_des->tag_code = p_mes_src->tag_code;
    p_mes_des->par_size = p_mes_src->par_size;
    p_mes_des->par_pntr = p_mes_src->par_pntr;
    INV_MEMCPY (p_mes_des->par_data, p_mes_src->par_data, MESSAGE_SIZE);
}

static void s_type_set(struct message *p_message, enum inv_drv_ipc_type type)
{
    uint8_t val = 0;

    switch (type) {
    case INV_DRV_IPC_TYPE__NONE:
        val = 0;
        break;
    case INV_DRV_IPC_TYPE__REQUEST:
        val = 1;
        break;
    case INV_DRV_IPC_TYPE__RESPONSE:
        val = 2;
        break;
    case INV_DRV_IPC_TYPE__NOTIFY:
        val = 3;
        break;
    default:
        break;
    }
    p_message->pay_load[1] = (p_message->pay_load[1] & 0x0F) | (val << 4);
}

static enum inv_drv_ipc_type s_type_get(struct message *p_message)
{
    enum inv_drv_ipc_type type = INV_DRV_IPC_TYPE__NONE;

    switch (p_message->pay_load[1] >> 4) {
    case 0x1:
        type = INV_DRV_IPC_TYPE__REQUEST;
        break;
    case 0x2:
        type = INV_DRV_IPC_TYPE__RESPONSE;
        break;
    case 0x3:
        type = INV_DRV_IPC_TYPE__NOTIFY;
        break;
    default:
        break;
    }
    return type;
}

static void s_op_code_set(struct message *p_message, inv_drv_ipc_op_code op_code)
{
    p_message->pay_load[0] = (uint8_t) (op_code & 0x00FF);
    p_message->pay_load[1] = (p_message->pay_load[1] & 0xF0) | (uint8_t) ((op_code >> 8) & 0x000F);
}

static inv_drv_ipc_op_code s_op_code_get(struct message *p_message)
{
    return (inv_drv_ipc_op_code) ((((uint16_t) (p_message->pay_load[1] & 0x0F)) << 8) | p_message->pay_load[0]);
}

static void s_tag_code_set(struct message *p_message, inv_drv_ipc_tag_code tag_code)
{
    p_message->pay_load[4] = (uint8_t) (tag_code);
}

static inv_drv_ipc_tag_code s_tag_code_get(struct message *p_message)
{
    return (inv_drv_ipc_tag_code) p_message->pay_load[4];
}

static uint16_t s_par_size_get(struct message *p_message)
{
    return (uint16_t) ((((uint16_t) (p_message->pay_load[3] & 0x3)) << 8) | p_message->pay_load[2]);
}

static void s_par_data_set(struct message *p_message, uint8_t *p_data, uint16_t par_size)
{
    p_message->pay_load[2] = (uint8_t) (par_size & 0x00FF);
    p_message->pay_load[3] = (uint8_t) ((par_size >> 8) & 0x0003);
    INV_MEMCPY (&p_message->pay_load[6], p_data, par_size);
}

static void s_par_data_get(struct message *p_message, uint8_t *p_data)
{
    INV_MEMCPY (p_data, &p_message->pay_load[6], p_message->size - 6);
}

static void s_check_sum_update(struct message *p_message)
{
    uint16_t size = 6 + s_par_size_get(p_message);
    uint8_t *p_data = p_message->pay_load;
    uint8_t cs = 0;

    p_message->size = size;
    p_message->pay_load[5] = 0;
    while (size--) {
        cs += *p_data;
        p_data++;
    }
    p_message->pay_load[5] = (uint8_t) (0 - ((int16_t) cs));
}

static bool_t s_message_valid_is(struct message *p_message)
{
    uint16_t size = p_message->size;
    uint8_t *p_data = p_message->pay_load;
    uint8_t cs = 0;

    while (size--) {
        cs += *p_data;
        p_data++;
    }
    return (cs) ? (false) : (true);
}

/*---------------------------------------------------------------------------*/
/* Packet layer functions                                                    */
/*---------------------------------------------------------------------------*/

static void s_raw_message_clear(struct message *p_message)
{
    p_message->size = 0;
    INV_MEMSET (p_message->pay_load, 0, MESSAGE_SIZE);
}

static uint8_t s_check_sum_get(uint8_t *p_buf, uint16_t len)
{
    uint8_t checksum = 0;

    while (len--) {
        checksum += *p_buf;
        p_buf++;
    }
    return (uint8_t) ((int16_t) 0 - checksum);
}

static bool_t s_check_sum_failed(uint8_t *p_buf, uint16_t len)
{
    uint8_t checksum = 0;

    while (len--) {
        checksum += *p_buf;
        p_buf++;
    }
    return (checksum) ? (true) : (false);
}

static void s_send_message_handler(struct ipc_obj *p_obj)
{
    uint8_t buffer[BUFFER_TOTAL_SIZE];
    uint8_t *pbuf = buffer;
    uint16_t check = p_obj->tx_message.size - p_obj->tx_chr_cnt1;
    uint8_t chr_cnt = 0;

    p_obj->tx_chr_cnt2 = p_obj->tx_chr_cnt1;

    if (BUFFER_PAYLOAD_SIZE < check)
        chr_cnt = BUFFER_PAYLOAD_SIZE;
    else {
        chr_cnt = (uint8_t) check;

        /*
         * Flag fragment number as last packet of message
         */
        p_obj->tx_frg_num |= 0x80;
    }

    *(pbuf++) = p_obj->tx_seq_num;
    *(pbuf++) = p_obj->tx_frg_num;
    *(pbuf++) = chr_cnt;
    *(pbuf++) = 0;
    INV_MEMCPY (pbuf, &p_obj->tx_message.pay_load[p_obj->tx_chr_cnt2], chr_cnt);
    p_obj->tx_chr_cnt2 += chr_cnt;
    chr_cnt += 4;

    /*
     * Write Checksum
     */
    buffer[3] = s_check_sum_get(buffer, chr_cnt);

    /*
     * Write packet to shared memory
     */
    inv_hal_ipc_fifo_write(p_obj->config.dev_id, buffer, chr_cnt);

    /*
     * set ready flag for receiver
     */
    inv_hal_ipc_token_assert(p_obj->config.dev_id, INV_DRV_IPC_TOKEN__PACKET_RDY);

    INV_LOG2A ("s_send_message_handler", 0, ("seq=%02X, frg=%02X, len=%02X, cs=%02X\n", (uint16_t) p_obj->tx_seq_num, (uint16_t) p_obj->tx_frg_num, (uint16_t) chr_cnt, (uint16_t) buffer[3]));
}

static void s_receive_message_handler(struct ipc_obj *p_obj)
{
    uint8_t buffer[BUFFER_TOTAL_SIZE];
    bool_t b_no_error = true;
    uint8_t seq_num = 0;
    uint8_t frg_num = 0;
    uint8_t datlen = 0;
    bool_t b_last_frag = false;

    /*
     * read packet header(4 bytes)
     */
    if (4 == inv_hal_ipc_fifo_read(p_obj->config.dev_id, buffer, 4)) {
        seq_num = buffer[0];
        frg_num = buffer[1] & 0x7F;
        datlen = buffer[2];
        b_last_frag = (buffer[1] & 0x80) ? (true) : (false);

        /*
         * check data length byte
         */
        if (BUFFER_PAYLOAD_SIZE < datlen) {
            INV_LOG2B (("Too many bytes in package!\n"));
            b_no_error = false;
        }
    } else {
        INV_LOG2B (("Not able to read 4-byte header!\n"));
        b_no_error = false;
    }
    INV_LOG2A ("s_receive_message_handler", 0, ("seq=%02X, frg=%02X, len=%02X, cs=%02X\n", (uint16_t) buffer[0], (uint16_t) buffer[1], (uint16_t) buffer[2], (uint16_t) buffer[3]));

    /*
     * Read rest of packet
     */
    if (b_no_error) {
        if (datlen != inv_hal_ipc_fifo_read(p_obj->config.dev_id, buffer + 4, datlen)) {
            INV_LOG2B (("Failure to read packet!\n"));
            b_no_error = false;
        }
    }

    /*
     * check checksum value
     */
    if (b_no_error) {
        if (s_check_sum_failed(buffer, datlen + 4)) {
            inv_hal_ipc_token_assert(p_obj->config.dev_id, INV_DRV_IPC_TOKEN__NACK);   /* checksum error */
            INV_LOG2B (("Wrong checksum!\n"));
            b_no_error = false;
        }
    }

    /*
     * Check fragment number
     */
    if (b_no_error) {
        if (0 == p_obj->rx_frg_num) {

            /*
             * Reset message sequence number
             */
            p_obj->rx_seq_num = seq_num;
        } else if (frg_num != p_obj->rx_frg_num) {
            inv_hal_ipc_token_assert(p_obj->config.dev_id, INV_DRV_IPC_TOKEN__NACK);   /* message is too big */
            INV_LOG2B (("Wrong fragment number!\n"));
            b_no_error = false;
        }
    }

    /*
     * Check message sequence number
     */
    if (b_no_error) {
        if (p_obj->rx_seq_num != seq_num) {
            inv_hal_ipc_token_assert(p_obj->config.dev_id, INV_DRV_IPC_TOKEN__NACK);   /* Wrong message sequence number received */
            INV_LOG2B (("Packet with wrong sequence number!\n"));
            b_no_error = false;
        }
    }

    /*
     * Process Packet
     */
    if (b_no_error) {
        INV_MEMCPY (&p_obj->rx_message.pay_load[p_obj->rx_chr_cnt], &buffer[4], datlen);
        p_obj->rx_chr_cnt += datlen;
        p_obj->rx_frg_num++;
        p_obj->rx_message.size = 0;

        if (b_last_frag) {
            p_obj->rx_message.size = p_obj->rx_chr_cnt;
            p_obj->rx_chr_cnt = 0;
            p_obj->rx_frg_num = 0;
        }

        /*
         * Indicate to sender that packet has been accepted
         */
        inv_hal_ipc_token_assert(p_obj->config.dev_id, INV_DRV_IPC_TOKEN__ACK);

        if (b_last_frag) {

            /*
             * Call back parent before acknowledging last packet
             */
            if (s_message_valid_is(&p_obj->rx_message))
                inv_drv_ipc_call_back(INV_OBJ2INST (p_obj), INV_DRV_IPC_EVENT__MESSAGE_RECEIVED);
        }
    } else {

        /*
         * Reset packet state state machine
         */
        p_obj->rx_chr_cnt = 0;
        p_obj->rx_frg_num = 0;
        inv_hal_ipc_token_assert(p_obj->config.dev_id, INV_DRV_IPC_TOKEN__NACK);
    }
}

static void s_token_handler(struct ipc_obj *p_obj, inv_drv_ipc_token_t token)
{
    INV_LOG2A ("s_token_handler", 0, ("Asserted tokens : 0x%02X\n", (uint16_t) token));

    if (token & INV_DRV_IPC_TOKEN__NACK) {
        if (p_obj->tx_message.size) {
            p_obj->tx_retry++;
            if (5 <= p_obj->tx_retry)
                /*
                 * T.b.d. some error handling may be needed here
                 */
                p_obj->tx_message.size = 0;
            else
                s_send_message_handler(p_obj); /* send remaining packets */

        }
    }

    if (token & INV_DRV_IPC_TOKEN__ACK) {
        if (p_obj->tx_message.size) {
            p_obj->tx_chr_cnt1 = p_obj->tx_chr_cnt2;

            if (p_obj->tx_message.size > p_obj->tx_chr_cnt1) {
                p_obj->tx_frg_num++;    /* Increase fragment number */
                p_obj->tx_retry = 0;    /* Reset retry counter */
                s_send_message_handler(p_obj); /* send remaining packets */
            } else {
                p_obj->tx_message.size = 0; /* Message has been sent */
                p_obj->tx_chr_cnt1 = 0; /* Message has been sent */
                p_obj->tx_seq_num++;    /* Increase sequence number for next message */
                inv_drv_ipc_call_back(INV_OBJ2INST (p_obj), INV_DRV_IPC_EVENT__MESSAGE_SENT);
            }
        }
    }

    if (token & INV_DRV_IPC_TOKEN__PACKET_RDY)
        s_receive_message_handler(p_obj);
}

uint16_t s_packet_message_send(struct ipc_obj *p_obj, const struct message *ip_message)
{
    INV_LOG2A ("s_packet_message_send", 0, ("size=%d,", ip_message->size));
    {
        uint16_t i = 0;

        for (i = 0; i < ip_message->size; i++)
            INV_LOG2B ((" %02X ", (uint16_t) ip_message->pay_load[i]));
    }

    INV_ASSERT (0 == p_obj->tx_message.size);

    INV_ASSERT (MESSAGE_SIZE >= ip_message->size);

    p_obj->tx_message = *ip_message;
    p_obj->tx_frg_num = 0;      /* reset fragment numer */
    s_send_message_handler(p_obj); /* start sending first packet */
    return ip_message->size;
}

static uint16_t s_packet_message_receive(struct ipc_obj *p_obj, struct message *op_message)
{
    uint16_t size = p_obj->rx_message.size;

    INV_LOG2A ("s_packet_message_receive", 0, ("size = %d,", size));
    if (size) {
        *op_message = p_obj->rx_message;
        {
            uint16_t i = 0;

            for (i = 0; i < size; i++)
                INV_LOG2B ((" %02X ", (uint16_t) p_obj->rx_message.pay_load[i]));
        }
    }
    return size;
}

/***** end of file ***********************************************************/
