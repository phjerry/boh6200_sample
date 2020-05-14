#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SAMPLE_DIR)/base.mak
CFLAGS  :=  -I$(DRV_DIR)/include
#===============================================================================
# local variable
#===============================================================================
ifeq ($(CFG_HI_FB_DECOMPRESS_SUPPORT),y)
CFLAGS += -DHI_FB_DECOMPRESS_SUPPORT
endif

SAMPLE_IMAGES := sample_fb

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS :=  -lhi_common -lhi_memory -lhi_securec -lhi_msp -lhi_dispmng

include $(SAMPLE_DIR)/hi_sample_rules.mak
