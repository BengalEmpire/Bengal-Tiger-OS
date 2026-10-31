# ============================================================================
# Bengal Tiger OS v1.0.0 - Build & Verification System
# ============================================================================

CC        = gcc
AS        = as
LD        = ld
GRUB_MK   = grub-mkrescue
QEMU      = qemu-system-i386
PYTHON    = python3

CFLAGS    = -m32 -ffreestanding -O2 -Wall -Wextra -Werror \
            -fno-pie -fno-stack-protector -fno-builtin \
            -nostdlib -nostdinc -Ikernel
ASFLAGS   = --32
LDFLAGS   = -m elf_i386 -T linker.ld -nostdlib

BUILD_DIR = build
ISO_DIR   = isodir
KERNEL    = $(BUILD_DIR)/kernel.bin
ISO       = bengaltiger.iso

BOOT_OBJ  = $(BUILD_DIR)/boot.o
ISR_OBJ   = $(BUILD_DIR)/isr_asm.o

C_SOURCES = \
	kernel/main.c \
	kernel/common.c \
	kernel/klog.c \
	kernel/serial.c \
	kernel/panic.c \
	kernel/cpu.c \
	kernel/gdt.c \
	kernel/idt.c \
	kernel/isr.c \
	kernel/pic.c \
	kernel/timer.c \
	kernel/rtc.c \
	kernel/paging.c \
	kernel/heap.c \
	kernel/vbe.c \
	kernel/font.c \
	kernel/keyboard.c \
	kernel/mouse.c \
	kernel/disk.c \
	kernel/fat.c \
	kernel/pci.c \
	kernel/nic.c \
	kernel/net.c \
	kernel/scheduler.c \
	kernel/editor.c \
	kernel/shell.c

C_OBJS   = $(patsubst kernel/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ALL_OBJS = $(BOOT_OBJ) $(ISR_OBJ) $(C_OBJS)

.PHONY: all kernel iso run run-serial run-disk check-multiboot test-unit test-qemu test clean

all: $(KERNEL)
	@if command -v $(GRUB_MK) >/dev/null 2>&1; then \
		$(MAKE) --no-print-directory iso; \
	else \
		echo "[INFO] Built $(KERNEL) (install grub-pc-bin + xorriso to build $(ISO))"; \
	fi

kernel: $(KERNEL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BOOT_OBJ): boot/boot.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(ISR_OBJ): kernel/isr.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: kernel/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(ALL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)

iso: $(KERNEL)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_MK) -o $(ISO) $(ISO_DIR)

check-multiboot: $(KERNEL)
	$(PYTHON) tests/verify_kernel_binary.py $(KERNEL)

test-unit: $(BUILD_DIR)
	$(CC) -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
		-DHOST_TEST -Ikernel \
		tests/test_host.c \
		kernel/common.c \
		kernel/klog.c \
		kernel/heap.c \
		kernel/fat.c \
		kernel/net.c \
		-o $(BUILD_DIR)/test_host
	./$(BUILD_DIR)/test_host

test-qemu: $(ISO)
	$(PYTHON) tests/qemu_smoke_test.py $(ISO)

test: check-multiboot test-unit

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -m 64M -netdev user,id=n0 -device e1000,netdev=n0 -serial stdio

run-serial: $(ISO)
	$(QEMU) -cdrom $(ISO) -m 64M -netdev user,id=n0 -device e1000,netdev=n0 -nographic

run-disk: $(ISO)
	@test -f disk.img || dd if=/dev/zero of=disk.img bs=1M count=32
	$(QEMU) -cdrom $(ISO) -hda disk.img -boot d -m 64M -netdev user,id=n0 -device e1000,netdev=n0 -serial stdio

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO) SHA256SUMS.txt
