#pragma once
#include <stdint.h>

#define PERIPH_BASE        0x3F000000UL

// GPIO
#define GPIO_BASE          (PERIPH_BASE + 0x200000)

// UART0
#define UART0_BASE         (PERIPH_BASE + 0x201000)

// System Timer
#define TIMER_BASE         (PERIPH_BASE + 0x003000)

// Interrupt Controller (GIC-400)
#define GIC_DIST_BASE      (PERIPH_BASE + 0x00B000)
#define GIC_CPU_BASE       (PERIPH_BASE + 0x00C000)

// Mailbox
#define MBOX_BASE          (PERIPH_BASE + 0x00B880)

// USB (DWC2 host controller)
#define USB_BASE           (PERIPH_BASE + 0x980000)

#define SPI0_BASE        (PERIPH_BASE + 0x204000)

// MMIO helpers
static inline void mmio_write(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t mmio_read(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

