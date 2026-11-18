#ifndef MATENOS_GAMES_H
#define MATENOS_GAMES_H

void mines_init(void);
void mines_draw(int x, int y, int w, int h);
void mines_click(int mx, int my, int x, int y, int w, int h, int right);

void snake_init(void);
void snake_draw(int x, int y, int w, int h);
void snake_click(int mx, int my, int x, int y, int w, int h);
void snake_key(int key);
void snake_tick(void);

#endif
