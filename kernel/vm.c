#include "vm.h"
#include "string.h"

static int emit(char *output, int size, int *used, char c)
{
    if (*used >= size - 1)
        return 0;
    output[(*used)++] = c;
    output[*used] = 0;
    return 1;
}

static void emit_text(char *output, int size, int *used, const char *text)
{
    while (*text)
        emit(output, size, used, *text++);
}

static void emit_dec(char *output, int size, int *used, uint32_t value)
{
    char temp[12];
    int count = 0;

    if (value == 0) {
        emit(output, size, used, '0');
        return;
    }
    while (value > 0) {
        temp[count++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (count > 0)
        emit(output, size, used, temp[--count]);
}

static void emit_hex(char *output, int size, int *used, uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    int shift;

    emit_text(output, size, used, "0x");
    for (shift = 28; shift >= 0; shift -= 4)
        emit(output, size, used, digits[(value >> shift) & 0x0F]);
}

static uint32_t read32(const uint8_t *code, int length, int *ip)
{
    uint32_t value;

    if (*ip + 4 > length)
        return 0;
    value = (uint32_t)code[*ip] |
            ((uint32_t)code[*ip + 1] << 8) |
            ((uint32_t)code[*ip + 2] << 16) |
            ((uint32_t)code[*ip + 3] << 24);
    *ip += 4;
    return value;
}

static int8_t read8s(const uint8_t *code, int length, int *ip)
{
    if (*ip >= length)
        return 0;
    return (int8_t)code[(*ip)++];
}

static void set_zf(vm_state *s, uint32_t value)
{
    s->zf = value == 0;
}

int vm_run(const uint8_t *code, int length, char *output, int output_size, vm_state *s)
{
    int ip = 0;
    int steps = 0;
    int used = 0;

    memset(s, 0, sizeof(*s));
    output[0] = 0;

    if (length <= 0) {
        emit_text(output, output_size, &used, "no code");
        return 0;
    }

    while (ip < length && steps < 100000) {
        uint8_t op = code[ip++];
        steps++;

        if (op >= 0xB8 && op <= 0xBF) {
            uint32_t value;
            if (ip + 4 > length)
                break;
            value = read32(code, length, &ip);
            s->r[op - 0xB8] = value;
        } else if (op >= 0x40 && op <= 0x47) {
            s->r[op - 0x40]++;
            set_zf(s, s->r[op - 0x40]);
        } else if (op >= 0x48 && op <= 0x4F) {
            s->r[op - 0x48]--;
            set_zf(s, s->r[op - 0x48]);
        } else if (op == 0x90) {
            /* nop */
        } else if (op == 0x89 || op == 0x8B || op == 0x31 || op == 0x01 || op == 0x29) {
            uint8_t modrm;
            int reg;
            int rm;
            if (ip >= length)
                break;
            modrm = code[ip++];
            if ((modrm & 0xC0) != 0xC0) {
                emit_text(output, output_size, &used, "\nmemory operand not supported");
                return -1;
            }
            reg = (modrm >> 3) & 7;
            rm = modrm & 7;
            if (op == 0x89) {
                s->r[rm] = s->r[reg];
            } else if (op == 0x8B) {
                s->r[reg] = s->r[rm];
            } else if (op == 0x31) {
                s->r[rm] ^= s->r[reg];
                set_zf(s, s->r[rm]);
            } else if (op == 0x01) {
                s->r[rm] += s->r[reg];
                set_zf(s, s->r[rm]);
            } else {
                s->r[rm] -= s->r[reg];
                set_zf(s, s->r[rm]);
            }
        } else if (op == 0x83) {
            uint8_t modrm;
            int reg;
            int rm;
            int8_t immediate;
            if (ip + 2 > length)
                break;
            modrm = code[ip++];
            immediate = (int8_t)code[ip++];
            if ((modrm & 0xC0) != 0xC0) {
                emit_text(output, output_size, &used, "\nmemory operand not supported");
                return -1;
            }
            reg = (modrm >> 3) & 7;
            rm = modrm & 7;
            if (reg == 0) {
                s->r[rm] += (uint32_t)(int32_t)immediate;
                set_zf(s, s->r[rm]);
            } else if (reg == 5) {
                s->r[rm] -= (uint32_t)(int32_t)immediate;
                set_zf(s, s->r[rm]);
            } else if (reg == 7) {
                set_zf(s, s->r[rm] - (uint32_t)(int32_t)immediate);
            } else {
                emit_text(output, output_size, &used, "\nbad 0x83 group");
                return -1;
            }
        } else if (op == 0x3D) {
            uint32_t value;
            if (ip + 4 > length)
                break;
            value = read32(code, length, &ip);
            set_zf(s, s->r[0] - value);
        } else if (op == 0x74 || op == 0x75) {
            int8_t displacement;
            if (ip >= length)
                break;
            displacement = read8s(code, length, &ip);
            if ((op == 0x74 && s->zf) || (op == 0x75 && !s->zf))
                ip += displacement;
        } else if (op == 0xEB) {
            int8_t displacement;
            if (ip >= length)
                break;
            displacement = read8s(code, length, &ip);
            ip += displacement;
        } else if (op == 0xE9) {
            int32_t displacement;
            if (ip + 4 > length)
                break;
            displacement = (int32_t)read32(code, length, &ip);
            ip += displacement;
        } else if (op == 0xCD) {
            uint8_t number;
            if (ip >= length)
                break;
            number = code[ip++];
            if (number == 0x20) {
                emit(output, output_size, &used, (char)(s->r[0] & 0xFF));
            } else if (number == 0x21) {
                emit_dec(output, output_size, &used, s->r[0]);
            } else if (number == 0x22) {
                emit(output, output_size, &used, '\n');
            } else if (number == 0x23) {
                emit_hex(output, output_size, &used, s->r[0]);
            } else if (number == 0x24) {
                break;
            } else {
                emit_text(output, output_size, &used, "\nunknown int");
                return -1;
            }
        } else if (op == 0xF4 || op == 0xC3) {
            break;
        } else {
            emit_text(output, output_size, &used, "\nbad opcode at 0x");
            {
                char hex[9];
                static const char digits[] = "0123456789ABCDEF";
                int i;
                uint32_t at = (uint32_t)(ip - 1);
                for (i = 7; i >= 0; i--) {
                    hex[i] = digits[at & 0xF];
                    at >>= 4;
                }
                hex[8] = 0;
                emit_text(output, output_size, &used, hex);
            }
            return -1;
        }

        if (ip < 0 || ip > 256) {
            emit_text(output, output_size, &used, "\npc out of range");
            return -1;
        }
    }

    s->eip = (uint32_t)ip;
    if (steps >= 100000)
        emit_text(output, output_size, &used, "\nstep limit");
    return 0;
}
