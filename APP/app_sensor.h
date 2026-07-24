#ifndef __APP_SENSOR_H
#define __APP_SENSOR_H

#include "dht11.h"

void APP_Sensor_Init(void);

HAL_StatusTypeDef APP_Sensor_Update(void);

const DHT11_Data_t *APP_Sensor_GetData(void);

#endif
