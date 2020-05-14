/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2008-2019. All rights reserved.
 * Description: sample test for ir function.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#include <assert.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_unf_ir.h"
#include "hi_errno.h"

#define PARAMETER_CONVERSION_FORMAT 10

static char *g_prot_names[] = {
    "nec full 2headers",
    "nec simple 2headers changshu",
    "nec simple uPD6121G like",
    "nec simple LC7461MCL13 like",
    "nec full dezhou like",
    "nec full huizhou like",
    "nec simple 2headers gd",
    "credit",
    "sony d7c5",
    "sony d7c8",
    "tc9012",
    "rc6 32bit data",
    "rc6 20bit data",
    "rc6 16bit data",
    "rc5 14bit data",
    "rstep",
    "xmp-1",
    "extended rc5 14bit"
};

void usage(char *base)
{
    printf("usage: %s [-m 0|1] [-u 0|1] [-U num] [-r 0|1] [-R num] [-T readtimeout] [-E 0|1]"\
           " [-g protocolname] [-l] [-s protocolname -e 0|1] [-h]\n"\
           "\t-m : key fetch g_mode, 0 to read key parsed by driver, 1 to read symbols received.\n"\
           "\t-u : set keyup event, 0 to disable keyup event, 1 to enable keyup event.\n"\
           "\t-U : set keyup timeout, e.g. -U 300, driver will send a keyup event of last key parsed.\n"\
           "\t     after 300ms if no follow key recevied. Not supported yet.\n"\
           "\t-r : set repeat key event report or not. 0 to disable repeat event, 1 to enable.\n"\
           "\t-R : set repeat key event interval time. Default 200ms.\n"\
           "\t     e.g. -R 100 was set, then driver will report a key repeat event each 100ms.\n"\
           "\t-T : set read timeout value. e.g. -T 1000 read syscall will return after 1000ms if no key recevied.\n"\
           "\t-E : enable or disable ir module. 0 to disable, 1 to enable. If 0 is set, ir module will do nothing.\n"
           "\t-g : get the enable/disable status of protocol which is specified by protocolname and then exit.\n"\
           "\t-l : list all default protocol name and exit. Only constains IR module, no customer's included!\n"\
           "\t-s : set the enable/disable status of protocol, 0 to enable, 1 to disable.\n"\
           "\t     while set disable state, the respect protocol will not be considerd as a protocol parser,\n"\
           "\t     and if some corresponding infrared symbol received, they are not parsed and may be discarded.\n"\
           "\t-e : 0 to disable, 1 to enable, must be used with -s\n"\
           "\t-h : display this message and exit.\n"\
           "\tNOTES: Only [-s -e] option can be set more than once!\n",
           base);
}

extern char  *optarg;
extern int   optind, opterr, optopt;

/*lint -save -e729 */
int          g_mode, g_mode_set;
int          g_keyup_event, g_keyup_event_set;
unsigned int g_keyup_timeout, g_keyup_timeout_set;
int          g_repeat_event, g_repeat_event_set;
unsigned int g_repeat_interval, g_repeat_interval_set;
unsigned int g_read_timeout = 200;
int          g_ir_enable, g_ir_enable_set;
int          g_prot_status;
char         *g_prot_name;
int          g_prot_name_set;
int          g_prot_enable, g_prot_enable_set;

int          g_thread_should_stop = 0;

static int check_params(void)
{
    if (g_mode_set >  1 || (g_mode != 0 && g_mode != 1)) {
        printf("fetch g_mode error!\n");
        return -1;
    }

    if (g_keyup_event_set > 1 || (g_keyup_event != 0 && g_keyup_event != 1)) {
        printf("keyup event error!\n");
        return -1;
    }
    if (g_repeat_event_set > 1 || g_repeat_interval_set > 1) {
        printf("Only -g [-s -e] option can be set more than once!\n");
        return -1;
    }
    return 0;
}
static int ir_get_prot_status(char *prot)
{
    int ret;
    hi_bool enabled;

    ret = hi_unf_ir_init();
    if (ret) {
        perror("Fail to open ir device!\n");
        return -1;
    }

    ret = hi_unf_ir_get_protocol_enabled(prot, &enabled);
    if (ret) {
        hi_unf_ir_deinit();
        printf("Fail to get protocol status!\n");
        return -1;
    }

    hi_unf_ir_deinit();
    return enabled;

}
static int ir_sample_set_prot_enable(char *prot, int enable)
{
    int ret;

    ret = hi_unf_ir_init();
    if (ret) {
        perror("Fail to open ir device!\n");
        return -1;
    }

    printf("Try set enable status %d to protocol %s.\n", enable, prot);
    if (enable) {
        ret = hi_unf_ir_enable_protocol(prot);
    } else {
        ret = hi_unf_ir_disable_protocol(prot);
    }
    if (ret) {
        printf("Fail to set enable status of %s\n", prot);
        hi_unf_ir_deinit();
        return -1;
    }

    printf("Check it out if we do have set succussfully!\n");
    if (enable == ir_get_prot_status(prot)) {
        printf("Check pass!\n");
    } else {
        printf("Check fail!\n");
    }

    hi_unf_ir_deinit();
    return 0;
}

static void process_options_mode(void)
{
    g_mode = atoi(optarg);
    g_mode_set++;
    printf("key fetch g_mode set to :%d\n", g_mode);
}
static void process_options_upevent(void)
{
    g_keyup_event = atoi(optarg);
    g_keyup_event_set++;
    printf("keyup event set to %d\n", g_keyup_event);
}
static void process_options_uptimeout(void)
{
    g_keyup_timeout = strtoul(optarg, 0, PARAMETER_CONVERSION_FORMAT);
    g_keyup_timeout_set ++;
    printf("keyup timeout set to %d\n", g_keyup_timeout);
}
static void process_options_repeat_event(void)
{
    g_repeat_event = atoi(optarg);
    g_repeat_event_set ++;
    printf("repeat event set to %d\n", g_repeat_event);
}
static void process_options_repeat_interval(void)
{
    g_repeat_interval = strtoul(optarg, 0, PARAMETER_CONVERSION_FORMAT);
    g_repeat_interval_set ++;
    printf("repeat interval set to %d\n", g_repeat_interval);
}
static void process_options_enable(void)
{
    g_ir_enable = atoi(optarg);
    if (g_ir_enable != 0 && g_ir_enable != 1) {
        printf("-E parameter not correct, only 0 or 1 allowed!\n");
    } else {
        g_ir_enable_set = 1;
        printf("Set ir g_mode to %s state!\n", g_ir_enable ? "enable" : "disable");
    }
}
static void process_options_prot_status(void)
{
    printf("try to get the enable status of %s\n", optarg);

    g_prot_status = ir_get_prot_status(optarg);
    if (g_prot_status >= 0) {
        printf("The status of %s is :%s\n",
               optarg,
               g_prot_status == 0 ? "disabled" : "enabled");
    } else {
        printf("Fail to get protocol status!\n");
    }
    exit(0);
}
static void process_options_prot_list(void)
{
    int i;
    printf("Available default protocols list:\n");
    for (i = 0; i < (hi_s32)(sizeof(g_prot_names) / sizeof(g_prot_names[0])); i++) {
        printf("%s\n", g_prot_names[i]);
    }
    exit(0);
}
static void process_options_prot_set(void)
{
    int ret;

    g_prot_name = optarg;
    g_prot_name_set = 1;
    printf("g_prot_name :%s.\n", g_prot_name);

    if (g_prot_enable_set && g_prot_name_set) {
        g_prot_enable_set = 0;
        g_prot_name_set = 0;
        ret = ir_sample_set_prot_enable(g_prot_name, g_prot_enable);
        if (ret) {
            perror("Fail to set g_prot_enable!\n");
            exit(-1);
        }
    }
}
static void process_options_protstatus_set(void)
{
    int ret;

    g_prot_enable = atoi(optarg);
    g_prot_enable_set = 1;

    printf("protocol enable status will set to :%d\n", g_prot_enable);

    if (g_prot_enable_set && g_prot_name_set) {
        g_prot_enable_set = 0;
        g_prot_name_set = 0;
        ret = ir_sample_set_prot_enable(g_prot_name, g_prot_enable);
        if (ret) {
            perror("Fail to set g_prot_enable!\n");
            exit(-1);
        }
    }
}
static void process_options(int argc, char *argv[])
{
    int op;
    opterr = 0;
    while ((op = getopt(argc, argv, "m:u:U:r:R:T:E:g:ls:e:h")) != -1) {
        switch (op) {
            case 'm' :
                process_options_mode();
                break;
            case 'u' :
                process_options_upevent();
                break;
            case 'U' :
                process_options_uptimeout();
                break;
            case 'r' :
                process_options_repeat_event();
                break;
            case 'R' :
                process_options_repeat_interval();
                break;
            case 'T' :
                g_read_timeout = strtoul(optarg, 0, PARAMETER_CONVERSION_FORMAT);
                printf("read syscall timeout set to %d\n", g_read_timeout);
                break;
            case 'E':
                process_options_enable();
                break;
            case 'g' :
                process_options_prot_status();
            /* fall-through */
            case 'l' :
                process_options_prot_list();
            /* fall-through */
            case 's' :
                process_options_prot_set();
                break;
            case 'e' :
                process_options_protstatus_set();
                break;
            case 'h' :
            /* fall-through */
            default :
                usage(argv[0]);
                exit(0);
        }
    }
}
/*lint -save -e715 */
void *ir_sample_thread(void *arg)
{
    int ret;
    hi_u64 key, lower, upper;
    char name[64];   /* 64:Protocol name max len */
    hi_unf_key_status status;

    while (!g_thread_should_stop) {
        if (g_mode) {
            ret = hi_unf_ir_get_symbol(&lower, &upper, g_read_timeout);
            if (!ret) {
                printf("Received symbols [%d, %d].\n", (hi_u32)lower, (hi_u32)upper);
            }
        } else {
            ret = hi_unf_ir_get_value_protocol(&status, &key, name, sizeof(name), g_read_timeout);
            if (!ret) {
                printf("Received key: 0x%.08llx, %s,\tprotocol: %s.\n",
                       key, status == HI_UNF_KEY_STATUS_DOWN ? "DOWN" :
                        (status == HI_UNF_KEY_STATUS_UP ? "UP" : "HOLD"),
                       name);
            }
        }
    }

    return (void *)0;
}
static void  ir_set(void)
{
    int ret;
    /* key g_mode. */
    if (!g_mode) {
        hi_unf_ir_set_code_type(0);
        if (g_keyup_event_set) {
            ret = hi_unf_ir_enable_keyup(g_keyup_event);
            if (ret) {
                perror("Fail to set keyup event!");
                exit(-1);
            }
        }
        if (g_keyup_timeout_set) {
            printf("This version cannot support dynamicly change keyup timeout!\n");
        }
        if (g_repeat_event_set) {
            ret = hi_unf_ir_enable_repkey(g_repeat_event);
            if (ret) {
                perror("Fail to set repeat key event!");
                exit(-1);
            }
        }
        if (g_repeat_interval_set) {
            ret = hi_unf_ir_set_repkey_timeout(g_repeat_interval);
            if (ret) {
                perror("Fail to set repeat interval!");
                exit(-1);
            }
        }
    }
    ret = hi_unf_ir_set_fetch_mode(g_mode);
    if (ret) {
        perror("Fail to set key fetch g_mode!");
        if (ret == HI_ERR_IR_UNSUPPORT) {
            printf("You may not use s2 g_mode, please set -E 1 parameter to enable ir module!\n");
        } else {
            exit(-1);
        }
    }
    if (g_ir_enable_set) {
        ret = hi_unf_ir_enable(g_ir_enable);
        if (ret) {
            perror("Fail to set enable state of ir module!");
            exit(-1);
        }
    }
}
int main(int argc, char *argv[])
{
    int ret;
    pthread_t thread;
    /*lint -save -e550 */
    char c;

    process_options(argc, argv);

    if (check_params() < 0) {
        usage(argv[0]);
        exit(-1);
    }
    ret = hi_unf_ir_init();
    if (ret) {
        perror("Fail to open ir dev!");
        exit(-1);
    }

    ir_set();

    printf("Create ir sample thread, press q to exit!\n");
    ret = pthread_create(&thread, NULL, ir_sample_thread, NULL);
    if (ret < 0) {
        perror("Failt to create thread!");
        exit(-1);
    }
    c = getchar();
    while (c != 'q') {
        sleep(3);  /* 3:sleep 3ms */
        c = getchar();
    }
    g_thread_should_stop = 1;
    pthread_join(thread, NULL);
    printf("Ir sample thread exit! Bye!\n");
    hi_unf_ir_deinit();
    return 0;
}
