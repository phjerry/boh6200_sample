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
CFLAGS :=  -I$(MSP_DIR)/jpge/include \
           -I$(MSP_DIR)/jpeg/include \
           -I$(MSP_DIR)/tde/include \
           -I$(DRV_DIR)/include \
           -I$(COMPONENT_DIR)/jpeg/include \
           -I$(SAMPLE_DIR)/iframe_dec

SAMPLE_IMAGES := sample_iframe_dec

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

ifneq ($(findstring $(CFG_HI_CHIP_TYPE),hi3716mv430),)
ifneq ($(CFG_HI_JPEG_MINI_SUPPORT), y)
COMMON_OBJS += ../iframe_dec/jpeg_soft_enc.o
CFLAGS += -DJPEG_SOFT_ENCODE_ENABLE
endif
endif

ifeq ($(CFG_HI_FRONTEND_SUPPORT), y)
CFLAGS += -DHI_FRONTEND_SUPPORT
endif

ifeq ($(CFG_HI_HIGO_SUPPORT),y)
cCFLAGS += -DHI_HIGO_SUPPORT
endif

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec

include $(SAMPLE_DIR)/hi_sample_rules.mak
