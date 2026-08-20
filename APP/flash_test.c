#include "flash_test.h"
#include "flash.h"
#include "usart_driver.h"
#include "usart.h"
#include <string.h>


#define FLASH_TEST_ADDRESS_A    FLASH_CONFIG_ADDRESS_A
#define FLASH_TEST_ADDRESS_B    FLASH_CONFIG_ADDRESS_B


static const uint8_t testDataA[] =
{
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,
    0x77,
    0x88
};


static const uint8_t testDataB[] =
{
    0xAA,
    0xBB,
    0xCC,
    0xDD,
    0xEE,
    0xFF,
    0x12,
    0x34
};


/**
 * @brief 擦除 Sector 6
 */
static uint8_t FLASH_Test_EraseSector6(void)
{
    USART_Printf(
        &huart1,
        "FLASH TEST: Erase Sector 6...\r\n"
    );

    if(FLASH_EraseSector(FLASH_SECTOR_6) != FLASH_OK)
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: Sector 6 ERASE FAILED\r\n"
        );

        return 0;
    }

    USART_Printf(
        &huart1,
        "FLASH TEST: Sector 6 ERASE OK\r\n"
    );

    return 1;
}


/**
 * @brief 擦除 Sector 7
 */
static uint8_t FLASH_Test_EraseSector7(void)
{
    USART_Printf(
        &huart1,
        "FLASH TEST: Erase Sector 7...\r\n"
    );

    if(FLASH_EraseSector(FLASH_SECTOR_7) != FLASH_OK)
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: Sector 7 ERASE FAILED\r\n"
        );

        return 0;
    }

    USART_Printf(
        &huart1,
        "FLASH TEST: Sector 7 ERASE OK\r\n"
    );

    return 1;
}


/**
 * @brief 检查 Flash 擦除结果
 *
 * 擦除后 Flash 应该全部为 0xFF。
 */
static uint8_t FLASH_Test_CheckErase(
    uint32_t address,
    uint32_t length)
{
    uint32_t i;

    uint8_t *p =
        (uint8_t *)address;

    for(i = 0; i < length; i++)
    {
        if(p[i] != 0xFF)
        {
            USART_Printf(
                &huart1,
                "FLASH TEST: ERASE CHECK FAILED addr=0x%08lX\r\n",
                address + i
            );

            return 0;
        }
    }

    return 1;
}


/**
 * @brief 测试写入和读取
 */
static uint8_t FLASH_Test_WriteRead(
    uint32_t address,
    const uint8_t *writeData,
    uint32_t length)
{
    uint8_t readData[16];

    uint32_t i;

    if(length > sizeof(readData))
    {
        return 0;
    }

    /*
     * 写入
     */
    if(FLASH_Write(
        address,
        (uint8_t *)writeData,
        length
    ) != FLASH_OK)
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: WRITE FAILED addr=0x%08lX\r\n",
            address
        );

        return 0;
    }

    /*
     * 读取
     */
    FLASH_Read(
        address,
        readData,
        length
    );

    /*
     * 比较
     */
    for(i = 0; i < length; i++)
    {
        if(readData[i] != writeData[i])
        {
            USART_Printf(
                &huart1,
                "FLASH TEST: READ CHECK FAILED\r\n"
            );

            USART_Printf(
                &huart1,
                "addr=0x%08lX index=%lu write=0x%02X read=0x%02X\r\n",
                address,
                i,
                writeData[i],
                readData[i]
            );

            return 0;
        }
    }

    return 1;
}

/**
 * @brief 完整 Flash 测试
 *
 * 测试内容：
 * 1. 擦除 Sector 6
 * 2. 检查 Sector 6 擦除结果
 * 3. 写入测试数据
 * 4. 读取并校验
 * 5. 擦除 Sector 7
 * 6. 检查 Sector 7 擦除结果
 * 7. 写入测试数据
 * 8. 读取并校验
 */
static void FLASH_Test_Full(void)
{
    /*
     * Sector 6
     */
    USART_Printf(
        &huart1,
        "\r\n========== FLASH TEST FULL ==========\r\n"
    );

    USART_Printf(
        &huart1,
        "TEST SECTOR 6\r\n"
    );

    if(!FLASH_Test_EraseSector6())
    {
        return;
    }

    if(!FLASH_Test_CheckErase(
            FLASH_TEST_ADDRESS_A,
            sizeof(testDataA)))
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: Sector 6 ERASE CHECK FAILED\r\n"
        );

        return;
    }

    USART_Printf(
        &huart1,
        "FLASH TEST: Sector 6 ERASE CHECK OK\r\n"
    );

    if(!FLASH_Test_WriteRead(
            FLASH_TEST_ADDRESS_A,
            testDataA,
            sizeof(testDataA)))
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: Sector 6 WRITE/READ FAILED\r\n"
        );

        return;
    }

    USART_Printf(
        &huart1,
        "FLASH TEST: Sector 6 WRITE/READ OK\r\n"
    );


    /*
     * Sector 7
     */
    USART_Printf(
        &huart1,
        "TEST SECTOR 7\r\n"
    );

    if(!FLASH_Test_EraseSector7())
    {
        return;
    }

    if(!FLASH_Test_CheckErase(
            FLASH_TEST_ADDRESS_B,
            sizeof(testDataB)))
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: Sector 7 ERASE CHECK FAILED\r\n"
        );

        return;
    }

    USART_Printf(
        &huart1,
        "FLASH TEST: Sector 7 ERASE CHECK OK\r\n"
    );

    if(!FLASH_Test_WriteRead(
            FLASH_TEST_ADDRESS_B,
            testDataB,
            sizeof(testDataB)))
    {
        USART_Printf(
            &huart1,
            "FLASH TEST: Sector 7 WRITE/READ FAILED\r\n"
        );

        return;
    }

    USART_Printf(
        &huart1,
        "FLASH TEST: Sector 7 WRITE/READ OK\r\n"
    );


    USART_Printf(
        &huart1,
        "========== FLASH TEST PASS ==========\r\n"
    );
}


/**
 * @brief Flash 测试入口
 */
void FLASH_Test_Run(void)
{
    switch(FLASH_TEST_MODE)
    {
    case FLASH_TEST_MODE_NONE:

        break;


    case FLASH_TEST_MODE_ERASE_A:

        FLASH_Test_EraseSector6();

        break;


    case FLASH_TEST_MODE_ERASE_B:

        FLASH_Test_EraseSector7();

        break;


    case FLASH_TEST_MODE_WRITE_READ_A:

        FLASH_Test_WriteRead(
            FLASH_TEST_ADDRESS_A,
            testDataA,
            sizeof(testDataA)
        );

        break;


    case FLASH_TEST_MODE_WRITE_READ_B:

        FLASH_Test_WriteRead(
            FLASH_TEST_ADDRESS_B,
            testDataB,
            sizeof(testDataB)
        );

        break;


    case FLASH_TEST_MODE_FULL:

        FLASH_Test_Full();

        break;


    default:

        break;
    }
}



