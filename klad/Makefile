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
          -I$(USR_DIR)/memory/include \
          -I$(USR_DIR)/common/include \
          -I$(USR_DIR)/securec \
          -I$(DRV_DIR)/include \
          -I$(TEE_API_INCLUDE)

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS :=  -lhi_common -lhi_memory -lhi_securec -lhi_msp -lhi_dispmng

SAMPLE_IMAGES := sample_klad_tsplay_clearcw  sample_klad_clear sample_keyslot sample_klad_oem_sw \
                  sample_klad_oem_r2r sample_klad_oem_hdcp \
                  sample_klad_com_sync sample_klad_com_async

ifeq ($(CFG_HI_TSR2RCIPHER_SUPPORT), y)
SAMPLE_IMAGES += sample_klad_tsr2rcipher
endif

ifeq ($(CFG_HI_TEE_SUPPORT),y)
SAMPLE_IMAGES +=   sample_keyslot_teec sample_klad_teec sample_klad_teec_com_sync sample_klad_teec_com_aync
endif
include $(SAMPLE_DIR)/hi_sample_rules.mak
