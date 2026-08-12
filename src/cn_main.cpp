// CN语言编译器命令行入口（阶段零：CLI框架 + 选项解析）
// 命名规范：标识符统一使用英文（GCC 7 不支持中文标识符，中文仅用于注释与输出文本）
#include <iostream>
#include <string>
#include <vector>

// 命令行选项结构：保存解析结果
struct CliOptions {
    std::string target = "win-x64";  // 目标平台
    int optLevel = 2;                // 优化级别
    std::string output;              // 输出文件路径
    bool verbose = false;            // 详细输出
};

// 打印版本信息
void printVersion() {
    std::cout << "CN语言编译器 cn 0.1.0\n";
    std::cout << "目标平台: win-x64 (初期), linux-arm64 (初期)\n";
}

// 打印帮助信息
void printHelp() {
    std::cout << "用法: cn <命令> [选项] <文件>\n";
    std::cout << "\n命令:\n";
    std::cout << "  build <文件.cn>        编译并生成可执行文件\n";
    std::cout << "  compile <文件.cn>      仅编译生成汇编文件(.asm)\n";
    std::cout << "  run <文件.cn>          编译并运行\n";
    std::cout << "  check <文件.cn>        仅检查语法和类型，不生成代码\n";
    std::cout << "  ir <文件.cn>           输出IR（调试用）\n";
    std::cout << "  ast <文件.cn>          输出AST（调试用）\n";
    std::cout << "  token <文件.cn>        输出Token流（调试用）\n";
    std::cout << "\n选项:\n";
    std::cout << "  --target <平台>        目标平台 (win-x64 | linux-arm64)\n";
    std::cout << "  --opt <级别>           优化级别 (0 | 1 | 2)\n";
    std::cout << "  --output <路径>        输出文件路径\n";
    std::cout << "  --verbose              详细输出\n";
    std::cout << "  --version, -v          显示版本信息\n";
    std::cout << "  --help, -h             显示帮助信息\n";
}

// 未实现的命令处理（阶段零仅支持版本/帮助，编译命令后续阶段逐个填充）
int unimplementedCommand(const std::string& command) {
    std::cerr << "错误: 命令 '" << command << "' 尚未实现（请参考实施计划对应阶段）\n";
    return 1;
}

// 解析选项与文件参数
// 参数: 待解析参数列表、当前下标（引用，前进）、选项结构、输出文件路径
// 返回: 错误消息（空字符串表示成功）
std::string parseOptions(const std::vector<std::string>& args, size_t& index,
                         CliOptions& options, std::string& file) {
    while (index < args.size()) {
        const std::string& current = args[index];
        if (current == "--target") {
            if (index + 1 >= args.size()) return "选项 --target 缺少参数";
            options.target = args[++index];
            if (options.target != "win-x64" && options.target != "linux-arm64")
                return "无效目标平台: " + options.target + "（应为 win-x64 或 linux-arm64）";
        } else if (current == "--opt") {
            if (index + 1 >= args.size()) return "选项 --opt 缺少参数";
            const std::string value = args[++index];
            if (value != "0" && value != "1" && value != "2")
                return "无效优化级别: " + value + "（应为 0、1 或 2）";
            options.optLevel = std::stoi(value);
        } else if (current == "--output") {
            if (index + 1 >= args.size()) return "选项 --output 缺少参数";
            options.output = args[++index];
        } else if (current == "--verbose") {
            options.verbose = true;
        } else if (current.rfind("--", 0) == 0) {
            return "未知选项: " + current;
        } else {
            // 位置参数：源文件
            if (!file.empty()) return "多余的源文件参数: " + current;
            file = current;
        }
        index++;
    }
    return "";
}

// 程序入口
int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    // 无参数：打印帮助
    if (args.empty()) {
        printHelp();
        return 0;
    }

    // 第一个参数为命令或帮助/版本选项
    const std::string command = args[0];
    if (command == "--version" || command == "-v") { printVersion(); return 0; }
    if (command == "--help" || command == "-h") { printHelp(); return 0; }

    // 校验是否为已知命令
    static const std::vector<std::string> knownCommands = {
        "build", "compile", "run", "check", "ir", "ast", "token"
    };
    bool isKnownCommand = false;
    for (const auto& c : knownCommands) {
        if (c == command) { isKnownCommand = true; break; }
    }
    if (!isKnownCommand) {
        std::cerr << "错误: 未知命令 '" << command << "'\n";
        printHelp();
        return 1;
    }

    // 解析选项与文件参数
    CliOptions options;
    std::string file;
    size_t index = 1;
    const std::string error = parseOptions(args, index, options, file);
    if (!error.empty()) {
        std::cerr << "错误: " << error << "\n";
        return 1;
    }

    // 编译类命令必须提供源文件
    if (file.empty()) {
        std::cerr << "错误: 命令 '" << command << "' 缺少源文件参数\n";
        printHelp();
        return 1;
    }

    // 详细输出模式：打印解析结果
    if (options.verbose) {
        std::cout << "命令: " << command << "\n";
        std::cout << "文件: " << file << "\n";
        std::cout << "目标平台: " << options.target << "\n";
        std::cout << "优化级别: " << options.optLevel << "\n";
        if (!options.output.empty()) std::cout << "输出: " << options.output << "\n";
    }

    // 阶段零：所有编译命令尚未实现
    return unimplementedCommand(command);
}
