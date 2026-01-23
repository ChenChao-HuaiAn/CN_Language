#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define HELLO_EXE ".\\hello_test.exe"
#else
#define HELLO_EXE "./hello_test"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <cnc 可执行文件路径>\n", argv[0]);
        return 1;
    }

    // 设置环境变量以便 cnc 找到运行时
#ifdef _WIN32
    // Windows 使用 CMake 多配置生成器，库文件在 Debug/Release 子目录
    _putenv("CN_RUNTIME_PATH=../../../build/src/runtime/Debug/cn_runtime.lib");
    _putenv("CN_RUNTIME_HEADER_PATH=../../../include/cnrt.h");
#else
    setenv("CN_RUNTIME_PATH", "../../../build/src/runtime/libcn_runtime.a", 1);
    setenv("CN_RUNTIME_HEADER_PATH", "../../../include/cnrt.h", 1);
#endif

    const char* cnc_path = argv[1];
    char cmd[1024];

    // 测试 1: hello_compile.cn
    snprintf(cmd, sizeof(cmd), "%s ../../../examples/hello_compile.cn -o hello_test", cnc_path);
    printf("正在运行: %s\n", cmd);
    int res = system(cmd);
    if (res != 0) {
        fprintf(stderr, "测试 1 编译失败: %d\n", res);
        return 1;
    }

    printf("正在运行生成的程序: %s\n", HELLO_EXE);
    res = system(HELLO_EXE);
    if (res != 0) {
        fprintf(stderr, "测试 1 运行失败: %d\n", res);
        return 1;
    }

    // 测试 2: arithmetic_compile.cn
    snprintf(cmd, sizeof(cmd), "%s ../../../examples/arithmetic_compile.cn -o arithmetic_test", cnc_path);
    printf("正在运行: %s\n", cmd);
    res = system(cmd);
    if (res != 0) {
        fprintf(stderr, "测试 2 编译失败: %d\n", res);
        return 1;
    }

    #ifdef _WIN32
    const char* arithmetic_exe = ".\\arithmetic_test.exe";
    #else
    const char* arithmetic_exe = "./arithmetic_test";
    #endif

    printf("正在运行生成的程序: %s\n", arithmetic_exe);
    res = system(arithmetic_exe);
    if (res != 0) {
        fprintf(stderr, "测试 2 运行失败: %d\n", res);
        return 1;
    }

    // 测试 3: control_flow_compile.cn
    snprintf(cmd, sizeof(cmd), "%s ../../../examples/control_flow_compile.cn -o control_flow_test", cnc_path);
    printf("正在运行: %s\n", cmd);
    res = system(cmd);
    if (res != 0) {
        fprintf(stderr, "测试 3 编译失败: %d\n", res);
        return 1;
    }

    #ifdef _WIN32
    const char* control_flow_exe = ".\\control_flow_test.exe";
    #else
    const char* control_flow_exe = "./control_flow_test";
    #endif

    printf("正在运行生成的程序: %s\n", control_flow_exe);
    res = system(control_flow_exe);
    if (res != 0) {
        fprintf(stderr, "测试 3 运行失败: %d\n", res);
        return 1;
    }

    // 测试 4: runtime_test_full.cn
    snprintf(cmd, sizeof(cmd), "%s ../../../examples/runtime_test_full.cn -o runtime_full_test", cnc_path);
    printf("正在运行: %s\n", cmd);
    res = system(cmd);
    if (res != 0) {
        fprintf(stderr, "测试 4 编译失败: %d\n", res);
        return 1;
    }

    #ifdef _WIN32
    const char* runtime_full_exe = ".\\runtime_full_test.exe";
    #else
    const char* runtime_full_exe = "./runtime_full_test";
    #endif

    printf("正在运行生成的程序: %s\n", runtime_full_exe);
    
    // 捕获输出并检查关键内容
    #ifdef _WIN32
    char capture_cmd[1024];
    snprintf(capture_cmd, sizeof(capture_cmd), "%s > runtime_full_output.txt 2>&1", runtime_full_exe);
    res = system(capture_cmd);
    #else
    char capture_cmd[1024];
    snprintf(capture_cmd, sizeof(capture_cmd), "%s > runtime_full_output.txt 2>&1", runtime_full_exe);
    res = system(capture_cmd);
    #endif
    
    if (res != 0) {
        fprintf(stderr, "测试 4 运行失败: %d\n", res);
        return 1;
    }
    
    // 读取输出并验证关键内容
    FILE* output_file = fopen("runtime_full_output.txt", "r");
    if (output_file) {
        char line[256];
        int found_hello = 0;
        
        while (fgets(line, sizeof(line), output_file)) {
            // 检查是否包含“你好”
            if (strstr(line, "你好") || strstr(line, "CN_Language")) {
                found_hello = 1;
                printf("检测到期望输出: %s", line);
            }
        }
        fclose(output_file);
        
        if (!found_hello) {
            fprintf(stderr, "测试 4 输出检查失败: 未找到期望的字符串内容\n");
            return 1;
        }
        
        printf("测试 4 输出检查通过!\n");
    } else {
        fprintf(stderr, "无法读取输出文件\n");
        return 1;
    }

    // 测试 5: 显式指定目标三元组 --target=x86_64-elf
    snprintf(cmd, sizeof(cmd), "%s ../../../examples/hello_compile.cn --target=x86_64-elf -o hello_target_test", cnc_path);
    printf("正在运行: %s\n", cmd);
    res = system(cmd);
    if (res != 0) {
        fprintf(stderr, "测试 5 编译失败: %d\n", res);
        return 1;
    }

#ifdef _WIN32
    const char* hello_target_exe = ".\\hello_target_test.exe";
#else
    const char* hello_target_exe = "./hello_target_test";
#endif

    printf("正在运行生成的程序: %s\n", hello_target_exe);
    res = system(hello_target_exe);
    if (res != 0) {
        fprintf(stderr, "测试 5 运行失败: %d\n", res);
        return 1;
    }

    printf("集成编译测试全部通过!\n");
    return 0;
}
