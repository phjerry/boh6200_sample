LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_customerkey
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_customerkey.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_privdata
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_privdata.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_serial_number
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_serial_number.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_selfboot
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_selfboot.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_bootdecrypt
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_bootdecrypt.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_scs
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_scs.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_trustzone
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_trustzone.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_ddrwakeup
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_ddrwakeup.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_ddrwakeupcheck
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_ddrwakeupcheck.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_globalotp
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_globalotp.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_runtimecheck
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_runtimecheck.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_bootversionid
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_bootversionid.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_bootvendorid
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_bootvendorid.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_chipid
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_chipid.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_asymmetrickeyhash
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_asymmetrickeyhash.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_msid
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_msid.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_jtagmode
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_jtagmode.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_bootmode
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_bootmode.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_rootkey
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_rootkey.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_lockidword
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_lockidword.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_productpv
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_productpv.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_longdata
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_longdata.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_taidandmsid
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_taidandmsid.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_teectrllock
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_teectrllock.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_uart
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_uart.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_algorithm
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_algorithm.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_hardonly
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_hardonly.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_rootkeyslotflag
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_rootkeyslotflag.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_vendorid
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_vendorid.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)

ifeq ($(CFG_HI_TEE_SUPPORT),y)
include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def
ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 26 ))" )))
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_MODULE := sample_otp_teec
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"
LOCAL_SRC_FILES := sample_otp_teec.c
LOCAL_C_INCLUDES := $(SDK_DIR)/source/linux/api/include \
                    $(USR_DIR)/include \
                    $(DRV_DIR)/include \
                    $(SAMPLE_DIR)/common
LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp
include $(BUILD_EXECUTABLE)
endif