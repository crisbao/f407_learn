/**
 ******************************************************************************
 * @file    app_timer.c
 * @author  crisbao
 * @brief   APP软件定时器模块
 ******************************************************************************
 */

#include "app_timer.h"

#include <string.h>
#include "stm32f4xx_hal.h"

/*----------------------------------------------------------
 * 宏定义
 *---------------------------------------------------------*/


/*----------------------------------------------------------
 * 静态变量
 *---------------------------------------------------------*/

/**
 * @brief 软件定时器表
 *
 * 保存系统中所有软件定时器对象。
 */
static APP_Timer_t timerTable[APP_TIMER_MAX];


/*----------------------------------------------------------
 * 静态函数声明
 *---------------------------------------------------------*/


/*----------------------------------------------------------
 * 对外接口
 *---------------------------------------------------------*/

/**
 * @brief 初始化软件定时器模块
 *
 * 清空所有软件定时器对象，
 * 使所有定时器处于关闭状态。
 *
 * @return 无
 */
void APP_Timer_Init(void)
{
    /*
     * 清空定时器表
     *
     * 包括：
     * enable
     * mode
     * interval
     * lastTick
     * callback
     */
    memset(
        timerTable,
        0,
        sizeof(timerTable)
    );
}

/**
 * @brief 创建软件定时器
 *
 * 根据输入参数初始化指定编号的软件定时器。
 *
 * 创建成功后定时器默认关闭，
 * 需要调用 APP_Timer_Start() 后才开始运行。
 *
 * @param[in] id
 *        软件定时器编号
 *
 * @param[in] interval
 *        定时间隔(ms)
 *
 * @param[in] mode
 *        定时器模式
 *
 * @param[in] callback
 *        超时回调函数
 *
 * @return
 *        APP_TIMER_OK      创建成功
 *        APP_TIMER_ERROR   创建失败
 */
uint8_t APP_Timer_Create(
    APP_TimerId_t id,
    uint32_t interval,
    APP_TimerMode_t mode,
    void (*callback)(void))
{
    APP_Timer_t *timer;

    /*
     * 检查Timer编号
     */
    if(id >= APP_TIMER_MAX)
    {
        return APP_TIMER_ERROR;
    }

    /*
     * 检查参数
     */
    if(interval == 0 ||
       callback == NULL)
    {
        return APP_TIMER_ERROR;
    }

    /*
     * 获取Timer对象
     */
    timer = &timerTable[id];

    /*
     * 保存Timer配置
     *
     * 此处只创建，
     * 不启动Timer。
     */
    timer->enable = 0;

    timer->mode = mode;

    timer->interval = interval;

    timer->lastTick = 0;

    timer->callback = callback;

    return APP_TIMER_OK;
}

/**
 * @brief 启动软件定时器
 *
 * 启动指定编号的软件定时器，
 * 并记录启动时刻。
 *
 * @param[in] id
 *        软件定时器编号
 *
 * @return
 *        APP_TIMER_OK
 *        APP_TIMER_ERROR
 */
uint8_t APP_Timer_Start(
    APP_TimerId_t id)
{
    APP_Timer_t *timer;

    /*
     * 检查Timer编号
     */
    if(id >= APP_TIMER_MAX)
    {
        return APP_TIMER_ERROR;
    }

    /*
     * 获取Timer对象
     */
    timer = &timerTable[id];

    /*
     * 检查Timer是否已经创建
     *
     * callback为空表示没有初始化
     */
    if(timer->callback == NULL)
    {
        return APP_TIMER_ERROR;
    }

    /*
     * 记录当前Tick
     *
     * 从启动时刻开始计时
     */
    timer->lastTick = HAL_GetTick();

    /*
     * 开启Timer
     */
    timer->enable = 1;


    return APP_TIMER_OK;
}

/**
 * @brief 停止软件定时器
 *
 * 禁止指定Timer继续运行。
 *
 * @param[in] id
 *        软件定时器编号
 *
 * @return
 *        APP_TIMER_OK
 *        APP_TIMER_ERROR
 */
uint8_t APP_Timer_Stop(
        APP_TimerId_t id)
{
    APP_Timer_t *timer;
    /*
     * 检查编号
     */
    if(id >= APP_TIMER_MAX)
    {
        return APP_TIMER_ERROR;
    }

    timer = &timerTable[id];

    /*
     * 关闭Timer
     */
    timer->enable = 0;

    return APP_TIMER_OK;
}

/**
 * @brief 设置软件定时器周期
 *
 * 修改指定Timer的时间间隔。
 *
 * @param[in] id
 *        软件定时器编号
 *
 * @param[in] interval
 *        新周期(ms)
 *
 * @return
 *        APP_TIMER_OK
 *        APP_TIMER_ERROR
 */
uint8_t APP_Timer_SetInterval(
        APP_TimerId_t id,
        uint32_t interval)
{
    APP_Timer_t *timer;

    if(id >= APP_TIMER_MAX)
    {
        return APP_TIMER_ERROR;
    }

    if(interval == 0)
    {
        return APP_TIMER_ERROR;
    }

    timer = &timerTable[id];

    timer->interval = interval;

    return APP_TIMER_OK;
}

/**
 * @brief 软件定时器处理函数
 *
 * 在主循环中周期调用，
 * 检查所有运行中的Timer是否到期。
 *
 * @return 无
 */
void APP_Timer_Process(void)
{
    uint8_t i;
    uint32_t now;

    /*
     * 获取当前系统Tick
     */
    now = HAL_GetTick();

    /*
     * 遍历所有Timer
     */
    for(i = 0;
        i < APP_TIMER_MAX;
        i++)
    {
        APP_Timer_t *timer;

        timer = &timerTable[i];

        /*
         * Timer未启动
         */
        if(timer->enable == 0)
        {
            continue;
        }

        /*
         * 判断是否超时
         */
        if((now - timer->lastTick)
            >= timer->interval)
        {
            /*
             * 更新触发时间
             *
             * 周期Timer重新计时
             */
            timer->lastTick = now;

            /*
             * 执行回调
             */
            if(timer->callback != NULL)
            {
                timer->callback();
            }

            /*
             * 单次Timer触发后关闭
             */
            if(timer->mode ==
               APP_TIMER_MODE_ONE_SHOT)
            {
                timer->enable = 0;
            }

        }

    }
}


/*----------------------------------------------------------
 * 静态函数
 *---------------------------------------------------------*/

