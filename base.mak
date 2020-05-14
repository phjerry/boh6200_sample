#==============COMPILE OPTIONS================================================================
CFG_HI_SAMPLE_CFLAGS := -Werror -Wall

CFG_HI_SAMPLE_CFLAGS += -fstack-protector-strong
CFG_HI_SAMPLE_CFLAGS += -Wl,-z,relro
CFG_HI_SAMPLE_CFLAGS += -z,now
CFG_HI_SAMPLE_CFLAGS += -Wl,-z,noexecstack
CFG_HI_SAMPLE_CFLAGS += -fPIE -pie

ifeq ($(CFG_HI_CHIP_TYPE), hi3796cv300 hi3751v900)
CFG_HI_SAMPLE_CFLAGS += -mcpu=cortex-a73
endif

CFG_HI_SAMPLE_CFLAGS += -D_GNU_SOURCE -Wall -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections
CFG_HI_SAMPLE_CFLAGS += -DCHIP_TYPE_$(CFG_HI_CHIP_TYPE) -DCFG_HI_SDK_VERSION=$(CFG_HI_SDK_VERSION)
CFG_HI_SAMPLE_CFLAGS += $(CFG_HI_BOARD_CONFIGS)

ifeq ($(CFG_HI_HDMI_SUPPORT_HDCP),y)
CFG_HI_SAMPLE_CFLAGS += -DHI_HDCP_SUPPORT
endif

ifeq ($(CFG_HI_DMX_TSBUF_MULTI_THREAD_SUPPORT), y)
CFG_HI_SAMPLE_CFLAGS += -DHI_DMX_TSBUF_MULTI_THREAD_SUPPORT
endif

ifeq ($(CFG_HI_LOG_SUPPORT),y)
CFG_HI_SAMPLE_CFLAGS += -DHI_LOG_SUPPORT=1
endif

ifeq ($(CFG_HI_ADVCA_FUNCTION),FINAL)
CFG_HI_SAMPLE_CFLAGS += -DHI_ADVCA_FUNCTION_RELEASE
else
CFG_HI_SAMPLE_CFLAGS += -DHI_ADVCA_FUNCTION_$(CFG_HI_ADVCA_FUNCTION)
endif

SYS_LIBS := -lpthread -lrt -lm -ldl

ifeq ($(CFG_HI_LIBCPP_SUPPORT),y)
SYS_LIBS += -lstdc++
endif

HI_LIBS := -lhi_memory -lhi_common -lfreetype -lhi_securec -lhi_dispmng

ifeq ($(CFG_HI_HIGO_SUPPORT),y)
HI_LIBS += -lhigo -lhigoadp
endif

ifneq ($(CFG_HI_LOADER_APPLOADER),y)
HI_LIBS += -lpng -lz -ljpeg
endif

HI_LIBS += -lhi_msp

ifeq ($(CFG_HI_TEE_SUPPORT),y)
HI_LIBS += -lteec
endif

#COMMON_SRCS :=  hi_adp_demux.c \
                hi_adp_data.c \
                hi_adp_mpi.c \
                hi_adp_search.c \
                hi_adp_ini.c \
                hi_filter.c

COMMON_SRCS :=  hi_adp_demux.c \
                hi_adp_data.c \
                hi_adp_search.c \
                hi_adp_ini.c \
                hi_adp_mpi.c \
                hi_filter.c

ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
COMMON_SRCS += hi_adp_frontend.c
endif

ifeq ($(CFG_HI_PVR_SUPPORT),y)
ifeq ($(CFG_HI_PVR_SIGNAL_FILE_STORAGE),y)
CFG_HI_SAMPLE_CFLAGS += -DHI_PVR_SIGNAL_FILE_STORAGE_SUPPORT
endif
COMMON_SRCS += hi_adp_pvr.c
endif

AT := 
