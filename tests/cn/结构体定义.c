#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;
struct 复合结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 字段1;
    char* 字段2;
};

struct 复合结构 {
    struct 测试结构 内嵌结构;
    long long 附加字段;
};

// Global Variables

// Forward Declarations

