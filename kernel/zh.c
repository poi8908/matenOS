#include "zh.h"
#include "gfx.h"
#include "string.h"

int zh_language;

const char *zh_tr(const char *text)
{
    return text;
}

uint32_t zh_utf8_next(const char **text)
{
    unsigned char c = (unsigned char)**text;
    if (!c)
        return 0;
    (*text)++;
    return c;
}

int zh_text_width(const char *text, int scale)
{
    return (int)strlen(text) * 8 * scale;
}

void zh_text(int x, int y, const char *text, uint32_t color, int scale)
{
    while (*text) {
        gfx_char(x, y, *text, color, scale);
        x += 8 * scale;
        text++;
    }
}
