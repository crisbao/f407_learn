#include "flash.h"
#include <string.h>


FLASH_Status_t  FLASH_EraseConfigSector(void)
{

    FLASH_EraseInitTypeDef erase;

    uint32_t sectorError;

    HAL_FLASH_Unlock();

    erase.TypeErase =
        FLASH_TYPEERASE_SECTORS;


    erase.Sector =
        FLASH_CONFIG_SECTOR;


    erase.NbSectors =
        1;


    erase.VoltageRange =
        FLASH_VOLTAGE_RANGE_3;

    if(HAL_FLASHEx_Erase(
        &erase,
        &sectorError) != HAL_OK)
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