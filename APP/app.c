#include "app.h"
#include "usart.h"
#include "usart_driver.h"
#include "flash_test.h"
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

/*----------------------------------------------------------
 * APP��ʼ��
 *---------------------------------------------------------*/
void APP_Init(void)
{
    /* ��ʼ��Ӧ��ģ�� */
	
		APP_Event_Init();
	
		APP_Timer_Init();

		APP_System_Init();
		
		FLASH_Test_Run();
	
		APP_Config_Init();
	
    
    APP_Sensor_Init();
		
    APP_Control_Init();

    APP_Display_Init();
    /*
     * �ϵ��¼�
     */	
    APP_Event_t event;

    event.type = APP_EVENT_SYSTEM;
    event.id = APP_SYSTEM_EVENT_BOOT;
    event.param = 0;
    APP_Event_Post(&event);

    APP_Protocol_Init();
		


}


/*----------------------------------------------------------
 * APP��ѭ��
 *---------------------------------------------------------*/
void APP_Run(void)
{
    
		APP_Timer_Process();
	
		/*
     * �����¼�
     */
    APP_Event_Process();


    /*
     * Flash��̨ά��
     */
    APP_Config_Process();


    /*
     * ��������
     */
    APP_Protocol_Process();


}

