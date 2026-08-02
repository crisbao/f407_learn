#ifndef __APP_TIMER_H
#define __APP_TIMER_H


#include "stdint.h"


/*
 * Timer数量
 */
#define APP_TIMER_MAX      8


/*
 * Timer状态
 */
#define APP_TIMER_OK       0
#define APP_TIMER_ERROR    1



/*
 * 软件定时器编号
 */
typedef enum
{
    APP_TIMER_SENSOR = 0,

    APP_TIMER_CONFIG,

    APP_TIMER_DISPLAY,

    APP_TIMER_TEST,

    APP_TIMER_MAX_ID

}APP_TimerId_t;



/*
 * 软件定时器模式
 */
typedef enum
{
    APP_TIMER_MODE_ONE_SHOT = 0,

    APP_TIMER_MODE_PERIODIC

}APP_TimerMode_t;



/*
 * 软件定时器对象
 */
typedef struct
{
    uint8_t enable;                 /**< 定时器使能状态 */

    APP_TimerMode_t mode;           /**< 定时器运行模式 */

    uint32_t interval;              /**< 定时间隔(ms) */

    uint32_t lastTick;              /**< 上一次触发时间 */

    void (*callback)(void);         /**< 超时回调函数 */

}APP_Timer_t;



// Initialize timer module
void APP_Timer_Init(void);


// Create timer
uint8_t APP_Timer_Create(
        APP_TimerId_t id,
        uint32_t interval,
        APP_TimerMode_t mode,
        void (*callback)(void));


// Start timer
uint8_t APP_Timer_Start(
        APP_TimerId_t id);


// Stop timer
uint8_t APP_Timer_Stop(
        APP_TimerId_t id);


// Set timer interval
uint8_t APP_Timer_SetInterval(
        APP_TimerId_t id,
        uint32_t interval);


// Process timer
void APP_Timer_Process(void);



#endif
