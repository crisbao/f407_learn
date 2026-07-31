#include "app_display.h"

#include "app_sensor.h"
#include "app_control.h"
#include "app_status.h"
#include "app_system.h"

#include "oled.h"


static DHT11_Data_t lastSensor;

static APP_LED_State_t lastLedState = APP_LED_OFF;

/* 首次刷新标志 */
static uint8_t firstRefresh = 1;

static APP_DisplayPage_t currentPage = DISPLAY_PAGE_HOME;

static APP_DisplayPage_t lastPage = DISPLAY_PAGE_HOME;

/**
 * @brief OLED初始化
 */
void APP_Display_Init(void)
{
    OLED_Init();

    OLED_Clear();

    firstRefresh = 1;
    currentPage = DISPLAY_PAGE_HOME;

    OLED_ShowString(0, 0, " Smart Home");

    OLED_Refresh();
}



/**
 * @brief 判断显示内容是否发生变化
 * @retval 1 数据变化，需要刷新
 *         0 数据未变化
 */
static uint8_t APP_Display_IsChanged(void)
{
    const DHT11_Data_t *sensor;

    sensor = APP_Sensor_GetData();

    /* 第一次一定刷新 */
    if(firstRefresh)
    {
        return 1;
    }

    if(currentPage != lastPage)
    {
        return 1;
    }

    if(sensor->temperature != lastSensor.temperature)
    {
        return 1;
    }

    if(sensor->humidity != lastSensor.humidity)
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
 * @brief 保存当前显示状态
 */
static void APP_Display_SaveState(void)
{
    const DHT11_Data_t *sensor;

    sensor = APP_Sensor_GetData();

    lastSensor = *sensor;

    lastLedState = APP_Control_GetLEDState();

    lastPage = currentPage;

    firstRefresh = 0;
}


static void APP_Display_ShowHomePage(void)
{
		const DHT11_Data_t *sensor;
		sensor = APP_Sensor_GetData();
	
		OLED_ShowString(0, 0, "Smart Home");

    OLED_Printf(0,16,"Temp:%d.%d C",sensor->temperature,sensor->temperature_dec);

    OLED_Printf(0,32,"Humi:%d.%d %%",sensor->humidity,sensor->humidity_dec);
		
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
    const DHT11_Data_t *sensor;

    sensor = APP_Sensor_GetData();


    OLED_ShowString(0, 0, "Sensor");

    OLED_Printf(0,
                16,
                "Temp: %d.%d C",
                sensor->temperature,
                sensor->temperature_dec);

    OLED_Printf(0,
                32,
                "Humi: %d.%d %%",
                sensor->humidity,
                sensor->humidity_dec);

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
    if(APP_Display_IsChanged() == 0)
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

        //case DISPLAY_PAGE_CONFIG:

         //   APP_Display_ShowConfigPage();

        //    break;

        case DISPLAY_PAGE_DEBUG:

            APP_Display_ShowDebugPage();

            break;

        default:

            break;
    }

    OLED_Refresh();

    APP_Display_SaveState();
}

void APP_Display_SetPage(APP_DisplayPage_t page)
{
    if(page >= DISPLAY_PAGE_MAX)
    {
        return;
    }

    currentPage = page;
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




