LOCAL_PATH := $(call my-dir)
SAMPLE_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

define first_makefile_under
    $(wildcard $(addsuffix /Android.mk, $(addprefix $(SAMPLE_PATH)/,$(1))))
endef

SAMPLE_MODULE := common network esplay tsplay omxvdec dispmng hdmitx i2c i2c-tools

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
SAMPLE_MODULE += demux frontend
endif

ifeq ($(CFG_HI_TEE_SUPPORT),y)
SAMPLE_MODULE += tee
endif

include $(call first_makefile_under, $(SAMPLE_MODULE))
