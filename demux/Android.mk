LOCAL_PATH := $(call my-dir)

#################################sample_demux_record_alldata####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_demux_record_alldata
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_demux_record_alldata.c

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(USR_DIR)/securec \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec

include $(BUILD_EXECUTABLE)

#################################sample_demux_es_unf####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_demux_es_unf
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_demux_es_unf.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(USR_DIR)/include \
                    $(USR_DIR)/securec \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec

include $(BUILD_EXECUTABLE)

#################################sample_record_index_unf####################################
include $(CLEAR_VARS)

include $(SDK_DIR)/Android.def

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_record_index_unf
LOCAL_MULTILIB := 32

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_SRC_FILES := sample_demux_record_index_unf.c

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(USR_DIR)/securec \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec

include $(BUILD_EXECUTABLE)
