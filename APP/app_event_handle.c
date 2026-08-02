#include "app_event_handle.h"
#include "app_event.h"
#include "app_display.h"
#include "app_timer.h"
#include "usart_driver.h"
#include "usart.h"

void APP_Event_Process(void)
{
    APP_Event_t event;

    while(APP_Event_Get(&event)
          == APP_EVENT_OK)
    {
//        USART_Printf(
//            &huart1,
//            "Process Event type=%d id=%d param=%lu\r\n",
//            event.type,
//            event.id,
//            event.param
//        );

        switch(event.type)
        {

        case APP_EVENT_SENSOR:

            if(event.id == APP_SENSOR_EVENT_UPDATE)
            {
                APP_Display_Update();
            }

            break;
        
            case APP_EVENT_SYSTEM:

            if(event.id == APP_SYSTEM_EVENT_BOOT)
            {

                APP_Display_Update();

            }

            break;

        case APP_EVENT_CONFIG:
            if(event.id == APP_CONFIG_EVENT_CHANGED)
            {

                APP_Timer_SetInterval(
                    APP_TIMER_SENSOR,
                    event.param
                );
                 
            }
            else if(event.id == APP_CONFIG_EVENT_SAVE_DONE)
            {
                USART_Printf(
                    &huart1,
                    "Config Flash Save Done addr=0x%08lX\r\n",
                    event.param
                );

            }

            break;

        case APP_EVENT_DISPLAY:	
			if(event.id == APP_DISPLAY_EVENT_PAGE_CHANGED)
            {

                APP_Display_SetPage(
                    (APP_DisplayPage_t)event.param
                                    );
                APP_Display_Update();

            }

            break;

        default:

            break;
        }

    }
}


