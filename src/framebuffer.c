#include "framebuffer.h"
#include "mailbox.h"
#include "peripherals.h"
#include "font8x12.h"
#include <stdint.h>
#include <stdbool.h>

// Integer sine lookup table: sin(deg) * 32767, 0–359°
const int16_t sin_table[360] = {
    0,572,1144,1716,2287,2858,3429,3999,4569,5138,
    5707,6275,6842,7408,7974,8538,9102,9664,10225,10785,
    11343,11900,12455,13009,13561,14111,14659,15205,15749,16291,
    16831,17368,17903,18436,18966,19494,20019,20541,21061,21577,
    22091,22601,23109,23613,24114,24612,25106,25597,26084,26568,
    27048,27525,27998,28467,28932,29394,29851,30305,30755,31200,
    31642,32079,32512,32941,33365,33785,34201,34612,35019,35421,
    35819,36212,36600,36984,37363,37737,38106,38470,38830,39184,
    39533,39877,40216,40550,40879,41202,41520,41833,42140,42442,
    42739,43030,43316,43596,43871,44140,44404,44662,44914,45161,
    45402,45637,45867,46091,46309,46521,46728,46929,47124,47313,
    47496,47673,47845,48010,48169,48323,48470,48612,48747,48877,
    49000,49117,49228,49333,49432,49525,49612,49693,49767,49836,
    49898,49954,50004,50048,50086,50118,50143,50163,50176,50183,
    50184,50179,50168,50151,50128,50099,50064,50023,49976,49923,
    49864,49799,49728,49651,49568,49480,49385,49285,49179,49067,
    48949,48826,48697,48562,48422,48276,48124,47967,47804,47636,
    47462,47283,47098,46908,46713,46512,46306,46095,45879,45657,
    45431,45199,44962,44720,44473,44221,43964,43702,43436,43164,
    42888,42607,42321,42031,41736,41437,41133,40825,40512,40195,
    39874,39548,39218,38884,38546,38204,37858,37508,37154,36796,
    36435,36069,35701,35328,34952,34573,34190,33804,33415,33022,
    32627,32228,31826,31421,31013,30602,30189,29773,29354,28933,
    28509,28083,27654,27223,26790,26355,25917,25477,25036,24592,
    24146,23699,23249,22798,22345,21891,21435,20977,20518,20057,
    19595,19132,18667,18201,17734,17266,16797,16327,15856,15384,
    14911,14438,13963,13488,13013,12537,12060,11583,11106,10628,
    10150,9672,9193,8715,8236,7758,7280,6801,6323,5846,
    5368,4891,4415,3939,3464,2989,2515,2042,1569,1098,
    627,157,-312,-781,-1249,-1716,-2183,-2649,-3114,-3578,
    -4041,-4503,-4964,-5423,-5881,-6338,-6793,-7247,-7699,-8150,
    -8599,-9046,-9491,-9935,-10376,-10816,-11253,-11688,-12121,-12552,
    -12981,-13407,-13831,-14253,-14672,-15089,-15503,-15915,-16324,-16730,
    -17134,-17535,-17933,-18328,-18720,-19109,-19495,-19878,-20258,-20635,
    -21009,-21380,-21747,-22111,-22472,-22829,-23183,-23534,-23881,-24225,
    -24565,-24902,-25235,-25565,-25891,-26214,-26533,-26848,-27160,-27468,
    -27772,-28073,-28369,-28662,-28951,-29236,-29517,-29794,-30067,-30336,
    -30601,-30862,-31119,-31372,-31621,-31865,-32106,-32342,-32574,-32802
};

static inline int iabs(int v) {
    return v < 0 ? -v : v;
}


static volatile uint32_t mbox[36] __attribute__((aligned(16)));


bool fb_init(framebuffer_t *fb, uint32_t w, uint32_t h, uint32_t depth) {
    mbox[0] = 35 * 4;
    mbox[1] = 0;

    mbox[2] = 0x00048003;
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = w;
    mbox[6] = h;

    mbox[7] = 0x00048004;
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = w;
    mbox[11] = h;

    mbox[12] = 0x00048005;
    mbox[13] = 4;
    mbox[14] = 4;
    mbox[15] = depth;

    mbox[16] = 0x00048006;
    mbox[17] = 4;
    mbox[18] = 4;
    mbox[19] = 0; // RGB

    mbox[20] = 0x00040001;
    mbox[21] = 8;
    mbox[22] = 8;
    mbox[23] = 16;
    mbox[24] = 0;

    mbox[25] = 0x00040008;
    mbox[26] = 4;
    mbox[27] = 4;
    mbox[28] = 0;

    mbox[29] = 0;

    if (!mbox_call(8, mbox))
        return false;

    if (mbox[23] == 0 || mbox[28] == 0)
        return false;

    uint32_t fb_addr = mbox[23] & 0x3FFFFFFF;

    fb->buf    = (volatile uint8_t *)(uintptr_t)fb_addr;
    fb->width  = w;
    fb->height = h;
    fb->pitch  = mbox[28];
    fb->is_rgb = 1;

    return true;
}

void fb_clear(framebuffer_t *fb, uint32_t color) {
    for (uint32_t y = 0; y < fb->height; y++) {
        uint32_t *row = (uint32_t *)(fb->buf + y * fb->pitch);
        for (uint32_t x = 0; x < fb->width; x++) {
            row[x] = color;
        }
    }
}

void fb_put_pixel(framebuffer_t *fb, uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height)
        return;

    uint32_t *row = (uint32_t *)(fb->buf + y * fb->pitch);
    row[x] = color;
}

void fb_fill_rect(framebuffer_t *fb, uint32_t x, uint32_t y,
                  uint32_t w, uint32_t h, uint32_t color) {
    if (x >= fb->width || y >= fb->height)
        return;

    if (x + w > fb->width)
        w = fb->width - x;

    if (y + h > fb->height)
        h = fb->height - y;

    for (uint32_t yy = 0; yy < h; yy++) {
        uint32_t *row = (uint32_t *)(fb->buf + (y + yy) * fb->pitch);
        for (uint32_t xx = 0; xx < w; xx++) {
            row[x + xx] = color;
        }
    }
}


void fb_draw_char(framebuffer_t *fb, uint32_t x, uint32_t y, char c, uint32_t color) {
    if (c < 32 || c > 126)
        return;

    const uint8_t *glyph = font8x12[c - 32];

    for (int row = 0; row < 12; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                fb_put_pixel(fb, x + col, y + row, color);
            }
        }
    }
}


void fb_draw_text(framebuffer_t *fb, uint32_t x, uint32_t y, const char *s, uint32_t color) {
    while (*s) {
        fb_draw_char(fb, x, y, *s, color);
        x += 8;
        s++;
    }
}

void fb_draw_line(framebuffer_t *fb, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        fb_put_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


void fb_draw_arc(framebuffer_t *fb, int cx, int cy, int r,
                 int start_deg, int end_deg, uint32_t color)
{
    start_deg = (start_deg % 360 + 360) % 360;
    end_deg   = (end_deg   % 360 + 360) % 360;

    // Oversample 4× for smooth arcs
    for (int a = start_deg * 4; a <= end_deg * 4; a++) {
        int deg = a / 4;

        int sa = sin_table[deg % 360];          // sin(a)
        int ca = sin_table[(deg + 270) % 360];  // cos(a) = sin(a - 90)

        int x = cx + (ca * r) / 32767;
        int y = cy - (sa * r) / 32767;

        fb_put_pixel(fb, x, y, color);
    }
}

