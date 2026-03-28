#ifndef CNLANG_RUNTIME_SYSTEM_API_H
#define CNLANG_RUNTIME_SYSTEM_API_H

/*
 * CN Language 系统编程 API
 * 
 * 本文件提供与已删除关键字对应的运行时函数别名
 * 这些函数替代了原先的语言级关键字,提供更灵活的系统编程能力
 * 
 * 对应关系:
 * - 读取内存    -> cn_rt_mem_read()
 * - 写入内存    -> cn_rt_mem_write()
 * - 内存复制    -> cn_rt_mem_copy()
 * - 内存设置    -> cn_rt_mem_set()
 * - 映射内存    -> cn_rt_mem_map()
 * - 解除映射    -> cn_rt_mem_unmap()
 * - 内联汇编    -> cn_rt_inline_asm()
 * - 中断处理    -> cn_rt_interrupt_register()
 */

#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/interrupt.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 内存操作 API
// 替代已删除的内存操作关键字
// =============================================================================

// 读取内存 - 替代"读取内存"关键字
// 参数: addr - 内存地址, size - 读取大小(1/2/4/8字节)
// 返回: 读取的值
static inline uintptr_t cn_rt_mem_read(uintptr_t addr, size_t size) {
    return cn_rt_memory_read_safe(addr, size);
}

// 写入内存 - 替代"写入内存"关键字
// 参数: addr - 内存地址, value - 写入的值, size - 写入大小(1/2/4/8字节)
static inline void cn_rt_mem_write(uintptr_t addr, uintptr_t value, size_t size) {
    cn_rt_memory_write_safe(addr, value, size);
}

// 内存复制 - 替代"内存复制"关键字
// 参数: dest - 目标地址, src - 源地址, size - 复制大小
static inline void cn_rt_mem_copy(void* dest, const void* src, size_t size) {
    cn_rt_memory_copy_safe(dest, src, size);
}

// 内存设置 - 替代"内存设置"关键字
// 参数: addr - 内存地址, value - 填充值, size - 设置大小
static inline void cn_rt_mem_set(void* addr, int value, size_t size) {
    cn_rt_memory_set_safe(addr, value, size);
}

// 映射内存 - 替代"映射内存"关键字
// 参数: addr - 建议地址, size - 映射大小, prot - 保护标志, flags - 映射标志
// 返回: 映射后的地址,失败返回NULL
static inline void* cn_rt_mem_map(void* addr, size_t size, int prot, int flags) {
    return cn_rt_memory_map_safe(addr, size, prot, flags);
}

// 解除映射 - 替代"解除映射"关键字
// 参数: addr - 映射地址, size - 映射大小
// 返回: 0表示成功,-1表示失败
static inline int cn_rt_mem_unmap(void* addr, size_t size) {
    return cn_rt_memory_unmap_safe(addr, size);
}

// =============================================================================
// 内联汇编 API
// 替代已删除的"内联汇编"关键字
// =============================================================================

// 内联汇编执行 - 替代"内联汇编"关键字
// 
// 注意: 这是一个占位实现,实际的内联汇编需要编译器支持
// 在C代码生成时,可以直接生成对应平台的asm语句
// 
// 参数:
//   asm_code - 汇编代码字符串
//   outputs - 输出操作数(格式依赖于平台)
//   inputs - 输入操作数(格式依赖于平台)
//   clobbers - 破坏的寄存器列表
// 
// 返回: 0表示成功,-1表示失败
//
// 示例(在CN语言中):
//   cn_rt_inline_asm("mov eax, 1", NULL, NULL, NULL);
//
static inline int cn_rt_inline_asm(
    const char* asm_code,
    const char* outputs,
    const char* inputs,
    const char* clobbers
) {
    // 在实际使用中,后端会将此函数调用转换为平台特定的内联汇编
    // 这里提供一个占位实现用于运行时检查
    (void)asm_code;
    (void)outputs;
    (void)inputs;
    (void)clobbers;
    
    // 在支持GCC内联汇编的平台上,可以这样实现:
    #if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
        // 示例: 简单执行汇编代码
        // 注意: 这是一个简化版本,完整版需要解析outputs/inputs/clobbers
        if (asm_code != NULL) {
            // __asm__ volatile (asm_code : : : );
            // 由于无法直接使用变量作为asm参数,这里返回错误
            return -1;
        }
    #endif
    
    return -1; // 当前平台不支持或参数无效
}

// =============================================================================
// 中断处理 API  
// 替代已删除的"中断处理"关键字
// =============================================================================

// 注册中断处理程序 - 替代"中断处理"关键字
// 
// 原语法: 中断处理 向量号 () { ... }
// 新方式: 先定义处理函数,再调用cn_rt_interrupt_register注册
//
// 参数:
//   vector_num - 中断向量号
//   handler - 中断处理函数
//   name - 中断名称(用于调试)
//
// 返回: 0表示成功,负数表示失败
//   -1: 向量号超出范围
//   -2: 无效的处理程序
//   -3: 已存在中断处理程序
//
// 示例(在CN语言中):
//   函数 timer_isr() { ... }
//   cn_rt_interrupt_register(0, timer_isr, "定时器");
//
// 注: 函数指针已经由cn_rt_interrupt_register提供,这里无需额外封装

// =============================================================================
// 辅助宏定义
// =============================================================================

// 内存保护标志(用于内存映射)
#ifndef PROT_NONE
#define PROT_NONE  0x0  // 页不可访问
#define PROT_READ  0x1  // 页可读
#define PROT_WRITE 0x2  // 页可写
#define PROT_EXEC  0x4  // 页可执行
#endif

// 映射标志
#ifndef MAP_SHARED
#define MAP_SHARED    0x01  // 共享映射
#define MAP_PRIVATE   0x02  // 私有映射
#define MAP_ANONYMOUS 0x20  // 匿名映射
#endif

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

#define 读取内存 cn_rt_mem_read
#define 写入内存 cn_rt_mem_write
#define 内存复制 cn_rt_mem_copy
#define 内存设置 cn_rt_mem_set
#define 映射内存 cn_rt_mem_map
#define 解除映射 cn_rt_mem_unmap
#define 内联汇编 cn_rt_inline_asm

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_SYSTEM_API_H */
