#include "app_event.h"
#include <stddef.h>


APP_Event_t event;
static APP_Event_t eventQueue[APP_EVENT_QUEUE_SIZE];

static uint16_t  head = 0;
static uint16_t  tail = 0;
static uint16_t  count = 0;
static uint32_t lostEvent = 0;
static uint32_t eventPostCount;
static uint32_t eventProcessCount;

void APP_Event_Init(void)
{
    head = 0;
    tail = 0;
    count = 0;
    lostEvent = 0;
	
	
}


uint8_t APP_Event_Post(
        const APP_Event_t *event)
{
    if(event == NULL)
		{
				return APP_EVENT_ERROR;
		}
		
		if(count >= APP_EVENT_QUEUE_SIZE)
		{
				lostEvent++;

				return APP_EVENT_ERROR;
		}
		
		eventQueue[tail] = *event;
		
		tail++;
		
		if(tail >= APP_EVENT_QUEUE_SIZE)
		{
				tail = 0;
		}
		
		count++;
    return APP_EVENT_OK;
}

uint8_t APP_Event_Get(
        APP_Event_t *event)
{
    if(event == NULL)
		{
				return APP_EVENT_ERROR;
		}
		
		if(count == 0)
		{
				return APP_EVENT_ERROR;
		}
		
		*event = eventQueue[head];
		head++;

		if(head >= APP_EVENT_QUEUE_SIZE)
		{
				head = 0;
		}
		count--;
    return APP_EVENT_OK;
}

uint8_t APP_Event_IsEmpty(void)
{

    return (count == 0);
}

uint32_t APP_Event_GetLostCount(void)
{
    return lostEvent;
}



