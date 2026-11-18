#ifndef MATENOS_KEYBOARD_H
#define MATENOS_KEYBOARD_H

void keyboard_init(void);
void keyboard_irq(void);
char keyboard_getchar(void);
int keyboard_poll(void);

#endif
