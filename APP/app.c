#include "app.h"
#include "usart.h"
#include "usart_driver.h"
#include "delay.h"

#include "app_sensor.h"
#include "app_display.h"
#include "app_control.h"
#include "app_protocol.h"


/*----------------------------------------------------------
 * APP初始化
 *---------------------------------------------------------*/
void APP_Init(void)
{
    /* 初始化应用模块 */
    
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
    static uint32_t lastTick = 0;

    if(HAL_GetTick() - lastTick >= 1000)
    {
        lastTick = HAL_GetTick();

        APP_Sensor_Update();
    }

    APP_Protocol_Process();

    APP_Display_Update();
}

