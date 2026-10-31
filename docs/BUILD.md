# Build, Run & Verification Guide

## Prerequisites (Debian / Ubuntu / WSL2)

```bash
sudo apt-get update
sudo apt-get install -y \
  gcc-multilib \
  binutils \
  grub-pc-bin \
  grub-common \
  xorriso \
  mtools \
  qemu-system-x86 \
  python3 \
  make
```

## Make Targets

| Command | Description |
| :--- | :--- |
| `make all` | Compiles `build/kernel.bin` (and builds `bengaltiger.iso` if `grub-mkrescue` is installed) |
| `make kernel` | Compiles `build/kernel.bin` only (zero warnings enforced via `-Wall -Wextra -Werror`) |
| `make iso` | Packages `build/kernel.bin` + `grub/grub.cfg` into bootable `bengaltiger.iso` |
| `make check-multiboot` | Verifies ELF32 structure, Multiboot1 header/checksum, stack alignment & section layout |
| `make test-unit` | Compiles & runs host ASan/UBSan test suite (`tests/test_host.c`) for heap, klog, FAT12/16/MBR, and TCP/IP/DHCP |
| `make test` | Runs `check-multiboot` + `test-unit` |
| `make test-qemu` | Runs automated two-boot QEMU E2E test (`tests/qemu_smoke_test.py`) testing DHCP, ICMP ping & persistent disk across reboot |
| `make run` | Boots `bengaltiger.iso` in QEMU with 64 MB RAM, Intel E1000 user networking & serial stdio |
| `make run-serial` | Boots `bengaltiger.iso` in headless terminal mode (`-nographic`) |
| `make run-disk` | Creates a 32 MB `disk.img` (if absent) and boots QEMU with persistent IDE hard disk attached |
| `make clean` | Removes `build/`, `isodir/`, `bengaltiger.iso`, and `SHA256SUMS.txt` |

## Running on Real Hardware

Write `bengaltiger.iso` directly to a USB flash drive (replace `/dev/sdX` carefully with your USB block device):

```bash
sudo dd if=bengaltiger.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

Boot the machine in Legacy BIOS / CSM mode and select **Bengal Tiger OS v1.0.0** from the GRUB menu.
