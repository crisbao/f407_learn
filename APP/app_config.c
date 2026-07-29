#include "app_config.h"
#include "app_status.h"
#include "app_system.h"
#include "usart_driver.h"
#include "usart.h"
#include "flash.h"
#include <string.h>


static APP_ConfigData_t appConfig;
static uint8_t configDirty = 0;


static uint32_t APP_Config_CRC32(const uint8_t *data,
                                uint32_t length);
static uint8_t APP_Config_Check(APP_ConfigData_t *config);
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


/**
 * @brief 从Flash加载配置
 */
APP_ConfigStatus_t APP_Config_Load(void)
{
    APP_ConfigStorage_t storage;

    FLASH_Read(
        FLASH_CONFIG_ADDRESS,
        (uint8_t *)&storage,
        sizeof(APP_ConfigStorage_t)
    );


    /*
     * 检查Magic
     */
    if(storage.magic != APP_CONFIG_MAGIC)
    {
        
        return APP_CONFIG_ERROR_MAGIC;
    }


    /*
     * 检查版本
     */
    if(storage.version != APP_CONFIG_VERSION)
    {
        
        return APP_CONFIG_ERROR_VERSION;
    }


    /*
     * 检查数据长度
     */
    if(storage.length != sizeof(APP_ConfigData_t))
    {
        
        return APP_CONFIG_ERROR_LENGTH;
    }


    /*
     * CRC校验
     */
    uint32_t crc;

    crc =
    APP_Config_CRC32(
        (const uint8_t *)&storage.data,
        sizeof(APP_ConfigData_t)
    );


    if(crc != storage.crc)
    {
        
        return APP_CONFIG_ERROR_CRC;
    }

    /*
    * 数据范围检查
    */
    if(APP_Config_Check(
            &storage.data) == 0)
    {
        return APP_CONFIG_ERROR_VALUE;
    }
    /*
     * 数据有效
     * 恢复配置
     */
    memcpy(
        &appConfig,
        &storage.data,
        sizeof(APP_ConfigData_t)
    );

    return APP_CONFIG_OK;

}


/**
 * @brief 保存配置到Flash
 */
APP_ConfigStatus_t  APP_Config_Save(void)
{
    APP_ConfigStorage_t storage;
    if(APP_Config_Check(&appConfig)==0)
    {
        return APP_CONFIG_ERROR_VALUE;
    }

    storage.magic =
        APP_CONFIG_MAGIC;

    storage.version =
        APP_CONFIG_VERSION;

    storage.length =
        sizeof(APP_ConfigData_t);


    memcpy(
        &storage.data,
        &appConfig,
        sizeof(APP_ConfigData_t)
    );


    storage.crc =
    APP_Config_CRC32(
        (const uint8_t *)&storage.data,
        sizeof(APP_ConfigData_t)
    );


    /*
     * 擦除配置Sector
     */
    if(FLASH_EraseConfigSector() != FLASH_OK)
    {
        return APP_CONFIG_ERROR_FLASH_Erase;
    }


    /*
     * 写入配置
     */
    if(FLASH_Write(
        FLASH_CONFIG_ADDRESS,
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



