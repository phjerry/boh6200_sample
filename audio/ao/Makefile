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

SAMPLE_IMAGES := sample_mixengine

# SAMPLE_IMAGES += sample_audio_mix

# COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_common -lhi_memory -lhi_securec -lhi_msp -lhi_dispmng

include $(SAMPLE_DIR)/hi_sample_rules.mak
