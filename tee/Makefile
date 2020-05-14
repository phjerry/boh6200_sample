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
SAMPLE_IMAGES := sample_tee_tsplay \
                 sample_tee_tsplay_pip

CFLAGS += -I$(TEE_API_INCLUDE) \
          -I$(USR_DIR)/include \
          -I$(DRV_DIR)/include \
          -I$(SAMPLE_DIR)/common

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
    SAMPLE_IMAGES +=sample_tee_dvbplay
endif

ifeq ($(CFG_HI_TEE_DEMO_SUPPORT),y)
    SAMPLE_IMAGES +=sample_tee_demo
endif

SAMPLE_IMAGES += sample_tee_dynamic_demo

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)
LOCAL_OBJS := sample_tee_common.o

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec -lteec

include $(SAMPLE_DIR)/hi_sample_rules.mak
