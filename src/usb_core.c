#include "usb.h"
#include "uart.h"
#include "timer.h"
#include "usb_dwc2.h"

usb_device_t usb_dev = {0};
static usb_state_t usb_state = USB_STATE_IDLE;

static uint8_t dev_desc[64];
static uint8_t cfg_desc[256];

void usb_init(void) {
    dwc2_init();
    usb_state = USB_STATE_PORT_RESET;
    usb_dev.ready = false;
    uart_puts("USB: init\n");
}

void usb_poll(void) {
    dwc2_poll();

    switch (usb_state) {

    case USB_STATE_PORT_RESET:
        if (dwc2_port_connected()) {
            uart_puts("USB: port reset\n");
            dwc2_port_reset();
            usb_state = USB_STATE_GET_DEV_DESC_SHORT;
        }
        break;

    case USB_STATE_GET_DEV_DESC_SHORT:
        if (usb_ctrl_get_descriptor(1, 0, dev_desc, 8)) {
            uart_puts("USB: short dev desc OK\n");
            usb_state = USB_STATE_SET_ADDRESS;
        }
        break;

    case USB_STATE_SET_ADDRESS:
        if (usb_ctrl_set_address(1)) {
            uart_puts("USB: address set\n");
            dwc2_set_address(1);
            usb_state = USB_STATE_GET_DEV_DESC_FULL;
        }
        break;

    case USB_STATE_GET_DEV_DESC_FULL:
        if (usb_ctrl_get_descriptor(1, 0, dev_desc, 18)) {
            uint16_t vid = dev_desc[8] | (dev_desc[9] << 8);
            uint16_t pid = dev_desc[10] | (dev_desc[11] << 8);

            if (vid == USB_VID_CANABLE && pid == USB_PID_CANABLE) {
                uart_puts("USB: CANable detected\n");
                usb_state = USB_STATE_GET_CONFIG_DESC;
            } else {
                uart_puts("USB: unknown device\n");
                usb_state = USB_STATE_IDLE;
            }
        }
        break;

    case USB_STATE_GET_CONFIG_DESC:
        if (usb_ctrl_get_descriptor(2, 0, cfg_desc, 256)) {
            uart_puts("USB: config desc OK\n");

            // Parse endpoints for gs_usb
            for (int i = 0; i < 256; ) {
                uint8_t len = cfg_desc[i];
                uint8_t type = cfg_desc[i+1];

                if (len == 0) break;

                if (type == 5) { // endpoint descriptor
                    uint8_t ep = cfg_desc[i+2];
                    uint16_t maxpkt = cfg_desc[i+4] | (cfg_desc[i+5] << 8);

                    if (ep & 0x80) {
                        // IN endpoint
                        if ((ep & 0x0F) == 1) {
                            usb_dev.bulk_in_ep = ep;
                            usb_dev.bulk_in_maxpkt = maxpkt;
                        } else {
                            usb_dev.intr_in_ep = ep;
                            usb_dev.intr_in_maxpkt = maxpkt;
                        }
                    } else {
                        // OUT endpoint
                        usb_dev.bulk_out_ep = ep;
                        usb_dev.bulk_out_maxpkt = maxpkt;
                    }
                }

                i += len;
            }

            usb_state = USB_STATE_SET_CONFIGURATION;
        }
        break;

    case USB_STATE_SET_CONFIGURATION:
        if (usb_ctrl_set_configuration(1)) {
            uart_puts("USB: configuration set\n");
            usb_state = USB_STATE_READY;
            usb_dev.ready = true;
        }
        break;

    case USB_STATE_READY:
        // Nothing to do here — gs_usb.c will drive transfers
        break;

    default:
        break;
    }
}

// ------------------------------------------------------------
// Control Transfers
// ------------------------------------------------------------
bool usb_ctrl_get_descriptor(uint8_t desc_type, uint8_t desc_index,
                             void *buf, uint16_t len)
{
    return dwc2_ctrl_xfer(0x80, 6, (desc_type << 8) | desc_index,
                          0, buf, len);
}

bool usb_ctrl_set_address(uint8_t addr) {
    return dwc2_ctrl_xfer(0x00, 5, addr, 0, 0, 0);
}

bool usb_ctrl_set_configuration(uint8_t cfg) {
    return dwc2_ctrl_xfer(0x00, 9, cfg, 0, 0, 0);
}

// ------------------------------------------------------------
// Bulk + Interrupt wrappers
// ------------------------------------------------------------
int usb_bulk_in(uint8_t ep, uint8_t *buf, uint16_t len) {
    return dwc2_bulk_in(ep, buf, len);
}

int usb_bulk_out(uint8_t ep, const uint8_t *buf, uint16_t len) {
    return dwc2_bulk_out(ep, buf, len);
}

int usb_intr_in(uint8_t ep, uint8_t *buf, uint16_t len) {
    return dwc2_intr_in(ep, buf, len);
}
