#ifndef __APP_CONTROL_H
#define __APP_CONTROL_H

#include "main.h"

typedef enum
{
    APP_LED_OFF = 0,
    APP_LED_ON
}APP_LED_State_t;

/* 初始化 */
void APP_Control_Init(void);

/* LED控制 */
void APP_Control_LED(APP_LED_State_t state);

/* 获取LED状态 */
APP_LED_State_t APP_Control_GetLEDState(void);

#endif
