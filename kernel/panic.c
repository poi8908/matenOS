#include "panic.h"
#include "console.h"
#include "idt.h"
#include "io.h"
#include "serial.h"

static void serial_hex(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    int shift;

    serial_write("0x");
    for (shift = 28; shift >= 0; shift -= 4)
        serial_putc(hex[(value >> shift) & 0x0F]);
}

void panic(const char *message, struct regs *registers)
{
    cpu_cli();

    console_set_color(VGA_LIGHT_RED, VGA_BLACK);
    console_write("\n\n*** matenOS kernel panic ***\n");
    console_write("reason: ");
    console_write(message);
    console_write("\n");

    serial_write("\n*** matenOS kernel panic ***\nreason: ");
    serial_write(message);
    serial_write("\n");

    if (registers) {
        console_write("vector: ");
        console_write_dec(registers->int_no);
        console_write("  error: ");
        console_write_hex(registers->err_code);
        console_write("\neip: ");
        console_write_hex(registers->eip);
        console_write("  cs: ");
        console_write_hex(registers->cs);
        console_write("  eflags: ");
        console_write_hex(registers->eflags);
        console_write("\n");

        serial_write("vector: ");
        serial_write("?");
        serial_write("  error: ");
        serial_hex(registers->err_code);
        serial_write("  eip: ");
        serial_hex(registers->eip);
        serial_write("\n");
    }

    console_write("system halted.");
    console_write("\n");
    serial_write("system halted.\n");

    for (;;)
        cpu_hlt();
}
