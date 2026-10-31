# Project BENGAL-IX: Masterplan for a Post-Binary CPU, Custom Hardware Platform & Native Bengal Tiger OS

> **Vision:** Move completely beyond 20th-century `{0, 1}` binary logic, clocked von Neumann bottlenecks, legacy x86/ARM/RISC-V baggage, and byte-sector filesystems — designing from scratch a **Balanced-Ternary (`{-1, 0, +1}`), Clockless Capability-Flow Processor (`TigerCore-27`)**, a **Non-Volatile Object-Memory & Packet-Switched Hardware Platform (`Sundarbans-1`)**, and a **Native Post-Binary Operating System (`Bengal Tiger OS: Ternary Edition`)**.

---

## Table of Contents

1. [Why Abandon Binary `{0, 1}` and Present-World Architectures?](#1-why-abandon-binary-0-1-and-present-world-architectures)
2. [Foundational Mathematics: Balanced Ternary (`T, 0, 1`) & Word Geometry](#2-foundational-mathematics-balanced-ternary-t-0-1--word-geometry)
3. [Physical Layer: Multi-Valued Signaling & Ternary Logic Gates](#3-physical-layer-multi-valued-signaling--ternary-logic-gates)
4. [CPU Architecture: `TigerCore-27` (Clockless Capability-Flow Processor)](#4-cpu-architecture-tigercore-27-clockless-capability-flow-processor)
5. [Platform Hardware: `Sundarbans-1` Motherboard & Peripherals](#5-platform-hardware-sundarbans-1-motherboard--peripherals)
6. [Compiler, Assembler & Binary-World Bridge Toolchain](#6-compiler-assembler--binary-world-bridge-toolchain)
7. [Operating System Redesign: `Bengal Tiger OS: Ternary Edition (BTOS-T)`](#7-operating-system-redesign-bengal-tiger-os-ternary-edition-btos-t)
8. [Subsystem Mapping: Current `Bengal Tiger OS v1.0.0` vs. `BTOS-T`](#8-subsystem-mapping-current-bengal-tiger-os-v100-vs-btos-t)
9. [7-Phase Engineering Roadmap (From Paper to Real Hardware)](#9-7-phase-engineering-roadmap-from-paper-to-real-hardware)
10. [Repository Blueprint & First Milestones](#10-repository-blueprint--first-milestones)

---

## 1. Why Abandon Binary `{0, 1}` and Present-World Architectures?

Every mainstream computer today (x86, ARM, RISC-V) inherits five 1940s–1970s compromises:

1. **Suboptimal Binary Radix (`{0, 1}`)**: Information theory proves the most hardware-efficient radix is Euler's constant $e \approx 2.71828$. Among integers, **Base-3 (Ternary)** achieves the optimal radix economy ($3 / \ln 3 \approx 2.731$ vs. binary's $2 / \ln 2 \approx 2.885$), packing more state per interconnect wire.
2. **Two's-Complement Asymmetry**: Binary requires sign-extension hacks and suffers from asymmetric ranges (`-128..+127`), causing signed overflow bugs.
3. **Forced Boolean Uncertainty**: Binary hardware forces every wire to claim `TRUE (1)` or `FALSE (0)` even when data is **uninitialized, unknown, or still arriving**, requiring complex out-of-band valid bits and enabling speculative side-channel leaks (Spectre/Meltdown class).
4. **Global Clock Wall & Von Neumann Bottleneck**: Up to 35% of modern CPU power goes into toggling a multi-GHz global clock tree while CPUs stall waiting to shuttle cache lines back and forth from dumb DRAM.
5. **Artificial RAM vs. Disk Split & Raw Pointer Insecurity**: Volatile DRAM + 512-byte disk sectors force multi-stage bootloaders (BIOS/UEFI/GRUB), filesystem serialization overhead, and raw integer pointers that allow buffer overflows and privilege escalation.

**Project BENGAL-IX** replaces all five simultaneously.

---

## 2. Foundational Mathematics: Balanced Ternary (`T, 0, 1`) & Word Geometry

### 2.1 The Balanced Trit

Instead of `{0, 1}` bits or `{0, 1, 2}` unbalanced ternary, **BENGAL-IX** uses **Balanced Ternary**:

| Symbol | Numeric Value | Logic Meaning (Kleene $K_3$) | Physical Voltage | Optical Polarization |
| :---: | :---: | :--- | :---: | :--- |
| **`T`** | **$-1$** | `FALSE` / `NEGATIVE` / `DENY` | $-V_{\text{ref}}$ (e.g., $-0.45\text{ V}$) | Left-Circular ($\sigma^-$) |
| **`0`** | **$0$** | `UNKNOWN` / `NEUTRAL` / `IDLE` | $0.00\text{ V}$ (Ground) | Zero Amplitude (Dark) |
| **`1`** | **$+1$** | `TRUE` / `POSITIVE` / `GRANT` | $+V_{\text{ref}}$ (e.g., $+0.45\text{ V}$) | Right-Circular ($\sigma^+$) |

A number of $n$ trits has value:

$$V = \sum_{i=0}^{n-1} d_i \cdot 3^i, \quad d_i \in \{-1, 0, +1\}$$

**Immediate Mathematical Superpowers:**
- **1-Wire Zero-Carry Negation**: Flipping `T` $\leftrightarrow$ `1` negates *any* integer or fixed-point value in **1 gate delay** with **zero carry ripple**.
- **Addition & Subtraction Are Identical**: `A - B` is simply `A + (-B)` with inverted `B` inputs — no separate subtractor mode or borrow chain.
- **Unbiased Rounding by Truncation**: Dropping lower trits rounds to the nearest value automatically with zero statistical bias.
- **Single-Trit 3-Way Comparison**: `CMP(A, B)` outputs a single trit (`T` if $A < B$, `0` if $A = B$, `1` if $A > B$), eliminating messy binary `ZF/CF/SF/OF` status flags.

### 2.2 Architectural Data Units

| Unit Name | Width | Total States ($3^n$) | Symmetric Integer Range | Binary Equivalent | Primary Role |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Trit (`t1`)** | 1 trit | $3$ | $-1 \dots +1$ | $\sim 1.58\text{ bits}$ | Three-valued logic, comparisons, permission flags |
| **Triad (`t3`)** | 3 trits | $27$ | $-13 \dots +13$ | $\sim 4.75\text{ bits}$ | Compact Base-27 digit (1 triad encodes `A–Z` + space) |
| **Tryte (`t9`)** | 9 trits | $19,683$ | $-9,841 \dots +9,841$ | $\sim 14.26\text{ bits}$ | Smallest addressable unit (replaces 8-bit `byte` + UTF code units) |
| **TigerWord (`W27`)** | 27 trits | $7.625 \times 10^{12}$ | $\pm 3,812,798,742,493$ | $\sim 42.79\text{ bits}$ | Standard CPU register, ALU integer/fixed-point word, instruction word |
| **Capability (`C81`)** | 81 trits | $4.434 \times 10^{38}$ | Hardware-structured | $\sim 128.38\text{ bits}$ | Unforgeable memory/object capability pointer |

---

## 3. Physical Layer: Multi-Valued Signaling & Ternary Logic Gates

### 3.1 Primitive Post-Binary Gate Library

Instead of `NAND`/`NOR`, `TigerCore-27` is synthesized from six balanced-ternary primitives:

1. **`STI(x)` (Simple Ternary Inverter)**: Maps `T -> 1`, `0 -> 0`, `1 -> T` (arithmetic negation `-x`).
2. **`PTI(x)` / `NTI(x)` (Positive / Negative Threshold Detectors)**: Level-sensitive threshold inverters used for decoding single trit states.
3. **`TMIN(x, y)` (Kleene AND / Consensus Minimum)**: Outputs $\min(x, y)$.
4. **`TMAX(x, y)` (Kleene OR / Gullibility Maximum)**: Outputs $\max(x, y)$.
5. **`TSUM(x, y)` (Modulo-3 Balanced Adder)**: Single-trit sum without carry.
6. **`TCONS(x, y)` (Balanced Carry Generator)**: Outputs `+1` iff $x=y=+1$, `-1` iff $x=y=-1$, else `0`.
7. **`TMUX3(sel, in_T, in_0, in_1)` (Symmetric 3-Way Multiplexer)**: Routes one of 3 inputs directly based on control trit `sel`.

### 3.2 Physical Realization Path (3 Stages)

- **Stage A — FPGA Dual-Rail Self-Timed Emulation (Buildable Immediately)**:
  Encode each trit on two differential wires `(rail_neg, rail_pos)`:
  - `00` = Spacer / Idle, `10` = `T (-1)`, `11` = `0 (Zero Data)`, `01` = `1 (+1)`.
  - Gives **true clockless delay-insensitive handshaking** on standard off-the-shelf FPGAs today!
- **Stage B — Discrete / Mixed-Signal Multi-Threshold CMOS (`{-V, 0V, +V}`)**:
  Split-rail power delivery (`-0.45V`, `GND`, `+0.45V`). Returning signals to `0V` recovers capacitive charge onto the center rail (adiabatic energy recycling).
- **Stage C — Native Multi-Valued Post-Silicon (CNTFET + Memristor + Photonics)**:
  Carbon Nanotube FETs (CNTFETs) have chirality-tunable threshold voltages ideal for native 3-level inverters in 6 transistors; $\text{HfO}_2$ memristors store `T / 0 / 1` resistance states non-volatilely.

---

## 4. CPU Architecture: `TigerCore-27` (Clockless Capability-Flow Processor)

`TigerCore-27` is **not** a binary RISC/CISC clone translated into base-3. It introduces four structural departures from existing CPUs:

```text
+-------------------------------------------------------------------------+
|                        TigerCore-27 ("Stripe" Tile)                     |
|                                                                         |
|  +----------------------+    +---------------------------------------+  |
|  | Self-Timed Fetch &   |--->| 3-Way Kleene Branch & Predication     |  |
|  | Instruction Window   |    | (Zero Speculation = Zero Spectre!)    |  |
|  +----------------------+    +---------------------------------------+  |
|             |                                    |                      |
|             v                                    v                      |
|  +----------------------+    +---------------------------------------+  |
|  | 27x W27 Data Regs    |<-->| Balanced-Ternary ALU + Triad SIMD     |  |
|  | (R_T13 ... R_0 ...   |    | (1-Gate Negate, 9x Triad Vector Ops)  |  |
|  |  ... R_P13)          |    +---------------------------------------+  |
|  +----------------------+                        |                      |
|  +----------------------+                        v                      |
|  | 27x C81 Capability   |<-->+---------------------------------------+  |
|  | Regs (Hardware       |    | Capability Bounds & Tag Engine        |  |
|  | Unforgeable Caps)    |    | (Replaces Page Tables & Ring 0/3!)    |  |
|  +----------------------+    +---------------------------------------+  |
|                                                  |                      |
|                                                  v                      |
|  +-------------------------------------------------------------------+  |
|  |        Asynchronous RoarLink Router + Compute-in-Memory Port      |  |
|  +-------------------------------------------------------------------+  |
+-------------------------------------------------------------------------+
```

### 4.1 Symmetric Register Indexing (`R[-13 .. +13]`)
- `27` general data registers (`W27`, 27 trits each) addressed by a **single 3-trit Triad (`-13` to `+13`)**!
  - **`R[0]`** is hardwired neutral `0` (`000...0`).
  - Negative-indexed registers (`R[-13..-1]`) hold caller-saved temporaries / expression stack frames; positive-indexed registers (`R[+1..+13]`) hold callee-preserved state.
- `27` hardware **Capability Registers (`C[-13 .. +13]`, 81 trits each)** + one irreducible Out-of-Band **Tag Trit (`t_cap`)** stored in memory ECC so user code can **never forge a pointer out of raw integers**.

### 4.2 Hardware Capability Addressing (`C81`) — Abolishing Page Tables & `Ring 0/3`
Instead of hierarchical x86 CR3 page tables and coarse `Ring 0` vs. `Ring 3` privilege bits, every object/memory reference is a `C81` hardware capability:

```text
+-----------------------+-----------------------+-----------------------+
|   Word 0 (27 trits)   |   Word 1 (27 trits)   |   Word 2 (27 trits)   |
|  Object Base Address  |  Length / Upper Bound | Perms(9t) + Epoch(18t)|
+-----------------------+-----------------------+-----------------------+
                                                 Perms (each is T/0/1):
                                                 [Read, Write, Exec,
                                                  LoadCap, StoreCap,
                                                  Seal, Invoke,
                                                  DeviceIO, Delegate]
```

- Permission trit `1` = Explicitly Granted, `0` = One-Shot / Supervised Trap, `T` = Explicitly Denied.
- Any out-of-bounds offset or unauthorized operation traps in **1 hardware cycle** — buffer overflows, use-after-free (caught via 18-trit `Epoch` generation counter), and null-pointer dereferences are **architecturally impossible**.

### 4.3 Clockless Asynchronous Execution (Zero Global Clock)
- Functional units communicate via **Self-Timed Ternary Handshake Channels** (`Req/Ack` embedded in dual-rail ternary wave-fronts).
- When the shell waits for a keystroke, **100% of gates stop switching automatically** (zero clock power, instantaneous picosecond wake on packet arrival — no `hlt` instruction needed!).
- **Zero Speculative Execution**: Instead of guessing branches and leaking secrets through caches, `TigerCore-27` uses **Kleene 3-Way Predication** (`if T / if 0 / if 1`) and lightweight hardware fiber switching across active tasks.

### 4.4 `B9-ISA` Instruction Format (Fixed 27-Trit Word)

Every instruction is exactly **one `W27` word (9 triads)**:

```text
[ Class: 1 triad ] [ Opcode: 2 triads ] [ Dest: 1 triad ] [ SrcA: 1 triad ] [ SrcB / Imm12: 4 triads ]
    (3 trits)          (6 trits)           (3 trits)         (3 trits)            (12 trits)
```

Core instruction families:
- **Arithmetic & Kleene Logic**: `TADD`, `TMUL`, `TDIVREM`, `TNEG`, `TMIN`, `TMAX`, `TXOR` (`TSUM`), `TSHIFT` (trit shift $\times 3^k$), `TCMP` (returns `T`, `0`, or `1`).
- **Triad Vector SIMD**: `V9ADD`, `V9DOT`, `V9MINMAX` (processes nine 3-trit triads or three 9-trit trytes in parallel inside a single `W27` register — ideal for graphics, audio, and neural inference!).
- **3-Way Control Flow**: `BR3 r_cond, off_neg, off_zero, off_pos` (branches three ways in one instruction!), `CALLCAP c_target`, `RETCAP`.
- **Capability Memory & Compute-in-Memory (`CiM`)**: `CLD`, `CST`, `CSUBSET` (narrows bounds/permissions monotonically — privileges can never be widened), `CIM_SCAN` (asks memory tile to search/sum/filter in place without moving data to the CPU!).
- **Mailbox & Fabric I/O**: `MBOX_SEND c_chan, r_msg`, `MBOX_RECV c_chan, r_dst` (blocks fiber at zero power if channel state is `0`).

---

## 5. Platform Hardware: `Sundarbans-1` Motherboard & Peripherals

Instead of copying a PC motherboard (PCIe, SATA, DDR5, DisplayPort, UART, PIC, RTC), **`Sundarbans-1`** defines a clean-sheet hardware platform:

```text
                       SUNDARBANS-1 POST-BINARY PLATFORM
                       
       +------------------------+       +------------------------+
       |  TigerCore-27 Tile #0  |<=====>|  TigerCore-27 Tile #1  |
       +-----------+------------+       +-----------+------------+
                   |                                |
        ===========+================================+===========
                    RoarLink Packet-Switched Ternary Mesh
        ====+================+================+===============+==
            |                |                |               |
            v                v                v               v
   +----------------+ +--------------+ +-------------+ +--------------+
   | UniRAM Vault   | | PrismVector  | | SenseHub    | | RoarNet      |
   | (Persistent    | | (Hardware 2D | | (Unified    | | (Native      |
   | Non-Volatile   | | Vector/Glyph | | HID + Timer | | Ternary Mesh |
   | Tryte Memory + | | Scene Engine | | + True RNG  | | + Binary Eth |
   | In-Mem Search) | | -> Display)  | | + Audio)    | | Bridge Tile) |
   +----------------+ +--------------+ +-------------+ +--------------+
```

### 5.1 `UniRAM`: Persistent Non-Volatile Tryte Memory (No RAM vs. Disk Split!)
- Uses non-volatile MRAM / ReRAM (or battery-backed / flash-shadowed SRAM in FPGA prototype) addressed directly at **9-trit Tryte granularity** with hardware `t_cap` tag trits.
- **No Bootloader, No BIOS, No Sector I/O**: Powering off and powering back on resumes execution right where capabilities point in `< 1 millisecond`.

### 5.2 `RoarLink`: Universal Self-Describing Peripheral Interconnect
- Replaces PCI, USB, SATA, I2C, SPI, UART, and interrupt lines with a **single uniform packet protocol**.
- Every peripheral exposes a **Capability Mailbox** (`C81` with `DeviceIO = 1`). Sending commands or receiving input is identical to sending a message to another thread (`MBOX_SEND` / `MBOX_RECV`).

### 5.3 `PrismVector`: Declarative Vector Display Engine (Replaces Dumb Framebuffers)
- Instead of the CPU wasting millions of cycles redrawing millions of RGB pixels (`VBE 1024x768x32`), the OS writes a compact **Scene Graph of Ternary Vector Primitives** (rectangles, Bezier curves, anti-aliased glyph runs, gradients, z-order layers) in `UniRAM`.
- The `PrismVector` hardware tile composites the scene directly to the display scanout at 60/120 Hz — resizing windows or moving the cursor updates **1 capability node (27 trits)** instead of copying megabytes of pixels!

### 5.4 `RoarNet` + Binary World Gateway Tile
- Native ternary machines network over **RoarNet** (ternary-framed capability streams with zero-copy `UniRAM` landing).
- A dedicated **Binary Bridge Hardware Tile** converts between `9-trit trytes` and `8-bit octets` (`Ethernet II / IPv4 / IPv6 / TCP / UDP`) at wire speed so `Sundarbans-1` talks seamlessly to existing internet routers and binary PCs!

---

## 6. Compiler, Assembler & Binary-World Bridge Toolchain

Because GCC and LLVM assume 8-bit bytes and two's-complement binary integers (`CHAR_BIT == 8`), Project BENGAL-IX introduces a dedicated clean-sheet toolchain:

1. **`tasm27` (Balanced-Ternary Macro Assembler)**:
   - Supports literal balanced ternary (`0t10T_110_0T1`), Base-27 compact notation (`0n9A3`), decimal, and UTF-Tryte text literals.
   - Emits `.tcap` (Ternary Capability Capsule) object modules.
2. **`Garjan` (Systems Programming Language for Post-Binary OS)**:
   - Clean C/Rust-inspired systems language with first-class post-binary primitives:
     ```rust
     // Example Garjan kernel snippet for BTOS-T
     fn compare_and_route(pkt: cap<Packet>, limit: word27) -> trit {
         branch3 (pkt.priority <=> limit) {
             neg  => return 0tT,   // Drop / low priority (-1)
             zero => return 0t0,   // Queue neutral (0)
             pos  => return 0t1,   // Fast-path dispatch (+1)
         }
     }
     ```
   - Native primitive types: `trit`, `triad`, `tryte`, `word27`, `word81`, and hardware-checked `cap<T>`.
   - Compiler enforces capability provenance statically and emits hardware `C81` instructions dynamically.
3. **`TigerSim-27` (Cycle- & Wire-Accurate Host Emulator)**:
   - Runs on today's Windows/Linux/macOS machines; renders `PrismVector` graphics in an SDL2/WebCanvas window, bridges `RoarNet` to host UDP/SLIRP, and includes a live **Trit Logic Oscilloscope & Capability Inspector**.

---

## 7. Operating System Redesign: `Bengal Tiger OS: Ternary Edition (BTOS-T)`

When porting **Bengal Tiger OS** from x86 binary hardware to `TigerCore-27` + `Sundarbans-1`, the OS becomes dramatically smaller, faster, and mathematically memory-safe:

### 7.1 Instant-On Capsule Lifecycle (Replaces `boot.s`, `GRUB`, `GDT`, `IDT`, `PIC`)
- On first factory initialization (`Cold Genesis`), `GenesisInit` mints root capabilities for `UniRAM`, `PrismVector`, `SenseHub`, and `RoarNet` and creates the persistent root `TigerVault`.
- On every subsequent power-on (`Warm Wake`), hardware resumes directly from the saved `FiberTable` in `UniRAM` — **0.5 ms boot time**.

### 7.2 Capability Object Allocator (Replaces `paging.c` + `heap.c`)
- Instead of `kmalloc(size)` returning a forgeable `void*`, `cap_alloc(tryte_count, perms)` allocates a persistent or ephemeral region in `UniRAM`, increments the hardware `Epoch`, and returns a bounded `C81` capability.
- `cap_free(cap)` increments the region's `Epoch` tag in `UniRAM`, instantly invalidating any dangling copies of `cap` anywhere in the system!

### 7.3 `TigerVault` Persistent Object Graph (Replaces `disk.c` + `fat.c`)
- Abolishes sector numbers, cluster chains, and serialization!
- Documents, scripts, configs, and programs live directly in `UniRAM` as **Typed Persistent Capsules** indexed by human names + cryptographic ternary hashes:
  - Editing a note in `edit` mutates a persistent text capsule via copy-on-write capability snapshots (instant undo/version history for free!).
  - An optional **FAT16 / Binary Tar Import-Export Translator** lets users exchange files with legacy USB sticks or binary hosts.

### 7.4 Cooperative + Preemptive Hardware Fiber Scheduler
- Hardware maintains a 27-slot active fiber ring per `TigerCore-27` tile; blocked fibers waiting on `MBOX_RECV` (`0` state) consume **zero cycles** and wake in hardware the instant a `RoarLink` packet arrives.

### 7.5 Interactive `BengalShell-27`, Vector `Desktop`, and `Edit-27`
- Preserves the beloved **Bengal Tiger OS** user experience (`info`, `ls`, `edit`, `sh`, `ps`, `ping`, `calc`, `gui`) while adding post-binary superpowers:
  - **`calc`** natively computes exact balanced-ternary & decimal expressions plus 3-valued Kleene logic tables.
  - **`caps`** inspects live `C81` capabilities, bounds, epochs, and permissions.
  - **`gui`** drives the hardware `PrismVector` engine for butter-smooth 60 FPS scalable vector cards, live trit-utilization meters, and zero-tearing mouse interaction.

---

## 8. Subsystem Mapping: Current `Bengal Tiger OS v1.0.0` vs. `BTOS-T`

| Current x86 Binary Module (`v1.0.0`) | Legacy Binary Constraint | Post-Binary `BENGAL-IX` Replacement | Architectural Gain |
| :--- | :--- | :--- | :--- |
| `boot/boot.s` + `grub.cfg` | BIOS -> GRUB -> Multiboot1 -> A20 gate | `genesis/warm_wake.tasm` (`UniRAM` Root Capsule) | `< 1 ms` instant-on; zero bootloader stages |
| `gdt.c`, `idt.c`, `pic.c`, `isr.s` | Segment descriptors, IRQ remapping, `cli/sti` | `RoarLink` Hardware Capability Mailboxes | Zero interrupt storms or `IF` race conditions |
| `paging.c` + `heap.c` | 4 KB x86 page tables + raw `void*` heap | `cap_mm.grj` (`C81` Bounds + `Epoch` Allocator) | Mathematically impossible buffer overflow or use-after-free |
| `disk.c` + `fat.c` | 512 B ATA PIO sectors + FAT12/16 tables | `tigervault.grj` (Direct Persistent `UniRAM` Capsules) | Zero serialization overhead; atomic snapshots |
| `vbe.c` + `font.c` | CPU-drawn `1024x768x32` dumb pixel array | `prism_scene.grj` (`PrismVector` Hardware Scene Graph) | Resolution-independent vector UI at <5% bus bandwidth |
| `keyboard.c` + `mouse.c` + `serial.c` | Legacy PS/2 `0x60` scancodes & `16550` UART | `sensehub.grj` (Unified `RoarPacket` Event Stream) | Single clean event protocol for all human & debug I/O |
| `pci.c` + `nic.c` + `net.c` | PCI BAR polling + E1000/RTL8139 octet rings | `roarnet.grj` + Hardware Binary-Bridge Tile | Native ternary mesh + wire-speed IPv4/ICMP compatibility |
| `shell.c` + `editor.c` | 8-bit ASCII strings & 2-way `if/else` C code | `shell27.grj` + `edit27.grj` (UTF-Tryte + 3-way `branch3`) | Native ternary math, capability sandboxing for every command |

---

## 9. 7-Phase Engineering Roadmap (From Paper to Real Hardware)

You do **not** need a billion-dollar semiconductor fab on Day 1. Each phase delivers a working, demonstrable artifact that validates the next phase:

### Phase 1: Formal Post-Binary Architecture & Gate Algebra *(Months 1–2)*
- Finalize the **`B9-ISA v1.0` Reference Manual**: trit/triad/tryte/W27/C81 encodings, balanced-ternary arithmetic proofs, floating/fixed-point format, and `C81` capability monotonicity rules.
- Prove ALU carry chains, `C81` bounds checks, and Kleene 3-way predication formally in **Python + Z3 Theorem Prover** (zero hardware logic bugs before writing RTL).
- **Deliverable**: `spec/B9_ISA_SPEC.md` + executable Python reference golden model.

### Phase 2: `TigerSim-27` Full-System Emulator & Visual Debugger *(Months 3–4)*
- Build **`TigerSim-27`** (in Rust or C + WebAssembly) modeling `TigerCore-27`, persistent `UniRAM` (backed by a host memory-mapped file), `PrismVector` display, `SenseHub` input, and `RoarNet` networking.
- Compile to **WebAssembly** as well so anyone can boot the post-binary computer right inside a web browser with a live `T / 0 / 1` register & wire visualizer!
- **Deliverable**: Interactive desktop + browser emulator capable of booting raw `.tcap` images.

### Phase 3: Post-Binary Toolchain (`tasm27` + `Garjan` Compiler) *(Months 5–7)*
- Implement **`tasm27`** assembler, **`tlink`** capability linker, and disassembler/debugger.
- Implement the **`Garjan`** compiler front-end and code generator (custom SSA IR with native 3-valued types and capability provenance verification).
- **Deliverable**: Self-tested compiler suite with automated golden-model test vectors.

### Phase 4: Native `Bengal Tiger OS: Ternary Edition (BTOS-T)` on Emulator *(Months 8–10)*
- Write `GenesisInit`, `CapMM` allocator, `TigerVault` persistent object store, hardware fiber scheduler, `PrismVector` GUI desktop, `Edit-27`, `RoarNet` stack, and `BengalShell-27` in `Garjan` + `tasm27`.
- Build the **Binary World Bridge** so `BTOS-T` can `ping` real IPv4 hosts and import/export files from x86 `Bengal Tiger OS v1.0.0`.
- **Deliverable**: Complete post-binary OS booting in `< 1 ms` on `TigerSim-27` (and live in browser!).

### Phase 5: FPGA Hardware Synthesis (`Sundarbans-FPGA` Prototype) *(Months 11–14)*
- Implement `TigerCore-27`, `RoarLink`, `PrismVector` (outputting real 720p/1080p HDMI), `SenseHub` (USB HID), and `UniRAM` (FRAM/MRAM SPI/parallel module + BRAM cache) in **SystemVerilog / Chisel** using **Dual-Rail Self-Timed Ternary Logic**.
- Synthesize onto an affordable open-toolchain FPGA board (e.g., **ULX3S Lattice ECP5-85F** or **Digilent Arty A7-100T**).
- **Deliverable**: Real physical board plugged into a real HDMI monitor, keyboard, mouse, and Ethernet cable running **Bengal Tiger OS: Ternary Edition** with **zero binary instructions**!

### Phase 6: True Physical 3-Voltage (`{-V, 0V, +V}`) Hardware Slice + Open ASIC Tapeout *(Months 15–20)*
- Design and fabricate a **Custom 4-Layer Mixed-Signal PCB ("TigerSlice-3V")** implementing a true physical 3-voltage (`-1.65V / 0V / +1.65V`) Balanced-Ternary ALU + `RoarLink` physical ternary transceiver connected to the FPGA fabric — verifiable live on an oscilloscope showing 3 distinct voltage levels!
- Submit a **`TigerCore-27` Tile ASIC** to an open-source multi-project wafer shuttle (**TinyTapeout** on **SkyWater 130nm** or **IHP 130nm**).
- **Deliverable**: Real custom silicon chip + physical 3-level voltage hardware board.

### Phase 7: Post-Silicon Horizon — CNTFET / Memristive / Photonic Core *(Long-Term Research)*
- Partner with university/open-foundry nanoelectronics labs to map the exact same verified `STI / TMIN / TMAX / TSUM` gate netlist onto **chirality-tuned CNTFETs** and **3-state $\text{HfO}_2$ memristor crossbars** (true single-device non-volatile ternary Compute-in-Memory).

---

## 10. Repository Blueprint & First Milestones

When starting implementation in a dedicated repo or branch (`bengal-ix/`), structure the project cleanly so hardware, toolchain, emulator, and OS evolve together in lockstep:

```text
bengal-ix/
├── spec/
│   ├── 01_balanced_ternary_math.md    # Trit/Triad/Tryte/W27/C81 formal definitions
│   ├── 02_b9_isa_encoding.md          # Complete 27-trit instruction set & opcodes
│   ├── 03_c81_capability_model.md     # Hardware bounds, permissions & epoch rules
│   └── 04_roarlink_and_prism.md       # Packet interconnect & vector display spec
├── proofs/
│   └── verify_alu_and_caps.py         # Z3 formal proofs of ternary ALU & capabilities
├── tigersim/                          # Cycle-accurate Rust/C + WebAssembly emulator
├── toolchain/
│   ├── tasm27/                        # Balanced-ternary macro assembler & linker
│   └── garjan/                        # Post-binary systems language compiler
├── hardware/
│   ├── rtl/                           # Self-timed dual-rail ternary SystemVerilog
│   ├── fpga_ecp5/                     # HDMI + FRAM/MRAM + Ethernet FPGA top-level
│   └── pcb_3level_alu/                # KiCad schematics for {-V, 0V, +V} physical board
└── btos-t/                            # Bengal Tiger OS: Ternary Edition (written in Garjan)
    ├── genesis/                       # Instant-on UniRAM capability bootstrap
    ├── kernel/                        # CapMM, Fiber scheduler, Mailbox IPC
    ├── vault/                         # Persistent object graph (replaces FAT)
    ├── prism_gui/                     # Declarative vector desktop & scene composer
    └── shell27/                       # Post-binary interactive shell & Edit-27
```

### Immediate Week-1 Action Items to Kick Off Phase 1 & Phase 2
1. **Write the 300-line Python/Rust `Trit` & `W27` Arithmetic Core** (`T, 0, 1` lookup tables for `STI`, `TMIN`, `TMAX`, `TSUM`, `TCONS`, 27-trit ripple/lookahead adder, 1-step negation, and `C81` capability bounds checker).
2. **Define the 27 core opcodes of `B9-ISA v0.1`** and assemble the very first post-binary program (`hello_prism.tcap`) using `tasm27`.
3. **Boot `hello_prism.tcap` inside `TigerSim-27`** rendering the Bengal Tiger vector crest and interactive `tiger@bengal-ix:~#` ternary prompt!
