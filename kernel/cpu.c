/**
 * Bengal Tiger OS - CPU Initialization Implementation
 *
 * @file cpu.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#include "cpu.h"
#include "common.h"

/* Global CPU info structure */
cpu_info_t cpu_info;

/**
 * Use the "Fast A20 Gate" method via port 0x92.
 * This is the simplest method supported on most modern systems.
 */
static int a20_enable_fast(void) {
    uint8_t port_val = inb(0x92);

    /* If A20 is already on, we're done */
    if (port_val & 0x02) {
        return 1;
    }

    /* Enable A20 gate by setting bit 1 */
    port_val |= 0x02;
    outb(0x92, port_val);

    /* Small delay for the gate to stabilize */
    for (volatile int i = 0; i < 100; i++);

    /* Verify it worked */
    port_val = inb(0x92);
    return (port_val & 0x02) ? 1 : 0;
}

/**
 * Use the Keyboard Controller method for A20 gate.
 * More compatible but slower.
 */
static int a20_enable_keyboard(void) {
    int timeout = 10000;

    /* Step 1: Disable keyboard (optional, but safer) */
    while (timeout-- && (inb(0x64) & 0x02));  /* Wait for input buffer empty */

    outb(0x64, 0xAD);  /* Disable keyboard */
    while (timeout-- && (inb(0x64) & 0x02));

    /* Step 2: Read from port 0x60 */
    outb(0x64, 0xD0);  /* Command: Read output port */
    while (timeout-- && !(inb(0x64) & 0x01));  /* Wait for output buffer full */

    uint8_t output = inb(0x60);

    /* Step 3: Write back with A20 bit set */
    while (timeout-- && (inb(0x64) & 0x02));

    outb(0x64, 0xD1);  /* Command: Write output port */
    while (timeout-- && (inb(0x64) & 0x02));

    outb(0x60, output | 0x02);  /* Set bit 1 (A20 gate) */
    while (timeout-- && (inb(0x64) & 0x02));

    /* Step 4: Re-enable keyboard */
    outb(0x64, 0xAE);

    /* Verify by reading back */
    while (timeout-- && (inb(0x64) & 0x02));
    outb(0x64, 0xD0);
    while (timeout-- && !(inb(0x64) & 0x01));
    output = inb(0x60);

    return (output & 0x02) ? 1 : 0;
}

/**
 * Verify A20 gate is enabled by checking memory wrap-around.
 * Writes a test pattern at 1MB boundary and reads it back.
 */
static int a20_check(void) {
    /* Use a simple test: write a value at 0x100000 (1MB),
     * then check if we can read it back from 0x000000.
     * If A20 is disabled, they'd be the same memory (wrapped). */
    volatile uint32_t *ptr_1mb = (volatile uint32_t*)0x100000;
    volatile uint32_t *ptr_0 = (volatile uint32_t*)0x000000;
    uint32_t saved = *ptr_0;

    *ptr_0 = 0xBEEFBEEF;
    *ptr_1mb = 0xDEADBEEF;

    int enabled = (*ptr_0 == 0xBEEFBEEF);

    *ptr_0 = saved;  /* Restore */
    return enabled;
}

int a20_enable(void) {
    /* First check if already enabled */
    if (a20_check()) {
        return 1;
    }

    /* Try fast A20 gate first */
    if (a20_enable_fast()) {
        if (a20_check()) {
            return 1;
        }
    }

    /* Fall back to keyboard controller method */
    if (a20_enable_keyboard()) {
        if (a20_check()) {
            return 1;
        }
    }

    return 0;  /* A20 enable failed */
}

int cpu_detect_cpuid(void) {
    /* CPUID available if we can toggle the ID flag (bit 21) in EFLAGS */
    uint32_t eflags;
    __asm__ volatile(
        "pushfl\n"
        "popl %0\n"
        : "=r"(eflags)
    );

    uint32_t old_eflags = eflags;

    /* Toggle the ID flag */
    eflags ^= (1 << 21);

    __asm__ volatile(
        "pushl %0\n"
        "popfl\n"
        "pushfl\n"
        "popl %0\n"
        : "+r"(eflags)
        :
        : "memory"
    );

    return (eflags != old_eflags);
}

void cpu_query_features(void) {
    if (!cpu_info.has_cpuid) {
        memset(&cpu_info, 0, sizeof(cpu_info));
        return;
    }

    /* Query vendor string (CPUID leaf 0) */
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0));

    /* Store vendor string (stored in EBX, EDX, ECX) */
    memcpy(cpu_info.vendor + 0, &ebx, 4);
    memcpy(cpu_info.vendor + 4, &edx, 4);
    memcpy(cpu_info.vendor + 8, &ecx, 4);
    cpu_info.vendor[12] = '\0';

    /* Query features (CPUID leaf 1) */
    if (eax >= 1) {
        uint32_t f_eax, f_ebx;
        __asm__ volatile("cpuid"
            : "=a"(f_eax), "=b"(f_ebx), "=c"(ecx), "=d"(edx)
            : "a"(1));

        cpu_info.family    = (f_eax >> 8) & 0xF;
        cpu_info.model     = (f_eax >> 4) & 0xF;
        cpu_info.stepping  = f_eax & 0xF;

        /* Handle extended family/model for newer CPUs */
        if (cpu_info.family == 0xF) {
            cpu_info.family += (f_eax >> 20) & 0xFF;
        }
        if (cpu_info.family >= 6) {
            cpu_info.model |= (f_eax >> 12) & 0xF0;
        }

        cpu_info.features_ecx = ecx;
        cpu_info.features_edx = edx;
        UNUSED(f_ebx);

        cpu_info.has_fpu  = (edx & CPUID_FEAT_EDX_FPU) ? 1 : 0;
        cpu_info.has_sse  = (edx & CPUID_FEAT_EDX_SSE) ? 1 : 0;
        cpu_info.has_sse2 = (edx & CPUID_FEAT_EDX_SSE2) ? 1 : 0;
        cpu_info.has_msr  = (edx & CPUID_FEAT_EDX_MSR) ? 1 : 0;
        cpu_info.has_apic = (edx & CPUID_FEAT_EDX_APIC) ? 1 : 0;
    }

    /* Query brand string (CPUID leaf 0x80000002-0x80000004) */
    memset(cpu_info.brand, 0, sizeof(cpu_info.brand));

    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000000));

    if (eax >= 0x80000004) {
        uint32_t *brand = (uint32_t*)cpu_info.brand;

        __asm__ volatile("cpuid"
            : "=a"(brand[0]), "=b"(brand[1]), "=c"(brand[2]), "=d"(brand[3])
            : "a"(0x80000002));

        __asm__ volatile("cpuid"
            : "=a"(brand[4]), "=b"(brand[5]), "=c"(brand[6]), "=d"(brand[7])
            : "a"(0x80000003));

        __asm__ volatile("cpuid"
            : "=a"(brand[8]), "=b"(brand[9]), "=c"(brand[10]), "=d"(brand[11])
            : "a"(0x80000004));
    }
}

void fpu_init(void) {
    /* Clear EM (Emulation) bit, set MP (Monitor Coprocessor) bit */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);  /* Clear EM */
    cr0 |=  (1 << 1);  /* Set MP */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    /* Initialize FPU */
    __asm__ volatile("fninit");
}

int sse_enable(void) {
    if (!cpu_info.has_sse && !cpu_info.has_sse2) {
        return 0;
    }

    /* Set OSFXSR and OSXMMEXCPT bits in CR4 */
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);   /* OSFXSR — Enable FXSAVE/FXRSTOR */
    cr4 |= (1 << 10);  /* OSXMMEXCPT — Enable unmasked SSE exceptions */
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4) : "memory");

    return 1;
}

void cpu_init(void) {
    memset(&cpu_info, 0, sizeof(cpu_info));

    /* Step 1: Enable A20 gate (critical for memory above 1MB) */
    a20_enable();

    /* Step 2: Detect CPUID availability */
    cpu_info.has_cpuid = cpu_detect_cpuid();

    /* Step 3: Query CPU features */
    cpu_query_features();

    /* Step 4: Initialize FPU if present */
    if (cpu_info.has_fpu) {
        fpu_init();
    }

    /* Step 5: Enable SSE if supported */
    if (cpu_info.has_sse || cpu_info.has_sse2) {
        sse_enable();
    }
}

const char* cpu_get_vendor(void) {
    return cpu_info.vendor;
}

const char* cpu_get_brand(void) {
    return cpu_info.brand;
}
