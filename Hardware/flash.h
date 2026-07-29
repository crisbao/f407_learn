#ifndef __FLASH_H
#define __FLASH_H


#include "stdint.h"
#include "stm32f4xx_hal.h"

/*
 * STM32F407VET6
 * Flash Sector7
 *
 * Address:
 * 0x08060000
 */

#define FLASH_CONFIG_ADDRESS 0x08060000U
#define FLASH_CONFIG_SECTOR FLASH_SECTOR_7

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERROR

} FLASH_Status_t;



FLASH_Status_t FLASH_EraseConfigSector(void);


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
