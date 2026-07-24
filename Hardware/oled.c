#include "oled.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static void OLED_WriteCmd(uint8_t cmd);
static void OLED_WriteData(uint8_t *data,uint16_t size);
static uint32_t OLED_Pow(uint32_t x,uint32_t y);
static uint8_t OLED_AutoRefresh = 0;   // 默认关闭
static uint8_t OLED_Dirty = 0;

/**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */

/* 显存缓冲区 */
uint8_t OLED_GRAM[8][128];

/**
 * @brief OLED I2C发送缓冲区
 * @note
 *      第1字节为SSD1306控制字(0x40)，
 *      后128字节为一页GRAM数据。
 *      使用静态缓冲区可减少任务栈占用，
 *      更适合FreeRTOS工程。
 */
static uint8_t OLED_TxBuffer[OLED_WIDTH + 1];

/**
 * @brief 获取一个十进制整数的位数
 */
static uint8_t OLED_NumLen(uint32_t num)
{
    uint8_t len = 1;

    while(num >= 10)
    {
        num /= 10;
        len++;
    }

    return len;
}

/**
 * @brief 写命令
 */
static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2];

    buf[0] = 0x00;
    buf[1] = cmd;

    HAL_I2C_Master_Transmit(&hi2c1,
                            OLED_ADDR,
                            buf,
                            2,
                            HAL_MAX_DELAY);
}

/**
 * @brief  向SSD1306写入一页显示数据
 * @param  data  指向128字节GRAM数据
 * @param  size  数据长度(最大128字节)
 * @note
 *         使用静态发送缓冲区，避免频繁申请栈空间，
 *         更适合FreeRTOS任务环境。
 */
static void OLED_WriteData(uint8_t *data, uint16_t size)
{
    /* 参数检查 */
    if (data == NULL)
    {
        return;
    }

    if (size > OLED_WIDTH)
    {
        return;
    }

    /* 第1字节为SSD1306数据控制字 */
    OLED_TxBuffer[0] = OLED_DATA;

    /* 拷贝显示数据 */
    memcpy(&OLED_TxBuffer[1], data, size);

    /* 发送整页数据 */
    (void)HAL_I2C_Master_Transmit(&hi2c1,
                                  OLED_ADDR,
                                  OLED_TxBuffer,
                                  size + 1U,
                                  HAL_MAX_DELAY);
}

/**
 * @brief OLED初始化
 */
void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0xB0);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xFF);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xA8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xD3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);
    OLED_WriteCmd(0xF0);
    OLED_WriteCmd(0xD9);
    OLED_WriteCmd(0x22);
    OLED_WriteCmd(0xDA);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0xAF);

    OLED_Clear();
		OLED_Refresh();
}


static uint32_t OLED_Pow(uint32_t x,uint32_t y)
{
    uint32_t result = 1;

    while(y--)
    {
        result *= x;
    }

    return result;
}

/**
 * @brief  将GRAM缓冲区刷新到OLED
 * @note
 *         SSD1306 共8页(Page)，每页128字节。
 *         当前采用整屏刷新方式，
 *         后续可升级为DMA刷新或局部刷新。
 */
static void OLED_Update(void)
{
    uint8_t page;

    for (page = 0U; page < OLED_PAGE_COUNT; page++)
    {
        /* 设置当前页地址 */
        OLED_WriteCmd(OLED_PAGE_ADDR_BASE + page);

        /* 设置列起始地址 */
        OLED_WriteCmd(OLED_COLUMN_LOW_ADDR);
        OLED_WriteCmd(OLED_COLUMN_HIGH_ADDR);

        /* 刷新当前页GRAM */
        OLED_WriteData(OLED_GRAM[page], OLED_WIDTH);
    }
}


/**
 * @brief  刷新OLED显示
 * @note
 *         仅当GRAM内容发生变化时才刷新屏幕，
 *         可减少I2C通信，提高显示效率。
 */
void OLED_Refresh(void)
{
    if (OLED_Dirty == 0U)
    {
        return;
    }

    /* 将GRAM刷新到OLED */
    OLED_Update();

    /* 清除更新标志 */
    OLED_Dirty = 0U;
}

void OLED_SetAutoRefresh(uint8_t enable)
{
    OLED_AutoRefresh = enable ? 1 : 0;
}

/**
 * @brief 清屏
 */
void OLED_Clear(void)
{
    memset(OLED_GRAM,0,sizeof(OLED_GRAM));
		OLED_Dirty = 1;
}

/**
 * @brief 用于以后调试
 */
void OLED_Fill(uint8_t color)
{
    memset(OLED_GRAM,
           color ? 0xFF : 0x00,
           sizeof(OLED_GRAM));
		OLED_Dirty = 1;
}

/**
 * @brief 图形函数的基础
 */
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t color)
{
    if(x>=128)
        return;

    if(y>=64)
        return;

    if(color)
    {
        OLED_GRAM[y/8][x] |= (1<<(y%8));
    }
    else
    {
        OLED_GRAM[y/8][x] &= ~(1<<(y%8));
    }
		
		 OLED_Dirty = 1;
}


void OLED_DisplayOn(void)
{
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);

    OLED_WriteCmd(0xAF);
}


void OLED_DisplayOff(void)
{
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x10);

    OLED_WriteCmd(0xAE);
}


/**
 * @brief  在指定位置显示一个 ASCII 字符（8×16）
 * @param  x   字符左上角 X 坐标（像素）
 * @param  y   字符左上角 Y 坐标（像素，必须为8的整数倍）
 * @param  chr ASCII 字符
 * @note
 *         1. 当前驱动使用 8×16 ASCII 字库
 *         2. 修改GRAM后不会立即刷新
 *         3. 若开启AutoRefresh，将自动刷新OLED
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr)
{
    uint8_t page;
    uint8_t index;
    uint8_t fontIndex;

    /*---------------- 参数检查 ----------------*/

    /* X坐标越界 */
    if (x > (OLED_WIDTH - OLED_CHAR_WIDTH))
    {
        return;
    }

    /* Y坐标越界 */
    if (y > (OLED_HEIGHT - OLED_CHAR_HEIGHT))
    {
        return;
    }

    /* 当前字库要求字符按页对齐 */
    if ((y % OLED_PAGE_SIZE) != 0U)
    {
        return;
    }

    /* 非法ASCII字符统一显示 '?' */
    if (((uint8_t)chr < ' ') || ((uint8_t)chr > '~'))
    {
        chr = '?';
    }

    /*---------------- 坐标转换 ----------------*/

    page = y / OLED_PAGE_SIZE;

    /* 一个8×16字符占两页，因此最大只能使用Page6 */
    if (page >= (OLED_PAGE_COUNT - 1U))
    {
        return;
    }

    fontIndex = (uint8_t)chr - ' ';

    /*---------------- 写入GRAM ----------------*/

    for (index = 0; index < OLED_CHAR_WIDTH; index++)
    {
        /* 上半部分 */
        OLED_GRAM[page][x + index] =
            OLED_F8x16[fontIndex][index];

        /* 下半部分 */
        OLED_GRAM[page + 1U][x + index] =
            OLED_F8x16[fontIndex][index + OLED_CHAR_WIDTH];
    }

    /* 标记GRAM已更新 */
    OLED_Dirty = 1U;

    /* 自动刷新 */
    if (OLED_AutoRefresh)
    {
        OLED_Refresh();
    }
}


void OLED_ShowString(uint8_t x,
                     uint8_t y,
                     const char *str)
{
    if(str == NULL)
    {
        return;
    }
    while(*str)
    {
        OLED_ShowChar(x,y,*str);

        x += 8;

        if(x > OLED_WIDTH - 8)
				{
						x = 0;
						y += 16;

						if(y > OLED_HEIGHT - 16)
								break;
				}

        str++;
    }
		
		
}

/**
 * @brief OLED格式化输出
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param fmt printf格式字符串
 *
 * @note
 *      内部使用vsnprintf格式化，
 *      最大支持31个字符。
 */
void OLED_Printf(uint8_t x,
                 uint8_t y,
                 const char *fmt,
                 ...)
{
    char buffer[32];
    va_list args;

    if (fmt == NULL)
    {
        return;
    }

    va_start(args, fmt);

    vsnprintf(buffer,
              sizeof(buffer),
              fmt,
              args);

    va_end(args);

    OLED_ShowString(x, y, buffer);
}

void OLED_ShowNum(uint8_t x,
                  uint8_t y,
                  int32_t num,
                  uint8_t len)
{
    uint8_t i;
    uint8_t temp;
    uint8_t leading = 1;

    if(len == 0)
        len = OLED_NumLen(num);

    for(i = 0; i < len; i++)
    {
        temp = (num / OLED_Pow(10, len - i - 1)) % 10;

        if(leading &&
           temp == 0 &&
           i != (len - 1))
        {
            OLED_ShowChar(x + i * 8,
                          y,
                          ' ');
        }
        else
        {
            leading = 0;

            OLED_ShowChar(x + i * 8,
                          y,
                          temp + '0');
        }
    }
		
}


void OLED_ShowUInt(uint8_t x,
                   uint8_t y,
                   uint32_t num)
{
    OLED_ShowNum(x,
                 y,
                 num,
                 OLED_NumLen(num));
}

void OLED_ShowSignedNum(uint8_t x,
                        uint8_t y,
                        int32_t num)
{
    if(num >= 0)
    {
        OLED_ShowChar(x, y, '+');
        OLED_ShowUInt(x + 8, y, (uint32_t)num);
    }
    else
    {
        OLED_ShowChar(x, y, '-');
        OLED_ShowUInt(x + 8, y, (uint32_t)(-num));
    }
}


void OLED_ShowHexNum(uint8_t x,
                     uint8_t y,
                     uint32_t num,
                     uint8_t len)
{
    uint8_t i;
    uint8_t temp;

    for(i = 0; i < len; i++)
    {
        temp = (num >> ((len - i - 1) * 4)) & 0x0F;

        if(temp < 10)
        {
            OLED_ShowChar(x + i * 8,
                          y,
                          temp + '0');
        }
        else
        {
            OLED_ShowChar(x + i * 8,
                          y,
                          temp - 10 + 'A');
        }
    }

}

void OLED_ShowBinNum(uint8_t x,
                     uint8_t y,
                     uint32_t num,
                     uint8_t len)
{
    uint8_t i;

    for(i = 0; i < len; i++)
    {
        if(num & (1 << (len - i - 1)))
        {
            OLED_ShowChar(x + i * 8,
                          y,
                          '1');
        }
        else
        {
            OLED_ShowChar(x + i * 8,
                          y,
                          '0');
        }
    }

}


void OLED_ShowFloat(uint8_t x,
                    uint8_t y,
                    float num,
                    uint8_t decimal)
{
    int32_t integer;
    uint32_t fraction;
    uint32_t factor = OLED_Pow(10, decimal);

    if(num < 0)
    {
        OLED_ShowChar(x, y, '-');
        num = -num;
        x += 8;
    }

    integer = (int32_t)num;

    fraction = (uint32_t)((num - integer) * factor + 0.5f);

    /* 如果四舍五入后小数溢出 */
    if(fraction >= factor)
    {
        integer++;
        fraction = 0;
    }

    /* 显示整数部分 */
    OLED_ShowUInt(x,y,integer);

    /* 找到小数点位置 */
    while(integer >= 10)
    {
        integer /= 10;
        x += 8;
    }

    x += 8;

    OLED_ShowChar(x, y, '.');

    OLED_ShowNum(x + 8,
                 y,
                 fraction,
                 decimal);

}


