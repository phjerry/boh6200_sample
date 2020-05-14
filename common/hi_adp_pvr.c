/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:
 * Author: sdk
 * Create: 2019-08-14
 */

#define _FILE_OFFSET_BITS    64

#include "hi_adp.h"
#include "hi_adp_pvr.h"
#include "hi_adp_mpi.h"
#include "hi_adp_demux.h"
#ifdef HI_TEE_PVR_TEST_SUPPORT
#include "tee_client_api.h"
#include "tee_client_id.h"
#endif

#define REC_IS_REWIND        1    /* 0 means not rewind recording, 1 means rewind recoding */
#define MAX_REC_FILE_SIZE   (2 * 1024 *1024*1024LLU) /* 2G */

/*************************** Structure Definition ****************************/
typedef struct ts_send_args
{
    hi_u32 dmx_id;
    hi_u32 port_id;
    FILE *ts_file;
} ts_send_args;

typedef struct tagpvr_event_type
{
    hi_u8 event_type_name[128];
    hi_unf_pvr_event event_id;
} pvr_event_type_st;

/********************** Global Variable declaration **************************/
hi_bool g_is_rec_stop = HI_FALSE;
static hi_bool g_stop_ts_thread = HI_FALSE;
hi_handle g_tsbuf_for_playback;
static hi_bool support_tvp = HI_FALSE;
static hi_bool register_extra = HI_FALSE;
static hi_bool register_tee_extra = HI_FALSE;
#ifdef HI_TEE_PVR_TEST_SUPPORT
static hi_bool support_sec_audio = HI_FALSE;
static hi_bool use_sec_ts_buffer = HI_FALSE;
#endif
static pvr_event_type_st g_event_type[] = {
    {"HI_UNF_PVR_EVENT_PLAY_EOF",       HI_UNF_PVR_EVENT_PLAY_EOF},
    {"HI_UNF_PVR_EVENT_PLAY_SOF",       HI_UNF_PVR_EVENT_PLAY_SOF},
    {"HI_UNF_PVR_EVENT_PLAY_ERROR",     HI_UNF_PVR_EVENT_PLAY_ERROR},
    {"HI_UNF_PVR_EVENT_PLAY_REACH_REC", HI_UNF_PVR_EVENT_PLAY_REACH_REC},
    {"HI_UNF_PVR_EVENT_REC_DISKFULL",   HI_UNF_PVR_EVENT_REC_DISKFULL},
    {"HI_UNF_PVR_EVENT_REC_ERROR",      HI_UNF_PVR_EVENT_REC_ERROR},
    {"HI_UNF_PVR_EVENT_REC_OVER_FIX",   HI_UNF_PVR_EVENT_REC_OVER_FIX},
    {"HI_UNF_PVR_EVENT_REC_REACH_PLAY", HI_UNF_PVR_EVENT_REC_REACH_PLAY},
    {"HI_UNF_PVR_EVENT_REC_DISK_SLOW",  HI_UNF_PVR_EVENT_REC_DISK_SLOW},
    {"HI_UNF_PVR_EVENT_MAX",            HI_UNF_PVR_EVENT_MAX}
};

hi_u8 *sample_pvr_get_event_string(hi_unf_pvr_event event_id);
hi_s32 sample_pvr_save_prog(pvr_prog_info *prog_info, hi_char *rec_file);
hi_s32 sample_pvr_get_prog(pvr_prog_info *prog_info, const hi_char *rec_file);
hi_s32 sample_pvr_set_avplay_attr(hi_handle av);

/******************************* API declaration *****************************/

static hi_s32 pvr_extra_callback(hi_unf_pvr_data_attr *data_attr, hi_unf_pvr_buf *dst, hi_unf_pvr_buf *src,
    hi_void *usr_data, hi_u32 usr_data_len)
{
    static hi_u32 count = 0;
    if ((data_attr == HI_NULL) || (dst == HI_NULL) || (src == HI_NULL)) {
        pvr_sample_printf("null pointer(%p, %p, %p)\n", data_attr, dst, src);
        return HI_SUCCESS;
    }
    if (usr_data != HI_NULL) {
        pvr_sample_printf("wrong userdata(usr_data=%p)\n", usr_data);
    }
    if (count % 1000 == 0) {
        pvr_sample_printf("pvr_extra_callback::(addr, offset, u32Count) = (%p, %lld, %u)\n",
            dst->vir_addr, data_attr->global_offset, count);
    }
    count++;
    return HI_SUCCESS;
}
#ifdef HI_TEE_PVR_TEST_SUPPORT
#define TEST_PVR_COPY_TO_REE 0x30000

typedef struct {
    hi_s32 init_cnt;
    TEEC_Context context;
    TEEC_Session session;
} sample_ta_info;

sample_ta_info g_tee_ctrl_info = {.init_cnt = 0};

static TEEC_UUID g_sample_uuid = {0x0E0E0E0E, 0x0E0E, 0x0E0E, {0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E}};

hi_s32 sample_tee_init(hi_void)
{
    TEEC_Result result;
    TEEC_Operation op;
    if (support_tvp == HI_FALSE) {
        return HI_SUCCESS;
    }

    pvr_sample_printf("\tENTER>>\n");

    g_tee_ctrl_info.init_cnt++;
    if (g_tee_ctrl_info.init_cnt == 1) {
        memset(&g_tee_ctrl_info.context, 0x00, sizeof(g_tee_ctrl_info.context));
        memset(&g_tee_ctrl_info.session, 0x00, sizeof(g_tee_ctrl_info.session));

        result = TEEC_InitializeContext(HI_NULL, &g_tee_ctrl_info.context);
        if (result != TEEC_SUCCESS) {
            g_tee_ctrl_info.init_cnt--;
            pvr_sample_printf("TEEC init failed,ret = 0x%08x\n", (hi_u32)result);
            return (hi_s32)result;
        }
        op.started = 1;
        op.cancel_flag = 0;
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT);
        result = TEEC_OpenSession(&g_tee_ctrl_info.context, &g_tee_ctrl_info.session, &g_sample_uuid,
            TEEC_LOGIN_IDENTIFY, HI_NULL, &op, HI_NULL);
        if (result != TEEC_SUCCESS) {
            g_tee_ctrl_info.init_cnt--;
            TEEC_FinalizeContext(&g_tee_ctrl_info.context);
            pvr_sample_printf("TEEC openSession failed,ret = 0x%08x\n", (hi_u32)result);
            return (hi_s32)result;
        }
        pvr_sample_printf("TEEC Init success!\n");
    }
    pvr_sample_printf("\tEXIT>>\n");
    return HI_SUCCESS;
}

hi_s32 sample_tee_deinit(hi_void)
{
    if (support_tvp == HI_FALSE) {
        return HI_SUCCESS;
    }

    pvr_sample_printf("\tENTER>>\n");

    if (g_tee_ctrl_info.init_cnt <= 0) {
        pvr_sample_printf("Not Init the PVR Tee!\n");
        return HI_SUCCESS;
    }

    g_tee_ctrl_info.init_cnt--;

    if (g_tee_ctrl_info.init_cnt == 0) {
        TEEC_CloseSession(&g_tee_ctrl_info.session);
        TEEC_FinalizeContext(&g_tee_ctrl_info.context);
        pvr_sample_printf("TEEC deinit success!\n");
    }
    pvr_sample_printf("\tEXIT>>\n");
    return HI_SUCCESS;
}

hi_s32 pvr_tee_rec_callback(hi_unf_pvr_data_attr *data_attr, hi_unf_pvr_buf *dst_buf,
    hi_unf_pvr_buf *src_buf, hi_u32 offset, hi_u32 data_size, hi_void *user_data)
{
    TEEC_Operation copy_op;
    TEEC_Result result;
    hi_u32 origin;
    static hi_u32 count = 0;

    if ((data_attr == HI_NULL) || (dst_buf == HI_NULL) || (src_buf == HI_NULL)) {
        pvr_sample_printf("null pointer(%p, %p, %p)\n", data_attr, dst_buf, src_buf);
        return HI_SUCCESS;
    }
    if (user_data != HI_NULL) {
        pvr_sample_printf("wrong userdata(%p)\n", user_data);
    }
    if (count % 1000 == 0) { /* invoid too much print in screen, print ingo when call back 1000 times */
        pvr_sample_printf("(GlobleOffset, datasize, dstHandle, srcHandle) = (0x%08llx, 0x%08x, %#llx, %#llx)\n",
            data_attr->global_offset, data_size, dst_buf->buf_handle.MemHandle, src_buf->buf_handle.MemHandle);
    }
    count++;
    if ((data_size != dst_buf->len) || (data_size != src_buf->len)) {
        pvr_sample_printf("(u32DataSize, pstDstBuf->u32Len, pstSrcBuf->u32Len) = (%u, %llu, %llu)\n",
            data_size, dst_buf->len, src_buf->len);
        return HI_SUCCESS;
    }
    memset(&copy_op, 0x00, sizeof(copy_op));
    copy_op.started = 1;
    copy_op.params[1].value.a = dst_buf->buf_handle.mem_handle;
    copy_op.params[1].value.b = 0xFFFF;
    copy_op.params[2].value.a = src_buf->buf_handle.mem_handle;
    copy_op.params[2].value.b = data_size;
    copy_op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_VALUE_INOUT, TEEC_VALUE_INOUT, TEEC_NONE);
    result = TEEC_InvokeCommand(&g_tee_ctrl_info.session, TEST_PVR_COPY_TO_REE, &copy_op, &origin);
    if (result != TEEC_SUCCESS) {
        pvr_sample_printf("Invoke failed, codes=0x%x, origin=0x%x\n", result, origin);
        return (hi_s32)result;
    }
    return HI_SUCCESS;
}

hi_s32 pvr_tee_playback_callback(hi_unf_pvr_data_attr *data_attr, hi_unf_pvr_buf *dst_buf,
    hi_unf_pvr_buf *src_buf, hi_u32 offset, hi_u32 data_size, hi_void *user_data)
{
    TEEC_Operation copy_op;
    TEEC_Result result;
    hi_u32 origin;
    static hi_u32 count = 0;
    hi_u32 total_write_len = 0;
    hi_u32 cur_write_len = data_size;

    if ((data_attr == HI_NULL) || (dst_buf == HI_NULL) || (src_buf == HI_NULL)) {
        pvr_sample_printf("null pointer(%p, %p, %p)\n", data_attr, dst_buf, src_buf);
        return HI_SUCCESS;
    }
    if (user_data != HI_NULL) {
        pvr_sample_printf("wrong userdata(%p)\n", user_data);
    }
    if (count % 1000 == 0) { /* invoid too much print in screen, print ingo when call back 1000 times */
        pvr_sample_printf("(GlobleOffset, datasize, dst_handle, src_handle) = (0x%08llx, 0x%08x, %#llx, %#llx)\n",
            data_attr->global_offset, data_size, dst_buf->buf_handle.MemHandle, src_buf->buf_handle.MemHandle);
    }
    count++;

    if ((data_size != dst_buf->len) || (data_size != src_buf->len)) {
        pvr_sample_printf("(u32DataSize, pstDstBuf->Len, pstSrcBuf->Len) = (%u, %llu, %llu)\n",
            data_size, dst_buf->len, src_buf->len);
        return HI_SUCCESS;
    }
    if (use_sec_ts_buffer != HI_TRUE) {
        return HI_SUCCESS;
    }

    do {
        /* add the limited(846*1024) for max for secure: forbid to copy much data to tee */
        cur_write_len = (cur_write_len > 846 * 1024) ? 846 * 1024 : cur_write_len;
        memset(&copy_op, 0x00, sizeof(copy_op));
        copy_op.started = 1;
        copy_op.params[1].value.a = dst_buf->buf_handle.mem_handle;
        copy_op.params[1].value.b = dst_buf->buf_handle.addr_offset + total_write_len;
        copy_op.params[2].value.a = src_buf->buf_handle.mem_handle;
        copy_op.params[2].value.b = src_buf->buf_handle.addr_offset + total_write_len;
        copy_op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_VALUE_INOUT, TEEC_VALUE_INOUT, TEEC_NONE);
        result = TEEC_InvokeCommand(&g_tee_ctrl_info.session, TEST_PVR_COPY_TO_REE, &copy_op, &origin);
        if (result != TEEC_SUCCESS) {
            pvr_sample_printf("Invoke failed, codes=0x%x, origin=0x%x\n", result, origin);
            return (hi_s32)result;
        }

        total_write_len += cur_write_len;
        cur_write_len = data_size - total_write_len;
        if (total_write_len >= data_size)
            break;
    } while (1);

    return HI_SUCCESS;
}
#endif

hi_void search_file_ts_send_thread(hi_void *args)
{
    hi_u32 read_len;
    hi_handle ts_buf;
    ts_send_args *para = args;
    hi_unf_stream_buf stream_buf;
    hi_unf_dmx_ts_buffer_attr ts_buf_attr;

    if (hi_unf_dmx_attach_ts_port(para->dmx_id, para->port_id) != HI_SUCCESS) {
        pvr_sample_printf("call VoInit failed.\n");
        return;
    }

    ts_buf_attr.secure_mode = 0;
    ts_buf_attr.buffer_size = 0x200000;
    if (hi_unf_dmx_create_ts_buffer(para->port_id, &ts_buf_attr, &ts_buf) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_create_ts_buffer failed.\n");
        return;
    }

    while (!g_stop_ts_thread) {
        if (hi_unf_dmx_get_ts_buffer(ts_buf, 188 * 50, &stream_buf, 0) != HI_SUCCESS) {
            usleep(10 * 1000) ;
            continue;
        }

        read_len = fread(stream_buf.data, sizeof(hi_s8), 188 * 50, para->ts_file);
        if (read_len <= 0) {
            pvr_sample_printf("read ts file error!\n");
            rewind(para->ts_file);
            continue;
        }

        if (hi_adp_dmx_push_ts_buf(ts_buf, &stream_buf, 0, read_len) != HI_SUCCESS) {
            pvr_sample_printf("call HI_UNF_DMX_PutTSBuffer failed.\n");
        }
    }

    if(hi_unf_dmx_destroy_ts_buffer(ts_buf) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_destroy_ts_buffer failed.\n");
    }
    hi_unf_dmx_detach_ts_port(para->dmx_id);

    return;
}

hi_s32 sample_pvr_srch_file(hi_u32 dmx_id, hi_u32 port_id, const hi_char *file_name, pmt_compact_tbl **prog_tb1)
{
    hi_s32 ret;
    pthread_t ts_thd;
    ts_send_args para;
    FILE *ts_file;

    ts_file = fopen(file_name, "rb");

    para.dmx_id = dmx_id;
    para.port_id = port_id;
    para.ts_file = (FILE *)ts_file;

    g_stop_ts_thread = HI_FALSE;
    pthread_create(&ts_thd, HI_NULL, (hi_void *)search_file_ts_send_thread, &para);
    sleep(1);
    hi_adp_search_init();
    ret = hi_adp_search_get_all_pmt(dmx_id, prog_tb1);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_search_get_all_pmt failed.\n");
    }
    hi_adp_search_de_init();

    g_stop_ts_thread = HI_TRUE;
    pthread_join(ts_thd, HI_NULL);
    fclose(ts_file);

    return ret;
}

hi_void sample_pvr_audio_info_rec_trans(pvr_prog_info *file_info, pmt_compact_prog *prog_info)
{
    hi_u32 max;
    hi_u32 i;

    max = PVR_PROG_MAX_AUDIO > PROG_MAX_AUDIO ? PROG_MAX_AUDIO : PVR_PROG_MAX_AUDIO;
    for (i = 0; i < max; i++) {
        file_info->audio_info[i].audio_pid = prog_info->audio_info[i].audio_pid;
        file_info->audio_info[i].audio_enc_type = prog_info->audio_info[i].audio_enc_type;
    }
    return;
}

hi_void sample_pvr_subtiting_info_rec_trans(pvr_prog_info *file_info, pmt_compact_prog *prog_info)
{
    hi_u32 i;
    hi_u32 j;
    hi_u32 max;
    hi_u32 num;

    max = PVR_SUBTITLING_MAX > SUBTITLING_MAX ? SUBTITLING_MAX : PVR_SUBTITLING_MAX;
    for (i = 0; i < max; i++) {
        num = PVR_SUBTDES_INFO_MAX > SUBTDES_INFO_MAX ? SUBTDES_INFO_MAX : PVR_SUBTDES_INFO_MAX;
        file_info->subtiting_info[i].des_tag = prog_info->subtiting_info[i].des_tag;
        file_info->subtiting_info[i].des_length = prog_info->subtiting_info[i].des_length;
        file_info->subtiting_info[i].des_info_cnt = prog_info->subtiting_info[i].des_info_cnt;
        file_info->subtiting_info[i].subtitling_pid = prog_info->subtiting_info[i].subtitling_pid;
        for (j = 0; j < num; j++) {
            file_info->subtiting_info[i].des_info[j].ancillary_page_id =
                prog_info->subtiting_info[i].des_info[j].ancillary_page_id;
            file_info->subtiting_info[i].des_info[j].page_id = prog_info->subtiting_info[i].des_info[j].page_id;
            file_info->subtiting_info[i].des_info[j].lang_code = prog_info->subtiting_info[i].des_info[j].lang_code;
            file_info->subtiting_info[i].des_info[j].subtitle_type =
                prog_info->subtiting_info[i].des_info[j].subtitle_type;
        }
    }
    return;
}

hi_void sample_pvr_closed_caption_rec_trans(pvr_prog_info *file_info, pmt_compact_prog *prog_info)
{
    hi_u32 max;
    hi_u32 i;

    max = PVR_CAPTION_SERVICE_MAX > CAPTION_SERVICE_MAX ? CAPTION_SERVICE_MAX : PVR_CAPTION_SERVICE_MAX;
    for (i = 0; i < max; i++) {
        file_info->closed_caption[i].lang_code = prog_info->closed_caption[i].lang_code;
        file_info->closed_caption[i].is_digital_cc = prog_info->closed_caption[i].is_digital_cc;
        file_info->closed_caption[i].service_number = prog_info->closed_caption[i].service_number;
        file_info->closed_caption[i].is_easy_reader = prog_info->closed_caption[i].is_easy_reader;
        file_info->closed_caption[i].is_wide_aspect_ratio = prog_info->closed_caption[i].is_wide_aspect_ratio;
    }
    return;
}

hi_void sample_pvr_ttx_info_rec_trans(pvr_prog_info *file_info, pmt_compact_prog *prog_info)
{
    hi_u32 i;
    hi_u32 j;
    hi_u32 max;
    hi_u32 num;

    max = PVR_TTX_MAX > TTX_MAX ? TTX_MAX : PVR_TTX_MAX;
    for (i = 0; i < max; i++) {
        num = PVR_TTX_DES_MAX > TTX_DES_MAX ? TTX_DES_MAX : PVR_TTX_DES_MAX;
        file_info->ttx_info[i].ttx_pid = prog_info->ttx_info[i].ttx_pid;
        file_info->ttx_info[i].des_tag = prog_info->ttx_info[i].des_tag;
        file_info->ttx_info[i].des_length = prog_info->ttx_info[i].des_length;
        file_info->ttx_info[i].des_info_cnt = prog_info->ttx_info[i].des_info_cnt;
        for (j = 0; j < num; j++) {
            file_info->ttx_info[i].ttx_des[j].iso639_language_code =
                prog_info->ttx_info[i].ttx_des[j].iso639_language_code;
            file_info->ttx_info[i].ttx_des[j].ttx_type = prog_info->ttx_info[i].ttx_des[j].ttx_type;
            file_info->ttx_info[i].ttx_des[j].ttx_magazine_number =
                prog_info->ttx_info[i].ttx_des[j].ttx_magazine_number;
            file_info->ttx_info[i].ttx_des[j].ttx_page_number = prog_info->ttx_info[i].ttx_des[j].ttx_page_number;
        }
    }
    return;
}

hi_void sample_pvr_prog_rec_trans(pvr_prog_info *file_info, pmt_compact_prog *prog_info)
{
    file_info->prog_id = prog_info->prog_id;
    file_info->pmt_pid = prog_info->pmt_pid;
    file_info->pcr_pid = prog_info->pcr_pid;
    file_info->video_type = prog_info->video_type;
    file_info->v_element_num = prog_info->v_element_num;
    file_info->v_element_pid = prog_info->v_element_pid;
    file_info->audio_type = prog_info->audio_type;
    file_info->a_element_num = prog_info->a_element_num;
    file_info->a_element_pid = prog_info->a_element_pid;
    file_info->subt_type = prog_info->subt_type;
    file_info->subtitling_num = prog_info->subtitling_num;
    file_info->closed_caption_num = prog_info->closed_caption_num;
    file_info->aribcc_pid = prog_info->aribcc_pid;
    file_info->ttx_num = prog_info->ttx_num;
    file_info->scte_subt_info.scte_subt_pid = prog_info->scte_subt_info.scte_subt_pid;
    file_info->scte_subt_info.language_code = prog_info->scte_subt_info.language_code;

    sample_pvr_audio_info_rec_trans(file_info, prog_info);
    sample_pvr_subtiting_info_rec_trans(file_info, prog_info);
    sample_pvr_closed_caption_rec_trans(file_info, prog_info);
    sample_pvr_ttx_info_rec_trans(file_info, prog_info);

    return;
}

hi_void sample_pvr_audio_info_play_trans(pmt_compact_prog *prog_info, pvr_prog_info *file_info)
{
    hi_u32 i;
    hi_u32 max;

    max = PVR_PROG_MAX_AUDIO > PROG_MAX_AUDIO ? PROG_MAX_AUDIO : PVR_PROG_MAX_AUDIO;
    for (i = 0; i < max; i++) {
        prog_info->audio_info[i].audio_pid = file_info->audio_info[i].audio_pid;
        prog_info->audio_info[i].audio_enc_type = file_info->audio_info[i].audio_enc_type;
    }
    return;
}

hi_void sample_pvr_subtiting_info_play_trans(pmt_compact_prog *prog_info, pvr_prog_info *file_info)
{
    hi_u32 i;
    hi_u32 j;
    hi_u32 max;
    hi_u32 num;

    max = PVR_SUBTITLING_MAX > SUBTITLING_MAX ? SUBTITLING_MAX : PVR_SUBTITLING_MAX;
    for (i = 0; i < max; i++) {
        num = PVR_SUBTDES_INFO_MAX > SUBTDES_INFO_MAX ? SUBTDES_INFO_MAX : PVR_SUBTDES_INFO_MAX;
        prog_info->subtiting_info[i].des_tag = file_info->subtiting_info[i].des_tag;
        prog_info->subtiting_info[i].des_length = file_info->subtiting_info[i].des_length;
        prog_info->subtiting_info[i].des_info_cnt = file_info->subtiting_info[i].des_info_cnt;
        prog_info->subtiting_info[i].subtitling_pid = file_info->subtiting_info[i].subtitling_pid;
        for (j = 0; j < num; j++) {
            prog_info->subtiting_info[i].des_info[j].ancillary_page_id =
                file_info->subtiting_info[i].des_info[j].ancillary_page_id;
            prog_info->subtiting_info[i].des_info[j].page_id =
                file_info->subtiting_info[i].des_info[j].ancillary_page_id;
            prog_info->subtiting_info[i].des_info[j].lang_code = file_info->subtiting_info[i].des_info[j].lang_code;
            prog_info->subtiting_info[i].des_info[j].subtitle_type =
                file_info->subtiting_info[i].des_info[j].subtitle_type;
        }
    }
    return;
}

hi_void sample_pvr_closed_caption_play_trans(pmt_compact_prog *prog_info, pvr_prog_info *file_info)
{
    hi_u32 i;
    hi_u32 max;

    max = PVR_CAPTION_SERVICE_MAX > CAPTION_SERVICE_MAX ? CAPTION_SERVICE_MAX : PVR_CAPTION_SERVICE_MAX;
    for (i = 0; i < max; i++) {
        prog_info->closed_caption[i].lang_code = file_info->closed_caption[i].lang_code;
        prog_info->closed_caption[i].is_digital_cc = file_info->closed_caption[i].is_digital_cc;
        prog_info->closed_caption[i].service_number = file_info->closed_caption[i].service_number;
        prog_info->closed_caption[i].is_easy_reader = file_info->closed_caption[i].is_easy_reader;
        prog_info->closed_caption[i].is_wide_aspect_ratio = file_info->closed_caption[i].is_wide_aspect_ratio;
    }
    return;
}

hi_void sample_pvr_ttx_info_play_trans(pmt_compact_prog *prog_info, pvr_prog_info *file_info)
{
        hi_u32 i;
    hi_u32 j;
    hi_u32 max;
    hi_u32 num;

    max = PVR_TTX_MAX > TTX_MAX ? TTX_MAX : PVR_TTX_MAX;
    for (i = 0; i < max; i++) {
        num = PVR_TTX_DES_MAX > TTX_DES_MAX ? TTX_DES_MAX : PVR_TTX_DES_MAX;
        prog_info->ttx_info[i].ttx_pid = file_info->ttx_info[i].ttx_pid;
        prog_info->ttx_info[i].des_tag = file_info->ttx_info[i].des_tag;
        prog_info->ttx_info[i].des_length = file_info->ttx_info[i].des_length;
        prog_info->ttx_info[i].des_info_cnt = file_info->ttx_info[i].des_info_cnt;
        for (j = 0; j < num; j++) {
            prog_info->ttx_info[i].ttx_des[j].iso639_language_code =
                file_info->ttx_info[i].ttx_des[j].iso639_language_code;
            prog_info->ttx_info[i].ttx_des[j].ttx_type = file_info->ttx_info[i].ttx_des[j].ttx_type;
            prog_info->ttx_info[i].ttx_des[j].ttx_magazine_number =
                file_info->ttx_info[i].ttx_des[j].ttx_magazine_number;
            prog_info->ttx_info[i].ttx_des[j].ttx_page_number = file_info->ttx_info[i].ttx_des[j].ttx_page_number;
        }
    }
    return;
}

hi_void sample_pvr_prog_play_trans(pmt_compact_prog *prog_info, pvr_prog_info *file_info)
{
    prog_info->prog_id = file_info->prog_id;
    prog_info->pmt_pid = file_info->pmt_pid;
    prog_info->pcr_pid = file_info->pcr_pid;
    prog_info->video_type = file_info->video_type;
    prog_info->v_element_num = file_info->v_element_num;
    prog_info->v_element_pid = file_info->v_element_pid;
    prog_info->audio_type = file_info->audio_type;
    prog_info->a_element_num = file_info->a_element_num;
    prog_info->a_element_pid = file_info->a_element_pid;
    prog_info->subt_type = file_info->subt_type;
    prog_info->subtitling_num = file_info->subtitling_num;
    prog_info->closed_caption_num = file_info->closed_caption_num;
    prog_info->aribcc_pid = file_info->aribcc_pid;
    prog_info->ttx_num = file_info->ttx_num;
    prog_info->scte_subt_info.scte_subt_pid = file_info->scte_subt_info.scte_subt_pid;
    prog_info->scte_subt_info.language_code = file_info->scte_subt_info.language_code;

    sample_pvr_audio_info_play_trans(prog_info, file_info);
    sample_pvr_subtiting_info_play_trans(prog_info, file_info);
    sample_pvr_closed_caption_play_trans(prog_info, file_info);
    sample_pvr_ttx_info_play_trans(prog_info, file_info);

    return;
}

#ifdef HI_TEE_PVR_TEST_SUPPORT
hi_void sample_pvr_set_sec_buf_flag(hi_bool is_support)
{
    use_sec_ts_buffer = is_support;
    support_sec_audio = is_support;
    return;
}
#endif

hi_void sample_pvr_set_tvp_flag(hi_bool is_support)
{
    support_tvp = is_support;
    return;
}

hi_void sample_pvr_set_cb_flag(hi_bool extra, hi_bool extend_tee)
{
    register_extra = extra;
    register_tee_extra = extend_tee;
    return;
}

hi_s32 sample_pvr_save_prog(pvr_prog_info *prog_info, hi_char *rec_file)
{
    hi_s32 ret;
    pvr_prog_info user_data;

    SAMPLE_CHECK_NULL_PTR(prog_info);

    memcpy(&user_data, prog_info, sizeof(pvr_prog_info));
    user_data.magic_number = PVR_PROG_INFO_MAGIC;

    ret = hi_unf_pvr_set_usr_data_by_file_name(rec_file, (hi_u8*)&user_data, sizeof(pvr_prog_info));
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("PVR_SetUsrDataInfoByFileName ERR:%#x.\n", ret);
        return ret;
    }

    usleep(10 * 1000);
    pvr_sample_printf("\n------------------\n");
    pvr_sample_printf("File Info:\n");
    pvr_sample_printf("Pid:  A=%d/%#x, V=%d/%#x.\n",prog_info->a_element_pid, prog_info->a_element_pid,
        prog_info->v_element_pid, prog_info->v_element_pid);
    pvr_sample_printf("Type: A=%d, V=%d.\n", prog_info->audio_type, prog_info->video_type);
    pvr_sample_printf("indexType: %d, rewind: %d. u64MaxSize: 0x%llx\n",
        prog_info->rec_attr.index_type, prog_info->rec_attr.is_rewind, prog_info->rec_attr.max_size_byte);
    pvr_sample_printf("------------------\n\n");
    usleep(10 * 1000);

    return HI_SUCCESS;
}

hi_s32 sample_pvr_get_prog(pvr_prog_info *prog_info, const hi_char *rec_file)
{
    hi_s32 ret;
    hi_u32 data_read;
    pvr_prog_info user_data;

    SAMPLE_CHECK_NULL_PTR(prog_info);
    SAMPLE_CHECK_NULL_PTR(rec_file);

    ret = hi_unf_pvr_get_usr_data_by_file_name(rec_file, (hi_u8*)&user_data, sizeof(pvr_prog_info), &data_read);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("PVR_SetUsrDataInfoByFileName ERR:%#x.\n", ret);
        return ret;
    }

    if (user_data.magic_number == PVR_PROG_INFO_MAGIC) {
        memcpy(prog_info, &(user_data),  sizeof(pvr_prog_info));
    } else {
        pvr_sample_printf("Can  only play the program record by sample. \n");
        return HI_FAILURE;
    }

    usleep(10 * 1000);
    pvr_sample_printf("\n------------------\n");
    pvr_sample_printf("File Info:\n");
    if (prog_info->a_element_num > 0) {
        pvr_sample_printf("Audio:\n");
        pvr_sample_printf("   PID = %#x\n",prog_info->a_element_pid);
        pvr_sample_printf("   Type= %d\n", prog_info->audio_type);
    } else {
        pvr_sample_printf("Audio: none\n");
    }
    if (prog_info->v_element_num > 0) {
        pvr_sample_printf("Video:\n");
        pvr_sample_printf("   PID = %#x\n",prog_info->v_element_pid);
        pvr_sample_printf("   Type= %d\n", prog_info->video_type);
    } else {
        pvr_sample_printf("Video: none\n\n");
    }

    pvr_sample_printf("indexType: %d, rewind: %d. u64MaxSize: 0x%llx\n",
        prog_info->rec_attr.index_type, prog_info->rec_attr.is_rewind, prog_info->rec_attr.max_size_byte);
    pvr_sample_printf("------------------\n");
    usleep(10 * 1000);

    return HI_SUCCESS;
}

hi_void sample_pvr_get_rec_attr(char *path, pmt_compact_prog *prog_info, hi_unf_pvr_rec_attr *rec_attr,
    hi_u32 *aud_pid, hi_u32 *vid_pid)
{
    hi_char file_name[PVR_MAX_FILENAME_LEN]= {0};

    if (prog_info->a_element_num > 0) {
        *aud_pid = prog_info->a_element_pid;
    }

    if (prog_info->v_element_num > 0) {
        *vid_pid = prog_info->v_element_pid;
        rec_attr->index_pid = *vid_pid;
        rec_attr->index_type = HI_UNF_PVR_REC_INDEX_TYPE_VIDEO;
        rec_attr->index_vid_type = prog_info->video_type;
    } else {
        rec_attr->index_pid = *aud_pid;
        rec_attr->index_type = HI_UNF_PVR_REC_INDEX_TYPE_AUDIO;
        rec_attr->index_vid_type = HI_UNF_VCODEC_TYPE_MAX;
    }

    snprintf(file_name, sizeof(file_name), "rec_v%d_a%d.ts", prog_info->v_element_pid, prog_info->a_element_pid);
    snprintf(rec_attr->file_name.name, sizeof(rec_attr->file_name.name), "%s/", path);
    strncat(rec_attr->file_name.name, file_name, sizeof(file_name) - strlen(rec_attr->file_name.name));

    rec_attr->file_name.name_len = strlen(rec_attr->file_name.name);
    rec_attr->rec_buf_size = PVR_STUB_TSDATA_SIZE * 4;  /* set davBuf size is Recommended value for FHD * 4 */
    rec_attr->stream_type = HI_UNF_PVR_STREAM_TYPE_TS;
    rec_attr->max_time_ms = 0;

    /* the one in index file is a multipleit of 40 bytes
     * CNcomment:索引文件里是40个字节对齐的
     */
    rec_attr->usr_data_size = sizeof(pvr_prog_info) + 100;
    if (support_tvp != HI_FALSE) {
        rec_attr->secure_mode = HI_UNF_PVR_SECURE_MODE_TEE;
    } else {
        rec_attr->secure_mode = HI_UNF_PVR_SECURE_MODE_REE;
    }
    return;
}

hi_void sample_pvr_rec_extend_back(hi_handle ch)
{
    if (register_extra != HI_FALSE) {
        hi_unf_pvr_register_extend_callback(ch, HI_UNF_PVR_RECORD_CALLBACK, pvr_extra_callback, HI_NULL, 0);
    }
    if (register_tee_extra != HI_FALSE) {
    /*
     * if register_extra == HI_TRUE for tee record,
     * app must register a callback using hi_unf_pvr_register_extend_callback.
     * the callback shall encrypt the ts data, and copy the ts data from the tee to the ree buffer,
     * reference the pvr_tee_rec_callback
     */
#ifdef HI_TEE_PVR_TEST_SUPPORT
        hi_unf_pvr_register_extend_callback(ch, HI_UNF_PVR_RECORD_CALLBACK, pvr_tee_rec_callback, HI_NULL, 0);
#endif
    }

    if ((register_extra != HI_FALSE) && (register_tee_extra != HI_FALSE)) {
        pvr_sample_printf("[%s_%s_%d]both callback had been registered\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return;
}

hi_s32 sample_pvr_rec_start(char *path, pmt_compact_prog *prog_info, hi_u32 demux_id, hi_handle *rec_chn)
{
    hi_handle ch;
    hi_s32 ret = HI_SUCCESS;
    hi_u32 vid_pid = 0;
    hi_u32 aud_pid = 0;
    hi_unf_pvr_rec_attr rec_attr = {0};
    pvr_prog_info file_info = {0};
    hi_u64 file_node_size = 64 * 1024 * 1024;
    hi_unf_pvr_rec_stop_opt opt = {0};
    rec_attr.demux_id = demux_id;
    opt.mode = HI_UNF_PVR_REC_STOP_WITH_FLUSH;
    opt.opt = HI_UNF_PVR_STOP_MODE_STILL;
    rec_attr.is_rewind = REC_IS_REWIND;
    rec_attr.max_size_byte = MAX_REC_FILE_SIZE;

    sample_pvr_get_rec_attr(path, prog_info, &rec_attr, &aud_pid, &vid_pid);

    SAMPLE_RUN(hi_unf_pvr_rec_chan_create(&ch, &rec_attr), ret);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    SAMPLE_RUN(hi_unf_pvr_rec_chan_set_attr(ch, HI_UNF_PVR_REC_ATTR_ID_FILE_FRAGMENT_SIZE, &file_node_size), ret);

    sample_pvr_rec_extend_back(ch);

    SAMPLE_RUN(hi_unf_pvr_rec_chan_add_pid(ch, 0), ret);
    SAMPLE_RUN(hi_unf_pvr_rec_chan_add_pid(ch, prog_info->pmt_pid), ret);
    if (prog_info->a_element_num > 0) {
        SAMPLE_RUN(hi_unf_pvr_rec_chan_add_pid(ch, aud_pid), ret);
    }

    if (prog_info->v_element_num > 0) {
        SAMPLE_RUN(hi_unf_pvr_rec_chan_add_pid(ch, vid_pid), ret);
    }

    SAMPLE_RUN(hi_unf_pvr_rec_chan_start(ch), ret);
    if (ret != HI_SUCCESS) {
        hi_unf_pvr_rec_chan_del_all_pid(ch);
        hi_unf_pvr_rec_chan_destroy(ch);
        return ret;
    }
    sample_pvr_prog_rec_trans(&file_info, prog_info);

    memcpy(&(file_info.rec_attr), &rec_attr, sizeof(file_info.rec_attr));

    SAMPLE_RUN(sample_pvr_save_prog(&file_info, rec_attr.file_name.name), ret);
    if (ret != HI_SUCCESS) {
        hi_unf_pvr_rec_chan_stop(ch, &opt);
        hi_unf_pvr_rec_chan_del_all_pid(ch);
        hi_unf_pvr_rec_chan_destroy(ch);
        return ret;
    }

    *rec_chn = ch;
    return HI_SUCCESS;
}

hi_s32 sample_pvr_rec_stop(hi_handle rec_chn)
{
    hi_s32 ret;
    hi_unf_pvr_rec_attr rec_attr;
    hi_unf_pvr_rec_stop_opt opt = {0};
    opt.mode = HI_UNF_PVR_REC_STOP_WITH_FLUSH;

    SAMPLE_RUN(hi_unf_pvr_rec_chan_get_attr(rec_chn, &rec_attr), ret);
    SAMPLE_RUN(hi_unf_pvr_rec_chan_stop(rec_chn, &opt), ret);
    SAMPLE_RUN(hi_unf_pvr_rec_chan_del_all_pid(rec_chn), ret);
    if (support_tvp != HI_FALSE) {
#ifdef HI_TEE_PVR_TEST_SUPPORT
        hi_unf_pvr_unregister_extend_callback(rec_chn, HI_UNF_PVR_RECORD_CALLBACK, pvr_tee_rec_callback);
#endif
    }
    SAMPLE_RUN(hi_unf_pvr_rec_chan_destroy(rec_chn), ret);

    return ret;
}

hi_s32 sample_pvr_switch_dmx_source(hi_u32 dmx_id, hi_u32 port_id)
{
    hi_s32 ret;

    hi_unf_dmx_detach_ts_port(dmx_id);
    SAMPLE_RUN(hi_unf_dmx_attach_ts_port(dmx_id, port_id), ret);

    return ret;
}

hi_s32 sample_pvr_cfg_avplay(hi_handle av, const pmt_compact_prog *prog_info)
{
    hi_u32 vid_pid;
    hi_u32 aud_pid;
    hi_u32 aud_type;
    hi_s32 ret;
    hi_unf_vcodec_type vid_type;
    hi_unf_vdec_attr vdec_attr = {0};
    hi_unf_audio_decode_attr adec_attr = {0};

    hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
    hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_ADEC, &adec_attr);

    if (prog_info->v_element_num > 0) {
        vid_pid = prog_info->v_element_pid;
        vid_type = prog_info->video_type;
    } else {
        vid_pid = INVALID_TSPID;
        vid_type = HI_UNF_VCODEC_TYPE_MAX;
    }

    if (prog_info->a_element_num > 0) {
        aud_pid  = prog_info->a_element_pid;
        aud_type = prog_info->audio_type;
    } else {
        aud_pid = INVALID_TSPID;
        aud_type = 0xffffffff;
    }

    if (vid_pid != INVALID_TSPID) {
        vdec_attr.type = vid_type;
        vdec_attr.error_cover = 100; /* vdec err_cover set 100 */
        ret = hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_VDEC, &vdec_attr);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("call hi_unf_avplay_set_attr failed.\n");
            return ret;
        }

        ret = hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_VID_PID, &vid_pid);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("call hi_unf_avplay_set_attr failed.\n");
            return ret;
        }
    }

    if (aud_pid != INVALID_TSPID) {
        ret = hi_adp_avplay_set_adec_attr(av, aud_type);
        ret |= hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &aud_pid);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("hi_adp_avplay_set_adec_attr failed:%#x\n", ret);
            return ret;
        }
    }

    return HI_SUCCESS;
}

hi_s32 sample_pvr_set_sync_attr(hi_handle av, hi_unf_avplay_media_chan media_type)
{
    hi_unf_avplay_frame_rate_param frm_rate_attr = {0};
    hi_unf_sync_attr sync_attr = {0};

    if ((media_type & HI_UNF_AVPLAY_MEDIA_CHAN_AUD) && (media_type & HI_UNF_AVPLAY_MEDIA_CHAN_VID)) {
        /* enable vo frame rate detect
         * CNcomment:使能VO自动帧率检测
         */
        frm_rate_attr.type = HI_UNF_AVPLAY_FRMRATE_TYPE_PTS;
        frm_rate_attr.frame_rate = 0;
        if (hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_FRAMERATE, &frm_rate_attr) != HI_SUCCESS) {
            pvr_sample_printf("set frame to VO fail.\n");
            return HI_FAILURE;
        }

        /* enable avplay A/V sync
         * CNcomment:使能avplay音视频同步
         */
        if (hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr) != HI_SUCCESS) {
            pvr_sample_printf("get avplay sync stRecAttr fail!\n");
            return HI_FAILURE;
        }

        sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
        sync_attr.start_region.vid_plus_time = 60;
        sync_attr.start_region.vid_negative_time = -20;
        sync_attr.pre_sync_timeout = 1000;
        sync_attr.quick_output_enable = HI_FALSE;

        if (hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr) != HI_SUCCESS) {
            pvr_sample_printf("set avplay sync stRecAttr fail!\n");
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

/* set audio and video PID attribution,start to play
 * CNcomment:设置音视频PID属性,开始播放
 */
hi_s32 sample_pvr_start_live(hi_handle av, const pmt_compact_prog *prog_info)
{
    hi_u32 ret;
    hi_u32 pid = 0;
    hi_unf_avplay_media_chan media_type = 0;
    hi_unf_avplay_codec_cmd_param vdec_cmd_para = {0};
    hi_unf_avplay_tplay_opt tplay_opts = {0};

    if (sample_pvr_cfg_avplay(av, prog_info) != HI_SUCCESS) {
        pvr_sample_printf("pvr set avplay pid and codec type failed!\n");
    }

    ret = hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &pid);
    if ((ret != HI_SUCCESS) || (pid == 0x1fff)) {
        pvr_sample_printf("has no audio stream!\n");
    } else {
        media_type |= HI_UNF_AVPLAY_MEDIA_CHAN_AUD;
    }

    ret = hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_VID_PID, &pid);
    if ((ret != HI_SUCCESS) || (pid == 0x1fff)) {
        pvr_sample_printf("has no video stream!\n");
    } else {
        media_type |= HI_UNF_AVPLAY_MEDIA_CHAN_VID;
    }

    ret = sample_pvr_set_sync_attr(av, media_type);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    /* start to play audio and video */ /* CNcomment:开始音视频播放 */
    SAMPLE_RUN(hi_unf_avplay_start(av, media_type, HI_NULL), ret);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    /* set avplay trick mode to normal */ /* CNcomment:设置avplay特技模式为正常 */
    if (media_type & HI_UNF_AVPLAY_MEDIA_CHAN_VID) {
        tplay_opts.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
        tplay_opts.speed_integer = 1;
        tplay_opts.speed_decimal = 0;
        vdec_cmd_para.cmd_id = HI_UNF_AVPLAY_SET_TPLAY_PARA_CMD;
        vdec_cmd_para.param = &tplay_opts;

        ret = hi_unf_avplay_invoke(av, HI_UNF_AVPLAY_INVOKE_VCODEC, (void *)&vdec_cmd_para);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("Resume Avplay trick mode to normal fail.\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

hi_s32 sample_pvr_stop_live(hi_handle av)
{
    hi_unf_avplay_stop_opt stop_option;

    stop_option.mode = HI_UNF_AVPLAY_STOP_MODE_STILL;
    stop_option.timeout = 0;

    pvr_sample_printf("stop live play ...\n");

    /* stop playing audio and video */ /* CNcomment:停止音视频设备 */
    return hi_unf_avplay_stop(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_option);
}

hi_void sample_pvr_get_play_attr(hi_handle av, const hi_char *file_name, hi_unf_pvr_play_attr *play_attr)
{
    hi_s32 ret;
    pvr_prog_info file_info = {0};
    pmt_compact_prog prog_info_tmp = {0};
    pmt_compact_tbl *pmt_tbl = HI_NULL;

    SAMPLE_RUN(sample_pvr_get_prog(&file_info, file_name), ret);
    if (ret != HI_SUCCESS) {
        PRINT_SMP("Can NOT get prog INFO, can't play.\n");
        sample_pvr_srch_file(PVR_DMX_ID_LIVE, PVR_DMX_PORT_ID_PLAYBACK, file_name, &pmt_tbl);
        sample_pvr_cfg_avplay(av, &(pmt_tbl->proginfo[0]));

        memcpy(play_attr->play_file.name, file_name, strlen(file_name) + 1);
        play_attr->play_file.name_len = strlen(file_name);
        play_attr->stream_type = HI_UNF_PVR_STREAM_TYPE_TS;
    } else {
        sample_pvr_prog_play_trans(&prog_info_tmp, &file_info);
        sample_pvr_cfg_avplay(av, &(prog_info_tmp));
        memcpy(play_attr->play_file.name, file_name, strlen(file_name) + 1);
        play_attr->play_file.name_len = strlen(file_name);
        play_attr->stream_type = file_info.rec_attr.stream_type;
    }

    if (support_tvp != HI_FALSE) {
        play_attr->secure_mode = HI_UNF_PVR_SECURE_MODE_TEE;
    } else {
        play_attr->secure_mode = HI_UNF_PVR_SECURE_MODE_REE;
    }
    return;
}

hi_void sample_pvr_play_extend_callback(hi_handle ch_id)
{
    if (register_extra != HI_FALSE) {
        hi_unf_pvr_register_extend_callback(ch_id, HI_UNF_PVR_PLAY_CALLBACK, pvr_extra_callback, HI_NULL, 0);
    }
#ifdef HI_TEE_PVR_TEST_SUPPORT
    /*
     * if register_extra == HI_TRUE for tee play, app must register a callback using hi_unf_pvr_register_extend_callback.
     * the callback shall config the demux with suitable key, not use the sample callback PvrTeeExtendCallback
     */
    if (register_tee_extra != HI_FALSE) {
        hi_unf_pvr_register_extend_callback(ch_id, HI_UNF_PVR_PLAY_CALLBACK, pvr_tee_playback_callback, HI_NULL, 0);
    }
#endif
    if ((register_extra != HI_FALSE) && (register_tee_extra != HI_FALSE)) {
        pvr_sample_printf("[%s_%s_%d]Both callback has been registered\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return;
}

hi_s32 sample_pvr_enable_avplay_sync(hi_handle av, hi_unf_avplay_media_chan *media_type)
{
    hi_s32 ret;
    hi_u32 pid = 0;
    hi_unf_sync_attr sync_attr = {0};

    ret = hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_AUD_PID, &pid);
    if ((ret != HI_SUCCESS) || (pid == 0x1fff)) {
        pvr_sample_printf("has no audio stream!\n");
    } else {
        *media_type |= HI_UNF_AVPLAY_MEDIA_CHAN_AUD;
    }

    ret = hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_VID_PID, &pid);
    if ((ret != HI_SUCCESS) || (pid == 0x1fff)) {
        pvr_sample_printf("has no video stream!\n");
    } else {
        *media_type |= HI_UNF_AVPLAY_MEDIA_CHAN_VID;
    }

    /*enable avplay A/V sync*//*CNcomment:使能avplay音视频同步*/
    if (hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr) != HI_SUCCESS) {
        pvr_sample_printf("get avplay sync stRecAttr fail!\n");
        return HI_FAILURE;
    }

    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    sync_attr.start_region.vid_plus_time = 60;
    sync_attr.start_region.vid_negative_time = -20;
    sync_attr.pre_sync_timeout = 1000;
    sync_attr.quick_output_enable = HI_TRUE;

    if (hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr) != HI_SUCCESS) {
        pvr_sample_printf("set avplay sync stRecAttr fail!\n");
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

hi_s32 sample_pvr_set_frm_rate_and_invoke(hi_handle av, hi_unf_avplay_media_chan media_type)
{
    hi_s32 ret;
    hi_unf_avplay_frame_rate_param frm_rate_attr = {0};
    hi_unf_avplay_tplay_opt tplay_opt = {0};
    hi_unf_avplay_codec_cmd_param vdec_cmd_para = {0};

    /* enable vo frame rate detect */ /* CNcomment:使能VO自动帧率检测 */
    frm_rate_attr.type = HI_UNF_AVPLAY_FRMRATE_TYPE_PTS;
    frm_rate_attr.frame_rate = 0;
    if (hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_FRAMERATE, &frm_rate_attr) != HI_SUCCESS) {
        pvr_sample_printf("set frame to VO fail.\n");
        return HI_FAILURE;
    }

    /* set avplay trick mode to normal */ /* CNcomment:设置avplay特技模式为正常 */
    if (media_type & HI_UNF_AVPLAY_MEDIA_CHAN_VID) {
        tplay_opt.direct = HI_UNF_AVPLAY_TPLAY_DIRECT_FORWARD;
        tplay_opt.speed_integer = 1;
        tplay_opt.speed_decimal = 0;
        vdec_cmd_para.cmd_id = HI_UNF_AVPLAY_SET_TPLAY_PARA_CMD;
        vdec_cmd_para.param = &tplay_opt;
        ret = hi_unf_avplay_invoke(av, HI_UNF_AVPLAY_INVOKE_VCODEC, (void *)&vdec_cmd_para);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("Resume Avplay trick mode to normal fail.\n");
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

/* start playback */ /* CNcomment:开始回放 */
hi_s32 sample_pvr_start_playback(const hi_char *file_name, hi_handle *play_chn, hi_handle av)
{
    hi_s32 ret;
    hi_handle ch_id;
    hi_unf_pvr_play_attr play_attr = {0};
    hi_unf_avplay_media_chan media_type = 0;
    hi_unf_dmx_ts_buffer_attr ts_buf_attr = {0};

    ret = hi_unf_dmx_attach_ts_port(PVR_DMX_ID_LIVE, PVR_DMX_PORT_ID_PLAYBACK);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_dmx_attach_ts_port failed.\n");
        hi_unf_dmx_deinit();
        return ret;
    }

    sample_pvr_get_play_attr(av, file_name, &play_attr);
    ts_buf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ts_buf_attr.buffer_size = 4 * 1024 * 1024;   /* DMX ts_buf size is 4 * 1024 * 1024 */

#ifdef HI_TEE_PVR_TEST_SUPPORT
    if (use_sec_ts_buffer != HI_FALSE) {
        ts_buf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_TEE;
    }
#endif

    SAMPLE_RUN(hi_unf_dmx_create_ts_buffer(PVR_DMX_PORT_ID_PLAYBACK, &ts_buf_attr, &g_tsbuf_for_playback), ret);
    if (ret != HI_SUCCESS) {
        hi_unf_dmx_detach_ts_port(PVR_DMX_ID_LIVE);
        hi_unf_dmx_deinit();
        return ret;
    }
    /*create new play channel*//*CNcomment:申请新的播放通道*/
    SAMPLE_RUN(hi_unf_pvr_play_chan_create(&ch_id, &play_attr, av, g_tsbuf_for_playback), ret);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    sample_pvr_play_extend_callback(ch_id);

    ret = sample_pvr_enable_avplay_sync(av, &media_type);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    SAMPLE_RUN(hi_unf_pvr_play_chan_start(ch_id), ret);
    if (ret != HI_SUCCESS) {
        hi_unf_pvr_play_chan_destroy(ch_id);
        return ret;
    }

    ret = sample_pvr_set_frm_rate_and_invoke(av, media_type);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    *play_chn = ch_id;
    return HI_SUCCESS;
}

/* stop playback
 * CNcomment: 停止回放
 */
hi_void sample_pvr_stop_playback(hi_handle play_chn)
{
    hi_unf_pvr_rec_stop_opt stop_opt;
    hi_unf_pvr_play_attr play_attr;

    stop_opt.mode = HI_UNF_PVR_REC_STOP_WITH_FLUSH;
    stop_opt.opt = HI_UNF_PVR_STOP_MODE_STILL;
    (hi_void)hi_unf_pvr_play_chan_get_attr(play_chn, &play_attr);
    (hi_void)hi_unf_pvr_play_chan_stop(play_chn, &stop_opt);
    (hi_void)hi_unf_pvr_play_chan_destroy(play_chn);
    (hi_void)hi_unf_dmx_destroy_ts_buffer(g_tsbuf_for_playback);
}

hi_s32 sample_pvr_set_avplay_attr(hi_handle av)
{
    hi_s32 ret;
    hi_unf_sync_attr sync_attr = {0};

    ret = hi_unf_avplay_get_attr(av, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_get_attr failed.\n");
        hi_unf_avplay_deinit();
        return ret;
    }

    sync_attr.ref_mode = HI_UNF_SYNC_REF_MODE_AUDIO;
    sync_attr.start_region.vid_plus_time = 60;
    sync_attr.start_region.vid_negative_time = -20;
    sync_attr.pre_sync_timeout = 1000;
    sync_attr.quick_output_enable = HI_FALSE;

    ret = hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_SYNC, &sync_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_set_attr failed.\n");
        hi_unf_avplay_deinit();
        return ret;
    }
    if (support_tvp != HI_FALSE) {
        hi_unf_avplay_smp_attr vid_smp_attr;
        vid_smp_attr.chan = HI_UNF_AVPLAY_MEDIA_CHAN_VID;
        vid_smp_attr.enable = HI_TRUE;
        ret = hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_SMP, &vid_smp_attr);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("[%s_%d]call hi_unf_avplay_set_attr failed.ret = 0x%08x\n", __FILE__, __LINE__, ret);
            hi_unf_avplay_deinit();
            return ret;
        }
    }
#ifdef HI_TEE_PVR_TEST_SUPPORT
    if (support_sec_audio != HI_FALSE) {
        hi_unf_avplay_smp_attr aud_smp_attr;
        aud_smp_attr.chan = HI_UNF_AVPLAY_MEDIA_CHAN_AUD;
        aud_smp_attr.enable = HI_TRUE;
        ret = hi_unf_avplay_set_attr(av, HI_UNF_AVPLAY_ATTR_ID_SMP, &aud_smp_attr);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("[%s_%d]call hi_unf_avplay_set_attr failed.ret = 0x%08x\n", __FILE__, __LINE__, ret);
            hi_unf_avplay_deinit();
            return ret;
        }
    }
#endif
    return HI_SUCCESS;
}

hi_s32 sample_pvr_open_avplay_chan(hi_handle av)
{
    hi_s32 ret;

    ret = hi_unf_avplay_chan_open(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_chan_open failed.\n");
        hi_unf_avplay_destroy(av);
        hi_unf_avplay_deinit();
        return ret;
    }

    ret = hi_unf_avplay_chan_open(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_chan_open failed.\n");
        hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
        hi_unf_avplay_destroy(av);
        return ret;
    }
    return HI_SUCCESS;
}

hi_s32 sample_pvr_attach_and_enable_win(hi_handle win, hi_handle av)
{
    hi_s32 ret;

    ret = hi_unf_win_attach_src(win, av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_win_attach_src failed.\n");
        goto close_chan;
    }

    ret = hi_unf_win_set_enable(win, HI_TRUE);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_win_set_enable failed.\n");
        goto detach_src;
    }
    return HI_SUCCESS;

detach_src:
    hi_unf_win_detach_src(win, av);
close_chan:
    hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    hi_unf_avplay_destroy(av);
    hi_unf_avplay_deinit();
    return ret;
}

hi_s32 sample_pvr_get_attr_and_attach_snd(hi_handle win, hi_handle av, hi_handle *sound_track)
{
    hi_s32 ret;
    hi_unf_audio_track_attr track_attr = {0};
    ret = hi_unf_snd_get_default_track_attr(HI_UNF_SND_TRACK_TYPE_MASTER, &track_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_snd_get_default_track_attr failed.\n");
        return ret;
    }
    ret = hi_unf_snd_create_track(HI_UNF_SND_0, &track_attr, sound_track);
    if (ret != HI_SUCCESS) {
        goto disable_win;
    }

    ret = hi_unf_snd_attach(*sound_track, av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call HI_SND_Attach failed.\n");
        goto err;
    }
    return HI_SUCCESS;

err:
    hi_unf_snd_destroy_track(*sound_track);
disable_win:
    hi_unf_win_set_enable(win, HI_FALSE);
    hi_unf_win_detach_src(win, av);
    hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
    hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
    hi_unf_avplay_destroy(av);
    hi_unf_avplay_deinit();
    return ret;
}

hi_s32 sample_pvr_av_init(hi_handle win, hi_handle *avplay, hi_handle *sound_track)
{
    hi_s32 ret;
    hi_handle av;
    hi_unf_avplay_attr av_attr = {0};

    if (sound_track == HI_NULL || avplay == HI_NULL) {
        return HI_FAILURE;
    }

    ret = hi_adp_avplay_init();
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_adp_avplay_init failed.\n");
        return ret;
    }

    ret = hi_unf_avplay_get_default_config(&av_attr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_get_default_config failed.\n");
        hi_unf_avplay_deinit();
        return ret;
    }

    av_attr.demux_id = PVR_DMX_ID_LIVE;
    av_attr.vid_buf_size = 2 * 1024 * 1024;
    av_attr.aud_buf_size = 192 * 1024;

    ret = hi_unf_avplay_create(&av_attr, &av);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_create failed.\n");
        hi_unf_avplay_deinit();
        return ret;
    }

    ret = sample_pvr_set_avplay_attr(av);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = sample_pvr_open_avplay_chan(av);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = sample_pvr_attach_and_enable_win(win, av);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    ret = sample_pvr_get_attr_and_attach_snd(win, av, sound_track);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    *avplay = av;
    return HI_SUCCESS;
}

hi_s32 sample_pvr_av_deinit(hi_handle av, hi_handle win, hi_handle sound_track)
{
    hi_unf_avplay_stop_opt stop_opt;

    stop_opt.mode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stop_opt.timeout = 0;

    if(hi_unf_avplay_stop(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &stop_opt) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_stop failed.\n");
    }

    hi_unf_win_set_enable(win,HI_FALSE);

    if(hi_unf_snd_detach(sound_track, av) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_snd_detach failed.\n");
    }

    if(hi_unf_snd_destroy_track(sound_track) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_snd_destroy_track failed.\n");
    }

    if(hi_unf_win_detach_src(win, av) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_win_detach_src failed.\n");
    }

    if(hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_VID) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_chan_close failed.\n");
    }

    if(hi_unf_avplay_chan_close(av, HI_UNF_AVPLAY_MEDIA_CHAN_AUD) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_chan_close failed.\n");
    }

    if(hi_unf_avplay_destroy(av) != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_destroy failed.\n");
    }

    if(hi_unf_avplay_deinit() != HI_SUCCESS) {
        pvr_sample_printf("call hi_unf_avplay_deinit failed.\n");
    }

    return HI_SUCCESS;
}

hi_bool sample_pvr_check_rec_stop(hi_void)
{
    return g_is_rec_stop;
}

hi_void sample_pvr_inform_rec_stop(hi_bool is_stop)
{
    g_is_rec_stop = is_stop;
}

hi_void sample_pvr_cb(hi_handle chn_id, hi_unf_pvr_event event_type, hi_s32 event_value,
    hi_void *usr_data, hi_u32 usr_data_len)
{

    pvr_sample_printf("==============call back================\n");

    if (event_type > HI_UNF_PVR_EVENT_MAX) {
        pvr_sample_printf("====callback error!!!\n");
        return;
    }

    pvr_sample_printf("====channel     %d\n", chn_id);
    pvr_sample_printf("====event:%s    %d\n", sample_pvr_get_event_string(event_type), event_type);
    pvr_sample_printf("====event value %d\n", event_value);

    if (event_type == HI_UNF_PVR_EVENT_PLAY_EOF) {
        pvr_sample_printf("==========play to end of file======\n");
    }
    if (event_type == HI_UNF_PVR_EVENT_PLAY_SOF) {
        pvr_sample_printf("==========play to start of file======\n");
    }
    if (event_type == HI_UNF_PVR_EVENT_PLAY_ERROR) {
        pvr_sample_printf("==========play internal error, check if the disk is insert to the box======\n");
    }
    if (event_type == HI_UNF_PVR_EVENT_PLAY_REACH_REC) {
        pvr_sample_printf("==========play reach to record ======\n");
    }

    if (event_type == HI_UNF_PVR_EVENT_REC_DISKFULL) {
        pvr_sample_printf("\n====disk full,  stop record=====\n\n");
        hi_unf_pvr_rec_stop_opt opt = {0};
        opt.mode = HI_UNF_PVR_REC_STOP_WITH_FLUSH;
        hi_unf_pvr_rec_chan_stop(chn_id, &opt);
        sample_pvr_inform_rec_stop(HI_TRUE);
    }
    if (event_type == HI_UNF_PVR_EVENT_REC_ERROR) {
        pvr_sample_printf("======disk write error, please check if the disk is insert to the box.====\n");
    }
    if (event_type == HI_UNF_PVR_EVENT_REC_OVER_FIX) {
        pvr_sample_printf("\n======reach the fixed size.==========\n\n");
    }
    if (event_type == HI_UNF_PVR_EVENT_REC_REACH_PLAY) {
        pvr_sample_printf("\n======record reach to play.==========\n\n");
    }
    if (event_type == HI_UNF_PVR_EVENT_REC_DISK_SLOW) {
        pvr_sample_printf("======disk is too slow, the stream record would be error.====\n");
    }

    pvr_sample_printf("=======================================\n\n");

    return;
}

hi_s32 sample_pvr_register_cb(hi_void)
{
    hi_s32 ret;

    ret = hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_PLAY_EOF, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_PLAY_SOF, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_PLAY_ERROR, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_PLAY_REACH_REC, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_REC_DISKFULL, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_REC_OVER_FIX, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_REC_DISK_SLOW, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_REC_REACH_PLAY, sample_pvr_cb, HI_NULL, 0);
    ret |= hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_REC_ERROR, sample_pvr_cb, HI_NULL, 0);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("call hi_mpi_pvr_register_event failed.\n");
        return ret;
    }

    return HI_SUCCESS;
}

hi_u8 *sample_pvr_get_event_string(hi_unf_pvr_event event_id)
{
    hi_u32 event_num = sizeof(g_event_type)/sizeof(g_event_type[0]);
    hi_u32 i;
    hi_u8 *ret = HI_NULL;

    for (i = 0; i < event_num; i++) {
        if (event_id == g_event_type[i].event_id) {
            ret = g_event_type[i].event_type_name;
            break;
        }
    }

    if (i == event_num) {
        ret = g_event_type[event_num - 1].event_type_name;
    }

    return ret;
}

typedef struct tagnet_stream_param
{
    hi_u16 port;
    hi_char igmp_addr[16];
    pthread_t net_stream_thread;
    hi_bool is_stop_net_stream_thread;
} net_stream_param;

net_stream_param g_net_thread_param = {0};

hi_void *NetStream(hi_void *args)
{
    hi_s32 socket_fd = -1;
    struct sockaddr_in server_addr;
    in_addr_t ip_addr;
    struct ip_mreq mreq;
    hi_u32 addr_len;
    hi_u32 read_len;
    hi_u32 get_buf_cnt = 0;
    hi_u32 receive_cnt = 0;
    hi_s32 ret;
    hi_handle ts_buf = 0;
    hi_unf_stream_buf stream_buf;
    hi_unf_dmx_ts_buffer_attr ts_buf_attr;
    net_stream_param *thread_param = (net_stream_param *)args;

    if (thread_param == HI_NULL) {
        return HI_NULL;
    }

    ts_buf_attr.secure_mode = HI_UNF_DMX_SECURE_MODE_REE;
    ts_buf_attr.buffer_size = 0x200000;
    do {
        if(hi_unf_dmx_create_ts_buffer(PVR_DMX_PORT_ID_IP, &ts_buf_attr, &ts_buf) != HI_SUCCESS) {
            break;
        }

        socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd < 0) {
            pvr_sample_printf("wait send TS to port %d.\n", PVR_DMX_PORT_ID_IP);
            break;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(thread_param->port);

        if (bind(socket_fd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr_in)) < 0) {
            pvr_sample_printf("socket bind error [%d].\n", errno);
            break;;
        }

        ip_addr = inet_addr(thread_param->igmp_addr);

        pvr_sample_printf("g_pszMultiAddr = %s, UdpPort=%u\n", thread_param->igmp_addr, thread_param->port);
        if (ip_addr) {
            mreq.imr_multiaddr.s_addr = ip_addr;
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq))) {
                pvr_sample_printf("wait send TS to port %d.\n", PVR_DMX_PORT_ID_IP);
                break;
            }
        }

        addr_len = sizeof(server_addr);

        while (thread_param->is_stop_net_stream_thread == HI_FALSE) {
            ret = hi_unf_dmx_get_ts_buffer(ts_buf, 188 * 50, &stream_buf, 0);
            if (ret != HI_SUCCESS) {
                get_buf_cnt++;
                if(get_buf_cnt >= 10) {
                    pvr_sample_printf("########## TS come too fast! #########, ret=%d\n", ret);
                    get_buf_cnt = 0;
                }

                usleep(10000) ;
                continue;
            }
            get_buf_cnt = 0;

            read_len = recvfrom(socket_fd, stream_buf.data, stream_buf.size, 0,
                (struct sockaddr *)&server_addr, &addr_len);
            if (read_len <= 0) {
                receive_cnt++;
                if (receive_cnt >= 50) {
                    pvr_sample_printf("########## TS come too slow or net error! #########\n");
                    receive_cnt = 0;
                }
            } else {
                receive_cnt = 0;
                if (hi_adp_dmx_push_ts_buf(ts_buf, &stream_buf, 0, read_len) != HI_SUCCESS) {
                    pvr_sample_printf("call hi_unf_dmx_put_ts_buffer failed.\n");
                }
            }
        }
    } while(0);

    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
    }

    if (ts_buf != 0) {
        hi_unf_dmx_destroy_ts_buffer(ts_buf);
        ts_buf = 0;
    }

    return HI_NULL;
}

hi_u32 pvr_start_net_stream(hi_char *igmp_addr, hi_u16 port)
{
    if (igmp_addr == HI_NULL) {
        return HI_FAILURE;
    }

    if (g_net_thread_param.net_stream_thread == 0) {
        g_net_thread_param.is_stop_net_stream_thread = HI_FALSE;
        memset(g_net_thread_param.igmp_addr, 0, sizeof(g_net_thread_param.igmp_addr));
        memcpy(g_net_thread_param.igmp_addr, igmp_addr, sizeof(g_net_thread_param.igmp_addr));

        g_net_thread_param.port = port;
        pthread_create(&g_net_thread_param.net_stream_thread, HI_NULL, NetStream, &g_net_thread_param);
    }

    return HI_SUCCESS;
}

hi_u32 pvr_stop_net_stream()
{
    if (g_net_thread_param.net_stream_thread != 0) {
        g_net_thread_param.is_stop_net_stream_thread = HI_TRUE;

        pthread_join(g_net_thread_param.net_stream_thread, HI_NULL);
    }

    return HI_FAILURE;
}

