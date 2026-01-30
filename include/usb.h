#pragma once
#include <stdint.h>
#include <stdbool.h>

#define USB_VID_CANABLE   0x1D50
#define USB_PID_CANABLE   0x606F

typedef enum {
    USB_STATE_IDLE,
    USB_STATE_PORT_RESET,
    USB_STATE_GET_DEV_DESC_SHORT,
    USB_STATE_SET_ADDRESS,
    USB_STATE_GET_DEV_DESC_FULL,
    USB_STATE_GET_CONFIG_DESC,
    USB_STATE_SET_CONFIGURATION,
    USB_STATE_READY
} usb_state_t;

typedef struct {
    uint8_t bulk_in_ep;
    uint8_t bulk_out_ep;
    uint8_t intr_in_ep;

    uint16_t bulk_in_maxpkt;
    uint16_t bulk_out_maxpkt;
    uint16_t intr_in_maxpkt;

    bool ready;
} usb_device_t;

extern usb_device_t usb_dev;

void usb_init(void);
void usb_poll(void);

// Control transfers
bool usb_ctrl_get_descriptor(uint8_t desc_type, uint8_t desc_index,
                             void *buf, uint16_t len);

bool usb_ctrl_set_address(uint8_t addr);
bool usb_ctrl_set_configuration(uint8_t cfg);

// Bulk transfers
int usb_bulk_in(uint8_t ep, uint8_t *buf, uint16_t len);
int usb_bulk_out(uint8_t ep, const uint8_t *buf, uint16_t len);

// Interrupt transfers
int usb_intr_in(uint8_t ep, uint8_t *buf, uint16_t len);
