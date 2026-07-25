#ifndef __APP_DISPLAY_H
#define __APP_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
    DISPLAY_PAGE_HOME = 0,

    DISPLAY_PAGE_SENSOR,

    DISPLAY_PAGE_SYSTEM,

    DISPLAY_PAGE_DEBUG,

    DISPLAY_PAGE_MAX

}APP_DisplayPage_t;

/**
 * @brief APP显示初始化
 */
void APP_Display_Init(void);

/**
 * @brief OLED刷新
 */
void APP_Display_Update(void);

void APP_Display_SetPage(APP_DisplayPage_t page);

void APP_Display_Clear(void);

APP_DisplayPage_t APP_Display_GetPage(void);


#ifdef __cplusplus
}
#endif

#endif
