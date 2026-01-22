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

int main(void) {
    if (test_string_concat() != 0) return 1;
    if (test_string_length() != 0) return 1;
    if (test_string_substring() != 0) return 1;
    if (test_string_trim() != 0) return 1;
    if (test_string_format() != 0) return 1;
    
    printf("runtime_string_test: OK\n");
    return 0;
}
