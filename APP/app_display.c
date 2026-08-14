#include "FreeRTOS.h"
//#include "semphr.h"

#include "app_display.h"
#include "app_sensor.h"
#include "app_control.h"
#include "app_status.h"
#include "app_system.h"
#include "app_freertos.h"

#include "usart.h"
#include "usart_driver.h"
#include "oled.h"



static APP_SensorData_t  lastSensor;

static APP_LED_State_t lastLedState = APP_LED_OFF;

/* �״�ˢ�±�־ */
static uint8_t firstRefresh = 1;

static APP_DisplayPage_t currentPage = DISPLAY_PAGE_HOME;

static APP_DisplayPage_t lastPage = DISPLAY_PAGE_HOME;

/**
 * @brief OLED��ʼ��
 */
//void APP_Display_Init(void)
//{
//    OLED_Init();
//    HAL_Delay(100);

//    OLED_Clear();

//    firstRefresh = 1;
//    currentPage = DISPLAY_PAGE_HOME;

//    lastPage = DISPLAY_PAGE_HOME;

//    memset(
//        &lastSensor,
//        0,
//        sizeof(lastSensor)
//    );

//}
void APP_Display_Init(void)
{
    USART_Printf(
        &huart1,
        "DISPLAY INIT 1\r\n"
    );

    OLED_Init();

    USART_Printf(
        &huart1,
        "DISPLAY INIT 2\r\n"
    );

    OLED_Clear();

    OLED_ShowString(
        0,
        0,
        "OLED TEST"
    );

    OLED_ShowString(
        0,
        16,
        "STM32F407"
    );

    OLED_ShowString(
        0,
        32,
        "I2C1 OK"
    );

    OLED_ShowString(
        0,
        48,
        "DATA TEST"
    );

    USART_Printf(
        &huart1,
        "OLED TEXT READY\r\n"
    );

    OLED_Refresh();

    USART_Printf(
        &huart1,
        "OLED REFRESH FINISH\r\n"
    );

    firstRefresh = 1;
    currentPage = DISPLAY_PAGE_HOME;
    lastPage = DISPLAY_PAGE_HOME;

    memset(
        &lastSensor,
        0,
        sizeof(lastSensor)
    );

    USART_Printf(
        &huart1,
        "DISPLAY INIT 3\r\n"
    );
}


/**
 * @brief �ж���ʾ�����Ƿ����仯
 * @retval 1 ���ݱ仯����Ҫˢ��
 *         0 ����δ�仯
 */
static uint8_t APP_Display_IsChanged(void)
{
    APP_SensorData_t  sensor;

    APP_Sensor_GetData(&sensor);

    /* ��һ��һ��ˢ�� */
    if(firstRefresh)
    {
        return 1;
    }

    if(currentPage != lastPage)
    {
        return 1;
    }

    if(sensor.temperature != lastSensor.temperature)
    {
        return 1;
    }

    if(sensor.humidity != lastSensor.humidity)
    {
        return 1;
    }

    if(APP_Control_GetLEDState() != lastLedState)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief ���浱ǰ��ʾ״̬
 */
static void APP_Display_SaveState(void)
{
    APP_SensorData_t  sensor;

    APP_Sensor_GetData(&sensor);

    lastSensor = sensor;

    lastLedState = APP_Control_GetLEDState();

    lastPage = currentPage;

    firstRefresh = 0;
}


static void APP_Display_ShowHomePage(void)
{
//    USART_Printf(
//        &huart1,
//        "Show Home Page\r\n"
//    );
		APP_SensorData_t  sensor;
		APP_Sensor_GetData(&sensor);
	
		OLED_ShowString(0, 0, "Smart Home");
    if(APP_System_IsSensorReady())
    {
        OLED_Printf(0,16,"Temp:%.1f C",sensor.temperature);

        OLED_Printf(0,32,"Humi:%.1f %%",sensor.humidity);
    }
    else
    {
        OLED_ShowString(0,16,"Temp:--.- C");

        OLED_ShowString(0,32,"Humi:--.- %%");
    }
		
	if(APP_Control_GetLEDState() == APP_LED_ON)
    {
        OLED_ShowString(0,48,"LED : ON ");
    }
    else
    {
        OLED_ShowString(0,48,"LED : OFF");
    }

}

static void APP_Display_ShowSensorPage(void)
{
    APP_SensorData_t sensor;

    APP_Sensor_GetData(&sensor);


    OLED_ShowString(0, 0, "Sensor");

    OLED_Printf(0,
                16,
                "Temp: %.1f C",
                sensor.temperature
                );

    OLED_Printf(0,
                32,
                "Humi: %.1f  %%",
                sensor.humidity
                );

}

static void APP_Display_ShowSystemPage(void)
{

    OLED_ShowString(0, 0, "System");
    
    OLED_Printf(0,
                16,
                "LED : %s",
                APP_Control_GetLEDState() == APP_LED_ON ?
                "ON" : "OFF");

    OLED_Printf(0,
                32,
                "HC05: %s",
                APP_System_GetBTStatus() ?
                "OK" :"ERR");
    
    OLED_Printf(0,
                48,
                "CFG: %s",
                APP_System_GetConfigStatus()
                == APP_CONFIG_OK ?
                "OK" :"ERR");

    //OLED_ShowString(0,
    //                64,
    //                "Mode: Bare");
}

static void APP_Display_ShowDebugPage(void)
{
    OLED_ShowString(0, 0, "Debug");

    OLED_Printf(0,
                16,
                "Tick:%lu",
                HAL_GetTick());

    OLED_ShowString(0,
                    32,
                    "Mode: Bare");

    OLED_ShowString(0,
                    48,
                    "Ready");
}



void APP_Display_Update(void)
{
    uint8_t changed;
    if (oledMutex == NULL)
    {
        return;
    }

    if (xSemaphoreTake(oledMutex, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    USART_Printf(
        &huart1,
        "DISPLAY MUTEX TAKE OK\r\n"
    );
   
    changed = APP_Display_IsChanged();

    if(changed == 0)
    {
        return;
    }


    OLED_Clear();

    switch(currentPage)
    {
        case DISPLAY_PAGE_HOME:

            APP_Display_ShowHomePage();

            break;

        case DISPLAY_PAGE_SENSOR:

            APP_Display_ShowSensorPage();

            break;

        case DISPLAY_PAGE_SYSTEM:

            APP_Display_ShowSystemPage();

            break;


        case DISPLAY_PAGE_DEBUG:

            APP_Display_ShowDebugPage();

            break;

        default:

            break;
    }


    OLED_Refresh();


    APP_Display_SaveState();
   
    USART_Printf(
        &huart1,
        "DISPLAY MUTEX GIVE\r\n"
    );

    xSemaphoreGive(oledMutex);
}

void APP_Display_SetPage(APP_DisplayPage_t page)
{
    if(page >= DISPLAY_PAGE_MAX)
    {
        return;
    }

    if(currentPage != page)
    {
        currentPage = page;
    }
}

void APP_Display_Clear(void)
{
    OLED_Clear();

    OLED_Refresh();
}

APP_DisplayPage_t APP_Display_GetPage(void)
{
    return currentPage;
}




