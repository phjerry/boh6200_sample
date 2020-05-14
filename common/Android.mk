LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

LOCAL_PRELINK_MODULE := false

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libhi_sample_common
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

LOCAL_CFLAGS := -fstack-protector-strong -Wl,-z,relro -Wl,-z,noexecstack -fPIE -pie

ifeq ($(CFG_HI_CHIP_TYPE), hi3796cv300 hi3751v900)
LOCAL_CFLAGS += -mcpu=cortex-a73
endif

LOCAL_CFLAGS := -DANDROIDP

LOCAL_CFLAGS += -D_GNU_SOURCE -Wall -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections
LOCAL_CFLAGS += -DCHIP_TYPE_$(CFG_HI_CHIP_TYPE) -DCFG_HI_SDK_VERSION=$(CFG_HI_SDK_VERSION)
LOCAL_CFLAGS += $(CFG_HI_BOARD_CONFIGS)

LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

ifeq ($(CFG_HI_HDMI_SUPPORT_HDCP),y)
LOCAL_CFLAGS += -DHI_HDCP_SUPPORT
endif

ifeq ($(CFG_HI_DMX_TSBUF_MULTI_THREAD_SUPPORT), y)
LOCAL_CFLAGS += -DHI_DMX_TSBUF_MULTI_THREAD_SUPPORT
endif

ifeq ($(CFG_HI_LOG_SUPPORT),y)
LOCAL_CFLAGS += -DHI_LOG_SUPPORT=1
endif

ifeq ($(CFG_HI_ADVCA_FUNCTION),FINAL)
LOCAL_CFLAGS += -DHI_ADVCA_FUNCTION_RELEASE
else
LOCAL_CFLAGS += -DHI_ADVCA_FUNCTION_$(CFG_HI_ADVCA_FUNCTION)
endif

SYS_LIBS := -lpthread -lrt -lm -ldl

ifeq ($(CFG_HI_LIBCPP_SUPPORT),y)
SYS_LIBS += -lstdc++
endif

#LOCAL_SRC_FILES := $(call all-c-files-under,.)
LOCAL_SRC_FILES := hi_adp_demux.c \
                   hi_adp_data.c \
                   hi_adp_search.c \
                   hi_adp_ini.c \
                   hi_filter.c \
                   hi_adp_mpi.c

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
LOCAL_SRC_FILES += hi_adp_frontend.c
endif

ifeq ($(CFG_HI_PVR_SUPPORT),y)
ifeq ($(CFG_HI_PVR_SIGNAL_FILE_STORAGE),y)
LOCAL_CFLAGS += -DHI_PVR_SIGNAL_FILE_STORAGE_SUPPORT
endif
LOCAL_SRC_FILES += hi_adp_pvr.c
endif

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(USR_DIR)/msp/include \
                    $(USR_DIR)/memory/include \
                    $(USR_DIR)/common/include \
                    $(USR_DIR)/msp/tde/include \
                    $(USR_DIR)/securec \
                    $(DRV_DIR)/include \
                    $(COMPONENT_DIR)/ha_codec/include

LOCAL_C_INCLUDES += $(TOP)/external/skia/include/core
LOCAL_C_INCLUDES += $(TOP)/external/skia/include/images
LOCAL_C_INCLUDES += $(TOP)/external/skia/tools
LOCAL_C_INCLUDES += $(TOP)/external/skia/include/private
LOCAL_C_INCLUDES += $(TOP)/frameworks/base/core/jni/include
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/libs/nativewindow/include
LOCAL_C_INCLUDES += $(TOP)/frameworks/native/include
LOCAL_C_INCLUDES += $(TOP)/device/hisilicon/bigfish/hardware/gpu/android/gralloc
LOCAL_C_INCLUDES += $(TOP)/device/hisilicon/bigfish/hardware/gpu/$(HISI_GPU_DIR)/android/gralloc
LOCAL_C_INCLUDES += $(SDK_DIR)/source/msp/api/tde/include
LOCAL_C_INCLUDES += $(SDK_DIR)/source/msp/api/higo/include

LOCAL_SHARED_LIBRARIES := libcutils libutils libhi_common libhi_msp libui libdl liblog libhi_memory libhi_securec libhi_dispmng

include $(BUILD_SHARED_LIBRARY)
