#ifndef MATENOS_ARCADE_H
#define MATENOS_ARCADE_H

void arcade_init(int app);
void arcade_draw(int app, int x, int y, int w, int h);
void arcade_click(int app, int mx, int my, int x, int y, int w, int h);
void arcade_key(int app, int key);
void arcade_tick(int app);

#endif
