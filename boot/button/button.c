#include "stm32f4xx.h"
#include "button.h"
#include "main.h"

void bl_button_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Medium_Speed;

    GPIO_Init(GPIOE, &GPIO_InitStructure);
}

bool bl_button_pressed(void)
{
    if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_0) == Bit_RESET)
    {
        bl_delay_ms(100);
        if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_0) == Bit_RESET)
        {
            return true;
        }
    }

    return false;
}

void button_wait_release(void)
{
    while (bl_button_pressed())
    {
        bl_delay_ms(100);
    }
}