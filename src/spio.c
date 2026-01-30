#include "peripherals.h"
#include "gpio.h"
#include "spi.h"

#define SPI0_CS    (SPI0_BASE + 0x00)
#define SPI0_FIFO  (SPI0_BASE + 0x04)
#define SPI0_CLK   (SPI0_BASE + 0x08)

#define SPI0_CS_TA   (1 << 7)
#define SPI0_CS_CLEAR (3 << 4)

void spi_init(void) {
    // GPIO7-11 to ALT0 for SPI0
    gpio_set_alt(7, 0);
    gpio_set_alt(8, 0);
    gpio_set_alt(9, 0);
    gpio_set_alt(10, 0);
    gpio_set_alt(11, 0);

    // Clear FIFOs, set mode 0, use CS0
    mmio_write(SPI0_CS, SPI0_CS_CLEAR);
    // Clock divider: 250MHz / 64 ≈ 3.9MHz
    mmio_write(SPI0_CLK, 64);
}

static void spi_begin(void) {
    uint32_t cs = mmio_read(SPI0_CS);
    cs |= SPI0_CS_TA;
    mmio_write(SPI0_CS, cs);
}

static void spi_end(void) {
    uint32_t cs = mmio_read(SPI0_CS);
    cs &= ~SPI0_CS_TA;
    mmio_write(SPI0_CS, cs);
}

uint8_t spi_transfer(uint8_t v) {
    spi_begin();
    // Clear FIFOs
    mmio_write(SPI0_CS, mmio_read(SPI0_CS) | SPI0_CS_CLEAR);

    // Write byte
    mmio_write(SPI0_FIFO, v);

    // Wait for done
    while (!(mmio_read(SPI0_CS) & (1 << 16))) { } // DONE

    uint8_t r = (uint8_t)mmio_read(SPI0_FIFO);
    spi_end();
    return r;
}

// If you want explicit CS control via GPIO instead of HW CS:
void spi_cs_low(void) {
    // e.g. use GPIO8 as manual CS if desired
    // gpio_write(8, 0);
}

void spi_cs_high(void) {
    // gpio_write(8, 1);
}
