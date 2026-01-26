// CN Runtime simple wrappers for hello_kernel
extern void cn_rt_kernel_print(const char *str);
extern void cn_rt_kernel_putchar(int ch);

void cn_rt_print_string(const char *str) {
    cn_rt_kernel_print(str);
}

void cn_rt_print_int(long long val) {
    if (val == 0) {
        cn_rt_kernel_putchar('0');
        return;
    }
    char buf[32];
    int i = 0;
    int neg = (val < 0);
    if (neg) val = -val;
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    if (neg) cn_rt_kernel_putchar('-');
    while (--i >= 0) cn_rt_kernel_putchar(buf[i]);
}
