#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Class Definitions

// ========================================
// 类定义: 学生
// ========================================

struct 学生;

typedef struct 学生 {
    const CnTypeInfo* type_info;  // 类型信息指针（RTTI）
    char* 姓名;
    long long 年龄;
    double 成绩;
} 学生;

// 类 学生 的方法前向声明

/* 类 学生 的类型转换缓存 */
static const CnCastInfo _学生_cast_cache[1] = {{0}};

/* 类 学生 的类型信息（RTTI） */
static const CnTypeInfo _学生_type_info = {
    .name = "学生",
    .name_length = 6,
    .size = sizeof(学生),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = NULL,
    .base_count = 0,
    .vtable = NULL,
    .depth = 0,
    .primary_base = NULL,
    .cast_cache = NULL,
    .cast_cache_count = 0
};

/* 自动注册类型信息 */
#if defined(_MSC_VER)
static void _学生_register_type(void);
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) static void (*_学生_register_type_ptr)(void) = _学生_register_type;
#else
__attribute__((constructor))
#endif
static void _学生_register_type(void) {
    cn_register_type_info(&_学生_type_info);
}

// 类 学生 的构造函数和方法声明

// 类 学生 的构造函数和方法实现
// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  struct 学生 r0;

  entry:
  struct 学生 cn_var_s1_0;
  r0 = (struct 学生){"张三", 20, 85.500000};
  cn_var_s1_0 = r0;
  return 0;
  cn_rt_exit();
}

