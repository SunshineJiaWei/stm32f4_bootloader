#ifndef __BUTTON_H
#define __BUTTON_H


#include <stdbool.h>


void bl_button_init(void);
bool bl_button_pressed(void);
void button_wait_release(void);


#endif /* __BUTTON_H */