/**
 * @file collections.h
 * @brief CN语言运行时集合数据结构
 * 
 * 提供动态数组（Vector）、链表（LinkedList）、哈希表（HashMap）等数据结构
 * 
 * @version v1.0
 * @date 2026-03-28
 */

#ifndef CNLANG_RUNTIME_COLLECTIONS_H
#define CNLANG_RUNTIME_COLLECTIONS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 动态数组（Vector）
 * ============================================================================
 * 自动扩容的动态数组，存储 void* 指针
 */

/**
 * @brief 动态数组结构体
 */
typedef struct {
    void** data;        /* 元素指针数组 */
    size_t size;        /* 当前元素数量 */
    size_t capacity;    /* 当前容量 */
} CnVector;

/**
 * @brief 创建动态数组
 * @param initial_capacity 初始容量（0表示使用默认值8）
 * @return 成功返回数组指针，失败返回NULL
 */
CnVector* cn_rt_vector_create(size_t initial_capacity);

/**
 * @brief 销毁动态数组
 * @param vec 动态数组指针
 */
void cn_rt_vector_destroy(CnVector* vec);

/**
 * @brief 在尾部添加元素
 * @param vec 动态数组指针
 * @param element 要添加的元素指针
 * @return 成功返回0，失败返回-1
 */
int cn_rt_vector_push(CnVector* vec, void* element);

/**
 * @brief 弹出尾部元素
 * @param vec 动态数组指针
 * @return 成功返回元素指针，数组为空返回NULL
 */
void* cn_rt_vector_pop(CnVector* vec);

/**
 * @brief 获取指定位置的元素
 * @param vec 动态数组指针
 * @param index 元素索引
 * @return 成功返回元素指针，索引越界返回NULL
 */
void* cn_rt_vector_get(CnVector* vec, size_t index);

/**
 * @brief 设置指定位置的元素
 * @param vec 动态数组指针
 * @param index 元素索引
 * @param element 新元素指针
 * @return 成功返回0，索引越界返回-1
 */
int cn_rt_vector_set(CnVector* vec, size_t index, void* element);

/**
 * @brief 获取动态数组元素数量
 * @param vec 动态数组指针
 * @return 元素数量
 */
size_t cn_rt_vector_size(CnVector* vec);

/**
 * @brief 获取动态数组当前容量
 * @param vec 动态数组指针
 * @return 当前容量
 */
size_t cn_rt_vector_capacity(CnVector* vec);

/**
 * @brief 清空动态数组所有元素
 * @param vec 动态数组指针
 */
void cn_rt_vector_clear(CnVector* vec);

/**
 * @brief 检查动态数组是否为空
 * @param vec 动态数组指针
 * @return 为空返回1，非空返回0
 */
int cn_rt_vector_is_empty(CnVector* vec);

/* 中文别名宏定义 - 动态数组 */
#define 创建动态数组 cn_rt_vector_create
#define 销毁动态数组 cn_rt_vector_destroy
#define 动态数组尾部添加 cn_rt_vector_push
#define 动态数组尾部弹出 cn_rt_vector_pop
#define 动态数组获取 cn_rt_vector_get
#define 动态数组设置 cn_rt_vector_set
#define 动态数组长度 cn_rt_vector_size
#define 动态数组容量 cn_rt_vector_capacity
#define 动态数组清空 cn_rt_vector_clear
#define 动态数组为空 cn_rt_vector_is_empty

/* ============================================================================
 * 双向链表（LinkedList）
 * ============================================================================
 * 双向链表数据结构，支持头部和尾部的高效插入删除操作
 */

/**
 * @brief 链表节点结构体
 */
typedef struct CnListNode {
    void* data;                 /* 元素指针 */
    struct CnListNode* prev;    /* 前驱节点 */
    struct CnListNode* next;    /* 后继节点 */
} CnListNode;

/**
 * @brief 双向链表结构体
 */
typedef struct {
    CnListNode* head;   /* 头节点 */
    CnListNode* tail;   /* 尾节点 */
    size_t size;        /* 元素数量 */
} CnList;

/**
 * @brief 创建空链表
 * @return 成功返回链表指针，失败返回NULL
 */
CnList* cn_rt_list_create(void);

/**
 * @brief 销毁链表
 * @param list 链表指针
 *
 * 注意：此函数只释放链表结构本身，不释放元素指针指向的内存
 * 调用者需要自行管理元素的内存生命周期
 */
void cn_rt_list_destroy(CnList* list);

/**
 * @brief 在头部插入元素
 * @param list 链表指针
 * @param data 要插入的元素指针
 * @return 成功返回0，失败返回-1
 */
int cn_rt_list_push_front(CnList* list, void* data);

/**
 * @brief 在尾部插入元素
 * @param list 链表指针
 * @param data 要插入的元素指针
 * @return 成功返回0，失败返回-1
 */
int cn_rt_list_push_back(CnList* list, void* data);

/**
 * @brief 删除头部元素
 * @param list 链表指针
 * @return 成功返回被删除的元素指针，链表为空返回NULL
 */
void* cn_rt_list_pop_front(CnList* list);

/**
 * @brief 删除尾部元素
 * @param list 链表指针
 * @return 成功返回被删除的元素指针，链表为空返回NULL
 */
void* cn_rt_list_pop_back(CnList* list);

/**
 * @brief 获取链表元素数量
 * @param list 链表指针
 * @return 元素数量
 */
size_t cn_rt_list_size(CnList* list);

/**
 * @brief 检查链表是否为空
 * @param list 链表指针
 * @return 为空返回1，非空返回0
 */
int cn_rt_list_is_empty(CnList* list);

/**
 * @brief 清空链表所有元素
 * @param list 链表指针
 *
 * 注意：此函数只释放节点结构，不释放元素指针指向的内存
 */
void cn_rt_list_clear(CnList* list);

/**
 * @brief 获取头部元素（不删除）
 * @param list 链表指针
 * @return 成功返回元素指针，链表为空返回NULL
 */
void* cn_rt_list_get_front(CnList* list);

/**
 * @brief 获取尾部元素（不删除）
 * @param list 链表指针
 * @return 成功返回元素指针，链表为空返回NULL
 */
void* cn_rt_list_get_back(CnList* list);

/* 中文别名宏定义 - 链表 */
#define 创建链表 cn_rt_list_create
#define 销毁链表 cn_rt_list_destroy
#define 链表头部插入 cn_rt_list_push_front
#define 链表尾部插入 cn_rt_list_push_back
#define 链表头部删除 cn_rt_list_pop_front
#define 链表尾部删除 cn_rt_list_pop_back
#define 链表长度 cn_rt_list_size
#define 链表为空 cn_rt_list_is_empty
#define 链表清空 cn_rt_list_clear
#define 链表获取头部 cn_rt_list_get_front
#define 链表获取尾部 cn_rt_list_get_back

/* ============================================================================
 * 哈希表（HashMap）
 * ============================================================================
 * 使用链地址法解决冲突的哈希表，键为字符串类型
 */

/**
 * @brief 哈希表节点结构体
 */
typedef struct CnMapNode {
    char* key;                  /* 键（字符串） */
    void* value;                /* 值指针 */
    struct CnMapNode* next;     /* 链地址法解决冲突 */
} CnMapNode;

/**
 * @brief 哈希表结构体
 */
typedef struct {
    CnMapNode** buckets;        /* 桶数组 */
    size_t bucket_count;        /* 桶数量 */
    size_t size;                /* 元素数量 */
} CnMap;

/**
 * @brief 创建哈希表
 * @param initial_capacity 初始桶数量（0表示使用默认值16）
 * @return 成功返回哈希表指针，失败返回NULL
 */
CnMap* cn_rt_map_create(size_t initial_capacity);

/**
 * @brief 销毁哈希表
 * @param map 哈希表指针
 *
 * 注意：此函数只释放哈希表结构本身，不释放值指针指向的内存
 * 调用者需要自行管理值的内存生命周期
 */
void cn_rt_map_destroy(CnMap* map);

/**
 * @brief 插入键值对
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @param value 值指针
 * @return 成功返回0，失败返回-1
 *
 * 如果键已存在，会更新对应的值
 */
int cn_rt_map_insert(CnMap* map, const char* key, void* value);

/**
 * @brief 获取键对应的值
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @return 成功返回值指针，键不存在返回NULL
 */
void* cn_rt_map_get(CnMap* map, const char* key);

/**
 * @brief 删除键值对
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @return 成功返回0，键不存在返回-1
 */
int cn_rt_map_remove(CnMap* map, const char* key);

/**
 * @brief 检查键是否存在
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @return 存在返回1，不存在返回0
 */
int cn_rt_map_contains(CnMap* map, const char* key);

/**
 * @brief 获取哈希表元素数量
 * @param map 哈希表指针
 * @return 元素数量
 */
size_t cn_rt_map_size(CnMap* map);

/**
 * @brief 检查哈希表是否为空
 * @param map 哈希表指针
 * @return 为空返回1，非空返回0
 */
int cn_rt_map_is_empty(CnMap* map);

/**
 * @brief 清空哈希表所有元素
 * @param map 哈希表指针
 *
 * 注意：此函数只释放节点结构，不释放值指针指向的内存
 */
void cn_rt_map_clear(CnMap* map);

/* 中文别名宏定义 - 哈希表 */
#define 创建哈希表 cn_rt_map_create
#define 销毁哈希表 cn_rt_map_destroy
#define 哈希表插入 cn_rt_map_insert
#define 哈希表获取 cn_rt_map_get
#define 哈希表删除 cn_rt_map_remove
#define 哈希表包含 cn_rt_map_contains
#define 哈希表大小 cn_rt_map_size
#define 哈希表为空 cn_rt_map_is_empty
#define 哈希表清空 cn_rt_map_clear

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_COLLECTIONS_H */
