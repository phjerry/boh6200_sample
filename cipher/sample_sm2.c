/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_sm2
 * Author: zhaoguihong
 * Create: 2019-06-18
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_cipher.h"
#include "hi_adp.h"

#define HI_ERR_CIPHER(format, arg...)  HI_PRINT("\033[0;1;31m" format "\033[0m", ##arg)
#define HI_INFO_CIPHER(format, arg...) HI_PRINT("\033[0;1;32m" format "\033[0m", ##arg)
#define TEST_END_PASS()                \
    HI_INFO_CIPHER("****************** %s test PASS !!! ******************\n", __FUNCTION__)
#define TEST_END_FAIL() \
    HI_ERR_CIPHER("****************** %s test FAIL !!! ******************\n", __FUNCTION__)
#define TEST_RESULT_PRINT() do {  \
        if (ret) {                \
            TEST_END_FAIL();      \
        }                         \
        else {                    \
            TEST_END_PASS();      \
        }                         \
    } while (0)

#define TEST_NORMAL_CNT   3
#define SM2_TEST_MSG_SIZE 1000

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

void get_rand(hi_u8 *buf, hi_u32 size)
{
    hi_u32 i;

    for (i = 0; i < size; i += 4) {
        *(hi_u32 *)(buf + i) = rand();
    }
}

/* SM2 signature and verify, use gold data */
hi_s32 sm2_test0(hi_void)
{
    hi_s32 ret;
    hi_unf_cipher_sm2_sign_param sm2_sign;
    hi_unf_cipher_sm2_verify_param sm2_verify;
    hi_unf_cipher_sm2_sign_verify_data sm2_sign_data;
    hi_u8 r[HI_UNF_CIPHER_SM2_LEN_IN_BYTE];
    hi_u8 s[HI_UNF_CIPHER_SM2_LEN_IN_BYTE];
    hi_u8 d[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
        "\x39\x45\x20\x8f\x7b\x21\x44\xb1\x3f\x36\xe3\x8a\xc6\xd3\x9f\x95"
        "\x88\x93\x93\x69\x28\x60\xb5\x1a\x42\xfb\x81\xef\x4d\xf7\xc5\xb8"
    };
    hi_u8 px[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
        "\x09\xf9\xdf\x31\x1e\x54\x21\xa1\x50\xdd\x7d\x16\x1e\x4b\xc5\xc6"
        "\x72\x17\x9f\xad\x18\x33\xfc\x07\x6b\xb0\x8f\xf3\x56\xf3\x50\x20"
    };
    hi_u8 py[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
        "\xcc\xea\x49\x0c\xe2\x67\x75\xa5\x2d\xc6\xea\x71\x8c\xc1\xaa\x60"
        "\x0a\xed\x05\xfb\xf3\x5e\x08\x4a\x66\x32\xf6\x07\x2d\xa9\xad\x13"
    };
    hi_u8 IDA[18] = { "\x42\x4c\x49\x43\x45\x31\x32\x33\x40\x59\x41\x48\x4f\x4f\x2\x43\x4f\x11" };
    /* hi_u8 u8k[HI_UNF_CIPHER_SM2_LEN_IN_BYTE]  = {
            "\x59\x27\x6\x27\xd5\x06\x86\x1_a\x16\x68\x0f\x3_a\xd9\xc0\x2d\xcc"
            "\xef\x3c\xc1\xfa\x3c\xdb\xe4\xce\x6d\x54\xb8\x0d\xea\xc1\xbc\x21"
       };
       hi_u8 gr[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
            "\x31\xfc\x32\x2e\x42\x04\xd0\x38\x87\x51\x8a\x3b\xaf\x93\xec\x86"
            "\xa9\xe6\x50\xce\xe1\x85\xde\x18\x5d\xeb\xae\xd9\xcf\xcb\xdd\xee"
       };
       hi_u8 gs[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
            "\xc0\xd8\x4e\x8f\xd1\x24\x8b\x7b\xac\x38\x11\xc8\xf1\xdc\xe8\x37"
            "\x45\xe8\x5e\x03\x97\x60\x69\x08\x2d\xd3\x22\xd2\x5b\xf7\xcf\x14"
       };
 */
    hi_u8 m[14] = { "\x6d\x65\x73\x73\x61\x67\x65\x20\x64\x69\x67\x65\x73\x74" };

    sm2_sign.id = IDA;
    sm2_sign.id_len = sizeof(IDA);
    memcpy(sm2_sign.d, d, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
    memcpy(sm2_sign.px, px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
    memcpy(sm2_sign.py, py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

    memset(&sm2_verify, 0, sizeof(hi_unf_cipher_sm2_verify_param));
    memcpy(sm2_verify.px, px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
    memcpy(sm2_verify.py, py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
    sm2_verify.id = IDA;
    sm2_verify.id_len = sizeof(IDA);

    hi_unf_cipher_init();

    sm2_sign_data.msg = m;
    sm2_sign_data.msg_size = sizeof(m);
    sm2_sign_data.sign_r = r;
    sm2_sign_data.sign_s = s;
    sm2_sign_data.sign_outbuf_len = HI_UNF_CIPHER_SM2_LEN_IN_BYTE;
    ret = hi_unf_cipher_sm2_sign(&sm2_sign, &sm2_sign_data);
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

    print_buffer("r", r, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
    print_buffer("s", s, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

    HI_PRINT("verify...\n");
    ret = hi_unf_cipher_sm2_verify(&sm2_verify, &sm2_sign_data);
    if (ret != HI_SUCCESS) {
        ret = HI_FAILURE;
        goto EXIT;
    }

EXIT:

    TEST_RESULT_PRINT();

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

/* SM2 signature and verify, use rand data */
hi_s32 sm2_test1(hi_void)
{
    hi_s32 ret;
    hi_unf_cipher_sm2_sign_param sm2_sign;
    hi_unf_cipher_sm2_verify_param sm2_verify;
    hi_unf_cipher_sm2_key sm2_key;
    hi_unf_cipher_sm2_sign_verify_data sm2_sign_data;
    hi_u8 r[HI_UNF_CIPHER_SM2_LEN_IN_BYTE];
    hi_u8 s[HI_UNF_CIPHER_SM2_LEN_IN_BYTE];
    hi_u8 *ida;
    hi_u8 *m;
    hi_u32 ida_len;
    hi_u32 m_len;
    hi_u32 i;

    hi_unf_cipher_init();

    ida = malloc(SM2_TEST_MSG_SIZE);
    m = malloc(SM2_TEST_MSG_SIZE);

    for (i = 0; i < TEST_NORMAL_CNT; i++) {
        HI_PRINT("\n-----------------progress: %d/%d----------------\n", i, TEST_NORMAL_CNT - 1);
        hi_unf_cipher_sm2_gen_key(&sm2_key);

        print_buffer("d", (hi_u8 *)sm2_key.d, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        print_buffer("px", (hi_u8 *)sm2_key.px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        print_buffer("py", (hi_u8 *)sm2_key.py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

        ida_len = rand() % SM2_TEST_MSG_SIZE;
        m_len = rand() % SM2_TEST_MSG_SIZE;
        HI_PRINT("IDALEN: 0x%x, MLEN:0x%x \n", ida_len, m_len);

        get_rand(ida, ida_len);
        get_rand(m, m_len);

        memset(&sm2_sign, 0, sizeof(hi_unf_cipher_sm2_sign_param));
        memcpy(sm2_sign.d, sm2_key.d, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

        memcpy(sm2_sign.px, sm2_key.px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        memcpy(sm2_sign.py, sm2_key.py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        sm2_sign.id = ida;
        sm2_sign.id_len = ida_len;

        memset(&sm2_verify, 0, sizeof(hi_unf_cipher_sm2_verify_param));
        memcpy(sm2_verify.px, sm2_key.px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        memcpy(sm2_verify.py, sm2_key.py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        sm2_verify.id = ida;
        sm2_verify.id_len = ida_len;

        sm2_sign_data.msg = m;
        sm2_sign_data.msg_size = m_len;
        sm2_sign_data.sign_r = r;
        sm2_sign_data.sign_s = s;
        sm2_sign_data.sign_outbuf_len = HI_UNF_CIPHER_SM2_LEN_IN_BYTE;
        ret = hi_unf_cipher_sm2_sign(&sm2_sign, &sm2_sign_data);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
        print_buffer("hi-r", r, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        print_buffer("hi-s", s, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

        memcpy(sm2_sign.d, sm2_key.d, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        ret = hi_unf_cipher_sm2_verify(&sm2_verify, &sm2_sign_data);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();

    free(ida);
    free(m);

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

/* SM2 encrypt and decrypt, use gold data */
hi_s32 sm2_test2(hi_void)
{
    hi_s32 ret;
    hi_u8 m[19] = { "encryption standard" };
    hi_u8 mo[19] = { 0 };
    hi_u8 c[65 + 19 + 32] = { 0 };
    hi_unf_cipher_sm2_enc_param sm2_enc;
    hi_unf_cipher_sm2_dec_param sm2_dec;
    hi_u32 mlen;
    hi_u32 clen;
    hi_u8 d[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
        "\x39\x45\x20\x8f\x7b\x21\x44\xb1\x3f\x36\xe3\x8a\xc6\xd3\x9f\x95"
        "\x88\x93\x93\x69\x28\x60\xb5\x1a\x42\xfb\x81\xef\x4d\xf7\xc5\xb8"
    };
    hi_u8 px[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
        "\x09\xf9\xdf\x31\x1e\x54\x21\xa1\x50\xdd\x7d\x16\x1e\x4b\xc5\xc6"
        "\x72\x17\x9f\xad\x18\x33\xfc\x07\x6b\xb0\x8f\xf3\x56\xf3\x50\x20"
    };
    hi_u8 py[HI_UNF_CIPHER_SM2_LEN_IN_BYTE] = {
        "\xcc\xea\x49\x0c\xe2\x67\x75\xa5\x2d\xc6\xea\x71\x8c\xc1\xaa\x60"
        "\x0a\xed\x05\xfb\xf3\x5e\x08\x4a\x66\x32\xf6\x07\x2d\xa9\xad\x13"
    };
    /* hi_u8 u8k[HI_UNF_CIPHER_SM2_LEN_IN_BYTE]  = {
            "\x59\x27\x6\x27\xd5\x06\x86\x1_a\x16\x68\x0f\x3_a\xd9\xc0\x2d\xcc"
            "\xef\x3c\xc1\xfa\x3c\xdb\xe4\xce\x6d\x54\xb8\x0d\xea\xc1\xbc\x21"
      };
    hi_u8 gc[65+19+32] = {
        "\x04\x04\xeb\xfc\x71\x8e\x8d\x17\x98\x62\x04\x32\x26\x8e\x77\xfe"
        "\xb6\x41\x5e\x2e\xde\x0e\x07\x3c\x0f\x4f\x64\x0e\xcd\x2e\x14\x9a"
        "\x73\xe8\x58\xf9\xd8\x1e\x54\x30\xa5\x7b\x36\xda\xab\x8f\x95\x0a"
        "\x3c\x64\xe6\xee\x6a\x63\x09\x4d\x99\x28\x3a\xff\x76\x7e\x12\x4d"
        "\xf0\x21\x88\x6c\xa9\x89\xca\x9c\x7d\x58\x08\x73\x07\xca\x93\x09"
        "\x2d\x65\x1e\xfa\x59\x98\x3c\x18\xf8\x09\xe2\x62\x92\x3c\x53\xae"
        "\xc2\x95\xd3\x03\x83\xb5\x4e\x39\xd6\x09\xd1\x60\xaf\xcb\x19\x08"
        "\xd0\xbd\x87\x66"}; */
    hi_unf_cipher_init();

    HI_PRINT("--------------------------encrypt-------------------------\n");
    memset(&sm2_enc, 0, sizeof(hi_unf_cipher_sm2_enc_param));
    memcpy(sm2_enc.px, px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
    memcpy(sm2_enc.py, py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

    memset(&sm2_dec, 0, sizeof(hi_unf_cipher_sm2_dec_param));
    memcpy(sm2_dec.d, d, 32);

    ret = hi_unf_cipher_sm2_encrypt(&sm2_enc, m, sizeof(m), c, &clen);
    if (clen != ((HI_UNF_CIPHER_SM2_LEN_IN_BYTE * 2) + sizeof(m) + HI_UNF_CIPHER_SM2_LEN_IN_BYTE)) {
        HI_ERR_CIPHER("error, sm2 encrypt failed, invalid clen(0x%x)!\n", clen);
        ret = HI_FAILURE;
        goto EXIT;
    }

    print_buffer("C", c, clen);

    HI_PRINT("--------------------------decrypt-------------------------\n");
    ret = hi_unf_cipher_sm2_decrypt(&sm2_dec, c, clen, mo, &mlen);
    if (ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    if (mlen != sizeof(m)) {
        HI_PRINT("mlen %d error\n", mlen);
        ret = HI_FAILURE;
        goto EXIT;
    }

    print_buffer("MO", mo, mlen);
    if (memcmp(m, mo, mlen) != 0) {
        HI_PRINT("sm2_decrypt check error\n");
        ret = HI_FAILURE;
        goto EXIT;
    }

EXIT:

    TEST_RESULT_PRINT();

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

/* SM2 encrypt and decrypt, use rand data */
hi_s32 sm2_test3(hi_void)
{
    hi_s32 ret;
    hi_u8 *mi;
    hi_u8 *mo;
    hi_u8 *mc;
    hi_unf_cipher_sm2_enc_param sm2_enc;
    hi_unf_cipher_sm2_dec_param sm2_dec;
    hi_unf_cipher_sm2_key sm2_key;
    hi_u32 mlen;
    hi_u32 clen;
    hi_u32 i;

    hi_unf_cipher_init();

    mi = malloc(SM2_TEST_MSG_SIZE);
    mo = malloc(SM2_TEST_MSG_SIZE);
    mc = malloc(SM2_TEST_MSG_SIZE + 65 + 32);

    for (i = 0; i < TEST_NORMAL_CNT; i++) {
        HI_PRINT("\n----------------------------progress: %d/%d---------------------------\n", i, TEST_NORMAL_CNT - 1);
        hi_unf_cipher_sm2_gen_key(&sm2_key);
        print_buffer("d", (hi_u8 *)sm2_key.d, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        print_buffer("px", (hi_u8 *)sm2_key.px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        print_buffer("py", (hi_u8 *)sm2_key.py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

        mlen = rand() % SM2_TEST_MSG_SIZE;
        get_rand(mi, mlen);

        HI_PRINT("mlen: 0x%x\n", mlen);

        memset(&sm2_enc, 0, sizeof(hi_unf_cipher_sm2_enc_param));
        memcpy(sm2_enc.px, sm2_key.px, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);
        memcpy(sm2_enc.py, sm2_key.py, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

        memset(&sm2_dec, 0, sizeof(hi_unf_cipher_sm2_dec_param));
        memcpy(sm2_dec.d, sm2_key.d, 32);

        HI_PRINT("--------------------------encrypt-------------------------\n");
        ret = hi_unf_cipher_sm2_encrypt(&sm2_enc, mi, mlen, mc, &clen);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }
        if (clen != ((HI_UNF_CIPHER_SM2_LEN_IN_BYTE * 2) + mlen + HI_UNF_CIPHER_SM2_LEN_IN_BYTE)) {
            HI_ERR_CIPHER("error, sm2 encrypt failed, invalid clen(0x%x)!\n", clen);
            return HI_FAILURE;
        }
        print_buffer("C", mc, clen > 128 ? 128 : clen);
        HI_PRINT("--------------------------decrypt-------------------------\n");
        if (clen != ((HI_UNF_CIPHER_SM2_LEN_IN_BYTE * 2) + mlen + HI_UNF_CIPHER_SM2_LEN_IN_BYTE)) {
            HI_ERR_CIPHER("error, sm2 encrypt failed, invalid clen(0x%x)!\n", clen);
            return HI_FAILURE;
        }

        memcpy(sm2_dec.d, sm2_key.d, HI_UNF_CIPHER_SM2_LEN_IN_BYTE);

        ret = hi_unf_cipher_sm2_decrypt(&sm2_dec, mc, clen, mo, &mlen);
        if (ret != HI_SUCCESS) {
            ret = HI_FAILURE;
            goto EXIT;
        }

        print_buffer("MO", mo, mlen > 128 ? 128 : mlen);

        if (memcmp(mi, mo, mlen) != 0) {
            HI_PRINT("sm2_decrypt check error\n");
            ret = HI_FAILURE;
            goto EXIT;
        }
    }

EXIT:

    TEST_RESULT_PRINT();

    free(mi);
    free(mo);
    free(mc);

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret;

    ret = sm2_test0();
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = sm2_test1();
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = sm2_test2();
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = sm2_test3();
    if (ret != HI_SUCCESS) {
        return ret;
    }

    return HI_SUCCESS;
}

