#include "peripherals.h"
#include "timer.h"

#define SYS_TIMER_CLO  (TIMER_BASE + 0x04)
#define SYS_TIMER_CHI  (TIMER_BASE + 0x08)

void timer_init(void) {
    // System timer needs no configuration
}

uint64_t timer_get_counter(void) {
    uint32_t hi = mmio_read(SYS_TIMER_CHI);
    uint32_t lo = mmio_read(SYS_TIMER_CLO);
    if (mmio_read(SYS_TIMER_CHI) != hi) {
        hi = mmio_read(SYS_TIMER_CHI);
        lo = mmio_read(SYS_TIMER_CLO);
    }
    return ((uint64_t)hi << 32) | lo;
}

void timer_delay_us(uint32_t us) {
    uint64_t start = timer_get_counter();
    uint64_t target = start + us;
    while (timer_get_counter() < target) { }
}
