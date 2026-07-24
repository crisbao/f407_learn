#include "usart_driver.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief USART驱动初始化
 * @note
 * USART外设已经由CubeMX初始化，
 * 当前版本无需额外初始化。
 *
 * 后续如果增加：
 * 1.DMA
 * 2.环形缓冲区(RingBuffer)
 * 3.中断接收
 * 都可以在这里完成初始化。
 */
void USART_Init(void)
{

}


/**
 * @brief 发送一个字节
 * @param huart 串口句柄
 * @param data  要发送的数据
 */
HAL_StatusTypeDef USART_SendByte(UART_HandleTypeDef *huart,
                                 uint8_t data)
{
    return HAL_UART_Transmit(huart,
                             &data,
                             1,
                             USART_TX_TIMEOUT);
}

/**
 * @brief 发送缓冲区
 * @param huart 串口句柄
 * @param buf   数据缓冲区
 * @param len   数据长度
 */
HAL_StatusTypeDef USART_SendBuffer(UART_HandleTypeDef *huart,
                                   const uint8_t *buf,
                                   uint16_t len)
{
    if ((buf == NULL) || (len == 0))
    {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(huart,
                             (uint8_t *)buf,
                             len,
                             HAL_MAX_DELAY);
}

/**
 * @brief 发送字符串
 * @param huart 串口句柄
 * @param str   字符串
 */
HAL_StatusTypeDef USART_SendString(UART_HandleTypeDef *huart,
                                   const char *str)
{
    if (str == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(huart,
                             (uint8_t *)str,
                             strlen(str),
                             HAL_MAX_DELAY);
}

/**
 * @brief printf格式发送
 * @param huart 串口句柄
 * @param fmt   格式字符串
 */
HAL_StatusTypeDef USART_Printf(UART_HandleTypeDef *huart,
                               const char *fmt,
                               ...)
{
    char buffer[128];

    va_list args;

    if (fmt == NULL)
    {
        return HAL_ERROR;
    }

    va_start(args, fmt);

    vsnprintf(buffer,
              sizeof(buffer),
              fmt,
              args);

    va_end(args);

    return USART_SendString(huart, buffer);
}


/**
 * @brief 接收一个字节
 * @param huart 串口句柄
 * @param data  接收缓冲区
 */
HAL_StatusTypeDef USART_ReceiveByte(UART_HandleTypeDef *huart,
                                    uint8_t *data)
{
    if (data == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Receive(huart,
                            data,
                            1,
                            10);
}

/**
 * @brief 判断串口是否忙
 * @param huart 串口句柄
 * @retval 1 忙
 * @retval 0 空闲
 */
uint8_t USART_IsBusy(UART_HandleTypeDef *huart)
{
    return (huart->gState != HAL_UART_STATE_READY);
}


/**
 * @brief 发送回车换行
 * @param huart 串口句柄
 */
void USART_NewLine(UART_HandleTypeDef *huart)
{
    USART_SendString(huart, "\r\n");
}

