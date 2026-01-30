#include "peripherals.h"
#include "gic.h"
#include "uart.h"

#define GICD_CTLR       (GIC_DIST_BASE + 0x000)
#define GICD_ISENABLER  (GIC_DIST_BASE + 0x100)
#define GICD_ICENABLER  (GIC_DIST_BASE + 0x180)
#define GICD_IPRIORITYR (GIC_DIST_BASE + 0x400)
#define GICD_ITARGETSR  (GIC_DIST_BASE + 0x800)

#define GICC_CTLR       (GIC_CPU_BASE + 0x000)
#define GICC_PMR        (GIC_CPU_BASE + 0x004)
#define GICC_IAR        (GIC_CPU_BASE + 0x00C)
#define GICC_EOIR       (GIC_CPU_BASE + 0x010)

void gic_init(void) {
    mmio_write(GICD_CTLR, 0);

    for (uint32_t i = 0; i < 32; i++) {
        mmio_write(GICD_IPRIORITYR + i * 4, 0xA0A0A0A0);
        mmio_write(GICD_ITARGETSR + i * 4, 0x01010101);
    }

    mmio_write(GICD_CTLR, 1);

    mmio_write(GICC_PMR, 0xFF);
    mmio_write(GICC_CTLR, 1);
}

void gic_enable_irq(uint32_t int_id) {
    uint32_t reg = int_id / 32;
    uint32_t bit = int_id % 32;
    mmio_write(GICD_ISENABLER + reg * 4, 1u << bit);
}

void gic_disable_irq(uint32_t int_id) {
    uint32_t reg = int_id / 32;
    uint32_t bit = int_id % 32;
    mmio_write(GICD_ICENABLER + reg * 4, 1u << bit);
}

void irq_handler(void) {
    uint32_t iar = mmio_read(GICC_IAR);
    uint32_t int_id = iar & 0x3FF;

    uart_puts("IRQ: ");
    uart_putc('0' + (int_id / 100) % 10);
    uart_putc('0' + (int_id / 10) % 10);
    uart_putc('0' + (int_id % 10));
    uart_puts("\n");

    mmio_write(GICC_EOIR, iar);
}
