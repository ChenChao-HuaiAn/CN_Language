#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 学生;

// CN Language Global Struct Definitions
struct 学生 {
    char* 姓名;
    long long 年龄;
    double 成绩;
};

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
  struct 学生 cn_var_s2_1;
  return 0;
  cn_rt_exit();
}

