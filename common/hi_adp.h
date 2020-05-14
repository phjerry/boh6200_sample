#ifndef __HI_ADP_H
#define __HI_ADP_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hi_type.h"

#if defined(ANDROID)
extern void LogPrint(const char *format, ...);
#endif

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define SAMPLE_COMMON_PRINTF
#else
#if defined(ANDROID)
#define SAMPLE_COMMON_PRINTF LogPrint
#else
#define SAMPLE_COMMON_PRINTF printf
#endif
#endif

#define HIAPI_RUN(api, ret) \
    do { \
        hi_s32 errCode; \
        errCode = api; \
        if (errCode != 0) \
        { \
            ret |= errCode; \
            SAMPLE_COMMON_PRINTF("\033[0;31m" "[Function: %s line: %d] %s failed ret = 0x%x \n" "\033[0m", \
            __FUNCTION__, __LINE__, # api, errCode); \
        } \
    } while (0)

#define HIAPI_RUN_RETURN(api) \
    do { \
        hi_s32 errCode; \
        errCode = api; \
        if (errCode != 0) \
        { \
            SAMPLE_COMMON_PRINTF("\033[0;31m" "[Function: %s line: %d] %s failed ret = 0x%x \n" "\033[0m", \
            __FUNCTION__, __LINE__, # api, errCode); \
            return HI_FAILURE; \
        } \
    } while (0)

#define HIAPI_RUN_RETURN_FN(api, fn) \
    do { \
        hi_s32 errCode; \
        errCode = api; \
        if (errCode != 0) \
        { \
            SAMPLE_COMMON_PRINTF("\033[0;31m" "[Function: %s line: %d] %s failed ret = 0x%x \n" "\033[0m", \
            __FUNCTION__, __LINE__, # api, errCode); \
            fn; \
            return HI_FAILURE; \
        } \
    } while (0)

#define HIAPI_ERR_PRINTF(ret) \
    do { \
        SAMPLE_COMMON_PRINTF("\033[0;31m" " [Function: %s line: %d]  ret = 0x%x \n" "\033[0m", __FUNCTION__, __LINE__, ret); \
    } while (0) \


#define PRINT_SMP(fmt...) SAMPLE_COMMON_PRINTF(fmt)

#define SAMPLE_RUN(api, ret) \
    do { \
        hi_s32 l_ret = api; \
        if (l_ret != HI_SUCCESS) \
        { \
            PRINT_SMP("run %s failed, ERRNO:%#x.\n", # api, l_ret); \
        } \
        ret = l_ret; \
    } while (0)

#define SAMPLE_CHECK_NULL_PTR(pointer) \
    do { \
        if (pointer == NULL) \
        { \
            PRINT_SMP("%s failed:NULL Pointer in Line:%d!\n", __FUNCTION__, __LINE__); \
            return HI_FAILURE; \
        } \
    } while (0)

#ifdef ANDROID
#define SAMPLE_GET_INPUTCMD(input_cmd) \
    do { \
        memset(input_cmd, 0, sizeof(input_cmd)); \
        read(0, input_cmd, sizeof(input_cmd)); \
    } while (0)

#define HISI_SAMPLE_FIFO "/dev/hisi_sample_fifo"

#define HI_GET_INPUTCMD(input_cmd)   \
    do { \
        int t_fd; \
        unlink(HISI_SAMPLE_FIFO); \
        if (mkfifo(HISI_SAMPLE_FIFO, 0777) != -1) \
        { \
            t_fd = open(HISI_SAMPLE_FIFO, O_RDONLY); \
            if (t_fd != -1) \
            { \
                memset(input_cmd, 0, sizeof(input_cmd)); \
                read(t_fd, input_cmd, sizeof(input_cmd)); \
                close(t_fd); \
            } \
            else \
            { \
                perror("Can't open the FIFO:"); \
                exit(0); \
            } \
        } \
        else \
        { \
            perror("Can't create FIFO channel:"); \
            exit(0); \
        } \
    } while (0)
#else
#define SAMPLE_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#define HI_GET_INPUTCMD(input_cmd) fgets((char *)(input_cmd), (sizeof(input_cmd) - 1), stdin)
#endif

#endif
