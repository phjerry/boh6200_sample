/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description:file process for pvr
 * Author: sdk
 * Create: 2019-08-24
 */

#include <stdlib.h>
#include "hi_adp.h"
#include "hi_adp_pvr.h"

typedef struct pvr_sample_truncate_callback_args_ {
    hi_bool complete_flag;
    hi_s32 error_code;
} trucate_sample_callback_args;

typedef struct pvr_sample_linear_info_ {
    hi_handle chn_id;
    hi_bool complete_flag;
    pthread_t thread_id;
} sample_linear_info;

static sample_linear_info g_linear_info;

/* callback for asynchronous processing */
static hi_s32 pvr_truncate_callback(hi_unf_pvr_file_name src_file, hi_unf_pvr_file_name dst_file,
    hi_s32 err_code, hi_void *user_data, hi_u32 user_data_len)
{
    trucate_sample_callback_args *complete_info = (trucate_sample_callback_args *)user_data;

    complete_info->complete_flag = HI_TRUE;
    complete_info->error_code = err_code;

    return HI_SUCCESS;
}

static hi_void pvr_file_print_info(hi_char *file_name, hi_unf_pvr_file_attr *old_attr)
{
    hi_s32 ret;
    hi_unf_pvr_file_attr attr;

    memset(&attr, 0x00, sizeof(attr));

    ret = hi_unf_pvr_get_file_attr_by_file_name(file_name, &attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("get file(%s) attr failed, ret = 0x%x\n", file_name, ret);
        return;
    }

    if (old_attr == HI_NULL) {
        pvr_sample_printf("file(%s) info-S/E:%u/%ums\n", file_name, attr.start_time_ms, attr.end_time_ms);
    } else {
        pvr_sample_printf("old-S/E:%u/%ums;new-S/E:%u/%ums\n", old_attr->start_time_ms,
            old_attr->end_time_ms, attr.start_time_ms, attr.end_time_ms);
    }

    return;
}

static hi_s32 sample_pvr_filetruncate(hi_char *in_file, hi_char *out_file, hi_u32 head_offset,
    hi_u32 end_offset, hi_bool sync_flag, hi_bool same_file_flag)
{
    hi_u32 i;
    hi_u32 time_out;
    hi_s32 ret;
    hi_char out_check_file[256] = {0};
    hi_unf_pvr_file_attr file_attr;
    trucate_sample_callback_args complete_info;
    hi_unf_pvr_file_trunc_info truncate_info;

    memset(&truncate_info, 0x00, sizeof(truncate_info));
    memset(&complete_info, 0x00, sizeof(complete_info));
    memset(&file_attr, 0x00, sizeof(file_attr));

    ret = hi_unf_pvr_get_file_attr_by_file_name(in_file, &file_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("calling get file attr failed, ret = 0x%x\n", ret);
        return ret;
    }

    if (head_offset + end_offset > file_attr.end_time_ms - file_attr.start_time_ms) {
        pvr_sample_printf("head:%u; end:%u; end_time:%u; start_time:%u\n",
            head_offset, end_offset, file_attr.end_time_ms, file_attr.start_time_ms);
        return HI_FAILURE;
    }

    /* 1. prepare the parameter for truncation */
    truncate_info.is_sync = sync_flag;
    truncate_info.is_trunc_head = (head_offset == 0) ? HI_FALSE : HI_TRUE;
    truncate_info.is_trunc_tail = (end_offset == 0) ? HI_FALSE : HI_TRUE;
    truncate_info.user_data = &complete_info;
    truncate_info.async_callback = pvr_truncate_callback;
    truncate_info.end_time_ms = file_attr.end_time_ms - end_offset;
    truncate_info.padding_time_ms = 0;
    truncate_info.start_time_ms = file_attr.start_time_ms + head_offset;

    pvr_sample_printf("same_file_flag:%u;sync_flag:%u; start_time_ms:%u; end_time_ms:%ums\n",
        same_file_flag, sync_flag, truncate_info.start_time_ms, truncate_info.end_time_ms);

    /* 2. begin to do the processing */
    if (same_file_flag == HI_TRUE) {
        ret = hi_unf_pvr_file_trunc(in_file, strlen(in_file),HI_NULL, 0, &truncate_info);
    } else {
        ret = hi_unf_pvr_file_trunc(in_file, strlen(in_file),out_file, strlen(out_file), &truncate_info);
    }

    if (ret != HI_SUCCESS) {
        pvr_sample_printf("calling hi_unf_pvr_file_trunc failed, ret = 0x%x\n", ret);
        return ret;
    }

    /* 3. verify the output file name */
    if (same_file_flag == HI_TRUE) {
        strncpy(out_check_file, in_file, 256);
    } else {
        strncpy(out_check_file, out_file, 256);
    }

    /* 4. if processing synchronously, return  directly */
    if (sync_flag == HI_TRUE) {
        pvr_file_print_info(out_check_file, &file_attr);
        return HI_SUCCESS;
    }

    /* 5. check and wait for finishing the processing */
    i = 0;
    time_out = (file_attr.end_time_ms - file_attr.start_time_ms) / 1000 + 1;
    while(1) {
        if (i >= time_out * 10) {
            pvr_sample_printf("calling hi_unf_pvr_file_trunc timeout\n");
            return HI_FAILURE;
        }

        if (complete_info.complete_flag == HI_TRUE) {
            pvr_sample_printf("truncate is finished, errno = 0x%x\n", complete_info.error_code);
            pvr_file_print_info(out_check_file, &file_attr);
            return complete_info.error_code;
        }

        sleep(1);
        i++;
    }

    return HI_SUCCESS;
}

static hi_void *linear_stat_thread(hi_void *args)
{
    sample_linear_info *linear_info = (sample_linear_info *)args;
    hi_unf_pvr_linear_status status;

    while (linear_info->complete_flag == HI_FALSE) {
        if (hi_unf_pvr_rec_file_get_linear_status(linear_info->chn_id, &status) == HI_SUCCESS) {
            pvr_sample_printf("current:%08u; total:%08u; percent:%02u%%\n", status.cur_proc_frame_num,
                status.total_frame_num, status.cur_proc_frame_num * 100 / status.total_frame_num);
        }
        sleep(1);
    }

    return HI_NULL;
}

static hi_void linear_callback(hi_handle chn_id, hi_unf_pvr_event event_type, hi_s32 event_value,
    hi_void *user_data, hi_u32 user_data_len)
{
    g_linear_info.complete_flag = HI_TRUE;
    return;
}

static hi_s32 sample_pvr_linearlization(hi_char *in_file_name)
{
    hi_u32 time_out;
    hi_u32 i;
    hi_char out_file_name[256] = {0};
    hi_s32 ret;
    hi_unf_pvr_file_attr file_attr;
    hi_unf_pvr_linear_attr linear_attr;

    memset(&linear_attr, 0x00, sizeof(linear_attr));
    memset(&g_linear_info, 0x00, sizeof(g_linear_info));
    snprintf(out_file_name, 256, "%s_out.ts", in_file_name);

    ret = hi_unf_pvr_get_file_attr_by_file_name(in_file_name, &file_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("calling hi_unf_pvr_get_file_attr_by_file_name failed, ret = 0x%x\n", ret);
        return ret;
    }

    ret = hi_unf_pvr_register_event(HI_UNF_PVR_EVENT_RECFILE_LINEARIZATION_COMPLETE, linear_callback, HI_NULL, 0);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("calling hi_unf_pvr_register_event failed, ret = 0x%x\n", ret);
        return ret;
    }

    /* 1.prepare the parameter */
    strncpy(linear_attr.src_file.name, in_file_name, 256);
    strncpy(linear_attr.dst_file.name, out_file_name, 256);
    linear_attr.dst_file.name_len = strlen(linear_attr.dst_file.name);
    linear_attr.src_file.name_len = strlen(linear_attr.src_file.name);

    /* 2.begin to linearize */
    ret = hi_unf_pvr_rec_file_start_linear(&g_linear_info.chn_id, &linear_attr);
    if (ret != HI_SUCCESS) {
        pvr_sample_printf("calling hi_unf_pvr_rec_file_start_linear failed, ret = 0x%x\n", ret);
        goto UNREG_EVENT;
    }

    /* 3.create one thread to check the progress */
    if (pthread_create(&g_linear_info.thread_id, HI_NULL, linear_stat_thread, &g_linear_info)) {
        pvr_sample_printf("create thread failed\n");
        goto STOP_CHN;
    }

    /* 4.wait for the complition of the process */
    i = 0;
    time_out = (file_attr.end_time_ms - file_attr.start_time_ms) / 1000 + 1;
    while(1) {
        if (i++ > time_out * 10) {
            pvr_sample_printf("time out!\n");
            ret = HI_FAILURE;
            g_linear_info.complete_flag = HI_TRUE;
            break;
        }

        if (g_linear_info.complete_flag == HI_TRUE) {
            pvr_sample_printf("process over!\n");
            ret = HI_SUCCESS;
            break;
        }
        sleep(1);
    }

    pthread_join(g_linear_info.thread_id, HI_NULL);
STOP_CHN:
    hi_unf_pvr_rec_file_stop_linear(g_linear_info.chn_id);
UNREG_EVENT:
    hi_unf_pvr_unregister_event(HI_UNF_PVR_EVENT_RECFILE_LINEARIZATION_COMPLETE);
    return ret;
}

hi_s32 main(int argc, char *argv[])
{
    hi_s32 ret = HI_SUCCESS;
    hi_bool sync_flag = HI_TRUE;
    hi_u32 head_offset = 10 * 1000;
    hi_u32 end_offset = 10 * 1000;
    hi_u32 ctrl_type = 0;
    hi_char out_file_name[256] = {0};
    hi_bool same_file_flag = HI_FALSE;

    if (argc < 2) {
        printf("Usage: %s in_file_name ctrl_type sync_flag head_offset, end_offset same_file_flag\n", argv[0]);
        printf("        in_file_name: original file name\n");
        printf("        ctrl_type: 0 - linearization; 1 - file truncate\n");
        printf("        sync_flag(file truncate): 0 - asynchronous; 1 - synchronous\n");
        printf("        head_offset(file truncate): head offset for file truncate\n");
        printf("        end_offset(file truncate): end offset for file truncate\n");
        printf("        same_file_flag(file truncate): 0 - output another file;1 - output the original file\n");
        return 0;
    }

    if (argc >= 3) {
        ctrl_type = strtol(argv[2], HI_NULL, 0);
    }
    if (argc >= 4) {
        sync_flag = (0 == strtol(argv[3], HI_NULL, 0)) ? HI_FALSE : HI_TRUE;
    }

    if (argc >= 5) {
        head_offset = strtol(argv[4], HI_NULL, 0);
    }

    if (argc >= 6) {
        end_offset = strtol(argv[5], HI_NULL, 0);
    }

    if (argc >= 7) {
        same_file_flag = (0 == strtol(argv[6],HI_NULL,0)) ? HI_FALSE : HI_TRUE;
    }

    snprintf(out_file_name, 256, "%s_out.ts", argv[1]);
    (hi_void)hi_unf_sys_init();
    (hi_void)hi_unf_pvr_rec_init();
    (hi_void)hi_unf_pvr_play_init();
    if (ctrl_type == 0) {
        pvr_sample_printf("do the linearlization for file:%s\n", argv[1]);
        ret = sample_pvr_linearlization(argv[1]);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("calling sample_pvr_linearlization failed, ret = 0x%x\n", ret);
        }
    } else {
        pvr_sample_printf("sync_flag:%d; head:%u; end:%u\n", sync_flag, head_offset, end_offset);
        ret = sample_pvr_filetruncate(argv[1], out_file_name, head_offset, end_offset, sync_flag, same_file_flag);
        if (ret != HI_SUCCESS) {
            pvr_sample_printf("calling sample_pvr_filetruncate failed, ret = 0x%x\n", ret);
        }
    }

    (hi_void)hi_unf_pvr_rec_deinit();
    (hi_void)hi_unf_pvr_play_deinit();
    (hi_void)hi_unf_sys_deinit();

    return 0;
}


