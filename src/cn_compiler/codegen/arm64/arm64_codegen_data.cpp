// ARM64 代码生成器——模块级数据段与文本段头（2051 拆分 自 arm64_codegen.cpp：
//   872 行+数组常量初始化列表发射将过 900 警戒线·「先拆再加」纪律·与 win
//   x64_codegen_data.cpp / linux_x64_codegen_data.cpp 同构命名）。
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/data_seg_init.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 生成 .data 段（字符串常量池 @str0/@str1/... + 浮点常量池 @fp0/@fp1/...）
void Arm64CodeGenerator::emitDataSection(Arm64AsmWriter& writer,
                                         const ir::IRModule& module) {
    writer.raw(".data");
    bool hasAny = false;
    for (std::size_t i = 0; i < module.stringConstants.size(); ++i) {
        const std::string label = "Lstr" + std::to_string(i);
        const std::string text = module.stringConstants[i];
        writer.raw(label + ":");
        if (text.empty()) {
            writer.raw("    .byte 0");
            hasAny = true;
            continue;
        }
        const std::size_t maxBytesPerLine = 24;
        std::size_t pos = 0;
        while (pos < text.size()) {
            const std::size_t chunk = std::min(maxBytesPerLine, text.size() - pos);
            writer.raw("    .byte " + hexBytesString(text.substr(pos, chunk)));
            pos += chunk;
        }
        writer.raw("    .byte 0");
        hasAny = true;
    }
    for (const std::string& key : floatConstOrder_) {
        const bool isDouble = (key.compare(0, 2, "d:") == 0);
        const std::string text = key.substr(2);
        const std::string label = floatConstLabels_[key];
        writer.raw(label + ":");
        if (isDouble) {
            writer.raw("    .quad " + floatBitsHex(text, true));
        } else {  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行形态原样保留·2051 拆分连带）
            writer.raw("    .word " + floatBitsHex(text, false));
        }
        hasAny = true;
    }
    // ---- 第 9 层 Debug（P3-8）：顶层静态变量 .data 全局存储 ----
    // 符号 _cn_gstatic_名（与 arm64_instructions.cpp ConstString 转换一致）
    // .align 3（2026-09-07）：.quad 槽 8 对齐——全局符号虽可借命名重定位绕过
    // 汇编期对齐检查，但运行期静态访存须对齐（Rust/LLVM 对照：对齐=发射期
    // 布局契约，非对齐访存不进产物）。**arm64 非对齐访存直接陷阱（SIGBUS）**，
    // 87-a 起每个静态槽前重发 .align 3（结构体槽 8 倍数不破坏后续对齐不变式）。
    // 87-a（2026-09-12 第八十七轮）两项根治（与 linux_x64 同口径）：
    //   ① 槽尺寸按类型分配：结构体静态按 typeSizeOf（取整 8 倍数、值本体存储）；
    //      其余恒 8 字节（类/容器=指针槽、字符串=句柄、标量=8 字节槽）。原实现
    //      恒 `.quad 0` → 结构体整体赋值越界写相邻 .data 符号（静默内存破坏）。
    //   ② 标量初值直存：整型/布尔/字符初值原被完全忽略（恒 0）。
    for (const auto& kv : module.globalStatics) {
        const std::string& name = kv.first;
        const std::string stType = kv.second;
        const std::string sym = "_cn_gstatic_" + nameMangle(name);
        std::string initText;
        const auto initIt = module.globalStaticInits.find(name);
        if (initIt != module.globalStaticInits.end()) initText = initIt->second;
        const std::string canonStatic = types::canonical(stType);
        // 槽尺寸（字节）：结构体按类型大小（取整到 8 的倍数）
        // 259（2026-10-07·#259 静态数组）：数组按元素大小×长度分配值本体
        std::size_t bytes = 8;
        if (semantic_ != nullptr && types::isArray(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType).value_or(8);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行·2051 拆分连带）
        } else if (semantic_ != nullptr && !types::isPointer(canonStatic) &&
            semantic_->isStructType(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType).value_or(8);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行·2051 拆分连带）
        }
        writer.raw(".align 3");
        writer.raw(".globl " + sym);
        writer.raw(".type " + sym + ", %object");
        writer.raw(sym + ":");
        // 331-a（T52·T50 同族）：128 位判定——函数内静态局部的 stType
        //   是 IR 名（i128/u128），顶层静态是中文名；T46（467-a）起收敛
        //   types::isInt128Type（单一归属）。
        // T46（467-a）缺口①：发射初值双 quad（低 64 位在前·与双槽内存布局
        //   一致）——原硬编码 .quad 0×2 无视 initText（.data 恒零占位）。
        //   初值经 parseInt128InitText 解析（raw 文本可能超出 int64 表示域），
        //   失败/无初值保持零占位（超界拒绝归 T9 方案 D 辖区）。
        if (types::isInt128Type(canonStatic)) {
            unsigned long long lo = 0, hi = 0;
            const bool ok = !initText.empty() &&
                types::parseInt128InitText(
                    initText, types::isInt128Signed(canonStatic), lo, hi);
            writer.raw("    .quad " + (ok ? uint64HexText(lo) : "0"));
            writer.raw("    .quad " + (ok ? uint64HexText(hi) : "0"));
        } else if (semantic_ != nullptr && types::isArray(canonStatic) &&
                   module.globalStaticArrayInits.count(name) > 0) {
            // 2051：静态/顶层数组常量初始化列表 .data 直发——共享打包
            //   （codegen/data_seg_init.hpp·元素按宽度小端+缺格补零），逐
            //   quad 发射每行 ≤32 值折行（#2050 数据段长链分批同口径）。IR
            //   层直存面已收口整数文本，打包无值=内部缺陷显形（禁静默零填充）。
            const auto& arrInit = module.globalStaticArrayInits.at(name);
            const int slotSz = semantic_->typeSizeOf(stType).value_or(8);
            const cn::Optional<int> elemSz = semantic_->typeSizeOf(
                types::arrayElemOf(canonStatic));
            const auto quads = codegen::packStaticArrayInitQuads(
                arrInit, static_cast<std::size_t>(elemSz.value_or(8)),
                static_cast<std::size_t>(((slotSz + 7) / 8) * 8));
            const bool packedOk = quads.has_value();
            if (!packedOk) {
                diagnostics_.report(Diagnostic::error(
                    SourceLocation(),
                    "内部错误: 数组初始化列表 .data 打包失败（" + name +
                        "）——IR 直存面应为整数族文本"));
            }
            if (packedOk) {
                const auto& qv = quads.value();
                constexpr std::size_t kQuadPerLine = 32;  // 行宽口径（2050 同款）
                std::size_t idx = 0;
                while (idx < qv.size()) {
                    const std::size_t end =
                        std::min(idx + kQuadPerLine, qv.size());
                    std::string line = "    .quad ";
                    for (std::size_t q = idx; q < end; ++q) {
                        if (q != idx) line += ", ";
                        line += uint64HexText(qv[q]);
                    }
                    writer.raw(line);
                    idx = end;
                }
            }
        } else if (canonStatic == "浮32") {
            writer.raw("    .word " +
                       (!initText.empty() ? floatBitsHex(initText, false) : "0"));
        } else if (canonStatic == "浮64") {
            writer.raw("    .quad " +
                       (!initText.empty() ? floatBitsHex(initText, true) : "0"));
        } else if (bytes > 8) {  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行·2051 拆分连带）
            // 结构体静态：值本体按类型尺寸零初始化（初值由入口注入逐字段写）
            writer.raw("    .zero " + std::to_string(bytes));
        } else {  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行形态原样保留·2051 拆分连带）
            // 标量静态：常量初值直存（整型/布尔/字符文本；字符串句柄等保持 0）。
            // 331-a（T50 根治）：判定改 types::isStaticScalarInitType（三后端单一
            //   归属）——原 types::isInteger 只认中文名，函数内静态局部（IR 名
            //   "i64"）判定失败 → 初值恒 .quad 0 静默丢。
            std::string text = "0";
            if (!initText.empty() && types::isStaticScalarInitType(canonStatic)) {
                text = initText;
            }
            writer.raw("    .quad " + text);
        }
        writer.comment("顶层静态 " + name + "（" + stType + "）");
        hasAny = true;
    }
    if (!hasAny) writer.comment("（无常量）");
}

// 生成 .text 段头部（导出函数符号 + 外部被调函数符号）
void Arm64CodeGenerator::emitTextHeader(Arm64AsmWriter& writer,
                                        const ir::IRModule& module) {
    writer.raw(".text");
    writer.comment("运行时外部符号（由 cn_runtime 库提供，链接器解析）");
    std::unordered_set<std::string> definedSymbols;
    for (auto& function : module.functions) {
        const std::string sym = symbolName(
            function.mangledName.empty() ? function.name : function.mangledName);
        definedSymbols.insert(sym);
    }
    std::unordered_set<std::string> externSet;
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if ((inst.opcode == ir::Opcode::Call ||
                     inst.opcode == ir::Opcode::FuncAddr) &&
                    !inst.extra.empty()) {
                    std::string sym = symbolName(inst.extra);
                    if (definedSymbols.find(sym) == definedSymbols.end()) {
                        externSet.insert(sym);
                    }
                }
            }
        }
    }
    for (auto& sym : externSet) {
        writer.raw(".globl " + sym);
    }
}

} // namespace cn_compiler
