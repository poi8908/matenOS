#ifndef MATENOS_GFX_H
#define MATENOS_GFX_H

#include <stdint.h>

#define GFX_BACKBUFFER ((uint32_t *)0x00200000)
#define GFX_FONT       ((const uint8_t *)0x00008000)

extern int gfx_ready;
extern int gfx_width;
extern int gfx_height;

void gfx_init(void);
void gfx_clear(uint32_t color);
void gfx_pixel(int x, int y, uint32_t color);
void gfx_fill(int x, int y, int width, int height, uint32_t color);
void gfx_rect(int x, int y, int width, int height, uint32_t color);
void gfx_hline(int x, int y, int width, uint32_t color);
void gfx_vline(int x, int y, int height, uint32_t color);
void gfx_char(int x, int y, char c, uint32_t color, int scale);
void gfx_text(int x, int y, const char *text, uint32_t color, int scale);
int  gfx_text_width(const char *text, int scale);
void gfx_blit(void);

#endif
