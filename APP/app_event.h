#ifndef __APP_EVENT_H
#define __APP_EVENT_H

#include "stdint.h"

/*
 * 事件类别
 */
typedef enum
{
    APP_EVENT_NONE = 0,

    APP_EVENT_SENSOR,

    APP_EVENT_CONFIG,

    APP_EVENT_DISPLAY,

    APP_EVENT_PROTOCOL,

    APP_EVENT_SYSTEM

} APP_EventType_t;

/*
 * Sensor事件
 */
typedef enum
{
    APP_SENSOR_EVENT_UPDATE = 0,

} APP_SensorEvent_t;

/*
 * Config事件
 */
typedef enum
{
    APP_CONFIG_EVENT_CHANGED = 0,

    APP_CONFIG_EVENT_REPAIR_DONE,

    APP_CONFIG_EVENT_SAVE_DONE

} APP_ConfigEvent_t;
/*
 * Display事件
 */

 typedef enum
{
    APP_SYSTEM_EVENT_BOOT = 0,

}APP_SystemEvent_t;

typedef enum
{
    APP_DISPLAY_EVENT_REFRESH = 0,

    APP_DISPLAY_EVENT_PAGE_CHANGED

} APP_DisplayEvent_t;

typedef struct
{
    APP_EventType_t type;

    uint16_t id;

    uint32_t param;

}APP_Event_t;

#define APP_EVENT_QUEUE_SIZE      8
#define APP_EVENT_OK       0
#define APP_EVENT_ERROR    1

void APP_Event_Init(void);

uint8_t APP_Event_Post(
        const APP_Event_t *event);

uint8_t APP_Event_Get(
        APP_Event_t *event);

uint8_t APP_Event_IsEmpty(void);
uint32_t APP_Event_GetLostCount(void);
uint32_t APP_Event_GetPostCount(void);

uint32_t APP_Event_GetProcessCount(void);

#endif
