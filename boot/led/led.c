#include <stdbool.h>
#include "stm32f4xx.h"
#include "led.h"


void bl_led_init(led_t *led)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = led->GPIO_Pin;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Medium_Speed;

    GPIO_Init(led->GPIO_Port, &GPIO_InitStructure);

    bl_led_off(led);
}

void bl_led_set(led_t *led)
{
    GPIO_WriteBit(led->GPIO_Port, led->GPIO_Pin, led->state ? Bit_RESET : Bit_SET);
}

void bl_led_on(led_t *led)
{
    led->state = true;
    bl_led_set(led);
}

void bl_led_off(led_t *led)
{
    led->state = false;
    bl_led_set(led);
}

void bl_led_toggle(led_t *led)
{
    led->state = !led->state;
    bl_led_set(led);
}

// 实例对象
led_t led0 = 
{
    .GPIO_Port = GPIOE,
    .GPIO_Pin = GPIO_Pin_5,
    .state = false
};