#include "hc05.h"
#include "app_system.h"
#include <string.h>


/**
 * @brief HC-05 接收环形缓冲区
 */
static HC05_RingBuffer_t HC05_RxBuffer;
/**
 * @brief 当前接收到的一个字节
 */
static uint8_t HC05_RxByte;



/**
 * @brief  HC-05 初始化
 * @note
 * USART3 已由 CubeMX 初始化，
 * 当前版本无需额外初始化。
 *
 * 后续如果增加：
 * 1. KEY 引脚
 * 2. STATE 引脚
 * 3. 中断接收初始化
 * 都可以在这里完成。
 */
void HC05_Init(void)
{
    /* 初始化缓冲区 */
    HC05_BufferInit();

    /* 启动串口中断接收 */
    HC05_StartReceive();
	
		APP_System_SetBTStatus(1);
}

/**
 * @brief 启动一次串口中断接收
 */
void HC05_StartReceive(void)
{
    HAL_StatusTypeDef ret;

    ret = HAL_UART_Receive_IT(&huart3,
                              &HC05_RxByte,
                              1);
		if(ret != HAL_OK)
    {
        USART_Printf(&huart1,"HC05 RX ERROR\r\n");
    }

}

/**
 * @brief 初始化环形缓冲区
 */
void HC05_BufferInit(void)
{
    HC05_RxBuffer.head = 0;
    HC05_RxBuffer.tail = 0;

    memset(HC05_RxBuffer.buffer,
           0,
           sizeof(HC05_RxBuffer.buffer));
}

/**
 * @brief 写入一个字节
 */
int HC05_BufferWrite(uint8_t data)
{
    uint16_t next;

    /* 计算下一位置 */
    next = (HC05_RxBuffer.head + 1) % HC05_RX_BUFFER_SIZE;

    /* 判断缓冲区是否已满 */
    if (next == HC05_RxBuffer.tail)
    {
        return -1;
    }

    /* 保存数据 */
    HC05_RxBuffer.buffer[HC05_RxBuffer.head] = data;

    /* 更新Head */
    HC05_RxBuffer.head = next;

    return 0;
}

/**
 * @brief 从RingBuffer读取一个字节
 */
int HC05_BufferRead(uint8_t *data)
{
    /* 判断缓冲区是否为空 */
    if (HC05_RxBuffer.head == HC05_RxBuffer.tail)
    {
        return -1;
    }

    /* 取出数据 */
    *data = HC05_RxBuffer.buffer[HC05_RxBuffer.tail];

    /* 更新Tail */
    HC05_RxBuffer.tail =
        (HC05_RxBuffer.tail + 1) % HC05_RX_BUFFER_SIZE;

    return 0;
}

/**
 * @brief UART接收完成回调函数
 * @param huart UART句柄
 *
 * 当HAL接收到一个字节以后，
 * 会自动进入这里。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 判断是不是HC-05对应的USART */
    if (huart->Instance == USART3)
    {
        /* 写入RingBuffer */
        if(HC05_BufferWrite(HC05_RxByte) != 0)
				{
						/* Buffer Full */
				}

        /* 继续接收下一个字节 */
        HAL_UART_Receive_IT(HC05_UART,
                            &HC05_RxByte,
                            1);
    }
}

/**
 * @brief 发送一个字节
 * @param data 要发送的数据
 * @retval HAL状态
 */
HAL_StatusTypeDef HC05_SendByte(uint8_t data)
{
    return USART_SendByte(HC05_UART, data);
}

/**
 * @brief 发送一段数据
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @retval HAL状态
 */
HAL_StatusTypeDef HC05_SendBuffer(const uint8_t *buf,
                                  uint16_t len)
{
    return USART_SendBuffer(HC05_UART,
                            buf,
                            len);
}

/**
 * @brief 发送字符串
 * @param str 字符串
 * @retval HAL状态
 */
HAL_StatusTypeDef HC05_SendString(const char *str)
{
    return USART_SendString(HC05_UART,
                            str);
}

/**
 * @brief printf格式发送
 * @param fmt 格式字符串
 * @retval HAL状态
 *
 * 示例：
 * HC05_Printf("Temp:%d\r\n",25);
 */
HAL_StatusTypeDef HC05_Printf(const char *fmt,
                              ...)
{
    char buffer[128];

    va_list args;

    va_start(args, fmt);

    vsnprintf(buffer,
              sizeof(buffer),
              fmt,
              args);

    va_end(args);

    return USART_SendString(HC05_UART,
                            buffer);
}

/**
 * @brief 接收一个字节
 * @param data 接收缓冲区
 * @retval HAL状态
 *
 * 当前版本采用阻塞接收。
 * 后续将升级为中断+环形缓冲区。
 */
HAL_StatusTypeDef HC05_ReceiveByte(uint8_t *data)
{
    return USART_ReceiveByte(HC05_UART,
                             data);
}






