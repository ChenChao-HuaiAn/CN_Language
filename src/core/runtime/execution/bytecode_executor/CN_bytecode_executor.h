/******************************************************************************
 * 文件名: CN_bytecode_executor.h
 * 功能: CN_Language 字节码执行器模块头文件
 * 
 * 定义字节码执行器的公共接口，提供字节码加载、执行和状态管理功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language架构委员会
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_BYTECODE_EXECUTOR_H
#define CN_BYTECODE_EXECUTOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 字节码执行器结构体
// ============================================================================

/**
 * @brief 字节码执行器结构体
 * 
 * 表示字节码执行器的实例，提供字节码加载和执行功能。
 */
typedef struct Stru_BytecodeExecutor_t {
    /**
     * @brief 私有数据指针
     */
    void* private_data;
} Stru_BytecodeExecutor_t;

// ============================================================================
// 字节码执行器接口函数
// ============================================================================

/**
 * @brief 创建字节码执行器
 * 
 * 创建并返回一个新的字节码执行器实例。
 * 
 * @param stack_size 栈大小（字节），0表示使用默认值
 * @return Stru_BytecodeExecutor_t* 新创建的字节码执行器实例
 */
Stru_BytecodeExecutor_t* F_create_bytecode_executor(size_t stack_size);

/**
 * @brief 销毁字节码执行器
 * 
 * 释放字节码执行器占用的所有资源。
 * 
 * @param executor 要销毁的字节码执行器
 */
void F_destroy_bytecode_executor(Stru_BytecodeExecutor_t* executor);

/**
 * @brief 加载字节码到执行器
 * 
 * 将字节码数据加载到执行器中，准备执行。
 * 
 * @param executor 字节码执行器
 * @param bytecode 字节码数据指针
 * @param bytecode_size 字节码数据大小
 * @return 加载成功返回true，失败返回false
 */
bool F_load_bytecode(Stru_BytecodeExecutor_t* executor, 
                    const uint8_t* bytecode, size_t bytecode_size);

/**
 * @brief 执行字节码
 * 
 * 执行已加载的字节码。
 * 
 * @param executor 字节码执行器
 * @param max_instructions 最大指令执行数，0表示无限制
 * @return 执行成功返回true，失败返回false
 */
bool F_execute_bytecode(Stru_BytecodeExecutor_t* executor, uint32_t max_instructions);

/**
 * @brief 获取执行器状态
 * 
 * 获取字节码执行器的当前状态。
 * 
 * @param executor 字节码执行器
 * @param program_counter 输出参数：程序计数器
 * @param stack_pointer 输出参数：栈指针
 * @param frame_pointer 输出参数：帧指针
 * @param running 输出参数：是否正在运行
 * @param error 输出参数：是否有错误
 */
void F_get_executor_state(const Stru_BytecodeExecutor_t* executor,
                         uint32_t* program_counter,
                         uint32_t* stack_pointer,
                         uint32_t* frame_pointer,
                         bool* running,
                         bool* error);

/**
 * @brief 获取错误信息
 * 
 * 获取字节码执行器的错误信息。
 * 
 * @param executor 字节码执行器
 * @return 错误信息字符串
 */
const char* F_get_executor_error(const Stru_BytecodeExecutor_t* executor);

/**
 * @brief 重置执行器
 * 
 * 重置字节码执行器到初始状态。
 * 
 * @param executor 字节码执行器
 */
void F_reset_executor(Stru_BytecodeExecutor_t* executor);

#ifdef __cplusplus
}
#endif

#endif /* CN_BYTECODE_EXECUTOR_H */
