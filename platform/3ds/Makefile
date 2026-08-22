ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),check-host)
    ifeq ($(strip $(DEVKITARM)),)
      $(error "DEVKITARM is not set. Install devkitPro 3ds-dev and export DEVKITPRO and DEVKITARM")
    endif
    include $(DEVKITARM)/3ds_rules
  endif
endif

TARGET := pokeplatinum-real-port
BUILD := build-3ds-real
SOURCES := platform/3ds src
INCLUDES := platform/3ds/include platform/3ds include
HOSTCC ?= cc

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
COMMON_FLAGS := -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
	-fsigned-char -std=c11 -include pokeplatinum_compat.h
CFLAGS := -g -O2 -mword-relocations $(ARCH) $(COMMON_FLAGS) $(INCLUDE)
LDFLAGS := -specs=3dsx.specs -g $(ARCH) \
	-Wl,--gc-sections,--no-enum-size-warning,-Map,$(notdir $*.map)
LIBS := -lctru -lm
LIBDIRS := $(CTRULIB)

GAME_CFILES := sys_task_manager.c map_tile_behavior.c
PLATFORM_CFILES := main_3ds.c real_port_smoke.c

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(BUILD)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
export APP_TITLE := Pokemon Platinum Real Port
export APP_DESCRIPTION := Original game code portability bootstrap
export APP_AUTHOR := pret contributors
export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS := --smdh=$(OUTPUT).smdh
export OFILES := $(GAME_CFILES:.c=.o) $(PLATFORM_CFILES:.c=.o)
export LD := $(CC)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all check-host clean

all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/platform/3ds/Makefile

check-host:
	@mkdir -p $(BUILD)
	$(HOSTCC) $(COMMON_FLAGS) $(foreach dir,$(INCLUDES),-I$(dir)) \
		-fno-short-enums \
		$(addprefix src/,$(GAME_CFILES)) \
		platform/3ds/real_port_smoke.c platform/3ds/assert_host.c \
		platform/3ds/smoke_host_main.c -o $(BUILD)/real-port-smoke
	$(BUILD)/real-port-smoke

clean:
	@rm -rf $(BUILD)

else

DEPENDS := $(OFILES:.o=.d)
ORIGINAL_ABI_OFILES := $(GAME_CFILES:.c=.o) real_port_smoke.o

$(ORIGINAL_ABI_OFILES): CFLAGS += -fno-short-enums

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
