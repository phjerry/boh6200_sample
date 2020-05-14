/*
 * Copyright (C) hisilicon technologies co., ltd. 2019-2019. all rights reserved.
 * Description: drivers of sample_dh
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

#define HI_ERR_CIPHER(format, arg...)  HI_PRINT("\033[0;1;31m" format "\033[0m", ##arg)
#define HI_INFO_CIPHER(format, arg...) HI_PRINT("\033[0;1;32m" format "\033[0m", ##arg)
#define TEST_END_PASS()                \
    HI_INFO_CIPHER("****************** %s test PASS !!! ******************\n", __FUNCTION__)
#define TEST_END_FAIL() \
    HI_ERR_CIPHER("****************** %s test FAIL !!! ******************\n", __FUNCTION__)
#define TEST_RESULT_PRINT() do { \
        if (ret) {            \
            TEST_END_FAIL();  \
        }                     \
        else {                \
            TEST_END_PASS();  \
        } \
    } while (0)

static hi_void print_buffer(const char *string, const hi_u8 *input, hi_u32 length)
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

static hi_u8 read_char(const char *str)
{
    hi_u8 ch = 0;

    if ((str[0] >= 'A') && (str[0] <= 'F')) {
        ch = str[0] - 'A' + 10;
    } else if ((str[0] >= 'a') && (str[0] <= 'f')) {
        ch = str[0] - 'a' + 10;
    } else if ((str[0] >= '0') && (str[0] <= '9')) {
        ch = str[0] - '0';
    }

    ch *= 16;

    if ((str[1] >= 'A') && (str[1] <= 'F')) {
        ch += str[1] - 'A' + 10;
    } else if ((str[1] >= 'a') && (str[1] <= 'f')) {
        ch += str[1] - 'a' + 10;
    } else if ((str[1] >= '0') && (str[1] <= '9')) {
        ch += str[1] - '0';
    }

    return ch;
}

static void read_string(const char *str, hi_u8 *buf, hi_u32 len)
{
    hi_u32 i;
    hi_u32 str_len;

    memset(buf, 0, len);

    str_len = strlen(str);
    if (len < str_len / 2) {
        str_len = len * 2;
    }

    for (i = (len - str_len / 2); i < len; i++) {
        buf[i] = read_char(str + i * 2);
    }
}

#define MBEDTLS_DHM_RFC3526_MODP_2048_P \
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"                      \
    "29024E088A67CC74020BBEA63B139B22514A08798E3404DD"                      \
    "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"                      \
    "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"                      \
    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"                      \
    "C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"                      \
    "83655D23DCA3AD961C62F356208552BB9ED529077096966D"                      \
    "670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"                      \
    "E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"                      \
    "DE2BCBF6955817183995497CEA956AE515D2261898FA0510"                      \
    "15728E5A8AACAA68FFFFFFFFFFFFFFFF"

#define MBEDTLS_DHM_RFC3526_MODP_2048_G \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "000000000000000000000000000000000000000000000000"                      \
    "00000000000000000000000000000002"

#define MBEDTLS_DHM_RFC5114_MODP_2048_P \
    "AD107E1E9123A9D0D660FAA79559C51FA20D64E5683B9FD1"                      \
    "B54B1597B61D0A75E6FA141DF95A56DBAF9A3C407BA1DF15"                      \
    "EB3D688A309C180E1DE6B85A1274A0A66D3F8152AD6AC212"                      \
    "9037C9EDEFDA4DF8D91E8FEF55B7394B7AD5B7D0B6C12207"                      \
    "C9F98D11ED34DBF6C6BA0B2C8BBC27BE6A00E0A0B9C49708"                      \
    "B3BF8A317091883681286130BC8985DB1602E714415D9330"                      \
    "278273C7DE31EFDC7310F7121FD5A07415987D9ADC0A486D"                      \
    "CDF93ACC44328387315D75E198C641A480CD86A1B9E587E8"                      \
    "BE60E69CC928B2B9C52172E413042E9B23F10B0E16E79763"                      \
    "C9B53DCF4BA80A29E3FB73C16B8E75B97EF363E2FFA31F71"                      \
    "CF9DE5384E71B81C0AC4DFFE0C10E64F"

#define MBEDTLS_DHM_RFC5114_MODP_2048_G \
    "AC4032EF4F2D9AE39DF30B5C8FFDAC506CDEBE7B89998CAF"                      \
    "74866A08CFE4FFE3A6824A4E10B9A6F0DD921F01A70C4AFA"                      \
    "AB739D7700C29F52C57DB17C620A8652BE5E9001A8D66AD7"                      \
    "C17669101999024AF4D027275AC1348BB8A762D0521BC98A"                      \
    "E247150422EA1ED409939D54DA7460CDB5F6C6B250717CBE"                      \
    "F180EB34118E98D119529A45D6F834566E3025E316A330EF"                      \
    "BB77A86F0C1AB15B051AE3D428C8F8ACB70A8137150B8EEB"                      \
    "10E183EDD19963DDD9E263E4770589EF6AA21E7F5F2FF381"                      \
    "B539CCE3409D13CD566AFBB48D6C019181E1BCFE94B30269"                      \
    "EDFE72FE9B6AA4BD7B5A0F1C71CFFF4C19C418E1F6EC0179"                      \
    "81BC087F2A7065B384B890D3191F2BFA"

#define MBEDTLS_DHM_INF_SEC_MODP_2048_G \
    "8B43673C16DE5AFD9109DC2E3D4074DE"                      \
    "89D74FE965EA4A4B99B976CA4AFEAAC8"                      \
    "ECADE8E4D9310445D584D878658C23A1"                      \
    "E04975FA89E45088CA762C1AC2C18FFE"                      \
    "D237E57877910E0C39A04FBEA02DF079"

#define MBEDTLS_DHM_INF_SEC_MODP_2048_P \
    "6D26037A747FB19258C07CAE416315E3"                      \
    "85AC5690002B1ABAEF55A5DC3773CB5C"                      \
    "5AD6A518932BD15DEB835CCBF2E217CC"                      \
    "84406713213365F52BA96143CD1DC669"                      \
    "DA9184AB8DBF93C6ECC200F9D7DDCA71"

#define MAX_KEY_SIZE     256
#define INF_SEC_KEY_SIZE 40

int main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_u32 i;
    hi_u32 dhm_sizes[] = { MAX_KEY_SIZE, MAX_KEY_SIZE, INF_SEC_KEY_SIZE };
    hi_unf_cipher_dh_gen_key_data dh_gen_key_data_a;
    hi_unf_cipher_dh_gen_key_data dh_gen_key_data_b;
    const char *dhm_p[] = { MBEDTLS_DHM_RFC3526_MODP_2048_P,
                            MBEDTLS_DHM_RFC5114_MODP_2048_P, MBEDTLS_DHM_INF_SEC_MODP_2048_P};
    const char *dhm_g[] = { MBEDTLS_DHM_RFC3526_MODP_2048_G, MBEDTLS_DHM_RFC5114_MODP_2048_G,
                            MBEDTLS_DHM_INF_SEC_MODP_2048_G};
    hi_u8 *p;
    hi_u8 *g;
    hi_u8 *priv_key_a;
    hi_u8 *priv_key_b;
    hi_u8 *pub_key_a;
    hi_u8 *pub_key_b;
    hi_u8 *shared_secret_a;
    hi_u8 *shared_secret_b;
    hi_u8 *buf;

    buf = (hi_u8 *)malloc(MAX_KEY_SIZE * 8);
    if (buf == HI_NULL) {
        HI_ERR_CIPHER("malloc for buf failed\n");
        return HI_FAILURE;
    }

    p = buf;
    g = p + MAX_KEY_SIZE;
    priv_key_a = g + MAX_KEY_SIZE;
    priv_key_b = priv_key_a + MAX_KEY_SIZE;
    pub_key_a = priv_key_b + MAX_KEY_SIZE;
    pub_key_b = pub_key_a + MAX_KEY_SIZE;
    shared_secret_a = pub_key_b + MAX_KEY_SIZE;
    shared_secret_b = shared_secret_a + MAX_KEY_SIZE;

    ret = hi_unf_cipher_init();
    if (HI_SUCCESS != ret) {
        return HI_FAILURE;
    }

    for (i = 0; i < 3; i++) {
        HI_INFO_CIPHER("\n*************************** D H - T E S T %d ***********************\n", i);

        read_string(dhm_g[i], g, dhm_sizes[i]);
        read_string(dhm_p[i], p, dhm_sizes[i]);

        dh_gen_key_data_a.g = g;
        dh_gen_key_data_a.p = p;
        dh_gen_key_data_a.input_priv_key = HI_NULL;
        dh_gen_key_data_a.output_priv_key = priv_key_a;
        dh_gen_key_data_a.pub_key = pub_key_a;
        dh_gen_key_data_a.key_size = dhm_sizes[i];

        dh_gen_key_data_b.g = g;
        dh_gen_key_data_b.p = p;
        dh_gen_key_data_b.input_priv_key = HI_NULL;
        dh_gen_key_data_b.output_priv_key = priv_key_b;
        dh_gen_key_data_b.pub_key = pub_key_b;
        dh_gen_key_data_b.key_size = dhm_sizes[i];

        // print_buffer("G", g, dhm_sizes[i]);
        // print_buffer("P", p, dhm_sizes[i]);

        ret = hi_unf_cipher_dh_gen_key(&dh_gen_key_data_a);
        if (HI_SUCCESS != ret) {
            HI_ERR_CIPHER("hi_unf_cipher_dh_gen_key A failed\n");
            return HI_FAILURE;
        }
        print_buffer("priv_key_a", priv_key_a, dhm_sizes[i]);
        print_buffer("pub_key_a", pub_key_a, dhm_sizes[i]);

        ret = hi_unf_cipher_dh_gen_key(&dh_gen_key_data_b);
        if (HI_SUCCESS != ret) {
            HI_ERR_CIPHER("hi_unf_cipher_dh_gen_key B failed\n");
            return HI_FAILURE;
        }
        print_buffer("priv_key_b", priv_key_b, dhm_sizes[i]);
        print_buffer("pub_key_b", pub_key_b, dhm_sizes[i]);

        ret = hi_unf_cipher_dh_compute_key(p, priv_key_a, pub_key_b, shared_secret_a, dhm_sizes[i]);
        if (HI_SUCCESS != ret) {
            HI_ERR_CIPHER("HI_UNF_CIPHER_DhComputeKey A failed\n");
            return HI_FAILURE;
        }
        print_buffer("shared_secret_a", shared_secret_a, dhm_sizes[i]);

        ret = hi_unf_cipher_dh_compute_key(p, priv_key_b, pub_key_a, shared_secret_b, dhm_sizes[i]);
        if (HI_SUCCESS != ret) {
            HI_ERR_CIPHER("HI_UNF_CIPHER_DhComputeKey B failed\n");
            return HI_FAILURE;
        }
        print_buffer("shared_secret_b", shared_secret_b, dhm_sizes[i]);

        if (memcmp(shared_secret_a, shared_secret_b, dhm_sizes[i]) != 0) {
            HI_ERR_CIPHER("shared_secret_a != shared_secret_b failed\n");
            print_buffer("A", shared_secret_a, dhm_sizes[i]);
            print_buffer("B", shared_secret_b, dhm_sizes[i]);
            return HI_FAILURE;
        }
    }

    TEST_END_PASS();

    free(buf);

    hi_unf_cipher_deinit();

    return HI_SUCCESS;
}
