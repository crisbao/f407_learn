#include "app_sensor.h"
#include "app_system.h"
#include "app_event.h"
#include <string.h>
#include "usart.h"
#include "usart_driver.h"

static DHT11_Data_t sensorData;
static DHT11_Data_t lastSensorData;


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
 * @brief 鏇存柊浼犳劅鍣ㄦ暟鎹?
 *
 * 璇诲彇DHT11鏁版嵁锛屽苟鍦ㄦ垚鍔熷悗鍙戦�佷紶鎰熷櫒鏇存柊浜嬩欢銆?
 *
 * @return
 *        HAL_OK      璇诲彇鎴愬姛
 *        鍏朵粬        璇诲彇澶辫触
 */
HAL_StatusTypeDef APP_Sensor_Update(void)
{
    HAL_StatusTypeDef ret;
		APP_Event_t event;
    ret = DHT11_Read(&sensorData);
    APP_System_SetDHTStatus(ret);

    if(ret == HAL_OK)
    {
        APP_System_SetSensorReady(1);
        /*
        * 判断传感器数据是否变化
        */
        if(memcmp(
            &sensorData,
            &lastSensorData,
            sizeof(DHT11_Data_t))
            != 0)
        {
            event.type =
                APP_EVENT_SENSOR;

            event.id =
                APP_SENSOR_EVENT_UPDATE;

            event.param = 0;

            APP_Event_Post(
                &event
            );
            /*
            * 保存当前数据
            */
            memcpy(
                &lastSensorData,
                &sensorData,
                sizeof(DHT11_Data_t)
            );
        }
    }

    return ret;
}
/**
 * @brief  锟斤拷取锟斤拷锟斤拷
 * 
 */
const DHT11_Data_t *APP_Sensor_GetData(void)
{
    return &sensorData;
}


