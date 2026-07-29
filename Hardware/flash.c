#include "flash.h"
#include <string.h>

/**
 * @brief 擦除指定Flash Sector
 *
 * @param sector
 *        FLASH_SECTOR_7
 *        FLASH_SECTOR_8
 */

FLASH_Status_t FLASH_EraseSector(uint32_t sector)
{

    FLASH_EraseInitTypeDef erase;

    uint32_t error;

    /*
     * 安全检查
     *
     * STM32F407VET6
     * 只允许配置区:
     *
     * Sector6
     * Sector7
     */
    if((sector != FLASH_SECTOR_6) &&
       (sector != FLASH_SECTOR_7))
    {
        return FLASH_ERROR;
    }

    HAL_FLASH_Unlock();

    erase.TypeErase =
        FLASH_TYPEERASE_SECTORS;

    erase.Sector =
        sector;

    erase.NbSectors =
        1;

    erase.VoltageRange =
        FLASH_VOLTAGE_RANGE_3;


    if(HAL_FLASHEx_Erase(
            &erase,
            &error)
            != HAL_OK)
    {

        HAL_FLASH_Lock();

        return FLASH_ERROR;
    }

    HAL_FLASH_Lock();

    return FLASH_OK;

}



FLASH_Status_t FLASH_Write(
        uint32_t address,
        uint8_t *data,
        uint32_t length)
{
    if(address % 4 != 0)
    {
        return FLASH_ERROR;
    }

    uint32_t data32;

    HAL_FLASH_Unlock();

    while(length)
    {

        data32 = 0xFFFFFFFF;


        uint32_t size =
            length >=4 ? 4:length;

        memcpy(
            &data32,
            data,
            size
        );


        if(HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            address,
            data32)
            != HAL_OK)
        {

            HAL_FLASH_Lock();

            return FLASH_ERROR;
        }



        address +=4;

        data +=size;

        length -=size;

    }

    HAL_FLASH_Lock();

    return FLASH_OK;
}



void FLASH_Read(
uint32_t address,
uint8_t *data,
uint32_t length)
{

    memcpy(
        data,
        (uint8_t*)address,
        length
    );

}