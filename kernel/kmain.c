#include "fs.h"
#include "gfx.h"
#include "gui.h"
#include "idt.h"
#include "io.h"
#include "keyboard.h"
#include "mouse.h"
#include "pic.h"
#include "serial.h"
#include "timer.h"

static void splash(void)
{
    uint32_t start;

    gfx_clear(0x00081018);
    gfx_text(140, 120, "hello,matenOS!", 0x006EE7A8, 3);
    gfx_text(170, 180, "a 32-bit operating system written in C", 0x00C0D0E0, 1);
    gfx_text(226, 214, "starting graphical shell...", 0x00607080, 1);
    gfx_blit();

    start = timer_ticks();
    while (timer_ticks() - start < 120)
        cpu_hlt();
}

void kmain(void)
{
    serial_init();
    gfx_init();
    fs_init();

    idt_init();
    pic_remap();
    timer_init();
    keyboard_init();
    mouse_init();

    if (!gfx_ready) {
        serial_write("no graphics mode\n");
        for (;;)
            cpu_hlt();
    }

    serial_write("[ ok ] GUI initialized\n");
    cpu_sti();

    splash();
    gui_run();
}
