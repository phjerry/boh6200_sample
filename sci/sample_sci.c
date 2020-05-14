/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: The sample for sci
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "hi_unf_sci.h"

#define ACTION_CARDIN 0       /* card inset flag */
#define ACTION_CARDOUT 1      /* card pull out flag */
#define ACTION_NONE 2         /* card immobile */

hi_u8 g_receive_buffer[512]; /* 512: define receive buffer length 512 */

/* t0_aidide_zeta */
hi_u8 g_t0_aidide_zeta_send[][17] = { /* 17: array member max length */
    { 0xd2, 0x42, 0x00, 0x00, 0x01 },
    { 0x1d },
    { 0xd2, 0xfe, 0x00, 0x00, 0x19 },
    { 0xd2, 0x40, 0x00, 0x00, 0x11 },
    {
        0x03, 0x06, 0x14, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x1d
    },
    { 0xd2, 0xfe, 0x00, 0x00, 0x09 },
    { 0xd2, 0x3c, 0x00, 0x00, 0x01 },
    { 0x22 },
    { 0xd2, 0xfe, 0x00, 0x00, 0x15 },
    { 0xd2, 0x3e, 0x00, 0x00, 0x01 },
    { 0x23 },
    { 0xd2, 0xfe, 0x00, 0x00, 0x0b },
    { 0xd2, 0x00, 0x00, 0x00, 0x01 },
    { 0x3c },
    { 0xd2, 0xfe, 0x00, 0x00, 0x1d },
};
hi_u8 g_t0_aidide_zeta_receive[][32] = { /* 32: array member max length */
    { 0x42 },
    { 0x90, 0x19 },
    {
        0xfe, 0x01, 0x02, 0x00, 0x00,
        0x21, 0x00, 0x00, 0x10, 0x03,
        0x06, 0x14, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x1c, 0x90, 0x00
    },
    { 0x40 },
    { 0x90, 0x09 },
    {
        0xfe, 0x01, 0x02, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0x1c,
        0x90, 0x00
    },
    { 0x3c },
    { 0x90, 0x15 },
    {
        0xfe, 0x01, 0x02, 0x00, 0x00,
        0x1e, 0x00, 0x00, 0x0c, 0x53,
        0x36, 0x01, 0x0e, 0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x4c, 0x90, 0x00
    },
    { 0x3e },
    { 0x90, 0x0b },
    {
        0xfe, 0x01, 0x02, 0x00, 0x00,
        0x1f, 0x00, 0x00, 0x02, 0x00,
        0x02, 0x23, 0x90, 0x00
    },
    { 0x00 },
    { 0x90, 0x1d },
    {
        0xfe, 0x01, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x14, 0x34,
        0x30, 0x32, 0x30, 0x33, 0x32,
        0x35, 0x31, 0x35, 0x32, 0x54,
        0x36, 0x33, 0x38, 0x35, 0x30,
        0x30, 0x30, 0x41, 0x00, 0x01,
        0x90, 0x00
    },
};

hi_u32 g_t0_aidide_zeta_send_length[15] = { 5, 1, 5, 5, 17, 5, 5, 1, 5, 5, 1, 5, 5, 1, 5 }; /* 15: 15 length */
hi_u32 g_t0_aidide_zeta_receive_length[15] = { 1, 2, 28, 1, 2, 12, 1, 2, 24, 1, 2, 14, 1, 2, 32 }; /* 15: 15 length */

/* T1 suantong */
hi_u8 g_atr_suantong_t1[18] = { /* 18: array g_atr_suantong_t1 length */
    0x3b, 0xe9, 0x00, 0x00, 0x81,
    0x31, 0xc3, 0x45, 0x99, 0x63,
    0x74, 0x69, 0x19, 0x99, 0x12,
    0x56, 0x10, 0xec
};
hi_u8 g_t1_suantong_send[][23] = { /* 23: array g_t1_suantong_send member max length */
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
hi_u8 g_t1_suantong_receive[][36] = { /* 36: array g_t1_suantong_receive member max length */
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
hi_u32 g_t1_suantong_send_length[19] = { 10, 9, 9, 9, 9, 9, 9, 9, 9, /* 19: The length of array is 19 */
    9, 9, 9, 9, 18, 23, 23, 23, 9, 9 };
hi_u32 g_t1_suantong_receive_length[19] = { 6, 6, 10, 11, 17, 14, 8, 14, 22, /* 19: The length of array is 19 */
    22, 11, 11, 12, 36, 24, 24, 24, 17, 6 };

/* t14_aidide_access */
hi_u8 g_atr_aidide_access_t14[20] = { /* 20: g_atr_aidide_access_t14 length */
    0x3b, 0x9f, 0x21,  0xe, 0x49, 0x52, 0x44, 0x45,
    0x54, 0x4f, 0x20, 0x41, 0x43, 0x53, 0x20, 0x56,
    0x32, 0x2e, 0x32, 0x98
};

hi_u8 g_t14_aidide_access_send[][7] = { /* 7: array member max length */
    { 0x1, 0x2, 0x0, 0x0, 0x0, 0x0, 0x3c },
    { 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x3d },
    { 0x1, 0x2, 0x2, 0x0, 0x0, 0x0, 0x3e },
    { 0x1, 0x2, 0x8, 0x0, 0x0, 0x0, 0x34 },
    { 0x1, 0x2, 0x3, 0x0, 0x0, 0x0, 0x3f },
};
hi_u8 g_t14_aidide_access_receive[][73] = { /* 73: array member max length */
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
hi_u32 g_t14_aidide_access_send_length[5] = { 7, 7, 7, 7, 7 }; /* 5: g_t14_aidide_access_send_length */
hi_u32 g_t14_aidide_access_receive_length[5] = { 29, 25, 25, 73, 33 }; /* 5: g_t14_aidide_access_receive_length */


/* g_t0_crypto_send */
hi_u8 g_t0_crypto_send[][5] = { /* 5: g_t0_crypto_send member max length */
    { 0xa4, 0xc0, 0x00, 0x00, 0x11 },
    { 0xa4, 0xa4, 0x00, 0x00, 0x02 },
    { 0x3f, 0x00 },
    { 0xa4, 0xa4, 0x00, 0x00, 0x02 },
    { 0x2f, 0x01 },
    { 0xa4, 0xa2, 0x00, 0x00, 0x01 },
    { 0xd1},
};

hi_s32 g_t0_crypto_send_length[7] = { 5, 5, 2, 5, 2, 5, 1 }; /* 7: array g_t0_crypto_send_length length */

hi_u8 g_t0_crypto_receive[][20] = { /* 20: array g_t0_crypto_receive member max length */
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

hi_s32 g_t0_crypto_receive_length[7] = { 20, 1, 2, 1, 2, 1, 2 }; /* 7: g_t0_crypto_receive_length length */

static hi_u32 g_max_send_length;
static hi_u32 g_send_times;
static hi_u32 *g_send_length;
static hi_u8 *g_send_buffer;

static hi_u32 g_max_receive_length;
static hi_u32 *g_single_receive_length;
static hi_u8 *g_single_receive_buffer;

static hi_bool g_drop_word;
static hi_u8 g_sci_port;

static hi_bool g_run_flag1;
static hi_bool g_run_flag2;
static pthread_mutex_t g_sci_mutex = PTHREAD_MUTEX_INITIALIZER;

/* g_card_status = HI_TRUE indicate card in ;g_card_status = HI_FALSE  indicate no card */
hi_bool g_card_status = HI_FALSE;
hi_u8 g_card_action = ACTION_NONE;

hi_s32 select_card(hi_u32 card_protocol)
{
    if (card_protocol == 0) { /* 0: protocol select T0 */
        g_send_buffer = (hi_u8 *)g_t0_aidide_zeta_send;
        g_send_length = g_t0_aidide_zeta_send_length;
        g_max_send_length = sizeof(g_t0_aidide_zeta_send[0]);
        g_send_times = sizeof(g_t0_aidide_zeta_send) / sizeof(g_t0_aidide_zeta_send[0]);

        g_single_receive_buffer = (hi_u8 *)g_t0_aidide_zeta_receive;
        g_single_receive_length = g_t0_aidide_zeta_receive_length;
        g_max_receive_length = sizeof(g_t0_aidide_zeta_receive[0]);
        g_drop_word = HI_TRUE;
    } else if (card_protocol == 1) { /* 1: protocol select T1 */
        g_send_buffer = (hi_u8 *)g_t1_suantong_send;
        g_send_length = g_t1_suantong_send_length;
        g_max_send_length = sizeof(g_t1_suantong_send[0]);
        g_send_times = sizeof(g_t1_suantong_send) / sizeof(g_t1_suantong_send[0]);

        g_single_receive_buffer = (hi_u8 *)g_t1_suantong_receive;
        g_single_receive_length = g_t1_suantong_receive_length;
        g_max_receive_length = sizeof(g_t1_suantong_receive[0]);
        g_drop_word = HI_FALSE;
    } else if (card_protocol == 14) { /* 14: protocol select T14 */
        g_send_buffer = (hi_u8 *)g_t14_aidide_access_send;
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
    hi_u8 atr_buffer[255]; /* 255: atr_buffer length */
    hi_u32 total_read_length;
    hi_u32 read_length;
    hi_u32 send_length;
    hi_u32 i;
    hi_u32 j;
    hi_u8 result = 0;
    hi_u32 send_timeout = 5000;
    hi_u32 read_timeout = 10000;

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
        if (reset_time >= 200) { /* 200:reset times */
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
                /* reset Success */
                printf("Reset Card Success\n");
                break;
            } else {
                printf("Reset Card Waiting...\n");
                usleep(50000); /* 50000: usleep times */
                reset_time += 1;
            }
        }
    }

    /* get and print ATR message */
    ret = hi_unf_sci_get_atr(g_sci_port, atr_buffer, 255, &atr_count); /* 255:atr_buffer length */
    if (ret != HI_SUCCESS) {
        printf("%s: %d ErrorCode=0x%x\n", __FILE__, __LINE__, ret);
        return ret;
    }

    printf("GetATR Count:%d\n", atr_count);

    for (i = 0; i < atr_count; i++) {
        printf("atr_buffer[%d]:%#x\n", i, atr_buffer[i]);
    }

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

    /* if status_flag is ture indicated the card have been pull out or push in */
    if (g_card_status != status_flag) {
        g_card_status = status_flag;
        if (status_flag == HI_TRUE) {
            *paction = ACTION_CARDIN; /* card in  */
        } else {
            *paction = ACTION_CARDOUT; /* card out */
        }
    } else {
        *paction = ACTION_NONE; /* no operation */
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
        usleep(50 * 1000); /* sleep time 50us * 1000 = 50 ms */
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
        } else {
            /* do nothing */
        }

        usleep(30 * 1000); /* sleep time 30us * 1000 = 30 ms */
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
        frequency = 6000; /* 6000: frequency */
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

    /* set sci card detect level.it reference the hardware design */
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

    /* set sci card detect level.it reference the hardware design */
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

    /* set sci card power enable level.it reference the hardware design */
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

    /* set sci card detect level.it reference the hardware design */
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

    /* Create thread to send and receive data */
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