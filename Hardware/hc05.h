#ifndef __HC05_H
#define __HC05_H

#include "main.h"
#include "usart_driver.h"
#include "usart.h"

/*================ HC-05 配置 =================*/

/* HC-05 使用 USART3 */
#define HC05_UART    (&huart3)

/*================ RingBuffer配置 =================*/

/* 接收缓冲区大小 */
#define HC05_RX_BUFFER_SIZE    128U


/**
 * @brief HC-05 接收环形缓冲区
 */
typedef struct
{
    /* 数据缓冲区 */
    uint8_t buffer[HC05_RX_BUFFER_SIZE];

    /* 写指针（中断修改） */
    volatile uint16_t head;

    /* 读指针（主程序修改） */
    volatile uint16_t tail;

} HC05_RingBuffer_t;


/*================ 接口函数 =================*/

void HC05_Init(void);
/**
 * @brief 启动串口中断接收
 */
void HC05_StartReceive(void);
/**
 * @brief 初始化环形缓冲区
 */
void HC05_BufferInit(void);

/**
 * @brief 写入一个字节到RingBuffer
 * @param data 数据
 * @retval 0 成功
 * @retval -1 缓冲区满
 */
int HC05_BufferWrite(uint8_t data);

/**
 * @brief 从RingBuffer读取一个字节
 * @param data 数据指针
 * @retval 0 成功
 * @retval -1 缓冲区为空
 */
int HC05_BufferRead(uint8_t *data);


HAL_StatusTypeDef HC05_SendByte(uint8_t data);

HAL_StatusTypeDef HC05_SendBuffer(const uint8_t *buf,
                                  uint16_t len);

HAL_StatusTypeDef HC05_SendString(const char *str);

HAL_StatusTypeDef HC05_Printf(const char *fmt,
                              ...);

HAL_StatusTypeDef HC05_ReceiveByte(uint8_t *data);

#endif

