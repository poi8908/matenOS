#include "mouse.h"
#include "gfx.h"
#include "io.h"

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_COMMAND 0x64

volatile int mouse_x;
volatile int mouse_y;
volatile uint8_t mouse_buttons;
volatile int mouse_moved;
volatile int mouse_clicked;
volatile int mouse_right_clicked;
volatile int mouse_released;
volatile int mouse_click_x;
volatile int mouse_click_y;
volatile int mouse_right_x;
volatile int mouse_right_y;

static uint8_t packet[3];
static int packet_index;
static uint8_t previous_buttons;

static int wait_input_clear(void)
{
    int i;
    for (i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS) & 0x02))
            return 1;
    }
    return 0;
}

static int wait_output_full(void)
{
    int i;
    for (i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS) & 0x01)
            return 1;
    }
    return 0;
}

static void mouse_write(uint8_t value)
{
    wait_input_clear();
    outb(PS2_COMMAND, 0xD4);
    wait_input_clear();
    outb(PS2_DATA, value);
}

static uint8_t mouse_read(void)
{
    wait_output_full();
    return inb(PS2_DATA);
}

void mouse_init(void)
{
    uint8_t config;

    mouse_x = gfx_width / 2;
    mouse_y = gfx_height / 2;
    mouse_buttons = 0;
    previous_buttons = 0;
    mouse_moved = 1;
    mouse_clicked = 0;
    mouse_right_clicked = 0;
    mouse_click_x = mouse_x;
    mouse_click_y = mouse_y;
    mouse_right_x = mouse_x;
    mouse_right_y = mouse_y;
    packet_index = 0;

    while (inb(PS2_STATUS) & 0x01)
        inb(PS2_DATA);

    outb(PS2_COMMAND, 0xA8);        /* enable auxiliary PS/2 port */

    outb(PS2_COMMAND, 0x20);        /* read controller configuration */
    config = mouse_read();
    config |= 0x02;                 /* enable IRQ12 */
    config &= (uint8_t)~0x20;       /* enable auxiliary clock */
    wait_input_clear();
    outb(PS2_COMMAND, 0x60);
    wait_input_clear();
    outb(PS2_DATA, config);

    mouse_write(0xF6);              /* set defaults */
    mouse_read();
    mouse_write(0xF4);              /* enable data reporting */
    mouse_read();
}

void mouse_irq(void)
{
    uint8_t status = inb(PS2_STATUS);
    uint8_t data;
    int dx;
    int dy;
    uint8_t buttons;

    if (!(status & 0x20))
        return;

    data = inb(PS2_DATA);

    if (packet_index == 0 && !(data & 0x08)) {
        packet_index = 0;
        return;
    }

    packet[packet_index++] = data;
    if (packet_index < 3)
        return;
    packet_index = 0;

    dx = (int)packet[1];
    dy = (int)packet[2];
    if (packet[0] & 0x10)
        dx -= 256;
    if (packet[0] & 0x20)
        dy -= 256;
    dy = -dy;
    buttons = (uint8_t)(packet[0] & 0x07);

    mouse_x += dx;
    mouse_y += dy;
    if (mouse_x < 0)
        mouse_x = 0;
    if (mouse_y < 0)
        mouse_y = 0;
    if (mouse_x > gfx_width - 1)
        mouse_x = gfx_width - 1;
    if (mouse_y > gfx_height - 1)
        mouse_y = gfx_height - 1;

    if ((buttons & 1) && !(previous_buttons & 1)) {
        mouse_clicked = 1;
        mouse_click_x = mouse_x;
        mouse_click_y = mouse_y;
    }
    if ((buttons & 2) && !(previous_buttons & 2)) {
        mouse_right_clicked = 1;
        mouse_right_x = mouse_x;
        mouse_right_y = mouse_y;
    }
    if (!(buttons & 1) && (previous_buttons & 1))
        mouse_released = 1;

    previous_buttons = buttons;
    mouse_buttons = buttons;
    mouse_moved = 1;
}

