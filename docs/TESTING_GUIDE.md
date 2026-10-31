# Bengal Tiger OS — Easy Run & Interactive Testing Guide

This guide walks you through the **easiest ways to boot `bengaltiger.iso`** (from a 30-second zero-install browser test up to VirtualBox, QEMU, and real USB hardware), shows **what each screen looks like**, and provides a **complete feature checklist** to test every subsystem.

---

## Step 0: Download the Ready-to-Boot ISO

Download **`bengaltiger.iso`** (~11.4 MB) from the official GitHub Release:

- **Latest Release Page**: <https://github.com/BengalEmpire/Bengal-Tiger-OS/releases/latest>
- **Direct ISO (`v1.0.0`)**: <https://github.com/BengalEmpire/Bengal-Tiger-OS/releases/download/v1.0.0/bengaltiger.iso>
- **SHA-256 Checksum (`v1.0.0`)**: <https://github.com/BengalEmpire/Bengal-Tiger-OS/releases/download/v1.0.0/SHA256SUMS.txt>

*(Optional)* Verify file integrity on Linux / macOS / Git Bash:

```bash
sha256sum -c SHA256SUMS.txt
```

---

## Method 1: Run Directly in Your Web Browser (Easiest — Zero Install, 30 Seconds)

You can boot and test `bengaltiger.iso` immediately inside **Chrome, Edge, Brave, or Firefox** without installing any software:

1. Download **`bengaltiger.iso`** to your computer.
2. Open **[https://copy.sh/v86/](https://copy.sh/v86/)** in your web browser.
3. Next to **`CD image`**, click **`Choose File`** and select **`bengaltiger.iso`**.
4. *(Optional)* Set **`Memory size`** to **`128 MB`**.
5. Scroll down and click **`Start Emulation`**.
6. At the **GRUB Boot Menu**, press **`Enter`** (or wait 3 seconds).
7. Click inside the emulator screen to type commands or move the mouse (`Esc` releases the mouse pointer).

> **Tip:** Browser emulation uses the built-in **512 KB Live FAT12 RAMDisk** automatically, so `ls`, `cat`, `edit`, `sh DEMO.SH`, `spawn`, and `gui` all work out of the box. For live `dhcp`/`ping` networking and persistent hard-disk storage across reboots, use **Method 2 (VirtualBox)** or **Method 3 (QEMU)**.

---

## Method 2: VirtualBox (Point-and-Click Desktop App — Full Network + Saved Files)

1. Download and install **[Oracle VirtualBox](https://www.virtualbox.org/)** (Windows, macOS, or Linux).
2. Open VirtualBox and click **`New`**:
   - **Name**: `Bengal Tiger OS`
   - **ISO Image**: Select **`bengaltiger.iso`**
   - **Type**: `Other`
   - **Version**: `Other/Unknown` *(32-bit)*
3. Set **Base Memory (RAM)** to **`128 MB`** (or `256 MB`) → click **`Next`**.
4. Select **Create a Virtual Hard Disk Now** (`64 MB` VDI/VHD is plenty) → click **`Finish`**.
5. Verify Network & Storage settings (click **`Settings`**):
   - **System → Boot Order**: Keep **Optical** above **Hard Disk** (or press `F12` on boot).
   - **Storage**: Ensure both the Optical Drive (`bengaltiger.iso`) and Virtual Hard Disk are under the **IDE Controller**.
   - **Network → Adapter 1**: Enable Adapter, Attached to **`NAT`**, click **`Advanced`** → Adapter Type: **`Intel PRO/1000 MT Desktop (82540EM)`**.
6. Click the green **`Start`** arrow!

---

## Method 3: QEMU (One-Command Launch for Windows, Linux & macOS)

### Quick Live Boot (RAMDisk + Live Networking)

- **Linux / macOS / WSL:**
  ```bash
  qemu-system-i386 \
    -cdrom bengaltiger.iso \
    -m 128M \
    -netdev user,id=n0 -device e1000,netdev=n0
  ```

- **Windows (PowerShell):**
  ```powershell
  # Install QEMU once if needed:
  winget install SoftwareFreedomConservancy.QEMU

  # Boot Bengal Tiger OS:
  & "C:\Program Files\qemu\qemu-system-i386.exe" `
    -cdrom bengaltiger.iso `
    -m 128M `
    -netdev user,id=n0 -device e1000,netdev=n0
  ```

### QEMU with Persistent Hard Disk (Saves Files Across Reboots)

```bash
# 1. Create a blank 32 MB raw disk image once
qemu-img create -f raw disk.img 32M

# 2. Boot with both the ISO and persistent disk attached
qemu-system-i386 \
  -cdrom bengaltiger.iso \
  -drive file=disk.img,format=raw,index=0,media=disk \
  -boot d \
  -m 128M \
  -netdev user,id=n0 -device e1000,netdev=n0 \
  -serial stdio
```

> Inside Bengal Tiger OS, run **`mkfs MYDISK`** once to format `disk.img` as persistent **FAT16**. Every file you create with `write` or `edit` will survive reboots!

---

## Method 4: Real x86 PC Hardware (Bootable USB Drive)

1. Flash **`bengaltiger.iso`** onto a USB drive:
   - **Windows**: Use **[Rufus](https://rufus.ie/)** (select **DD Image mode** when prompted) or **[balenaEtcher](https://etcher.balena.io/)**.
   - **Linux**:
     ```bash
     # Replace /dev/sdX carefully with your USB device!
     sudo dd if=bengaltiger.iso of=/dev/sdX bs=4M status=progress oflag=sync
     ```
2. Boot the target PC in **Legacy BIOS / CSM** mode and choose the USB drive from the BIOS boot menu.

---

## Visual Walkthrough: What You Will See & Feel

### 1. GRUB Bootloader Menu (First 3 Seconds)
- Displays resolution choices:
  - `Bengal Tiger OS v1.0.0 (Graphics 1024x768x32)` *(Default — auto-boots in 3s)*
  - `Bengal Tiger OS v1.0.0 (VGA Text 80x25)`
  - `Bengal Tiger OS v1.0.0 (Graphics 1280x1024x32)`
  - `Bengal Tiger OS v1.0.0 (Graphics 800x600x32)`
  - `Bengal Tiger OS v1.0.0 (Headless / Serial Debug)`

### 2. High-Resolution Console (`1024x768x32`)
Within ~1 second, the kernel initializes hardware, negotiates a DHCPv4 lease (if an E1000 or RTL8139 NIC is present), mounts storage, and opens the dark-slate console with a live tiger-orange header bar:

```text
 Bengal Tiger OS v1.0.0   [RAMDisk] [IP: 10.0.2.15]               2026-11-01 14:35:10 UTC
──────────────────────────────────────────────────────────────────────────────────────────
================================================================
  Bengal Tiger OS v1.0.0 (Royal Bengal) - Ready
================================================================
  Type 'help' for commands, 'info' for system summary, 'edit' to edit files,
  'gui' (or F2) for graphical desktop, 'ping 10.0.2.2' to test networking.

tiger@bengal:~$ █
```

- **Keyboard feel**:
  - `Up` / `Down` arrows browse command history.
  - `Tab` auto-completes commands and filenames.
  - `Left` / `Right` / `Home` / `End` / `Delete` edit anywhere on the current line.
  - `Ctrl+L` clears the screen; `Ctrl+C` cancels the current input line.

### 3. Full-Screen Text Editor (`edit <filename>`)
Running `edit notes.txt` opens a full-screen 32 KB text editor:
- Top bar displays filename and `[MODIFIED]` indicator.
- Bottom status bar shows live `Ln` / `Col` cursor coordinates and shortcuts.
- **`Ctrl+S`** writes directly to FAT12/FAT16 storage; **`Ctrl+Q`** returns cleanly to the shell.

### 4. Graphical Desktop Workspace (`gui` or `F2`)
Running `gui` (or pressing **`F2`**) switches to the `1024x768` graphical desktop with a live **PS/2 mouse cursor** and 3 real-time dashboard cards:
- **System & Memory Monitor**: CPU brand, free physical frames, live kernel heap usage, active tasks, and uptime seconds.
- **Network Stack (TCP/IP)**: Active NIC driver, IPv4 address, RX/TX packet counters, and a clickable **`[ Ping Gateway ]`** button (or press **`P`**).
- **FAT Filesystem Explorer**: Live directory listing with file sizes on your mounted RAMDisk or ATA hard drive.
- Click **`Exit GUI [ESC]`** in the top-right corner (or press **`ESC`** / **`Q`**) to return to the shell.

---

## Step-by-Step Interactive Feature Test Checklist

Run these commands at the `tiger@bengal:~$ ` prompt to test every part of the OS:

### 1. System Info, Clock & Kernel Logs
```text
info
cpu
date
uptime
dmesg
calc (15 + 27) * 2
```
- **Expected**: Bengal Tiger summary banner, CPU vendor/brand & features, live UTC date/time from CMOS RTC, timestamped kernel boot log, and `= 84`.

### 2. Filesystem & Built-in Shell Scripting (Works Immediately on Live RAMDisk)
```text
df
ls
cat WELCOME.TXT
sh DEMO.SH
write test.txt Hello_from_Bengal_Tiger_OS
append test.txt _v1.0.0
cat test.txt
hexdump test.txt
```
- **Expected**: Lists preloaded files (`README.TXT`, `WELCOME.TXT`, `HELP.TXT`, `DEMO.SH`), executes the demo script, creates/appends `test.txt`, and prints both text and hex dump views.

### 3. Full-Screen Text Editor
```text
edit mynote.txt
```
1. Type a few lines of text using `Enter` and arrow keys.
2. Press **`Ctrl+S`** to save (`Saved to disk` appears in the bottom bar).
3. Press **`Ctrl+Q`** to exit back to the shell.
4. Verify with `cat mynote.txt`.

### 4. Preemptive Multitasking & Dynamic Memory
```text
mem
spawn 3
ps
top
```
- **Expected**: Spawns 3 preemptive `tiger_worker` kernel threads scheduled at 100 Hz alongside the shell without freezing input, and displays heap/frame statistics.

### 5. Live Networking (VirtualBox NAT / QEMU E1000)
```text
lspci
ifconfig
ping 10.0.2.2 4
arp
netstat
```
- **Expected**: Detects the PCI Ethernet controller, shows DHCP-assigned IPv4 (`10.0.2.15` on QEMU/VirtualBox NAT), receives 4/4 ICMP Echo replies (`0% packet loss`), and lists the gateway MAC in the ARP table.

### 6. Persistent Hard Disk Across Reboots (VirtualBox / QEMU with Disk Attached)
```text
disk
mkfs MYDISK
write saved.txt Survived_Reboot_Successfully
shutdown
```
Boot the VM a second time and run:
```text
df
cat saved.txt
```
- **Expected**: `MYDISK` mounts automatically as `Persistent ATA` (`FAT16`) at boot, and `cat saved.txt` prints `Survived_Reboot_Successfully`.

### 7. Graphical Desktop & Mouse
```text
gui
```
- **Expected**: Move your mouse cursor, click **`[ Ping Gateway ]`** (or press `P`), inspect live stats, then press **`ESC`** to return to `tiger@bengal:~$ `.

---

## Automated Verification From Source (For Developers)

If you cloned the repository on Ubuntu / Debian / WSL2, you can run the full automated test pipeline (Multiboot verifier + ASan/UBSan unit suite + two-boot headless QEMU integration test) with:

```bash
make iso        # Build build/kernel.bin and bootable bengaltiger.iso
make test       # Run ELF32/Multiboot1 check + ASan/UBSan host unit tests
make test-qemu  # Run automated 2-boot QEMU hardware E2E verification
```
