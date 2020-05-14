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
          -I$(SAMPLE_DIR)/common \
		  -I$(SAMPLE_DIR)/seperateoutput \
		  -I$(USR_DIR)/msp/include

SAMPLE_IMAGES := seperateoutput

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

#DEPEND_LIBS := $(HI_LIBS)
DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec

include $(SAMPLE_DIR)/hi_sample_rules.mak
