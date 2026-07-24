#ifndef __LED_H
#define __LED_H

#include "main.h"

/* LED硬件定义 */
#define LED1_GPIO_PORT      GPIOC
#define LED1_GPIO_PIN       GPIO_PIN_13

/* 根据自己的硬件修改 */
#define LED_ON_LEVEL        GPIO_PIN_RESET
#define LED_OFF_LEVEL       GPIO_PIN_SET

/* 函数声明 */
void LED_Init(void);
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
GPIO_PinState LED_Read(void);

#endif
