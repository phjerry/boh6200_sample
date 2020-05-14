#===============================================================================
# export variable
#===============================================================================
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)

include $(SDK_DIR)/base.mak

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================
CFLAGS := -I$(DRV_DIR)/include

SAMPLE_IMAGES := sample_ipcfg

LOCAL_OBJS := hi_api_eth.o

DEPEND_LIBS := -lhi_securec -lhi_memory -lhi_common

include $(SAMPLE_DIR)/hi_sample_rules.mak
