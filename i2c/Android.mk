LOCAL_PATH := $(call my-dir)

#########################DVB_TRANSCODE##############################
include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE := inv478x_main
ifeq (${CFG_HI_CPU_ARCH},arm64)
LOCAL_32_BIT_ONLY := true
else
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)
endif

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := $(CFG_HI_CFLAGS) $(CFG_HI_BOARD_CONFIGS)
LOCAL_CFLAGS += -DLOG_TAG=\"$(LOCAL_MODULE)\"

LOCAL_CFLAGS += -Wno-error=date-time

LOCAL_SRC_FILES := ./inv478x_app_tx_apis/inv478x_main.c \
				   ./inv_host_drivers/inv478x/inv478x_hal/inv478x_hal.c \
				   ./inv_host_drivers/inv478x/inv478x.c \
				   ./inv_host_drivers/common/cra/inv_drv_cra.c \
				   ./inv_host_drivers/common/ipc/inv_drv_ipc.c\
				   ./inv_host_drivers/common/isp/inv_drv_isp.c \
				   ./inv_host_drivers/common/log/inv_sys_log.c \
				   ./inv_host_drivers/common/malloc/inv_sys_malloc.c \
				   ./inv_host_drivers/common/obj/inv_sys_obj.c \
				   ./inv_host_drivers/common/time/inv_sys_time.c \
				   ./inv_host_drivers/common/ipc_master/inv_drv_ipc_master.c \
				   ./inv_host_drivers/platforms/cec/inv_cec_api.c \
				   ./inv_host_drivers/platforms/pc/inv_platform_wrapper.c \
				   ./inv_host_drivers/platforms/pc/inv_adapter.c
#				   ./inv478x_ArcRx_example/inv478x_main.c \

LOCAL_C_INCLUDES := $(USR_DIR)/include \
                    $(DRV_DIR)/include \
					$(SAMPLE_DIR)/common \
                    $(SDK_DIR)/source/component/ha_codec/include \
                    $(SDK_DIR)/source/linux/api/securec \
                    $(SDK_DIR)/source/linux/drv/hifb/include \
					$(LOCAL_PATH)/inv478x_ArcRx_example   \
					$(LOCAL_PATH)/inv_host_drivers/inv478x/inv478x_hal   \
					$(LOCAL_PATH)/inv_host_drivers/inv478x \
					$(LOCAL_PATH)/inv_host_drivers/common/cra\
					$(LOCAL_PATH)/inv_host_drivers/common/ipc  \
					$(LOCAL_PATH)/inv_host_drivers/common/isp  \
					$(LOCAL_PATH)/inv_host_drivers/common/log  \
					$(LOCAL_PATH)/inv_host_drivers/common/malloc  \
					$(LOCAL_PATH)/inv_host_drivers/common/obj  \
					$(LOCAL_PATH)/inv_host_drivers/common/seq  \
					$(LOCAL_PATH)/inv_host_drivers/common/time  \
					$(LOCAL_PATH)/inv_host_drivers/internal \
					$(LOCAL_PATH)/inv_host_drivers/platforms/pc \
					$(LOCAL_PATH)/inv_host_drivers/platforms \
					$(LOCAL_PATH)/inv_host_drivers/platforms/cec\
					$(LOCAL_PATH)/inv_host_drivers/common\
					$(LOCAL_PATH)/inv_host_drivers/common/ipc_master\
					$(LOCAL_PATH)/inv_host_drivers/inv478x/comlib

LOCAL_SHARED_LIBRARIES := libcutils libhi_common libhi_msp libhi_sample_common libhi_memory libhi_securec libhi_dispmng

include $(BUILD_EXECUTABLE)

