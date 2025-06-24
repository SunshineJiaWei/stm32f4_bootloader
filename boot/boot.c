#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "led.h"
#include "button.h"
#include "main.h"
#include "flash_layout.h"
#include "uart.h"
#include "ringbuffer8.h"
#include "boot.h"
#include "crc32.h"
#include "norflash.h"
#include "arginfo.h"


#define LOG_TAG     "boot"
#define LOG_LVL     ELOG_LVL_INFO
#include "elog.h"


static ringbuffer8_t serial_rb;                             // rb8实例
static uint8_t serial_rb_buffer[BL_UART_BUFFER_SIZE];       // rbB的缓存数组
static bl_ctrl_t bl_ctrl;                                   // bl控制块
static uint32_t last_pkt_time;                              // 上一次收到一帧数据包的MS数

void boot_application(void);

/**
 * @brief 串口接收回调函数，将接收到的数据放入rb8
 * 
 * @param data 
 * @param len 
 */
static void serial_recv_callback(uint8_t *data, uint32_t len)
{
    rb8_puts(serial_rb, data, len);
}

/**
 * @brief 返回响应数据
 * 
 * @param opcode 操作码
 * @param data   数据
 * @param len    数据长度
 */
static void bl_response(bl_op_t opcode, uint8_t *data, uint16_t len)
{
    const uint8_t header = BL_PACKET_HEADER;

    uint32_t crc = 0;
    crc = crc32_update(crc, (uint8_t *)&header, 1);
    crc = crc32_update(crc, (uint8_t *)&opcode, 1);
    crc = crc32_update(crc, (uint8_t *)&len, 2);
    crc = crc32_update(crc, data, len);

    bl_uart_write((uint8_t*)&header, 1);
    bl_uart_write((uint8_t*)&opcode, 1);
    bl_uart_write((uint8_t*)&len, 2);
    bl_uart_write(data, len);
    bl_uart_write((uint8_t*)&crc, 4);
}

/**
 * @brief 返回响应结果
 * 
 * @param opcode 操作码
 * @param err    响应码
 */
static void bl_response_ack(bl_op_t opcode, bl_err_t err)
{
    bl_response(opcode, (uint8_t*)&err, 1);
}

/**
 * @brief 重置状态机和数据缓存器
 * 
 * @param ctrl 
 */
static void bl_reset(bl_ctrl_t* ctrl)
{
    ctrl->pkt.index = 0;
    ctrl->rx.index = 0;
    ctrl->sm = BL_SM_STATR;
}

/**
 * @brief 
 * 
 * @param pkt 校验一帧数据包
 * @return true 
 * @return false 
 */
static bool bl_pkt_crc_verify(bl_pkt_t* pkt)
{
    const uint8_t header = BL_PACKET_HEADER;
    uint32_t crc = 0;

    crc = crc32_update(crc, (uint8_t *)&header, 1);
    crc = crc32_update(crc, (uint8_t *)&pkt->opcode, 1);
    crc = crc32_update(crc, (uint8_t *)&pkt->length, 2);
    crc = crc32_update(crc, pkt->param, pkt->length);

    return crc == pkt->crc;
}

/**
 * @brief 五阶段状态机解析数据操作
 * 
 * @param ctrl bl控制块结构体
 * @param data 
 * @return true 
 * @return false 
 */
static bool bl_recv_handler(bl_ctrl_t *ctrl, uint8_t data)
{
    bool fullpkt = false;

    bl_rx_t *rx = &ctrl->rx;
    bl_pkt_t *pkt = &ctrl->pkt;

    rx->data[rx->index++] = data;

    switch (ctrl->sm)
    {
        case BL_SM_STATR:
        {
            log_d("sm start");

            rx->index = 0;
            if (rx->data[0] == 0xAA)
            {
                ctrl->sm = BL_SM_OPCODE;
            }
            break;
        }
        case BL_SM_OPCODE:
        {
            log_d("sm opcode");

            rx->index = 0;
            pkt->opcode = rx->data[0];
            ctrl->sm = BL_SM_LENGTH;

            break;  
        }
        case BL_SM_LENGTH:
        {
            log_d("sm length");
            
            if (rx->index == 2)
            {
                rx->index = 0;
                uint16_t length = *(uint16_t*)rx->data;
                
                if (length <= BL_PACKET_PAYLOAD_SIZE)
                {
                    pkt->length = length;
                    if (length == 0) ctrl->sm = BL_SM_CRC;
                    else ctrl->sm = BL_SM_PARAM;
                }
                else
                {
                    // 给出错误响应
                    log_e("param length overflow");
                    bl_response_ack(pkt->opcode, BL_ERR_OVERFLOW);
                    bl_reset(ctrl);
                }
            }
            break;
        }
        case BL_SM_PARAM:
        {
            rx->index = 0;
            if (pkt->index < pkt->length)
            {
                pkt->param[pkt->index++] = rx->data[0];
                if (pkt->index == pkt->length)
                {
                    ctrl->sm = BL_SM_CRC;
                }
            }
            else
            {
                // 给出错误响应
                log_w("no need receive param");
                bl_response_ack(pkt->opcode, BL_ERR_PARAM);
                bl_reset(ctrl);
            }
            break;
        }
        case BL_SM_CRC:
        {
            if (rx->index == 4)
            {
                rx->index = 0;
                pkt->crc = *(uint32_t*)rx->data;
                // crc校验
                if (bl_pkt_crc_verify(pkt))
                {
                    fullpkt = true;
                }
                else
                {
                    log_e("crc mismatch");
                    bl_response_ack(pkt->opcode, BL_ERR_VERIFY);
                    bl_reset(ctrl);
                }
            }
            break;
        }
        default:
        {
            log_e("opcode error");
            bl_response_ack(pkt->opcode, BL_ERR_OPCODE);
            bl_reset(ctrl);
            break;
        } 
    }

    return fullpkt;
}

/**
 * @brief 查询版本号和最大传输单元操作
 * 
 * @param data 0：查询版本号 1：查询最大传输单元
 * @param len 
 */
static void bl_op_inquiry_handler(uint8_t *data, uint16_t len)
{
    log_i("inquiry");
    bl_inquiry_param_t *inquiry = (bl_inquiry_param_t *)data;
    
    if (len != sizeof(bl_inquiry_param_t))
    {
        log_e("length mismatch %d != %d", len, sizeof(bl_inquiry_param_t));
        bl_response_ack(BL_OP_INQUIRY, BL_ERR_PARAM);
        return;
    }

    log_i("subcode: %02X", inquiry->subcode);
    switch (inquiry->subcode)
    {
        case BL_INQUIRY_VERSION:
        {
            uint8_t version[] = { BL_VERSION_MAJOR, BL_VERSION_MINOR };
            bl_response(BL_OP_INQUIRY, version, sizeof(version));
            break;
        }
        case BL_INQUIRY_MTU:
        {
            uint16_t mtu = BL_PACKET_PAYLOAD_SIZE;
            bl_response(BL_OP_INQUIRY, (uint8_t*)&mtu, sizeof(mtu));
            break;
        }
        default:
        {
            bl_response_ack(BL_OP_INQUIRY, BL_ERR_PARAM);
            break;
        }
    }
}

/**
 * @brief 跳转APP操作
 * 
 */
static void bl_op_boot_handler(void)
{
    log_i("boot into app");

    bl_response_ack(BL_OP_BOOT, BL_OK);

    boot_application();
}

/**
 * @brief 复位操作
 * 
 */
static void bl_op_reset_handler(void)
{
    log_i("system reset");
    bl_response_ack(BL_OP_RESET, BL_OK);
    NVIC_SystemReset();
}

/**
 * @brief 擦除Flash分区操作
 * 
 * @param data 擦除地址+固件大小
 * @param len 
 */
static void bl_op_erase_handler(uint8_t *data, uint8_t len)
{
    log_i("erase sector");

    bl_erase_param_t *erase = (bl_erase_param_t*)data;

    if (len != sizeof(bl_erase_param_t))
    {
        log_e("length mismatch %d != %d", len, sizeof(bl_erase_param_t));
        bl_response_ack(BL_OP_ERASE, BL_ERR_PARAM);
    }

    if (erase->address >= FLASH_BOOT_ADDRESS && 
        erase->address < FLASH_BOOT_ADDRESS + FLASH_BOOT_SIZE)
    {
        log_e("address: %08X is protected", erase->address);
        bl_response_ack(BL_OP_ERASE, BL_ERR_UNKNOWN);
        return;
    }

    log_i("erase 0x%08X, size %d", erase->address, erase->size);
    bl_norflash_unlock();
    bl_norflash_erase(erase->address, erase->size);
    bl_norflash_lock();

    bl_response_ack(BL_OP_ERASE, BL_OK);
}

static void bl_op_read_handler(void)
{
    log_i("read");
    bl_response_ack(BL_OP_READ, BL_OK);
}

/**
 * @brief 固件写入Flash操作
 * 
 * @param data 写地址+固件大小+数据
 * @param len addr-4B + size-4B + param-n*B
 */
static void bl_op_write_handler(uint8_t *data, uint16_t len)
{
    log_i("write flash");
    bl_write_param_t *write = (bl_write_param_t*)data;

    if (len != sizeof(bl_write_param_t) + write->size)
    {
        log_e("length mismatch %d != %d", len, sizeof(bl_write_param_t) + write->size);
        bl_response_ack(BL_OP_WRITE, BL_ERR_PARAM);
    }

    if (write->address >= FLASH_BOOT_ADDRESS && 
        write->address < FLASH_BOOT_ADDRESS + FLASH_BOOT_SIZE)
    {
        log_e("address: %08X is protected", write->address);
        bl_response_ack(BL_OP_WRITE, BL_ERR_UNKNOWN);
        return;
    }

    log_i("write 0x%08X, size: %d", write->address, write->size);

    bl_norflash_unlock();
    bl_norflash_write(write->address, write->size, write->data);
    bl_norflash_lock();

    bl_response_ack(BL_OP_WRITE, BL_OK);
}

/**
 * @brief 校验固件操作
 * 
 * @param data address：APP固件起始地址 size：APP固件大小 crc：APP固件校验码
 * @param len 
 */
static void bl_op_verify_handler(uint8_t *data, uint16_t len)
{
    log_i("verify firmware");

    bl_verify_param_t *verify = (bl_verify_param_t*)data;

    if (len != sizeof(bl_verify_param_t))
    {
        log_e("length mismatch %d != %d", len, sizeof(bl_verify_param_t));
        bl_response_ack(BL_OP_VERIFY, BL_ERR_PARAM);
        return;
    }

    log_i("verify: 0x%08X, size: %d", verify->address, verify->size);

    uint32_t crc = crc32_update(0, (uint8_t*)verify->address, verify->size);

    log_i("crc: %08X, verify: %08X", crc, verify->crc);
    if (crc == verify->crc)
    {
        bl_response_ack(BL_OP_VERIFY, BL_OK);
    }
    else
    {
        bl_response_ack(BL_OP_VERIFY, BL_ERR_VERIFY);
    }

}

/**
 * @brief 根据一帧数据包中的操作码执行对应的操作
 * 
 * @param pkt 一帧数据包
 */
static void bl_pkt_handler(bl_pkt_t* pkt)
{
    log_i("opcode: %02X, ByteLen: %d", pkt->opcode, pkt->length);
    switch (pkt->opcode)
    {
        case BL_OP_NONE:
        {
            bl_response_ack(pkt->opcode, BL_ERR_UNKNOWN);
            break;
        }
        case BL_OP_INQUIRY:
        {
            bl_op_inquiry_handler(pkt->param, pkt->length);
            break;
        }
        case BL_OP_BOOT:
        {
            bl_op_boot_handler();
            break;
        }
        case BL_OP_RESET:
        {
            bl_op_reset_handler();
            break;
        }
        case BL_OP_ERASE:
        {
            bl_op_erase_handler(pkt->param, pkt->length);
            break;
        }
        case BL_OP_READ:
        {
            bl_op_read_handler();
            break;
        }
        case BL_OP_WRITE:
        {
            bl_op_write_handler(pkt->param, pkt->length);
            break;
        }
        case BL_OP_VERIFY:
        {
            bl_op_verify_handler(pkt->param, pkt->length);
            break;
        }
        default:
            break;
    }
}

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
    USART_DeInit(USART2);

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
    uint32_t _sp = *(volatile uint32_t*)(addr + 0);
    uint32_t _pc = *(volatile uint32_t*)(addr + 4);

    (void)_sp;
    entry_t entry_app = (entry_t)_pc;

    log_i("booting application at 0x%X", addr);

    bl_lowlevel_deinit();

    entry_app();
}

/**
 * @brief 进入APP之前，从arginfo区取出魔幻数、A区固件大小，以及CRC校验码
 * 
 * @return true 
 * @return false 
 */
bool verify_application(void)
{
    uint32_t size, crc;
    if(!bl_arginfo_read(&size, &crc))
    {
        return false;
    }

    uint32_t ccrc = crc32_update(0, (uint8_t *)FLASH_APP_ADDRESS, size);    
    
    if (ccrc != crc)
    {
        log_w("crc mismatch %08X != %08X", ccrc, crc);
        return false;
    }

    return true;
}

/**
 * @brief bl主循环：1、三秒倒计时引导APP，2、从rb8中逐字节取出并处理数据
 * 
 * @param boot_delay 
 */
void bootloader_main(uint32_t boot_delay)
{
    /*
    main_trap实现两种运行模式的切换
    main_trap为false时，在倒计时结束后引导进入app
    main_trap为true时，表示在倒计时期间接收到有效数据包，
    捕获当前状态，停止自动引导，进入持续等待和处理命令的循环
    */
    bool main_trap = false;
    uint32_t main_enter_time = 0;

    bl_uart_recv_cb_register(serial_recv_callback);

    serial_rb = rb8_new(serial_rb_buffer, BL_UART_BUFFER_SIZE);

    main_enter_time = bl_now();
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

        // rb为空时数据处理逻辑
        if (rb8_empty(serial_rb))
        {
            if (bl_ctrl.rx.index == 0)
            {
                last_pkt_time = bl_now();
            }
            else
            {
                // 若要开启DEBUG模式，则这里的超时时间需拉长，否则固件升级失败
                if (bl_now() - last_pkt_time > BL_TIMEOUT_MS)
                {
                    log_w("recv timeout");
                    #if DEBUG
                    elog_hexdump("recv", 16, bl_ctrl.rx.data, bl_ctrl.rx.index);
                    #endif
                    bl_reset(&bl_ctrl);
                }
            }
            // rb8为空则不进行后续的数据解析
            continue;
        }

        // 数据逐字节处理
        uint8_t data = 0;
        rb8_get(serial_rb, &data);
        log_d("recv: %02X", data);
        if (bl_recv_handler(&bl_ctrl, data))
        {
            // 已接受完整的一帧数据包
            bl_pkt_handler(&bl_ctrl.pkt);
            bl_reset(&bl_ctrl);
            main_trap = true;
            last_pkt_time = bl_now();
        }
        
    }
}

