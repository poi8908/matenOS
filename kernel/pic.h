#ifndef MATENOS_PIC_H
#define MATENOS_PIC_H

#include <stdint.h>

void pic_remap(void);
void pic_eoi(uint8_t irq);

#endif
