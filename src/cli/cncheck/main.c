#include "cnlang/analysis/static_check.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#else
#include <sys/stat.h>
#include <dirent.h>
#define PATH_SEPARATOR '/'
#endif

// 打印使用说明
static void print_usage(const char *program_name)
{
    printf("CN Language 静态检查工具 (cncheck)\n\n");
    printf("用法: %s [选项] <文件或目录>\n\n", program_name);
    printf("选项:\n");
    printf("  -r, --recursive          递归检查目录下所有 .cn 文件\n");
    printf("  --check-only             仅检查返回码模式（用于 CI）\n");
    printf("  --disable-unused-var     禁用未使用变量检查\n");
    printf("  --disable-unused-param   禁用未使用参数检查\n");
    printf("  --disable-complexity     禁用复杂度检查\n");
    printf("  --max-statements <n>     设置函数最大语句数（默认 50）\n");
    printf("  --max-nesting <n>        设置最大嵌套层级（默认 5）\n");
    printf("  -h, --help               显示此帮助信息\n\n");
    printf("示例:\n");
    printf("  %s hello.cn                      # 检查单个文件\n", program_name);
    printf("  %s -r examples/                  # 递归检查目录\n", program_name);
    printf("  %s --check-only hello.cn         # CI 模式，仅返回码\n", program_name);
    printf("  %s --max-statements 30 hello.cn  # 自定义复杂度阈值\n", program_name);
}

// 检查路径是否为目录
static bool is_directory(const char *path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
#endif
}

// 检查文件是否以 .cn 结尾
static bool is_cn_file(const char *filename)
{
    size_t len = strlen(filename);
    return len > 3 && strcmp(filename + len - 3, ".cn") == 0;
}

// 检查单个文件
static bool check_single_file(
    const char *filepath,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics)
{
    if (!config->check_only) {
        printf("检查文件: %s\n", filepath);
    }

    bool success = cn_check_file(filepath, config, diagnostics);

    if (!config->check_only) {
        if (success) {
            printf("  ✓ 检查通过\n");
        } else {
            printf("  ✗ 发现问题\n");
        }
    }

    return success;
}

// 递归检查目录
static int check_directory(
    const char *dir_path,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics)
{
    int file_count = 0;

#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);

    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "无法打开目录: %s\n", dir_path);
        return -1;
    }

    do {
        const char *name = find_data.cFileName;
        
        // 跳过 . 和 ..
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, name);

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 递归检查子目录
            int sub_count = check_directory(full_path, config, diagnostics);
            if (sub_count > 0) {
                file_count += sub_count;
            }
        } else if (is_cn_file(name)) {
            // 检查 .cn 文件
            check_single_file(full_path, config, diagnostics);
            file_count++;
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
#else
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "无法打开目录: %s\n", dir_path);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        
        // 跳过 . 和 ..
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);

        if (is_directory(full_path)) {
            // 递归检查子目录
            int sub_count = check_directory(full_path, config, diagnostics);
            if (sub_count > 0) {
                file_count += sub_count;
            }
        } else if (is_cn_file(name)) {
            // 检查 .cn 文件
            check_single_file(full_path, config, diagnostics);
            file_count++;
        }
    }

    closedir(dir);
#endif

    return file_count;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // 初始化检查配置
    CnCheckConfig config;
    cn_check_config_init_default(&config);
    
    const char *input_path = NULL;
    bool recursive = false;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            recursive = true;
        } else if (strcmp(argv[i], "--check-only") == 0) {
            config.check_only = true;
        } else if (strcmp(argv[i], "--disable-unused-var") == 0) {
            config.enabled_rules[CN_CHECK_RULE_UNUSED_VAR] = false;
        } else if (strcmp(argv[i], "--disable-unused-param") == 0) {
            config.enabled_rules[CN_CHECK_RULE_UNUSED_PARAM] = false;
        } else if (strcmp(argv[i], "--disable-complexity") == 0) {
            config.enabled_rules[CN_CHECK_RULE_COMPLEXITY] = false;
        } else if (strcmp(argv[i], "--max-statements") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "错误: --max-statements 选项需要指定数值\n");
                return 1;
            }
            config.max_statements_per_function = atoi(argv[++i]);
            if (config.max_statements_per_function <= 0) {
                fprintf(stderr, "错误: 最大语句数必须为正整数\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--max-nesting") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "错误: --max-nesting 选项需要指定数值\n");
                return 1;
            }
            config.max_nesting_level = atoi(argv[++i]);
            if (config.max_nesting_level <= 0) {
                fprintf(stderr, "错误: 最大嵌套层级必须为正整数\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            if (input_path != NULL) {
                fprintf(stderr, "错误: 只能指定一个输入文件或目录\n");
                return 1;
            }
            input_path = argv[i];
        } else {
            fprintf(stderr, "错误: 未知选项 %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!input_path) {
        fprintf(stderr, "错误: 未指定输入文件或目录\n");
        print_usage(argv[0]);
        return 1;
    }

    // 初始化诊断系统
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    // 执行检查
    bool success = true;
    int file_count = 0;

    if (is_directory(input_path)) {
        if (!recursive) {
            fprintf(stderr, "错误: %s 是目录，请使用 -r 选项进行递归检查\n", input_path);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }

        if (!config.check_only) {
            printf("正在递归检查目录: %s\n", input_path);
        }
        
        file_count = check_directory(input_path, &config, &diagnostics);
        
        if (file_count < 0) {
            fprintf(stderr, "检查目录失败\n");
            success = false;
        } else if (!config.check_only) {
            printf("\n共检查 %d 个文件\n", file_count);
        }
    } else {
        // 检查单个文件
        success = check_single_file(input_path, &config, &diagnostics);
        file_count = 1;
    }

    // 打印诊断信息（除非在 check_only 模式）
    if (!config.check_only) {
        printf("\n");
        cn_support_diagnostics_print(&diagnostics);
    }

    // 统计结果
    size_t warning_count = diagnostics.count - cn_support_diagnostics_error_count(&diagnostics);
    size_t error_count = cn_support_diagnostics_error_count(&diagnostics);

    if (!config.check_only) {
        printf("\n========== 检查结果 ==========\n");
        printf("检查文件数: %d\n", file_count);
        printf("警告: %zu\n", warning_count);
        printf("错误: %zu\n", error_count);
        
        if (error_count > 0 || warning_count > 0) {
            printf("状态: ✗ 发现问题\n");
        } else {
            printf("状态: ✓ 检查通过\n");
        }
    }

    // 清理资源
    cn_support_diagnostics_free(&diagnostics);

    // 返回码：存在错误或警告时返回非零
    return (error_count > 0 || warning_count > 0) ? 1 : 0;
}
