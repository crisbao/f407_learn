#ifndef APP_FREERTOS_H
#define APP_FREERTOS_H

#include "FreeRTOS.h"
#include "semphr.h"
#ifdef __cplusplus
extern "C" {
#endif
extern SemaphoreHandle_t oledMutex;
extern SemaphoreHandle_t sensorDataMutex;
void APP_FreeRTOS_Init(void);

#ifdef __cplusplus
}
#endif

#endif
