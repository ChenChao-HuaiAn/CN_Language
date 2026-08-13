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
        case Opcode::FuncAddr: return "函数地址";
        case Opcode::Phi: return "汇合";
    }
    return "未知";
}

} // namespace ir

// ==================== 类型映射 ====================

// 源码类型 -> IR类型映射（规格书7.5类型表示）
std::string IRGenerator::mapType(const std::string& type) {
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

// 运算符 -> IR操作码映射（整型/浮点共用算术操作码，阶段一浮点变体后续区分）
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

// 是否复合赋值运算符
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

// 复合赋值的基础运算符（+= -> +）
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

// ==================== 字符串字面量解码（Task 2.5 完整实现） ====================

// 将 Unicode 码点编码为 UTF-8 字节串（普通字符串以 UTF-8 字节存储，codegen 逐字节十六进制输出）
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

// 字符串字面量解码（Task 2.5）：识别 普通/原始/多行/原始多行 前缀，
// 剥离前缀与引号，普通/多行处理转义，原始/原始多行保留反斜杠原样
// raw 为 lexer 保留的完整原文（含前缀与引号，如 原始"..." / 多行"""..."""）
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

// ==================== IR构建辅助 ====================

// 新建虚拟寄存器
ir::IRValue IRGenerator::newReg() {
    return ir::IRValue::reg(regCounter_++, "");
}

// 添加指令到当前块
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

// 添加带结果寄存器的指令
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

// 生成零常量（一元负号/按位非辅助）
ir::IRValue IRGenerator::zeroConst(const std::string& type) {
    return emitResult(ir::Opcode::ConstInt, {}, type, "0", SourceLocation());
}

// 终结当前块：无条件跳转
void IRGenerator::endJump(const std::string& target) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "跳转";
    currentBlock_->termTarget = target;
}

// 终结当前块：条件跳转（条件寄存器记录在最后一条指令操作数中）
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

// 终结当前块：返回
void IRGenerator::endReturn(const std::string& valueReg) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "返回";
    currentBlock_->termReturnValue = valueReg;
}

// ==================== 基本块与变量管理 ====================

// 新建基本块并加入当前函数（同时设为当前块）
ir::IRBlock* IRGenerator::newBlock(const std::string& label) {
    auto block = std::make_unique<ir::IRBlock>();
    block->label = label;
    blockCounter_++;
    function_->blocks.push_back(std::move(block));
    currentBlock_ = function_->blocks.back().get();
    return currentBlock_;
}

// 设置当前生成块
void IRGenerator::setCurrentBlock(ir::IRBlock* block) {
    currentBlock_ = block;
}

// 分配变量寄存器：Alloca指令 + 压入当前作用域
// 唯一内部名 name$N：同名变量（遮蔽）在不同作用域分配不同内部名，
// 使代码生成层能为每个作用域实例分配独立栈槽（否则遮蔽变量共用槽导致值串扰）
// Task 2.4：srcType 保留源码复合类型（整32[5]/整32*），数组登记多槽
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

// 登记变量栈槽数（唯一内部名 -> 槽数量）：数组按元素间距、结构体按总大小/8、普通变量1
// 说明：槽区是 8 字节一槽的连续栈区。数组元素间距 = 元素大小（结构体数组按
//       结构体总大小，Task 2.7）；总槽数 = 数组长度 × 元素槽数，否则结构体数组
//       的元素槽会越界覆盖相邻变量（集成验证发现，Task 2.7 修复）
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

// 从内到外查找变量的唯一内部名（未找到返回空串）
std::string IRGenerator::lookupVarName(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.uniqueName;
    }
    return "";
}

// 从内到外查找变量的IR类型（未找到返回空串）
std::string IRGenerator::lookupVarType(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.type;
    }
    return "";
}

// 查找变量寄存器（未找到返回id=-1）
ir::IRValue IRGenerator::lookupVar(const std::string& name) {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return ir::IRValue::reg(found->second.regId, found->second.type);
        }
    }
    return ir::IRValue::reg(-1, "");
}

// 查找变量的源码类型（指针/数组复合类型；未找到返回空串，Task 2.4）
std::string IRGenerator::lookupSrcType(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.srcType;
    }
    return "";
}

// 指针算术步进（字节）：普通指针按8字节（栈槽宽）；结构体指针按结构体总大小。
// 集成验证发现：指针 + 整型 / p[i] / p++ 曾固定按8字节，导致结构体指针
// 遍历错位（学生 24 字节却每次只前进8字节）——Task 2.7 修复
std::int64_t IRGenerator::ptrElemStride(const std::string& srcType) const {
    if (semantic_ != nullptr && types::isPointer(srcType)) {
        const std::string elem = types::pointeeOf(srcType);
        if (semantic_->isStructType(types::canonical(elem))) {
            return semantic_->typeSizeOf(elem);
        }
        // 修复集成审查 BUG #5：i128/正128 指针元素 stride = 16 字节
        if (types::isI128(types::canonical(elem))) {
            return 16;
        }
    }
    return 8;
}

// 推导成员表达式对象的源码结构体类型（Task 2.7/修复10）
// 返回 对象指向的结构体源码类型（如 方形 -> 形状；方形.顶点[0] -> 坐标；
//   方形指针->顶点[0] -> 坐标）。
// 规则：
//   1. object 为变量：查变量源码类型（arrow 剥指针）
//   2. object 为嵌套成员：递归查字段类型（arrow 剥指针）
//   3. object 为下标（数组字段元素）：元素类型 = 字段数组元素类型
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

// 数组越界检查插桩（Task 2.4，错误码2）：
//   比较 index < 0 || index >= arrayLen，越界则跳转到错误块调用 __cn_runtime_error(2)
// 实现：当前块末尾 endBranch（bad -> 错误块, ok -> 继续块），
//       错误块调用运行时错误函数后 endReturn（终止），继续块承接后续指令
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

// 计算左值地址（Task 2.4）：标识符/数组下标/解引用 -> 地址值（ptr）
// 标识符：AddrOf(变量槽)；下标：基址 + index*8；解引用：指针值本身
ir::IRValue IRGenerator::lvalueAddress(Expr* node) {
    if (node->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node);
        const std::string unique = lookupVarName(ident->name);
        const std::string srcType = lookupSrcType(ident->name);
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
            const std::string srcType = lookupSrcType(ident->name);
            if (!unique.empty() && types::isArray(srcType)) {
                ir::IRValue base = emitResult(
                    ir::Opcode::AddrOf,
                    {ir::IRValue::var(unique, mapType(types::arrayElemOf(srcType)))},
                    "ptr", unique, idx->location);
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
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            stride = ptrElemStride(st);
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

// ==================== 声明节点 ====================

// 程序入口：遍历顶层函数声明
void IRGenerator::visitProgram(Program* node) {
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            visitFunctionDecl(static_cast<FunctionDecl*>(decl.get()));
        }
    }
}

// 主入口：生成IR模块
ir::IRModule IRGenerator::generate(Program* program) {
    ir::IRModule module;
    module_ = &module;
    regCounter_ = 0;
    blockCounter_ = 0;
    varCounter_ = 0;
    varStack_.clear();
    loopStack_.clear();
    visitProgram(program);
    module_ = nullptr;
    return module;
}

// 函数定义 -> IRFunction
// Task 2.2：函数原型声明（无函数体）不生成IR函数——
//   语义层已注册符号供调用检查；IR层不产出定义，使"声明后无定义却调用"在链接期报缺失
void IRGenerator::visitFunctionDecl(FunctionDecl* node) {
    if (node->body == nullptr) return;  // 函数原型声明：不生成IR函数（链接期缺失检测）
    ir::IRFunction func;
    func.name = node->name;
    func.returnType = mapType(node->returnType.empty() ? "空类型" : node->returnType);
    // Task 完善A：结构体返回值标记（返回类型为自定义结构体时走隐藏返回指针）
    if (semantic_ != nullptr && !node->returnType.empty() &&
        semantic_->isStructType(types::canonical(node->returnType))) {
        func.structReturn = true;
        // 记录精确大小（字节）：epilogue 按此拷贝到隐藏返回缓冲区（避免 64 字节
        //   硬编码越界写破坏相邻栈变量——班级 16 字节被写 64 字节越界 48 字节）
        func.structReturnSize = semantic_->typeSizeOf(node->returnType);
    }
    // Task 完善A：提前绑定 function_，使 registerVarSlots（参数多槽登记）
    //   在参数循环内即可生效（此前 function_ 在循环后设置，结构体参数
    //   的 varSlots 登记被 registerVarSlots 的 function_==nullptr 检查跳过）
    function_ = &func;
    // 参数进入最外层作用域，生成唯一内部名（name$N）。
    // params 保留源码名（对外可读/测试契约），paramUniques 存唯一名，
    // 代码生成层按 paramUniques 登记/查询栈槽，保证遮蔽变量各自独立槽
    varStack_.emplace_back();
    for (std::size_t pi = 0; pi < node->params.size(); ++pi) {
        auto& param = node->params[pi];
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        // Task 2.2：函数指针参数（整32(*func)(整32, 整32)）类型为 ptr
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(param->typeName);
        // Task 2.4：数组参数按多槽登记（varSlots）
        registerVarSlots(unique, param->funcPtr.isFunctionPtr() ? "" : param->typeName);
        // Task 完善A：结构体按值参数标记（语义层查询——结构体源码类型）
        if (semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
            semantic_->isStructType(types::canonical(param->typeName))) {
            func.structParamIndexes.insert(static_cast<int>(pi));
        }
        func.params.emplace_back(param->name, paramIrType);
        func.paramUniques.push_back(unique);
        ir::IRValue reg = newReg();
        reg.type = paramIrType;
        VarEntry entryInfo;
        entryInfo.regId = reg.id;
        entryInfo.uniqueName = unique;
        entryInfo.type = reg.type;
        entryInfo.srcType = param->typeName;  // 指针/数组复合类型源码名
        varStack_.back()[param->name] = entryInfo;
    }
    // 注意：varCounter_ 不可重置！参数已用 varCounter_ 生成唯一名，
    // 若重置则函数体内同名遮蔽变量会生成相同唯一名（如 x$0）导致槽冲突
    blockCounter_ = 0;
    // 入口基本块（ASCII标签 bbN：ml64 不识别中文标识符，阶段一统一 ASCII）
    ir::IRBlock* entry = newBlock("bb0");
    // 函数体
    if (node->body != nullptr) {
        genBlock(node->body.get());
    }
    // 无终止指令：补充默认返回（空类型函数 / 原型声明）
    if (!function_->blocks.empty()) {
        ir::IRBlock* last = function_->blocks.back().get();
        if (!last->terminated) {
            setCurrentBlock(last);
            endReturn("");
        }
    } else if (node->body == nullptr) {
        // 原型声明：生成空入口块 + 默认返回
        setCurrentBlock(entry);
        endReturn("");
    }
    module_->functions.push_back(std::move(func));
    function_ = nullptr;
}

void IRGenerator::visitParamDecl(ParamDecl* node) {
    // 参数由 visitFunctionDecl 统一处理
    (void)node;
}

// 变量声明：Alloca + 初始值 Store
void IRGenerator::visitVarDecl(VarDecl* node) {
    genVarDecl(node);
}

// ==================== 语句生成 ====================

// 代码块：顺序生成语句（进入子作用域，支持同名变量遮蔽）
void IRGenerator::genBlock(BlockStmt* node) {
    varStack_.emplace_back();  // 进入子作用域
    for (auto& stmt : node->statements) {
        genStmt(stmt.get());
    }
    varStack_.pop_back();  // 退出子作用域
}

// 表达式语句
void IRGenerator::visitExprStmt(ExprStmt* node) {
    genExpr(node->expr.get());
}

// 返回语句
// Task 2.3：返回值类型与函数返回类型不同时先隐式转换 Cast
// （如 整32 返回值经 整64 返回类型时宽化；8/16位返回类型截断）
// Task 完善A：返回结构体初始化（返回 点对{...}）——在临时变量构建结构体，
//   返回其地址（ptr）；调用方 CopyStruct 到目标。
void IRGenerator::visitReturnStmt(ReturnStmt* node) {
    if (node->value != nullptr) {
        // 结构体初始化返回：分配临时结构体变量（多槽），emitStructInitTo 写入，返回地址
        if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
            StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
            const std::string structType = types::canonical(init->typeName);
            if (semantic_->isStructType(structType)) {
                const std::string temp = "__ret" + std::to_string(varCounter_++);
                const std::string irType = "ptr";  // 结构体在 IR 层按 ptr（地址）
                emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, irType),
                     temp, irType, node->location);
                registerVarSlots(temp, structType);
                ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                              {ir::IRValue::var(temp, "i64")},
                                              "ptr", temp, node->location);
                emitStructInitTo(init, base, node->location);
                endReturn(base.toString());
                return;
            }
        }
        ir::IRValue value = genExpr(node->value.get());
        if (function_ != nullptr) {
            const std::string retType = function_->returnType;
            if (value.type != retType && !retType.empty() && retType != "void" &&
                value.type != "void") {
                value = emitResult(ir::Opcode::Cast, {value}, retType, "", node->location);
            }
        }
        endReturn(value.toString());
    } else {
        endReturn("");
    }
}

// 中断语句：跳转到循环出口块或选择语句出口块（语义层已保证合法性）
void IRGenerator::visitBreakStmt(BreakStmt* node) {
    (void)node;
    if (!loopStack_.empty()) {
        endJump(loopStack_.back().breakTarget);
    } else if (!switchStack_.empty()) {
        endJump(switchStack_.back());
    }
}

// 继续语句：跳转到循环更新块
void IRGenerator::visitContinueStmt(ContinueStmt* node) {
    (void)node;
    if (!loopStack_.empty()) {
        endJump(loopStack_.back().continueTarget);
    }
}

// 如果语句（AstVisitor入口）：转发到genIf
void IRGenerator::visitIfStmt(IfStmt* node) {
    genIf(node);
}

// 当循环（AstVisitor入口）：转发到genWhile
void IRGenerator::visitWhileStmt(WhileStmt* node) {
    genWhile(node);
}

// 循环语句（AstVisitor入口）：转发到genFor
void IRGenerator::visitForStmt(ForStmt* node) {
    genFor(node);
}

// 选择语句（AstVisitor入口）：转发到genSwitch
void IRGenerator::visitSwitchStmt(SwitchStmt* node) {
    genSwitch(node);
}

// 情况标签：由 genSwitch 统一生成（本入口不单独处理，防御性空实现）
void IRGenerator::visitCaseLabel(CaseLabel* node) {
    (void)node;
}

// 默认标签：由 genSwitch 统一生成（本入口不单独处理，防御性空实现）
void IRGenerator::visitDefaultLabel(DefaultLabel* node) {
    (void)node;
}

// 如果语句：条件跳转生成分支块
// 标签"预消费"blockCounter_（每标签++）：嵌套控制流（否则如果链/体内嵌套）从后续
// 编号开始，避免与外层预留标签重叠（否则 else-if 链出现 A2005 符号重定义）
void IRGenerator::genIf(IfStmt* node) {
    ir::IRValue cond = genExpr(node->condition.get());
    std::string thenLabel = "bb" + std::to_string(blockCounter_++);
    std::string elseLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 记录条件并终结当前块
    endBranch(cond.toString(), thenLabel, elseLabel);
    // 真分支
    setCurrentBlock(newBlock(thenLabel));
    if (node->thenBranch != nullptr) genBlock(node->thenBranch.get());
    if (!currentBlock_->terminated) endJump(endLabel);
    // 否则分支（可能为否则如果链）
    setCurrentBlock(newBlock(elseLabel));
    if (node->elseBranch != nullptr) {
        if (node->elseBranch->getType() == NodeType::IfStmt) {
            genIf(static_cast<IfStmt*>(node->elseBranch.get()));
        } else {
            genBlock(static_cast<BlockStmt*>(node->elseBranch.get()));
        }
    }
    if (!currentBlock_->terminated) endJump(endLabel);
    // 汇合块
    setCurrentBlock(newBlock(endLabel));
}

// 当循环：条件块 -> 循环体 -> 回边
// 标签预消费（同 genIf）：循环体内嵌套控制流从后续编号开始，避免标签重叠
void IRGenerator::genWhile(WhileStmt* node) {
    std::string condLabel = "bb" + std::to_string(blockCounter_++);
    std::string bodyLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 无条件跳入条件块
    endJump(condLabel);
    setCurrentBlock(newBlock(condLabel));
    ir::IRValue cond = genExpr(node->condition.get());
    endBranch(cond.toString(), bodyLabel, endLabel);
    // 循环体
    setCurrentBlock(newBlock(bodyLabel));
    loopStack_.push_back(LoopContext{endLabel, condLabel});  // 继续 -> 条件块
    if (node->body != nullptr) genBlock(node->body.get());
    loopStack_.pop_back();
    if (!currentBlock_->terminated) endJump(condLabel);
    // 出口块
    setCurrentBlock(newBlock(endLabel));
}

// 循环语句：初始化 -> 条件块 -> 循环体 -> 更新块 -> 回边
void IRGenerator::genFor(ForStmt* node) {
    // 初始化（入当前块）
    if (node->init != nullptr) {
        genStmt(node->init.get());
        if (currentBlock_->terminated) {
            // 初始化已终结（如返回）：后续不可达，补出口块
            setCurrentBlock(newBlock("bb" + std::to_string(blockCounter_)));
            return;
        }
    }
    std::string condLabel = "bb" + std::to_string(blockCounter_++);
    std::string bodyLabel = "bb" + std::to_string(blockCounter_++);
    std::string updLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 跳入条件块
    endJump(condLabel);
    setCurrentBlock(newBlock(condLabel));
    if (node->condition != nullptr) {
        ir::IRValue cond = genExpr(node->condition.get());
        endBranch(cond.toString(), bodyLabel, endLabel);
    } else {
        // 无限循环：无条件跳入循环体
        endJump(bodyLabel);
    }
    // 循环体
    setCurrentBlock(newBlock(bodyLabel));
    loopStack_.push_back(LoopContext{endLabel, updLabel});  // 继续 -> 更新块
    if (node->body != nullptr) genBlock(node->body.get());
    loopStack_.pop_back();
    if (!currentBlock_->terminated) endJump(updLabel);
    // 更新块
    setCurrentBlock(newBlock(updLabel));
    if (node->update != nullptr) {
        genExpr(node->update.get());
    }
    if (!currentBlock_->terminated) endJump(condLabel);
    // 出口块
    setCurrentBlock(newBlock(endLabel));
}

// 选择语句：级联条件跳转生成CFG
// 结构：进入块 -> 各case判断块（Eq比较+条件跳转）-> case体块 -> 汇合块
//   fallthrough：case体块未终止（无中断/返回）则跳转下一case体块
//   Phi 节点为优化器（Task 2.6）预留：IR为非严格SSA，变量按名寻址，本任务不生成Phi
// 实现要点：所有标签"随建随取"（newBlock 内部 blockCounter_++ 保证唯一），
//           绝不预先按偏移计算未来标签——case 体块内的嵌套控制流会新增块，
//           使预留编号与实际编号错位（A2005 符号重定义）
void IRGenerator::genSwitch(SwitchStmt* node) {
    const std::size_t caseCount = node->cases.size();
    // 第一遍：预创建全部标签块（判断块+体块+默认块+出口块），
    // 此时只记录标签字符串，块的创建按顺序推进（blockCounter_ 自增唯一）
    std::vector<std::string> testLabels;   // 各case判断块标签
    std::vector<std::string> bodyLabels;   // 各case体块标签
    testLabels.reserve(caseCount);
    bodyLabels.reserve(caseCount);
    const std::string defLabel = (node->defaultCase != nullptr)
        ? "bb" + std::to_string(blockCounter_++) : "";
    for (std::size_t i = 0; i < caseCount; ++i) {
        testLabels.push_back("bb" + std::to_string(blockCounter_++));
        bodyLabels.push_back("bb" + std::to_string(blockCounter_++));
    }
    const std::string endLabel = "bb" + std::to_string(blockCounter_++);

    // 选择值 -> 临时寄存器（仍生成在入口块）
    ir::IRValue sel = genExpr(node->condition.get());
    // 入口块跳入第一个判断块（无 case 时直接跳出口）
    endJump(caseCount > 0 ? testLabels.front() : endLabel);

    // 生成判断链（每case：cmp+条件跳转到体块，不匹配落入下一判断）
    const std::string fallLabel = node->defaultCase != nullptr ? defLabel : endLabel;
    for (std::size_t i = 0; i < caseCount; ++i) {
        setCurrentBlock(newBlock(testLabels[i]));
        // 比较：sel == 情况值（IR类型与选择值一致）
        ir::IRValue caseConst = ir::IRValue::constant(
            std::to_string(node->cases[i]->value), sel.type);
        ir::IRValue cmp = emitResult(ir::Opcode::Eq, {sel, caseConst}, "i1", "", node->location);
        // 匹配 -> 体块；不匹配 -> 下一判断/默认/出口
        const std::string nextTarget =
            (i + 1 < caseCount) ? testLabels[i + 1] : fallLabel;
        endBranch(cmp.toString(), bodyLabels[i], nextTarget);
        // 体块（嵌套控制流在此处 newBlock 自增，不影响已分配标签的唯一性）
        setCurrentBlock(newBlock(bodyLabels[i]));
        switchStack_.push_back(endLabel);  // case 分支内中断跳出选择
        for (auto& stmt : node->cases[i]->statements) {
            genStmt(stmt.get());
        }
        switchStack_.pop_back();
        // fallthrough：体块未终止则落入下一体块/默认体块/出口
        if (!currentBlock_->terminated) {
            if (i + 1 < caseCount) {
                endJump(bodyLabels[i + 1]);
            } else if (node->defaultCase != nullptr) {
                endJump(defLabel);
            } else {
                endJump(endLabel);
            }
        }
    }
    // 默认体块（若存在）
    if (node->defaultCase != nullptr) {
        setCurrentBlock(newBlock(defLabel));
        switchStack_.push_back(endLabel);  // 默认分支内中断跳出选择
        for (auto& stmt : node->defaultCase->statements) {
            genStmt(stmt.get());
        }
        switchStack_.pop_back();
        if (!currentBlock_->terminated) endJump(endLabel);
    }
    // 汇合块
    setCurrentBlock(newBlock(endLabel));
}

// 代码块语句：嵌套块顺序生成
void IRGenerator::visitBlockStmt(BlockStmt* node) {
    genBlock(node);
}

// 语句分发
void IRGenerator::genStmt(Stmt* node) {
    if (node == nullptr) return;
    switch (node->getType()) {
        case NodeType::VarDecl:
            genVarDecl(static_cast<VarDecl*>(node));
            break;
        case NodeType::ExprStmt:
            visitExprStmt(static_cast<ExprStmt*>(node));
            break;
        case NodeType::ReturnStmt:
            visitReturnStmt(static_cast<ReturnStmt*>(node));
            break;
        case NodeType::BreakStmt:
            visitBreakStmt(static_cast<BreakStmt*>(node));
            break;
        case NodeType::ContinueStmt:
            visitContinueStmt(static_cast<ContinueStmt*>(node));
            break;
        case NodeType::IfStmt:
            genIf(static_cast<IfStmt*>(node));
            break;
        case NodeType::WhileStmt:
            genWhile(static_cast<WhileStmt*>(node));
            break;
        case NodeType::ForStmt:
            genFor(static_cast<ForStmt*>(node));
            break;
        case NodeType::SwitchStmt:
            genSwitch(static_cast<SwitchStmt*>(node));
            break;
        case NodeType::BlockStmt:
            genBlock(static_cast<BlockStmt*>(node));
            break;
        case NodeType::CaseLabel:
        case NodeType::DefaultLabel:
            // 标签节点由 genSwitch 单独处理，不作为独立语句生成（防御性空实现）
            break;
        default:
            break;
    }
}

// 变量声明生成：Alloca + Store（Task 2.4：数组多槽分配 + 初始化列表）
void IRGenerator::genVarDecl(VarDecl* node) {
    // 源码类型（Task 2.4：整32* / 整32[5] 复合类型保留用于元素类型推断/数组槽数）
    const std::string srcType = node->funcPtr.isFunctionPtr() ? "函数指针" : node->typeName;
    // 类型推断：无显式类型时按初始值（阶段一简化）
    std::string irType = mapType(node->typeName.empty() ? "整32" : node->typeName);
    // Task 2.2：函数指针变量（整32(*回调)(整32, 整32)）类型为 ptr
    if (node->funcPtr.isFunctionPtr()) {
        irType = "ptr";
    }
    if (node->typeName.empty() && !node->funcPtr.isFunctionPtr() &&
        node->initializer != nullptr) {
        // Task 2.3：按初始值类型推断（字面量后缀决定位宽）
        if (node->initializer->getType() == NodeType::IntegerLiteral) {
            IntegerLiteral* lit = static_cast<IntegerLiteral*>(node->initializer.get());
            std::string litType = types::literalTypeOf(lit->raw, false);
            irType = mapType(litType.empty() ? "整32" : litType);
        } else if (node->initializer->getType() == NodeType::FloatLiteral) {
            FloatLiteral* lit = static_cast<FloatLiteral*>(node->initializer.get());
            irType = mapType(types::literalTypeOf(lit->raw, true));
        } else if (node->initializer->getType() == NodeType::StringLiteral) {
            irType = "ptr";
        } else if (node->initializer->getType() == NodeType::BoolLiteral) {
            irType = "i1";
        } else if (node->initializer->getType() == NodeType::CharLiteral) {
            irType = "i32";
        } else if (node->initializer->getType() == NodeType::NullLiteral) {
            irType = "ptr";  // 空指针字面量：指针类型（无 = 0）
        }
    }
    // 分配变量槽（数组自动多槽：registerVarSlots 按数组长度预留）
    allocVar(node->name, irType, srcType, node->location);
    const std::string unique = lookupVarName(node->name);

    // 初始值处理：
    //   1. 数组初始化列表 { 1, 2, 3 }：逐元素 Store 到数组槽[i]（部分初始化补零）
    //   2. 普通表达式：Store 到变量槽（Task 2.3 类型不一致先 Cast）
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::InitListExpr &&
        types::isArray(srcType)) {
        InitListExpr* initList = static_cast<InitListExpr*>(node->initializer.get());
        const std::string elemSrc = types::arrayElemOf(srcType);
        const std::string elemIrType = mapType(elemSrc);
        const int arrayLen = types::arrayLenOf(srcType);
        // 元素间距：结构体数组按结构体总大小（Task 2.7），普通类型 8 字节
        std::int64_t elemStride = 8;
        if (semantic_ != nullptr && semantic_->isStructType(types::canonical(elemSrc))) {
            elemStride = semantic_->typeSizeOf(elemSrc);
        }
        // 修复集成审查 BUG #5：i128/正128 数组初始化元素 stride = 16
        if (types::isI128(types::canonical(elemSrc))) {
            elemStride = 16;
        }
        // 逐元素存储：目标为 数组槽[i]（地址 = 数组基址 + i*元素大小，基址槽最深）
        for (std::size_t i = 0; i < initList->elements.size(); ++i) {
            // 数组槽i的地址 = 数组基址 + i*元素大小
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, elemIrType)},
                                          "ptr", unique, node->location);
            ir::IRValue offset = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                            std::to_string(static_cast<long long>(i) * elemStride),
                                            node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {base, offset}, "ptr", "",
                                          node->location);
            // 结构体元素（学生{...}）：逐字段写入（Task 2.7 修复——此前只写占位0）
            if (initList->elements[i]->getType() == NodeType::StructInitExpr &&
                semantic_ != nullptr) {
                emitStructInitTo(static_cast<StructInitExpr*>(initList->elements[i].get()),
                                 addr, node->location);
                continue;
            }
            // 普通元素：生成值 + Cast + StorePtr
            ir::IRValue elem = genExpr(initList->elements[i].get());
            if (elem.type != elemIrType) {
                elem = emitResult(ir::Opcode::Cast, {elem}, elemIrType, "",
                                  node->location);
            }
            emit(ir::Opcode::StorePtr, {addr, elem}, ir::IRValue(), "", elemIrType,
                 node->location);
        }
        // 部分初始化补零：剩余槽置0（C语义；结构体数组按元素间距步进）
        if (arrayLen > 0 && static_cast<int>(initList->elements.size()) < arrayLen) {
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, elemIrType)},
                                          "ptr", unique, node->location);
            for (int i = static_cast<int>(initList->elements.size()); i < arrayLen; ++i) {
                ir::IRValue offset = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                                std::to_string(static_cast<long long>(i) * elemStride),
                                                node->location);
                ir::IRValue addr = emitResult(ir::Opcode::Add, {base, offset}, "ptr", "",
                                              node->location);
                emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "", "i64",
                     node->location);
            }
        }
        return;
    }
    // 结构体/联合体初始化：类型名{ 字段 = 值, ... }（Task 2.7）
    // 逐字段计算字段地址（FieldAddr），再 StorePtr 写入字段值（嵌套结构体递归展开）
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::StructInitExpr &&
        semantic_ != nullptr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->initializer.get());
        const std::string structType = types::canonical(node->typeName);
        const StructDecl* decl = semantic_->findStruct(structType);
        if (decl != nullptr) {
            // 结构体基址 = 变量槽地址
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, "i64")},
                                          "ptr", unique, node->location);
            emitStructInitTo(init, base, node->location);
        }
        return;
    }
    // 普通表达式初始值 -> Store（目标用唯一内部名，保证遮蔽变量写入自己的槽）
    if (node->initializer != nullptr) {
        ir::IRValue value = genExpr(node->initializer.get());
        // 结构体变量初始化值为"函数返回的结构体地址（ptr）"（Task 完善A）：
        //   学生 张三加 = 加分(张三) —— 值是指向返回临时结构体的指针，
        //   需 CopyStruct 到本变量槽区（按值拷贝）
        if (semantic_ != nullptr && value.type == "ptr" &&
            semantic_->isStructType(types::canonical(node->typeName))) {
            const int size = semantic_->typeSizeOf(types::canonical(node->typeName));
            ir::IRValue dstAddr = emitResult(ir::Opcode::AddrOf,
                                             {ir::IRValue::var(unique, "i64")},
                                             "ptr", unique, node->location);
            emit(ir::Opcode::CopyStruct, {dstAddr, value}, ir::IRValue(),
                 std::to_string(size), "void", node->location);
            lastExpr_ = value;
            return;
        }
        if (value.type != irType && !irType.empty()) {
            value = emitResult(ir::Opcode::Cast, {value}, irType, "", node->location);
        }
        emit(ir::Opcode::Store, {value}, ir::IRValue(),
             unique, irType, node->location);
    }
}

// ==================== 表达式生成 ====================

// 整数字面量（Task 2.3：后缀决定类型，规格书4.3 + Task 完善A：i128 全范围）
// 无后缀 -> 整32；L -> 整64；LL -> 整128；U -> 正32；UL -> 正64；ULL -> 正128
// 特殊：无后缀但值超出 int32 范围（5000000000 等）自动提升为整64（值自适应）；
//       无后缀但值超出 int64 范围（>2^63-1）自动提升为整128（Task 完善A）
void IRGenerator::visitIntegerLiteral(IntegerLiteral* node) {
    std::string type = types::literalTypeOf(node->raw, false);
    if (type.empty()) type = "整32";
    // 无后缀（整32）但值超出 int32 范围：自动提升为整64
    if (type == "整32" && node->value > 2147483647LL) type = "整64";
    // 无后缀但值超出 int64 范围：自动提升为整128（十进制文本比较，2^63-1 上限）
    if ((type == "整32" || type == "整64") &&
        types::textExceedsInt64(types::stripLiteralSuffix(node->raw))) {
        type = "整128";
    }
    // i128/正128 字面量：常量文本存 低64位:高64位 十六进制（codegen 拆双槽加载）
    // 其余类型：直接存剥后缀十进制文本
    const std::string stripped = types::stripLiteralSuffix(node->raw);
    if (types::isI128(type)) {
        // Task 完善A：i128（有符号）越界检查——正128 上限 2^128-1（无符号），
        //   整128 上限 2^127-1（有符号）。超限报错（规格书4.3 字面量范围）。
        const bool isSigned = (type == "整128");
        const std::string limit = isSigned
                                      ? "170141183460469231731687303715884105727"  // 2^127-1
                                      : "340282366920938463463374607431768211455";  // 2^128-1
        if (stripped.size() > limit.size() ||
            (stripped.size() == limit.size() && stripped > limit)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "整数字面量超出" +
                                    std::string(isSigned ? "整128（2^127-1）" : "正128（2^128-1）") +
                                    "范围");
            lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, mapType(type), "0:0",
                                   node->location);
            return;
        }
        const std::string split = types::splitI128Text(stripped);
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, mapType(type),
                               split.empty() ? stripped : split, node->location);
    } else {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, mapType(type),
                               stripped, node->location);
    }
}

// 浮点字面量（Task 2.3：f后缀 -> 浮32，无后缀 -> 浮64）
// 常量以原始文本存储（codegen生成浮点位模式，MASM不支持浮点立即数）
void IRGenerator::visitFloatLiteral(FloatLiteral* node) {
    std::string type = types::literalTypeOf(node->raw, true);
    lastExpr_ = emitResult(ir::Opcode::ConstFloat, {}, mapType(type),
                           types::stripLiteralSuffix(node->raw), node->location);
}

// 字符串字面量：收集到常量池
void IRGenerator::visitStringLiteral(StringLiteral* node) {
    std::string text = decodeString(node->raw);
    int index = -1;
    auto it = module_->stringIndex.find(text);
    if (it != module_->stringIndex.end()) {
        index = it->second;
    } else {
        index = static_cast<int>(module_->stringConstants.size());
        module_->stringConstants.push_back(text);
        module_->stringIndex[text] = index;
    }
    lastExpr_ = emitResult(ir::Opcode::ConstString, {}, "ptr",
                           "@str" + std::to_string(index), node->location);
}

// 字符字面量：按Unicode码点常量（阶段一简化：取引号内首字符值）
// 注意：字符字面量为单引号（如 'B'），decodeString 只处理双引号字符串，
//       必须单独剥离单引号，否则取到的是 "'"（39）而非字符本身
void IRGenerator::visitCharLiteral(CharLiteral* node) {
    std::string text = node->raw;
    if (text.size() >= 2 && text.front() == '\'' && text.back() == '\'') {
        text = text.substr(1, text.size() - 2);
    }
    int code = 0;
    if (!text.empty()) code = static_cast<unsigned char>(text[0]);
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", std::to_string(code),
                           node->location);
}

// 布尔字面量
void IRGenerator::visitBoolLiteral(BoolLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstBool, {}, "i1",
                           node->value ? "真" : "假", node->location);
}

// 标识符表达式：加载变量 或 函数地址（Task 2.2）
// 变量：用唯一内部名定位栈槽（遮蔽变量读自己的槽）
// 数组名（Task 2.4）：退化为首元素地址（AddrOf 数组槽0，即基址）
// 函数名（不在变量表）：生成 FuncAddr 指令（函数指针赋值场景：回调 = 加）
void IRGenerator::visitIdentifierExpr(IdentifierExpr* node) {
    ir::IRValue reg = lookupVar(node->name);
    if (reg.id < 0) {
        // 未找到变量：可能是函数名（函数指针赋值）。生成函数地址。
        // 防御性：若连函数也不是（语义已报错），仍生成FuncAddr避免IR中断
        lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr", node->name, node->location);
        return;
    }
    const std::string unique = lookupVarName(node->name);
    const std::string srcType = lookupSrcType(node->name);
    if (types::isArray(srcType)) {
        // 数组名退化：AddrOf 数组基址（栈槽0）-> ptr
        lastExpr_ = emitResult(ir::Opcode::AddrOf,
                               {ir::IRValue::var(unique, reg.type)},
                               "ptr", unique, node->location);
        return;
    }
    // 结构体/联合体变量（Task 完善A）：按值语义——表达式值为"结构体地址"（ptr），
    // 而非 Load 读变量槽（槽内容是 8 字节垃圾）。后续按值传参/赋值/CopyStruct 用地址。
    if (semantic_ != nullptr && semantic_->isStructType(types::canonical(srcType))) {
        lastExpr_ = emitResult(ir::Opcode::AddrOf,
                               {ir::IRValue::var(unique, "i64")},
                               "ptr", unique, node->location);
        return;
    }
    lastExpr_ = emitResult(ir::Opcode::Load,
                           {ir::IRValue::var(unique, reg.type)},
                           reg.type, unique, node->location);
}

// 二元运算：递归生成左右操作数，输出运算指令
// Task 2.3：操作数类型不一致时先隐式转换 Cast 到公共类型
// （如 整8 + 整8 -> 整32 提升；整32 + 整64 -> 整64 宽化；整 + 浮 -> 浮64）
// Task 2.4：指针算术（ptr ± 整型）按8字节步进（栈槽统一8字节宽）
// Task 2.5：字符串连接（ptr + ptr -> __cn_str_concat 调用，语义层已保证为字符串）
void IRGenerator::visitBinaryExpr(BinaryExpr* node) {
    ir::IRValue left = genExpr(node->left.get());
    ir::IRValue right = genExpr(node->right.get());
    bool isFloat = (left.type == "f32" || left.type == "f64" ||
                    right.type == "f32" || right.type == "f64");
    // ---- 字符串连接（Task 2.5）：两个指针（字符串/字符*）的 + -> 运行时连接 ----
    // 说明：字符串类型在IR层映射为 ptr；两操作数均为 ptr 且运算符为 + 时视为连接
    //       （指针+整数 仍走下方指针算术分支；指针+指针 由语义层保证为字符串连接）
    if (left.type == "ptr" && right.type == "ptr" && node->op == Operator::Add) {
        lastExpr_ = emitResult(ir::Opcode::Call, {left, right}, "ptr",
                               "__cn_str_concat", node->location);
        return;
    }
    // ---- 指针算术（Task 2.4）：ptr ± 整型 -> 指针（偏移量×元素大小） ----
    if ((left.type == "ptr" && right.type != "ptr") ||
        (right.type == "ptr" && left.type != "ptr")) {
        const bool ptrLeft = (left.type == "ptr");
        ir::IRValue ptr = ptrLeft ? left : right;
        ir::IRValue delta = ptrLeft ? right : left;
        // 仅 + / - 允许指针算术（语义层已检查）
        if (node->op != Operator::Add && node->op != Operator::Subtract) {
            lastExpr_ = ptr;
            return;
        }
        // 偏移量转 i64；步进 = 指针所指元素大小（结构体指针按结构体总大小，
        // 普通指针8字节；Task 2.7 修复）
        if (delta.type != "i64") {
            delta = emitResult(ir::Opcode::Cast, {delta}, "i64", "", node->location);
        }
        // 指针操作数的源码类型：标识符查变量表，否则按 8 字节（无法推断）
        std::string ptrSrcType = "";
        if (ptrLeft && node->left->getType() == NodeType::IdentifierExpr) {
            ptrSrcType = lookupSrcType(
                static_cast<IdentifierExpr*>(node->left.get())->name);
        } else if (!ptrLeft && node->right->getType() == NodeType::IdentifierExpr) {
            ptrSrcType = lookupSrcType(
                static_cast<IdentifierExpr*>(node->right.get())->name);
        }
        const std::int64_t stride = ptrElemStride(ptrSrcType);
        ir::IRValue scaled = emitResult(
            ir::Opcode::Mul,
            {delta, ir::IRValue::constant(std::to_string(stride), "i64")},
            "i64", "", node->location);
        // 指针 - 整数：减法（delta 取负）；指针 + 整数：加法
        lastExpr_ = emitResult(
            node->op == Operator::Add ? ir::Opcode::Add : ir::Opcode::Sub,
            {ptr, scaled}, "ptr", "", node->location);
        return;
    }
    // 公共类型（浮点优先 f64；整型取 rank 高者由语义层保证可转换）
    if (!isFloat && left.type != right.type &&
        left.type != "i1" && right.type != "i1" &&
        left.type != "ptr" && right.type != "ptr") {
        // 整型混合：按语义 rank 提升（i8/i16 -> i32；整32 + 整64 -> 整64）
        const bool leftWider = (left.type == "i64" || left.type == "u64" ||
                                left.type == "i128" || left.type == "u128" ||
                                left.type == "u32" && right.type == "i32" ||
                                left.type == "i32" && right.type == "i8" ||
                                left.type == "i32" && right.type == "i16" ||
                                left.type == "u32" && right.type == "i8" ||
                                left.type == "u32" && right.type == "i16");
        const std::string common = leftWider ? left.type : right.type;
        if (left.type != common) {
            left = emitResult(ir::Opcode::Cast, {left}, common, "", node->location);
        }
        if (right.type != common) {
            right = emitResult(ir::Opcode::Cast, {right}, common, "", node->location);
        }
    } else if (isFloat && left.type != right.type) {
        // 整 + 浮：整侧转浮64；浮32 + 浮64：浮32转浮64
        const std::string common = "f64";
        if (left.type != common) {
            left = emitResult(ir::Opcode::Cast, {left}, common, "", node->location);
        }
        if (right.type != common) {
            right = emitResult(ir::Opcode::Cast, {right}, common, "", node->location);
        }
    }
    ir::Opcode opcode;
    std::string resultType;
    if (mapBinaryOp(node->op, isFloat, opcode)) {
        // 结果类型推导：
        //   比较/逻辑 -> i1
        //   位运算/移位 -> 整型（取左操作数类型，与语义 commonNumericType 一致）
        //   算术 -> 浮点取 f64（阶段一简化）/整型取左操作数类型
        switch (node->op) {
            case Operator::EqualEqual: case Operator::BangEqual:
            case Operator::Less: case Operator::LessEqual:
            case Operator::Greater: case Operator::GreaterEqual:
            case Operator::AndAnd: case Operator::OrOr:
                resultType = "i1";
                break;
            case Operator::Amp: case Operator::Pipe: case Operator::Caret:
            case Operator::LessLess: case Operator::GreaterGreater:
                resultType = left.type;
                break;
            default:
                resultType = isFloat ? "f64" : left.type;
                break;
        }
        lastExpr_ = emitResult(opcode, {left, right}, resultType, "", node->location);
    } else {
        // 不支持的操作：回退左操作数
        lastExpr_ = left;
    }
}

// 一元运算：! / - / ~ / ++ / --
void IRGenerator::visitUnaryExpr(UnaryExpr* node) {
    ir::IRValue operand = genExpr(node->operand.get());
    switch (node->op) {
        case Operator::Bang:
            lastExpr_ = emitResult(ir::Opcode::Not, {operand}, "i1", "", node->location);
            break;
        case Operator::Subtract: {
            // 一元负号：0 - 操作数
            ir::IRValue zero = zeroConst(operand.type);
            lastExpr_ = emitResult(ir::Opcode::Sub, {zero, operand}, operand.type, "",
                                   node->location);
            break;
        }
        case Operator::Tilde: {
            // 按位非（Task 2.3）：~x = x ^ -1（真正的位非，codegen 发射 not 指令）
            ir::IRValue minusOne = emitResult(ir::Opcode::ConstInt, {}, operand.type,
                                              "-1", node->location);
            lastExpr_ = emitResult(ir::Opcode::BitXor, {operand, minusOne}, operand.type,
                                   "", node->location);
            break;
        }
        case Operator::Increment:
        case Operator::Decrement: {
            // 自增/自减：数值 x = x ± 1；指针 x = x ± 元素大小（Task 2.4）
            std::string deltaText = "1";
            if (operand.type == "ptr") {
                // 指针步进：元素大小。IR层指针统一为ptr，元素大小通过操作数
                // 源码类型推断（结构体指针按结构体总大小，普通指针8字节，
                // Task 2.7 修复——此前固定8字节导致结构体指针遍历错位）
                std::string srcType = "";
                if (node->operand->getType() == NodeType::IdentifierExpr) {
                    srcType = lookupSrcType(
                        static_cast<IdentifierExpr*>(node->operand.get())->name);
                }
                deltaText = std::to_string(ptrElemStride(srcType));
            }
            ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, "i64", deltaText,
                                           node->location);
            ir::IRValue result = emitResult(
                node->op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                {operand, delta}, "ptr", "", node->location);
            // 仅当操作数为变量引用时写回（用唯一内部名定位槽）
            if (node->operand->getType() == NodeType::IdentifierExpr) {
                IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->operand.get());
                ir::IRValue slot = lookupVar(ident->name);
                if (slot.id >= 0) {
                    emit(ir::Opcode::Store, {result}, ir::IRValue(),
                         lookupVarName(ident->name), "ptr", node->location);
                }
            }
            lastExpr_ = result;
            break;
        }
        case Operator::AddressOf: {
            // 取地址 &变量（Task 2.4）：AddrOf(变量引用) -> 变量地址
            // 数组名作 & 操作数：数组名已是首元素地址，&数组 与 数组名 等价
            // （C语义；此处直接生成 AddrOf 取变量槽地址）
            const std::string unique = lookupVarName(
                node->operand->getType() == NodeType::IdentifierExpr
                    ? static_cast<IdentifierExpr*>(node->operand.get())->name
                    : "");
            if (node->operand->getType() == NodeType::IdentifierExpr && !unique.empty()) {
                // 变量槽地址：AddrOf 指令（codegen 生成 lea）
                lastExpr_ = emitResult(ir::Opcode::AddrOf,
                                       {ir::IRValue::var(unique, operand.type)},
                                       "ptr", unique, node->location);
            } else if (node->operand->getType() == NodeType::IndexExpr) {
                // &数组[i]：等价 数组[i] 的地址（lvalueAddress 计算基址+偏移）
                lastExpr_ = lvalueAddress(node->operand.get());
            } else if (node->operand->getType() == NodeType::MemberExpr) {
                // &p.x / &指针->x：字段地址（lvalueAddress 计算基址+偏移，Task 2.7）
                lastExpr_ = lvalueAddress(node->operand.get());
            } else if (node->operand->getType() == NodeType::UnaryExpr &&
                       static_cast<UnaryExpr*>(node->operand.get())->op == Operator::Deref) {
                // &*p：等价 p（解引用的地址即指针值）
                lastExpr_ = genExpr(static_cast<UnaryExpr*>(node->operand.get())->operand.get());
            } else {
                // 其他左值：直接使用其地址值（表达式本身是地址）
                lastExpr_ = genExpr(node->operand.get());
            }
            break;
        }
        case Operator::Deref: {
            // 解引用 *p（Task 2.4）：LoadPtr(指针值) 从指针地址加载元素
            // 元素IR类型：从操作数源码指针类型推断（整32* -> i32、浮64* -> f64）
            std::string elemType = "i64";  // 默认按64位（指针所指值存8字节槽）
            if (node->operand->getType() == NodeType::IdentifierExpr) {
                IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->operand.get());
                const std::string srcType = lookupSrcType(ident->name);
                if (!srcType.empty() && types::isPointer(srcType)) {
                    elemType = mapType(types::pointeeOf(srcType));
                }
            } else if (node->operand->getType() == NodeType::BinaryExpr) {
                // *（p + 1）等复合指针表达式：语义层已推导元素类型，
                // IR层指针运算结果保持 ptr；解引用按 64 位读取（i64）
                // 数组元素为 整32 时读满8字节槽取低32位，值语义正确
            }
            // 空指针检查（错误码3）：codegen 在 LoadPtr/StorePtr 处插桩
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {operand}, elemType,
                                   "", node->location);
            break;
        }
        default:
            lastExpr_ = operand;
            break;
    }
}

// 赋值表达式：Store + 返回值（标识符左值经 Store；下标/解引用左值经 StorePtr）
// Task 2.4：支持 数组[i] = v、*p = v 可写左值（lvalueAddress 计算目标地址）
void IRGenerator::visitAssignmentExpr(AssignmentExpr* node) {
    // 结构体字段左值：p.x = v / 指针->x = v（Task 2.7）
    // 通过 lvalueAddress 计算字段地址（FieldAddr），StorePtr 写入
    if (node->target->getType() == NodeType::MemberExpr) {
        MemberExpr* member = static_cast<MemberExpr*>(node->target.get());
        // 枚举成员赋值不合法（枚举值为只读常量）
        if (!member->isArrow && member->object->getType() == NodeType::IdentifierExpr) {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(member->object.get());
            std::int64_t v = 0;
            if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
                semantic_->enumValueOf(ident->name, member->memberName, v)) {
                lastExpr_ = genExpr(node->value.get());
                return;  // 语义层已报错（只读）
            }
        }
        ir::IRValue value = genExpr(node->value.get());
        // 目标类型：字段IR类型
        // 修复10：对象可为 变量（方形.x）/ 数组字段元素（方形.顶点[0].x）/
        //   指针字段元素（p[0].x）——递归推导对象结构体类型
        std::string objSrcType = "";
        if (member->object->getType() == NodeType::IdentifierExpr) {
            objSrcType = lookupSrcType(
                static_cast<IdentifierExpr*>(member->object.get())->name);
        } else if (member->object->getType() == NodeType::IndexExpr) {
            IndexExpr* idx = static_cast<IndexExpr*>(member->object.get());
            if (idx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(st)) objSrcType = types::arrayElemOf(st);
                else if (types::isPointer(st)) objSrcType = types::pointeeOf(st);
            } else if (idx->object->getType() == NodeType::MemberExpr) {
                // 方形.顶点[0].x：内层 方形.顶点 数组字段 -> 元素类型 坐标
                MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
                std::string innerObj = "";
                if (inner->object->getType() == NodeType::IdentifierExpr) {
                    innerObj = lookupSrcType(
                        static_cast<IdentifierExpr*>(inner->object.get())->name);
                }
                const StructDecl* innerDecl =
                    semantic_->findStruct(types::canonical(innerObj));
                if (innerDecl != nullptr) {
                    for (const auto& f : innerDecl->fields) {
                        if (f.name == inner->memberName) {
                            objSrcType = types::isArray(f.type)
                                             ? types::arrayElemOf(f.type) : f.type;
                            break;
                        }
                    }
                }
            }
        }
        if (member->isArrow && types::isPointer(objSrcType)) {
            objSrcType = types::pointeeOf(objSrcType);
        }
        std::string targetType = "i64";
        const StructDecl* decl = (semantic_ != nullptr)
                                     ? semantic_->findStruct(types::canonical(objSrcType))
                                     : nullptr;
        if (decl != nullptr) {
            for (const auto& f : decl->fields) {
                if (f.name == member->memberName) {
                    targetType = mapType(f.type);
                    break;
                }
            }
        }
        if (value.type != targetType) {
            value = emitResult(ir::Opcode::Cast, {value}, targetType, "", node->location);
        }
        // 字段地址 = 基址 + 偏移（FieldAddr；含空指针检查错误码3）
        ir::IRValue addr = lvalueAddress(node->target.get());
        // 复合赋值（p.x += 1 等）
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                value = emitResult(opcode, {current, value}, targetType, "", node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
             node->location);
        lastExpr_ = value;
        return;
    }
    // 下标/解引用左值：通过 lvalueAddress 计算目标地址，StorePtr 写入
    if (node->target->getType() == NodeType::IndexExpr ||
        (node->target->getType() == NodeType::UnaryExpr &&
         static_cast<UnaryExpr*>(node->target.get())->op == Operator::Deref)) {
        ir::IRValue value = genExpr(node->value.get());
        // 目标类型：语义层已推导（整32 元素 / 解引用元素类型）
        std::string targetType = "i64";
        if (node->target->getType() == NodeType::IndexExpr) {
            IndexExpr* idx = static_cast<IndexExpr*>(node->target.get());
            if (idx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(st)) targetType = mapType(types::arrayElemOf(st));
                else if (types::isPointer(st)) targetType = mapType(types::pointeeOf(st));
            } else if (idx->object->getType() == NodeType::MemberExpr) {
                // 出.分数[1] = v：对象为结构体数组字段（整32[3] 分数）——
                //   元素类型 = 字段数组元素类型（memberObjStructType 推导对象结构体）
                const std::string innerType = memberObjStructType(
                    static_cast<MemberExpr*>(idx->object.get()));
                const StructDecl* innerDecl = (semantic_ != nullptr)
                                                  ? semantic_->findStruct(types::canonical(innerType))
                                                  : nullptr;
                if (innerDecl != nullptr) {
                    MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
                    for (const auto& f : innerDecl->fields) {
                        if (f.name == inner->memberName) {
                            if (types::isArray(f.type)) {
                                targetType = mapType(types::arrayElemOf(f.type));
                            } else if (types::isPointer(f.type)) {
                                targetType = mapType(types::pointeeOf(f.type));
                            }
                            break;
                        }
                    }
                }
            }
        } else if (node->target->getType() == NodeType::UnaryExpr) {
            UnaryExpr* un = static_cast<UnaryExpr*>(node->target.get());
            if (un->operand->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(un->operand.get())->name);
                if (types::isPointer(st)) targetType = mapType(types::pointeeOf(st));
            }
        }
        if (value.type != targetType) {
            value = emitResult(ir::Opcode::Cast, {value}, targetType, "",
                               node->location);
        }
        // 复合赋值（*p += 1 等）：先读当前值再运算（简化：直接读地址）
        ir::IRValue addr = lvalueAddress(node->target.get());
        // 集成验证修复 Bug：下标/解引用目标的结构体整体赋值——
        //   `名单[j] = 名单[j+1]`（结构体指针数组元素交换）目标为 IndexExpr，
        //   原实现走 StorePtr 只存 8 字节 -> 结构体数据破坏。
        //   目标元素类型为结构体、右值为结构体值（IndexExpr/标识符）时生成 CopyStruct。
        if (semantic_ != nullptr &&
            node->target->getType() == NodeType::IndexExpr) {
            IndexExpr* tIdx = static_cast<IndexExpr*>(node->target.get());
            std::string tElemSrc;
            if (tIdx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(tIdx->object.get())->name);
                if (types::isArray(st)) tElemSrc = types::arrayElemOf(st);
                else if (types::isPointer(st)) tElemSrc = types::pointeeOf(st);
            }
            if (!tElemSrc.empty() &&
                semantic_->isStructType(types::canonical(tElemSrc)) &&
                !isCompoundAssignOp(node->op)) {
                // 右值：IndexExpr 结构体元素 -> 其地址（lvalueAddress）；
                // 标识符结构体变量 -> AddrOf
                ir::IRValue srcAddr;
                if (node->value->getType() == NodeType::IndexExpr ||
                    node->value->getType() == NodeType::MemberExpr) {
                    srcAddr = lvalueAddress(node->value.get());
                } else if (node->value->getType() == NodeType::IdentifierExpr) {
                    const std::string srcUnique = lookupVarName(
                        static_cast<IdentifierExpr*>(node->value.get())->name);
                    srcAddr = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(srcUnique, "i64")},
                                         "ptr", srcUnique, node->location);
                }
                if (srcAddr.id >= 0) {
                    const int size = semantic_->typeSizeOf(types::canonical(tElemSrc));
                    emit(ir::Opcode::CopyStruct, {addr, srcAddr}, ir::IRValue(),
                         std::to_string(size), "void", node->location);
                    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                           node->location);
                    return;
                }
            }
        }
        // 结构体初始化赋值：点数组[0] = 点{ x = 5, y = 6 }（Task 2.7）
        // 按目标地址逐字段写入（结构体不能作为单寄存器值）
        if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
            emitStructInitTo(static_cast<StructInitExpr*>(node->value.get()),
                             addr, node->location);
            lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
            return;
        }
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                value = emitResult(opcode, {current, value}, targetType, "",
                                   node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
             node->location);
        lastExpr_ = value;
        return;
    }
    // 标识符左值（原有路径）：Store
    if (node->target->getType() != NodeType::IdentifierExpr) {
        lastExpr_ = genExpr(node->value.get());
        return;
    }
    IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
    ir::IRValue value = genExpr(node->value.get());
    // 查找变量类型与唯一内部名
    std::string targetType = lookupVarType(ident->name);
    if (targetType.empty()) targetType = value.type;
    const std::string unique = lookupVarName(ident->name);
    // ---- 结构体整体赋值（Task 完善A）：b = a（C 语义逐字段拷贝 = 内存拷贝） ----
    // 目标/源均为结构体变量（源码类型是自定义结构体）时，生成 CopyStruct 指令：
    //   CopyStruct dstAddr=AddrOf(目标), srcAddr=AddrOf(源), extra=拷贝字节数
    // 结构体大小由语义层 typeSizeOf 计算（含数组字段，整体拷贝）。
    if (semantic_ != nullptr) {
        const std::string targetSrcType = lookupSrcType(ident->name);
        // 右值：标识符（b = a）或成员/下标（b = 名单[0]）或 链式赋值（b = c = a）
        std::string valueSrcType;
        bool isChainedAssign = false;  // 链式赋值：右值为内层赋值表达式
        if (node->value->getType() == NodeType::IdentifierExpr) {
            valueSrcType = lookupSrcType(
                static_cast<IdentifierExpr*>(node->value.get())->name);
        } else if (node->value->getType() == NodeType::MemberExpr) {
            valueSrcType = memberObjStructType(static_cast<MemberExpr*>(node->value.get()));
        } else if (node->value->getType() == NodeType::IndexExpr) {
            IndexExpr* ix = static_cast<IndexExpr*>(node->value.get());
            if (ix->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(ix->object.get())->name);
                if (types::isArray(st)) valueSrcType = types::arrayElemOf(st);
                else if (types::isPointer(st)) valueSrcType = types::pointeeOf(st);
            }
        } else if (node->value->getType() == NodeType::AssignmentExpr) {
            // 链式赋值 a = (b = c)：内层已把值写入 b（CopyStruct），
            // 内层返回值 = 源地址（ptr 寄存器，指向 c）。结构体源类型与目标相同。
            isChainedAssign = true;
            valueSrcType = targetSrcType;
        }
        if (semantic_->isStructType(types::canonical(targetSrcType)) &&
            semantic_->isStructType(types::canonical(valueSrcType))) {
            const int size = semantic_->typeSizeOf(types::canonical(targetSrcType));
            ir::IRValue dstAddr = emitResult(ir::Opcode::AddrOf,
                                             {ir::IRValue::var(unique, "i64")},
                                             "ptr", unique, node->location);
            // 源地址：标识符 -> AddrOf；成员/下标 -> lvalueAddress；
            // 链式赋值 -> 内层返回值（ptr 寄存器，源地址，内容与内层目标相同）
            ir::IRValue srcAddr;
            if (isChainedAssign) {
                srcAddr = value;  // 内层 CopyStruct 返回的源地址（ptr 寄存器）
            } else if (node->value->getType() == NodeType::IdentifierExpr) {
                const std::string srcUnique = lookupVarName(
                    static_cast<IdentifierExpr*>(node->value.get())->name);
                srcAddr = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(srcUnique, "i64")},
                                     "ptr", srcUnique, node->location);
            } else {
                srcAddr = lvalueAddress(node->value.get());
            }
            emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
                 std::to_string(size), "void", node->location);
            lastExpr_ = value;
            return;
        }
    }
    // 复合赋值：值 = 当前值 op 右值
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = genExpr(node->target.get());
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            ir::IRValue combined = emitResult(opcode, {current, value}, targetType, "",
                                              node->location);
            emit(ir::Opcode::Store, {combined}, ir::IRValue(), unique, targetType,
                 node->location);
            lastExpr_ = combined;
            return;
        }
    }
    // 简单赋值（Task 2.3：右值类型与目标类型不同时先隐式转换 Cast，
    // 如 整8 x = 30000 需截断、整32 -> 整64 需扩展、整 -> 浮 需转换）
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "", node->location);
    }
    emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, targetType,
         node->location);
    lastExpr_ = value;
}

// 函数调用：实参寄存器 -> Call / CallIndirect（Task 2.2）
// 直接调用（函数名）：Call 指令，extra=函数名
// 间接调用（函数指针变量）：先取指针值，再 CallIndirect（operand[0]=指针寄存器）
void IRGenerator::visitCallExpr(CallExpr* node) {
    // 判断是否为直接函数名调用（函数名不在变量表中）
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        if (lookupVar(calleeName).id < 0) isDirect = true;  // 不在变量表 -> 函数名
    }

    // ---- 打印行 展开（Task 2.5）：打印行("值:", 42) / 打印行(42) ----
    // 逐参数按类型调用运行时打印函数（字符串/整数/浮点），最后统一换行。
    // 展开条件：多参数（任意类型）或 单参数非字符串（整数/浮点直接打印）。
    // 单参数字符串保持原有 printLine 路径（codegen 映射），输出一致且不破坏既有行为。
    // 注意：打印函数返回 void，用 emit 直接发射（不分配结果寄存器，
    //       codegen 对 id<0 的结果不生成返回值存储）
    bool printLineExpand = false;
    if (isDirect && calleeName == "打印行") {
        if (node->arguments.size() > 1) {
            printLineExpand = true;  // 多参数：任意类型均展开
        } else if (node->arguments.size() == 1) {
            ir::IRValue first = genExpr(node->arguments[0].get());
            printLineExpand = (first.type != "ptr");  // 单参数非字符串展开
        }
    }
    if (printLineExpand) {
        for (auto& arg : node->arguments) {
            ir::IRValue argVal = genExpr(arg.get());
            // 参数类型 -> 打印函数名（字符串/字符* -> 字符串；整型 -> 整数；浮点 -> 浮点）
            std::string printFn;
            if (argVal.type == "ptr") {
                printFn = "__cn_print_str";
            } else if (argVal.type == "f32" || argVal.type == "f64") {
                printFn = "__cn_print_float";
            } else if (argVal.type == "i128" || argVal.type == "u128") {
                // i128/正128（Task 完善A）：传双寄存器地址（低64位槽地址），
                // 运行时辅助函数 __cn_print_i128 读 16 字节双槽
                printFn = (argVal.type == "u128") ? "__cn_print_u128"
                                                  : "__cn_print_i128";
            } else {
                // 整型（含 i1 布尔）：统一按 i64 传递
                if (argVal.type != "i64") {
                    argVal = emitResult(ir::Opcode::Cast, {argVal}, "i64", "",
                                        node->location);
                }
                printFn = "__cn_print_int";
            }
            emit(ir::Opcode::Call, {argVal}, ir::IRValue(), printFn, "void",
                 node->location);
        }
        // 统一换行
        emit(ir::Opcode::Call, {}, ir::IRValue(), "__cn_print_newline", "void",
             node->location);
        lastExpr_ = ir::IRValue();  // 无返回值（空类型）
        return;
    }

    // ---- 字符串API名称映射（Task 2.5 + Task 2.8）：中文函数名 -> 运行时符号 ----
    if (isDirect) {
        if (calleeName == "字符串长度") calleeName = "__cn_str_len";
        else if (calleeName == "字符串比较") calleeName = "__cn_str_eq";
        else if (calleeName == "字符串连接") calleeName = "__cn_str_concat";
        else if (calleeName == "字符串复制") calleeName = "__cn_str_copy";
        else if (calleeName == "字符串查找") calleeName = "__cn_str_find";
        // Task 2.8 补充API（规格书10.1 标注"常见字符串库补充"）
        else if (calleeName == "字符串子串") calleeName = "__cn_str_sub";
        else if (calleeName == "字符串字典序") calleeName = "__cn_str_cmp";
        else if (calleeName == "字符串大写") calleeName = "__cn_str_upper";
        else if (calleeName == "字符串小写") calleeName = "__cn_str_lower";
        else if (calleeName == "字符串前缀") calleeName = "__cn_str_starts_with";
        else if (calleeName == "字符串后缀") calleeName = "__cn_str_ends_with";
        else if (calleeName == "字符串包含") calleeName = "__cn_str_contains";
        else if (calleeName == "字符串修剪") calleeName = "__cn_str_trim";
        else if (calleeName == "字符串反转") calleeName = "__cn_str_reverse";
        else if (calleeName == "字符串从整数") calleeName = "__cn_str_from_int";
        else if (calleeName == "字符串从浮点") calleeName = "__cn_str_from_float";
        else if (calleeName == "字符串从字符") calleeName = "__cn_str_from_char";
        else if (calleeName == "字符串释放") calleeName = "__cn_str_free";
    }

    std::vector<ir::IRValue> args;
    for (std::size_t ai = 0; ai < node->arguments.size(); ++ai) {
        ir::IRValue argVal = genExpr(node->arguments[ai].get());
        // Task 2.3：8/16位整数实参先扩展为 i64；i128/u128 实参截断为 i64
        // （Win x64 ABI 整参按64位传递；否则 emitCall 的 mov rax, op 读到槽中高位垃圾）
        // 所有 <64位 整数实参统一 Cast 到 i64（Win x64 ABI 整参按64位传递；
        // u32/i32 等若直接 mov rcx, [槽] 会读到槽高位栈残留垃圾）
        // 集成验证修复：i128/u128 实参传给 i128/u128 参数时不得截断（如
        //   `月薪(调后.年薪)`、`调整年薪(员工[0], 250000000000000000LL)`）——
        //   原实现无条件截断为 i64，被调方按 16 字节读参数槽读到垃圾高位。
        bool argIs128 = (argVal.type == "i128" || argVal.type == "u128");
        bool paramIs128 = false;
        if (argIs128 && isDirect && semantic_ != nullptr) {
            const auto paramTypes = semantic_->funcParamTypesOf(calleeName);
            if (ai < paramTypes.size()) {
                const std::string canon = types::canonical(paramTypes[ai]);
                paramIs128 = (canon == "整128" || canon == "正128");
            }
        }
        if (argVal.type == "i8" || argVal.type == "i16" ||
            argVal.type == "u8" || argVal.type == "u16" ||
            argVal.type == "i32" || argVal.type == "u32" ||
            (argIs128 && !paramIs128)) {
            argVal = emitResult(ir::Opcode::Cast, {argVal}, "i64", "", node->location);
        } else if (argVal.type == "f32") {
            // f32 实参提升为 f64（Win x64 ABI 浮点参数按 xmm 传双精度）
            argVal = emitResult(ir::Opcode::Cast, {argVal}, "f64", "", node->location);
        }
        args.push_back(argVal);
    }
    if (isDirect) {
        // 直接调用：extra=函数名；按函数返回类型设置结果类型（Task 2.5 字符串API；
        // Task 2.7 集成修复：用户自定义函数经语义层查真实返回类型，避免浮64
        // 结果被误标 i32 导致 codegen 用 eax 读 xmm0 返回值）
        std::string resultType = "i32";
        if (calleeName == "__cn_str_len" || calleeName == "__cn_str_find" ||
            calleeName == "__cn_str_cmp") {
            resultType = "i64";       // 字符串长度/查找/字典序 -> 整64
        } else if (calleeName == "__cn_str_eq" || calleeName == "__cn_str_starts_with" ||
                   calleeName == "__cn_str_ends_with" || calleeName == "__cn_str_contains") {
            resultType = "i1";        // 字符串比较/前缀/后缀/包含 -> 布尔
        } else if (calleeName == "__cn_str_concat" || calleeName == "__cn_str_copy" ||
                   calleeName == "__cn_str_sub" || calleeName == "__cn_str_upper" ||
                   calleeName == "__cn_str_lower" || calleeName == "__cn_str_trim" ||
                   calleeName == "__cn_str_reverse" || calleeName == "__cn_str_from_int" ||
                   calleeName == "__cn_str_from_float" || calleeName == "__cn_str_from_char") {
            resultType = "ptr";       // 连接/复制/子串/大写/小写/修剪/反转/数字/字符转换 -> 字符串（指针）
        } else if (calleeName == "__cn_str_free") {
            // 字符串释放：空类型返回，resultType 保持 i32（与用户 void 函数调用一致：
            // 语义层"空类型"->mapType "void" 被下方过滤，emitResult 结果寄存器写入
            // eax 无害且符合现有 void 调用惯例）
        } else if (semantic_ != nullptr) {
            // 用户函数：查询语义层返回类型（未映射（空/未知）回退 i32）
            const std::string ret = semantic_->funcReturnTypeOf(calleeName);
            if (!ret.empty()) {
                const std::string mapped = mapType(ret);
                if (mapped != "void" && mapped != "") resultType = mapped;
            }
        }
        // 结构体返回值函数（Task 完善A）：调用方分配返回缓冲区（结构体临时变量），
        //   以隐藏指针（rcx，Win x64 ABI）传给被调方；被调方写入缓冲区。
        //   结果 = 缓冲区地址（ptr），供调用方 CopyStruct 到目标。
        if (resultType == "ptr" && semantic_ != nullptr &&
            semantic_->isStructType(types::canonical(
                semantic_->funcReturnTypeOf(calleeName)))) {
            const std::string retType = semantic_->funcReturnTypeOf(calleeName);
            const std::string temp = "__retbuf" + std::to_string(varCounter_++);
            emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
                 temp, "ptr", node->location);
            // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 的 64 字节
            //   拷贝上限一致（结构体实际大小 ≤64 字节场景）
            function_->varSlots[temp] = 8;
            registerVarSlots(temp, retType);
            ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(temp, "i64")},
                                         "ptr", temp, node->location);
            // 隐藏返回指针作为第一个参数（rcx）
            std::vector<ir::IRValue> hiddenArgs = args;
            hiddenArgs.insert(hiddenArgs.begin(), buf);
            emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(), calleeName, "void",
                 node->location);
            lastExpr_ = buf;
            return;
        }
        // 字符串释放（空类型返回）：用 emit 直接发射，不分配结果寄存器
        // （与打印行 void 展开一致；codegen 对 result.id<0 不生成返回值存储）
        if (calleeName == "__cn_str_free") {
            emit(ir::Opcode::Call, args, ir::IRValue(), calleeName, "void",
                 node->location);
            lastExpr_ = ir::IRValue();
            return;
        }
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType, calleeName,
                               node->location);
        return;
    }
    // 间接调用：先求被调者表达式（函数指针变量），再 CallIndirect
    ir::IRValue calleeVal = genExpr(node->callee.get());
    args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
    lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, "i32", "", node->location);
}

// 空指针字面量：无 -> 常量0（ptr）
void IRGenerator::visitNullLiteral(NullLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", node->location);
}

// 数组下标访问（Task 2.4）：数据[i] / p[i]
// 生成：地址 = 基址 + index*8（栈槽8字节宽） -> LoadPtr 加载元素
// 数组越界检查（错误码2）：运行时调用 cn_runtime_error(2)（codegen 插桩）
// 说明：IR层发射 Call cn_runtime_error 作为越界处理（由 codegen 处理成错误输出）
void IRGenerator::visitIndexExpr(IndexExpr* node) {
    // 对象：数组名（IdentifierExpr）或指针表达式
    if (node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->object.get());
        const std::string unique = lookupVarName(ident->name);
        const std::string srcType = lookupSrcType(ident->name);
        if (!unique.empty() && types::isArray(srcType)) {
            // 数组对象：基址 = AddrOf(数组槽0)；元素类型来自数组元素类型
            const std::string elemSrc = types::arrayElemOf(srcType);
            const std::string elemIrType = mapType(elemSrc);
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, elemIrType)},
                                          "ptr", unique, node->location);
            ir::IRValue index = genExpr(node->index.get());
            // 越界检查插桩：index < 0 || index >= 数组长度 -> 运行时错误(2)
            // （IR 层仅生成比较 + 条件跳转到错误块，codegen 处理）
            emitBoundsCheck(index, types::arrayLenOf(srcType), node->location);
            // 地址 = base + index*元素大小（C语义；结构体数组按总大小步进，Task 2.7）
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            std::int64_t elemStride = 8;
            if (semantic_ != nullptr) {
                if (semantic_->isStructType(elemSrc)) {
                    elemStride = semantic_->typeSizeOf(elemSrc);
                }
            }
            // 修复集成审查 BUG #5：i128/正128 数组元素 stride = 16 字节
            //   （原实现只处理结构体类型，i128 数组 `大数[1]` 步进 8 字节
            //   读到 大数[0] 高64位槽 -> 求和值错误）
            if (types::isI128(types::canonical(elemSrc))) {
                elemStride = 16;
            }
            ir::IRValue scaled = emitResult(
                ir::Opcode::Mul,
                {index, ir::IRValue::constant(std::to_string(elemStride), "i64")},
                "i64", "", node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {base, scaled}, "ptr", "",
                                          node->location);
            // 结构体元素（Task 完善A）：数组元素作为"结构体值"时返回地址（ptr），
            //   供按值传参/赋值/整体拷贝使用；普通元素 LoadPtr 加载值
            if (semantic_ != nullptr && semantic_->isStructType(types::canonical(elemSrc))) {
                lastExpr_ = addr;
                return;
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, elemIrType, "",
                                   node->location);
            return;
        }
        if (!unique.empty() && types::isPointer(srcType)) {
            // 指针对象（p[i]）：等价 *(p + i)；步进按元素大小（Task 2.7 修复）
            ir::IRValue ptr = genExpr(node->object.get());
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            const std::int64_t stride = ptrElemStride(srcType);
            ir::IRValue scaled = emitResult(ir::Opcode::Mul,
                                            {index, ir::IRValue::constant(std::to_string(stride), "i64")},
                                            "i64", "", node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {ptr, scaled}, "ptr", "",
                                          node->location);
            // 集成验证修复 Bug：结构体指针元素（如 排序(员工档案* 名单) 中
            //   名单[j]）作为"结构体值"应返回地址（供整体赋值/按值传参/字段访问），
            //   原实现统一 LoadPtr 读首 8 字节当指针 -> 排序交换读到垃圾地址崩溃
            const std::string pointee = types::pointeeOf(srcType);
            if (semantic_ != nullptr &&
                semantic_->isStructType(types::canonical(pointee))) {
                lastExpr_ = addr;
                return;
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                                   mapType(pointee), "",
                                   node->location);
            return;
        }
    }
    // 其他对象形式（下标表达式等）：直接取对象值作为地址（防御性）
    // 修复10：数组字段（方形.顶点[i]）按字段数组元素大小步进（坐标[4] -> 坐标 8 字节），
    //   非数组字段按 8 字节（指针假设）
    ir::IRValue obj = genExpr(node->object.get());
    ir::IRValue index = genExpr(node->index.get());
    if (index.type != "i64") {
        index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
    }
    std::int64_t stride = 8;
    if (node->object->getType() == NodeType::MemberExpr) {
        // 修复10/10b/10c：数组字段元素步进 + 越界检查（memberObjStructType 递归处理 arrow）
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        const std::string innerType = memberObjStructType(inner);
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName && types::isArray(f.type)) {
                    const std::string elemSrc = types::arrayElemOf(f.type);
                    stride = semantic_->isStructType(elemSrc)
                                 ? semantic_->typeSizeOf(elemSrc)
                                 : types::typeSize(elemSrc);
                    emitBoundsCheck(index, types::arrayLenOf(f.type), node->location);
                    break;
                }
            }
        }
    }
    // 元素 IR 类型（Task 完善A）：数组字段（一班.分数[i]）按字段数组元素类型；
    //   结构体元素返回地址（供按值传参），普通元素 LoadPtr 按元素类型
    std::string elemIrType = "i64";
    bool elemIsStruct = false;
    if (node->object->getType() == NodeType::MemberExpr) {
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        const std::string innerType = memberObjStructType(inner);
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName && types::isArray(f.type)) {
                    const std::string elemSrc = types::arrayElemOf(f.type);
                    elemIrType = mapType(elemSrc);
                    if (semantic_ != nullptr &&
                        semantic_->isStructType(types::canonical(elemSrc))) {
                        elemIsStruct = true;
                    }
                    break;
                }
            }
        }
    } else if (node->object->getType() == NodeType::IdentifierExpr) {
        const std::string st = lookupSrcType(
            static_cast<IdentifierExpr*>(node->object.get())->name);
        if (types::isPointer(st)) {
            const std::string elemSrc = types::pointeeOf(st);
            elemIrType = mapType(elemSrc);
            if (semantic_ != nullptr && semantic_->isStructType(types::canonical(elemSrc))) {
                elemIsStruct = true;
            }
        }
    }
    ir::IRValue scaled = emitResult(ir::Opcode::Mul,
                                    {index, ir::IRValue::constant(std::to_string(stride), "i64")},
                                    "i64", "", node->location);
    ir::IRValue addr = emitResult(ir::Opcode::Add, {obj, scaled}, "ptr", "",
                                  node->location);
    if (elemIsStruct) {
        lastExpr_ = addr;  // 结构体元素：返回地址（按值传参/整体拷贝用）
        return;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, elemIrType, "", node->location);
}

// 初始化列表作为表达式：不支持（语义层已报错），生成0占位
void IRGenerator::visitInitListExpr(InitListExpr* node) {
    (void)node;
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}

// 结构体/联合体初始化展开（Task 2.7）：将 StructInitExpr 的字段逐个写入 targetBase
// 嵌套结构体字段（值为 StructInitExpr）递归展开到 基址+字段偏移；
// 普通字段 genExpr 后 Cast 到字段IR类型再 StorePtr 写入
void IRGenerator::emitStructInitTo(StructInitExpr* init, const ir::IRValue& targetBase,
                                   const SourceLocation& loc) {
    if (semantic_ == nullptr || init == nullptr) return;
    const std::string structType = types::canonical(init->typeName);
    const StructDecl* decl = semantic_->findStruct(structType);
    if (decl == nullptr) return;
    for (auto& fieldPair : init->fields) {
        const std::string& fieldName = fieldPair.first;
        const int offset = semantic_->fieldOffsetOf(decl, fieldName);
        if (offset < 0) continue;  // 语义层已报错
        // 字段地址 = 基址 + 偏移（FieldAddr）
        ir::IRValue fieldAddr = emitResult(ir::Opcode::FieldAddr, {targetBase}, "ptr",
                                           std::to_string(offset), loc);
        // 嵌套结构体字段：递归展开
        if (fieldPair.second->getType() == NodeType::StructInitExpr) {
            emitStructInitTo(static_cast<StructInitExpr*>(fieldPair.second.get()),
                             fieldAddr, loc);
            continue;
        }
        // 数组字段初始化（Task 完善A）：字段值为 InitListExpr（如 分数 = { 80, 90, 70 }），
        //   逐元素写入 字段地址 + i*元素大小（C 语义）
        if (fieldPair.second->getType() == NodeType::InitListExpr) {
            InitListExpr* list = static_cast<InitListExpr*>(fieldPair.second.get());
            std::string fieldSrcType;
            for (const auto& f : decl->fields) {
                if (f.name == fieldName) { fieldSrcType = f.type; break; }
            }
            const std::string elemSrc = types::arrayElemOf(fieldSrcType);
            const std::string elemIrType = mapType(elemSrc);
            std::int64_t elemStride = types::typeSize(elemSrc);
            if (elemStride <= 0) elemStride = 8;  // 防御：未知类型按 8 字节
            if (semantic_->isStructType(types::canonical(elemSrc))) {
                elemStride = semantic_->typeSizeOf(elemSrc);
            } else if (types::isI128(types::canonical(elemSrc))) {
                elemStride = 16;
            }
            for (std::size_t i = 0; i < list->elements.size(); ++i) {
                ir::IRValue elemOff = emitResult(
                    ir::Opcode::ConstInt, {}, "i64",
                    std::to_string(static_cast<long long>(i) * elemStride), loc);
                ir::IRValue elemAddr = emitResult(ir::Opcode::Add, {fieldAddr, elemOff},
                                                  "ptr", "", loc);
                // 结构体数组元素（元素为 StructInitExpr）：递归展开
                if (list->elements[i]->getType() == NodeType::StructInitExpr &&
                    semantic_ != nullptr) {
                    emitStructInitTo(
                        static_cast<StructInitExpr*>(list->elements[i].get()),
                        elemAddr, loc);
                    continue;
                }
                ir::IRValue elem = genExpr(list->elements[i].get());
                if (elem.type != elemIrType && !elemIrType.empty() && elem.type != "") {
                    elem = emitResult(ir::Opcode::Cast, {elem}, elemIrType, "", loc);
                }
                emit(ir::Opcode::StorePtr, {elemAddr, elem}, ir::IRValue(), "",
                     elemIrType, loc);
            }
            continue;
        }
        // 普通字段：生成值 + Cast + StorePtr
        ir::IRValue value = genExpr(fieldPair.second.get());
        std::string fieldIrType = "i32";
        for (const auto& f : decl->fields) {
            if (f.name == fieldName) {
                fieldIrType = mapType(f.type);
                break;
            }
        }
        if (value.type != fieldIrType && !fieldIrType.empty() && value.type != "") {
            value = emitResult(ir::Opcode::Cast, {value}, fieldIrType, "", loc);
        }
        emit(ir::Opcode::StorePtr, {fieldAddr, value}, ir::IRValue(), "",
             fieldIrType, loc);
    }
}

// 结构体/联合体初始化作为表达式（Task 2.7）
// 无法作为独立值（结构体可能 > 8 字节），此处生成 0 占位；
// 实际字段写入由调用方（genVarDecl / 赋值左值路径）按目标地址调用 emitStructInitTo
void IRGenerator::visitStructInitExpr(StructInitExpr* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}

// 成员访问：结构体/联合体字段读取（Task 2.7）
// .   ：对象为结构体值 → 对象地址(AddrOf/lvalueAddress) + 字段偏移
// ->  ：对象为结构体指针 → 指针值 + 字段偏移（隐含空指针检查，错误码3）
// 枚举引用（枚举名.成员）：生成枚举成员整数值（ConstInt）
void IRGenerator::visitMemberExpr(MemberExpr* node) {
    // 枚举引用：枚举名.成员 → 整数值（Task 2.7）
    if (!node->isArrow && node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->object.get());
        std::int64_t enumVal = 0;
        if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
            semantic_->enumValueOf(ident->name, node->memberName, enumVal)) {
            lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32",
                                   std::to_string(enumVal), node->location);
            return;
        }
    }
    // 结构体字段访问：计算字段地址（FieldAddr），再按字段类型 LoadPtr
    // 地址：直接调用 lvalueAddress(node) 递归处理（. 对象为变量/嵌套成员/下标；
    //        -> 对象为指针值——lvalueAddress 对 MemberExpr 已按 isArrow 区分）
    // 说明：lvalueAddress(MemberExpr) 递归计算 基址+偏移，-> 隐含空指针检查（错误码3）
    ir::IRValue fieldAddr = lvalueAddress(node);
    // 查询字段类型（语义层布局，递归解析对象类型）
    if (semantic_ == nullptr) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 对象源码类型：. 访问为变量源码类型（含嵌套 r.左上 的字段类型递归）；
    //               -> 访问为指针所指类型
    std::string objSrcType = "";
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objSrcType = lookupSrcType(static_cast<IdentifierExpr*>(node->object.get())->name);
    } else if (node->object->getType() == NodeType::MemberExpr) {
        // 嵌套成员：r.左上 的类型 = 外层结构体"矩形"的字段"左上"类型
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        std::string innerObjType = "";
        if (inner->object->getType() == NodeType::IdentifierExpr) {
            innerObjType = lookupSrcType(
                static_cast<IdentifierExpr*>(inner->object.get())->name);
        }
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerObjType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName) {
                    objSrcType = f.type;
                    break;
                }
            }
        }
    } else if (node->object->getType() == NodeType::IndexExpr) {
        // 数组元素成员：点数组[1].x — 元素类型 = 数组元素类型（结构体）
        IndexExpr* idx = static_cast<IndexExpr*>(node->object.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string arrType = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            if (types::isArray(arrType)) {
                objSrcType = types::arrayElemOf(arrType);
            } else if (types::isPointer(arrType)) {
                // 集成验证修复 Bug：指针下标元素成员（如 排序(员工档案* 名单)
                //   中 名单[j].年薪）— 元素类型 = 指针所指类型。
                //   原实现只认数组变量，指针下标 objSrcType 推导失败 ->
                //   decl==nullptr -> 字段访问降级为 0，i128 比较恒假（排序失效）
                objSrcType = types::pointeeOf(arrType);
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 修复10/10b（数组字段元素成员读取）：方形.顶点[0].x /
            //   方形指针->顶点[0].x — 内层成员是数组字段，元素类型 = 字段数组元素类型（坐标）
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            const std::string innerType = memberObjStructType(inner);
            const StructDecl* innerDecl =
                semantic_->findStruct(types::canonical(innerType));
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName) {
                        objSrcType = types::isArray(f.type) ? types::arrayElemOf(f.type)
                                                            : f.type;
                        break;
                    }
                }
            }
        }
    } else if (node->isArrow && node->object->getType() == NodeType::BinaryExpr) {
        // 指针算术结果成员：(名单 + (n-1))->分数 — 从左操作数（指针变量）
        // 推导元素类型（Task 2.7 集成修复：此前 decl==nullptr 返回占位0，
        // 导致结构体指针算术+成员访问组合读取恒为0）
        BinaryExpr* bin = static_cast<BinaryExpr*>(node->object.get());
        if (bin->left->getType() == NodeType::IdentifierExpr) {
            const std::string ptrType = lookupSrcType(
                static_cast<IdentifierExpr*>(bin->left.get())->name);
            if (types::isPointer(ptrType)) {
                objSrcType = types::pointeeOf(ptrType);
            }
        }
    }
    if (node->isArrow && types::isPointer(objSrcType)) {
        objSrcType = types::pointeeOf(objSrcType);
    }
    const StructDecl* decl = semantic_->findStruct(types::canonical(objSrcType));
    if (decl == nullptr) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 字段类型（IR类型）
    std::string fieldSrcType = "";
    for (const auto& f : decl->fields) {
        if (f.name == node->memberName) {
            fieldSrcType = f.type;
            break;
        }
    }
    // 修复10（数组字段退化）：结构体数组字段（如 方形.顶点）作为值表达式时，
    //   按 C 语义退化为指向首元素的指针——返回字段地址（FieldAddr）而非 LoadPtr
    //   读取字段处 8 字节当指针（垃圾值 -> 空指针错误3/访问冲突崩溃）。
    //   后续 方形.顶点[i] 的基址即此字段地址。
    if (types::isArray(fieldSrcType)) {
        lastExpr_ = fieldAddr;
        return;
    }
    // 字段加载（LoadPtr 含空指针检查：错误码3）
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {fieldAddr},
                           mapType(fieldSrcType), "", node->location);
}

// 结构体/联合体声明：类型定义不生成IR（布局已在语义层计算，字段访问按需查询）
void IRGenerator::visitStructDecl(StructDecl* node) {
    (void)node;
}

// 枚举声明：不生成IR（枚举值为编译期常量，使用时直接 ConstInt）
void IRGenerator::visitEnumDecl(EnumDecl* node) {
    (void)node;
}

// 类型节点：不生成IR
void IRGenerator::visitType(Type* node) {
    (void)node;
}

// ==================== 表达式分发 ====================

// 生成表达式，返回结果寄存器
ir::IRValue IRGenerator::genExpr(Expr* node) {
    if (node == nullptr) {
        return ir::IRValue::reg(-1, "");
    }
    node->accept(*this);
    return lastExpr_;
}

} // namespace cn_compiler
