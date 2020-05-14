/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_hash
 * Author: zhaoguihong
 * Create: 2019-06-18
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <assert.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_cipher.h"
#include "hi_adp.h"

#define HI_ERR_CIPHER(format, arg...)  HI_PRINT("\033[0;1;31m" format "\033[0m", ##arg)
#define HI_INFO_CIPHER(format, arg...) HI_PRINT("\033[0;1;32m" format "\033[0m", ##arg)
#define TEST_END_PASS()                \
    HI_INFO_CIPHER("****************** %s test PASS !!! ******************\n", __FUNCTION__)
#define TEST_END_FAIL()                \
    HI_ERR_CIPHER("****************** %s test FAIL !!! ******************\n", __FUNCTION__)
#define TEST_RESULT_PRINT() do { \
        if (ret) {            \
            TEST_END_FAIL();  \
        }                     \
        else {                \
            TEST_END_PASS();  \
        } \
    } while (0)

#define MAX_HASH_HANDLE 4
#define MAX_HMAC_HANDLE 3
#define LONG_DATA_SIZE 10000

static hi_void print_buffer(const char *string, const hi_u8 *input, hi_u32 length)
{
    hi_u32 i = 0;

    if (NULL != string) {
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

static unsigned char sha1_buf[3][128] = {
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" }
};
static const int sha1_buflen[4] = { 3, 56, 112, 1000000 };

static const unsigned char sha2_sum[4][32] = {
    /** SHA-256 sample vectors */
    { 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA, 0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
      0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C, 0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD },
    { 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8, 0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
      0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67, 0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 },
    { 0x59, 0xf1, 0x09, 0xd9, 0x53, 0x3b, 0x2b, 0x70, 0xe7, 0xc3, 0xb8, 0x14, 0xa2, 0xbd, 0x21, 0x8f,
      0x78, 0xea, 0x5d, 0x37, 0x14, 0x45, 0x5b, 0xc6, 0x79, 0x87, 0xcf, 0x0d, 0x66, 0x43, 0x99, 0xcf },
    { 0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92, 0x81, 0xa1, 0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67,
      0xf1, 0x80, 0x9a, 0x48, 0xa4, 0x97, 0x20, 0x0e, 0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, 0x2c, 0xd0 },
};

static const unsigned char sha224_sum[4][32] = {
    { 0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22, 0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
      0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7, 0xe3, 0x6c, 0x9d, 0xa7 },
    { 0x75, 0x38, 0x8b, 0x16, 0x51, 0x27, 0x76, 0xcc, 0x5d, 0xba, 0x5d, 0xa1, 0xfd, 0x89, 0x01, 0x50,
      0xb0, 0xc6, 0x45, 0x5c, 0xb4, 0xf5, 0x8b, 0x19, 0x52, 0x52, 0x25, 0x25 },
    { 0x7d, 0xe2, 0xf9, 0x3b, 0x0d, 0x0a, 0x1f, 0x5c, 0xaf, 0x83, 0x77, 0x39, 0xda, 0x74, 0x16, 0x7a,
      0x03, 0xbd, 0x64, 0xb7, 0x93, 0x06, 0x7e, 0xbd, 0x40, 0x73, 0xd0, 0xdc },
    { 0x20, 0x79, 0x46, 0x55, 0x98, 0x0c, 0x91, 0xd8, 0xbb, 0xb4, 0xc1, 0xea, 0x97, 0x61, 0x8a, 0x4b,
      0xf0, 0x3f, 0x42, 0x58, 0x19, 0x48, 0xb2, 0xee, 0x4e, 0xe7, 0xad, 0x67 },
};

static const unsigned char sha384_sum[4][48] ={
    {0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
     0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63, 0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
     0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7},
    {0x33, 0x91, 0xfd, 0xdd, 0xfc, 0x8d, 0xc7, 0x39, 0x37, 0x07, 0xa6, 0x5b, 0x1b, 0x47, 0x09, 0x39,
     0x7c, 0xf8, 0xb1, 0xd1, 0x62, 0xaf, 0x05, 0xab, 0xfe, 0x8f, 0x45, 0x0d, 0xe5, 0xf3, 0x6b, 0xc6,
     0xb0, 0x45, 0x5a, 0x85, 0x20, 0xbc, 0x4e, 0x6f, 0x5f, 0xe9, 0x5b, 0x1f, 0xe3, 0xc8, 0x45, 0x2b},
    {0x76, 0xfe, 0x0d, 0x4f, 0x6a, 0xb0, 0x17, 0xcf, 0xcd, 0x2c, 0x3e, 0x24, 0xd5, 0x98, 0x2f, 0x8c,
     0x74, 0x08, 0x45, 0xbe, 0x17, 0xac, 0x20, 0xe4, 0x14, 0xef, 0x7c, 0x16, 0x2f, 0xed, 0xf4, 0x4f,
     0xb3, 0x3b, 0x4c, 0x97, 0x68, 0x75, 0xba, 0x77, 0xf9, 0xe4, 0x20, 0xed, 0x25, 0xa2, 0x12, 0xf0},
    {0x9d, 0x0e, 0x18, 0x09, 0x71, 0x64, 0x74, 0xcb, 0x08, 0x6e, 0x83, 0x4e, 0x31, 0x0a, 0x4a, 0x1c,
     0xed, 0x14, 0x9e, 0x9c, 0x00, 0xf2, 0x48, 0x52, 0x79, 0x72, 0xce, 0xc5, 0x70, 0x4c, 0x2a, 0x5b,
     0x07, 0xb8, 0xb3, 0xdc, 0x38, 0xec, 0xc4, 0xeb, 0xae, 0x97, 0xdd, 0xd8, 0x7f, 0x3d, 0x89, 0x85},
};

static const unsigned char sha512_sum[4][64] ={
    {0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
     0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
     0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
     0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f},
    {0x20, 0x4a, 0x8f, 0xc6, 0xdd, 0xa8, 0x2f, 0x0a, 0x0c, 0xed, 0x7b, 0xeb, 0x8e, 0x08, 0xa4, 0x16,
     0x57, 0xc1, 0x6e, 0xf4, 0x68, 0xb2, 0x28, 0xa8, 0x27, 0x9b, 0xe3, 0x31, 0xa7, 0x03, 0xc3, 0x35,
     0x96, 0xfd, 0x15, 0xc1, 0x3b, 0x1b, 0x07, 0xf9, 0xaa, 0x1d, 0x3b, 0xea, 0x57, 0x78, 0x9c, 0xa0,
     0x31, 0xad, 0x85, 0xc7, 0xa7, 0x1d, 0xd7, 0x03, 0x54, 0xec, 0x63, 0x12, 0x38, 0xca, 0x34, 0x45},
    {0x73, 0x61, 0xec, 0x4a, 0x61, 0x7b, 0x64, 0x73, 0xfb, 0x75, 0x1c, 0x44, 0xd1, 0x02, 0x6d, 0xb9,
     0x44, 0x29, 0x15, 0xa5, 0xfc, 0xea, 0x1a, 0x41, 0x9e, 0x61, 0x5d, 0x2f, 0x3b, 0xc5, 0x06, 0x94,
     0x94, 0xda, 0x28, 0xb8, 0xcf, 0x2e, 0x44, 0x12, 0xa1, 0xdc, 0x97, 0xd6, 0x84, 0x8f, 0x9c, 0x84,
     0xa2, 0x54, 0xfb, 0x88, 0x4a, 0xd0, 0x72, 0x0a, 0x83, 0xea, 0xa0, 0x43, 0x4a, 0xea, 0xfd, 0x8c},
    {0xe7, 0x18, 0x48, 0x3d, 0x0c, 0xe7, 0x69, 0x64, 0x4e, 0x2e, 0x42, 0xc7, 0xbc, 0x15, 0xb4, 0x63,
     0x8e, 0x1f, 0x98, 0xb1, 0x3b, 0x20, 0x44, 0x28, 0x56, 0x32, 0xa8, 0x03, 0xaf, 0xa9, 0x73, 0xeb,
     0xde, 0x0f, 0xf2, 0x44, 0x87, 0x7e, 0xa6, 0x0a, 0x4c, 0xb0, 0x43, 0x2c, 0xe5, 0x77, 0xc3, 0x1b,
     0xeb, 0x00, 0x9c, 0x5c, 0x2c, 0x49, 0xaa, 0x2e, 0x4e, 0xad, 0xb2, 0x17, 0xad, 0x8c, 0xc0, 0x9b},
};

static const unsigned char sm3_sum[5][32] = {
    { 0x66, 0xc7, 0xf0, 0xf4, 0x62, 0xee, 0xed, 0xd9, 0xd1, 0xf2, 0xd4, 0x6b, 0xdc, 0x10, 0xe4, 0xe2,
      0x41, 0x67, 0xc4, 0x87, 0x5c, 0xf2, 0xf7, 0xa2, 0x29, 0x7d, 0xa0, 0x2b, 0x8f, 0x4b, 0xa8, 0xe0 },
    { 0x63, 0x9b, 0x6c, 0xc5, 0xe6, 0x4d, 0x9e, 0x37, 0xa3, 0x90, 0xb1, 0x92, 0xdf, 0x4f, 0xa1, 0xea,
      0x07, 0x20, 0xab, 0x74, 0x7f, 0xf6, 0x92, 0xb9, 0xf3, 0x8c, 0x4e, 0x66, 0xad, 0x7b, 0x8c, 0x05 },
    { 0x65, 0x60, 0xd3, 0x7d, 0x98, 0x4e, 0xbd, 0x3c, 0xff, 0x3b, 0xb7, 0xbe, 0xfe, 0x92, 0xd5, 0xe1,
      0xc6, 0x14, 0x33, 0x60, 0x32, 0x63, 0xc7, 0xcb, 0xc5, 0xf1, 0x93, 0x23, 0xa7, 0xed, 0x66, 0x9b },
    { 0xc8, 0xaa, 0xf8, 0x94, 0x29, 0x55, 0x40, 0x29, 0xe2, 0x31, 0x94, 0x1a, 0x2a, 0xcc, 0x0a, 0xd6,
      0x1f, 0xf2, 0xa5, 0xac, 0xd8, 0xfa, 0xdd, 0x25, 0x84, 0x7a, 0x3a, 0x73, 0x2b, 0x3b, 0x02, 0xc3 },
};

static hi_u8 sha1_hmac_test_key[3][26] = {
    { "Hi ThereHi There" },//16 bytes
    { "There is the key" },
    { "This is your dog" },
};

static hi_u8 sha2_hmac_test_sum[3][32] = {

    {   0x92, 0x89, 0xA8, 0xC5, 0x56, 0x35, 0xAE, 0x82,
        0x9A, 0x63, 0x52, 0x55, 0xCF, 0x61, 0x42, 0x13,
        0x58, 0x24, 0xD3, 0xDB, 0xAB, 0x2D, 0x91, 0x1A,
        0xBC, 0x84, 0x22, 0x9C, 0x65, 0xCD, 0x3D, 0x4A
    },  // one block
    {   0xEF, 0x03, 0xC8, 0xB5, 0x2F, 0xFC, 0x02, 0x3B,
        0x7A, 0x99, 0x69, 0xF7, 0x0E, 0xF6, 0x03, 0x22,
        0x90, 0x2B, 0x47, 0x98, 0x1F, 0x9B, 0x8F, 0xAF,
        0xFE, 0x43, 0xE5, 0x61, 0x33, 0x8A, 0xE3, 0x44
    },  // two blocks
    {   0x4B, 0x3B, 0xF5, 0xA2, 0x57, 0xB6, 0xF9, 0x10,
        0x35, 0x9E, 0xF4, 0x8D, 0x8A, 0x3B, 0xE1, 0x6E,
        0x0A, 0x09, 0x3A, 0x96, 0x53, 0x51, 0x72, 0xBD,
        0x6F, 0x82, 0x0D, 0xBC, 0xFB, 0x1D, 0x9B, 0x00
    }  // 10000 'a'
};

static hi_u8 abuf[LONG_DATA_SIZE];

hi_s32 SHA256()
{
    hi_s32 ret;
    hi_u8 hash[32];
    hi_u32 i = 0;
    hi_u32 j = 0;
    hi_u32 hash_len = 32;
    hi_handle handle[MAX_HASH_HANDLE];
    hi_unf_cipher_hash_attr hash_attr;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        memset(&hash_attr, 0, sizeof(hi_unf_cipher_hash_attr));
        hash_attr.hash_type = HI_UNF_CIPHER_HASH_TYPE_SHA256;

        ret = hi_unf_cipher_hash_init(&hash_attr, &handle[i]);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
    }
    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        if (i == 3) {
            memset(abuf, 'a', LONG_DATA_SIZE);
            for (j = 0; j < 1000000 / LONG_DATA_SIZE; j++) {
                ret = hi_unf_cipher_hash_update(handle[i], abuf, LONG_DATA_SIZE);
                if (ret != HI_SUCCESS) {
                    ret = HI_FAILURE;
                    goto EXIT;
                }
            }
        } else {
            ret = hi_unf_cipher_hash_update(handle[i], sha1_buf[i], sha1_buflen[i]);
            if (ret != HI_SUCCESS) {
                ret = HI_FAILURE;
                goto EXIT;
            }
        }
    }
    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        ret = hi_unf_cipher_hash_final(handle[i], hash, &hash_len);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }

        if (memcmp(hash, sha2_sum[i], 32) != 0) {
            HI_ERR_CIPHER("SHA256 run failed, sample %d!\n", i);
            print_buffer("sha256 result:", hash, 32);
            print_buffer("golden data:", sha2_sum[i], 20);
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();

    hi_unf_cipher_deinit();
    return HI_SUCCESS;
}

hi_s32 SHA384()
{
    hi_s32 ret;
    hi_u8 hash[64];
    hi_u32 i = 0;
    hi_u32 j = 0;
    hi_u32 hash_len = 64;
    hi_handle handle[MAX_HASH_HANDLE];
    hi_unf_cipher_hash_attr hash_attr;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    memset(hash, 0, 20);

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        memset(&hash_attr, 0, sizeof(hi_unf_cipher_hash_attr));
        hash_attr.hash_type = HI_UNF_CIPHER_HASH_TYPE_SHA384;

        ret = hi_unf_cipher_hash_init(&hash_attr, &handle[i]);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
    }
    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        if (i == 3) {
            memset(abuf, 'a', LONG_DATA_SIZE);
            for (j = 0; j < 1000000 / LONG_DATA_SIZE; j++) {
                ret = hi_unf_cipher_hash_update(handle[i], abuf, LONG_DATA_SIZE);
                if (ret != HI_SUCCESS) {
                    ret = HI_FAILURE;
                    goto EXIT;
                }
            }
        } else {
            ret = hi_unf_cipher_hash_update(handle[i], sha1_buf[i], sha1_buflen[i]);
            if (ret != HI_SUCCESS) {
                ret = HI_FAILURE;
                goto EXIT;
            }
        }
    }

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        ret = hi_unf_cipher_hash_final(handle[i], hash, &hash_len);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
        if (memcmp(hash, sha384_sum[i], 48) != 0) {
            HI_ERR_CIPHER("SHA1 run failed, sample %d!\n", i);
            print_buffer("sha384 result:", hash, 48);
            print_buffer("golden data:", sha384_sum[i], 48);
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();
    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

hi_s32 SHA512()
{
    hi_s32 ret;
    hi_u8 hash[64];
    hi_u32 i = 0;
    hi_u32 j = 0;
    hi_u32 hash_len = 64;
    hi_handle handle[MAX_HASH_HANDLE];
    hi_unf_cipher_hash_attr hash_attr;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    memset(hash, 0, 20);

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        memset(&hash_attr, 0, sizeof(hi_unf_cipher_hash_attr));
        hash_attr.hash_type = HI_UNF_CIPHER_HASH_TYPE_SHA512;

        ret = hi_unf_cipher_hash_init(&hash_attr, &handle[i]);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
    }
    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        if (i == 3) {
            memset(abuf, 'a', LONG_DATA_SIZE);
            for (j = 0; j < 1000000 / LONG_DATA_SIZE; j++) {
                ret = hi_unf_cipher_hash_update(handle[i], abuf, LONG_DATA_SIZE);
                if (ret != HI_SUCCESS) {
                    ret = HI_FAILURE;
                    goto EXIT;
                }
            }
        } else {
            ret = hi_unf_cipher_hash_update(handle[i], sha1_buf[i], sha1_buflen[i]);
            if (ret != HI_SUCCESS) {
                ret = HI_FAILURE;
                goto EXIT;
            }
        }
    }

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        ret = hi_unf_cipher_hash_final(handle[i], hash, &hash_len);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
        if (memcmp(hash, sha512_sum[i], 64) != 0) {
            HI_ERR_CIPHER("SHA1 run failed, sample %d!\n", i);
            print_buffer("sha224 result:", hash, 64);
            print_buffer("golden data:", sha224_sum[i], 64);
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();
    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

hi_s32 HMAC_SHA256()
{
    hi_s32 ret;
    hi_u8 hash[32];
    hi_u32 i;
    hi_u32 hash_len = 32;
    hi_handle handle[MAX_HMAC_HANDLE];
    hi_unf_cipher_hash_attr hash_attr;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    memset(&hash_attr, 0, sizeof(hi_unf_cipher_hash_attr));
    hash_attr.hmac_key = malloc(16);
    hash_attr.hmac_key_len = 16;
    if (hash_attr.hmac_key == NULL) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    memset(hash, 0, 32);

    for (i = 0; i < MAX_HMAC_HANDLE; i++) {
        hash_attr.hash_type = HI_UNF_CIPHER_HASH_TYPE_HMAC_SHA256;
        memcpy(hash_attr.hmac_key, sha1_hmac_test_key[i], 16);

        ret = hi_unf_cipher_hash_init(&hash_attr, &handle[i]);
        if (ret != HI_SUCCESS) {
            goto EXIT;
        }
    }
    for (i = 0; i < MAX_HMAC_HANDLE; i++) {
        if (i == 2) {
            memset(abuf, 'a', 10000);
            ret = hi_unf_cipher_hash_update(handle[i], abuf, 10000);
        } else {
            ret = hi_unf_cipher_hash_update(handle[i], sha1_buf[i], sha1_buflen[i]);
        }
        if (ret != HI_SUCCESS) {
            goto EXIT;
        }
    }
    for (i = 0; i < MAX_HMAC_HANDLE; i++) {
        ret = hi_unf_cipher_hash_final(handle[i], hash, &hash_len);
        if (ret != HI_SUCCESS) {
            goto EXIT;
        }

        print_buffer("hmac-sha256 result:", hash, 32);
        if (memcmp(hash, sha2_hmac_test_sum[i], 32) != 0) {
            HI_ERR_CIPHER("HMAC_SHA256 run failed, sample %d!\n", i);
            print_buffer("HMAC256 result:", hash, 32);
            print_buffer("golden data:", sha2_hmac_test_sum[i], 20);
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();

    free(hash_attr.hmac_key);
    hash_attr.hmac_key = NULL;
    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

hi_s32 SM3()
{
    hi_s32 ret;
    hi_u8 hash[64];
    hi_u32 i = 0;
    hi_u32 j = 0;
    hi_u32 hash_len = 64;
    hi_handle handle[MAX_HASH_HANDLE];
    hi_unf_cipher_hash_attr hash_attr;

    ret = hi_unf_cipher_init();
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    memset(hash, 0, 32);

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        memset(&hash_attr, 0, sizeof(hi_unf_cipher_hash_attr));
        hash_attr.hash_type = HI_UNF_CIPHER_HASH_TYPE_SM3;

        ret = hi_unf_cipher_hash_init(&hash_attr, &handle[i]);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
    }
    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        if (i == 3) {
            memset(abuf, 'a', LONG_DATA_SIZE);
            for (j = 0; j < 1000000 / LONG_DATA_SIZE; j++) {
                ret = hi_unf_cipher_hash_update(handle[i], abuf, LONG_DATA_SIZE);
                if (ret != HI_SUCCESS) {
                    ret = HI_FAILURE;
                    goto EXIT;
                }
            }
        } else {
            ret = hi_unf_cipher_hash_update(handle[i], sha1_buf[i], sha1_buflen[i]);
            if (ret != HI_SUCCESS) {
                ret = HI_FAILURE;
                goto EXIT;
            }
        }
    }

    for (i = 0; i < MAX_HASH_HANDLE; i++) {
        ret = hi_unf_cipher_hash_final(handle[i], hash, &hash_len);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
        if (memcmp(hash, sm3_sum[i], 32) != 0) {
            HI_ERR_CIPHER("SM3 run failed, sample %d!\n", i);
            print_buffer("SM3 result:", hash, 32);
            print_buffer("golden data:", sm3_sum[i], 32);
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

int main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_u32 func_number;

    if (argc != 2) {
        HI_INFO_CIPHER("usage: %s func_number\n\n", argv[0]);
        HI_INFO_CIPHER("#########cipher sample##########\n"
                       "[0] SHA256\n"
                       "[1] HMAC-SHA256\n"
                       "[2] SHA384\n"
                       "[3] SHA512\n"
                       "[4] SM3\n");

        return HI_SUCCESS;
    }
    func_number = strtol(argv[1], NULL, 0);

    if (func_number == 0) { /* case 0 */
        ret = SHA256();
    } else if (func_number == 1) { /* case 1 */
        ret = HMAC_SHA256();
    } else if (func_number == 2) { /* case 2 */
        ret = SHA384();
    } else if (func_number == 3) { /* case 3 */
        ret = SHA512();
    } else if (func_number == 4) { /* case 4 */
        ret = SM3();
    }
    else {
        HI_INFO_CIPHER("func_number %d do not exist!\n", func_number);
        ret = HI_FAILURE;
    }

    if (ret != HI_SUCCESS) {
        HI_INFO_CIPHER("func_number %d run failure.\n", func_number);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
