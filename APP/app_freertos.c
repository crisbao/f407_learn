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

static TaskHandle_t configTaskHandle;


static QueueHandle_t sensorTriggerQueue;
static void APP_RTOS_EventHandle(const APP_Event_t *event);

uint8_t APP_Sensor_Trigger(void);

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

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief FreeRTOS 传感器任务
 *
 * @note
 *       等待 Sensor Trigger Queue。
 *       收到触发后执行一次传感器采集。
 */
static void APP_SensorTask(void *argument)
{
    uint8_t trigger;

    (void)argument;

    for (;;)
    {
        /*
         * 等待传感器触发
         *
         * 没有触发时，任务阻塞。
         */
        if(xQueueReceive(
                sensorTriggerQueue,
                &trigger,
                portMAX_DELAY
            ) == pdTRUE)
        {
            USART_Printf(
                &huart1,
                "SENSOR TASK\r\n"
            );

            /*
             * 执行一次传感器采集
             */
            APP_Sensor_Update();
        }
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

static void APP_ConfigTask(void *argument)
{
    uint32_t remaining;
    uint32_t notifyResult;

    for(;;)
    {
        /*
         * 等待配置任务通知
         *
         * 通知来源：
         * 1. 配置发生变化
         * 2. 系统启动时发现需要 Repair
         */
        notifyResult =
            ulTaskNotifyTake(
                pdTRUE,
                portMAX_DELAY
            );

        if(notifyResult > 0)
        {
            USART_Printf(
                &huart1,
                "CONFIG TASK\r\n"
            );
        }


        /*
         * ========================================
         * 第一优先级：处理 Repair
         * ========================================
         */
        if(APP_Config_IsRepairPending())
        {
            APP_ConfigStatus_t status;

            USART_Printf(
                &huart1,
                "CONFIG REPAIR PENDING\r\n"
            );

            status = APP_Config_Repair();

            if(status != APP_CONFIG_OK)
            {
                USART_Printf(
                    &huart1,
                    "CONFIG REPAIR FAILED status=%d\r\n",
                    status
                );

                /*
                 * Repair失败：
                 * 等待一段时间后重新尝试。
                 */
                vTaskDelay(
                    pdMS_TO_TICKS(
                        APP_CONFIG_REPAIR_RETRY_DELAY_MS
                    )
                );

                /*
                 * 不处理Dirty，
                 * 重新进入本轮循环，
                 * 再次检查RepairPending。
                 */
                continue;
            }

            USART_Printf(
                &huart1,
                "CONFIG REPAIR DONE\r\n"
            );
        }


        /*
         * ========================================
         * 第二优先级：处理 Dirty Save
         * ========================================
         */
        while(APP_Config_IsDirty())
        {
            remaining =
                APP_Config_GetSaveRemainingTime();


            /*
             * 已经到保存时间
             */
            if(remaining == 0)
            {
                APP_Config_Process();

                break;
            }


            /*
             * 阻塞等待：
             *
             * 1. 新配置通知
             * 2. 保存时间到期
             */
            notifyResult =
                ulTaskNotifyTake(
                    pdTRUE,
                    pdMS_TO_TICKS(remaining)
                );


            /*
             * 收到新的配置通知
             *
             * 不立即保存，
             * 重新计算剩余时间。
             */
            if(notifyResult > 0)
            {
                continue;
            }


            /*
             * timeout：
             * 保存时间到了
             */
            APP_Config_Process();
        }
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
    TickType_t lastWakeTime;

    (void)argument;

    lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        APP_Timer_Process();

        vTaskDelayUntil(
            &lastWakeTime,
            pdMS_TO_TICKS(10)
        );
    }
}


/**
 * @brief FreeRTOS应用初始化
 *
 * 创建应用层主任务。
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
    
    ret = xTaskCreate(
    APP_ConfigTask,
    "Config",
    256,
    NULL,
    1,
    &configTaskHandle
    );

    USART_Printf(
        &huart1,
        "Config create=%d\r\n",
        ret
    );
    if(ret == pdPASS)
    {
        if(APP_Config_IsRepairPending())
        {
            APP_ConfigTaskNotify();
        }
    }
    /* 启动 Sensor Timer */
    if(sensorTriggerQueue != NULL)
    {
        APP_Timer_Start(APP_TIMER_SENSOR);

        USART_Printf(
            &huart1,
            "Sensor Timer START\r\n"
        );
    }

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

void APP_ConfigTaskNotify(void)
{
    USART_Printf(
        &huart1,
        "CONFIG NOTIFY\r\n"
    );

    if(configTaskHandle == NULL)
    {
        USART_Printf(
            &huart1,
            "CONFIG HANDLE NULL\r\n"
        );
        return;
    }

    BaseType_t ret;

    ret = xTaskNotifyGive(configTaskHandle);

    USART_Printf(
        &huart1,
        "CONFIG NOTIFY GIVE ret=%ld\r\n",
        ret
    );
}

/**
 * @brief 触发一次传感器采样
 *
 * @return 1：发送成功
 *         0：发送失败
 */
uint8_t APP_Sensor_Trigger(void)
{
    uint8_t trigger = 1;

    if(sensorTriggerQueue == NULL)
    {
        return 0;
    }

    if(xQueueSend(
            sensorTriggerQueue,
            &trigger,
            0
        ) == pdTRUE)
    {
        return 1;
    }

    return 0;
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
