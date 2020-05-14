/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:record AllTs
 * Author: sdk
 * Create: 2019-08-20
 */


#include "hi_adp.h"
#include "hi_adp_boardcfg.h"
#include "hi_adp_mpi.h"
#include "hi_adp_frontend.h"
#include "hi_adp_pvr.h"

#define MAX_REC_FILE_SIZE (2 * 1024 *1024*1024LLU) /* 2G */

hi_u32 g_tuner_freq;
hi_u32 g_tuner_srate;
hi_u32 g_third_param;

extern hi_bool g_is_rec_stop;
static pthread_t g_cmd_thd;

hi_s32 dmx_init(hi_u32 tuner_id)
{
    hi_s32 ret;

    ret = hi_unf_dmx_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_init failed.\n");
        return ret;
    }

    ret = hi_adp_dmx_attach_ts_port(PVR_DMX_ID_REC, tuner_id);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_dmx_attach_ts_port for REC failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 dmx_deinit(hi_void)
{
    hi_s32 ret;

    ret = hi_unf_dmx_detach_ts_port(PVR_DMX_ID_REC);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_detach_ts_port failed.\n");
        return ret;
    }

    ret = hi_unf_dmx_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_deinit failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_s32 pvr_rec_all_ts_start(char *path, hi_u32 demux_id, hi_u32 freq, hi_u64 max_size_byte, hi_u32 *rec_chn_id)
{
    hi_u32 rec_chn;
    hi_s32 ret = HI_SUCCESS;
    hi_unf_pvr_rec_attr attr;
    hi_char file_name[PVR_MAX_FILENAME_LEN];


    memset(&attr, 0x00, sizeof(attr));
    sprintf(file_name, "rec_freq_%d.ts", freq);

    sprintf(attr.file_name.name, "%s/", path);

    strcat(attr.file_name.name, file_name);

    pvr_sample_printf("record file name:%s\n", attr.file_name.name);

    attr.file_name.name_len = strlen(attr.file_name.name);
    attr.demux_id = demux_id;
    attr.rec_buf_size = PVR_STUB_TSDATA_SIZE;
    attr.stream_type = HI_UNF_PVR_STREAM_TYPE_ALL_TS;
    attr.is_rewind = HI_FALSE;
    attr.max_size_byte= max_size_byte; /* source */
    attr.usr_data_size = 0;
    attr.index_pid = 0x1fff;
    attr.index_type = HI_UNF_PVR_REC_INDEX_TYPE_NONE;
    attr.index_vid_type = HI_UNF_VCODEC_TYPE_MPEG2;
    SAMPLE_RUN(hi_unf_pvr_rec_chan_create(&rec_chn, &attr), ret);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    SAMPLE_RUN(hi_unf_pvr_rec_chan_start(rec_chn), ret);
    if (ret != HI_SUCCESS) {
        hi_unf_pvr_rec_chan_destroy(rec_chn);
        return ret;
    }

    *rec_chn_id = rec_chn;

    return HI_SUCCESS;
}


hi_s32 pvr_rec_all_ts_stop(hi_u32 rec_chn_id)
{
    hi_s32 ret;
    hi_unf_pvr_rec_attr rec_attr;
    hi_unf_pvr_rec_stop_opt opt = {0};
    opt.mode = HI_UNF_PVR_REC_STOP_WITH_FLUSH;

    SAMPLE_RUN(hi_unf_pvr_rec_chan_get_attr(rec_chn_id, &rec_attr), ret);
    SAMPLE_RUN(hi_unf_pvr_rec_chan_stop(rec_chn_id, &opt), ret);
    SAMPLE_RUN(hi_unf_pvr_rec_chan_destroy(rec_chn_id), ret);

    return ret;
}

hi_void *cmd_thread(hi_void *args)
{
    hi_char input_cmd[32];

    while (sample_pvr_check_rec_stop() == HI_FALSE) {
        pvr_sample_printf("please input the 'q' to quit.\n");
        SAMPLE_GET_INPUTCMD(input_cmd);

        if (input_cmd[0] == 'q') {
            pvr_sample_printf("now exit!\n");
            sample_pvr_inform_rec_stop(HI_TRUE);
        } else {
            sleep(1);
            continue;
        }
    }

    return HI_NULL;
}


hi_s32 main(hi_s32 argc,hi_char *argv[])
{
    hi_s32 ret;
    hi_u32 rec_chn;
    hi_unf_pvr_rec_status rec_status;
    hi_u32 i = 0;
    hi_u32 tuner;

    if (argc == 6) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        tuner = strtol(argv[5], HI_NULL, 0);
    } else if (argc == 5) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = strtol(argv[4], HI_NULL, 0);
        tuner = 0;
    } else if(argc == 4) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = strtol(argv[3], HI_NULL, 0);
        g_third_param = (g_tuner_freq > 1000) ? 0 : 64;
        tuner = 0;
    } else if(argc == 3) {
        g_tuner_freq = strtol(argv[2], HI_NULL, 0);
        g_tuner_srate = (g_tuner_freq > 1000) ? 27500 : 6875;
        g_third_param = (g_tuner_freq > 1000) ? 0 : 64;
        tuner = 0;
    } else {
        printf("Usage: %s path freq [srate] [qamtype or polarization] [Tuner]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n"
                "       Tuner: value can be 0, 1, 2, 3\n", argv[0]);

        printf("Example: %s ./ 618 6875 64 0\n", argv[0]);
        printf("  Board HI3796CDMO1B support 3 tuners: \n");
        printf("    Satellite : %s ./ 3840 27500 0  0 0\n", argv[0]);
        printf("    Cable     : %s ./ 618 6875 64   1 0\n", argv[0]);
        printf("    Terestrial: %s ./ 474 8000 64   2 0\n", argv[0]);
        return HI_FAILURE;
    }

    hi_unf_sys_init();

    //hi_adp_mce_exit();

    ret = hi_adp_frontend_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call HIADP_Demux_Init failed.\n");
        return ret;
    }

    ret = hi_adp_frontend_connect(tuner,g_tuner_freq,g_tuner_srate,g_third_param);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_frontend_connect failed.\n");
        return ret;
    }


    ret = dmx_init(tuner);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call dmx_init failed.\n");
        return ret;
    }

    ret = hi_unf_pvr_rec_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_mpi_pvr_rec_init failed.\n");
        return ret;
    }

    ret = sample_pvr_register_cb();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call sample_pvr_register_cb failed.\n");
        return ret;
    }

    pvr_rec_all_ts_start(argv[1], PVR_DMX_ID_REC, g_tuner_freq, MAX_REC_FILE_SIZE, &rec_chn);
    pthread_create(&g_cmd_thd, HI_NULL, cmd_thread, HI_NULL);

    while (sample_pvr_check_rec_stop() == HI_FALSE) {
        sleep(1);

        if (i % 5 == 0) {
            ret = hi_unf_pvr_rec_chan_get_status(rec_chn, &rec_status);
            if (ret != HI_SUCCESS) {
                pvr_sample_printf("call hi_unf_pvr_rec_chan_get_status failed.\n");
                break;
            }

            pvr_sample_printf("== now record: \n");
            pvr_sample_printf("      size:%lld M\n", rec_status.cur_write_pos / 1024 / 1024);
            pvr_sample_printf("      time:%d s\n", rec_status.cur_time_ms / 1000);
            pvr_sample_printf("      buff:%d/%d \n", rec_status.rec_buf_status.used_size,
                rec_status.rec_buf_status.total_size);
        }
        i++;
    }

    sample_pvr_inform_rec_stop(HI_TRUE);
    pthread_join(g_cmd_thd, HI_NULL);
    pvr_rec_all_ts_stop(rec_chn);

    ret = dmx_deinit();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call dmx_deinit failed.\n");
        return ret;
    }

    hi_unf_sys_deinit();

    return 0;
}


