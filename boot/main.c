#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"



#define LOG_TAG     "main"
#define LOG_LVL     ELOG_LVL_INFO
#include "elog.h"


int main(void)
{
    bl_lowlevel_init();

    bl_delay_init();
    bl_led_init(&led0);
    bl_button_init();
    bl_uart_init();

#if DEBUG
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG);
    elog_start();
#endif
    
    bool trap_boot = false;

    if (bl_button_pressed())
    {
        trap_boot = true;
        log_i("button pressed, trap into boot");
    }
    else if (!verify_application())
    {
        log_w("application verify failed, trap into boot");
        trap_boot = true;
    }

    if (trap_boot)
    {
        bl_led_on(&led0);
        button_wait_release();
    }

    bootloader_main(trap_boot ? 0 : 3);
}
