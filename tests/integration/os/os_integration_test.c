/*
 * OS 集成测试驱动程序
 * 
 * 此程序驱动内核编译和测试流程：
 * 1. 调用构建脚本编译内核
 * 2. 检查镜像文件是否生成
 * 3. 验证镜像格式
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// 执行 QEMU 测试（如果 QEMU 可用）
static bool run_qemu_test(const char *kernel_image, const char *expected_output) {
    printf("[\u6d4b\u8bd5] \u8fd0\u884c QEMU \u6d4b\u8bd5: %s\n", kernel_image);
    
#ifdef _WIN32
    char cmd[1024];
    if (expected_output && expected_output[0] != '\0') {
        snprintf(cmd, sizeof(cmd), 
                 "pwsh.exe -ExecutionPolicy Bypass -File scripts/run_qemu_test.ps1 "
                 "-KernelImage \"%s\" -TimeoutSeconds 3 -ExpectedOutput \"%s\" "
                 "> nul 2>&1",
                 kernel_image, expected_output);
    } else {
        snprintf(cmd, sizeof(cmd), 
                 "pwsh.exe -ExecutionPolicy Bypass -File scripts/run_qemu_test.ps1 "
                 "-KernelImage \"%s\" -TimeoutSeconds 3 "
                 "> nul 2>&1",
                 kernel_image);
    }
    
    int result = system(cmd);
    return result == 0;
#else
    char cmd[1024];
    if (expected_output && expected_output[0] != '\0') {
        snprintf(cmd, sizeof(cmd), 
                 "./scripts/run_qemu_test.ps1 '%s' -TimeoutSeconds 3 -ExpectedOutput '%s' "
                 "> /dev/null 2>&1",
                 kernel_image, expected_output);
    } else {
        snprintf(cmd, sizeof(cmd), 
                 "./scripts/run_qemu_test.ps1 '%s' -TimeoutSeconds 3 "
                 "> /dev/null 2>&1",
                 kernel_image);
    }
    
    int result = system(cmd);
    return WIFEXITED(result) && WEXITSTATUS(result) == 0;
#endif
}

// 检查 QEMU 是否可用
static bool is_qemu_available(void) {
#ifdef _WIN32
    int result = system("where qemu-system-x86_64 > nul 2>&1");
    return result == 0;
#else
    int result = system("which qemu-system-x86_64 > /dev/null 2>&1");
    return WIFEXITED(result) && WEXITSTATUS(result) == 0;
#endif
}

// 执行构建脚本
static bool build_kernel(const char *kernel_source, const char *output_image) {
    printf("[测试] 编译内核: %s -> %s\n", kernel_source, output_image);
    
#ifdef _WIN32
    char cmd[1024];
    // 使用相对于 tests/integration/os 的路径
    snprintf(cmd, sizeof(cmd), 
             "pwsh.exe -ExecutionPolicy Bypass -File scripts/build_kernel.ps1 "
             "-KernelSource \"%s\" -OutputImage \"%s\" "
             "-CncPath \"../../../build/src/cnc.exe\"",
             kernel_source, output_image);
    
    int result = system(cmd);
    return result == 0;
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "./scripts/build_kernel.ps1 '%s' '%s'",
             kernel_source, output_image);
    
    int result = system(cmd);
    return WIFEXITED(result) && WEXITSTATUS(result) == 0;
#endif
}

// 检查文件是否存在
static bool file_exists(const char *path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) == 0;
#endif
}

// 测试最小内核编译
static bool test_minimal_kernel_build(void) {
    printf("\n=== 测试：最小内核编译 ===\n");
    
    const char *source = "kernels/minimal_kernel.cn";
    const char *output = "test_kernel.elf";
    
    // 检查源文件是否存在
    if (!file_exists(source)) {
        printf("[失败] 找不到内核源文件: %s\n", source);
        return false;
    }
    
    // 构建内核
    if (!build_kernel(source, output)) {
        printf("[失败] 内核编译失败\n");
        return false;
    }
    
    // 检查输出文件
    if (!file_exists(output)) {
        printf("[失败] 未生成内核镜像: %s\n", output);
        return false;
    }
    
    printf("[成功] 内核编译成功，镜像: %s\n", output);
    
    // 清理输出文件
    remove(output);
    
    return true;
}

// 测试带输出验证的内核（需要 QEMU）
static bool test_kernel_with_output_validation(void) {
    printf("\n=== \u6d4b\u8bd5\uff1a\u5e26\u8f93\u51fa\u9a8c\u8bc1\u7684\u5185\u6838 ===\n");
    
    // 检查 QEMU 是否可用
    if (!is_qemu_available()) {
        printf("[\u8df3\u8fc7] QEMU \u672a\u5b89\u88c5\uff0c\u8df3\u8fc7\u6b64\u6d4b\u8bd5\n");
        return true;  // 不影\u54cd\u6574体\u7ed3\u679c
    }
    
    const char *source = "kernels/test_kernel.cn";
    const char *output = "test_kernel_output.elf";
    
    // 检查源文件是否存在
    if (!file_exists(source)) {
        printf("[\u5931\u8d25] \u627e\u4e0d\u5230\u5185\u6838\u6e90\u6587\u4ef6: %s\n", source);
        return false;
    }
    
    // 构建内核（使用测试\u542f\u52a8\u4ee3\u7801）
    printf("[\u6d4b\u8bd5] \u7f16\u8bd1\u5185\u6838: %s -> %s\n", source, output);
    
#ifdef _WIN32
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "pwsh.exe -ExecutionPolicy Bypass -File scripts/build_kernel.ps1 "
             "-KernelSource \"%s\" -OutputImage \"%s\" "
             "-BootCode \"boot/boot_test.c\" "
             "-CncPath \"../../../build/src/cnc.exe\"",
             source, output);
    
    int result = system(cmd);
    if (result != 0) {
        printf("[\u5931\u8d25] \u5185\u6838\u7f16\u8bd1\u5931\u8d25\n");
        return false;
    }
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "./scripts/build_kernel.ps1 '%s' '%s' -BootCode 'boot/boot_test.c'",
             source, output);
    
    int result = system(cmd);
    if (!(WIFEXITED(result) && WEXITSTATUS(result) == 0)) {
        printf("[\u5931\u8d25] \u5185\u6838\u7f16\u8bd1\u5931\u8d25\n");
        return false;
    }
#endif
    
    // 检查\u8f93\u51fa\u6587\u4ef6
    if (!file_exists(output)) {
        printf("[\u5931\u8d25] \u672a\u751f\u6210\u5185\u6838\u955c\u50cf: %s\n", output);
        return false;
    }
    
    // 运\u884c QEMU \u6d4b\u8bd5\uff0c\u9a8c\u8bc1\u8f93\u51fa
    if (!run_qemu_test(output, "PASS")) {
        printf("[\u5931\u8d25] QEMU \u6d4b\u8bd5\u5931\u8d25\u6216\u8f93\u51fa\u4e0d\u5339\u914d\n");
        remove(output);
        return false;
    }
    
    printf("[\u6210\u529f] \u5185\u6838\u8f93\u51fa\u9a8c\u8bc1\u901a\u8fc7\n");
    
    // 清理输出文件
    remove(output);
    
    return true;
}

// 测试 Hello Kernel 示例（验收用例）
static bool test_hello_kernel_example() {
    printf("\n=== 测试：Hello Kernel 示例（阶段 5 验收） ===\n");
    
    const char *source = "kernels/hello_kernel.cn";
    const char *output = "test_hello_kernel.elf";
    
    // 检查源文件是否存在
    if (!file_exists(source)) {
        printf("[失败] 找不到内核源文件: %s\n", source);
        return false;
    }
    
    // 构建内核（使用 boot_hello.c 启动代码）
    printf("[测试] 编译 Hello Kernel 示例...\n");
    
#ifdef _WIN32
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "pwsh.exe -ExecutionPolicy Bypass -File scripts/build_kernel.ps1 "
             "-KernelSource \"%s\" -OutputImage \"%s\" "
             "-BootCode \"boot/boot_hello.c\" "
             "-CncPath \"../../../build/src/cnc.exe\"",
             source, output);
    
    int result = system(cmd);
    if (result != 0) {
        printf("[失败] Hello Kernel 编译失败\n");
        return false;
    }
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
             "./scripts/build_kernel.ps1 '%s' '%s' -BootCode 'boot/boot_hello.c'",
             source, output);
    
    int result = system(cmd);
    if (!(WIFEXITED(result) && WEXITSTATUS(result) == 0)) {
        printf("[失败] Hello Kernel 编译失败\n");
        return false;
    }
#endif
    
    // 检查输出文件
    if (!file_exists(output)) {
        printf("[失败] 未生成内核镜像: %s\n", output);
        return false;
    }
    
    printf("[成功] Hello Kernel 编译成功\n");
    
    // 如果 QEMU 可用，运行并验证输出
    if (is_qemu_available()) {
        printf("[测试] 在 QEMU 中验证输出...\n");
        
        // 验证关键输出："Hello from CN Kernel"
        if (!run_qemu_test(output, "Hello from CN Kernel")) {
            printf("[失败] QEMU 输出验证失败\n");
            remove(output);
            return false;
        }
        
        printf("[成功] Hello Kernel 输出验证通过\n");
    } else {
        printf("[跳过] QEMU 未安装，跳过输出验证\n");
    }
    
    // 清理输出文件
    remove(output);
    
    return true;
}

int main(int argc, char **argv) {
    printf("=== CN Language OS 集成测试 ===\n");
    
    // 注意：此测试需要在 tests/integration/os/ 目录下运行
    
    bool all_passed = true;
    
    // 测试 1: 最小内核编译
    if (!test_minimal_kernel_build()) {
        all_passed = false;
    }
    
    // 测试 2: 带输出验证的内核（需要 QEMU）
    if (!test_kernel_with_output_validation()) {
        all_passed = false;
    }
    
    // 测试 3: Hello Kernel 示例（阶段 5 验收用例）
    if (!test_hello_kernel_example()) {
        all_passed = false;
    }
    
    // 总结
    printf("\n=== 测试结果 ===\n");
    if (all_passed) {
        printf("[通过] 所有测试通过\n");
        return 0;
    } else {
        printf("[失败] 部分测试失败\n");
        return 1;
    }
}
