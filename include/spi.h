#pragma once
#include <stdint.h>

void spi_init(void);
uint8_t spi_transfer(uint8_t v);
void spi_cs_low(void);
void spi_cs_high(void);
