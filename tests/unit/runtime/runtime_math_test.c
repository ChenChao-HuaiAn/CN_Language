#include "cnlang/runtime/math.h"
#include <stdio.h>
#include <math.h>

static int test_math_basic(void) {
    if (cn_rt_abs(-10) != 10) return 1;
    if (cn_rt_min(5, 10) != 5) return 1;
    if (cn_rt_max(5, 10) != 10) return 1;
    if (fabs(cn_rt_sqrt(16.0) - 4.0) > 0.0001) return 1;
    if (fabs(cn_rt_pow(2.0, 3.0) - 8.0) > 0.0001) return 1;
    return 0;
}

int main(void) {
    if (test_math_basic() != 0) return 1;
    printf("runtime_math_test: OK\n");
    return 0;
}
