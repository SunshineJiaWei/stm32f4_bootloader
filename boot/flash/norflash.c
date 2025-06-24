#include "stm32f4xx.h"
#include "norflash.h"

#define LOG_TAG     "norflash"
#define LOG_LVL     ELOG_LVL_INFO
#include "elog.h"

#define FLASH_BASE_ADDR     0x08000000

typedef struct
{
    uint32_t flash_sector;
    uint32_t sector_size;
} sector_desc_t;

static const sector_desc_t sector_descs[] =
{
    {FLASH_Sector_0, 16 * 1024},
    {FLASH_Sector_1, 16 * 1024},
    {FLASH_Sector_2, 16 * 1024},
    {FLASH_Sector_3, 16 * 1024},
    {FLASH_Sector_4, 64 * 1024},
    {FLASH_Sector_5, 128 * 1024},
    {FLASH_Sector_6, 128 * 1024},
    {FLASH_Sector_7, 128 * 1024},
    {FLASH_Sector_8, 128 * 1024},
    {FLASH_Sector_9, 128 * 1024},
    {FLASH_Sector_10, 128 * 1024},
    {FLASH_Sector_11, 128 * 1024},
};

void bl_norflash_lock(void)
{
    log_i("norflash lock");
    FLASH_Lock();
}

void bl_norflash_unlock(void)
{
    log_i("norflash unlock");
    FLASH_Unlock();
}

void bl_norflash_erase(uint32_t address, uint32_t size)
{
    uint32_t erase_addr = FLASH_BASE_ADDR;
    for (uint8_t i = 0; i < sizeof(sector_descs) / sizeof(sector_descs[0]); i ++)
    {
        if (erase_addr >= address && erase_addr < address + size)
        {
            log_i("erase sector%u, addr: 0x%08x, size: %u", i, erase_addr, sector_descs[i].sector_size);
            if (FLASH_EraseSector(sector_descs[i].flash_sector, VoltageRange_3) != FLASH_COMPLETE)
            {
                log_w("erase sector %u failed", i);
            }
        }

        erase_addr += sector_descs[i].sector_size;
    }
}

void bl_norflash_write(uint32_t address, uint32_t size, uint8_t *data)
{
    for(uint32_t i = 0; i < size; i += 4)
    {
        if (FLASH_ProgramWord(address + i, *(uint32_t*)(data + i)) != FLASH_COMPLETE)
        {
            log_w("write flash error, addr: 0x%08X", address + i);
        }
    }
    
}