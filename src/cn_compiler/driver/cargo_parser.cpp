// CN语言编译器 货舱.toml 解析器实现（模块系统 v2.0 第 5 层，规格书09-二）
// TOML 子集：支持 [节] 标题、键 = "值"（双引号字符串）、# 注释（整行/行尾）、空行。
// 不引入外部 TOML 库——规范 2.1 规定货舱.toml 为 TOML 子集，本解析器覆盖全部所需语法。
// 单文件 <=1000 行、单函数 <=100 行约束。
#include "cn_compiler/driver/cargo_parser.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>  // MultiByteToWideChar：UTF-8 路径 -> UTF-16 宽路径（_wfopen）
#endif

namespace cn_compiler {
namespace driver {

namespace {

// 去除首尾空白（空格/制表/回车/换行）
std::string trimSpaces(const std::string& text) {
    std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

// 去除行内注释：在双引号字符串之外遇到 # 即截断（TOML 行尾注释）。
// 返回去除注释后的文本；扫描时跟踪引号状态，避免把字符串内的 # 当注释。
std::string stripComment(const std::string& line) {
    bool inString = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            inString = !inString;  // 转义忽略（TOML 子集无 \x 转义需求，够用）
        } else if (ch == '#' && !inString) {
            return line.substr(0, i);
        }
    }
    return line;
}

// 解析双引号字符串值：输入为 trim 后的整行（键 = "值" 或 键 = "值" # 注释 已截断）。
// 提取第一个 " 与最后一个 " 之间的内容；失败返回 false 并写 error。
bool parseQuotedValue(const std::string& line, std::string& out, std::string& error) {
    const std::size_t first = line.find('"');
    if (first == std::string::npos) {
        error = "值应为双引号字符串: " + line;
        return false;
    }
    // 找配对的结束引号（跳过可能存在的转义；TOML 子集仅保留双引号内容原样）
    std::size_t last = line.find('"', first + 1);
    if (last == std::string::npos) {
        error = "字符串缺少结束引号: " + line;
        return false;
    }
    out = line.substr(first + 1, last - first - 1);
    return true;
}

} // namespace

// 按依赖名查找（未找到返回 nullptr）
const CargoDependency* CargoConfig::findDependency(const std::string& depName) const {
    for (const auto& dep : deps) {
        if (dep.name == depName) return &dep;
    }
    return nullptr;
}

// 解析 TOML 字符串数组值：["名1", "名2"]（239-a [特性] 启用 用）。
//   提取方括号内全部双引号字符串（逗号/空白分隔；空数组 [] 合法）。
//   失败返回 false 并写 error。
bool parseQuotedArray(const std::string& line, std::vector<std::string>& out,
                      std::string& error) {
    const std::size_t open = line.find('[');
    if (open == std::string::npos) {
        error = "值应为字符串数组 [\"名\", ...]: " + line;
        return false;
    }
    const std::size_t close = line.find(']', open);
    if (close == std::string::npos) {
        error = "数组缺少 ]: " + line;
        return false;
    }
    std::string body = line.substr(open + 1, close - open - 1);
    std::size_t pos = 0;
    while (true) {
        const std::size_t first = body.find('"', pos);
        if (first == std::string::npos) break;
        const std::size_t last = body.find('"', first + 1);
        if (last == std::string::npos) {
            error = "数组元素缺少结束引号: " + line;
            return false;
        }
        out.push_back(body.substr(first + 1, last - first - 1));
        pos = last + 1;
    }
    return true;
}

// 解析 货舱.toml 文本（TOML 子集）：
//   [货舱]  -> 名称/版本；[依赖] -> 名称 = "版本"；[特性] -> 启用 = ["名", ...]
// 未知节/未知键忽略（宽容扩展）；[依赖] 中 键 = "值" 每行一个依赖。
bool parseCargoToml(const std::string& text, CargoConfig& out, std::string& error) {
    out = CargoConfig();  // 重置（幂等解析）
    error.clear();

    std::istringstream stream(text);
    std::string line;
    std::string section;  // 当前节名（货舱/依赖；其他节忽略）
    while (std::getline(stream, line)) {
        // 去注释（字符串外 # 截断）-> 去首尾空白
        const std::string stripped = trimSpaces(stripComment(line));
        if (stripped.empty()) continue;  // 空行/纯注释

        // 节标题：[货舱] / [依赖]
        if (stripped.front() == '[') {
            const std::size_t close = stripped.find(']');
            if (close == std::string::npos) {
                error = "节标题缺少 ]: " + stripped;
                return false;
            }
            section = trimSpaces(stripped.substr(1, close - 1));
            continue;
        }

        // 键值对：键 = "值"
        const std::size_t eq = stripped.find('=');
        if (eq == std::string::npos) {
            error = "无法解析的行（应为 键 = \"值\" 或 [节]）: " + stripped;
            return false;
        }
        const std::string key = trimSpaces(stripped.substr(0, eq));
        const std::string valueLine = trimSpaces(stripped.substr(eq + 1));
        std::string value;
        if (!parseQuotedValue(valueLine, value, error)) return false;

        if (section == "货舱") {
            if (key == "名称") {
                out.name = value;
            } else if (key == "版本") {
                out.version = value;
            }
            // 其他键忽略（宽容）
        } else if (section == "依赖") {
            out.deps.push_back(CargoDependency{key, value});
        } else if (section == "特性") {
            // 239-a：启用 = ["名1", "名2"]（其余键宽容忽略）
            if (key == "启用") {
                if (!parseQuotedArray(valueLine, out.features, error)) return false;
            }
        }
        // 其他节忽略（宽容扩展，如 [构建] 等）
    }
    return true;
}

// 读取 货舱.toml 文件内容（UTF-8 路径兼容）：
//   Windows 下路径可能由"ASCII 目录 + UTF-8 文件名"拼成（自动发现），
//   std::ifstream 窄字符按 ANSI 解释会失败——复用 module::readSourceFile 的
//   先窄后宽策略（与 数学.cn 等中文模块名同机制，Task 3.6 已验证）。
// 返回 false 表示文件不存在/读取失败（不写 error——loadCargoConfig 负责提示）。
bool readTomlFile(const std::string& tomlPath, std::string& content) {
#ifdef _WIN32
    // 先试窄字符（GBK/ASCII 入口路径，如 --货舱 参数）
    FILE* fp = nullptr;
    if (fopen_s(&fp, tomlPath.c_str(), "rb") != 0 || fp == nullptr) {
        // 窄字符失败：尝试 UTF-8 路径 -> UTF-16 宽路径（自动发现拼出的中文文件名）
        const int wideLen = MultiByteToWideChar(CP_UTF8, 0, tomlPath.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) return false;
        std::vector<wchar_t> widePath(static_cast<std::size_t>(wideLen));
        MultiByteToWideChar(CP_UTF8, 0, tomlPath.c_str(), -1, widePath.data(), wideLen);
        if (_wfopen_s(&fp, widePath.data(), L"rb") != 0 || fp == nullptr) return false;
    }
    std::fseek(fp, 0, SEEK_END);
    const long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    content.clear();
    if (size > 0) {
        content.resize(static_cast<std::size_t>(size));
        content.resize(std::fread(&content[0], 1, static_cast<std::size_t>(size), fp));
    }
    std::fclose(fp);
#else
    std::ifstream in(tomlPath, std::ios::binary);
    if (!in) return false;  // 不存在：非错误（自动发现允许无 货舱.toml）
    std::ostringstream buf;
    buf << in.rdbuf();
    content = buf.str();
#endif
    // 去除 UTF-8 BOM（EF BB BF），与 readSourceFile 一致
    if (content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        content = content.substr(3);
    }
    return true;
}

// 读取并解析 货舱.toml 文件（UTF-8，自动去除 BOM）。
// 文件不存在返回 false 且 error 为空（自动发现场景允许无配置）。
bool loadCargoConfig(const std::string& tomlPath, CargoConfig& out, std::string& error) {
    error.clear();
    std::string content;
    if (!readTomlFile(tomlPath, content)) return false;  // 不存在/读失败（非错误）
    return parseCargoToml(content, out, error);
}

} // namespace driver
} // namespace cn_compiler
