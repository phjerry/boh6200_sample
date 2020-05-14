LOCAL_PATH := $(call my-dir)

#########################DVB_TRANSCODE##############################
include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE := sample_i2c_read
ifeq (${CFG_HI_CPU_ARCH},arm64)
LOCAL_32_BIT_ONLY := true
else
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)
endif

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_i2c_read.c

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(DRV_DIR)/include \
		    $(SAMPLE_DIR)/common \
                    $(SDK_DIR)/source/component/ha_codec/include \
                    $(SDK_DIR)/source/linux/api/securec \
                    $(SDK_DIR)/source/linux/drv/hifb/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec libhi_dispmng

include $(BUILD_EXECUTABLE)


#########################DVB_TRANSCODE##############################
include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE := sample_i2c_write
ifeq (${CFG_HI_CPU_ARCH},arm64)
LOCAL_32_BIT_ONLY := true
else
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)
endif

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_i2c_write.c

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(DRV_DIR)/include \
		    $(SAMPLE_DIR)/common \
                    $(SDK_DIR)/source/component/ha_codec/include \
                    $(SDK_DIR)/source/linux/api/securec \
                    $(SDK_DIR)/source/linux/drv/hifb/include

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec libhi_dispmng

include $(BUILD_EXECUTABLE)
