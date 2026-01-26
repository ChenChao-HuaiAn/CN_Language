#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// CN Language Enum Definitions
enum 颜色 {
    红色,
    绿色,
    蓝色
};

enum 状态 {
    未开始 = 0,
    进行中 = 1,
    已完成 = 2
};

enum 优先级 {
    低 = 1,
    中 = 5,
    高 = 10
};

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();

  entry:
  int cn_var_c;
  cn_var_c = 0;
  int cn_var_s;
  cn_var_s = 1;
  int cn_var_p;
  cn_var_p = 10;
  return 0;
  cn_rt_exit();
}

