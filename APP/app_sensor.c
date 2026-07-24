#include "app_sensor.h"
#include <string.h>
#include "usart.h"
#include "usart_driver.h"

static DHT11_Data_t sensorData;

void APP_Sensor_Init(void)
{
    DHT11_Init();
		memset(&sensorData,
           0,
           sizeof(sensorData));
}

/**
 * @brief  ����
 * 
 */
HAL_StatusTypeDef APP_Sensor_Update(void)
{
    //HAL_StatusTypeDef ret;

    //ret = DHT11_Read(&sensorData);

    //return ret;
    HAL_StatusTypeDef ret;

    ret = DHT11_Read(&sensorData);

    USART_Printf(&huart1,
                 "ret=%d Temp=%d.%d C Humi=%d.%d %%\r\n",
                 ret,
                 sensorData.temperature,
                 sensorData.temperature_dec,
                 sensorData.humidity,
                 sensorData.humidity_dec);

    return ret;
}
/**
 * @brief  ��ȡ����
 * 
 */
const DHT11_Data_t *APP_Sensor_GetData(void)
{
    return &sensorData;
}


