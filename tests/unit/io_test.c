/*
 * CN_Language I/O系统单元测试
 * 测试文件操作、标准输入输出和缓冲区管理功能
 */

#include "cnlang/runtime/stdlib.h"
#include "cnlang/runtime/io.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return 0; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("运行测试: %s ... ", #test_func); \
        if (test_func()) { \
            printf("通过\n"); \
            passed++; \
        } else { \
            printf("失败\n"); \
            failed++; \
        } \
        total++; \
    } while(0)

// 测试文件写入和读取
int 测试文件写入读取(void)
{
    const char* 测试文件 = "test_file_io.txt";
    const char* 测试内容 = "Hello CN Language!";
    
    // 写入测试
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "打开文件用于写入失败");
    
    size_t 写入字节 = 写入文件(文件, 测试内容, 获取字符串长度(测试内容));
    TEST_ASSERT(写入字节 == 获取字符串长度(测试内容), "写入字节数不匹配");
    
    关闭文件(文件);
    
    // 读取测试
    文件 = 打开文件(测试文件, "r");
    TEST_ASSERT(文件 != NULL, "打开文件用于读取失败");
    
    char 缓冲区[256];
    size_t 读取字节 = 读取文件(文件, 缓冲区, sizeof(缓冲区) - 1);
    缓冲区[读取字节] = '\0';
    
    TEST_ASSERT(读取字节 == 获取字符串长度(测试内容), "读取字节数不匹配");
    TEST_ASSERT(比较字符串(缓冲区, 测试内容) == 0, "读取内容不匹配");
    
    关闭文件(文件);
    
    return 1;
}

// 测试文件定位
int 测试文件定位(void)
{
    const char* 测试文件 = "test_seek.txt";
    const char* 内容 = "0123456789ABCDEF";
    
    // 写入测试数据
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    写入文件(文件, 内容, 获取字符串长度(内容));
    关闭文件(文件);
    
    // 测试定位
    文件 = 打开文件(测试文件, "r");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    
    // 获取文件大小
    文件定位(文件, 0, 2);  // SEEK_END
    long 文件大小 = 获取文件位置(文件);
    TEST_ASSERT(文件大小 == (long)获取字符串长度(内容), "文件大小不正确");
    
    // 定位到开头
    文件定位(文件, 0, 0);  // SEEK_SET
    TEST_ASSERT(获取文件位置(文件) == 0, "定位到开头失败");
    
    // 定位到中间
    文件定位(文件, 5, 0);
    char 字符;
    读取文件(文件, &字符, 1);
    TEST_ASSERT(字符 == '5', "定位到中间后读取字符不正确");
    
    关闭文件(文件);
    
    return 1;
}

// 测试文件追加
int 测试文件追加(void)
{
    const char* 测试文件 = "test_append.txt";
    
    // 第一次写入
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    const char* 第一行 = "第一行\n";
    写入文件(文件, 第一行, 获取字符串长度(第一行));
    关闭文件(文件);
    
    // 追加写入
    文件 = 打开文件(测试文件, "a");
    TEST_ASSERT(文件 != NULL, "以追加模式打开文件失败");
    const char* 第二行 = "第二行\n";
    写入文件(文件, 第二行, 获取字符串长度(第二行));
    关闭文件(文件);
    
    // 验证内容
    文件 = 打开文件(测试文件, "r");
    char 缓冲区[256];
    size_t 读取 = 读取文件(文件, 缓冲区, sizeof(缓冲区) - 1);
    缓冲区[读取] = '\0';
    
    TEST_ASSERT(strstr(缓冲区, "第一行") != NULL, "找不到第一行内容");
    TEST_ASSERT(strstr(缓冲区, "第二行") != NULL, "找不到第二行内容");
    
    关闭文件(文件);
    
    return 1;
}

// 测试文件EOF检测
int 测试文件EOF(void)
{
    const char* 测试文件 = "test_eof.txt";
    const char* 内容 = "短文本";
    
    // 写入短文本
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    写入文件(文件, 内容, 获取字符串长度(内容));
    关闭文件(文件);
    
    // 读取并检测EOF
    文件 = 打开文件(测试文件, "r");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    
    char 缓冲区[256];
    读取文件(文件, 缓冲区, sizeof(缓冲区));
    
    // 读取后应该到达EOF
    TEST_ASSERT(判断文件结束(文件) != 0, "应该检测到EOF");
    
    关闭文件(文件);
    
    return 1;
}

// 测试缓冲区设置
int 测试缓冲区设置(void)
{
    const char* 测试文件 = "test_buffer.txt";
    
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    
    // 设置无缓冲
    int 结果 = cn_rt_file_setbuf((CnRtFile)文件, NULL, 2, 0);
    TEST_ASSERT(结果 == 0, "设置无缓冲失败");
    
    // 写入数据
    const char* 数据 = "测试无缓冲写入\n";
    写入文件(文件, 数据, 获取字符串长度(数据));
    
    关闭文件(文件);
    
    // 验证数据已写入
    文件 = 打开文件(测试文件, "r");
    char 缓冲区[256];
    size_t 读取 = 读取文件(文件, 缓冲区, sizeof(缓冲区) - 1);
    缓冲区[读取] = '\0';
    
    TEST_ASSERT(比较字符串(缓冲区, 数据) == 0, "无缓冲写入内容不匹配");
    
    关闭文件(文件);
    
    return 1;
}

// 测试缓冲区刷新
int 测试缓冲区刷新(void)
{
    const char* 测试文件 = "test_flush.txt";
    
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "打开文件失败");
    
    const char* 数据 = "测试刷新";
    写入文件(文件, 数据, 获取字符串长度(数据));
    
    // 显式刷新
    int 结果 = 刷新文件缓冲(文件);
    TEST_ASSERT(结果 == 0, "刷新文件缓冲失败");
    
    关闭文件(文件);
    
    return 1;
}

// 测试格式化字符串
int 测试格式化字符串(void)
{
    char 缓冲区[256];
    
    // 基本格式化
    int 结果 = 格式化字符串(缓冲区, "数字: %d", 42);
    TEST_ASSERT(结果 > 0, "格式化字符串失败");
    TEST_ASSERT(strstr(缓冲区, "42") != NULL, "格式化结果不包含数字");
    
    // 安全格式化
    char 小缓冲[10];
    结果 = 安全格式化字符串(小缓冲, sizeof(小缓冲), "很长的字符串测试");
    TEST_ASSERT(结果 >= 0, "安全格式化失败");
    
    return 1;
}

// 测试中文接口包装
int 测试中文接口(void)
{
    const char* 测试文件 = "test_chinese_api.txt";
    
    // 使用中文接口进行文件操作
    void* 文件 = 打开文件(测试文件, "w");
    TEST_ASSERT(文件 != NULL, "中文接口打开文件失败");
    
    const char* 测试数据 = "中文接口测试";
    size_t 写入 = 写入文件(文件, 测试数据, 获取字符串长度(测试数据));
    TEST_ASSERT(写入 > 0, "中文接口写入失败");
    
    刷新文件缓冲(文件);
    关闭文件(文件);
    
    // 读取验证
    文件 = 打开文件(测试文件, "r");
    char 缓冲区[256];
    size_t 读取 = 读取文件(文件, 缓冲区, sizeof(缓冲区) - 1);
    缓冲区[读取] = '\0';
    
    TEST_ASSERT(比较字符串(缓冲区, 测试数据) == 0, "中文接口读取数据不匹配");
    
    关闭文件(文件);
    
    return 1;
}

// 测试错误处理
int 测试错误处理(void)
{
    // 测试打开不存在的文件
    void* 文件 = 打开文件("不存在的文件.txt", "r");
    TEST_ASSERT(文件 == NULL, "应该无法打开不存在的文件");
    
    // 测试NULL参数
    文件 = 打开文件(NULL, "r");
    TEST_ASSERT(文件 == NULL, "NULL路径应该返回NULL");
    
    return 1;
}

// 主测试函数
int main(void)
{
    int total = 0;
    int passed = 0;
    int failed = 0;
    
    printf("\n========================================\n");
    printf("CN_Language I/O系统单元测试\n");
    printf("========================================\n\n");
    
    // 运行所有测试
    RUN_TEST(测试文件写入读取);
    RUN_TEST(测试文件定位);
    RUN_TEST(测试文件追加);
    RUN_TEST(测试文件EOF);
    RUN_TEST(测试缓冲区设置);
    RUN_TEST(测试缓冲区刷新);
    RUN_TEST(测试格式化字符串);
    RUN_TEST(测试中文接口);
    RUN_TEST(测试错误处理);
    
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总计: %d\n", total);
    printf("  通过: %d\n", passed);
    printf("  失败: %d\n", failed);
    printf("========================================\n\n");
    
    return failed == 0 ? 0 : 1;
}
