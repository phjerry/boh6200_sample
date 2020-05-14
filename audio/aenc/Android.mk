LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_aenc
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_aenc.c

LOCAL_C_INCLUDES := $(USR_DIR)/include
LOCAL_C_INCLUDES += $(USR_DIR)/msp/include \
LOCAL_C_INCLUDES += $(DRV_DIR)/include \
LOCAL_C_INCLUDES += $(SAMPLE_DIR)/common

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_aenc_track
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_aenc_track.c

LOCAL_C_INCLUDES := $(USR_DIR)/include
LOCAL_C_INCLUDES += $(USR_DIR)/msp/include \
LOCAL_C_INCLUDES += $(DRV_DIR)/include \
LOCAL_C_INCLUDES += $(SAMPLE_DIR)/common

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common

include $(BUILD_EXECUTABLE)
