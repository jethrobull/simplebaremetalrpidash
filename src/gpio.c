#include "peripherals.h"
#include "gpio.h"

#define GPFSEL(pin)   (GPIO_BASE + ((pin) / 10) * 4)
#define GPSET0        (GPIO_BASE + 0x1C)
#define GPCLR0        (GPIO_BASE + 0x28)

void gpio_set_alt(uint32_t pin, uint32_t alt) {
    uintptr_t reg = GPFSEL(pin);
    uint32_t shift = (pin % 10) * 3;
    uint32_t val = mmio_read(reg);
    val &= ~(7u << shift);
    val |= (alt & 7u) << shift;
    mmio_write(reg, val);
}

void gpio_set_output(uint32_t pin) {
    uintptr_t reg = GPFSEL(pin);
    uint32_t shift = (pin % 10) * 3;
    uint32_t val = mmio_read(reg);
    val &= ~(7u << shift);
    val |= (1u << shift); // output
    mmio_write(reg, val);
}

void gpio_write(uint32_t pin, uint32_t value) {
    if (value)
        mmio_write(GPSET0, 1u << pin);
    else
        mmio_write(GPCLR0, 1u << pin);
}
