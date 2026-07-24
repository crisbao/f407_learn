#include "app_protocol.h"
#include "app_control.h"
#include "app_sensor.h"
#include "app_display.h"
#include "hc05.h"
#include "oled.h"
#include "dht11.h"

#define APP_COMMAND_NUM \
(sizeof(APP_CommandTable)/sizeof(APP_CommandTable[0]))

#define APP_CMD_MAX_LEN 64
static char APP_CmdBuffer[APP_CMD_MAX_LEN];

static uint16_t APP_CmdIndex;

static void APP_Protocol_Input(uint8_t ch);
static void APP_Cmd_Status(void);
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


static void APP_Cmd_Test(void)
{
    HC05_Printf("OK\r\n");
}

static void APP_Cmd_OLED_Clear(void)
{
    OLED_Clear();
		OLED_Refresh();

    HC05_Printf("OK\r\n");
}

static void APP_Cmd_LED_ON(void)
{
    APP_Control_LED(APP_LED_ON);

    HC05_Printf("OK\r\n");
}

static void APP_Cmd_LED_OFF(void)
{
    APP_Control_LED(APP_LED_OFF);

    HC05_Printf("OK\r\n");
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

static void APP_Cmd_Status(void)
{
    const DHT11_Data_t *sensor;

    sensor = APP_Sensor_GetData();

    HC05_Printf("\r\n");
    HC05_Printf("------ STATUS ------\r\n");

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

    HC05_Printf("--------------------\r\n");
}

/**
 * @brief 命令表
 */
static const APP_Command_t APP_CommandTable[] =
{
    {"LED ON",  APP_Cmd_LED_ON},

    {"LED OFF", APP_Cmd_LED_OFF},

    {"TEST",    APP_Cmd_Test},
		
		{"OLED CLR",APP_Cmd_OLED_Clear},
		
		{"STATUS", APP_Cmd_Status},
};

/**
 * @brief 解析一条完整命令
 * @param cmd 命令字符串
 */
static void APP_Protocol_Parse(const char *cmd)
{
    uint16_t i;

    for(i = 0; i < APP_COMMAND_NUM; i++)
    {
        if(strcmp(cmd, APP_CommandTable[i].cmd) == 0)
        {
            APP_CommandTable[i].handler();

            return;
        }
    }

    /* 未找到命令 */
    HC05_Printf("ERROR: Unknown Command\r\n");
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

