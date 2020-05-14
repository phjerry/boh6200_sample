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
          -I$(DRV_DIR)/include

SAMPLE_IMAGES := \
        sample_rng \
        sample_rsa_enc \
        sample_rsa_sign \
        sample_dh \
        sample_ecdh \
        sample_ecdsa \
        sample_sm2 \
        sample_cipher \
        sample_cbcmac \
        sample_multicipher \
        sample_cenc \
        sample_cenc_asyn \
        sample_hash

DEPEND_LIBS := $(HI_LIBS)

include $(SAMPLE_DIR)/hi_sample_rules.mak
