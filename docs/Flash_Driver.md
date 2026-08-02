# STM32F407 Flash 配置存储系统设计文档

> **项目**: STM32F407 智能家居 | **版本**: v0.3 | **日期**: 2026-08-02
> **涉及文件**: `Hardware/flash.c/h`、`APP/app_config.c/h`、`APP/app_event.h`、`APP/app_status.h`

---

# 1. 模块简介

本模块是 STM32F407 智能家居系统的**非易失配置存储子系统**，负责在内部 Flash 中持久化系统运行参数，支持双备份、数据校验、自动修复和延迟写入。

当前系统中 Flash 模块用于保存：

- 传感器采样周期（`sensorIntervalMs`，范围 500ms ~ 60000ms）
- 配置元数据（Magic / Version / Sequence / CRC）

**整体架构：**

```
┌─────────────────────┐
│    APP_Config       │  应用层：配置生命周期管理
│  (app_config.c)     │  校验 / 双备份 / 延迟保存 / 自动修复 / Event 通知
└─────────┬───────────┘
          │ FLASH_EraseSector() / FLASH_Write() / FLASH_Read()
          ▼
┌─────────────────────┐
│   Flash Driver      │  驱动层：硬件访问抽象
│    (flash.c)        │  扇区擦除 / Word 写入 / 直接地址读取
└─────────┬───────────┘
          │ HAL_FLASHEx_Erase() / HAL_FLASH_Program()
          ▼
┌─────────────────────┐
│ STM32F407 Internal  │  硬件层
│   Flash (512KB)     │  Sector6 (0x08040000) + Sector7 (0x08060000)
└─────────────────────┘
```

- **Flash Driver** 负责：Sector 擦除、Flash 写入、Flash 读取、地址对齐校验、Sector 安全保护
- **APP_Config** 负责：配置加载、配置保存、五重数据校验、双备份管理、Sequence 版本控制、自动修复、延迟保存、Event 通知

---

# 2. 软件结构

## 2.1 工程文件

```
f407_learn/
├── Hardware/
│   ├── flash.c                    # Flash 底层驱动（擦除 / 写入 / 读取）
│   └── flash.h                    # Flash 驱动头文件（接口声明 / 地址宏 / 状态枚举）
├── APP/
│   ├── app_config.c               # 配置管理层（校验 / 双备份 / 序列号 / 修复 / 延迟保存）
│   ├── app_config.h               # 配置管理头文件（结构体 / 宏 / 接口声明）
│   ├── app_status.h               # 配置状态码枚举（8 种状态）
│   └── app_event.h                # 事件系统定义（APP_Event_t / Config 事件 ID）
```

## 2.2 模块职责

| 模块 | 职责 | 不负责 |
|------|------|--------|
| **Flash Driver** | Sector 擦除、Word 写入、地址对齐、Sector 安全保护 | 数据校验、备份管理、配置生命周期 |
| **APP_Config** | Magic/Version/Length/CRC/Value 五重校验、双备份选择、Sequence 管理、自动修复、延迟保存、Event 通知 | 底层 Flash 寄存器操作 |

---

# 3. STM32F407 Flash 资源说明

- **MCU**: STM32F407VET6
- **Flash 总容量**: 512KB（0x08000000 ~ 0x0807FFFF）
- **Sector 布局**: 4×16KB + 1×64KB + 3×128KB + 4×128KB = **Sector 0~11 共 12 个扇区**

> **注意**: STM32F407VET6 为 512KB 型号，**仅有 Sector 0~7，没有 Sector 8~11**。Sector 8~11 仅存在于 1MB 型号（VGTx）。

## 配置存储区域

| 区域 | Sector | 地址范围 | 大小 | 用途 |
|------|--------|---------|------|------|
| Config A | Sector 6 | `0x08040000` ~ `0x0805FFFF` | 128KB | 配置备份 A |
| Config B | Sector 7 | `0x08060000` ~ `0x0807FFFF` | 128KB | 配置备份 B |

```c
// [Hardware/flash.h:19-21]
#define FLASH_CONFIG_ADDRESS_A  0x08040000U   // Sector 6 起始
#define FLASH_CONFIG_ADDRESS_B  0x08060000U   // Sector 7 起始
```

> **为什么不用 Sector 11**：STM32F407VET6 只有 512KB Flash，物理上不存在 Sector 8~11。若使用不存在的 Sector 将导致硬件错误。

---

# 4. Flash Driver 设计

## 4.1 FLASH_EraseSector()

```c
FLASH_Status_t FLASH_EraseSector(uint32_t sector);
```

**作用**：擦除指定的 Flash Sector。

**安全保护** — 当前只允许擦除 Sector 6 和 Sector 7，防止误擦程序代码区域：

```c
// [Hardware/flash.c:28-32]
if ((sector != FLASH_SECTOR_6) &&
    (sector != FLASH_SECTOR_7))
{
    return FLASH_ERROR;   // 拒绝擦除其他 Sector
}
```

**执行流程：**

```
FLASH_EraseSector(sector)
    │
    ├─ [sector != SECTOR_6 && sector != SECTOR_7] → return FLASH_ERROR
    │
    ├─ HAL_FLASH_Unlock()                         解锁 Flash 控制寄存器
    │
    ├─ HAL_FLASHEx_Erase(&erase, &error)          擦除指定 Sector
    │    │  .TypeErase   = FLASH_TYPEERASE_SECTORS
    │    │  .Sector      = sector
    │    │  .NbSectors   = 1
    │    │  .VoltageRange = FLASH_VOLTAGE_RANGE_3  (2.7V~3.6V)
    │    │
    │    └─ [失败] → HAL_FLASH_Lock() → return FLASH_ERROR
    │
    └─ HAL_FLASH_Lock()                           return FLASH_OK
```

## 4.2 FLASH_Write()

```c
FLASH_Status_t FLASH_Write(uint32_t address, uint8_t *data, uint32_t length);
```

**作用**：向内部 Flash 按字（Word = 4 字节）写入数据。

**特点**：

- **地址 4 字节对齐校验**：`address % 4 != 0` 时直接返回 `FLASH_ERROR`
- **WORD 方式写入**：每次调用 `HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ...)`
- **自动补齐不足 4 字节的数据**：末尾不足 4 字节时，用 `0xFFFFFFFF` 填充高位字节后写入

```c
// [Hardware/flash.c:73-76] 地址对齐检查
if (address % 4 != 0)
{
    return FLASH_ERROR;
}

// [Hardware/flash.c:85-95] 补齐不足 4 字节
data32 = 0xFFFFFFFF;              // 初始化为全 1
uint32_t size = length >= 4 ? 4 : length;
memcpy(&data32, data, size);      // 覆盖实际数据字节
```

**执行流程：**

```
FLASH_Write(address, data, length)
    │
    ├─ [address % 4 != 0] → return FLASH_ERROR
    │
    ├─ HAL_FLASH_Unlock()
    │
    ├─ while (length > 0):
    │    ├─ 构造 32 位字 (不足 4 字节自动补齐 0xFF)
    │    ├─ HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data32)
    │    │    └─ [失败] → HAL_FLASH_Lock() → return FLASH_ERROR
    │    ├─ address += 4, data += size, length -= size
    │    └─ 循环
    │
    └─ HAL_FLASH_Lock() → return FLASH_OK
```

> **STM32 Flash 物理特性**：Flash 存储器只能从 **1 写为 0**，不能从 **0 写为 1**。要恢复为全 1，必须先执行 Sector Erase。因此修改任何数据必须遵循 **Erase → Write** 流程。

## 4.3 FLASH_Read()

```c
void FLASH_Read(uint32_t address, uint8_t *data, uint32_t length);
```

**作用**：从 Flash 读取数据。

**实现**：Flash 直接映射到 CPU 地址空间，通过 `memcpy` 从地址指针直接复制：

```c
// [Hardware/flash.c:127-138]
void FLASH_Read(uint32_t address, uint8_t *data, uint32_t length)
{
    memcpy(data, (uint8_t *)address, length);
}
```

---

# 5. APP_Config 配置管理设计

`APP_Config` 位于应用层，负责管理 Flash 配置的完整生命周期。

**公开接口：**

| 函数 | 作用 | 调用者 |
|------|------|--------|
| `APP_Config_Init()` | 初始化：复位默认值 → 尝试从 Flash 恢复 → 设置状态 | `APP_Init()` |
| `APP_Config_Load()` | 读取双备份 → 五重校验 → 选择最新有效配置 | `APP_Config_Init()` |
| `APP_Config_Save()` | 选择写入目标 → 计算 Sequence → 擦除 → 写入 → 发 Event | `APP_Config_Process()` / `APP_Config_Repair()` |
| `APP_Config_Process()` | 后台任务：优先修复 → 延迟保存（30s） | `APP_Run()` 主循环 |
| `APP_Config_SetSensorInterval(ms)` | 修改参数 + 置 dirty 标志 + 发 CHANGED Event | `APP_Protocol` / 蓝牙命令 |
| `APP_Config_GetSensorInterval()` | 读取当前参数 | `APP_Run()` / `APP_Protocol` / 显示层 |
| `APP_Config_Reset()` | 恢复默认值 | `APP_Config_Init()` / Load 失败时 |
| `APP_Config_IsDirty()` | 查询是否有未保存的修改 | 外部查询 |
| `APP_Config_PrintInfo()` | 调试输出：打印双备份状态 / RAM 值 / 修复状态 | 调试命令 |

---

# 6. Flash 配置数据结构

配置不是直接保存原始参数，而是包装为带元数据的存储结构：

```
┌──────────────────────────────────────┐
│         APP_ConfigStorage_t          │
├──────────┬───────────────────────────┤
│  magic   │ uint32_t  魔数 0x53484F4D │  ← 数据合法性标记
│ version  │ uint16_t  配置版本 0x0002 │  ← 版本兼容检查
│ length   │ uint16_t  数据体字节数    │  ← 结构大小检查
│ sequence │ uint32_t  单调递增序号    │  ← 最新版本判定
│ data     │ APP_ConfigData_t          │  ← 实际配置参数
│ crc      │ uint32_t  CRC32 校验值    │  ← 数据完整性校验
└──────────┴───────────────────────────┘
```

```c
// [APP/app_config.h:36-51]
typedef struct
{
    uint32_t magic;              // 魔数: 0x53484F4D ("SHOM")
    uint16_t version;            // 版本: 0x0002
    uint16_t length;             // sizeof(APP_ConfigData_t)
    uint32_t sequence;           // 单调递增版本号
    APP_ConfigData_t data;       // 配置数据体
    uint32_t crc;                // data 字段的 CRC32
} APP_ConfigStorage_t;

typedef struct
{
    uint32_t sensorIntervalMs;   // 传感器采样间隔 (ms)
} APP_ConfigData_t;
```

| 字段 | 作用 | 失败后果 |
|------|------|---------|
| `magic` | 判断该 Sector 是否存有有效配置 | `APP_CONFIG_ERROR_MAGIC` |
| `version` | 防止新旧版本结构不兼容 | `APP_CONFIG_ERROR_VERSION` |
| `length` | 防止结构体大小变化导致越界 | `APP_CONFIG_ERROR_LENGTH` |
| `sequence` | 双备份时选择最新版本 | — |
| `data` | 实际业务参数 | — |
| `crc` | 校验 data 字段完整性（多项式 0xEDB88320） | `APP_CONFIG_ERROR_CRC` |

---

# 7. 配置加载流程

```c
APP_ConfigStatus_t APP_Config_Load(void);
```

**完整流程：**

```
APP_Config_Init()
    │
    ├─ APP_Config_Reset()              恢复到默认值
    │
    └─ APP_Config_Load()
         │
         ├─ FLASH_Read(ADDRESS_A, &storageA, sizeof)   ← 读取 Sector6
         ├─ FLASH_Read(ADDRESS_B, &storageB, sizeof)   ← 读取 Sector7
         │
         ├─ APP_Config_CheckStorage(&storageA) → validA
         ├─ APP_Config_CheckStorage(&storageB) → validB
         │
         ├─ [情况 1: A 和 B 都有效]
         │    ├─ storageA.sequence >= storageB.sequence ? 选 A : 选 B
         │    └─ memcpy 到 appConfig → return APP_CONFIG_OK
         │
         ├─ [情况 2: A 有效, B 无效]
         │    ├─ memcpy 到 appConfig (恢复 A 的数据)
         │    ├─ 保存 A 到 repairStorage
         │    ├─ configRepairPending = 1, configRepairTarget = REPAIR_TO_B
         │    └─ return APP_CONFIG_OK
         │
         ├─ [情况 3: B 有效, A 无效]
         │    ├─ memcpy 到 appConfig (恢复 B 的数据)
         │    ├─ 保存 B 到 repairStorage
         │    ├─ configRepairPending = 1, configRepairTarget = REPAIR_TO_A
         │    └─ return APP_CONFIG_OK
         │
         └─ [情况 4: A 和 B 都无效]
              ├─ APP_Config_Reset()                使用默认值
              ├─ 构造 repairStorage (seq=1, 默认参数)
              ├─ configRepairPending = 1
              ├─ configRepairTarget = REPAIR_TO_A   先修复 A
              └─ return APP_CONFIG_ERROR_CRC
```

---

# 8. 配置有效性检查

## 8.1 APP_Config_CheckStorage()

```c
static uint8_t APP_Config_CheckStorage(APP_ConfigStorage_t *storage);
```

**五重校验流程：**

```
APP_Config_CheckStorage(storage)
    │
    ├─ [storage == NULL] → return 0
    │
    ├─ ① Magic 检查
    │    └─ storage->magic != 0x53484F4D → return 0
    │
    ├─ ② Version 检查
    │    └─ storage->version != 0x0002 → return 0
    │
    ├─ ③ Length 检查
    │    └─ storage->length != sizeof(APP_ConfigData_t) → return 0
    │
    ├─ ④ 参数范围检查 (APP_Config_Check)
    │    └─ sensorIntervalMs ∉ [500, 60000] → return 0
    │
    ├─ ⑤ CRC32 校验
    │    └─ APP_Config_CRC32(data, sizeof) ≠ storage->crc → return 0
    │
    └─ return 1   ← 全部通过
```

## 8.2 参数范围检查

```c
static uint8_t APP_Config_Check(APP_ConfigData_t *config);
```

合法范围定义在 `app_config.h`：

```c
#define APP_CONFIG_MIN_INTERVAL_MS  500U    // 最小 500ms
#define APP_CONFIG_MAX_INTERVAL_MS  60000U  // 最大 60s
```

## 8.3 CRC32 校验

软件 CRC32，多项式 `0xEDB88320`（标准 CRC-32/MPEG-2 的反射多项式）：

```c
static uint32_t APP_Config_CRC32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    while (length--)
    {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}
```

---

# 9. 双 Flash 备份机制

系统将同一份配置保存在两个独立的 Flash Sector 中：

```
┌──────────────────────────────────┐
│  Sector 6 (0x08040000)          │
│  ┌────────────────────────────┐ │
│  │  Config A (备份 A)          │ │
│  │  magic/version/length       │ │
│  │  sequence/data/crc          │ │
│  └────────────────────────────┘ │
├──────────────────────────────────┤
│  Sector 7 (0x08060000)          │
│  ┌────────────────────────────┐ │
│  │  Config B (备份 B)          │ │
│  │  magic/version/length       │ │
│  │  sequence/data/crc          │ │
│  └────────────────────────────┘ │
└──────────────────────────────────┘
```

**设计目的**：

- 防止掉电导致配置损坏（擦除到写入之间的窗口期）
- 防止擦写失败导致配置丢失
- 保证至少有一个有效配置可用

**四种加载情况分析：**

| A 状态 | B 状态 | 处理策略 |
|--------|--------|---------|
| 有效 | 有效 | 比较 `sequence`，选择**较大者**（最新版本） |
| 有效 | 无效 | 加载 A，标记需要**后台修复 B** |
| 无效 | 有效 | 加载 B，标记需要**后台修复 A** |
| 无效 | 无效 | 恢复**默认配置**，标记需要**初始化修复 A** |

---

# 10. 配置保存流程

```c
APP_ConfigStatus_t APP_Config_Save(void);
```

**写入策略**：优先写入**较旧**的备份或**无效**的备份——始终保护最新的那份有效数据。

```
APP_Config_Save()
    │
    ├─ target = APP_Config_GetWriteTarget()    确定写入目标
    │    │
    │    ├─ [AB 都有效] → 选 sequence 较小的 (旧的)
    │    ├─ [A 有效 B 无效] → 选 B
    │    ├─ [B 有效 A 无效] → 选 A
    │    └─ [都无效] → 选 A (首次初始化)
    │
    ├─ 填充 storage:
    │    ├─ storage.magic    = 0x53484F4D
    │    ├─ storage.version  = 0x0002
    │    ├─ storage.length   = sizeof(APP_ConfigData_t)
    │    ├─ storage.sequence = APP_Config_GetNextSequence()  ← max(A.seq, B.seq) + 1
    │    ├─ storage.data     = memcpy from appConfig (RAM)
    │    └─ storage.crc      = APP_Config_CRC32(&data)
    │
    ├─ FLASH_EraseSector(target.sector)        擦除目标 Sector
    │    └─ [失败] → return APP_CONFIG_ERROR_FLASH_Erase
    │
    ├─ FLASH_Write(target.address, &storage, sizeof)   写入
    │    └─ [失败] → return APP_CONFIG_ERROR_FLASH_Write
    │
    ├─ APP_Event_Post({                         发送保存完成事件
    │    .type  = APP_EVENT_CONFIG,
    │    .id    = APP_CONFIG_EVENT_SAVE_DONE,
    │    .param = target.address
    │  })
    │
    └─ return APP_CONFIG_OK
```

---

# 11. Sequence 版本管理

```c
static uint32_t APP_Config_GetNextSequence(void);
```

**作用**：读取 A 和 B 两个 Sector 的 `sequence` 值，取**最大值 + 1**，确保全局单调递增。

**示例：**

```
Sector 6:  seq = 20  (有效)
Sector 7:  seq = 21  (有效)
              ↓
下一次保存: seq = 22    ← max(20, 21) + 1

Sector 6:  seq = 15  (有效)
Sector 7:  无效
              ↓
下一次保存: seq = 16    ← max(15, 0) + 1
```

```c
static uint32_t APP_Config_GetNextSequence(void)
{
    uint32_t maxSequence = 0;

    FLASH_Read(ADDRESS_A, &storageA, sizeof);    // 读 A
    if (CheckStorage(&storageA))
        if (storageA.sequence > maxSequence)
            maxSequence = storageA.sequence;

    FLASH_Read(ADDRESS_B, &storageB, sizeof);    // 读 B
    if (CheckStorage(&storageB))
        if (storageB.sequence > maxSequence)
            maxSequence = storageB.sequence;

    return maxSequence + 1;
}
```

---

# 12. 自动修复机制

```c
static APP_ConfigStatus_t APP_Config_Repair(void);
```

**触发条件**：`APP_Config_Load()` 检测到只有一个备份有效，或两个都无效时，设置 `configRepairPending = 1` 及修复目标。

**修复流程：**

```
APP_Config_Repair()
    │
    ├─ [configRepairPending == 0] → return APP_CONFIG_OK  (无需修复)
    │
    ├─ 根据 configRepairTarget 确定目标地址和 Sector:
    │    ├─ REPAIR_TO_A → address=0x08040000, sector=SECTOR_6
    │    └─ REPAIR_TO_B → address=0x08060000, sector=SECTOR_7
    │
    ├─ FLASH_EraseSector(sector)              擦除损坏的 Sector
    │    └─ [失败] → return APP_CONFIG_ERROR_FLASH_Erase
    │
    ├─ FLASH_Write(address, &repairStorage, sizeof)   写入有效数据
    │    └─ [失败] → return APP_CONFIG_ERROR_FLASH_Write
    │
    ├─ configRepairPending = 0                清除修复标志
    ├─ configRepairTarget = REPAIR_NONE
    │
    └─ return APP_CONFIG_OK
```

> **设计要点**：修复不会阻塞初始化——`APP_Config_Load()` 立即用有效备份恢复 RAM 并返回 `APP_CONFIG_OK`，修复动作由 `APP_Config_Process()` 在后续主循环中异步执行。

---

# 13. 延迟保存机制

**触发**：`APP_Config_SetSensorInterval()` 修改参数时**不立即写 Flash**，只修改 RAM 并设置脏标志。

```c
void APP_Config_SetSensorInterval(uint32_t ms)
{
    if (ms < APP_CONFIG_MIN_INTERVAL_MS)
        ms = APP_CONFIG_MIN_INTERVAL_MS;

    if (appConfig.sensorIntervalMs != ms)
    {
        appConfig.sensorIntervalMs = ms;       // ① 仅修改 RAM
        configDirty = 1;                        // ② 标记为脏
        configDirtyTick = HAL_GetTick();        // ③ 记录修改时间

        APP_Event_Post({                        // ④ 发送变更事件
            .type  = APP_EVENT_CONFIG,
            .id    = APP_CONFIG_EVENT_CHANGED,
            .param = ms
        });
    }
}
```

**为什么要延迟保存**：

- **减少擦写次数**：Flash 寿命有限（典型 10,000 次擦写），频繁修改参数时不立即写入
- **防抖动**：参数可能在短时间内多次修改（如蓝牙反复发送 INTERVAL 命令），30 秒内只写一次

**延迟时间**：`APP_CONFIG_SAVE_DELAY_MS = 30000`（30 秒）

```c
// [APP/app_config.h:27]
#define APP_CONFIG_SAVE_DELAY_MS  30000U
```

---

# 14. APP_Config_Process 后台任务

在主循环 `APP_Run()` 中每次迭代都会调用：

```c
// [APP/app.c:93]
APP_Config_Process();
```

**执行优先级**：修复 > 延迟保存。

```
APP_Config_Process()
    │
    ├─ [configRepairPending != 0]
    │    └─ APP_Config_Repair()               优先处理修复
    │         └─ return  (本轮不再处理保存)
    │
    └─ [configDirty != 0]
         │
         └─ [HAL_GetTick() - configDirtyTick >= 30000ms]
              │
              └─ APP_Config_Save()
                   │
                   └─ [成功] → configDirty = 0
```

---

# 15. Event 系统关联

`APP_Config` 在两种场景下通过 `APP_Event_Post()` 发送事件：

| 触发条件 | event.type | event.id | event.param | 用途 |
|---------|-----------|----------|-------------|------|
| 参数被修改 (`SetSensorInterval`) | `APP_EVENT_CONFIG` | `APP_CONFIG_EVENT_CHANGED` | 新的间隔值 (ms) | 通知系统参数已变化 |
| Flash 写入完成 (`Save` / `Repair`) | `APP_EVENT_CONFIG` | `APP_CONFIG_EVENT_SAVE_DONE` | 写入的目标地址 | 通知系统持久化完成 |

```c
// [APP/app_config.c:387-402] Save 完成后通知
APP_Event_t event;
event.type  = APP_EVENT_CONFIG;
event.id    = APP_CONFIG_EVENT_SAVE_DONE;
event.param = target.address;
APP_Event_Post(&event);
```

---

# 16. 调试信息

```c
void APP_Config_PrintInfo(void);
```

通过 USART1 输出完整的配置系统状态：

```
========== CONFIG ==========
FLASH A:
valid=1 seq=22 interval=2000 crc=0xABCD1234
FLASH B:
valid=1 seq=21 interval=1000 crc=0x1234ABCD
RAM:
interval=2000 ms
RepairPending=0 Target=0
============================
```

---

# 17. 容易出错的问题

## 1. Sector 选择错误

STM32F407VET6 仅有 512KB Flash（Sector 0~7），不能使用 Sector 8~11。`FLASH_EraseSector()` 已加入安全保护，对非法 Sector 直接返回 `FLASH_ERROR`。

## 2. Flash 不能覆盖写

**错误做法**：直接向已编程的地址写新数据。

**正确做法**：先 `FLASH_EraseSector()` 擦除整个 Sector，再 `FLASH_Write()` 写入新数据。当前 `APP_Config_Save()` 严格遵循 Erase → Write 流程。

## 3. Flash 擦写时禁止在中断中执行

Flash 擦除/写入操作耗时较长（擦除一个 128KB Sector 约需 1~2 秒），且期间 Flash 总线停滞。**绝不能**在 ISR 上下文中执行 Flash 操作。

**正确做法**：当前设计将 Flash 操作放在 `APP_Config_Process()` 中，由主循环调用——不阻塞中断响应。

---

# 18. FreeRTOS 迁移注意事项

**当前架构（裸机）：**

```
APP_Run() 主循环
    │
    └─ APP_Config_Process()
         ├─ APP_Config_Repair()   ← 可能耗时 1~2s
         └─ APP_Config_Save()     ← 可能耗时 1~2s
```

**迁移后（FreeRTOS）：**

```
ConfigTask (独立任务，低优先级)
    │
    ├─ xQueueReceive() 等待保存请求
    │
    └─ APP_Config_Save() / Repair()
         └─ 阻塞整个任务，不阻塞其他任务
```

**迁移要点**：

| 事项 | 策略 |
|------|------|
| Flash 操作为阻塞操作 | 必须放入独立 Task，**不能**在 ISR 或高优先级任务中执行 |
| `configDirty` 标志 | 无需互斥锁（只有 ConfigTask 和 INTERVAL 命令修改，后者可通过 Queue 发消息） |
| 延迟保存计时 | `HAL_GetTick()` 改为 `xTaskGetTickCount()` |
| Sequence 读取 | 当前在主循环中执行，迁移后在 ConfigTask 中执行，无需改动 |
| Event 通知 | 保留 `APP_Event_Post()`，但 Event 系统本身需改为线程安全（Queue 实现） |

**推荐架构：**

```
ISR / 蓝牙命令 / 其他 Task
    │
    ▼  发送 Config Event
Event Queue
    │
    ▼
ConfigTask (xQueueReceive 阻塞等待)
    │
    ▼
APP_Config_Process() → Repair / Delayed Save
```

---

# 19. 总结

当前 Flash 配置存储系统已经实现：

- ✅ Flash 底层驱动（Sector 擦除 / Word 写入 / 直接地址读取 / 4 字节对齐校验）
- ✅ Sector 安全保护（仅允许 Sector 6 和 Sector 7）
- ✅ 双备份存储（Sector6 = A, Sector7 = B）
- ✅ 五重数据校验（Magic → Version → Length → Value Range → CRC32）
- ✅ Sequence 单调递增版本管理
- ✅ 自动修复（单备份有效时后台恢复双备份）
- ✅ 延迟保存（30 秒防抖，减少 Flash 擦写次数）
- ✅ Event 通知（CHANGED / SAVE_DONE）
- ✅ 调试信息输出（双备份状态 / RAM 值 / 修复状态）

**核心设计思想：**

> **Flash Driver 负责可靠访问硬件，APP_Config 负责配置可靠性管理。**

分层设计带来的收益：
1. **安全性**：Sector 保护 + 五重校验 + 双备份，容忍掉电和写入失败
2. **可维护性**：驱动层只做硬件抽象，应用层管理配置生命周期，各司其职
3. **可移植性**：Flash Driver 接口独立于业务逻辑，FreeRTOS 迁移时仅需调整任务调度
