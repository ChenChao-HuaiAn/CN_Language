#include "cnlang/runtime/math.h"
#include <math.h>

/*
 * CN Language 运行时数学实现
 */

long long cn_rt_abs(long long val) {
    return val < 0 ? -val : val;
}

long long cn_rt_min(long long a, long long b) {
    return a < b ? a : b;
}

long long cn_rt_max(long long a, long long b) {
    return a > b ? a : b;
}

double cn_rt_pow(double base, double exp) {
    return pow(base, exp);
}

double cn_rt_sqrt(double val) {
    return sqrt(val);
}
