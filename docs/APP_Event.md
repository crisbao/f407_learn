# STM32F407 APP_Event 事件系统设计文档

> **项目**: STM32F407 智能家居 | **版本**: v0.3 | **日期**: 2026-08-02
> **涉及文件**: `APP/app_event.c/h`、`APP/app_event_handle.c/h`、`APP/app.c`、`APP/app_config.c`、`APP/app_sensor.c`、`APP/app_protocol.c`

---

# 1. 模块简介

APP_Event 模块是 STM32F407 智能家居系统 APP 层的**事件通信中心**，用于降低各功能模块之间的耦合。

**传统轮询方式：**

```
main 循环
    │
    ├─ 轮询传感器
    ├─ 轮询蓝牙协议
    ├─ 轮询配置
    ├─ 刷新显示
    └─ ...每个模块都要知道何时触发
```

**当前事件驱动方式：**

```
模块产生事件 → Event 队列 → 统一分发 → 执行对应动作
```

模块之间不再直接调用，而是通过 Event 系统传递消息。例如：

- Sensor 模块不直接调用 Display，而是发送 `APP_EVENT_SENSOR` / `APP_SENSOR_EVENT_UPDATE`
- Protocol 模块不直接调用 Display，而是发送 `APP_EVENT_DISPLAY` / `APP_DISPLAY_EVENT_PAGE_CHANGED`
- Config 参数变更不直接调用 Timer，而是发送 `APP_EVENT_CONFIG` / `APP_CONFIG_EVENT_CHANGED`

---

# 2. APP 层整体事件架构

```
                          ┌─────────────────────┐
                          │     APP_Event       │
                          │   (事件队列中心)      │
                          └──────────┬──────────┘
                                     │
          ┌──────────────┬───────────┼───────────┬──────────────┐
          │              │           │           │              │
          ▼              ▼           ▼           ▼              ▼
    ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
    │ Sensor   │  │ Config   │  │ Display  │  │ System   │  │ Protocol │
    │ (数据源)  │  │ (参数源)  │  │ (消费者)  │  │ (启动源)  │  │ (命令源)  │
    └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘
         │              │           │              │              │
         └──────────────┴─────┬─────┴──────────────┴──────────────┘
                              │
                              ▼
                   ┌──────────────────────┐
                   │ APP_Event_Process()  │
                   │ (统一事件分发)         │
                   └──────────┬───────────┘
                              │
                              ▼
                     执行业务逻辑
```

- **生产者**：Sensor、Config、System、Protocol（任何模块均可通过 `APP_Event_Post()` 发送事件）
- **消费者**：`APP_Event_Process()` 统一从队列取出事件并分发到对应处理逻辑
- **队列**：8 槽位 FIFO 队列，head/tail 指针 + count 计数

---

# 3. 文件结构

```
APP/
├── app_event.h              # 事件系统头文件（枚举 / 结构体 / 接口声明）
├── app_event.c              # 事件队列实现（Post / Get / Init / 丢失计数）
├── app_event_handle.h       # 事件处理头文件（APP_Event_Process 声明）
└── app_event_handle.c       # 事件处理实现（switch 分发到各模块）
```

- **app_event.c**：实现事件队列的存储和管理（不关心事件含义）
- **app_event_handle.c**：根据事件类型和 ID 分发到具体处理逻辑（理解业务含义）

---

# 4. Event 数据结构设计

事件使用统一的 `APP_Event_t` 结构体在模块之间传递：

```c
// [APP/app_event.h:64-72]
typedef struct
{
    APP_EventType_t type;    // 事件大类（哪个模块产生）
    uint16_t       id;       // 具体事件（什么动作）
    uint32_t       param;    // 附加数据（目标地址 / 新参数值 / 页面编号等）
} APP_Event_t;
```

| 成员 | 类型 | 作用 | 示例 |
|------|------|------|------|
| `type` | `APP_EventType_t` | 事件大类，标识来源模块 | `APP_EVENT_SENSOR`、`APP_EVENT_CONFIG` |
| `id` | `uint16_t` | 具体事件 ID，标识发生的动作 | `APP_SENSOR_EVENT_UPDATE`、`APP_CONFIG_EVENT_CHANGED` |
| `param` | `uint32_t` | 附加参数，传递动作相关数据 | 新的间隔值(ms)、Flash 目标地址、页面枚举值 |

---

# 5. Event 类型设计

```c
// [APP/app_event.h:9-23]
typedef enum
{
    APP_EVENT_NONE    = 0,   // 无效事件
    APP_EVENT_SENSOR,        // 传感器模块事件
    APP_EVENT_CONFIG,        // 配置模块事件
    APP_EVENT_DISPLAY,       // 显示模块事件
    APP_EVENT_PROTOCOL,      // 协议模块事件（预留）
    APP_EVENT_SYSTEM         // 系统模块事件
} APP_EventType_t;
```

| 事件类型 | 枚举值 | 来源模块 | 作用 |
|---------|--------|---------|------|
| `APP_EVENT_SENSOR` | 1 | `app_sensor.c` | 通知传感器数据已更新 |
| `APP_EVENT_CONFIG` | 2 | `app_config.c` | 通知配置参数变化 / Flash 保存完成 |
| `APP_EVENT_DISPLAY` | 3 | `app_protocol.c` | 通知显示页面切换 |
| `APP_EVENT_SYSTEM` | 5 | `app.c` | 系统启动事件 |
| `APP_EVENT_PROTOCOL` | 4 | — | 预留，当前未使用 |

---

# 6. Event ID 设计

每种事件类型下定义具体的子事件 ID：

```c
// [APP/app_event.h:28-63]
// Sensor 子事件
typedef enum {
    APP_SENSOR_EVENT_UPDATE = 0,   // 传感器数据已更新
} APP_SensorEvent_t;

// Config 子事件
typedef enum {
    APP_CONFIG_EVENT_CHANGED   = 0,   // 配置参数已变化
    APP_CONFIG_EVENT_REPAIR_DONE,     // 配置修复完成（预留，当前未使用）
    APP_CONFIG_EVENT_SAVE_DONE        // Flash 保存完成
} APP_ConfigEvent_t;

// Display 子事件
typedef enum {
    APP_DISPLAY_EVENT_REFRESH      = 0,   // 请求刷新显示
    APP_DISPLAY_EVENT_PAGE_CHANGED        // 页面切换
} APP_DisplayEvent_t;

// System 子事件
typedef enum {
    APP_SYSTEM_EVENT_BOOT = 0,   // 系统启动完成
} APP_SystemEvent_t;
```

**当前实际使用的事件 ID 汇总：**

| type | id | 生产者 | 消费者动作 | param 含义 |
|------|-----|--------|-----------|-----------|
| `APP_EVENT_SENSOR` | `APP_SENSOR_EVENT_UPDATE` | `APP_Sensor_Update()` | `APP_Display_Update()` | 未使用 (0) |
| `APP_EVENT_CONFIG` | `APP_CONFIG_EVENT_CHANGED` | `APP_Config_SetSensorInterval()` | `APP_Timer_SetInterval()` | 新的采样间隔 (ms) |
| `APP_EVENT_CONFIG` | `APP_CONFIG_EVENT_SAVE_DONE` | `APP_Config_Save()` | USART1 调试打印 | Flash 写入地址 |
| `APP_EVENT_DISPLAY` | `APP_DISPLAY_EVENT_PAGE_CHANGED` | `APP_Cmd_Page()` | `APP_Display_SetPage()` + `APP_Display_Update()` | 页面枚举值 |
| `APP_EVENT_SYSTEM` | `APP_SYSTEM_EVENT_BOOT` | `APP_Init()` | `APP_Display_Update()` | 未使用 (0) |

---

# 7. Event 初始化流程

```c
// [APP/app_event.c:15-23]
void APP_Event_Init(void)
{
    head = 0;
    tail = 0;
    count = 0;
    lostEvent = 0;
}
```

**调用时机**：`APP_Init()` 中第一个初始化（[APP/app.c:34](APP/app.c#L34)），在其他模块初始化之前完成。

**流程：**

```
系统启动
    │
    ▼
main() → APP_Init()
    │
    ├─ APP_Event_Init()          ← ① 最先初始化事件队列
    │    ├─ head = 0             清零读指针
    │    ├─ tail = 0             清零写指针
    │    ├─ count = 0            清零事件计数
    │    └─ lostEvent = 0        清零丢失计数
    │
    ├─ [...]                     其他模块初始化
    │
    └─ APP_Event_Post({BOOT})    ← 其他模块就绪后发送系统启动事件
```

---

# 8. Event 发送流程

```c
// [APP/app_event.c:26-52]
uint8_t APP_Event_Post(const APP_Event_t *event);
```

任何模块产生事件都通过此接口发送。

**流程：**

```
模块产生动作
    │
    ├─ 创建 APP_Event_t 结构体
    │    ├─ .type  = APP_EVENT_XXX    填写事件大类
    │    ├─ .id    = APP_XXX_EVENT_YYY 填写具体事件 ID
    │    └─ .param = 附加数据          填写参数
    │
    └─ APP_Event_Post(&event)
         │
         ├─ [event == NULL] → return APP_EVENT_ERROR
         ├─ [count >= 8]    → lostEvent++ → return APP_EVENT_ERROR  (队列满)
         │
         ├─ eventQueue[tail] = *event    写入队列
         ├─ tail++
         ├─ [tail >= 8] → tail = 0      尾指针回绕
         ├─ count++
         └─ return APP_EVENT_OK
```

**实例：Sensor 模块发送更新事件**

```c
// [APP/app_sensor.c:57-67]
APP_Event_t event;
event.type  = APP_EVENT_SENSOR;
event.id    = APP_SENSOR_EVENT_UPDATE;
event.param = 0;
APP_Event_Post(&event);
```

**实例：Config 模块发送参数变更事件**

```c
// [APP/app_config.c:428-444]
APP_Event_t event;
event.type  = APP_EVENT_CONFIG;
event.id    = APP_CONFIG_EVENT_CHANGED;
event.param = ms;              // 新的传感器采样间隔
APP_Event_Post(&event);
```

---

# 9. Event 读取流程

```c
// [APP/app_event.c:54-76]
uint8_t APP_Event_Get(APP_Event_t *event);
```

**流程：**

```
APP_Event_Get(&event)
    │
    ├─ [event == NULL] → return APP_EVENT_ERROR
    ├─ [count == 0]    → return APP_EVENT_ERROR   (队列空)
    │
    ├─ *event = eventQueue[head]    从队列读取
    ├─ head++
    ├─ [head >= 8] → head = 0      头指针回绕
    ├─ count--
    └─ return APP_EVENT_OK
```

**辅助查询函数：**

```c
// [APP/app_event.c:78-82]
uint8_t APP_Event_IsEmpty(void)      // count == 0 → 队列为空

// [APP/app_event.c:84-87]
uint32_t APP_Event_GetLostCount(void) // 返回 lostEvent（曾丢失的事件数）
```

> **注意**：`APP_Event_GetPostCount()` 和 `APP_Event_GetProcessCount()` 在头文件中声明但 **当前未实现**（.c 文件中对应的 `eventPostCount` 和 `eventProcessCount` 变量已定义但从未递增）。

---

# 10. APP_Event_Process 处理流程

```c
// [APP/app_event_handle.c:8-87]
void APP_Event_Process(void);
```

在主循环 `APP_Run()` 中每次迭代调用，将所有待处理事件一次性消费完毕：

```
APP_Run()                               [APP/app.c:79]
    │
    ├─ APP_Timer_Process()
    │
    └─ APP_Event_Process()              [app_event_handle.c:8]
         │
         └─ while (APP_Event_Get(&event) == APP_EVENT_OK)
              │
              └─ switch (event.type)
                   │
                   ├─ APP_EVENT_SENSOR:
                   │    └─ [id == APP_SENSOR_EVENT_UPDATE]
                   │         └─ APP_Display_Update()       → 刷新 OLED
                   │
                   ├─ APP_EVENT_SYSTEM:
                   │    └─ [id == APP_SYSTEM_EVENT_BOOT]
                   │         └─ APP_Display_Update()       → 刷新 OLED
                   │
                   ├─ APP_EVENT_CONFIG:
                   │    ├─ [id == APP_CONFIG_EVENT_CHANGED]
                   │    │    └─ APP_Timer_SetInterval(      → 更新传感器定时器周期
                   │    │         APP_TIMER_SENSOR,
                   │    │         event.param)               param = 新间隔(ms)
                   │    │
                   │    └─ [id == APP_CONFIG_EVENT_SAVE_DONE]
                   │         └─ USART_Printf(&huart1,        → 调试输出
                   │              "...addr=0x%08lX",
                   │              event.param)
                   │
                   ├─ APP_EVENT_DISPLAY:
                   │    └─ [id == APP_DISPLAY_EVENT_PAGE_CHANGED]
                   │         ├─ APP_Display_SetPage(param)   → 切换显示页面
                   │         │    param = APP_DisplayPage_t 枚举值
                   │         └─ APP_Display_Update()         → 刷新 OLED
                   │
                   └─ default: break
```

---

# 11. 当前系统事件实例分析

## 11.1 Sensor 事件（DHT11 数据更新 → 显示刷新）

```
APP_Timer_SensorCallback()          [app.c] 定时器到期
    │
    └─ APP_Sensor_Update()          [app_sensor.c:38]
         │
         ├─ DHT11_Read(&sensorData)
         ├─ [读取成功 + 数据变化]
         │    │
         │    └─ APP_Event_Post({.type=SENSOR, .id=UPDATE})
         │         │
         │         └─ 进入事件队列
         │
         └─ 保存 lastSensorData 快照（防止重复触发）

[...主循环继续...]

APP_Event_Process()                 [app_event_handle.c:8]
    │
    └─ case APP_EVENT_SENSOR:
         └─ APP_Display_Update()    OLED 读取最新数据并刷新
```

> **设计要点**：`APP_Sensor_Update` 只在数据**真正变化**时才发送事件（通过 `memcmp` 与上次数据对比），避免无意义刷新。

## 11.2 Config 事件（参数变化 → 定时器周期更新 → 异步保存）

```
蓝牙 INTERVAL 命令
    │
    └─ APP_Config_SetSensorInterval(2000)  [app_config.c:410]
         │
         ├─ appConfig.sensorIntervalMs = 2000   (仅修改 RAM)
         ├─ configDirty = 1, configDirtyTick = now
         │
         └─ APP_Event_Post({.type=CONFIG, .id=CHANGED, .param=2000})
              │
              └─ 进入事件队列

[...主循环...]

APP_Event_Process()
    │
    └─ case APP_EVENT_CONFIG / CHANGED:
         └─ APP_Timer_SetInterval(SENSOR, 2000)
              → 传感器采样周期立即生效

[...30 秒后延迟保存...]

APP_Config_Process()
    └─ APP_Config_Save()
         ├─ FLASH_EraseSector + FLASH_Write
         │
         └─ APP_Event_Post({.type=CONFIG, .id=SAVE_DONE, .param=addr})
              │
              └─ APP_Event_Process()
                   └─ USART_Printf("Config Flash Save Done addr=0x0804xxxx")
```

## 11.3 Display 事件（蓝牙 PAGE 命令 → 页面切换）

```
蓝牙 PAGE DEBUG 命令
    │
    └─ APP_Cmd_Page("DEBUG")           [app_protocol.c:115]
         │
         ├─ page = DISPLAY_PAGE_DEBUG
         │
         └─ APP_Event_Post({.type=DISPLAY, .id=PAGE_CHANGED, .param=DEBUG})
              │
              └─ 进入事件队列

[...主循环...]

APP_Event_Process()
    │
    └─ case APP_EVENT_DISPLAY / PAGE_CHANGED:
         ├─ APP_Display_SetPage(DEBUG)   切换页面变量
         └─ APP_Display_Update()         强制刷新 OLED
```

> **已知问题**：`APP_Cmd_Page()` 中 `APP_Event_Post()` 被调用两次（[app_protocol.c:146-150](APP/app_protocol.c#L146) 和 [app_protocol.c:152-153](APP/app_protocol.c#L152)），第二次调用用于判断返回值并返回 "EVENT FULL"。这意味着同一事件会被重复投递两次。

## 11.4 Boot 事件（系统启动 → 初始显示）

```
APP_Init()
    │
    ├─ [所有模块初始化完毕]
    │
    └─ APP_Event_Post({.type=SYSTEM, .id=BOOT, .param=0})
         │
         └─ 进入事件队列

[...主循环第一帧...]

APP_Event_Process()
    │
    └─ case APP_EVENT_SYSTEM / BOOT:
         └─ APP_Display_Update()
              → 首次渲染 OLED（首页 + 传感器数据 + LED 状态）
```

---

# 12. Event 与模块关系图

```
                          ┌──────────────┐
                          │    HC-05     │
                          │ (蓝牙输入)    │
                          └──────┬───────┘
                                 │ HC05_BufferRead()
                                 ▼
                          ┌──────────────┐
                          │ APP_Protocol │
                          │  (命令解析)   │
                          └──┬───────┬───┘
                             │       │
                    PAGE 命令│       │ INTERVAL 命令
                             │       │
                  ┌──────────┘       └──────────────┐
                  │  APP_Event_Post()               │  APP_Config_SetSensorInterval()
                  ▼                                 │
         ┌────────────────┐                        │
         │  APP_EVENT     │◄───────────────────────┘
         │  DISPLAY       │     APP_Event_Post()
         │  PAGE_CHANGED  │
         └───────┬────────┘
                 │
    ┌────────────┼────────────┬────────────────┐
    │            │            │                │
    ▼            ▼            ▼                ▼
┌───────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
│Sensor │  │ Config   │  │ Display  │  │ System   │
│       │  │          │  │          │  │          │
│UPDATE │  │CHANGED   │  │PAGE_CHGD │  │BOOT      │
│       │  │SAVE_DONE │  │          │  │          │
└───┬───┘  └────┬─────┘  └────┬─────┘  └────┬─────┘
    │            │             │              │
    │            │             │              │
    ▼            ▼             ▼              ▼
┌───────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
│ DHT11 │  │  Flash   │  │   OLED   │  │  启动    │
│       │  │          │  │          │  │          │
│ 数据源 │  │  存储    │  │  显示    │  │  初始化   │
└───────┘  └──────────┘  └──────────┘  └──────────┘
```

---

# 13. Event 系统优势

## 降低模块耦合

Sensor 模块不知道 OLED 存在，也不调用任何 Display 函数。它只发送事件，由 `APP_Event_Process` 统一转发。

**对比：**

| 传统方式 | 事件方式 |
|---------|---------|
| `APP_Sensor_Update()` 直接调用 `APP_Display_Update()` | `APP_Sensor_Update()` 发送 Event，`APP_Event_Process()` 调用 `APP_Display_Update()` |
| Sensor 依赖 Display 头文件 | Sensor 只依赖 `app_event.h` |
| 新增消费者需修改 Sensor 代码 | 新增消费者只需在 `APP_Event_Process` 中增加 case |

## 方便扩展

增加新模块响应事件只需两步：

1. 增加 Event 类型和 ID（或复用现有类型）
2. 在 `APP_Event_Process()` 的 switch 中增加 case

不需要修改事件生产者（Sensor / Config / Protocol）的代码。

## 适合 FreeRTOS 迁移

当前架构天然兼容 RTOS 的消息传递模型：

```
当前:  eventQueue[8] + while 轮询
未来:  FreeRTOS Queue + Task 阻塞等待
```

事件的生产者/消费者接口（`Post` / `Get`）可以无缝替换为 FreeRTOS Queue API。

---

# 14. 容易出现的问题

## 14.1 Event 忘记初始化

**现象**：系统启动后没有事件被处理，所有 `APP_Event_Post()` 返回 `APP_EVENT_OK` 但 `APP_Event_Process()` 取不到事件。

**原因**：`APP_Event_Init()` 未被调用，`head`/`tail`/`count` 处于未初始化状态。

**当前状态**：`APP_Event_Init()` 在 `APP_Init()` 中**第一个调用**（其他模块初始化前），确保最早就绪。

## 14.2 Event 参数错误

**现象**：`type` 正确但 `id` 错误，事件被 switch 命中但 if 判断不匹配，静默丢弃。

**示例**：发送 `{type=CONFIG, id=999}` → `APP_EVENT_CONFIG` case 命中，但 `999` 不等于 `CHANGED` 也不等于 `SAVE_DONE`，if 全部跳过，事件丢失。

**预防**：生产端使用枚举值而非魔数，消费者 switch 中增加 `default` 分支。

## 14.3 Event 队列满

**原因**：事件产生速度超过处理速度（当前队列仅 8 槽位）。

```c
// [APP/app_event.c:34-39]
if (count >= APP_EVENT_QUEUE_SIZE)
{
    lostEvent++;              // 累计丢失计数
    return APP_EVENT_ERROR;   // 本次投递失败
}
```

**当前缓解措施**：
- `lostEvent` 计数器记录丢失次数（可通过 `APP_Event_GetLostCount()` 查询）
- `APP_Event_Process()` 在主循环中每次迭代处理**全部**待处理事件（while 循环直到队列空）
- Sensor 更新做了数据变化检测，不会无意义地刷事件

## 14.4 在事件处理中执行耗时操作

**错误做法**：

```
APP_Event_Process()
    ↓
Flash 擦写（1~2 秒阻塞）
```

**正确做法**（当前实现）：

```
APP_Event_Process()
    ↓
case CHANGED: 只修改 Timer 周期（微秒级）
case SAVE_DONE: 只打印日志（微秒级）

Flash 操作在 APP_Config_Process() 中独立处理
```

---

# 15. FreeRTOS 迁移注意事项

**当前架构：**

```
main loop
    │
    └─ APP_Event_Process()
         └─ while (APP_Event_Get(&event) == OK)
              └─ switch ... 处理
```

**迁移后架构：**

```
Event Producers (ISR / Task)
    │
    │  APP_Event_Post() → xQueueSend() / xQueueSendFromISR()
    ▼
FreeRTOS Queue (替代 eventQueue[8])
    │
    ▼
EventTask (独立任务)
    │
    │  xQueueReceive() (阻塞等待，无事件时挂起)
    ▼
switch ... 处理
```

**替换对照：**

| 当前（裸机） | 迁移后（FreeRTOS） | 说明 |
|------------|-------------------|------|
| `eventQueue[8]` | `xQueueCreate(8, sizeof(APP_Event_t))` | 队列容量不变 |
| `APP_Event_Post()` | `xQueueSend()` | Task 上下文发送 |
| `APP_Event_Post()` (ISR 中) | `xQueueSendFromISR()` | ISR 上下文必须用 FromISR 版本 |
| `APP_Event_Get()` | `xQueueReceive()` | 阻塞等待，无需 while 轮询 |
| `APP_Event_IsEmpty()` | `uxQueueMessagesWaiting() == 0` | 查询队列状态 |
| `lostEvent` | Queue 满时 `xQueueSend` 返回 `errQUEUE_FULL` | 可选择性记录 |

**注意事项**：

- ISR 中发事件必须使用 `xQueueSendFromISR()`，不能使用普通 `xQueueSend()`
- `APP_Event_Process()` 的 while 轮询改为 Task 阻塞等待，解放 CPU
- `APP_Event_t` 结构体可直接复用，无需修改
- 事件类型枚举和 ID 枚举**完全保持不变**

---

# 16. 总结

当前 APP_Event 事件系统已经实现：

- ✅ 统一事件队列（8 槽位 FIFO，head/tail + count 管理）
- ✅ Sensor 事件（DHT11 数据变化 → 显示刷新）
- ✅ Config 事件（参数变更 → Timer 周期更新 + Flash 保存完成 → 调试日志）
- ✅ Display 事件（蓝牙 PAGE 命令 → 页面切换 + 刷新）
- ✅ System 事件（启动完成 → 首次显示渲染）
- ✅ 参数传递（`event.param` 承载新间隔值 / 页面枚举 / Flash 地址）
- ✅ 队列满保护（`lostEvent` 丢失计数）
- ✅ 统一事件处理（`APP_Event_Process()` switch 分发）

**当前已知问题：**

- ⚠️ `APP_Cmd_Page()` 中 `APP_Event_Post()` 被重复调用两次（app_protocol.c:146 和 152）
- ⚠️ `eventPostCount` 和 `eventProcessCount` 变量已声明但从未递增
- ⚠️ `APP_Event_GetPostCount()` 和 `APP_Event_GetProcessCount()` 在头文件中声明但未实现
- ⚠️ `APP_EVENT_PROTOCOL` 枚举已定义但未使用
- ⚠️ `APP_CONFIG_EVENT_REPAIR_DONE` 枚举已定义但未使用
- ⚠️ 全局变量 `APP_Event_t event;` (app_event.c:5) 未使用

**核心设计思想：**

> **模块产生事件，Event 中心管理，业务模块响应。**

事件系统将模块间的"直接调用"解耦为"事件通知"，为后续 FreeRTOS 迁移提供了天然的 Queue→Task 架构基础。迁移时只需替换底层队列实现，上层事件类型和分发逻辑完全不变。
