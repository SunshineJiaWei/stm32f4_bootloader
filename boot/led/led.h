#ifndef __LED_H
#define __LED_H


#include "stm32f4xx.h"
#include <stdbool.h>


typedef struct led
{
    GPIO_TypeDef* GPIO_Port;
    uint32_t GPIO_Pin;
    bool state;
}led_t;


void bl_led_init(led_t *led);
void bl_led_set(led_t *led);
void bl_led_on(led_t *led);
void bl_led_off(led_t *led);
void bl_led_toggle(led_t *led);


extern led_t led0;


#endif /* __LED_H */