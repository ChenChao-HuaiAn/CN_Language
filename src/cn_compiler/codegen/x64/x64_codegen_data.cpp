// CN Win x64 代码生成器——模块装配与函数框架（D1 行数整改 119-a：自 x64_codegen.cpp 按族拆出）
//   族 = 模块级数据段与常量文本（escapeString 字符串转义 / hexBytesString 字节序列 / uint64HexText 十六进制文本 / floatBitsHex 浮点位模式 / registerFloatConstant 浮点常量池注册 / emitDataSection 数据段发射）；纯重构零行为变更（成员函数实现搬迁——声明仍在对应 hpp）。
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 模块级汇编生成 ====================

// 字符串转汇编字面量（转义反斜杠/引号/控制字符，UTF-8字节保留）
std::string X64CodeGenerator::escapeString(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\0': out += "\\0"; break;
            default:
                out += static_cast<char>(c);
                break;
        }
    }
    return out;
}

// 字符串转MASM db十六进制字节序列（UTF-8字节逐字节 0XXh，逗号分隔）
// ml64 对原始UTF-8中文字节报 A2044（invalid character），十六进制字节可避免
std::string X64CodeGenerator::hexBytesString(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        if (!out.empty()) out += ",";
        char buf[8];
        std::snprintf(buf, sizeof(buf), "0%02Xh", static_cast<int>(c));
        out += buf;
    }
    return out;
}

// 浮点常量文本 -> IEEE754位模式十六进制（f32 转 uint32、f64 转 uint64）
// 64位无符号整数 -> MASM 立即数十六进制文本
// MASM 不接受 0x 前缀（A2206），必须 数字+h；首字符为字母时加 0 前缀（A2085）
std::string X64CodeGenerator::uint64HexText(std::uint64_t value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(value));
    std::string hex = buf;
    if (!hex.empty() && (hex[0] >= 'A' && hex[0] <= 'F')) hex = "0" + hex;
    return hex + "h";
}

// 返回 MASM 兼容十六进制文本（如 f64 1.5 -> "3FF8000000000000h"），供 .data 段生成字节
// 注意：MASM 不接受 0x 前缀（A2206 missing operator），必须用 数字+h 格式
std::string X64CodeGenerator::floatBitsHex(const std::string& text, bool isDouble) {
    std::uint64_t bits = 0;
    try {
        const double value = std::stod(text);
        if (isDouble) {
            // f64：直接按 double 位模式
            std::memcpy(&bits, &value, sizeof(double));
        } else {
            // f32：先转 float（截断），再取位模式
            const float f = static_cast<float>(value);
            std::uint32_t bits32 = 0;
            std::memcpy(&bits32, &f, sizeof(float));
            bits = bits32;
        }
    } catch (...) {
        bits = 0;  // 解析失败按0处理（词法层已保证合法）
    }
    char buf[32];
    // MASM 十六进制：以十六进制数字开头（避免 A2085 以字母开头需 0 前缀），
    // 尾部加 h。f32 8位十六进制（4字节）、f64 16位（8字节）
    std::snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(bits));
    std::string hex = buf;
    // 审查修复：负数浮点位模式（如 -3.75 -> C00E000000000000h）以字母 C 开头，
    //   ml64 报 A2006 undefined symbol（把 C00E... 当标识符）。
    //   与 uint64HexText 一致：以 A-F 开头时补前导 0。
    if (!hex.empty() && (hex[0] >= 'A' && hex[0] <= 'F')) hex = "0" + hex;
    return hex + "h";
}

// 在 .data 段登记浮点常量（@fpN），重复文本复用同一标签
// 标签按登记顺序编号：@fp0/@fp1/...（与 emitDataSection 发射顺序一致）
std::string X64CodeGenerator::registerFloatConstant(const std::string& text, bool isDouble) {
    const std::string key = (isDouble ? "d:" : "f:") + text;
    auto it = floatConstLabels_.find(key);
    if (it != floatConstLabels_.end()) return it->second;
    const std::string label = "@fp" + std::to_string(floatConstOrder_.size());
    floatConstLabels_[key] = label;
    floatConstOrder_.push_back(key);
    return label;
}

// 生成 .data 段（字符串常量池 @str0/@str1/... + 浮点常量池 @fp0/@fp1/...，Task 2.3）
void X64CodeGenerator::emitDataSection(AsmWriter& writer, const ir::IRModule& module) {
    writer.raw(".data");
    bool hasAny = false;
    // 字符串常量（原有）：长字符串（中文多字节/长文本）拆分为多行 db 定义，
    // 避免单行字节过多触发 ml64 A2042（statement too complex，集成验证发现）
    for (std::size_t i = 0; i < module.stringConstants.size(); ++i) {
        std::string label = "@str" + std::to_string(i);
        const std::string text = module.stringConstants[i];
        if (text.empty()) {
            writer.raw(label + " db 0");
            hasAny = true;
            continue;
        }
        // 每行最多 MAX_BYTES_PER_DB 个字节（ml64 单行过长会报 A2042；
        // 行尾不得有逗号——MASM 尾逗号报 A2008 syntax error）
        const std::size_t maxBytesPerLine = 24;
        std::size_t pos = 0;
        bool first = true;
        while (pos < text.size()) {
            const std::size_t chunk = std::min(maxBytesPerLine, text.size() - pos);
            std::string line = first ? (label + " db ") : "      db ";
            line += hexBytesString(text.substr(pos, chunk));
            writer.raw(line);
            pos += chunk;
            first = false;
        }
        // 以 0 结尾（C 字符串）
        writer.raw("      db 0");
        hasAny = true;
    }
    // 浮点常量（Task 2.3：MASM不支持浮点立即数，常量存 .data 段，SSE 用 movsd/movss 加载）
    for (const std::string& key : floatConstOrder_) {
        const bool isDouble = (key.compare(0, 2, "d:") == 0);
        const std::string text = key.substr(2);
        const std::string label = floatConstLabels_[key];
        if (isDouble) {
            // f64：8字节 QWORD（低位在前，MASM dq 已按小端）
            writer.raw(label + " dq " + floatBitsHex(text, true));
        } else {
            // f32：4字节 DWORD
            writer.raw(label + " dd " + floatBitsHex(text, false));
        }
        hasAny = true;
    }
    // ---- 第 9 层 Debug（P3-8）：顶层静态变量 .data 全局存储 ----
    // 符号 ?gstatic_名（与 IR 层 ConstString "?gstatic_名" 一致，nameMangle 修饰）。
    // 初始值为字面量时写入；无初始值/表达式时零初始化。
    for (const auto& kv : module.globalStatics) {
        const std::string& name = kv.first;
        const std::string stType = kv.second;
        const std::string sym = "?gstatic_" + nameMangle(name);
        std::string initText;
        const auto initIt = module.globalStaticInits.find(name);
        if (initIt != module.globalStaticInits.end()) initText = initIt->second;
        // 按类型分配：i128 16 字节双槽；f32 4 字节；结构体按类型大小；其余 8 字节
        // 宿主根治（2026-09-01）：类/容器静态统一「指针槽模型」——.data 符号只存
        //   8 字节对象指针（主 入口 NewObject + 构造 + StorePtr 入槽，与局部类
        //   变量槽同构）。原按 typeSizeOf 分配对象本体是「对象内联 .data」模型
        //   （与读取路径 LoadPtr 不符——读出首 8 字节字段当指针，实测空指针崩溃）。
        //   结构体静态保持值语义按类型大小分配（无构造/指针语义）。
        // 87-a（2026-09-12 第八十七轮）：与 linux_x64/arm64 口径统一——ALIGN 8
        //   （字符串常量池 .byte 长度任意，静态槽前须 8 对齐）+ canonical 判定 +
        //   字符串初值文本（含引号）恒零（不可作 .data 初始值；运行期入口注入）。
        const std::string canonStatic = types::canonical(stType);
        int qwords = 1;
        if (semantic_ != nullptr &&
            semantic_->isStructType(canonStatic) && !types::isPointer(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType);
            if (sz > 8) qwords = (sz + 7) / 8;
        }
        writer.raw("ALIGN 8");
        // 331-a（T52·T50 同族）：128 位判定——T46（467-a）起收敛
        //   types::isInt128Type（单一归属·原四处四连 ‖ 同串重复）。
        // T46（467-a）缺口①：128 位分支发射初值双 quad——原硬编码
        //   "dq 0, 0" 无视 initText（.data 恒零占位·静态 整128 a = 42;
        //   实测读 0）。初值经 parseInt128InitText 解析（raw 文本可能超出
        //   int64 表示域·唯一完整信息源），失败/无初值保持零占位
        //   （超界编译期拒绝归 T9 方案 D 辖区）。
        if (types::isInt128Type(canonStatic)) {
            unsigned long long lo = 0, hi = 0;
            const bool ok = !initText.empty() &&
                types::parseInt128InitText(
                    initText, types::isInt128Signed(canonStatic), lo, hi);
            writer.raw(sym + " dq " + (ok ? uint64HexText(lo) : "0") + ", " +
                       (ok ? uint64HexText(hi) : "0"));
        } else if (canonStatic == "浮32") {
            // f32 初始值：dd floatBitsHex；无初始值零初始化
            writer.raw(sym + " dd " +
                       (!initText.empty() ? floatBitsHex(initText, false) : "0"));
        } else if (canonStatic == "浮64") {
            // f64 初始值：dq floatBitsHex（MASM 无浮点立即数，须位模式十六进制）
            writer.raw(sym + " dq " +
                       (!initText.empty() ? floatBitsHex(initText, true) : "0"));
        } else if (qwords > 1) {
            // 结构体对象：按类型大小分配多 qword（零初始化；初值由入口注入逐字段写）
            std::string line = sym + " dq 0";
            for (int qi = 1; qi < qwords; ++qi) line += ", 0";
            writer.raw(line);
        } else {
            // 331-a（T50 根治·三后端单一归属）：整数/布尔/字符初值直存判定改
            //   types::isStaticScalarInitType（原「非浮点文本」宽松判定与
            //   linux_x64/arm64 的 types::isInteger 判定分叉——同一源码同一语义
            //   三后端口径必须一致；字符串句柄仍置零，运行期入口注入物化）。
            if (!initText.empty() && types::isStaticScalarInitType(canonStatic)) {
                writer.raw(sym + " dq " + initText);
            } else {
                writer.raw(sym + " dq 0");
            }
        }
        writer.comment("顶层静态 " + name + "（" + stType + "）");
        hasAny = true;
    }
    if (!hasAny) writer.comment("（无常量）");
}

} // namespace cn_compiler
