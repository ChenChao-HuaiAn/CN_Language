/*
 * CN Language 运行时系统API测试
 * 
 * 测试替代已删除关键字的运行时函数:
 * - 内存操作: cn_rt_mem_read/write/copy/set
 * - 中断处理: cn_rt_interrupt_register
 * - 内联汇编: cn_rt_inline_asm (占位测试)
 */

#include "cnlang/runtime/system_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 测试结果统计
static int g_tests_run = 0;
static int g_tests_passed = 0;

// 测试宏
#define TEST_START(name) \
    do { \
        g_tests_run++; \
        printf("运行测试: %s ... ", name);

#define TEST_END() \
        g_tests_passed++; \
        printf("通过\n"); \
    } while(0)

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            printf("失败\n"); \
            printf("  期望: %llu\n", (unsigned long long)(expected)); \
            printf("  实际: %llu\n", (unsigned long long)(actual)); \
            printf("  位置: %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("失败\n"); \
            printf("  条件为假: %s\n", #cond); \
            printf("  位置: %s:%d\n", __FILE__, __LINE__); \
            return -1; \
        } \
    } while(0)

// =============================================================================
// 内存操作测试
// =============================================================================

// 测试内存读写
static int test_mem_read_write(void)
{
    TEST_START("cn_rt_mem_read/write");
    
    // 创建测试数据
    uint32_t test_data = 0x12345678;
    uintptr_t addr = (uintptr_t)&test_data;
    
    // 测试读取
    uint32_t value = (uint32_t)cn_rt_mem_read(addr, 4);
    ASSERT_EQ(value, 0x12345678);
    
    // 测试写入
    cn_rt_mem_write(addr, 0xABCDEF00, 4);
    ASSERT_EQ(test_data, 0xABCDEF00);
    
    // 测试不同大小的读写
    uint8_t byte_data = 0x42;
    addr = (uintptr_t)&byte_data;
    
    uint8_t read_byte = (uint8_t)cn_rt_mem_read(addr, 1);
    ASSERT_EQ(read_byte, 0x42);
    
    cn_rt_mem_write(addr, 0xFF, 1);
    ASSERT_EQ(byte_data, 0xFF);
    
    TEST_END();
    return 0;
}

// 测试内存复制
static int test_mem_copy(void)
{
    TEST_START("cn_rt_mem_copy");
    
    char src[20] = "Hello, CN Language!";
    char dest[20] = {0};
    
    // 复制内存
    cn_rt_mem_copy(dest, src, sizeof(src));
    
    // 验证复制结果
    ASSERT_TRUE(strcmp(dest, "Hello, CN Language!") == 0);
    
    TEST_END();
    return 0;
}

// 测试内存设置
static int test_mem_set(void)
{
    TEST_START("cn_rt_mem_set");
    
    char buffer[10];
    
    // 设置内存
    cn_rt_mem_set(buffer, 0xAA, sizeof(buffer));
    
    // 验证所有字节都被设置
    for (size_t i = 0; i < sizeof(buffer); i++) {
        ASSERT_EQ((unsigned char)buffer[i], 0xAA);
    }
    
    TEST_END();
    return 0;
}

// 测试内存映射(仅在非Windows平台)
static int test_mem_map_unmap(void)
{
    TEST_START("cn_rt_mem_map/unmap");
    
    #if !defined(_WIN32) && !defined(_WIN64)
        // 映射一页匿名内存
        size_t page_size = 4096;
        void* mapped = cn_rt_mem_map(NULL, page_size, 
                                      PROT_READ | PROT_WRITE, 
                                      MAP_PRIVATE | MAP_ANONYMOUS);
        
        if (mapped != NULL) {
            // 写入数据验证可用性
            char* ptr = (char*)mapped;
            ptr[0] = 'A';
            ptr[page_size - 1] = 'Z';
            
            ASSERT_EQ(ptr[0], 'A');
            ASSERT_EQ(ptr[page_size - 1], 'Z');
            
            // 解除映射
            int result = cn_rt_mem_unmap(mapped, page_size);
            ASSERT_EQ(result, 0);
        } else {
            printf("跳过(平台不支持) ");
        }
    #else
        printf("跳过(Windows平台) ");
    #endif
    
    TEST_END();
    return 0;
}

// =============================================================================
// 中断处理测试
// =============================================================================

// 测试中断处理程序标志
static volatile int g_interrupt_handled = 0;
static volatile uint32_t g_interrupt_vector = 0;

// 测试中断处理程序
static void test_interrupt_handler(void)
{
    g_interrupt_handled = 1;
    g_interrupt_vector = cn_rt_interrupt_get_current();
}

// 测试中断注册和触发
static int test_interrupt_register(void)
{
    TEST_START("cn_rt_interrupt_register");
    
    // 初始化中断系统
    cn_rt_interrupt_init();
    
    // 注册中断处理程序
    int result = cn_rt_interrupt_register(CN_RT_IRQ_TIMER_0, 
                                          test_interrupt_handler, 
                                          "测试定时器");
    ASSERT_EQ(result, 0);
    
    // 启用中断
    cn_rt_interrupt_enable(CN_RT_IRQ_TIMER_0);
    cn_rt_interrupt_enable_all();
    
    // 触发中断
    g_interrupt_handled = 0;
    g_interrupt_vector = 0;
    cn_rt_interrupt_trigger(CN_RT_IRQ_TIMER_0);
    
    // 验证中断已处理
    ASSERT_EQ(g_interrupt_handled, 1);
    ASSERT_EQ(g_interrupt_vector, CN_RT_IRQ_TIMER_0);
    
    // 注销中断
    result = cn_rt_interrupt_unregister(CN_RT_IRQ_TIMER_0);
    ASSERT_EQ(result, 0);
    
    TEST_END();
    return 0;
}

// 测试中断使能/禁用
static int test_interrupt_enable_disable(void)
{
    TEST_START("cn_rt_interrupt_enable/disable");
    
    // 初始化
    cn_rt_interrupt_init();
    cn_rt_interrupt_register(CN_RT_IRQ_TIMER_1, 
                             test_interrupt_handler, 
                             "测试定时器1");
    
    // 测试禁用状态下不触发中断
    cn_rt_interrupt_disable(CN_RT_IRQ_TIMER_1);
    g_interrupt_handled = 0;
    cn_rt_interrupt_trigger(CN_RT_IRQ_TIMER_1);
    ASSERT_EQ(g_interrupt_handled, 0);
    
    // 测试启用后可以触发中断
    cn_rt_interrupt_enable(CN_RT_IRQ_TIMER_1);
    cn_rt_interrupt_trigger(CN_RT_IRQ_TIMER_1);
    ASSERT_EQ(g_interrupt_handled, 1);
    
    // 测试全局禁用
    cn_rt_interrupt_disable_all();
    g_interrupt_handled = 0;
    cn_rt_interrupt_trigger(CN_RT_IRQ_TIMER_1);
    ASSERT_EQ(g_interrupt_handled, 0);
    
    TEST_END();
    return 0;
}

// 测试中断状态查询
static int test_interrupt_state_query(void)
{
    TEST_START("cn_rt_interrupt_state_query");
    
    // 初始化
    cn_rt_interrupt_init();
    
    // 测试初始状态
    ASSERT_TRUE(!cn_rt_interrupt_is_enabled(CN_RT_IRQ_KEYBOARD));
    ASSERT_TRUE(!cn_rt_interrupt_is_pending(CN_RT_IRQ_KEYBOARD));
    
    // 启用后检查状态
    cn_rt_interrupt_enable(CN_RT_IRQ_KEYBOARD);
    ASSERT_TRUE(cn_rt_interrupt_is_enabled(CN_RT_IRQ_KEYBOARD));
    
    // 获取中断状态结构
    const CnRtInterruptState* state = cn_rt_interrupt_get_state();
    ASSERT_TRUE(state != NULL);
    ASSERT_TRUE(state->global_enabled);
    
    TEST_END();
    return 0;
}

// =============================================================================
// 内联汇编测试
// =============================================================================

// 测试内联汇编占位实现
static int test_inline_asm(void)
{
    TEST_START("cn_rt_inline_asm");
    
    // 测试占位实现(应该返回失败)
    int result = cn_rt_inline_asm("nop", NULL, NULL, NULL);
    ASSERT_EQ(result, -1); // 当前实现返回-1
    
    printf("跳过(占位实现) ");
    
    TEST_END();
    return 0;
}

// =============================================================================
// 主测试函数
// =============================================================================

int main(void)
{
    printf("========================================\n");
    printf("CN Language 运行时系统API测试\n");
    printf("========================================\n\n");
    
    int result = 0;
    
    // 内存操作测试
    printf("--- 内存操作测试 ---\n");
    if (test_mem_read_write() != 0) result = -1;
    if (test_mem_copy() != 0) result = -1;
    if (test_mem_set() != 0) result = -1;
    if (test_mem_map_unmap() != 0) result = -1;
    printf("\n");
    
    // 中断处理测试
    printf("--- 中断处理测试 ---\n");
    if (test_interrupt_register() != 0) result = -1;
    if (test_interrupt_enable_disable() != 0) result = -1;
    if (test_interrupt_state_query() != 0) result = -1;
    printf("\n");
    
    // 内联汇编测试
    printf("--- 内联汇编测试 ---\n");
    if (test_inline_asm() != 0) result = -1;
    printf("\n");
    
    // 输出测试结果
    printf("========================================\n");
    printf("测试结果: %d/%d 通过\n", g_tests_passed, g_tests_run);
    printf("========================================\n");
    
    if (result == 0 && g_tests_passed == g_tests_run) {
        printf("✅ 所有测试通过!\n");
        return 0;
    } else {
        printf("❌ 部分测试失败!\n");
        return 1;
    }
}
