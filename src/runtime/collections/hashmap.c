/**
 * @file hashmap.c
 * @brief CN语言运行时哈希表（HashMap）实现
 * 
 * 使用链地址法解决冲突，djb2哈希算法
 * 
 * @version v1.0
 * @date 2026-03-28
 */

#include "cnlang/runtime/collections.h"
#include "cnlang/runtime/memory.h"
#include <string.h>

/* 默认桶数量 */
#define CN_MAP_DEFAULT_BUCKETS 16

/**
 * @brief djb2哈希函数
 * 
 * 经典的字符串哈希算法，分布均匀，冲突率低
 * 
 * @param str 要哈希的字符串
 * @return unsigned long 哈希值
 */
static unsigned long cn_rt_hash_djb2(const char* str) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        /* hash * 33 + c */
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

/**
 * @brief 创建哈希表节点
 * 
 * @param key 键字符串
 * @param value 值指针
 * @return CnMapNode* 成功返回节点指针，失败返回NULL
 */
static CnMapNode* cn_rt_map_node_create(const char* key, void* value) {
    /* 分配节点结构体 */
    CnMapNode* node = (CnMapNode*)cn_rt_malloc(sizeof(CnMapNode));
    if (node == NULL) {
        return NULL;
    }
    
    /* 复制键字符串 */
    node->key = cn_rt_string_dup(key);
    if (node->key == NULL) {
        cn_rt_free(node);
        return NULL;
    }
    
    /* 设置值和next指针 */
    node->value = value;
    node->next = NULL;
    
    return node;
}

/**
 * @brief 销毁哈希表节点
 * 
 * @param node 节点指针
 */
static void cn_rt_map_node_destroy(CnMapNode* node) {
    if (node == NULL) {
        return;
    }
    
    /* 释放键字符串 */
    cn_rt_free(node->key);
    
    /* 释放节点结构体 */
    cn_rt_free(node);
}

/**
 * @brief 创建哈希表
 * 
 * @param initial_capacity 初始桶数量（0表示使用默认值16）
 * @return CnMap* 成功返回哈希表指针，失败返回NULL
 */
CnMap* cn_rt_map_create(size_t initial_capacity) {
    /* 分配哈希表结构体 */
    CnMap* map = (CnMap*)cn_rt_malloc(sizeof(CnMap));
    if (map == NULL) {
        return NULL;
    }
    
    /* 设置桶数量 */
    map->bucket_count = initial_capacity > 0 ? initial_capacity : CN_MAP_DEFAULT_BUCKETS;
    map->size = 0;
    
    /* 分配桶数组 */
    map->buckets = (CnMapNode**)cn_rt_calloc(map->bucket_count, sizeof(CnMapNode*));
    if (map->buckets == NULL) {
        cn_rt_free(map);
        return NULL;
    }
    
    return map;
}

/**
 * @brief 销毁哈希表
 * 
 * 注意：此函数只释放哈希表结构本身，不释放值指针指向的内存
 * 调用者需要自行管理值的内存生命周期
 * 
 * @param map 哈希表指针
 */
void cn_rt_map_destroy(CnMap* map) {
    if (map == NULL) {
        return;
    }
    
    /* 清空所有元素 */
    cn_rt_map_clear(map);
    
    /* 释放桶数组 */
    cn_rt_free(map->buckets);
    
    /* 释放哈希表结构体 */
    cn_rt_free(map);
}

/**
 * @brief 插入键值对
 * 
 * 如果键已存在，会更新对应的值
 * 
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @param value 值指针
 * @return int 成功返回0，失败返回-1
 */
int cn_rt_map_insert(CnMap* map, const char* key, void* value) {
    if (map == NULL || key == NULL) {
        return -1;
    }
    
    /* 计算桶索引 */
    unsigned long hash = cn_rt_hash_djb2(key);
    size_t index = hash % map->bucket_count;
    
    /* 检查键是否已存在 */
    CnMapNode* current = map->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            /* 键已存在，更新值 */
            current->value = value;
            return 0;
        }
        current = current->next;
    }
    
    /* 创建新节点 */
    CnMapNode* new_node = cn_rt_map_node_create(key, value);
    if (new_node == NULL) {
        return -1;
    }
    
    /* 头插法插入链表 */
    new_node->next = map->buckets[index];
    map->buckets[index] = new_node;
    map->size++;
    
    return 0;
}

/**
 * @brief 获取键对应的值
 * 
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @return void* 成功返回值指针，键不存在返回NULL
 */
void* cn_rt_map_get(CnMap* map, const char* key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }
    
    /* 计算桶索引 */
    unsigned long hash = cn_rt_hash_djb2(key);
    size_t index = hash % map->bucket_count;
    
    /* 在链表中查找键 */
    CnMapNode* current = map->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 删除键值对
 * 
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @return int 成功返回0，键不存在返回-1
 */
int cn_rt_map_remove(CnMap* map, const char* key) {
    if (map == NULL || key == NULL) {
        return -1;
    }
    
    /* 计算桶索引 */
    unsigned long hash = cn_rt_hash_djb2(key);
    size_t index = hash % map->bucket_count;
    
    /* 在链表中查找并删除 */
    CnMapNode* current = map->buckets[index];
    CnMapNode* prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            /* 找到键，删除节点 */
            if (prev == NULL) {
                /* 删除头节点 */
                map->buckets[index] = current->next;
            } else {
                /* 删除中间或尾节点 */
                prev->next = current->next;
            }
            
            cn_rt_map_node_destroy(current);
            map->size--;
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
    return -1;
}

/**
 * @brief 检查键是否存在
 * 
 * @param map 哈希表指针
 * @param key 键（字符串）
 * @return int 存在返回1，不存在返回0
 */
int cn_rt_map_contains(CnMap* map, const char* key) {
    if (map == NULL || key == NULL) {
        return 0;
    }
    
    /* 计算桶索引 */
    unsigned long hash = cn_rt_hash_djb2(key);
    size_t index = hash % map->bucket_count;
    
    /* 在链表中查找键 */
    CnMapNode* current = map->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return 1;
        }
        current = current->next;
    }
    
    return 0;
}

/**
 * @brief 获取哈希表元素数量
 * 
 * @param map 哈希表指针
 * @return size_t 元素数量
 */
size_t cn_rt_map_size(CnMap* map) {
    if (map == NULL) {
        return 0;
    }
    return map->size;
}

/**
 * @brief 检查哈希表是否为空
 * 
 * @param map 哈希表指针
 * @return int 为空返回1，非空返回0
 */
int cn_rt_map_is_empty(CnMap* map) {
    if (map == NULL) {
        return 1;
    }
    return map->size == 0 ? 1 : 0;
}

/**
 * @brief 清空哈希表所有元素
 * 
 * 注意：此函数只释放节点结构，不释放值指针指向的内存
 * 
 * @param map 哈希表指针
 */
void cn_rt_map_clear(CnMap* map) {
    if (map == NULL) {
        return;
    }
    
    /* 遍历所有桶 */
    for (size_t i = 0; i < map->bucket_count; i++) {
        /* 释放链表中所有节点 */
        CnMapNode* current = map->buckets[i];
        while (current != NULL) {
            CnMapNode* next = current->next;
            cn_rt_map_node_destroy(current);
            current = next;
        }
        map->buckets[i] = NULL;
    }
    
    map->size = 0;
}
