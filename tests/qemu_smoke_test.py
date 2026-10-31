#!/usr/bin/env python3
"""
Bengal Tiger OS - End-to-End Two-Boot QEMU Hardware Integration Test

Executes two real x86 virtual machine boots from `bengaltiger.iso` with:
  - 64 MB RAM
  - Raw 16 MB IDE ATA hard disk (`test_disk.img`)
  - Intel PRO/1000 (E1000) PCI Ethernet on QEMU SLIRP user network (`10.0.2.0/24`)
  - Interactive COM1 serial console
"""

import os
import select
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def gh_escape(msg: str) -> str:
    return msg.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def emit_error(title: str, details: str) -> None:
    tail = details[-3000:] if len(details) > 3000 else details
    print(f"::error title={title}::{gh_escape(tail)}")
    print(f"[FAIL] {title}\n{details}", file=sys.stderr)
    sys.exit(1)


def run_qemu_session(iso_path: Path, disk_path: Path, commands: list[str], timeout_sec: float = 60.0) -> str:
    cmd = [
        "qemu-system-i386",
        "-cdrom", str(iso_path),
        "-drive", f"file={disk_path},format=raw,index=0,media=disk",
        "-boot", "d",
        "-m", "64M",
        "-netdev", "user,id=n0",
        "-device", "e1000,netdev=n0",
        "-device", "isa-debug-exit,iobase=0xf4,iosize=0x04",
        "-serial", "stdio",
        "-display", "none",
        "-no-reboot",
    ]

    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0,
    )

    output = bytearray()
    deadline = time.monotonic() + timeout_sec
    cmd_idx = 0
    prompt_marker = b"tiger@bengal:~$ "
    search_cursor = 0

    try:
        while time.monotonic() < deadline:
            if proc.poll() is not None:
                assert proc.stdout is not None
                rest = proc.stdout.read()
                if rest:
                    output.extend(rest)
                break

            rlist, _, _ = select.select([proc.stdout], [], [], 0.1)
            if rlist:
                assert proc.stdout is not None
                chunk = os.read(proc.stdout.fileno(), 4096)
                if chunk:
                    output.extend(chunk)

            pos = output.find(prompt_marker, search_cursor)
            if pos != -1 and cmd_idx < len(commands):
                search_cursor = pos + len(prompt_marker)
                line = commands[cmd_idx] + "\n"
                cmd_idx += 1
                assert proc.stdin is not None
                proc.stdin.write(line.encode("ascii"))
                proc.stdin.flush()

        if proc.poll() is None:
            proc.kill()
            proc.wait(timeout=5)
            emit_error(
                f"QEMU Timeout ({cmd_idx}/{len(commands)} cmds)",
                output.decode("latin1", errors="replace"),
            )
    finally:
        if proc.poll() is None:
            proc.kill()

    return output.decode("latin1", errors="replace")


def assert_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        emit_error(f"Assertion failed in {label}: missing '{needle}'", text)


def main() -> None:
    iso_path = Path(sys.argv[1] if len(sys.argv) > 1 else "bengaltiger.iso")
    if not iso_path.is_file():
        emit_error("Missing ISO", f"ISO not found: {iso_path}")

    with tempfile.TemporaryDirectory() as tmpdir:
        disk_path = Path(tmpdir) / "test_disk.img"
        disk_path.write_bytes(b"\x00" * (16 * 1024 * 1024))

        print("[1/2] Booting QEMU Session #1 (Live boot, E1000 DHCP, Ping, mkfs, Write)...")
        boot1_cmds = [
            "info",
            "date",
            "calc (15 + 27) * 2",
            "spawn 2",
            "ps",
            "lspci",
            "ifconfig",
            "ping 10.0.2.2 2",
            "mkfs MYDISK",
            "write persist.txt Bengal_Persistent_Storage_Verified_v1",
            "cat persist.txt",
            "shutdown",
        ]
        out1 = run_qemu_session(iso_path, disk_path, boot1_cmds)

        assert_contains(out1, "Bengal Tiger OS v1.0.0", "Boot #1 banner")
        assert_contains(out1, "UTC", "Boot #1 RTC date")
        assert_contains(out1, "= 84", "Boot #1 calculator")
        assert_contains(out1, "tiger_worker", "Boot #1 preemptive task spawn")
        assert_contains(out1, "Intel", "Boot #1 PCI scan")
        assert_contains(out1, "10.0.2.15", "Boot #1 E1000 DHCPv4 IP")
        assert_contains(out1, "2 packets transmitted, 2 packets received, 0% packet loss", "Boot #1 ICMP ping")
        assert_contains(out1, "Filesystem formatted and mounted successfully", "Boot #1 mkfs")
        assert_contains(out1, "Bengal_Persistent_Storage_Verified_v1", "Boot #1 file write/cat")
        assert_contains(out1, "Shutting down Bengal Tiger OS", "Boot #1 ACPI shutdown")
        print("[PASS] Boot #1 verified: Multiboot, RTC, Scheduler, E1000 DHCP, ICMP Ping & FAT16 mkfs!")

        print("[2/2] Booting QEMU Session #2 (Reboot persistence & shell script execution)...")
        boot2_cmds = [
            "df",
            "cat persist.txt",
            "sh demo.sh",
            "shutdown",
        ]
        out2 = run_qemu_session(iso_path, disk_path, boot2_cmds)

        assert_contains(out2, "MYDISK", "Boot #2 volume label")
        assert_contains(out2, "Persistent ATA", "Boot #2 df backend")
        assert_contains(out2, "Bengal_Persistent_Storage_Verified_v1", "Boot #2 persisted file")
        assert_contains(out2, "Running Bengal Tiger OS automated demo...", "Boot #2 demo.sh script")
        print("[PASS] Boot #2 verified: Persistent ATA FAT16 survived reboot & executed demo.sh!")
        print("::notice title=QEMU E2E Verified::Both Boot #1 and Boot #2 passed DHCP, ICMP Ping, Preemptive Multitasking & Persistent FAT16 across reboot!")


if __name__ == "__main__":
    main()
