#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 点 {
    int x;
    int y;
};

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r1;
  struct 点 r0;

  entry:
  struct 点 cn_var_p1;
  cn_var_p1 = (struct 点){.x = 10, .y = 20};
  struct 点 cn_var_p2;
  cn_var_p2 = (struct 点){.x = 10, .y = 20};
  struct 点 cn_var_p3;
  cn_var_p3 = (struct 点){.x = 30, .y = 40};
  struct 点 cn_var_p4;
  cn_var_p4 = (struct 点){.x = 50, .y = 60};
  int cn_var_px;
  r0 = cn_var_p1;
  r1 = r0.x;
  cn_var_px = r1;
  return 0;
  cn_rt_exit();
}

