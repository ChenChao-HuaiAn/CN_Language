#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 颜色 {
    红,
    绿,
    蓝
};

enum 状态 {
    待处理 = 0,
    处理中 = 1,
    已完成 = 2,
    已取消 = -1
};

enum 星期 {
    周一,
    周二,
    周三,
    周四,
    周五,
    周六,
    周日
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  _Bool r1;

  entry:
  enum 颜色 cn_var_当前颜色;
  cn_var_当前颜色 = 0;
  enum 状态 cn_var_任务状态;
  cn_var_任务状态 = 0;
  enum 星期 cn_var_今天;
  cn_var_今天 = 0;
  r0 = cn_var_当前颜色;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_var_当前颜色 = 1;
  goto if_merge_1;

  if_merge_1:
  cn_var_任务状态 = 2;
  return 0;
  cn_rt_exit();
}

