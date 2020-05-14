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
CFLAGS := -I$(DRV_DIR)/include                \
          -I$(USR_DIR)/securec                \
          -I$(DRV_DIR)/hifb/include           \
          -I$(COMPONENT_DIR)/jpeg/include     \
          -I$(COMPONENT_DIR)/png/include      \
          -I$(COMPONENT_DIR)/zlib/include     \
          -I$(COMPONENT_DIR)/freetype/include \
          -I$(COMPONENT_DIR)/freetype/include/freetype2

CFLAGS += -DCONFIG_DISP_CALL

SAMPLE_IMAGES := sample_disp

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)
LOCAL_OBJS := sample_text.o sample_jpeg.o sample_png.o

DEPEND_LIBS :=  -lhi_common -lhi_memory -lhi_securec -lhi_msp -lhi_dispmng -ljpeg -lpng -lfreetype -lz

include $(SAMPLE_DIR)/hi_sample_rules.mak
