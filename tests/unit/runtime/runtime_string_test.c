#include "cnlang/runtime/runtime.h"
#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <string.h>

static int test_string_concat(void) {
    char* s = cn_rt_string_concat("Hello, ", "World!");
    if (strcmp(s, "Hello, World!") != 0) {
        fprintf(stderr, "test_string_concat: mismatch. Got '%s'\n", s);
        cn_rt_free(s);
        return 1;
    }
    cn_rt_free(s);
    return 0;
}

static int test_string_length(void) {
    if (cn_rt_string_length("你好") != 6) { // UTF-8 "你好" is 6 bytes
        fprintf(stderr, "test_string_length: failed for '你好'\n");
        return 1;
    }
    if (cn_rt_string_length(NULL) != 0) {
        return 1;
    }
    return 0;
}

static int test_string_substring(void) {
    char* s = cn_rt_string_substring("CN_Language", 3, 8);
    if (strcmp(s, "Language") != 0) {
        fprintf(stderr, "test_string_substring: Got '%s'\n", s);
        cn_rt_free(s);
        return 1;
    }
    cn_rt_free(s);
    return 0;
}

static int test_string_trim(void) {
    char* s = cn_rt_string_trim("  hello  ");
    if (strcmp(s, "hello") != 0) {
        fprintf(stderr, "test_string_trim: Got '%s'\n", s);
        cn_rt_free(s);
        return 1;
    }
    cn_rt_free(s);
    return 0;
}

static int test_string_format(void) {
    char* s = cn_rt_string_format("Num: %d, Str: %s", 42, "test");
    if (strcmp(s, "Num: 42, Str: test") != 0) {
        fprintf(stderr, "test_string_format: Got '%s'\n", s);
        cn_rt_free(s);
        return 1;
    }
    cn_rt_free(s);
    return 0;
}

// 测试空字符串长度
static int test_empty_string_length(void) {
    if (cn_rt_string_length("") != 0) {
        fprintf(stderr, "test_empty_string_length: expected 0, got %zu\n", cn_rt_string_length(""));
        return 1;
    }
    return 0;
}

// 测试单字符字符串长度
static int test_single_char_string_length(void) {
    if (cn_rt_string_length("A") != 1) {
        fprintf(stderr, "test_single_char_string_length: expected 1, got %zu\n", cn_rt_string_length("A"));
        return 1;
    }
    return 0;
}

// 测试空字符串拼接
static int test_empty_string_concat(void) {
    char* s1 = cn_rt_string_concat("", "hello");
    if (strcmp(s1, "hello") != 0) {
        fprintf(stderr, "test_empty_string_concat: case 1 failed\n");
        cn_rt_free(s1);
        return 1;
    }
    cn_rt_free(s1);
    
    char* s2 = cn_rt_string_concat("hello", "");
    if (strcmp(s2, "hello") != 0) {
        fprintf(stderr, "test_empty_string_concat: case 2 failed\n");
        cn_rt_free(s2);
        return 1;
    }
    cn_rt_free(s2);
    
    char* s3 = cn_rt_string_concat("", "");
    if (strcmp(s3, "") != 0) {
        fprintf(stderr, "test_empty_string_concat: case 3 failed\n");
        cn_rt_free(s3);
        return 1;
    }
    cn_rt_free(s3);
    
    return 0;
}

int main(void) {
    if (test_string_concat() != 0) return 1;
    if (test_string_length() != 0) return 1;
    if (test_string_substring() != 0) return 1;
    if (test_string_trim() != 0) return 1;
    if (test_string_format() != 0) return 1;
    if (test_empty_string_length() != 0) return 1;       // 新增测试
    if (test_single_char_string_length() != 0) return 1; // 新增测试
    if (test_empty_string_concat() != 0) return 1;       // 新增测试
    
    printf("runtime_string_test: OK\n");
    return 0;
}
