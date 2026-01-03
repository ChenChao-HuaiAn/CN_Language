/******************************************************************************
 * 文件名: test_windows_platform.c
 * 功能: 测试Windows平台实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "src/infrastructure/platform/windows/CN_platform_windows.h"

int main(void)
{
    printf("=== 测试Windows平台实现 ===\n");
    
    // 测试平台检测
    printf("1. 平台检测测试:\n");
    printf("   是否为Windows平台: %s\n", CN_platform_is_windows() ? "是" : "否");
    printf("   平台类型: %s\n", CN_platform_get_type());
    printf("   是否为Linux平台: %s\n", CN_platform_is_linux() ? "是" : "否");
    printf("   是否为macOS平台: %s\n", CN_platform_is_macos() ? "是" : "否");
    printf("   是否为Unix-like平台: %s\n", CN_platform_is_unix() ? "是" : "否");
    
    // 测试平台初始化
    printf("\n2. 平台初始化测试:\n");
    bool init_result = CN_platform_initialize();
    printf("   平台初始化结果: %s\n", init_result ? "成功" : "失败");
    
    if (init_result)
    {
        // 测试获取平台接口
        printf("\n3. 获取平台接口测试:\n");
        Stru_CN_PlatformInterface_t* platform = CN_platform_get_default();
        if (platform)
        {
            printf("   成功获取平台接口\n");
            
            // 检查各个子系统接口
            if (platform->filesystem)
            {
                printf("   文件系统接口: 可用\n");
            }
            else
            {
                printf("   文件系统接口: 不可用\n");
            }
            
            if (platform->thread)
            {
                printf("   线程接口: 可用\n");
            }
            else
            {
                printf("   线程接口: 不可用\n");
            }
            
            if (platform->network)
            {
                printf("   网络接口: 可用\n");
            }
            else
            {
                printf("   网络接口: 不可用\n");
            }
            
            if (platform->time)
            {
                printf("   时间接口: 可用\n");
            }
            else
            {
                printf("   时间接口: 不可用\n");
            }
            
            if (platform->system)
            {
                printf("   系统接口: 可用\n");
            }
            else
            {
                printf("   系统接口: 不可用\n");
            }
        }
        else
        {
            printf("   获取平台接口失败\n");
        }
        
        // 测试Windows特定接口
        printf("\n4. Windows特定接口测试:\n");
        
        Stru_CN_WindowsRegistryInterface_t* registry = CN_platform_windows_get_registry();
        if (registry)
        {
            printf("   注册表接口: 可用\n");
        }
        else
        {
            printf("   注册表接口: 不可用\n");
        }
        
        Stru_CN_WindowsGUIInterface_t* gui = CN_platform_windows_get_gui();
        if (gui)
        {
            printf("   GUI接口: 可用\n");
        }
        else
        {
            printf("   GUI接口: 不可用\n");
        }
        
        Stru_CN_WindowsCOMInterface_t* com = CN_platform_windows_get_com();
        if (com)
        {
            printf("   COM接口: 可用\n");
        }
        else
        {
            printf("   COM接口: 不可用\n");
        }
        
        // 清理平台
        printf("\n5. 平台清理测试:\n");
        CN_platform_cleanup();
        printf("   平台清理完成\n");
    }
    
    printf("\n=== 测试完成 ===\n");
    return 0;
}
