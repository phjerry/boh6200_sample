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
          -I$(USR_DIR)/msp/include \
          -I$(USR_DIR)/securec \
          -I$(DRV_DIR)/include \
          -I$(SAMPLE_DIR)/common \
          -Icommon \

CFLAGS += -D_GNU_SOURCE -D_XOPEN_SOURCE=600
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

SAMPLE_IMAGES := sample_hdmi_tsplay

LOCAL_OBJS := common/sample_hdmi_common.o \
              common/hdmi_test_cmd.o \
              sample_hdmi_tsplay.o

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec -lhi_dispmng

ifeq ($(CFG_HI_BOOT_HOMOLOGOUS_SUPPORT),y)
CFLAGS += -DHI_BOOT_HOMOLOGOUS_SUPPORT
endif

include $(SAMPLE_DIR)/hi_sample_rules.mak
