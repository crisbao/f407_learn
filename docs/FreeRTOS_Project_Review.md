# FreeRTOS 项目错误复盘与知识地图

> 本文是一次"学习型错误复盘"，不是 Bug 列表。
> 目标：搞清楚"这次开发过程中为什么会反复出现这些问题"，为以后学习 FreeRTOS、嵌入式系统与软件架构建立可复用的判断能力。
>
> **本次复盘只读分析，未修改任何代码。**
> 所有结论均可从当前磁盘源码得到证明（`main.c` / `app_freertos.c` / `app_event.c` / `app_event_handle.c` / `app_sensor.c` / `app_config.c` / `app_protocol.c` / `app_display.c` / `app_timer.c` 等）。

---

# 1. 本次项目总体问题画像

把这一路踩过的坑压缩成最核心的几条，后面全部展开都围绕它们：

1. **裸机思维没有切换到 RTOS 思维** —— 我把"轮询、检查、做事"的裸机习惯，直接翻译成 Task，而不是重新设计"谁等什么、谁被谁唤醒"。
2. **一个功能存在多个调度源** —— DHT11 采样同时被 SensorTask（`vTaskDelayUntil`）和 TimerTask（`APP_TIMER_SENSOR`）驱动，导致双倍采样。
3. **一个事件存在多个处理入口** —— `APP_Event_Process()`（`while + Get`）与 `APP_RTOS_EventHandle()`（switch）两套分发器，其中一套因阻塞设计成为死代码。
4. **"代码写完了"被误当成"功能闭环了"** —— `APP_Config_Process()` 负责落盘，但没有任何任务调用它，于是配置"运行时能改、断电就丢"。
5. **旧架构没有彻底退出，新架构又叠了一层** —— 裸机 `APP_Run` / 软件 `APP_Timer` / 裸机事件循环 与 FreeRTOS Task / Queue 并存，职责重叠、互相干扰。
6. **共享资源没有按"竞争模型"分类处理** —— 该用 Queue 的用了 Queue，但该用 Mutex 的（OLED GRAM / I2C、sensorData）没有加锁。
7. **低层 C / 嵌入式习惯问题** —— float `!=` 比较、栈被砍到 1KB、阻塞式 I2C 被抢占、调试代码长期残留。

一句话总结：**这次真正暴露的不是"FreeRTOS API 不会用"，而是"缺少把业务拆成'谁等什么、谁发什么、谁负责唯一一件事'的底层建模思维"。**

---

# 2. 问题时间线

按项目实际演进阶段回顾，每个阶段"当时在做什么 / 当时埋下了什么雷"。

### 阶段一：裸机（bare-metal v0.1）
- 内容：HAL + CubeMX，主循环 `while(1)` 里跑 `APP_Run()`，软件轮询。
- 状态：功能是闭环的（DHT11 采、OLED 显、蓝牙命令、Flash 存配置）。
- 埋下的雷：**所有业务都长在"主循环轮询"这一个隐含前提上**。这个前提后面迁移时没有被显式推翻。

### 阶段二：APP 模块化
- 内容：把 app 拆成 sensor / display / protocol / config / timer / event / system 等模块。
- 状态：结构清晰，但**每个模块都被设计成"被某个循环周期调用一次"的形态**（`APP_xxx_Process()`）。
- 埋下的雷：模块接口是"过程式轮询函数"，天然适合裸机主循环，不适合被 Task 直接拥有。

### 阶段三：引入 Event（事件系统）
- 内容：8 槽 FIFO 事件队列，`APP_Event_Post` / `APP_Event_Get` / `APP_Event_Process`。
- 状态：解耦了"产生者"和"处理者"。
- 埋下的雷：`APP_Event_Process()` 写成 `while(APP_Event_Get(&e)==OK){...}` —— **这个 while 是"处理完这一批就返回"的裸机语义**。它是后面最大的坑。

### 阶段四：引入 FreeRTOS（内核启动）
- 内容：SysTick 交给 FreeRTOS、TIM6 做 HAL 时基、`vTaskStartScheduler()`、Stack/Heap 调整。
- 状态：内核真正跑起来了。
- 埋下的雷：**只启动了内核，没有重画"任务边界"**。业务函数原样搬进 Task。

### 阶段五：Task 拆分
- 内容：创建 Main / Sensor / Timer / Protocol / Event 5 个 Task。
- 状态：Task 有了，但**很多是"脚手架 + 调试打印"**（MainTask 只打印心跳）。
- 埋下的雷：没有 DisplayTask；SensorTask 与 TimerTask 职责开始重叠；MainTask 该干的 `APP_Config_Process()` 被注释掉。

### 阶段六：Queue 迁移
- 内容：`APP_Event_Post/Get` 底层从自定义 FIFO 换成 `xQueueSend/xQueueReceive`。
- 状态：**这是最关键的转折点**。`APP_Event_Get` 从"非阻塞取一个，没有返回错误"变成了"`portMAX_DELAY` 阻塞等待"。
- 埋下的雷：`APP_Event_Process()` 的 `while(Get==OK)` 语义被悄悄破坏，但没有同步修改 `app_event_handle.c`，也没有删除/合并 `APP_RTOS_EventHandle()`。

### 阶段七：Timer 引入
- 内容：裸机软件定时器 `APP_Timer` 保留，又加了 TimerTask 轮询它。
- 状态：Timer 模块本身完整。
- 埋下的雷：`APP_TIMER_SENSOR` 仍在驱动采样，与 SensorTask 的 `vTaskDelayUntil` 形成**双调度源**。

### 阶段八：Config 引入
- 内容：双备份 Flash + CRC32 + 延迟保存 + 自动修复，非常完整。
- 状态：**读路径完整，写路径断裂**。
- 埋下的雷：`APP_Config_Process()`（延迟保存的驱动者）原本在 `APP_Run()` 里，而 `APP_Run()` 已经不再被调用；MainTask 里它被注释掉。

### 阶段九：当前状态
- 内核跑起来了，主链路（Sensor → Queue → EventTask → OLED）能通。
- 但存在三处硬伤（事件双分发 / 配置不落盘 / 传感器双采样），以及若干 C 层隐患。
- 本质：**裸机旧架构没退干净，RTOS 新架构没完全立起来，两套并存。**

> 规律：**几乎每个"阶段"的正确产出，都为"下一阶段"埋下了"没有重新审视前提"的雷。** 核心不是某一处代码写错，而是每次升级都没有先"重画全局图"。

---

# 3. 错误复盘表

| 问题 | 表面现象 | 根本原因 | 所属知识点 | 当前状态 | 今后如何避免 |
|---|---|---|---|---|---|
| 事件双分发 | 两套事件处理函数，`APP_RTOS_EventHandle` 永远不执行 | `APP_Event_Get` 改阻塞后，`APP_Event_Process` 的 `while` 永不返回；新分发器又单独写了一套 | Queue 阻塞语义 + 单一职责 | 当前未解决 | 迁移 API 语义时同步审查所有调用方；一个事件只有一个处理入口 |
| Config 不落盘 | INTERVAL 运行时生效，断电丢失 | `APP_Config_Process()` 无任何任务调用 | 功能闭环 / 谁驱动后台任务 | 当前未解决 | 每个"后台定期要做的事"必须明确绑定到某个 Task 或 Software Timer |
| Sensor 双采样 | 每周期串口打印两次采样 | SensorTask 与 `APP_TIMER_SENSOR` 同时驱动 `APP_Sensor_Update` | 单一调度源原则 | 当前未解决 | 一个功能只有一个主要调度源；迁移时删掉旧调度 |
| PAGE 双投递 | 同一事件入队两次 | `APP_Event_Post` 被调用两次（一次裸调、一次判断返回值） | 返回值与副作用的区分 | 当前未解决 | 发送函数只调一次；要用返回值就只调一次再判断 |
| `while(Get==OK)` 失效 | 逻辑上应该"取空就返回"，实际卡死 | Get 从非阻塞改成 `portMAX_DELAY` | 阻塞 vs 非阻塞接收 | 当前未解决 | 明确"消费方应该阻塞等，还是轮询取"；两者不能混用 |
| float 变化检测 | 温湿度变化判断用 `!=` | float 精确相等不可靠 | 浮点比较 / 定点化 | 当前未解决（暂稳定） | 传感器用整数/定点，或比较用阈值 `fabs(diff)>eps` |
| OLED 多任务竞争 | 潜在花屏/错乱 | EventTask 更新 GRAM 与 ProtocolTask 执行 `OLED CLEAR` 无锁 | 互斥 / 临界区 | 当前未解决 | 共享外设/缓冲区必须加 Mutex 或集中到单一任务 |
| sensorData 无保护 | 多任务读写同一结构体 | 未识别出"跨任务共享状态" | 共享数据竞争 | 当前未解决 | 用 Mutex 或"数据归一个任务所有、其余只读快照" |
| Stack 砍到 1KB | 潜在栈溢出 | 迁移时盲目缩小 Stack_Size | 栈大小评估 | 当前未解决 | 用 `uxTaskGetStackHighWaterMark` 实测，再定栈大小 |
| 任务创建失败只打印 | 失败仍继续启动 | 缺少失败处理策略 | 错误处理 / 防御式编程 | 当前未解决 | 创建失败要有明确降级或停机策略 |
| 调试代码残留 | 大量 `USART_Printf` / 注释块 | 调试与业务代码未分离 | 工程化习惯 | 当前未解决 | 用日志宏统一管理，可编译期开关 |
| `Mode: BARE` 文案过时 | STATUS 显示仍为 BARE | 迁移后未同步 UI 文案 | 细节一致性 | 当前未解决 | 状态字符串与状态定义集中管理 |

---

# 4. 最重要的架构性错误

这一节不讲"哪行代码错"，讲背后的软件工程原则。

### 4.1 裸机思维没有完全切换到 RTOS 思维

裸机程序的本质是：**"一个 while(1)，周期性地把所有事都检查一遍、做一遍。"**
FreeRTOS 程序的本质是：**"多个独立执行流，每个执行流阻塞等待它关心的信号，被唤醒后只做它负责的那一件事。"**

我把裸机习惯直接搬进 Task 的表现：
- MainTask 里只有 `USART_Printf` + `vTaskDelay(1000)` —— 这是"为了有个 Task 而有个 Task"，不是"这个 Task 在等什么、做什么"。
- TimerTask 用 10ms 轮询去驱动一个本可以靠 `vTaskDelayUntil` 的周期任务。
- 没有 DisplayTask，显示逻辑寄生在 EventTask 的事件分发里。

**判断标准**：问自己"这个 Task 的 `for(;;)` 里，哪一行是'阻塞等待'，哪一行是'被唤醒后要做的事'？" 如果答不上来，说明它还停留在轮询。

### 4.2 一个功能多个调度源

DHT11 采样这个"功能"，被两个调度源驱动：
- SensorTask：`vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(APP_Config_GetSensorInterval()))`
- TimerTask：`APP_Timer_Process()` → 触发 `APP_TIMER_SENSOR` 回调（回调即 `APP_Sensor_Update`）

**软件工程原则：一个功能只能有一个"权威调度源"（single source of truth for scheduling）。** 多个调度源并存，意味着"这个功能多久执行一次"这件事没有唯一答案，必然产生双倍/漏/竞争。迁移到 RTOS 时，旧调度源必须被显式退役，而不是"新加一个、旧的也留着"。

### 4.3 一个事件多个处理入口

事件处理出现两套：
- `APP_Event_Process()`（`app_event_handle.c`）：`while(Get) + switch`
- `APP_RTOS_EventHandle()`（`app_freertos.c`）：switch

**软件工程原则：一个事件（消息）只能有一个消费者，一个消费者只能有一个处理函数。** 两套入口的根因是"迁移时没有删掉旧入口，又在别处新写了一个入口"，本质是**没有把"事件分发"当作一个需要明确所有权的资源**。事件分发器应该是一个被唯一认领的组件。

### 4.4 业务逻辑与任务调度职责混杂

`APP_Event_Process()` 既"取事件"（调度/传输职责）又"根据事件调业务"（业务分发职责）。当底层从 FIFO 换成 Queue、阻塞语义改变后，这个函数的两重职责一起失效了，导致很难定位。

**软件工程原则：传输层（Queue/怎么拿）与业务层（拿到后干什么）要分开。** 这样底层换实现（FIFO→Queue→Notification）时，业务逻辑不动。

### 4.5 旧架构和新架构并存

现在工程里同时存在：
- 裸机 `APP_Run()`（不再调用，但函数还在）
- 软件定时器 `APP_Timer`（仍被 TimerTask 轮询）
- 裸机事件循环 `APP_Event_Process`（语义已破）
- FreeRTOS Task / Queue 新机制

**软件工程原则：迁移是"替换"，不是"叠加"。** 每引入一种新机制，就要回答"旧的等价机制是否应该退役、由谁负责删除"。注释掉的旧代码、无人调用的旧函数、被绕过的旧路径，都会成为"看似无害、实则持续误导"的技术债。

---

# 5. FreeRTOS 知识漏洞地图

按"这次实际犯过的错误"来分级，而不是照搬教材目录。

### A. 已经通过项目实际验证 ✅
- **Task 创建**（`xTaskCreate` 参数、栈、优先级）—— 5 个任务确实跑起来了。
- **Scheduler 启动**（`vTaskStartScheduler`）—— 内核确实在跑。
- **Queue 基本收发**（`xQueueCreate` / `xQueueSend` / `xQueueReceive`）—— 事件队列在跑。
- **vTaskDelay / vTaskDelayUntil** —— SensorTask 的 `vTaskDelayUntil` 用法正确（无累积漂移）。
- **SysTick 交给 FreeRTOS + TIM6 做 HAL 时基** —— 移植套路走通了。

### B. 代码会写但理解还不够 ⚠️
- **Blocking（阻塞）**：写了 `portMAX_DELAY`，但不理解"阻塞会改变整个函数的返回语义"，才酿成 `while + Get` 失效。**这是本项目的头号知识漏洞。**
- **Task 状态（Ready/Running/Blocked）**：能写阻塞，但没意识到"阻塞 = 让出 CPU = 函数停在半路不返回"，所以没预判 `APP_Event_Process` 会卡住。
- **队列非阻塞 vs 阻塞接收的区别**：把两种语义混在一套 `APP_Event_Get` 里，没分清"消费方应阻塞等，还是轮询取"。

### C. 当前明显薄弱 ❌
- **Mutex（互斥）**：OLED GRAM / I2C、sensorData 这些共享资源没有保护，暴露了对"竞争模型"不敏感。
- **单一职责 / 所有权**：调度源不唯一、事件入口不唯一，暴露了"谁拥有什么资源"的建模能力弱。
- **ISR 与任务边界**：知道 ISR 现在没用 FromISR API，但不清楚"什么时候必须用、优先级为什么要调整"。
- **栈与内存评估**：直接砍 Stack_Size，没做过 `uxTaskGetStackHighWaterMark` 实测。

### D. 下一阶段必须学习 🎯
- **Mutex / 临界区** —— 用来解决当前 OLED / sensorData 竞争。
- **Task Notification** —— 用"通知"替代一部分"队列传事件"，理解"轻量同步"。
- **Software Timer** —— 替换裸机 `APP_Timer`，理解 Timer Service Task 的职责。
- **ISR → FromISR API** —— 理解"中断里不能阻塞、要发信号给任务"。
- **Binary Semaphore** —— 作为"ISR 唤醒任务"的最小同步原语。

### E. 暂时不需要学习 ⏸
- **EventGroup**：本项目没有"多事件组合等待"的需求，跳过（不要为学而学）。
- **Message Buffer / Stream Buffer**：没有流式数据需求，跳过。
- **Memory Pool（内存池）**：没有频繁小对象分配需求，跳过。
- **Co-routine**：已被 FreeRTOS 官方边缘化，不学。

> 分级的核心判断依据：**"这次踩坑踩到哪，知识就缺到哪"**。A 是踩过坑后验证过的；B 是踩了坑但还没真正懂的；C 是坑已经暴露但还没处理的；D 是解决当前 C 所必需的下一块拼图。

---

# 6. "错误 → 知识点 → 实验"

每个重要错误配一个**最小实验**（不修改当前大型项目，单独建一个小工程或用一个测试目标验证）。

### 实验 1：Queue Blocking 的语义

**错误**：`APP_Event_Get` 用 `portMAX_DELAY` 后，`while(Get==OK)` 逻辑失效。

**知识点**：Queue Blocking / Task 的 Blocked 状态。

**实验**：
1. 建两个 Task：Task A 每 2 秒 `xQueueSend` 一条消息；Task B `xQueueReceive(portMAX_DELAY)`。
2. 在 Task B 收到前、收到后各打一条日志，用 `vTaskList` 或断点观察 Task B 状态。
3. 观察：**无消息时 Task B 进入 Blocked，`xQueueReceive` 不返回；消息到达后立刻返回**。
4. 关键领悟：`xQueueReceive` 会"停在半路"，所以"收到后继续 while 再收"只能写在循环体里、不能期待它"取空返回"。

### 实验 2：非阻塞 vs 阻塞接收

**错误**：`while(Get==OK)` 的写法来自"非阻塞取空返回"的旧语义。

**知识点**：`portMAX_DELAY` / 0 / 固定 timeout 三种接收模式。

**实验**：
1. 同一 Queue，分别用 `xQueueReceive(q, &item, 0)` 和 `xQueueReceive(q, &item, portMAX_DELAY)` 接收。
2. 空队列时：timeout=0 立刻返回 `errQUEUE_EMPTY`；`portMAX_DELAY` 卡住。
3. 领悟：**"轮询消费"用 0，"事件等待"用 portMAX_DELAY，"超时控制"用固定值——三种模式不能混用。**

### 实验 3：单一调度源

**错误**：SensorTask 与 TimerTask 同时采样 DHT11。

**知识点**：任务职责划分 / 单一调度源。

**实验**：
1. 写一个最简单的周期任务：`for(;;){ do_work(); vTaskDelayUntil(&lastWake, period); }`。
2. 再开一个 Software Timer 也调用 `do_work()`。
3. 对比日志：周期任务串口打印一次，加 Timer 后打印两次。
4. 领悟：**"多久做一次"只能有一个答案；两个调度源 = 两个周期。**

### 实验 4：配置持久化闭环

**错误**：`APP_Config_Process` 无人调用，配置不落盘。

**知识点**：后台任务 / 功能闭环（输入 → 处理 → 输出 → 可观察结果）。

**实验**：
1. 建一个 Task，每 100ms 检查"脏标志"，脏则写 Flash。
2. 改一个配置值 → 观察脏标志置位 → 等后台任务写盘 → 断电重启 → 读回值是否还在。
3. 领悟：**"写了 Save 函数"≠"Save 会被执行"；要画一条完整的"触发→执行→落盘→回读验证"闭环。**

### 实验 5：Mutex 保护共享资源

**错误**：OLED GRAM / sensorData 多任务无锁。

**知识点**：Mutex / 临界区 / 数据竞争。

**实验**：
1. 两个 Task 同时往一个全局数组写不同内容（无锁），打印结果观察错乱。
2. 加 `xSemaphoreCreateMutex` 后，写前后 `xSemaphoreTake/Give`，观察不再错乱。
3. 领悟：**Queue 解决"传数据"，Mutex 解决"保护同一块数据"——两者不是一回事。**

### 实验 6：ISR → 任务（FromISR）

**错误**：USART3 ISR 现在只用 RingBuffer，但未来要直连 Queue 会踩优先级坑。

**知识点**：`xQueueSendFromISR` / 中断优先级 / `configMAX_SYSCALL_INTERRUPT_PRIORITY`。

**实验**：
1. 在 ISR 里 `xQueueSendFromISR` + `portYIELD_FROM_ISR`。
2. 确认该中断优先级**数值上 ≥ configMAX_SYSCALL_INTERRUPT_PRIORITY**（F4 上数值越大优先级越低）。
3. 观察：ISR 收到数据 → 唤醒阻塞的任务。
4. 领悟：**ISR 不能阻塞等待，只能"发一个信号 + 立即返回"，由任务去做重活。**

---

# 7. 裸机思维 vs FreeRTOS 思维

| 裸机 | FreeRTOS | 我这次在哪里混用了 |
|---|---|---|
| `while(1)` 轮询所有事 | 多个 Task，各阻塞等自己的信号 | MainTask 仍是"空转打印"，没转成"等某事" |
| 软件轮询 Timer | `vTaskDelayUntil` / Software Timer | TimerTask 轮询 `APP_Timer`，与 SensorTask 周期重复 |
| "检查有没有事件"（非阻塞取，取空返回） | Queue Blocking（没事件就睡） | `APP_Event_Get` 改阻塞，但 `while` 还是非阻塞旧写法 |
| 全局变量通信 | Queue / Mutex / Notification | sensorData / appConfig / OLED GRAM 仍当全局变量用，无锁 |
| 中断置标志位，主循环查标志 | ISR + FromISR 唤醒任务 | USART3 ISR 还在手动 RingBuffer，没转 FromISR |
| 主循环统一调度 | Scheduler 按优先级/阻塞调度 | 业务逻辑仍依赖"被某个循环调用"的旧假设 |
| `HAL_Delay` 忙等 | `vTaskDelay` 让出 CPU | 部分路径仍残留 HAL_Delay 式思路（阻塞 I2C 等） |
| 一个函数周期性被调 | 一个 Task 拥有一个周期 | SensorTask 与 TimerTask 抢采样周期 |

**我混用两种思维的核心证据**：事件队列明明已经换成了 FreeRTOS Queue（新思维），但消费它的 `while` 还是裸机"取空返回"（旧思维）；采样周期明明已经有了 `vTaskDelayUntil`（新思维），但还留着裸机软件定时器（旧思维）。**迁移时只换了"容器"，没换"使用容器的思维方式"。**

---

# 8. 最终学习路线

按"本次暴露的问题"排序，而不是按教材目录。

### 第 1 步：先补 Blocking / Task 状态（当前最缺的底层理解）
- **为什么现在学**：事件双分发、`while+Get` 失效的根子就是不懂"阻塞会让函数停在半路不返回"。
- **学完应能解释**：为什么 `xQueueReceive(portMAX_DELAY)` 后的 while 逻辑会失效；Task 的 Ready/Running/Blocked 三态转换。
- **用哪个问题验证**：实验 1、实验 2（§6）。
- **进入下一阶段条件**：能独立画出"生产者→Queue→阻塞消费者"的状态图，并解释阻塞点。

### 第 2 步：修好事件系统（单一入口 + 阻塞消费）
- **为什么现在学**：这是当前最严重的架构错误，也是理解"单一职责"的最佳载体。
- **学完应能解释**：为什么一个事件只能有一个消费者/入口；消费方应阻塞等一次取一个。
- **用哪个问题验证**：`APP_Event_Process` vs `APP_RTOS_EventHandle` 二选一，删掉死代码。
- **进入下一阶段条件**：事件链路变成"Queue → 一个阻塞任务 → 一个 switch"，无死代码。

### 第 3 步：接回 Config 持久化（功能闭环）
- **为什么现在学**：理解"写了函数≠功能完成"，学会画完整闭环。
- **学完应能解释**：一个配置从"收到命令"到"断电重启后仍存在"要经过哪几环。
- **用哪个问题验证**：实验 4（§6），让 `APP_Config_Process` 被某个 Task 周期驱动，断电验证。
- **进入下一阶段条件**：能断电保存 INTERVAL 并重启回读成功。

### 第 4 步：学 Mutex / 临界区（解决竞争）
- **为什么现在学**：OLED / sensorData 竞争是下一个真实风险，且 Mutex 是同步三件套里最基础的一个。
- **学完应能解释**：Queue 传数据 vs Mutex 保护数据 的区别；Take/Give 成对、死锁风险。
- **用哪个问题验证**：实验 5（§6），给 OLED 更新 / sensorData 访问加锁。
- **进入下一阶段条件**：能说明"为什么 Queue 解决不了 OLED 竞争、必须 Mutex"。

### 第 5 步：学 Task Notification / Semaphore（轻量同步）
- **为什么现在学**：很多"发一个信号"的场景不需要传数据，用 Notification 更轻，能进一步精简事件系统。
- **学完应能解释**：Notification 与 Queue 的取舍；Binary Semaphore 如何做"ISR 唤醒任务"。
- **用哪个问题验证**：把"配置保存完成"或"采样完成"用 Notification 通知，替代部分事件。
- **进入下一阶段条件**：能选出"该用 Queue 还是 Notification"并说明理由。

### 第 6 步：Software Timer + ISR FromISR（替代旧机制）
- **为什么现在学**：裸机 `APP_Timer` 和手动 RingBuffer 是最后两块旧机制，学完才能彻底退役。
- **学完应能解释**：Software Timer 的 Service Task 职责；ISR 里为何只能 FromISR + 优先级约束。
- **用哪个问题验证**：实验 6（§6），USART3 直连 Queue；用 Software Timer 替换 `APP_TIMER_SENSOR`。
- **进入下一阶段条件**：工程里不再有裸机软件定时器和手动 RingBuffer 承载业务同步。

> 这条路线每一步都"踩在本次的一个真实错误上"，学一步、修一步、验证一步，不跳步。

---

# 9. 以后写 FreeRTOS 项目前的检查清单

- [ ] 一个功能只有一个主要调度源（谁决定"多久做一次"是唯一的）
- [ ] 一个事件/消息只有一个明确的处理入口（没有第二套 switch）
- [ ] 明确每个 Queue 的接收方：阻塞等（portMAX_DELAY）还是轮询取（0/timeout）
- [ ] 明确每个 Task 的 `for(;;)` 里：哪行是"阻塞等待"，哪行是"被唤醒后做的事"
- [ ] 明确每个共享资源的保护方式：Queue 传数据 / Mutex 保护数据 / Notification 发信号
- [ ] 明确每个 ISR：是否调用 FreeRTOS API？是否用了正确的 FromISR 版本？
- [ ] 确认中断优先级与 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 的关系
- [ ] 每个"配置修改"是否真正落盘，并有断电重启回读验证
- [ ] 每个功能是否有完整"输入 → 处理 → 输出 → 可观察结果"闭环
- [ ] 是否存在裸机旧代码与 RTOS 新代码并存、旧调度源未退役
- [ ] 是否有注释掉的旧代码 / 无人调用的旧函数残留
- [ ] 栈大小是否用 `uxTaskGetStackHighWaterMark` 实测过，而非拍脑袋
- [ ] 传感器等数值是否避免 float 直接 `==`/`!=` 比较
- [ ] 是否真的通过运行日志/断电重启验证过，而不仅是编译通过

---

# 总结：这次项目真正暴露的是什么

这次项目真正暴露的，**不是"有哪些 Bug"，而是我在学习 FreeRTOS 时缺少的几层底层思维：**

**第一，缺少"执行流"思维。**
我一直把代码理解成"函数按顺序被调用"，而 FreeRTOS 要求理解成"多个执行流并发推进、随时可能被打断、会停在阻塞点不返回"。正因为缺这一层，我才会写出 `while + portMAX_DELAY` 这种"把阻塞 API 当非阻塞 API 用"的错误——我没意识到一个函数可以在半路"停住"。

**第二，缺少"所有权"思维。**
调度源不唯一、事件入口不唯一，本质是我没有为"每个功能、每个资源"明确一个唯一的 owner。FreeRTOS 的建模，第一步永远是"谁拥有这个资源、谁负责这件事"，而不是"我有哪些函数"。

**第三，缺少"闭环验证"思维。**
"写了 Save 函数"被我当成"实现了持久化"，暴露出我习惯用"代码存在"代替"功能闭环"。真正的实现必须能回答"从触发到结果，每一环都被某个执行流调用，并且能观察到结果"。

**第四，缺少"替换而非叠加"的迁移思维。**
每次升级我都在"加新机制"而不是"退役旧机制"，导致裸机与 RTOS 两套体系并存。健康的迁移是：每引入一个新东西，就明确"旧的哪个等价物该退役、由谁删除"。

这四层思维，比任何单个 FreeRTOS API 都更重要。**API 只是工具，这四层才是决定代码是"能跑"还是"正确"的分水岭。** 以后每写一个 FreeRTOS 项目，先问四个问题：

1. 这条执行流在哪里阻塞、被谁唤醒？
2. 这个功能/资源归谁所有？
3. 这个功能从触发到结果是否闭环、能否验证？
4. 我新引入的机制，是否已经让旧的等价机制退役？

把这四个问题答清楚了，这次踩的绝大多数坑都不会再出现。
