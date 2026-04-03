#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Class Definitions

// ========================================
// 类定义: 人员
// ========================================

struct 人员;

typedef struct 人员 {
    struct 人员_vtable* vtable;  // 虚函数表指针（偏移量: 0字节）
    const CnTypeInfo* type_info;  // 类型信息指针（RTTI）
    char* 姓名;
    long long 年龄;
} 人员;

// 类 人员 的方法前向声明
人员* 人员_construct(人员* self, char* 名, long long 龄);
void 人员_显示信息(struct 人员* self);
char* 人员_获取类型(struct 人员* self);

typedef struct 人员_vtable {
    void (*显示信息)(struct 人员* self);
    char* (*获取类型)(struct 人员* self);
} 人员_vtable;

static 人员_vtable _人员_vtable = {
    .显示信息 = 人员_显示信息,
    .获取类型 = 人员_获取类型
};

/* 类 人员 的类型转换缓存 */
static const CnCastInfo _人员_cast_cache[1] = {{0}};

/* 类 人员 的类型信息（RTTI） */
static const CnTypeInfo _人员_type_info = {
    .name = "人员",
    .name_length = 6,
    .size = sizeof(人员),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = NULL,
    .base_count = 0,
    .vtable = &_人员_vtable,
    .depth = 0,
    .primary_base = NULL,
    .cast_cache = NULL,
    .cast_cache_count = 0
};

/* 自动注册类型信息 */
#if defined(_MSC_VER)
static void _人员_register_type(void);
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) static void (*_人员_register_type_ptr)(void) = _人员_register_type;
#else
__attribute__((constructor))
#endif
static void _人员_register_type(void) {
    cn_register_type_info(&_人员_type_info);
}

// 类 人员 的构造函数和方法声明
人员* 人员_construct(人员* self, char* 名, long long 龄);
void 人员_显示信息(struct 人员* self);
char* 人员_获取类型(struct 人员* self);

// 类 人员 的构造函数和方法实现
人员* 人员_construct(人员* self, char* 名, long long 龄) {
    // 初始化虚函数表指针
    self->vtable = &_人员_vtable;
    // 初始化类型信息指针（RTTI）
    self->type_info = &_人员_type_info;
    // 构造函数体
    self->姓名 = 名;
    self->年龄 = 龄;
    return self;
}

void 人员_显示信息(struct 人员* self) {
    // 空方法
}

char* 人员_获取类型(struct 人员* self) {
    // 方法体
    return "人员";
}

// ========================================
// 类定义: 学生
// ========================================

struct 学生;

typedef struct 学生 {
    struct 人员 人员_base;  // 基类子对象（偏移量: 0字节，公开继承）
    struct 学生_vtable* vtable;  // 虚函数表指针（偏移量: 32字节）
    const CnTypeInfo* type_info;  // 类型信息指针（RTTI）
    char* 学号;
    double 成绩;
} 学生;

/* 类 学生 的基类偏移量常量（用于this指针调整） */
#define 学生_人员_OFFSET 0  /*  */

/* this指针调整宏 */
#define 学生_TO_人员(ptr) ((struct 人员*)((char*)(ptr) + 学生_人员_OFFSET))

// 类 学生 的方法前向声明
学生* 学生_construct(学生* self, char* 名, long long 龄, char* 号, double 分);
void 学生_显示信息(struct 学生* self);
char* 学生_获取类型(struct 学生* self);
void 学生_学习(struct 学生* self);

typedef struct 学生_vtable {
    void (*显示信息)(struct 学生* self);
    char* (*获取类型)(struct 学生* self);
} 学生_vtable;

static 学生_vtable _学生_vtable = {
    .显示信息 = 学生_显示信息,
    .获取类型 = 学生_获取类型
};

/* 类 学生 的基类信息 */
static const CnBaseClassInfo _学生_bases[] = {
    {
        .type = &_人员_type_info,  // 基类 人员
        .offset = 学生_人员_OFFSET,
        .is_virtual = false,
        .is_public = true
    },
};

/* 类 学生 的类型转换缓存 */
static const CnCastInfo _学生_cast_cache[] = {
    {
        .target_type = &_人员_type_info,  // 基类 人员
        .offset = 学生_人员_OFFSET,
        .flags = CN_CAST_FLAG_NONE
    },
};

/* 类 学生 的类型信息（RTTI） */
static const CnTypeInfo _学生_type_info = {
    .name = "学生",
    .name_length = 6,
    .size = sizeof(学生),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = _学生_bases,
    .base_count = 1,
    .vtable = &_学生_vtable,
    .depth = 1,
    .primary_base = &_人员_type_info,  // 主基类 人员
    .cast_cache = _学生_cast_cache,
    .cast_cache_count = 1
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
学生* 学生_construct(学生* self, char* 名, long long 龄, char* 号, double 分);
void 学生_显示信息(struct 学生* self);
char* 学生_获取类型(struct 学生* self);
void 学生_学习(struct 学生* self);

// 类 学生 的构造函数和方法实现
学生* 学生_construct(学生* self, char* 名, long long 龄, char* 号, double 分) {
    // 初始化虚函数表指针
    self->vtable = &_学生_vtable;
    // 初始化类型信息指针（RTTI）
    self->type_info = &_学生_type_info;
    // 初始化非虚基类 人员（偏移量: 使用宏 学生_人员_OFFSET）
    人员_construct(&self->人员_base, 名, 名);  // 调用基类构造函数
    // 构造函数体
    self->人员_base.姓名 = 名;
    self->人员_base.年龄 = 龄;
    self->学号 = 号;
    self->成绩 = 分;
    return self;
}

void 学生_显示信息(struct 学生* self) {
    // 空方法
}

char* 学生_获取类型(struct 学生* self) {
    // 方法体
    return "学生";
}

void 学生_学习(struct 学生* self) {
    // 空方法
}

// ========================================
// 类定义: 研究生
// ========================================

struct 研究生;

typedef struct 研究生 {
    struct 学生 学生_base;  // 基类子对象（偏移量: 0字节，公开继承）
    struct 研究生_vtable* vtable;  // 虚函数表指针（偏移量: 32字节）
    const CnTypeInfo* type_info;  // 类型信息指针（RTTI）
    char* 导师;
    char* 研究方向;
} 研究生;

/* 类 研究生 的基类偏移量常量（用于this指针调整） */
#define 研究生_学生_OFFSET 0  /*  */

/* this指针调整宏 */
#define 研究生_TO_学生(ptr) ((struct 学生*)((char*)(ptr) + 研究生_学生_OFFSET))

// 类 研究生 的方法前向声明
研究生* 研究生_construct(研究生* self, char* 名, long long 龄, char* 号, double 分, char* 导);
char* 研究生_获取类型(struct 研究生* self);
void 研究生_显示完整信息(struct 研究生* self);

typedef struct 研究生_vtable {
    void (*显示信息)(struct 研究生* self);
    char* (*获取类型)(struct 研究生* self);
} 研究生_vtable;

static 研究生_vtable _研究生_vtable = {
    .显示信息 = 学生_显示信息,
    .获取类型 = 研究生_获取类型
};

/* 类 研究生 的基类信息 */
static const CnBaseClassInfo _研究生_bases[] = {
    {
        .type = &_学生_type_info,  // 基类 学生
        .offset = 研究生_学生_OFFSET,
        .is_virtual = false,
        .is_public = true
    },
};

/* 类 研究生 的类型转换缓存 */
static const CnCastInfo _研究生_cast_cache[] = {
    {
        .target_type = &_学生_type_info,  // 基类 学生
        .offset = 研究生_学生_OFFSET,
        .flags = CN_CAST_FLAG_NONE
    },
};

/* 类 研究生 的类型信息（RTTI） */
static const CnTypeInfo _研究生_type_info = {
    .name = "研究生",
    .name_length = 9,
    .size = sizeof(研究生),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = _研究生_bases,
    .base_count = 1,
    .vtable = &_研究生_vtable,
    .depth = 1,
    .primary_base = &_学生_type_info,  // 主基类 学生
    .cast_cache = _研究生_cast_cache,
    .cast_cache_count = 1
};

/* 自动注册类型信息 */
#if defined(_MSC_VER)
static void _研究生_register_type(void);
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) static void (*_研究生_register_type_ptr)(void) = _研究生_register_type;
#else
__attribute__((constructor))
#endif
static void _研究生_register_type(void) {
    cn_register_type_info(&_研究生_type_info);
}

// 类 研究生 的构造函数和方法声明
研究生* 研究生_construct(研究生* self, char* 名, long long 龄, char* 号, double 分, char* 导);
char* 研究生_获取类型(struct 研究生* self);
void 研究生_显示完整信息(struct 研究生* self);

// 类 研究生 的构造函数和方法实现
研究生* 研究生_construct(研究生* self, char* 名, long long 龄, char* 号, double 分, char* 导) {
    // 初始化虚函数表指针
    self->vtable = &_研究生_vtable;
    // 初始化类型信息指针（RTTI）
    self->type_info = &_研究生_type_info;
    // 初始化非虚基类 学生（偏移量: 使用宏 研究生_学生_OFFSET）
    学生_construct(&self->学生_base, 名, 名, 名, 名);  // 调用基类构造函数
    // 构造函数体
    self->学生_base.人员_base.姓名 = 名;
    self->学生_base.人员_base.年龄 = 龄;
    self->学生_base.学号 = 号;
    self->学生_base.成绩 = 分;
    self->导师 = 导;
    return self;
}

char* 研究生_获取类型(struct 研究生* self) {
    // 方法体
    return "研究生";
}

void 研究生_显示完整信息(struct 研究生* self) {
    // 方法体
    学生_显示信息((struct 学生*)((char*)self + 研究生_学生_OFFSET));
}

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  struct 人员* r3;
  struct 学生* r4;
  struct 研究生* r5;
  struct 人员 r0;
  struct 学生 r1;
  struct 研究生 r2;

  entry:
  struct 人员 cn_var_普通人_0;
  r0 = (struct 人员){"张三", 30};
  cn_var_普通人_0 = r0;
  struct 学生 cn_var_本科生_1;
  r1 = (struct 学生){"李四", 20, "2024001", 85.500000};
  cn_var_本科生_1 = r1;
  struct 研究生 cn_var_博士生_2;
  r2 = (struct 研究生){"王五", 25, "2024002", 90.000000, "赵教授"};
  cn_var_博士生_2 = r2;
  r3 = &cn_var_普通人_0;
  人员_显示信息(r3);
  r4 = &cn_var_本科生_1;
  学生_显示信息(r4);
  r5 = &cn_var_博士生_2;
  研究生_显示完整信息(r5);
  return 0;
  cn_rt_exit();
}

