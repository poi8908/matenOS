#include "prog.h"
#include "fs.h"
#include "gfx.h"
#include "string.h"
#include "vm.h"

#define PROG_MAX 256

#define C_TEXT  0x00141C24
#define C_MUTED 0x00607080
#define C_GREEN 0x006EE7A8
#define C_WHITE 0x00FFFFFF
#define C_BLUE  0x004A90D9
#define C_TITLE 0x002F6699
#define C_BLACK 0x00000000

static uint8_t code[PROG_MAX];
static int code_length;
static int nibble = -1;
static char output[512];
static int output_length;
static vm_state registers;
static int has_result;
static int volume;
static char file_name[FS_MAX_NAME] = "program.bin";
static char status[48];

static const uint8_t demo_code[] = {
    0xB8, 0x48, 0x00, 0x00, 0x00, 0xCD, 0x20,
    0xB8, 0x69, 0x00, 0x00, 0x00, 0xCD, 0x20,
    0xB8, 0x21, 0x00, 0x00, 0x00, 0xCD, 0x20,
    0xB8, 0x0A, 0x00, 0x00, 0x00, 0xCD, 0x20,
    0xF4
};

static void set_status(const char *text)
{
    strcpy(status, text);
}

static void run_program(void)
{
    output_length = 0;
    output[0] = 0;
    if (code_length == 0) {
        set_status("no code");
        return;
    }
    vm_run(code, code_length, output, (int)sizeof(output), &registers);
    output_length = (int)strlen(output);
    has_result = 1;
    set_status("done");
}

static void load_demo(void)
{
    code_length = (int)sizeof(demo_code);
    memcpy(code, demo_code, sizeof(demo_code));
    nibble = -1;
    has_result = 0;
    set_status("demo loaded");
}

void prog_init(void)
{
    code_length = 0;
    nibble = -1;
    output[0] = 0;
    output_length = 0;
    has_result = 0;
    volume = 0;
    strcpy(file_name, "program.bin");
    set_status("type hex bytes, Enter = run");
}

void prog_open(int vol, const char *name)
{
    fs_file *file = fs_find(vol, name);

    volume = vol;
    strcpy(file_name, name);
    code_length = 0;
    nibble = -1;
    has_result = 0;
    output[0] = 0;
    output_length = 0;

    if (file) {
        code_length = file->size;
        if (code_length > PROG_MAX)
            code_length = PROG_MAX;
        memcpy(code, file->data, (size_t)code_length);
        set_status("loaded");
    } else {
        set_status("new program");
    }
}

static void save_program(void)
{
    fs_write(volume, file_name, code, code_length);
    set_status("saved");
}

void prog_key(int key)
{
    int value = -1;

    if (key >= '0' && key <= '9')
        value = key - '0';
    else if (key >= 'a' && key <= 'f')
        value = key - 'a' + 10;
    else if (key >= 'A' && key <= 'F')
        value = key - 'A' + 10;

    if (value >= 0) {
        if (code_length >= PROG_MAX)
            return;
        if (nibble < 0) {
            nibble = value;
        } else {
            code[code_length++] = (uint8_t)((nibble << 4) | value);
            nibble = -1;
        }
        return;
    }

    if (key == '\b') {
        if (nibble >= 0)
            nibble = -1;
        else if (code_length > 0)
            code_length--;
    } else if (key == '\n') {
        run_program();
    } else if (key == 'x' || key == 'X') {
        code_length = 0;
        nibble = -1;
        has_result = 0;
        output[0] = 0;
        set_status("cleared");
    }
}

static void button_rect(int index, int x, int y, int width, int height,
                        int *bx, int *by)
{
    (void)width;
    *bx = x + 8 + index * 132;
    *by = y + height - 56;
}

void prog_click(int mouse_x, int mouse_y, int x, int y, int width, int height)
{
    int i;

    for (i = 0; i < 4; i++) {
        int bx;
        int by;
        button_rect(i, x, y, width, height, &bx, &by);
        if (mouse_x >= bx && mouse_x < bx + 122 &&
            mouse_y >= by && mouse_y < by + 38) {
            if (i == 0)
                run_program();
            else if (i == 1) {
                code_length = 0;
                nibble = -1;
                has_result = 0;
                output[0] = 0;
                set_status("cleared");
            } else if (i == 2)
                load_demo();
            else
                save_program();
            return;
        }
    }

    if (mouse_x >= x && mouse_x < x + 300 && mouse_y >= y + 18 && mouse_y < y + 238) {
        int row = (mouse_y - (y + 18)) / 8;
        int column = (mouse_x - x) / 24;
        int position = row * 12 + column;
        if (position >= 0 && position <= code_length)
            code_length = position;
        nibble = -1;
    }
}

static void hex_byte(char *buffer, uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    buffer[0] = digits[(value >> 4) & 0x0F];
    buffer[1] = digits[value & 0x0F];
    buffer[2] = ' ';
    buffer[3] = 0;
}

void prog_draw(int x, int y, int width, int height)
{
    int i;
    int line;
    int column;
    int bx;
    int by;
    const char *labels[4] = { "Run", "Clear", "Demo", "Save" };

    gfx_text(x, y, "x86 machine code - hex bytes", C_TEXT, 1);
    gfx_fill(x, y + 18, 300, 220, C_WHITE);
    gfx_rect(x, y + 18, 300, 220, C_MUTED);

    for (i = 0; i < code_length; i++) {
        char text[4];
        line = i / 12;
        column = i % 12;
        hex_byte(text, code[i]);
        gfx_text(x + 4 + column * 24, y + 22 + line * 8, text, C_TEXT, 1);
    }
    if (nibble >= 0) {
        static const char digits[] = "0123456789ABCDEF";
        char text[4];
        text[0] = digits[nibble];
        text[1] = '_';
        text[2] = 0;
        i = code_length;
        line = i / 12;
        column = i % 12;
        gfx_text(x + 4 + column * 24, y + 22 + line * 8, text, C_BLUE, 1);
    }

    gfx_text(x + 310, y, "output / registers", C_TEXT, 1);
    gfx_fill(x + 310, y + 18, width - 310, 220, 0x00101820);
    gfx_rect(x + 310, y + 18, width - 310, 220, C_MUTED);

    line = 0;
    column = 0;
    for (i = 0; i < output_length && line < 14; i++) {
        char c = output[i];
        if (c == '\n') {
            line++;
            column = 0;
            continue;
        }
        if (column >= 26) {
            line++;
            column = 0;
        }
        if (line < 14)
            gfx_char(x + 316 + column * 8, y + 22 + line * 8, c, C_GREEN, 1);
        column++;
    }

    if (has_result) {
        char text[16];
        int base = y + 148;
        static const char *names[5] = { "EAX", "ECX", "EDX", "EBX", "EIP" };
        for (i = 0; i < 5; i++) {
            int j;
            gfx_text(x + 316, base + i * 10, names[i], C_MUTED, 1);
            for (j = 0; j < 8; j++)
                text[j] = "0123456789ABCDEF"[(registers.r[i] >> (28 - j * 4)) & 0xF];
            text[8] = 0;
            gfx_text(x + 352, base + i * 10, text, C_WHITE, 1);
        }
    }

    gfx_text(x, y + 246, status, C_MUTED, 1);
    gfx_text(x + 300, y + 246, "INT 20=putc 21=dec 22=nl 23=hex", C_MUTED, 1);

    for (i = 0; i < 4; i++) {
        button_rect(i, x, y, width, height, &bx, &by);
        gfx_fill(bx, by, 122, 38, i == 0 ? C_GREEN : C_TITLE);
        gfx_text(bx + (122 - gfx_text_width(labels[i], 2)) / 2, by + 10,
                 labels[i], i == 0 ? C_BLACK : C_WHITE, 2);
    }
}
