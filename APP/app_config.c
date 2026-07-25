#include "app_config.h"


static uint32_t sensorInterval = 1000;


/**
 * @brief 初始化
 */
void APP_Config_Init(void)
{
    sensorInterval = 1000;
}


/**
 * @brief 设置周期
 */
void APP_Config_SetSensorInterval(uint32_t ms)
{
    if(ms < 500)
    {
        ms = 500;
    }


    sensorInterval = ms;
}


/**
 * @brief 获取周期
 */
uint32_t APP_Config_GetSensorInterval(void)
{
    return sensorInterval;
}

