#include "led.h"

/**
 * @brief LED初始化
 * @note GPIO已经由CubeMX初始化，这里预留接口
 */
void LED_Init(void)
{
    LED_Off();
}

/**
 * @brief 点亮LED
 */
void LED_On(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_PORT,
                      LED1_GPIO_PIN,
                      LED_ON_LEVEL);
}

/**
 * @brief 熄灭LED
 */
void LED_Off(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_PORT,
                      LED1_GPIO_PIN,
                      LED_OFF_LEVEL);
}

/**
 * @brief LED状态翻转
 */
void LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED1_GPIO_PORT,
                       LED1_GPIO_PIN);
}

/**
 * @brief 读取LED状态
 * @retval GPIO_PIN_SET 或 GPIO_PIN_RESET
 */
GPIO_PinState LED_Read(void)
{
    return HAL_GPIO_ReadPin(LED1_GPIO_PORT,
                            LED1_GPIO_PIN);
}


