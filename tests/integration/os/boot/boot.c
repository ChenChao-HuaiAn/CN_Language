/*
 * 最小 x86_64 启动代码（C 版本）
 * 
 * 此代码在 bootloader 加载后执行，负责：
 * 1. 初始化栈
 * 2. 调用 CN Language 编写的内核主函数
 * 3. 进入无限循环（防止 CPU 执行到未知代码）
 */

// 外部符号：由 CN Language 编译生成的内核主函数
extern int kernel_main(void);

// 栈空间（16KB）
static char stack[16384] __attribute__((aligned(16)));

// 入口点：bootloader 会跳转到这里
void _start(void) {
    // 设置栈指针（指向栈顶）
    __asm__ volatile (
        "mov %0, %%rsp\n"
        : 
        : "r"(stack + sizeof(stack))
    );
    
    // 调用内核主函数
    int ret = kernel_main();
    
    // 主函数返回后，进入无限循环
    while (1) {
        __asm__ volatile ("hlt");
    }
    
    // 防止编译器警告
    (void)ret;
}
