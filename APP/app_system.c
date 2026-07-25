#include "app_system.h"


static uint32_t systemStartTick;


static HAL_StatusTypeDef dhtStatus;


static uint8_t btStatus;



void APP_System_Init(void)
{

    systemStartTick = HAL_GetTick();


    dhtStatus = HAL_ERROR;


    btStatus = 0;

}



/*
 * 获取运行时间
 * 单位：秒
 */
uint32_t APP_System_GetUptime(void)
{

    return (HAL_GetTick()
           -
           systemStartTick)
           /1000;

}



void APP_System_SetDHTStatus(
        HAL_StatusTypeDef status)
{
    dhtStatus = status;
}



HAL_StatusTypeDef APP_System_GetDHTStatus(void)
{
    return dhtStatus;
}



void APP_System_SetBTStatus(uint8_t status)
{
    btStatus = status;
}



uint8_t APP_System_GetBTStatus(void)
{
    return btStatus;
}

