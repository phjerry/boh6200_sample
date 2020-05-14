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
          -I$(USR_DIR)/memory/include \
          -I$(USR_DIR)/common/include \
          -I$(USR_DIR)/securec \
          -I$(DRV_DIR)/include

CFLAGS += -DHI_PVR_LINEARIZATION_SUPPORT
CFLAGS += -DHI_PVR_POST_PROCESSING_SUPPORT

SAMPLE_IMAGES := sample_pvr_rec \
                 sample_pvr_eth_timeshift \
                 sample_pvr_play \
                 sample_pvr_rec_allts \
                 sample_pvr_timeshift \
                 sample_pvr_file_ctrl

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

#DEPEND_LIBS := $(HI_LIBS)
DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec

include $(SAMPLE_DIR)/hi_sample_rules.mak
