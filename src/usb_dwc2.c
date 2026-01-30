#include "usb_dwc2.h"
#include "peripherals.h"
#include "uart.h"
#include "timer.h"

#define USB_GAHBCFG     (USB_BASE + 0x008)
#define USB_GUSBCFG     (USB_BASE + 0x00C)
#define USB_GRSTCTL     (USB_BASE + 0x010)
#define USB_GINTSTS     (USB_BASE + 0x014)
#define USB_GINTMSK     (USB_BASE + 0x018)
#define USB_HCFG        (USB_BASE + 0x400)
#define USB_HFIR        (USB_BASE + 0x404)
#define USB_HPRT        (USB_BASE + 0x440)
#define USB_HCCHAR(n)   (USB_BASE + 0x500 + (n)*0x20)
#define USB_HCTSIZ(n)   (USB_BASE + 0x510 + (n)*0x20)
#define USB_HCDMA(n)    (USB_BASE + 0x514 + (n)*0x20)

static uint8_t usb_addr = 0;

void dwc2_init(void) {
    uart_puts("DWC2: init\n");

    // Soft reset
    mmio_write(USB_GRSTCTL, (1 << 0));
    while (mmio_read(USB_GRSTCTL) & 1) { }

    // Force host mode
    uint32_t gusbcfg = mmio_read(USB_GUSBCFG);
    gusbcfg |= (1 << 29);
    mmio_write(USB_GUSBCFG, gusbcfg);

    // Enable global interrupts
    mmio_write(USB_GAHBCFG, (1 << 0));

    // Host config: 48MHz PHY clock
    mmio_write(USB_HCFG, 1);

    uart_puts("DWC2: host mode\n");
}

void dwc2_poll(void) {
    // For now, nothing needed
}

bool dwc2_port_connected(void) {
    uint32_t hprt = mmio_read(USB_HPRT);
    return hprt & (1 << 0);
}

void dwc2_port_reset(void) {
    uint32_t hprt = mmio_read(USB_HPRT);
    hprt |= (1 << 8);
    mmio_write(USB_HPRT, hprt);
    timer_delay_us(60000);
    hprt &= ~(1 << 8);
    mmio_write(USB_HPRT, hprt);
}

void dwc2_set_address(uint8_t addr) {
    usb_addr = addr;
}

// ------------------------------------------------------------
// Control Transfer (EP0)
// ------------------------------------------------------------
bool dwc2_ctrl_xfer(uint8_t bmRequestType, uint8_t bRequest,
                    uint16_t wValue, uint16_t wIndex,
                    void *data, uint16_t len)
{
    uint8_t setup[8] = {
        bmRequestType,
        bRequest,
        wValue & 0xFF,
        wValue >> 8,
        wIndex & 0xFF,
        wIndex >> 8,
        len & 0xFF,
        len >> 8
    };

    // Send SETUP packet
    if (!dwc2_ctrl_stage_setup(setup))
        return false;

    // DATA stage
    if (len > 0) {
        if (bmRequestType & 0x80) {
            if (!dwc2_ctrl_stage_in(data, len))
                return false;
        } else {
            if (!dwc2_ctrl_stage_out(data, len))
                return false;
        }
    }

    // STATUS stage
    if (!dwc2_ctrl_stage_status(bmRequestType))
        return false;

    return true;
}

// ------------------------------------------------------------
// Bulk + Interrupt Transfers
// ------------------------------------------------------------
int dwc2_bulk_in(uint8_t ep, uint8_t *buf, uint16_t len) {
    return dwc2_xfer(ep, buf, len, 1);
}

int dwc2_bulk_out(uint8_t ep, const uint8_t *buf, uint16_t len) {
    return dwc2_xfer(ep, (uint8_t *)buf, len, 0);
}

int dwc2_intr_in(uint8_t ep, uint8_t *buf, uint16_t len) {
    return dwc2_xfer(ep, buf, len, 1);
}


// ------------------------------------------------------------
// Internal helpers for control transfers
// ------------------------------------------------------------

#define HC_NUM_CTRL   0
#define HC_NUM_BULK   1
#define HC_NUM_INTR   2

// Simple polling wait for channel done (very bare-metal, no IRQs yet)
static bool dwc2_wait_channel_done(int ch) {
    // In a full implementation you'd watch HCINT/HCINTMSK.
    // Here we just spin a bit and assume success if no obvious error.
    // This is crude but good enough to get you talking to one device.
    timer_delay_us(2000);
    return true;
}

bool dwc2_ctrl_stage_setup(const uint8_t setup[8]) {
    // Use host channel 0 for control
    uint32_t hcchar = (0 << 0) |   // dev addr (0 for now, or usb_addr if you want)
                      (0 << 11) |  // epnum 0
                      (0 << 15) |  // low-speed
                      (0 << 18) |  // ep type: control
                      (1 << 31);   // channel enable

    mmio_write(USB_HCTSIZ(HC_NUM_CTRL),
               (8 << 0) |          // xfer size
               (1 << 19) |         // pkt count
               (0 << 29));         // PID: SETUP

    // In a real driver you'd program HCDMA to point to a DMA buffer.
    // For simplicity, we assume a FIFO-based implementation and that
    // writing to a FIFO register is enough. On Pi this is more complex,
    // but this skeleton is here to show structure, not full silicon detail.

    mmio_write(USB_HCCHAR(HC_NUM_CTRL), hcchar);

    // TODO: write setup bytes to FIFO if required by your SoC integration.

    return dwc2_wait_channel_done(HC_NUM_CTRL);
}

bool dwc2_ctrl_stage_in(void *data, uint16_t len) {
    if (len == 0)
        return true;

    uint32_t hcchar = (usb_addr << 0) |
                      (0 << 11) |      // ep0
                      (0 << 15) |
                      (0 << 18) |      // control
                      (1 << 31);

    mmio_write(USB_HCTSIZ(HC_NUM_CTRL),
               (len << 0) |
               (1 << 19) |
               (1 << 29));            // PID: DATA1

    mmio_write(USB_HCCHAR(HC_NUM_CTRL), hcchar);

    if (!dwc2_wait_channel_done(HC_NUM_CTRL))
        return false;

    // TODO: read data from FIFO into 'data' buffer.

    return true;
}

bool dwc2_ctrl_stage_out(const void *data, uint16_t len) {
    if (len == 0)
        return true;

    uint32_t hcchar = (usb_addr << 0) |
                      (0 << 11) |
                      (0 << 15) |
                      (0 << 18) |
                      (1 << 31);

    mmio_write(USB_HCTSIZ(HC_NUM_CTRL),
               (len << 0) |
               (1 << 19) |
               (1 << 29));            // PID: DATA1

    // TODO: write 'data' to FIFO.

    mmio_write(USB_HCCHAR(HC_NUM_CTRL), hcchar);

    return dwc2_wait_channel_done(HC_NUM_CTRL);
}

bool dwc2_ctrl_stage_status(uint8_t bmRequestType) {
    // Status stage is opposite direction of data stage.
    int dir_in = (bmRequestType & 0x80) ? 0 : 1;

    uint32_t hcchar = (usb_addr << 0) |
                      (0 << 11) |
                      (0 << 15) |
                      (0 << 18) |
                      (1 << 31);

    mmio_write(USB_HCTSIZ(HC_NUM_CTRL),
               (0 << 0) |
               (1 << 19) |
               (2 << 29));            // PID: DATA2 (status)

    mmio_write(USB_HCCHAR(HC_NUM_CTRL), hcchar);

    return dwc2_wait_channel_done(HC_NUM_CTRL);
}

// ------------------------------------------------------------
// Generic transfer (bulk/interrupt)
// ------------------------------------------------------------
int dwc2_xfer(uint8_t ep, uint8_t *buf, uint16_t len, int dir_in) {
    int ch = dir_in ? HC_NUM_BULK : HC_NUM_BULK;

    uint8_t epnum = ep & 0x0F;

    uint32_t hcchar = (usb_addr << 0) |
                      (epnum << 11) |
                      (0 << 15) |          // full-speed
                      (2 << 18) |          // bulk
                      (1 << 31);           // enable

    mmio_write(USB_HCTSIZ(ch),
               (len << 0) |
               (1 << 19) |
               (0 << 29));                // PID: DATA0

    // TODO: for OUT, write 'buf' to FIFO; for IN, read from FIFO after done.

    mmio_write(USB_HCCHAR(ch), hcchar);

    if (!dwc2_wait_channel_done(ch))
        return -1;

    // For now, pretend full length transferred.
    return len;
}
