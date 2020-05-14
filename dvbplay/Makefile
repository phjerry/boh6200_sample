#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================
CFLAGS := -I$(USR_DIR)/include \
          -I$(DRV_DIR)/include \
          -I$(SAMPLE_DIR)/common

CFLAGS += -D_GNU_SOURCE -D_XOPEN_SOURCE=600
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

ifeq ($(CFG_HI_KEYLED_SUPPORT),y)
CFLAGS += -DHI_KEYLED_SUPPORT
endif

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
CFLAGS += -DHI_FRONTEND_SUPPORT
endif

SAMPLE_IMAGES := sample_dvbplay \
                 sample_dvbplay_ir \
                 sample_dvbplay_pid

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec

include $(SAMPLE_DIR)/hi_sample_rules.mak
