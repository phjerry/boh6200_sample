/*****************************************************************************
 *                                                                           *
 * INVECAS CONFIDENTIAL                                                      *
 * ____________________                                                      *
 *                                                                           *
 * [2018] - [2023] INVECAS, Incorporated                                     *
 * All Rights Reserved.                                                      *
 *                                                                           *
 * NOTICE:  All information contained herein is, and remains                 *
 * the property of INVECAS, Incorporated, its licensors and suppliers,       *
 * if any.  The intellectual and technical concepts contained                *
 * herein are proprietary to INVECAS, Incorporated, its licensors            *
 * and suppliers and may be covered by U.S. and Foreign Patents,             *
 * patents in process, and are protected by trade secret or copyright law.   *
 * Dissemination of this information or reproduction of this material        *
 * is strictly forbidden unless prior written permission is obtained         *
 * from INVECAS, Incorporated.                                               *
 *                                                                           *
 *****************************************************************************/
/**
* file inv478x_ipc_def.h
*
* brief IPC message handler
*
*****************************************************************************/

#ifndef INV478x_IPC_DEF_H
#define INV478x_IPC_DEF_H

/***** public macro definitions **********************************************/

/*****************************************************************************/
/***** driver general management */
/*****************************************************************************/

#define INV478X_CHIPID_QUERY                            0x0001
#define INV478X_CHIPREVISION_QUERY                      0x0002
#define INV478X_PRODUCTID_QUERY                         0x0003
#define INV478X_FIRMWAREVERSION_QUERY                   0x0004
#define INV478X_EVENTFLAGSMASK_SET                      0x0005
#define INV478X_EVENTFLAGSMASK_GET                      0x0006
#define INV478X_EVENT_FLAG_STAT_CLEAR                   0x0007
#define INV478X_PWR_DOWN_REQ                            0x0008
#define INV478X_FREEZE_SET                              0x0009
#define INV478X_FREEZE_GET                              0x000A
#define INV478X_BOOT_STATUS_QUERY                       0x000B
#define INV478X_EVENT_FLAG_STAT_QUERY                   0x000C
#define INV478X_DL_SWEEP_SET                            0x000E
#define INV478X_DL_SWEEP_GET                            0x000F
#define INV478X_CHIP_SERIAL_QUERY                       0x0010
#define INV478X_LICENSE_CERT_SET                        0x0011
#define INV478X_LICENSE_ID_QUERY                        0x0012
#define INV478X_DL_MODE_SET                             0x0018
#define INV478X_DL_MODE_GET                             0x0019
#define INV478X_DL_SPLIT_OVERLAP_SET                    0x001A
#define INV478X_DL_SPLIT_OVERLAP_GET                    0x001B
#define INV478X_HDCP_KSV_BIG_ENDIAN_SET                 0x001C
#define INV478X_HDCP_KSV_BIG_ENDIAN_GET                 0x001D
#define INV478X_REG_WRITE                               0x001E
#define INV478X_REG_READ                                0x001F
#define INV478X_HEART_BEAT_SET                          0x0020
#define INV478X_HEART_BEAT_GET                          0x0021
#define INV478X_GPIO_ENABLE_SET                         0x0026
#define INV478X_GPIO_ENABLE_GET                         0x0027
#define INV478X_GPIO_EVT_MASK_SET                       0x0028
#define INV478X_GPIO_EVT_MASK_GET                       0x0029
#define INV478X_GPIO_EVT_STAT_QUERY                     0x002A
#define INV478X_GPIO_QUERY                              0x002B
#define INV478X_GPIO_BIT_CLR                            0x002C
#define INV478X_GPIO_BIT_SET                            0x002D
#define INV478X_GPIO_BIT_GET                            0x002E
#define INV478X_GPIO_SET                                0x002F
#define INV478X_GPIO_GET                                0x0030
#define INV478X_GPIO_MODE_SET                           0x0031
#define INV478X_GPIO_MODE_GET                           0x0032
#define INV478X_FIRMWAREUSERTAG_QUERY                   0x0033

/*****************************************************************************/
/***** test pattern generator(TPG) */
/*****************************************************************************/

#define INV478X_VTPGENABLE_SET                           0x0214
#define INV478X_VTPGENABLE_GET                           0x0215
#define INV478X_VTPGPATTERN_SET                          0x0216
#define INV478X_VTPGPATTERN_GET                          0x0217
#define INV478X_VTPGVIDFRM_SET                           0x0218
#define INV478X_VTPGVIDFRM_GET                           0x0219


#define INV478X_CEC_ARC_INIT_NOTFY                      0x021C
#define INV478X_CEC_ARC_TERM_NOTFY                      0x021D

/*****************************************************************************/
/***** miscellaneous */
/*****************************************************************************/

#define INV478X_SCDCRXCONFIG_QUERY                      0x021E
#define INV478X_SCDCTX0CONFIG_QUERY                     0x021F

/****************************************************
*AP0/AP1 related
*****************************************************/

#define INV478X_AP_MODE_SET                             0x0420
#define INV478X_AP_MODE_GET                             0x0421
#define INV478X_AP_TX_SRC_SET                           0x0422
#define INV478X_AP_TX_SRC_GET                           0x0423
#define INV478X_AP_RX_FMT_SET                           0x0424
#define INV478X_AP_RX_FMT_GET                           0x0425
#define INV478X_AP_RX_INFO_SET                          0x0426
#define INV478X_AP_RX_INFO_GET                          0x0427
#define INV478X_AP_TX_FMT_QUERY                         0x0428
#define INV478X_AP_TX_INFO_QUERY                        0x0429
#define INV478X_AP_TX_MUTE_SET                          0x042A
#define INV478X_AP_TX_MUTE_GET                          0x042B
#define INV478X_EARC_MODE_SET                           0x042C
#define INV478X_EARC_MODE_GET                           0x042D
#define INV478X_EARC_AUD_FMT_QUERY                      0x042E
#define INV478X_EARC_AUD_INFO_QUERY                     0x042F
#define INV478X_EARC_LINK_QUERY                         0x0430
#define INV478X_EARC_LATENCY_SET                        0x0431
#define INV478X_EARC_LATENCY_GET                        0x0432
#define INV478X_EARC_LATENCY_QUERY                      0x0433
#define INV478X_EARC_RX_RXCAP_STRUCT_SET                0x0434
#define INV478X_EARC_RX_RXCAP_STRUCT_GET                0x0435
#define INV478X_EARC_TX_RX_CAP_STRUCT_QUERY             0x0436
#define INV478X_EARC_HDMI_HPD_SET                       0x0437
#define INV478X_EARC_HDMI_HPD_GET                       0x0438
#define INV478X_EARC_EARC_HPD_SET                       0x0439
#define INV478X_EARC_EARC_HPD_GET                       0x043A
#define INV478X_EARC_EARC_HPD_LOW_TIME_SET              0x043B
#define INV478X_EARC_EARC_HPD_LOW_TIME_GET              0x043C
#define INV478X_EARC_PACKET_SET                         0x043D
#define INV478X_EARC_PACKET_GET                         0x043E
#define INV478X_EARC_PACKET_QUERY                       0x043F
#define INV478X_AP_MCLK_SET                             0x0440
#define INV478X_AP_MCLK_GET                             0x0441
#define INV478X_ATPG_FMT_SET                             0x0442
#define INV478X_ATPG_FMT_GET                             0x0443
#define INV478X_ATPG_AMPLITUDE_SET                       0x0444
#define INV478X_ATPG_AMPLITUDE_GET                       0x0445
#define INV478X_ATPG_FREQUENCY_SET                       0x0446
#define INV478X_ATPG_FREQUENCY_GET                       0x0447
#define INV478X_ATPG_PATTERN_SET                         0x0448
#define INV478X_ATPG_PATTERN_GET                         0x0449
#define INV478X_ATPG_CHANNEL_MASK_SET                    0x044C
#define INV478X_ATPG_CHANNEL_MASK_GET                    0x044D
#define INV478X_AP_CONFIG_SET                           0x044E
#define INV478X_AP_CONFIG_GET                           0x044F
#define INV478X_EARC_CONFIG_SET                         0x0450
#define INV478X_EARC_CONFIG_GET                         0x0451
#define INV478X_ATPG_SRC_MCLK_SET                        0x0452
#define INV478X_ATPG_SRC_MCLK_GET                        0x0453
#define INV478X_ATPG_FS_SET                              0x0454
#define INV478X_ATPG_FS_GET                              0x0455

/*****************************************************************************/
/***** A/V up-stream management */
/*****************************************************************************/

#define INV478X_PLUS5V_QUERY                            0x0640
#define INV478X_INPUTSELECT_SET                         0x0641
#define INV478X_INPUTSELECT_GET                         0x0642
#define INV478X_EDIDRX_SET                              0x0643
#define INV478X_EDIDRX_GET                              0x0644
#define INV478X_RX_HPD_ENABLE_SET                       0x0645
#define INV478X_RX_HPD_ENABLE_GET                       0x0646
#define INV478X_FASTSWITCH_SET                          0x0647
#define INV478X_FASTSWITCH_GET                          0x0648
#define INV478X_AVLINKRX_QUERY                          0x0649
#define INV478X_VIDEOTIMING_QUERY                       0x064A
#define INV478X_AUTOSPLITCHRRATE_SET                    0x064B
#define INV478X_AUTOSPLITCHRRATE_GET                    0x064C
#define INV478X_ARC_SUPPORT_MODE_SET                    0x064D
#define INV478X_ARC_SUPPORT_MODE_GET                    0x064E
#define INV478X_AUDIOFMT_QUERY                          0x064F
#define INV478X_AVMUTE_QUERY                            0x0650
#define INV478X_RX_DEEPCOLOR_QUERY                      0x0651
#define INV478X_RX_PIXEL_FMT_QUERY                      0x0652
#define INV478X_HPD_REPLICATE_SET                       0x0653
#define INV478X_HPD_REPLICATE_GET                       0x0654
#define INV478X_EDID_REPLICATE_SET                      0x0655
#define INV478X_EDID_REPLICATE_GET                      0x0656
#define INV478X_CHANNEL_STATUS_SET                      0x0657
#define INV478X_CHANNEL_STATUS_GET                      0x0658
#define INV478X_AIF_SET                                 0x0659
#define INV478X_AIF_GET                                 0x065A
#define INV478X_EARC_TX_AUD_SRC_SET                     0x065B
#define INV478X_EARC_AUDIO_MUTE_SET                     0x065C
#define INV478X_EARC_AUDIO_MUTE_GET                     0x065D
#define INV478X_EARC_AUD_INSERTION_MODE_SET             0x065E
#define INV478X_EARC_STAND_BY_SET                       0x0661
#define INV478X_EARC_STAND_BY_GET                       0x0662
#define INV478X_I2S_AUD_SRC_SET                         0x0663
#define INV478X_RX_HPD_PHY_QUERY                        0x0664
#define INV478X_RX_HPD_LOW_TIME_SET                     0x0665
#define INV478X_RX_HPD_LOW_TIME_GET                     0x0666
#define INV478X_RX_RSEN_ENABLE_SET                      0x0667
#define INV478X_RX_RSEN_ENABLE_GET                      0x0668
#define INV478X_RX_VIDEO_FMT_QUERY                      0x0669

#define INV478X_RX_EDID_ENABLE_SET                      0x066B
#define INV478X_RX_EDID_ENABLE_GET                      0x066C
/*#define INV478x_RX_EDID_CEC_ADDR_SET                  0x066D *
* #define INV478x_RX_EDID_CEC_ADDR_GET                  0x066E */
#define INV478X_RX_AVMUTE_QUERY                         0x066F
#define INV478X_RX_PLUS5V_QUERY                         0x0670
/*#define INV478x_RX_HDCP_CAPABILITY_SET                0x0671*
*#define INV478x_RX_HDCP_CAPABILITY_GET                 0x0672*/
#define INV478X_RX_HDCP_RPTR_SET                        0x0673
#define INV478X_RX_HDCP_RPTR_GET                        0x0674
#define INV478X_RX_HDCP_BKSV_QUERY                      0x0675
#define INV478X_RX_HDCP_RXID_QUERY                      0x0676
#define INV478X_RX_HDCP_AKSV_QUERY                      0x0677
#define INV478X_RX_HDCP_STAT_QUERY                      0x0678
#define INV478X_RX_AUDIO_FMT_QUERY                      0x0679
#define INV478X_RX_AUDIO_INFO_QUERY                     0x067A
#define INV478X_RX_AUDIO_CSTN_QUERY                     0x067B
#define INV478X_RX_SCDC_REGISTER_QUERY                  0x067C
#define INV478X_RX_PACKET_QUERY                         0x067D
#define INV478X_RX_HDCP_MODE_SET                        0x067E
#define INV478X_RX_HDCP_MODE_GET                        0x067F
#define INV478X_RX_HDCP_TRASH_REQ                       0x0680
#define INV478X_RX_HDCP_TYPE_QUERY                      0x0681
#define INV478X_RX_PACKET_ACT_STAT_QUERY                0x0682
#define INV478X_RX_PACKET_EVT_STAT_QUERY                0x0683
#define INV478X_RX_PACKET_EVT_MASK_SET                  0x0684
#define INV478X_RX_PACKET_EVT_MASK_GET                  0x0685
#define INV478X_RX_HDCP_CSM_QUERY                       0x0686
#define INV478X_RX_RSEN_VALUE_SET                       0x0687
#define INV478X_RX_RSEN_VALUE_GET                       0x0688
#define INV478X_RX_EDID_AUTO_ADB_SET                    0x0689
#define INV478X_RX_EDID_AUTO_ADB_GET                    0x068A
#define INV478X_RX_HDCP_CAP_SET                         0x068B
#define INV478X_RX_HDCP_CAP_GET                         0x068C
#define INV478X_RX_HDCP_TOPOLOGY_QUERY                  0x069C
#define INV478X_RX_HDCP_KSVLIST_QUERY                   0x069D
/*API's implemented for External Repeater functionality*/
#define INV478X_RX_HDCP_EXT_ENABLE_SET                  0x068D
#define INV478X_RX_HDCP_EXT_ENABLE_GET                  0x068E
#define INV478X_RX_HDCP_EXT_TOPOLOGY_SET                0x068F
#define INV478X_RX_HDCP_EXT_KSV_LIST_SET                0x069A
#define INV478X_RX_HDCP_ENCRYPTION_GET                  0x069B
#define INV478X_RX_PACKET_VS_USER_IEEE_SET              0x069E
#define INV478X_RX_PACKET_VS_USER_IEEE_GET              0x069F

#define INV478X_RX_AV_LINK_SET                          0x06A0
#define INV478X_RX_AV_LINK_GET                          0x06A1

/*****************************************************************************/
/***** A/V down-stream management */
/*****************************************************************************/

#define INV478X_TX_BITDEPTH_SET                         0x08B0
#define INV478X_TX_BITDEPTH_GET                         0x08B1
#define INV478X_EARC_TX_AUDIO_SRC_SET                   0x08B2
#define INV478X_EARC_TX_AUDIO_SRC_GET                   0x08B3
#define INV478X_TX_AV_LINK_SET                          0x08B4
#define INV478X_TX_AV_LINK_GET                          0x08B5
#define INV478X_HPDTX_QUERY                             0x08B6
#define INV478X_EDIDTX_QUERY                            0x08B7
#define INV478X_TX_AUDIO_INFO_QUERY                     0x08B8
#define INV478X_TX_AV_SRC_SELECT_SET                    0x08B9
#define INV478X_TX_AV_SRC_SELECT_GET                    0x08BA
#define INV478X_TX_RSEN_QUERY                           0x08BB
#define INV478X_TX_EDID_STATUS_QUERY                    0x08BC
/* #define INV478x_TX_AUDIO_FMT_SET                        0x08BD */
/* #define INV478x_TX_AUDIO_FMT_GET                        0x08BE */
#define INV478X_TX_AUDIO_FMT_QUERY                      0x08BF
/* #define INV478x_TX_AUDIO_CHANNEL_STATUS_QUERY           0x08C0 */
#define INV478X_TX_AUDIO_MUTE_SET                       0x08C1
#define INV478X_TX_AUDIO_MUTE_GET                       0x08C2
#define INV478X_TX_AUDIO_MUTE_QUERY                     0x08C3
#define INV478X_TX_AUDIO_CTSN_QUERY                     0x08C4
#define INV478X_TX_340_SCRAMBLING_SET                   0x08C5
#define INV478X_TX_340_SCRAMBLING_GET                   0x08C6
#define INV478X_TX_TMDS2_1OVER4_QUERY                   0x08C7
#define INV478X_TX_TMDS2_SCRAMBLE340_SET                0x08C8
#define INV478X_TX_TMDS2_SCRAMBLE340_GET                0x08C9
#define INV478X_TX_HDCP_MODE_SET                        0x08CA
#define INV478X_TX_HDCP_MODE_GET                        0x08CB
#define INV478X_TX_HDCP_STAT_QUERY                      0x08CC
#define INV478X_TX_HDCP_TYPE_QUERY                      0x08CD
/* #define INV478x_TX_AV_LINK_PT_SET                       0x08CE *
* #define INV478x_TX_AV_LINK_PT_GET                       0x08CF */
#define INV478X_TX_AVMUTE_SET                           0x08D0
#define INV478X_TX_AVMUTE_GET                           0x08D1
#define INV478X_TX_AVMUTE_QUERY                         0x08D2
#define INV478X_TX_GATED_BY_HPD_SET                     0x08D3
#define INV478X_TX_GATED_BY_HPD_GET                     0x08D4
#define INV478X_TX_EDID_READ_ON_HPD_SET                 0x08D5
#define INV478X_TX_EDID_READ_ON_HPD_GET                 0x08D6
#define INV478X_TX_HDCP_KSV_LIST_QUERY                  0x08DB
#define INV478X_TX_HDCP_CSM_SET                         0x08DC
#define INV478X_TX_HDCP_CSM_GET                         0x08DD
#define INV478X_TX_HDCP_CSM_QUERY                       0x08DE
#define INV478X_TX_HDCP_AKSV_QUERY                      0x08DF
#define INV478X_TX_HDCP_FAILURE_QUERY                   0x08E0
#define INV478X_TX_HDCP_TOPOLOGY_QUERY                  0x08E1
#define INV478X_TX_HDCP_ENCRYPT_SET                     0x08E2
#define INV478X_TX_HDCP_ENCRYPT_GET                     0x08E3
#define INV478X_TX_HDCP_APPROVE                         0x08E4
#define INV478X_TX_CSC_ENABLE_SET                       0x08E5
#define INV478X_TX_CSC_ENABLE_GET                       0x08E6
#define INV478X_TX_CSC_FAILURE_QUERY                    0x08E7
#define INV478X_TX_CSC_PIXEL_FMT_SET                    0x08E8
#define INV478X_TX_CSC_PIXEL_FMT_GET                    0x08E9
#define INV478X_TX_HDCP_REAUTH_REQ                      0x08EA
#define INV478X_TX_HDCP_DS_CAP_QUERY                    0x08EB
#define INV478X_TX_HDCP_AUTO_APPROVE_SET                0x08EC
#define INV478X_TX_HDCP_AUTO_APPROVE_GET                0x08ED
#define INV478X_TX_DEEP_CLR_QUERY                       0x08EE
#define INV478X_TX_AUDIO_SRC_SET                        0x08F0
#define INV478X_TX_AUDIO_SRC_GET                        0x08F1
#define INV478X_TX_AVMUTE_AUTO_CLEAR_SET                0x08F2
#define INV478X_TX_AVMUTE_AUTO_CLEAR_GET                0x08F3
#define INV478X_TX_PIN_SWAP_SET                         0x08F4
#define INV478X_TX_PIN_SWAP_GET                         0x08F5
#define INV478X_TX_HVSYNC_POL_SET                       0x08F6
#define INV478X_TX_HVSYNC_POL_GET                       0x08F7
#define INV478X_TX_AUTO_ENABLE_SET                      0x08F8
#define INV478X_TX_AUTO_ENABLE_GET                      0x08F9
#define INV478X_TX_REENABLE                             0x08FA
#define INV478X_TX_BLANK_COLOR_SET                      0x08FB
#define INV478X_TX_BLANK_COLOR_GET                      0x08FC
#define INV478X_TX_SCDC_READ_REQ_SET                    0x08FD
#define INV478X_TX_SCDC_READ_REQ_GET                    0x08FE
#define INV478X_TX_SCDC_POLL_PERIOD_SET                 0x08FF
#define INV478X_TX_SCDC_POLL_PERIOD_GET                 0x0900
#define INV478X_TX_SCDC_WRITE                           0x0901
#define INV478X_TX_SCDC_READ                            0x0902
#define INV478X_TX_DDC_WRITE                            0x0903
#define INV478X_TX_DDC_READ                             0x0904
#define INV478X_TX_DDC_FREQENCY_SET                     0x0905
#define INV478X_TX_DDC_FREQENCY_GET                     0x0906
#define INV478X_TX_AV_LINK_QUERY                        0x0909
#define INV478X_TX_TMDS2_SCRAMBLE_QUERY                 0x090A
#define INV478X_EARC_TX_AUDIO_LAYOUT_SET                0x090B
#define INV478X_EARC_TX_AUDIO_LAYOUT_GET                0x090C
#define INV478X_VTPG_VIDEO_TIMING_SET                    0x090D
#define INV478X_VTPG_VIDEO_TIMING_GET                    0x090E
#define INV478X_TX_BLANK_ENABLE_SET                     0x090F
#define INV478X_TX_BLANK_ENABLE_GET                     0x0910
#define INV478X_TX_PACKET_SET                           0x0911
#define INV478X_TX_PACKET_GET                           0x0912
#define INV478X_TX_PACKET_SRC_SET                       0x0913
#define INV478X_TX_PACKET_SRC_GET                       0x0914
#define INV478X_TX_HDCP_START                           0x0915

#define INV478X_TX_DDC_ENABLE_SET                       0x0918
#define INV478X_TX_DDC_ENABLE_GET                       0x0919

/* macro INV478x_COMMODE_SET has been used in STM32F4 application for com-mode */
#define INV478X_COMMODE_SET                             0x00FF


#endif /* INV478x_IPC_DEF_H */
