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
          -I$(SAMPLE_DIR)/common \
          -I$(SDK_DIR)/source/linux/api/securec


SAMPLE_IMAGES := sample_display_mode sample_tsplay_multiscreen sample_mono_display sample_dual_display

COMMON_OBJS := $(COMMON_SRCS:%.c=%.o)

DEPEND_LIBS := -lhi_msp -lhi_memory -lhi_common -lhi_securec -lhi_dispmng

#===============================================================================
# sample rules
#===============================================================================
SRC_DIR := $(CURDIR)

OBJ_DIR := $(HI_OUT_DIR)/obj/$(subst $(SDK_DIR)/,,$(SRC_DIR))
OBJ64_DIR := $(HI_OUT_DIR)/obj64/$(subst $(SDK_DIR)/,,$(SRC_DIR))

COMMON_OBJ_DIR := $(HI_OUT_DIR)/obj/$(subst $(SDK_DIR)/,,$(SAMPLE_DIR)/common)
COMMON_OBJ64_DIR := $(HI_OUT_DIR)/obj64/$(subst $(SDK_DIR)/,,$(SAMPLE_DIR)/common)

vpath %.c $(SAMPLE_DIR)/common

SAMPLE_RES := $(HI_OUT_DIR)/obj/sample
SAMPLE_RES64 := $(HI_OUT_DIR)/obj64/sample

ifeq ($(HI_USER_SPACE_LIB),y)
HI_SAMPLE_IMAGES := $(addprefix $(OBJ_DIR)/,$(SAMPLE_IMAGES))
HI_COMMON_OBJS := $(addprefix $(COMMON_OBJ_DIR)/,$(COMMON_OBJS))
endif

ifeq ($(HI_USER_SPACE_LIB64),y)
HI_SAMPLE_IMAGES64 := $(addprefix $(OBJ64_DIR)/,$(SAMPLE_IMAGES))
HI_COMMON_OBJS64 := $(addprefix $(COMMON_OBJ64_DIR)/,$(COMMON_OBJS))
endif

CFLAGS  +=  $(CFG_HI_SAMPLE_CFLAGS)

CFLAGS  +=  -I$(HI_INCLUDE_DIR) \
            -I$(SAMPLE_DIR)/common

HI_LIB_PATHS := -L$(HI_SHARED_LIB_DIR) -L$(HI_EXTERN_LIB_DIR) -L$(HI_STATIC_LIB_DIR) $(LIB_PATHS)
HI_LIB64_PATHS := -L$(HI_SHARED_LIB64_DIR) -L$(HI_EXTERN_LIB64_DIR) -L$(HI_STATIC_LIB64_DIR) $(LIB64_PATHS)

HI_DEPEND_LIBS := -Wl,--start-group $(SYS_LIBS) $(DEPEND_LIBS) $(HI_LIBS) -Wl,--end-group

TARGETS :=

ifeq ($(HI_USER_SPACE_LIB),y)
TARGETS += $(if $(HI_SAMPLE_IMAGES),$(HI_SAMPLE_IMAGES),$(HI_COMMON_OBJS))
endif

ifeq ($(HI_USER_SPACE_LIB64),y)
TARGETS += $(if $(HI_SAMPLE_IMAGES64),$(HI_SAMPLE_IMAGES64),$(HI_COMMON_OBJS64))
endif

ifneq ($(CFG_HI_SAMPLE_STATIC_SUPPORT),y)
STATIC_FLAG :=
else
STATIC_FLAG := -static
endif

#===============================================================================
# rules
#===============================================================================
.PHONY: all clean prepare

all: prepare $(TARGETS)

clean:
	$(AT)rm -rf $(OBJ_DIR) $(OBJ64_DIR) $(HI_COMMON_OBJS) $(HI_COMMON_OBJS64)
	$(AT)rm -rf $(HI_SAMPLE_IMAGES) $(HI_SAMPLE_IMAGES64)

$(HI_SAMPLE_IMAGES) : % : $(HI_COMMON_OBJS) $(addprefix $(OBJ_DIR)/,$(LOCAL_OBJS)) %.o
	$(AT)$(CFG_HI_ARM_TOOLCHAINS_NAME)-gcc -o $@ $^ $(STATIC_FLAG) $(HI_LIB_PATHS) $(HI_DEPEND_LIBS)

$(HI_SAMPLE_IMAGES64) : % : $(HI_COMMON_OBJS64) $(addprefix $(OBJ64_DIR)/,$(LOCAL_OBJS)) %.o
	$(AT)$(CFG_HI_AARCH64_TOOLCHAINS_NAME)-gcc -o $@ $^ $(STATIC_FLAG) $(HI_LIB64_PATHS) $(HI_DEPEND_LIBS)

prepare:
ifeq ($(HI_USER_SPACE_LIB),y)
	$(AT)test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	$(AT)test -e $(OBJ_DIR)/picture01.dat || cp -f $(SRC_DIR)/picture0*.dat $(OBJ_DIR)/
endif
ifeq ($(HI_USER_SPACE_LIB64),y)
	$(AT)test -d $(OBJ64_DIR) || mkdir -p $(OBJ64_DIR)
	$(AT)test -e $(OBJ64_DIR)/picture01.dat || cp -f $(SRC_DIR)/picture0*.dat $(OBJ64_DIR)/
endif

$(COMMON_OBJ_DIR)/%.o : $(SAMPLE_DIR)/common/%.c
	$(AT)test -d $(dir $@) || mkdir -p $(dir $@)
	$(AT)$(CFG_HI_ARM_TOOLCHAINS_NAME)-gcc $(CFLAGS) $(FLOAT_OPTIONS) -o $@ -c $^

$(OBJ_DIR)/%.o : %.c
	$(AT)test -d $(dir $@) || mkdir -p $(dir $@)
	$(AT)$(CFG_HI_ARM_TOOLCHAINS_NAME)-gcc $(CFLAGS) $(FLOAT_OPTIONS) -o $@ -c $^

$(COMMON_OBJ64_DIR)/%.o : $(SAMPLE_DIR)/common/%.c
	$(AT)test -d $(dir $@) || mkdir -p $(dir $@)
	$(AT)$(CFG_HI_AARCH64_TOOLCHAINS_NAME)-gcc $(CFLAGS) -o $@ -c $^

$(OBJ64_DIR)/%.o : %.c
	$(AT)test -d $(dir $@) || mkdir -p $(dir $@)
	$(AT)$(CFG_HI_AARCH64_TOOLCHAINS_NAME)-gcc $(CFLAGS) -o $@ -c $^


