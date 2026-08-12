请在我的项目文件夹中生成以下文档：



路径：docs/FreeRTOS\_list\_c\_Summary.md



\# FreeRTOS 源码导读 — list.c 模块完整总结（V202212.01）



\---



\# 一、模块概览



\## 1. list.c 在 FreeRTOS 内核中的位置



`list.c` 是 FreeRTOS 内核中最基础的数据结构模块，负责实现内核使用的双向循环链表。



在 FreeRTOS 中，几乎所有核心模块都依赖 list.c：



\* tasks.c —— 管理任务状态链表

\* queue.c —— 管理等待队列

\* timers.c —— 管理软件定时器

\* event group —— 管理事件等待列表



整个内核的数据组织可以概括为：



```

tasks.c

queue.c

timers.c

&#x20;    │

&#x20;    ▼

&#x20; list.c

```



可以说：



> \*\*任务调度、任务阻塞、任务唤醒，本质上都是链表操作。\*\*



\---



\## 2. 为什么 FreeRTOS 要自己实现链表？



FreeRTOS 面向 MCU 实时系统，具有以下特点：



\* RAM 极小

\* 不依赖 C 标准库

\* 不依赖 malloc

\* 要求执行时间确定

\* 要求可移植



普通链表通常采用：



```

Node

&#x20;│

malloc

&#x20;│

Data

```



这种方式存在：



\* 内存碎片

\* 分配失败风险

\* 时间不可预测



FreeRTOS 使用的是：



> \*\*侵入式双向循环链表（Intrusive Doubly Circular Linked List）\*\*



即：



```

TCB

&#x20;│

&#x20;└── ListItem

```



链表节点直接嵌入对象内部，无需额外申请节点内存。



\---



\## 3. list.c 与裸机数组/队列的本质区别



你的裸机框架：



```

APP\_Event



eventQueue\[32]



head

tail

count

```



适用于：



\* FIFO

\* 消息传递

\* 固定容量



而 FreeRTOS：



```

TCB

&#x20;│

ListItem

&#x20;│

List

```



适用于：



\* 状态管理

\* 排序

\* 多状态迁移

\* 高效调度



\---



\# 二、核心数据结构



FreeRTOS list.c 包含三个核心结构：



```

List\_t

&#x20;  │

&#x20;  ▼

MiniListItem\_t（哨兵节点）



ListItem\_t（普通节点）

&#x20;  │

&#x20;  ▼

TCB

```



\---



\## 1. List\_t（链表控制块）



```c

typedef struct xLIST

{

&#x20;   UBaseType\_t uxNumberOfItems;



&#x20;   ListItem\_t \*pxIndex;



&#x20;   MiniListItem\_t xListEnd;



} List\_t;

```



\### 字段说明



| 字段              | 作用       |

| --------------- | -------- |

| uxNumberOfItems | 当前链表节点数量 |

| pxIndex         | 当前遍历位置   |

| xListEnd        | 哨兵节点     |



\### uxNumberOfItems



保存链表中节点数量。



例如：



```

Ready List



TaskA

TaskB

TaskC



uxNumberOfItems = 3

```



查询复杂度：



```

O(1)

```



无需遍历。



\---



\### pxIndex



用于链表遍历。



例如：



```

TaskA

&#x20;↓

TaskB

&#x20;↓

TaskC

```



pxIndex 保存当前遍历位置。



\---



\### xListEnd



这是：



> 哨兵节点（Sentinel Node）



特点：



\* 永远存在

\* 不属于任何任务

\* next / previous 永远有效



避免大量：



```c

if(node == NULL)

```



判断。



\---



\## 2. ListItem\_t（链表节点）



```c

typedef struct xLIST\_ITEM

{

&#x20;   TickType\_t xItemValue;



&#x20;   struct xLIST\_ITEM \*pxNext;



&#x20;   struct xLIST\_ITEM \*pxPrevious;



&#x20;   void \*pvOwner;



&#x20;   struct xLIST \*pxContainer;



} ListItem\_t;

```



\### 字段说明



| 字段          | 作用       |

| ----------- | -------- |

| xItemValue  | 排序依据     |

| pxNext      | 后继节点     |

| pxPrevious  | 前驱节点     |

| pvOwner     | 拥有该节点的对象 |

| pxContainer | 当前所属链表   |



\---



\### xItemValue



用于排序。



例如 Delay List：



```

100 Tick



500 Tick



1000 Tick

```



按 Tick 从小到大排列。



\---



\### pvOwner



这是 FreeRTOS 最重要的设计之一。



作用：



> 从链表节点快速找到所属对象。



关系：



```

ListItem

&#x20;   │

pvOwner

&#x20;   │

&#x20;   ▼

&#x20;TCB

```



调度器通过：



```

ListItem

&#x20;   ↓

pvOwner

&#x20;   ↓

TCB

```



快速找到任务控制块。



\---



\### pxContainer



作用：



> 记录当前节点属于哪个链表。



例如：



```

TaskA



&#x20;│



ListItem



&#x20;│



pxContainer



&#x20;│



ReadyList

```



当任务从 Ready List 移动到 Delay List 时：



pxContainer 会同步改变。



\---



\## 3. MiniListItem\_t



```c

typedef struct xMINI\_LIST\_ITEM

{

&#x20;   TickType\_t xItemValue;



&#x20;   struct xLIST\_ITEM \*pxNext;



&#x20;   struct xLIST\_ITEM \*pxPrevious;



} MiniListItem\_t;

```



这是哨兵节点。



由于它不是任务，因此无需：



\* pvOwner

\* pxContainer



从而节省 RAM。



\---



\## 三者完整关系图



```text

&#x20;               List\_t



&#x20;     +-----------------------+

&#x20;     | uxNumberOfItems       |

&#x20;     | pxIndex               |

&#x20;     | xListEnd              |

&#x20;     +-----------------------+



&#x20;               │

&#x20;               ▼



&#x20;      MiniListItem\_t

&#x20;        （哨兵节点）



&#x20;               ▲

&#x20;               │



&#x20;        ListItem\_t

&#x20;     +----------------+

&#x20;     | xItemValue     |

&#x20;     | pxNext         |

&#x20;     | pxPrevious     |

&#x20;     | pvOwner -------+------+

&#x20;     | pxContainer ---+      |

&#x20;     +----------------+      |

&#x20;                             |

&#x20;                             ▼



&#x20;                           TCB\_t

```



\---



\# 三、链表操作函数



\## 1. vListInitialise()



\### 原型



```c

void vListInitialise(List\_t \* const pxList);

```



\### 作用



初始化一个空链表。



\### 调用者



\* tasks.c

\* 系统初始化



\### 核心流程



```

初始化数量=0



↓



初始化pxIndex



↓



初始化xListEnd



↓



next/previous 指向自己



↓



完成

```



复杂度：



```

O(1)

```



\---



\## 2. vListInitialiseItem()



\### 原型



```c

void vListInitialiseItem(ListItem\_t \*pxItem);

```



\### 作用



初始化链表节点。



\### 核心流程



```

pxContainer = NULL



↓



表示未加入任何链表

```



复杂度：



```

O(1)

```



\---



\## 3. vListInsertEnd()



\### 原型



```c

void vListInsertEnd(

&#x20;   List\_t \*pxList,

&#x20;   ListItem\_t \*pxNewListItem

);

```



\### 作用



尾部插入节点。



\### 应用



Ready List。



\### 流程



```

找到尾部



↓



修改前后指针



↓



数量++



↓



结束

```



复杂度：



```

O(1)

```



\---



\## 4. vListInsert()



\### 原型



```c

void vListInsert(

&#x20;   List\_t \*pxList,

&#x20;   ListItem\_t \*pxNewListItem

);

```



\### 作用



按 xItemValue 排序插入。



\### 应用



Delay List。



\### 流程



```

遍历寻找位置



↓



按Tick排序



↓



插入



↓



数量++

```



复杂度：



```

O(n)

```



\---



\## 5. uxListRemove()



\### 原型



```c

UBaseType\_t uxListRemove(

&#x20;   ListItem\_t \*pxItemToRemove

);

```



\### 作用



删除链表节点。



\### 流程



```

修改前后节点



↓



数量--



↓



pxContainer=NULL



↓



返回剩余节点数量

```



复杂度：



```

O(1)

```



\---



\# 四、链表查询宏



\## listGET\_OWNER\_OF\_NEXT\_ENTRY()



作用：



遍历获取下一个节点对应对象。



流程：



```

移动pxIndex



↓



获取ListItem



↓



pvOwner



↓



TCB

```



\---



\## listGET\_OWNER\_OF\_HEAD\_ENTRY()



作用：



获取头节点拥有者。



主要用于：



Delay List 唤醒任务。



\---



\## listCURRENT\_LIST\_LENGTH()



定义：



```c

\#define listCURRENT\_LIST\_LENGTH(pxList) \\

((pxList)->uxNumberOfItems)

```



作用：



快速获取链表长度。



复杂度：



```

O(1)

```



\---



\## listIS\_CONTAINED\_WITHIN()



定义：



```c

\#define listIS\_CONTAINED\_WITHIN(pxList, pxItem) \\

((pxItem)->pxContainer == (pxList))

```



作用：



判断节点是否属于指定链表。



\---



\# 五、与裸机框架的对比



\## APP\_Event 队列 vs FreeRTOS 链表



你的：



```

APP\_Event



eventQueue\[]



head

tail

count

```



特点：



\* FIFO

\* 固定数组

\* 消息管理



FreeRTOS：



```

TCB



↓



ListItem



↓



List

```



特点：



\* 状态管理

\* 动态组织

\* 支持排序



| 项目   | APP\_Event | FreeRTOS List |

| ---- | --------- | ------------- |

| 数据结构 | 数组        | 链表            |

| 管理对象 | 事件        | 任务            |

| 容量   | 固定        | 动态            |

| 是否排序 | 否         | 是             |

| 管理方式 | FIFO      | 状态迁移          |



\---



\## APP\_Timer vs Delay List



你的：



```c

for(i=0;i<TIMER\_MAX;i++)

{

&#x20;   check();

}

```



复杂度：



```

O(n)

```



FreeRTOS：



```

Delay List



100 Tick



500 Tick



1000 Tick

```



Tick 到达时：



只检查：



```

第一个节点

```



唤醒复杂度：



```

O(1)

```



\---



\## 侵入式 vs 非侵入式



普通链表：



```

ListNode



↓



TCB\*

```



FreeRTOS：



```

TCB



↓



ListItem

```



优势：



\* 无 malloc

\* RAM 更少

\* 查找更快

\* 无额外节点对象



\---



\# 六、学习要点与设计思想总结



\## 五个核心设计思想



\### 1. 哨兵节点（Sentinel）



链表永远完整。



避免 NULL 判断。



\---



\### 2. 侵入式链表（Intrusive List）



对象内部携带链表节点。



无需动态申请节点。



\---



\### 3. 双向循环链表



支持：



\* O(1) 删除

\* O(1) 插入尾部

\* 正反遍历



\---



\### 4. 排序插入



Delay List：



牺牲：



```

O(n)

```



插入时间。



换取：



```

O(1)

```



唤醒效率。



\---



\### 5. Container 反向索引



利用：



```c

pxContainer

```



快速判断：



> 当前节点属于哪个链表。



\---



\## 三个最容易困惑的问题



\### 困惑一



为什么一个 TCB 有两个 ListItem？



答案：



一个管理任务状态，一个管理事件等待。



\---



\### 困惑二



为什么不用数组管理任务？



因为任务状态会不断迁移：



```

Ready



↓



Blocked



↓



Delay



↓



Ready

```



链表迁移效率更高。



\---



\### 困惑三



list.c 只是一个普通链表吗？



不是。



它实际上是：



> \*\*整个 FreeRTOS 调度器的数据组织基础。\*\*



tasks.c、queue.c、timers.c 都建立在它之上。



\---



\# 七、一句话总结



> \*\*FreeRTOS 的 list.c 通过侵入式双向循环链表，将任务控制块、状态管理和调度机制有机结合，实现了实时操作系统中高效、确定性的对象组织方式，是整个 FreeRTOS 内核调度器最重要的基础数据结构模块。\*\*



