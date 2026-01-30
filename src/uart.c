#include "peripherals.h"
#include "gpio.h"
#include "uart.h"

#define UART0_DR    (UART0_BASE + 0x00)
#define UART0_FR    (UART0_BASE + 0x18)
#define UART0_IBRD  (UART0_BASE + 0x24)
#define UART0_FBRD  (UART0_BASE + 0x28)
#define UART0_LCRH  (UART0_BASE + 0x2C)
#define UART0_CR    (UART0_BASE + 0x30)
#define UART0_ICR   (UART0_BASE + 0x44)

void uart_init(void) {
    mmio_write(UART0_CR, 0x00000000);

    // GPIO14/15 → ALT0 (TXD0/RXD0)
    gpio_set_alt(14, 0);
    gpio_set_alt(15, 0);

    mmio_write(UART0_ICR, 0x7FF);

    // 115200 baud @ 3MHz UARTCLK
    mmio_write(UART0_IBRD, 1);
    mmio_write(UART0_FBRD, 40);

    mmio_write(UART0_LCRH, (3 << 5) | (1 << 4)); // 8N1, FIFO
    mmio_write(UART0_CR, (1 << 9) | (1 << 8) | 1);
}

void uart_putc(char c) {
    while (mmio_read(UART0_FR) & (1 << 5)) { }
    mmio_write(UART0_DR, c);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}
