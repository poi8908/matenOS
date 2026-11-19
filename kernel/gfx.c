#include "gfx.h"
#include "zh.h"
#include "serial.h"
#include "string.h"

int gfx_ready;
int gfx_width;
int gfx_height;

static volatile uint32_t *framebuffer;
static int pitch_pixels;
static int fb_width;
static int fb_height;
static int fb_scale = 1;
static int fb_y_offset;
static int fb_x_offset;

void gfx_init(void)
{
    uint16_t width = *(volatile uint16_t *)0x5100;
    uint16_t height = *(volatile uint16_t *)0x5102;
    uint8_t bits = *(volatile uint8_t *)0x5104;
    uint16_t pitch = *(volatile uint16_t *)0x5106;
    uint32_t address = *(volatile uint32_t *)0x5108;

    gfx_ready = 0;
    fb_width = width;
    fb_height = height;
    gfx_width = width;
    gfx_height = height;
    if (width >= 640 && height >= 480) {
        gfx_width = 640;
        gfx_height = 480;
    }
    {
        int sx = gfx_width > 0 ? width / gfx_width : 1;
        int sy = gfx_height > 0 ? height / gfx_height : 1;
        fb_scale = sx < sy ? sx : sy;
    }
    if (fb_scale < 1)
        fb_scale = 1;
    fb_x_offset = (width - gfx_width * fb_scale) / 2;
    fb_y_offset = (height - gfx_height * fb_scale) / 2;
    if (fb_x_offset < 0)
        fb_x_offset = 0;
    if (fb_y_offset < 0)
        fb_y_offset = 0;

    serial_write("video: ");
    serial_write("VBE ");
    if (bits != 32 || width == 0 || height == 0) {
        serial_write("unsupported\n");
        return;
    }

    framebuffer = (volatile uint32_t *)address;
    pitch_pixels = pitch / 4;
    if (pitch_pixels < width)
        pitch_pixels = width;
    gfx_ready = 1;

    {
        int yy;
        int xx;
        for (yy = 0; yy < fb_height; yy++)
            for (xx = 0; xx < fb_width; xx++)
                framebuffer[yy * pitch_pixels + xx] = 0x00101820;
    }

    serial_write("640x480x32 ready\n");
}

void gfx_clear(uint32_t color)
{
    int i;
    uint32_t *buffer = GFX_BACKBUFFER;

    for (i = 0; i < gfx_width * gfx_height; i++)
        buffer[i] = color;
}

void gfx_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 || x >= gfx_width || y >= gfx_height)
        return;
    GFX_BACKBUFFER[y * gfx_width + x] = color;
}

void gfx_fill(int x, int y, int width, int height, uint32_t color)
{
    int px;
    int py;

    for (py = y; py < y + height; py++) {
        if (py < 0 || py >= gfx_height)
            continue;
        for (px = x; px < x + width; px++) {
            if (px < 0 || px >= gfx_width)
                continue;
            GFX_BACKBUFFER[py * gfx_width + px] = color;
        }
    }
}

void gfx_rect(int x, int y, int width, int height, uint32_t color)
{
    gfx_hline(x, y, width, color);
    gfx_hline(x, y + height - 1, width, color);
    gfx_vline(x, y, height, color);
    gfx_vline(x + width - 1, y, height, color);
}

void gfx_hline(int x, int y, int width, uint32_t color)
{
    int i;

    if (y < 0 || y >= gfx_height)
        return;
    for (i = 0; i < width; i++) {
        int px = x + i;
        if (px >= 0 && px < gfx_width)
            GFX_BACKBUFFER[y * gfx_width + px] = color;
    }
}

void gfx_vline(int x, int y, int height, uint32_t color)
{
    int i;

    if (x < 0 || x >= gfx_width)
        return;
    for (i = 0; i < height; i++) {
        int py = y + i;
        if (py >= 0 && py < gfx_height)
            GFX_BACKBUFFER[py * gfx_width + x] = color;
    }
}

void gfx_char(int x, int y, char c, uint32_t color, int scale)
{
    const uint8_t *glyph = GFX_FONT + (uint8_t)c * 8;
    int row;
    int column;

    if (!gfx_ready || scale < 1)
        return;

    for (row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (column = 0; column < 8; column++) {
            if (bits & (0x80 >> column)) {
                int px = x + column * scale;
                int py = y + row * scale;
                gfx_fill(px, py, scale, scale, color);
            }
        }
    }
}

void gfx_text(int x, int y, const char *text, uint32_t color, int scale)
{
    int start = x;

    if (zh_language) {
        const char *translated = zh_tr(text);
        if (translated != text) {
            zh_text(x, y, translated, color, scale > 1 ? 1 : scale);
            return;
        }
    }

    while (*text) {
        if (*text == '\n') {
            x = start;
            y += 8 * scale;
            text++;
            continue;
        }
        gfx_char(x, y, *text, color, scale);
        x += 8 * scale;
        text++;
    }
}

int gfx_text_width(const char *text, int scale)
{
    if (zh_language) {
        const char *translated = zh_tr(text);
        if (translated != text)
            return zh_text_width(translated, scale > 1 ? 1 : scale);
    }
    return (int)strlen(text) * 8 * scale;
}

void gfx_blit(void)
{
    int y;
    int count;

    if (!gfx_ready)
        return;

    if (fb_scale > 1) {
        int sy;
        int sx;
        int dy;
        int dx;
        for (sy = 0; sy < gfx_height; sy++) {
            for (sx = 0; sx < gfx_width; sx++) {
                uint32_t color = GFX_BACKBUFFER[sy * gfx_width + sx];
                for (dy = 0; dy < fb_scale; dy++) {
                    volatile uint32_t *dst =
                        framebuffer + (sy * fb_scale + dy + fb_y_offset) * pitch_pixels + fb_x_offset + sx * fb_scale;
                    for (dx = 0; dx < fb_scale; dx++)
                        dst[dx] = color;
                }
            }
        }
        return;
    }

    count = gfx_width * 4;
    for (y = 0; y < gfx_height; y++) {
        memcpy((void *)(framebuffer + y * pitch_pixels),
               (const void *)(GFX_BACKBUFFER + y * gfx_width),
               (size_t)count);
    }
}



