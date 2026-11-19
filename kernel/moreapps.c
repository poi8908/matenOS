#include "moreapps.h"
#include "gfx.h"
#include "string.h"
#include "timer.h"

#define C_TEXT  0x00141C24
#define C_MUTED 0x00607080
#define C_GREEN 0x006EE7A8
#define C_WHITE 0x00FFFFFF
#define C_BLUE  0x004A90D9
#define C_RED   0x00D9534F
#define C_DARK  0x00202830
#define C_BLACK 0x00000000
#define C_PANEL 0x00E8EEF4

/* app ids must match the enum in gui.c: 13..24 */
enum {
    M_CALENDAR = 13, M_TODO, M_ASCII, M_COLORS, M_FONTS,
    M_CONVERT, M_DICE, M_COIN, M_RANDOM, M_INFO, M_STOPWATCH
};

static int todo_done[5];
static int dice_a;
static int dice_b;
static int coin_heads;
static int random_value;
static int convert_value = 1234;
static int stop_running;
static uint32_t stop_start;
static uint32_t stop_elapsed;
static uint32_t rng_state = 0x12345;

static uint32_t rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void num(char *out, int value)
{
    char tmp[12];
    int i = 0;
    int n = 0;
    if (value == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }
    if (value < 0) {
        out[n++] = '-';
        value = -value;
    }
    while (value > 0) {
        tmp[i++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (i > 0)
        out[n++] = tmp[--i];
    out[n] = 0;
}

static void hex(char *out, uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    int i;
    out[0] = '0';
    out[1] = 'x';
    for (i = 0; i < 8; i++)
        out[2 + i] = digits[(value >> (28 - i * 4)) & 0xF];
    out[10] = 0;
}

static void header(int x, int y, const char *text)
{
    gfx_text(x + 6, y + 6, text, C_MUTED, 1);
}

void more_init(int app)
{
    if (app == M_DICE) {
        dice_a = 1 + (int)(rnd() % 6);
        dice_b = 1 + (int)(rnd() % 6);
    } else if (app == M_COIN) {
        coin_heads = (int)(rnd() & 1);
    } else if (app == M_RANDOM) {
        random_value = (int)(rnd() % 100);
    } else if (app == M_STOPWATCH) {
        stop_running = 0;
        stop_elapsed = 0;
    } else if (app == M_TODO) {
        memset(todo_done, 0, sizeof(todo_done));
    }
}

static void draw_dice(int x, int y, int value)
{
    int i;
    int cx;
    int cy;
    for (i = 0; i < value; i++) {
        static const int px[6] = { -1, 0, 1, -1, 0, 1 };
        static const int py[6] = { -1, -1, -1, 1, 1, 1 };
        cx = x + 36 + px[i] * 16;
        cy = y + 36 + py[i] * 16;
        gfx_fill(cx - 5, cy - 5, 10, 10, C_TEXT);
    }
}

static void draw_clock_like(int x, int y, const char *label, uint32_t ticks)
{
    char text[16];
    uint32_t seconds = ticks / 100;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    int p = 0;

    header(x, y, label);
    text[p++] = (char)('0' + (hours / 10) % 10);
    text[p++] = (char)('0' + hours % 10);
    text[p++] = ':';
    text[p++] = (char)('0' + (minutes / 10) % 10);
    text[p++] = (char)('0' + minutes % 10);
    text[p++] = ':';
    text[p++] = (char)('0' + (seconds / 10) % 10);
    text[p++] = (char)('0' + seconds % 10);
    text[p] = 0;
    gfx_text(x + 90, y + 90, text, C_TEXT, 4);
}

void more_draw(int app, int x, int y, int w, int h)
{
    char text[24];
    int i;

    switch (app) {
    case M_CALENDAR:
        header(x, y, "calendar - January 2026");
        gfx_text(x + 30, y + 40, "Su Mo Tu We Th Fr Sa", C_MUTED, 2);
        for (i = 0; i < 31; i++) {
            int col = (i + 4) % 7;
            int row = (i + 4) / 7;
            num(text, i + 1);
            gfx_text(x + 30 + col * 66, y + 70 + row * 40, text, C_TEXT, 3);
        }
        break;
    case M_TODO:
        header(x, y, "todo - click a row");
        for (i = 0; i < 5; i++) {
            gfx_fill(x + 30, y + 36 + i * 44, 470, 36, C_PANEL);
            gfx_rect(x + 30, y + 36 + i * 44, 470, 36, C_MUTED);
            gfx_text(x + 42, y + 46 + i * 44, todo_done[i] ? "[x]" : "[ ]", C_GREEN, 2);
            gfx_text(x + 94, y + 46 + i * 44, "task", C_TEXT, 2);
            if (i == 0) gfx_text(x + 190, y + 46 + i * 44, "write a tiny OS", C_TEXT, 2);
            if (i == 1) gfx_text(x + 190, y + 46 + i * 44, "add 22 programs", C_TEXT, 2);
            if (i == 2) gfx_text(x + 190, y + 46 + i * 44, "test mouse", C_TEXT, 2);
            if (i == 3) gfx_text(x + 190, y + 46 + i * 44, "read license", C_TEXT, 2);
            if (i == 4) gfx_text(x + 190, y + 46 + i * 44, "ship it", C_TEXT, 2);
        }
        break;
    case M_ASCII:
        header(x, y, "ASCII table 32..127");
        for (i = 0; i < 96; i++) {
            int col = i % 8;
            int row = i / 8;
            int code = 32 + i;
            text[0] = (char)code;
            text[1] = 0;
            gfx_text(x + 20 + col * 64, y + 32 + row * 34, text, C_TEXT, 2);
            num(text, code);
            gfx_text(x + 20 + col * 64, y + 52 + row * 34, text, C_MUTED, 1);
        }
        break;
    case M_COLORS:
        header(x, y, "palette - click a color");
        for (i = 0; i < 16; i++) {
            int col = i % 4;
            int row = i / 4;
            gfx_fill(x + 24 + col * 124, y + 38 + row * 64, 112, 52,
                     (uint32_t)(i * 0x101010 + 0x202020));
        }
        gfx_text(x + 24, y + 300, "click a swatch to show its RGB", C_MUTED, 1);
        break;
    case M_FONTS:
        header(x, y, "BIOS 8x8 font");
        for (i = 32; i < 128; i++) {
            int col = (i - 32) % 16;
            int row = (i - 32) / 16;
            text[0] = (char)i;
            text[1] = 0;
            gfx_text(x + 18 + col * 32, y + 34 + row * 58, text, C_TEXT, 3);
        }
        break;
    case M_CONVERT:
        header(x, y, "base converter - click +1 / -1");
        num(text, convert_value);
        gfx_text(x + 20, y + 48, "DEC", C_MUTED, 2);
        gfx_text(x + 100, y + 48, text, C_TEXT, 3);
        hex(text, (uint32_t)convert_value);
        gfx_text(x + 20, y + 110, "HEX", C_MUTED, 2);
        gfx_text(x + 100, y + 110, text, C_TEXT, 3);
        {
            char bits[40];
            int p = 0;
            for (i = 15; i >= 0; i--) {
                bits[p++] = (char)('0' + ((convert_value >> i) & 1));
                if (i % 4 == 0 && i) bits[p++] = ' ';
            }
            bits[p] = 0;
            gfx_text(x + 20, y + 172, "BIN", C_MUTED, 2);
            gfx_text(x + 100, y + 172, bits, C_TEXT, 2);
        }
        gfx_fill(x + 300, y + 240, 100, 40, C_GREEN);
        gfx_text(x + 320, y + 250, "-1", C_BLACK, 2);
        gfx_fill(x + 420, y + 240, 100, 40, C_BLUE);
        gfx_text(x + 440, y + 250, "+1", C_WHITE, 2);
        break;
    case M_DICE:
        header(x, y, "dice - click roll");
        gfx_fill(x + 100, y + 60, 100, 100, C_WHITE);
        gfx_rect(x + 100, y + 60, 100, 100, C_TEXT);
        draw_dice(x + 100, y + 60, dice_a);
        gfx_fill(x + 240, y + 60, 100, 100, C_WHITE);
        gfx_rect(x + 240, y + 60, 100, 100, C_TEXT);
        draw_dice(x + 240, y + 60, dice_b);
        gfx_fill(x + 150, y + 210, 160, 44, C_BLUE);
        gfx_text(x + 190, y + 222, "Roll", C_WHITE, 2);
        break;
    case M_COIN:
        header(x, y, "coin flip - click flip");
        gfx_fill(x + 150, y + 70, 180, 180, coin_heads ? C_GREEN : C_BLUE);
        gfx_text(x + 200, y + 140, coin_heads ? "HEADS" : "TAILS", C_WHITE, 2);
        gfx_fill(x + 150, y + 270, 180, 40, C_RED);
        gfx_text(x + 190, y + 280, "Flip", C_WHITE, 2);
        break;
    case M_RANDOM:
        header(x, y, "random 0..99 - click");
        num(text, random_value);
        gfx_text(x + 180, y + 90, text, C_TEXT, 6);
        gfx_fill(x + 150, y + 240, 200, 44, C_BLUE);
        gfx_text(x + 190, y + 252, "Again", C_WHITE, 2);
        break;
    case M_INFO:
        header(x, y, "system info");
        gfx_text(x + 20, y + 46, "resolution: 1280x960", C_TEXT, 1);
        gfx_text(x + 20, y + 66, "logical UI: 640x480 (2x)", C_TEXT, 1);
        gfx_text(x + 20, y + 86, "timer: PIT 100 Hz", C_TEXT, 1);
        num(text, (int)(timer_ticks() / 100));
        gfx_text(x + 20, y + 106, "uptime seconds:", C_TEXT, 1);
        gfx_text(x + 180, y + 106, text, C_GREEN, 1);
        gfx_text(x + 20, y + 126, "kernel: C + NASM, 32-bit", C_TEXT, 1);
        gfx_text(x + 20, y + 146, "filesystem: RAM only", C_TEXT, 1);
        gfx_text(x + 20, y + 166, "apps: see desktop pages", C_TEXT, 1);
        break;
    case M_STOPWATCH:
        if (stop_running)
            stop_elapsed = timer_ticks() - stop_start;
        draw_clock_like(x, y, "stopwatch (1/100 s)", stop_elapsed);
        gfx_fill(x + 60, y + 220, 140, 44, C_GREEN);
        gfx_text(x + 84, y + 232, stop_running ? "Stop" : "Start", C_BLACK, 2);
        gfx_fill(x + 220, y + 220, 140, 44, C_RED);
        gfx_text(x + 250, y + 232, "Reset", C_WHITE, 2);
        break;
    }
}

void more_click(int app, int mx, int my, int x, int y, int w, int h)
{
    (void)w;
    (void)h;
    switch (app) {
    case M_TODO:
        if (mx >= x + 30 && mx < x + 500 && my >= y + 36 && my < y + 36 + 5 * 44) {
            int row = (my - (y + 36)) / 44;
            if (row >= 0 && row < 5)
                todo_done[row] = !todo_done[row];
        }
        break;
    case M_DICE:
        if (mx >= x + 150 && mx < x + 310 && my >= y + 210 && my < y + 254) {
            dice_a = 1 + (int)(rnd() % 6);
            dice_b = 1 + (int)(rnd() % 6);
        }
        break;
    case M_COIN:
        if (mx >= x + 150 && mx < x + 330 && my >= y + 270 && my < y + 310)
            coin_heads = !coin_heads;
        break;
    case M_RANDOM:
        if (mx >= x + 150 && mx < x + 350 && my >= y + 240 && my < y + 284)
            random_value = (int)(rnd() % 100);
        break;
    case M_CONVERT:
        if (my >= y + 240 && my < y + 280) {
            if (mx >= x + 300 && mx < x + 400)
                convert_value--;
            else if (mx >= x + 420 && mx < x + 520)
                convert_value++;
        }
        break;
    case M_STOPWATCH:
        if (my >= y + 220 && my < y + 264) {
            if (mx >= x + 60 && mx < x + 200) {
                if (stop_running) {
                    stop_elapsed = timer_ticks() - stop_start;
                    stop_running = 0;
                } else {
                    stop_start = timer_ticks() - stop_elapsed;
                    stop_running = 1;
                }
            } else if (mx >= x + 220 && mx < x + 360) {
                stop_running = 0;
                stop_elapsed = 0;
            }
        }
        break;
    }
}

void more_key(int app, int key)
{
    if (app == M_DICE && (key == 'r' || key == 'R')) {
        dice_a = 1 + (int)(rnd() % 6);
        dice_b = 1 + (int)(rnd() % 6);
    } else if (app == M_RANDOM && (key == 'r' || key == 'R')) {
        random_value = (int)(rnd() % 100);
    }
}

