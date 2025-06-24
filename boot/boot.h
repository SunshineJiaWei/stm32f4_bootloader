#ifndef __BOOT_H
#define __BOOT_H


/* format
 *
 * | start | opcode | length | payload | crc32 |
 * | u8    | u8     | u16    | u8 * n  | u32   |
 *
 * start: 0xAA
 */

#define BL_VERSION_MAJOR            1
#define BL_VERSION_MINOR            0

#define BL_PACKET_HEADER            0xAA
#define BL_UART_BUFFER_SIZE         512ul
#define BL_PACKET_HEAD_SIZE         8ul
#define BL_PACKET_PAYLOAD_SIZE      4096ul
#define BL_PACKET_PARAM_SIZE        BL_PACKET_HEAD_SIZE + BL_PACKET_PAYLOAD_SIZE
#define BL_TIMEOUT_MS               500ul


// 查询码
typedef enum
{
    BL_INQUIRY_VERSION,
    BL_INQUIRY_MTU
} bl_inquiry_t;

// 操作码-描述一帧数据包所要执行的操作
typedef enum
{
    BL_OP_NONE      = 0X00,
    BL_OP_INQUIRY   = 0X10,
    BL_OP_BOOT      = 0X11,
    BL_OP_RESET     = 0X1F,
    BL_OP_ERASE     = 0X20,
    BL_OP_READ      = 0X21,
    BL_OP_WRITE     = 0X22,
    BL_OP_VERIFY    = 0X23
} bl_op_t;

// 响应码
typedef enum
{
    BL_OK,
    BL_ERR_OPCODE,
    BL_ERR_OVERFLOW,
    BL_ERR_TIMEOUT,
    BL_ERR_FORMAT,
    BL_ERR_VERIFY,
    BL_ERR_PARAM,
    BL_ERR_UNKNOWN = 0XFF
} bl_err_t;

// 状态机-描述接收数据时所处的状态
typedef enum 
{
    BL_SM_STATR,
    BL_SM_OPCODE,
    BL_SM_LENGTH,
    BL_SM_PARAM,
    BL_SM_CRC
} bl_state_machine_t;

// 一帧数据包结构体
typedef struct 
{
    bl_op_t opcode;
    uint16_t length;
    uint32_t crc;
    uint16_t index;
    uint8_t param[BL_PACKET_PARAM_SIZE];
} bl_pkt_t;

// 当前状态接收数据的结构体
typedef struct 
{
    uint8_t index;
    uint8_t data[16];
} bl_rx_t;

// bl控制块结构体
typedef struct
{
    bl_rx_t rx;
    bl_pkt_t pkt;
    bl_state_machine_t sm;    
} bl_ctrl_t;

// 查询结构体
typedef struct 
{
    uint8_t subcode;
} bl_inquiry_param_t;

// 擦除FLASH结构体
typedef struct 
{
    uint32_t address;
    uint32_t size;
} bl_erase_param_t;

// 写FLASH结构体
typedef struct
{
    uint32_t address;
    uint32_t size;
    uint8_t data[];
} bl_write_param_t;

// 校验固件结构体
typedef struct
{
    uint32_t address;
    uint32_t size;
    uint32_t crc;
} bl_verify_param_t;


bool verify_application(void);
void bootloader_main(uint32_t delay_ms);


#endif /* __BOOT_H */