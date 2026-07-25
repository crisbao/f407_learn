#include "app_control.h"
#include "led.h"

static APP_LED_State_t ledState ;

/**
 * @brief ?????
 */
void APP_Control_Init(void)
{
    LED_Off();
    ledState = APP_LED_OFF;
}

/**
 * @brief LED????
 */
void APP_Control_LED(APP_LED_State_t state)
{
    if(state == APP_LED_ON)
    {
        LED_On();
    }
    else
    {
        LED_Off();
    }

    ledState = state;
}

/**
 * @brief ???LED??
 */
APP_LED_State_t APP_Control_GetLEDState(void)
{
    return ledState;
}

