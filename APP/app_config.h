#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "app_status.h"
#include "stdint.h"

#define APP_CONFIG_MAGIC       0x53484F4D

#define APP_CONFIG_VERSION     0x0002


#define APP_CONFIG_DEFAULT_INTERVAL 1000
#define APP_CONFIG_INITIAL_SEQUENCE    1U


#define APP_CONFIG_REPAIR_PENDING    1U

/**
 * @brief ��С��������������(ms)
 */
#define APP_CONFIG_MIN_INTERVAL_MS          500U
/**
 * @brief ��󴫸�����������(ms)
 */
#define APP_CONFIG_MAX_INTERVAL_MS          60000U
/**
 * @brief �����ӳٱ���ʱ��(ms)
 */
#define APP_CONFIG_SAVE_DELAY_MS    10000U


#define APP_CONFIG_REPAIR_RETRY_DELAY_MS    1000U


typedef struct
{
    uint32_t sensorIntervalMs;

} APP_ConfigData_t;

typedef struct
{

    uint32_t magic;

    uint16_t version;

    uint16_t length;

    uint32_t sequence;

    APP_ConfigData_t data;

    uint32_t crc;

} APP_ConfigStorage_t;




/**
 * @brief ��ʼ������
 */
void APP_Config_Init(void);


/**
 * @brief ���ô�������������
 */
uint32_t APP_Config_SetSensorInterval(uint32_t ms);


/**
 * @brief ��ȡ��������
 */
uint32_t APP_Config_GetSensorInterval(void);


APP_ConfigStatus_t  APP_Config_Load(void);


APP_ConfigStatus_t  APP_Config_Save(void);

/**
 * @brief �ָ�Ĭ������
 */
void APP_Config_Reset(void);

uint8_t APP_Config_IsDirty(void);

uint8_t APP_Config_IsRepairPending(void);

APP_ConfigStatus_t APP_Config_Repair(void);

void APP_Config_Process(void);

void APP_Config_PrintInfo(void);

uint32_t APP_Config_GetSaveRemainingTime(void);


#endif
