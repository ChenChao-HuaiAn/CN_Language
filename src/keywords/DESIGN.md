# CN_Language 中文关键字模块 设计文档

## 设计目标

### 核心目标
1. **模块化设计**：遵循单一职责原则，每个文件不超过500行，每个函数不超过50行
2. **可扩展性**：支持动态添加、删除和修改关键字
3. **高效查询**：提供快速的关键字检查和信息查询
4. **完整信息**：每个关键字包含分类、描述、英文等价、优先级等完整信息
5. **易于集成**：提供清晰的API接口，便于其他模块集成

### 设计原则
1. **单一职责**：每个文件专注于特定功能
2. **接口清晰**：提供简单明了的API接口
3. **内存安全**：正确管理内存，避免泄漏
4. **错误处理**：提供完善的错误处理机制
5. **文档完整**：为所有接口提供详细文档

## 架构设计

### 模块结构
```
src/keywords/
├── CN_keywords.h          # 公共头文件（接口定义）
├── CN_keyword_table.c     # 关键字表管理（核心数据结构）
├── CN_keyword_predefined.c # 预定义关键字初始化
├── CN_keyword_query.c     # 关键字查询功能
├── CN_keyword_utils.c     # 工具函数和打印功能
├── README.md              # 模块概述
├── API.md                 # API接口文档
├── EXAMPLES.md            # 使用示例
└── DESIGN.md              # 设计原理（本文档）
```

### 核心数据结构

#### 1. 关键字分类枚举 (Eum_CN_KeywordCategory_t)
```c
typedef enum Eum_CN_KeywordCategory_t
{
    Eum_KEYWORD_CATEGORY_VARIABLE,      // 变量和类型声明
    Eum_KEYWORD_CATEGORY_CONTROL,       // 控制结构
    Eum_KEYWORD_CATEGORY_FUNCTION,      // 函数相关
    Eum_KEYWORD_CATEGORY_LOGICAL,       // 逻辑运算符
    Eum_KEYWORD_CATEGORY_LITERAL,       // 字面量
    Eum_KEYWORD_CATEGORY_MODULE,        // 模块系统
    Eum_KEYWORD_CATEGORY_OPERATOR,      // 运算符
    Eum_KEYWORD_CATEGORY_TYPE,          // 类型
    Eum_KEYWORD_CATEGORY_OTHER          // 其他
} Eum_CN_KeywordCategory_t;
```

#### 2. 关键字信息结构体 (Stru_CN_KeywordInfo_t)
```c
typedef struct Stru_CN_KeywordInfo_t
{
    const char* keyword;                // 关键字字符串
    size_t length;                      // 关键字长度
    Eum_CN_KeywordCategory_t category;  // 关键字分类
    const char* description;            // 功能描述
    const char* english_equivalent;     // 英文等价
    bool is_reserved;                   // 是否为保留关键字
    int precedence;                     // 运算符优先级
} Stru_CN_KeywordInfo_t;
```

#### 3. 关键字表结构体 (Stru_CN_KeywordTable_t)
```c
typedef struct Stru_CN_KeywordTable_t
{
    Stru_CN_KeywordInfo_t* keywords;    // 关键字数组
    size_t count;                       // 关键字数量
    size_t capacity;                    // 数组容量
} Stru_CN_KeywordTable_t;
```

### 内存管理设计

#### 1. 动态内存分配
- 关键字字符串、描述和英文等价字符串动态分配
- 关键字表容量动态扩展
- 使用安全的内存分配函数（safe_malloc/safe_realloc）

#### 2. 内存释放策略
- 每个创建函数都有对应的销毁函数
- 销毁函数对NULL参数安全
- 释放所有关联的内存

#### 3. 字符串管理
- 使用`duplicate_string()`函数复制字符串
- 确保字符串以null结尾
- 检查字符串长度限制

### 算法设计

#### 1. 关键字查询算法
- **线性搜索**：使用简单的线性搜索算法
- **时间复杂度**：O(n)，其中n为关键字数量
- **优化考虑**：适合中小规模关键字表（<1000个关键字）
- **未来优化**：可扩展为哈希表或Trie树

#### 2. 关键字表扩展算法
- **容量管理**：初始容量64，每次翻倍扩展
- **内存分配**：使用realloc扩展内存
- **错误处理**：扩展失败时保持原表不变

#### 3. 排序算法
- **默认排序**：使用qsort进行快速排序
- **排序规则**：先按分类排序，再按关键字字符串排序
- **自定义排序**：支持用户自定义比较函数

## 接口设计

### 1. 关键字表管理接口
```c
// 创建和销毁
Stru_CN_KeywordTable_t* F_create_keyword_table(void);
void F_destroy_keyword_table(Stru_CN_KeywordTable_t* table);

// 添加和删除
bool F_add_keyword(Stru_CN_KeywordTable_t* table, ...);
bool F_remove_keyword(Stru_CN_KeywordTable_t* table, ...);
```

### 2. 关键字查询接口
```c
// 基本查询
bool F_is_keyword(Stru_CN_KeywordTable_t* table, ...);
const Stru_CN_KeywordInfo_t* F_get_keyword_info(Stru_CN_KeywordTable_t* table, ...);

// 分类查询
Stru_CN_KeywordInfo_t** F_get_keywords_by_category(Stru_CN_KeywordTable_t* table, ...);
size_t F_get_keyword_count_by_category(Stru_CN_KeywordTable_t* table, ...);

// 中英文互查
const char* F_get_english_equivalent(Stru_CN_KeywordTable_t* table, ...);
const char* F_get_chinese_equivalent(Stru_CN_KeywordTable_t* table, ...);
```

### 3. 迭代器接口
```c
// 迭代器操作
size_t F_keyword_table_begin(Stru_CN_KeywordTable_t* table);
size_t F_keyword_table_end(Stru_CN_KeywordTable_t* table);
size_t F_keyword_table_next(Stru_CN_KeywordTable_t* table, size_t index);
const Stru_CN_KeywordInfo_t* F_get_keyword_by_index(Stru_CN_KeywordTable_t* table, size_t index);
```

### 4. 统计接口
```c
// 统计功能
size_t F_get_keyword_count(Stru_CN_KeywordTable_t* table);
size_t F_get_reserved_keyword_count(Stru_CN_KeywordTable_t* table);
```

### 5. 工具接口
```c
// 打印功能
void F_print_keyword_info(const Stru_CN_KeywordInfo_t* keyword_info);
void F_print_keyword_table(Stru_CN_KeywordTable_t* table);
void F_print_keyword_table_by_category(Stru_CN_KeywordTable_t* table);

// 排序功能
int F_compare_keywords(const void* a, const void* b);
void F_sort_keyword_table(Stru_CN_KeywordTable_t* table, ...);
```

## 预定义关键字设计

### 1. 关键字分类
设计了9种关键字分类，覆盖编程语言的各个方面：

1. **变量和类型声明** (10个)：变量、整数、小数、字符串等
2. **控制结构** (9个)：如果、否则、当、循环等
3. **函数相关** (5个)：函数、返回、主程序等
4. **逻辑运算符** (4个)：与、或、非等
5. **字面量** (4个)：真、假、空等
6. **模块系统** (5个)：模块、导入、导出等
7. **运算符** (11个)：加、减、乘、除等
8. **类型** (6个)：类型、接口、类等
9. **其他** (16个)：常量、静态、公开等

**总计**：70个预定义关键字

### 2. 关键字信息设计
每个预定义关键字包含：
- **中文关键字**：CN语言中的关键字
- **英文等价**：对应的英文关键字
- **功能描述**：关键字的用途说明
- **分类**：关键字的功能类别
- **保留状态**：是否为保留关键字
- **优先级**：运算符的优先级（仅运算符）

### 3. 初始化策略
- **懒加载**：首次访问时初始化预定义关键字表
- **内存共享**：使用全局只读关键字表，避免重复初始化
- **错误恢复**：初始化失败时返回NULL，不影响其他功能

## 性能考虑

### 1. 时间复杂度分析
| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| 创建关键字表 | O(n) | n为预定义关键字数量 |
| 销毁关键字表 | O(n) | 释放所有内存 |
| 添加关键字 | O(1) 平均 | 可能需要扩展容量 |
| 删除关键字 | O(n) | 需要移动元素 |
| 查询关键字 | O(n) | 线性搜索 |
| 分类查询 | O(n) | 遍历所有关键字 |

### 2. 空间复杂度分析
- **关键字表**：O(n)，n为关键字数量
- **字符串存储**：O(∑len)，所有关键字字符串的总长度
- **临时内存**：O(1) 或 O(k)（分类查询时）

### 3. 优化策略
1. **容量预分配**：初始容量64，减少重新分配次数
2. **字符串共享**：相同字符串只存储一次（未来优化）
3. **缓存友好**：连续存储关键字信息，提高缓存命中率
4. **搜索优化**：可扩展为哈希表（未来版本）

## 错误处理设计

### 1. 错误类型
1. **内存错误**：分配失败、释放失败
2. **参数错误**：NULL指针、无效参数
3. **逻辑错误**：关键字已存在、关键字不存在
4. **资源错误**：文件操作失败

### 2. 错误处理策略
1. **返回值指示**：使用布尔值或NULL指示错误
2. **错误信息**：通过stderr输出错误信息
3. **资源清理**：错误时正确释放已分配资源
4. **状态保持**：操作失败时保持原状态不变

### 3. 安全设计
1. **NULL安全**：所有函数对NULL参数安全
2. **边界检查**：检查所有数组和字符串边界
3. **内存检查**：检查内存分配结果
4. **字符串检查**：检查字符串长度和格式

## 扩展性设计

### 1. 接口扩展
- **新分类支持**：在枚举中添加新分类
- **新功能接口**：添加新的查询或操作接口
- **回调函数**：支持用户自定义回调函数

### 2. 数据结构扩展
- **哈希表支持**：可替换为哈希表实现
- **Trie树支持**：可扩展为Trie树存储
- **持久化存储**：支持文件存储和加载

### 3. 功能扩展
- **正则表达式支持**：支持模式匹配查询
- **多语言支持**：支持更多语言的关键字
- **语法高亮**：提供语法高亮所需信息

## 集成设计

### 1. 与词法分析器集成
```c
// 在词法分析中使用关键字模块
bool is_cn_keyword(const char* str, size_t length)
{
    const Stru_CN_KeywordTable_t* table = F_get_predefined_keyword_table();
    return F_is_keyword((Stru_CN_KeywordTable_t*)table, str, length);
}
```

### 2. 与语法分析器集成
```c
// 在语法分析中使用关键字信息
void process_keyword_token(const char* keyword)
{
    const Stru_CN_KeywordTable_t* table = F_get_predefined_keyword_table();
    const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(
        (Stru_CN_KeywordTable_t*)table, keyword, 0);
    
    if (info != NULL) {
        // 根据关键字分类进行不同的语法处理
        switch (info->category) {
            case Eum_KEYWORD_CATEGORY_VARIABLE:
                process_variable_declaration();
                break;
            case Eum_KEYWORD_CATEGORY_CONTROL:
                process_control_structure();
                break;
            // ... 其他分类处理
        }
    }
}
```

### 3. 与IDE集成
```c
// 为IDE提供关键字信息
void provide_keyword_completion()
{
    const Stru_CN_KeywordTable_t* table = F_get_predefined_keyword_table();
    
    // 获取所有关键字
    size_t count = F_get_keyword_count((Stru_CN_KeywordTable_t*)table);
    
    // 为代码补全提供关键字列表
    for (size_t i = 0; i < count; i++) {
        const Stru_CN_KeywordInfo_t* info = F_get_keyword_by_index(
            (Stru_CN_KeywordTable_t*)table, i);
        
        if (info != NULL) {
            add_completion_suggestion(info->keyword, info->description);
        }
    }
}
```

## 测试策略

### 1. 单元测试
- **创建销毁测试**：测试关键字表的创建和销毁
- **添加删除测试**：测试关键字的添加和删除
- **查询测试**：测试各种查询功能
- **边界测试**：测试边界条件和错误情况

### 2. 集成测试
- **与词法分析器集成测试**：测试关键字识别功能
- **与语法分析器集成测试**：测试关键字处理功能
- **性能测试**：测试大规模关键字表的性能

### 3. 回归测试
- **功能回归**：确保新功能不影响原有功能
- **性能回归**：确保性能不会退化
- **内存回归**：确保没有内存泄漏

## 未来改进方向

### 1. 性能优化
- **哈希表实现**：替换线性搜索为哈希表
- **Trie树实现**：支持前缀匹配查询
- **内存池**：使用内存池提高分配效率

### 2. 功能增强
- **正则表达式查询**：支持模式匹配查询
- **模糊查询**：支持相似关键字查询
- **多语言支持**：支持更多编程语言关键字

### 3. 工具集成
- **代码生成**：自动生成关键字相关代码
- **文档生成**：自动生成关键字文档
- **测试生成**：自动生成测试用例

## 总结

CN_Language中文关键字模块采用模块化设计，遵循单一职责原则，提供了完整的关键字管理功能。模块具有以下特点：

1. **设计清晰**：结构清晰，接口明确
2. **功能完整**：支持关键字的所有基本操作
3. **性能合理**：针对中小规模关键字表优化
4. **易于扩展**：支持未来功能扩展
5. **文档完善**：提供完整的文档和示例

该模块为CN_Language的词法分析和语法分析提供了坚实的基础，同时也为IDE集成和其他工具提供了支持。
