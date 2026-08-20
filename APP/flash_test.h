#ifndef __FLASH_TEST_H
#define __FLASH_TEST_H

#include "stdint.h"

/*
 * Flash ����ģʽ
 */
typedef enum
{
    FLASH_TEST_MODE_NONE = 0,

    FLASH_TEST_MODE_ERASE_A,
    FLASH_TEST_MODE_ERASE_B,

    FLASH_TEST_MODE_WRITE_READ_A,
    FLASH_TEST_MODE_WRITE_READ_B,

    FLASH_TEST_MODE_FULL

} FLASH_TestMode_t;


/*
 * ��ǰ����ģʽ
 *
 * NONE��
 * �������У��������κ� Flash ����
 */
#define FLASH_TEST_MODE    FLASH_TEST_MODE_NONE


void FLASH_Test_Run(void);


#endif
