#ifndef __USART_DRIVER_H
#define __USART_DRIVER_H

#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* 串口发送超时时间(ms) */
#define USART_TX_TIMEOUT     100U

/**
 * @brief USART驱动初始化
 * @note 目前由CubeMX完成初始化，此函数预留扩展
 */
void USART_Init(void);

/**
 * @brief 发送一个字节
 */
HAL_StatusTypeDef USART_SendByte(UART_HandleTypeDef *huart,
                                 uint8_t data);

/**
 * @brief 发送缓冲区
 */
HAL_StatusTypeDef USART_SendBuffer(UART_HandleTypeDef *huart,
                                   const uint8_t *buf,
                                   uint16_t len);

/**
 * @brief 发送字符串
 */
HAL_StatusTypeDef USART_SendString(UART_HandleTypeDef *huart,
                                   const char *str);

/**
 * @brief printf格式发送
 */
HAL_StatusTypeDef USART_Printf(UART_HandleTypeDef *huart,
                               const char *fmt,
                               ...);

/**
 * @brief 接收一个字节
 */
HAL_StatusTypeDef USART_ReceiveByte(UART_HandleTypeDef *huart,
                                    uint8_t *data);

/**
 * @brief 判断串口是否忙
 */
uint8_t USART_IsBusy(UART_HandleTypeDef *huart);

/**
 * @brief 发送换行
 */
void USART_NewLine(UART_HandleTypeDef *huart);


#endif
