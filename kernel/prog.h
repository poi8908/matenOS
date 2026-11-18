#ifndef MATENOS_PROG_H
#define MATENOS_PROG_H

#include "fs.h"

void prog_draw(int x, int y, int width, int height);
void prog_click(int mouse_x, int mouse_y, int x, int y, int width, int height);
void prog_key(int key);
void prog_open(int volume, const char *name);
void prog_init(void);

#endif
