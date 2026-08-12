#include "FreeRTOS.h"
#include "task.h"

#include "app_freertos.h"
#include "app.h"


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
        APP_Run();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


/**
 * @brief FreeRTOS应用初始化
 *
 * 创建应用层主任务。
 */
void APP_FreeRTOS_Init(void)
{
    xTaskCreate(
        APP_MainTask,
        "APP_Main",
        512,
        NULL,
        2,
        NULL
    );
}


/**
 * @brief FreeRTOS任务栈溢出Hook
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
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
    taskDISABLE_INTERRUPTS();

    for (;;)
    {
    }
}
