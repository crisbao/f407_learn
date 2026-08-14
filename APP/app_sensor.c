#include "FreeRTOS.h"


#include "app_sensor.h"
#include "app_system.h"
#include "app_event.h"
#include "app_freertos.h"
#include <string.h>
#include "dht11.h"
#include "usart.h"
#include "usart_driver.h"

//static DHT11_Data_t sensorData;
//static DHT11_Data_t lastSensorData;
static APP_SensorData_t sensorData;
static APP_SensorData_t lastSensorData;


void APP_Sensor_Init(void)
{
    DHT11_Init();

    memset(
        &sensorData,
        0,
        sizeof(sensorData)
    );

    memset(
        &lastSensorData,
        0,
        sizeof(lastSensorData)
    );
}

/**
 * @brief 更新传感器数�?
 *
 * 读取DHT11数据，并在成功后发送传感器更新事件�?
 *
 * @return
 *        HAL_OK      读取成功
 *        其他        读取失败
 */
HAL_StatusTypeDef APP_Sensor_Update(void)
{
    HAL_StatusTypeDef ret;

    DHT11_Data_t rawData;

    APP_Event_t event;
    USART_Printf(
        &huart1,
        "SENSOR UPDATE START\r\n"
    );
    /* 读取DHT11原始数据 */
    ret = DHT11_Read(&rawData);
    USART_Printf(
        &huart1,
        "DHT11 READ RET=%d\r\n",
        ret
    );
    /* 更新DHT11状态 */
    APP_System_SetDHTStatus(ret);

    if (ret == HAL_OK)
    {
			        USART_Printf(
            &huart1,
            "DHT11 DATA T=%d.%d H=%d.%d\r\n",
            rawData.temperature,
            rawData.temperature_dec,
            rawData.humidity,
            rawData.humidity_dec
        );
        /* 传感器已经准备完成 */
        APP_System_SetSensorReady(1);

        /*
         * 将DHT11原始数据转换为APP层数据
         */
        if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE)
        {
            USART_Printf(
                &huart1,
                "SENSOR DATA MUTEX TAKE OK\r\n"
            );

            sensorData.temperature =
                (float)rawData.temperature +
                (float)rawData.temperature_dec / 10.0f;

            sensorData.humidity =
                (float)rawData.humidity +
                (float)rawData.humidity_dec / 10.0f;

            xSemaphoreGive(sensorDataMutex);

            USART_Printf(
                &huart1,
                "SENSOR DATA MUTEX GIVE\r\n"
            );

            
        }

        /*
         * 判断传感器数据是否发生变化
         */
        if ((sensorData.temperature != lastSensorData.temperature) ||
						(sensorData.humidity != lastSensorData.humidity))
        {
            event.type = APP_EVENT_SENSOR;
            event.id = APP_SENSOR_EVENT_UPDATE;
            event.param = 0;
            USART_Printf(
    &huart1,
    "POST SENSOR EVENT ONLY\r\n"
);
            APP_Event_Post(&event);

            /*
             * 保存当前数据
             */
            memcpy(
                &lastSensorData,
                &sensorData,
                sizeof(APP_SensorData_t)
            );
        }
    }

    return ret;
}

/**
 * @brief 获取当前传感器数据
 *
 * @param data 用于接收传感器数据的结构体
 */
void APP_Sensor_GetData(APP_SensorData_t *data)
{
    if(data == NULL)
    {
        return;
    }

    if(xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE)
    {
        *data = sensorData;

        xSemaphoreGive(sensorDataMutex);
    }
}


