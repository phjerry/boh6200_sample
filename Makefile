#===============================================================================
# export variable
#===============================================================================
SDK_DIR := $(shell cd $(CURDIR)/.. && /bin/pwd)

include $(SDK_DIR)/base.mak

include $(SAMPLE_DIR)/base.mak

#===============================================================================
# local variable
#===============================================================================
CFLAGS += $(CFG_HI_SAMPLE_CFLAGS)
CFLAGS += -I$(HI_INCLUDE_DIR) -I$(SAMPLE_DIR)/common

objects :=  capture cipher common dispmng esplay es_ts_switch tsplay otp klad audio\
            fb flash gpio hdmitx i2c iframe_dec ipplay ir jpeg keyled license lsadc\
            macrovision mosaic omxvdec omxvenc otp pip pmoc pq rawplay tde\
            uart vdec wdg

ifeq ($(CFG_HI_PLAYER_SUPPORT),y)
objects += localplay
endif

ifeq ($(CFG_HI_VENC_SUPPORT),y)
objects += venc
objects += transcode
endif

ifeq ($(CFG_HI_SCI_SUPPORT),y)
objects += sci
endif

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
objects += dvbplay
objects += frontend
endif

ifeq ($(CFG_HI_DEMUX_SUPPORT),y)
objects += demux
endif

ifeq ($(CFG_HI_TSR2RCIPHER_SUPPORT),y)
objects += tsr2rcipher
endif

ifeq ($(CFG_HI_PVR_SUPPORT),y)
ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
objects += pvr
endif
endif

ifeq ($(CFG_HI_HIGO_SUPPORT),y)
objects += 3dtv
endif

ifeq ($(CFG_HI_TEE_SUPPORT),y)
ifneq ($(findstring $(CFG_HI_CHIP_TYPE), hi3796cv300 hi3798cv200 hi3798mv200 hi3798mv300 hi3798mv310 hi3796mv200 hi3716mv450),)
objects += tee
endif
endif

ifeq ($(CFG_HI_WIFI_SUPPORT),y)
objects += wifi
endif

objects_clean := $(addsuffix _clean, $(objects))

SAMPLE_RES := $(HI_OUT_DIR)/obj/sample
SAMPLE_RES64 := $(HI_OUT_DIR)/obj64/sample

SAMPLE_INSTALL_DIR := $(HI_ROOTBOX_DIR)/usr/local

HI_SAMPLE_INSTALL := n

ifeq ($(CFG_HI_ADVCA_SUPPORT),y)
HI_SAMPLE_INSTALL := n
endif

#===============================================================================
# rules
#===============================================================================
.PHONY: all clean image $(objects) $(objects_clean) prepare sample_common sample_common_clean

all: $(objects)

clean: $(objects_clean)

image: all
ifeq ($(HI_SAMPLE_INSTALL),y)
ifeq ($(HI_USER_SPACE_LIB),y)
	$(AT)mkdir -p $(SAMPLE_INSTALL_DIR)/sample
	$(AT)test -d $(HI_OUT_DIR)/obj/sample && cp -drf $(HI_OUT_DIR)/obj/sample $(SAMPLE_INSTALL_DIR)
	$(AT)rm -rf $(SAMPLE_INSTALL_DIR)/sample/common $(SAMPLE_INSTALL_DIR)/sample/gpu/common $(SAMPLE_INSTALL_DIR)/sample/test/ddr_test/base
	$(AT)rm -rf $(SAMPLE_INSTALL_DIR)/sample/directfb $(SAMPLE_INSTALL_DIR)/sample/boringssl $(SAMPLE_INSTALL_DIR)/sample/factory_detect
	$(AT)rm -rf $(SAMPLE_INSTALL_DIR)/sample/res/cfg
endif

ifeq ($(HI_USER_SPACE_LIB64),y)
	$(AT)mkdir -p $(SAMPLE_INSTALL_DIR)/sample64
	$(AT)test -d $(HI_OUT_DIR)/obj64/sample && cp -drf $(HI_OUT_DIR)/obj64/sample/* $(SAMPLE_INSTALL_DIR)/sample64
ifeq ($(HI_USER_SPACE_LIB),y)
	$(AT)rm -rf $(SAMPLE_INSTALL_DIR)/sample64/res
	$(AT)ln -s ../sample/res $(SAMPLE_INSTALL_DIR)/sample64/res
endif
	$(AT)rm -rf $(SAMPLE_INSTALL_DIR)/sample64/common $(SAMPLE_INSTALL_DIR)/sample64/gpu/common $(SAMPLE_INSTALL_DIR)/sample64/test/ddr_test/base
	$(AT)rm -rf $(SAMPLE_INSTALL_DIR)/sample64/directfb $(SAMPLE_INSTALL_DIR)/sample64/boringssl $(SAMPLE_INSTALL_DIR)/sample64/factory_detect
endif

	$(AT)find $(SAMPLE_INSTALL_DIR) -name "*.o" | xargs rm -rf
endif

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
	$(AT)mkdir -p $(SAMPLE_INSTALL_DIR)/cfg
    ifeq ($(CFG_HI_CHIP_TYPE),hi3796mv200)
		$(AT)cp  $(SAMPLE_DIR)/res/cfg/frontend_config.hi3796mv2*.ini $(SAMPLE_INSTALL_DIR)/cfg
        ifneq ($(findstring dmb, $(HI_OUT_DIR)),)
			$(AT)cp  $(SAMPLE_INSTALL_DIR)/cfg/*dmb.ini $(SAMPLE_INSTALL_DIR)/cfg/frontend_config.ini
        else
			$(AT)cp  $(SAMPLE_INSTALL_DIR)/cfg/*dma.ini $(SAMPLE_INSTALL_DIR)/cfg/frontend_config.ini
        endif
    else
		$(AT)cp -drf $(SAMPLE_DIR)/res/cfg/frontend_config*.ini  $(SAMPLE_INSTALL_DIR)/cfg
    endif
	test ! -f $(SAMPLE_DIR)/res/cfg/frontend_config.ini || cp $(SAMPLE_DIR)/res/cfg/frontend_config.ini $(SAMPLE_INSTALL_DIR)/cfg/frontend_config.ini
endif
	$(AT)echo "" > /dev/null

$(objects): sample_common
	$(AT)make -C $@

$(objects_clean): sample_common_clean
	$(AT)make -C $(patsubst %_clean, %, $@) clean

sample_common: prepare
	$(AT)make -C common

sample_common_clean:
	$(AT)make -C common clean

prepare:
ifeq ($(HI_USER_SPACE_LIB),y)
	$(AT)test -d $(SAMPLE_RES) || mkdir -p $(SAMPLE_RES)
	$(AT)test -d $(SAMPLE_RES)/res || cp -rf $(SAMPLE_DIR)/res $(SAMPLE_RES)/
endif
ifeq ($(HI_USER_SPACE_LIB64),y)
	$(AT)test -d $(SAMPLE_RES64) || mkdir -p $(SAMPLE_RES64)
	$(AT)test -d $(SAMPLE_RES64)/res || cp -rf $(SAMPLE_DIR)/res $(SAMPLE_RES64)/
endif
