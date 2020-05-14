#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SAMPLE_DIR)/base.mak

CFLAGS  +=  -I$(DRV_DIR)/include
#===============================================================================
# local variable
#===============================================================================
SAMPLE_IMAGES := sample_blend \
    sample_colorfill \
    sample_colorkey \
    sample_quickcopy \
    sample_resize \
    sample_yuv_to_rgb \
    sample_compose

LOCAL_OBJS  := tde_common.o

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_common -lhi_memory -lhi_securec -lhi_msp

include $(SAMPLE_DIR)/hi_sample_rules.mak
