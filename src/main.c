#include "mcp2515.h"
#include "framebuffer.h"
#include "uart.h"
#include "timer.h"
#include "gauges.h"
#include <stdio.h>

#define MAX_LOG_LINES 30

// Set this to the CAN ID that carries RPM
#define RPM_CAN_ID 0x0CFF1234

static char log_lines[MAX_LOG_LINES][64];
static int log_head = 0;

static int fmt_u32_dec(char *buf, uint32_t v) {
    char tmp[10];
    int i = 0;

    if (v == 0) {
        buf[0] = '0';
        return 1;
    }

    while (v > 0) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }

    for (int j = 0; j < i; j++)
        buf[j] = tmp[i - j - 1];

    return i;
}

static int fmt_u32_hex(char *buf, uint32_t v, int width) {
    static const char hex[] = "0123456789ABCDEF";
    char tmp[8];
    int i = 0;

    do {
        tmp[i++] = hex[v & 0xF];
        v >>= 4;
    } while (v && i < 8);

    while (i < width)
        tmp[i++] = '0';

    for (int j = 0; j < i; j++)
        buf[j] = tmp[i - j - 1];

    return i;
}



// ------------------------------------------------------------
// CAN logging
// ------------------------------------------------------------

static void log_can_frame(const can_frame_t *f) {
    char *line = log_lines[log_head];
    log_head = (log_head + 1) % MAX_LOG_LINES;

    int n = 0;

    // ID: 3‑digit hex
    n += fmt_u32_hex(line + n, f->id, 3);
    line[n++] = ' ';

    // DLC: decimal
    n += fmt_u32_dec(line + n, f->dlc);
    line[n++] = ' ';

    // Data bytes
    for (uint8_t i = 0; i < f->dlc && n < 64 - 3; i++) {
        n += fmt_u32_hex(line + n, f->data[i], 2);
        line[n++] = ' ';
    }

    line[n] = 0;
}


static void draw_log(framebuffer_t *fb) {
    fb_fill_rect(fb, 0, 0, fb->width, fb->height, 0x00000000);

    int idx = log_head;
    uint32_t y = 10;

    for (int i = 0; i < MAX_LOG_LINES; i++) {
        const char *line = log_lines[idx];
        fb_draw_text(fb, 10, y, line, 0x00FFFFFF);
        y += 12;
        idx = (idx + 1) % MAX_LOG_LINES;
    }
}

// ------------------------------------------------------------
// RPM decoding (common format: ((A<<8)|B)/4 )
// ------------------------------------------------------------
static int decode_rpm(const can_frame_t *f) {
    if (f->dlc < 2)
        return 0;

    uint16_t raw = (f->data[0] << 8) | f->data[1];
    return raw / 4;
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
void main(void) {
    uart_init();
    timer_init();

    framebuffer_t fb;
    fb_init(&fb, 800, 480, 32);
    fb_clear(&fb, 0x00000000);

    uart_puts("CAN analyser starting\n");

    if (!mcp2515_init(MCP_XTAL_16MHZ, MCP_BITRATE_500K)) {
        uart_puts("MCP2515 init failed\n");
        while (1) { }
    }

    can_frame_t rx;
    int rpm_value = 0;

    while (1) {
        if (mcp2515_recv(&rx)) {

            // Log every frame
            log_can_frame(&rx);
            draw_log(&fb);

            // Check if this frame contains RPM
            if (rx.id == RPM_CAN_ID) {
                rpm_value = decode_rpm(&rx);
            }

            // Draw RPM gauge
            draw_rpm_gauge(&fb, 400, 240, 150, rpm_value);
        }

        timer_delay_us(1000);
    }
}
