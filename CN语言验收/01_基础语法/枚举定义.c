#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 颜色 {
    颜色_红,
    颜色_绿,
    颜色_蓝
};

enum 状态 {
    状态_待处理 = 0,
    状态_处理中 = 1,
    状态_已完成 = 2,
    状态_已取消 = -1
};

enum 星期 {
    星期_周一,
    星期_周二,
    星期_周三,
    星期_周四,
    星期_周五,
    星期_周六,
    星期_周日
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  _Bool r1;
  enum 颜色 r0;

  entry:
  enum 颜色 cn_var_当前颜色_0;
  cn_var_当前颜色_0 = 0;
  enum 状态 cn_var_任务状态_1;
  cn_var_任务状态_1 = 0;
  enum 星期 cn_var_今天_2;
  cn_var_今天_2 = 0;
  r0 = cn_var_当前颜色_0;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_var_当前颜色_0 = 1;
  goto if_merge_1;

  if_merge_1:
  cn_var_任务状态_1 = 2;
  return 0;
  cn_rt_exit();
}

