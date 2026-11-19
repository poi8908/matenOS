#ifndef MATENOS_CARDS_H
#define MATENOS_CARDS_H

void solitaire_init(void);
void solitaire_draw(int x, int y, int w, int h);
void solitaire_click(int mx, int my, int x, int y, int w, int h);
void solitaire_key(int key);

void spider_init(void);
void spider_draw(int x, int y, int w, int h);
void spider_click(int mx, int my, int x, int y, int w, int h);
void spider_key(int key);

#endif
