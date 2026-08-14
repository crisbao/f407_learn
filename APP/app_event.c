#include "app_event.h"
#include "usart_driver.h"
#include "FreeRTOS.h"
#include "queue.h"

#include <stddef.h>

//APP_Event_t event;
//static APP_Event_t eventQueue[APP_EVENT_QUEUE_SIZE];

//static uint16_t  head = 0;
//static uint16_t  tail = 0;
//static uint16_t  count = 0;
//static uint32_t lostEvent = 0;
//static uint32_t eventPostCount;
//static uint32_t eventProcessCount;
extern UART_HandleTypeDef huart1;
/*
 * FreeRTOS事件队列
 */
static QueueHandle_t eventQueue;


/*
 * 丢失事件数量
 */
static uint32_t lostEvent;

void APP_Event_Init(void)
{
//    head = 0;
//    tail = 0;
//    count = 0;
//    lostEvent = 0;
	    /*
     * 创建FreeRTOS事件队列
     *
     * 队列长度：
     * APP_EVENT_QUEUE_SIZE
     *
     * 每个元素：
     * APP_Event_t
     */
    eventQueue = xQueueCreate(
        APP_EVENT_QUEUE_SIZE,
        sizeof(APP_Event_t)
    );

    /*
     * 清零丢失事件统计
     */
    lostEvent = 0;
	if(eventQueue == NULL)
    {
        USART_Printf(
            &huart1,
            "EventQueue create FAILED\r\n"
        );
    }
    else
    {
        USART_Printf(
            &huart1,
            "EventQueue create OK\r\n"
        );
    }
}

/**
 * @brief 发送事件
 *
 * @param event 要发送的事件
 *
 * @return
 *         APP_EVENT_OK
 *         APP_EVENT_ERROR
 */
uint8_t APP_Event_Post(
    const APP_Event_t *event)
{
    BaseType_t ret;

    /*
     * 参数检查
     */
    if(event == NULL)
    {
        return APP_EVENT_ERROR;
    }

    /*
     * 检查队列是否创建成功
     */
    if(eventQueue == NULL)
    {
        return APP_EVENT_ERROR;
    }

    /*
     * 向FreeRTOS Queue发送事件
     *
     * 这里等待时间为0，
     * 表示队列满时立即返回。
     */
    ret = xQueueSend(
        eventQueue,
        event,
        0
    );

    if(ret != pdPASS)
    {
        /*
         * 队列满，
         * 记录丢失事件数量。
         */
        lostEvent++;
        USART_Printf(
            &huart1,
            "EVENT POST FAILED type=%d id=%d\r\n",
            event->type,
            event->id
        );
        return APP_EVENT_ERROR;
    }
    USART_Printf(
        &huart1,
        "EVENT POST OK type=%d id=%d\r\n",
        event->type,
        event->id
    );
    return APP_EVENT_OK;
}


/**
 * @brief 获取事件
 *
 * @param event 用于保存获取到的事件
 *
 * @return
 *         APP_EVENT_OK
 *         APP_EVENT_ERROR
 */
uint8_t APP_Event_Get(
    APP_Event_t *event)
{
    BaseType_t ret;

    /*
     * 参数检查
     */
    if(event == NULL)
    {
        return APP_EVENT_ERROR;
    }

    /*
     * 检查队列是否创建成功
     */
    if(eventQueue == NULL)
    {
        return APP_EVENT_ERROR;
    }

		/*
     * 阻塞等待事件
     *
     * 没有事件时，
     * 当前任务进入Blocked状态。
     */
    ret = xQueueReceive(
    eventQueue,
    event,
    portMAX_DELAY
		);

    if(ret != pdPASS)
    {
        return APP_EVENT_ERROR;
    }

    return APP_EVENT_OK;
}


/**
 * @brief 判断事件队列是否为空
 *
 * @return
 *         1 空
 *         0 非空
 */
uint8_t APP_Event_IsEmpty(void)
{
    if(eventQueue == NULL)
    {
        return 1;
    }

    return (uxQueueMessagesWaiting(eventQueue) == 0);
}


/**
 * @brief 获取丢失事件数量
 *
 * @return 丢失事件数量
 */
uint32_t APP_Event_GetLostCount(void)
{
    return lostEvent;
}
//uint8_t APP_Event_Post(
//        const APP_Event_t *event)
//{
//    if(event == NULL)
//		{
//				return APP_EVENT_ERROR;
//		}
//		
//		if(count >= APP_EVENT_QUEUE_SIZE)
//		{
//				lostEvent++;

//				return APP_EVENT_ERROR;
//		}
//		
//		eventQueue[tail] = *event;
//		
//		tail++;
//		
//		if(tail >= APP_EVENT_QUEUE_SIZE)
//		{
//				tail = 0;
//		}
//		
//		count++;
//    return APP_EVENT_OK;
//}

//uint8_t APP_Event_Get(
//        APP_Event_t *event)
//{
//    if(event == NULL)
//		{
//				return APP_EVENT_ERROR;
//		}
//		
//		if(count == 0)
//		{
//				return APP_EVENT_ERROR;
//		}
//		
//		*event = eventQueue[head];
//		head++;

//		if(head >= APP_EVENT_QUEUE_SIZE)
//		{
//				head = 0;
//		}
//		count--;
//    return APP_EVENT_OK;
//}

//uint8_t APP_Event_IsEmpty(void)
//{

//    return (count == 0);
//}

//uint32_t APP_Event_GetLostCount(void)
//{
//    return lostEvent;
//}



