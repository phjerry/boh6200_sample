#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "hi_mpi_vdec.h"
#include "hi_type.h"
#include "hi_unf_system.h"

#define CRC_DIR "/crc/TestStream/"
#define VDEC_TEST_READ_ONCE 4*1024

#define POS() \
    do { \
        printf("%s %d\n", __func__, __LINE__); \
    } while(0)

#define VDEC_TEST_ASSERT_RET(cond, ret) \
    do { \
        if (!cond) { \
            printf("%s %d %s is not ok.!\n", __func__, __LINE__, #cond); \
            return ret; \
        } \
    } while(0)

typedef struct {
    char file_name[200];
    hi_vdec_std std;
} sample_vdec_test_param;

static hi_s32 recieve_frame_try = 0;
static hi_s32 recieve_frame_suc = 0;
static hi_s32 recieve_frame_fail = 0;

static hi_s32 release_frame_try = 0;
static hi_s32 release_frame_suc = 0;
static hi_s32 release_frame_fail = 0;

static hi_bool g_stop = HI_TRUE;
static hi_handle g_handle;
static hi_void vdec_test_print_recieve(hi_void)
{
    printf("recieve frame [try:%d suc:%d fail:%d]\n", recieve_frame_try, recieve_frame_suc, recieve_frame_fail);
    return;
}

static hi_void vdec_test_print_release(hi_void)
{
    printf("relese frame [try:%d suc:%d fail:%d]\n", release_frame_try, release_frame_suc, release_frame_fail);
    return;
}

#define UINT64_PTR(ptr) ((hi_void *)(hi_ulong)(ptr))
#define PTR_UINT64(ptr) ((hi_u64)(hi_ulong)(ptr))


hi_void* vdec_test_stream_thread(hi_void *args)
{
    hi_s32 ret = HI_FAILURE;
    FILE *fp = HI_NULL;
    hi_vdec_stream stream;
    hi_vdec_opt_deq_stm option;

    fp = fopen((hi_char*)args, "rb");
    if (fp == HI_NULL) {
        printf("%s %d open stream file %s fail!\n", __func__, __LINE__, (hi_char*)args);
        return HI_NULL;
    }

    while (g_stop == HI_FALSE) {
        option.expect_size = VDEC_TEST_READ_ONCE;
        ret = hi_mpi_vdec_dequeue_stream(g_handle, &option, &stream);
        if (ret != HI_SUCCESS) {
            printf("%s %d dequeue stream error!\n", __func__, __LINE__);
            sleep(10);
            continue;
        }

        ret = fread(UINT64_PTR(stream.buf_vir.integer), 1, option.expect_size, fp);
        if (ret < option.expect_size) {
            printf("read len:%d\n", ret);
            g_stop = HI_TRUE;
        }

        ret = hi_mpi_vdec_queue_stream(g_handle, HI_NULL, &stream);
        if (ret != HI_SUCCESS) {
            printf("%s %d queue stream error!\n", __func__, __LINE__);
            return HI_NULL;
        }

    }

    ret = hi_mpi_vdec_command(g_handle, HI_VDEC_CMD_SEND_EOS, HI_NULL, 0);
    if (ret != HI_SUCCESS) {
        printf("set eos fail!\n");
        return HI_NULL;
    }

    return HI_NULL;

}

hi_s32 vdec_test_case00(sample_vdec_test_param *param)
{
    hi_s32 ret = HI_FAILURE;
    hi_handle handle = HI_INVALID_HANDLE;
    hi_vdec_attr attr = {0};
    hi_vdec_frame frame = {0};
    pthread_t thread;
    hi_u32 vdec_es_size = 4 * 1024 *1024;

    ret = hi_mpi_vdec_create(&handle, HI_NULL);
    VDEC_TEST_ASSERT_RET(((ret != HI_FAILURE) && (handle != HI_INVALID_HANDLE)), ret);

    g_handle = handle;
    attr.standard = param->std;
    attr.ext_fs_num = 4;
    attr.cmp_mode = HI_VDEC_CMP_OFF;
    ret = hi_mpi_vdec_set_attr(handle, &attr);
    VDEC_TEST_ASSERT_RET((ret != HI_FAILURE), ret);

    ret = hi_mpi_vdec_command(handle, HI_VDEC_CMD_INIT_INPUT_BUF, &vdec_es_size, sizeof(hi_u32));
    VDEC_TEST_ASSERT_RET((ret != HI_FAILURE), ret);
    ret = hi_mpi_vdec_start(handle, HI_NULL);
    VDEC_TEST_ASSERT_RET((ret != HI_FAILURE), ret);
    g_stop = HI_FALSE;
    pthread_create(&thread, HI_NULL, vdec_test_stream_thread, param->file_name);

    while (recieve_frame_suc < 320 && recieve_frame_try < 1000) {
        recieve_frame_try++;
        frame.type = HI_VDEC_FRM_INFO_DRV_VIDEO;
        ret = hi_mpi_vdec_acquire_frame(handle, HI_NULL, &frame);
        if (ret != HI_SUCCESS) {
            sleep(1);
            recieve_frame_fail++;
            vdec_test_print_recieve();
            continue;
        }

        recieve_frame_suc++;
        vdec_test_print_recieve();

        release_frame_try++;
        ret = hi_mpi_vdec_release_frame(handle, HI_NULL, &frame);
        if (ret != HI_SUCCESS) {
            sleep(1);
            release_frame_fail++;
            vdec_test_print_release();
            continue;
        }
        release_frame_suc++;
        vdec_test_print_release();
    }

    g_stop = HI_TRUE;
    ret = hi_mpi_vdec_stop(handle, HI_NULL);
    VDEC_TEST_ASSERT_RET((ret != HI_FAILURE), ret);

    ret = hi_mpi_vdec_command(handle, HI_VDEC_CMD_DEINIT_INPUT_BUF, HI_NULL, 0);
    VDEC_TEST_ASSERT_RET((ret != HI_FAILURE), ret);

    ret = hi_mpi_vdec_destroy(handle, HI_NULL);
    VDEC_TEST_ASSERT_RET((ret != HI_FAILURE), ret);

    return HI_SUCCESS;
}

static hi_vdec_std vdec_test_parse_std(char *std)
{
    if (!strcmp(std, "h264") || !strcmp(std, "H264")) {
        return HI_VDEC_STD_H264;
    } else if (!strcmp(std, "h265") || !strcmp(std, "H265") ||
                !strcmp(std, "hevc") || !strcmp(std, "HEVC")) {
        return HI_VDEC_STD_H265;
    }

    return HI_VDEC_STD_H265;
}

hi_s32 main(hi_s32 argc, char *argv[])
{
    sample_vdec_test_param param = {0};
    //printf("%s %d\n", __func__, __LINE__);
    hi_unf_sys_init();
    //printf("%s %d\n", __func__, __LINE__);
    if (argc < 3) {
        printf("%s %d\n", __func__, __LINE__);
        return HI_FAILURE;
    }
    //printf("%s %d\n", __func__, __LINE__);
    param.std = vdec_test_parse_std(argv[2]);
    //printf("%s %d\n", __func__, __LINE__);
    snprintf(param.file_name, sizeof(sample_vdec_test_param), "%s%s", CRC_DIR, argv[1]);
    //printf("%s %d\n", __func__, __LINE__);

    vdec_test_case00(&param);
    //printf("%s %d\n", __func__, __LINE__);
    hi_unf_sys_deinit();
    //printf("%s %d\n", __func__, __LINE__);

    return HI_SUCCESS;
}
