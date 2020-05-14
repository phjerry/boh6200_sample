/**********************************************************************************
 *              Copyright 2006 - 2016, Hisilicon Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 * FileName: hi_api_eth.c
 * Description: X5V100 SF module API function
 *
 * History:
 * Version   Date             Author    DefectNum    Description
 * V1.00     Sep 28th         w54542    NA           1 st release
 ***********************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <linux/if_arp.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/route.h>

#include "hi_type.h"
#include "hi_errno.h"
#include "ethtool.h"
#include "hi_api_eth.h"

/******************* sf register definition**************/
#define REG_SF_BASE              0x600D0000
#define REG_SF_GLB_FWCTRL        (REG_SF_BASE + 0x1310)
#define REG_SF_GLB_MACTCTRL      (REG_SF_BASE + 0x1314)
#define REG_SF_GLB_NOVLANCTRL    (REG_SF_BASE + 0x1320)
#define REG_SF_GLB_UNKNOWN_VLAN  (REG_SF_BASE + 0x1324)
#define REG_SF_GLB_CPU_VLANCTRL  (REG_SF_BASE + 0x1328)
#define REG_SF_GLB_PORT_VLANID   (REG_SF_BASE + 0x132C)
#define REG_SF_GLB_SPEVLANCTRL   (REG_SF_BASE + 0x133C)
#define REG_SF_GLB_MAC0_L32      (REG_SF_BASE + 0x1400)
#define REG_SF_GLB_MAC0_H16      (REG_SF_BASE + 0x1404)
#define REG_SF_GLB_VLAN0         (REG_SF_BASE + 0x1500)
#define REG_SF_U_GLB_FC_LEVEL    (REG_SF_BASE + 0x0348)
#define REG_SF_U_GLB_FC_TIMECTRL (REG_SF_BASE + 0x0370)
#define REG_SF_U_GLB_FC_RXLIMIT  (REG_SF_BASE + 0x0374)
#define REG_SF_U_GLB_FC_DROPCTRL (REG_SF_BASE + 0x0378)
#define REG_SF_D_GLB_FC_LEVEL    (REG_SF_BASE + 0x2348)
#define REG_SF_D_GLB_FC_TIMECTRL (REG_SF_BASE + 0x2370)
#define REG_SF_D_GLB_FC_RXLIMIT  (REG_SF_BASE + 0x2374)
#define REG_SF_D_GLB_FC_DROPCTRL (REG_SF_BASE + 0x2378)
#define BIT_VLAN_ENA_SHIFT        0
#define BIT_MACT_ENA_UP_SHIFT     7
#define BIT_MACT_ENA_DOWN_SHIFT   15
/* FWCTRL */
#define BIT_FW2DOWN_ENA_UP_SHIFT  4
#define BIT_FW2CPU_ENA_UP_SHIFT   5
#define BIT_FWALL2DOWN_UP_SHIFT   6
#define BIT_FWALL2CPU_UP_SHIFT    7
#define BIT_FW2UP_ENA_DOWN_SHIFT  8
#define BIT_FW2CPU_ENA_DOWN_SHIFT 9
#define BIT_FWALL2UP_DOWN_SHIFT   10
#define BIT_FWALL2CPU_DOWN_SHIFT  12
/* MACTCTRL */
#define BIT_MACT_UNI2DOWN_UP      0
#define BIT_MACT_UNI2CPU_UP       1
#define BIT_MACT_MULTI2DOWN_UP    2
#define BIT_MACT_MULTI2CPU_UP     3
#define BIT_MACT_BROAD2DOWN_UP    4
#define BIT_MACT_BROAD2CPU_UP     5
#define BIT_MACT_UNI2UP_DOWN      8
#define BIT_MACT_UNI2CPU_DOWN     9
#define BIT_MACT_MULTI2UP_DOWN    10
#define BIT_MACT_MULTI2CPU_DOWN   11
#define BIT_MACT_BROAD2UP_DOWN    12
#define BIT_MACT_BROAD2CPU_DOWN   13
/* MAC_H16 */
#define BIT_MAC_DOWN_SHIFT       16
#define BIT_MAC_UP_SHIFT         17
#define BIT_FW2UP_DOWN_SHIFT      18
#define BIT_FW2CPU_DOWN_SHIFT     19
#define BIT_FW2DOWN_UP_SHIFT      20
#define BIT_FW2CPU_UP_SHIFT       21

/******************* macro definition  ******************/
#define ETH_NAME_LEN (5)
#define WRITE_CMD_BUF_LEN (100)
#define eth_sys_cmd_powerdown "echo 0x3>/proc/x5net/client_iface/work_mode"
#define eth_sys_cmd_normal "echo 0x4>/proc/x5net/client_iface/work_mode"

//#define ETH_CMD_PRINT

#define API_DEBUG printf

#ifdef ETH_CMD_PRINT
 #define eth_cmd_dbg( fmt, args... ) \
    do { \
        printf( "%s -%s(): %d Line: ", __FILE__, __FUNCTION__, __LINE__ ); \
        printf( fmt, ## args); \
    } while (0);
#else
 #define eth_cmd_dbg(fmt, args... )
#endif

#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif

/******************* struct definition ******************/

/******************* global variable   ******************/
const hi_char eth_name_array[2][ETH_NAME_LEN] =
{
    "eth0",
    "eth1"
};

static hi_s32 g_s32ETHFd[2] = {0, 0};

/******************* fuction declaration ****************/
static hi_char* skip_space(hi_char * line)
{
    hi_char * p = line;

    while ((*p == ' ') || (*p == '\t'))
    {
        p++;
    }

    return p;
}

static hi_char* skip_word(hi_char* line)
{
    hi_char * p = line;

    while ((*p != '\t') && (*p != ' ') && (*p != '\n') && (*p != 0))
    {
        p++;
    }

    return p;
}

static hi_char* get_word(hi_char* line, hi_char* value)
{
    hi_char * p = line;

    p = skip_space(p);
    while ((*p != '\t') && (*p != ' ') && (*p != '\n') && (*p != 0))
    {
        *value++ = *p++;
    }

    *value = 0;

    return p;
}

hi_s32 is_ip(hi_char *ipaddr)
{
    hi_char *pnum, *pdot = ipaddr;
    hi_s32 i = 0;

    for (; *ipaddr; ipaddr = pdot++, i++)
    {
        hi_s32 t = 0, e = 1;
        if (*(pnum = pdot) == '.')
        {
            return HI_FAILURE;
        }

        while ((*pdot != '.') && *pdot)
        {
            ++pdot;
        }

        ipaddr = pdot - 1;
        while (ipaddr >= pnum)
        {
            t += e * (*ipaddr-- - '0');
            e *= 10;
        }

        //if(t<0||t>255||(pdot-pnum==3&&t<100)||(pdot-pnum==2&&t<10))
        if ((t < 0) || (t > 255))
        {
            return HI_FAILURE;
        }
    }

    if (i != 4)
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 parse_mac(hi_char *dst, const hi_char *src)
{
    const hi_char *p = src;
    hi_char q[3];
    hi_s32 colon_cnt = 0;
    hi_s32 i = 0;

    if ((NULL == src) || (NULL == dst))
    {
        return HI_FAILURE;
    }

    while ('\0' != *p)
    {
        if (i >= 3)
        {
            return HI_FAILURE;
        }                             //too many chars between ':'
        if (':' == *p)
        {
            if (colon_cnt >= 5)
            {
                return HI_FAILURE;
            }                                       //too many ':'
            q[i] = '\0';
            dst[colon_cnt++] = strtoul(q, NULL, 0);
            i = 0;
        }
        else
        {
            q[i] = *p;
            i++;
        }

        p++;
    }

    if (i >= 3)
    {
        return HI_FAILURE;
    }                               //too many chars between ':' and '\0'
    q[i] = '\0';
    dst[colon_cnt] = strtoul(q, NULL, 0);
    if (colon_cnt != 5)
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/* sprintf mac addr */
static hi_char *pr_ether(hi_char *ptr)
{
    static hi_char buff[32];

    memset(buff, 0, 32);
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X",
             (ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
             (ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377)
    );
    return (buff);
}

/*----------------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_Open(Eth_Port_E ePort)
Description     :switch to powerdown mode
Calls           :
Data Accessed   :
Data Updated    :
Input           :NA
Output          :NA
Return          :NA
Others          :NA
*----------------------------------------------------------------------------------------*/
hi_s32 HI_ETH_Open(Eth_Port_E ePort)
{
    struct ifreq ifr;
    int sockfd;

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    g_s32ETHFd[ePort] = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    strcpy(ifr.ifr_name,eth_name_array[ePort]);
    if((ioctl(sockfd,SIOCGIFFLAGS,(caddr_t)&ifr))==-1)
    {
        perror("SIOCGIFFLAGS fail :");
        close(sockfd);
        return HI_FAILURE;
    }

    ifr.ifr_flags |= IFF_UP;
    if((ioctl(sockfd,SIOCSIFFLAGS,(caddr_t)&ifr))==-1)
    {
        perror("SIOCSIFFLAGS fail :");
        close(sockfd);
        return HI_FAILURE;
    }

    close(sockfd);
    return HI_SUCCESS;
}

/*----------------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_Close(Eth_Port_E ePort)
Description     :switch to powerdown mode
Calls           :
Data Accessed   :
Data Updated    :
Input           :NA
Output          :NA
Return          :NA
Others          :NA
*----------------------------------------------------------------------------------------*/
hi_s32 HI_ETH_Close(Eth_Port_E ePort)
{
    struct ifreq ifr;
    int sockfd;

    if (ePort <= ETH_PORT_MAX)
    {
        API_DEBUG("invalid parameter:%d\n",ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }
    g_s32ETHFd[ePort] = 0;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    strcpy(ifr.ifr_name,eth_name_array[ePort]);
    if ((ioctl(sockfd,SIOCGIFFLAGS,(caddr_t)&ifr))==-1)
    {
        perror("SIOCGIFFLAGS fail :");
        close(sockfd);
        return HI_FAILURE;
    }

    ifr.ifr_flags &= (~IFF_UP);
    if ((ioctl(sockfd,SIOCSIFFLAGS,(caddr_t)&ifr))==-1)
    {
        perror("SIOCSIFFLAGS fail :");
        close(sockfd);
        return HI_FAILURE;
    }

    close(sockfd);
    return HI_SUCCESS;
}

/*-----------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_IPAddressSet(Eth_Port_E ePort, HI_U8* ipAdd)
Description     :set ip address
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  ipAdd
Output          :NA
Return          :HI_SUCCESS/HI_FAILURE
Others          :NA
 *------------------------------------------------------------------------------*/
hi_s32 HI_ETH_IPAddressSet (Eth_Port_E ePort, hi_char* ipAdd)
{
    hi_s32 sockfd;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    if (NULL == ipAdd)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (is_ip(ipAdd) != 0)
    {
        API_DEBUG("invalid ip address:%s\n", ipAdd);
        return HI_FAILURE;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth_name_array[ePort], sizeof(ifr.ifr_name) - 1);
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    sin->sin_family = AF_INET;

    if (inet_pton(AF_INET, ipAdd, &sin->sin_addr) <= 0)  // fix: change from < to <= for invalid ipAddr input
    {
        API_DEBUG("inet_pton  fail!\n");
        close(sockfd);
        return HI_FAILURE;
    }

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0)
    {
        API_DEBUG("set ipaddr fail!\n");
        close(sockfd);
        return HI_FAILURE;
    }

    close(sockfd);
    return HI_SUCCESS;
}

/*-----------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_IPAddressGet (Eth_Port_E ePort, HI_S8* ipAdd)
Description     :get ipaddress from ePort and save to ipAdd buffer
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  ipAdd
Output          :ipAdd
Return          :HI_SUCCESS/HI_FAILURE
Others          :NA
 *------------------------------------------------------------------------------*/
hi_s32 HI_ETH_IPAddressGet (Eth_Port_E ePort, hi_char *ipAdd)
{
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *s_in;

    if (NULL == ipAdd)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, eth_name_array[ePort]);
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
    {
        API_DEBUG("ioctl fail!\n");
        close(sockfd);
        return HI_FAILURE;
    }

    s_in = (struct sockaddr_in *)(&ifr.ifr_addr);
    memcpy((void *)ipAdd, inet_ntoa(s_in->sin_addr), 15);

    close(sockfd);
    return HI_SUCCESS;
}

/*--------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_SubNetmaskSet(Eth_Port_E ePort, HI_S8* subNetmask)
Description     :set subnetmask for ePort,the value get from subNetmask pointer
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  subNetmask
Output          :subNetmask
Return          :HI_SUCCESS
                :HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT
                :HI_ERR_ETH_INVALID_PARA
Others          :
*--------------------------------------------------------------------------------*/
hi_s32 HI_ETH_SubNetmaskSet (Eth_Port_E ePort, hi_char* subNetmask)
{
    hi_s32 sockfd;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    if (NULL == subNetmask)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if (is_ip(subNetmask) != 0)
    {
        API_DEBUG("invalid subNetmask:%s\n", subNetmask);
        return HI_FAILURE;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth_name_array[ePort], sizeof(ifr.ifr_name) - 1);
    sin = (struct sockaddr_in *)&ifr.ifr_netmask;
    sin->sin_family = AF_INET;

    if (inet_pton(AF_INET, subNetmask, &sin->sin_addr) < 0)
    {
        API_DEBUG("inet_pton  fail!\n");
        close(sockfd);
        return HI_FAILURE;
    }

    if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0)
    {
        API_DEBUG("set netmask fail!\n");
        close(sockfd);
        return HI_FAILURE;
    }

    close(sockfd);
    return HI_SUCCESS;
}

/*--------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_SubNetmaskGet(Eth_Port_E ePort, HI_U8* subNetmask)
Description     :get subnetmask from ePort,save to subNetmask buffer
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  subNetmask
Output          :subNetmask
Return          :HI_SUCCESS
                :HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT
                :HI_ERR_ETH_INVALID_PARA
Others          :NA
*--------------------------------------------------------------------------------*/
hi_s32 HI_ETH_SubNetmaskGet (Eth_Port_E ePort, hi_char* subNetmask)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *s_in;

    if (NULL == subNetmask)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, eth_name_array[ePort]);
    if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0)
    {
        API_DEBUG("ioctl fail!\n");
        close(skfd);
        return HI_FAILURE;
    }

    s_in = (struct sockaddr_in *)(&ifr.ifr_netmask);

    memcpy((void *)subNetmask, inet_ntoa(s_in->sin_addr), 15);

    close(skfd);
    return HI_SUCCESS;
}

/*--------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_GatewaySet(Eth_Port_E ePort, HI_U8* gateway)
Description     :set gateway for ePort
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  gateway
Output          :subNetmask
Return          :HI_SUCCESS
                :HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT
                :HI_ERR_ETH_INVALID_PARA
Others          :NA
*--------------------------------------------------------------------------------*/
hi_s32 HI_ETH_GatewaySet (Eth_Port_E ePort, hi_char* gateway)
{
    hi_s32 sockfd;
    struct rtentry rm;

    if (NULL == gateway)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n",ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if (is_ip(gateway) != 0)
    {
        API_DEBUG("invalid gateway:%s\n",gateway);
        return HI_FAILURE;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    memset(&rm, 0, sizeof(struct rtentry));

    (( struct sockaddr_in*)&rm.rt_dst)->sin_family = AF_INET;
    (( struct sockaddr_in*)&rm.rt_dst)->sin_addr.s_addr = 0;
    (( struct sockaddr_in*)&rm.rt_dst)->sin_port = 0;

    (( struct sockaddr_in*)&rm.rt_genmask)->sin_family = AF_INET;
    (( struct sockaddr_in*)&rm.rt_genmask)->sin_addr.s_addr = 0;
    (( struct sockaddr_in*)&rm.rt_genmask)->sin_port = 0;

    (( struct sockaddr_in*)&rm.rt_gateway)->sin_family = AF_INET;
    (( struct sockaddr_in*)&rm.rt_gateway)->sin_addr.s_addr = inet_addr(gateway);
    (( struct sockaddr_in*)&rm.rt_gateway)->sin_port = 0;

    rm.rt_flags = RTF_UP | RTF_GATEWAY;
    if (( ioctl(sockfd, SIOCADDRT, &rm)) < 0)
    {
        perror("SIOCADDRT fail :");
        close(sockfd);
        return HI_FAILURE;
    }

    close(sockfd);
    return HI_SUCCESS;

}

/*--------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_GatewayGet(Eth_Port_E ePort, HI_U8* gateway)
Description     :set gateway for ePort
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  gateway
Output          :gateway
Return          :HI_SUCCESS
                :HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT
                :HI_ERR_ETH_INVALID_PARA
Others          :NA
*--------------------------------------------------------------------------------*/
hi_s32 HI_ETH_GatewayGet (Eth_Port_E ePort, hi_char* gateway)
{
    hi_char line[512];
    hi_char str[16];
    FILE* fp;

    if (NULL == gateway)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if (gateway == NULL)
    {
        strcpy(gateway, "0.0.0.0");
        return HI_FAILURE;
    }

    fp = fopen("/proc/net/route", "r");
    if (fp)
    {
        memset(str, 0, sizeof(str));
        while (fgets(line, sizeof(line), fp) != 0)
        {
            char* p = line;
            p = strstr(p, eth_name_array[ePort]);
            if (p)
            {
                p = get_word(p, str);
                p = get_word(p, str);
                if (strcmp(str, "00000000") == 0)
                {
                    p = get_word(p, str);
                    if (strcmp(str, "00000000") != 0)
                    {
                        int a, b, c, d;
                        sscanf(str, "%02X%02X%02X%02X", &a, &b, &c, &d);
                        sprintf(gateway, "%d.%d.%d.%d", d, c, b, a);
                        fclose(fp);
                        return HI_SUCCESS;
                    }
                }
            }
        }

        fclose(fp);
    }

    strcpy(gateway, "0.0.0.0");
    return HI_FAILURE;
}


/*----------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_DNSSet (Eth_Port_E ePort, hi_bool enable, HI_S8* dns)
Description     :set dns for ePort
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  enable dns
Output          :dns
Return          :HI_SUCCESS
                :HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT
                :HI_ERR_ETH_INVALID_PARA
Others          :NA
 *-----------------------------------------------------------------------------------*/
hi_s32 HI_ETH_DNSSet (Eth_Port_E ePort, hi_bool enable, hi_char *dns)
{
    FILE* fp;

    if (NULL == dns)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if (is_ip(dns) != 0)
    {
        return HI_FAILURE;
    }

    fp = fopen("/etc/resolv.conf", "w");
    if (fp)
    {
        fprintf(fp, "nameserver %s", dns);
        fclose(fp);
        return HI_SUCCESS;
    }

    return HI_FAILURE;
}

/*--------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_DNSGet (Eth_Port_E ePort, HI_U8* dns)
Description     :set dns for ePort
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  dns
Output          :subNetmask
Return          :HI_SUCCESS
                :HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT
                :HI_ERR_ETH_INVALID_PARA
Others          :NA
*--------------------------------------------------------------------------------*/
hi_s32 HI_ETH_DNSGet (Eth_Port_E ePort, hi_char *dns)
{
    hi_char line[100];
    hi_char str[16];
    FILE* fp;

    if (NULL == dns)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    memset(str, 0, 16);
    fp = fopen("/etc/resolv.conf", "r");
    if (fp)
    {
        while (fgets(line, 100, fp) != 0)
        {
            char* p;
            p = strstr(line, "nameserver");
            if (p)
            {
                p = skip_word(p);
                get_word(p, str);
                break;
            }
        }

        fclose(fp);
    }

    if (strlen(str) == 0)
    {
        strcpy(dns, "0.0.0.0");
        return HI_FAILURE;
    }
    else
    {
        strcpy(dns, str);
        return HI_SUCCESS;
    }
}

/*-----------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_SetMac(Eth_Port_E ePort, HI_U8 *mac)
Description     :set mac address
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  mac
Output          :NA
Return          :HI_SUCCESS/HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT/HI_ERR_ETH_INVALID_PARA
Others          :NA
 *------------------------------------------------------------------------------*/
hi_s32 HI_ETH_SetMac(Eth_Port_E ePort, hi_char *mac)
{
    hi_s32 sockfd;
    struct ifreq ifr;
    hi_char mac_real[6];
    hi_s32 i;

    if (NULL == mac)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n",ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if (HI_SUCCESS != parse_mac(mac_real, mac))
    {
        API_DEBUG("invalid parameter:%s\n",mac);
        return HI_ERR_ETH_INVALID_PARA;
    }

    printf("set mac: ");
    for (i = 0; i < 6; i++)
    {
        printf("%x  ",mac_real[i]);
    }
    printf("\n");

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("socket create fail!\n");
        return HI_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, eth_name_array[ePort]);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, mac_real, 6);

    /* Before set HW MAC addresss, down the ethernet port firstly */
    HI_ETH_Close(ePort);

    if (ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0)
    {
        perror("SIOCSIFHWADDR fail :");
        close(sockfd);
        return HI_FAILURE;
    }

    /* Restore the ethernet port to up status */
    HI_ETH_Open(ePort);

    close(sockfd);
    return HI_SUCCESS;
}

/*-----------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_GetMac(Eth_Port_E ePort, HI_U8 *mac)
Description     :get mac address
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  mac
Output          :NA
Return          :HI_SUCCESS/HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT/HI_ERR_ETH_INVALID_PARA
Others          :NA
 *------------------------------------------------------------------------------*/
hi_s32 HI_ETH_GetMac(Eth_Port_E ePort, hi_char *mac)
{
    int skfd;
    struct ifreq ifr;
    hi_char buf[16];

    if (NULL == mac)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        API_DEBUG("socket create error!\n");
        return HI_FAILURE;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, eth_name_array[ePort]);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        perror("SIOCGIFHWADDR fail :");
        close(skfd);
        return HI_FAILURE;
    }

    memset(buf, 0, 16);
    memcpy(buf, ifr.ifr_hwaddr.sa_data, 8);
    memcpy(mac, pr_ether(buf), strlen(pr_ether(buf)));

    close(skfd);
    return HI_SUCCESS;
}

/*------------------------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_GetLinkStatus(Eth_Port_E ePort, Eth_LinkStatus_E *ptrLinkStatus)
Description     :get link status for ePort
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort  ptrLinkStatus
Output          :ptrLinkStatus
Return          :HI_SUCCESS/HI_FAILURE
                :HI_ERR_ETH_INVALID_POINT/HI_ERR_ETH_INVALID_PARA
Others          :NA
 *-------------------------------------------------------------------------------------------------*/
hi_s32 HI_ETH_GetLinkStatus(Eth_Port_E ePort, Eth_LinkStatus_E *ptrLinkStatus)
{
    hi_s32 sockfd;
    struct ifreq ifr;

    if (NULL == ptrLinkStatus)
    {
        API_DEBUG("null pointer\n");
        return HI_ERR_ETH_INVALID_POINT;
    }

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("create socket failed\n");
        return HI_FAILURE;
    }

    bzero((char *)&ifr, sizeof(ifr));
    strcpy(ifr.ifr_name, eth_name_array[ePort]);
    if (ioctl(sockfd, SIOCGIFFLAGS, (char *)&ifr) < 0)
    {
        close(sockfd);
        return HI_FAILURE;
    }

    if ((ifr.ifr_flags & IFF_RUNNING) == 0)
    {
        *ptrLinkStatus = ETH_LINK_STATUS_OFF;
    }
    else
    {
        *ptrLinkStatus = ETH_LINK_STATUS_ON;
    }

    close(sockfd);
    return HI_SUCCESS;
}

/*----------------------------------------------------------------------------------------*
Function        :hi_s32 HI_ETH_LinkCfg(Eth_Port_E ePort, Eth_LinkCfg_E eLinkForce)
Description     :force link config between sf and eth devices,as following:
                 ETH_LinkCfg_10M_HALF
                 ETH_LinkCfg_10M_FULL
                 ETH_LinkCfg_100M_HALF
                 ETH_LinkCfg_100M_FULL
Calls           :
Data Accessed   :
Data Updated    :
Input           :ePort eLinkCfg
Output          :NA
Return          :HI_ERR_ETH_INVALID_PARA
                :HI_SUCCESS
Others          :NA
*----------------------------------------------------------------------------------------*/
hi_s32 HI_ETH_LinkCfg(Eth_Port_E ePort, Eth_LinkCfg_E eLinkCfg)
{
    hi_s32 sockfd;
    struct ifreq ifr;
    struct ethtool_cmd ecmd;

    if (ETH_PORT_MAX <= ePort)
    {
        API_DEBUG("invalid parameter:%d\n", ePort);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if (ETH_LinkCfg_MAX <= eLinkCfg)
    {
        API_DEBUG("invalid parameter:%d\n", eLinkCfg);
        return HI_ERR_ETH_INVALID_PARA;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        API_DEBUG("create socket failed\n");
        return HI_FAILURE;
    }

    bzero((char *)&ifr, sizeof(ifr));
    strcpy(ifr.ifr_name, eth_name_array[ePort]);

    ecmd.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t)&ecmd;
    if (ioctl(sockfd, SIOCETHTOOL, (char *)&ifr) < 0)
    {
        close(sockfd);
        return HI_FAILURE;
    }

    /*TODO: set ecmd */
    switch (eLinkCfg)
    {
        case ETH_LinkCfg_10M_HALF :
            ecmd.speed = 10;   /* SPEED_10 */
            ecmd.speed_hi = 0;
            ecmd.duplex = 0;   /* DUPLEX_HALF */
            ecmd.autoneg = 0;  /* AUTONEG_DISABLE */
            break;
        case ETH_LinkCfg_10M_FULL:
            ecmd.speed = 10;
            ecmd.speed_hi = 0;
            ecmd.duplex = 1;   /* DUPLEX_FULL */
            ecmd.autoneg = 0;
            break;
        case ETH_LinkCfg_100M_HALF :
            ecmd.speed = 100;   /* SPEED_100 */
            ecmd.speed_hi = 0;
            ecmd.duplex = 0;
            ecmd.autoneg = 0;
            break;
        case ETH_LinkCfg_100M_FULL:
            ecmd.speed = 100;   /* SPEED_100 */
            ecmd.speed_hi = 0;
            ecmd.duplex = 1;
            ecmd.autoneg = 0;
            break;
        case ETH_LinkCfg_AUTONEG:
        default:
            ecmd.advertising = ecmd.supported;
            ecmd.autoneg = 1;     /* AUTONEG_ENABLE */
    }

    ecmd.cmd = ETHTOOL_SSET;
    ifr.ifr_data = (caddr_t)&ecmd;
    if (ioctl(sockfd, SIOCETHTOOL, (char *)&ifr) < 0)
    {
        close(sockfd);
        return HI_FAILURE;
    }

    close(sockfd);
    return HI_SUCCESS;
}
