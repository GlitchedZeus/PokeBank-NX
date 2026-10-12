.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TARGET      := PokeBankReturnHost
BUILD       := build
SOURCES     := source
INCLUDES    :=
NO_ICON     := 1
export APP_TITLE   := PokeBank Return Host
export APP_AUTHOR  := PokeBank NX
export APP_VERSION := 0.1.0

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS      := -g -Wall -O2 -ffunction-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__SWITCH__
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS        := -lnx
LIBDIRS     := $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
export LD   := $(CC)
export OFILES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
# Do not inherit the parent PokeBank NRO's RomFS/NACP flags into this nested helper.
export NROFLAGS := --nacp=$(CURDIR)/$(TARGET).nacp
export NACPFLAGS :=

.PHONY: all clean $(BUILD)
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET).lst $(TARGET).map

else

.PHONY: all
DEPENDS := $(OFILES:.o=.d)
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)

endif
