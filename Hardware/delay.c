#include "delay.h"


static uint32_t us_factor = 0;

void Delay_Init(void)
{
    /* 使能 DWT  开启 Cortex-M 的 Trace 功能。 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 清零 CYCCNT */
    DWT->CYCCNT = 0;

    /* 启动 CYCCNT */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 计算每微秒对应的 Cycle 数 */
    us_factor = SystemCoreClock / 1000000;
}


void Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t delay = us * us_factor;

    while ((DWT->CYCCNT - start) < delay)
    {
    }
}


void Delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

