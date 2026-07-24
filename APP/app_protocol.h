#ifndef __APP_PROTOCOL_H
#define __APP_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef struct
{
    const char *cmd;

    void (*handler)(void);

}APP_Command_t;

/* 初始化协议层 */
void APP_Protocol_Init(void);

/* 协议处理 */
void APP_Protocol_Process(void);

#ifdef __cplusplus
}
#endif

#endif
