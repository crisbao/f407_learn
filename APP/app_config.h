#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H


#include "stdint.h"


/**
 * @brief 初始化配置
 */
void APP_Config_Init(void);


/**
 * @brief 设置传感器采样周期
 */
void APP_Config_SetSensorInterval(uint32_t ms);


/**
 * @brief 获取采样周期
 */
uint32_t APP_Config_GetSensorInterval(void);


#endif
