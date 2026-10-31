#!/usr/bin/env python3
"""
Bengal Tiger OS - ELF32 & Multiboot1 Kernel Binary Integrity Verifier

Validates:
  1. Valid ELF32 i386 executable (ET_EXEC, EM_386, entry >= 0x100000).
  2. Valid Multiboot1 header within the first 8,192 bytes of kernel.bin:
       - Magic == 0x1BADB002 (4-byte aligned)
       - Flags == 0x00000007 (Page-Align + Memory Map + Video Info)
       - Bit 16 (AOUT_KLUDGE) == 0 (uses ELF32 program headers cleanly)
       - Checksum: (magic + flags + checksum) & 0xFFFFFFFF == 0
       - Valid video preference fields (1024x768x32)
  3. Non-overlapping memory layout:
       - 32KB 16-byte-aligned bootstrap stack (stack_top - stack_bottom == 32768)
       - bss_end < 0x00400000 (zero collision with dynamic kernel heap)
       - Zero orphan .rodata.* / .text.* sections
"""

import struct
import subprocess
import sys
from pathlib import Path


def fail(msg: str) -> None:
    print(f"[FAIL] {msg}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    kernel_path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/kernel.bin")
    if not kernel_path.is_file():
        fail(f"Kernel binary not found: {kernel_path}")

    data = kernel_path.read_bytes()
    if len(data) < 4096:
        fail(f"Kernel binary suspiciously small ({len(data)} bytes)")

    # 1. Verify ELF32 i386 Header
    if data[:4] != b"\x7fELF":
        fail("Missing ELF magic header")
    ei_class, ei_data = data[4], data[5]
    if ei_class != 1 or ei_data != 1:
        fail(f"Expected 32-bit little-endian ELF, got class={ei_class}, data={ei_data}")

    e_type, e_machine, _, e_entry = struct.unpack_from("<HHII", data, 16)
    if e_type != 2 or e_machine != 3:
        fail(f"Expected ET_EXEC(2)/EM_386(3), got type={e_type}, machine={e_machine}")
    if e_entry < 0x00100000:
        fail(f"Entry point 0x{e_entry:08X} is below 1MB (0x00100000)")

    # 2. Scan first 8192 bytes for Multiboot1 header
    mb_offset = None
    search_window = data[:8192]
    for off in range(0, len(search_window) - 48, 4):
        magic, flags, checksum = struct.unpack_from("<III", search_window, off)
        if magic == 0x1BADB002 and ((magic + flags + checksum) & 0xFFFFFFFF) == 0:
            mb_offset = off
            break

    if mb_offset is None:
        fail("No valid Multiboot1 header found within first 8,192 bytes")

    magic, flags, checksum = struct.unpack_from("<III", data, mb_offset)
    if flags & (1 << 16):
        fail(f"Multiboot flags 0x{flags:08X} illegally sets AOUT_KLUDGE (bit 16) on ELF32 image")
    if flags != 0x00000007:
        fail(f"Expected Multiboot flags 0x00000007, got 0x{flags:08X}")

    mode_type, width, height, depth = struct.unpack_from("<IIII", data, mb_offset + 32)
    if mode_type != 0 or width != 1024 or height != 768 or depth != 32:
        fail(f"Unexpected Multiboot video header: mode={mode_type} {width}x{height}x{depth}")

    # 3. Verify Symbols & Section Layout via nm / readelf
    nm_out = subprocess.check_output(["nm", "-n", str(kernel_path)], text=True)
    syms = {}
    for line in nm_out.splitlines():
        parts = line.split()
        if len(parts) >= 3:
            syms[parts[2]] = int(parts[0], 16)

    for req in ("start", "kmain", "stack_bottom", "stack_top", "bss_end", "isr_common"):
        if req not in syms:
            fail(f"Required kernel symbol '{req}' missing from binary")

    stack_size = syms["stack_top"] - syms["stack_bottom"]
    if stack_size < 32768:
        fail(f"Bootstrap stack too small: {stack_size} bytes")
    if syms["stack_top"] % 16 != 0:
        fail(f"stack_top (0x{syms['stack_top']:08X}) is not 16-byte aligned")
    if syms["bss_end"] >= 0x00400000:
        fail(f"bss_end (0x{syms['bss_end']:08X}) collides with HEAP_START (0x00400000)")

    sec_out = subprocess.check_output(["readelf", "-S", str(kernel_path)], text=True)
    for line in sec_out.splitlines():
        if ".rodata." in line or ".text." in line or ".data." in line:
            fail(f"Orphan unmerged compiler section detected: {line.strip()}")

    print(f"[PASS] ELF32 i386 verified: entry=0x{e_entry:08X}, bss_end=0x{syms['bss_end']:08X}")
    print(f"[PASS] Multiboot1 header @ offset 0x{mb_offset:04X}: flags=0x{flags:08X}, {width}x{height}x{depth}")
    print(f"[PASS] Bootstrap stack: 0x{syms['stack_bottom']:08X}..0x{syms['stack_top']:08X} ({stack_size // 1024} KB, 16B-aligned)")


if __name__ == "__main__":
    main()
