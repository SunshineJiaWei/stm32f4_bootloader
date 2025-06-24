#ifndef __NOR_FLASH_H
#define __NOR_FLASH_H


#include <stdint.h>


void bl_norflash_lock(void);
void bl_norflash_unlock(void);
void bl_norflash_erase(uint32_t address, uint32_t size);
void bl_norflash_write(uint32_t address, uint32_t size, uint8_t *data);


#endif /* __NOR_FLASH_H */