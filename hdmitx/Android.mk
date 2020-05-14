LOCAL_PATH := $(call my-dir)

#################################sample_hdmi_tsplay####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_hdmi_tsplay
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := common/sample_hdmi_common.c \
              common/hdmi_test_cmd.c \
              sample_hdmi_tsplay.c

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common \
		    $(LOCAL_PATH)/common \
                    $(SDK_DIR)/source/component/ha_codec/include \
                    $(SDK_DIR)/source/linux/api/securec \
                    $(SDK_DIR)/source/linux/drv/hifb/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec libhi_dispmng

include $(BUILD_EXECUTABLE)
