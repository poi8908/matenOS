#include "string.h"

void *memset(void *dst, int value, size_t count)
{
    unsigned char *out = (unsigned char *)dst;
    while (count--)
        *out++ = (unsigned char)value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t count)
{
    unsigned char *out = (unsigned char *)dst;
    const unsigned char *in = (const unsigned char *)src;
    while (count--)
        *out++ = *in++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t count)
{
    unsigned char *out = (unsigned char *)dst;
    const unsigned char *in = (const unsigned char *)src;

    if (out < in) {
        while (count--)
            *out++ = *in++;
    } else if (out > in) {
        out += count;
        in += count;
        while (count--)
            *--out = *--in;
    }
    return dst;
}

size_t strlen(const char *text)
{
    size_t length = 0;
    while (text[length])
        length++;
    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left && *left == *right) {
        left++;
        right++;
    }
    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t count)
{
    while (count && *left && *left == *right) {
        left++;
        right++;
        count--;
    }
    if (!count)
        return 0;
    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

char *strcpy(char *dst, const char *src)
{
    char *result = dst;
    while ((*dst++ = *src++))
        ;
    return result;
}

int atoi(const char *text)
{
    int value = 0;
    int sign = 1;

    while (*text == ' ')
        text++;
    if (*text == '-') {
        sign = -1;
        text++;
    }
    while (*text >= '0' && *text <= '9')
        value = value * 10 + (*text++ - '0');
    return value * sign;
}
