#include "dht11.h"
#include "delay.h"





/**
 * @brief  将 DHT11 数据引脚配置为推挽输出
 */
static void DHT11_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;      // 推挽输出
   GPIO_InitStruct.Pull = GPIO_NOPULL;              // 
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    // 高速

    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}



/**
 * @brief  将 DHT11 数据引脚配置为输入模式
 */
static void DHT11_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;      // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // 上拉输入

    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}


void DHT11_Init(void)
{
    DHT11_Pin_Input();
}

static void DHT11_Start(void)
{
    /* 配置为输出 */
    DHT11_Pin_Output();

    /* 拉低总线 */
    DHT11_LOW();

    /* 至少18ms */
    Delay_ms(DHT11_START_LOW_TIME_MS);

    /* 拉高 */
    DHT11_HIGH();

    /* 等待20~40us */
    Delay_us(DHT11_START_HIGH_TIME_US);

    /* 切换为输入，等待DHT11响应 */
    DHT11_Pin_Input();
}


static HAL_StatusTypeDef DHT11_Checksum(const uint8_t *buffer)
{
    uint8_t sum;

    if (buffer == NULL)
    {
        return HAL_ERROR;
    }

    sum = buffer[0] +
          buffer[1] +
          buffer[2] +
          buffer[3];

    return (sum == buffer[4]) ? HAL_OK : HAL_ERROR;
}


/**
 * @brief  等待GPIO到指定电平
 * @param  level      等待的电平
 * @param  timeout_us 超时时间(us)
 * @retval HAL_OK
 *         HAL_TIMEOUT
 */
static HAL_StatusTypeDef DHT11_WaitLevel(GPIO_PinState level,
                                         uint32_t timeout_us)
{
    uint32_t start;
    uint32_t timeout_cycle;

    timeout_cycle = timeout_us * (SystemCoreClock / 1000000U);

    start = DWT->CYCCNT;

    while (DHT11_READ() != level)
    {
        if ((DWT->CYCCNT - start) >= timeout_cycle)
        {
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}


/**
 * @brief  检测DHT11响应
 * @retval HAL_OK      DHT11响应正常
 * @retval HAL_ERROR   响应超时
 */
static HAL_StatusTypeDef DHT11_CheckResponse(void)
{
    if (DHT11_WaitLevel(GPIO_PIN_RESET,
                        DHT11_TIMEOUT_US) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitLevel(GPIO_PIN_SET,
                        DHT11_TIMEOUT_US) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitLevel(GPIO_PIN_RESET,
                        DHT11_TIMEOUT_US) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    return HAL_OK;
}

HAL_StatusTypeDef  DHT11_Test(void)
{
    DHT11_Start();

    return DHT11_CheckResponse();
}

/**
 * @brief  读取一个Bit
 * @retval 0 或 1
 */
static HAL_StatusTypeDef DHT11_ReadBit(uint8_t *bit)
{
    if (DHT11_WaitLevel(GPIO_PIN_SET,
                        DHT11_TIMEOUT_US) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    Delay_us(DHT11_BIT_SAMPLE_TIME_US);
		
		
    *bit = DHT11_IS_HIGH() ? 1U : 0U;
		if(bit == NULL)
			{
					return HAL_ERROR;
			}

    if (DHT11_WaitLevel(GPIO_PIN_RESET,
                        DHT11_TIMEOUT_US) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    return HAL_OK;
}

/**
 * @brief  读取一个字节
 * @param  data 读取的数据
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef DHT11_ReadByte(uint8_t *data)
{
    uint8_t i;
    uint8_t bit;
    HAL_StatusTypeDef status;

    if (data == NULL)
    {
        return HAL_ERROR;
    }

    *data = 0;

    for (i = 0; i < 8; i++)
    {
        status = DHT11_ReadBit(&bit);

        if (status != HAL_OK)
        {
            return status;
        }

        *data <<= 1;

        *data |= bit;
    }

    return HAL_OK;
}


HAL_StatusTypeDef DHT11_Read(DHT11_Data_t *data)
{
    HAL_StatusTypeDef status;
    uint8_t buffer[5];
		uint8_t i;

    /* 参数检查 */
    if (data == NULL)
    {
        return HAL_ERROR;
    }

    /* 发送开始信号 */
    DHT11_Start();

    /* 检测响应 */
    status = DHT11_CheckResponse();
    if (status != HAL_OK)
    {
        return status;
    }

    /* 读取5个字节 */
    for (i = 0; i < 5; i++)
    {
        status = DHT11_ReadByte(&buffer[i]);

        if (status != HAL_OK)
        {
            return status;
        }
    }

    /* 校验数据 */
    status = DHT11_Checksum(buffer);

		if(status != HAL_OK)
		{
				return status;
		}
		/* 保存数据 */
		
		data->humidity        = buffer[0];
		data->humidity_dec    = buffer[1];
		data->temperature     = buffer[2];
		data->temperature_dec = buffer[3];

		return HAL_OK;
}



