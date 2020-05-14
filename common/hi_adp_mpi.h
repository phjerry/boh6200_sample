#ifndef  __HI_ADP_MPI_H__
#define  __HI_ADP_MPI_H__

#include "hi_type.h"
#include "hi_unf_video.h"
#include "hi_unf_audio.h"
#include "hi_unf_system.h"
#include "hi_unf_memory.h"
#include "hi_unf_avplay.h"
#include "hi_unf_win.h"
#include "hi_adp.h"
#include "hi_adp_search.h"
#include "hi_adp_boardcfg.h"
#include "hi_unf_ai.h"
#include "hi_svr_dispmng.h"

#define TSPLAY_SUPPORT_VID_CHAN
#define TSPLAY_SUPPORT_AUD_CHAN

#define DOLBYPLUS_HACODEC_SUPPORT
#define SLIC_AUDIO_DEVICE_ENABLE

#define G711_FRAME_LEN 320

/********************************* Demux Common Interface *******************************/
hi_s32 hi_adp_demux_init(hi_u32 dmx_port_id, hi_u32 ts_port_id);

hi_s32 hi_adp_demux_deinit(hi_u32 dmx_port_id);

/************************************DISPLAY  Common Interface*******************************/
hi_unf_video_format hi_adp_str2fmt(const hi_char* format_str);

hi_s32 hi_adp_disp_set_format(const hi_svr_dispmng_display_id id, const hi_unf_video_format format);

hi_s32 hi_adp_disp_init(const hi_unf_video_format format);

hi_s32 hi_adp_disp_deinit(hi_void);


/****************************VO  Common Interface********************************************/
hi_s32 hi_adp_win_init(hi_void);

hi_s32 hi_adp_win_create(const hi_unf_video_rect* rect, hi_handle* win);

hi_s32 hi_adp_win_create_ext(hi_unf_video_rect *rect, hi_handle *win, hi_svr_dispmng_display_id id);

hi_s32 hi_adp_win_deinit(hi_void);

/*****************************************SOUND  common interface************************************/
hi_s32 hi_adp_snd_init(hi_void);
hi_s32 hi_adp_snd_deinit(hi_void);

#ifdef HI_AUDIO_AI_SUPPORT
/*only support single AI chn*/
hi_s32 hi_adp_ai_init(hi_unf_ai_port ai_src, hi_handle *ai_handle, hi_handle *track_slave, hi_handle *a_track_vir);
hi_s32 hi_adp_ai_de_init(hi_handle h_ai, hi_handle h_ai_slave, hi_handle h_ai_vir);
#endif

/**************************************AVPLAY  common interface***************************************/
hi_s32 hi_adp_avplay_init();

hi_s32 hi_adp_avplay_create(hi_handle *avplay,
                            hi_u32 dmx_id,
                            hi_unf_avplay_stream_type stream_type,
                            hi_u32 channel_flag);

hi_s32 hi_adp_avplay_set_vdec_attr(hi_handle avplay, hi_unf_vcodec_type type, hi_unf_vdec_work_mode mode);

hi_s32 hi_adp_avplay_set_adec_attr(hi_handle avplay, hi_u32 acodec_id);

hi_s32 hi_adp_avplay_play_prog(hi_handle avplay, pmt_compact_tbl *tbl, hi_u32 prog_id, hi_u32 channel_flag);

hi_s32 hi_adp_avplay_switch_aud(hi_handle avplay, hi_u32 aud_pid, hi_u32 acodec_id);

hi_s32 hi_adp_dmx_attach_ts_port(hi_u32 dmx_id, hi_u32 tuner_id);

hi_s32 hi_adp_dmx_push_ts_buf(hi_handle ts_buf_handle, hi_unf_stream_buf *buf, hi_u32 start_pos, hi_u32 valid_len);

#endif

