#define CN_FREESTANDING

#include "cnlang/runtime/freestanding.h"
#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/runtime.h"
#include <stdio.h>
#include <assert.h>

/*
 * Freestanding 模式单元测试
 * 
 * 验证 freestanding 模式下的基础功能：
 * - 字符串和内存操作
 * - 静态内存分配器
 */

void test_freestanding_memcpy() {
    char src[] = "Hello, World!";
    char dest[20];
    
    cn_rt_memcpy(dest, src, sizeof(src));
    
    assert(cn_rt_strcmp(dest, src) == 0);
    printf("test_freestanding_memcpy: OK\n");
}

void test_freestanding_memmove() {
    char buffer[] = "1234567890";
    
    // 向前移动（有重叠）
    cn_rt_memmove(buffer + 2, buffer, 5);
    
    assert(buffer[2] == '1');
    assert(buffer[3] == '2');
    printf("test_freestanding_memmove: OK\n");
}

void test_freestanding_memset() {
    char buffer[10];
    
    cn_rt_memset(buffer, 'A', sizeof(buffer));
    
    for (size_t i = 0; i < sizeof(buffer); i++) {
        assert(buffer[i] == 'A');
    }
    printf("test_freestanding_memset: OK\n");
}

void test_freestanding_memcmp() {
    char a[] = "Hello";
    char b[] = "Hello";
    char c[] = "World";
    
    assert(cn_rt_memcmp(a, b, 5) == 0);
    assert(cn_rt_memcmp(a, c, 5) != 0);
    printf("test_freestanding_memcmp: OK\n");
}

void test_freestanding_strlen() {
    const char* str1 = "Hello";
    const char* str2 = "";
    
    assert(cn_rt_strlen(str1) == 5);
    assert(cn_rt_strlen(str2) == 0);
    printf("test_freestanding_strlen: OK\n");
}

void test_freestanding_strcpy() {
    char src[] = "Test";
    char dest[10];
    
    cn_rt_strcpy(dest, src);
    
    assert(cn_rt_strcmp(dest, src) == 0);
    printf("test_freestanding_strcpy: OK\n");
}

void test_freestanding_strcat() {
    char buffer[20] = "Hello";
    const char* suffix = ", World!";
    
    cn_rt_strcat(buffer, suffix);
    
    assert(cn_rt_strcmp(buffer, "Hello, World!") == 0);
    printf("test_freestanding_strcat: OK\n");
}

void test_freestanding_strcmp() {
    const char* a = "abc";
    const char* b = "abc";
    const char* c = "abd";
    
    assert(cn_rt_strcmp(a, b) == 0);
    assert(cn_rt_strcmp(a, c) < 0);
    assert(cn_rt_strcmp(c, a) > 0);
    printf("test_freestanding_strcmp: OK\n");
}

void test_freestanding_strncmp() {
    const char* a = "abcdef";
    const char* b = "abcxyz";
    
    assert(cn_rt_strncmp(a, b, 3) == 0);
    assert(cn_rt_strncmp(a, b, 4) != 0);
    printf("test_freestanding_strncmp: OK\n");
}

void test_freestanding_allocator() {
    // 初始化 freestanding 分配器
    cn_rt_freestanding_init_allocator();
    
    // 获取初始状态
    size_t used_before, total, alloc_count_before;
    cn_rt_freestanding_get_pool_info(&used_before, &total, &alloc_count_before);
    
    assert(used_before == 0);
    assert(alloc_count_before == 0);
    
    // 分配内存
    void* ptr1 = cn_rt_malloc(100);
    assert(ptr1 != NULL);
    
    void* ptr2 = cn_rt_malloc(200);
    assert(ptr2 != NULL);
    
    // 检查使用情况
    size_t used_after, alloc_count_after;
    cn_rt_freestanding_get_pool_info(&used_after, &total, &alloc_count_after);
    
    assert(used_after > used_before);
    assert(alloc_count_after == 2);
    
    // 重置内存池
    cn_rt_freestanding_reset_pool();
    
    size_t used_reset;
    cn_rt_freestanding_get_pool_info(&used_reset, &total, NULL);
    assert(used_reset == 0);
    
    printf("test_freestanding_allocator: OK\n");
}

void test_freestanding_runtime_init() {
    // 测试运行时初始化（在 freestanding 模式下使用静态分配器）
    cn_rt_init();
    
    // 测试字符串操作（使用运行时的高层 API）
    const char* str1 = "Hello";
    const char* str2 = "World";
    
    char* result = cn_rt_string_concat(str1, str2);
    assert(result != NULL);
    assert(cn_rt_strcmp(result, "HelloWorld") == 0);
    
    size_t len = cn_rt_string_length(result);
    assert(len == 10);
    
    printf("test_freestanding_runtime_init: OK\n");
}

int main(void) {
    printf("运行 Freestanding 模式单元测试...\n\n");
    
    // 基础内存操作测试
    test_freestanding_memcpy();
    test_freestanding_memmove();
    test_freestanding_memset();
    test_freestanding_memcmp();
    
    // 基础字符串操作测试
    test_freestanding_strlen();
    test_freestanding_strcpy();
    test_freestanding_strcat();
    test_freestanding_strcmp();
    test_freestanding_strncmp();
    
    // 静态分配器测试
    test_freestanding_allocator();
    
    // 运行时集成测试
    test_freestanding_runtime_init();
    
    printf("\n所有 Freestanding 模式测试通过!\n");
    return 0;
}
