// CN-IR生成器实现：字符串解码 + 类型映射族（350 重构E2 自 ir.cpp 纯机械搬移）
// 职责（函数体逐字搬移·零语义变化；成员声明保持在 ir.hpp 原位）：
//   appendUtf8/decodeEscapes/charLiteralCodePoint —— 字符串/字符字面量解码
//   mapType/mapBinaryOp/isCompoundAssignOp/baseOpOfCompound/widenCompoundRhs
//                                                    —— 类型与运算符映射
//   decodeString —— 字符串字面量前缀识别+引号剥离+转义解码
#include <cstdint>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 字符串字面量解码（Task 2.5 完整实现） ====================

// 919（超承载字面量参与表达式·常量引用面收口）：整数字面量文本 -> IR
//   ConstInt 发射单点——910 判域收口原内联于 visitIntegerLiteral，常量引用
//   面（常量值文本直存原恒 "i64"：20 位文本被当 i64 常量 mov rax,<文本> →
//   ml64 A2084 汇编失败·探针 p36 实锚）复用同构逻辑=第 3 处提炼纪律兑现。
//   行为：types::integerLiteralRuntimeType 判域（910 提升链单点）+ 超 128 域
//   诊断 + 规范十进制文本（i128/正128 走 splitI128Text 低:高 双半——codegen
//   拆双槽加载；其余规范十进制直存）。
ir::IRValue IRGenerator::emitIntLiteralConst(const std::string& raw,
                                             const SourceLocation& loc) {
    int st910 = 0;
    const std::string type = types::integerLiteralRuntimeType(raw, &st910);
    if (st910 != 0) {
        diagnostics_.report(
            DiagnosticLevel::Error, loc,
            "整数字面量超出" +
                std::string(st910 == 2 ? "正128（2^128-1）" : "整128（2^127-1）") +
                "范围");
        return emitResult(ir::Opcode::ConstInt, {}, mapType(type), "0:0", loc);
    }
    const std::string dec = types::normalizeIntLiteralToDecimal(raw);
    if (types::isI128(type)) {
        const std::string split = types::splitI128Text(dec);
        return emitResult(ir::Opcode::ConstInt, {}, mapType(type),
                          split.empty() ? dec : split, loc);
    }
    // 静默豁免业务值: 910 收口非 128 常量发射分支（规范十进制文本直存 ConstInt·910 登记条目迁移至本单点）
    return emitResult(ir::Opcode::ConstInt, {}, mapType(type), dec, loc);
}

// ==================== 类型映射 ====================

// 源码类型 -> IR类型映射（规格书7.5类型表示）
std::string IRGenerator::mapType(const std::string& type) {
    // Task 6.1（泛型函数实例化）：类型参数 T/U 先替换为实参类型
    //   （最小<整32> 函数体内 返回 T / 局部 T 变量须映射为整32 而非 ptr 兜底）
    if (!genericTypeParams_.empty()) {
        const std::string subst = substGenericType(type);
        if (subst != type) return mapType(subst);
    }
    if (type == "整8") return "i8";
    if (type == "整16") return "i16";
    if (type == "整32" || type == "整数") return "i32";
    if (type == "整64") return "i64";
    if (type == "整128") return "i128";
    if (type == "正8") return "u8";
    if (type == "正16") return "u16";
    if (type == "正32") return "u32";
    if (type == "正64") return "u64";
    if (type == "正128") return "u128";
    if (type == "浮32") return "f32";
    if (type == "浮64" || type == "小数") return "f64";
    if (type == "布尔") return "i1";
    if (type == "字符") return "i32";
    if (type == "字符串") return "ptr";
    if (type == "空类型") return "void";
    if (!type.empty() && type.back() == '*') return "ptr";
    // Task 2.4：数组类型（整32[10]）在IR层按元素类型处理（栈上连续分配，
    // 数组名引用为元素指针；Alloca 元素大小按元素类型登记）
    if (types::isArray(type)) return mapType(types::arrayElemOf(type));
    // Task 2.2：函数指针类型（函数指针<返回>(参数,...)）按指针处理
    if (!type.empty() && type.rfind("函数指针<", 0) == 0) return "ptr";
    // 031（#1075）：绑定方法值类型（方法值<类,签名>）按指针处理——两槽值
    //   （this/code）在 IR 层降为两个 8 字节标量；表达式通道值=槽对基名
    //   （ptr 型），槽数经 registerVarSlots 方法值分支登记 2（ir.cpp）。
    if (types::isMethodValueType(type)) return "ptr";
    // Task 2.7：枚举类型按整32处理（与语义层 typeSizeOf 一致）
    if (semantic_ != nullptr && semantic_->isEnumType(type)) return "i32";
    // 648 归档（豁免边界声明）：mapType 为宽接口纯映射函数，被全树消费
    //   （自定义类型/类对象=指针语义是合法主路径），未知串 -> ptr 不可在此改
    //   报错（会把海量合法「类=ptr」误伤）。真正未识别串的 miss 由各消费点
    //   按「miss 不可静默」立法处置（先例=ir_call.cpp 返回类型/形参表收口）。
    if (!type.empty() && type != "未知") return "ptr";  // 自定义类型按指针处理
    return "void";
}
bool IRGenerator::mapBinaryOp(Operator op, bool isFloat, ir::Opcode& out) {
    (void)isFloat;  // 浮点/整型共用Opcode，由 codegen 按类型分派（Task 2.3）
    switch (op) {
        // ---- 算术（整型与浮点共用Opcode，codegen按类型分派SSE/整型指令） ----
        case Operator::Add: out = ir::Opcode::Add; return true;
        case Operator::Subtract: out = ir::Opcode::Sub; return true;
        case Operator::Multiply: out = ir::Opcode::Mul; return true;
        case Operator::Divide: out = ir::Opcode::Div; return true;
        case Operator::Modulo: out = ir::Opcode::Mod; return true;
        // ---- 比较（结果为布尔） ----
        case Operator::EqualEqual: out = ir::Opcode::Eq; return true;
        case Operator::BangEqual: out = ir::Opcode::Ne; return true;
        case Operator::Less: out = ir::Opcode::Lt; return true;
        case Operator::LessEqual: out = ir::Opcode::Le; return true;
        case Operator::Greater: out = ir::Opcode::Gt; return true;
        case Operator::GreaterEqual: out = ir::Opcode::Ge; return true;
        // ---- 逻辑（布尔操作数） ----
        case Operator::AndAnd: out = ir::Opcode::And; return true;
        case Operator::OrOr: out = ir::Opcode::Or; return true;
        // ---- 位运算与移位（Task 2.3 新增，整型专用；浮点不允许位运算） ----
        case Operator::Amp: out = ir::Opcode::BitAnd; return true;
        case Operator::Pipe: out = ir::Opcode::BitOr; return true;
        case Operator::Caret: out = ir::Opcode::BitXor; return true;
        case Operator::LessLess: out = ir::Opcode::Shl; return true;
        case Operator::GreaterGreater: out = ir::Opcode::Shr; return true;
        default: return false;
    }
}
bool IRGenerator::isCompoundAssignOp(Operator op) {
    switch (op) {
        case Operator::PlusAssign: case Operator::MinusAssign:
        case Operator::StarAssign: case Operator::SlashAssign:
        case Operator::PercentAssign:
        case Operator::AmpAssign: case Operator::PipeAssign:
        case Operator::CaretAssign: case Operator::LessLessAssign:
        case Operator::GreaterGreaterAssign:
            return true;
        default:
            return false;
    }
}
Operator IRGenerator::baseOpOfCompound(Operator op) {
    switch (op) {
        case Operator::PlusAssign: return Operator::Add;
        case Operator::MinusAssign: return Operator::Subtract;
        case Operator::StarAssign: return Operator::Multiply;
        case Operator::SlashAssign: return Operator::Divide;
        case Operator::PercentAssign: return Operator::Modulo;
        case Operator::AmpAssign: return Operator::Amp;
        case Operator::PipeAssign: return Operator::Pipe;
        case Operator::CaretAssign: return Operator::Caret;
        case Operator::LessLessAssign: return Operator::LessLess;
        case Operator::GreaterGreaterAssign: return Operator::GreaterGreater;
        default: return Operator::Assign;
    }
}

// 复合赋值右值宽化（331-a·T51 单一归属）——见 ir.hpp 声明注释。
ir::IRValue IRGenerator::widenCompoundRhs(const ir::IRValue& rhs,
                                          const std::string& targetType,
                                          const SourceLocation& loc) {
    if (targetType != "i128" && targetType != "u128") return rhs;
    if (rhs.type == targetType) return rhs;
    return emitResult(ir::Opcode::Cast, {rhs}, targetType, "", loc);
}
std::string IRGenerator::decodeString(const std::string& raw) {
    // 识别前缀（组合前缀 原始多行 / 多行原始 优先）。
    // 注意：中文前缀为 UTF-8 多字节，偏移必须用字节数（substr 按字节切割）：
    //       "原始"=6字节、"多行"=6字节、"原始多行"=12字节
    bool rawMode = false;
    bool multiLine = false;
    std::size_t start = 0;
    if (raw.rfind("原始多行", 0) == 0 || raw.rfind("多行原始", 0) == 0) {
        rawMode = true;
        multiLine = true;
        start = 12;
    } else if (raw.rfind("原始", 0) == 0) {
        rawMode = true;
        start = 6;
    } else if (raw.rfind("多行", 0) == 0) {
        multiLine = true;
        start = 6;
    }
    // 剥离引号：多行用三引号 """..."""；普通/原始用单引号 "..."（可带前缀）
    std::string body;
    if (multiLine) {
        if (raw.size() >= start + 6 && raw.compare(start, 3, "\"\"\"") == 0 &&
            raw.compare(raw.size() - 3, 3, "\"\"\"") == 0) {
            body = raw.substr(start + 3, raw.size() - start - 6);
        } else {
            body = raw.substr(start);  // 防御：非法形态保留原文
        }
    } else {
        if (raw.size() >= start + 2 && raw[start] == '"' && raw.back() == '"') {
            body = raw.substr(start + 1, raw.size() - start - 2);
        } else {
            body = raw.substr(start);
        }
    }
    // 原始形态：不处理转义（所见即所得）
    if (rawMode) return body;
    // 普通/多行：处理转义序列
    return decodeEscapes(body);
}

// 1073：顶层常量引用整数发射单点——常量文本（规范十进制·可带负号）值域分流：
//   ① 超 128 域 → 诊断拒 + 0:0 哨兵（错误恢复；语义层 535/638 判域已前置拦截，
//      本面同 910 口径兜底）；② 超 64 位承载 → 128 双半文本（splitI128Text·
//      与 visitIntegerLiteral i128 分支同契约——低64位:高64位 十六进制·第 2 处
//      重复留此标记：第 3 处出现前提炼）；③ 承载内 → i64 直存（原行为保平）。
void IRGenerator::emitConstRefIntLiteral(const std::string& decText,
                                         const SourceLocation& loc) {
    int st = 0;
    const std::string rt = types::integerLiteralRuntimeType(decText, &st);
    if (st != 0) {
        diagnostics_.report(
            DiagnosticLevel::Error, loc,
            "整数字面量超出" +
                std::string(st == 2 ? "正128（2^128-1）" : "整128（2^127-1）") +
                "范围");
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, mapType(rt), "0:0", loc);
        return;
    }
    if (types::isI128(rt)) {
        // i128/正128：常量文本存 低64位:高64位 十六进制（codegen 拆双槽加载）
        const std::string split = types::splitI128Text(decText);
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, mapType(rt),
                               split.empty() ? decText : split, loc);
        return;
    }
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i64", decText, loc);
}

// 938（字符串常量全链·纪律二第三处前提炼）：字符串内容入常量池（去重·返回池
//   编号）——字符串字面量（visitStringLiteral）/默认值字符串（evalDefaultExpr）/
//   顶层字符串常量内联（visitIdentifierExpr 常量分支）/空串（internEmptyString）
//   多消费点单点复用。原各自内联注册；第三处（常量内联）原直接以值文本作
//   ConstString extra → codegen `lea X, AB`（未注册池·ml64 undefined symbol·
//   宿主字符串常量全形态无法构建·938 探针实录）。
int IRGenerator::internString(const std::string& content) {
    auto it = module_->stringIndex.find(content);
    if (it != module_->stringIndex.end()) {
        return it->second;
    }
    const int index = static_cast<int>(module_->stringConstants.size());
    module_->stringConstants.push_back(content);
    module_->stringIndex[content] = index;
    return index;
}

} // namespace cn_compiler
