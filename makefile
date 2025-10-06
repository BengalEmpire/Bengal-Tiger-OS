CC = gcc
LD = ld
AS = gcc
CFLAGS = -m32 -fno-pic -fno-builtin -nostdlib -nostartfiles -nodefaultlibs -Wall -Wextra -g
ASFLAGS = -m32 -g -c
LDFLAGS = -m elf_i386

OBJS = build/boot.o build/kernel.o build/common.o build/idt.o build/isr.o build/pic.o build/keyboard.o build/paging.o build/disk.o build/fat.o build/scheduler.o build/shell.o build/pci.o build/nic.o

.PHONY: all clean iso run

all: build bengaltiger.iso

build:
	mkdir -p build
	mkdir -p iso/boot/grub

# Assemble boot.s
build/boot.o: boot/boot.s
	$(AS) $(ASFLAGS) -o $@ $<

# Compile kernel files
build/kernel.o: kernel/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/common.o: kernel/common.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/idt.o: kernel/idt.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/isr.o: kernel/isr.s
	$(AS) $(ASFLAGS) -o $@ $<

build/pic.o: kernel/pic.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/paging.o: kernel/paging.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/disk.o: kernel/disk.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/fat.o: kernel/fat.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/scheduler.o: kernel/scheduler.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/shell.o: kernel/shell.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/pci.o: kernel/pci.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/nic.o: kernel/nic.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Link kernel
build/kernel.bin: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

# Copy to ISO dir
iso: build/kernel.bin
	cp build/kernel.bin iso/boot/kernel.bin
	cp grub/grub.cfg iso/boot/grub/grub.cfg
	# Add sample file for FAT test
	echo "Hello from file!" > iso/boot/hello.txt

# Create ISO
bengaltiger.iso: iso
	grub-mkrescue -o bengaltiger.iso iso

run: bengaltiger.iso
	qemu-system-i386 -cdrom bengaltiger.iso -m 512M

clean:
	rm -rf build iso bengaltiger.iso