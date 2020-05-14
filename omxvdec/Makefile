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
SAMPLE_IMAGES := sample_omxvdec

LOCAL_OBJS :=  sample_omxvdec.o sample_queue.o sample_tidx.o

#DEPEND_LIBS := $(HI_LIBS) $(SYS_LIBS) -lOMX_Core
DEPEND_LIBS := -lhi_common -lhi_memory -lhi_securec -lOMX_Core

CFLAGS += -I$(USR_DIR)/omx/include
CFLAGS += -I$(USR_DIR)/include                    \
          -I$(USR_DIR)/common/include             \
          -I$(DRV_DIR)/include                    \
          -I$(DRV_DIR)/common/include             \
          -I$(USR_DIR)/securec

ifeq ($(CFG_HI_SMMU_SUPPORT),y)
CFLAGS += -DHI_SMMU_SUPPORT
endif

ifeq ($(CFG_HI_TEE_SUPPORT), y)
CFLAGS += -DHI_OMX_TEE_SUPPORT
CFLAGS += -I$(TEE_API_INCLUDE)
CFLAGS += -I$(COMMON_API_INCLUDE)/
DEPEND_LIBS += -lteec
endif

include $(SAMPLE_DIR)/hi_sample_rules.mak


