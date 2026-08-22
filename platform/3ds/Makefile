ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),host-check)
    ifeq ($(strip $(DEVKITARM)),)
      $(error "DEVKITARM is not set. Install devkitPro 3ds-dev and export DEVKITPRO and DEVKITARM")
    endif
    include $(DEVKITARM)/3ds_rules
  endif
endif

TARGET := pokeplatinum-3ds
HOST_CC ?= cc
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
GENERATED_EVO_NARC := build/res/pokemon/evo.narc
GENERATED_ICON_NARC := build/res/pokemon/pl_poke_icon.narc
HOST_RUNTIME_SMOKE := build-3ds-host/game_runtime_smoke
HOST_TASK_SMOKE := build-3ds-host/game_task_smoke
HOST_APPLICATION_SMOKE := build-3ds-host/game_application_smoke
HOST_CLOCK_SMOKE := build-3ds-host/game_clock_smoke
HOST_MEMORY_SMOKE := build-3ds-host/memory_smoke

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -Wextra -Werror -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS += $(INCLUDE) -D__3DS__ -std=c11 -DPORT3DS_DEBUG=$(DEBUG) -DPORT3DS_DEBUG_OVERLAY=$(DEBUG)
CFLAGS += -DPORT3DS_FATAL_SMOKE=$(FATAL_SMOKE)
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lcitro2d -lcitro3d -lctru -lm
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

CFILES := main_3ds.c platform_3ds.c input_3ds.c filesystem_3ds.c debug_3ds.c time_3ds.c game_clock_3ds.c save_3ds.c graphics_3ds.c memory.c archive.c nitro2d.c game_runtime.c game_task.c game_application.c game_clock.c
export OFILES := $(CFILES:.c=.o)
export LD := $(CC)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean host-application-smoke host-check host-clock-smoke host-memory-smoke host-runtime-smoke host-task-smoke

all: host-check
	@mkdir -p $(BUILD) build
	@ninja -C build res/pokemon/evo.narc res/pokemon/pl_poke_icon.narc
	@rm -rf $(ROMFS)
	@mkdir -p $(ROMFS)/generated
	@cp -R $(STATIC_ROMFS)/. $(ROMFS)
	@cp $(GENERATED_EVO_NARC) $(ROMFS)/generated/evo.narc
	@cp $(GENERATED_ICON_NARC) $(ROMFS)/generated/pl_poke_icon.narc
	@rm -f build/$(TARGET).elf build/$(TARGET).3dsx
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/platform/3ds/Makefile

host-runtime-smoke:
	@mkdir -p $(dir $(HOST_RUNTIME_SMOKE))
	@$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iplatform/include \
		platform/common/game_runtime.c platform/tests/game_runtime_smoke.c \
		-o $(HOST_RUNTIME_SMOKE)
	@$(HOST_RUNTIME_SMOKE)

host-task-smoke:
	@mkdir -p $(dir $(HOST_TASK_SMOKE))
	@$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iplatform/include \
		platform/common/game_task.c platform/tests/game_task_smoke.c \
		-o $(HOST_TASK_SMOKE)
	@$(HOST_TASK_SMOKE)

host-application-smoke:
	@mkdir -p $(dir $(HOST_APPLICATION_SMOKE))
	@$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iplatform/include \
		platform/common/game_application.c platform/tests/game_application_smoke.c \
		-o $(HOST_APPLICATION_SMOKE)
	@$(HOST_APPLICATION_SMOKE)

host-clock-smoke:
	@mkdir -p $(dir $(HOST_CLOCK_SMOKE))
	@$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iplatform/include \
		platform/common/game_clock.c platform/tests/game_clock_smoke.c \
		-o $(HOST_CLOCK_SMOKE)
	@$(HOST_CLOCK_SMOKE)

host-memory-smoke:
	@mkdir -p $(dir $(HOST_MEMORY_SMOKE))
	@$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iplatform/include \
		platform/common/memory.c platform/tests/memory_smoke.c \
		-o $(HOST_MEMORY_SMOKE)
	@$(HOST_MEMORY_SMOKE)

host-check: host-runtime-smoke host-task-smoke host-application-smoke host-clock-smoke host-memory-smoke

clean:
	@rm -rf build-3ds-debug build-3ds-debug-fatal build-3ds-release \
		build-3ds-host \
		build/$(TARGET).elf build/$(TARGET).3dsx build/$(TARGET).smdh build/$(TARGET).map

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
