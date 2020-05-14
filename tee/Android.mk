LOCAL_PATH := $(call my-dir)

#################################sample_tee_tsplay####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_tee_tsplay
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_tee_tsplay.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common \
                    $(SDK_DIR)/source/component/ha_codec/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common

include $(BUILD_EXECUTABLE)

#################################sample_tee_tsplay_pip####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_tee_tsplay_pip
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_tee_tsplay_pip.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common \
                    $(SDK_DIR)/source/component/ha_codec/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common

include $(BUILD_EXECUTABLE)

#################################sample_tee_dvbplay####################################
ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_tee_dvbplay
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_tee_dvbplay.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common \
                    $(SDK_DIR)/source/component/ha_codec/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common

include $(BUILD_EXECUTABLE)
endif

#################################sample_tee_dynamic_demo####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_tee_dynamic_demo
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_tee_dynamic_demo.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common \
                    $(TEE_API_INCLUDE) \
                    $(SDK_DIR)/source/component/ha_codec/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libteec

include $(BUILD_EXECUTABLE)
