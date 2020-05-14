#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SAMPLE_DIR)/base.mak

CFLAGS := -I$(USR_DIR)/include \
          -I$(DRV_DIR)/include \
          -I$(SAMPLE_DIR)/common

#===============================================================================
# local variable
#===============================================================================
SAMPLE_IMAGES := sample_ts_transcode

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
SAMPLE_IMAGES += sample_dvb_transcode
endif

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec

include $(SAMPLE_DIR)/hi_sample_rules.mak
