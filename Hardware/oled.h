#ifndef __OLED_H
#define __OLED_H

#include "main.h"
#include "i2c.h"
#include "oledfont.h"

/******************************************************************************
 * @brief SSD1306 I2C 参数
 ******************************************************************************/

/* SSD1306 7位I2C地址：0x3C
 * HAL库需要左移1位，因此为0x78
 */
#define OLED_ADDR              (0x3CU << 1)

/* SSD1306 控制字 */
#define OLED_CMD               0x00U
#define OLED_DATA              0x40U

/******************************************************************************
 * @brief SSD1306 OLED 基本参数
 ******************************************************************************/

/* OLED 屏幕分辨率 */
#define OLED_WIDTH                 128U
#define OLED_HEIGHT                 64U

/* SSD1306 每页(Page)包含8行像素 */
#define OLED_PAGE_SIZE               8U
#define OLED_PAGE_COUNT             (OLED_HEIGHT / OLED_PAGE_SIZE)

/* 当前字符尺寸(8×16 ASCII) */
#define OLED_CHAR_WIDTH              8U
#define OLED_CHAR_HEIGHT            16U

/* OLED颜色定义 */
#define OLED_COLOR_BLACK             0U
#define OLED_COLOR_WHITE             1U

/* OLED驱动版本 */
#define OLED_DRIVER_VERSION      "V2.0"

/******************************************************************************
 * @brief SSD1306 指令
 ******************************************************************************/

/* 设置页地址(Page Address) */
#define OLED_PAGE_ADDR_BASE          0xB0U

/* 设置列地址(Column Address) */
#define OLED_COLUMN_LOW_ADDR         0x00U
#define OLED_COLUMN_HIGH_ADDR        0x10U

/**
 * @brief 格式化显示字符串
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param fmt 格式字符串
 */
void OLED_Printf(uint8_t x,
                 uint8_t y,
                 const char *fmt,
                 ...);

/*========================
 * 基础驱动
 *========================*/
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t color);

void OLED_DisplayOn(void);
void OLED_DisplayOff(void);

void OLED_SetAutoRefresh(uint8_t enable);
void OLED_Refresh(void);

/*========================
 * 绘图接口
 *========================*/
void OLED_DrawPoint(uint8_t x,
                    uint8_t y,
                    uint8_t color);

void OLED_DrawLine(uint8_t x1,
                   uint8_t y1,
                   uint8_t x2,
                   uint8_t y2);

void OLED_DrawRectangle(uint8_t x,
                        uint8_t y,
                        uint8_t width,
                        uint8_t height);

void OLED_FillRectangle(uint8_t x,
                        uint8_t y,
                        uint8_t width,
                        uint8_t height);

void OLED_DrawCircle(uint8_t x,
                     uint8_t y,
                     uint8_t radius);

/*========================
 * 文本显示
 *========================*/
void OLED_ShowChar(uint8_t x,
                   uint8_t y,
                   char chr);

void OLED_ShowString(uint8_t x,
                     uint8_t y,
                     const char *str);

void OLED_ShowNum(uint8_t x,
                  uint8_t y,
                  int32_t num,
                  uint8_t len);

void OLED_ShowSignedNum(uint8_t x,
                        uint8_t y,
                        int32_t num);

void OLED_ShowHexNum(uint8_t x,
                     uint8_t y,
                     uint32_t num,
                     uint8_t len);

void OLED_ShowBinNum(uint8_t x,
                     uint8_t y,
                     uint32_t num,
                     uint8_t len);



/*========================
 * 图片显示
 *========================*/
void OLED_ShowImage(uint8_t x,
                    uint8_t y,
                    uint8_t width,
                    uint8_t height,
                    const uint8_t *image);

#endif
