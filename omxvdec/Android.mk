# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ---------------------------------------------------------------------------------
#			Make the apps-test (mm-vdec-omx-test)
# ---------------------------------------------------------------------------------
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

#LOCAL_MODULE_TAGS		:= eng
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE			:= sample_omxvdec

ifeq (${CFG_HI_CPU_ARCH},arm64)
LOCAL_32_BIT_ONLY := true
else
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)
endif

LOCAL_CFLAGS			:= -pthread -ldl -O2
LOCAL_C_INCLUDES		:= $(USR_DIR)/omx/include
LOCAL_C_INCLUDES		+= $(USR_DIR)/common/include 
LOCAL_C_INCLUDES                += $(USR_DIR)/include
LOCAL_C_INCLUDES                += $(DRV_DIR)/include
LOCAL_C_INCLUDES                += $(DRV_DIR)/common/include
LOCAL_C_INCLUDES                += $(USR_DIR)/securec
LOCAL_SHARED_LIBRARIES  := libutils libOMX_Core libbinder libhi_common libcutils liblog libhi_memory
LOCAL_SRC_FILES                 := sample_omxvdec.c sample_queue.c sample_tidx.c

ifeq ($(CFG_HI_SMMU_SUPPORT),y)
LOCAL_CFLAGS += -DHI_SMMU_SUPPORT
endif

ifeq ($(findstring $(CFG_HI_CHIP_TYPE), hi3798hv100 hi3798cv200),)
LOCAL_CFLAGS += -DSUPPORT_CA2TA
endif


ifeq ($(CFG_HI_TVP_SUPPORT), y)
LOCAL_CFLAGS += -DHI_TEE_SUPPORT
LOCAL_CFLAGS += -DHI_OMX_TEE_SUPPORT
LOCAL_CFLAGS += -DHI_TVP_SUPPORT
LOCAL_C_INCLUDES += $(HI_TEE_OS_DIR)/libteec/inc/
LOCAL_SHARED_LIBRARIES += libteec libc_sec
endif


include $(BUILD_EXECUTABLE)
