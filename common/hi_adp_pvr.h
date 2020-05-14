/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:
 * Author: sdk
 * Create: 2019-08-14
 */

#ifndef __SAMPLE_PVR_COMMON_H__
#define __SAMPLE_PVR_COMMON_H__

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
#include "hi_unf_audio.h"
#include "hi_unf_video.h"
#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_demux.h"
#include "hi_unf_pvr.h"
#include "hi_adp_search.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Macro Definition ******************************/
#define PVR_FS_DIR              "/tmpfs/"
#define PVR_CIPHER_KEY          "688PVR-KEY-123456789"

#define PVR_DMX_ID_LIVE             0
#define PVR_DMX_PORT_ID_IP          HI_UNF_DMX_PORT_RAM_0
#define PVR_DMX_PORT_ID_DVB         HI_UNF_DMX_PORT_IF_0
#define PVR_DMX_PORT_ID_PLAYBACK    HI_UNF_DMX_PORT_RAM_1
#define PVR_DMX_ID_REC              1
#define PVR_PROG_INFO_MAGIC         0xABCDE
#ifdef CONFIG_SUPPORT_CA_RELEASE
#define pvr_sample_printf(fmt, args...)
#else
#define pvr_sample_printf(fmt, args...) \
    ({printf("[%s:%d]: " fmt "", __FUNCTION__, __LINE__, ## args);})
#endif

#define PVR_PROG_MAX_AUDIO         8
#define PVR_SUBTITLING_MAX         15
#define PVR_SUBTDES_INFO_MAX       10
#define PVR_CAPTION_SERVICE_MAX    16
#define PVR_TTX_MAX                15
#define PVR_TTX_DES_MAX            10

/*************************** Structure Definition ****************************/
typedef struct {
    hi_u32 audio_enc_type;
    hi_u16 audio_pid;
} pvr_pmt_audio;

typedef struct {
    hi_u32 lang_code; /* low 24-bit valid */
    hi_u8  subtitle_type;
    hi_u16 page_id;
    hi_u16 ancillary_page_id;
} pvr_pmp_subtitle_des;

typedef struct {
    hi_u16 subtitling_pid;
    hi_u8 des_tag; /*  */
    hi_u8 des_length;
    hi_u8 des_info_cnt;
    pvr_pmp_subtitle_des des_info[SUBTDES_INFO_MAX];
} pvr_pmt_subtitle;

typedef struct {
    hi_u16 scte_subt_pid;
    hi_u32 language_code;
} pvr_pmt_scte_subtitle;

typedef struct {
    hi_u32 lang_code;
    hi_u8 is_digital_cc;
    hi_u8 service_number;
    hi_u8 is_easy_reader;
    hi_u8 is_wide_aspect_ratio;
} pvr_pmt_closed_caption;

typedef struct {
    hi_u32 iso639_language_code; /* low 24-bit valid */
    hi_u8  ttx_type;
    hi_u8  ttx_magazine_number;
    hi_u8  ttx_page_number;
} pvr_pmt_ttx_des;

typedef struct {
    hi_u16 ttx_pid;
    hi_u8 des_tag;
    hi_u8 des_length;
    hi_u8 des_info_cnt;
    pvr_pmt_ttx_des ttx_des[TTX_DES_MAX];
} pvr_pmt_ttx;

typedef struct hipvr_prog_info
{
    hi_u32 magic_number;
    hi_u32 prog_id;          /* program ID */
    hi_u32 pmt_pid;          /*program PMT PID*/
    hi_u32 pcr_pid;          /*program PCR PID*/
    hi_u32 video_type;
    hi_u16 v_element_num;        /* video stream number */
    hi_u16 v_element_pid;        /* the first video stream PID*/
    hi_u32 audio_type;
    hi_u16 a_element_num;        /* audio stream number */
    hi_u16 a_element_pid;        /* the first audio stream PID*/
    pvr_pmt_audio audio_info[PROG_MAX_AUDIO];     /* multi-audio info, added by gaoyanfeng 00182102 */
    hi_u32 subt_type;            /*0---NONE,1---DVB,2---SCTE,3---BOTH*/
    hi_u16 subtitling_num;
    pvr_pmt_subtitle subtiting_info[PVR_SUBTITLING_MAX];
    pvr_pmt_scte_subtitle scte_subt_info;
    hi_u16 closed_caption_num;
    pvr_pmt_closed_caption closed_caption[CAPTION_SERVICE_MAX];
    hi_u16 aribcc_pid;
    hi_u16 ttx_num;
    pvr_pmt_ttx ttx_info[TTX_MAX];
    hi_unf_pvr_rec_attr rec_attr;
} pvr_prog_info;

/******************************* API declaration *****************************/
hi_s32 sample_pvr_rec_start(char *path, pmt_compact_prog *prog_info, hi_u32 demux_id, hi_handle *rec_chn);
hi_s32 sample_pvr_rec_stop(hi_handle rec_chn);
hi_s32 sample_pvr_start_live(hi_handle av, const pmt_compact_prog *prog_info);
hi_s32 sample_pvr_stop_live(hi_handle av);
hi_s32 sample_pvr_start_playback(const hi_char *file_name, hi_handle *play_chn, hi_handle av);
hi_void sample_pvr_stop_playback(hi_handle play_chn);
hi_s32 sample_pvr_switch_dmx_source(hi_u32 dmx_id, hi_u32 port_id);
hi_s32 sample_pvr_srch_file(hi_u32 dmx_id, hi_u32 port_id, const hi_char *file_name, pmt_compact_tbl **prog_tb1);
hi_s32 sample_pvr_av_init(hi_handle win, hi_handle *av, hi_handle *sound_track);
hi_s32 sample_pvr_av_deinit(hi_handle av, hi_handle win, hi_handle sound_track);
hi_s32 sample_pvr_cfg_avplay(hi_handle av, const pmt_compact_prog *prog_info);
hi_void sample_pvr_cb(hi_handle chn_id, hi_unf_pvr_event event_type, hi_s32 event_value,
    hi_void *usr_data, hi_u32 usr_data_len);
hi_s32 sample_pvr_register_cb(hi_void);
hi_void sample_pvr_set_tvp_flag(hi_bool is_support);
hi_void sample_pvr_set_cb_flag(hi_bool extra, hi_bool extend_tee);
hi_void sample_pvr_prog_rec_trans(pvr_prog_info *file_info, pmt_compact_prog *prog_info);
hi_void sample_pvr_prog_play_trans(pmt_compact_prog *prog_info, pvr_prog_info *file_info);

#ifdef HI_TEE_PVR_TEST_SUPPORT
hi_void sample_pvr_set_sec_buf_flag(hi_bool is_support);
hi_s32 sample_tee_init(hi_void);
hi_s32 sample_tee_deinit(hi_void);
#endif
hi_bool sample_pvr_check_rec_stop(hi_void);
hi_void sample_pvr_inform_rec_stop(hi_bool is_stop);
#ifdef __cplusplus
}
#endif
#endif /* __SAMPLE_PVR_COMMON_H__ */


