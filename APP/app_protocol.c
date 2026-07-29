#include "app_protocol.h"
#include "app_control.h"
#include "app_sensor.h"
#include "app_display.h"
#include "app_config.h"
#include "app_system.h"
#include "hc05.h"
#include "usart_driver.h"



#include <string.h>
#include <stdlib.h>

#define APP_COMMAND_NUM \
(sizeof(APP_CommandTable)/sizeof(APP_CommandTable[0]))

#define APP_CMD_MAX_LEN 64
static char APP_CmdBuffer[APP_CMD_MAX_LEN];

static uint16_t APP_CmdIndex;


static void APP_Protocol_Input(uint8_t ch);
static void APP_Cmd_Status(char *param);
static void APP_Cmd_LED(char *param);
static void APP_Cmd_Page(char *param);
static void APP_Cmd_SetInterval(char *param);
static void APP_Cmd_OLED(char *param);
/**
 * @brief 协议初始化
 */
void APP_Protocol_Init(void)
{
    HC05_Init();
}

/**
 * @brief 协议处理
 */
void APP_Protocol_Process(void)
{
    uint8_t ch;

    while (HC05_BufferRead(&ch) == 0)
    {
        APP_Protocol_Input(ch);
    }
}


static void APP_Cmd_Test(char *param)
{
    HC05_Printf("OK\r\n");
}

static void APP_Cmd_OLED_Clear(char *param)
{
    APP_Display_Clear();
    HC05_Printf("OK\r\n");
}

static void APP_Cmd_LED(char *param)
{
    if(param == NULL)
    {
        HC05_Printf("ERR PARAM\r\n");
        return;
    }


    if(strcmp(param,"ON")==0)
    {
        APP_Control_LED(APP_LED_ON);

        HC05_Printf("LED ON OK\r\n");
    }

    else if(strcmp(param,"OFF")==0)
    {
        APP_Control_LED(APP_LED_OFF);

        HC05_Printf("LED OFF OK\r\n");
    }

    else
    {
        HC05_Printf("ERR LED PARAM\r\n");
    }
}

static void APP_Cmd_OLED(char *param)
{

    if(param==NULL)
    {
        return;
    }


    if(strcmp(param,"CLEAR")==0)
    {

        APP_Display_Clear();

        HC05_Printf(
            "OLED CLEAR OK\r\n"
        );

    }

}

static void APP_Cmd_Page(char *param)
{

    if(param == NULL)
    {
        HC05_Printf("ERR PARAM\r\n");
        return;
    }


    if(strcmp(param,"HOME")==0)
    {
        APP_Display_SetPage(DISPLAY_PAGE_HOME);
    }

    else if(strcmp(param,"SENSOR")==0)
    {
        APP_Display_SetPage(DISPLAY_PAGE_SENSOR);
    }

    else if(strcmp(param,"SYSTEM")==0)
    {
        APP_Display_SetPage(DISPLAY_PAGE_SYSTEM);
    }

    else if(strcmp(param,"DEBUG")==0)
    {
        APP_Display_SetPage(DISPLAY_PAGE_DEBUG);
    }

    else
    {
        HC05_Printf("ERR PAGE\r\n");
        return;
    }


    HC05_Printf("PAGE %s OK\r\n",param);
}

static void APP_Cmd_SetInterval(char *param)
{
    uint32_t sec;
    APP_ConfigStatus_t ret;

    if(param == NULL)
    {
        HC05_Printf("ERR PARAM\r\n");
        return;
    }

    sec = atoi(param);

    if(sec == 0)
    {
        HC05_Printf("ERR VALUE\r\n");
        return;
    }

    APP_Config_SetSensorInterval(sec * 1000);

    ret = APP_Config_Save();

    if(ret == APP_CONFIG_OK)
    {
        HC05_Printf(
            "INTERVAL=%lu s OK\r\n",
            sec
        );

        USART_Printf(
            &huart1,
            "Config Saved\r\n"
            "Interval = %lu ms\r\n",
            sec * 1000
        );
    }
    else
    {
        HC05_Printf("Save Failed\r\n");

        USART_Printf(
            &huart1,
            "Config Save Failed\r\n"
        );
    }
}

static const char *APP_PageToString(APP_DisplayPage_t page)
{
    switch(page)
    {
        case DISPLAY_PAGE_HOME:
            return "HOME";

        case DISPLAY_PAGE_SENSOR:
            return "SENSOR";

        case DISPLAY_PAGE_SYSTEM:
            return "SYSTEM";

        case DISPLAY_PAGE_DEBUG:
            return "DEBUG";

        default:
            return "UNKNOWN";
    }
}

static void APP_Cmd_Status(char *param)
{
    const DHT11_Data_t *sensor;
    APP_ConfigStatus_t cfg;
    cfg = APP_System_GetConfigStatus();

    sensor = APP_Sensor_GetData();

    HC05_Printf("\r\n");
    HC05_Printf("------ STATUS ------\r\n");
	
		HC05_Printf("Uptime : %lus\r\n",APP_System_GetUptime());


    HC05_Printf("DHT    : %s\r\n",APP_System_GetDHTStatus()== HAL_OK ?"OK":"ERR");


    HC05_Printf("BT     : %s\r\n",APP_System_GetBTStatus()?"OK":"ERR");

    if(cfg == APP_CONFIG_OK)
    {
        HC05_Printf(
            "CONFIG:FLASH\r\n"
        );
    }
    else
    {
        HC05_Printf(
            "CONFIG:DEFAULT\r\n"
        );
    }

    HC05_Printf("Temp : %d.%d C\r\n",
                sensor->temperature,
                sensor->temperature_dec);

    HC05_Printf("Humi : %d.%d %%\r\n",
                sensor->humidity,
                sensor->humidity_dec);

    HC05_Printf("LED  : %s\r\n",
                APP_Control_GetLEDState() == APP_LED_ON ?
                "ON" : "OFF");

    HC05_Printf("Page : %s\r\n",
            APP_PageToString(APP_Display_GetPage()));
		
		HC05_Printf("Interval : %lus\r\n",APP_Config_GetSensorInterval()/1000);


    HC05_Printf("Mode   : BARE\r\n");

    HC05_Printf("--------------------\r\n");
}

/**
 * @brief 命令表
 */
static const APP_Command_t APP_CommandTable[] =
{
    {"LED",  APP_Cmd_LED},
		
		{"OLED",APP_Cmd_OLED},

    {"TEST",    APP_Cmd_Test},
		
		{"OLED CLR",APP_Cmd_OLED_Clear},
		
		{"STATUS", APP_Cmd_Status},

    {"PAGE",APP_Cmd_Page},
		
		{"INTERVAL",APP_Cmd_SetInterval}
};

/**
 * @brief 解析一条完整命令
 * @param cmd 命令字符串
 */
static void APP_Protocol_Parse(char *cmd)
{
    uint16_t i;

    char *param = NULL;


    /* 查找第一个空格 */
    param = strchr(cmd,' ');


    if(param != NULL)
    {
        *param = '\0';

        param++;
    }


    for(i=0;i<APP_COMMAND_NUM;i++)
    {

        if(strcmp(cmd,
                  APP_CommandTable[i].cmd)==0)
        {

            APP_CommandTable[i].handler(param);

            return;
        }

    }

    HC05_Printf(
        "ERROR: Unknown Command\r\n"
    );

}

/**
 * @brief 输入一个字符
 */
static void APP_Protocol_Input(uint8_t ch)
{
    if(ch == '\r')
    {
        APP_CmdBuffer[APP_CmdIndex] = '\0';

        if(APP_CmdIndex > 0)
        {
            APP_Protocol_Parse(APP_CmdBuffer);
        }

        APP_CmdIndex = 0;
				memset(APP_CmdBuffer,0,sizeof(APP_CmdBuffer));

        return;
    }

    if(ch == '\n')
    {
        return;
    }

    if(APP_CmdIndex < APP_CMD_MAX_LEN - 1)
    {
        APP_CmdBuffer[APP_CmdIndex++] = ch;
    }
    else
    {
        APP_CmdIndex = 0;
				memset(APP_CmdBuffer,0,sizeof(APP_CmdBuffer));

        HC05_Printf("ERROR: Command Too Long\r\n");
    }
}



