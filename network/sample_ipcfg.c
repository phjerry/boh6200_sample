#include <stdio.h>
#include <string.h>

#include "hi_type.h"
#include "hi_api_eth.h"

const hi_char eth_name[2][16] = {
    "eth0",
    "eth1"
};

int main(hi_s32 argc, hi_char *argv[])
{
    Eth_LinkStatus_E linkflag;
    hi_char ipaddr[16];
    hi_char ipmask[16];
    hi_char getway[16];
    hi_char dns[16];
    hi_char mac[18];
    hi_s32 ret;
    Eth_Port_E eth = ETH_PORT_UP;

    if (argc == 2) {
        if (strcmp(argv[1],"eth0") == 0) {
            eth = ETH_PORT_UP;
        } else if (strcmp(argv[1],"eth1") == 0) {
            eth = ETH_PORT_DOWN;
        }
    }

    ret = HI_ETH_GetLinkStatus(eth, &linkflag);
    if (ret != HI_SUCCESS) {
        printf("get link status failed\n");
        return -1;
    }

    if (linkflag == ETH_LINK_STATUS_OFF) {
        printf("%s Port Is not link\n", eth_name[eth]);
        return 0;
    } else {
        printf(" %s Port Is link\n", eth_name[eth]);
    }

    ret = HI_ETH_Open(eth);
    if (ret != HI_SUCCESS) {
        printf("HI_ETH_Open failed\n");
        return -1;
    }

    memset(ipaddr, 0, sizeof(ipaddr));

    ret = HI_ETH_IPAddressGet(eth, ipaddr);
    if (ret != HI_SUCCESS) {
        strcpy(ipaddr, "10.85.180.59");
        ret = HI_ETH_IPAddressSet(eth, ipaddr);
    }

    printf("ipaddr = %s\n", ipaddr);

    memset(ipmask, 0, sizeof(ipmask));

    ret = HI_ETH_SubNetmaskGet(eth, ipmask);
    if (ret != HI_SUCCESS) {
        strcpy(ipmask, "255.255.254.0");
        ret = HI_ETH_SubNetmaskSet(eth, ipmask);
    }

    printf("ipmask = %s\n", ipmask);

    memset(getway, 0, sizeof(getway));

    ret = HI_ETH_GatewayGet(eth, getway);
    if (ret != HI_SUCCESS) {
        strcpy(getway, "10.85.180.1");
        ret = HI_ETH_GatewaySet(eth, getway);
    }

    printf("getway = %s\n", getway);

    memset(dns, 0, sizeof(dns));

    ret = HI_ETH_DNSGet(eth, dns);
    if (ret != HI_SUCCESS) {
        strcpy(dns, "10.73.55.101");
        ret = HI_ETH_DNSSet(eth, HI_TRUE, dns);
    }

    printf("dns = %s\n", dns);

    memset(mac, 0, sizeof(mac));

    ret = HI_ETH_GetMac(eth, mac);
    if (ret != HI_SUCCESS) {
        printf("HI_ETH_GetMac failed\n");
        return -1;
    }

    printf("mac = %s\n", mac);

    (void)HI_ETH_SetMac(eth, "00:01:02:03:04:05");

    memset(mac, 0, sizeof(mac));

    ret = HI_ETH_GetMac(eth, mac);
    if (ret != HI_SUCCESS) {
        printf("HI_ETH_GetMac failed\n");
        return -1;
    }

    printf("set mac = 00:01:02:03:04:05 get mac = %s\n", mac);

    return 1;
}
