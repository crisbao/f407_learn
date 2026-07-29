#ifndef __FLASH_H
#define __FLASH_H


#include "stdint.h"
#include "stm32f4xx_hal.h"

/*
 * STM32F407VET6 512KB Flash
 *
 * Sector6:
 * 0x08040000 ~ 0x0805FFFF
 *
 * Sector7:
 * 0x08060000 ~ 0x0807FFFF
 *
 */

#define FLASH_CONFIG_ADDRESS_A 0x08040000U

#define FLASH_CONFIG_ADDRESS_B 0x08060000U
#define FLASH_CONFIG_SECTOR FLASH_SECTOR_7

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERROR

} FLASH_Status_t;




FLASH_Status_t FLASH_EraseSector(uint32_t sector);


FLASH_Status_t FLASH_Write(
        uint32_t address,
        uint8_t *data,
        uint32_t length
);



/**
 * @brief Flash¶ÁÈ¡Êý¾Ý
 */
void FLASH_Read(uint32_t address,
                uint8_t *data,
                uint32_t length);



#endif
