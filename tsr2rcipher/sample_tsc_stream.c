/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: tsr2rcipher encrypt test.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_tsr2rcipher.h"
#include "hi_unf_keyslot.h"
#include "hi_unf_klad.h"

#define SAMPLE_GET_INPUTCMD(input_cmd)     fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define TSC_MARK()                         printf("[%-4s][line:%04d]mark\n", __FUNCTION__, __LINE__)
#define TSC_ERR_PRINT_HEX(val)             printf("[%-4s][line:%04d]%s = 0x%08x\n", __FUNCTION__, __LINE__, #val, val)
#define TSC_ERR_PRINT_INFO(val)            printf("[%-4s][line:%04d]%s\n", __FUNCTION__, __LINE__, val)
#define TSC_ERR_PRINT_VAL(val)             printf("[%-4s][line:%04d]%s = %d\n", __FUNCTION__, __LINE__, #val, val)
#define TSC_ERR_PRINT_POINT(val)           printf("[%-4s][line:%04d]%s = %p\n", __FUNCTION__, __LINE__, #val, val)
#define TSC_PRINT_ERR_CODE(err_code)       printf("[%-4s][line:%04d]return [0x%08x]\n", __FUNCTION__, \
                                                   __LINE__, err_code)
#define TSC_PRINT_ERR_FUNC(func, err_code) printf("[%-4s][line:%04d]call [%s] return [0x%08x]\n", __FUNCTION__, \
                                                   __LINE__, #func, err_code)

#define DATA_LEN (188 * 10000)

hi_bool g_encrypt;
FILE *g_src_file = HI_NULL;
FILE *g_dst_file = HI_NULL;
FILE *g_config_file = HI_NULL;
hi_unf_tsr2rcipher_alg g_alg;
hi_unf_tsr2rcipher_mode g_mode;
hi_bool g_is_odd_key;
hi_unf_tsr2rcipher_iv_type g_iv_type;
hi_u8 g_key[16] = {0};
hi_u8 g_iv[16] = {0};

hi_s32 tsc_work(hi_void)
{
    hi_s32 ret;
    hi_handle handle_ks = 0;
    hi_handle handle_klad = 0;
    hi_handle handle_tsc = 0;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    hi_unf_tsr2rcipher_attr tsc_attr;
    hi_unf_keyslot_attr keyslot_attr;
    hi_unf_tsr2rcipher_mem_handle src_mem_handle = {0};
    hi_unf_tsr2rcipher_mem_handle dst_mem_handle = {0};
    hi_u8 *src_virt = HI_NULL;
    hi_u8 *dst_virt = HI_NULL;
    hi_u32 read_len = 0;

    ret = hi_unf_sys_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_sys_init, ret);
        goto out;
    }

    ret = hi_unf_tsr2rcipher_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_init, ret);
        goto SYS_DEINIT;
    }

    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_keyslot_init, ret);
        goto TSC_DEINIT;
    }

    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_init, ret);
        goto KS_DEINIT;
    }

    ret = hi_unf_tsr2rcipher_get_default_attr(&tsc_attr);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_get_default_attr, ret);
        goto KLAD_DEINIT;
    }

    tsc_attr.alg = g_alg;
    tsc_attr.mode = g_mode;
    tsc_attr.is_create_keyslot = HI_FALSE;
    ret = hi_unf_tsr2rcipher_create(&tsc_attr, &handle_tsc);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_create, ret);
        goto KLAD_DEINIT;
    }

    keyslot_attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    keyslot_attr.type = HI_UNF_KEYSLOT_TYPE_TSCIPHER;
    ret = hi_unf_keyslot_create(&keyslot_attr, &handle_ks);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_keyslot_create, ret);
        goto TSC_DESTROY;
    }

    ret = hi_unf_klad_create(&handle_klad);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_create, ret);
        goto KS_DESTORY;
    }

    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = HI_UNF_KLAD_TYPE_CLEARCW;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = (hi_unf_crypto_alg)g_alg;
    ret = hi_unf_klad_set_attr(handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_set_attr, ret);
        goto KLAD_DESTORY;
    }

    ret = hi_unf_klad_attach(handle_klad, handle_ks);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_attach, ret);
        goto KLAD_DESTORY;
    }

    ret = hi_unf_tsr2rcipher_attach_keyslot(handle_tsc, handle_ks);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_attach_keyslot, ret);
        goto KLAD_DETACH;
    }

    memset(&key_clear, 0, sizeof(hi_unf_klad_clear_key));
    key_clear.odd = g_is_odd_key;
    key_clear.key_size = 16;
    memcpy(key_clear.key, g_key, 16);
    ret = hi_unf_klad_set_clear_key(handle_klad, &key_clear);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_klad_set_clear_key, ret);
        goto TSC_DETACH;
    }

    ret = hi_unf_tsr2rcipher_get_attr(handle_tsc, &tsc_attr);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_get_attr, ret);
        goto TSC_DETACH;
    }

    tsc_attr.is_odd_key = g_is_odd_key;
    ret = hi_unf_tsr2rcipher_set_attr(handle_tsc, &tsc_attr);
    if (ret != HI_SUCCESS) {
        TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_set_attr, ret);
        goto TSC_DETACH;
    }

    if (g_alg == HI_UNF_TSR2RCIPHER_ALG_AES_CBC || g_alg == HI_UNF_TSR2RCIPHER_ALG_AES_CTR ||
        g_alg == HI_UNF_TSR2RCIPHER_ALG_SMS4_CBC) {
        ret = hi_unf_tsr2rcipher_set_iv(handle_tsc, g_iv_type, g_iv, 16);
        if (ret != HI_SUCCESS) {
            TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_set_iv, ret);
            goto TSC_DETACH;
        }
    }

    src_mem_handle.mem_handle = hi_unf_mem_new("src_buf", strlen("src_buf") + 1, DATA_LEN, HI_FALSE);
    if (src_mem_handle.mem_handle < 0) {
        TSC_PRINT_ERR_FUNC(hi_mpi_mmz_new, HI_FAILURE);
        goto TSC_DETACH;
    }
    src_mem_handle.addr_offset = 0;
    printf("src_mem_handle[%lld]!\n", src_mem_handle.mem_handle);

    src_virt = hi_unf_mem_map(src_mem_handle.mem_handle, DATA_LEN);
    if (src_virt == HI_NULL) {
        TSC_PRINT_ERR_FUNC(hi_mpi_mmz_map, HI_FAILURE);
        goto SRC_DEL;
    }

    dst_mem_handle.mem_handle = hi_unf_mem_new("dst_buf", strlen("dst_buf") + 1, DATA_LEN, HI_FALSE);
    if (dst_mem_handle.mem_handle < 0) {
        TSC_PRINT_ERR_FUNC(hi_mpi_mmz_new, HI_FAILURE);
        goto SRC_UNMAP;
    }
    dst_mem_handle.addr_offset = 0;
    printf("dst_mem_handle[%lld]!\n", dst_mem_handle.mem_handle);

    dst_virt = hi_unf_mem_map(dst_mem_handle.mem_handle, DATA_LEN);
    if (dst_virt == HI_NULL) {
        TSC_PRINT_ERR_FUNC(hi_unf_mem_map, HI_FAILURE);
        goto DST_DEL;
    }

    while (1) {
        memset(src_virt, 0, DATA_LEN);
        memset(dst_virt, 0, DATA_LEN);
        read_len = fread(src_virt, sizeof(hi_u8), DATA_LEN, g_src_file);
        if (read_len <= 0) {
            break;
        }

        if (g_encrypt == HI_TRUE) {
            ret = hi_unf_tsr2rcipher_encrypt(handle_tsc, src_mem_handle, dst_mem_handle, DATA_LEN);
        } else {
            ret = hi_unf_tsr2rcipher_decrypt(handle_tsc, src_mem_handle, dst_mem_handle, DATA_LEN);
        }

        if (ret != HI_SUCCESS) {
            TSC_PRINT_ERR_FUNC(hi_unf_tsr2rcipher_encrypt, ret);
            goto DST_UNMAP;
        }

        fwrite(dst_virt, 1, DATA_LEN, g_dst_file);
    }

    ret = HI_SUCCESS;

DST_UNMAP:
    hi_unf_mem_unmap(dst_virt, DATA_LEN);
DST_DEL:
    hi_unf_mem_delete(dst_mem_handle.mem_handle);
SRC_UNMAP:
    hi_unf_mem_unmap(src_virt, DATA_LEN);
SRC_DEL:
    hi_unf_mem_delete(src_mem_handle.mem_handle);
TSC_DETACH:
    hi_unf_tsr2rcipher_detach_keyslot(handle_tsc, handle_ks);
KLAD_DETACH:
    hi_unf_klad_detach(handle_klad, handle_ks);
KLAD_DESTORY:
    hi_unf_klad_destroy(handle_klad);
KS_DESTORY:
    hi_unf_keyslot_destroy(handle_ks);
TSC_DESTROY:
    hi_unf_tsr2rcipher_destroy(handle_tsc);
KLAD_DEINIT:
    hi_unf_klad_deinit();
KS_DEINIT:
    hi_unf_keyslot_deinit();
TSC_DEINIT:
    hi_unf_tsr2rcipher_deinit();
SYS_DEINIT:
    hi_unf_sys_deinit();
out:
    return ret;
}

static hi_u32 to_u32(hi_char c)
{
    hi_u32 res = 0;

    if (c >= '0' && c <= '9') {
        res += c - '0';
    } else if (c >= 'A' && c <= 'F') {
        res += c - 55;
    } else if (c >= 'a' && c <= 'f') {
        res += c - 87;
    }

    return res;
}

hi_s32 parse_config(hi_void)
{
    hi_s32 ret;
    hi_u32 cnt;
    hi_u32 i, j;
    hi_char line[256] = {0};

    g_config_file = fopen("./cw_key_config.ini", "rb");
    if (g_config_file == HI_NULL) {
        printf("open config file failed!\n");
        return HI_FAILURE;
    }

    while (NULL != fgets(line, sizeof(line), g_config_file))
    {
        cnt = strlen(line);
        if (cnt == 0 || line[0] == '#') {
            continue;
        }

        if (strncmp(line, "ALGTYPE", 7) == 0) {
            if (strncmp(line + 8, "AES_ECB", 7) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_AES_ECB;
            } else if (strncmp(line + 8, "AES_CBC", 7) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_AES_CBC;
            } else if (strncmp(line + 8, "AES_IPTV", 8) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_AES_IPTV;
            } else if (strncmp(line + 8, "AES_CTR", 7) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_AES_CTR;
            } else if (strncmp(line + 8, "SMS4_ECB", 8) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_SMS4_ECB;
            } else if (strncmp(line + 8, "SMS4_CBC", 8) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_SMS4_CBC;
            } else if (strncmp(line + 8, "SMS4_IPTV", 9) == 0) {
                g_alg = HI_UNF_TSR2RCIPHER_ALG_SMS4_IPTV;
            } else {
                printf("invalid config(alg)!\n");
                ret = HI_FAILURE;
                goto out;
            }
        } else if (strncmp(line, "MODE", 4) == 0) {
            if (strncmp(line + 5, "PAYLOAD", 7) == 0) {
                g_mode = HI_UNF_TSR2RCIPHER_MODE_PAYLOAD;
            } else if (strncmp(line + 5, "RAW", 3) == 0) {
                g_mode = HI_UNF_TSR2RCIPHER_MODE_RAW;
            } else {
                printf("invalid config(mode)!\n");
                ret = HI_FAILURE;
                goto out;
            }
        } else if (strncmp(line, "KEYTYPE", 7) == 0) {
            if (strncmp(line + 8, "ODD", 3) == 0) {
                g_is_odd_key = HI_TRUE;
            } else if (strncmp(line + 8, "EVEN", 4) == 0) {
                g_is_odd_key = HI_FALSE;
            } else {
                printf("invalid config(key_type)!\n");
                ret = HI_FAILURE;
                goto out;
            }
        } else if (strncmp(line, "IVTYPE", 6) == 0) {
            if (strncmp(line + 7, "ODD", 3) == 0) {
                g_iv_type = HI_UNF_TSR2RCIPHER_IV_ODD;
            } else if (strncmp(line + 7, "EVEN", 4) == 0) {
                g_iv_type = HI_UNF_TSR2RCIPHER_IV_EVEN;
            } else {
                printf("invalid config(iv_type)!\n");
                ret = HI_FAILURE;
                goto out;
            }
        } else if (strncmp(line, "KEYVALUE", 8) == 0 && cnt == 42) {
            for (i = 0, j = 9; i < 16; i++) {
                g_key[i] = to_u32(line[j]) * 16 + to_u32(line[j + 1]);
                j += 2;
            }
        } else if (strncmp(line, "IVVALUE", 7) == 0 && cnt == 41) {
            for (i = 0, j = 8; i < 16; i++) {
                g_iv[i] = to_u32(line[j]) * 16 + to_u32(line[j + 1]);
                j += 2;
            }
        }
    }

    ret = HI_SUCCESS;

out:
    fclose(g_config_file);
    return ret;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;

    if (argc < 4) {
        printf("Usage: %s src_tsfile dst_tsfile encrypt/decrypt\n", argv[0]);
        printf("Example:\n\t%s ./src.ts ./dst.ts encrypt\n", argv[0]);
        return -1;
    }

    g_src_file = fopen(argv[1], "rb");
    if (g_src_file == HI_NULL) {
        printf("open file %s error!\n", argv[1]);
        return -1;
    }

    g_dst_file = fopen(argv[2], "wb");
    if (g_dst_file == HI_NULL) {
        printf("open file %s error!\n", argv[2]);
        ret = HI_FAILURE;
        goto out0;
    }

    if (strcasecmp(argv[3], "encrypt") == 0) {
        g_encrypt = HI_TRUE;
    } else if (strcasecmp(argv[3], "decrypt") == 0) {
        g_encrypt = HI_FALSE;
    } else {
        printf("para must be encrypt or decrypt!\n");
        ret = HI_FAILURE;
        goto out1;
    }

    ret = parse_config();
    if (ret != HI_SUCCESS) {
        printf("parse config failed!\n");
        goto out1;
    }

    ret = tsc_work();
    if (ret != HI_SUCCESS) {
        printf("tsr2rcipher work failed!\n");
    } else {
        printf("encrypt/decrypt success!\n");
    }

out1:
    fclose(g_dst_file);
out0:
    fclose(g_src_file);

    return ret;
}

