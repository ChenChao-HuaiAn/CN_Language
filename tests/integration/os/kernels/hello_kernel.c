#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// CN Language Runtime Function Declarations (Freestanding Mode)
void cn_rt_print_string(const char *str);
void cn_rt_print_int(long long val);
size_t cn_rt_string_length(const char *str);

int cn_func_kernel_main() {

  entry:
  cn_rt_print_string("=== CN Language Kernel ===\n");
  cn_rt_print_string("Hello from CN Kernel!\n");
  cn_rt_print_string("Kernel initialization complete.\n");
  cn_rt_print_string("[OK] All tests passed.\n");
  return 0;
}

