/******************************************************************************
 * 文件名: CN_log_async.c
 * 功能: CN_Language日志系统异步队列处理
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，从CN_log_core.c中提取异步队列函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_log_internal.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 异步队列函数实现
// ============================================================================

/**
 * @brief 初始化异步队列
 */
bool CN_log_async_queue_init(Stru_CN_AsyncLogQueue_t* queue, size_t capacity)
{
    if (!queue || capacity == 0)
        return false;
    
    queue->items = (Stru_CN_AsyncLogItem_t*)malloc(capacity * sizeof(Stru_CN_AsyncLogItem_t));
    if (!queue->items)
        return false;
    
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    return true;
}

/**
 * @brief 销毁异步队列
 */
void CN_log_async_queue_destroy(Stru_CN_AsyncLogQueue_t* queue)
{
    if (!queue)
        return;
    
    // 释放队列中所有消息
    Stru_CN_AsyncLogItem_t item;
    while (CN_log_async_queue_pop(queue, &item))
    {
        if (item.message)
            CN_log_free_message(item.message);
    }
    
    free(queue->items);
    queue->items = NULL;
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

/**
 * @brief 推送消息到异步队列
 */
bool CN_log_async_queue_push(Stru_CN_AsyncLogQueue_t* queue,
                            Stru_CN_LogMessage_t* message,
                            size_t message_size)
{
    if (!queue || !message || queue->count >= queue->capacity)
        return false;
    
    queue->items[queue->tail].message = message;
    queue->items[queue->tail].message_size = message_size;
    
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    return true;
}

/**
 * @brief 从异步队列弹出消息
 */
bool CN_log_async_queue_pop(Stru_CN_AsyncLogQueue_t* queue,
                           Stru_CN_AsyncLogItem_t* item)
{
    if (!queue || !item || queue->count == 0)
        return false;
    
    *item = queue->items[queue->head];
    
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    return true;
}

/**
 * @brief 异步处理日志消息
 */
bool CN_log_process_message_async(Stru_CN_LogMessage_t* message)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !message || !state->config.async_mode)
        return false;
    
    // 计算消息大小
    size_t message_size = sizeof(Stru_CN_LogMessage_t) + strlen(message->message) + 1;
    
    // 添加到异步队列
    return CN_log_async_queue_push(&state->async_queue, message, message_size);
}
