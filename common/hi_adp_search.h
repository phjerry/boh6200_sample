#ifndef _COMMON_SEARCH_H
#define _COMMON_SEARCH_H

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/********************Descriptor flag definition******************/
/********************CNcomment:描述符标识符定义******************/

#define STREAM_TYPE_11172_VIDEO              0x01
#define STREAM_TYPE_13818_VIDEO              0x02
#define STREAM_TYPE_11172_AUDIO              0x03
#define STREAM_TYPE_13818_AUDIO              0x04
#define STREAM_TYPE_14496_2_VIDEO            0x10    /*  MPEG4 */
#define STREAM_TYPE_14496_10_VIDEO           0x1B    /*  H264 */
#define STREAM_TYPE_AVS_VIDEO                0x42    /*  AVS */
#define STREAM_TYPE_AVS2_VIDEO               0xD2    /*  AVS2 */
#define STREAM_TYPE_AVS3_VIDEO               0xD6    /*  AVS3 */
#define STREAM_TYPE_HEVC_VIDEO               0x24    /*  HEVC */
#define STREAM_TYPE_13818_7_AUDIO            0x0F    /*  AAC */
#define STREAM_TYPE_14496_3_AUDIO            0x11    /*  AAC */
#define STREAM_TYPE_AC3_AUDIO                0x81    /*  AC3 */
#define STREAM_TYPE_SCTE                     0x82    /*  TS packets containing SCTE data */
#define STREAM_TYPE_DTS_AUDIO                0x82    /*  DTS */
#define STREAM_TYPE_DOLBY_TRUEHD_AUDIO       0x83    /*  dolby true HD */
#define STREAM_TYPE_DTS_MA                   0x86    /*  DTS MA which conflict with CAPTION_SERVICE_DESCRIPTOR */
#define STREAM_TYPE_PRIVATE                  0x06    /*  PES packets containing private data */

#define VIDEO_STREAM_DESCRIPTOR              0x02
#define AUDIO_STREAM_DESCRIPTOR              0x03
#define HIERACHY_DESCRIPTOR                  0x04
#define REGISTRATION_DESCRIPTOR              0x05
#define DATA_STREAM_ALIGNMENT_DESCRIPTOR     0x06
#define TARGET_BACKGROUND_GRID_DESCRIPTOR    0x07
#define VIDEO_WINDOW_DESCRIPTOR              0x08
#define CA_DESCRIPTOR                        0x09
#define LANGUAGE_DESCRIPTOR                  0x0A
#define SYSTEM_CLOCK_DESCRIPTOR              0x0B
#define MULTIPLEX_BUFFER_USAGE_DESCRIPTOR    0x0C
#define COPYRIGHT_DESCRIPTOR                 0x0D
#define MAXIMUM_BITRATE_DESCRIPTOR           0x0E
#define PRIVATE_DATA_INDICATOR_DESCRIPTOR    0x0F
#define SMOOTHING_BUFFER_DESCRIPTOR          0x10
#define STD_DESCRIPTOR                       0x11
#define IBP_DESCRIPTOR                       0x12
#define AC4_DESCRIPTOR                       0x15

#define NETWORK_NAME_DESCRIPTOR              0x40
#define SERVICE_LIST_DESCRIPTOR              0x41
#define STUFFING_DESCRIPTOR                  0x42
#define SATELLITE_DELIVERY_DESCRIPTOR        0x43
#define CABLE_DELIVERY_DESCRIPTOR            0x44
#define BOUQUET_NAME_DESCRIPTOR              0x47
#define SERVICE_DESCRIPTOR                   0x48
#define COUNTRY_AVAILABILITY_DESCRIPTOR      0x49
#define LINKAGE_DESCRIPTOR                   0x4A
#define NVOD_REFERENCE_DESCRIPTOR            0x4B
#define TIME_SHIFTED_SERVICE_DESCRIPTOR      0x4C
#define SHORT_EVENT_DESCRIPTOR               0x4D
#define EXTENDED_EVENT_DESCRIPTOR            0x4E
#define TIME_SHIFTED_EVENT_DESCRIPTOR        0x4F
#define COMPONENT_DESCRIPTOR                 0x50
#define MOSAIC_DESCRIPTOR                    0x51
#define STREAM_IDENTIFIER_DESCRIPTOR         0x52
#define CA_IDENTIFIER_DESCRIPTOR             0x53
#define CONTENT_DESCRIPTOR                   0x54
#define PARENTAL_RATING_DESCRIPTOR           0x55
#define TELETEXT_DESCRIPTOR                  0x56
#define TELEPHONE_DESCRIPTOR                 0x57
#define LOCAL_TIME_OFFSET_DESCRIPTOR         0x58
#define SUBTITLING_DESCRIPTOR                0x59
#define TERRESTRIAL_DELIVERY_DESCRIPTOR      0x5A
#define MULTILINGUAL_NETWORK_NAME_DESCRIPTOR 0x5B
#define MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR 0x5C
#define MULTILINGUAL_SERVICE_NAME_DESCRIPTOR 0x5D
#define MULTILINGUAL_COMPONENT_DESCRIPTOR    0x5E
#define PRIVATE_DATA_SPECIFIER_DESCRIPTOR    0x5F
#define SERVICE_MOVE_DESCRIPTOR              0x60
#define SHORT_SMOOTHING_BUFFER_DESCRIPTOR    0x61
#define FREQUENCY_LIST_DESCRIPTOR            0x62
#define PARTIAL_TRANSPORT_STREAM_DESCRIPTOR  0x63
#define DATA_BROADCAST_DESCRIPTOR            0x64
#define CA_SYSTEM_DESCRIPTOR                 0x65
#define DATA_BROADCAST_ID_DESCRIPTOR         0x66
#define TRANSPORT_STREAM_DESCRIPTOR          0x67
#define DSNG_DESCRIPTOR                      0x68
#define PDC_DESCRIPTOR                       0x69
#define AC3_DESCRIPTOR                       0x6A
#define AC3_PLUS_DESCRIPTOR                  0x7A
#define ANCILLARY_DATA_DESCRIPTOR            0x6B
#define CELL_LIST_DESCRIPTOR                 0x6C
#define CELL_FREQUENCY_LINK_DESCRIPTOR       0x6D
#define ANNOUNCEMENT_SUPPORT_DESCRIPTOR      0x6E
#define DRA_DESCRIPTOR                       0x05

#define AC3_EXT_DESCRIPTOR                   0x52

#define CAPTION_SERVICE_DESCRIPTOR           0x86
#define EXTENSION_DESCRIPTOR                 0x7F
#define SUPPLEMENTARY_AUDIO_DESCRIPTOR       0x06

#define STREAM_TYPE_HEVC_VIDEO_IDENTIFY      0x48455643
#define STREAM_TYPE_DTS1_AUDIO_IDENTIFY      0x44545331
#define STREAM_TYPE_DTS2_AUDIO_IDENTIFY      0x44545332
#define STREAM_TYPE_DTS3_AUDIO_IDENTIFY      0x44545333

#define MAX_PMT_LEN                          1024
/***********TS PID defintion**************/

#define PAT_TSPID (0x0000)
#define CAT_TSPID (0x0001)
#define NIT_TSPID (0x0010)
#define EIT_TSPID (0x0012)
#define TOT_TSPID (0x0014)
#define TDT_TSPID (0x0014)
#define SDT_TSPID (0x0011)
#define BAT_TSPID (0x0011)

#define INVALID_TSPID (0x1fff)

/***********Table ID defintion**************/
#define PAT_TABLE_ID                       (0x00)
#define CAT_TABLE_ID                       (0x01)
#define PMT_TABLE_ID                       (0x02)
#define NIT_TABLE_ID_ACTUAL                (0x40)
#define NIT_TABLE_ID_OTHER                 (0x41)

#define SDT_TABLE_ID_ACTUAL                (0x42)
#define SDT_TABLE_ID_OTHER                 (0x46)

#define BAT_TABLE_ID                       (0x4A)
#define EIT_TABLE_ID_PF_ACTUAL             (0x4E)
#define EIT_TABLE_ID_PF_OTHER              (0x4F)
#define EIT_TABLE_ID_SCHEDULE_ACTUAL_LOW   (0x50)
#define EIT_TABLE_ID_SCHEDULE_ACTUAL_HIGH  (0x5F)

#define EIT_TABLE_ID_SCHEDULE_OTHER_LOW    (0x60)
#define EIT_TABLE_ID_SCHEDULE_OTHER_HIGH   (0x6F)

#define TDT_TABLE_ID                       (0x70)
#define TOT_TABLE_ID                       (0x73)

#define INVALID_TABLE_ID                   (0xff)
#define CHANNEL_MAX_PROG                   256
#define PROG_MAX_VIDEO                     8
#define PROG_MAX_AUDIO                     8
#define PROG_MAX_CA                        15

#define SUBTDES_INFO_MAX                   10
#define SUBTITLING_MAX                     1500
#define CAPTION_SERVICE_MAX                16
#define TTX_DES_MAX                        10
#define TTX_MAX                            15

#define SUBT_TYPE_DVB                      (0x1)
#define SUBT_TYPE_SCTE                     (0x2)
#define SUBT_TYPE_BOTH                     (SUBT_TYPE_DVB | SUBT_TYPE_SCTE)

typedef struct hi_pat_info {
    hi_u16 service_id;     /*Progam 's SERVICE ID*/
    hi_u16 pmt_pid;        /*Progam 's PMT ID*/
} pat_info;

typedef struct hi_pat_tb {
    hi_u16 prog_num;
    hi_u16 ts_id;
    pat_info pat_info[CHANNEL_MAX_PROG];
} pat_tb;

typedef struct hi_pmt_video {
    hi_u32 video_enc_type;
    hi_u16 video_pid;
} pmt_video;

typedef struct hi_pmt_audio {
    hi_u32         audio_enc_type;
    hi_u16         audio_pid;
    hi_u16         ad_type;
    hi_u8          aud_lang[3];
} pmt_audio;
typedef struct hi_pmt_ca {
    hi_u16 ca_system_id;
    hi_u16 cap_id ;
} pmt_ca;

typedef struct hi_pmp_subtitle_des {
    hi_u32 lang_code;            /* low 24-bit valid */
    hi_u8  subtitle_type;
    hi_u16 page_id;
    hi_u16 ancillary_page_id;
} pmp_subtitle_des;

typedef struct hi_pmt_subtitle {
    hi_u16 subtitling_pid;
    hi_u8 des_tag;
    hi_u8 des_length;
    hi_u8 des_info_cnt;
    pmp_subtitle_des des_info[SUBTDES_INFO_MAX];
} pmt_subtitle;

typedef struct hi_pmt_scte_subtitle {
    hi_u16 scte_subt_pid;
    hi_u32 language_code;
} pmt_scte_subtitle;

typedef struct hi_pmt_closed_caption {
    hi_u32 lang_code;
    hi_u8 is_digital_cc;
    hi_u8 service_number;
    hi_u8 is_easy_reader;
    hi_u8 is_wide_aspect_ratio;
} pmt_closed_caption;

typedef struct hi_pmt_ttx_des {
    hi_u32 iso639_language_code; /* low 24-bit valid */
    hi_u8  ttx_type;
    hi_u8  ttx_magazine_number;
    hi_u8  ttx_page_number;
} pmt_ttx_des;

typedef struct hi_pmt_ttx {
    hi_u16 ttx_pid;
    hi_u8 des_tag;
    hi_u8 des_length;
    hi_u8 des_info_cnt;
    pmt_ttx_des ttx_des[TTX_DES_MAX];

} pmt_ttx;


typedef struct hi_pmt_tb {
    hi_u16 service_id;
    hi_u16 pcr_pid;
    hi_u16 video_num;
    hi_u16 audo_num;
    hi_u16 ca_num;
    pmt_video video_info[PROG_MAX_VIDEO];
    pmt_audio audio_info[PROG_MAX_AUDIO];
    pmt_ca ca_system[PROG_MAX_CA];

    hi_u16 subtitling_num;
    pmt_subtitle subtiting_info[SUBTITLING_MAX];
    pmt_scte_subtitle scte_subt_info;
    hi_u16 closed_caption_num;
    pmt_closed_caption closed_caption[CAPTION_SERVICE_MAX];
    hi_u16 aribcc_pid;

    hi_u16 ttx_num;
    pmt_ttx ttx_info[TTX_MAX];
    hi_u8  pmt_data[MAX_PMT_LEN];
    hi_u32 pmt_len;
} pmt_tb;

typedef enum hi_run_state {
    UNDEFINED = 0,
    NOTRUN,
    STARTINSECONDS,
    PAUSE,
    RUNNING,
    RUN_RESERVED1,
    RUN_RESERVED2,
    RUN_RESERVED3
} run_state;

typedef enum hi_ca_mode {
    CA_NOT_NEED = 0,
    CA_NEED
} ca_mode;

typedef struct hi_sdt_info {
    hi_u16      service_id;
    hi_u8       eit_flag;
    hi_u8       eit_flag_pf;
    run_state   run_state;
    ca_mode     ca_mode;

    hi_u32      service_type;
    hi_s8       prog_name[32];
} sdt_info;

typedef struct hi_sdt_tb {
    hi_u32 prog_num;
    hi_u16 ts_id;
    hi_u16 net_id;
    sdt_info sdt_info[CHANNEL_MAX_PROG];
} sdt_tb;


typedef struct hi_pmt_compact_prog {
    hi_u32               prog_id;               /* program ID */
    hi_u32               pmt_pid;               /*program PMT PID*/
    hi_u32               pmt_remap_pid;         /* overlapped when remux, remap to new pid */
    hi_u32               pcr_pid;               /*program PCR PID*/

    hi_u32               video_type;
    hi_u16               v_element_num;         /* video stream number */
    hi_u16               v_element_pid;         /* the first video stream PID*/
    hi_u16               v_element_remap_pid;   /* overlapped when remux, remap to new pid */

    hi_u32               audio_format;
    hi_u32               audio_type;
    hi_u16               a_element_num;         /* audio stream number */
    hi_u16               a_element_pid;         /* the first audio stream PID*/
    hi_u16               a_element_remap_pid;   /* overlapped when remux, remap to new pid */

    hi_u16               ca_num;
    pmt_ca               ca_system[PROG_MAX_CA];
    pmt_audio
    audio_info[PROG_MAX_AUDIO];                 /* multi-audio info, added by gaoyanfeng 00182102 */

    hi_u32              subt_type;              /*0---NONE,1---DVB,2---SCTE,3---BOTH*/
    hi_u16              subtitling_num;
    pmt_subtitle        subtiting_info[SUBTITLING_MAX];
    pmt_scte_subtitle   scte_subt_info;
    hi_u16              closed_caption_num;
    pmt_closed_caption  closed_caption[CAPTION_SERVICE_MAX];
    hi_u16              aribcc_pid;

    hi_u16              ttx_num;
    pmt_ttx             ttx_info[TTX_MAX];
    hi_u8               pmt_data[MAX_PMT_LEN];
    hi_u32              pmt_len;
} pmt_compact_prog;

typedef struct hi_pmt_compact_tbl {
    hi_u32            prog_num;
    pmt_compact_prog *proginfo;
} pmt_compact_tbl;

hi_s32      dvb_search_start(hi_u32 dmx_id);
hi_void     dvb_save_search(hi_u32 frontend_id);
hi_void     dvb_list_prog();
hi_s32      srh_parse_sdt(const hi_u8 *section_data, hi_s32 length, hi_u8 *section_struct);
hi_s32      srh_parse_pmt ( const hi_u8 *section_data, hi_s32 length, hi_u8 *section_struct);
hi_s32      srh_parse_pat( const hi_u8  *section_data, hi_s32 length, hi_u8 *section_struct);
hi_s32      srh_pat_request(hi_u32 dmx_id, pat_tb *pat_tb);
hi_s32      srh_pmt_request(hi_u32 dmx_id, pmt_tb *pmt_tb, hi_u16 pmt_pid, hi_u16 service_id);
hi_s32      srh_pat_request_ext(hi_u32 dmx_id, pat_tb *pat_tb, hi_bool tee_enable);
hi_s32      srh_pmt_request_ext(hi_u32 dmx_id, pmt_tb *pmt_tb, hi_u16 pmt_pid, hi_u16 service_id, hi_bool tee_enable);
hi_s32      srh_sdt_request(hi_u32 dmx_id, sdt_tb *sdt_tb);
/******************************************Search public interface***********************************/
hi_void     hi_adp_search_init();
hi_s32      hi_adp_search_get_all_pmt(hi_u32 dmx_id, pmt_compact_tbl **pp_prog_table);
hi_s32      hi_adp_search_free_all_pmt(pmt_compact_tbl *prog_table);
hi_void     hi_adp_search_de_init();
hi_s32      hi_adp_search_get_all_pmt_ext(hi_u32 dmx_id, pmt_compact_tbl **pp_prog_table, hi_bool tee_enable);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /*__SEARCH_H*/
