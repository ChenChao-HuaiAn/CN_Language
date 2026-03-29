/**
 * @file exception.h
 * @brief CN语言异常处理运行时支持
 * 
 * 本文件定义了CN语言的异常处理机制，包括：
 * - 异常结构体定义
 * - 异常帧（用于异常栈）
 * - setjmp/longjmp异常跳转API
 * - 异常类型匹配支持
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#ifndef CNLANG_RUNTIME_EXCEPTION_H
#define CNLANG_RUNTIME_EXCEPTION_H

#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 异常结构体定义
 * ============================================================================ */

/**
 * @brief 异常对象结构体
 * 
 * 存储异常的详细信息，包括类型、消息、抛出位置等
 */
typedef struct CnException {
    const char *type_name;      ///< 异常类型名（UTF-8编码中文）
    size_t type_name_length;    ///< 类型名长度（字节数）
    const char *message;        ///< 异常消息（可选）
    size_t message_length;      ///< 消息长度
    const char *file;           ///< 抛出位置文件名
    int line;                   ///< 抛出位置行号
    void *user_data;            ///< 用户自定义数据（可选）
    bool is_rethrown;           ///< 是否为重新抛出
} CnException;

/**
 * @brief 异常帧结构体
 * 
 * 用于构建异常处理栈，每个try块创建一个异常帧
 */
typedef struct CnExceptionFrame {
    jmp_buf jump_buffer;                ///< setjmp缓冲区
    struct CnExceptionFrame *prev;      ///< 前一帧（链表结构）
    CnException *current_exception;     ///< 当前捕获的异常
    int catch_handled;                  ///< catch块是否已处理
    int finally_executed;               ///< finally块是否已执行
} CnExceptionFrame;

/* ============================================================================
 * 异常栈管理
 * ============================================================================ */

/**
 * @brief 获取当前异常栈顶帧
 * @return 当前异常帧指针，如果无则返回NULL
 */
CnExceptionFrame *cn_exception_get_top_frame(void);

/**
 * @brief 压入新的异常帧
 * @param frame 新的异常帧指针
 */
void cn_exception_push_frame(CnExceptionFrame *frame);

/**
 * @brief 弹出异常帧
 * @return 被弹出的异常帧指针
 */
CnExceptionFrame *cn_exception_pop_frame(void);

/* ============================================================================
 * 异常抛出和捕获API
 * ============================================================================ */

/**
 * @brief 抛出异常
 * 
 * 填充异常信息并跳转到最近的异常帧
 * 
 * @param exception 异常对象指针
 * @param file 抛出位置文件名（使用__FILE__）
 * @param line 抛出位置行号（使用__LINE__）
 * @note 此函数不返回，通过longjmp跳转
 */
void cn_throw(CnException *exception, const char *file, int line);

/**
 * @brief 开始try块
 * 
 * 初始化异常帧并调用setjmp
 * 
 * @param frame 异常帧指针（由调用者分配）
 * @return setjmp返回值：0表示正常执行，非0表示异常跳转
 */
int cn_try_begin(CnExceptionFrame *frame);

/**
 * @brief 结束try块
 * 
 * 清理异常帧，如果异常未被处理则重新抛出
 * 
 * @param frame 异常帧指针
 */
void cn_try_end(CnExceptionFrame *frame);

/**
 * @brief 捕获指定类型的异常
 * 
 * 检查当前异常是否匹配指定类型
 * 
 * @param type_name 异常类型名（UTF-8编码中文）
 * @return 如果匹配返回异常指针，否则返回NULL
 */
CnException *cn_catch(const char *type_name);

/**
 * @brief 捕获任意类型的异常
 * 
 * @return 当前异常指针，如果没有异常返回NULL
 */
CnException *cn_catch_any(void);

/**
 * @brief 重新抛出当前异常
 * 
 * @note 此函数不返回，通过longjmp跳转
 */
void cn_rethrow(void);

/* ============================================================================
 * 异常类型匹配
 * ============================================================================ */

/**
 * @brief 检查异常类型是否匹配
 * 
 * 支持继承层次结构中的类型匹配
 * 
 * @param exception 异常对象
 * @param type_name 要匹配的类型名
 * @return 如果匹配返回true，否则返回false
 */
bool cn_exception_type_matches(const CnException *exception, const char *type_name);

/* ============================================================================
 * 异常创建辅助函数
 * ============================================================================ */

/**
 * @brief 初始化异常对象
 * 
 * @param exception 异常对象指针
 * @param type_name 异常类型名
 * @param message 异常消息（可为NULL）
 */
void cn_exception_init(CnException *exception, const char *type_name, const char *message);

/**
 * @brief 创建并抛出简单异常
 * 
 * 便捷函数，用于抛出简单的字符串异常
 * 
 * @param type_name 异常类型名
 * @param message 异常消息
 * @param file 文件名
 * @param line 行号
 */
void cn_throw_simple(const char *type_name, const char *message, 
                     const char *file, int line);

/* ============================================================================
 * 预定义异常类型
 * ============================================================================ */

/* 基础异常类型名 */
#define CN_EXCEPTION_BASE          "异常"          ///< 基础异常类型
#define CN_EXCEPTION_RUNTIME       "运行时异常"    ///< 运行时异常
#define CN_EXCEPTION_NULL_POINTER  "空指针异常"    ///< 空指针异常
#define CN_EXCEPTION_OUT_OF_RANGE  "越界异常"      ///< 越界异常
#define CN_EXCEPTION_DIVIDE_BY_ZERO "除零异常"     ///< 除零异常
#define CN_EXCEPTION_INVALID_ARG   "无效参数异常"  ///< 无效参数异常
#define CN_EXCEPTION_MEMORY        "内存异常"      ///< 内存分配异常
#define CN_EXCEPTION_IO            "输入输出异常"  ///< IO异常

/* ============================================================================
 * 便捷宏定义
 * ============================================================================ */

/**
 * @brief 抛出异常宏
 * 
 * 自动填充文件名和行号
 */
#define CN_THROW(exception) cn_throw(exception, __FILE__, __LINE__)

/**
 * @brief 抛出简单异常宏
 */
#define CN_THROW_SIMPLE(type, msg) cn_throw_simple(type, msg, __FILE__, __LINE__)

/**
 * @brief try块开始宏
 */
#define CN_TRY(frame) if (cn_try_begin(frame) == 0)

/**
 * @brief catch块宏
 */
#define CN_CATCH(type, var) else if ((var = cn_catch(type)) != NULL)

/**
 * @brief catch任意异常宏
 */
#define CN_CATCH_ANY(var) else if ((var = cn_catch_any()) != NULL)

/**
 * @brief finally块标记
 */
#define CN_FINALLY(frame) cn_try_end(frame)

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_EXCEPTION_H */
