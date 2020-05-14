/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_ecdh
 * Author: zhaoguihong
 * Create: 2019-06-18
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "hi_type.h"
#include "hi_unf_system.h"
#include "hi_unf_cipher.h"

#define HI_ERR_CIPHER(format, arg...)     HI_PRINT( "\033[0;1;31m" format "\033[0m", ## arg)
#define HI_INFO_CIPHER(format, arg...)    HI_PRINT( "\033[0;1;32m" format "\033[0m", ## arg)
#define TEST_END_PASS()                   HI_INFO_CIPHER("****************** %s test PASS !!! ******************\n", __FUNCTION__)
#define TEST_END_FAIL()                   HI_ERR_CIPHER("****************** %s test FAIL !!! ******************\n", __FUNCTION__)
#define TEST_RESULT_PRINT()              { if (ret) TEST_END_FAIL(); else TEST_END_PASS();}

static hi_s32 print_buffer(const char *string, const hi_u8 *input, hi_u32 length)
{
    hi_u32 i = 0;

    if ( NULL != string )
    {
        HI_PRINT("%s\n", string);
    }

    for ( i = 0 ; i < length; i++ )
    {
        if( (i % 16 == 0) && (i != 0)) HI_PRINT("\n");
        HI_PRINT("0x%02x ", input[i]);
    }
    HI_PRINT("\n");

    return HI_SUCCESS;
}

static hi_u8 read_char(const char *str)
{
    hi_u8 ch = 0;

    if ((str[0] >= 'A') && (str[0] <= 'F'))
    {
        ch = str[0] - 'A' + 10;
    }
    else if ((str[0] >= 'a') && (str[0] <= 'f'))
    {
        ch = str[0] - 'a' + 10;
    }
    else if ((str[0] >= '0') && (str[0] <= '9'))
    {
        ch = str[0] - '0';
    }

    ch *= 16;

    if ((str[1] >= 'A') && (str[1] <= 'F'))
    {
        ch += str[1] - 'A' + 10;
    }
    else if ((str[1] >= 'a') && (str[1] <= 'f'))
    {
        ch += str[1] - 'a' + 10;
    }
    else if ((str[1] >= '0') && (str[1] <= '9'))
    {
        ch += str[1] - '0';
    }

    return ch;
}

static void read_string(const char *str, hi_u8 * buf, hi_u32 len)
{
    hi_u32 i;
    hi_u32 str_len;

    memset(buf, 0, len);

    str_len = strlen(str);
    if (len < str_len/2)
    {
        str_len = len *2;
    }

    for(i= (len - str_len / 2); i<len; i++)
    {
        buf[i] = read_char(str+i*2);
    }
}

//http://www.secg.org/sec2-v2.pdf
#define ECDH_SECP192R1_P  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFF"
#define ECDH_SECP192R1_A  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFC"
#define ECDH_SECP192R1_B  "64210519E59C80E70FA7E9AB72243049FEB8DEECC146B9B1"
#define ECDH_SECP192R1_GX "188DA80EB03090F67CBF20EB43A18800F4FF0AFD82FF1012"
#define ECDH_SECP192R1_GY "07192B95FFC8DA78631011ED6B24CDD573F977A11E794811"
#define ECDH_SECP192R1_N  "FFFFFFFFFFFFFFFFFFFFFFFF99DEF836146BC9B1B4D22831"
#define ECDH_SECP192R1_H                                                   1

#define ECDH_SECP224R1_P  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000001"
#define ECDH_SECP224R1_A  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFE"
#define ECDH_SECP224R1_B  "B4050A850C04B3ABF54132565044B0B7D7BFD8BA270B39432355FFB4"
#define ECDH_SECP224R1_GX "B70E0CBD6BB4BF7F321390B94A03C1D356C21122343280D6115C1D21"
#define ECDH_SECP224R1_GY "BD376388B5F723FB4C22DFE6CD4375A05A07476444D5819985007E34"
#define ECDH_SECP224R1_N  "FFFFFFFFFFFFFFFFFFFFFFFFFFFF16A2E0B8F03E13DD29455C5C2A3D"
#define ECDH_SECP224R1_H                                                    1

#define ECDH_SECP256R1_P  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F"
#define ECDH_SECP256R1_A  "0000000000000000000000000000000000000000000000000000000000000000"
#define ECDH_SECP256R1_B  "0000000000000000000000000000000000000000000000000000000000000007"
#define ECDH_SECP256R1_GX "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"
#define ECDH_SECP256R1_GY "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"
#define ECDH_SECP256R1_N  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"
#define ECDH_SECP256R1_H                                                   1

#define ECDH_SECP384R1_P  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF"
#define ECDH_SECP384R1_A  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFC"
#define ECDH_SECP384R1_B  "B3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEF"
#define ECDH_SECP384R1_GX "AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7"
#define ECDH_SECP384R1_GY "3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5F"
#define ECDH_SECP384R1_N  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973"
#define ECDH_SECP384R1_H                                                   1

#define ECDH_SECP512R1_P  "01FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
#define ECDH_SECP512R1_A  "01FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC"
#define ECDH_SECP512R1_B  "0051953EB9618E1C9A1F929A21A0B68540EEA2DA725B99B315F3B8B489918EF109E156193951EC7E937B1652C0BD3BB1BF073573DF883D2C34F1EF451FD46B503F00"
#define ECDH_SECP512R1_GX "00C6858E06B70404E9CD9E3ECB662395B4429C648139053FB521F828AF606B4D3DBAA14B5E77EFE75928FE1DC127A2FFA8DE3348B3C1856A429BF97E7E31C2E5BD66"
#define ECDH_SECP512R1_GY "011839296A789A3BC0045C8A5FB42C7D1BD998F54449579B446817AFBD17273E662C97EE72995EF42640C550B9013FAD0761353C7086A272C24088BE94769FD16650"
#define ECDH_SECP512R1_N  "01FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFA51868783BF2F966B7FCC0148F709A5D03BB5C9B8899C47AEBB6FB71E91386409"
#define ECDH_SECP512R1_H                                                   1

#define MAX_KEY_SIZE     66

int main(int argc, char* argv[])
{
    hi_s32 ret = HI_SUCCESS;
    hi_u32 i;
    hi_unf_cipher_ecc_param ecc_param;
    hi_u32 ecdh_sizes[] =   { 24, 28, 32, 48, 66};
    const char *ecdh_p[] =  {ECDH_SECP192R1_P, ECDH_SECP224R1_P, ECDH_SECP256R1_P, ECDH_SECP384R1_P, ECDH_SECP512R1_P};
    const char *ecdh_a[] =  {ECDH_SECP192R1_A, ECDH_SECP224R1_A, ECDH_SECP256R1_A, ECDH_SECP384R1_A, ECDH_SECP512R1_A};
    const char *ecdh_b[] =  {ECDH_SECP192R1_B, ECDH_SECP224R1_B, ECDH_SECP256R1_B, ECDH_SECP384R1_B, ECDH_SECP512R1_B};
    const char *ecdh_gx[] = {ECDH_SECP192R1_GX, ECDH_SECP224R1_GX, ECDH_SECP256R1_GX, ECDH_SECP384R1_GX, ECDH_SECP512R1_GX};
    const char *ecdh_gy[] = {ECDH_SECP192R1_GY, ECDH_SECP224R1_GY, ECDH_SECP256R1_GY, ECDH_SECP384R1_GY, ECDH_SECP512R1_GY};
    const char *ecdh_n[] =  {ECDH_SECP192R1_N, ECDH_SECP224R1_N, ECDH_SECP256R1_N, ECDH_SECP384R1_N, ECDH_SECP512R1_N};
    const hi_u32 ecdh_h[] =  {ECDH_SECP192R1_H, ECDH_SECP224R1_H, ECDH_SECP256R1_H, ECDH_SECP384R1_H, ECDH_SECP512R1_H};
    hi_u8 *priv_key_a;
    hi_u8 *priv_key_b;
    hi_u8 *pub_key_ax;
    hi_u8 *pub_key_ay;
    hi_u8 *pub_key_bx;
    hi_u8 *pub_key_by;
    hi_u8 *shared_secret_a;
    hi_u8 *shared_secret_b;
    hi_u8 *buf;

    buf = (hi_u8*)malloc(MAX_KEY_SIZE * 14);
    if(buf == HI_NULL)
    {
        HI_ERR_CIPHER("malloc for buf failed\n");
        return HI_FAILURE;
    }

    ecc_param.p  = buf;
    ecc_param.a  = ecc_param.p + MAX_KEY_SIZE;
    ecc_param.b  = ecc_param.a + MAX_KEY_SIZE;
    ecc_param.gx = ecc_param.b + MAX_KEY_SIZE;
    ecc_param.gy = ecc_param.gx + MAX_KEY_SIZE;
    ecc_param.n  = ecc_param.gy + MAX_KEY_SIZE;
    priv_key_a = ecc_param.n + MAX_KEY_SIZE;
    priv_key_b = priv_key_a + MAX_KEY_SIZE;
    pub_key_ax = priv_key_b + MAX_KEY_SIZE;
    pub_key_ay = pub_key_ax + MAX_KEY_SIZE;
    pub_key_bx = pub_key_ay + MAX_KEY_SIZE;
    pub_key_by = pub_key_bx + MAX_KEY_SIZE;
    shared_secret_a = pub_key_by + MAX_KEY_SIZE;
    shared_secret_b = shared_secret_a + MAX_KEY_SIZE;

    ret = hi_unf_cipher_init();
    if ( HI_SUCCESS != ret )
    {
        return HI_FAILURE;
    }

    for(i=0; i<5; i++)
    {
        HI_INFO_CIPHER("\n*************************** E C D H - T E S T %d***********************\n", i);
        read_string(ecdh_p[i], ecc_param.p, ecdh_sizes[i]);
        read_string(ecdh_a[i], ecc_param.a, ecdh_sizes[i]);
        read_string(ecdh_b[i], ecc_param.b, ecdh_sizes[i]);
        read_string(ecdh_gx[i], ecc_param.gx, ecdh_sizes[i]);
        read_string(ecdh_gy[i], ecc_param.gy, ecdh_sizes[i]);
        read_string(ecdh_n[i], ecc_param.n, ecdh_sizes[i]);
        ecc_param.h = ecdh_h[i];
        ecc_param.key_size = ecdh_sizes[i];

        HI_PRINT("ecdh_sizes: %d\n", ecdh_sizes[i]);
 /*     print_buffer("P", ecc_param.p, ecdh_sizes[i]);
        print_buffer("A", ecc_param.a, ecdh_sizes[i]);
        print_buffer("B", ecc_param.b, ecdh_sizes[i]);
        print_buffer("GX", ecc_param.gx, ecdh_sizes[i]);
        print_buffer("GY", ecc_param.gy, ecdh_sizes[i]);
        print_buffer("N", ecc_param.n, ecdh_sizes[i]);*/

        ret = hi_unf_cipher_ecc_gen_key(&ecc_param, HI_NULL, priv_key_a, pub_key_ax, pub_key_ay);
        if ( HI_SUCCESS != ret )
        {
            HI_ERR_CIPHER("hi_unf_cipher_ecc_gen_key A failed, ret = 0x%x\n", -ret);
            return HI_FAILURE;
        }
    /*  print_buffer("priv_key_a", priv_key_a, ecdh_sizes[i]);
        print_buffer("pub_key_ax", pub_key_ax, ecdh_sizes[i]);
        print_buffer("pub_key_ay", pub_key_ay, ecdh_sizes[i]);*/

        ret = hi_unf_cipher_ecc_gen_key(&ecc_param, HI_NULL, priv_key_b, pub_key_bx, pub_key_by);
        if ( HI_SUCCESS != ret )
        {
            HI_ERR_CIPHER("hi_unf_cipher_ecc_gen_key B failed\n");
            return HI_FAILURE;
        }
   /*   print_buffer("priv_key_b", priv_key_b, ecdh_sizes[i]);
        print_buffer("pub_key_bx", pub_key_bx, ecdh_sizes[i]);
        print_buffer("pub_key_by", pub_key_by, ecdh_sizes[i]);*/

        ret = hi_unf_cipher_ecdh_compute_key(&ecc_param, priv_key_a, pub_key_bx, pub_key_by, shared_secret_a);
        if ( HI_SUCCESS != ret )
        {
            HI_ERR_CIPHER("hi_unf_cipher_ecdh_compute_key A failed, ret = 0x%x\n", -ret);
            return HI_FAILURE;
        }
 //     print_buffer("shared_secret_a", shared_secret_a, ecdh_sizes[i]);

        ret = hi_unf_cipher_ecdh_compute_key(&ecc_param, priv_key_b, pub_key_ax, pub_key_ay, shared_secret_b);
        if ( HI_SUCCESS != ret )
        {
            HI_ERR_CIPHER("hi_unf_cipher_ecdh_compute_key B failed, ret = 0x%x\n", -ret);
            return HI_FAILURE;
        }
 //     print_buffer("shared_secret_b", shared_secret_b, ecdh_sizes[i]);

        if(memcmp(shared_secret_a, shared_secret_b, ecdh_sizes[i]) != 0)
        {
            HI_ERR_CIPHER("shared_secret_a != shared_secret_b failed\n");
            print_buffer("A", shared_secret_a, ecdh_sizes[i]);
            print_buffer("B", shared_secret_b, ecdh_sizes[i]);
            return HI_FAILURE;
        }
    }

    TEST_END_PASS();

    free(buf);

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}
