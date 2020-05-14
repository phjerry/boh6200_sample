/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_rng
 * Author: zhaoguihong
 * Create: 2019-06-18
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_cipher.h"
#include "hi_errno.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define HI_ERR_RNG(format, arg...)
#define HI_DEBUG_RNG(format...)    printf(format)
#else
#define HI_ERR_RNG(format, arg...) printf("%s,%d: " format, __FUNCTION__, __LINE__, ##arg)
#define HI_DEBUG_RNG(format...)    printf(format)
#endif

#define RAND_BYTE_CNT 127

static hi_void print_buffer(char *string, hi_u8 *input, hi_u32 length)
{
    hi_u32 i = 0;

    if (string != NULL) {
        printf("%s\n", string);
    }

    for (i = 0; i < length; i++) {
        if ((i % 16 == 0) && (i != 0)) {
            printf("\n");
        }
        printf("0x%02x ", input[i]);
    }
    printf("\n");

    return;
}

int main(void)
{
    hi_s32 ret;
    hi_s32 index = 0;
    hi_u8 rand_byte[RAND_BYTE_CNT] = { 0 };
    hi_u32 random_number = 0;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        HI_ERR_RNG("HI_UNF_CIPHER_init ERROR!\n");
        return ret;
    }

    for (index = 0; index < 10; index++) {
        ret = hi_unf_cipher_get_random_number(&random_number);
        if (ret == HI_ERR_CIPHER_NO_AVAILABLE_RNG) {
            /* "there is no ramdom number available now. try again! */
            index--;
            continue;
        }

        if (ret != HI_SUCCESS) {
            HI_ERR_RNG("HI_UNF_CIPHER_get_random_number failed!\n");
            return ret;
        }

        HI_DEBUG_RNG("random number: %08x\n", random_number);
    }

    ret = hi_unf_cipher_get_multi_random_bytes(RAND_BYTE_CNT, rand_byte);
    if (ret != HI_SUCCESS) {
        HI_ERR_RNG("HI_UNF_CIPHER_get_random_byte failed!\n");
        return ret;
    }
    print_buffer("rand_byte", rand_byte, RAND_BYTE_CNT);

    ret = hi_unf_cipher_deinit();
    if (ret != HI_SUCCESS) {
        HI_ERR_RNG("CIPHER de_init ERROR! return value = %d.\n", ret);
        return ret;
    }

    return ret;
}

