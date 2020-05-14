/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: The sample for complex_sci
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "hi_unf_sci.h"

hi_u8 g_receive_buffer[512]; /* define receive buffer length 512 */

#define TEST_NEGOTIATION 0
#define TEST_SET_ETU 0
#define TEST_OTHER 0
#define T0_TRANSFER
#define MS 1000

#define ACTION_CARDIN 0       /* card inset flag */
#define ACTION_CARDOUT 1      /* card pull out flag */
#define ACTION_NONE 2         /* card immobile */

/* t0_aidide_zeta */
hi_u8 g_t0_aidide_zeta_send[][22] = { /* 22: array member max length is 22 */
    { 0xd2, 0x42, 0x00, 0x00, 0x01, 0x1d },
    { 0xd2, 0xfe, 0x00, 0x00, 0x19 },
    {
        0xd2, 0x40, 0x00, 0x00, 0x11,
        0x03, 0x06, 0x14, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x1d
    },
    { 0xd2, 0xfe, 0x00, 0x00, 0x09 },
    { 0xd2, 0x3c, 0x00, 0x00, 0x01, 0x22 },
    { 0xd2, 0xfe, 0x00, 0x00, 0x15 },
    { 0xd2, 0x3e, 0x00, 0x00, 0x01, 0x23 },
    { 0xd2, 0xfe, 0x00, 0x00, 0x0b },
    { 0xd2, 0x00, 0x00, 0x00, 0x01, 0x3c },
    { 0xd2, 0xfe, 0x00, 0x00, 0x1d },
};
hi_u8 g_t0_aidide_zeta_receive[][32] = { /* 32: array member max length is 32 */
    { 0x90, 0x19 },
    {
        0x01, 0x02, 0x00, 0x00,
        0x21, 0x00, 0x00, 0x10, 0x03,
        0x06, 0x14, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x1c, 0x90, 0x00
    },
    { 0x90, 0x09 },
    {
        0x01, 0x02, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0x1c,
        0x90, 0x00
    },
    { 0x90, 0x15 },
    {
        0x01, 0x02, 0x00, 0x00,
        0x1e, 0x00, 0x00, 0x0c, 0x53,
        0x36, 0x01, 0x0e, 0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x4c, 0x90, 0x00
    },
    { 0x90, 0x0b },
    {
        0x01, 0x02, 0x00, 0x00,
        0x1f, 0x00, 0x00, 0x02, 0x00,
        0x02, 0x23, 0x90, 0x00
    },
    { 0x90, 0x1d },
    {
        0x01, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x14, 0x34,
        0x30, 0x32, 0x30, 0x33, 0x32,
        0x35, 0x31, 0x35, 0x32, 0x54,
        0x36, 0x33, 0x38, 0x35, 0x30,
        0x30, 0x30, 0x41, 0x00, 0x01,
        0x90, 0x00
    },
};

hi_u32 g_t0_aidide_zeta_send_length[10] = { 6, 5, 22, 5, 6, 5, 6, 5, 6, 5 }; /* 10: The length of send_array is 10 */
hi_u32 g_t0_aidide_zeta_receive_length[10] = { 2, 27, 2, 11, 2, 23, 2, 13, 2, 31 }; /* 10: The length array is 10 */

/* T1 suantong */
hi_u8 g_atr_suantong_t1[18] = { /* 18: array length is 19 */
    0x3b, 0xe9, 0x00, 0x00, 0x81,
    0x31, 0xc3, 0x45, 0x99, 0x63,
    0x74, 0x69, 0x19, 0x99, 0x12,
    0x56, 0x10, 0xec
};
hi_u8 g_t1_suantong_send[][23] = { /* 23: array member max length is 23 */
    { 0x00, 0x00, 0x06, 0x00, 0x32, 0x10, 0x01, 0x01, 0x01, 0x25 },
    { 0x00, 0x00, 0x05, 0x00, 0x25, 0xa0, 0x21, 0x20, 0x81 },
    { 0x00, 0x00, 0x05, 0x81, 0xdd, 0x00, 0x10, 0x04, 0x4d },
    { 0x00, 0x00, 0x05, 0x81, 0xd4, 0x00, 0x01, 0x05, 0x54 },
    { 0x00, 0x00, 0x05, 0x81, 0xd4, 0x00, 0x01, 0x0b, 0x5a },
    { 0x00, 0x00, 0x05, 0x81, 0xd0, 0x00, 0x01, 0x08, 0x5d },
    { 0x00, 0x00, 0x05, 0x81, 0xc0, 0x00, 0x01, 0x0a, 0x4f },
    { 0x00, 0x00, 0x05, 0x00, 0x31, 0x05, 0x00, 0x08, 0x39 },
    { 0x00, 0x00, 0x05, 0x81, 0xd1, 0x00, 0x01, 0x10, 0x44 },
    { 0x00, 0x00, 0x05, 0x81, 0xd2, 0x00, 0x01, 0x10, 0x47 },
    { 0x00, 0x00, 0x05, 0x81, 0xa3, 0x00, 0x00, 0x05, 0x22 },
    { 0x00, 0x00, 0x05, 0x81, 0xa3, 0x00, 0x01, 0x05, 0x23 },
    { 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x02, 0x01, 0x23 },
    {
        0x00, 0x00, 0x0e, 0x02, 0x68, 0x00, 0x00, 0x09, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x82, 0x02, 0x50, 0x03, 0xbf
    },
    {
        0x00, 0x00, 0x13, 0x02, 0x68, 0x00, 0x00, 0x0e, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x82, 0x07, 0x90, 0x03, 0x00, 0x00, 0x10,
        0xa7, 0x10, 0xc7
    },
    {
        0x00, 0x00, 0x13, 0x02, 0x68, 0x00, 0x00, 0x0e, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x82, 0x07, 0x90, 0x03, 0x00, 0x00, 0x20,
        0xa7, 0x10, 0xf7
    },
    {
        0x00, 0x00, 0x13, 0x02, 0x68, 0x00, 0x00, 0x0e, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x82, 0x07, 0x90, 0x03, 0x00, 0x00, 0x30,
        0xa7, 0x10, 0xe7
    },
    { 0x00, 0x00, 0x05, 0x81, 0xd4, 0x00, 0x01, 0x0b, 0x5a },
    { 0x00, 0x00, 0x05, 0x00, 0x25, 0x80, 0x14, 0x01, 0xb5 },
};
hi_u8 g_t1_suantong_receive[][36] = { /* 36: array member max length is 36 */
    { 0x00, 0x00, 0x02, 0x90, 0x00, 0x92 },
    { 0x00, 0x00, 0x02, 0x6a, 0x86, 0xee },
    { 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x01, 0x90, 0x00, 0x96 },
    {
        0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x90, 0x00,
        0x96
    },
    {
        0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x86, 0xf1, 0x90, 0x00, 0xeb
    },
    {
        0x00, 0x00, 0x0a, 0x33, 0xc5, 0xcf, 0xc0, 0x02, 0xed, 0x02,
        0x7b, 0x90, 0x00, 0xf5
    },
    { 0x00, 0x00, 0x04, 0x30, 0x00, 0x90, 0x00, 0xa4 },
    {
        0x00, 0x00, 0x0a, 0x32, 0x2e, 0x35, 0x31, 0x00, 0x00, 0x00,
        0x00, 0x90, 0x00, 0x82
    },
    {
        0x00, 0x00, 0x12, 0x00, 0x1c, 0x6b, 0xf6, 0x14, 0x9f, 0x39,
        0xe9, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x90,
        0x00, 0x58
    },
    {
        0x00, 0x00, 0x12, 0x43, 0x54, 0x49, 0x5f, 0xd3, 0xc3, 0xbb,
        0xa7, 0xbf, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90,
        0x00, 0x98
    },
    {
        0x00, 0x00, 0x07, 0x1c, 0x29, 0x0b, 0x20, 0x00, 0x90, 0x00,
        0x89
    },
    {
        0x00, 0x00, 0x07, 0x20, 0xa9, 0x0b, 0x20, 0x00, 0x90, 0x00,
        0x35
    },
    {
        0x00, 0x00, 0x08, 0x00, 0x25, 0x00, 0x02, 0x01, 0x12, 0x90,
        0x00, 0xac
    },
    {
        0x00, 0x00, 0x20, 0x50, 0x09, 0x1b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x30, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x90, 0x00, 0xf2
    },
    {
        0x00, 0x00, 0x14, 0xa7, 0x10, 0x53, 0x54, 0x42, 0x20, 0x54,
        0x65, 0x61, 0x6d, 0x28, 0x43, 0x41, 0x29, 0x00, 0x00, 0x00,
        0x00, 0x90, 0x00, 0x68
    },
    {
        0x00, 0x00, 0x14, 0xa7, 0x10, 0x32, 0xb1, 0xb1, 0xbe, 0xa9,
        0xb6, 0xfe, 0xcc, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x90, 0x00, 0x3a
    },
    {
        0x00, 0x00, 0x14, 0xa7, 0x10, 0x33, 0xb1, 0xb1, 0xbe, 0xa9,
        0xc8, 0xfd, 0xcc, 0xa8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x90, 0x00, 0x46
    },
    {
        0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x86, 0xf1, 0x90, 0x00, 0xeb
    },
    { 0x00, 0x00, 0x02, 0x6a, 0x86, 0xee },
};

hi_u32 g_t1_suantong_send_length[19] = { 10, 9, 9, 9, 9, 9, 9, 9, 9, /* 19: The length of send_len array is 19 */
    9, 9, 9, 9, 18, 23, 23, 23, 9, 9 };
hi_u32 g_t1_suantong_receive_length[19] = { 6, 6, 10, 11, 17, 14, 8, 14, 22, /* 19: The length of array is 19 */
    22, 11, 11, 12, 36, 24, 24, 24, 17, 6 };

/* t14_aidide_access */
hi_u8 g_atr_aidide_access_t14[20] = { /* 20: array member max length is 20 */
    0x3b, 0x9f, 0x21,  0xe, 0x49, 0x52, 0x44, 0x45,
    0x54, 0x4f, 0x20, 0x41, 0x43, 0x53, 0x20, 0x56,
    0x32, 0x2e, 0x32, 0x98
};
hi_u8 g_t14_aidide_access_send[][7] = { /* 7: array member max length is 7 */
    { 0x1, 0x2, 0x0, 0x0, 0x0, 0x0, 0x3c },
    { 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x3d },
    { 0x1, 0x2, 0x2, 0x0, 0x0, 0x0, 0x3e },
    { 0x1, 0x2, 0x8, 0x0, 0x0, 0x0, 0x34 },
    { 0x1, 0x2, 0x3, 0x0, 0x0, 0x0, 0x3f },
};
hi_u8 g_t14_aidide_access_receive[][73] = { /* 73: array member max length is 73 */
    {
        0x1, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x14,
        0x34, 0x30, 0x30, 0x32, 0x33, 0x33, 0x30, 0x34,
        0x39, 0x34, 0x54, 0x33, 0x32, 0x34, 0x30, 0x31,
        0x41, 0x0, 0x0, 0x0, 0x6
    },
    {
        0x1, 0x2, 0x0, 0x0, 0x1, 0x0, 0x0, 0x10,
        0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x4, 0x7, 0x37, 0x94, 0xe6, 0x18,
        0x8c
    },
    {
        0x1, 0x2, 0x0, 0x0, 0x2, 0x0, 0x0, 0x10,
        0x4, 0x6, 0x12, 0x6, 0x23, 0x6, 0x24, 0x6,
        0x25, 0x6, 0x26, 0x0, 0x0, 0x43, 0x48, 0x4e,
        0x7f
    },
    {
        0x1, 0x2, 0x0, 0x0, 0x8, 0x0, 0x0, 0x40,
        0x0, 0x1, 0x8, 0x20, 0x0, 0x15, 0x60, 0x2,
        0x60, 0x0, 0x0, 0x3, 0x81, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0xc8
    },
    {
        0x1, 0x2, 0x0, 0x0, 0x3, 0x0, 0x0, 0x18,
        0x0, 0x3, 0xe9, 0x97, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x2, 0x11, 0x1, 0x1, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x49
    },
};
hi_u32 g_t14_aidide_access_send_length[5] = { 7, 7, 7, 7, 7 }; /* 5: array length is 5 */
hi_u32 g_t14_aidide_access_receive_length[5] = { 29, 25, 25, 73, 33 }; /* 5: array length is 5 */

/* g_t0_crypto_send */
hi_u8 g_t0_crypto_send[][5] = { /* 5: array member max length is 5 */
    { 0xa4, 0xc0, 0x00, 0x00, 0x11 },
    { 0xa4, 0xa4, 0x00, 0x00, 0x02 },
    { 0x3f, 0x00 },
    { 0xa4, 0xa4, 0x00, 0x00, 0x02 },
    { 0x2f, 0x01 },
    { 0xa4, 0xa2, 0x00, 0x00, 0x01 },
    { 0xd1 },
};
hi_u32 g_t0_crypto_send_length[7] = { 5, 5, 2, 5, 2, 5, 1 }; /* 7: array length is 7 */

hi_u8 g_t0_crypto_receive[][20] = { /* 20: array member max length is 20 */
    {
        0xc0, 0xdf, 0x0f, 0x05, 0x04, 0x00, 0x09, 0x3f, 0x00, 0x01,
        0x00, 0xf0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x90, 0x00
    },
    { 0xa4 },
    { 0x9f, 0x11 },
    { 0xa4 },
    { 0x9f, 0x11 },
    { 0xa2 },
    { 0x9f, 0x04 },
};
hi_u32 g_t0_crypto_receive_length[7] = { 20, 1, 2, 1, 2, 1, 2 }; /* 7: array length is 7 */

static hi_u32 g_max_send_length;
static hi_u32 g_send_times;
static hi_u32 *g_send_length;
static hi_char *g_send_buffer;

static hi_u32 g_max_receive_length;
static hi_u32 *g_single_receive_length;
static hi_u8 *g_single_receive_buffer;

static hi_bool g_drop_word;
static hi_u8 g_sci_port;

hi_bool g_card_status = HI_FALSE; /* g_card_status = HI_TRUE indicate card in; status = HI_FALSE indicate no card */
hi_u8 g_card_action = ACTION_NONE;

static hi_bool g_run_flag1;
static hi_bool g_run_flag2;
static pthread_mutex_t g_sci_mutex = PTHREAD_MUTEX_INITIALIZER;

hi_s32 select_card(hi_u32 card_protocol)
{
    if (card_protocol == 0) { /* 0: protocol select T0 */
        g_send_buffer = (hi_char *)g_t0_aidide_zeta_send;
        g_send_length = g_t0_aidide_zeta_send_length;
        g_max_send_length = sizeof(g_t0_aidide_zeta_send[0]);
        g_send_times = sizeof(g_t0_aidide_zeta_send) / sizeof(g_t0_aidide_zeta_send[0]);

        g_single_receive_buffer = (hi_u8 *)g_t0_aidide_zeta_receive;
        g_single_receive_length = g_t0_aidide_zeta_receive_length;
        g_max_receive_length = sizeof(g_t0_aidide_zeta_receive[0]);
        g_drop_word = HI_TRUE;
    } else if (card_protocol == 1) { /* 1: protocol select T1 */
        g_send_buffer = (hi_char *)g_t1_suantong_send;
        g_send_length = g_t1_suantong_send_length;
        g_max_send_length = sizeof(g_t1_suantong_send[0]);
        g_send_times = sizeof(g_t1_suantong_send) / sizeof(g_t1_suantong_send[0]);

        g_single_receive_buffer = (hi_u8 *)g_t1_suantong_receive;
        g_single_receive_length = g_t1_suantong_receive_length;
        g_max_receive_length = sizeof(g_t1_suantong_receive[0]);
        g_drop_word = HI_FALSE;
    } else if (card_protocol == 14) { /* 14: protocol select T14 */
        g_send_buffer = (hi_char *)g_t14_aidide_access_send;
        g_send_length = g_t14_aidide_access_send_length;
        g_max_send_length = sizeof(g_t14_aidide_access_send[0]);
        g_send_times = sizeof(g_t14_aidide_access_send) / sizeof(g_t14_aidide_access_send[0]);

        g_single_receive_buffer = (hi_u8 *)g_t14_aidide_access_receive;
        g_single_receive_length = g_t14_aidide_access_receive_length;
        g_max_receive_length = sizeof(g_t14_aidide_access_receive[0]);
        g_drop_word = HI_FALSE;
    } else {
        printf("not supported in this sample now\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

hi_s32 deal_with_procedure_byte(hi_unf_sci_port sci_port, hi_u8 *procedure_byte, hi_u8 *respond_data,
                                hi_u32 *respond_length)
{
    hi_u32 count = 0;
    hi_u32 receive_length = 0;
    hi_s32 ret = HI_FAILURE;

    ret = hi_unf_sci_receive(sci_port, procedure_byte, 1, &receive_length, 4000); /* 4000: receive timeout 40000ms */
    if ((ret != HI_SUCCESS) || (receive_length != 1)) {
        printf(" t0_communication ErrorCode:%08x, receive data length:%d\n", ret, receive_length);
        return HI_FAILURE;
    }

    while (*procedure_byte == 0x60) {
        usleep(10 * 1000); /* 10 * 1000: sleep 10ms */
        ret = hi_unf_sci_receive(sci_port, procedure_byte, 1, &receive_length, 4000); /* 4000: recv timeout 4000ms */
        count++;
        if (ret != HI_SUCCESS) {
            printf(" t0_communication ErrorCode:%08x\n", ret);
            return ret;
        }

        if (receive_length != 1) {
            printf("t0_communication cmd fail, receive length:%d ,expect: 1", receive_length);
            return HI_FAILURE;
        }

        if (*procedure_byte != 0x60) {
            printf(" receive procedure byte time = %d \n", count);
            break;
        }
    }

    /* the first byte of status bytes is 0x6x or 0x9x,will receive next status byte */
    if (((*procedure_byte & 0xF0) == 0x60) || ((*procedure_byte & 0xF0) == 0x90)) {
        *respond_data = *procedure_byte;
        ret = hi_unf_sci_receive(sci_port, respond_data + 1, 1, &receive_length, 4000); /* 4000: recv timeout 4000ms */
        if (ret != HI_SUCCESS) {
            printf("t0_communication ErrorCode:%08x\n", ret);
            return ret;
        }

        if (receive_length != 1) {
            printf("t0_communication cmd fail, receive length:%d ,expect: 1", receive_length);
            return HI_FAILURE;
        }

        *respond_length = 2; /* 2: respond data length is 2 */
        return HI_SUCCESS;
    }

    return HI_SUCCESS;
}

hi_s32 t0_communication(hi_unf_sci_port sci_port, hi_u8 *send_data, hi_u32 send_length, hi_u8 *respond_data,
                        hi_u32 *respond_length)
{
    hi_s32 ret = HI_FAILURE;
    hi_u32 length = 0;
    hi_u32 receive_length = 0;
    hi_u8 procedure_byte = 0;
    hi_u8 send_ins = 0;
    hi_u8 receive_buffer[258]  = {0};  /* 258: max length, 256bytes + SW1 SW2 */
    hi_u32 actual_length = 0;
    hi_u32 send_head = 5;
    hi_u32 receive_timeout = 4000;
    hi_u32 buffer = 256;

    /*  if cmd length <5 will not support ,but can fix it length */
    if (send_length < 5) { /* 5: send buffer length is 5 */
        printf(" t0_communication cmd length invalid:%d!\n", send_length);
        return HI_FAILURE;
    }

    /* CLA check */
    if (*send_data == 0xFF) {
        printf(" t0_communication  0xFF is PPS cmd not support on here!\n");
        return HI_FAILURE;
    }

    send_ins = *(send_data + 1);
    if (((send_ins & 0x1) == 1) ||
            ((send_ins & 0xF0) == 0x60) ||
            ((send_ins & 0xF0) == 0x90)) {
        printf(" t0_communication INS invalid!\n");
        return HI_FAILURE;
    }

    ret = hi_unf_sci_send(sci_port, send_data, send_head, &actual_length, 4000); /* 4000: send timeout is 4000ms */
    if (ret != HI_SUCCESS) {
        printf(" t0_communication ErrorCode:%08x\n", ret);
        return HI_FAILURE;
    }

    /* receive data command mode have no procedure byte */
    if (send_length == 5) { /* 5: send data length is 5 */
        ret = deal_with_procedure_byte(sci_port, receive_buffer, respond_data, respond_length);
        if (ret != HI_SUCCESS) {
            printf("%s %d :deal_with_procedure_byte respond command error ! \n ", __FUNCTION__, __LINE__);
            return ret;
        }
        /* if command length == 5 and received first data is't procedure byte,will receive Le data */
        if (send_ins != (receive_buffer[0] & 0xFE)) {
            printf(" t0_communication get procedure byte:%02x when send:%02x!\n", receive_buffer[0], send_ins);
        }

        if (*(send_data + 4) == 0) { /* 4: array send_data[] subscript */
            length = 256; /* 256: receive length will be 256 */
        } else {
            length = *(send_data + 4); /* 4: array send_data[] subscript */
        }

        ret = hi_unf_sci_receive(sci_port, receive_buffer, length, &receive_length, 4000); /* 4000: timeout 4000ms */
        if ((ret != HI_SUCCESS) || (length != receive_length)) {
            printf(" t0_communication ErrorCode:%08x, receive data length:%d (expect:%d)\n",
                   ret, receive_length, length);
            return HI_FAILURE;
        }

        ret = hi_unf_sci_receive(sci_port, receive_buffer + length,
                                 2, &receive_length, receive_timeout); /* 2: receive command head is 2 */
        if ((ret != HI_SUCCESS) || (receive_length != 2)) { /* 2:receive_length is 2 */
            printf(" t0_communication ErrorCode:%08x, receive data length:%d (expect:2)\n", ret, receive_length);
            return HI_FAILURE;
        }

        *respond_length = length + 2; /* 2: received data length */
        memcpy((hi_void *)respond_data, receive_buffer, *respond_length);
    } else {
        /* common length >5 , procedure disposal,and send remain data, send mode */
        ret = deal_with_procedure_byte(sci_port, &procedure_byte, respond_data, respond_length);
        if (ret != HI_SUCCESS) {
            printf("%s %d :deal_with_procedure_byte respond command error ! \n ", __FUNCTION__, __LINE__);
            return ret;
        }
        /* if procedure_byte xor 0xFE  == send_ins will send all remain data,
         * else if  procedure_byte xor 0xFE  == send_ins^0xFE
         * will send next one byte, will wait to receive new procedure byte after all
         */
        if ((procedure_byte & 0xFE) == send_ins) {
            /* if Lc = 0 will send remain 256 data, else send remain Lc data */
            if (*(send_data + 4) == 0) { /* 4: array send_data[] subscript */
                ret = hi_unf_sci_send(sci_port, send_data + 5, /* 5: send_data subscript */
                                      buffer, &actual_length, receive_timeout);
            } else {
                ret = hi_unf_sci_send(sci_port, send_data + 5, /* 5: array send_data[] subscript */
                                      *(send_data + 4), &actual_length, receive_timeout); /* 4: array subscript */
            }

            if (ret != HI_SUCCESS) {
                printf(" t0_communication ErrorCode:%08x\n", ret);
                return HI_FAILURE;
            }
        } else if ((procedure_byte & 0xFE) == (send_ins ^ 0xFE)) {
            ret = hi_unf_sci_send(sci_port, send_data + 5, 1, &actual_length, receive_timeout); /* 5:array subscript */
            if (ret != HI_SUCCESS) {
                printf(" t0_communication ErrorCode:%08x\n", ret);
                return HI_FAILURE;
            }
        } else {
            printf(" t0_communication invalid ACK:%02x\n", procedure_byte);
            return HI_FAILURE;
        }

        ret = deal_with_procedure_byte(sci_port, &procedure_byte, respond_data, respond_length);
        if (ret != HI_SUCCESS) {
            printf("%s %d :deal_with_procedure_byte respond command error ! \n ", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    return HI_SUCCESS;
}

hi_s32 card_out_process()
{
    return HI_SUCCESS;
}

hi_s32 card_in_process()
{
    hi_s32 ret;
    hi_unf_sci_status sci_status;
    hi_s32 reset_time;
    hi_u8 atr_count;
    hi_u8 atr_buffer[255]; /* 255: atr_buffer length 255 */
    hi_u32 read_length;
    hi_u32 i;
    hi_u32 j;
    hi_u8 result = 0;

#ifndef T0_TRANSFER
    hi_u32 total_read_length;
    hi_u32 send_length;
#endif

    struct timeval time_value_1;
    struct timeval time_value_2;
    struct timezone time_zone;
    hi_s32 usd;

#if TEST_NEGOTIATION
    hi_u8 pps_buffer[6] = { 0xff, 0x10, 0x12 }; /* 6: pps_buffer length 6 */
    hi_u8 receive_buffer[6]; /* 6: receive_buffer length is 6 */
    hi_u32 pps_length = 0;
    hi_u32 pps_data_length = 0;
#endif

    gettimeofday(&time_value_1, &time_zone);

    ret = hi_unf_sci_reset_card(g_sci_port, 0);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    } else {
        printf("Reset Card\n");
    }

    reset_time = 0;
    while (1) {
        /* will exit reset when reseting out of 10S */
        if (reset_time >= 200) { /* 200: 200 * 1000ms = 10s */
            printf("Reset Card Failure\n");
            return HI_FAILURE;
        }

        /* get SCI card status */
        ret = hi_unf_sci_get_card_status(g_sci_port, &sci_status);
        if (ret != HI_SUCCESS) {
            printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
            return ret;
        } else {
            if (sci_status >= HI_UNF_SCI_STATUS_READY) {
                gettimeofday(&time_value_2, &time_zone);
                usd = (time_value_2.tv_sec * MS + time_value_2.tv_usec / MS) -
                      (time_value_1.tv_sec * MS + time_value_1.tv_usec / MS);

                printf("Reset:%d ms \n", usd);

                /* reset success */
                printf("Reset Card Success\n");
                break;
            } else {
                printf("Reset Card Waiting...\n");
                usleep(50000); /* 50000: 50000us */
                reset_time += 1;
            }
        }
    }

    /* get and print ATR message */
    ret = hi_unf_sci_get_atr(g_sci_port, atr_buffer, 255, &atr_count); /* 255: atr_buffer length is 255 */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    printf("GetATR Count:%d\n", atr_count);

    for (i = 0; i < atr_count; i++) {
        printf("atr_buffer[%d]:%#x\n", i, atr_buffer[i]);
    }

    /* if the card support PPS Negotiation, you can test NegotiatePPS function */
#if TEST_NEGOTIATION
    ret = hi_unf_sci_negotiate_pps(g_sci_port, (hi_u8 *)&pps_buffer, pps_length, 1000); /* 1000: wait timeout 1000ms */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    ret = hi_unf_sci_get_pps_respond_data(g_sci_port, (hi_u8 *)receive_buffer, &pps_data_length);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    for (i = 0; i < pps_data_length; i++) {
        printf("PPSData[%d]:%#x\n", i, receive_buffer[i]);
    }
#endif

    /*
     * if the card don't support PPS Negotiation or PPS Negotiation fail.
     * you can try to set_etu_factor to change the baudrate, or set_guard_time
     * to change extra char guardtime, but this function only for tianbai card currently.
     */
#if TEST_SET_ETU
    ret = hi_unf_sci_set_etu_factor(g_sci_port, 372, 2); /* 372: F factor  2:D factor */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    ret = hi_unf_sci_set_guard_time(g_sci_port, 2); /* 2: extra guard time */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }
#endif

    /* test other unf function */
#if TEST_OTHER
    ret = hi_unf_sci_set_char_timeout(g_sci_port, HI_UNF_SCI_PROTOCOL_T0, 10000); /* 10000: set char tiomout 10000 */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    ret = hi_unf_sci_set_block_timeout(g_sci_port, 300000); /* 300000: set block timeout 300000 */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    ret = hi_unf_sci_set_tx_retries(g_sci_port, 5); /* 5: Tx retry times */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }
#endif

    hi_unf_sci_params sci_param;

    ret = hi_unf_sci_get_params(g_sci_port, &sci_param);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    printf("sci_param.Fi = %d\n", sci_param.fi);
    printf("sci_param.Di = %d\n", sci_param.di);
    printf("sci_param.protocol_type = %d\n", sci_param.protocol_type);
    printf("sci_param.guard_delay = %d\n", sci_param.guard_delay);
    printf("sci_param.tx_retries = %d\n", sci_param.tx_retries);
    printf("sci_param.char_timeouts = %d\n", sci_param.char_timeouts);
    printf("sci_param.sci_port = %d\n", sci_param.sci_port);

    printf("sci_param.block_timeouts = %d\n", sci_param.block_timeouts);
    printf("sci_param.actual_clk_rate = %d\n", sci_param.actual_clk_rate);
    printf("sci_param.actual_bit_rate = %d\n", sci_param.actual_bit_rate);

    /* start send and receive data */
    printf("now begin send and receive\n");
    printf("we should send %d times\n", g_send_times);
    for (j = 0; j < g_send_times; j++) {
        printf("this is round %d\n", j);
        printf("send sequence:");
        for (i = 0; i < g_send_length[j]; i++) {
            printf("%#x ", g_send_buffer[j * g_max_send_length + i]);
        }

        printf("\n");

#ifdef T0_TRANSFER
        ret = t0_communication(g_sci_port, (hi_u8 *)g_send_buffer + j * g_max_send_length, g_send_length[j],
                               (hi_u8 *)&g_receive_buffer, &read_length);
        if (ret != HI_SUCCESS) {
            printf("%s->%d hi_unf_sci_receive return %#x read_length:%d\n", __func__, __LINE__, ret, read_length);
            return ret;
        }

#else
        hi_u32 send_timeout = 5000;
        hi_u32 read_timeout = 10000;

        ret = hi_unf_sci_send(g_sci_port, (hi_u8 *)g_send_buffer + j * g_max_send_length,
                              g_send_length[j], &send_length, send_timeout);
        if ((ret != HI_SUCCESS) || (g_send_length[j] != send_length)) {
            printf("%s->%d, hi_unf_sci_send return %d SendLen:%d\n", __func__, __LINE__, ret, send_length);
            return ret;
        }

        total_read_length = 0;
        while (total_read_length < g_single_receive_length[j]) {
            ret = hi_unf_sci_receive(g_sci_port, (hi_u8 *)&g_receive_buffer[total_read_length],
                                     1, &read_length, read_timeout);
            if (ret != HI_SUCCESS) {
                printf("%s->%d hi_unf_sci_receive return %#x ReadLen:%d\n", __func__, __LINE__, ret, read_length);
                return ret;
            }

            if ((g_receive_buffer[total_read_length] == 0x60) && (g_drop_word == HI_TRUE)) {
                printf("drop 0x60\n");
                continue;
            }

            total_read_length++;

        }
#endif

        printf("expect receive sequence:");
        for (i = 0; i < g_single_receive_length[j]; i++) {
            printf("%#x ", g_single_receive_buffer[j * g_max_receive_length + i]);
        }

        printf("\nactual receive sequence:");
        for (i = 0; i < g_single_receive_length[j]; i++) {
            if ((g_receive_buffer[i] == 0x60) && (g_drop_word == HI_TRUE)) {
                printf("drop 0x60\n");
                continue;
            }

            printf("%#x ", g_receive_buffer[i]);
            if ((result == 0) && (g_receive_buffer[i] != g_single_receive_buffer[j * g_max_receive_length + i])) {
                result = 1;
            }
        }

        printf("\n");

        if (result == 1) {
            printf("ERROR\n");
        } else {
            printf("OK\n");
        }

        result = 0;
    }

    return HI_SUCCESS;
}

hi_s32 check_ca(hi_u8 *paction)
{
    hi_s32 ret;
    hi_unf_sci_status status;
    hi_bool status_flag;

    ret = hi_unf_sci_get_card_status(g_sci_port, &status);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    if (status <= HI_UNF_SCI_STATUS_NOCARD) {
        status_flag = HI_FALSE;  /* no card */
    } else {
        status_flag = HI_TRUE;   /* have card */
    }

    /* if status_flag is ture indicate the card have been pull out or push in */
    if (g_card_status != status_flag) {
        g_card_status = status_flag;
        if (status_flag == HI_TRUE) {
            *paction = ACTION_CARDIN;     /* card in */
        } else {
            *paction = ACTION_CARDOUT;    /* card out */
        }
    } else {
        *paction = ACTION_NONE;           /* no operation */
    }

    return HI_SUCCESS;
}

hi_void *check_sci(hi_void *args)
{
    hi_s32 ret;

    while (g_run_flag1) {
        pthread_mutex_lock(&g_sci_mutex);
        ret = check_ca(&g_card_action);
        if (ret != HI_SUCCESS) {
            printf("check_ca failed\n");
        }

        pthread_mutex_unlock(&g_sci_mutex);
        usleep(50 * 1000); /* sleep time 50us * 1000 = 50ms */
    }

    return (hi_void *)HI_SUCCESS;
}

hi_void *run_sci(hi_void *args)
{
    hi_u8 card_action;

    while (g_run_flag2) {
        pthread_mutex_lock(&g_sci_mutex);
        card_action = g_card_action;
        pthread_mutex_unlock(&g_sci_mutex);
        if (card_action == ACTION_CARDIN) {
            printf("CARD IN\n");
            card_in_process();
        } else if (card_action == ACTION_CARDOUT) {
            printf("CARD OUT\n");
            card_out_process();
        }

        usleep(50 * 1000); /* sleep time 50us * 1000 = 50ms */
    }

    return (hi_void *)HI_SUCCESS;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;
    hi_u32 frequency;
    hi_unf_sci_mode mode;
    hi_u32 protocol;
    hi_unf_sci_protocol protocol_type;
    hi_unf_sci_level vccen_level;
    hi_unf_sci_level detect_level;
    hi_u32 value = 0;
    pthread_t task1;
    pthread_t task2;
    hi_char a[10] = {0}; /* 10: array size is 10 */

    printf("please select sci prot: 0-SCI0 1-SCI1\n");
    scanf("%s", a);
    g_sci_port = (hi_u32)strtoul(a, 0, 0);
    if ((g_sci_port != 0) && (g_sci_port != 1)) {
        printf("input parament error!\n");
        return HI_FAILURE;
    }

    printf("please select protocol type: 0-T0 1-T1 14-T14 others-T0\n");
    scanf("%s", a);
    protocol = (hi_u32)strtoul(a, 0, 0);
    if ((protocol != 0) && (protocol != 1) && (protocol != 14)) { /* 14: protocol type T14 */
        protocol = 0;
    }

    printf("protocol = %d\n", protocol);
    ret = select_card(protocol);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    if (protocol == 14) { /* 14: protocol type T14 */
        protocol_type = HI_UNF_SCI_PROTOCOL_T14;
        frequency = 5000; /* 5000: frequency */
    } else {
        protocol_type = protocol;
        frequency = 3570; /* 3570: frequency */
    }

    hi_unf_sci_init();

    /* open SCI device */
    ret = hi_unf_sci_open(g_sci_port, protocol_type, frequency);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        hi_unf_sci_deinit();
        return ret;
    }

    printf("please input clock mode(default %d): %d-CMOS, %d-OD, others-default\n",
           HI_UNF_SCI_MODE_CMOS, HI_UNF_SCI_MODE_CMOS, HI_UNF_SCI_MODE_OD);

    scanf("%s", a);
    value = (hi_u32)strtoul(a, 0, 0);
    if ((value != HI_UNF_SCI_MODE_CMOS) && (value != HI_UNF_SCI_MODE_OD)) {
        mode = HI_UNF_SCI_MODE_CMOS;
    } else {
        mode = value;
    }

    ret = hi_unf_sci_config_clock_mode(g_sci_port, mode);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto err1;
    }

    printf("please input vccen mode (default %d): %d-CMOS, %d-OD, others-default\n",
           HI_UNF_SCI_MODE_CMOS, HI_UNF_SCI_MODE_CMOS, HI_UNF_SCI_MODE_OD);

    scanf("%s", a);
    value = (hi_u32)strtoul(a, 0, 0);

    if ((value != HI_UNF_SCI_MODE_CMOS) && (value != HI_UNF_SCI_MODE_OD)) {
        mode = HI_UNF_SCI_MODE_CMOS;
    } else {
        mode = value;
    }

    /* set sci card detect level. it reference the hardware design */
    ret = hi_unf_sci_config_vccen_mode(g_sci_port, mode);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto err1;
    }

    printf("please input reset mode (default %d): %d-CMOS, %d-OD, others-default\n",
           HI_UNF_SCI_MODE_CMOS, HI_UNF_SCI_MODE_CMOS, HI_UNF_SCI_MODE_OD);

    scanf("%s", a);
    value = (hi_u32)strtoul(a, 0, 0);

    if ((value != HI_UNF_SCI_MODE_CMOS) && (value != HI_UNF_SCI_MODE_OD)) {
        mode = HI_UNF_SCI_MODE_CMOS;
    } else {
        mode = value;
    }

    /* set sci card detect level. it reference the hardware design */
    ret = hi_unf_sci_config_reset_mode(g_sci_port, mode);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto err1;
    }

    printf("please input vccen level(default %d): %d-Low level, %d-High level, others-default\n",
           HI_UNF_SCI_LEVEL_LOW, HI_UNF_SCI_LEVEL_LOW, HI_UNF_SCI_LEVEL_HIGH);

    scanf("%s", a);
    value = (hi_u32)strtoul(a, 0, 0);

    if ((value != HI_UNF_SCI_LEVEL_LOW) && (value != HI_UNF_SCI_LEVEL_HIGH)) {
        vccen_level = HI_UNF_SCI_LEVEL_LOW;
    } else {
        vccen_level = value;
    }

    /* set sci card power enable level. it reference the hardware design */
    ret = hi_unf_sci_config_vccen(g_sci_port, vccen_level);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto err1;
    }

    printf("please input detect level(default %d): %d-Low level, %d-High level, others-default\n",
           HI_UNF_SCI_LEVEL_LOW, HI_UNF_SCI_LEVEL_LOW, HI_UNF_SCI_LEVEL_HIGH);

    scanf("%s", a);
    value = (hi_u32)strtoul(a, 0, 0);

    if ((value != HI_UNF_SCI_LEVEL_LOW) && (value != HI_UNF_SCI_LEVEL_HIGH)) {
        detect_level = HI_UNF_SCI_LEVEL_LOW;
    } else {
        detect_level = value;
    }

    /* set sci card detect level. it reference the hardware design */
    ret = hi_unf_sci_config_detect(g_sci_port, detect_level);
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        goto err1;
    }

    printf("protocol:%d, vcc-en:%d, detect-level:%d\n",
           protocol, vccen_level, detect_level);

    pthread_mutex_init(&g_sci_mutex, NULL);

    /* create thread to checkca */
    g_run_flag1 = 1;
    ret = pthread_create(&task1, NULL, check_sci, NULL);
    if (ret != HI_SUCCESS) {
        goto err2;
    }

    /* create thread to send and receive data */
    g_run_flag2 = 1;
    ret = pthread_create(&task2, NULL, run_sci, NULL);
    if (ret != HI_SUCCESS) {
        goto err3;
    }

    printf("Press any key to finish sci demo\n");
    getchar();
    getchar();

    g_run_flag1 = 0;
    g_run_flag2 = 0;

    pthread_join(task1, NULL);
    pthread_join(task2, NULL);

    pthread_mutex_destroy(&g_sci_mutex);
    hi_unf_sci_deactive_card(g_sci_port);
    hi_unf_sci_close(g_sci_port);

    printf("\nrun sci demo finished\n");
    return HI_SUCCESS;

err3:
    g_run_flag1 = 0;
    pthread_join(task1, NULL);
err2:
    pthread_mutex_destroy(&g_sci_mutex);
err1:
    hi_unf_sci_deactive_card(g_sci_port);
    hi_unf_sci_close(g_sci_port);
    hi_unf_sci_deinit();
    return HI_FAILURE;
}