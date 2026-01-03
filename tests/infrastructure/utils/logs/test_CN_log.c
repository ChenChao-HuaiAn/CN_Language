/******************************************************************************
 * 文件名: test_CN_log.c
 * 功能: CN_Language日志系统测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026-01-03
 * 修改历史:
 *  2026-01-03: 创建文件，测试CN_log模块功能
 * 版权: MIT许可证
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 包含日志系统头文件 */
#include "src/infrastructure/utils/logs/CN_log.h"

/* 测试文件配置 */
#define TEST_LOG_FILE "test_log_output.log"

/* 测试辅助函数 */
static bool test_file_exists(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file)
    {
        fclose(file);
        return true;
    }
    return false;
}

static size_t test_get_file_size(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fclose(file);
    
    return size;
}

static void print_test_result(const char* test_name, bool passed)
{
    printf("  %s: %s\n", test_name, passed ? "✓ 通过" : "✗ 失败");
}

/* 主测试函数 */
int main(void)
{
    printf("=== CN_Language 日志系统测试 ===\n\n");
    
    bool all_tests_passed = true;
    
    /* 测试1: 基本初始化和关闭 */
    printf("1. 测试日志系统初始化和关闭:\n");
    {
        bool test_passed = true;
        
        /* 使用默认配置初始化 */
        if (!CN_log_init(NULL))
        {
            printf("  错误: 使用默认配置初始化失败\n");
            test_passed = false;
        }
        
        /* 添加控制台输出 */
        if (!CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL))
        {
            printf("  错误: 添加控制台输出失败\n");
            test_passed = false;
        }
        
        /* 记录测试日志 */
        CN_LOG_INFO("测试日志系统初始化和关闭");
        
        /* 关闭日志系统 */
        CN_log_shutdown();
        
        print_test_result("基本初始化和关闭", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 测试2: 日志级别 */
    printf("\n2. 测试日志级别功能:\n");
    {
        bool test_passed = true;
        
        /* 初始化 */
        if (!CN_log_init(NULL))
        {
            printf("  错误: 初始化失败\n");
            test_passed = false;
        }
        else
        {
            CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
            
            /* 测试默认级别 */
            Eum_CN_LogLevel_t level = CN_log_get_level();
            if (level != Eum_LOG_LEVEL_INFO)
            {
                printf("  错误: 默认日志级别不正确\n");
                test_passed = false;
            }
            
            /* 测试设置级别 */
            CN_log_set_level(Eum_LOG_LEVEL_DEBUG);
            level = CN_log_get_level();
            if (level != Eum_LOG_LEVEL_DEBUG)
            {
                printf("  错误: 设置日志级别失败\n");
                test_passed = false;
            }
            
            /* 测试不同级别日志 */
            printf("  记录不同级别的日志:\n");
            CN_log_set_level(Eum_LOG_LEVEL_TRACE);
            CN_LOG_TRACE("TRACE级别日志");
            CN_LOG_DEBUG("DEBUG级别日志");
            CN_LOG_INFO("INFO级别日志");
            CN_LOG_WARN("WARN级别日志");
            CN_LOG_ERROR("ERROR级别日志");
            CN_LOG_FATAL("FATAL级别日志");
            
            CN_log_shutdown();
        }
        
        print_test_result("日志级别功能", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 测试3: 日志宏 */
    printf("\n3. 测试日志宏:\n");
    {
        bool test_passed = true;
        
        if (!CN_log_init(NULL))
        {
            printf("  错误: 初始化失败\n");
            test_passed = false;
        }
        else
        {
            CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
            CN_log_set_level(Eum_LOG_LEVEL_DEBUG);
            
            printf("  测试各种日志宏:\n");
            int value = 42;
            const char* text = "测试文本";
            
            CN_LOG_TRACE("TRACE宏: 值=%d, 文本=%s", value, text);
            CN_LOG_DEBUG("DEBUG宏: 值=%d", value);
            CN_LOG_INFO("INFO宏: 文本=%s", text);
            CN_LOG_WARN("WARN宏: 警告值=%d", value);
            CN_LOG_ERROR("ERROR宏: 错误文本=%s", text);
            CN_LOG_FATAL("FATAL宏: 致命错误");
            
            CN_log_shutdown();
        }
        
        print_test_result("日志宏", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 测试4: 文件输出 */
    printf("\n4. 测试文件输出:\n");
    {
        bool test_passed = true;
        
        /* 清理可能存在的测试文件 */
        remove(TEST_LOG_FILE);
        
        if (!CN_log_init(NULL))
        {
            printf("  错误: 初始化失败\n");
            test_passed = false;
        }
        else
        {
            /* 添加文件输出 */
            if (!CN_log_add_output(Eum_LOG_OUTPUT_FILE, TEST_LOG_FILE))
            {
                printf("  错误: 添加文件输出失败\n");
                test_passed = false;
            }
            else
            {
                /* 添加控制台输出以便查看 */
                CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
                
                /* 记录测试日志 */
                const char* test_message = "文件输出测试消息";
                CN_LOG_INFO("%s", test_message);
                CN_LOG_WARN("警告: 测试文件输出");
                
                /* 刷新输出 */
                CN_log_flush_all();
                
                /* 验证文件 */
                if (!test_file_exists(TEST_LOG_FILE))
                {
                    printf("  错误: 日志文件不存在\n");
                    test_passed = false;
                }
                else
                {
                    size_t file_size = test_get_file_size(TEST_LOG_FILE);
                    if (file_size == 0)
                    {
                        printf("  错误: 日志文件为空\n");
                        test_passed = false;
                    }
                    else
                    {
                        printf("  日志文件创建成功，大小: %zu 字节\n", file_size);
                    }
                }
            }
            
            CN_log_shutdown();
        }
        
        /* 清理测试文件 */
        remove(TEST_LOG_FILE);
        
        print_test_result("文件输出", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 测试5: 格式化器 */
    printf("\n5. 测试格式化器:\n");
    {
        bool test_passed = true;
        
        if (!CN_log_init(NULL))
        {
            printf("  错误: 初始化失败\n");
            test_passed = false;
        }
        else
        {
            CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
            
            printf("  测试默认格式化器:\n");
            CN_LOG_INFO("使用默认格式化器");
            
            /* 测试切换到简单格式化器 */
            if (CN_log_set_formatter("simple"))
            {
                printf("  使用简单格式化器:\n");
                CN_LOG_INFO("使用简单格式化器");
            }
            else
            {
                printf("  警告: 简单格式化器不可用\n");
            }
            
            /* 测试切换到JSON格式化器 */
            if (CN_log_set_formatter("json"))
            {
                printf("  使用JSON格式化器:\n");
                CN_LOG_INFO("使用JSON格式化器");
            }
            else
            {
                printf("  警告: JSON格式化器不可用\n");
            }
            
            /* 切换回默认 */
            CN_log_set_formatter("default");
            
            CN_log_shutdown();
        }
        
        print_test_result("格式化器", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 测试6: 配置管理 */
    printf("\n6. 测试配置管理:\n");
    {
        bool test_passed = true;
        
        /* 使用自定义配置 */
        Stru_CN_LogConfig_t config = {
            .default_level = Eum_LOG_LEVEL_DEBUG,
            .thread_safe = true,
            .async_mode = false,
            .enable_colors = true
        };
        
        if (!CN_log_init(&config))
        {
            printf("  错误: 自定义配置初始化失败\n");
            test_passed = false;
        }
        else
        {
            CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
            
            /* 获取配置 */
            Stru_CN_LogConfig_t current_config = CN_log_get_config();
            if (current_config.default_level != Eum_LOG_LEVEL_DEBUG)
            {
                printf("  错误: 配置default_level不正确\n");
                test_passed = false;
            }
            if (!current_config.thread_safe)
            {
                printf("  错误: 配置thread_safe不正确\n");
                test_passed = false;
            }
            if (!current_config.enable_colors)
            {
                printf("  错误: 配置enable_colors不正确\n");
                test_passed = false;
            }
            
            printf("  自定义配置下的日志:\n");
            CN_LOG_INFO("自定义配置测试日志");
            
            CN_log_shutdown();
        }
        
        print_test_result("配置管理", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 测试7: 性能优化功能 */
    printf("\n7. 测试性能优化功能:\n");
    {
        bool test_passed = true;
        
        if (!CN_log_init(NULL))
        {
            printf("  错误: 初始化失败\n");
            test_passed = false;
        }
        else
        {
            CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
            
            /* 测试启用/禁用 */
            if (!CN_log_is_enabled())
            {
                printf("  错误: 日志默认未启用\n");
                test_passed = false;
            }
            
            CN_log_set_enabled(false);
            if (CN_log_is_enabled())
            {
                printf("  错误: 日志禁用失败\n");
                test_passed = false;
            }
            
            CN_log_set_enabled(true);
            if (!CN_log_is_enabled())
            {
                printf("  错误: 日志启用失败\n");
                test_passed = false;
            }
            
            /* 测试统计信息 */
            size_t total, filtered, failed;
            CN_log_get_stats(&total, &filtered, &failed);
            printf("  初始统计: 总数=%zu, 过滤=%zu, 失败=%zu\n", total, filtered, failed);
            
            /* 记录一些日志 */
            CN_LOG_INFO("性能测试日志1");
            CN_LOG_INFO("性能测试日志2");
            
            CN_log_get_stats(&total, &filtered, &failed);
            printf("  更新后统计: 总数=%zu, 过滤=%zu, 失败=%zu\n", total, filtered, failed);
            
            /* 测试刷新 */
            CN_log_flush_all();
            
            CN_log_shutdown();
        }
        
        print_test_result("性能优化功能", test_passed);
        if (!test_passed) all_tests_passed = false;
    }
    
    /* 总结 */
    printf("\n=== 测试总结 ===\n");
    if (all_tests_passed)
    {
        printf("所有测试通过! ✓\n");
    }
    else
    {
        printf("部分测试失败! ✗\n");
    }
    
    return all_tests_passed ? 0 : 1;
}
