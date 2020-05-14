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

SAMPLE_IMAGES :=  sample_demux_ts_mpi \
                  sample_demux_es_mpi \
                  sample_demux_sec_mpi \
                  sample_demux_pes_mpi \
                  sample_demux_record_mpi \
                  sample_dsc_clearkey_mpi \
                  sample_dsc_clearkey_unf \
                  sample_demux_ts_unf \
                  sample_demux_es_unf \
                  sample_demux_sec_unf \
                  sample_demux_pes_unf \
                  sample_demux_record_unf \
                  sample_demux_es_perf \
                  sample_demux_ts_pidcopy \
                  sample_demux_record_play_mpi \
                  sample_demux_record_play_unf \
                  sample_demux_record_index_unf \
                  sample_demux_record_clearkey_unf \
                  sample_demux_play_with_index \
                  sample_demux_tsio \
                  sample_demux_secure_es_unf \
                  sample_demux_secure_ram_ts_mpi
ifeq ($(CFG_HI_FRONTEND_SUPPORT),y)
SAMPLE_IMAGES +=  sample_demux_record_alldata
endif

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_common -lhi_memory -lhi_securec -lhi_msp

include $(SAMPLE_DIR)/hi_sample_rules.mak
