#===============================================================================
# export variable
#===============================================================================
SDK_DIR := $(shell cd $(CURDIR)/../.. && /bin/pwd)

include $(SDK_DIR)/base.mak

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================

CFLAGS := -I$(USR_DIR)/include \
          -I$(USR_DIR)/msp/include \
          -I$(USR_DIR)/memory/include \
          -I$(USR_DIR)/common/include \
          -I$(USR_DIR)/securec \
          -I$(DRV_DIR)/include

ifeq ($(CFG_HI_BOOT_HOMOLOGOUS_SUPPORT),y)
CFLAGS += -DHI_BOOT_HOMOLOGOUS_SUPPORT
endif

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

include $(SAMPLE_DIR)/hi_sample_rules.mak
