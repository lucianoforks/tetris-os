.PHONY: all dirs clean proper iso

UNAME=$(shell uname | tr a-z A-Z)
ifneq ($(findstring LINUX, $(UNAME)),)
  BUILD_OS=LINUX
endif
ifneq ($(findstring DARWIN, $(UNAME)),)
  BUILD_OS=MACOSX
endif
ifeq ($(BUILD_OS),)
  $(error $(UNAME) support has not yet been coded into this Makefile)
endif


ifeq ($(BUILD_OS),MACOSX)
  CC=i386-elf-gcc
  ASM=i386-elf-as
  LD=i386-elf-ld
  ASFLAGS=
  LDFLAGS=
  BSCTFLAGS=
endif
ifeq ($(BUILD_OS),LINUX)
  CC=gcc
  ASM=as
  LD=ld
  ASFLAGS=--32
  LDFLAGS=-m elf_i386 -s
  BSCTFLAGS:=$(LDFLAGS)
endif

GFLAGS=
CCFLAGS=-m32 -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
CCFLAGS+=-Wno-pointer-arith -Wno-unused-parameter
CCFLAGS+=-nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector
CCFLAGS+=-fno-builtin-function -fno-builtin

BOOTSECT_SRCS=\
	src/stage0.S

BOOTSECT_OBJS=$(BOOTSECT_SRCS:.S=.o)

KERNEL_C_SRCS=$(wildcard src/*.c)
KERNEL_S_SRCS=$(filter-out $(BOOTSECT_SRCS), $(wildcard src/*.S))
KERNEL_OBJS=$(KERNEL_C_SRCS:.c=.o) $(KERNEL_S_SRCS:.S=.o)

BOOTSECT=bootsect.bin
KERNEL=kernel.bin
ISO=boot.iso

all: iso
iso: dirs $(ISO)
proper: clean all

dirs:
	mkdir -p bin

clean:
	rm -f ./**/*.o
	rm -f ./*.iso
	rm -f ./**/*.elf
	rm -f ./**/*.bin
	rm -f $(BOOTSECT) $(KERNEL)

%.o: %.c
	$(CC) -o $@ -c $< $(GFLAGS) $(CCFLAGS)

%.o: %.S
	$(ASM) -o $@ -c $< $(GFLAGS) $(ASFLAGS)

$(BOOTSECT): $(BOOTSECT_OBJS)
	$(LD) -o ./bin/$(BOOTSECT) $^ $(BSCTFLAGS) -Ttext 0x7C00 --oformat=binary

$(KERNEL): $(KERNEL_OBJS)
	$(LD) -o ./bin/$(KERNEL) $^ $(LDFLAGS) -Tsrc/link.ld

$(ISO): $(BOOTSECT) $(KERNEL)
	dd if=/dev/zero of=boot.iso bs=512 count=2880
	dd if=./bin/$(BOOTSECT) of=boot.iso conv=notrunc bs=512 seek=0 count=1
	dd if=./bin/$(KERNEL) of=boot.iso conv=notrunc bs=512 seek=1 count=2048
