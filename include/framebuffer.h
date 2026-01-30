#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t is_rgb;
    volatile uint8_t *buf;
} framebuffer_t;

bool fb_init(framebuffer_t *fb, uint32_t w, uint32_t h, uint32_t depth);

void fb_clear(framebuffer_t *fb, uint32_t color);
void fb_put_pixel(framebuffer_t *fb, uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(framebuffer_t *fb, uint32_t x, uint32_t y,
                  uint32_t w, uint32_t h, uint32_t color);
void fb_draw_char(framebuffer_t *fb, uint32_t x, uint32_t y, char c, uint32_t color);
void fb_draw_text(framebuffer_t *fb, uint32_t x, uint32_t y, const char *s, uint32_t color);

/* Draw a line (Bresenham) */
void fb_draw_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, uint32_t color);
/* Draw an arc (for gauge outlines) */
void fb_draw_arc(framebuffer_t *fb, int cx, int cy, int r, int start_deg, int end_deg, uint32_t color);

extern const int16_t sin_table[360];

