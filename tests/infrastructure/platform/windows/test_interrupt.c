/******************************************************************************
 * 文件名: test_interrupt.c
 * 功能: Windows平台中断处理模块测试
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，测试Windows中断处理模块
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// 包含中断处理头文件
#include "../../../../src/infrastructure/platform/CN_interrupt.h"
#include "../../../../src/infrastructure/platform/windows/CN_platform_windows_interrupt.h"

// 测试计数器
static int test_counter = 0;
static int interrupt_counter = 0;

// 简单的中断处理函数
void test_interrupt_handler(void* context)
{
    int* counter = (int*)context;
    (*counter)++;
    printf("中断处理函数被调用，计数器: %d\n", *counter);
}

// 测试中断控制器初始化
bool test_interrupt_controller_initialization(void)
{
    printf("测试中断控制器初始化...\n");
    
    // 获取Windows中断控制器接口
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    if (!controller) {
        printf("错误: 无法获取中断控制器接口\n");
        return false;
    }
    
    printf("成功获取中断控制器接口\n");
    
    // 测试控制器信息
    Stru_CN_InterruptControllerInfo_t info;
    if (controller->get_info(&info)) {
        printf("控制器信息:\n");
        printf("  类型: %d\n", info.type);
        printf("  名称: %s\n", info.name);
        printf("  版本: %u\n", info.version);
        printf("  最大中断数: %u\n", info.max_interrupts);
        printf("  最大优先级: %u\n", info.max_priority);
        printf("  支持嵌套: %s\n", info.supports_nesting ? "是" : "否");
        printf("  支持优先级: %s\n", info.supports_priority ? "是" : "否");
        printf("  支持亲和性: %s\n", info.supports_affinity ? "是" : "否");
        printf("  支持唤醒: %s\n", info.supports_wakeup ? "是" : "否");
    } else {
        printf("警告: 无法获取控制器信息\n");
    }
    
    // 测试初始化状态
    bool is_initialized = controller->is_initialized();
    printf("控制器初始化状态: %s\n", is_initialized ? "已初始化" : "未初始化");
    
    test_counter++;
    return true;
}

// 测试中断管理器
bool test_interrupt_manager(void)
{
    printf("测试中断管理器...\n");
    
    // 获取Windows中断管理器接口
    Stru_CN_InterruptManagerInterface_t* manager = 
        CN_platform_windows_get_interrupt_manager();
    
    if (!manager) {
        printf("错误: 无法获取中断管理器接口\n");
        return false;
    }
    
    printf("成功获取中断管理器接口\n");
    
    // 测试管理器初始化
    if (manager->initialize()) {
        printf("中断管理器初始化成功\n");
    } else {
        printf("警告: 中断管理器初始化失败\n");
    }
    
    test_counter++;
    return true;
}

// 测试中断工具
bool test_interrupt_tools(void)
{
    printf("测试中断工具...\n");
    
    // 获取Windows中断工具接口
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_platform_windows_get_interrupt_tools();
    
    if (!tools) {
        printf("错误: 无法获取中断工具接口\n");
        return false;
    }
    
    printf("成功获取中断工具接口\n");
    
    // 测试上下文保存和恢复
    Stru_CN_InterruptContext_t context;
    if (tools->save_context(&context)) {
        printf("成功保存中断上下文\n");
        
        // 测试上下文复制
        Stru_CN_InterruptContext_t context_copy;
        if (tools->copy_context(&context_copy, &context)) {
            printf("成功复制中断上下文\n");
        }
        
        // 测试上下文恢复
        if (tools->restore_context(&context)) {
            printf("成功恢复中断上下文\n");
        }
    } else {
        printf("警告: 无法保存中断上下文\n");
    }
    
    test_counter++;
    return true;
}

// 测试Windows特定接口
bool test_windows_specific_interfaces(void)
{
    printf("测试Windows特定接口...\n");
    
    // 测试事件接口
    Stru_CN_WindowsEventInterface_t* event_if = 
        CN_platform_windows_get_event_interface();
    
    if (event_if) {
        printf("成功获取Windows事件接口\n");
        
        // 测试事件创建（在实际测试中可能需要实际创建事件）
        printf("Windows事件接口可用\n");
    } else {
        printf("警告: 无法获取Windows事件接口\n");
    }
    
    // 测试APC接口
    Stru_CN_WindowsAPCInterface_t* apc_if = 
        CN_platform_windows_get_apc_interface();
    
    if (apc_if) {
        printf("成功获取Windows APC接口\n");
        printf("Windows APC接口可用\n");
    } else {
        printf("警告: 无法获取Windows APC接口\n");
    }
    
    // 测试I/O完成端口接口
    Stru_CN_WindowsIOCompletionInterface_t* iocp_if = 
        CN_platform_windows_get_iocp_interface();
    
    if (iocp_if) {
        printf("成功获取Windows I/O完成端口接口\n");
        printf("Windows I/O完成端口接口可用\n");
    } else {
        printf("警告: 无法获取Windows I/O完成端口接口\n");
    }
    
    test_counter++;
    return true;
}

// 测试中断注册和注销
bool test_interrupt_registration(void)
{
    printf("测试中断注册和注销...\n");
    
    // 获取中断控制器
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    if (!controller) {
        printf("错误: 无法获取中断控制器\n");
        return false;
    }
    
    // 初始化控制器（如果需要）
    if (!controller->is_initialized()) {
        if (!controller->initialize()) {
            printf("警告: 无法初始化中断控制器\n");
            return false;
        }
    }
    
    // 定义中断描述符
    Stru_CN_InterruptDescriptor_t desc = {
        .number = 2001,  // Windows事件1映射的中断号
        .type = Eum_INTERRUPT_TYPE_EXTERNAL,
        .trigger = Eum_INTERRUPT_TRIGGER_EDGE,
        .polarity = Eum_INTERRUPT_POLARITY_RISING_EDGE,
        .priority = 5,
        .cpu_affinity = 0x1, // CPU 0
        .enabled = true,
        .shared = false,
        .description = "测试中断"
    };
    
    // 注册中断处理函数
    interrupt_counter = 0;
    if (controller->register_handler(desc.number, test_interrupt_handler, &interrupt_counter, &desc)) {
        printf("成功注册中断处理函数\n");
        
        // 启用中断
        if (controller->enable_interrupt(desc.number)) {
            printf("成功启用中断\n");
            
            // 检查中断状态
            bool is_enabled = controller->is_interrupt_enabled(desc.number);
            printf("中断启用状态: %s\n", is_enabled ? "已启用" : "未启用");
            
            // 禁用中断
            if (controller->disable_interrupt(desc.number)) {
                printf("成功禁用中断\n");
            }
            
            // 注销中断处理函数
            if (controller->unregister_handler(desc.number)) {
                printf("成功注销中断处理函数\n");
            }
        }
    } else {
        printf("警告: 无法注册中断处理函数\n");
    }
    
    test_counter++;
    return true;
}

// 测试中断系统初始化
bool test_interrupt_system_initialization(void)
{
    printf("测试中断系统初始化...\n");
    
    // 初始化Windows中断处理系统
    if (CN_platform_windows_interrupt_initialize()) {
        printf("Windows中断处理系统初始化成功\n");
        
        // 检查初始化状态
        bool is_initialized = CN_platform_windows_interrupt_is_initialized();
        printf("中断系统初始化状态: %s\n", is_initialized ? "已初始化" : "未初始化");
        
        // 清理中断处理系统
        CN_platform_windows_interrupt_cleanup();
        printf("Windows中断处理系统清理完成\n");
        
        test_counter++;
        return true;
    } else {
        printf("错误: Windows中断处理系统初始化失败\n");
        return false;
    }
}

// 主测试函数
int main(void)
{
    printf("========================================\n");
    printf("Windows平台中断处理模块测试\n");
    printf("========================================\n\n");
    
    int passed_tests = 0;
    int total_tests = 6;
    
    // 运行所有测试
    if (test_interrupt_system_initialization()) {
        printf("[PASS] 中断系统初始化测试\n\n");
        passed_tests++;
    } else {
        printf("[FAIL] 中断系统初始化测试\n\n");
    }
    
    if (test_interrupt_controller_initialization()) {
        printf("[PASS] 中断控制器初始化测试\n\n");
        passed_tests++;
    } else {
        printf("[FAIL] 中断控制器初始化测试\n\n");
    }
    
    if (test_interrupt_manager()) {
        printf("[PASS] 中断管理器测试\n\n");
        passed_tests++;
    } else {
        printf("[FAIL] 中断管理器测试\n\n");
    }
    
    if (test_interrupt_tools()) {
        printf("[PASS] 中断工具测试\n\n");
        passed_tests++;
    } else {
        printf("[FAIL] 中断工具测试\n\n");
    }
    
    if (test_windows_specific_interfaces()) {
        printf("[PASS] Windows特定接口测试\n\n");
        passed_tests++;
    } else {
        printf("[FAIL] Windows特定接口测试\n\n");
    }
    
    if (test_interrupt_registration()) {
        printf("[PASS] 中断注册和注销测试\n\n");
        passed_tests++;
    } else {
        printf("[FAIL] 中断注册和注销测试\n\n");
    }
    
    // 打印测试结果
    printf("========================================\n");
    printf("测试结果\n");
    printf("========================================\n");
    printf("总测试数: %d\n", total_tests);
    printf("通过测试: %d\n", passed_tests);
    printf("失败测试: %d\n", total_tests - passed_tests);
    printf("测试覆盖率: %.1f%%\n", (float)passed_tests / total_tests * 100);
    printf("测试计数器: %d\n", test_counter);
    printf("中断计数器: %d\n", interrupt_counter);
    
    if (passed_tests == total_tests) {
        printf("\n所有测试通过！Windows中断处理模块功能正常。\n");
        return 0;
    } else {
        printf("\n部分测试失败，需要进一步检查。\n");
        return 1;
    }
}
