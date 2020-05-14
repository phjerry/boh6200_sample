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
* @file inv_common.h
*
* @brief Host Driver for Inv478x device.
*
*****************************************************************************/

#ifndef INV_COMMON_H
#define INV_COMMON_H

/***** #include statements ***************************************************/

#include "inv_datatypes.h"
#include "inv_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** public macro definitions **********************************************/

#define INV478X_SLAVE_ADDRESS                  0x40
#define REG08_MCU_STATUS                       REG08__MCU_REG__MCU__MCU_STATUS
#define REG08_MCU_INT_TRIGGER                  REG08__MCU_REG__MCU__MCU_INT_TRIGGER
#define INV478X_FLAG_NACK                      0x4

#define INV_HDMI_PACKET_FLAG__ACG                  0x00000001
#define INV_HDMI_PACKET_FLAG__ACP                  0x00000002
#define INV_HDMI_PACKET_FLAG__ISRC1                0x00000004
#define INV_HDMI_PACKET_FLAG__ISRC2                0x00000008
#define INV_HDMI_PACKET_FLAG__GMP                  0x00000010
#define INV_HDMI_PACKET_FLAG__CVT_EMP              0x00000020
#define INV_HDMI_PACKET_FLAG__VT_EMP               0x00000040
#define INV_HDMI_PACKET_FLAG__HDR_EMP              0x00000080
#define INV_HDMI_PACKET_FLAG__VSDS_EMP             0x00000100
#define INV_HDMI_PACKET_FLAG__VS_H14               0x00000200
#define INV_HDMI_PACKET_FLAG__AVI                  0x00000400
#define INV_HDMI_PACKET_FLAG__SPD                  0x00000800
#define INV_HDMI_PACKET_FLAG__AUD                  0x00001000
#define INV_HDMI_PACKET_FLAG__MPG                  0x00002000
#define INV_HDMI_PACKET_FLAG__DRM                  0x00004000
#define INV_HDMI_PACKET_FLAG__VS_USER              0x00008000
#define INV_HDMI_PACKET_FLAG__VBI                  0x00010000
#define INV_HDMI_PACKET_FLAG__VS_HF                0x00020000

typedef uint32_t inv_khz_t;     /* Pixel clock frequency in KHz */
typedef uint16_t inv_pix_t;     /* number of pixels/lines */

/**
* @brief Boot-loader status
*/
enum inv_sys_boot_stat {
    INV_SYS_BOOT_STAT__SUCCESS, /* booting is done successfully */
    INV_SYS_BOOT_STAT__IN_PROGRESS, /* booting is in progress */
    INV_SYS_BOOT_STAT__FAILURE, /* booting failed */
};

/**
* @brief Audio/Video Link Mode
*/
enum inv_hdmi_av_link {
    INV_HDMI_AV_LINK__NONE,     /* No Video */
    INV_HDMI_AV_LINK__DVI,      /* DVI mode */
    INV_HDMI_AV_LINK__TMDS1,    /* HDMI 1.4 mode */
    INV_HDMI_AV_LINK__TMDS2,    /* HDMI 2.0 mode */
    INV_HDMI_AV_LINK__FRL3x3,   /* HDMI 2.1 mode. FRL 3 Lanes, 3 Gbps per lane */
    INV_HDMI_AV_LINK__FRL3x6,   /* HDMI 2.1 mode. FRL 3 Lanes, 6 Gbps per lane */
    INV_HDMI_AV_LINK__FRL4x6,   /* HDMI 2.1 mode. FRL 4 Lanes, 6 Gbps per lane */
    INV_HDMI_AV_LINK__FRL4x8,   /* HDMI 2.1 mode. FRL 4 Lanes, 8 Gbps per lane */
    INV_HDMI_AV_LINK__FRL4x10,  /* HDMI 2.1 mode. FRL 4 Lanes, 10 Gbps per lane */
    INV_HDMI_AV_LINK__AUTO,     /* AV link mode based on downstream capability. Valid for Tx port only. */
    INV_HDMI_AV_LINK__UNKNOWN   /* Mode is unknown */
};

/**
* @brief HPD Replicate
*/
enum inv_hdmi_hpd_repl {
    INV_HDMI_HPD_REPL__NONE,    /* HPD is not replicated */
    INV_HDMI_HPD_REPL__LEVEL_TX0,   /* HPD is replicated by downstream level on TX0 */
    INV_HDMI_HPD_REPL__EDGE_TX0,    /* HPD is replicated by downstream falling edge on TX0 */
    INV_HDMI_HPD_REPL__LEVEL_TX1,   /* HPD is replicated by downstream level on TX1 */
    INV_HDMI_HPD_REPL__EDGE_TX1,    /* HPD is replicated by downstream falling edge on TX1 */
};

enum inv_hdmi_edid_repl {
    INV_HDMI_EDID_REPL__NONE,   /* EDID is not replicated */
    INV_HDMI_EDID_REPL__TX0,    /* EDID is replicated from Tx0 port */
    INV_HDMI_EDID_REPL__TX1,    /* EDID is replicated from Tx1 port */
};


enum inv_hdmi_earc_mode {
    INV_HDMI_EARC_MODE__NONE,   /* eARC/ARC is disabled */
    INV_HDMI_EARC_MODE__ARC,    /* Legacy ARC mode */
    INV_HDMI_EARC_MODE__EARC    /* eARC mode */
};

struct inv_video_timing {
    inv_pix_t v_act;            /* Number of active lines per video frame */
    inv_pix_t v_syn;            /* Vertical Sync width in number of lines */
    inv_pix_t v_fnt;            /* Vertical front porch width in number of lines */
    inv_pix_t v_bck;            /* Vertical back porch width in number of lines */
    inv_pix_t h_act;            /* Number of active pixels per video line */
    inv_pix_t h_syn;            /* Vertical Sync width in number of pixels */
    inv_pix_t h_fnt;            /* Vertical front porch width in number of pixels */
    inv_pix_t h_bck;            /* Vertical back porch width in number of pixels */
    inv_khz_t pclk;             /* Pixel clock frequency in KHz */
    uint8_t v_pol;              /* True for positive v-sync polarity */
    uint8_t h_pol;              /* True for positive h-sync polarity */
    uint8_t intl;               /* True for interlaced formats */
};

enum inv_hdmi_packet_type {
    INV_HDMI_PACKET_TYPE__NULL, /* NULL packet */
    INV_HDMI_PACKET_TYPE__ACG,  /* Audio Clock Generation packet(N/CTS) */
    INV_HDMI_PACKET_TYPE__ACP,  /* ACP packet */
    INV_HDMI_PACKET_TYPE__ISRC1,    /* ISRC1 packet */
    INV_HDMI_PACKET_TYPE__ISRC2,    /* ISRC2 packet */
    INV_HDMI_PACKET_TYPE__GMP,  /* Gamma Metadata Packet */
    INV_HDMI_PACKET_TYPE__CVT_EMP,  /* CVT_EMP */
    INV_HDMI_PACKET_TYPE__VT_EMP,   /* VT_EMP */
    INV_HDMI_PACKET_TYPE__HDR_EMP,  /* HDR_EMP */
    INV_HDMI_PACKET_TYPE__VSDS_EMP, /* VSDS_EMP */
    INV_HDMI_PACKET_TYPE__VS_H14,  /* Vendor Specific Info-frame H14*/
    INV_HDMI_PACKET_TYPE__AVI,  /* Auxiliary Video Information Info-frame */
    INV_HDMI_PACKET_TYPE__SPD,  /* Source Product Description Info-frame */
    INV_HDMI_PACKET_TYPE__AUD,  /* Audio Info-frame */
    INV_HDMI_PACKET_TYPE__MPG,  /* MPEG Info-frame */
    INV_HDMI_PACKET_TYPE__DRM,  /* Dynamic Range Mastering Info-frame */
    INV_HDMI_PACKET_TYPE__VS_USER,  /* User definable VS-IF */
    INV_HDMI_PACKET_TYPE__VBI,  /* NTSC VBI */
    INV_HDMI_PACKET_TYPE__VS_HF,    /* Vendor Specific Info-frame HF*/
    INV_HDMI_PACKET_TYPE__UNKNOWN,  /* Unknown Info-frame */
};

struct inv_hdmi_packet {
    enum inv_hdmi_packet_type type;
    uint8_t b[31];              /* HDMI META packets are limited to 31 bytes */
};

enum inv_audio_fs {
    INV_AUDIO_FS__NONE,         /* Zero sampling frequency, no sampling */
    INV_AUDIO_FS__22K05HZ,      /* Sampling frequency 22.5 KHz */
    INV_AUDIO_FS__24KHZ,        /* Sampling frequency 24 KHz */
    INV_AUDIO_FS__32KHZ,        /* Sampling frequency 32 KHz */
    INV_AUDIO_FS__44K1HZ,       /* Sampling frequency 44.1 KHz */
    INV_AUDIO_FS__48KHZ,        /* Sampling frequency 48 KHz */
    INV_AUDIO_FS__88K2HZ,       /* Sampling frequency 88.2 KHz */
    INV_AUDIO_FS__96KHZ,        /* Sampling frequency 96 KHz */
    INV_AUDIO_FS__176K4HZ,      /* Sampling frequency 176.4 KHz */
    INV_AUDIO_FS__192KHZ,       /* Sampling frequency 192 KHz */
    INV_AUDIO_FS__768KHZ        /* Sampling frequency 768 KHz */
};

enum inv_audio_ap_mode {
    INV_AUDIO_AP_MODE__NONE,    /* Audio port is disabled */
    INV_AUDIO_AP_MODE__SPDIF,   /* SPDIF (for ASP2 only) */
    INV_AUDIO_AP_MODE__I2S,     /* I2S */
    INV_AUDIO_AP_MODE__TDM,     /* TDM SINGLE LANE */
    INV_AUDIO_AP_MODE__AUTO,    /* WILL FOLLOW  RX SOURCE MODE */
};

struct inv_audio_cs {
    uint8_t b[9];               /* First 72 bits out of 192 channel status bits */
};

struct inv_audio_map {
    uint8_t b[10];              /* Channel mapping based on Audio Info-Frame byte 4-10, CEA861 */
};

struct inv_audio_info {
    struct inv_audio_cs cs;     /*First 72 bits out of 192 channel status bits */
    struct inv_audio_map map;   /*Channel mapping based on Audio Info-Frame byte 4-10, CEA861 */
};

struct inv_hdmi_audio_ctsn {
    uint32_t cts;               /* 24-bit CTS value */
    uint32_t n;                 /* N value */
};

struct inv_hdcp_top {
    bool_t hdcp1_dev_ds;        /* HDCP 1.X compliant device in the topology if set to true, HDCP 2.X use only */
    bool_t hdcp2_rptr_ds;       /* HDCP 2.0 compliant repeater in the topology if set to true, HDCP 2.X use only */
    bool_t max_cas_exceed;      /* More than seven level for HDCP 1.X or four levels for HDCP 2.X
                                 * of repeaters cascaded together if set to true */
    bool_t max_devs_exceed;     /* More than 127 devices(for HDCP1.X) or 31 devices(for HDCP 2.X) as attached
                                 * if set to true */
    uint8_t dev_count;          /* Total number of attached downstream devices */
    uint8_t rptr_depth;         /* Repeater cascade depth */
    uint32_t seq_num_v;         /* seq_num_V value, HDCP 2.X use only */
};

struct inv_hdcp_ksv {
    uint8_t b[5];               /* 40-bit KSV */
};

struct inv_hdcp_csm {
    uint32_t seq_num_m;         /* seq_num_M value, HDCP 2.X use only */
    uint16_t stream_id_type;    /* StreamID_Type */
};

enum inv_hdcp_failure {
    INV_HDCP_FAILURE__NONE,     /*!< no failure detected so far */
    INV_HDCP_FAILURE__DDC_NACK, /*!< downstream device does not acknowledge HDCP registers; firmware continues trying */
    INV_HDCP_FAILURE__BKSV_RXID,    /*!< bksv and rxid invalid; firmware does not try until HPD low to high transition */
    INV_HDCP_FAILURE__AUTH_FAIL,    /*!< R0, H!=H' or L!==L'; firmware continues trying */
    INV_HDCP_FAILURE__READY_TO, /*!< repeater ready failure; firmware continues trying */
    INV_HDCP_FAILURE__V,        /*!< V verification failed; firmware continues trying */
    INV_HDCP_FAILURE__TOPOLOGY, /*!< too many devices and depth is exceeded; firmware does not try until HPD low to high transition */
    INV_HDCP_FAILURE__RI,       /*!< RI verification failed; firmware continues trying */
    INV_HDCP_FAILURE__REAUTH_REQ,   /*!< reauth request received; firmware continues trying */
    INV_HDCP_FAILURE__CONTENT_TYPE, /*!< content type exceeds sink HDCP revision support; firmware continues trying */
    INV_HDCP_FAILURE__AUTH_TIME_OUT,    /*!< repeater authentication timeout; firmware continues trying */
    INV_HDCP_FAILURE__HASH,     /*!< content stream M check failure */
    INV_HDCP_FAILURE__UNKNOWN,  /*!< authentication failed due to unknown reason */
};

enum inv_video_colorimetry {
    INV_VIDEO_COLORIMETRY__RGB, /* R’G’B’ */
    INV_VIDEO_COLORIMETRY__OPRGB,   /* opR’G’B’ */
    INV_VIDEO_COLORIMETRY__RGB_P3D65,   /* P3D65 R’G’B’ */
    INV_VIDEO_COLORIMETRY__RGB_P3DCI,   /* P3DCI R’G’B’ (theater) */
    INV_VIDEO_COLORIMETRY__RGB2020, /* R’G’B’ based on ITU-2020 */
    INV_VIDEO_COLORIMETRY__YCC601,  /* Y’C’C’601(SMPTE-170M) */
    INV_VIDEO_COLORIMETRY__YCC709,  /* Y’C’C’709(ITU-R BT.709) */
    INV_VIDEO_COLORIMETRY__XVYCC601,    /* xvY’C’C’601 */
    INV_VIDEO_COLORIMETRY__XVYCC709,    /* xvY’C’C’709 */
    INV_VIDEO_COLORIMETRY__SYCC601, /* sY’C’C’601 */
    INV_VIDEO_COLORIMETRY__OPYCC601,    /* opY’C’C’601 */
    INV_VIDEO_COLORIMETRY__YCC2020NCL,  /* Y’C’C’ ITU-2020 Non Constant Luminance */
    INV_VIDEO_COLORIMETRY__YCC2020CL,   /* Y’C’C’ ITU-2020 Constant Luminance */
};

enum inv_video_chroma_smpl {
    INV_VIDEO_CHROMA_SMPL__444RGB,  /* 4:4:4 or RGB */
    INV_VIDEO_CHROMA_SMPL__422, /* 4:2:2 */
    INV_VIDEO_CHROMA_SMPL__420  /* 4:2:0 */
};

enum inv_video_bitdepth {
    INV_VIDEO_BITDEPTH__8BIT,   /* 8 bit */
    INV_VIDEO_BITDEPTH__10BIT,  /* 10 bit */
    INV_VIDEO_BITDEPTH__12BIT   /* 12 bit */
};

struct inv_video_pixel_fmt {
    enum inv_video_colorimetry colorimetry; /*Colorimetry value */
    enum inv_video_chroma_smpl chroma_smpl; /*Sample space value */
    bool_t full_range;          /* Limited range or full range quantization. */
    enum inv_video_bitdepth bitdepth;   /*Color depth value */
};

enum inv_hdcp_type {
    INV_HDCP_TYPE__NONE,        /* No HDCP */
    INV_HDCP_TYPE__HDCP1,       /* HDCP1.x */
    INV_HDCP_TYPE__HDCP2,       /* HDCP2.x */
};

enum inv_hdmi_deep_clr {
    INV_HDMI_DEEP_CLR__NONE,    /* No deep color(24 bits) */
    INV_HDMI_DEEP_CLR__30BIT,   /* Bit depth is limited to 10 bits */
    INV_HDMI_DEEP_CLR__36BIT,   /* Bit depth is limited to 12 bits */
};

enum inv_audio_mclk {
    INV_AUDIO_MCLK__NONE,       /* MClk disabled */
    INV_AUDIO_MCLK__FS128,      /* MCLK frequency is 128 * Audio Sample Rate */
    INV_AUDIO_MCLK__FS256,      /* MCLK frequency is 256 * Audio Sample Rate */
    INV_AUDIO_MCLK__FS512       /* MCLK frequency is 512 * Audio Sample Rate */
};

enum inv_video_hvsync_pol {
    INV_VIDEO_HVSYNC_POL__PP,   /* Hpos, Vpos */
    INV_VIDEO_HVSYNC_POL__PN,   /* Hpos, Vneg */
    INV_VIDEO_HVSYNC_POL__NP,   /* Hpos, Vpos */
    INV_VIDEO_HVSYNC_POL__NN,   /* Hneg, Vneg */
    INV_VIDEO_HVSYNC_POL__PASSTHRU, /* Polarity remains unchanged */
};

struct inv_video_rgb_value {
    uint8_t r;                  /* Red component */
    uint8_t g;                  /* Green component */
    uint8_t b;                  /* Blue component */
};

enum inv_audio_fmt {
    INV_AUDIO_FMT__NONE,        /* no audio / mute */
    INV_AUDIO_FMT__ASP2,        /* 2 channel layout */
    INV_AUDIO_FMT__ASP8,        /* 8 channel layout */
    INV_AUDIO_FMT__HBRA,        /* High bit rate audio */
    INV_AUDIO_FMT__DSD6,        /* 6 channel DSD */
};

enum inv_hdmi_pin_swap {
    INV_HDMI_PIN_SWAP__NONE,    /* swap disabled */
    INV_HDMI_PIN_SWAP__LANE,    /* pin swap only */
    INV_HDMI_PIN_SWAP__POL,     /* polarity inversion only */
    INV_HDMI_PIN_SWAP__FULL     /* both pin and polarity */
};

/**
* @brief Video resolution.

*
* VIC refers o CEA-861 specification's VIC code.
* HDMI VIC refers to HDMI specification's HDMI VIC code.
*/
#define    INV_VIDEO_FMT__NONE                      0
#define       INV_VIDEO_FMT__1_VGA60                   1   /*!< \f$ 640 \times 480\f$ 60Hz progressive(VGA) (CEA VIC 2/3) */
#define    INV_VIDEO_FMT__3_720X480P60              3   /*!< \f$ 720 \times 480\f$ 60Hz progressive(480P60) (CEA VIC 2/3) */
#define    INV_VIDEO_FMT__4_1280X720P60             4   /*!< \f$ 1280 \times 720\f$ 60Hz progressive(720P60) (CEA VIC 4/69) */
#define    INV_VIDEO_FMT__16_1920X1080P60           16  /*!< \f$ 1920 \times 1080\f$ 60Hz progressive(1080P60) (CEA VIC 16/76) */
#define    INV_VIDEO_FMT__18_720X576P50             18  /*!< \f$ 720 \times 576\f$ 50Hz progressive(576P60) (CEA VIC 17/18) */
#define    INV_VIDEO_FMT__19_1280X720P50            19  /*!< \f$ 1280 \times 720\f$ 50Hz progressive(720P50) (CEA VIC 19/68) */
#define    INV_VIDEO_FMT__31_1920X1080P50           31  /*!< \f$ 1920 \times 1080\f$ 50Hz progressive(1080P50) (CEA VIC 31/75) */
#define    INV_VIDEO_FMT__32_1920X1080P24           32  /*!< \f$ 1920 \times 1080\f$ 24Hz progressive(1080P24) (CEA VIC 32/72) */
#define    INV_VIDEO_FMT__33_1920X1080P25           33  /*!< \f$ 1920 \times 1080\f$ 25Hz progressive(1080P25) (CEA VIC 33/73) */
#define    INV_VIDEO_FMT__34_1920X1080P30           34  /*!< \f$ 1920 \times 1080\f$ 30Hz progressive(1080P30) (CEA VIC 34/74) */
#define    INV_VIDEO_FMT__63_1920X1080P120          63  /*!< \f$ 19200 \times 1080\f$ 120Hz progressive(CEA VIC 63/78) */
#define    INV_VIDEO_FMT__64_1920X1080P100          64  /*!< \f$ 19200 \times 1080\f$ 100Hz progressive(CEA VIC 64/77) */
#define    INV_VIDEO_FMT__91_2560X1080P100          91  /*!< \f$ 2560 \times 1080\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__93_3840X2160P24           93  /*!< \f$ 3840 \times 2160\f$ 24Hz progressive(CEA VIC 93/103) */
#define    INV_VIDEO_FMT__94_3840X2160P25           94  /*!< \f$ 3840 \times 2160\f$ 25Hz progressive(CEA VIC 94/104) */
#define    INV_VIDEO_FMT__95_3840X2160P30           95  /*!< \f$ 3840 \times 2160\f$ 30Hz progressive(CEA VIC 95/105) */
#define    INV_VIDEO_FMT__96_3840X2160P50           96  /*!< \f$ 3840 \times 2160\f$ 50Hz progressive(CEA VIC 96/106) */
#define    INV_VIDEO_FMT__97_3840X2160P60           97  /*!< \f$ 3840 \times 2160\f$ 60Hz progressive(CEA VIC 97/107) */
#define    INV_VIDEO_FMT__98_4096X2160P24           98  /*!< \f$ 4160 \times 2160\f$ 24Hz progressive  */
#define    INV_VIDEO_FMT__99_4096X2160P25           99  /*!< \f$ 4160 \times 2160\f$ 25Hz progressive  */
#define    INV_VIDEO_FMT__100_4096X2160P30          100 /*!< \f$ 4160 \times 2160\f$ 30Hz progressive  */
#define    INV_VIDEO_FMT__101_4096X2160P50          101 /*!< \f$ 4160 \times 2160\f$ 50Hz progressive  */
#define    INV_VIDEO_FMT__102_4096X2160P60          102 /*!< \f$ 4160 \times 2160\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__117_3840X2160P100         117 /*!< \f$ 3840 \times 2160\f$ 100Hz progressive(CEA VIC 117/119) */
#define    INV_VIDEO_FMT__118_3840X2160P120         118
#define    INV_VIDEO_FMT__125_5120X2160P50          125 /*!< \f$ 5120 \times 2160\f$ 50Hz progressive  */
#define    INV_VIDEO_FMT__126_5120X2160P60          126 /*!< \f$ 5120 \times 2160\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__127_5120X2160P100         127 /*!< \f$ 5120 \times 2160\f$ 100Hz progressive  */
#define    INV_VIDEO_FMT__193_5120X2160P120         193 /*!< \f$ 5120 \times 2160\f$ 120Hz progressive  */
#define    INV_VIDEO_FMT__194_7680X4320P24          194 /*!< \f$ 7680 \times 4320\f$ 24Hz progressive(CEA VIC 194/202) */
#define    INV_VIDEO_FMT__195_7680X4320P25          195 /*!< \f$ 7680 \times 4320\f$ 25Hz progressive(CEA VIC 195/203) */
#define    INV_VIDEO_FMT__196_7680X4320P30          196 /*!< \f$ 7680 \times 4320\f$ 30Hz progressive(CEA VIC 196/204) */
#define    INV_VIDEO_FMT__198_7680X4320P50          198 /*!< \f$ 7680 \times 4320\f$ 50Hz progressive(CEA VIC 198/206) */
#define    INV_VIDEO_FMT__199_7680X4320P60          199 /*!< \f$ 7680 \times 4320\f$ 60Hz progressive(CEA VIC 199/207) */
#define    INV_VIDEO_FMT__218_4096X2160P100         218 /*!< \f$ 4160 \times 2160\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__219_4096X2160P120         219 /*!< \f$ 4160 \times 2160\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_SVGA60                  1000    /*!< \f$ 800  \times 600\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_XGA60                   1001    /*!< \f$ 1024 \times 768\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_WXGA60                  1002    /*!< \f$ 1280 \times 768\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_WXGA60_800              1003    /*!< \f$ 1280 \times 800\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_FWXGA60                 1004    /*!< \f$ 1366 \times 768\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_WXGA60_PLUS             1005    /*!< \f$ 1440 \times 900\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_SXGA60                  1006    /*!< \f$ 1280 \times 1024\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_WXGA60_2PLUS            1007    /*!< \f$ 1600 \times 900\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_WSXGA60_PLUS            1008    /*!< \f$ 1680 \times 1050\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_UXGA60                  1009    /*!< \f$ 1600 \times 1200\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__0_WUXGA60                 1010    /*!< \f$ 1920 \times 100\f$ 60Hz progressive */
#define    INV_VIDEO_FMT__7680X2160P60             65531    /*!< \f$ 7680 \times 2160\f$ 60Hz progressive  */
#define    INV_VIDEO_FMT__3840X4320P50             65532
#define    INV_VIDEO_FMT__3840X4320P60             65533
#define    INV_VIDEO_FMT__USER                     0xFFFE
#define    INV_VIDEO_FMT__UNKNOWN                  0xFFFF   /*!< Video format does not match any of the formats in this enumeration */

enum inv_hdcp_stat {
    INV_HDCP_STAT__NONE,        /* No HDCP */
    INV_HDCP_STAT__AUTH,        /* HDCP authentication is in progress */
    INV_HDCP_STAT__CSM,         /* HDCP CSM pending(HDCP2.2 only) */
    INV_HDCP_STAT__SUCCESS,     /* HDCP link is authenticated */
    INV_HDCP_STAT__FAILED       /* HDCP authentication has failed. Re-try is in progress */
};

enum inv_hdcp_mode {
    INV_HDCP_MODE__NONE,        /* HDCP not supported */
    INV_HDCP_MODE__SRSK,        /* HDCP interface functions as source or sink device */
    INV_HDCP_MODE__RPTR,        /* HDCP interface functions as a repeater device */
};

enum inv_vtpg_ptrn {
    INV_VTPG_PTRN__RED,         /* Solid Red(255,0,0) */
    INV_VTPG_PTRN__GREEN,       /* Solid Green(0,255,0) */
    INV_VTPG_PTRN__BLUE,        /* Solid Blue(0,0,255) */
    INV_VTPG_PTRN__CYAN,        /* Solid Cyan(0,255,255) */
    INV_VTPG_PTRN__MAGENTA,     /* Solid Magenta(255,0,0) */
    INV_VTPG_PTRN__YELLOW,      /* Solid Yellow(255,255,0) */
    INV_VTPG_PTRN__BLACK,       /* Solid Black(0,0,0) */
    INV_VTPG_PTRN__WHITE,       /* Solid White(255,255,255) */
    INV_VTPG_PTRN__CHKBRD,      /* Checkerboard Pattern */
    INV_VTPG_PTRN__CLRBAR,      /* RGB Color bars */
    INV_VTPG_PTRN__SPECIAL,     /* Customized image */
};


#ifdef __cplusplus
}
#endif

#endif /* INV_COMMON_H */