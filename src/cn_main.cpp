// CN语言编译器命令行入口（阶段零：CLI框架 + 选项解析）
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
void 打印版本() {
    std::cout << "CN语言编译器 cn 0.1.0\n";
    std::cout << "目标平台: win-x64 (初期), linux-arm64 (初期)\n";
}

// 打印帮助信息
void 打印帮助() {
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
int 未实现命令(const std::string& 命令) {
    std::cerr << "错误: 命令 '" << 命令 << "' 尚未实现（请参考实施计划对应阶段）\n";
    return 1;
}

// 解析选项与文件参数
// 参数: 待解析参数列表、当前下标（引用，前进）、选项结构、输出文件路径
// 返回: 错误消息（空字符串表示成功）
std::string 解析选项(const std::vector<std::string>& 参数, size_t& 下标,
                     CliOptions& 选项, std::string& 文件) {
    while (下标 < 参数.size()) {
        const std::string& 当前 = 参数[下标];
        if (当前 == "--target") {
            if (下标 + 1 >= 参数.size()) return "选项 --target 缺少参数";
            选项.target = 参数[++下标];
            if (选项.target != "win-x64" && 选项.target != "linux-arm64")
                return "无效目标平台: " + 选项.target + "（应为 win-x64 或 linux-arm64）";
        } else if (当前 == "--opt") {
            if (下标 + 1 >= 参数.size()) return "选项 --opt 缺少参数";
            const std::string 值 = 参数[++下标];
            if (值 != "0" && 值 != "1" && 值 != "2")
                return "无效优化级别: " + 值 + "（应为 0、1 或 2）";
            选项.optLevel = std::stoi(值);
        } else if (当前 == "--output") {
            if (下标 + 1 >= 参数.size()) return "选项 --output 缺少参数";
            选项.output = 参数[++下标];
        } else if (当前 == "--verbose") {
            选项.verbose = true;
        } else if (当前.rfind("--", 0) == 0) {
            return "未知选项: " + 当前;
        } else {
            // 位置参数：源文件
            if (!文件.empty()) return "多余的源文件参数: " + 当前;
            文件 = 当前;
        }
        下标++;
    }
    return "";
}

// 程序入口
int main(int argc, char** argv) {
    std::vector<std::string> 参数(argv + 1, argv + argc);

    // 无参数：打印帮助
    if (参数.empty()) {
        打印帮助();
        return 0;
    }

    // 第一个参数为命令或帮助/版本选项
    const std::string 命令 = 参数[0];
    if (命令 == "--version" || 命令 == "-v") { 打印版本(); return 0; }
    if (命令 == "--help" || 命令 == "-h") { 打印帮助(); return 0; }

    // 校验是否为已知命令
    static const std::vector<std::string> 已知命令 = {
        "build", "compile", "run", "check", "ir", "ast", "token"
    };
    bool 是已知命令 = false;
    for (const auto& c : 已知命令) {
        if (c == 命令) { 是已知命令 = true; break; }
    }
    if (!是已知命令) {
        std::cerr << "错误: 未知命令 '" << 命令 << "'\n";
        打印帮助();
        return 1;
    }

    // 解析选项与文件参数
    CliOptions 选项;
    std::string 文件;
    size_t 下标 = 1;
    const std::string 错误 = 解析选项(参数, 下标, 选项, 文件);
    if (!错误.empty()) {
        std::cerr << "错误: " << 错误 << "\n";
        return 1;
    }

    // 编译类命令必须提供源文件
    if (文件.empty()) {
        std::cerr << "错误: 命令 '" << 命令 << "' 缺少源文件参数\n";
        打印帮助();
        return 1;
    }

    // 详细输出模式：打印解析结果
    if (选项.verbose) {
        std::cout << "命令: " << 命令 << "\n";
        std::cout << "文件: " << 文件 << "\n";
        std::cout << "目标平台: " << 选项.target << "\n";
        std::cout << "优化级别: " << 选项.optLevel << "\n";
        if (!选项.output.empty()) std::cout << "输出: " << 选项.output << "\n";
    }

    // 阶段零：所有编译命令尚未实现
    return 未实现命令(命令);
}
