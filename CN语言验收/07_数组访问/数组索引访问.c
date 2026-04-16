#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long* 数组;
    long long 长度;
};

// Global Variables

// Forward Declarations
long long 测试指针数组访问(struct 测试结构*, long long);
long long 测试直接数组访问(struct 测试结构, long long);
long long 测试简单数组(long long*, long long);

long long 测试指针数组访问(struct 测试结构* cn_var_指针, long long cn_var_i) {
  long long r1;
  long long* r0;
  long long* r2;
  long long* r3;

  entry:
  r0 = cn_var_指针->数组;
  r1 = cn_var_i;
  r2 = &r0[r1];
  r3 = r2;
  return r3;
}

long long 测试直接数组访问(struct 测试结构 cn_var_结构, long long cn_var_i) {
  long long r1;
  long long* r0;
  long long* r2;
  long long* r3;

  entry:
  r0 = cn_var_结构.数组;
  r1 = cn_var_i;
  r2 = &r0[r1];
  r3 = r2;
  return r3;
}

long long 测试简单数组(long long* cn_var_数组, long long cn_var_i) {
  long long r1;
  long long* r0;
  long long* r2;
  long long* r3;

  entry:
  r0 = cn_var_数组;
  r1 = cn_var_i;
  r2 = &r0[r1];
  r3 = r2;
  return r3;
}

