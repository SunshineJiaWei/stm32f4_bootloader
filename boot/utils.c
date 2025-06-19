#include <stdint.h>
#include "stm32f4xx.h"
#include "system_stm32f4xx.h"


static uint32_t ticks;

/**
 * @brief delay初始化
 * 
 */
void bl_delay_init(void)
{
    // 设置重装载值
    SysTick_Config(SystemCoreClock / 1000);
}

/**
 * @brief delay毫秒延时
 * 
 * @param ms 
 */
void bl_delay_ms(uint32_t ms)
{
    uint32_t start = ticks;
    while(ticks - start < ms);
}

/**
 * @brief 返回上电后至今的毫秒数
 * 
 */
void bl_now(void)
{
    return ticks;
}

/**
 * @brief 一毫秒产生一次中断
 * 
 */
void SysTick_Handler(void)
{
    ticks ++;
}