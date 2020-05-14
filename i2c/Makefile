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
SAMPLE_IMAGES := sample_i2c_read sample_i2c_write

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec

include $(SAMPLE_DIR)/hi_sample_rules.mak
