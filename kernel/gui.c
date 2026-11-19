#include "gui.h"
#include "fs.h"
#include "gfx.h"
#include "games.h"
#include "cards.h"
#include "moreapps.h"
#include "arcade.h"
#include "zh.h"
#include "io.h"

static int language;
#include "keyboard.h"
#include "mouse.h"
#include "prog.h"
#include "string.h"
#include "timer.h"

#define SCREEN_W 640
#define SCREEN_H 480

#define TOPBAR_H 30
#define WIN_X 40
#define WIN_Y 52
#define WIN_W 560
#define WIN_H 388
#define CONTENT_X (WIN_X + 12)
#define CONTENT_Y (WIN_Y + 38)
#define CONTENT_W (WIN_W - 24)
#define CONTENT_H (WIN_H - 50)

#define C_DESKTOP  0x00101820
#define C_DESKTOP2 0x00182430
#define C_DARK     0x00202830
#define C_GREY     0x00A0A8B0

static int theme_index;
static const uint32_t theme_bg[3] = {
    0x00101820, 0x000F2018, 0x00201810
};
static const uint32_t theme_panel[3] = {
    0x001E2A38, 0x001A3028, 0x0038281A
};
static const uint32_t theme_accent[3] = {
    0x002F6699, 0x002F9955, 0x0099662F
};
#define C_PANEL    0x001E2A38
#define C_PANEL2   0x00253648
#define C_WINDOW   0x00E9EEF5
#define C_WHITE    0x00FFFFFF
#define C_BLACK    0x00000000
#define C_TITLE    0x002F6699
#define C_TITLE2   0x001F4566
#define C_TEXT     0x00141C24
#define C_MUTED    0x00607080
#define C_GREEN    0x006EE7A8
#define C_RED      0x00D9534F
#define C_YELLOW   0x00F0C040
#define C_BLUE     0x004A90D9

enum {
    APP_NONE,
    APP_CALC,
    APP_CLOCK,
    APP_TIMER,
    APP_PAINT,
    APP_NOTE,
    APP_FILES,
    APP_PROG,
    APP_MINES,
    APP_SNAKE,
    APP_SETTINGS,
    APP_SOLITAIRE,
    APP_SPIDER,
    APP_CALENDAR,
    APP_TODO,
    APP_ASCII,
    APP_COLORS,
    APP_FONTS,
    APP_CONVERT,
    APP_DICE,
    APP_COIN,
    APP_RANDOM,
    APP_INFO,
    APP_STOPWATCH,
    APP_COUNT
};

static const char *app_titles[APP_COUNT] = {
    "", "Calculator", "Tic-Tac-Toe", "Timer", "Paint", "Notepad", "Files",
    "Machine Code", "Minesweeper", "Snake", "Settings", "Solitaire", "Spider",
    "Memory", "2048", "Pong", "Breakout", "Fonts", "Convert",
    "Dice", "Life", "Random", "Maze", "Stopwatch"
};

static const char *app_titles_zh[APP_COUNT] = {
    "", "Calculator", "Tic-Tac-Toe", "Timer", "Paint", "Notepad", "Files",
    "Machine Code", "Minesweeper", "Snake", "Settings", "Solitaire", "Spider",
    "Memory", "2048", "Pong", "Breakout", "Fonts", "Convert",
    "Dice", "Life", "Random", "Maze", "Stopwatch"
};

static int active_app;

/* ------------------------------------------------------------------ */
/* small helpers                                                       */
/* ------------------------------------------------------------------ */

static int inside(int x, int y, int rx, int ry, int rw, int rh)
{
    return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}

static void text_center(int y, const char *text, uint32_t color, int scale)
{
    int width = gfx_text_width(text, scale);
    gfx_text((SCREEN_W - width) / 2, y, text, color, scale);
}

static int int_to_str(int value, char *buffer)
{
    char temp[12];
    int count = 0;
    int length = 0;
    unsigned int v;

    if (value < 0) {
        buffer[length++] = '-';
        v = (unsigned int)(-value);
    } else {
        v = (unsigned int)value;
    }
    if (v == 0) {
        buffer[length++] = '0';
        buffer[length] = 0;
        return length;
    }
    while (v > 0) {
        temp[count++] = (char)('0' + v % 10);
        v /= 10;
    }
    while (count > 0)
        buffer[length++] = temp[--count];
    buffer[length] = 0;
    return length;
}

static void two_digits(char *buffer, int value)
{
    buffer[0] = (char)('0' + (value / 10) % 10);
    buffer[1] = (char)('0' + value % 10);
}

static void fill_circle(int cx, int cy, int radius, uint32_t color)
{
    int y;
    int x;

    for (y = -radius; y <= radius; y++) {
        for (x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius)
                gfx_pixel(cx + x, cy + y, color);
        }
    }
}

/* ------------------------------------------------------------------ */
/* RTC                                                                 */
/* ------------------------------------------------------------------ */

static uint8_t cmos_read(uint8_t reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t value)
{
    return (uint8_t)((value & 0x0F) + ((value >> 4) * 10));
}

static void rtc_read(int *hour, int *minute, int *second,
                     int *day, int *month, int *year)
{
    uint8_t sec;
    uint8_t min;
    uint8_t hr;
    uint8_t dy;
    uint8_t mo;
    uint8_t yr;
    uint8_t status;
    int tries;

    for (tries = 0; tries < 100000; tries++) {
        if (!(cmos_read(0x0A) & 0x80))
            break;
    }

    sec = cmos_read(0x00);
    min = cmos_read(0x02);
    hr = cmos_read(0x04);
    dy = cmos_read(0x07);
    mo = cmos_read(0x08);
    yr = cmos_read(0x09);
    status = cmos_read(0x0B);

    if (!(status & 0x04)) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        dy = bcd_to_bin(dy);
        mo = bcd_to_bin(mo);
        yr = bcd_to_bin(yr);

        {
            uint8_t pm = (uint8_t)(hr & 0x80);
            hr = bcd_to_bin((uint8_t)(hr & 0x7F));
            if (!(status & 0x02)) {
                if (pm && hr < 12)
                    hr = (uint8_t)(hr + 12);
                if (!pm && hr == 12)
                    hr = 0;
            }
        }
    } else if (!(status & 0x02)) {
        uint8_t pm = (uint8_t)(hr & 0x80);
        hr &= 0x7F;
        if (pm && hr < 12)
            hr = (uint8_t)(hr + 12);
        if (!pm && hr == 12)
            hr = 0;
    }

    if (sec > 59 || min > 59 || hr > 23 || mo == 0 || mo > 12 || dy == 0 || dy > 31) {
        uint32_t ticks = timer_ticks() / 100;
        sec = (uint8_t)(ticks % 60);
        min = (uint8_t)((ticks / 60) % 60);
        hr = (uint8_t)((ticks / 3600) % 24);
        dy = 1;
        mo = 1;
        yr = 26;
    }

    *second = sec;
    *minute = min;
    *hour = hr;
    *day = dy;
    *month = mo;
    *year = 2000 + yr;
}

/* ------------------------------------------------------------------ */
/* calculator                                                          */
/* ------------------------------------------------------------------ */

static int calc_acc;
static int calc_current;
static int calc_entering;
static int calc_error;
static char calc_pending;
static char calc_text[16] = "0";

static void calc_refresh(void)
{
    if (calc_error) {
        strcpy(calc_text, "Error");
        return;
    }
    int_to_str(calc_entering ? calc_current : calc_acc, calc_text);
}

static void calc_clear(void)
{
    calc_acc = 0;
    calc_current = 0;
    calc_entering = 0;
    calc_error = 0;
    calc_pending = 0;
    strcpy(calc_text, "0");
}

static void calc_compute(void)
{
    if (!calc_pending)
        return;

    if (calc_pending == '+')
        calc_acc += calc_current;
    else if (calc_pending == '-')
        calc_acc -= calc_current;
    else if (calc_pending == '*')
        calc_acc *= calc_current;
    else if (calc_pending == '/') {
        if (calc_current == 0) {
            calc_error = 1;
            return;
        }
        calc_acc /= calc_current;
    }
}

static void calc_digit(int digit)
{
    if (calc_error)
        calc_clear();
    if (!calc_entering) {
        calc_current = digit;
        calc_entering = 1;
    } else if (calc_current < 100000000) {
        calc_current = calc_current * 10 + digit;
    }
    calc_refresh();
}

static void calc_operator(char op)
{
    if (calc_error)
        return;

    if (!calc_pending) {
        calc_acc = calc_current;
    } else if (calc_entering) {
        calc_compute();
        if (calc_error)
            return;
    }

    calc_pending = op;
    calc_entering = 0;
    calc_current = 0;
    calc_refresh();
}

static void calc_equals(void)
{
    if (calc_error)
        return;

    if (calc_pending) {
        calc_compute();
        if (calc_error)
            return;
    } else {
        calc_acc = calc_current;
    }

    calc_entering = 0;
    calc_pending = 0;
    calc_current = calc_acc;
    calc_refresh();
}

static const char *calc_labels[4][4] = {
    { "7", "8", "9", "/" },
    { "4", "5", "6", "*" },
    { "1", "2", "3", "-" },
    { "0", "C", "=", "+" }
};

static void calc_button_rect(int row, int column, int *x, int *y)
{
    *x = CONTENT_X + column * 136;
    *y = CONTENT_Y + 60 + row * 68;
}

static void draw_calculator(void)
{
    int row;
    int column;
    int width;
    int length;

    gfx_fill(CONTENT_X, CONTENT_Y, CONTENT_W, 44, 0x00101820);
    gfx_rect(CONTENT_X, CONTENT_Y, CONTENT_W, 44, C_BLUE);
    length = (int)strlen(calc_text);
    width = length * 24;
    gfx_text(CONTENT_X + CONTENT_W - width - 12, CONTENT_Y + 8,
             calc_text, C_GREEN, 3);

    for (row = 0; row < 4; row++) {
        for (column = 0; column < 4; column++) {
            int x;
            int y;
            int label_width;
            const char *label = calc_labels[row][column];
            uint32_t color = C_PANEL2;

            calc_button_rect(row, column, &x, &y);
            if (label[0] == '/' || label[0] == '*' ||
                label[0] == '-' || label[0] == '+' || label[0] == '=')
                color = C_TITLE;
            if (label[0] == 'C')
                color = C_RED;

            gfx_fill(x, y, 128, 60, color);
            label_width = gfx_text_width(label, 2);
            gfx_text(x + (128 - label_width) / 2, y + 14, label, C_WHITE, 2);
        }
    }
}

/* ------------------------------------------------------------------ */
/* clock                                                               */
/* ------------------------------------------------------------------ */

static void clock_string(char *buffer)
{
    int hour;
    int minute;
    int second;
    int day;
    int month;
    int year;

    rtc_read(&hour, &minute, &second, &day, &month, &year);
    two_digits(buffer + 0, hour);
    buffer[2] = ':';
    two_digits(buffer + 3, minute);
    buffer[5] = ':';
    two_digits(buffer + 6, second);
    buffer[8] = 0;
}

static void date_string(char *buffer)
{
    int hour;
    int minute;
    int second;
    int day;
    int month;
    int year;

    rtc_read(&hour, &minute, &second, &day, &month, &year);
    buffer[0] = (char)('0' + (year / 1000) % 10);
    buffer[1] = (char)('0' + (year / 100) % 10);
    buffer[2] = (char)('0' + (year / 10) % 10);
    buffer[3] = (char)('0' + year % 10);
    buffer[4] = '-';
    two_digits(buffer + 5, month);
    buffer[7] = '-';
    two_digits(buffer + 8, day);
    buffer[10] = 0;
}

static void draw_clock(void)
{
    char time[9];
    char date[11];
    char uptime[32];
    uint32_t seconds = timer_ticks() / 100;

    clock_string(time);
    date_string(date);

    text_center(CONTENT_Y + 60, time, C_TEXT, 6);
    text_center(CONTENT_Y + 140, date, C_MUTED, 2);

    strcpy(uptime, "uptime ");
    int_to_str((int)(seconds / 60), uptime + strlen(uptime));
    strcpy(uptime + strlen(uptime), " min ");
    int_to_str((int)(seconds % 60), uptime + strlen(uptime));
    strcpy(uptime + strlen(uptime), " sec");
    text_center(CONTENT_Y + 200, uptime, C_BLUE, 2);
}

/* ------------------------------------------------------------------ */
/* timer                                                               */
/* ------------------------------------------------------------------ */

static int timer_running;
static uint32_t timer_base;
static uint32_t timer_elapsed;

static uint32_t timer_value(void)
{
    if (timer_running)
        return timer_ticks() - timer_base;
    return timer_elapsed;
}

static void timer_start(void)
{
    if (!timer_running) {
        timer_base = timer_ticks() - timer_elapsed;
        timer_running = 1;
    }
}

static void timer_stop(void)
{
    if (timer_running) {
        timer_elapsed = timer_ticks() - timer_base;
        timer_running = 0;
    }
}

static void timer_reset(void)
{
    timer_running = 0;
    timer_elapsed = 0;
}

static void timer_string(char *buffer)
{
    uint32_t value = timer_value();
    uint32_t total_seconds = value / 100;
    int minutes = (int)(total_seconds / 60);
    int seconds = (int)(total_seconds % 60);
    int hundredths = (int)(value % 100);

    if (minutes > 99)
        minutes = 99;
    two_digits(buffer + 0, minutes);
    buffer[2] = ':';
    two_digits(buffer + 3, seconds);
    buffer[5] = '.';
    two_digits(buffer + 6, hundredths);
    buffer[8] = 0;
}

static void draw_timer(void)
{
    char display[9];
    int i;
    const char *labels[3] = { "Start", "Stop", "Reset" };
    uint32_t colors[3] = { C_GREEN, C_RED, C_BLUE };

    timer_string(display);
    text_center(CONTENT_Y + 40, display, C_TEXT, 5);

    for (i = 0; i < 3; i++) {
        int x = CONTENT_X + 20 + i * 176;
        int y = CONTENT_Y + 170;
        int width = gfx_text_width(labels[i], 2);
        gfx_fill(x, y, 160, 60, colors[i]);
        gfx_text(x + (160 - width) / 2, y + 14, labels[i], C_WHITE, 2);
    }
}

/* ------------------------------------------------------------------ */
/* paint                                                               */
/* ------------------------------------------------------------------ */

#define PAINT_W CONTENT_W
#define PAINT_H (CONTENT_H - 58)

static uint8_t paint_canvas[PAINT_W * PAINT_H];
static int paint_color = 4;
static int paint_drawing;
static int paint_last_x;
static int paint_last_y;
static int paint_tool;
static int shape_active;
static int shape_x0;
static int shape_y0;

static const uint32_t paint_palette[8] = {
    0x00101820, 0x00D9534F, 0x00F0C040, 0x006EE7A8,
    0x004A90D9, 0x00B06AD9, 0x00F5F5F5, 0x00808080
};

static void paint_pixel(int x, int y)
{
    if (x >= 0 && y >= 0 && x < PAINT_W && y < PAINT_H)
        paint_canvas[y * PAINT_W + x] = (uint8_t)(paint_tool == 5 ? 7 : paint_color);
}

static void paint_line(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    int err;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    err = dx - dy;

    for (;;) {
        paint_pixel(x0, y0);
        if (x0 == x1 && y0 == y1)
            break;
        {
            int e2 = err * 2;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

static void paint_plot(int x, int y)
{
    int cx = x - CONTENT_X;
    int cy = y - (CONTENT_Y + 58);

    if (cx < 0 || cy < 0 || cx >= PAINT_W || cy >= PAINT_H) {
        paint_drawing = 0;
        return;
    }

    if (paint_drawing)
        paint_line(paint_last_x, paint_last_y, cx, cy);
    else
        paint_pixel(cx, cy);

    paint_last_x = cx;
    paint_last_y = cy;
    paint_drawing = 1;
}

static void paint_clear(void)
{
    memset(paint_canvas, 7, sizeof(paint_canvas));
    paint_drawing = 0;
}

static void paint_rect(int x0, int y0, int x1, int y1)
{
    int t;
    if (x0 > x1) { t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { t = y0; y0 = y1; y1 = t; }
    paint_line(x0, y0, x1, y0);
    paint_line(x0, y1, x1, y1);
    paint_line(x0, y0, x0, y1);
    paint_line(x1, y0, x1, y1);
}

static void paint_oval(int x0, int y0, int x1, int y1)
{
    int cx = (x0 + x1) / 2;
    int cy = (y0 + y1) / 2;
    int rx = (x0 > x1 ? x0 - x1 : x1 - x0) / 2;
    int ry = (y0 > y1 ? y0 - y1 : y1 - y0) / 2;
    int x;
    int y;

    if (rx < 1 || ry < 1)
        return;
    for (y = -ry; y <= ry; y++) {
        for (x = -rx; x <= rx; x++) {
            long v = (long)x * x * ry * ry + (long)y * y * rx * rx;
            long rr = (long)rx * rx * ry * ry;
            if (v <= rr) {
                long vx = (long)(x + 1) * (x + 1) * ry * ry + (long)y * y * rx * rx;
                long vy = (long)x * x * ry * ry + (long)(y + 1) * (y + 1) * rx * rx;
                if (vx > rr || vy > rr || x == -rx || x == rx || y == -ry || y == ry)
                    paint_pixel(cx + x, cy + y);
            }
        }
    }
}

static void paint_fill(int x, int y)
{
    int target;
    int color = paint_tool == 5 ? 7 : paint_color;
    static int stack[8192];
    int top = 0;

    if (x < 0 || y < 0 || x >= PAINT_W || y >= PAINT_H)
        return;
    target = paint_canvas[y * PAINT_W + x];
    if (target == color)
        return;

    stack[top++] = y * PAINT_W + x;
    while (top > 0 && top < 8191) {
        int index = stack[--top];
        int px = index % PAINT_W;
        int py = index / PAINT_W;
        if (paint_canvas[index] != target)
            continue;
        paint_canvas[index] = (uint8_t)color;
        if (px > 0) stack[top++] = index - 1;
        if (px < PAINT_W - 1) stack[top++] = index + 1;
        if (py > 0) stack[top++] = index - PAINT_W;
        if (py < PAINT_H - 1) stack[top++] = index + PAINT_W;
    }
}

static void paint_begin(int x, int y)
{
    int cx = x - CONTENT_X;
    int cy = y - (CONTENT_Y + 58);

    if (cx < 0 || cy < 0 || cx >= PAINT_W || cy >= PAINT_H)
        return;

    if (paint_tool == 4) {
        paint_fill(cx, cy);
        return;
    }

    shape_active = 1;
    shape_x0 = cx;
    shape_y0 = cy;
    paint_drawing = 0;
    if (paint_tool == 0 || paint_tool == 5)
        paint_plot(x, y);
}

static void paint_end(int x, int y)
{
    int cx = x - CONTENT_X;
    int cy = y - (CONTENT_Y + 58);

    if (!shape_active)
        return;
    shape_active = 0;
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx >= PAINT_W) cx = PAINT_W - 1;
    if (cy >= PAINT_H) cy = PAINT_H - 1;

    if (paint_tool == 1)
        paint_line(shape_x0, shape_y0, cx, cy);
    else if (paint_tool == 2)
        paint_rect(shape_x0, shape_y0, cx, cy);
    else if (paint_tool == 3)
        paint_oval(shape_x0, shape_y0, cx, cy);
}

static void draw_paint(void)
{
    int i;
    int x;
    int y;

    for (i = 0; i < 8; i++) {
        gfx_fill(CONTENT_X + i * 40, CONTENT_Y, 34, 28, paint_palette[i]);
        if (i == paint_color)
            gfx_rect(CONTENT_X + i * 40 - 1, CONTENT_Y - 1, 36, 30, C_TEXT);
    }
    gfx_fill(CONTENT_X + 460, CONTENT_Y, 76, 28, C_RED);
    gfx_text(CONTENT_X + 472, CONTENT_Y + 10, "Clear", C_WHITE, 1);
    gfx_text(CONTENT_X + 4, CONTENT_Y + 34,
             "1 Pen  2 Line  3 Rect  4 Oval  5 Fill  6 Eraser", C_MUTED, 1);

    for (y = 0; y < PAINT_H; y++) {
        for (x = 0; x < PAINT_W; x++) {
            uint8_t index = paint_canvas[y * PAINT_W + x];
            if (index > 7)
                index = 7;
            gfx_pixel(CONTENT_X + x, CONTENT_Y + 58 + y, paint_palette[index]);
        }
    }
}

/* ------------------------------------------------------------------ */
/* notepad                                                             */
/* ------------------------------------------------------------------ */

static char note_text[FS_MAX_DATA];
static int note_length;
static char note_name[FS_MAX_NAME] = "untitled.txt";
static int note_volume;
static int note_saved = 1;
static int note_valid = 1;

static void note_open(int volume, const char *name)
{
    fs_file *file = fs_find(volume, name);

    note_volume = volume;
    strcpy(note_name, name ? name : "untitled.txt");
    note_length = 0;
    if (file && file->size > 0) {
        note_length = file->size;
        if (note_length > FS_MAX_DATA)
            note_length = FS_MAX_DATA;
        memcpy(note_text, file->data, (size_t)note_length);
    }
    note_saved = 1;
    note_valid = 1;
}

static void note_save(void)
{
    if (!note_valid)
        return;
    if (!fs_write(note_volume, note_name, note_text, note_length)) {
        note_valid = 0;
        return;
    }
    note_saved = 1;
}

static void note_new(void)
{
    char name[FS_MAX_NAME];
    int number = fs_count(note_volume) + 1;
    int guard = 0;

    while (guard++ < 1000) {
        strcpy(name, "note");
        int_to_str(number, name + 4);
        strcpy(name + strlen(name), ".txt");
        if (!fs_find(note_volume, name))
            break;
        number++;
    }

    if (!fs_create(note_volume, name)) {
        note_valid = 0;
        note_length = 0;
        note_saved = 0;
        return;
    }

    strcpy(note_name, name);
    note_length = 0;
    note_saved = 0;
    note_valid = 1;
}

static void note_putc(char c)
{
    if (note_length < FS_MAX_DATA - 1) {
        note_text[note_length++] = c;
        note_saved = 0;
    }
}

static void note_backspace(void)
{
    if (note_length > 0) {
        note_length--;
        note_saved = 0;
    }
}

static void draw_notepad(void)
{
    int x = CONTENT_X;
    int y = CONTENT_Y;
    int column = 0;
    int row = 0;
    int i;
    char status[48];

    gfx_fill(x, y, CONTENT_W, CONTENT_H - 44, C_WHITE);
    gfx_rect(x, y, CONTENT_W, CONTENT_H - 44, C_MUTED);

    for (i = 0; i < note_length; i++) {
        char c = note_text[i];
        if (c == '\n') {
            column = 0;
            row++;
            continue;
        }
        if (column >= 66) {
            column = 0;
            row++;
        }
        if (row < (CONTENT_H - 44) / 8)
            gfx_char(x + 6 + column * 8, y + 6 + row * 8, c, C_TEXT, 1);
        column++;
    }

    if (!note_valid) {
        strcpy(status, "FS FULL");
    } else {
        strcpy(status, note_name);
        strcpy(status + strlen(status), note_saved ? "  [saved]" : "  [modified]");
    }
    gfx_text(x, y + CONTENT_H - 38, status, C_MUTED, 1);

    gfx_fill(x + CONTENT_W - 240, y + CONTENT_H - 44, 110, 34, C_TITLE);
    gfx_text(x + CONTENT_W - 222, y + CONTENT_H - 34, "Save", C_WHITE, 2);
    gfx_fill(x + CONTENT_W - 120, y + CONTENT_H - 44, 110, 34, C_PANEL2);
    gfx_text(x + CONTENT_W - 104, y + CONTENT_H - 34, "New", C_WHITE, 2);
}

/* ------------------------------------------------------------------ */
/* files                                                               */
/* ------------------------------------------------------------------ */

static int files_volume;
static int files_selected;

static void files_refresh(void)
{
    int count = fs_count(files_volume);
    if (files_selected >= count)
        files_selected = count - 1;
    if (files_selected < 0)
        files_selected = 0;
}

static void files_open_selected(void)
{
    fs_file *file = fs_at(files_volume, files_selected);
    if (!file)
        return;

    if (strlen(file->name) > 4 &&
        !strcmp(file->name + strlen(file->name) - 4, ".bin")) {
        prog_open(files_volume, file->name);
        active_app = APP_PROG;
        return;
    }

    note_open(files_volume, file->name);
    active_app = APP_NOTE;
}

static void draw_files(void)
{
    int i;
    int x = CONTENT_X;
    int y = CONTENT_Y;
    const char *volumes[2] = { "SYSTEM", "DISK" };
    const char *labels[4] = { "Open", "New", "Delete", "Move >" };
    int count = fs_count(files_volume);

    for (i = 0; i < 2; i++) {
        uint32_t color = (i == files_volume) ? C_TITLE : C_PANEL2;
        gfx_fill(x + i * 140, y, 130, 34, color);
        gfx_text(x + i * 140 + 24, y + 9, volumes[i], C_WHITE, 2);
    }
    gfx_text(x + 300, y + 9, fs_volume_name(files_volume), C_MUTED, 2);
    gfx_text(x + 300, y + 9, fs_volume_name(files_volume), C_MUTED, 2);

    gfx_fill(x, y + 44, CONTENT_W, 200, C_WHITE);
    gfx_rect(x, y + 44, CONTENT_W, 200, C_MUTED);

    for (i = 0; i < count && i < 8; i++) {
        fs_file *file = fs_at(files_volume, i);
        int row_y = y + 50 + i * 24;
        char size_text[16];

        if (i == files_selected)
            gfx_fill(x + 2, row_y - 3, CONTENT_W - 4, 22, 0x00CFE4F5);
        gfx_text(x + 10, row_y, file->name, C_TEXT, 1);
        int_to_str(file->size, size_text);
        strcpy(size_text + strlen(size_text), " B");
        gfx_text(x + CONTENT_W - 90, row_y, size_text, C_MUTED, 1);
    }
    if (count == 0)
        gfx_text(x + 10, y + 50, "(empty volume)", C_MUTED, 1);

    for (i = 0; i < 4; i++) {
        int bx = x + i * 134;
        int by = y + 258;
        uint32_t color = (i == 2) ? C_RED : C_TITLE;
        gfx_fill(bx, by, 122, 42, color);
        gfx_text(bx + 16, by + 12, labels[i], C_WHITE, 2);
    }
}

/* ------------------------------------------------------------------ */
/* desktop / window                                                    */
/* ------------------------------------------------------------------ */

static const char *icon_labels[22] = {
    "Calc", "Timer", "Paint", "Notes", "Files", "Code",
    "Mines", "Snake", "Settings", "Solitaire", "Spider", "TicTacToe",
    "Memory", "2048", "Breakout", "Fonts", "Convert",
    "Dice", "Life", "Random", "Maze", "Stopwatch"
};

static const char *icon_labels_zh[22] = {
    "Calc", "Timer", "Paint", "Notes", "Files", "Code",
    "Mines", "Snake", "Settings", "Solitaire", "Spider", "TicTacToe",
    "Memory", "2048", "Breakout", "Fonts", "Convert",
    "Dice", "Life", "Random", "Maze", "Stopwatch"
};

static const int icon_art[22] = {
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22
};

static const int icon_apps[22] = {
    APP_CALC, APP_TIMER, APP_PAINT, APP_NOTE, APP_FILES, APP_PROG,
    APP_MINES, APP_SNAKE, APP_SETTINGS, APP_SOLITAIRE, APP_SPIDER, APP_CLOCK,
    APP_CALENDAR, APP_TODO, APP_COLORS, APP_FONTS, APP_CONVERT,
    APP_DICE, APP_COIN, APP_RANDOM, APP_INFO, APP_STOPWATCH
};

static int desktop_page;

static void icon_rect(int index, int *x, int *y)
{
    int slot = index % 12;
    *x = 50 + (slot % 4) * 140;
    *y = 50 + (slot / 4) * 105;
}

static void draw_icon(int slot)
{
    int art = icon_art[slot];
    int x;
    int y;
    int cx;
    int cy;
    int label_width;
    uint32_t color = C_PANEL2;

    icon_rect(slot, &x, &y);
    cx = x + 42;
    cy = y + 36;

    gfx_fill(x, y, 84, 74, color);
    gfx_rect(x, y, 84, 74, C_TITLE);
    gfx_fill(x + 6, y + 6, 72, 52, C_DESKTOP2);

    if (art == 0) {
        gfx_fill(cx - 16, cy - 18, 32, 36, C_PANEL);
        gfx_fill(cx - 12, cy - 14, 24, 8, C_GREEN);
        gfx_fill(cx - 12, cy - 2, 10, 8, C_WHITE);
        gfx_fill(cx + 2, cy - 2, 10, 8, C_WHITE);
        gfx_fill(cx - 12, cy + 8, 24, 8, C_YELLOW);
    } else if (art == 2) {
        fill_circle(cx, cy, 20, C_WHITE);
        fill_circle(cx, cy, 16, C_DESKTOP2);
        gfx_fill(cx - 2, cy - 12, 4, 22, C_RED);
        gfx_fill(cx - 8, cy + 8, 16, 4, C_RED);
    } else if (art == 3) {
        gfx_fill(cx - 20, cy - 16, 40, 30, C_WHITE);
        gfx_fill(cx - 12, cy - 8, 10, 10, C_RED);
        gfx_fill(cx + 2, cy - 8, 10, 10, C_BLUE);
        gfx_fill(cx - 12, cy + 6, 10, 6, C_YELLOW);
    } else if (art == 4) {
        gfx_fill(cx - 18, cy - 20, 36, 40, C_WHITE);
        gfx_fill(cx - 10, cy - 12, 20, 2, C_MUTED);
        gfx_fill(cx - 10, cy - 4, 20, 2, C_MUTED);
        gfx_fill(cx - 10, cy + 4, 20, 2, C_MUTED);
        gfx_fill(cx - 10, cy + 12, 12, 2, C_MUTED);
    } else if (art == 5) {
        gfx_fill(cx - 20, cy - 12, 40, 28, C_YELLOW);
        gfx_fill(cx - 12, cy - 18, 18, 8, C_YELLOW);
        gfx_fill(cx - 12, cy - 4, 16, 2, C_DESKTOP2);
    } else if (art == 7) {
        fill_circle(cx, cy, 20, C_GREY);
        fill_circle(cx, cy, 13, C_DARK);
        gfx_fill(cx - 3, cy - 3, 6, 6, C_RED);
    } else if (art == 8) {
        gfx_fill(cx - 20, cy - 4, 14, 12, C_GREEN);
        gfx_fill(cx - 8, cy - 4, 14, 12, 0x0040A060);
        gfx_fill(cx + 4, cy + 4, 14, 12, 0x0040A060);
        gfx_fill(cx + 12, cy - 14, 8, 8, C_RED);
    } else if (art == 9) {
        fill_circle(cx, cy, 19, C_GREY);
        fill_circle(cx, cy, 12, C_PANEL);
        gfx_fill(cx - 2, cy - 26, 4, 10, C_GREY);
        gfx_fill(cx - 2, cy + 16, 4, 10, C_GREY);
        gfx_fill(cx - 26, cy - 2, 10, 4, C_GREY);
        gfx_fill(cx + 16, cy - 2, 10, 4, C_GREY);
        fill_circle(cx, cy, 5, C_GREEN);
    } else if (art == 10) {
        gfx_fill(cx - 22, cy - 20, 26, 32, C_WHITE);
        gfx_rect(cx - 22, cy - 20, 26, 32, C_TEXT);
        gfx_text(cx - 18, cy - 15, "A", C_RED, 2);
        gfx_fill(cx - 8, cy - 12, 26, 32, C_WHITE);
        gfx_rect(cx - 8, cy - 12, 26, 32, C_TEXT);
        gfx_text(cx - 4, cy - 7, "K", C_TEXT, 2);
    } else if (art == 11) {
        gfx_fill(cx - 26, cy - 18, 24, 26, C_WHITE);
        gfx_rect(cx - 26, cy - 18, 24, 26, C_TEXT);
        gfx_fill(cx - 14, cy - 10, 24, 26, C_WHITE);
        gfx_rect(cx - 14, cy - 10, 24, 26, C_TEXT);
        gfx_text(cx - 8, cy - 6, "S", C_BLUE, 1);
        gfx_fill(cx - 2, cy - 2, 24, 26, C_WHITE);
        gfx_rect(cx - 2, cy - 2, 24, 26, C_TEXT);
        gfx_text(cx + 4, cy + 2, "P", C_RED, 1);
    } else if (art >= 12) {
        char letter[2];
        letter[0] = icon_labels[slot][0];
        letter[1] = 0;
        gfx_fill(cx - 22, cy - 22, 44, 44, C_TITLE);
        gfx_rect(cx - 22, cy - 22, 44, 44, C_GREEN);
        gfx_text(cx - 6, cy - 8, letter, C_WHITE, 2);
    }

    if (art == 6) {
        gfx_fill(cx - 20, cy - 20, 40, 40, C_PANEL);
        gfx_rect(cx - 20, cy - 20, 40, 40, C_GREEN);
        gfx_text(cx - 12, cy - 8, "01", C_GREEN, 2);
    }

    {
        const char *label = language ? icon_labels_zh[slot] : icon_labels[slot];
        label_width = zh_text_width(label, 1);
        zh_text(x + (84 - label_width) / 2, y + 58, label, C_WHITE, 1);
    }
}

static void draw_desktop(void)
{
    int i;

    gfx_clear(theme_bg[theme_index]);
    for (i = 0; i < 12; i++)
        gfx_fill(0, 150 + i * 24, SCREEN_W, 1, C_DESKTOP2);

    gfx_fill(0, 0, SCREEN_W, TOPBAR_H, theme_panel[theme_index]);
    gfx_text(12, 7, "matenOS", C_GREEN, 2);
    gfx_text(130, 10, "graphical shell", C_MUTED, 1);

    {
        int start = desktop_page * 12;
        int end = start + 12;
        if (end > 22)
            end = 22;
        for (i = start; i < end; i++)
            draw_icon(i);
    }

    gfx_fill(20, SCREEN_H - 72, 60, 30, C_BLUE);
    gfx_text(34, SCREEN_H - 64, "<", C_WHITE, 3);
    gfx_fill(SCREEN_W - 80, SCREEN_H - 72, 60, 30, C_BLUE);
    gfx_text(SCREEN_W - 62, SCREEN_H - 64, ">", C_WHITE, 3);
    gfx_fill(SCREEN_W - 150, 4, 132, 22, C_TITLE);
    gfx_text(SCREEN_W - 142, 10, desktop_page ? "PAGE 1 <" : "PAGE 2 >", C_WHITE, 1);
    text_center(SCREEN_H - 52, desktop_page ? "Page 2/2  [ ] switch" : "Page 1/2  [ ] switch", C_GREEN, 1);
    gfx_text(18, SCREEN_H - 22, "Click an icon to open an app. ESC closes the window.", C_MUTED, 1);
}

static int settings_page;

static const char license_text[] =
    "This is free and unencumbered software released into the public domain.\n\n"
    "Anyone is free to copy, modify, publish, use, compile, sell, or\n"
    "distribute this software, either in source code form or as a compiled\n"
    "binary, for any purpose, commercial or non-commercial, and by any\n"
    "means.\n\n"
    "In jurisdictions that recognize copyright laws, the author or authors\n"
    "of this software dedicate any and all copyright interest in the\n"
    "software to the public domain. We make this dedication for the benefit\n"
    "of the public at large and to the detriment of our heirs and\n"
    "successors. We intend this dedication to be an overt act of\n"
    "relinquishment in perpetuity of all present and future rights to this\n"
    "software under copyright law.\n\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
    "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
    "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
    "IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n"
    "OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n"
    "ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
    "OTHER DEALINGS IN THE SOFTWARE.\n\n"
    "For more information, please refer to <https://unlicense.org>\n";

static void draw_settings_page1(void)
{
    int x = CONTENT_X;
    int y = CONTENT_Y;
    char value[4];
    const char *themes[3] = { "Sea", "Forest", "Amber" };

    gfx_text(x + 10, y + 20, language ? "" : "Mouse speed", C_TEXT, 2);
    if (language)
        zh_text(x + 10, y + 20, "??????", C_TEXT, 1);
    gfx_fill(x + 300, y + 14, 40, 36, C_PANEL2);
    gfx_text(x + 314, y + 24, "-", C_WHITE, 2);
    int_to_str(mouse_speed, value);
    gfx_text(x + 360, y + 24, value, C_TEXT, 2);
    gfx_fill(x + 410, y + 14, 40, 36, C_PANEL2);
    gfx_text(x + 420, y + 24, "+", C_WHITE, 2);

    if (language)
        zh_text(x + 10, y + 80, "??????", C_TEXT, 1);
    else
        gfx_text(x + 10, y + 80, "Cursor size", C_TEXT, 2);
    gfx_fill(x + 300, y + 74, 60, 36, mouse_cursor_scale == 1 ? C_GREEN : C_PANEL2);
    gfx_text(x + 318, y + 84, "1x", mouse_cursor_scale == 1 ? C_BLACK : C_WHITE, 2);
    gfx_fill(x + 370, y + 74, 60, 36, mouse_cursor_scale == 2 ? C_GREEN : C_PANEL2);
    gfx_text(x + 388, y + 84, "2x", mouse_cursor_scale == 2 ? C_BLACK : C_WHITE, 2);

    if (language)
        zh_text(x + 10, y + 140, "???", C_TEXT, 1);
    else
        gfx_text(x + 10, y + 140, "Theme", C_TEXT, 2);
    {
        int i;
        for (i = 0; i < 3; i++) {
            int bx = x + 300 + i * 72;
            gfx_fill(bx, y + 134, 64, 36, i == theme_index ? C_GREEN : C_PANEL2);
            gfx_text(bx + 8, y + 148, themes[i],
                     i == theme_index ? C_BLACK : C_WHITE, 1);
        }
    }

    if (language)
        zh_text(x + 10, y + 200, "??? Y", C_TEXT, 1);
    else
        gfx_text(x + 10, y + 200, "Invert Y", C_TEXT, 2);
    gfx_fill(x + 300, y + 194, 80, 36, mouse_invert_y ? C_GREEN : C_PANEL2);
    if (language)
        zh_text(x + 314, y + 208, "???", mouse_invert_y ? C_BLACK : C_WHITE, 1);
    else
        gfx_text(x + 314, y + 208, "Invert", mouse_invert_y ? C_BLACK : C_WHITE, 1);
    gfx_fill(x + 390, y + 194, 80, 36, !mouse_invert_y ? C_GREEN : C_PANEL2);
    if (language)
        zh_text(x + 408, y + 208, "???", !mouse_invert_y ? C_BLACK : C_WHITE, 1);
    else
        gfx_text(x + 400, y + 208, "Normal", !mouse_invert_y ? C_BLACK : C_WHITE, 1);

    gfx_fill(x + 10, y + 250, 150, 36, C_RED);
    if (language)
        zh_text(x + 34, y + 260, "???", C_WHITE, 1);
    else
        gfx_text(x + 34, y + 260, "Reset", C_WHITE, 2);
    gfx_fill(x + 180, y + 250, 150, 36, C_TITLE);
    if (language)
        zh_text(x + 204, y + 260, "???", C_WHITE, 1);
    else
        gfx_text(x + 204, y + 260, "License", C_WHITE, 2);

    gfx_text(x + 10, y + 300, "Resolution: 1920x1080", C_MUTED, 1);
    gfx_text(x + 10, y + 316, "UI 640x480, 2x, centered", C_MUTED, 1);
}

static void settings_click_page1(int mx, int my)
{
    int x = CONTENT_X;
    int y = CONTENT_Y;
    int i;

    if (inside(mx, my, x + 300, y + 14, 40, 36)) {
        if (mouse_speed > 1)
            mouse_speed--;
    } else if (inside(mx, my, x + 410, y + 14, 40, 36)) {
        if (mouse_speed < 5)
            mouse_speed++;
    } else if (inside(mx, my, x + 300, y + 74, 60, 36)) {
        mouse_cursor_scale = 1;
    } else if (inside(mx, my, x + 370, y + 74, 60, 36)) {
        mouse_cursor_scale = 2;
    } else if (inside(mx, my, x + 300, y + 194, 80, 36)) {
        mouse_invert_y = 1;
    } else if (inside(mx, my, x + 390, y + 194, 80, 36)) {
        mouse_invert_y = 0;
    } else if (inside(mx, my, x + 10, y + 250, 150, 36)) {
        mouse_speed = 3;
        mouse_cursor_scale = 1;
        mouse_invert_y = 0;
        theme_index = 0;
    } else if (inside(mx, my, x + 180, y + 250, 150, 36)) {
        settings_page = 1;
    } else {
        for (i = 0; i < 3; i++) {
            int bx = x + 300 + i * 72;
            if (inside(mx, my, bx, y + 134, 64, 36)) {
                theme_index = i;
                return;
            }
        }
    }
}

static void draw_wrapped(int x, int y, int max_width, const char *text)
{
    int column = 0;
    int row = 0;

    while (*text && y + row * 8 < CONTENT_Y + CONTENT_H - 6) {
        char c = *text++;
        if (c == '\r')
            continue;
        if (c == '\n') {
            column = 0;
            row++;
            continue;
        }
        if (column * 8 >= max_width - 8) {
            column = 0;
            row++;
        }
        gfx_char(x + column * 8, y + row * 8, c, C_TEXT, 1);
        column++;
    }
}

static void draw_nav_arrow(int bx, int by, int direction)
{
    int i;
    int cy = by + 14;
    int start = direction > 0 ? bx + 8 : bx + 32;

    for (i = 0; i < 12; i++) {
        int x = start + direction * i;
        int half = 12 - i;
        gfx_vline(x, cy - half, half * 2, C_WHITE);
    }
}

static void draw_settings(void)
{
    int x = CONTENT_X;
    int y = CONTENT_Y;

    if (settings_page == 1) {
        gfx_text(x + 10, y + 6, "The Unlicense", C_TEXT, 2);
        draw_wrapped(x + 8, y + 38, CONTENT_W - 16, license_text);
        gfx_fill(x + CONTENT_W - 46, y + 2, 40, 28, C_TITLE);
        draw_nav_arrow(x + CONTENT_W - 46, y + 2, -1);
        return;
    }

    draw_settings_page1();
    gfx_fill(x + CONTENT_W - 46, y + 2, 40, 28, C_TITLE);
    draw_nav_arrow(x + CONTENT_W - 46, y + 2, 1);
}

static void settings_click(int mx, int my)
{
    int x = CONTENT_X;
    int y = CONTENT_Y;

    if (inside(mx, my, x + CONTENT_W - 46, y + 2, 40, 28)) {
        settings_page = settings_page ? 0 : 1;
        return;
    }
    if (settings_page == 0)
        settings_click_page1(mx, my);
}

static int arcade_app(int app)
{
    return app == 2 || app == 13 || app == 14 || app == 15 ||
           app == 16 || app == 20 || app == 22;
}

static void draw_window(void)
{
    char title[40];

    gfx_fill(WIN_X - 3, WIN_Y - 3, WIN_W + 6, WIN_H + 6, 0x00081018);
    gfx_fill(WIN_X, WIN_Y, WIN_W, WIN_H, C_WINDOW);
    gfx_fill(WIN_X, WIN_Y, WIN_W, 34, theme_accent[theme_index]);
    gfx_fill(WIN_X, WIN_Y, WIN_W, 4, C_GREEN);

    strcpy(title, language ? app_titles_zh[active_app] : app_titles[active_app]);
    if (active_app == APP_NOTE) {
        strcpy(title + strlen(title), " - ");
        strcpy(title + strlen(title), note_name);
    }
    if (language)
        zh_text(WIN_X + 14, WIN_Y + 10, title, C_WHITE, 1);
    else
        gfx_text(WIN_X + 14, WIN_Y + 9, title, C_WHITE, 2);

    gfx_fill(WIN_X + WIN_W - 34, WIN_Y + 8, 24, 20, C_RED);
    gfx_text(WIN_X + WIN_W - 27, WIN_Y + 11, "x", C_WHITE, 2);

    if (active_app == APP_CALC)
        draw_calculator();
    else if (active_app == APP_CLOCK)
        arcade_draw(active_app, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    else if (active_app == APP_TIMER)
        draw_timer();
    else if (active_app == APP_PAINT)
        draw_paint();
    else if (active_app == APP_NOTE)
        draw_notepad();
    else if (active_app == APP_FILES)
        draw_files();
    else if (active_app == APP_PROG)
        prog_draw(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    else if (active_app == APP_MINES)
        mines_draw(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    else if (active_app == APP_SNAKE)
        snake_draw(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    else if (active_app == APP_SETTINGS)
        draw_settings();
    else if (active_app == APP_SOLITAIRE)
        solitaire_draw(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    else if (active_app == APP_SPIDER)
        spider_draw(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    else if (active_app >= APP_CALENDAR && active_app <= APP_STOPWATCH) {
        if (arcade_app(active_app))
            arcade_draw(active_app, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
        else
            more_draw(active_app, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    }
}

static void draw_cursor(void)
{
    static const uint16_t shape[12] = {
        0x800, 0xC00, 0xE00, 0xF00, 0xF80, 0xFC0,
        0xFE0, 0xFF0, 0xC00, 0xC00, 0x000, 0x000
    };
    int x = mouse_x;
    int y = mouse_y;
    int scale = mouse_cursor_scale;
    int row;
    int column;

    for (row = 0; row < 12; row++) {
        for (column = 0; column < 12; column++) {
            if (shape[row] & (0x800 >> column)) {
                gfx_fill(x + column * scale - 1, y + row * scale - 1,
                         scale + 2, scale + 2, C_BLACK);
            }
        }
    }
    for (row = 0; row < 12; row++) {
        for (column = 0; column < 12; column++) {
            if (shape[row] & (0x800 >> column))
                gfx_fill(x + column * scale, y + row * scale, scale, scale, C_WHITE);
        }
    }
}

static void gui_draw(void)
{
    draw_desktop();
    if (active_app != APP_NONE)
        draw_window();
    draw_cursor();
}

/* ------------------------------------------------------------------ */
/* input                                                               */
/* ------------------------------------------------------------------ */

static void open_app(int app)
{
    active_app = app;
    if (app == APP_PAINT) {
        static int initialized;
        if (!initialized) {
            paint_clear();
            initialized = 1;
        }
    }
    if (app == APP_FILES) {
        files_volume = 0;
        files_selected = 0;
    }
    if (app == APP_PROG)
        prog_init();
    if (app == APP_MINES)
        mines_init();
    if (app == APP_SNAKE)
        snake_init();
    if (app == APP_SETTINGS)
        settings_page = 0;
    if (app == APP_SOLITAIRE)
        solitaire_init();
    if (app == APP_SPIDER)
        spider_init();
    if (app >= APP_CALENDAR && app <= APP_STOPWATCH) {
        if (arcade_app(app))
            arcade_init(app);
        else
            more_init(app);
    }
    if (app == APP_CLOCK)
        arcade_init(app);
    if (app == APP_NOTE && note_name[0] == 0)
        note_open(0, "untitled.txt");
}

static void close_app(void)
{
    active_app = APP_NONE;
}

static void handle_click(int x, int y)
{
    int i;

    if (active_app == APP_NONE) {
        if (inside(x, y, SCREEN_W - 150, 4, 132, 22)) {
            desktop_page = desktop_page ? 0 : 1;
            return;
        }
        if (inside(x, y, 20, SCREEN_H - 72, 60, 30)) {
            desktop_page = desktop_page ? 0 : 1;
            return;
        }
        if (inside(x, y, SCREEN_W - 80, SCREEN_H - 72, 60, 30)) {
            desktop_page = desktop_page ? 0 : 1;
            return;
        }
        int start = desktop_page * 12;
        int end = start + 12;
        if (end > 22)
            end = 22;
        for (i = start; i < end; i++) {
            int ix;
            int iy;
            icon_rect(i, &ix, &iy);
            if (inside(x, y, ix, iy, 84, 74)) {
                open_app(icon_apps[i]);
                return;
            }
        }
        return;
    }

    if (inside(x, y, WIN_X + WIN_W - 34, WIN_Y + 8, 24, 20)) {
        close_app();
        return;
    }

    if (active_app == APP_CALC) {
        int row;
        int column;
        for (row = 0; row < 4; row++) {
            for (column = 0; column < 4; column++) {
                int bx;
                int by;
                calc_button_rect(row, column, &bx, &by);
                if (inside(x, y, bx, by, 128, 60)) {
                    const char *label = calc_labels[row][column];
                    if (label[0] >= '0' && label[0] <= '9')
                        calc_digit(label[0] - '0');
                    else if (label[0] == 'C')
                        calc_clear();
                    else if (label[0] == '=')
                        calc_equals();
                    else
                        calc_operator(label[0]);
                    return;
                }
            }
        }
    } else if (active_app == APP_CLOCK) {
        arcade_click(active_app, x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    } else if (active_app == APP_TIMER) {
        for (i = 0; i < 3; i++) {
            int bx = CONTENT_X + 20 + i * 176;
            int by = CONTENT_Y + 170;
            if (inside(x, y, bx, by, 160, 60)) {
                if (i == 0)
                    timer_start();
                else if (i == 1)
                    timer_stop();
                else
                    timer_reset();
                return;
            }
        }
    } else if (active_app == APP_PAINT) {
        if (inside(x, y, CONTENT_X + 460, CONTENT_Y, 76, 28)) {
            paint_clear();
            return;
        }
        for (i = 0; i < 8; i++) {
            if (inside(x, y, CONTENT_X + i * 40, CONTENT_Y, 34, 28)) {
                paint_color = i;
                return;
            }
        }
        paint_begin(x, y);
    } else if (active_app == APP_NOTE) {
        int bx = CONTENT_X + CONTENT_W - 240;
        int by = CONTENT_Y + CONTENT_H - 44;
        if (inside(x, y, bx, by, 110, 34)) {
            note_save();
            return;
        }
        if (inside(x, y, bx + 120, by, 110, 34)) {
            note_new();
            return;
        }
    } else if (active_app == APP_FILES) {
        for (i = 0; i < 2; i++) {
            if (inside(x, y, CONTENT_X + i * 140, CONTENT_Y, 130, 34)) {
                files_volume = i;
                files_selected = 0;
                return;
            }
        }
        if (inside(x, y, CONTENT_X, CONTENT_Y + 44, CONTENT_W, 200)) {
            int row = (y - (CONTENT_Y + 44)) / 24;
            if (row >= 0 && row < fs_count(files_volume))
                files_selected = row;
            return;
        }
        for (i = 0; i < 4; i++) {
            int bx = CONTENT_X + i * 134;
            int by = CONTENT_Y + 258;
            if (inside(x, y, bx, by, 122, 42)) {
                if (i == 0)
                    files_open_selected();
                else if (i == 1) {
                    char name[20] = "new";
                    int number = fs_count(files_volume) + 1;
                    int_to_str(number, name + 3);
                    strcpy(name + strlen(name), ".txt");
                    fs_create(files_volume, name);
                    note_open(files_volume, name);
                    active_app = APP_NOTE;
                } else if (i == 2) {
                    fs_file *file = fs_at(files_volume, files_selected);
                    if (file)
                        fs_delete(files_volume, file->name);
                    files_refresh();
                } else {
                    fs_file *file = fs_at(files_volume, files_selected);
                    if (file) {
                        fs_copy(files_volume, file->name, 1 - files_volume);
                        if (fs_find(1 - files_volume, file->name))
                            fs_delete(files_volume, file->name);
                        files_refresh();
                    }
                }
                return;
            }
        }
    } else if (active_app == APP_PROG) {
        prog_click(x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    } else if (active_app == APP_MINES) {
        mines_click(x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, 0);
    } else if (active_app == APP_SNAKE) {
        snake_click(x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    } else if (active_app == APP_SETTINGS) {
        settings_click(x, y);
    } else if (active_app == APP_SOLITAIRE) {
        solitaire_click(x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    } else if (active_app == APP_SPIDER) {
        spider_click(x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    } else if (active_app >= APP_CALENDAR && active_app <= APP_STOPWATCH) {
        if (arcade_app(active_app))
            arcade_click(active_app, x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
        else
            more_click(active_app, x, y, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);
    }
}

static void handle_key(int key)
{
    if (active_app == APP_NONE && (key == '[' || key == ']')) {
        desktop_page = desktop_page ? 0 : 1;
        return;
    }
    if (key == 27) {
        close_app();
        return;
    }

    if (active_app == APP_CALC) {
        if (key >= '0' && key <= '9')
            calc_digit(key - '0');
        else if (key == '+' || key == '-' || key == '*' || key == '/')
            calc_operator((char)key);
        else if (key == '=' || key == '\n')
            calc_equals();
        else if (key == 'c' || key == 'C')
            calc_clear();
    } else if (active_app == APP_PAINT) {
        if (key >= '1' && key <= '6')
            paint_tool = key - '1';
    } else if (active_app == APP_NOTE) {
        if (key == '\b') {
            note_backspace();
        } else if (key == '\n' || (key >= 32 && key < 127)) {
            note_putc((char)key);
        }
    } else if (active_app == APP_CLOCK) {
        arcade_key(active_app, key);
    } else if (active_app == APP_TIMER) {
        if (key == ' ')
            timer_running ? timer_stop() : timer_start();
        else if (key == 'r' || key == 'R')
            timer_reset();
    } else if (active_app == APP_FILES) {
        if (key == '\n')
            files_open_selected();
    } else if (active_app == APP_PROG) {
        prog_key(key);
    } else if (active_app == APP_SNAKE) {
        snake_key(key);
    } else if (active_app == APP_SETTINGS) {
        if (key == '+' && mouse_speed < 5)
            mouse_speed++;
        else if (key == '-' && mouse_speed > 1)
            mouse_speed--;
        else if (key == 'r' || key == 'R') {
            mouse_speed = 3;
            mouse_cursor_scale = 1;
            mouse_invert_y = 0;
            theme_index = 0;
        }
    } else if (active_app == APP_SOLITAIRE) {
        solitaire_key(key);
    } else if (active_app == APP_SPIDER) {
        spider_key(key);
    } else if (active_app >= APP_CALENDAR && active_app <= APP_STOPWATCH) {
        if (arcade_app(active_app))
            arcade_key(active_app, key);
        else
            more_key(active_app, key);
    }
}

void gui_run(void)
{
    int dirty = 1;

    active_app = APP_NONE;
    note_open(0, "untitled.txt");

    for (;;) {
        int key;

        if (active_app == APP_TIMER && timer_running)
            dirty = 1;
        if (active_app == APP_SNAKE) {
            snake_tick();
            dirty = 1;
        }
        if (arcade_app(active_app)) {
            static uint32_t last_arcade_tick;
            uint32_t arcade_now = timer_ticks();
            if (arcade_now != last_arcade_tick) {
                last_arcade_tick = arcade_now;
                arcade_tick(active_app);
                dirty = 1;
            }
        }

        if (active_app == APP_PAINT) {
            if (mouse_buttons & 1) {
                if (paint_tool == 0 || paint_tool == 5) {
                    paint_plot(mouse_x, mouse_y);
                    dirty = 1;
                }
            } else {
                paint_drawing = 0;
            }
        }

        if (mouse_moved) {
            mouse_moved = 0;
            dirty = 1;
        }
        if (mouse_clicked) {
            handle_click(mouse_click_x, mouse_click_y);
            mouse_clicked = 0;
            dirty = 1;
        }
        if (mouse_right_clicked) {
            if (active_app == APP_MINES)
                mines_click(mouse_right_x, mouse_right_y,
                            CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, 1);
            mouse_right_clicked = 0;
            dirty = 1;
        }
        if (mouse_released) {
            if (active_app == APP_PAINT)
                paint_end(mouse_x, mouse_y);
            mouse_released = 0;
            dirty = 1;
        }

        while ((key = keyboard_poll()) >= 0) {
            handle_key(key);
            dirty = 1;
        }

        if (dirty) {
            gui_draw();
            gfx_blit();
            dirty = 0;
        }

        cpu_hlt();
    }
}




