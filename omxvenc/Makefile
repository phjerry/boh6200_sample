#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SDK_DIR)/sample/base.mak

#===============================================================================
# local variable
#===============================================================================
SAMPLE_IMAGES := sample_omxvenc

LOCAL_OBJS :=  sample_omxvenc.o queue.o

DEPEND_LIBS := -lhi_common -lhi_memory -lhi_securec -lOMX_Core -lrt -lm

CFLAGS  += -I$(USR_DIR)/omx/include
CFLAGS  += -I$(USR_DIR)/common/include	\
		   -I$(DRV_DIR)/include			\
		   -I$(USR_DIR)/securec			\
		   -I$(USR_DIR)/memory/include \

ifeq ($(CFG_HI_SMMU_SUPPORT),y)
CFLAGS += -DHI_SMMU_SUPPORT
endif

#===============================================================================
# rules
#===============================================================================
include $(SAMPLE_DIR)/hi_sample_rules.mak
