#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "hi_type.h"

#include "hi_unf_demux.h"
#include "hi_unf_avplay.h"
#include "hi_adp.h"
#include "hi_adp_demux.h"
#include "hi_adp_search.h"

static hi_s32 dmx_data_read(hi_handle h_channel, hi_u32 time_outms, hi_u8 *buf, hi_u32 *acquired_num, hi_u32 *buff_size);

hi_s32 dmx_section_start_data_filter_ext(hi_u32 dmx_id, dmx_data_filter *data_filter, hi_bool tee_enable)
{
    hi_u32 i;
    hi_u8 *p;
    hi_unf_dmx_chan_attr chan_attr = {0};
    hi_handle h_chan, h_filter;
    hi_s32 ret;
    hi_unf_dmx_filter_attr filter_attr = {0};
    hi_u32 aquired_num = 0;
    hi_u8 *data_buf = HI_NULL;
    hi_u32 buf_size[MAX_SECTION_NUM] = {0};
    data_buf = (hi_u8 *)malloc(sizeof(hi_u8) * MAX_SECTION_LEN * MAX_SECTION_NUM);
    if (data_buf == HI_NULL) {
        SAMPLE_COMMON_PRINTF("malloc data_buf failed!\n");
        ret = HI_FAILURE;
        return ret;
    }

    hi_unf_dmx_get_play_chan_default_attr(&chan_attr);

    if (tee_enable == HI_TRUE) {
        chan_attr.buffer_size    = 2 * 1024 * 1024;
        chan_attr.secure_mode  = HI_UNF_DMX_SECURE_MODE_TEE;
    } else {
        chan_attr.buffer_size    = 64 * 1024;
    }

    chan_attr.chan_type = HI_UNF_DMX_CHAN_TYPE_SEC;
    chan_attr.crc_mode     = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
    ret = hi_unf_dmx_create_play_chan(dmx_id, &chan_attr, &h_chan);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_dmx_create_play_chan fail 0x%x", ret);
        goto DMX_EXIT;
    }

    ret = hi_unf_dmx_set_play_chan_pid(h_chan, data_filter->tspid);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_dmx_set_play_chan_pid fail 0x%x", ret);
        goto DMX_DESTROY;
    }

    filter_attr.filter_depth = data_filter->filter_depth;
    memcpy(filter_attr.match, data_filter->match, DMX_FILTER_MAX_DEPTH);
    memcpy(filter_attr.mask, data_filter->mask, DMX_FILTER_MAX_DEPTH);
    memcpy(filter_attr.negate, data_filter->negate, DMX_FILTER_MAX_DEPTH);

    ret = hi_unf_dmx_create_filter(dmx_id, &filter_attr, &h_filter);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_dmx_create_filter fail 0x%x", ret);
        goto DMX_DESTROY;
    }

    ret = hi_unf_dmx_set_filter_attr(h_filter, &filter_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_dmx_set_filter_attr fail 0x%x", ret);
        goto DMX_FILTER_DESTROY;
    }

    ret = hi_unf_dmx_attach_filter(h_filter, h_chan);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_dmx_attach_filter fail 0x%x", ret);
        goto DMX_FILTER_DESTROY;
    }

    ret = hi_unf_dmx_open_play_chan(h_chan);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMMON_PRINTF("call hi_unf_dmx_open_play_chan fail 0x%x", ret);
        goto DMX_FILTER_DETACH;
    }

    ret = dmx_data_read(h_chan, data_filter->time_out, data_buf, &aquired_num, buf_size);
    if (ret == HI_SUCCESS) {
        /* multi-SECTION parse */
        p = data_buf;
        for (i = 0; i < aquired_num; i++) {
            data_filter->fun_section_fun_callback(p, buf_size[i], data_filter->section_struct);
            p = p + MAX_SECTION_LEN;
        }
    }

    HIAPI_RUN(hi_unf_dmx_close_play_chan(h_chan), ret);

DMX_FILTER_DETACH:
    HIAPI_RUN(hi_unf_dmx_detach_filter(h_filter, h_chan), ret);
DMX_FILTER_DESTROY:
    HIAPI_RUN(hi_unf_dmx_destroy_filter(h_filter), ret);
DMX_DESTROY:
    HIAPI_RUN(hi_unf_dmx_destroy_play_chan(h_chan), ret);
DMX_EXIT:
    if (data_buf != HI_NULL) {
        free((void *)data_buf);
        data_buf = HI_NULL;
    }
    return ret;
}
hi_s32 dmx_section_start_data_filter(hi_u32 dmx_id, dmx_data_filter *data_filter)
{
    return dmx_section_start_data_filter_ext(dmx_id, data_filter, HI_FALSE);
}

static hi_s32 dmx_data_read(hi_handle h_channel, hi_u32 time_outms, hi_u8 *buf, hi_u32 *acquired_num, hi_u32 *buff_size)
{
    hi_u8  table_id;
    hi_unf_dmx_data section[32] = {0};
    hi_u32 num, i;
    hi_u32 count = 0 ;
    hi_u32 times = 0;
    hi_u32 sec_total_num = 0;
    hi_u32 sec_num;
    hi_u32 request_num = 32;
    hi_u8 sec_got_flag[MAX_SECTION_NUM] = {0};

    times =  10;

    memset(sec_got_flag, 0, sizeof(sec_got_flag));

    while (--times) {
        num = 0;
        if ((hi_unf_dmx_acquire_buffer(h_channel, request_num, &num, section, time_outms) == HI_SUCCESS)&& (num > 0)) {
            for (i = 0; i < num; i++) {
                if (section[i].data_type == HI_UNF_DMX_DATA_TYPE_WHOLE) {
                    table_id = section[i].data[0];
                    if ( ((EIT_TABLE_ID_SCHEDULE_ACTUAL_LOW <= table_id)
                            && ( table_id <= EIT_TABLE_ID_SCHEDULE_ACTUAL_HIGH))
                            || ((EIT_TABLE_ID_SCHEDULE_OTHER_LOW <= table_id)
                                && ( table_id <= EIT_TABLE_ID_SCHEDULE_OTHER_HIGH))) {
                        sec_num = section[i].data[6] >> 3;
                        sec_total_num = (section[i].data[7] >> 3) + 1;
                    } else if ( TDT_TABLE_ID == table_id || TOT_TABLE_ID == table_id) {
                        sec_num = 0;
                        sec_total_num = 1;
                    } else {
                        sec_num = section[i].data[6];
                        sec_total_num = section[i].data[7] + 1;
                    }

                    if (sec_got_flag[sec_num] == 0) {
                        memcpy((void *)(buf + sec_num * MAX_SECTION_LEN), section[i].data, section[i].size);
                        sec_got_flag[sec_num] = 1;
                        buff_size[  sec_num] = section[i].size;
                        count++;
                    }
                }
            }

            hi_unf_dmx_release_buffer(h_channel, num, section);

            /* to check if all sections are received*/
            if (sec_total_num == count) {
                break;
            }
        } else {
            SAMPLE_COMMON_PRINTF("hi_unf_dmx_acquire_buffer time out\n");
            continue;
        }
    }

    if (times == 0) {
        SAMPLE_COMMON_PRINTF("hi_unf_dmx_acquire_buffer time out\n");
        return HI_FAILURE;
    }

    if (acquired_num != HI_NULL) {
        *acquired_num = sec_total_num;
    }

    return HI_SUCCESS;
}

