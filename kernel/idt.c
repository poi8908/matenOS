#include "idt.h"
#include "console.h"
#include "io.h"
#include "keyboard.h"
#include "mouse.h"
#include "panic.h"
#include "pic.h"
#include "string.h"
#include "timer.h"

#pragma pack(push, 1)
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
};

struct idt_pointer {
    uint16_t limit;
    uint32_t base;
};
#pragma pack(pop)

static struct idt_entry idt[256];
static struct idt_pointer idtp;

extern void idt_load(struct idt_pointer *pointer);

#define DECLARE_ISR(number) extern void isr##number(void);
DECLARE_ISR(0)  DECLARE_ISR(1)  DECLARE_ISR(2)  DECLARE_ISR(3)
DECLARE_ISR(4)  DECLARE_ISR(5)  DECLARE_ISR(6)  DECLARE_ISR(7)
DECLARE_ISR(8)  DECLARE_ISR(9)  DECLARE_ISR(10) DECLARE_ISR(11)
DECLARE_ISR(12) DECLARE_ISR(13) DECLARE_ISR(14) DECLARE_ISR(15)
DECLARE_ISR(16) DECLARE_ISR(17) DECLARE_ISR(18) DECLARE_ISR(19)
DECLARE_ISR(20) DECLARE_ISR(21) DECLARE_ISR(22) DECLARE_ISR(23)
DECLARE_ISR(24) DECLARE_ISR(25) DECLARE_ISR(26) DECLARE_ISR(27)
DECLARE_ISR(28) DECLARE_ISR(29) DECLARE_ISR(30) DECLARE_ISR(31)
DECLARE_ISR(32) DECLARE_ISR(33) DECLARE_ISR(34) DECLARE_ISR(35)
DECLARE_ISR(36) DECLARE_ISR(37) DECLARE_ISR(38) DECLARE_ISR(39)
DECLARE_ISR(40) DECLARE_ISR(41) DECLARE_ISR(42) DECLARE_ISR(43)
DECLARE_ISR(44) DECLARE_ISR(45) DECLARE_ISR(46) DECLARE_ISR(47)

static const char *exception_names[32] = {
    "divide error", "debug", "non-maskable interrupt", "breakpoint",
    "overflow", "bound range exceeded", "invalid opcode", "device not available",
    "double fault", "coprocessor segment overrun", "invalid TSS",
    "segment not present", "stack-segment fault", "general protection fault",
    "page fault", "reserved", "x87 floating-point exception", "alignment check",
    "machine check", "SIMD floating-point exception", "virtualization exception",
    "control protection exception", "reserved", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved", "reserved", "reserved",
    "reserved"
};

void idt_set_gate(uint8_t number, uint32_t base, uint16_t selector, uint8_t flags)
{
    idt[number].base_low = (uint16_t)(base & 0xFFFF);
    idt[number].selector = selector;
    idt[number].zero = 0;
    idt[number].flags = flags;
    idt[number].base_high = (uint16_t)((base >> 16) & 0xFFFF);
}

#define SET_GATE(number) idt_set_gate((number), (uint32_t)isr##number, 0x08, 0x8E);

void idt_init(void)
{
    memset(idt, 0, sizeof(idt));
    idtp.limit = (uint16_t)(sizeof(idt) - 1);
    idtp.base = (uint32_t)&idt;

    SET_GATE(0)  SET_GATE(1)  SET_GATE(2)  SET_GATE(3)
    SET_GATE(4)  SET_GATE(5)  SET_GATE(6)  SET_GATE(7)
    SET_GATE(8)  SET_GATE(9)  SET_GATE(10) SET_GATE(11)
    SET_GATE(12) SET_GATE(13) SET_GATE(14) SET_GATE(15)
    SET_GATE(16) SET_GATE(17) SET_GATE(18) SET_GATE(19)
    SET_GATE(20) SET_GATE(21) SET_GATE(22) SET_GATE(23)
    SET_GATE(24) SET_GATE(25) SET_GATE(26) SET_GATE(27)
    SET_GATE(28) SET_GATE(29) SET_GATE(30) SET_GATE(31)
    SET_GATE(32) SET_GATE(33) SET_GATE(34) SET_GATE(35)
    SET_GATE(36) SET_GATE(37) SET_GATE(38) SET_GATE(39)
    SET_GATE(40) SET_GATE(41) SET_GATE(42) SET_GATE(43)
    SET_GATE(44) SET_GATE(45) SET_GATE(46) SET_GATE(47)

    idt_load(&idtp);
}

static void dispatch_irq(uint32_t irq)
{
    if (irq == 0)
        timer_tick();
    else if (irq == 1)
        keyboard_irq();
    else if (irq == 12)
        mouse_irq();

    pic_eoi((uint8_t)irq);
}

void isr_handler(struct regs *registers)
{
    if (registers->int_no < 32) {
        panic(exception_names[registers->int_no], registers);
    } else if (registers->int_no < 48) {
        dispatch_irq(registers->int_no - 32);
    }
}
