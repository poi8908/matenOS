#include "timer.h"
#include "io.h"

#define PIT_FREQUENCY 1193182u
#define PIT_HZ        100u

static volatile uint32_t ticks;

void timer_init(void)
{
    uint16_t divisor = (uint16_t)(PIT_FREQUENCY / PIT_HZ);

    outb(0x43, 0x36);                       /* channel 0, lobyte/hibyte, mode 3 */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
}

void timer_tick(void)
{
    ticks++;
}

uint32_t timer_ticks(void)
{
    return ticks;
}
