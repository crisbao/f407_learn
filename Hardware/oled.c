#include "oled.h"
#include "usart.h"
#include "usart_driver.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static HAL_StatusTypeDef OLED_WriteCmd(uint8_t cmd);
static void OLED_WriteData(uint8_t *data,uint16_t size);
static uint32_t OLED_Pow(uint32_t x,uint32_t y);
static uint8_t OLED_AutoRefresh = 0;   // Ĭ�Ϲر�
static uint8_t OLED_Dirty = 0;

/**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */

/* �Դ滺���� */
uint8_t OLED_GRAM[8][128];

/**
 * @brief OLED I2C���ͻ�����
 * @note
 *      ��1�ֽ�ΪSSD1306������(0x40)��
 *      ��128�ֽ�ΪһҳGRAM���ݡ�
 *      ʹ�þ�̬�������ɼ�������ջռ�ã�
 *      ���ʺ�FreeRTOS���̡�
 */
static uint8_t OLED_TxBuffer[OLED_WIDTH + 1];

/**
 * @brief ��ȡһ��ʮ����������λ��
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
 * @brief д����
 */
//static void OLED_WriteCmd(uint8_t cmd)
//{
//    uint8_t buf[2];

//    buf[0] = 0x00;
//    buf[1] = cmd;

//    HAL_I2C_Master_Transmit(&hi2c1,
//                            OLED_ADDR,
//                            buf,
//                            2,
//                            HAL_MAX_DELAY);
//}
static HAL_StatusTypeDef OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    buf[0] = 0x00;
    buf[1] = cmd;

    ret = HAL_I2C_Master_Transmit(
        &hi2c1,
        OLED_ADDR,
        buf,
        2,
        100
    );

    if(ret != HAL_OK)
    {
        USART_Printf(
            &huart1,
            "OLED I2C CMD ERROR cmd=0x%02X ret=%d err=0x%08lX\r\n",
            cmd,
            ret,
            HAL_I2C_GetError(&hi2c1)
        );
    }

    return ret;
}
/**
 * @brief  ��SSD1306д��һҳ��ʾ����
 * @param  data  ָ��128�ֽ�GRAM����
 * @param  size  ���ݳ���(���128�ֽ�)
 * @note
 *         ʹ�þ�̬���ͻ�����������Ƶ������ջ�ռ䣬
 *         ���ʺ�FreeRTOS���񻷾���
 */
static void OLED_WriteData(uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef ret;

    if (data == NULL)
    {
        return;
    }

    if (size > OLED_WIDTH)
    {
        return;
    }

    /* �� 1 �ֽڣ����ݿ����� */
    OLED_TxBuffer[0] = OLED_DATA;

    /* ������������� OLED ���� */
    memcpy(
        &OLED_TxBuffer[1],
        data,
        size
    );

    ret = HAL_I2C_Master_Transmit(
        &hi2c1,
        OLED_ADDR,
        OLED_TxBuffer,
        size + 1U,
        100
    );

    if(ret != HAL_OK)
    {
        USART_Printf(
            &huart1,
            "OLED I2C DATA ERROR ret=%d err=0x%08lX\r\n",
            ret,
            HAL_I2C_GetError(&hi2c1)
        );
    }
    else
    {
        USART_Printf(
            &huart1,
            "OLED DATA OK size=%d\r\n",
            size
        );
    }
}


/**
 * @brief OLED��ʼ��
 */
void OLED_Init(void)
{
    //HAL_Delay(100);

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
 * @brief  ��GRAM������ˢ�µ�OLED
 * @note
 *         SSD1306 ��8ҳ(Page)��ÿҳ128�ֽڡ�
 *         ��ǰ��������ˢ�·�ʽ��
 *         ����������ΪDMAˢ�»�ֲ�ˢ�¡�
 */
static void OLED_Update(void)
{
    uint8_t page;

    for (page = 0U; page < OLED_PAGE_COUNT; page++)
    {
        /* ���õ�ǰҳ��ַ */
        OLED_WriteCmd(OLED_PAGE_ADDR_BASE + page);

        /* ��������ʼ��ַ */
        OLED_WriteCmd(OLED_COLUMN_LOW_ADDR);
        OLED_WriteCmd(OLED_COLUMN_HIGH_ADDR);

        /* ˢ�µ�ǰҳGRAM */
        OLED_WriteData(OLED_GRAM[page], OLED_WIDTH);
    }
}


/**
 * @brief  ˢ��OLED��ʾ
 * @note
 *         ����GRAM���ݷ����仯ʱ��ˢ����Ļ��
 *         �ɼ���I2Cͨ�ţ������ʾЧ�ʡ�
 */
void OLED_Refresh(void)
{
    if (OLED_Dirty == 0U)
    {
        return;
    }

    /* ��GRAMˢ�µ�OLED */
    OLED_Update();

    /* ������±�־ */
    OLED_Dirty = 0U;
}

void OLED_SetAutoRefresh(uint8_t enable)
{
    OLED_AutoRefresh = enable ? 1 : 0;
}

/**
 * @brief ����
 */
void OLED_Clear(void)
{
    memset(OLED_GRAM,0,sizeof(OLED_GRAM));
		OLED_Dirty = 1;
}

/**
 * @brief �����Ժ����
 */
void OLED_Fill(uint8_t color)
{
    memset(OLED_GRAM,
           color ? 0xFF : 0x00,
           sizeof(OLED_GRAM));
		OLED_Dirty = 1;
}

/**
 * @brief ͼ�κ����Ļ���
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
 * @brief  ��ָ��λ����ʾһ�� ASCII �ַ���8��16��
 * @param  x   �ַ����Ͻ� X ���꣨���أ�
 * @param  y   �ַ����Ͻ� Y ���꣨���أ�����Ϊ8����������
 * @param  chr ASCII �ַ�
 * @note
 *         1. ��ǰ����ʹ�� 8��16 ASCII �ֿ�
 *         2. �޸�GRAM�󲻻�����ˢ��
 *         3. ������AutoRefresh�����Զ�ˢ��OLED
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr)
{
    uint8_t page;
    uint8_t index;
    uint8_t fontIndex;

    /*---------------- ������� ----------------*/

    /* X����Խ�� */
    if (x > (OLED_WIDTH - OLED_CHAR_WIDTH))
    {
        return;
    }

    /* Y����Խ�� */
    if (y > (OLED_HEIGHT - OLED_CHAR_HEIGHT))
    {
        return;
    }

    /* ��ǰ�ֿ�Ҫ���ַ���ҳ���� */
    if ((y % OLED_PAGE_SIZE) != 0U)
    {
        return;
    }

    /* �Ƿ�ASCII�ַ�ͳһ��ʾ '?' */
    if (((uint8_t)chr < ' ') || ((uint8_t)chr > '~'))
    {
        chr = '?';
    }

    /*---------------- ����ת�� ----------------*/

    page = y / OLED_PAGE_SIZE;

    /* һ��8��16�ַ�ռ��ҳ��������ֻ��ʹ��Page6 */
    if (page >= (OLED_PAGE_COUNT - 1U))
    {
        return;
    }

    fontIndex = (uint8_t)chr - ' ';

    /*---------------- д��GRAM ----------------*/

    for (index = 0; index < OLED_CHAR_WIDTH; index++)
    {
        /* �ϰ벿�� */
        OLED_GRAM[page][x + index] =
            OLED_F8x16[fontIndex][index];

        /* �°벿�� */
        OLED_GRAM[page + 1U][x + index] =
            OLED_F8x16[fontIndex][index + OLED_CHAR_WIDTH];
    }

    /* ���GRAM�Ѹ��� */
    OLED_Dirty = 1U;

    /* �Զ�ˢ�� */
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
 * @brief OLED��ʽ�����
 * @param x ��ʼX����
 * @param y ��ʼY����
 * @param fmt printf��ʽ�ַ���
 *
 * @note
 *      �ڲ�ʹ��vsnprintf��ʽ����
 *      ���֧��31���ַ���
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

    /* ������������С����� */
    if(fraction >= factor)
    {
        integer++;
        fraction = 0;
    }

    /* ��ʾ�������� */
    OLED_ShowUInt(x,y,integer);

    /* �ҵ�С����λ�� */
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


