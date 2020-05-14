/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: tee keyslot test.
 * Author: linux SDK team
 * Create: 2019-07-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hi_unf_keyslot.h"

#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)

#define HI_ERR_DFT(format, arg...)     printf( "%s,%d: " format , __FUNCTION__, __LINE__, ## arg)
#define HI_INFO_DFT(format, arg...)    printf( "%s,%d: " format , __FUNCTION__, __LINE__, ## arg)

int main(int argc, char *argv[])
{
    unsigned int handle[0x4] = {0};
    hi_unf_keyslot_attr attr = {0};
    int i;
    int ret;
    hi_char input_cmd[0x20] = {0};

    hi_unf_keyslot_init();
    sleep(1);
    for (i = 0; i < 0x4; i ++) {
        handle[i] = 0;
        attr.type = HI_UNF_KEYSLOT_TYPE_MCIPHER;
        attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
        ret = hi_unf_keyslot_create(&attr, &handle[i]);
        HI_ERR_DFT("REE============>  0x%08x= 0x%08x\n", ret, handle[i]);
        sleep(1);
    }
    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }

    for (i = 0; i < 0x4; i ++) {
        ret = hi_unf_keyslot_destroy(handle[i]);
        HI_ERR_DFT("************>  0x%08x= 0x%08x\n", ret, handle[i]);
        sleep(1);
    }

    for (i = 0; i < 0x4; i ++) {
        handle[i] = 0;
        attr.type = HI_UNF_KEYSLOT_TYPE_MCIPHER;
        attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_TEE;
        ret = hi_unf_keyslot_create(&attr, &handle[i]);
        HI_ERR_DFT("TEE============>  0x%08x= 0x%08x\n", ret, handle[i]);
        sleep(1);
    }
    printf("please input 'q' to quit!\n");
    while (1) {
        SAMPLE_GET_INPUTCMD(input_cmd);
        if ('q' == input_cmd[0]) {
            printf("prepare to quit!\n");
            break;
        }
    }
    for (i = 0; i < 0x4; i ++) {
        ret = hi_unf_keyslot_destroy(handle[i]);
        HI_ERR_DFT("************>  0x%08x= 0x%08x\n", ret, handle[i]);
        sleep(1);
    }
    hi_unf_keyslot_deinit();
    return 0;
}

