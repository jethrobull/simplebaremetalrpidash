    .arch armv8-a
    .section .text.boot
    .align  7
    .global _start

_start:
    // Determine current exception level
    mrs     x0, CurrentEL
    lsr     x0, x0, #2        // shift to get EL number

    cmp     x0, #1
    beq     1f                // already EL1 → skip drop


    bl      drop_to_el1

1:
    // Set up stack
    ldr     x0, =_stack_top
    mov     sp, x0

    // Zero BSS
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
0:
    cmp     x1, x2
    b.hs    1f
    str     xzr, [x1], #8
    b       0b
1:

    // Install vector table
    ldr     x0, =vector_table
    msr     VBAR_EL1, x0
    isb

    bl      main

hang:
    wfe
    b       hang


// ------------------------------------------------------------
// Drop from EL2 → EL1h
// ------------------------------------------------------------
drop_to_el1:
    // HCR_EL2: RW=1 → AArch64 EL1
    mov     x0, #(1 << 31)
    msr     HCR_EL2, x0

    // SCTLR_EL1: minimal
    mov     x0, #0
    msr     SCTLR_EL1, x0

    // Set SP_EL1 = current SP
    mov     x0, sp
    msr     SP_EL1, x0

    // Return address after ERET
    adr     x0, 2f
    msr     ELR_EL2, x0

    // SPSR_EL2: EL1h, interrupts enabled
    mov     x0, #(0b0101)
    msr     SPSR_EL2, x0

    eret
2:
    ret


// ------------------------------------------------------------
// Exception Vector Table
// ------------------------------------------------------------
    .align 11
vector_table:
    // EL1t
    b sync_el1t
    b irq_el1t
    b fiq_el1t
    b serr_el1t

    // EL1h
    b sync_el1h
    b irq_el1h
    b fiq_el1h
    b serr_el1h

    // EL0 64-bit
    b sync_el0_64
    b irq_el0_64
    b fiq_el0_64
    b serr_el0_64

    // EL0 32-bit
    b sync_el0_32
    b irq_el0_32
    b fiq_el0_32
    b serr_el0_32


// ------------------------------------------------------------
// Exception stubs
// ------------------------------------------------------------
    .macro VEC name
\name:
    b .
    .endm

    VEC sync_el1t
    VEC irq_el1t
    VEC fiq_el1t
    VEC serr_el1t

sync_el1h:
    bl irq_handler
    eret

irq_el1h:
    b .

    VEC fiq_el1h
    VEC serr_el1h

    VEC sync_el0_64
    VEC irq_el0_64
    VEC fiq_el0_64
    VEC serr_el0_64

    VEC sync_el0_32
    VEC irq_el0_32
    VEC fiq_el0_32
    VEC serr_el0_32

    .extern main
    .extern irq_handler
    .extern __bss_start
    .extern __bss_end
    .global _stack_top
