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

ifeq ($(CFG_HI_TEE_SUPPORT),y)
SAMPLE_IMAGES +=   sample_otp_teec
endif

SAMPLE_IMAGES += sample_otp_customerkey \
                 sample_otp_privdata \
                 sample_otp_serial_number \
                 sample_otp_selfboot \
                 sample_otp_bootdecrypt \
                 sample_otp_scs \
                 sample_otp_trustzone \
                 sample_otp_ddrwakeup \
                 sample_otp_ddrwakeupcheck \
                 sample_otp_globalotp \
                 sample_otp_runtimecheck \
                 sample_otp_bootversionid \
                 sample_otp_bootvendorid \
                 sample_otp_chipid \
                 sample_otp_asymmetrickeyhash  \
                 sample_otp_msid \
                 sample_otp_jtagmode \
                 sample_otp_bootmode \
                 sample_otp_rootkey \
                 sample_otp_lockidword \
                 sample_otp_productpv \
                 sample_otp_longdata \
                 sample_otp_taidandmsid \
                 sample_otp_teectrllock \
                 sample_otp_uart \
                 sample_otp_algorithm \
                 sample_otp_hardonly \
                 sample_otp_rootkeyslotflag \
                 sample_otp_vendorid
    
include $(SAMPLE_DIR)/hi_sample_rules.mak
