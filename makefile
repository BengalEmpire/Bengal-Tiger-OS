# Bengal Tiger OS - Makefile
# 
# Build system for Bengal Tiger OS kernel
# Supports: make all, make clean, make run, make iso
#
# Author: Bengal Tiger OS Team
# Version: 0.3.0

# ==============================================================================
# TOOLCHAIN CONFIGURATION
# ==============================================================================

CC      = gcc
LD      = ld
AS      = gcc
NASM    = nasm

# Compiler flags for 32-bit freestanding kernel
CFLAGS  = -m32 \
          -I./kernel \
          -fno-pic \
          -fno-builtin \
          -nostdlib \
          -nostartfiles \
          -nodefaultlibs \
          -fno-stack-protector \
          -Wall \
          -Wextra \
          -Werror=implicit-function-declaration \
          -O2 \
          -g

# Assembler flags
ASFLAGS = -m32 -g -c

# Linker flags for i386 ELF
LDFLAGS = -m elf_i386 --no-warn-rwx-segments

# ==============================================================================
# SOURCE FILES
# ==============================================================================

# Assembly sources
ASM_SOURCES = boot/boot.s \
              kernel/isr.s

# C sources - Core
C_SOURCES = kernel/main.c \
            kernel/common.c \
            kernel/idt.c \
            kernel/isr.c \
            kernel/pic.c

# C sources - Memory Management
C_SOURCES += kernel/paging.c \
             kernel/heap.c

# C sources - Drivers
C_SOURCES += kernel/keyboard.c \
             kernel/timer.c \
             kernel/disk.c \
             kernel/fat.c \
             kernel/pci.c \
             kernel/nic.c

# C sources - CPU / System
C_SOURCES += kernel/cpu.c \
             kernel/gdt.c

# C sources - Drivers
C_SOURCES += kernel/rtc.c \
             kernel/serial.c \
             kernel/mouse.c

# C sources - Graphics / Font
C_SOURCES += kernel/vbe.c \
             kernel/font.c

# C sources - Subsystems
C_SOURCES += kernel/scheduler.c \
             kernel/shell.c \
             kernel/panic.c

# ==============================================================================
# OBJECT FILES
# ==============================================================================

# Generate object file names from sources
ASM_OBJECTS = $(patsubst %.s,build/%.o,$(notdir $(ASM_SOURCES)))
C_OBJECTS   = $(patsubst %.c,build/%.o,$(notdir $(C_SOURCES)))
OBJECTS     = $(ASM_OBJECTS) $(C_OBJECTS)

# Object files in order for linking
OBJS = build/boot.o \
       build/main.o \
       build/common.o \
       build/idt.o \
       build/isr.o \
       build/isr_c.o \
       build/pic.o \
       build/paging.o \
       build/heap.o \
       build/cpu.o \
       build/gdt.o \
       build/rtc.o \
       build/serial.o \
       build/mouse.o \
       build/keyboard.o \
       build/timer.o \
       build/disk.o \
       build/fat.o \
       build/vbe.o \
       build/font.o \
       build/scheduler.o \
       build/shell.o \
       build/pci.o \
       build/nic.o \
       build/panic.o

# ==============================================================================
# PHONY TARGETS
# ==============================================================================

.PHONY: all clean build iso run debug help

# ==============================================================================
# BUILD TARGETS
# ==============================================================================

all: build bengaltiger.iso
	@echo ""
	@echo "============================================"
	@echo "  Bengal Tiger OS build complete!"
	@echo "  ISO: bengaltiger.iso"
	@echo "  Run: make run"
	@echo "============================================"

build:
	@mkdir -p build
	@mkdir -p iso/boot/grub

help:
	@echo "Bengal Tiger OS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the kernel and create ISO"
	@echo "  clean   - Remove all build artifacts"
	@echo "  run     - Run in QEMU"
	@echo "  debug   - Run in QEMU with GDB server"
	@echo "  iso     - Create bootable ISO"
	@echo "  help    - Show this help message"

# ==============================================================================
# COMPILATION RULES
# ==============================================================================

# Boot assembly
build/boot.o: boot/boot.s
	$(AS) $(ASFLAGS) -o $@ $<

# ISR assembly
build/isr.o: kernel/isr.s
	$(AS) $(ASFLAGS) -o $@ $<

# Main kernel
build/main.o: kernel/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Common utilities
build/common.o: kernel/common.c
	$(CC) $(CFLAGS) -c -o $@ $<

# IDT
build/idt.o: kernel/idt.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ISR C handlers (named isr_c.o to avoid conflict with isr.o)
build/isr_c.o: kernel/isr.c
	$(CC) $(CFLAGS) -c -o $@ $<

# PIC
build/pic.o: kernel/pic.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Keyboard driver
build/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Timer driver
build/timer.o: kernel/timer.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Paging
build/paging.o: kernel/paging.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Heap allocator
build/heap.o: kernel/heap.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Disk driver
build/disk.o: kernel/disk.c
	$(CC) $(CFLAGS) -c -o $@ $<

# FAT filesystem
build/fat.o: kernel/fat.c
	$(CC) $(CFLAGS) -c -o $@ $<

# CPU init
build/cpu.o: kernel/cpu.c
	$(CC) $(CFLAGS) -c -o $@ $<

# GDT
build/gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c -o $@ $<

# RTC
build/rtc.o: kernel/rtc.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Serial
build/serial.o: kernel/serial.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Mouse
build/mouse.o: kernel/mouse.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Scheduler
build/scheduler.o: kernel/scheduler.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Shell
build/shell.o: kernel/shell.c
	$(CC) $(CFLAGS) -c -o $@ $<

# PCI bus
build/pci.o: kernel/pci.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Network driver
build/nic.o: kernel/nic.c
	$(CC) $(CFLAGS) -c -o $@ $<

# VBE framebuffer
build/vbe.o: kernel/vbe.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Font data
build/font.o: kernel/font.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Panic handler
build/panic.o: kernel/panic.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ==============================================================================
# LINKING
# ==============================================================================

build/kernel.bin: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)
	@echo "Kernel size: $$(stat -c %s $@ 2>/dev/null || stat -f %z $@) bytes"

# ==============================================================================
# ISO CREATION
# ==============================================================================

iso: build/kernel.bin build
	@cp build/kernel.bin iso/boot/kernel.bin
	@cp grub/grub.cfg iso/boot/grub/grub.cfg
	# Create config file for first boot detection
	@dd if=/dev/zero of=iso/boot/config.cfg bs=512 count=1 2>/dev/null
	@echo "Welcome to Bengal Tiger OS filesystem!" > iso/boot/hello.txt

bengaltiger.iso: iso
	@echo "Creating bootable ISO..."
	@if command -v grub-mkrescue >/dev/null 2>&1 && command -v mformat >/dev/null 2>&1; then \
		grub-mkrescue -o bengaltiger.iso iso; \
	elif command -v xorriso >/dev/null 2>&1; then \
		xorriso -as mkisofs -o bengaltiger.iso -c boot.cat \
			-b boot/grub/i386-pc/eltorito.img -no-emul-boot \
			-boot-load-size 4 -boot-info-table iso; \
	else \
		echo "WARNING: grub-mkrescue (with mtools) or xorriso not found. ISO not created."; \
		echo "Kernel binary is available at build/kernel.bin"; \
	fi

# ==============================================================================
# RUNNING / DEBUGGING
# ==============================================================================

# Run in QEMU with FAT disk emulation for persistence
run: bengaltiger.iso
	qemu-system-i386 \
		-cdrom bengaltiger.iso \
		-m 512M \
		-drive file=fat:rw:iso/boot,format=raw \
		-boot d \
		-vga std

# Run with debug server enabled
debug: bengaltiger.iso
	qemu-system-i386 \
		-cdrom bengaltiger.iso \
		-m 512M \
		-drive file=fat:rw:iso/boot,format=raw \
		-boot d \
		-s -S \
		-vga std

# ==============================================================================
# CLEANUP
# ==============================================================================

clean:
	rm -rf build
	rm -rf iso
	rm -f bengaltiger.iso
	@echo "Cleaned all build artifacts."