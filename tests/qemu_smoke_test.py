#!/usr/bin/env python3
"""
Bengal Tiger OS - End-to-End Two-Boot QEMU Hardware Integration Test

Executes two real x86 virtual machine boots from `bengaltiger.iso` with:
  - 64 MB RAM
  - Raw 16 MB IDE ATA hard disk (`test_disk.img`)
  - Intel PRO/1000 (E1000) PCI Ethernet on QEMU SLIRP user network (`10.0.2.0/24`)
  - Interactive COM1 serial console

Verifies across Boot #1 and Boot #2:
  1. GRUB Multiboot1 boot -> E820 PMM, Paging, Heap, PIT, CMOS RTC, E1000 DHCPv4
  2. Shell commands: `info`, `date`, `mem`, `calc`, `spawn` + `ps`, `lspci`
  3. Live TCP/IP Networking: `ifconfig` (`10.0.2.15`) + `ping 10.0.2.2 2` (0% loss)
  4. Persistent Storage: `mkfs MYDISK` -> `write persist.txt ...` -> ACPI `shutdown`
  5. Second Boot: Automatic mount of `MYDISK` ATA FAT16 volume & verification
     that `persist.txt` survived hardware poweroff and reboot intact!
"""

import os
import select
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def run_qemu_session(iso_path: Path, disk_path: Path, commands: list[str], timeout_sec: float = 45.0) -> str:
    cmd = [
        "qemu-system-i386",
        "-cdrom", str(iso_path),
        "-hda", str(disk_path),
        "-boot", "d",
        "-m", "64M",
        "-netdev", "user,id=n0",
        "-device", "e1000,netdev=n0",
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
    prompt_marker = b"@bengal:~$ "
    search_cursor = 0

    try:
        while time.monotonic() < deadline:
            if proc.poll() is not None:
                rest = proc.stdout.read()
                if rest:
                    output.extend(rest)
                break

            rlist, _, _ = select.select([proc.stdout], [], [], 0.1)
            if rlist:
                chunk = os.read(proc.stdout.fileno(), 4096)
                if chunk:
                    output.extend(chunk)

            # Whenever a fresh shell prompt appears after our last command, send next
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
            raise RuntimeError(
                f"QEMU session timed out (sent {cmd_idx}/{len(commands)} commands).\n"
                f"--- SERIAL OUTPUT ---\n{output.decode('latin1', errors='replace')}"
            )
    finally:
        if proc.poll() is None:
            proc.kill()

    return output.decode("latin1", errors="replace")


def assert_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        print(f"[FAIL] Missing expected '{needle}' in {label}!", file=sys.stderr)
        print("--- FULL QEMU OUTPUT ---", file=sys.stderr)
        print(text, file=sys.stderr)
        sys.exit(1)


def main() -> None:
    iso_path = Path(sys.argv[1] if len(sys.argv) > 1 else "bengaltiger.iso")
    if not iso_path.is_file():
        print(f"[FAIL] ISO not found: {iso_path}", file=sys.stderr)
        sys.exit(1)

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

        assert_contains(out2, "Mounted persistent ATA FAT volume", "Boot #2 auto-mount")
        assert_contains(out2, "MYDISK", "Boot #2 volume label")
        assert_contains(out2, "Persistent ATA", "Boot #2 df backend")
        assert_contains(out2, "Bengal_Persistent_Storage_Verified_v1", "Boot #2 persisted file")
        assert_contains(out2, "Running Bengal Tiger OS automated demo...", "Boot #2 demo.sh script")
        print("[PASS] Boot #2 verified: Persistent ATA FAT16 survived reboot & executed demo.sh!")


if __name__ == "__main__":
    main()
