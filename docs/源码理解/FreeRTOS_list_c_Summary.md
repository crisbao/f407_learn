# FreeRTOS list.c / list.h 源码解读

> **源码文件**: `FreeRTOS/Source/list.c` + `FreeRTOS/Source/include/list.h`
> **版本**: FreeRTOS Kernel V10.5.1 (FreeRTOSv202212.01)
> **地位**: 这是 FreeRTOS 调度器**唯一使用的"容器数据结构"**。就绪任务链表、延时任务链表、挂起任务链表——全部用这个双向链表实现。

---

## 一、为什么要自己写链表？

C 标准库没有链表，STL 是 C++ 的。更关键的是，FreeRTOS 的链表有**嵌入式实时系统的特殊需求**：

| 通用链表 | FreeRTOS 链表 |
|----------|---------------|
| 链表拥有数据 (list contains data) | 数据拥有链表节点 (TCB contains ListItem) |
| 内存分散，需要额外分配 list node | 链表节点内嵌在 TCB 中，无需额外 malloc |
| 通用排序 key | xItemValue 可以是优先级、tick 计数值 —— 一个字段，多种语义 |

这就是注释里说的：

> *"There is effectively a two way link between the object containing the list item and the list item itself."*

通过 `pvOwner`（ListItem → 拥有者）和 `pxContainer`（ListItem → 所在的链表），实现了**双向关联**。

---

## 二、核心数据结构

### 2.1 `ListItem_t` — 链表节点 (list.h:144-154)

```
┌──────────────────────────────────────────────────┐
│                  ListItem_t                       │
├──────────────────────────────────────────────────┤
│  xItemValue    │  排序用的键值                     │
│                │  可以是: 优先级 / tick超时时间     │
├──────────────────────────────────────────────────┤
│  pxNext ──────→ 下一个 ListItem                   │
│  pxPrevious ──→ 上一个 ListItem                   │
├──────────────────────────────────────────────────┤
│  pvOwner ─────→ 指向"拥有此节点的对象"             │
│                │  通常是 TCB（任务控制块）          │
├──────────────────────────────────────────────────┤
│  pxContainer ─→ 指向"我所在的链表"                │
│                │  NULL = 当前不在任何链表中         │
└──────────────────────────────────────────────────┘
```

**关键设计意图**：
- `pvOwner` — 从链表节点反查"这是哪个任务的节点"
- `pxContainer` — 节点自己知道自己在哪个链表里，这样 `uxListRemove()` **只需要传入节点指针**，不用传链表指针！

```c
struct xLIST_ITEM
{
    listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE           // 完整性校验字段（可选）
    configLIST_VOLATILE TickType_t xItemValue;          // 排序用的键值
    struct xLIST_ITEM * configLIST_VOLATILE pxNext;     // 指向下一个节点
    struct xLIST_ITEM * configLIST_VOLATILE pxPrevious; // 指向上一个节点
    void * pvOwner;                                     // 指向拥有此节点的对象（通常是 TCB）
    struct xLIST * configLIST_VOLATILE pxContainer;     // 指向此节点所在的链表
    listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE          // 完整性校验字段（可选）
};
typedef struct xLIST_ITEM ListItem_t;
```

### 2.2 `MiniListItem_t` — 精简版节点 (list.h:157-164)

```c
struct xMINI_LIST_ITEM
{
    listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE
    configLIST_VOLATILE TickType_t xItemValue;
    struct xLIST_ITEM * configLIST_VOLATILE pxNext;
    struct xLIST_ITEM * configLIST_VOLATILE pxPrevious;
};
typedef struct xMINI_LIST_ITEM MiniListItem_t;
```

**为什么存在？** 每个链表都有一个"**末尾标记节点**"（`xListEnd`）。这个标记节点不需要 `pvOwner` 和 `pxContainer`，用 `MiniListItem_t` 可以**省 RAM**。ARM Cortex-M 上一个指针 4 字节，省两个就是 8 字节。系统里有很多链表（就绪链表就有多个），积少成多。

> `configUSE_MINI_LIST_ITEM` 默认是 1（启用优化）。设为 0 时 `MiniListItem_t` 退化为完整 `ListItem_t`。

### 2.3 `List_t` — 链表本身 (list.h:172-179)

```
┌───────────────────────────────────────────────────┐
│                    List_t                          │
├───────────────────────────────────────────────────┤
│  uxNumberOfItems │  当前链表中有多少个节点          │
│                  │  (不含 xListEnd 标记)            │
├───────────────────────────────────────────────────┤
│  pxIndex ───────→  "游标"，用于遍历链表             │
│                  │  记录上次访问到哪了               │
├───────────────────────────────────────────────────┤
│  xListEnd        │  末尾标记节点 (MiniListItem_t)    │
│                  │  xItemValue = portMAX_DELAY      │
│                  │  (始终是链表中的最大值)            │
└───────────────────────────────────────────────────┘
```

```c
typedef struct xLIST
{
    listFIRST_LIST_INTEGRITY_CHECK_VALUE
    volatile UBaseType_t uxNumberOfItems;
    ListItem_t * configLIST_VOLATILE pxIndex;  // 游标，用于遍历链表
    MiniListItem_t xListEnd;                   // 末尾标记节点（哨兵）
    listSECOND_LIST_INTEGRITY_CHECK_VALUE
} List_t;
```

---

## 三、核心设计：带哨兵的双向循环链表

```
                    ┌──────────────────────────────────────────────┐
                    │           List_t (链表)                       │
                    │                                              │
                    │   pxIndex ──┐                                │
                    │              │                                │
                    │   ┌──────────▼──────────────────────────┐    │
                    │   │         xListEnd (哨兵节点)           │    │
                    │   │    xItemValue = portMAX_DELAY (最大) │    │
                    │   │    pxNext ──────────────┐            │    │
                    │   │    pxPrevious ────────┐ │            │    │
                    │   └───────────────────────┼─┼────────────┘    │
                    │                           │ │                 │
                    └───────────────────────────┼─┼─────────────────┘
                                                │ │
                        ┌───────────────────────┘ └──────────────────────┐
                        ▼                                                ▼
              ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
              │   ListItem A    │      │   ListItem B    │      │   ListItem C    │
              │ xItemValue = 5  │      │ xItemValue = 10 │      │ xItemValue = 15 │
              │ pvOwner → TCB1  │←────→│ pvOwner → TCB2  │←────→│ pvOwner → TCB3  │
              │ pxContainer→List│      │ pxContainer→List│      │ pxContainer→List│
              └─────────────────┘      └─────────────────┘      └─────────────────┘
```

**关键特性**：

1. **循环链表** — 最后一个节点的 `pxNext` 指回 `xListEnd`，`xListEnd` 的 `pxNext` 指向第一个真实节点
2. **哨兵节点 `xListEnd`** — 它的 `xItemValue = portMAX_DELAY`（最大值），保证它**始终在链表末尾**
3. **升序排列** — `vListInsert()` 按 `xItemValue` 从小到大插入。同值节点**后插入的排在后面**，保证同优先级任务轮转调度（Round Robin）

---

## 四、函数逐个解析

### 4.1 `vListInitialise()` — 初始化链表 (list.c:50-83)

```c
void vListInitialise( List_t * const pxList )
{
    // ① pxIndex 指向 xListEnd
    pxList->pxIndex = (ListItem_t *) &(pxList->xListEnd);

    // ② 哨兵节点的值是"无穷大"
    pxList->xListEnd.xItemValue = portMAX_DELAY;

    // ③ 空链表：哨兵节点自己指向自己（形成闭环）
    pxList->xListEnd.pxNext     = (ListItem_t *) &(pxList->xListEnd);
    pxList->xListEnd.pxPrevious = (ListItem_t *) &(pxList->xListEnd);

    // ④ 节点计数清零
    pxList->uxNumberOfItems = (UBaseType_t) 0U;
}
```

**初始状态图示**（空链表）：

```
        ┌────────────────────────┐
        │  List_t                │
        │  uxNumberOfItems = 0   │
        │  pxIndex ──┐           │
        │  ┌─────────▼─────────┐ │
        │  │     xListEnd      │ │
        │  │  pxNext ────┐     │ │
        │  │  pxPrevious ┘     │ │    ← 自己指自己，形成闭环
        │  │  xItemValue=MAX  │ │
        │  └──────────────────┘ │
        └────────────────────────┘
```

### 4.2 `vListInitialiseItem()` — 初始化链表节点 (list.c:86-95)

```c
void vListInitialiseItem( ListItem_t * const pxItem )
{
    // 标记为"不在任何链表中"
    pxItem->pxContainer = NULL;

    // 设置完整性校验值（如果启用）
    listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
    listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem );
}
```

### 4.3 `vListInsert()` — 按值升序插入 (list.c:128-196)

**这是最重要的插入函数**，调度器用它将任务按优先级（或超时 tick）插入链表。

```c
void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t * pxIterator;
    const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    // ★ 特殊情况: 如果插入值 == portMAX_DELAY
    //   直接放到哨兵前面（即链表最末尾），避免下面的 for 循环死循环
    if( xValueOfInsertion == portMAX_DELAY )
    {
        pxIterator = pxList->xListEnd.pxPrevious;  // 从最后一个真实节点开始
    }
    else
    {
        // ★ 标准情况: 从哨兵开始，找到第一个 "下一个节点的值 > 插入值" 的位置
        for( pxIterator = (ListItem_t *) &(pxList->xListEnd);
             pxIterator->pxNext->xItemValue <= xValueOfInsertion;
             pxIterator = pxIterator->pxNext )
        {
            // 空循环体，只管遍历
        }
    }

    // ★ 标准双向链表插入操作（4 步）
    pxNewListItem->pxNext = pxIterator->pxNext;           // ① 新节点的 next
    pxNewListItem->pxNext->pxPrevious = pxNewListItem;    // ② 后继节点的 prev
    pxNewListItem->pxPrevious = pxIterator;               // ③ 新节点的 prev
    pxIterator->pxNext = pxNewListItem;                   // ④ 前驱节点的 next

    pxNewListItem->pxContainer = pxList;                  // 记住自己在哪个链表
    (pxList->uxNumberOfItems)++;
}
```

**插入算法示意**：假设链表中有值 5→10→15→哨兵(MAX)，现在插入值 12：

```
遍历过程:
  pxIterator = 哨兵
  → 哨兵.pxNext.xItemValue = 5 <= 12? 是，继续
  → 5.pxNext.xItemValue = 10 <= 12? 是，继续
  → 10.pxNext.xItemValue = 15 <= 12? 否，停止！

  所以 pxIterator 停在节点 10，新节点插入到 10 和 15 之间：

  之前: 哨兵 ←→ 5 ←→ 10 ←→ 15 ←→ 哨兵
                              ↑
                         pxIterator

  之后: 哨兵 ←→ 5 ←→ 10 ←→ [12] ←→ 15 ←→ 哨兵
```

> **⚠️ 关键细节**：比较使用的是 `<=`，即遇到**相同值**时会**继续往后找**。这意味着**后插入的同值节点排在后面**。这是实现**同优先级任务时间片轮转（Round Robin）**的基础！

### 4.4 `vListInsertEnd()` — 插到"末尾" (list.c:98-125)

注意：它**不是**插到链表物理末尾，而是插到 `pxIndex` 指向位置的前面：

```c
void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t * const pxIndex = pxList->pxIndex;  // 获取当前游标位置

    // 新节点插入到 pxIndex 之前
    pxNewListItem->pxNext = pxIndex;
    pxNewListItem->pxPrevious = pxIndex->pxPrevious;

    pxIndex->pxPrevious->pxNext = pxNewListItem;
    pxIndex->pxPrevious = pxNewListItem;

    pxNewListItem->pxContainer = pxList;
    (pxList->uxNumberOfItems)++;
}
```

**为什么需要这个函数？** 注释原文解释：

> *"Placing an item using vListInsertEnd effectively places the item in the list position pointed to by pxIndex. This means that every other item within the list will be returned by listGET_OWNER_OF_NEXT_ENTRY before the pxIndex again points to the item being inserted."*

用白话解释：当用 `listGET_OWNER_OF_NEXT_ENTRY` 轮询链表时（pxIndex 逐个移动），`vListInsertEnd` 插入的节点会是**最后一个被轮到的**。这在延时任务列表（`xDelayedTaskList`）中非常关键——任务延时到期后重新插入就绪列表时，用它保证公平性。

### 4.5 `uxListRemove()` — 删除节点 (list.c:199-225)

```c
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
{
    // ★ 神奇之处: 通过 pxContainer 找到链表！
    //   调用者不需要知道节点在哪个链表中
    List_t * const pxList = pxItemToRemove->pxContainer;

    // 标准双向链表删除（2 步）
    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
    pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

    // ★ 重要: 如果 pxIndex 正好指向被删除的节点
    //   把 pxIndex 移到前一个节点，保证遍历不中断
    if( pxList->pxIndex == pxItemToRemove )
    {
        pxList->pxIndex = pxItemToRemove->pxPrevious;
    }

    pxItemToRemove->pxContainer = NULL;  // 标记为"不在任何链表中"
    (pxList->uxNumberOfItems)--;

    return pxList->uxNumberOfItems;  // 返回删除后剩余节点数
}
```

**设计亮点**：删除操作不需要传入链表指针！因为 `pxContainer` 已经记录了节点所属链表。这极大地简化了调度器代码。

---

## 五、关键宏解析

### 5.1 `listGET_OWNER_OF_NEXT_ENTRY()` — 轮询遍历 (list.h:285-296)

```c
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )
{
    List_t * const pxConstList = ( pxList );
    // ① pxIndex 先移动到下一个节点
    ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;
    // ② 如果指向了哨兵节点，再跳一步（跳过哨兵）
    if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) )
    {
        ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;
    }
    // ③ 返回当前节点的拥有者 (TCB)
    ( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;
}
```

**这就是时间片轮转的实现！** 每次调用取下一个任务的 TCB 指针。同优先级的就绪任务排在一个链表里，调度器通过反复调这个宏实现 Round Robin。

### 5.2 `listREMOVE_ITEM()` — 内联版删除 (list.h:314-330)

```c
#define listREMOVE_ITEM( pxItemToRemove )
{
    List_t * const pxList = ( pxItemToRemove )->pxContainer;

    ( pxItemToRemove )->pxNext->pxPrevious = ( pxItemToRemove )->pxPrevious;
    ( pxItemToRemove )->pxPrevious->pxNext = ( pxItemToRemove )->pxNext;

    if( pxList->pxIndex == ( pxItemToRemove ) )
    {
        pxList->pxIndex = ( pxItemToRemove )->pxPrevious;
    }

    ( pxItemToRemove )->pxContainer = NULL;
    ( pxList->uxNumberOfItems )--;
}
```

和 `uxListRemove()` 功能一样，但写成宏（内联展开），因为它在 `xTaskIncrementTick()` 中被频繁调用，需要省掉函数调用的开销。

### 5.3 `listINSERT_END()` — 内联版尾部插入 (list.h:354-377)

```c
#define listINSERT_END( pxList, pxNewListItem )
{
    ListItem_t * const pxIndex = ( pxList )->pxIndex;

    listTEST_LIST_INTEGRITY( ( pxList ) );
    listTEST_LIST_ITEM_INTEGRITY( ( pxNewListItem ) );

    ( pxNewListItem )->pxNext = pxIndex;
    ( pxNewListItem )->pxPrevious = pxIndex->pxPrevious;

    pxIndex->pxPrevious->pxNext = ( pxNewListItem );
    pxIndex->pxPrevious = ( pxNewListItem );

    ( pxNewListItem )->pxContainer = ( pxList );
    ( ( pxList )->uxNumberOfItems )++;
}
```

功能同 `vListInsertEnd()`，也是为性能关键路径（如 `xTaskIncrementTick`）做的内联优化。

### 5.4 其他常用访问宏

| 宏 | 功能 |
|-----|------|
| `listSET_LIST_ITEM_OWNER(pxItem, pxOwner)` | 设置节点拥有者 |
| `listGET_LIST_ITEM_OWNER(pxItem)` | 获取节点拥有者 |
| `listSET_LIST_ITEM_VALUE(pxItem, xValue)` | 设置节点排序值 |
| `listGET_LIST_ITEM_VALUE(pxItem)` | 获取节点排序值 |
| `listGET_ITEM_VALUE_OF_HEAD_ENTRY(pxList)` | 获取链表第一个节点的值 |
| `listGET_HEAD_ENTRY(pxList)` | 获取链表第一个节点 |
| `listGET_NEXT(pxItem)` | 获取下一个节点 |
| `listGET_END_MARKER(pxList)` | 获取哨兵节点 |
| `listLIST_IS_EMPTY(pxList)` | 判断链表是否为空 |
| `listCURRENT_LIST_LENGTH(pxList)` | 获取链表长度 |
| `listGET_OWNER_OF_HEAD_ENTRY(pxList)` | 获取链表第一个节点的拥有者 |
| `listIS_CONTAINED_WITHIN(pxList, pxItem)` | 判断节点是否在指定链表中 |
| `listLIST_ITEM_CONTAINER(pxItem)` | 获取节点所在的链表 |
| `listLIST_IS_INITIALISED(pxList)` | 判断链表是否已初始化 |

---

## 六、核心设计要点

### 🔑 为什么 `pxIndex` 存在？

`pxIndex` 是一个**游标/迭代器**，不是永远指向链表头。在同一优先级任务的就绪链表中，`pxIndex` 指向"**下一个应该被调度的任务**"。通过 `listGET_OWNER_OF_NEXT_ENTRY` 不断后移，实现了时间片轮转。

### 🔑 为什么同值节点"后插入的排后面"？

`vListInsert` 的比较条件是 `<=`（不是 `<`）。所以遍历遇到同值时**不停止**，新节点放到所有同值节点的最后面。这保证了：**先进入就绪链表的任务先被调度**，后进入的排队——公平！

### 🔑 `vListInsert` vs `vListInsertEnd` 的用法场景

| 函数 | 用途 | 插入位置 |
|------|------|----------|
| `vListInsert` | 就绪任务列表（按优先级排序） | 按 xItemValue 升序 |
| `vListInsertEnd` | 延时任务列表（任务刚被唤醒时） | 插到 pxIndex 前面 |

### 🔑 `volatile` 的设计权衡 (list.h:65-95)

链表结构的成员不加 `volatile` 修饰，以获取更好的优化性能。原因：这些成员只在临界区或调度器挂起期间被修改，这些上下文本身就是"功能上的原子操作"。

如果编译器跨模块优化导致问题，用户可以通过在 `FreeRTOSConfig.h` 中定义：
```c
#define configLIST_VOLATILE volatile
```
来强制所有链表成员为 `volatile`。

---

## 七、总结：list 模块在 FreeRTOS 全局中的位置

```
┌─────────────────────────────────────────────────────┐
│                  FreeRTOS 内核                       │
│                                                     │
│   tasks.c ──→ 用 List_t 管理：                       │
│      ├── pxReadyTasksLists[]  (就绪任务链表数组)      │
│      ├── xDelayedTaskList1    (延时任务链表1)         │
│      ├── xDelayedTaskList2    (延时任务链表2)         │
│      ├── xPendingReadyList    (挂起就绪链表)          │
│      ├── xSuspendedTaskList   (挂起任务链表)          │
│      └── xTasksWaitingTermination (等待删除的任务)    │
│                                                     │
│   queue.c ──→ 用 List_t 管理：                       │
│      ├── xTasksWaitingToSend  (等待发送的任务队列)    │
│      └── xTasksWaitingToReceive (等待接收的任务队列)  │
│                                                     │
│   timers.c ──→ 用 List_t 管理：                      │
│      └── xActiveTimerList    (活跃定时器链表)        │
│                                                     │
│   全部以 list.c 为基础！                              │
└─────────────────────────────────────────────────────┘
```

### 代码量统计

| 文件 | 大小 | 说明 |
|------|------|------|
| list.h | ~500 行 | 数据结构定义 + 内联宏 + 函数声明 |
| list.c | ~230 行 | 只包含 5 个核心函数 |

虽然代码量极小，但它是整个 FreeRTOS 内核的**数据骨架**。理解这个模块是理解任务调度、队列、定时器的前提。
