#===============================================================================
# export variable
#===============================================================================
ifeq ($(CFG_HI_EXPORT_FLAG),)
SDK_DIR     := $(shell cd $(CURDIR)/../../.. && /bin/pwd)
SAMPLE_DIR    := $(shell cd $(SDK_DIR)/sample && /bin/pwd)
include $(SDK_DIR)/base.mak
endif

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================

CFLAGS := -I$(USR_DIR)/include \
          -I$(DRV_DIR)/include \
          -I$(SAMPLE_DIR)/common

CFLAGS  +=  -I$(CURDIR)
		  
CFLAGS  +=  -I$(SDK_DIR)/source/linux/api/memory/include

ifeq ($(CFG_HI_FB_DECOMPRESS_SUPPORT),y)
CFLAGS := -DHI_FB_DECOMPRESS_SUPPORT
endif
SAMPLE_IMAGES := sample_vmin 


COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)
COMMON_OBJS += ../test/vmin/hi_fb_ui.o \
			   ../test/vmin/hi_demux_record.o

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec -lhi_dispmng

ifeq ($(CFG_HI_BOOT_HOMOLOGOUS_SUPPORT),y)
CFLAGS += -DHI_BOOT_HOMOLOGOUS_SUPPORT
endif

include $(SAMPLE_DIR)/hi_sample_rules.mak




