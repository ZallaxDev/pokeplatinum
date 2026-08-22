ifneq ($(MAKECMDGOALS),clean)
  ifeq ($(strip $(DEVKITARM)),)
    $(error "DEVKITARM is not set. Install devkitPro 3ds-dev and export DEVKITPRO and DEVKITARM")
  endif
  include $(DEVKITARM)/3ds_rules
endif

TARGET := pokeplatinum-3ds
FATAL_SMOKE ?= 0
ifeq ($(FATAL_SMOKE),1)
  ifneq ($(DEBUG),1)
    $(error "FATAL_SMOKE=1 requires DEBUG=1")
  endif
endif
ifeq ($(DEBUG),1)
  ifeq ($(FATAL_SMOKE),1)
    BUILD := build-3ds-debug-fatal
  else
    BUILD := build-3ds-debug
  endif
else
BUILD := build-3ds-release
endif
SOURCES := platform/3ds platform/common
INCLUDES := platform/include
STATIC_ROMFS := platform/3ds/romfs
GENERATED_NARC := build/res/pokemon/evo.narc

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -Wextra -Werror -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS += $(INCLUDE) -D__3DS__ -std=c11 -DPORT3DS_DEBUG=$(DEBUG) -DPORT3DS_DEBUG_OVERLAY=$(DEBUG)
CFLAGS += -DPORT3DS_FATAL_SMOKE=$(FATAL_SMOKE)
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lctru -lm
LIBDIRS := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/build/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
export ROMFS := $(CURDIR)/$(BUILD)/romfs
export APP_TITLE := Pokemon Platinum 3DS
export APP_DESCRIPTION := Native 3DS port bootstrap
export APP_AUTHOR := pret contributors
export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS := --smdh=$(OUTPUT).smdh --romfs=$(ROMFS)

CFILES := main_3ds.c platform_3ds.c input_3ds.c filesystem_3ds.c debug_3ds.c time_3ds.c save_3ds.c memory.c
export OFILES := $(CFILES:.c=.o)
export LD := $(CC)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean

all:
	@mkdir -p $(BUILD) build
	@ninja -C build res/pokemon/evo.narc
	@rm -rf $(ROMFS)
	@mkdir -p $(ROMFS)/generated
	@cp -R $(STATIC_ROMFS)/. $(ROMFS)
	@cp $(GENERATED_NARC) $(ROMFS)/generated/evo.narc
	@rm -f build/$(TARGET).elf build/$(TARGET).3dsx
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/platform/3ds/Makefile

clean:
	@rm -rf build-3ds-debug build-3ds-debug-fatal build-3ds-release \
		build/$(TARGET).elf build/$(TARGET).3dsx build/$(TARGET).smdh build/$(TARGET).map

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
