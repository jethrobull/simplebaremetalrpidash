#pragma once
#include <stdint.h>

void gic_init(void);
void gic_enable_irq(uint32_t int_id);
void gic_disable_irq(uint32_t int_id);
void irq_handler(void);
