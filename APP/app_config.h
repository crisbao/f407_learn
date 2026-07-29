#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "app_status.h"
#include "stdint.h"

#define APP_CONFIG_FLASH_ADDRESS 0x08060000
#define APP_CONFIG_MAGIC       0x53484F4D

#define APP_CONFIG_VERSION     0x0002


#define APP_CONFIG_DEFAULT_INTERVAL 1000


typedef struct
{
    uint32_t sensorIntervalMs;

} APP_ConfigData_t;

typedef struct
{

    uint32_t magic;

    uint16_t version;

    uint16_t length;

    uint32_t sequence;

    APP_ConfigData_t data;

    uint32_t crc;

} APP_ConfigStorage_t;




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


APP_ConfigStatus_t  APP_Config_Load(void);


APP_ConfigStatus_t  APP_Config_Save(void);

/**
 * @brief 恢复默认配置
 */
void APP_Config_Reset(void);

uint8_t APP_Config_IsDirty(void);

#endif
