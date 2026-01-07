# 队列工具模块 (Queue Utilities Module)

## 概述

队列工具模块提供了队列的高级工具功能。这些功能扩展了队列的基本操作，包括复制、比较、批量操作、转换和查找等高级功能。

## 功能特性

- **队列复制**：创建队列的深拷贝
- **队列比较**：比较两个队列的内容是否相等
- **批量操作**：支持批量入队和出队
- **队列转换**：在队列和数组之间转换
- **元素查找**：在队列中查找元素
- **元素检查**：检查队列是否包含特定元素

## API 接口

### 队列操作

- `F_queue_copy(Stru_Queue_t* src_queue)` - 复制队列
- `F_queue_equals(Stru_Queue_t* queue1, Stru_Queue_t* queue2, int (*compare_func)(const void*, const void*))` - 比较队列

### 批量操作

- `F_queue_enqueue_batch(Stru_Queue_t* queue, void** items, size_t count)` - 批量入队
- `F_queue_dequeue_batch(Stru_Queue_t* queue, void** out_items, size_t max_count)` - 批量出队

### 转换操作

- `F_queue_to_array(Stru_Queue_t* queue, void** out_array)` - 将队列转换为数组
- `F_queue_from_array(void** items, size_t count, size_t item_size)` - 从数组创建队列

### 查找操作

- `F_queue_find(Stru_Queue_t* queue, void* item, int (*compare_func)(const void*, const void*))` - 查找元素
- `F_queue_contains(Stru_Queue_t* queue, void* item, int (*compare_func)(const void*, const void*))` - 检查是否包含元素

## 实现细节

### 队列复制

队列复制是深拷贝，包括：
1. 创建新的队列结构体
2. 复制所有元素数据
3. 保持相同的元素顺序
4. 处理内存分配失败的情况

### 队列比较

队列比较支持：
1. 自定义比较函数
2. 默认使用memcmp比较
3. 处理NULL元素
4. 检查队列大小和元素大小

### 批量操作

批量操作优化了性能：
1. 减少函数调用开销
2. 保持操作的原子性
3. 处理部分成功的情况

### 队列-数组转换

转换操作提供：
1. 队列到数组的线性化
2. 数组到队列的批量创建
3. 内存管理的一致性

## 使用示例

### 队列复制和比较

```c
#include "queue_utils/CN_queue_utils.h"
#include "queue_core/CN_queue_core.h"
#include <stdio.h>
#include <string.h>

// 自定义比较函数
int compare_strings(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}

int main() {
    // 创建字符串队列
    Stru_Queue_t* queue1 = F_create_queue(32);
    
    // 添加字符串
    const char* strings[] = {"Hello", "World", "Queue", "Example"};
    for (int i = 0; i < 4; i++) {
        F_queue_enqueue(queue1, (void*)strings[i]);
    }
    
    // 复制队列
    Stru_Queue_t* queue2 = F_queue_copy(queue1);
    if (queue2 == NULL) {
        printf("队列复制失败\n");
        F_destroy_queue(queue1);
        return 1;
    }
    
    // 比较队列
    bool equal = F_queue_equals(queue1, queue2, compare_strings);
    printf("队列相等: %s\n", equal ? "是" : "否");
    
    // 清理
    F_destroy_queue(queue1);
    F_destroy_queue(queue2);
    return 0;
}
```

### 批量操作

```c
#include "queue_utils/CN_queue_utils.h"
#include "queue_core/CN_queue_core.h"
#include <stdio.h>

int main() {
    // 创建队列
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    
    // 准备批量数据
    int items[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    void* item_ptrs[10];
    for (int i = 0; i < 10; i++) {
        item_ptrs[i] = &items[i];
    }
    
    // 批量入队
    size_t enqueued = F_queue_enqueue_batch(queue, item_ptrs, 10);
    printf("批量入队数量: %zu\n", enqueued);
    
    // 批量出队
    int dequeued_items[5];
    void* dequeued_ptrs[5];
    for (int i = 0; i < 5; i++) {
        dequeued_ptrs[i] = &dequeued_items[i];
    }
    
    size_t dequeued = F_queue_dequeue_batch(queue, dequeued_ptrs, 5);
    printf("批量出队数量: %zu\n", dequeued);
    
    // 打印出队的元素
    printf("出队的元素: ");
    for (int i = 0; i < dequeued; i++) {
        printf("%d ", dequeued_items[i]);
    }
    printf("\n");
    
    // 清理
    F_destroy_queue(queue);
    return 0;
}
```

### 队列-数组转换

```c
#include "queue_utils/CN_queue_utils.h"
#include "queue_core/CN_queue_core.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 创建队列并添加元素
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    int values[] = {10, 20, 30, 40, 50};
    
    for (int i = 0; i < 5; i++) {
        F_queue_enqueue(queue, &values[i]);
    }
    
    // 将队列转换为数组
    void* array = NULL;
    size_t array_size = F_queue_to_array(queue, &array);
    
    if (array != NULL) {
        printf("转换后的数组大小: %zu\n", array_size);
        printf("数组元素: ");
        for (size_t i = 0; i < array_size; i++) {
            int* element = (int*)((char*)array + i * sizeof(int));
            printf("%d ", *element);
        }
        printf("\n");
        
        // 释放数组内存
        free(array);
    }
    
    // 从数组创建队列
    int new_values[] = {100, 200, 300};
    void* new_ptrs[3];
    for (int i = 0; i < 3; i++) {
        new_ptrs[i] = &new_values[i];
    }
    
    Stru_Queue_t* new_queue = F_queue_from_array(new_ptrs, 3, sizeof(int));
    if (new_queue != NULL) {
        printf("从数组创建的队列大小: %zu\n", F_queue_size(new_queue));
        F_destroy_queue(new_queue);
    }
    
    // 清理
    F_destroy_queue(queue);
    return 0;
}
```

### 元素查找

```c
#include "queue_utils/CN_queue_utils.h"
#include "queue_core/CN_queue_core.h"
#include <stdio.h>
#include <string.h>

int compare_integers(const void* a, const void* b) {
    int int_a = *(const int*)a;
    int int_b = *(const int*)b;
    return int_a - int_b;
}

int main() {
    // 创建队列
    Stru_Queue_t* queue = F_create_queue(sizeof(int));
    int values[] = {10, 20, 30, 40, 50};
    
    for (int i = 0; i < 5; i++) {
        F_queue_enqueue(queue, &values[i]);
    }
    
    // 查找元素
    int search_value = 30;
    int position = F_queue_find(queue, &search_value, compare_integers);
    printf("元素 %d 的位置: %d\n", search_value, position);
    
    // 检查是否包含元素
    int check_value = 25;
    bool contains = F_queue_contains(queue, &check_value, compare_integers);
    printf("队列包含 %d: %s\n", check_value, contains ? "是" : "否");
    
    // 清理
    F_destroy_queue(queue);
    return 0;
}
```

## 性能特征

### 时间复杂度
- 队列复制：O(n)，其中n是队列大小
- 队列比较：O(n)，其中n是队列大小
- 批量入队：O(n)，其中n是批量大小
- 批量出队：O(n)，其中n是批量大小
- 队列到数组转换：O(n)，其中n是队列大小
- 数组到队列转换：O(n)，其中n是数组大小
- 元素查找：O(n)，其中n是队列大小

### 空间复杂度
- 队列复制：O(n)，需要分配新队列
- 队列到数组转换：O(n)，需要分配数组
- 其他操作：O(1)额外空间

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、0、false等）
- 内存分配失败返回错误
- 部分成功时返回实际成功数量

## 使用场景

### 1. 队列备份和恢复
```c
// 备份队列状态
Stru_Queue_t* backup = F_queue_copy(original_queue);
// ... 执行一些操作 ...
// 恢复队列状态
F_destroy_queue(original_queue);
original_queue = F_queue_copy(backup);
F_destroy_queue(backup);
```

### 2. 批量数据处理
```c
// 批量处理网络数据包
size_t processed = F_queue_dequeue_batch(packet_queue, packets, BATCH_SIZE);
for (size_t i = 0; i < processed; i++) {
    process_packet(packets[i]);
}
```

### 3. 队列持久化
```c
// 将队列保存到文件
void* array;
size_t size = F_queue_to_array(queue, &array);
if (array != NULL) {
    save_to_file("queue_data.bin", array, size * queue->item_size);
    free(array);
}
```

### 4. 队列验证
```c
// 验证队列内容
bool valid = F_queue_equals(expected_queue, actual_queue, compare_function);
if (!valid) {
    log_error("队列验证失败");
}
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`
- 队列核心模块：`queue_core/CN_queue_core.h`

## 测试

工具模块包含完整的单元测试，覆盖所有API接口和边界条件。

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-07 | 初始版本 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
