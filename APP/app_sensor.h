#ifndef __APP_SENSOR_H
#define __APP_SENSOR_H

#include "main.h"

/**
 * @brief 传感器数据结构
 */
typedef struct
{
    float temperature;
    float humidity;

} APP_SensorData_t;

void APP_Sensor_Init(void);

HAL_StatusTypeDef APP_Sensor_Update(void);

void APP_Sensor_GetData(APP_SensorData_t *data);



#endif
