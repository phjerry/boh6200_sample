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
	-I$(DRV_DIR)/include \
	-I$(SAMPLE_DIR)/common

SAMPLE_IMAGES := sample_aenc sample_aenc_track

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_common -lhi_memory -lhi_securec -lhi_msp -lhi_dispmng

include $(SAMPLE_DIR)/hi_sample_rules.mak
