#ifndef __MAIN_H
#define __MAIN_H


#include <stdint.h>
#include "led.h"
#include "button.h"
#include "uart.h"


void bl_lowlevel_init(void);
void bootloader_main(uint32_t delay_ms);

void bl_delay_init(void);
void bl_delay_ms(uint32_t ms);
uint32_t bl_now(void);


#endif /* __MAIN_H */