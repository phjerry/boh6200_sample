LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_frontend
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_frontend.c

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(USR_DIR)/msp/include \
                    $(USR_DIR)/memory/include \
                    $(USR_DIR)/common/include \
                    $(USR_DIR)/securec \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common

LOCAL_SHARED_LIBRARIES := libhi_common libhi_memory libhi_securec libhi_msp libhi_dispmng libhi_sample_common

include $(BUILD_EXECUTABLE)
