#ifndef MATENOS_MOUSE_H
#define MATENOS_MOUSE_H

#include <stdint.h>

extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile uint8_t mouse_buttons;
extern volatile int mouse_moved;
extern volatile int mouse_clicked;
extern volatile int mouse_right_clicked;
extern volatile int mouse_released;
extern volatile int mouse_click_x;
extern volatile int mouse_click_y;
extern volatile int mouse_right_x;
extern volatile int mouse_right_y;
extern int mouse_speed;
extern int mouse_cursor_scale;
extern int mouse_invert_y;

void mouse_init(void);
void mouse_irq(void);
void mouse_poll_events(void);

#endif
