#pragma once
#include <stdint.h>

void gpio_set_alt(uint32_t pin, uint32_t alt);
void gpio_set_output(uint32_t pin);
void gpio_write(uint32_t pin, uint32_t value);
