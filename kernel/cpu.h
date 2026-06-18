/**
 * Bengal Tiger OS - CPU Initialization and Feature Detection
 *
 * Handles:
 *   - A20 Gate enable (critical for accessing memory above 1MB)
 *   - CPUID detection and CPU feature flags
 *   - FPU (x87) initialization
 *   - SSE/SSE2 initialization if available
 *
 * @file cpu.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef CPU_H
#define CPU_H

#include "common.h"

/* CPUID feature flags (ECX after CPUID leaf 1) */
#define CPUID_FEAT_ECX_SSE3         (1 << 0)
#define CPUID_FEAT_ECX_SSSE3        (1 << 9)
#define CPUID_FEAT_ECX_SSE4_1       (1 << 19)
#define CPUID_FEAT_ECX_SSE4_2       (1 << 20)

/* CPUID feature flags (EDX after CPUID leaf 1) */
#define CPUID_FEAT_EDX_FPU          (1 << 0)
#define CPUID_FEAT_EDX_VME          (1 << 1)
#define CPUID_FEAT_EDX_DE           (1 << 2)
#define CPUID_FEAT_EDX_PSE          (1 << 3)
#define CPUID_FEAT_EDX_TSC          (1 << 4)
#define CPUID_FEAT_EDX_MSR          (1 << 5)
#define CPUID_FEAT_EDX_PAE          (1 << 6)
#define CPUID_FEAT_EDX_MCE          (1 << 7)
#define CPUID_FEAT_EDX_CX8          (1 << 8)
#define CPUID_FEAT_EDX_APIC         (1 << 9)
#define CPUID_FEAT_EDX_SEP          (1 << 11)  /* SYSENTER/SYSEXIT */
#define CPUID_FEAT_EDX_MTRR         (1 << 12)
#define CPUID_FEAT_EDX_PGE          (1 << 13)
#define CPUID_FEAT_EDX_MCA          (1 << 14)
#define CPUID_FEAT_EDX_CMOV         (1 << 15)
#define CPUID_FEAT_EDX_PAT          (1 << 16)
#define CPUID_FEAT_EDX_PSE36        (1 << 17)
#define CPUID_FEAT_EDX_PSN          (1 << 18)
#define CPUID_FEAT_EDX_CLFLUSH      (1 << 19)
#define CPUID_FEAT_EDX_DTES         (1 << 21)
#define CPUID_FEAT_EDX_ACPI         (1 << 22)
#define CPUID_FEAT_EDX_MMX          (1 << 23)
#define CPUID_FEAT_EDX_FXSR         (1 << 24)  /* FXSAVE/FXRSTOR */
#define CPUID_FEAT_EDX_SSE          (1 << 25)
#define CPUID_FEAT_EDX_SSE2         (1 << 26)
#define CPUID_FEAT_EDX_HTT          (1 << 28)

/** CPU feature flags struct */
typedef struct {
    char vendor[13];            /* CPU vendor string (e.g., "GenuineIntel") */
    char brand[49];             /* CPU brand string (e.g., "Intel(R) Core(TM)") */
    uint32_t family;            /* CPU family */
    uint32_t model;             /* CPU model */
    uint32_t stepping;          /* CPU stepping */
    uint32_t features_ecx;      /* ECX feature flags */
    uint32_t features_edx;      /* EDX feature flags */
    uint8_t has_cpuid : 1;      /* CPUID instruction available */
    uint8_t has_fpu   : 1;      /* x87 FPU present */
    uint8_t has_sse   : 1;      /* SSE supported */
    uint8_t has_sse2  : 1;      /* SSE2 supported */
    uint8_t has_msr   : 1;      /* RDMSR/WRMSR supported */
    uint8_t has_apic  : 1;      /* Local APIC present */
} cpu_info_t;

/** Global CPU information */
extern cpu_info_t cpu_info;

/* ============================================ */
/* Functions                                    */
/* ============================================ */

/**
 * Initialize CPU features:
 *   1. Enable A20 gate (ensures memory above 1MB is accessible)
 *   2. Detect CPUID availability
 *   3. Query CPU features
 *   4. Initialize FPU if present
 *   5. Enable SSE/SSE2 if available
 *
 * Must be called very early in kmain(), before any memory operations
 * that might access addresses above 1MB.
 */
void cpu_init(void);

/**
 * Enable the A20 gate.
 * Uses multiple methods in order of reliability:
 *   1. Check if already enabled
 *   2. Fast A20 gate (port 0x92)
 *   3. Keyboard controller method
 *
 * @return 1 if A20 is enabled, 0 on failure
 */
int a20_enable(void);

/**
 * Check CPUID availability by testing the ID flag (bit 21) in EFLAGS.
 *
 * @return 1 if CPUID is available, 0 otherwise
 */
int cpu_detect_cpuid(void);

/**
 * Query CPU vendor, features, and brand string via CPUID.
 * Populates the global cpu_info structure.
 */
void cpu_query_features(void);

/**
 * Initialize the x87 FPU.
 *   1. Set EM (Emulation) bit in CR0 to 0
 *   2. Set MP (Monitor Coprocessor) bit in CR0 to 1
 *   3. Issue FNINIT to reset FPU state
 */
void fpu_init(void);

/**
 * Enable SSE instructions if supported.
 * Sets OSFXSR and OSXMMEXCPT bits in CR4.
 *
 * @return 1 if SSE enabled, 0 if not supported
 */
int sse_enable(void);

/**
 * Return CPU vendor string.
 */
const char* cpu_get_vendor(void);

/**
 * Return CPU brand string.
 */
const char* cpu_get_brand(void);

#endif /* CPU_H */
