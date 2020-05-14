LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_omx_play
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_omx_play.c

LOCAL_C_INCLUDES := $(USR_DIR)/include
LOCAL_C_INCLUDES += $(USR_DIR)/msp/include
LOCAL_C_INCLUDES += $(DRV_DIR)/include
LOCAL_C_INCLUDES += $(SAMPLE_DIR)/common
LOCAL_C_INCLUDES += $(USR_DIR)/omx/include
LOCAL_C_INCLUDES += $(USR_DIR)/omx/omx_audio/common/base
LOCAL_C_INCLUDES += $(USR_DIR)/omx/omx_audio/common/osal

LOCAL_SHARED_LIBRARIES := \
    libutils\
    liblog \
    libOMX_Core \
    libOMX.hisi.audio.utils \
    liblog \
    libcutils \
    libdl \
    libm \
    libhi_common\
    libhi_msp \
    libhi_sample_common

include $(BUILD_EXECUTABLE)
