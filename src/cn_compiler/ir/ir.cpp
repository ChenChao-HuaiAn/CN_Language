// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

namespace ir {

// 操作码转字符串（调试/测试输出）
const char* opcodeToString(Opcode opcode) {
    switch (opcode) {
        case Opcode::ConstInt: return "常量整数";
        case Opcode::ConstFloat: return "常量浮点";
        case Opcode::ConstString: return "常量字符串";
        case Opcode::ConstBool: return "常量布尔";
        case Opcode::Add: return "加";
        case Opcode::Sub: return "减";
        case Opcode::Mul: return "乘";
        case Opcode::Div: return "除";
        case Opcode::Mod: return "取余";
        case Opcode::BitAnd: return "位与";
        case Opcode::BitOr: return "位或";
        case Opcode::BitXor: return "位异或";
        case Opcode::Shl: return "左移";
        case Opcode::Shr: return "右移";
        case Opcode::Cast: return "转换";
        case Opcode::Eq: return "相等";
        case Opcode::Ne: return "不等";
        case Opcode::Lt: return "小于";
        case Opcode::Le: return "小于等于";
        case Opcode::Gt: return "大于";
        case Opcode::Ge: return "大于等于";
        case Opcode::And: return "逻辑与";
        case Opcode::Or: return "逻辑或";
        case Opcode::Not: return "逻辑非";
        case Opcode::Load: return "加载";
        case Opcode::Store: return "存储";
        case Opcode::Alloca: return "分配";
        case Opcode::AddrOf: return "取地址";
        case Opcode::LoadPtr: return "指针加载";
        case Opcode::StorePtr: return "指针存储";
        case Opcode::FieldAddr: return "字段地址";
        case Opcode::CopyStruct: return "结构体拷贝";
        case Opcode::Jump: return "跳转";
        case Opcode::Branch: return "条件跳转";
        case Opcode::Call: return "调用";
        case Opcode::CallIndirect: return "间接调用";
        case Opcode::Return: return "返回";
        // 阶段3 OOP（Task 3.1/3.2）
        case Opcode::NewObject: return "新建对象";
        case Opcode::DeleteObject: return "删除对象";
        case Opcode::VirtualCall: return "虚调用";
        case Opcode::VtableAddr: return "虚表地址";
        case Opcode::FuncAddr: return "函数地址";
        case Opcode::Phi: return "汇合";
    }
    return "未知";
}

} // namespace ir

// ==================== 字符串字面量解码（Task 2.5 完整实现） ====================

// 将 Unicode 码点编码为 UTF-8 字节串（普通字符串以 UTF-8 字节存储，codegen 逐字节十六进制输出）
// （B-1 拆分时从 ir.cpp 中部移回主文件的自由函数，decodeEscapes 依赖）
static void appendUtf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// 解码转义序列（\n \t \r \0 \\ \" \' \u{XXXX}），返回解码后的字符串
// （B-1 拆分时从 ir.cpp 中部移回主文件的自由函数，decodeString 依赖）
static std::string decodeEscapes(const std::string& body) {
    std::string out;
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char c = body[i];
        if (c != '\\') {
            out += c;
            continue;
        }
        if (i + 1 >= body.size()) {
            out += c;  // 末尾孤立反斜杠：原样保留（词法层已保证闭合，防御）
            break;
        }
        const char n = body[i + 1];
        switch (n) {
            case 'n': out += '\n'; i += 1; break;
            case 't': out += '\t'; i += 1; break;
            case 'r': out += '\r'; i += 1; break;
            case '0': out += '\0'; i += 1; break;
            case '\\': out += '\\'; i += 1; break;
            case '"': out += '"'; i += 1; break;
            case '\'': out += '\''; i += 1; break;
            case 'u': {
                // \u{XXXX}：Unicode 码点转义（规格书4.3）
                if (i + 2 < body.size() && body[i + 2] == '{') {
                    std::size_t j = i + 3;
                    const std::size_t hexStart = j;
                    while (j < body.size() && body[j] != '}') ++j;
                    if (j < body.size() && j > hexStart) {
                        const std::string hex = body.substr(hexStart, j - hexStart);
                        char32_t cp = 0;
                        bool ok = true;
                        for (char h : hex) {
                            cp *= 16;
                            if (h >= '0' && h <= '9') cp += h - '0';
                            else if (h >= 'a' && h <= 'f') cp += h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') cp += h - 'A' + 10;
                            else { ok = false; break; }
                        }
                        if (ok) appendUtf8(out, cp);
                        i = j;  // 跳到 '}'（循环 ++i 后到达 '}' 之后）
                    } else {
                        out += '\\';  // 非法 \u{：保留反斜杠
                    }
                } else {
                    out += '\\';
                }
                break;
            }
            default: out += c; break;  // 未知转义（如 \d）：保留反斜杠原样（C 语义）
        }
    }
    return out;
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
    // Task 2.7：枚举类型按整32处理（与语义层 typeSizeOf 一致）
    if (semantic_ != nullptr && semantic_->isEnumType(type)) return "i32";
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
ir::IRValue IRGenerator::newReg() {
    return ir::IRValue::reg(regCounter_++, "");
}
void IRGenerator::emit(ir::Opcode opcode,
                       const std::vector<ir::IRValue>& operands,
                       const ir::IRValue& result,
                       const std::string& extra,
                       const std::string& type,
                       const SourceLocation& loc) {
    ir::IRInstruction inst;
    inst.opcode = opcode;
    inst.operands = operands;
    inst.result = result;
    inst.extra = extra;
    inst.type = type;
    inst.loc = loc;
    currentBlock_->instructions.push_back(std::move(inst));
}
ir::IRValue IRGenerator::emitResult(ir::Opcode opcode,
                                    const std::vector<ir::IRValue>& operands,
                                    const std::string& type,
                                    const std::string& extra,
                                    const SourceLocation& loc) {
    ir::IRValue result = newReg();
    result.type = type;
    // Task 完善A：i128/正128 结果占用 2 个连续虚拟寄存器
    //   （result.id = 低64位槽、result.id+1 = 高64位槽，寄存器槽区小端连续16字节）。
    //   后续寄存器编号顺延，保证 i128 值占满连续槽（codegen 按双槽读写，
    //   并可 lea 槽区地址传给指针式辅助函数 __cn_*_i128）。
    if (type == "i128" || type == "u128") {
        ++regCounter_;
    }
    emit(opcode, operands, result, extra, type, loc);
    return result;
}
ir::IRValue IRGenerator::zeroConst(const std::string& type) {
    return emitResult(ir::Opcode::ConstInt, {}, type, "0", SourceLocation());
}
void IRGenerator::endJump(const std::string& target) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "跳转";
    currentBlock_->termTarget = target;
}
void IRGenerator::endBranch(const std::string& condReg, const std::string& trueTarget,
                            const std::string& falseTarget) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "条件跳转";
    currentBlock_->termTrueTarget = trueTarget;
    currentBlock_->termFalseTarget = falseTarget;
    if (condReg.size() > 2 && condReg[0] == '%' && condReg[1] == 'v') {
        int regId = std::stoi(condReg.substr(2));
        if (!currentBlock_->instructions.empty()) {
            currentBlock_->instructions.back().operands.push_back(
                ir::IRValue::reg(regId, "i1"));
        }
    }
}
void IRGenerator::endReturn(const std::string& valueReg) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "返回";
    currentBlock_->termReturnValue = valueReg;
}
ir::IRBlock* IRGenerator::newBlock(const std::string& label) {
    auto block = std::make_unique<ir::IRBlock>();
    block->label = label;
    blockCounter_++;
    function_->blocks.push_back(std::move(block));
    currentBlock_ = function_->blocks.back().get();
    return currentBlock_;
}
void IRGenerator::setCurrentBlock(ir::IRBlock* block) {
    currentBlock_ = block;
}
ir::IRValue IRGenerator::allocVar(const std::string& name, const std::string& irType,
                                  const std::string& srcType,
                                  const SourceLocation& loc) {
    ir::IRValue reg = newReg();
    reg.type = irType;
    // 生成唯一内部名：name$N（N为函数级递增计数）
    std::string unique = name + "$" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, reg, unique, irType, loc);
    // 数组变量登记多槽（codegen 预留连续槽区；Alloca extra 仍为唯一名）
    registerVarSlots(unique, srcType);
    if (varStack_.empty()) varStack_.emplace_back();  // 确保存在当前作用域
    VarEntry entry;
    entry.regId = reg.id;
    entry.uniqueName = unique;
    entry.type = irType;
    entry.srcType = srcType;
    varStack_.back()[name] = entry;
    return reg;
}
void IRGenerator::registerVarSlots(const std::string& unique, const std::string& srcType) {
    if (function_ == nullptr) return;
    // Task 完善A：i128/正128 变量/参数按 2 个连续 8 字节槽登记
    // （低64位槽 + 高64位槽 name$s1，与结构体 2 槽机制一致）
    if (types::isI128(types::canonical(srcType))) {
        function_->varSlots[unique] = 2;
        return;
    }
    if (types::isArray(srcType)) {
        const int len = types::arrayLenOf(srcType);
        if (len <= 0) {
            function_->varSlots[unique] = 1;
            return;
        }
        const std::string elemSrc = types::arrayElemOf(srcType);
        int elemSlots = 1;
        if (semantic_ != nullptr && semantic_->isStructType(types::canonical(elemSrc))) {
            // 结构体数组：每元素占 ceil(结构体大小/8) 个 8 字节槽
            const int size = semantic_->typeSizeOf(elemSrc);
            elemSlots = (size + 7) / 8;
        } else if (types::isI128(types::canonical(elemSrc))) {
            // i128 数组：每元素 2 槽
            elemSlots = 2;
        }
        function_->varSlots[unique] = len * elemSlots;
    } else if (semantic_ != nullptr && semantic_->isStructType(types::canonical(srcType))) {
        // 结构体/联合体变量：按总大小分配 8 字节槽（ceil(总大小/8)）
        const int size = semantic_->typeSizeOf(srcType);
        function_->varSlots[unique] = (size + 7) / 8;
    } else {
        function_->varSlots[unique] = 1;
    }
}
std::string IRGenerator::lookupVarName(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.uniqueName;
    }
    return "";
}
std::string IRGenerator::lookupVarType(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.type;
    }
    return "";
}
ir::IRValue IRGenerator::lookupVar(const std::string& name) {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return ir::IRValue::reg(found->second.regId, found->second.type);
        }
    }
    return ir::IRValue::reg(-1, "");
}
std::string IRGenerator::lookupSrcType(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.srcType;
    }
    return "";
}
bool IRGenerator::isByRefCapture(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.byRef;
    }
    return false;
}
std::int64_t IRGenerator::ptrElemStride(const std::string& srcType) const {
    if (semantic_ != nullptr && types::isPointer(srcType)) {
        const std::string elem = types::pointeeOf(srcType);
        if (semantic_->isStructType(types::canonical(elem))) {
            return semantic_->typeSizeOf(elem);
        }
        // H8 补完（2026-08-25）：类类型指针元素（向量<T> 数据 = T*，T=映射
        //   56 字节）步长须按类总大小——原兜底 8 导致 追加/元素/删除 错位越界
        //   0xC0000374（分配已按 sizeof 但索引按 8）。
        if (semantic_->isClassType(types::canonical(elem))) {
            return semantic_->typeSizeOf(elem);
        }
        // 修复集成审查 BUG #5：i128/正128 指针元素 stride = 16 字节
        if (types::isI128(types::canonical(elem))) {
            return 16;
        }
    }
    return 8;
}
std::string IRGenerator::pointerPointeeSrcType(Expr* node) const {
    if (node == nullptr) return "";
    if (node->getType() == NodeType::IdentifierExpr) {
        const std::string srcType = lookupSrcType(
            static_cast<IdentifierExpr*>(node)->name);
        if (!srcType.empty() && types::isPointer(srcType)) {
            return types::pointeeOf(srcType);
        }
        return "";
    }
    if (node->getType() == NodeType::BinaryExpr) {
        BinaryExpr* bin = static_cast<BinaryExpr*>(node);
        if (bin->op == Operator::Add || bin->op == Operator::Subtract) {
            // 指针算术：指针可能在左或右（p+1 / 1+p 罕见但支持）
            std::string left = pointerPointeeSrcType(bin->left.get());
            if (!left.empty()) return left;
            std::string right = pointerPointeeSrcType(bin->right.get());
            if (!right.empty()) return right;
        }
        return "";
    }
    if (node->getType() == NodeType::UnaryExpr) {
        UnaryExpr* un = static_cast<UnaryExpr*>(node);
        // &x / *x / 指针自增 p++：递归 operand 推导
        if (un->op == Operator::AddressOf || un->op == Operator::Deref ||
            un->op == Operator::Increment || un->op == Operator::Decrement) {
            return pointerPointeeSrcType(un->operand.get());
        }
        return "";
    }
    return "";
}
std::string IRGenerator::memberObjStructType(MemberExpr* node) const {
    if (semantic_ == nullptr) return "";
    std::string objType = "";
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objType = lookupSrcType(
            static_cast<IdentifierExpr*>(node->object.get())->name);
    } else if (node->object->getType() == NodeType::MemberExpr) {
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        const std::string innerType = memberObjStructType(inner);
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName) {
                    objType = f.type;
                    break;
                }
            }
        }
        // 宿主缺陷根治（2026-08-25）：结果/可选 .值/.错误 不是合成结构体直接字段
        //   （在联合体内）——嵌套成员（查.值.类型ID）须按结果/可选成员映射推导，
        //   否则 lvalueAddress 找不到对象类型而不加外层字段偏移（类型ID 在偏移8
        //   读成 0 偏移 名ID 值 实测）。
        if (objType.empty()) {
            const std::string canonInner = types::canonical(innerType);
            if (SemanticAnalyzer::isResultType(canonInner)) {
                const std::vector<std::string> rargs =
                    SemanticAnalyzer::resultTypeArgs(canonInner);
                if (inner->memberName == "值" && rargs.size() == 2) objType = rargs[0];
                else if (inner->memberName == "错误" && rargs.size() == 2) objType = rargs[1];
                else if (inner->memberName == "正常") objType = "布尔";
            } else if (SemanticAnalyzer::isOptionalType(canonInner)) {
                if (inner->memberName == "值") objType = SemanticAnalyzer::optionalTypeArg(canonInner);
                else if (inner->memberName == "有值") objType = "布尔";
            }
        }
    } else if (node->object->getType() == NodeType::IndexExpr) {
        IndexExpr* idx = static_cast<IndexExpr*>(node->object.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            if (types::isArray(st)) objType = types::arrayElemOf(st);
            else if (types::isPointer(st)) objType = types::pointeeOf(st);
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 方形.顶点[0] / 方形指针->顶点[0]：内层成员字段（数组）-> 元素类型
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            const std::string innerType = memberObjStructType(inner);
            const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName) {
                        objType = types::isArray(f.type) ? types::arrayElemOf(f.type)
                                                         : f.type;
                        break;
                    }
                }
            }
        }
    }
    // arrow 成员：对象是结构体指针 -> 剥指针取所指类型
    if (node->isArrow && types::isPointer(objType)) {
        objType = types::pointeeOf(objType);
    }
    return objType;
}
void IRGenerator::emitBoundsCheck(const ir::IRValue& indexRaw, int arrayLen,
                                  const SourceLocation& loc) {
    if (arrayLen <= 0 || currentBlock_ == nullptr) return;
    // 下标先 Cast 到 i64（比较按64位无符号/有符号语义）
    ir::IRValue idx = indexRaw;
    if (idx.type != "i64") {
        idx = emitResult(ir::Opcode::Cast, {idx}, "i64", "", loc);
    }
    // index < 0：按有符号比较（负下标越界）
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    ir::IRValue ltZero = emitResult(ir::Opcode::Lt, {idx, zero}, "i1", "", loc);
    // index >= len：按有符号比较（上界越界；数组长度为正，有符号安全）
    ir::IRValue lenConst = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                      std::to_string(arrayLen), loc);
    ir::IRValue geLen = emitResult(ir::Opcode::Ge, {idx, lenConst}, "i1", "", loc);
    // 合并：bad = (index<0) || (index>=len)
    ir::IRValue bad = emitResult(ir::Opcode::Or, {ltZero, geLen}, "i1", "", loc);
    // 标签：错误块 + 继续块。先预取标签字符串（newBlock 内部会 blockCounter_++，
    // 不能手动 ++ 后调 newBlock，否则双 ++ 导致标签错位）。
    const std::string errLabel = "bb" + std::to_string(blockCounter_);
    const std::string okLabel = "bb" + std::to_string(blockCounter_ + 1);
    // 终结当前块（含 bad 比较的块）：bad -> 错误块, ok -> 继续块。
    // 必须在创建新块之前调用（endBranch 终结 currentBlock_）
    endBranch(bad.toString(), errLabel, okLabel);
    // 创建错误块（newBlock 内部 blockCounter_++ 推进计数）
    setCurrentBlock(newBlock(errLabel));
    ir::IRValue errCode = emitResult(ir::Opcode::ConstInt, {}, "i64", "2", loc);
    emitResult(ir::Opcode::Call, {errCode}, "i32", "__cn_runtime_error", loc);
    endReturn("");
    // 创建继续块（newBlock 内部 blockCounter_++ 推进计数）
    setCurrentBlock(newBlock(okLabel));
}
ir::IRValue IRGenerator::lvalueAddress(Expr* node) {
    if (node->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node);
        const std::string unique = lookupVarName(ident->name);
        const std::string srcType = lookupSrcType(ident->name);
        // 缺陷修复（[&] 引用捕获左值）：参数槽存被捕获变量地址，取地址 = Load 参数槽
        //   （而非 AddrOf 参数槽——否则 &捕获变量 取到的是参数槽自身的地址）
        if (isByRefCapture(ident->name)) {
            return emitResult(ir::Opcode::Load,
                              {ir::IRValue::var(unique, "ptr")},
                              "ptr", unique, node->location);
        }
        const std::string irType = mapType(types::isArray(srcType)
                                               ? types::arrayElemOf(srcType) : srcType);
        return emitResult(ir::Opcode::AddrOf,
                          {ir::IRValue::var(unique, irType)},
                          "ptr", unique, node->location);
    }
    if (node->getType() == NodeType::IndexExpr) {
        // 下标左值：计算数组元素地址（数组槽向栈下方扩展：元素 i 在 基址 + i*8）
        IndexExpr* idx = static_cast<IndexExpr*>(node);
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(idx->object.get());
            const std::string unique = lookupVarName(ident->name);
            const std::string srcTypeRaw = lookupSrcType(ident->name);
            // A-1（引用参数）：引用数组参数（整32[5]&）先剥 & 再判数组形态
            const std::string srcType = types::stripRef(srcTypeRaw);
            if (!unique.empty() && types::isArray(srcType)) {
                // 缺陷修复（[&] 捕获数组）：参数槽存被捕获数组地址，基址 = Load 参数槽
                //   （而非 AddrOf 参数槽——否则下标落到参数槽自身区域，读垃圾/越界）
                ir::IRValue base;
                if (isByRefCapture(ident->name)) {
                    base = emitResult(ir::Opcode::Load,
                                      {ir::IRValue::var(unique, "ptr")},
                                      "ptr", unique, idx->location);
                } else {
                    base = emitResult(
                        ir::Opcode::AddrOf,
                        {ir::IRValue::var(unique, mapType(types::arrayElemOf(srcType)))},
                        "ptr", unique, idx->location);
                }
                ir::IRValue index = genExpr(idx->index.get());
                emitBoundsCheck(index, types::arrayLenOf(srcType), idx->location);
                if (index.type != "i64") {
                    index = emitResult(ir::Opcode::Cast, {index}, "i64", "",
                                       idx->location);
                }
                // 元素间距：结构体数组按结构体总大小（Task 2.7），普通类型 8 字节
                std::int64_t elemStride = 8;
                if (semantic_ != nullptr) {
                    const std::string elemSrc = types::arrayElemOf(srcType);
                    if (semantic_->isStructType(elemSrc)) {
                        elemStride = semantic_->typeSizeOf(elemSrc);
                    }
                    // 修复集成审查 BUG #5：i128/正128 数组元素 stride = 16
                    if (types::isI128(types::canonical(elemSrc))) {
                        elemStride = 16;
                    }
                }
                ir::IRValue scaled = emitResult(
                    ir::Opcode::Mul, {index,
                                      ir::IRValue::constant(std::to_string(elemStride), "i64")},
                    "i64", "", idx->location);
                // 数组元素 i 的地址 = base + i*元素大小（C语义，与指针算术方向一致）：
                // codegen 逆序登记使基址槽最深（元素0），元素 i 槽在基址上方
                // （元素1 = base+步长、元素2 = base+2*步长 ... 位于变量槽区内）
                return emitResult(ir::Opcode::Add, {base, scaled}, "ptr", "",
                                  idx->location);
            }
        }
        // 其他对象（指针 p[i] / 数组字段 方形.顶点[i]）：地址 = 基址 + index*元素大小
        // （结构体指针/数组字段按元素大小，普通指针8字节；Task 2.7/修复10）
        ir::IRValue obj = genExpr(idx->object.get());
        ir::IRValue index = genExpr(idx->index.get());
        if (index.type != "i64") {
            index = emitResult(ir::Opcode::Cast, {index}, "i64", "", idx->location);
        }
        // 元素步进：数组字段（方形.顶点 / 方形指针->顶点，坐标[4]）按字段数组元素
        //   类型大小；普通指针按 ptrElemStride（结构体指针按总大小）
        std::int64_t stride = 8;
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            // A-3（2026-08）：隐式类字段对象（方法体内 数据[位置] = 值 赋值目标，
            //   lookupSrcType 为空）——按字段源码类型推导步进（向量 数据 T* 的
            //   结构体元素 24 字节，此前固定 8 导致元素错位/越界）
            if (st.empty() && isInstanceField(
                    static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                st = classFieldType(currentClass_,
                                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(st)) {
                    // 数组字段：按 C 布局元素大小（与成员数组字段同规则）
                    const std::string elemSrc = types::arrayElemOf(st);
                    stride = (semantic_ != nullptr &&
                              semantic_->isStructType(types::canonical(elemSrc)))
                                 ? semantic_->typeSizeOf(elemSrc)
                                 : types::typeSize(elemSrc);
                    emitBoundsCheck(index, types::arrayLenOf(st), idx->location);
                } else {
                    stride = ptrElemStride(st);
                }
            } else {
                stride = ptrElemStride(st);
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 修复10/10b/10c：方形.顶点[0] / 方形指针->顶点[0] — object 为数组字段成员，
            //   元素步进 = 字段数组元素类型大小（memberObjStructType 递归处理 arrow）；
            //   越界检查 = 字段数组长度（错误码2，修复10c）
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            const std::string innerType = memberObjStructType(inner);
            const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName && types::isArray(f.type)) {
                        const std::string elemSrc = types::arrayElemOf(f.type);
                        stride = semantic_->isStructType(elemSrc)
                                     ? semantic_->typeSizeOf(elemSrc)
                                     : types::typeSize(elemSrc);
                        emitBoundsCheck(index, types::arrayLenOf(f.type), idx->location);
                        break;
                    }
                }
            }
        }
        ir::IRValue scaled = emitResult(
            ir::Opcode::Mul, {index, ir::IRValue::constant(std::to_string(stride), "i64")},
            "i64", "", idx->location);
        return emitResult(ir::Opcode::Add, {obj, scaled}, "ptr", "", idx->location);
    }
    // 解引用 *p：地址即指针值（指针本身的值），而非 LoadPtr 读取的元素值
    if (node->getType() == NodeType::UnaryExpr &&
        static_cast<UnaryExpr*>(node)->op == Operator::Deref) {
        return genExpr(static_cast<UnaryExpr*>(node)->operand.get());
    }
    // 结构体字段左值：p.x / 指针->x（Task 2.7）
    // 字段地址 = 基址 + 偏移（FieldAddr；-> 隐含空指针检查错误码3）
    if (node->getType() == NodeType::MemberExpr) {
        MemberExpr* member = static_cast<MemberExpr*>(node);
        // ---- 阶段3 OOP（Task 3.1）：类字段左值地址（对象.字段 / 类名.静态字段） ----
        // 实例字段 -> 对象指针 + FieldAddr；静态字段 -> ?static_类名_字段名 符号地址。
        // 供 对象.字段 = v 赋值、&对象.字段 取地址 使用。
        ir::IRValue oopAddr;
        if (handleClassMemberLvalue(member, oopAddr)) {
            return oopAddr;
        }
        // 枚举引用不是左值，防御性按值处理
        if (!member->isArrow && member->object->getType() == NodeType::IdentifierExpr) {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(member->object.get());
            std::int64_t v = 0;
            if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
                semantic_->enumValueOf(ident->name, member->memberName, v)) {
                return genExpr(node);
            }
        }
        // 基址：-> 为指针值；. 为对象地址（递归处理嵌套成员/数组元素）
        ir::IRValue base = member->isArrow
                               ? genExpr(member->object.get())
                               : lvalueAddress(member->object.get());
        if (semantic_ == nullptr) return base;
        // 对象源码类型：变量 / 嵌套成员 / 数组字段元素（memberObjStructType 递归
        //   处理 arrow 指针剥除，修复10/10b）
        std::string objSrcType = memberObjStructType(member);
        if (objSrcType.empty() && member->isArrow &&
            member->object->getType() == NodeType::BinaryExpr) {
            // 指针算术结果成员：(名单 + (n-1))->分数 — 从左操作数推导元素类型
            BinaryExpr* bin = static_cast<BinaryExpr*>(member->object.get());
            if (bin->left->getType() == NodeType::IdentifierExpr) {
                const std::string ptrType = lookupSrcType(
                    static_cast<IdentifierExpr*>(bin->left.get())->name);
                if (types::isPointer(ptrType)) {
                    objSrcType = types::pointeeOf(ptrType);
                }
            }
        }
        const StructDecl* decl = semantic_->findStruct(types::canonical(objSrcType));
        if (decl == nullptr) return base;
        const int offset = semantic_->fieldOffsetOf(decl, member->memberName);
        if (offset < 0) return base;
        return emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                          std::to_string(offset), member->location);
    }
    // 其他表达式（防御性）：按表达式值处理
    return genExpr(node);
}
ir::IRValue IRGenerator::genExpr(Expr* node) {
    if (node == nullptr) {
        return ir::IRValue::reg(-1, "");
    }
    node->accept(*this);
    return lastExpr_;
}
} // namespace cn_compiler
