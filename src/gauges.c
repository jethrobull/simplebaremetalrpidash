#include "gauges.h"
#include "framebuffer.h"
#include <stdint.h>

extern const int16_t sin_table[360];

void draw_rpm_gauge(framebuffer_t *fb, int cx, int cy, int r, int rpm)
{
    // Clear gauge area
    fb_fill_rect(fb, cx - r - 5, cy - r - 5, (r * 2) + 10, (r * 2) + 10, 0x00000000);

    // Draw arc from -120° to +120°
    fb_draw_arc(fb, cx, cy, r, -120, 120, 0x00FFFFFF);

    // Clamp RPM
    if (rpm < 0) rpm = 0;
    if (rpm > 8000) rpm = 8000;

    // Map RPM (0..8000) → angle (-120..120)
    // integer math: angle = -120 + (rpm * 240) / 8000
    int angle = -120 + (rpm * 240) / 8000;

    // Normalize to 0..359
    int a = (angle % 360 + 360) % 360;

    // sin(a)
    int sa = sin_table[a];

    // cos(a) = sin(a - 90) = sin(a + 270)
    int ca = sin_table[(a + 270) % 360];

    // Compute needle endpoint
    int x = cx + (ca * (r - 10)) / 32767;
    int y = cy - (sa * (r - 10)) / 32767;

    // Draw needle
    fb_draw_line(fb, cx, cy, x, y, 0x00FF0000);
}

