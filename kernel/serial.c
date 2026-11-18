#include "serial.h"
#include "io.h"

#define COM1 0x3F8

void serial_init(void)
{
    outb(COM1 + 1, 0x00);   /* disable interrupts */
    outb(COM1 + 3, 0x80);   /* enable divisor latch */
    outb(COM1 + 0, 0x03);   /* 38400 baud */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   /* 8 bits, no parity, one stop bit */
    outb(COM1 + 2, 0xC7);   /* FIFO enabled, cleared */
    outb(COM1 + 4, 0x0B);   /* IRQs enabled, RTS/DSR set */
}

static int transmit_ready(void)
{
    return (inb(COM1 + 5) & 0x20) != 0;
}

void serial_putc(char c)
{
    while (!transmit_ready())
        ;
    outb(COM1, (uint8_t)c);
}

void serial_write(const char *text)
{
    while (*text)
        serial_putc(*text++);
}
