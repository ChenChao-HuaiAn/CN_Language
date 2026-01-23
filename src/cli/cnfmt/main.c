#include "cnlang/format/formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

// 打印使用说明
static void print_usage(const char *program_name)
{
    printf("CN Language 格式化工具 (cnfmt)\n\n");
    printf("用法: %s [选项] <文件或目录>\n\n", program_name);
    printf("选项:\n");
    printf("  -i, --in-place       原地修改文件（默认输出到标准输出）\n");
    printf("  -o <文件>            指定输出文件路径\n");
    printf("  -r, --recursive      递归格式化目录下所有 .cn 文件\n");
    printf("  --check              仅检查模式：检查文件是否需要格式化，不修改文件\n");
    printf("                       如果文件需要格式化，返回非零退出码\n");
    printf("  --verify-idempotence 验证格式化幂等性：检查多次格式化结果是否一致\n");
    printf("  --indent-size <n>    设置缩进空格数（默认 4）\n");
    printf("  --max-line-width <n> 设置最大行宽（默认 100）\n");
    printf("  --no-space-ops       运算符两侧不加空格\n");
    printf("  --no-space-comma     逗号后不加空格\n");
    printf("  --brace-same-line    函数定义的大括号不独立成行\n");
    printf("  -h, --help           显示此帮助信息\n\n");
    printf("示例:\n");
    printf("  %s hello.cn                  # 格式化文件并输出到标准输出\n", program_name);
    printf("  %s -i hello.cn               # 原地修改文件\n", program_name);
    printf("  %s -o output.cn hello.cn     # 格式化并保存到指定文件\n", program_name);
    printf("  %s -r -i examples/           # 递归格式化目录下所有 .cn 文件\n", program_name);
    printf("  %s --check hello.cn          # 检查文件是否需要格式化\n", program_name);
    printf("  %s --verify-idempotence hello.cn  # 验证格式化幂等性\n", program_name);
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

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // 解析命令行参数
    CnFormatOptions options;
    cn_format_options_init_default(&options);
    
    const char *input_path = NULL;
    bool recursive = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--in-place") == 0) {
            options.in_place = true;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "错误: -o 选项需要指定输出文件路径\n");
                return 1;
            }
            options.output_file = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            recursive = true;
        } else if (strcmp(argv[i], "--check") == 0) {
            options.check_only = true;
        } else if (strcmp(argv[i], "--verify-idempotence") == 0) {
            options.verify_idempotence = true;
        } else if (strcmp(argv[i], "--indent-size") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "错误: --indent-size 选项需要指定数值\n");
                return 1;
            }
            options.config.indent_size = atoi(argv[++i]);
            if (options.config.indent_size <= 0) {
                fprintf(stderr, "错误: 缩进大小必须为正整数\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--max-line-width") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "错误: --max-line-width 选项需要指定数值\n");
                return 1;
            }
            options.config.max_line_width = atoi(argv[++i]);
            if (options.config.max_line_width <= 0) {
                fprintf(stderr, "错误: 最大行宽必须为正整数\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--no-space-ops") == 0) {
            options.config.space_around_ops = false;
        } else if (strcmp(argv[i], "--no-space-comma") == 0) {
            options.config.space_after_comma = false;
        } else if (strcmp(argv[i], "--brace-same-line") == 0) {
            options.config.brace_on_new_line_func = false;
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

    // 检查输出选项冲突
    if (options.in_place && options.output_file) {
        fprintf(stderr, "错误: -i 和 -o 选项不能同时使用\n");
        return 1;
    }

    // check_only 和 verify_idempotence 模式与输出选项不兼容
    if ((options.check_only || options.verify_idempotence) && (options.in_place || options.output_file)) {
        fprintf(stderr, "错误: --check 和 --verify-idempotence 模式不能与 -i/-o 选项同时使用\n");
        return 1;
    }

    if (!options.in_place && !options.output_file && !options.check_only) {
        options.to_stdout = true;
    }

    // 处理输入
    bool success = false;
    
    if (is_directory(input_path)) {
        if (!recursive) {
            fprintf(stderr, "错误: %s 是目录，请使用 -r 选项进行递归格式化\n", input_path);
            return 1;
        }
        
        if (options.output_file) {
            fprintf(stderr, "错误: 递归格式化目录时不能使用 -o 选项\n");
            return 1;
        }

        if (options.to_stdout) {
            fprintf(stderr, "错误: 递归格式化目录时必须使用 -i 选项进行原地修改\n");
            return 1;
        }

        printf("正在递归格式化目录: %s\n", input_path);
        int count = cn_format_directory(input_path, &options);
        if (count >= 0) {
            printf("\n成功格式化 %d 个文件\n", count);
            success = true;
        } else {
            fprintf(stderr, "格式化目录失败\n");
            success = false;
        }
    } else {
        // 格式化单个文件
        success = cn_format_file(input_path, &options);
        if (success) {
            if (options.in_place) {
                printf("已成功格式化文件: %s\n", input_path);
            } else if (options.output_file) {
                printf("已成功格式化并保存到: %s\n", options.output_file);
            }
        } else {
            fprintf(stderr, "格式化文件失败: %s\n", input_path);
        }
    }

    return success ? 0 : 1;
}
