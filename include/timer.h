#pragma once
#include <stdint.h>

void timer_init(void);
void timer_delay_us(uint32_t us);
uint64_t timer_get_counter(void);
