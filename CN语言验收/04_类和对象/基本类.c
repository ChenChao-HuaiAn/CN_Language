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
学生* 学生_construct(学生* self);
学生* 学生_construct(学生* self, char* 名, long long 龄, double 分);
void 学生_打印信息(struct 学生* self);
_Bool 学生_是否及格(struct 学生* self);
void 学生_设置成绩(struct 学生* self, double 新成绩);

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
学生* 学生_construct(学生* self);
学生* 学生_construct(学生* self, char* 名, long long 龄, double 分);
void 学生_打印信息(struct 学生* self);
_Bool 学生_是否及格(struct 学生* self);
void 学生_设置成绩(struct 学生* self, double 新成绩);

// 类 学生 的构造函数和方法实现
学生* 学生_construct(学生* self) {
    // 初始化类型信息指针（RTTI）
    self->type_info = &_学生_type_info;
    // 构造函数体
    self->姓名 = "未命名";
    self->年龄 = 0;
    self->成绩 = 0.000000;
    return self;
}

学生* 学生_construct(学生* self, char* 名, long long 龄, double 分) {
    // 初始化类型信息指针（RTTI）
    self->type_info = &_学生_type_info;
    // 构造函数体
    self->姓名 = 名;
    self->年龄 = 龄;
    self->成绩 = 分;
    return self;
}

void 学生_打印信息(struct 学生* self) {
    // 空方法
}

_Bool 学生_是否及格(struct 学生* self) {
    // 方法体
    return (self->成绩 >= 60.000000);
}

void 学生_设置成绩(struct 学生* self, double 新成绩) {
    // 方法体
    self->成绩 = 新成绩;
}

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  struct 学生* r1;
  struct 学生* r2;
  struct 学生* r3;
  struct 学生* r4;
  _Bool r5;
  struct 学生 r0;

  entry:
  struct 学生 cn_var_学生1_0;
  struct 学生 cn_var_学生2_1;
  r0 = (struct 学生){"张三", 20, 85.500000};
  cn_var_学生2_1 = r0;
  r1 = &cn_var_学生1_0;
  学生_打印信息(r1);
  r2 = &cn_var_学生2_1;
  学生_打印信息(r2);
  r3 = &cn_var_学生1_0;
  学生_设置成绩(r3, 90.000000);
  _Bool cn_var_及格_2;
  r4 = &cn_var_学生2_1;
  r5 = 学生_是否及格(r4);
  cn_var_及格_2 = r5;
  return 0;
  cn_rt_exit();
}

