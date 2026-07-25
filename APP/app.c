#include "app.h"
#include "usart.h"
#include "usart_driver.h"
#include "delay.h"

#include "app_sensor.h"
#include "app_display.h"
#include "app_control.h"
#include "app_protocol.h"
#include "app_config.h"
#include "app_system.h"


/*----------------------------------------------------------
 * APP初始化
 *---------------------------------------------------------*/
void APP_Init(void)
{
    /* 初始化应用模块 */
	
		APP_System_Init();

		APP_Config_Init();
    
    APP_Sensor_Init();

    APP_Control_Init();

    APP_Display_Init();

    
    APP_Protocol_Init();
}

/*----------------------------------------------------------
 * APP主循环
 *---------------------------------------------------------*/
void APP_Run(void)
{
    static uint32_t sensorTick = 0;


    if(HAL_GetTick()-sensorTick >= 
       APP_Config_GetSensorInterval())
    {

        sensorTick = HAL_GetTick();


        APP_Sensor_Update();

    }


    APP_Protocol_Process();


    APP_Display_Update();

}

