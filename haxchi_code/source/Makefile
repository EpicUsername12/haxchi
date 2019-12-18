PATH := $(DEVKITPPC)/bin:$(PATH)
PREFIX ?= powerpc-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc
CFLAGS = -std=gnu99 -O3 -nostdinc -fno-builtin
ASFLAGS = -mregnames -x assembler-with-cpp
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
LDFLAGS=-Ttext 1800000 -L$(DEVKITPPC)/lib/gcc/powerpc-eabi/8.3.0
OBJDUMP ?= $(PREFIX)objdump
project	:=	.
root	:=	$(CURDIR)
build	:=	 $(root)/bin

sd_loader_elf := sd_loader/sd_loader.elf

CFLAGS += -DUSE_SD_LOADER
ASFLAGS += -DUSE_SD_LOADER
FIRMWARE = 550

all: clean setup sd_loader.h

sd_loader.h: $(sd_loader_elf)
	xxd -i $< | sed "s/unsigned/static const unsigned/g;s/loader/loader/g;s/build_//g" > $@

$(sd_loader_elf):
	make -C sd_loader

setup:
	mkdir -p $(root)/bin/

clean:
	rm -rf $(build)
	make clean -C sd_loader