#ifndef _COMMON_DATA_H
#define _COMMON_DATA_H

#include "hi_type.h"

#define     MAX_PROGNAME_LENGTH 32
#define     MAX_AUDIO_LANGUAGE  5
#define     MAX_PROG_COUNT      200
#define     MAX_FRONTEND_COUNT  30

#define     SEARCHING_FRONTEND_ID 0xffff

typedef enum hi_frontend_type {
    FE_TYPE_RF   = 1,
    FE_TYPE_IP   = 2,
    FE_TYPE_FILE = 3
} frontend_type;

typedef enum hi_file_type {
    FILE_TYPE_TS = 0,
    FILE_TYPE_ES = 1,
} file_type;

typedef enum hi_file_audio_type {
    FILE_AUDIO_TYPE_NONE  = 0,
    FILE_AUDIO_TYPE_AAC   = 1,
    FILE_AUDIO_TYPE_MP3   = 2,
    FILE_AUDIO_TYPE_AC3   = 3,
    FILE_AUDIO_TYPE_DTS   = 4,
    FILE_AUDIO_TYPE_DRA   = 5
} file_audio_type;

typedef enum hi_file_video_type {
    FILE_VIDEO_TYPE_NONE  = 0,
    FILE_VIDEO_TYPE_MPEG2 = 1,
    FILE_VIDEO_TYPE_MPEG4 = 2,
    FILE_VIDEO_TYPE_H263  = 3,
    FILE_VIDEO_TYPE_H264  = 4,
    FILE_VIDEO_TYPE_AVS   = 5,
    FILE_VIDEO_TYPE_REAL  = 6,
    FILE_VIDEO_TYPE_AV1   = 7
} file_video_type;

typedef struct hi_fe_typeip {
    hi_char multi_ip_addr[20];
    hi_u32  port;
} fe_type_ip;

typedef struct hi_fe_typerf {
    hi_u32 tuner_port;  /*Tuner port */
    hi_u32 frequency;   /*unit:MHZ */
    hi_u32 symbol_rate; /*unit:KHZ */
    hi_u32 modulation;  /* 0:16QAM 1:32QAM 2:64QAM 3:128QAM 4:256QAM */
} fe_type_rf;

typedef struct hi_fe_typefile {
    hi_char path[256];
    file_type filetype;
    file_video_type videotype;
    file_audio_type audiotype;
} fe_type_file;

/* channel dot struct */
typedef struct hi_db_frontend {
    frontend_type fe_type;
    hi_u16          network_id;
    hi_u16          ts_id;
    union {
        fe_type_file  fe_para_file;  /* file type */
        fe_type_ip    fe_para_ip;    /* IP type */
        fe_type_rf    fe_para_rf;    /* RF type */
    } f_etype;

} db_frontend;

/* program struct */
typedef struct hi_db_videoex {
    hi_u16 video_pid;
    hi_u32 video_enc_type;
} db_videoex;

typedef struct hi_db_audioex {
    hi_u16 u16audiopid;
    hi_u16 u16audiolan;
    hi_u32 audio_enc_type;
} db_audioex;

typedef struct  hi_db_program {
    hi_u16 frontend_id;
    hi_u16 network_id;
    hi_u16 ts_id;
    hi_u16 service_id;

    hi_u8  service_type;

    hi_s8  program_name[MAX_PROGNAME_LENGTH];

    hi_u16 pmt_pid;
    hi_u16 pcr_pid;

    hi_u32 prog_property;

    hi_u16 audio_vol;
    hi_u8  audio_channel;
    hi_u8  video_channel;

    db_videoex video_ex;
    db_audioex audio_ex[MAX_AUDIO_LANGUAGE];

    hi_u16 reserved;
} db_program;

hi_s32 db_get_dvbprog_info_by_service_id(hi_u16 service_id, db_program *proginfo);
hi_s32 db_get_dvbprog_info(hi_u32 prognum, db_program *proginfo);
hi_s32 db_set_dvbprog_info(hi_u32 prognum, const db_program *proginfo);
hi_s32 db_add_dvbprog(const db_program *proginfo);
hi_s32 db_get_prog_total_count(void);
hi_s32 db_get_fechan_info(hi_u32 channum, db_frontend *chaninfo);
hi_s32 db_set_fechan_info(hi_u32 channum, const db_frontend *chaninfo);
hi_s32 db_add_fechan(const db_frontend *chaninfo);
hi_s32 db_get_fechan_total_count(void);
hi_s32 db_reset(void);
hi_s32 db_restore_from_file(FILE *filestream);
hi_s32 db_save_to_file(FILE *filestream);

#endif
