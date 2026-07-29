#include "app_config.h"
#include "app_status.h"
#include "app_system.h"
#include "usart_driver.h"
#include "usart.h"
#include "flash.h"
#include <string.h>


typedef struct
{
    uint32_t address;

    uint32_t sector;

}APP_ConfigTarget_t;
static APP_ConfigData_t appConfig;
static uint8_t configDirty = 0;



static uint32_t APP_Config_CRC32(const uint8_t *data,
                                uint32_t length);
static uint8_t APP_Config_Check(APP_ConfigData_t *config);
static uint32_t APP_Config_GetNextSequence(void);
static APP_ConfigTarget_t APP_Config_GetWriteTarget(void);
uint8_t APP_Config_IsDirty(void);


/**
 * @brief 初始化配置
 */
void APP_Config_Init(void)
{
    APP_ConfigStatus_t status;
    /*
     * 设置默认配置
     */
    APP_Config_Reset();


    /*
     * 尝试从Flash恢复配置
     */
    status = APP_Config_Load();

    /*
     * 保存配置状态
     */
    APP_System_SetConfigStatus(status);
		
		USART_Printf(
        &huart1,
        "Config status=%d\r\n",
        status);
}


/**
 * @brief 恢复默认配置
 */
void APP_Config_Reset(void)
{
    appConfig.sensorIntervalMs =
        APP_CONFIG_DEFAULT_INTERVAL;
}

static uint8_t APP_Config_CheckStorage(
        APP_ConfigStorage_t *storage
)
{

    if(storage == NULL)
    {
        return 0;
    }

    /*
     * Magic检查
     */
    if(storage->magic != APP_CONFIG_MAGIC)
    {
        return 0;
    }

    /*
     * Version检查
     */
    if(storage->version != APP_CONFIG_VERSION)
    {
        return 0;
    }

    /*
     * 长度检查
     */
    if(storage->length != sizeof(APP_ConfigData_t))
    {
        return 0;
    }

    /*
     * 数据范围检查
     */
    if(!APP_Config_Check(
            &storage->data))
    {
        return 0;
    }

    /*
     * CRC检查
     */
    uint32_t crc;

    crc =
    APP_Config_CRC32(
        (const uint8_t *)&storage->data,
        sizeof(APP_ConfigData_t)
    );

    if(crc != storage->crc)
    {
        return 0;
    }

    return 1;

}

/**
 * @brief 从Flash加载配置
 */
APP_ConfigStatus_t APP_Config_Load(void)
{

    APP_ConfigStorage_t storageA;

    APP_ConfigStorage_t storageB;

    uint8_t validA;

    uint8_t validB;

    /*
     * 读取Sector6
     */
    FLASH_Read(
        FLASH_CONFIG_ADDRESS_A,
        (uint8_t *)&storageA,
        sizeof(APP_ConfigStorage_t)
    );

    /*
     * 读取Sector7
     */
    FLASH_Read(
        FLASH_CONFIG_ADDRESS_B,
        (uint8_t *)&storageB,
        sizeof(APP_ConfigStorage_t)
    );

    /*
     * 检查有效性
     */
    validA =
    APP_Config_CheckStorage(
        &storageA
    );

    validB =
    APP_Config_CheckStorage(
        &storageB
    );

    /*
     * 两个都有效
     */
    if(validA && validB)
    {

        if(storageA.sequence >=
           storageB.sequence)
        {

            memcpy(
                &appConfig,
                &storageA.data,
                sizeof(APP_ConfigData_t)
            );

        }
        else
        {

            memcpy(
                &appConfig,
                &storageB.data,
                sizeof(APP_ConfigData_t)
            );

        }

        return APP_CONFIG_OK;
    }

    /*
     * A有效 B无效
     */
    if(validA)
    {

        memcpy(
            &appConfig,
            &storageA.data,
            sizeof(APP_ConfigData_t)
        );


        return APP_CONFIG_OK;
    }

    /*
     * B有效 A无效
     */
    if(validB)
    {

        memcpy(
            &appConfig,
            &storageB.data,
            sizeof(APP_ConfigData_t)
        );


        return APP_CONFIG_OK;
    }

    /*
     * 两个都无效
     */
    APP_Config_Reset();

    return APP_CONFIG_ERROR_CRC;

}

/**
 * @brief 保存配置到Flash
 */
APP_ConfigStatus_t APP_Config_Save(void)
{

    APP_ConfigStorage_t storage;


    APP_ConfigTarget_t target;

    /*
     * 获取写入目标
     */
    target =
        APP_Config_GetWriteTarget();

    /*
     * 填充头信息
     */
    storage.magic =
        APP_CONFIG_MAGIC;

    storage.version =
        APP_CONFIG_VERSION;

    storage.length =
        sizeof(APP_ConfigData_t);

    /*
     * 生成新的sequence
     */
    storage.sequence =
        APP_Config_GetNextSequence();

    /*
     * 拷贝配置数据
     */
    memcpy(
        &storage.data,
        &appConfig,
        sizeof(APP_ConfigData_t)
    );

    /*
     * CRC计算
     */
    storage.crc =
    APP_Config_CRC32(
        (const uint8_t *)&storage.data,
        sizeof(APP_ConfigData_t)
    );

    /*
     * 擦除目标Sector
     */
    if(FLASH_EraseSector(
            target.sector)
            != FLASH_OK)
    {
        return APP_CONFIG_ERROR_FLASH_Erase;
    }

    /*
     * 写入配置
     */
    if(FLASH_Write(
            target.address,
            (uint8_t *)&storage,
            sizeof(storage))
            != FLASH_OK)
    {
        return APP_CONFIG_ERROR_FLASH_Write;
    }

    return APP_CONFIG_OK;

}


/**
 * @brief 设置传感器采样周期
 */
void APP_Config_SetSensorInterval(uint32_t ms)
{

    if(ms < 500)
    {
        ms = 500;
    }

    if(appConfig.sensorIntervalMs != ms)
    {

        appConfig.sensorIntervalMs = ms;

        configDirty = 1;

    }

}


/**
 * @brief 获取传感器采样周期
 */
uint32_t APP_Config_GetSensorInterval(void)
{
    return appConfig.sensorIntervalMs;
}


/**
 * @brief CRC32计算
 */
static uint32_t APP_Config_CRC32(const uint8_t *data,
                                uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;


    while(length--)
    {
        crc ^= *data++;


        for(uint8_t i = 0; i < 8; i++)
        {
            if(crc & 1)
            {
                crc =
                (crc >> 1)
                ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }


    return crc ^ 0xFFFFFFFF;
}

static uint8_t APP_Config_Check(
    APP_ConfigData_t *config
)
{
    if(config == NULL)
    {
        return 0;
    }

    /*
     * 采样周期范围检查
     *
     * 最小:
     * 500ms
     *
     * 最大:
     * 60000ms
     */
    if(config->sensorIntervalMs < 500)
    {
        return 0;
    }


    if(config->sensorIntervalMs > 60000)
    {
        return 0;
    }


    return 1;
}

uint8_t APP_Config_IsDirty(void)
{
    return configDirty;
}

static uint32_t APP_Config_GetNextSequence(void)
{

    APP_ConfigStorage_t storageA;

    APP_ConfigStorage_t storageB;

    uint32_t maxSequence = 0;


    /*
     * 读取A区
     */
    FLASH_Read(
        FLASH_CONFIG_ADDRESS_A,
        (uint8_t *)&storageA,
        sizeof(APP_ConfigStorage_t)
    );

    /*
     * 读取B区
     */
    FLASH_Read(
        FLASH_CONFIG_ADDRESS_B,
        (uint8_t *)&storageB,
        sizeof(APP_ConfigStorage_t)
    );

    /*
     * A有效
     */
    if(APP_Config_CheckStorage(&storageA))
    {
        if(storageA.sequence > maxSequence)
        {
            maxSequence =
                storageA.sequence;
        }
    }

    /*
     * B有效
     */
    if(APP_Config_CheckStorage(&storageB))
    {
        if(storageB.sequence > maxSequence)
        {
            maxSequence =
                storageB.sequence;
        }
    }

    return maxSequence + 1;

}

static APP_ConfigTarget_t APP_Config_GetWriteTarget(void)
{

    APP_ConfigStorage_t storageA;

    APP_ConfigStorage_t storageB;

    APP_ConfigTarget_t target;

    uint8_t validA;

    uint8_t validB;

    FLASH_Read(
        FLASH_CONFIG_ADDRESS_A,
        (uint8_t *)&storageA,
        sizeof(APP_ConfigStorage_t)
    );

    FLASH_Read(
        FLASH_CONFIG_ADDRESS_B,
        (uint8_t *)&storageB,
        sizeof(APP_ConfigStorage_t)
    );


    validA =
    APP_Config_CheckStorage(
        &storageA
    );

    validB =
    APP_Config_CheckStorage(
        &storageB
    );

    /*
     * A、B都有效
     *
     * 擦除旧的数据
     */
    if(validA && validB)
    {

        if(storageA.sequence <= storageB.sequence)
        {
            target.address =
                FLASH_CONFIG_ADDRESS_A;

            target.sector =
                FLASH_SECTOR_6;
        }
        else
        {
            target.address =
                FLASH_CONFIG_ADDRESS_B;

            target.sector =
                FLASH_SECTOR_7;
        }

        return target;

    }

    /*
     * A有效，B无效
     *
     * 写B，保留A备份
     */
    if(validA)
    {

        target.address =
            FLASH_CONFIG_ADDRESS_B;

        target.sector =
            FLASH_SECTOR_7;


        return target;

    }

    /*
     * B有效，A无效
     *
     * 写A
     */
    if(validB)
    {

        target.address =
            FLASH_CONFIG_ADDRESS_A;

        target.sector =
            FLASH_SECTOR_6;


        return target;

    }

    /*
     * 两个都无效
     *
     * 第一次初始化写A
     */
    target.address =
        FLASH_CONFIG_ADDRESS_A;

    target.sector =
        FLASH_SECTOR_6;

    return target;

}

