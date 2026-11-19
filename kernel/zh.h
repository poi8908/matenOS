#ifndef MATENOS_ZH_H
#define MATENOS_ZH_H

#include <stdint.h>

typedef struct {
    uint32_t codepoint;
    uint8_t bits[32];
} zh_glyph_t;

extern const zh_glyph_t zh_font[];
extern const int zh_font_count;
extern int zh_language;

const char *zh_tr(const char *text);

uint32_t zh_utf8_next(const char **text);
int zh_text_width(const char *text, int scale);
void zh_text(int x, int y, const char *text, uint32_t color, int scale);

#endif
