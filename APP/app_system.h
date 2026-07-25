#ifndef __APP_SYSTEM_H
#define __APP_SYSTEM_H


#include "stdint.h"
#include "stm32f4xx_hal.h"


/*
 * 初始化系统状态
 */
void APP_System_Init(void);


/*
 * 系统运行时间
 */
uint32_t APP_System_GetUptime(void);


/*
 * DHT状态
 */
void APP_System_SetDHTStatus(HAL_StatusTypeDef status);


HAL_StatusTypeDef APP_System_GetDHTStatus(void);


/*
 * 蓝牙状态
 */
void APP_System_SetBTStatus(uint8_t status);


uint8_t APP_System_GetBTStatus(void);


#endif
