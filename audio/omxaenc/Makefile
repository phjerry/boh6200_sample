#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell cd $(CURDIR)/../../.. && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================
CFLAGS := \
    -I$(USR_DIR)/include \
    -I$(USR_DIR)/msp/include \
    -I$(USR_DIR)/memory/include \
    -I$(USR_DIR)/common/include \
    -I$(USR_DIR)/securec \
    -I$(DRV_DIR)/include

SAMPLE_IMAGES := sample_omxaenc_aac

CFLAGS += \
    -I$(USR_DIR)/omx/include \
    -I$(USR_DIR)/omx/omx_audio/common/base \
    -I$(USR_DIR)/omx/omx_audio/common/osal

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lOMX_Core

include $(SAMPLE_DIR)/hi_sample_rules.mak
