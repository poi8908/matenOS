#ifndef MATENOS_PANIC_H
#define MATENOS_PANIC_H

struct regs;

void panic(const char *message, struct regs *registers);

#endif
