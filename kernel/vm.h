#ifndef MATENOS_VM_H
#define MATENOS_VM_H

#include <stdint.h>

typedef struct {
    uint32_t r[8];      /* eax, ecx, edx, ebx, esp, ebp, esi, edi */
    uint32_t eip;
    int zf;
} vm_state;

int vm_run(const uint8_t *code, int length, char *output, int output_size, vm_state *state);

#endif
