#pragma once
#include <stdint.h>
#include <stdbool.h>

bool mbox_call(uint8_t ch, volatile uint32_t *mbox);
