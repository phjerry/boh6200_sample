/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: TEE keyladder test sample.
 * Author: linux SDK team
 * Create: 2019-07-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tee_client_api.h"

#define HI_ERR_DFT(format, arg...)     printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)
#define HI_INFO_DFT(format, arg...)    printf("%s,%d: " format, __FUNCTION__, __LINE__, ## arg)

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

#define klad_mark()                printf("[%-32s][line:%04d]mark\n", __FUNCTION__, __LINE__)
#define klad_err_print_hex(val)    printf("[%-32s][line:%04d]%s = 0x%08x\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_info(val)   printf("[%-32s][line:%04d]%s\n", __FUNCTION__, __LINE__, val)
#define klad_err_print_val(val)    printf("[%-32s][line:%04d]%s = %d\n", __FUNCTION__, __LINE__, #val, val)
#define klad_err_print_point(val)  printf("[%-32s][line:%04d]%s = %p\n", __FUNCTION__, __LINE__, #val, val)
#define klad_print_error_code(err_code) \
    printf("[%-32s][line:%04d]return [0x%08x]\n", __FUNCTION__, __LINE__, err_code)
#define klad_print_error_func(func, err_code) \
    printf("[%-32s][line:%04d]call [%s] return [0x%08x]\n", __FUNCTION__, __LINE__, #func, err_code)

/*
 * *********************************KLAD****************************************
 */
typedef unsigned char           hi_uchar;
typedef unsigned char           hi_u8;
typedef unsigned int            hi_u32;
typedef char                    hi_char;
typedef int                     hi_s32;
typedef void                    hi_void;
typedef hi_u32                  hi_handle;
typedef enum {
    HI_FALSE = 0,
    HI_TRUE  = 1
} hi_bool;

typedef enum {
    HI_TEE_ROOTKEY_NULL  = 0x0,
    HI_TEE_ROOTKEY_CSA2  = 0x1,
    HI_TEE_ROOTKEY_CSA3  = 0x2,
    HI_TEE_ROOTKEY_AES   = 0x3,
    HI_TEE_ROOTKEY_TDES  = 0x4,
    HI_TEE_ROOTKEY_SM4   = 0x5,
    HI_TEE_ROOTKEY_MISC  = 0x6,
    HI_TEE_ROOTKEY_R2R   = 0x7,
    HI_TEE_ROOTKEY_HDCP  = 0x8,
    HI_TEE_ROOTKEY_DCAS  = 0x9,
    HI_TEE_ROOTKEY_DYM   = 0xFF,
} hi_tee_rootkey_type;

/*
 * Keyladder type list
 */
typedef enum {
    HI_TEE_KLAD_COM   = 0x10,
    HI_TEE_KLAD_TA    = 0x11,
    HI_TEE_KLAD_FP    = 0x12,
    HI_TEE_KLAD_NONCE = 0x13,
    HI_TEE_KLAD_CLR   = 0x14,
} hi_tee_klad_type;

#define HI_TEE_KLAD_INSTANCE(CA, RK, KLAD, ID) \
    ((((CA) << 24) & 0xFF000000) + (((RK) << 16) & 0xFF0000) + (((KLAD) << 8) & 0xFF00)+ (ID))

#define HI_CA_ID_BASIC               0x80

/*
 * Clear route keyladder
 */
#define HI_TEE_KLAD_TYPE_CLEARCW     HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_NULL, HI_TEE_KLAD_CLR, 0x01)

/*
 * Dynamic keyladder, it can be customized
 */
#define HI_TEE_KLAD_TYPE_DYNAMIC     HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_DYM, HI_TEE_KLAD_COM, 0x01)

/*
 * STBM TA keyladder
 * 1 stage keyladder
 * Keyladder algorithm use AES, target engine is MCipher and target engine algorithm is AES.
 */
#define HI_TEE_KLAD_TYPE_OEM_TA     HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_R2R, HI_TEE_KLAD_COM, 0x01)

/*
 * CSA2 keyladder
 * 2 stage keyladder
 * Keyladder algorithm use AES/TDES/SM4, target engine is TSCIPHER and target engine algorithm is CSA2.
 */
#define HI_TEE_KLAD_TYPE_CSA2   HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_CSA2, HI_TEE_KLAD_COM, 0x01)

/*
 * CSA3 keyladder
 * 2 stage keyladder
 * Keyladder algorithm use AES/TDES/SM4, target engine is TSCIPHER and target engine algorithm is CSA3.
 */
#define HI_TEE_KLAD_TYPE_CSA3   HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_CSA3, HI_TEE_KLAD_COM, 0x01)

/*
 * R2R keyladder
 * 2 stage keyladder
 * Keyladder algorithm use AES/TDES/SM4, target engine is MCipher and target engine algorithm is AES/TDES/SM4.
 */
#define HI_TEE_KLAD_TYPE_R2R   HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_R2R, HI_TEE_KLAD_COM, 0x01)

/*
 * SP keyladder
 * 2 stage keyladder
 * Keyladder algorithm use AES/TDES/SM4, target engine is TSCIPHER and target engine algorithm is AES.
 */
#define HI_TEE_KLAD_TYPE_SP   HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_AES, HI_TEE_KLAD_COM, 0x01)

/*
 * MISC keyladder
 * 2 stage keyladder
 * Keyladder algorithm use AES/TDES/SM4, target engine is TSCIPHER and target engine algorithm is CSA2/CSA3/AES/TDES.
 */
#define HI_TEE_KLAD_TYPE_MISC   HI_TEE_KLAD_INSTANCE(HI_CA_ID_BASIC, HI_TEE_ROOTKEY_MISC, HI_TEE_KLAD_COM, 0x01)

/* Define the maximum session key level */
#define HI_TEE_SESSION_KEY_MAX_LEVEL 0x04

/* Define the maximum key length. */
#define HI_TEE_KLAD_MAX_KEY_LEN      32

/* Define the algorithm of crypto engine. */
typedef enum {
    HI_TEE_CRYPTO_ALG_CSA2 = 0,          /**<CSA2.0*/
    HI_TEE_CRYPTO_ALG_CSA3,              /**<CSA3.0*/
    HI_TEE_CRYPTO_ALG_ASA,               /**<ASA 64/128 Algorithm*/
    HI_TEE_CRYPTO_ALG_ASA_LIGHT,         /**<ASA light Algorithm*/

    HI_TEE_CRYPTO_ALG_AES_ECB_T = 0x10,  /**<SPE AES ECB, the clear stream left in the tail*/
    HI_TEE_CRYPTO_ALG_AES_ECB_L,         /**<AES_ECB_L the clear stream left in the leading*/

    HI_TEE_CRYPTO_ALG_AES_CBC_T,         /**<AES CBC, the clear stream left in the tail*/
    HI_TEE_CRYPTO_ALG_AES_CISSA,         /**<Common IPTV Software-oriented Scrambling Algorithm(CISSA), golbal IV*/
    HI_TEE_CRYPTO_ALG_AES_CBC_L,         /**<AES_CBC_L the clear stream left in the leading*/

    HI_TEE_CRYPTO_ALG_AES_CBC_IDSA,      /**<AES128 CBC Payload, ATIS IIF Default Scrambling Algorithm (IDSA), the difference between AES_CBC_IDSA and AES_IPTV is AES_CBC_IDSA only support 0 IV*/
    HI_TEE_CRYPTO_ALG_AES_IPTV,          /**<AES IPTV of SPE*/
    HI_TEE_CRYPTO_ALG_AES_CTR,           /**<AES CTR*/

    HI_TEE_CRYPTO_ALG_DES_CI = 0x20,     /**<DES CBC*/
    HI_TEE_CRYPTO_ALG_DES_CBC,           /**<DES CBC*/
    HI_TEE_CRYPTO_ALG_DES_CBC_IDSA,      /**<DES CBC Payload, ATIS IIF Default Scrambling Algorithm(IDSA), Not support set IV*/

    HI_TEE_CRYPTO_ALG_SMS4_ECB = 0x30,   /**<SMS4 ECB*/
    HI_TEE_CRYPTO_ALG_SMS4_CBC,          /**<SMS4 CBC*/
    HI_TEE_CRYPTO_ALG_SMS4_CBC_IDSA,     /**<SMS4 CBC Payload, ATIS IIF Default Scrambling Algorithm(IDSA), Not support set IV*/

    HI_TEE_CRYPTO_ALG_TDES_ECB = 0x40,   /**<TDES ECB*/
    HI_TEE_CRYPTO_ALG_TDES_CBC,          /**<TDES CBC*/
    HI_TEE_CRYPTO_ALG_TDES_CBC_IDSA,     /**<TDES CBC Payload, ATIS IIF Default Scrambling Algorithm(IDSA), Not support set IV*/

    HI_TEE_CRYPTO_ALG_MULTI2_ECB = 0x50, /**<MULTI2 ECB*/
    HI_TEE_CRYPTO_ALG_MULTI2_CBC,        /**<MULTI2 CBC*/
    HI_TEE_CRYPTO_ALG_MULTI2_CBC_IDSA,   /**<MULTI2 CBC Payload, ATIS IIF Default Scrambling Algorithm(IDSA), Not support set IV*/

    HI_TEE_CRYPTO_ALG_RAW_AES = 0x4000,
    HI_TEE_CRYPTO_ALG_RAW_DES,
    HI_TEE_CRYPTO_ALG_RAW_SM4,
    HI_TEE_CRYPTO_ALG_RAW_TDES,
    HI_TEE_CRYPTO_ALG_RAW_HMAC_SHA1,
    HI_TEE_CRYPTO_ALG_RAW_HMAC_SHA2,
    HI_TEE_CRYPTO_ALG_RAW_HMAC_SM3,
    HI_TEE_CRYPTO_ALG_RAW_HDCP,

    HI_TEE_CRYPTO_ALG_MAX
} hi_tee_crypto_alg;

/* Define the key security attribute. */
typedef enum {
    HI_TEE_KLAD_SEC_ENABLE = 0,
    HI_TEE_KLAD_SEC_DISABLE,
    HI_TEE_KLAD_SEC_MAX
} hi_tee_klad_sec;

/* Define the keyladder algorithm. */
typedef enum {
    HI_TEE_KLAD_ALG_TYPE_DEFAULT   = 0, /* Default value */
    HI_TEE_KLAD_ALG_TYPE_TDES      = 1,
    HI_TEE_KLAD_ALG_TYPE_AES,
    HI_TEE_KLAD_ALG_TYPE_SM4,
    HI_TEE_KLAD_ALG_TYPE_MAX
} hi_tee_klad_alg_type;

/* Define the keyladder level. */
typedef enum {
    HI_TEE_KLAD_LEVEL1 = 0,
    HI_TEE_KLAD_LEVEL2,
    HI_TEE_KLAD_LEVEL3,
    HI_TEE_KLAD_LEVEL4,
    HI_TEE_KLAD_LEVEL5,
    HI_TEE_KLAD_LEVEL6,
    HI_TEE_KLAD_LEVEL_MAX
} hi_tee_klad_level;

/* Define the structure of keyladder configuration. */
typedef struct {
    hi_u32 owner_id;          /* Keyladder owner ID. Different keyladder have different ID. */
    hi_u32 klad_type;         /* Keyladder type. */
} hi_tee_klad_config;

/* Define the structure of content key configurations. */
typedef struct {
    hi_bool decrypt_support;         /* The content key can be used for decrypting. */
    hi_bool encrypt_support;         /* The content key can be used for encrypting. */
    hi_tee_crypto_alg engine;        /* The content key can be used for which algorithm of the crypto engine. */
} hi_tee_klad_key_config;

/* Define the structure of content key security configurations. */
typedef struct {
    hi_tee_klad_sec key_sec;
    hi_bool dest_buf_sec_support;     /* The destination buffer of target engine can be secure. */
    hi_bool dest_buf_non_sec_support; /* The destination buffer of target engine can be non-secure. */
    hi_bool src_buf_sec_support;      /* The source buffer of target engine can be secure. */
    hi_bool src_buf_non_sec_support;  /* The source buffer of target engine can be non-secure. */
} hi_tee_klad_key_secure_config;

/* Structure of keyladder extend attributes. */
typedef struct {
    hi_tee_klad_config klad_cfg;               /* The keyladder configuration. */
    hi_tee_klad_key_config key_cfg;            /* The content key configuration. */
    hi_tee_klad_key_secure_config key_sec_cfg; /* The content key security configuration. */
} hi_tee_klad_attr;

/* Structure of setting session key. */
typedef struct {
    hi_tee_klad_level level;            /* The level of session key. */
    hi_tee_klad_alg_type alg;           /* The algorithm used to decrypt session key. */
    hi_u32 key_size;                    /* The size of session key. */
    hi_u8 key[HI_TEE_KLAD_MAX_KEY_LEN]; /* The session key. */
} hi_tee_klad_session_key;

/* Structure of setting content key. */
typedef struct {
    hi_bool odd;                        /* Odd or Even key flag. */
    hi_tee_klad_alg_type alg;           /* The algorithm of the content key. */
    hi_u32 key_size;                    /* The size of content key. */
    hi_u8 key[HI_TEE_KLAD_MAX_KEY_LEN]; /* The content key. */
} hi_tee_klad_content_key;

/* Structure of sending clear key. */
typedef struct {
    hi_bool odd;                        /* Odd or Even key flag. */
    hi_u32 key_size;                    /* The size of content key. */
    hi_u8 key[HI_TEE_KLAD_MAX_KEY_LEN]; /* The content key. */
} hi_tee_klad_clear_key;

/* Structure of generating keyladder key. */
typedef struct {
    hi_tee_klad_alg_type alg;               /* The algorithm of the content key. */
    hi_u32 key_size;                        /* The size of content key. */
    hi_u8 key[HI_TEE_KLAD_MAX_KEY_LEN];
    hi_u32 gen_key_size;                    /* The size of generated key. */
    hi_u8 gen_key[HI_TEE_KLAD_MAX_KEY_LEN];
} hi_tee_klad_gen_key;

/* Structure of setting Nonce keyladder key. */
typedef struct {
    hi_tee_klad_alg_type alg;             /* The algorithm of the content key. */
    hi_u32 key_size;                      /* The size of content key. */
    hi_u8 key[HI_TEE_KLAD_MAX_KEY_LEN];
    hi_u32 nonce_size;                    /* The size of nonce key. */
    hi_u8 nonce[HI_TEE_KLAD_MAX_KEY_LEN]; /* The size of nonce key. */
} hi_tee_klad_nonce_key;

/* Rootkey slot. */
typedef enum {
    HI_TEE_BOOT_ROOTKEY_SLOT   = 0x0,
    HI_TEE_HISI_ROOTKEY_SLOT   = 0x1,
    HI_TEE_OEM_ROOTKEY_SLOT    = 0x2,
    HI_TEE_CAS_ROOTKEY_SLOT0   = 0x10,
    HI_TEE_CAS_ROOTKEY_SLOT1   = 0x11,
    HI_TEE_CAS_ROOTKEY_SLOT2   = 0x12,
    HI_TEE_CAS_ROOTKEY_SLOT3   = 0x13,
    HI_TEE_CAS_ROOTKEY_SLOT4   = 0x14,
    HI_TEE_CAS_ROOTKEY_SLOT5   = 0x15,
    HI_TEE_CAS_ROOTKEY_SLOT6   = 0x16,
    HI_TEE_CAS_ROOTKEY_SLOT7   = 0x17,
    HI_TEE_ROOTKEY_SLOT_MAX
} hi_tee_rootkey_select;

/* Configure crypto engine type. */
typedef struct {
    hi_bool mcipher_support;  /* Support send key to Mcipher or not. */
    hi_bool tscipher_support; /* Support send key to TScipher(TSR2RCipher and Demux) or not. */
} hi_tee_rootkey_target;

/* Configure crypto engine algorithm. */
typedef struct {
    hi_bool sm4_support;      /* Target engine support SM4 algorithm or not. */
    hi_bool tdes_support;     /* Target engine support TDES algorithm or not. */
    hi_bool aes_support;      /* Target engine support AES algorithm or not. */

    hi_bool csa2_support;     /* Target engine support CSA2 algorithm or not. */
    hi_bool csa3_support;     /* Target engine support CSA3 algorithm or not. */
    hi_bool hmac_sha_support; /* Target engine support HMAC SHA or not. */
    hi_bool hmac_sm3_support; /* Target engine support HMAC SM3 or not. */
} hi_tee_rootkey_target_alg;

/* Configure target engine features. */
typedef struct {
    hi_bool encrypt_support;  /* Target engine support encrypt or not. */
    hi_bool decrypt_support;  /* Target engine support decrypt or not. */
} hi_tee_rootkey_target_feature;

/* Configure keyladder algorithm. */
typedef struct {
    hi_bool sm4_support;      /* Keyladder support SM4 algorithm or not. */
    hi_bool tdes_support;     /* Keyladder support TDES algorithm or not. */
    hi_bool aes_support;      /* Keyladder support AES algorithm or not. */
} hi_tee_rootkey_alg;

/* Configure keyladder stage. */
typedef enum {
    HI_TEE_ROOTKEY_LEVEL1 = 0, /* Keyladder support 1 stage. */
    HI_TEE_ROOTKEY_LEVEL2,     /* Keyladder support 2 stage. */
    HI_TEE_ROOTKEY_LEVEL3,     /* Keyladder support 3 stage. */
    HI_TEE_ROOTKEY_LEVEL4,     /* Keyladder support 4 stage. */
    HI_TEE_ROOTKEY_LEVEL5,     /* Keyladder support 5 stage. */
    HI_TEE_ROOTKEY_LEVEL_MAX
} hi_tee_rootkey_level;

/* Structure of Rootkey attributes. */
typedef struct {
    hi_tee_rootkey_select  rootkey_sel; /* Rootkey slot select. */
    hi_tee_rootkey_target target_support;                 /* Crypto engine select. */
    hi_tee_rootkey_target_alg target_alg_support;         /* Crypto engine algorithm. */
    hi_tee_rootkey_target_feature target_feature_support;
    hi_tee_rootkey_level level;                           /* Keyladder stage. */
    hi_tee_rootkey_alg alg_support;                       /* Keyladder algorithm. */
} hi_tee_rootkey_attr;

/*
\brief Declare keyladder callback function interface
\param[in] err_code     Return error code.
\param[in] args         Receive buffer.
\param[in] size         The length of cArgs.
\param[in] user_data    User private data.
\param[in] user_data_len    User private data length.
*/
typedef hi_s32(*hi_tee_klad_func)(hi_s32 err_code, hi_char *args, hi_u32 size,
                                  hi_void *user_data, hi_u32 user_data_len);

/* Define cb descriptor */
typedef struct {
    hi_tee_klad_func done_callback; /* Keyladder callback function interface */
    hi_void *user_data;         /*  user private data */
    hi_u32  user_data_len;      /*  user private data length */
} hi_tee_klad_done_callback;

#define CMD_KLAD_INIT                       0x10
#define CMD_KLAD_DEINIT                     0x11
#define CMD_KLAD_CREATE                     0x1
#define CMD_KLAD_DESTROY                    0x2
#define CMD_KLAD_ATTACH                     0x3
#define CMD_KLAD_DETACH                     0x4
#define CMD_KLAD_GET_ATTR                   0x5
#define CMD_KLAD_SET_ATTR                   0x6
#define CMD_RK_GET_ATTR                     0x25
#define CMD_RK_SET_ATTR                     0x26
#define CMD_KLAD_SET_SESSION_KEY            0x7
#define CMD_KLAD_SET_CONTENT_KEY            0x9
#define CMD_KLAD_ASYNC_SET_CONTENT_KEY      0x19
#define CMD_KLAD_SET_CLEAR_KEY              0xa
#define CMD_KLAD_GET_NONCE_KEY              0xb
#define CMD_KLAD_FP_KEY                     0xc
#define CMD_KLAD_GENERATE_KEY               0xd
#define CMD_API_LOG_LEVEL                   0xFe
#define CMD_KLAD_LOG_LEVEL                  0xFF

static int g_session_open = -1;
static TEEC_Context g_teec_context = {0};
static TEEC_Session g_teec_session = {0};
static TEEC_UUID g_teec_uuid = { 0xc9cf6b2a, 0x4b60, 0x11e7, { 0xa9, 0x19, 0x92, 0xeb, 0xcb, 0x67, 0xfe, 0x33 } } ;

int tee_klad_ta_init()
{
    TEEC_Result teec_rst;
    TEEC_Operation sess_op = {0};
    uint32_t origin = 0;

    if (g_session_open > 0) {
        g_session_open++;
        return 0;
    }
    HI_ERR_DFT("===================init=======================\n");

    teec_rst = TEEC_InitializeContext(NULL, &g_teec_context);
    if (teec_rst != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec Initialize context failed!\n");
        return teec_rst;
    }
    sess_op.started = 1;
    sess_op.paramTypes = TEEC_PARAM_TYPES(
                             TEEC_NONE,
                             TEEC_NONE,
                             TEEC_MEMREF_TEMP_INPUT,
                             TEEC_MEMREF_TEMP_INPUT);
    teec_rst = TEEC_OpenSession(&g_teec_context,
                                &g_teec_session,
                                &g_teec_uuid,
                                TEEC_LOGIN_IDENTIFY,
                                NULL,
                                &sess_op,
                                &origin);
    if (teec_rst != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec open session failed!\n");
        (void)TEEC_FinalizeContext(&g_teec_context);
        return (int)teec_rst;
    }
    g_session_open = 1;
    return 0;
}

int tee_klad_ta_deinit()
{
    if (g_session_open > 0) {
        g_session_open--;
    }

    if (g_session_open != 0) {
        return 0;
    }
    (void)TEEC_CloseSession(&g_teec_session);
    (void)TEEC_FinalizeContext(&g_teec_context);

    g_session_open = -1;

    return 0;
}

int tee_klad_init()
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};
    uint32_t origin = 0;

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_INIT, &teec_operation, &origin);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_create(hi_handle *klad)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_MEMREF_TEMP_OUTPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].tmpref.buffer = (unsigned int *)klad;
    teec_operation.params[0].tmpref.size = 0x04;
    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_CREATE, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_attach(hi_handle klad, hi_handle target)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[0].value.b = (unsigned int)target;

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_ATTACH, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_detach(hi_handle klad, hi_handle target)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[0].value.b = (unsigned int)target;


    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_DETACH, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_set_rootkey_attr(hi_handle klad, hi_tee_rootkey_attr *attr)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};
    hi_tee_rootkey_attr tee_rk_attr;

    memcpy(&tee_rk_attr, attr, sizeof(hi_tee_rootkey_attr));
    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)(&tee_rk_attr);
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_rootkey_attr);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_RK_SET_ATTR, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_set_attr(hi_handle klad, hi_tee_klad_attr *attr)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};
    hi_tee_klad_attr tee_attr;

    memcpy(&tee_attr, attr, sizeof(hi_tee_klad_attr));
    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)(&tee_attr);
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_klad_attr);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_SET_ATTR, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}


int tee_klad_get_attr(hi_handle klad, hi_tee_klad_attr *attr)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};
    hi_tee_klad_attr tee_attr = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_OUTPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)&tee_attr;
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_klad_attr);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_GET_ATTR, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    memcpy(attr, &tee_attr, sizeof(hi_tee_klad_attr));
    return 0;
}


int tee_klad_set_session_key(hi_handle klad, hi_tee_klad_session_key *key)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)key;
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_klad_session_key);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_SET_SESSION_KEY, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}


int tee_klad_set_content_key(hi_handle klad, hi_tee_klad_content_key *key)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)key;
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_klad_content_key);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_SET_CONTENT_KEY, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_set_async_content_key(hi_handle klad, hi_tee_klad_content_key *key)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)key;
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_klad_content_key);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_ASYNC_SET_CONTENT_KEY, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}


int tee_klad_set_clear_key(hi_handle klad, hi_tee_klad_clear_key *key)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;
    teec_operation.params[1].tmpref.buffer = (void *)key;
    teec_operation.params[1].tmpref.size = sizeof(hi_tee_klad_clear_key);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_SET_CLEAR_KEY, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

int tee_klad_destroy(hi_handle klad)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)klad;

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_DESTROY, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }

    return 0;
}

int tee_klad_log_level(hi_u32 level)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)level;

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_LOG_LEVEL, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }

    return 0;
}

int tee_api_log_level(hi_u32 level)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)level;

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_API_LOG_LEVEL, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }

    return 0;
}

int tee_klad_deinit()
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);

    teec_result = TEEC_InvokeCommand(&g_teec_session, CMD_KLAD_DEINIT, &teec_operation, NULL);
    if (teec_result != 0) {
        HI_ERR_DFT("teec invoke command failed, cmd = 0x%08x\n", teec_result);
        return (int)teec_result;
    }
    return 0;
}

/*
* *********************************KEYSLOT****************************************
*/
typedef enum {
    HI_KEYSLOT_TYPE_TSCIPHER = 0x00,
    HI_KEYSLOT_TYPE_MCIPHER,
    HI_KEYSLOT_TYPE_HMAC,
    HI_KEYSLOT_TYPE_MAX
} hi_keyslot_type;

#define KEYSLOT_CMD_CREATE         0
#define KEYSLOT_CMD_DESTROY        1
#define KEYSLOT_CMD_LOG_LEVEL      0xff

static int g_session_ks_open = -1;
static TEEC_Context g_teec_ks_context = {0};
static TEEC_Session g_teec_ks_session = {0};
TEEC_UUID g_teec_ks_uuid = { 0x59e80d08, 0xad42, 0x11e9, { 0xa2, 0xa3, 0x2a, 0x2a, 0xe2, 0xdb, 0xcc, 0xe4 } };

int tee_ks_ta_init()
{
    TEEC_Result teec_rst;
    TEEC_Operation sess_op = {0};
    uint32_t origin = 0;

    if (g_session_ks_open > 0) {
        g_session_ks_open++;
        return 0;
    }
    HI_ERR_DFT("===================init=======================\n");

    teec_rst = TEEC_InitializeContext(NULL, &g_teec_ks_context);
    if (teec_rst != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec Initialize context failed!\n");
        return teec_rst;
    }
    sess_op.started = 1;
    sess_op.paramTypes = TEEC_PARAM_TYPES(
                             TEEC_NONE,
                             TEEC_NONE,
                             TEEC_MEMREF_TEMP_INPUT,
                             TEEC_MEMREF_TEMP_INPUT);
    teec_rst = TEEC_OpenSession(&g_teec_ks_context,
                                &g_teec_ks_session,
                                &g_teec_ks_uuid,
                                TEEC_LOGIN_IDENTIFY,
                                NULL,
                                &sess_op,
                                &origin);
    if (teec_rst != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec open session failed!\n");
        (void)TEEC_FinalizeContext(&g_teec_ks_context);
        return (int)teec_rst;
    }
    g_session_ks_open = 1;
    return 0;
}

int tee_ks_ta_deinit()
{
    if (g_session_ks_open > 0) {
        g_session_ks_open--;
    }

    if (g_session_ks_open != 0) {
        return 0;
    }
    (void)TEEC_CloseSession(&g_teec_ks_session);
    (void)TEEC_FinalizeContext(&g_teec_ks_context);

    g_session_ks_open = -1;

    return 0;
}

int tee_sec_key_slot_create(hi_keyslot_type keyslot_type, unsigned int *handle)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INOUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = (unsigned int)keyslot_type;
    teec_result = TEEC_InvokeCommand(&g_teec_ks_session, KEYSLOT_CMD_CREATE, &teec_operation, NULL);
    if (teec_result != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec invoke command failed, cmd = 0x%08x\n", KEYSLOT_CMD_CREATE);
    }
    *handle = teec_operation.params[0].value.b;
    return (int)teec_result;
}

int tee_sec_key_slot_destroy(unsigned int handle)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = handle;
    teec_result = TEEC_InvokeCommand(&g_teec_ks_session, KEYSLOT_CMD_DESTROY, &teec_operation, NULL);
    if (teec_result != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec invoke command failed, cmd = 0x%08x\n", KEYSLOT_CMD_DESTROY);
    }
    return (int)teec_result;
}


int tee_sec_key_slot_log_level(unsigned int level)
{
    TEEC_Result teec_result;
    TEEC_Operation teec_operation = {0};

    teec_operation.started = 1;
    teec_operation.paramTypes = TEEC_PARAM_TYPES(
                                    TEEC_VALUE_INPUT,
                                    TEEC_NONE,
                                    TEEC_NONE,
                                    TEEC_NONE);
    teec_operation.params[0].value.a = level;
    teec_result = TEEC_InvokeCommand(&g_teec_ks_session, KEYSLOT_CMD_LOG_LEVEL, &teec_operation, NULL);
    if (teec_result != TEEC_SUCCESS) {
        HI_ERR_DFT("Teec invoke command failed, cmd = 0x%08x\n", KEYSLOT_CMD_LOG_LEVEL);
    }
    return (int)teec_result;
}
/*
**********************************TEST****************************************
*/
hi_u8 test_key[16] = {
    0x67, 0x47, 0x9C, 0x36, 0x6F, 0x2C, 0x19, 0xC7,
    0x2D, 0x3A, 0x12, 0xB6, 0x75, 0x0F, 0x26, 0x98
};

int main(int argc, char *argv[])
{
    hi_s32 ret;
    hi_handle handle_ks = 0;
    hi_handle handle_klad = 0;
    hi_tee_klad_attr attr_klad = {0};
    hi_tee_klad_clear_key key_clear = {0};
    hi_char input_cmd[0x20] = {0};

    ret = tee_klad_ta_init();
    if (ret != 0) {
        klad_print_error_func(tee_klad_ta_init, ret);
        return ret;
    }
    ret = tee_ks_ta_init();
    if (ret != 0) {
        klad_print_error_func(tee_ks_ta_init, ret);
        goto out0;
    }
    sleep(1);
    tee_sec_key_slot_log_level(1);
    tee_klad_log_level(3);
    tee_api_log_level(3);
    ret = tee_klad_init();
    if (ret != 0) {
        klad_print_error_func(tee_klad_init, ret);
        goto out1;
    }
    ret = tee_sec_key_slot_create(HI_KEYSLOT_TYPE_MCIPHER, &handle_ks);
    if (ret != 0) {
        klad_print_error_func(tee_sec_key_slot_create, ret);
        goto out2;
    }
    ret = tee_klad_create(&handle_klad);
    if (ret != 0) {
        klad_print_error_func(tee_klad_create, ret);
        goto out3;
    }
    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_TEE_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = HI_TEE_CRYPTO_ALG_RAW_AES;
    attr_klad.key_sec_cfg.key_sec = HI_TRUE;
    attr_klad.key_sec_cfg.dest_buf_non_sec_support = 1;
    attr_klad.key_sec_cfg.dest_buf_sec_support = 1;
    attr_klad.key_sec_cfg.src_buf_non_sec_support = 1;
    attr_klad.key_sec_cfg.src_buf_sec_support = 1;
    ret = tee_klad_set_attr(handle_klad, &attr_klad);
    if (ret != 0) {
        klad_print_error_func(tee_klad_set_attr, ret);
        goto out4;
    }
    ret = tee_klad_attach(handle_klad, handle_ks);
    if (ret != 0) {
        klad_print_error_func(tee_klad_attach, ret);
        goto out4;
    }
    key_clear.odd = 0;
    key_clear.key_size = 16;
    memcpy(key_clear.key, test_key, 16);
    ret = tee_klad_set_clear_key(handle_klad, &key_clear);
    if (ret != 0) {
        klad_print_error_func(tee_klad_set_clear_key, ret);
        goto out5;
    }

    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

out5:
    tee_klad_detach(handle_klad, handle_ks);
out4:
    tee_klad_destroy(handle_klad);
out3:
    tee_sec_key_slot_destroy(handle_ks);
out2:
    tee_klad_deinit();
out1:
    sleep(1);
    tee_ks_ta_deinit();
out0:
    tee_klad_ta_deinit();
    return 0;
}

