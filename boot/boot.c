#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "led.h"
#include "button.h"
#include "main.h"
#include "flash_layout.h"

#define LOG_TAG     "boot"
#define LOG_LVL     ELOG_LVL_INFO
#include "elog.h"


/**
 * @brief 清除B区所用到的外设和中断
 * 
 */
void bl_lowlevel_deinit(void)
{
#if DEBUG
    elog_deinit();
#endif

    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOE);
    USART_DeInit(USART1);

    SysTick->CTRL = 0;

    __disable_irq();
}

/**
 * @brief bl跳转Application
 * 
 */
void boot_application(void)
{
    typedef void (*entry_t)(void);

    uint32_t addr = FLASH_APP_ADDRESS;
    uint32_t _sp = *(volatile uint32_t*)FLASH_APP_ADDRESS;
    uint32_t _pc = *(volatile uint32_t*)(FLASH_APP_ADDRESS + 4);

    entry_t entry_app = (entry_t)_pc;

    log_i("booting application at 0x%X", addr);

    bl_lowlevel_deinit();

    entry_app();
}

void bootloader_main(uint32_t boot_delay)
{
    bool main_trap = false;
    uint32_t main_enter_time = bl_now();

    while(1)
    {
        // 3s倒计时进入Application
        if (boot_delay > 0 && !main_trap)
        {
            static uint32_t last_time_arrived = 0;
            uint32_t time_passed = bl_now() - main_enter_time;
            if (last_time_arrived == 0)
            {
                log_i("boot in %d seconds", boot_delay);
                last_time_arrived = 1;
                time_passed = 1;
            }
            else if (time_passed / 1000 != last_time_arrived / 1000)
            {
                log_i("boot in %d seconds", boot_delay - time_passed / 1000);
            }

            if (time_passed >= boot_delay * 1000)
            {
                log_i("skip into app");
                boot_application();
            }

            last_time_arrived = time_passed;
        }

        // 在boot模式下，按下重启
        if (bl_button_pressed())
        {
            log_i("boot scope button pressed, system reset");
            button_wait_release();
            NVIC_SystemReset();
        }
    }
}


