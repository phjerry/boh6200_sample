#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/reboot.h>

#include <assert.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include "hi_unf_pmoc.h"
#include "hi_unf_gpio.h"
#include "hi_unf_pdm.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define HI_INFO_PMOC(format, arg ...)
#else
#define HI_INFO_PMOC(format, arg ...) printf( format, ## arg)
#endif

#define PMOC_NOTICE(format...)             printf("> "); printf(format);

#define MODIFY_NUMBER(x) do { \
    hi_u32 num; \
    if (get_input_number(&num) == 0) { \
        (x) = num; \
    } \
} while (0)

#define U32_MAX 0xFFFFFFFF

typedef struct {
    hi_unf_pmoc_wakeup_src source;
    hi_char wakeup_source[20];
} pmoc_wakeup_source_map;

static pmoc_wakeup_source_map g_wakeup_source_map[] = {
    { HI_UNF_PMOC_WAKEUP_IR,            "IR" },
    { HI_UNF_PMOC_WAKEUP_KEYLED,        "KEYLED" },
    { HI_UNF_PMOC_WAKEUP_GPIO,          "GPIO" },
    { HI_UNF_PMOC_WAKEUP_LSADC,         "LSADC" },
    { HI_UNF_PMOC_WAKEUP_UART,          "UART" },
    { HI_UNF_PMOC_WAKEUP_ETH,           "ETH" },
    { HI_UNF_PMOC_WAKEUP_USB,           "USB" },
    { HI_UNF_PMOC_WAKEUP_VGA,           "VGA" },
    { HI_UNF_PMOC_WAKEUP_SCART,         "SCART" },
    { HI_UNF_PMOC_WAKEUP_HDMIRX_PLUGIN, "HDMIRX_PLUGIN" },
    { HI_UNF_PMOC_WAKEUP_HDMIRX_CEC,    "HDMIRX_CEC" },
    { HI_UNF_PMOC_WAKEUP_HDMIRX_PLUGIN, "HDMIRX_PLUGIN" },
    { HI_UNF_PMOC_WAKEUP_TIMEOUT,       "TIMEOUT" },
};

hi_void show_usage_info(hi_void)
{
    PMOC_NOTICE("usage: sample_pmoc [-t trigger] [-s set] [-g get] [[command] arg].\n");
    PMOC_NOTICE("command as follows:\n");
    PMOC_NOTICE("sample_pmoc -t                     -- enter standby.\n");
    PMOC_NOTICE("sample_pmoc -s suspend_param       -- configure wakeup params.\n");
    PMOC_NOTICE("sample_pmoc -s wakeup_type         -- configure wakeup type.\n");
    PMOC_NOTICE("sample_pmoc -s display             -- configure keyled display params.\n");
    PMOC_NOTICE("sample_pmoc -s power_off_gpio      -- configure power off gpio params.\n");
    PMOC_NOTICE("sample_pmoc -s memory_standby_flag -- configure memory standby flag.\n");
    PMOC_NOTICE("sample_pmoc -g memory_standby_flag -- get memory standby flag.\n");
    PMOC_NOTICE("sample_pmoc -g suspend_param       -- get wakeup params.\n");
    PMOC_NOTICE("sample_pmoc -g chip_temperature    -- get chip temperature.\n");

    return;
}

static int get_input_number(hi_u32 *num)
{
    hi_u32 bytes;
    char buffer[128] = {0}; /* console I/O buffer, size: 128 */

    HI_INFO_PMOC(" ? ");
    fflush(stdout);
    read(0, buffer, 127); /* read size: 127 */

    bytes = strlen(buffer);

    if (bytes == 0) {
        return -1;
    } else {
        char *end;
        /* hex */
        if (!strncmp(buffer, "0x", sizeof("0x") - 1)) {
            *num = strtoul(buffer, &end, 16); /* 16 system */
        } else {
            *num = strtoul(buffer, &end, 10); /* 10 system */
        }

        bytes = (hi_u32)(end - buffer);
        if (bytes) {
            return 0;
        } else {
            return -1;
        }
    }
}

hi_s32 tigger_standby(hi_void)
{
    hi_s32 ret;
    hi_u32 runnint_count = 1;
    hi_u32 i, j;
    hi_u32 standby_period;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};
    hi_unf_pmoc_wakeup_src source;
    hi_unf_pmoc_wakeup_attr attr;

    PMOC_NOTICE("===================== enter standby ====================\n");
    PMOC_NOTICE("Please input standby running count[%d]", runnint_count);
    MODIFY_NUMBER(runnint_count);

    if (runnint_count > 1) {
        ret = hi_unf_pmoc_get_suspend_param(HI_UNF_PMOC_WAKEUP_TIMEOUT, &enable, &param);
        if (ret != HI_SUCCESS) {
            HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
            return ret;
        }

        if (!enable) {
            enable = HI_TRUE;

            PMOC_NOTICE("Please input new standby period[%d s]", param.timeout_param.suspend_period);
            scanf("%d", &param.timeout_param.suspend_period);

            ret = hi_unf_pmoc_set_suspend_param(HI_UNF_PMOC_WAKEUP_TIMEOUT, enable, &param);
            if (ret != HI_SUCCESS) {
                HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
                return ret;
            }
        }
    }

    for (i = 0; i < runnint_count; i++) {
        ret = hi_unf_pmoc_enter_standby(&source);
        if (ret != HI_SUCCESS) {
            HI_INFO_PMOC("hi_unf_pmoc_enter_standby err, 0x%08x !\n", ret);
            return ret;
        }

        for (j = 0; j < sizeof(g_wakeup_source_map) / sizeof(g_wakeup_source_map[0]); j++) {
            if (source == g_wakeup_source_map[j].source) {
                HI_INFO_PMOC("wakeup by %s !!!\n", g_wakeup_source_map[j].wakeup_source);
            }
        }

        ret = hi_unf_pmoc_get_wakeup_attr(&attr);
        if (ret != HI_SUCCESS) {
            HI_INFO_PMOC("hi_unf_pmoc_get_wakeup_attr err, 0x%08x !\n", ret);
            return ret;
        }

        if (attr.source == HI_UNF_PMOC_WAKEUP_IR) {
            HI_INFO_PMOC("wakeup key(Low): 0x%x, key(High):0x%x \n", attr.wakeup_param.ir_param.ir_low_val,
                attr.wakeup_param.ir_param.ir_high_val);
        } else if (attr.source == HI_UNF_PMOC_WAKEUP_GPIO) {
            HI_INFO_PMOC("wakeup by GPIO%d_%d \n", attr.wakeup_param.gpio_param.group,
                attr.wakeup_param.gpio_param.bit);
        }

        ret = hi_unf_pmoc_get_standby_period(&standby_period);
        if (ret != HI_SUCCESS) {
            HI_INFO_PMOC("hi_unf_pmoc_get_standby_period err, 0x%08x !\n", ret);
            return ret;
        }

        HI_INFO_PMOC("\nstandby period = %d \n", standby_period);

        HI_INFO_PMOC("%%%%%%%% cnt = %d %%%%%%%% \n", i);

        sleep(1);
    }

    PMOC_NOTICE("========================================================\n");

    return HI_SUCCESS;
}

static hi_void power_off(hi_void)
{
    reboot(RB_POWER_OFF);
}

static hi_s32 set_ir_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify ir suspend param =================\n");

    source = HI_UNF_PMOC_WAKEUP_IR;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new ir wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);

    if (enable) {
        HI_INFO_PMOC("Select ir type(0-nec simple, 1-tc9012, 2-nec full, 3-sony_12bit, 4-raw, 5-not support)\n");
        PMOC_NOTICE("Please input new ir type[%d]", param.ir_param.ir_type);
        scanf("%d", (int *)&param.ir_param.ir_type);

        if (param.ir_param.ir_type == HI_UNF_IR_CODE_RAW) {
            /* Here is ir powerkey code values of support */
            param.ir_param.ir_num = 6;

            param.ir_param.ir_low_val[0] = 0x48140f00; /* XMP-1 short-IR */
            param.ir_param.ir_high_val[0] = 0x0;
            param.ir_param.ir_low_val[1] = 0x47180f00; /* XMP-1 long-IR */
            param.ir_param.ir_high_val[1] = 0x0;
            param.ir_param.ir_low_val[2] = 0x639cff00; /* nec simple */
            param.ir_param.ir_high_val[2] = 0x0;
            param.ir_param.ir_low_val[3] = 0x8010260c; /* RC6 32bit */
            param.ir_param.ir_high_val[3] = 0x0000001c;
            param.ir_param.ir_low_val[4] = 0x0000000c; /* RC5 14bit */
            param.ir_param.ir_high_val[4] = 0x00000000;
            param.ir_param.ir_low_val[5] = 0x58535754; /* nec full dezhou like */
            param.ir_param.ir_high_val[5] = 0x00005750;
        } else if (param.ir_param.ir_type == HI_UNF_IR_CODE_SONY_12BIT) {
            param.ir_param.ir_num = 1;
            param.ir_param.ir_low_val[0] = 0x00000095;
            param.ir_param.ir_high_val[0] = 0x0;
        } else if (param.ir_param.ir_type == HI_UNF_IR_CODE_NEC_FULL) {
            param.ir_param.ir_num = 1;
            param.ir_param.ir_low_val[0] = 0x58595a53;
            param.ir_param.ir_high_val[0] = 0x5750;
        } else if (param.ir_param.ir_type == HI_UNF_IR_CODE_TC9012) {
            param.ir_param.ir_num = 1;
            param.ir_param.ir_low_val[0] = 0xf30c0e0e;
            param.ir_param.ir_high_val[0] = 0x0;
        } else {
            param.ir_param.ir_num = 1;
            param.ir_param.ir_low_val[0] = 0x639cff00;
            param.ir_param.ir_high_val[0] = 0x0;
        }
    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 set_keyled_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify keyled suspend param =============\n");
    source = HI_UNF_PMOC_WAKEUP_KEYLED;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new keyled wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);
    if (enable) {
        HI_INFO_PMOC("Select keyled type(0-CT1642, 1-FD650, 2-gpiokey, 3-not support)\n");
        PMOC_NOTICE("Please input new keyled type[%d]", param.keyled_param.keyled_type);
        scanf("%d", (int *)&param.keyled_param.keyled_type);

        if (param.keyled_param.keyled_type < HI_UNF_KEYLED_TYPE_GPIOKEY) {
            PMOC_NOTICE("Please input new keyled wakeup key[%d]", param.keyled_param.wakeup_key);
            scanf("%d", (int *)&param.keyled_param.wakeup_key);
        }
    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 set_gpio_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_u8 i;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify gpio suspend param ===============\n");
    source = HI_UNF_PMOC_WAKEUP_GPIO;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new gpio wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);
    if (enable) {
        PMOC_NOTICE("Please input new gpio wakeup num[%d]", param.gpio_param.num);
        scanf("%d", &param.gpio_param.num);

        if (param.gpio_param.num > HI_UNF_PMOC_WAKEUP_GPIO_MAXNUM) {
            HI_INFO_PMOC("num is bigger than %d, force it to %d \n", HI_UNF_PMOC_WAKEUP_GPIO_MAXNUM,
                HI_UNF_PMOC_WAKEUP_GPIO_MAXNUM);
            param.gpio_param.num = HI_UNF_PMOC_WAKEUP_GPIO_MAXNUM;
        }

        for (i = 0; i < param.gpio_param.num; i++) {
            PMOC_NOTICE("Please input num%d new wakeup group[%d]", i, param.gpio_param.group[i]);
            scanf("%d", (int *)&param.gpio_param.group[i]);
            PMOC_NOTICE("Please input num%d new wakeup bit[%d]", i, param.gpio_param.bit[i]);
            scanf("%d", (int *)&param.gpio_param.bit[i]);
            HI_INFO_PMOC("Select gpio interrupt type(0-up edge, 1-down edge, 2-both the up and down edge, ");
            HI_INFO_PMOC("3-high level, 4-low level, 5-not support\n");
            PMOC_NOTICE("Please input num%d new interrupt type[%d]", i, param.gpio_param.interrupt_type[i]);
            scanf("%d", (int *)&param.gpio_param.interrupt_type[i]);
        }
    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 set_uart_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify uart suspend param ===============\n");
    source = HI_UNF_PMOC_WAKEUP_UART;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new uart wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);
    if (enable) {
        PMOC_NOTICE("Please input new uart wakeup key[%d]", param.uart_param.wakeup_key);
        scanf("%d", (int *)&param.uart_param.wakeup_key);
    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 set_eth_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_u8 i;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify eth suspend param ================\n");
    source = HI_UNF_PMOC_WAKEUP_ETH;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new eth wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);
    if (enable) {
        param.eth_param.index = 0; /* 0-eth0, 1-eth1 */

        PMOC_NOTICE("Please input new single packet wakeup enable flag[%d]", param.eth_param.unicast_packet_enable);
        scanf("%d", (int *)&param.eth_param.unicast_packet_enable);
        if (param.eth_param.unicast_packet_enable == HI_FALSE) {
            PMOC_NOTICE("Please input new magic packet wakeup enable flag[%d]", param.eth_param.magic_packet_enable);
            scanf("%d", (int *)&param.eth_param.magic_packet_enable);

            if (param.eth_param.magic_packet_enable == HI_FALSE) {
                PMOC_NOTICE("Please input magic frame filter wakeup enable flag[%d]", param.eth_param.wakeup_frame_enable);
                scanf("%d", (int *)&param.eth_param.wakeup_frame_enable);
            }
        }

        if (param.eth_param.wakeup_frame_enable) {
            /* normal filter */
            param.eth_param.frame[0].mask_bytes = U32_MAX;
            param.eth_param.frame[0].offset = 15; /* offset: 15 */
            param.eth_param.frame[0].filter_valid = HI_TRUE;

            for (i = 0; i < HI_UNF_PMOC_FILTER_VALUE_COUNT; i++) {
                param.eth_param.frame[0].value[i] = i;
            }

            /* enable part of mask bytes */
            param.eth_param.frame[1].mask_bytes = 0xf0ffffff;
            param.eth_param.frame[1].offset = 15; /* offset: 15 */
            param.eth_param.frame[1].filter_valid = HI_TRUE;

            for (i = 0; i < HI_UNF_PMOC_FILTER_VALUE_COUNT; i++) {
                param.eth_param.frame[1].value[i] = i;
            }

            /* offset > 128 */
            param.eth_param.frame[2].mask_bytes = U32_MAX;
            param.eth_param.frame[2].offset = 130; /* offset: 130 */
            param.eth_param.frame[2].filter_valid = HI_TRUE;

            for (i = 0; i < HI_UNF_PMOC_FILTER_VALUE_COUNT; i++) {
                param.eth_param.frame[2].value[i] = 0x2; /* frame value: 0x2 */
            }

            /* false filter, invalid */
            param.eth_param.frame[3].mask_bytes = U32_MAX;
            param.eth_param.frame[3].offset = 9; /* offset: 9 */
            param.eth_param.frame[3].filter_valid = HI_FALSE;

            for (i = 0; i < HI_UNF_PMOC_FILTER_VALUE_COUNT; i++) {
                param.eth_param.frame[3].value[i] = 0x1; /* frame value: 0x1 */
            }
        }

        if (param.eth_param.unicast_packet_enable || param.eth_param.magic_packet_enable ||
            param.eth_param.wakeup_frame_enable) {
            PMOC_NOTICE("Please input Eth wakeup mode to pasive standby time[%d]",
                param.eth_param.time_to_passive_standby);
            scanf("%d", (int *)&param.eth_param.time_to_passive_standby);
        }
    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 set_usb_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify usb suspend param ================\n");
    source = HI_UNF_PMOC_WAKEUP_USB;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new usb wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 set_hdmitx_cec_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("============= Modify hdmitx cec suspend param =============\n");
    source = HI_UNF_PMOC_WAKEUP_HDMITX_CEC;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new hdmitx cec wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);

    if (enable) {
        param.hdmitx_cec_param.id[0] = 1;
        HI_INFO_PMOC("Select cec control type(0-bilateral, 1-tv to chip, 2-chip to tv, 3-not support)\n");
        PMOC_NOTICE("Please input new hdmitx cec control type [%d]", param.hdmitx_cec_param.cec_control[0]);
        scanf("%d", (int *)&param.hdmitx_cec_param.cec_control[0]);
    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}


static hi_s32 set_timeout_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= Modify timeout suspend param ============\n");
    source = HI_UNF_PMOC_WAKEUP_TIMEOUT;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("Please input new timeout wakeup enable flag[%d]", enable);
    scanf("%d", (int *)&enable);
    if (enable) {
        PMOC_NOTICE("Please input new suspend period[%d]", param.timeout_param.suspend_period);
        scanf("%d", (int *)&param.timeout_param.suspend_period);
        PMOC_NOTICE("Please input new pvr enable flag[%d]", param.timeout_param.pvr_enable);
        scanf("%d", (int *)&param.timeout_param.pvr_enable);

    }

    ret = hi_unf_pmoc_set_suspend_param(source, enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 set_suspend_param(hi_void)
{
    hi_s32 ret;

    ret = set_ir_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_ir_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_keyled_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_keyled_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_gpio_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_gpio_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_uart_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_uart_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_eth_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_eth_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_hdmitx_cec_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_hdmi_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_usb_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_usb_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = set_timeout_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("set_timeout_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 get_ir_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_u8 i;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= ir suspend param ========================\n");
    source = HI_UNF_PMOC_WAKEUP_IR;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("ir wakeup \t:%s \n", enable ? "enable" : "disable");

    if (enable) {
        HI_INFO_PMOC("ir type \t:%d \n", param.ir_param.ir_type);

        for (i = 0; i < param.ir_param.ir_num; i++) {
            HI_INFO_PMOC("IR wakeup key \t:High 32-bit(0x%08x), Low 32-bit(0x%08x)\n",
                param.ir_param.ir_high_val[i], param.ir_param.ir_low_val[i]);
        }
    }

    return HI_SUCCESS;
}

static hi_s32 get_keyled_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= keyled suspend param ====================\n");
    source = HI_UNF_PMOC_WAKEUP_KEYLED;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("keyled wakeup \t:%s \n", enable ? "enable" : "disable");

    if (enable) {
        HI_INFO_PMOC("keyled type \t:%d(0-CT1642, 1-FD650, 2-gpiokey, 3-not support)\n",
            param.keyled_param.keyled_type);

        if (param.keyled_param.keyled_type < HI_UNF_KEYLED_TYPE_GPIOKEY) {
            HI_INFO_PMOC("wakeup key \t:%d \n", param.keyled_param.wakeup_key);
        }
    }

    return HI_SUCCESS;
}

static hi_s32 get_gpio_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_u8 i;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= gpio suspend param ======================\n");
    source = HI_UNF_PMOC_WAKEUP_GPIO;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("gpio wakeup \t:%s \n", enable ? "enable" : "disable");

    if (enable) {
        for (i = 0; i < param.gpio_param.num; i++) {
            HI_INFO_PMOC("gpio port \t:%d_%d \n", param.gpio_param.group[i], param.gpio_param.bit[i]);
            HI_INFO_PMOC("interrupt type \t:%d \n", param.gpio_param.interrupt_type[i]);
        }
    }

    return HI_SUCCESS;
}

static hi_s32 get_uart_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= uart suspend param ======================\n");
    source = HI_UNF_PMOC_WAKEUP_UART;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("uart wakeup \t:%s \n", enable ? "enable" : "disable");

    if (enable) {
        HI_INFO_PMOC("wakeup key \t:%d \n", param.uart_param.wakeup_key);
    }

    return HI_SUCCESS;
}

static hi_s32 get_eth_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= eth suspend param =======================\n");
    source = HI_UNF_PMOC_WAKEUP_ETH;
    param.eth_param.index = 0;

    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    if (enable) {
        HI_INFO_PMOC("Magic packet wakeup \t:%s \n",
            param.eth_param.magic_packet_enable ? "enable" : "disable");
        HI_INFO_PMOC("Unicast packet wakeup \t:%s \n",
            param.eth_param.unicast_packet_enable ? "enable" : "disable");
        HI_INFO_PMOC("Wakeup frame wakeup \t:%s \n",
            param.eth_param.wakeup_frame_enable ? "enable" : "disable");

        HI_INFO_PMOC("Eth wakeup mode to passive standby time \t:%d \n", param.eth_param.time_to_passive_standby);

    } else {
        HI_INFO_PMOC("eth wakeup \t:%s \n", enable ? "enable" : "disable");
    }

    return HI_SUCCESS;
}

static hi_s32 get_usb_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= usb suspend param =======================\n");
    source = HI_UNF_PMOC_WAKEUP_USB;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("usb wakeup \t:%s \n", enable ? "enable" : "disable");

    return HI_SUCCESS;
}

static hi_s32 get_hdmitx_cec_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= hdmitx cec suspend param ================\n");
    source = HI_UNF_PMOC_WAKEUP_HDMITX_CEC;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("hdmitx cec wakeup \t:%s \n", enable ? "enable" : "disable");

    return HI_SUCCESS;
}


static hi_s32 get_timeout_suspend_param(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_src source;
    hi_bool enable;
    hi_unf_pmoc_suspend_param param = {0};

    PMOC_NOTICE("================= timeout suspend param ===================\n");
    source = HI_UNF_PMOC_WAKEUP_TIMEOUT;
    ret = hi_unf_pmoc_get_suspend_param(source, &enable, &param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    HI_INFO_PMOC("timeout wakeup \t:%s \n", enable ? "enable" : "disable");

    if (enable) {
        HI_INFO_PMOC("suspend period \t:%d\n", param.timeout_param.suspend_period);
        HI_INFO_PMOC("pvr enable flag \t:%d\n", param.timeout_param.pvr_enable);
    }

    return HI_SUCCESS;
}

hi_s32 get_suspend_param(hi_void)
{
    hi_s32 ret;

    ret = get_ir_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_ir_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_keyled_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_keyled_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_gpio_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_gpio_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_uart_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_uart_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_eth_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_eth_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_usb_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_usb_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_hdmitx_cec_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_hdmitx_cec_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    ret = get_timeout_suspend_param();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_timeout_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 set_wakeup_type(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_wakeup_type wakeup_type = {0};

    PMOC_NOTICE("============= Modify wakeup type =============\n");
    PMOC_NOTICE("(0-wakeup to ddr, 1-reset) Select wakeup type[%d]", wakeup_type);
    scanf("%d", (int *)&wakeup_type);

    ret = hi_unf_pmoc_set_wakeup_type(wakeup_type);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("get_timeout_suspend_param err, 0x%08x !\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 set_display_params(hi_void)
{
    hi_s32 ret;
    time_t real_time;
    struct tm *display_time;
    hi_unf_pmoc_display_param display_param = {0};

    PMOC_NOTICE("============= Modify keyled display params =============\n");
    HI_INFO_PMOC("Select keyled type(0-CT1642, 1-FD650, 3-gpiokey, 4-not support)");
    PMOC_NOTICE("Please input new keyled type[%d]", display_param.keyled_type);
    scanf("%d", (int *)&display_param.keyled_type);

    HI_INFO_PMOC("Select display type(0-no display, 1-display number, 2-display time(in second))");
    PMOC_NOTICE("Please input new display type[%d]", display_param.display_type);
    scanf("%d", (int *)&display_param.display_type);

    if (display_param.display_type == HI_UNF_PMOC_DISPLAY_DIGIT) {
        PMOC_NOTICE("Please input new display digit[%d]", display_param.display_value);
        scanf("%d", (int *)&display_param.display_value);
    } else if (display_param.display_type == HI_UNF_PMOC_DISPLAY_TIME) {
        ret = time(&real_time);
        if (ret == -1) {
            display_param.display_time_info.hour = 6; /* hour: 6 */
            display_param.display_time_info.minute = 9; /* minute: 9 */
            display_param.display_time_info.second = 36; /* second: 36 */
        } else {
            display_time = gmtime(&real_time);
            display_param.display_time_info.hour = (hi_u32)display_time->tm_hour;
            display_param.display_time_info.minute = (hi_u32)display_time->tm_min;
            display_param.display_time_info.second = (hi_u32)display_time->tm_sec;
        }
    }

    ret = hi_unf_pmoc_set_standby_display_param(&display_param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_standby_display_param err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("keyled display params modify success! \n");
    PMOC_NOTICE("========================================================\n");

    return HI_SUCCESS;
}

hi_s32 set_gpio_power_off(hi_void)
{
    hi_s32 ret;
    hi_u8 i;
    hi_unf_pmoc_poweroff_gpio_param param = {0};

    PMOC_NOTICE("============= Modify power off gpio params =============\n");

    PMOC_NOTICE("Please input poweroff gpio num[%d]", param.num);
    scanf("%d", &param.num);

    if (param.num > HI_UNF_PMOC_POWEROFF_GPIO_MAXNUM) {
        HI_INFO_PMOC("num is bigger than %d, force it to %d", HI_UNF_PMOC_POWEROFF_GPIO_MAXNUM,
            HI_UNF_PMOC_POWEROFF_GPIO_MAXNUM);
        param.num = HI_UNF_PMOC_POWEROFF_GPIO_MAXNUM;
    }

    for (i = 0; i < param.num; i++) {
        PMOC_NOTICE("Please input poweroff num%d group:[%d] ", i, param.group[i]);
        scanf("%d", (int *)&param.group[i]);
        printf("\n param.group[%d] :%d \n", i, param.group[i]);
        PMOC_NOTICE("Please input poweroff num%d bit:[%d] ", i, param.bit[i]);
        scanf("%d", (int *)&param.bit[i]);
        printf("\n param.bit[%d] :%d \n", i, param.bit[i]);
        PMOC_NOTICE("Please input poweroff num%d level:[%d] ", i, param.level[i]);
        scanf("%d", (int *)&param.level[i]);
        printf("\n param.level[%d] :%d \n", i, param.level[i]);
    }

    ret = hi_unf_pmoc_set_gpio_power_off(&param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_set_gpio_power_off err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("power off gpio params modify success! \n");
    PMOC_NOTICE("========================================================\n");

    return HI_SUCCESS;
}

hi_s32 set_memory_standby_flag(hi_void)
{
    hi_s32 ret;
    hi_unf_pdm_pmoc_param pmoc_param = {0};

    PMOC_NOTICE("================ set memory standby flag ==============\n");

    pmoc_param.power_on_mode = HI_UNF_PDM_PMOC_MEMORY_STANDBY;

    ret = hi_unf_pdm_update_pmoc_param(&pmoc_param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("HI_UNF_PDM_UpdateBaseParam err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("memory standby flag modify success! \n");
    PMOC_NOTICE("========================================================\n");

    return HI_SUCCESS;
}

hi_s32 get_memory_standby_flag(hi_void)
{
    hi_s32 ret;
    hi_unf_pdm_pmoc_param pmoc_param = {0};

    ret = hi_unf_pdm_get_pmoc_param(&pmoc_param);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pdm_get_pmoc_param err, 0x%08x !\n", ret);
        return ret;
    }

    if (pmoc_param.power_on_mode == HI_UNF_PDM_PMOC_MEMORY_STANDBY) {
        printf("memory standby flag has been set\n");
    } else {
        printf("memory standby flag has not been set\n");
    }

    return HI_SUCCESS;
}

hi_s32 get_chip_temperature(hi_void)
{
    hi_s32 ret;
    hi_unf_pmoc_chip_temperature chip_temprature = {0};

    ret = hi_unf_pmoc_get_chip_temperature(&chip_temprature);
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_get_chip_temperature err, 0x%08x !\n", ret);
        return ret;
    }

    PMOC_NOTICE("==================== chip temperature ==================\n");
    PMOC_NOTICE("chip temperature        \t:%d \n", chip_temprature.t_sensor1_temperature);
    PMOC_NOTICE("========================================================\n");

    return HI_SUCCESS;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret;
    hi_s32 result;

    ret = hi_unf_pmoc_init();
    if (ret != HI_SUCCESS) {
        HI_INFO_PMOC("hi_unf_pmoc_init err, 0x%08x !\n", ret);
        return ret;
    }

    result = getopt(argc, argv, "tps:g:");

    switch (result) {
        case 't': {
            tigger_standby();
            hi_unf_pmoc_deinit();
            return 0;
        }
        case 'p': {
            power_off();
            hi_unf_pmoc_deinit();
            return 0;
        }
        case 's': {
            if (!strcmp("suspend_param", optarg)) {
                ret = set_suspend_param();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("set_suspend_param err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            } else if (!strcmp("wakeup_type", optarg)) {
                ret = set_wakeup_type();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("set_wakeup_type err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            } else if (!strcmp("display", optarg)) {
                ret = set_display_params();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("set_display_params err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            } else if (!strcmp("power_off_gpio", optarg)) {
                ret = set_gpio_power_off();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("set_gpio_powe_roff err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            } else if (!strcmp("memory_standby_flag", optarg)) {
                ret = set_memory_standby_flag();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("set_memory_standby_flag err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            }
            break;
        }
        case 'g': {
            if (!strcmp("suspend_param", optarg)) {
                ret = get_suspend_param();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("get_suspend_param err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            } else if (!strcmp("memory_standby_flag", optarg)) {
                ret = get_memory_standby_flag();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("get_memory_standby_flag err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            } else if (!strcmp("chip_temperature", optarg)) {
                ret = get_chip_temperature();
                if (ret != HI_SUCCESS) {
                    HI_INFO_PMOC("get_chip_temperature err, 0x%08x !\n", ret);
                }

                hi_unf_pmoc_deinit();
                return ret;
            }
            break;
        }
        default: {
            break;
        }
    }

    hi_unf_pmoc_deinit();

    show_usage_info();

    return ret;
}

