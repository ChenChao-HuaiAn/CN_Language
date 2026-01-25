#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Forward Declarations
int cn_func___isr_0();
int cn_func___isr_2();
int main();

// 中断处理函数 (ISR) - 中断向量号: 0
int cn_func___isr_0() {

  entry:
  return 0;
}

// 中断处理函数 (ISR) - 中断向量号: 2
int cn_func___isr_2() {

  entry:
  return 0;
}

int main() {
  cn_rt_init();
  cn_rt_interrupt_register(0, cn_func___isr_0, "cn_func___isr_0");
  cn_rt_interrupt_register(2, cn_func___isr_2, "cn_func___isr_2");

  entry:
  return 0;
  cn_rt_exit();
}

