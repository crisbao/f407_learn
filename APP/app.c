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
#include "app_event.h"
#include "app_event_handle.h"
#include "app_timer.h"

static void APP_Timer_TestCallback(void)
{
    USART_Printf(
        &huart1,
        "TIMER CALLBACK\r\n"
    );
}
/*----------------------------------------------------------
 * APP初始化
 *---------------------------------------------------------*/
void APP_Init(void)
{
    /* 初始化应用模块 */
	
		APP_Event_Init();
	
		APP_Timer_Init();
APP_Timer_Create(
    APP_TIMER_SENSOR,
    3000,
    APP_TIMER_MODE_PERIODIC,
    APP_Timer_TestCallback
);

APP_Timer_Start(
    APP_TIMER_SENSOR
);
		APP_System_Init();

		APP_Config_Init();
	
    
    APP_Sensor_Init();
		
    APP_Control_Init();

    APP_Display_Init();
    /*
     * 上电事件
     */	
    APP_Event_t event;

    event.type = APP_EVENT_SYSTEM;
    event.id = APP_SYSTEM_EVENT_BOOT;
    event.param = 0;
    APP_Event_Post(&event);

    APP_Protocol_Init();
		


}


/*----------------------------------------------------------
 * APP主循环
 *---------------------------------------------------------*/
void APP_Run(void)
{
    
		APP_Timer_Process();
	
		/*
     * 处理事件
     */
    APP_Event_Process();


    /*
     * Flash后台维护
     */
    APP_Config_Process();


    /*
     * 蓝牙命令
     */
    APP_Protocol_Process();


}

