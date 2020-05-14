
#ifdef __cplusplus
extern "C" {
#endif

/* eth */
typedef enum {
    ETH_LINK_STATUS_OFF = 0,
    ETH_LINK_STATUS_ON,
    ETH_LINK_STATUS_MAX
} Eth_LinkStatus_E;

typedef enum {
    ETH_PORT_UP = 0,
    ETH_PORT_DOWN,
    ETH_PORT_MAX
} Eth_Port_E;

typedef enum {
    ETH_WORKMODE_UMDS = 0,
    ETH_WORKMODE_DMUS,
    ETH_WORKMODE_UMDM,
    ETH_WORKMODE_MAX
} Eth_WorkMode_E;

typedef enum {
    ETH_LinkCfg_10M_HALF = 0,
    ETH_LinkCfg_10M_FULL,
    ETH_LinkCfg_100M_HALF,
    ETH_LinkCfg_100M_FULL,
    ETH_LinkCfg_AUTONEG,
    ETH_LinkCfg_MAX
} Eth_LinkCfg_E;

typedef enum {
    ETH_MACTBL_INDEX_0 = 0,
    ETH_MACTBL_INDEX_1,
    ETH_MACTBL_INDEX_2,
    ETH_MACTBL_INDEX_3,
    ETH_MACTBL_INDEX_4,
    ETH_MACTBL_INDEX_5,
    ETH_MACTBL_INDEX_6,
    ETH_MACTBL_INDEX_7,
    ETH_MACTBL_INDEX_MAX
} Eth_MacTblIdx_E;

typedef enum {
    ETH_VLANTBL_INDEX_0 = 0,
    ETH_VLANTBL_INDEX_1,
    ETH_VLANTBL_INDEX_2,
    ETH_VLANTBL_INDEX_3,
    ETH_VLANTBL_INDEX_4,
    ETH_VLANTBL_INDEX_5,
    ETH_VLANTBL_INDEX_6,
    ETH_VLANTBL_INDEX_7,
    ETH_VLANTBL_INDEX_MAX
} Eth_VlanTblIdx_E;

typedef enum {
    ETH_FW_ALL2CPU_D = 0,
    ETH_FW_ALL2UP_D,
    ETH_FW_ENA2CPU_D,
    ETH_FW_ENA2UP_D,
    ETH_FW_ALL2CPU_U,
    ETH_FW_ALL2UP_U,
    ETH_FW_ENA2CPU_U,
    ETH_FW_ENA2UP_U,
    ETH_FW_REG_CFG,
    ETH_FW_MAX
} Eth_Forward_E;

typedef enum {
    ETH_MACT_BROAD2CPU_D = 0,
    ETH_MACT_BROAD2UP_D,
    ETH_MACT_MULTI2CPU_D,
    ETH_MACT_MULTI2UP_D,
    ETH_MACT_UNI2CPU_D,
    ETH_MACT_UNI2UP_D,
    ETH_MACT_BROAD2CPU_U,
    ETH_MACT_BROAD2DOWN_U,
    ETH_MACT_MULTI2CPU_U,
    ETH_MACT_MULTI2DOWN_U,
    ETH_MACT_UNI2CPU_U,
    ETH_MACT_UNI2DOWN_U,
    ETH_MACT_REG_CFG,
    ETH_MACT_MAX
} Eth_Mactctrl_E;

hi_s32	HI_ETH_Open(Eth_Port_E ePort);
hi_s32	HI_ETH_Close(Eth_Port_E ePort);
hi_s32	HI_ETH_IPAddressSet(Eth_Port_E ePort, hi_char* ipAdd);
hi_s32	HI_ETH_IPAddressGet(Eth_Port_E ePort, hi_char *ipAdd);
hi_s32	HI_ETH_SubNetmaskSet(Eth_Port_E ePort, hi_char* subNetmask);
hi_s32	HI_ETH_SubNetmaskGet(Eth_Port_E ePort, hi_char* subNetmask);
hi_s32	HI_ETH_GatewaySet(Eth_Port_E ePort, hi_char* gateway);
hi_s32	HI_ETH_GatewayGet(Eth_Port_E ePort, hi_char* gateway);
hi_s32	HI_ETH_DNSSet(Eth_Port_E ePort, hi_bool enable, hi_char *dns);
hi_s32	HI_ETH_DNSGet(Eth_Port_E ePort, hi_char *dns);
hi_s32	HI_ETH_SetMac(Eth_Port_E ePort, hi_char *mac);
hi_s32	HI_ETH_GetMac(Eth_Port_E ePort, hi_char *mac);
hi_s32	HI_ETH_GetLinkStatus(Eth_Port_E ePort, Eth_LinkStatus_E *ptrLinkStatus);
hi_s32	HI_ETH_LinkCfg(Eth_Port_E ePort, Eth_LinkCfg_E eLinkCfg);
hi_s32	HI_ETH_WorkModeSet(Eth_WorkMode_E eWorkMode);

#ifdef __cplusplus
}
#endif
