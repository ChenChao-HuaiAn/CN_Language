/******************************************************************************
 * 文件名: test_debug.c
 * 功能: CN_debug调试系统测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，测试调试系统功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "../src/debug/CN_debug.h"
#include <stdio.h>
#include <time.h>

int main()
{
    printf("=== CN_debug调试系统测试 ===\n\n");
    
    // 测试1: 基本调试级别设置
    printf("测试1: 基本调试级别设置\n");
    F_debug_init(Eum_DEBUG_LEVEL_DEBUG);
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "调试系统初始化完成");
    
    // 测试2: 不同级别的调试输出
    printf("\n测试2: 不同级别的调试输出\n");
    CN_DEBUG(Eum_DEBUG_LEVEL_ERROR, "这是一个错误信息: 错误代码 %d", 404);
    CN_DEBUG(Eum_DEBUG_LEVEL_WARN, "这是一个警告信息: 内存使用超过80%%");
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "这是一个普通信息: 程序启动成功");
    CN_DEBUG(Eum_DEBUG_LEVEL_DEBUG, "这是一个调试信息: 变量value = %d", 12345);
    
    // 测试3: 文件重定向
    printf("\n测试3: 文件重定向测试\n");
    F_debug_set_output_file("test_debug_output.log");
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "这条信息将写入到test_debug_output.log文件");
    CN_DEBUG(Eum_DEBUG_LEVEL_DEBUG, "调试信息也写入文件: timestamp=%ld", time(NULL));
    
    // 恢复到标准错误输出
    F_debug_set_output_file(NULL);
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "恢复到标准错误输出");
    
    // 测试4: 断言功能
    printf("\n测试4: 断言功能测试\n");
    int test_value = 100;
    CN_DEBUG(Eum_DEBUG_LEVEL_DEBUG, "测试断言: test_value = %d", test_value);
    CN_ASSERT(test_value > 0, "测试值必须大于0");
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "断言测试通过");
    
    // 测试5: 调试级别控制
    printf("\n测试5: 调试级别控制测试\n");
    F_debug_init(Eum_DEBUG_LEVEL_WARN);
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "这条信息不会显示（级别低于WARN）");
    CN_DEBUG(Eum_DEBUG_LEVEL_WARN, "这条警告信息会显示");
    CN_DEBUG(Eum_DEBUG_LEVEL_ERROR, "这条错误信息会显示");
    
    printf("\n=== 测试完成 ===\n");
    printf("请查看stderr输出和test_debug_output.log文件\n");
    
    return 0;
}
