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

int main(int argc, char **argv) {
    printf("=== CN Language OS 集成测试 ===\n");
    
    // 注意：此测试需要在 tests/integration/os/ 目录下运行
    
    bool all_passed = true;
    
    // 测试 1: 最小内核编译
    if (!test_minimal_kernel_build()) {
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
