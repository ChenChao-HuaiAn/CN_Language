/**
 * @file vector.c
 * @brief CN语言运行时动态数组（Vector）实现
 * 
 * 提供自动扩容的动态数组功能
 * 
 * @version v1.0
 * @date 2026-03-28
 */

#include <string.h>
#include "cnlang/runtime/collections.h"
#include "cnlang/runtime/memory.h"

/* 默认初始容量 */
#define CN_VECTOR_DEFAULT_CAPACITY 8

/**
 * @brief 创建动态数组
 * 
 * @param initial_capacity 初始容量（0表示使用默认值）
 * @return CnVector* 成功返回数组指针，失败返回NULL
 */
CnVector* cn_rt_vector_create(size_t initial_capacity) {
    /* 分配动态数组结构体 */
    CnVector* vec = (CnVector*)cn_rt_malloc(sizeof(CnVector));
    if (vec == NULL) {
        return NULL;
    }
    
    /* 设置容量：如果传入0则使用默认值 */
    vec->capacity = (initial_capacity > 0) ? initial_capacity : CN_VECTOR_DEFAULT_CAPACITY;
    vec->size = 0;
    
    /* 分配元素指针数组 */
    vec->data = (void**)cn_rt_calloc(vec->capacity, sizeof(void*));
    if (vec->data == NULL) {
        cn_rt_free(vec);
        return NULL;
    }
    
    return vec;
}

/**
 * @brief 销毁动态数组
 * 
 * 注意：此函数只释放数组结构本身，不释放元素指针指向的内存
 * 调用者需要自行管理元素的内存生命周期
 * 
 * @param vec 动态数组指针
 */
void cn_rt_vector_destroy(CnVector* vec) {
    if (vec == NULL) {
        return;
    }
    
    /* 释放元素指针数组 */
    if (vec->data != NULL) {
        cn_rt_free(vec->data);
    }
    
    /* 释放结构体 */
    cn_rt_free(vec);
}

/**
 * @brief 在尾部添加元素（自动扩容）
 * 
 * @param vec 动态数组指针
 * @param element 要添加的元素指针
 * @return int 成功返回0，失败返回-1
 */
int cn_rt_vector_push(CnVector* vec, void* element) {
    if (vec == NULL) {
        return -1;
    }
    
    /* 检查是否需要扩容 */
    if (vec->size >= vec->capacity) {
        /* 2倍扩容策略 */
        size_t new_capacity = vec->capacity * 2;
        void** new_data = (void**)cn_rt_realloc(vec->data, new_capacity * sizeof(void*));
        if (new_data == NULL) {
            return -1;
        }
        
        vec->data = new_data;
        vec->capacity = new_capacity;
        
        /* 初始化新分配的内存区域为NULL */
        for (size_t i = vec->size; i < vec->capacity; i++) {
            vec->data[i] = NULL;
        }
    }
    
    /* 添加元素到尾部 */
    vec->data[vec->size] = element;
    vec->size++;
    
    return 0;
}

/**
 * @brief 弹出尾部元素
 * 
 * @param vec 动态数组指针
 * @return void* 成功返回元素指针，数组为空返回NULL
 */
void* cn_rt_vector_pop(CnVector* vec) {
    if (vec == NULL || vec->size == 0) {
        return NULL;
    }
    
    /* 获取尾部元素 */
    void* element = vec->data[vec->size - 1];
    vec->data[vec->size - 1] = NULL;  /* 清空原位置 */
    vec->size--;
    
    return element;
}

/**
 * @brief 获取指定位置的元素
 * 
 * @param vec 动态数组指针
 * @param index 元素索引
 * @return void* 成功返回元素指针，索引越界返回NULL
 */
void* cn_rt_vector_get(CnVector* vec, size_t index) {
    if (vec == NULL || index >= vec->size) {
        return NULL;
    }
    
    return vec->data[index];
}

/**
 * @brief 设置指定位置的元素
 * 
 * @param vec 动态数组指针
 * @param index 元素索引
 * @param element 新元素指针
 * @return int 成功返回0，索引越界返回-1
 */
int cn_rt_vector_set(CnVector* vec, size_t index, void* element) {
    if (vec == NULL || index >= vec->size) {
        return -1;
    }
    
    vec->data[index] = element;
    return 0;
}

/**
 * @brief 获取动态数组元素数量
 * 
 * @param vec 动态数组指针
 * @return size_t 元素数量
 */
size_t cn_rt_vector_size(CnVector* vec) {
    if (vec == NULL) {
        return 0;
    }
    return vec->size;
}

/**
 * @brief 获取动态数组当前容量
 * 
 * @param vec 动态数组指针
 * @return size_t 当前容量
 */
size_t cn_rt_vector_capacity(CnVector* vec) {
    if (vec == NULL) {
        return 0;
    }
    return vec->capacity;
}

/**
 * @brief 清空动态数组所有元素
 * 
 * 注意：此函数不释放元素指针指向的内存
 * 
 * @param vec 动态数组指针
 */
void cn_rt_vector_clear(CnVector* vec) {
    if (vec == NULL) {
        return;
    }
    
    /* 将所有元素指针设为NULL */
    for (size_t i = 0; i < vec->size; i++) {
        vec->data[i] = NULL;
    }
    
    vec->size = 0;
}

/**
 * @brief 检查动态数组是否为空
 * 
 * @param vec 动态数组指针
 * @return int 为空返回1，非空或NULL返回0
 */
int cn_rt_vector_is_empty(CnVector* vec) {
    if (vec == NULL) {
        return 0;  /* NULL指针视为非空（错误状态） */
    }
    return (vec->size == 0) ? 1 : 0;
}
