/**
 * @file list.c
 * @brief CN语言运行时双向链表（LinkedList）实现
 * 
 * 提供双向链表的基本操作功能
 * 
 * @version v1.0
 * @date 2026-03-28
 */

#include "cnlang/runtime/collections.h"
#include "cnlang/runtime/memory.h"

/**
 * @brief 创建空链表
 * 
 * @return CnList* 成功返回链表指针，失败返回NULL
 */
CnList* cn_rt_list_create(void) {
    /* 分配链表结构体 */
    CnList* list = (CnList*)cn_rt_malloc(sizeof(CnList));
    if (list == NULL) {
        return NULL;
    }
    
    /* 初始化为空链表 */
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    
    return list;
}

/**
 * @brief 销毁链表
 * 
 * 注意：此函数只释放链表结构本身，不释放元素指针指向的内存
 * 调用者需要自行管理元素的内存生命周期
 * 
 * @param list 链表指针
 */
void cn_rt_list_destroy(CnList* list) {
    if (list == NULL) {
        return;
    }
    
    /* 清空所有节点 */
    cn_rt_list_clear(list);
    
    /* 释放链表结构体 */
    cn_rt_free(list);
}

/**
 * @brief 在头部插入元素
 * 
 * @param list 链表指针
 * @param data 要插入的元素指针
 * @return int 成功返回0，失败返回-1
 */
int cn_rt_list_push_front(CnList* list, void* data) {
    if (list == NULL) {
        return -1;
    }
    
    /* 创建新节点 */
    CnListNode* node = (CnListNode*)cn_rt_malloc(sizeof(CnListNode));
    if (node == NULL) {
        return -1;
    }
    
    /* 设置节点数据 */
    node->data = data;
    node->prev = NULL;
    node->next = list->head;
    
    /* 更新头节点的前驱指针 */
    if (list->head != NULL) {
        list->head->prev = node;
    } else {
        /* 空链表，尾节点也需要指向新节点 */
        list->tail = node;
    }
    
    /* 更新头节点 */
    list->head = node;
    list->size++;
    
    return 0;
}

/**
 * @brief 在尾部插入元素
 * 
 * @param list 链表指针
 * @param data 要插入的元素指针
 * @return int 成功返回0，失败返回-1
 */
int cn_rt_list_push_back(CnList* list, void* data) {
    if (list == NULL) {
        return -1;
    }
    
    /* 创建新节点 */
    CnListNode* node = (CnListNode*)cn_rt_malloc(sizeof(CnListNode));
    if (node == NULL) {
        return -1;
    }
    
    /* 设置节点数据 */
    node->data = data;
    node->prev = list->tail;
    node->next = NULL;
    
    /* 更新尾节点的后继指针 */
    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        /* 空链表，头节点也需要指向新节点 */
        list->head = node;
    }
    
    /* 更新尾节点 */
    list->tail = node;
    list->size++;
    
    return 0;
}

/**
 * @brief 删除头部元素
 * 
 * @param list 链表指针
 * @return void* 成功返回被删除的元素指针，链表为空返回NULL
 */
void* cn_rt_list_pop_front(CnList* list) {
    if (list == NULL || list->head == NULL) {
        return NULL;
    }
    
    /* 获取头节点 */
    CnListNode* node = list->head;
    void* data = node->data;
    
    /* 更新头节点 */
    list->head = node->next;
    
    if (list->head != NULL) {
        list->head->prev = NULL;
    } else {
        /* 链表变为空，尾节点也需要清空 */
        list->tail = NULL;
    }
    
    /* 释放节点 */
    cn_rt_free(node);
    list->size--;
    
    return data;
}

/**
 * @brief 删除尾部元素
 * 
 * @param list 链表指针
 * @return void* 成功返回被删除的元素指针，链表为空返回NULL
 */
void* cn_rt_list_pop_back(CnList* list) {
    if (list == NULL || list->tail == NULL) {
        return NULL;
    }
    
    /* 获取尾节点 */
    CnListNode* node = list->tail;
    void* data = node->data;
    
    /* 更新尾节点 */
    list->tail = node->prev;
    
    if (list->tail != NULL) {
        list->tail->next = NULL;
    } else {
        /* 链表变为空，头节点也需要清空 */
        list->head = NULL;
    }
    
    /* 释放节点 */
    cn_rt_free(node);
    list->size--;
    
    return data;
}

/**
 * @brief 获取链表元素数量
 * 
 * @param list 链表指针
 * @return size_t 元素数量
 */
size_t cn_rt_list_size(CnList* list) {
    if (list == NULL) {
        return 0;
    }
    return list->size;
}

/**
 * @brief 检查链表是否为空
 * 
 * @param list 链表指针
 * @return int 为空返回1，非空返回0
 */
int cn_rt_list_is_empty(CnList* list) {
    if (list == NULL) {
        return 1;
    }
    return (list->size == 0) ? 1 : 0;
}

/**
 * @brief 清空链表所有元素
 * 
 * 注意：此函数只释放节点结构，不释放元素指针指向的内存
 * 
 * @param list 链表指针
 */
void cn_rt_list_clear(CnList* list) {
    if (list == NULL) {
        return;
    }
    
    /* 遍历释放所有节点 */
    CnListNode* current = list->head;
    while (current != NULL) {
        CnListNode* next = current->next;
        cn_rt_free(current);
        current = next;
    }
    
    /* 重置链表状态 */
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/**
 * @brief 获取头部元素（不删除）
 * 
 * @param list 链表指针
 * @return void* 成功返回元素指针，链表为空返回NULL
 */
void* cn_rt_list_get_front(CnList* list) {
    if (list == NULL || list->head == NULL) {
        return NULL;
    }
    return list->head->data;
}

/**
 * @brief 获取尾部元素（不删除）
 * 
 * @param list 链表指针
 * @return void* 成功返回元素指针，链表为空返回NULL
 */
void* cn_rt_list_get_back(CnList* list) {
    if (list == NULL || list->tail == NULL) {
        return NULL;
    }
    return list->tail->data;
}
