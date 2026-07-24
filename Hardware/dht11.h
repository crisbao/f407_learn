#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

#define DHT11_GPIO_PORT    GPIOA
#define DHT11_GPIO_PIN     GPIO_PIN_3

/**************** DHT11 时序参数 ****************/

/* 主机启动信号 */
#define DHT11_START_LOW_TIME_MS         20U
#define DHT11_START_HIGH_TIME_US        30U

/* DHT11响应最大等待时间 */
#define DHT11_TIMEOUT_US               150U

/* Bit采样时间 */
#define DHT11_BIT_SAMPLE_TIME_US        40U

#define DHT11_HIGH()   HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET)
#define DHT11_LOW()    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET)

#define DHT11_READ()   HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

#define DHT11_IS_HIGH()   (DHT11_READ() == GPIO_PIN_SET)

#define DHT11_IS_LOW()    (DHT11_READ() == GPIO_PIN_RESET)


typedef struct
{
    uint8_t temperature;      /* 温度整数 */
    uint8_t temperature_dec;  /* 温度小数 */

    uint8_t humidity;         /* 湿度整数 */
    uint8_t humidity_dec;     /* 湿度小数 */

} DHT11_Data_t;

typedef enum
{
    DHT11_OK = 0,

    DHT11_ERROR_START,        /* 起始信号失败 */

    DHT11_ERROR_RESPONSE,     /* 无响应 */

    DHT11_ERROR_TIMEOUT,      /* 等待超时 */

    DHT11_ERROR_CHECKSUM      /* 校验失败 */

} DHT11_Result_t;

/* 初始化 */
void DHT11_Init(void);

/* 读取一次数据 */
HAL_StatusTypeDef DHT11_Test(void);

HAL_StatusTypeDef DHT11_Read(DHT11_Data_t *data);




#endif
