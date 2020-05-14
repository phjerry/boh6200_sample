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

SAMPLE_IMAGES := sample_tsplay \
                 sample_tsplay_pid \
                 sample_tsplay_multiaud

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)
LOCAL_OBJS := sample_tsplay_common.o

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec -lhi_dispmng

ifeq ($(CFG_HI_BOOT_HOMOLOGOUS_SUPPORT),y)
CFLAGS += -DHI_BOOT_HOMOLOGOUS_SUPPORT
endif

include $(SAMPLE_DIR)/hi_sample_rules.mak
