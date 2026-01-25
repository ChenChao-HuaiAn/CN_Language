#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cnlang/runtime/stdlib.h"

/*
 * 标准库中文接口单元测试
 * 测试覆盖：内存管理、字符串处理、I/O函数、辅助函数
 */

// 测试计数器
static int tests_run = 0;
static int tests_passed = 0;

// 测试宏
#define TEST(name) \
    static void name(void); \
    static void name##_wrapper(void) { \
        printf("运行测试: %s\n", #name); \
        tests_run++; \
        name(); \
        tests_passed++; \
        printf("  ✓ %s 通过\n", #name); \
    } \
    static void name(void)

#define RUN_TEST(test) test##_wrapper()

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_EQ(a, b, message) \
    do { \
        if ((a) != (b)) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    期望: %lld, 实际: %lld\n", (long long)(b), (long long)(a)); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_STR_EQ(a, b, message) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    期望: \"%s\", 实际: \"%s\"\n", (b), (a)); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// =============================================================================
// 内存管理函数测试
// =============================================================================

TEST(test_分配内存)
{
    void* ptr = 分配内存(100);
    ASSERT(ptr != NULL, "分配内存应该成功");
    释放内存(ptr);
}

TEST(test_分配清零内存)
{
    int* arr = (int*)分配清零内存(10, sizeof(int));
    ASSERT(arr != NULL, "分配清零内存应该成功");
    
    // 验证内存已清零
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(arr[i], 0, "分配的内存应该初始化为0");
    }
    
    释放内存(arr);
}

TEST(test_重新分配内存)
{
    char* str = (char*)分配内存(10);
    ASSERT(str != NULL, "初始分配应该成功");
    
    strcpy(str, "Hello");
    
    // 重新分配更大空间
    str = (char*)重新分配内存(str, 20);
    ASSERT(str != NULL, "重新分配应该成功");
    ASSERT_STR_EQ(str, "Hello", "重新分配后内容应该保持");
    
    释放内存(str);
}

// =============================================================================
// 字符串处理函数测试
// =============================================================================

TEST(test_复制字符串)
{
    char dest[20];
    char* result = 复制字符串(dest, "你好世界");
    ASSERT(result == dest, "应该返回目标缓冲区");
    ASSERT_STR_EQ(dest, "你好世界", "字符串应该被正确复制");
}

TEST(test_限长复制字符串)
{
    char dest[20] = {0};
    限长复制字符串(dest, "Hello World", 5);
    ASSERT_EQ(dest[0], 'H', "前5个字符应该被复制");
    ASSERT_EQ(dest[4], 'o', "前5个字符应该被复制");
}

TEST(test_连接字符串)
{
    char dest[30] = "Hello";
    连接字符串(dest, " World");
    ASSERT_STR_EQ(dest, "Hello World", "字符串应该被正确连接");
}

TEST(test_限长连接字符串)
{
    char dest[30] = "Hello";
    限长连接字符串(dest, " World!!!", 6);
    ASSERT_STR_EQ(dest, "Hello World", "应该只连接前6个字符");
}

TEST(test_比较字符串)
{
    ASSERT_EQ(比较字符串("abc", "abc"), 0, "相同字符串应该返回0");
    ASSERT(比较字符串("abc", "abd") < 0, "abc应该小于abd");
    ASSERT(比较字符串("abd", "abc") > 0, "abd应该大于abc");
}

TEST(test_限长比较字符串)
{
    ASSERT_EQ(限长比较字符串("abcde", "abcfg", 3), 0, "前3个字符相同应该返回0");
    ASSERT(限长比较字符串("abcde", "abcfg", 4) < 0, "第4个字符不同应该有差异");
}

TEST(test_获取字符串长度)
{
    ASSERT_EQ(获取字符串长度("Hello"), 5, "应该返回正确的字符串长度");
    ASSERT_EQ(获取字符串长度(""), 0, "空字符串长度应该为0");
}

TEST(test_查找字符)
{
    const char* str = "Hello World";
    char* pos = 查找字符(str, 'o');
    ASSERT(pos != NULL, "应该找到字符");
    ASSERT_EQ(pos - str, 4, "应该返回第一个'o'的位置");
}

TEST(test_反向查找字符)
{
    const char* str = "Hello World";
    char* pos = 反向查找字符(str, 'o');
    ASSERT(pos != NULL, "应该找到字符");
    ASSERT_EQ(pos - str, 7, "应该返回最后一个'o'的位置");
}

TEST(test_查找子串)
{
    const char* str = "Hello World";
    char* pos = 查找子串(str, "World");
    ASSERT(pos != NULL, "应该找到子串");
    ASSERT_EQ(pos - str, 6, "应该返回子串的位置");
    
    pos = 查找子串(str, "xyz");
    ASSERT(pos == NULL, "不存在的子串应该返回NULL");
}

TEST(test_复制内存)
{
    int src[] = {1, 2, 3, 4, 5};
    int dest[5] = {0};
    
    复制内存(dest, src, sizeof(src));
    
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(dest[i], src[i], "内存应该被正确复制");
    }
}

TEST(test_设置内存)
{
    char buf[10];
    设置内存(buf, 'A', sizeof(buf));
    
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(buf[i], 'A', "内存应该被设置为指定值");
    }
}

TEST(test_比较内存)
{
    int a[] = {1, 2, 3};
    int b[] = {1, 2, 3};
    int c[] = {1, 2, 4};
    
    ASSERT_EQ(比较内存(a, b, sizeof(a)), 0, "相同内存应该返回0");
    ASSERT(比较内存(a, c, sizeof(a)) < 0, "不同内存应该有差异");
}

// =============================================================================
// 辅助函数测试
// =============================================================================

TEST(test_获取绝对值)
{
    ASSERT_EQ(获取绝对值(10), 10, "正数的绝对值应该是自己");
    ASSERT_EQ(获取绝对值(-10), 10, "负数的绝对值应该是正数");
    ASSERT_EQ(获取绝对值(0), 0, "0的绝对值应该是0");
}

TEST(test_求最大值)
{
    ASSERT_EQ(求最大值(10, 20), 20, "应该返回较大值");
    ASSERT_EQ(求最大值(30, 15), 30, "应该返回较大值");
    ASSERT_EQ(求最大值(5, 5), 5, "相等时返回任一值");
}

TEST(test_求最小值)
{
    ASSERT_EQ(求最小值(10, 20), 10, "应该返回较小值");
    ASSERT_EQ(求最小值(30, 15), 15, "应该返回较小值");
    ASSERT_EQ(求最小值(5, 5), 5, "相等时返回任一值");
}

// =============================================================================
// 标准I/O函数测试（仅限hosted模式）
// =============================================================================

#ifndef CN_FREESTANDING

TEST(test_格式化字符串)
{
    char buf[100];
    int len = 格式化字符串(buf, "数字: %d, 字符串: %s", 42, "测试");
    ASSERT(len > 0, "应该返回写入的字符数");
    ASSERT_STR_EQ(buf, "数字: 42, 字符串: 测试", "应该正确格式化");
}

TEST(test_安全格式化字符串)
{
    char buf[20];
    int len = 安全格式化字符串(buf, sizeof(buf), "很长的字符串测试测试测试");
    ASSERT(len >= 0, "应该返回非负值");
    ASSERT_EQ(strlen(buf), sizeof(buf) - 1, "应该被截断到缓冲区大小");
}

#endif /* CN_FREESTANDING */

// =============================================================================
// 边界情况测试
// =============================================================================

TEST(test_空指针处理_内存)
{
    // 释放NULL应该安全
    释放内存(NULL);
    
    // 重新分配NULL应该等同于分配
    void* ptr = 重新分配内存(NULL, 10);
    ASSERT(ptr != NULL, "重新分配NULL应该成功");
    释放内存(ptr);
}

TEST(test_空指针处理_字符串)
{
    ASSERT_EQ(获取字符串长度(NULL), 0, "NULL字符串长度应该返回0");
    ASSERT_EQ(比较字符串(NULL, NULL), 0, "两个NULL应该相等");
    ASSERT(比较字符串(NULL, "test") < 0, "NULL应该小于非NULL");
}

TEST(test_零长度操作)
{
    char buf[10] = "test";
    
    限长复制字符串(buf, "hello", 0);
    // 应该不改变buf
    
    ASSERT_EQ(限长比较字符串("abc", "xyz", 0), 0, "比较0个字符应该返回0");
}

// =============================================================================
// 主测试函数
// =============================================================================

int main(void)
{
    printf("\n");
    printf("==================================================\n");
    printf("CN_Language 标准库中文接口单元测试\n");
    printf("==================================================\n\n");
    
    // 内存管理测试
    printf("--- 内存管理函数测试 ---\n");
    RUN_TEST(test_分配内存);
    RUN_TEST(test_分配清零内存);
    RUN_TEST(test_重新分配内存);
    
    // 字符串处理测试
    printf("\n--- 字符串处理函数测试 ---\n");
    RUN_TEST(test_复制字符串);
    RUN_TEST(test_限长复制字符串);
    RUN_TEST(test_连接字符串);
    RUN_TEST(test_限长连接字符串);
    RUN_TEST(test_比较字符串);
    RUN_TEST(test_限长比较字符串);
    RUN_TEST(test_获取字符串长度);
    RUN_TEST(test_查找字符);
    RUN_TEST(test_反向查找字符);
    RUN_TEST(test_查找子串);
    RUN_TEST(test_复制内存);
    RUN_TEST(test_设置内存);
    RUN_TEST(test_比较内存);
    
    // 辅助函数测试
    printf("\n--- 辅助函数测试 ---\n");
    RUN_TEST(test_获取绝对值);
    RUN_TEST(test_求最大值);
    RUN_TEST(test_求最小值);
    
#ifndef CN_FREESTANDING
    // I/O函数测试
    printf("\n--- 标准I/O函数测试 ---\n");
    RUN_TEST(test_格式化字符串);
    RUN_TEST(test_安全格式化字符串);
#endif
    
    // 边界情况测试
    printf("\n--- 边界情况测试 ---\n");
    RUN_TEST(test_空指针处理_内存);
    RUN_TEST(test_空指针处理_字符串);
    RUN_TEST(test_零长度操作);
    
    // 测试总结
    printf("\n==================================================\n");
    printf("测试完成: %d/%d 通过\n", tests_passed, tests_run);
    printf("==================================================\n\n");
    
    return tests_passed == tests_run ? 0 : 1;
}
