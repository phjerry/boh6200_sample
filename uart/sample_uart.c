/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
 * Description:unf_lasdc.c
 * Author: DPT_BSP
 * Create: 2011-11-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "securec.h"
#include "hi_unf_uart.h"

#define TEST_DATA_LEN    512
#define TEST_BUF_LEN     (110 * 1024)
#define OPTSET           ":hvb:c:d:f:l:m:p:s:t:"
#define FILE_LEN         52

#define AMA0        "/dev/ttyAMA0"
#define AMA1        "/dev/ttyAMA1"

void usage()
{
    printf("Usage: no option means default attribute[baud speed=115200, databits=8, stopbits=1, parity=n]\
        \n-b(baud speed=[230400 115200 57600 38400 19200 9600 4800 2400 1200 600 300 110])\
        \n-c(buffer size)\
        \n-d(databits=[7 8])\
        \n-f(device name)\
        \n-s(stopbis=[1 2])\
        \n-p(parity=[n e o N E O] (n/N-none e/E-even o/O-odd))\n");

    return;
}

int param_verify(int speed, int databit, int stopbit, char parity)
{
    if (speed && (speed < 110 || speed > 230400)) { /* speed：110~230400 */
        printf("only support baud speed [230400 115200 57600 38400 "
            "19200 9600 4800 2400 1200 600 300 110] while speed=%d\n", speed);
        return -1;
    }

    if (databit && (databit != 7 && databit != 8)) { /* only support databit 7 and 8 */
        printf("only support databit 7 and 8\n");
        return -1;
    }

    if (stopbit && (stopbit != 1 && stopbit != 2)) { /* only support stopbit 1 and 2 */
        printf("only support stopbit 1 and 2\n");
        return -1;
    }

    if (parity && (parity != 'n' && parity != 'N'
        && parity != 'o' && parity != 'O'
        && parity != 'e' && parity != 'E')) {
        printf("only support parity in n/e/o or N/E/O\n");
        return -1;
    }

    /* default setting is 8/1/N  */
    if (hi_unf_uart_set_attr(databit, stopbit, parity)) {
        return -1;
    }

    /* default setting is 115200 */
    if (hi_unf_uart_set_speed(speed)) {
        return -1;
    }

    return 0;
}

int get_user_parameter(int argc, char **argv, int *ttyama)
{
    int opt = 0;
    char file[FILE_LEN] = AMA1; /* default device */
    int speed = 0;
    int databit = 0;
    int stopbit = 0;
    char parity = 0;

    while ((opt = getopt(argc, argv, OPTSET)) != -1) {
        switch (opt) {
            case 'b':
                speed = atoi(optarg);
                break;
            case 'd':
                databit = atoi(optarg);
                break;
            case 'f':
                (void)memset_s(file, FILE_LEN, 0, FILE_LEN);
                (void)strncpy_s(file, FILE_LEN, optarg, FILE_LEN - 1);
                break;
            case 'p':
                parity = optarg[0];
                break;
            case 's':
                stopbit = atoi(optarg);
                break;
            default:
                printf("Unknown error.\n\n");
                usage();
                return -1;
        }
    }

    /* uart0 used by system */
    if (!strcmp(file, AMA0)) {
        *ttyama = 0;
    } else if (!strcmp(file, AMA1)) {
        *ttyama = 1;
    } else {
        printf("only support ttyAMA1, extend your %s manually.\n", file);
        return -1;
    }

    if (param_verify(speed, databit, stopbit, parity)) {
        return -1;
    }

    printf("Current uart is ttyAMA%d, baudspeed=%d, attribute=%d/%d/%c\n",
           *ttyama, speed, databit, stopbit, parity);

    return 0;
}

int main(int argc, char *argv[])
{
    int nread = 0;
    int ttyama, old_ttyama;
    char *buff = NULL; /* data buffer */

    if (hi_unf_uart_init()) {
        return -1;
    }

    if (get_user_parameter(argc, argv, &ttyama)) {
        goto sample_error;
    }

    /* get current uart */
    old_ttyama = hi_unf_uart_get_number();
    if (ttyama == old_ttyama) {
        printf("ttyAMA is in working.\n");
        goto sample_error;
    }

    /* switch to ttyAMA1 */
    if (hi_unf_uart_switch(ttyama)) {
        goto sample_error;
    }

    buff = (char *)malloc(TEST_BUF_LEN);
    if (!buff) {
        perror("malloc");
        goto sample_error1;
    }

    while (1) {
        (void)memset_s(buff, TEST_BUF_LEN, 0, TEST_BUF_LEN);
        nread = hi_unf_uart_read(buff, TEST_BUF_LEN);

        usleep(50000); /* delay 500000 */
        hi_unf_uart_write(buff, nread);
    }

    free(buff);

sample_error1:
    hi_unf_uart_switch(old_ttyama);

sample_error:
    hi_unf_uart_deinit();

    return 0;
}
