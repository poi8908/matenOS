#include "console.h"
#include "io.h"
#include "string.h"

#define VGA_TEXT ((volatile uint16_t *)0xB8000)
#define BLANK     ((uint16_t)(' ' | (0x07 << 8)))

static uint8_t attribute = 0x0A;
static int cursor_row;
static int cursor_column;

static void move_hardware_cursor(void)
{
    uint16_t position = (uint16_t)(cursor_row * VGA_WIDTH + cursor_column);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(position >> 8));
}

static void scroll(void)
{
    int i;

    for (i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++)
        VGA_TEXT[i] = VGA_TEXT[i + VGA_WIDTH];
    for (i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_TEXT[i] = BLANK;
    cursor_row = VGA_HEIGHT - 1;
}

void console_init(void)
{
    cursor_row = 0;
    cursor_column = 0;
    attribute = (uint8_t)(VGA_LIGHT_GREEN | (VGA_BLACK << 4));
    console_clear();
}

void console_clear(void)
{
    int i;

    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_TEXT[i] = BLANK;
    cursor_row = 0;
    cursor_column = 0;
    move_hardware_cursor();
}

void console_set_color(uint8_t foreground, uint8_t background)
{
    attribute = (uint8_t)((background << 4) | (foreground & 0x0F));
}

void console_goto(int row, int column)
{
    if (row < 0)
        row = 0;
    if (row >= VGA_HEIGHT)
        row = VGA_HEIGHT - 1;
    if (column < 0)
        column = 0;
    if (column >= VGA_WIDTH)
        column = VGA_WIDTH - 1;
    cursor_row = row;
    cursor_column = column;
    move_hardware_cursor();
}

void console_putc(char c)
{
    if (c == '\n') {
        cursor_column = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_column = 0;
    } else if (c == '\b') {
        if (cursor_column > 0) {
            cursor_column--;
            VGA_TEXT[cursor_row * VGA_WIDTH + cursor_column] = BLANK;
        }
    } else if (c == '\t') {
        int next = (cursor_column + 8) & ~7;
        while (cursor_column < next && cursor_column < VGA_WIDTH) {
            VGA_TEXT[cursor_row * VGA_WIDTH + cursor_column] =
                (uint16_t)(unsigned char)' ' | ((uint16_t)attribute << 8);
            cursor_column++;
        }
    } else {
        VGA_TEXT[cursor_row * VGA_WIDTH + cursor_column] =
            (uint16_t)(unsigned char)c | ((uint16_t)attribute << 8);
        cursor_column++;
        if (cursor_column >= VGA_WIDTH) {
            cursor_column = 0;
            cursor_row++;
        }
    }

    if (cursor_row >= VGA_HEIGHT)
        scroll();
    move_hardware_cursor();
}

void console_write(const char *text)
{
    while (*text)
        console_putc(*text++);
}

void console_write_centered(int row, const char *text)
{
    int length = (int)strlen(text);
    int column;

    if (length > VGA_WIDTH)
        length = VGA_WIDTH;
    column = (VGA_WIDTH - length) / 2;

    cursor_row = row;
    cursor_column = column;
    while (*text)
        console_putc(*text++);
    cursor_column = column + length;
}

void console_write_dec(uint32_t value)
{
    char digits[11];
    int index = 0;

    if (value == 0) {
        console_putc('0');
        return;
    }
    while (value > 0) {
        digits[index++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (index > 0)
        console_putc(digits[--index]);
}

void console_write_hex(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    int shift;

    console_write("0x");
    for (shift = 28; shift >= 0; shift -= 4)
        console_putc(hex[(value >> shift) & 0x0F]);
}
