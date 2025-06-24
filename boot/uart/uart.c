#include "stm32f4xx.h"
#include "uart.h"


static bl_uart_recv_cb_t bl_uart_recv_cb;


static void uart_io_init(void)
{
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); // TX
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); // RX

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void uart_nvic_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
}

static void uart_lowlevel_init(void)
{
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

void bl_uart_init(void)
{
    uart_io_init();
    uart_nvic_init();
    uart_lowlevel_init();
}

void bl_uart_write(uint8_t *data, uint16_t len)
{
    // 确保这一帧数据发送前,TC标志是未置位的，以及确保正确等待当前数据的完整发送
    USART_ClearFlag(USART2, USART_FLAG_TC);

    for (uint16_t i = 0; i < len; i ++)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
        USART_SendData(USART2, data[i]);
    }

    // TC=1，之后才可禁止 USART 或使微控制器进入低功率模式
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET);
}

void bl_uart_recv_cb_register(bl_uart_recv_cb_t callback)
{
    bl_uart_recv_cb = callback;
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = USART_ReceiveData(USART2);
        if (bl_uart_recv_cb)
        {
            bl_uart_recv_cb(&data, 1);
        }
    }
}