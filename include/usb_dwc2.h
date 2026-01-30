#pragma once
#include <stdint.h>
#include <stdbool.h>

void dwc2_init(void);
void dwc2_poll(void);

bool dwc2_port_connected(void);
void dwc2_port_reset(void);
void dwc2_set_address(uint8_t addr);

// High-level control transfer
bool dwc2_ctrl_xfer(uint8_t bmRequestType, uint8_t bRequest,
                    uint16_t wValue, uint16_t wIndex,
                    void *data, uint16_t len);

// Bulk / interrupt transfers
int dwc2_bulk_in(uint8_t ep, uint8_t *buf, uint16_t len);
int dwc2_bulk_out(uint8_t ep, const uint8_t *buf, uint16_t len);
int dwc2_intr_in(uint8_t ep, uint8_t *buf, uint16_t len);

// Internal helpers (used only in usb_dwc2.c, but declared here if you want to instrument)
bool dwc2_ctrl_stage_setup(const uint8_t setup[8]);
bool dwc2_ctrl_stage_in(void *data, uint16_t len);
bool dwc2_ctrl_stage_out(const void *data, uint16_t len);
bool dwc2_ctrl_stage_status(uint8_t bmRequestType);

int dwc2_xfer(uint8_t ep, uint8_t *buf, uint16_t len, int dir_in);
