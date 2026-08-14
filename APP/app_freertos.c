#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "usart_driver.h"

#include "app_freertos.h"
#include "app.h"
#include "app_sensor.h"
#include "app_config.h"
#include "app_display.h"
#include "app_timer.h"
#include "app_event.h"
#include "app_event_handle.h"
#include "app_protocol.h"

typedef struct
{
    uint32_t value;

} APP_TestMessage_t;

extern UART_HandleTypeDef huart1;

SemaphoreHandle_t oledMutex = NULL;
SemaphoreHandle_t sensorDataMutex = NULL;
//static QueueHandle_t testQueue;
//static QueueHandle_t appEventQueue;
//static volatile uint32_t queueRecvCount = 0;
//static volatile uint32_t queueRecvValue = 0;
static QueueHandle_t sensorTriggerQueue;
static void APP_RTOS_EventHandle(const APP_Event_t *event);

/**
 * @brief FreeRTOS主任务
 *
 * 负责暂时承接原裸机系统的 APP_Run()。
 */
static void APP_MainTask(void *argument)
{
    (void)argument;

    for (;;)
    {
			USART_Printf(
        &huart1,
        "MAIN TASK\r\n"
    );
        //APP_Timer_Process();


        APP_Config_Process();

//        USART_Printf(
//            &huart1,
//            "RecvCount=%lu RecvValue=%lu\r\n",
//            queueRecvCount,
//            queueRecvValue
//        );
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief  FreeRTOS传感器任务
 *
 * @note   周期执行传感器采集。
 *         采样周期由APP_Config动态提供。
 */
static void APP_SensorTask(void *argument)
{
    (void)argument;

    /* 记录任务上一次唤醒时间 */
    TickType_t lastWakeTime;

    /* 获取当前Tick作为周期计算基准 */
    lastWakeTime = xTaskGetTickCount();

    /* 任务永久运行 */
    for (;;)
    {
			USART_Printf(
            &huart1,
            "SENSOR TASK\r\n"
        );
			 
        /*
         * 执行一次传感器采集。
         */
        APP_Sensor_Update();

        /*
         * 获取当前传感器采样周期。
         *
         * 配置单位为ms，
         * 转换为FreeRTOS Tick后使用。
         */
        TickType_t sensorPeriod =
            pdMS_TO_TICKS(APP_Config_GetSensorInterval());

        /*
         * 按固定时间基准等待下一次采集。
         */
        vTaskDelayUntil(
            &lastWakeTime,
            sensorPeriod
        );
    }
}



static void APP_ProtocolTask(void *argument)
{
    (void)argument;

    for (;;)
    {

        APP_Protocol_Process();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void APP_EventTask(void *argument)
{
    (void)argument;
    APP_Event_t event;


    for (;;)
    {

			if (APP_Event_Get(&event) == APP_EVENT_OK)
				{
					            USART_Printf(
                    &huart1,
                    "EVENT GET type=%d id=%d\r\n",
                    event.type,
                    event.id
                    );


					APP_RTOS_EventHandle(&event);
 
				}
    }
    
}

static void APP_TimerTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        APP_Timer_Process();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


/**
 * @brief FreeRTOS应用初始化
 *
 * 创建应用层主任务。
 */
//void APP_FreeRTOS_Init(void)
//{
//    BaseType_t retSend;
//    BaseType_t retRecv;
//    /*
//     * 初始化FreeRTOS事件队列
//     */
//    APP_Event_Init();

////    testQueue = xQueueCreate(
////                                8,
////                                sizeof(APP_TestMessage_t)
////                            );
////        if(testQueue == NULL)
////    {
////        USART_Printf(
////            &huart1,
////            "testQueue create FAILED\r\n"
////        );
////    }
////    else
////    {
////        USART_Printf(
////            &huart1,
////            "testQueue create OK\r\n"
////        );
////    }

//    xTaskCreate(
//        APP_MainTask,
//        "APP_Main",
//        512,
//        NULL,
//        2,
//        NULL
//    );
//    /*
//     * 创建传感器任务。
//     *
//     * 参数：
//     * 任务函数、任务名称、栈大小、任务参数、
//     * 任务优先级、任务句柄。
//     */
//    xTaskCreate(
//        APP_SensorTask,
//        "Sensor",
//        256,
//        NULL,
//        1,
//        NULL
//    );
//    xTaskCreate(
//        APP_DisplayTask,
//        "Display",
//        512,
//        NULL,
//        1,
//        NULL
//    );
//    xTaskCreate(
//        APP_ProtocolTask,
//        "Protocol",
//        512,
//        NULL,
//        2,
//        NULL
//    );
//    xTaskCreate(
//        APP_EventTask,
//        "EventTask",
//        512,
//        NULL,
//        2,
//        NULL
//    );
//    retSend = xTaskCreate(
//        APP_QueueSendTask,
//        "QueueSend",
//        256,
//        NULL,
//        1,
//        NULL
//    );

//    retRecv = xTaskCreate(
//        APP_QueueReceiveTask,
//        "QueueRecv",
//        256,
//        NULL,
//        1,
//        NULL
//    );
//    USART_Printf(
//        &huart1,
//        "QueueSend create=%ld\r\n",
//        (long)retSend
//    );

//    USART_Printf(
//        &huart1,
//        "QueueRecv create=%ld\r\n",
//        (long)retRecv
//    );
//}
/**
 * @brief FreeRTOS应用初始化
 */
void APP_FreeRTOS_Init(void)
{
    BaseType_t ret;
    oledMutex = xSemaphoreCreateMutex();
    sensorDataMutex = xSemaphoreCreateMutex();

    if(sensorDataMutex == NULL)
    {
        USART_Printf(
            &huart1,
            "Sensor Data Mutex create FAILED\r\n"
        );
    }
    else
    {
        USART_Printf(
            &huart1,
            "Sensor Data Mutex create OK\r\n"
        );
    }
    if(oledMutex == NULL)
    {
        USART_Printf(
        &huart1,
        "OLED Mutex create FAILED\r\n"
    );
    }
    else
    {
        USART_Printf(
        &huart1,
        "OLED Mutex create OK\r\n"
        );
    }
    
    USART_Printf(
        &huart1,
        "APP_FreeRTOS_Init START\r\n"
    );
    sensorTriggerQueue = xQueueCreate(
        1,
        sizeof(uint8_t)
    );

    if (sensorTriggerQueue == NULL)
    {
        USART_Printf(
            &huart1,
            "SensorTriggerQueue create FAILED\r\n"
        );
    }
    else
    {
        USART_Printf(
            &huart1,
            "SensorTriggerQueue create OK\r\n"
        );
    }

    /*
     * 注意：
     * APP_Event_Init() 已经在 APP_Init() 中调用。
     * 这里不能再次调用。
     */

    ret = xTaskCreate(
        APP_MainTask,
        "APP_Main",
        512,
        NULL,
        2,
        NULL
    );

    USART_Printf(
        &huart1,
        "Main create=%ld\r\n",
        (long)ret
    );


    ret = xTaskCreate(
        APP_SensorTask,
        "Sensor",
        256,
        NULL,
        1,
        NULL
    );

    USART_Printf(
        &huart1,
        "Sensor create=%ld\r\n",
        (long)ret
    );


    ret = xTaskCreate(
    APP_TimerTask,
    "Timer",
    256,
    NULL,
    2,
    NULL
    );

    USART_Printf(
        &huart1,
        "Timer create=%ld\r\n",
        (long)ret
    );




    ret = xTaskCreate(
        APP_ProtocolTask,
        "Protocol",
        512,
        NULL,
        2,
        NULL
    );

    USART_Printf(
        &huart1,
        "Protocol create=%ld\r\n",
        (long)ret
    );


    ret = xTaskCreate(
        APP_EventTask,
        "EventTask",
        512,
        NULL,
        2,
        NULL
    );

    USART_Printf(
        &huart1,
        "Event create=%ld\r\n",
        (long)ret
    );


    USART_Printf(
        &huart1,
        "APP_FreeRTOS_Init DONE\r\n"
    );
}

static void APP_RTOS_EventHandle(const APP_Event_t *event)
{

    if (event == NULL)
    {
        return;
    }
	USART_Printf(
    &huart1,
    "EVENT HANDLE type=%d id=%d\r\n",
    event->type,
    event->id
);
    switch (event->type)
    {
        /*
         * ==============================
         * 传感器事件
         * ==============================
         */
        case APP_EVENT_SENSOR:

            if (event->id == APP_SENSOR_EVENT_UPDATE)
            {
                APP_Display_Update();
            }

            break;
        /*
         * ==============================
         * 系统事件
         * ==============================
         */
        case APP_EVENT_SYSTEM:

            if (event->id == APP_SYSTEM_EVENT_BOOT)
            {
                APP_Display_Update();
            }

            break;
        /*
         * ==============================
         * 配置事件
         * ==============================
         */
        case APP_EVENT_CONFIG:

            if (event->id == APP_CONFIG_EVENT_CHANGED)
            {
                APP_Timer_SetInterval(
                    APP_TIMER_SENSOR,
                    event->param
                );
            }
            else if (event->id == APP_CONFIG_EVENT_SAVE_DONE)
            {
                USART_Printf(
                    &huart1,
                    "Config Flash Save Done addr=0x%08lX\r\n",
                    event->param
                );
            }

            break;
        /*
         * ==============================
         * 显示事件
         * ==============================
         */
        case APP_EVENT_DISPLAY:

            if (event->id == APP_DISPLAY_EVENT_PAGE_CHANGED)
            {
                APP_Display_SetPage(
                    (APP_DisplayPage_t)event->param
                );

                APP_Display_Update();
            }

            break;

        default:

            break;
    }
}

/**
 * @brief FreeRTOS任务栈溢出Hook
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
	    USART_Printf(
        &huart1,
        "!!! STACK OVERFLOW: %s !!!\r\n",
        pcTaskName
    );
    (void)xTask;
    (void)pcTaskName;

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
    }
}


/**
 * @brief FreeRTOS内存申请失败Hook
 */
void vApplicationMallocFailedHook(void)
{
		USART_Printf(
        &huart1,
        "!!! FreeRTOS MALLOC FAILED !!!\r\n"
    );

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
    }
}
