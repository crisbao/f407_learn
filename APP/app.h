#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief APP层初始化
 */
void APP_Init(void);

/**
 * @brief APP层循环
 * @note 裸机阶段调用，FreeRTOS阶段将由各Task替代
 */
void APP_Run(void);

#ifdef __cplusplus
}
#endif

#endif
