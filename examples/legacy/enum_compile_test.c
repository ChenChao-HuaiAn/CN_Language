#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// CN Language Enum Definitions
enum 状态 {
    成功 = 0,
    失败 = 1,
    待定 = 2
};

int main() {
  cn_rt_init();
  long long r0;

  entry:
  int cn_var_s;
  cn_var_s = 0;
  r0 = cn_var_s;
  return r0;
  cn_rt_exit();
}

