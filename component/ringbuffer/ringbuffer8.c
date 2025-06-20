#include <stdlib.h>
#include <string.h>
#include "ringbuffer8.h"


#define rbb_len         rb->length
// #define rbb_idx(x)      (uint8_t *)rbb_buff + rbb_size * (x)
// #define dat_idx(d, x)   (uint8_t *)(d) + rbb_size * (x)

struct ringbuffer8
{
    uint32_t tail;   // 读指针
    uint32_t head;   // 写指针
    uint32_t length; // buffer可存储的字节数-这里data刚好是1Byte，即buffer长度

    uint8_t buffer[];
};

/**
 * @brief ringBuffer初始化
 *
 * @param buff
 * @param length
 * @return ringbuffer8_t 结构体指针
 */
ringbuffer8_t rb8_new(uint8_t *buff, uint32_t length)
{
    ringbuffer8_t rb = (ringbuffer8_t)buff;           // rb指向外部传入进来的一块连续的数组空间
    rb->length = length - sizeof(struct ringbuffer8); // 明确buffer的最大容量，防止内存踩踏
    rb->head = 0;
    rb->tail = 0;

    return rb;
}

static inline uint16_t next_head(ringbuffer8_t rb)
{
    return rb->head + 1 < rbb_len ? rb->head + 1 : 0;
}

static inline uint16_t next_tail(ringbuffer8_t rb)
{
    return rb->tail + 1 < rbb_len ? rb->tail + 1 : 0;
}

/**
 * @brief 判断rb8是否为空
 *
 * @param rb
 * @return true
 * @return false
 */
bool rb8_empty(ringbuffer8_t rb)
{
    return rb->head == rb->tail;
}

/**
 * @brief 判断rb8是否为满
 *
 * @param rb
 * @return true
 * @return false
 */
bool rb8_full(ringbuffer8_t rb)
{
    return next_head(rb) == rb->tail;
}

/**
 * @brief rb8写一字节数据
 *
 * @param rb
 * @param data
 * @return true
 * @return false
 */
bool rb8_put(ringbuffer8_t rb, uint8_t data)
{
    if (next_head(rb) == rb->tail)
        // printf("ringbuffer overflow");
        return false;

    rb->buffer[rb->head] = data;
    rb->head = next_head(rb);

    return true;
}

/**
 * @brief rb8写数据
 *
 * @param rb
 * @param data
 * @param size
 * @return true
 * @return false
 */
bool rb8_puts(ringbuffer8_t rb, uint8_t *data, uint32_t size)
{
    bool ret = true;

    for (uint16_t i = 0; i < size && ret; i++)
    {
        ret = rb8_put(rb, data[i]);
    }

    return ret;
}

/**
 * @brief rb8取一字节数据
 *
 * @param rb
 * @param data
 * @return true
 * @return false
 */
bool rb8_get(ringbuffer8_t rb, uint8_t *data)
{
    if (rb->head == rb->tail)
        return false;

    *data = rb->buffer[rb->tail];
    rb->tail = next_tail(rb);

    return true;
}

/**
 * @brief rb8取数据
 *
 * @param rb
 * @param data
 * @param size
 * @return true
 * @return false
 */
bool rb8_gets(ringbuffer8_t rb, uint8_t *data, uint32_t size)
{
    bool ret = true;

    for (uint16_t i = 0; i < size && ret; i++)
    {
        ret = rb8_get(rb, &data[i]);
    }

    return ret;
}
