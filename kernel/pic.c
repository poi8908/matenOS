#include "pic.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

void pic_remap(void)
{
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    outb(PIC1_DATA, 0x20);      /* master vector offset: 32 */
    io_wait();
    outb(PIC2_DATA, 0x28);      /* slave vector offset: 40 */
    io_wait();

    outb(PIC1_DATA, 0x04);      /* slave is on IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    outb(PIC1_DATA, 0x01);      /* 8086 mode */
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    outb(PIC1_DATA, 0xF8);      /* IRQ0 timer, IRQ1 keyboard, IRQ2 cascade */
    outb(PIC2_DATA, 0xEF);      /* IRQ12 PS/2 mouse */
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);
}
