/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description: This is a sample for klad test
 * Author: Hisilicon hisecurity group
 * Create: 2019-09-19
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "hi_debug.h"
#include "hi_module.h"
#include "hi_unf_system.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_win.h"
#include "hi_unf_descrambler.h"
#include "hi_unf_klad.h"
#include "hi_unf_demux.h"
#include "hi_unf_keyslot.h"
#include "hi_adp_mpi.h"

#define TIME_MS2US 1000
#define TIME_MS2NS 1000000
#define TIME_S2MS  1000
#define TIME_S2US  1000000
#define TIME_S2NS  1000000000

#define KEY_LEN            16
#define TS_PACKET          (188 * 200)
#define TIME_OUT           1000

#define HI_ID_KLAD HI_ID_USR_START

#define HI_DBG_KLAD(fmt...)                 HI_DBG_PRINT(HI_ID_KLAD, fmt)
#define HI_FATAL_KLAD(fmt...)               HI_FATAL_PRINT(HI_ID_KLAD, fmt)
#define HI_ERR_KLAD(fmt...)                 HI_ERR_PRINT(HI_ID_KLAD, fmt)
#define HI_WARN_KLAD(fmt...)                HI_WARN_PRINT(HI_ID_KLAD, fmt)
#define HI_INFO_KLAD(fmt...)                HI_INFO_PRINT(HI_ID_KLAD, fmt)

#define print_err(val)                   HI_ERR_KLAD("%s\n", val)

#define print_dbg_hex(val)               HI_INFO_KLAD("%s = 0x%08x\n", #val, val)
#define print_dbg_hex2(x, y)             HI_INFO_KLAD("%s = 0x%08x %s = 0x%08x\n", #x, x, #y, y)
#define print_dbg_hex3(x, y, z)          HI_INFO_KLAD("%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #x, x, #y, y, #z, z)

#define print_err_hex(val)               HI_ERR_KLAD("%s = 0x%08x\n", #val, val)
#define print_err_hex2(x, y)             HI_ERR_KLAD("%s = 0x%08x %s = 0x%08x\n", #x, x, #y, y)
#define print_err_hex3(x, y, z)          HI_ERR_KLAD("%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #x, x, #y, y, #z, z)
#define print_err_hex4(w, x, y, z)       HI_ERR_KLAD("%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #w, \
                                                     w, #x, x, #y, y, #z, z)

#define print_dbg_func_hex(func, val)    HI_INFO_KLAD("call [%s]%s = 0x%08x\n", #func, #val, val)
#define print_dbg_func_hex2(func, x, y)  HI_INFO_KLAD("call [%s]%s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y)
#define print_dbg_func_hex3(func, x, y, z) \
    HI_INFO_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y, #z, z)
#define print_dbg_func_hex4(func, w, x, y, z) \
    HI_INFO_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #w,  w, #x, x, #y, y, #z, z)

#define print_err_func_hex(func, val)    HI_ERR_KLAD("call [%s]%s = 0x%08x\n", #func, #val, val)
#define print_err_func_hex2(func, x, y)  HI_ERR_KLAD("call [%s]%s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y)
#define print_err_func_hex3(func, x, y, z) \
    HI_ERR_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #x, x, #y, y, #z, z)
#define print_err_func_hex4(func, w, x, y, z) \
    HI_ERR_KLAD("call [%s]%s = 0x%08x %s = 0x%08x %s = 0x%08x %s = 0x%08x\n", #func, #w,  w, #x, x, #y, y, #z, z)

#define dbg_print_dbg_hex(val)           HI_DBG_KLAD("%s = 0x%08x\n", #val, val)
#define print_err_val(val)               HI_ERR_KLAD("%s = %d\n", #val, val)
#define print_err_point(val)             HI_ERR_KLAD("%s = %p\n", #val, val)
#define print_err_code(err_code)         HI_ERR_KLAD("return [0x%08x]\n", err_code)
#define print_warn_code(err_code)        HI_WARN_KLAD("return [0x%08x]\n", err_code)
#define print_err_func(func, err_code)   HI_ERR_KLAD("call [%s] return [0x%08x]\n", #func, err_code)

struct sample_klad_initial {
    pthread_mutex_t              lock;

    /* ts stream */
    FILE                         *ts_file;
    pthread_t                    ts_thread;
    hi_bool                      ts_thread_stop;
    pmt_compact_tbl              *prog_tbl;

    /* dmx */
    hi_u32                       dmx_id;
    hi_handle                    handle_ts_buf;
    /* descramble */
    hi_u32 vid_pid;
    hi_u32 aud_pid;
    hi_handle handle_descrambler;
    hi_handle handle_vid_chn_disp;
    hi_handle handle_aud_chn_disp;
    hi_handle handle_vid_chn_dsc;
    hi_handle handle_aud_chn_dsc;
    hi_bool descrambler_attached_ks;
    hi_bool avplay_attached_descrambler;

    /* window */
    hi_handle handle_win;
    /* avpley */
    hi_handle handle_avplay;

    /* track */
    hi_handle handle_track;
    /* keyslot */
    hi_handle handle_ks;
    /* klad */
    hi_handle handle_klad;

    /* keyladder config */
    hi_bool                      is_enc_cw;
    hi_u32                       klad_type;
    hi_u32                       owner_id;

    hi_unf_klad_alg_type         klad_alg;
    hi_unf_crypto_alg            engine_alg;

    hi_u8                        session_key1[KEY_LEN];
    hi_u8                        session_key2[KEY_LEN];
    hi_u8                        session_key3[KEY_LEN];
    hi_u8                        session_key4[KEY_LEN];
    hi_u32                       final_key_size;
    hi_u8                        enc_odd_key[KEY_LEN];
    hi_u8                        enc_even_key[KEY_LEN];
    hi_u8                        clr_odd_key[KEY_LEN];
    hi_u8                        clr_even_key[KEY_LEN];
    hi_u8                        clr_odd_iv[KEY_LEN];
    hi_u8                        clr_even_iv[KEY_LEN];
};

struct time_ns {
    hi_ulong tv_sec;
    hi_ulong tv_nsec;
};

static struct sample_klad_initial g_sample_klad = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    /* ts stream */
    .ts_thread_stop = HI_FALSE,

    /* dmx */
    .dmx_id  = 0,
    .descrambler_attached_ks = HI_FALSE,
    .avplay_attached_descrambler = HI_FALSE,

    /* keyladder config */
    .is_enc_cw  = HI_FALSE,
    .klad_type  = HI_UNF_KLAD_TYPE_CLEARCW,
    .klad_alg   = HI_UNF_KLAD_ALG_TYPE_MAX,
    .engine_alg = HI_UNF_CRYPTO_ALG_CSA2,

    .session_key1 = {0},
    .session_key2 = {0},
    .session_key3 = {0},
    .session_key4 = {0},
    .final_key_size = 0x8,
    .enc_odd_key  = {0},
    .enc_even_key = {0},
    .clr_odd_key  = { 0xff,0xff,0xff,0xfd,0xff,0xff,0xff,0xfd },
    .clr_even_key = { 0xff,0xff,0xff,0xfd,0xff,0xff,0xff,0xfd },
    .clr_odd_iv   = {0},
    .clr_even_iv  = {0},
};

struct sample_klad_initial *__get_sample_klad_initial(hi_void)
{
    return &g_sample_klad;
}

hi_void get_time(struct time_ns *time)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time->tv_sec = ts.tv_sec;
    time->tv_nsec = ts.tv_nsec;
}

hi_void get_cost(hi_char *str, struct time_ns *time_b, struct time_ns *time_e)
{
    if (time_b->tv_sec ==  time_e->tv_sec) {
        HI_INFO_KLAD("%ld.%09ld ns-->%ld.%09ld ns, cost:%ld.%ld ms <<%s\n",
                    time_b->tv_sec, time_b->tv_nsec, time_e->tv_sec, time_e->tv_nsec,
                    (time_e->tv_nsec - time_b->tv_nsec) / TIME_MS2NS,
                    (time_e->tv_nsec - time_b->tv_nsec) % TIME_MS2NS,
                    str);
    } else {
        HI_INFO_KLAD("%ld.%09ld ns-->%ld.%09ld ns, cost:%ld.%ld ms <<%s\n",
                    time_b->tv_sec, time_b->tv_nsec, time_e->tv_sec, time_e->tv_nsec,
                    ((time_e->tv_sec - time_b->tv_sec) * TIME_S2NS + time_e->tv_nsec - time_b->tv_nsec) / TIME_MS2NS,
                    ((time_e->tv_sec - time_b->tv_sec) * TIME_S2NS + time_e->tv_nsec - time_b->tv_nsec) % TIME_MS2NS,
                    str);
    }
}

hi_void get_curr_cost(hi_char *str, struct time_ns *time_b)
{
    struct time_ns time_e;

    get_time(&time_e);
    get_cost(str, time_b, &time_e);
}

hi_void print_buffer(hi_char *string, hi_u8 *input, hi_u32 length)
{
    hi_u32 i;

    if (string != NULL) {
        printf("%s\n", string);
    }

    for (i = 0; i < length; i++) {
        if ((i % KEY_LEN == 0) && (i != 0)) {
            printf("\n");
        }
        printf("0x%02x ", input[i]);
    }
    printf("\n");
}

static hi_void pthread_ts_send(hi_void *args)
{
    hi_s32 ret;
    hi_u32 readlen;
    hi_unf_stream_buf buf;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    pthread_mutex_lock(&initial->lock);

    while (!initial->ts_thread_stop) {

        pthread_mutex_unlock(&initial->lock);
        usleep(10); /* deley 10 us, make sure mutex can be locked by other thread. */
        pthread_mutex_lock(&initial->lock);

        ret = hi_unf_dmx_get_ts_buffer(initial->handle_ts_buf, TS_PACKET, &buf, TIME_OUT);
        if (ret != HI_SUCCESS) {
            continue;
        }

        readlen = fread(buf.data, sizeof(hi_s8), TS_PACKET, initial->ts_file);
        if (readlen <= 0) {
            printf("read ts file end and rewind!\n");
            rewind(initial->ts_file);
            continue;
        }

        ret = hi_adp_dmx_push_ts_buf(initial->handle_ts_buf, &buf, 0, readlen);
        if (ret != HI_SUCCESS) {
            print_err_func(hi_adp_dmx_push_ts_buf, ret);
        }
    }

    pthread_mutex_unlock(&initial->lock);
    ret = hi_unf_dmx_reset_ts_buffer(initial->handle_ts_buf);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_dmx_reset_ts_buffer, ret);
    }
}

static hi_void show_usage(hi_void)
{
    printf("./sample_ca_tsplay tsfilename\n");
}

static hi_s32 process_initialize(hi_void)
{
    hi_s32 ret;

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_unf_keyslot_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_keyslot_init, ret);
        goto sys_deinit;
    }
    ret = hi_unf_klad_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_klad_init, ret);
        goto ks_deinit;
    }

    ret = hi_adp_snd_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_adp_snd_init, ret);
        goto ca_deinit;
    }

    ret = hi_adp_disp_init(HI_UNF_VIDEO_FMT_1080P_60);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_adp_disp_init, ret);
        goto snd_deinit;
    }

    ret = hi_adp_win_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_adp_win_init, ret);
        goto disp_deinit;
    }

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_dmx_init, ret);
        goto vo_deinit;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        print_err_func(hi_adp_avplay_init, ret);
        goto dmx_deinit;
    }
    return HI_SUCCESS;

dmx_deinit:
    hi_unf_dmx_deinit();

vo_deinit:
    hi_adp_win_deinit();

disp_deinit:
    hi_adp_disp_deinit();

snd_deinit:
    hi_adp_snd_deinit();

ca_deinit:
    hi_unf_klad_deinit();

ks_deinit:
    hi_unf_keyslot_deinit();
sys_deinit:
    hi_unf_sys_deinit();
    return ret;
}

static hi_void process_terminate(hi_void)
{
    hi_unf_avplay_deinit();

    hi_unf_dmx_deinit();

    hi_adp_win_deinit();

    hi_adp_disp_deinit();

    hi_adp_snd_init();

    hi_unf_klad_deinit();

    hi_unf_keyslot_deinit();

    hi_unf_sys_deinit();

    return ;
}

static hi_s32 __process_descramble_create(hi_void)
{

    hi_s32 ret;
    hi_unf_dmx_desc_attr descrambler_attr = {0};
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    descrambler_attr.entropy_reduction = HI_UNF_DMX_CA_ENTROPY_REDUCTION_CLOSE;
    descrambler_attr.is_create_keyslot = HI_FALSE;

    ret = hi_unf_dmx_desc_create(initial->dmx_id, &descrambler_attr, &initial->handle_descrambler);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_dmx_desc_create, initial->dmx_id, ret);
    }
    return ret;
}

static hi_void __process_descramble_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_dmx_desc_destroy(initial->handle_descrambler);
}

/*
* hi_unf_dmx_get_play_chan_pid and hi_unf_dmx_get_pid_chan_handle can be instead by api hi_mpi_dmx_play_get_pid_ch
*/
static hi_s32 __process_avplay_attach_descrambler(hi_void)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_avplay_get_demux_handle(initial->handle_avplay, HI_UNF_AVPLAY_DEMUX_HANDLE_VID,
                                         &initial->handle_vid_chn_disp);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_get_demux_handle, initial->handle_avplay, ret);
        goto out;
    }

    ret = hi_unf_avplay_get_demux_handle(initial->handle_avplay, HI_UNF_AVPLAY_DEMUX_HANDLE_AUD,
                                         &initial->handle_aud_chn_disp);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_get_demux_handle, initial->handle_avplay, ret);
        goto out;
    }

    ret =  hi_unf_dmx_get_play_chan_pid(initial->handle_vid_chn_disp, &initial->vid_pid);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_dmx_get_play_chan_pid, initial->handle_vid_chn_disp, ret);
        goto out;
    }

    ret =  hi_unf_dmx_get_play_chan_pid(initial->handle_aud_chn_disp, &initial->aud_pid);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_dmx_get_play_chan_pid, initial->handle_aud_chn_disp, ret);
        goto out;
    }

    ret =  hi_unf_dmx_get_pid_chan_handle(initial->dmx_id, initial->vid_pid, &initial->handle_vid_chn_dsc);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_unf_dmx_get_pid_chan_handle, initial->dmx_id, initial->vid_pid, ret);
        goto out;
    }

    ret =  hi_unf_dmx_get_pid_chan_handle(initial->dmx_id, initial->aud_pid, &initial->handle_aud_chn_dsc);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_unf_dmx_get_pid_chan_handle, initial->dmx_id, initial->aud_pid, ret);
        goto out;
    }

    ret = hi_unf_dmx_desc_attach_pid_chan(initial->handle_descrambler, initial->handle_vid_chn_dsc);
    if (ret != HI_SUCCESS) {
        print_err_hex3(initial->handle_descrambler, initial->handle_vid_chn_dsc, ret);
        goto out;
    }

    ret = hi_unf_dmx_desc_attach_pid_chan(initial->handle_descrambler, initial->handle_aud_chn_dsc);
    if (ret != HI_SUCCESS) {
        print_err_hex3(initial->handle_descrambler, initial->handle_aud_chn_dsc, ret);
        goto detach_vid_descramble;
    }
    return HI_SUCCESS;

detach_vid_descramble:
    hi_unf_dmx_desc_detach_pid_chan(initial->handle_descrambler, initial->handle_vid_chn_dsc);
out:
    return ret;
}

static hi_void __process_avplay_detach_descrambler(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_dmx_desc_detach_pid_chan(initial->handle_descrambler, initial->handle_vid_chn_dsc);
    hi_unf_dmx_desc_detach_pid_chan(initial->handle_descrambler, initial->handle_aud_chn_dsc);
}

static hi_s32 __process_descrambler_attach_ks(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    return hi_unf_dmx_desc_attach_key_slot(initial->handle_descrambler, initial->handle_ks);
}

static hi_void __process_descrambler_detach_ks(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_dmx_desc_detach_key_slot(initial->handle_descrambler);

}

static hi_s32 __process_avplay_create_open(hi_void)
{
    hi_s32 ret;
    hi_unf_avplay_attr avplay_attr;
    hi_unf_sync_attr sync_attr;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_avplay_get_default_config(&avplay_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_avplay_get_default_config, ret);
        goto out;
    }

    avplay_attr.demux_id = initial->dmx_id;
    ret = hi_unf_avplay_create(&avplay_attr, &initial->handle_avplay);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_avplay_create, ret);
        goto out;
    }

    ret = hi_unf_avplay_chan_open(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_chan_open, initial->handle_avplay, ret);
        goto avplay_destroy;
    }

    ret = hi_unf_avplay_chan_open(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_chan_open, initial->handle_avplay, ret);
        goto vchn_close;
    }
    ret = hi_unf_avplay_get_attr(initial->handle_avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_get_attr, initial->handle_avplay, ret);
        goto achn_close;
    }
    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_NONE;
    ret = hi_unf_avplay_set_attr(initial->handle_avplay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_set_attr, initial->handle_avplay, ret);
        goto achn_close;
    }
    return HI_SUCCESS;
achn_close:
    hi_unf_avplay_chan_close(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
vchn_close:
    hi_unf_avplay_chan_close(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
avplay_destroy:
    hi_unf_avplay_destroy(initial->handle_avplay);
out:
    return ret;
}

static hi_void __process_avplay_close_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_avplay_chan_close(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_chan_close(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    hi_unf_avplay_destroy(initial->handle_avplay);
}

static hi_s32 __process_ts_buffer_create(hi_void)
{
    hi_s32 ret;
    hi_unf_dmx_ts_buffer_attr attr;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_dmx_attach_ts_port(initial->dmx_id, HI_UNF_DMX_PORT_RAM_0);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_dmx_attach_ts_port, initial->dmx_id, ret);
        goto out;
    }
    attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    attr.buffer_size = 0x200000;
    ret = hi_unf_dmx_create_ts_buffer(HI_UNF_DMX_PORT_RAM_0, &attr, &initial->handle_ts_buf);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_dmx_create_ts_buffer, ret);
        goto ts_detach;
    }
    return HI_SUCCESS;
ts_detach:
    hi_unf_dmx_detach_ts_port(initial->dmx_id);
out:
    return ret;
}

static hi_void __process_ts_buffer_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_dmx_destroy_ts_buffer(initial->handle_ts_buf);
    hi_unf_dmx_detach_ts_port(initial->dmx_id);
}

static hi_s32 __process_track_create_attach(hi_void)
{
    hi_s32 ret;
    hi_unf_audio_track_attr track_attr;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_snd_get_default_track_attr, ret);
        goto out;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, &initial->handle_track);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_snd_create_track, initial->handle_track, ret);
        goto out;
    }

    ret = hi_unf_snd_attach(initial->handle_track, initial->handle_avplay);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_unf_snd_attach, initial->handle_track, initial->handle_avplay, ret);
        goto destroy;
    }
    return HI_SUCCESS;

destroy:
    hi_unf_snd_destroy_track(initial->handle_track);
out:
    return ret;
}

static hi_void __process_track_detach_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_snd_detach(initial->handle_track, initial->handle_avplay);
    hi_unf_snd_destroy_track(initial->handle_track);
}

static hi_s32 __process_win_create_attach_enable(hi_void)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_adp_win_create((hi_unf_video_rect *)HI_NULL, &initial->handle_win);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_adp_win_create, ret);
        goto out;
    }

    ret = hi_unf_win_attach_src(initial->handle_win, initial->handle_avplay);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_unf_win_attach_src, initial->handle_win, initial->handle_avplay, ret);
        goto win_destroy;
    }

    ret = hi_unf_win_set_enable(initial->handle_win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_win_set_enable, initial->handle_win, ret);
        goto win_detach;
    }
    return HI_SUCCESS;
win_detach:
    hi_unf_win_detach_src(initial->handle_win, initial->handle_avplay);
win_destroy:
    hi_unf_win_destroy(initial->handle_win);
out:
    return ret;
}

static hi_void __process_win_disable_detach_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_win_set_enable(initial->handle_win, HI_FALSE);
    hi_unf_win_detach_src(initial->handle_win, initial->handle_avplay);
    hi_unf_win_destroy(initial->handle_win);
}

static hi_s32 __process_klad_ks_create(hi_void)
{
    hi_s32 ret;
    hi_unf_keyslot_attr attr;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_klad_create(&initial->handle_klad);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_klad_create, ret);
        goto out;
    }

    attr.type = HI_UNF_KEYSLOT_TYPE_TSCIPHER;
    attr.secure_mode = HI_UNF_KEYSLOT_SECURE_MODE_NONE;
    ret = hi_unf_keyslot_create(&attr, &initial->handle_ks);
    if (ret != HI_SUCCESS) {
        print_err_func(hi_unf_keyslot_create, ret);
        goto klad_destroy;
    }
    return HI_SUCCESS;

klad_destroy:
    hi_unf_klad_destroy(initial->handle_klad);
out:
    return ret;
}

static hi_void __process_klad_ks_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_unf_keyslot_destroy(initial->handle_ks);

    hi_unf_klad_destroy(initial->handle_klad);
}

static hi_s32 process_create(hi_void)
{
    hi_s32 ret;

    ret = __process_ts_buffer_create();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_ts_buffer_create, ret);
        goto out;
    }
    ret = __process_klad_ks_create();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_klad_ks_create, ret);
        goto ts_buf_destroy;
    }
    ret = __process_avplay_create_open();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_avplay_create_open, ret);
        goto klad_ks_destroy;
    }

    ret = __process_win_create_attach_enable();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_win_create_attach_enable, ret);
        goto avplay_destroy;
    }

    ret = __process_descramble_create();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_descramble_create, ret);
        goto win_destroy;
    }
    ret = __process_track_create_attach();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_track_create_attach, ret);
        goto descramble_destroy;
    }
    return HI_SUCCESS;
descramble_destroy:
    __process_descramble_destroy();
win_destroy:
    __process_win_disable_detach_destroy();
avplay_destroy:
    __process_avplay_close_destroy();
klad_ks_destroy:
    __process_klad_ks_destroy();
ts_buf_destroy:
    __process_ts_buffer_destroy();
out:
    return ret;
}

static hi_void process_destroy(hi_void)
{
    __process_track_detach_destroy();
    __process_descramble_destroy();
    __process_win_disable_detach_destroy();
    __process_avplay_close_destroy();
    __process_klad_ks_destroy();
    __process_ts_buffer_destroy();
}

static hi_s32 __process_fmt_create_phrase(hi_void)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_adp_search_init();

    ret = hi_adp_search_get_all_pmt(initial->dmx_id, &initial->prog_tbl);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_adp_search_get_all_pmt, initial->dmx_id, ret);
    }
    return ret;
}

static hi_void __process_fmt_destroy(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    hi_adp_search_free_all_pmt(initial->prog_tbl);
    initial->prog_tbl = HI_NULL;
    hi_adp_search_de_init();
}


static hi_s32 __process_descramble_attach(hi_void)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = __process_avplay_attach_descrambler();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_avplay_attach_descrambler, ret);
        goto out;
    }
    ret = __process_descrambler_attach_ks();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_descrambler_attach_ks, ret);
        goto detach_descrambler;
    }
    initial->avplay_attached_descrambler = HI_TRUE;
    initial->descrambler_attached_ks = HI_TRUE;
    return HI_SUCCESS;

detach_descrambler:
    __process_avplay_detach_descrambler();
out:
    return ret;
}

static hi_void __process_descramble_detach(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    if (initial->descrambler_attached_ks == HI_TRUE) {
        __process_descrambler_detach_ks();
    }
    if (initial->avplay_attached_descrambler == HI_TRUE) {
        __process_avplay_detach_descrambler();
    }
}

static hi_s32 __process_avplay_set_vid_pid(pmt_compact_tbl *prog_tbl, hi_u32 prog)
{
    hi_u32 ret;
    hi_u32 prog_num = prog;
    hi_u32 vid_pid;
    hi_unf_vcodec_type vid_type;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    prog_num = prog_num % prog_tbl->prog_num;
    if (prog_tbl->proginfo[prog_num].v_element_num <= 0) {
        return HI_SUCCESS;
    }
    vid_pid = prog_tbl->proginfo[prog_num].v_element_pid;
    vid_type = prog_tbl->proginfo[prog_num].video_type;

    ret = hi_adp_avplay_set_vdec_attr(initial->handle_avplay, vid_type, HI_UNF_VDEC_WORK_MODE_NORMAL);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_adp_avplay_set_vdec_attr, initial->handle_avplay, vid_type, ret);
        return ret;
    }

    ret = hi_unf_avplay_set_attr(initial->handle_avplay, HI_UNF_AVPLAY_ATTR_ID_VID_PID, &vid_pid);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_set_attr, initial->handle_avplay, ret);
        return ret;
    }
    return HI_SUCCESS;
}

static hi_s32 __process_avplay_set_aud_pid(pmt_compact_tbl *prog_tbl, hi_u32 prog)
{
    hi_u32 ret;
    hi_u32 prog_num = prog;
    hi_u32 aud_pid;
    hi_u32 aud_type;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    prog_num = prog_num % prog_tbl->prog_num;

    if (prog_tbl->proginfo[prog_num].a_element_num <= 0) {
        return HI_SUCCESS;
    }
    aud_pid  = prog_tbl->proginfo[prog_num].a_element_pid;
    aud_type = prog_tbl->proginfo[prog_num].audio_type;

    ret  = hi_adp_avplay_set_adec_attr(initial->handle_avplay, aud_type);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_adp_avplay_set_adec_attr, initial->handle_avplay, aud_type, ret);
        return ret;
    }
    ret = hi_unf_avplay_set_attr(initial->handle_avplay, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &aud_pid);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_set_attr, initial->handle_avplay, ret);
        return ret;
    }
    return HI_SUCCESS;
}

hi_s32 __process_avplay_set_pid(pmt_compact_tbl *prog_tbl, hi_u32 prog)
{
    hi_u32 ret;

    ret = __process_avplay_set_vid_pid(prog_tbl, prog);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(__process_avplay_set_vid_pid, prog, ret);
        return ret;
    }
    ret = __process_avplay_set_aud_pid(prog_tbl, prog);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(__process_avplay_set_aud_pid, prog, ret);
        return ret;
    }
    return HI_SUCCESS;
}


static hi_s32 __process_prog_start_vid(hi_void)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_avplay_start(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_start, initial->handle_avplay, ret);
    }
    return ret;
}

static hi_s32 __process_prog_start_aud(hi_void)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    ret = hi_unf_avplay_start(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_avplay_start, initial->handle_avplay, ret);
        return ret;
    }
    return ret;
}

static hi_void __process_prog_stop(hi_void)
{
    hi_unf_avplay_stop_opt stop;
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    stop.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop.timeout = 0;
    hi_unf_avplay_stop(initial->handle_avplay, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop);
}

static hi_s32 __process_klad_send_key(hi_void)
{
    hi_s32 ret;
    hi_unf_klad_attr attr_klad = {0};
    hi_unf_klad_clear_key key_clear = {0};
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    attr_klad.klad_cfg.owner_id = 0;
    attr_klad.klad_cfg.klad_type = initial->klad_type;
    attr_klad.key_cfg.decrypt_support = 1;
    attr_klad.key_cfg.encrypt_support = 1;
    attr_klad.key_cfg.engine = initial->engine_alg;
    ret = hi_unf_klad_set_attr(initial->handle_klad, &attr_klad);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_klad_set_attr, initial->handle_klad, ret);
        goto out;
    }
    ret = hi_unf_klad_attach(initial->handle_klad, initial->handle_ks);
    if (ret != HI_SUCCESS) {
        print_err_func_hex3(hi_unf_klad_attach, initial->handle_klad, initial->handle_ks, ret);
        goto out;
    }

    key_clear.odd = HI_FALSE;
    key_clear.key_size = initial->final_key_size;
    memcpy(key_clear.key, initial->clr_even_key, KEY_LEN);
    ret = hi_unf_klad_set_clear_key(initial->handle_klad, &key_clear);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_klad_set_clear_key, initial->handle_klad, ret);
        goto detach;
    }
    key_clear.odd = HI_TRUE;
    key_clear.key_size = initial->final_key_size;
    memcpy(key_clear.key, initial->clr_odd_key, KEY_LEN);
    ret = hi_unf_klad_set_clear_key(initial->handle_klad, &key_clear);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(hi_unf_klad_set_clear_key, initial->handle_klad, ret);
        goto detach;
    }
detach:
    hi_unf_klad_detach(initial->handle_klad, initial->handle_ks);
out:
    return ret;
}

static hi_s32 __process_prog_switch(hi_u32 prog_num)
{
    hi_s32 ret;
    struct sample_klad_initial *initial = __get_sample_klad_initial();
    struct time_ns time_b;

    printf(">>>>>>>>>>>>>>>> switch %d\n", prog_num);
    get_time(&time_b);
    pthread_mutex_lock(&initial->lock);

    /* stop avplay */
    __process_prog_stop();

    /* detach avpley~descramber and descramber~keyslot */
    __process_descramble_detach();

    /* set new program number vid/aud pid */
    ret = __process_avplay_set_pid(initial->prog_tbl, prog_num);
    if (ret != HI_SUCCESS) {
        print_err_func_hex2(__process_avplay_set_pid, prog_num, ret);
        goto out;
    }

    /* attach avpley~descramber and descramber~keyslot */
    ret = __process_descramble_attach();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_descramble_attach, ret);
        goto out;
    }
    /* send key */
    ret = __process_klad_send_key();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_klad_send_key, ret);
        goto out;
    }

    /* reset and rewind strean/buffer */
    rewind(initial->ts_file);

    hi_unf_dmx_reset_ts_buffer(initial->handle_ts_buf);

    /* start avplay */
    ret = __process_prog_start_vid();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_prog_start_vid, ret);
        goto out;
    }

    ret = __process_prog_start_aud();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_prog_start_aud, ret);
        goto stop;
    }
    get_curr_cost("witch prog", &time_b);

    pthread_mutex_unlock(&initial->lock);
    printf("<<<<<<<<<<<<<<<\n");
    return HI_SUCCESS;
stop:
    __process_prog_stop();
out:
    pthread_mutex_unlock(&initial->lock);

    return ret;
}

static hi_s32 __process_klad_create_send_thread(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    initial->ts_thread_stop = HI_FALSE;
    pthread_create(&initial->ts_thread, HI_NULL, (hi_void *)pthread_ts_send, HI_NULL);
    return HI_SUCCESS;
}

static hi_void __process_klad_destroy_send_thread(hi_void)
{
    struct sample_klad_initial *initial = __get_sample_klad_initial();

    initial->ts_thread_stop = HI_TRUE;
    pthread_join(initial->ts_thread, HI_NULL);
}

static hi_void process_ts_dsc_cmd_switch(hi_void)
{
    hi_s32 ret;
    hi_char input_cmd[0x20];
    hi_u32 prog_num;

    while (1) {
        printf("please input 'q' to quit!\n");
        memset(input_cmd, 0, 0X20);
        (hi_void)fgets(input_cmd, 0X20, stdin);
        if (input_cmd[0] == 'q') {
            printf("prepare to quit!\n");
            break;
        }

        prog_num = atoi(input_cmd);
        if (prog_num == 0) {
            prog_num = 1;
        }
        ret = __process_prog_switch(prog_num - 1);
        if (ret != HI_SUCCESS) {
            print_err_func(__process_prog_switch, ret);
        }
    }
}

static hi_s32 process_ts_dsc_play(hi_void)
{
    hi_s32 ret;

    ret = __process_klad_create_send_thread();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_klad_create_send_thread, ret);
        goto out;
    }
    ret = __process_fmt_create_phrase();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_fmt_create_phrase, ret);
        goto thread_stop;
    }

    ret = __process_klad_send_key();
    if (ret != HI_SUCCESS) {
        print_err_func(__process_klad_send_key, ret);
        goto fmt_destroy;
    }
    ret = __process_prog_switch(0);
    if (ret != HI_SUCCESS) {
        print_err_func(__process_prog_switch, ret);
        goto fmt_destroy;
    }

    process_ts_dsc_cmd_switch();

    __process_prog_stop();

fmt_destroy:
    __process_fmt_destroy();
thread_stop:
    __process_klad_destroy_send_thread();
out:
    return ret;
}

static hi_s32 process_ts_dsc_start(hi_void)
{
    hi_s32 ret;

    ret = process_create();
    if (ret != HI_SUCCESS) {
        print_err_func(process_create, ret);
        goto out;
    }
    ret = process_ts_dsc_play();
    if (ret != HI_SUCCESS) {
        print_err_func(process_ts_dsc_play, ret);
        goto destroy;
    }

destroy:
    process_destroy();
out:
    return ret;
}

hi_s32 main(hi_s32 argc, hi_char *argv[])
{
    hi_s32 ret = HI_FAILURE;

    struct sample_klad_initial *initial = __get_sample_klad_initial();

    if (argc < 2) { /* Check the value of argv is less than 2. */
        show_usage();
        goto out;
    }

    initial->ts_file = fopen(argv[1], "rb");
    if (initial->ts_file  == HI_NULL) {
        printf("open file %s error!\n", argv[1]);
        goto out;
    }
    printf("open file %s.\n", argv[1]);

    ret = process_initialize();
    if (ret != HI_SUCCESS) {
        print_err_func(process_initialize, ret);
        goto close;
    }

    ret = process_ts_dsc_start();
    if (ret != HI_SUCCESS) {
        print_err_func(process_ts_dsc_start, ret);
    }

    process_terminate();

close:
    fclose(initial->ts_file);

out:
    return ret;
}


