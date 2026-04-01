#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations - 从导入模块
long long 加法(long long, long long);
long long 减法(long long, long long);
long long 乘法(long long, long long);
long long 除法(long long, long long);
long long 获取版本(void);
long long 测试函数(void);
long long 使用测试(void);

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13;

  entry:
  cn_rt_print_string("=== CN语言模块系统功能展示 ===\n\n");
  cn_rt_print_string("1. 数学运算:\n");
  long long cn_var_和;
  r0 = 加法(15, 25);
  cn_var_和 = r0;
  long long cn_var_差;
  r1 = 减法(50, 20);
  cn_var_差 = r1;
  long long cn_var_积;
  r2 = 乘法(6, 7);
  cn_var_积 = r2;
  long long cn_var_商;
  r3 = 除法(100, 4);
  cn_var_商 = r3;
  cn_rt_print_string("  15 + 25 = ");
  r4 = cn_var_和;
  cn_rt_print_int(r4);
  cn_rt_print_string("\n");
  cn_rt_print_string("  50 - 20 = ");
  r5 = cn_var_差;
  cn_rt_print_int(r5);
  cn_rt_print_string("\n");
  cn_rt_print_string("  6 × 7 = ");
  r6 = cn_var_积;
  cn_rt_print_int(r6);
  cn_rt_print_string("\n");
  cn_rt_print_string("  100 ÷ 4 = ");
  r7 = cn_var_商;
  cn_rt_print_int(r7);
  cn_rt_print_string("\n\n");
  cn_rt_print_string("2. 通配符导入:\n");
  long long cn_var_测试值;
  r8 = 测试函数();
  cn_var_测试值 = r8;
  cn_rt_print_string("  测试函数返回: ");
  r9 = cn_var_测试值;
  cn_rt_print_int(r9);
  cn_rt_print_string("\n\n");
  cn_rt_print_string("3. 包导入:\n");
  long long cn_var_版本;
  r10 = 获取版本();
  cn_var_版本 = r10;
  cn_rt_print_string("  工具包版本: ");
  r11 = cn_var_版本;
  cn_rt_print_int(r11);
  cn_rt_print_string("\n\n");
  cn_rt_print_string("4. 命名空间导入:\n");
  long long cn_var_使用值;
  r12 = 使用测试();
  cn_var_使用值 = r12;
  cn_rt_print_string("  模块函数返回: ");
  r13 = cn_var_使用值;
  cn_rt_print_int(r13);
  cn_rt_print_string("\n\n");
  cn_rt_print_string("=== 所有功能测试通过! ===\n");
  return 0;
  cn_rt_exit();
}

