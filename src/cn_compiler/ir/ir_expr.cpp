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
    // 审查修复：无符号后缀（U=正32 / UL=正64）字面量值超出对应位宽时提升。
    //   原实现漏了无符号提升，`9000000000000000000U`（正32 但值 > 2^32-1）走
    //   stripped 十进制文本直接存，codegen std::stoull 溢出回绕（3800301568）。
    //   正32 超 2^32-1 -> 正64；正64 超 2^64-1 -> 正128。
    const std::string strippedForUnsigned = types::stripLiteralSuffix(node->raw);
    if (type == "正32" && types::textExceedsU32(strippedForUnsigned)) {
        type = "正64";
    }
    if (type == "正64" && types::textExceedsU64(strippedForUnsigned)) {
        type = "正128";
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
void IRGenerator::visitFloatLiteral(FloatLiteral* node) {
    std::string type = types::literalTypeOf(node->raw, true);
    lastExpr_ = emitResult(ir::Opcode::ConstFloat, {}, mapType(type),
                           types::stripLiteralSuffix(node->raw), node->location);
}
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
int IRGenerator::internEmptyString() {
    auto it = module_->stringIndex.find("");
    if (it != module_->stringIndex.end()) return it->second;
    const int index = static_cast<int>(module_->stringConstants.size());
    module_->stringConstants.push_back("");
    module_->stringIndex[""] = index;
    return index;
}
ir::IRValue IRGenerator::evalDefaultExpr(Expr* expr, ir::IRFunction& func) {
    (void)func;
    switch (expr->getType()) {
        case NodeType::IntegerLiteral: {
            IntegerLiteral* lit = static_cast<IntegerLiteral*>(expr);
            std::string type = types::literalTypeOf(lit->raw, false);
            const std::string irType = mapType(type.empty() ? "整32" : type);
            return ir::IRValue::constant(std::to_string(lit->value), irType);
        }
        case NodeType::FloatLiteral: {
            FloatLiteral* lit = static_cast<FloatLiteral*>(expr);
            std::string type = types::literalTypeOf(lit->raw, true);
            return ir::IRValue::constant(types::stripLiteralSuffix(lit->raw),
                                         mapType(type));
        }
        case NodeType::StringLiteral: {
            StringLiteral* lit = static_cast<StringLiteral*>(expr);
            const std::string text = decodeString(lit->raw);
            int index = -1;
            auto it = module_->stringIndex.find(text);
            if (it != module_->stringIndex.end()) {
                index = it->second;
            } else {
                index = static_cast<int>(module_->stringConstants.size());
                module_->stringConstants.push_back(text);
                module_->stringIndex[text] = index;
            }
            return ir::IRValue::constant("@str" + std::to_string(index), "ptr");
        }
        case NodeType::BoolLiteral: {
            BoolLiteral* lit = static_cast<BoolLiteral*>(expr);
            return ir::IRValue::constant(lit->value ? "真" : "假", "i1");
        }
        case NodeType::CharLiteral: {
            CharLiteral* lit = static_cast<CharLiteral*>(expr);
            std::string text = lit->raw;
            // 字符字面量：单引号内首字符码点（'A' -> 65；'\u{4E2D}' 全解码）
            if (text.size() >= 3 && text.front() == '\'' && text.back() == '\'') {
                const std::string inner = text.substr(1, text.size() - 2);
                if (inner.size() == 1) {
                    return ir::IRValue::constant(
                        std::to_string(static_cast<unsigned char>(inner[0])), "i32");
                }
            }
            return ir::IRValue::constant("0", "i32");
        }
        case NodeType::UnaryExpr: {
            // 一元负号：-N（常量取负）
            UnaryExpr* un = static_cast<UnaryExpr*>(expr);
            if (un->op == Operator::Subtract) {
                ir::IRValue inner = evalDefaultExpr(un->operand.get(), func);
                if (inner.isConstant && inner.type != "ptr" && inner.type != "i1") {
                    try {
                        const long long v = std::stoll(inner.extra);
                        return ir::IRValue::constant(std::to_string(-v), inner.type);
                    } catch (...) {
                        return ir::IRValue::constant("0", inner.type);
                    }
                }
            }
            return ir::IRValue::constant("0", "i64");
        }
        default:
            // 非字面量默认值（标识符/调用等）：语义层已报错，此处补 0
            return ir::IRValue::constant("0", "i64");
    }
}
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
void IRGenerator::visitBoolLiteral(BoolLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstBool, {}, "i1",
                           node->value ? "真" : "假", node->location);
}
void IRGenerator::visitIdentifierExpr(IdentifierExpr* node) {
    // ---- 阶段3 OOP（Task 3.1）：方法体内直接字段读取（无 自身. 前缀） ----
    // 字段名 不在当前方法作用域（局部变量/参数）但命中类字段表 -> this+偏移 LoadPtr。
    // 注意：须在 lookupVar 之前判定（字段可能被语义层并入作用域，但 IR 层未并入）。
    if (handleClassFieldRead(node)) {
        return;
    }
    // 第 4 层（v2.0 决策9，P1-4）：顶层常量引用——编译期常量折叠。
    //   常量名 -> 字面量值文本（语义层 globalConstValue 查询），直接生成
    //   常量加载（ConstInt/ConstFloat/ConstString），避免按变量生成栈槽。
    if (semantic_ != nullptr) {
        const std::string constText = semantic_->globalConstValue(node->name);
        if (!constText.empty()) {
            if (constText.size() >= 2 &&
                (constText.front() == '"' || constText.front() == '\'')) {
                // 字符串常量：去引号后入字符串常量池
                std::string strVal = constText;
                if (strVal.size() >= 2) strVal = strVal.substr(1, strVal.size() - 2);
                lastExpr_ = emitResult(ir::Opcode::ConstString, {}, "ptr", strVal,
                                       node->location);
            } else if (constText.find_first_of(".eE") != std::string::npos) {
                lastExpr_ = emitResult(ir::Opcode::ConstFloat, {}, "f64", constText,
                                       node->location);
            } else {
                // 整数常量（可能为十六进制/负数 raw，codegen 按文本解析）
                lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i64", constText,
                                       node->location);
            }
            return;
        }
    }
    ir::IRValue reg = lookupVar(node->name);
    if (reg.id < 0) {
        // 第 9 层 Debug（P3-8）：顶层静态变量读取——全局 .data 符号 LoadPtr。
        //   静态变量不在函数局部 varStack_，须按全局符号地址读取（?gstatic_名）；
        //   放在 lookupVar 失败后（局部变量优先，防止同名遮蔽误读全局）。
        if (semantic_ != nullptr && semantic_->isGlobalStatic(node->name)) {
            const std::string stType = semantic_->globalStaticType(node->name);
            const std::string irT = mapType(stType.empty() ? "整64" : stType);
            ir::IRValue addr = emitResult(
                ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + node->name, node->location);
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, irT, "", node->location);
            return;
        }
        // 未找到变量：可能是函数名（函数指针赋值）。生成函数地址。
        // 防御性：若连函数也不是（语义已报错），仍生成FuncAddr避免IR中断
        // Task 2.10 重载：函数名作值（回调 = 加）须用决议后的签名 key——
        //   定义处符号按 mangledName（名#参数串）发射，此处取首个签名保持一致；
        //   无参函数/单版本函数 sigKey 即纯名，行为不变
        std::string funcSym = node->name;
        if (semantic_ != nullptr) {
            const std::string sig = semantic_->funcFirstSigKey(node->name);
            if (!sig.empty()) funcSym = sig;
        }
        lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr", funcSym, node->location);
        return;
    }
    const std::string unique = lookupVarName(node->name);
    const std::string srcType = lookupSrcType(node->name);
    // 缺陷修复（[&] 引用捕获读取，规格书04-一D）：参数槽存被捕获变量地址——
    //   数组/结构体：[&] 捕获时存的是变量地址（AddrOf 值），读取 = 取出该地址
    //   （地址形态，供下标/字段/按值传参使用）；
    //   标量/字符串/i128：[&] 捕获须解引用（LoadPtr）读最新值。
    if (isByRefCapture(node->name)) {
        // A-1（引用参数）：srcType 可能是 账户& / 整32[5]&（引用参数带 &），
        //   数组/结构体形态判断前先剥 &（被引用对象形状决定取地址还是取值）
        const std::string stRef = types::stripRef(srcType);
        if (types::isArray(stRef) ||
            (semantic_ != nullptr &&
             semantic_->isStructType(types::canonical(stRef)))) {
            lastExpr_ = emitResult(ir::Opcode::Load,
                                   {ir::IRValue::var(unique, "ptr")},
                                   "ptr", unique, node->location);
            return;
        }
        ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(unique, "ptr")},
                                         "ptr", unique, node->location);
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {capAddr}, reg.type,
                               "", node->location);
        return;
    }
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
bool IRGenerator::isStringTypedExpr(Expr* node) const {
    if (node == nullptr) return false;
    switch (node->getType()) {
        case NodeType::StringLiteral:
            return true;
        case NodeType::IdentifierExpr: {
            const std::string st = types::canonical(lookupSrcType(
                static_cast<IdentifierExpr*>(node)->name));
            return (st == "字符串" || st == "字符*");
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(node);
            if (bin->op == Operator::Add) {
                // 连接/拼接结果：左字符串（含数值拼接）即结果字符串
                return isStringTypedExpr(bin->left.get());
            }
            return false;
        }
        case NodeType::IndexExpr: {
            // 数组元素：元素类型为字符串/字符（字符串[i] -> 字符 仍可参与拼接）
            IndexExpr* idx = static_cast<IndexExpr*>(node);
            if (idx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = types::canonical(lookupSrcType(
                    static_cast<IdentifierExpr*>(idx->object.get())->name));
                if (types::isArray(st)) {
                    const std::string elem = types::canonical(types::arrayElemOf(st));
                    return (elem == "字符串" || elem == "字符");
                }
            }
            return false;
        }
        default:
            return false;
    }
}
void IRGenerator::visitBinaryExpr(BinaryExpr* node) {
    // ---- 逻辑与/或短路求值（2026-09-08 根治，缺陷零容忍；rustc 同构手法）----
    // 原实现走通用二元路径：两侧先各自 genExpr 再发单条 And/Or 指令=RHS 无条件
    //   求值，与短路语义分歧——RHS 带副作用时（函数调用/除法/驻留等）行为错误。
    //   组件对拍灰色点（v2p 与 cn_self 字符串池编号稳定差 2）即其可观测指纹：
    //   v2 编译器源码大量「当前ID(...) == 驻留("&")」判定，宿主全求值使 驻留("&")
    //   提前入池，v2p 与 cn_self 的池序漂移。v2 自举编译器已正确短路（条件块
    //   发射实证），根治=宿主对齐 rustc HIR->THIR：&&/|| 在 IR 生成期脱糖为
    //   控制流（性能：跳过不需 BraHS 求值更快；安全：副作用按语义执行）。
    //   a && b : 结果槽预置 假 -> a 真? 求值 b 存槽 : 直达汇合
    //   a || b : 结果槽预置 真 -> a 真? 直达汇合 : 求值 b 存槽
    // 语义层保证逻辑操作数恒为布尔（IR i1），LHS/RHS 均无需真值转换。
    // And/Or 指令保留：编译器内部布尔组合（空/边界检查，ir.cpp/ir_oop*.cpp）
    //   操作数均为纯值，全求值语义等价，不受本修复影响。
    if (node->op == Operator::AndAnd || node->op == Operator::OrOr) {
        const bool isAnd = (node->op == Operator::AndAnd);
        ir::IRValue lhs = genExpr(node->left.get());
        // 三块结构（对齐 genIf 惯例：endBranch 时条件指令必须在块尾——若在其后
        //   夹入 Alloca/Store，条件寄存器会挂错指令，分支读到错误条件）：
        //   主块(LHS 求值 -> endBranch) -> {RHS块(求值b存槽) | 默认块(存短路值)} -> 汇合块
        //   a && b : a 真? -> RHS块（结果=b） : -> 默认块（结果=假）
        //   a || b : a 真? -> 默认块（结果=真） : -> RHS块（结果=b）
        // 结果临时槽（__sc$N，对齐 __ternary$N 模式：8 字节槽，i1 按 Store/Load
        //   的 type 字段决定读写宽度）
        std::string tempName = "__sc$" + std::to_string(varCounter_++);
        std::string rhsLabel = "bb" + std::to_string(blockCounter_++);
        std::string defaultLabel = "bb" + std::to_string(blockCounter_++);
        std::string endLabel = "bb" + std::to_string(blockCounter_++);
        endBranch(lhs.toString(),
                  isAnd ? rhsLabel : defaultLabel,
                  isAnd ? defaultLabel : rhsLabel);
        // RHS 块：仅语义需要时进入（短路保证）
        setCurrentBlock(newBlock(rhsLabel));
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "i64"),
             tempName, "i64", node->location);
        function_->varSlots[tempName] = 1;
        ir::IRValue rhs = genExpr(node->right.get());
        emit(ir::Opcode::Store, {rhs}, ir::IRValue(), tempName, rhs.type,
             node->location);
        if (!currentBlock_->terminated) endJump(endLabel);
        // 默认块：短路直达（&& 左假=假 / || 左真=真）
        setCurrentBlock(newBlock(defaultLabel));
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "i64"),
             tempName, "i64", node->location);
        ir::IRValue defaultVal = ir::IRValue::constant(isAnd ? "假" : "真", "i1");
        // i1 常量文本契约：loadOperandToX 仅识别 "真"/"假"（其余装载 0）
        emit(ir::Opcode::Store, {defaultVal}, ir::IRValue(), tempName, "i1",
             node->location);
        if (!currentBlock_->terminated) endJump(endLabel);
        setCurrentBlock(newBlock(endLabel));
        lastExpr_ = emitResult(ir::Opcode::Load,
                               {ir::IRValue::var(tempName, "i1")}, "i1", "",
                               node->location);
        return;
    }
    ir::IRValue left = genExpr(node->left.get());
    ir::IRValue right = genExpr(node->right.get());
    // ---- 阶段3 OOP（Task 3.7）：运算符重载降级为成员方法调用 ----
    // 左操作数为类实例且类有 运算符X 成员（+ - * / % == != < > <= >=）时，
    //   this=左操作数指针，实参=右操作数，Call 类名$运算符X#参数串（非虚）。
    // 注意：须在字符串连接/指针算术分支之前（语义层重载决议优先于隐式转换）。
    if (handleOperatorOverload(node, left, right)) {
        return;
    }
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
    // ---- 字符串 + 数值 隐式拼接（Task 2.9）：左为字符串(ptr)，右为数值/布尔/字符/枚举 ----
    // 展开：右操作数先转字符串（__cn_str_from_int/float/char/bool），再 __cn_str_concat。
    //   整数/枚举 -> __cn_str_from_int（整32 先 Cast i64；整128 截断 i64——值域≤2^63 语义正确）
    //   浮点      -> __cn_str_from_float（f32 先 Cast f64）
    //   字符      -> __cn_str_from_char（i32 值）
    //   布尔      -> __cn_str_from_bool（i1 -> "真"/"假"）
    // 多操作数左结合："a" + 1 + 2 = ("a"+1)+2：内层结果为 ptr（连接产物），外层再拼。
    //   连接/转换产物为动态内存，调用方负责 字符串释放（与 Task 2.5 语义一致）。
    // 注意：Call 有副作用，CSE 已排除（cse.cpp isSideEffect），不会被错误合并。
    // 关键区分：ptr 左操作数可能是 字符串（拼接）或 普通指针（指针算术 q + 1）。
    //   拼接仅当 左操作数源码类型为 字符串/字符*（字面量 或 字符串变量/数组元素）；
    //   普通指针 + 整数 必须走下方指针算术分支（05_array_pointer 回归教训）。
    const bool leftIsString = isStringTypedExpr(node->left.get());
    if (leftIsString && left.type == "ptr" && node->op == Operator::Add &&
        right.type != "ptr" && right.type != "i128" && right.type != "u128") {
        ir::IRValue strArg = right;
        std::string convFn;
        if (right.type == "f32" || right.type == "f64") {
            if (strArg.type != "f64") {
                strArg = emitResult(ir::Opcode::Cast, {strArg}, "f64", "",
                                    node->location);
            }
            convFn = "__cn_str_from_float";
        } else if (right.type == "i1") {
            convFn = "__cn_str_from_bool";  // 布尔转 "真"/"假"
        } else if (right.type == "i32" || right.type == "u32") {
            // 字符字面量/字符变量（IR i32）走字符转换；整32/枚举 走整数转换。
            // 区分依据：右操作数 AST 形态（CharLiteral 直判）+ 变量源码类型查表
            bool isChar = false;
            if (node->right->getType() == NodeType::CharLiteral) {
                isChar = true;
            } else if (node->right->getType() == NodeType::IdentifierExpr) {
                const std::string st = types::canonical(lookupSrcType(
                    static_cast<IdentifierExpr*>(node->right.get())->name));
                isChar = (st == "字符");
            }
            if (isChar) {
                convFn = "__cn_str_from_char";
            } else {
                if (strArg.type != "i64") {
                    strArg = emitResult(ir::Opcode::Cast, {strArg}, "i64", "",
                                        node->location);
                }
                convFn = "__cn_str_from_int";
            }
        } else {
            // 整数（i64/u64/i8/i16 等）：缺陷修复——无符号（正8~正64）走
            //   __cn_str_from_uint（%llu 语义，值超 2^63 正确显示正数），
            //   有符号统一转 i64 走 __cn_str_from_int（%lld）
            const bool isUnsignedNum = (strArg.type == "u8" ||
                                        strArg.type == "u16" ||
                                        strArg.type == "u32" ||
                                        strArg.type == "u64");
            if (isUnsignedNum) {
                if (strArg.type != "u64") {
                    strArg = emitResult(ir::Opcode::Cast, {strArg}, "u64", "",
                                        node->location);
                }
                convFn = "__cn_str_from_uint";
            } else {
                if (strArg.type != "i64") {
                    strArg = emitResult(ir::Opcode::Cast, {strArg}, "i64", "",
                                        node->location);
                }
                convFn = "__cn_str_from_int";
            }
        }
        ir::IRValue rightStr = emitResult(ir::Opcode::Call, {strArg}, "ptr",
                                          convFn, node->location);
        lastExpr_ = emitResult(ir::Opcode::Call, {left, rightStr}, "ptr",
                               "__cn_str_concat", node->location);
        return;
    }
    // 字符串 + 整128/正128（IR 双槽 ptr 形态）：转字符串（截断 i64）再连接
    // i128 值在 IR 层为"指向16字节双槽内存的 ptr"（低64位槽+高64位槽），
    // right 本身就是该地址（变量槽地址或临时双槽地址）。取低64位 LoadPtr 转整64
    //（值域≤2^63 语义正确；超范围拼接场景后续 Task 再支持全量转换）。
    if (leftIsString && left.type == "ptr" && node->op == Operator::Add &&
        (right.type == "i128" || right.type == "u128")) {
        // i128 值是"双槽寄存器值"（IRValue.id=高64位槽、id+1=低64位槽），
        // 而非指针地址——不能用 LoadPtr 解引用（会把槽值当地址访问导致
        // 0xC0000005）。正确做法：Cast i128 -> i64 取低64位（emitCast 已有
        // i128/u128->i64 分支，值域≤2^63 语义正确）。
        ir::IRValue lo = emitResult(ir::Opcode::Cast, {right}, "i64", "",
                                    node->location);
        ir::IRValue rightStr = emitResult(ir::Opcode::Call, {lo}, "ptr",
                                          "__cn_str_from_int", node->location);
        lastExpr_ = emitResult(ir::Opcode::Call, {left, rightStr}, "ptr",
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
                                (left.type == "u32" && right.type == "i32") ||
                                (left.type == "i32" && right.type == "i8") ||
                                (left.type == "i32" && right.type == "i16") ||
                                (left.type == "u32" && right.type == "i8") ||
                                (left.type == "u32" && right.type == "i16"));
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
void IRGenerator::visitUnaryExpr(UnaryExpr* node) {
    ir::IRValue operand = genExpr(node->operand.get());
    // P2-14：单目运算符重载（- ! ~）——类实例且类有 运算符X（0 参数）成员时
    //   降级为成员方法调用（this=操作数指针，无右实参）；须在内置一元运算之前。
    if (handleUnaryOperatorOverload(node, operand)) {
        return;
    }
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
        case Operator::Propagate: {
            // C-1（错误传播运算符，2026-08）：表达式? ——操作数地址 = 结果/可选
            //   合成结构体（{i1 正常, pad, 联合体{值, 错误}}，正常标志 @0、值/错误 @8）。
            //   正常 -> 取 .值 继续；失败 -> 构造 结果{正常=false, 错误=E} 并返回
            //   （Rust ? 语义；node->propagateType 由语义层回填）
            ir::IRValue propOperand = genExpr(node->operand.get());
            const std::string ptype = node->propagateType;  // 函数返回类型（写侧）
            // 宿主缺陷根治（2026-08-25）：propagateType 现为当前函数返回类型
            //   （语义层回填 currentReturnType_）。写侧（失败临时/返回）用返回
            //   类型，读侧（操作数 .值/.错误）用操作数类型——宽错误码传播
            //   （操作数 结果<整32,整32>、函数返回 结果<整32,整64>）两者不同。
            std::string retValSrc;
            std::string retErrSrc;
            if (SemanticAnalyzer::isResultType(ptype)) {
                const std::vector<std::string> args = SemanticAnalyzer::resultTypeArgs(ptype);
                if (args.size() == 2) {
                    retValSrc = args[0];
                    retErrSrc = args[1];
                }
            } else if (SemanticAnalyzer::isOptionalType(ptype)) {
                retValSrc = SemanticAnalyzer::optionalTypeArg(ptype);
                retErrSrc = "";  // 可选：失败 = 无值（结构体无错误字段）
            }
            if (retValSrc.empty()) {
                lastExpr_ = propOperand;
                break;
            }
            // 读侧类型：操作数自身类型（值/错误读取宽度）；未知时回退返回类型
            const std::string operandType = exprSrcType(node->operand.get());
            std::string valSrc = retValSrc;
            std::string errSrc = retErrSrc;
            if (SemanticAnalyzer::isResultType(operandType)) {
                const std::vector<std::string> oargs =
                    SemanticAnalyzer::resultTypeArgs(operandType);
                if (oargs.size() == 2) { valSrc = oargs[0]; errSrc = oargs[1]; }
            } else if (SemanticAnalyzer::isOptionalType(operandType)) {
                valSrc = SemanticAnalyzer::optionalTypeArg(operandType);
            }
            // 合成结构体名（结果$T$E / 可选$T，按返回类型），失败分支分配临时槽
            std::string structName;
            if (SemanticAnalyzer::isResultType(ptype)) {
                structName = SemanticAnalyzer::resultStructName(
                    types::canonical(retValSrc), types::canonical(retErrSrc));
            } else {
                structName = SemanticAnalyzer::optionalStructName(
                    types::canonical(retValSrc));
            }
            // 正常标志 @ 偏移 0；值/错误 @ 真实布局偏移（宿主缺陷根治 2026-08-25：
            //   整64/结构体 联合体偏移 8、整32 偏移 4——原硬编码 8 使 结果<整32,整32>
            //   的 ? 运算符读/写错位，正常传播读垃圾（-889198960 实测））
            int valueOff = 8;
            const StructDecl* pdecl =
                (semantic_ != nullptr) ? semantic_->findStruct(structName) : nullptr;
            if (pdecl != nullptr) {
                for (const auto& pf : pdecl->fields) {
                    const bool want = SemanticAnalyzer::isResultType(ptype)
                                          ? (pf.name == "错误值联合")
                                          : (pf.name == "值");
                    if (want) {
                        valueOff = semantic_->fieldOffsetOf(pdecl, pf.name);
                        break;
                    }
                }
            }
            if (valueOff < 0) valueOff = 8;
            // 宿主缺陷根治（2026-08-25）：读偏移按操作数自身类型（内层 结果<整32,整32>
            //   联合体偏移 4）、写偏移按传播目标类型 ptype（外层 结果<整32,整64> 偏移 8）
            //   ——原统一用 ptype 偏移，宽错误码传播（E 整32->整64）读内层错位
            //   （错误码 1 读成 4294967296 实测）。operandType 已在类型解析处声明。
            int readOff = valueOff;  // 默认：传播目标偏移（操作数类型未知时回退）
            if (!operandType.empty() && semantic_ != nullptr) {
                std::string opStruct;
                if (SemanticAnalyzer::isResultType(operandType)) {
                    const std::vector<std::string> oargs =
                        SemanticAnalyzer::resultTypeArgs(operandType);
                    if (oargs.size() == 2) {
                        opStruct = SemanticAnalyzer::resultStructName(
                            types::canonical(oargs[0]), types::canonical(oargs[1]));
                    }
                } else if (SemanticAnalyzer::isOptionalType(operandType)) {
                    opStruct = SemanticAnalyzer::optionalStructName(
                        types::canonical(SemanticAnalyzer::optionalTypeArg(operandType)));
                }
                const StructDecl* opdecl =
                    (!opStruct.empty()) ? semantic_->findStruct(opStruct) : nullptr;
                if (opdecl != nullptr) {
                    for (const auto& pf : opdecl->fields) {
                        const bool want = SemanticAnalyzer::isResultType(operandType)
                                              ? (pf.name == "错误值联合")
                                              : (pf.name == "值");
                        if (want) {
                            readOff = semantic_->fieldOffsetOf(opdecl, pf.name);
                            break;
                        }
                    }
                }
            }
            if (readOff < 0) readOff = 8;
            ir::IRValue flagAddr = emitResult(ir::Opcode::FieldAddr, {propOperand}, "ptr",
                                              "0", node->location);
            ir::IRValue flag = emitResult(ir::Opcode::LoadPtr, {flagAddr}, "i1", "",
                                          node->location);
            ir::IRValue valAddr = emitResult(ir::Opcode::FieldAddr, {propOperand}, "ptr",
                                             std::to_string(readOff), node->location);
            const std::string errLabel = "bb" + std::to_string(blockCounter_);
            const std::string okLabel = "bb" + std::to_string(blockCounter_ + 1);
            endBranch(flag.toString(), okLabel, errLabel);  // 真->ok 假->err
            // 失败块：构造 结果{正常=false, 错误=E} 临时结构体并返回
            setCurrentBlock(newBlock(errLabel));
            const std::string tempName = "?prop$" + std::to_string(varCounter_++);
            ir::IRValue temp = newReg();
            temp.type = "ptr";
            emit(ir::Opcode::Alloca, {}, temp, tempName, "ptr", node->location);
            // 按结果/可选合成结构体实际大小登记槽（ensureLoweredType 防御：
            //   泛型实例化方法体内首次出现的类型可能在 lower 后才注册）
            if (semantic_ != nullptr && semantic_->findStruct(structName) == nullptr) {
                semantic_->ensureLoweredType(ptype);
            }
            registerVarSlots(tempName, structName);
            ir::IRValue tempAddr = emitResult(ir::Opcode::AddrOf,
                                              {ir::IRValue::var(tempName, "i64")},
                                              "ptr", tempName, node->location);
            ir::IRValue falseV = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                            node->location);
            emit(ir::Opcode::StorePtr, {tempAddr, falseV}, ir::IRValue(), "", "i32",
                 node->location);
            if (SemanticAnalyzer::isResultType(ptype) && !errSrc.empty()) {
                // 宿主缺陷根治（2026-08-25）：读操作数错误值按操作数错误类型宽度
                //   （宽错误码传播 E 整32 读 i32），写返回临时按返回类型错误宽度
                //   （E 整64 写 i64，Cast 符号扩展）——原统一返回类型导致 i32 错误
                //   读成 i64 高位垃圾（错误码 1 变 4294967296 实测）。
                const std::string errIr = mapType(errSrc);        // 读：操作数错误宽度
                ir::IRValue errVal = emitResult(ir::Opcode::LoadPtr, {valAddr}, errIr,
                                                "", node->location);
                std::string storeType = mapType(retErrSrc);        // 写：返回错误宽度
                if (storeType == "i1") storeType = "i8";
                if (errVal.type != storeType && !storeType.empty()) {
                    errVal = emitResult(ir::Opcode::Cast, {errVal}, storeType, "",
                                        node->location);
                }
                ir::IRValue errField = emitResult(ir::Opcode::FieldAddr, {tempAddr},
                                                  "ptr", std::to_string(valueOff),
                                                  node->location);
                emit(ir::Opcode::StorePtr, {errField, errVal}, ir::IRValue(), "",
                     storeType, node->location);
            }
            endReturn(tempAddr.toString());
            // 正常块：取 .值（联合体 @8）
            setCurrentBlock(newBlock(okLabel));
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {valAddr}, mapType(valSrc), "",
                                   node->location);
            break;
        }
        case Operator::Increment:
        case Operator::Decrement: {
            // 缺陷5 修复：类字段（静态/实例）自增自减——字段名不在 varStack_，
            //   须走"读-算-写回"专用路径（原实现只读不写，静态字段 总数++ 恒 0）
            if (node->operand->getType() == NodeType::IdentifierExpr &&
                handleClassFieldIncDec(
                    static_cast<IdentifierExpr*>(node->operand.get()),
                    node->op, node->location)) {
                break;
            }
            // 第 9 层 Debug（P3-8）：顶层静态变量自增/自减——全局符号"读-算-写回"。
            if (node->operand->getType() == NodeType::IdentifierExpr &&
                semantic_ != nullptr) {
                IdentifierExpr* gsIdent = static_cast<IdentifierExpr*>(node->operand.get());
                if (semantic_->isGlobalStatic(gsIdent->name)) {
                    const std::string stType = semantic_->globalStaticType(gsIdent->name);
                    const std::string irT = mapType(stType.empty() ? "整64" : stType);
                    ir::IRValue gsAddr = emitResult(
                        ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + gsIdent->name,
                        node->location);
                    ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {gsAddr}, irT, "",
                                                 node->location);
                    ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, "i64", "1",
                                                   node->location);
                    ir::IRValue res = emitResult(
                        node->op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                        {cur, delta}, irT, "", node->location);
                    emit(ir::Opcode::StorePtr, {gsAddr, res}, ir::IRValue(), "", irT,
                         node->location);
                    lastExpr_ = res;
                    break;
                }
            }
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
                    // 缺陷修复（[&] 引用捕获自增自减）：参数槽存被捕获变量地址，
                    //   写回经 StorePtr（更新外部变量）；值捕获走本地 Store
                    if (isByRefCapture(ident->name)) {
                        const std::string unique = lookupVarName(ident->name);
                        ir::IRValue capAddr = emitResult(
                            ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")},
                            "ptr", unique, node->location);
                        emit(ir::Opcode::StorePtr, {capAddr, result},
                             ir::IRValue(), "", "ptr", node->location);
                    } else {
                        emit(ir::Opcode::Store, {result}, ir::IRValue(),
                             lookupVarName(ident->name), "ptr", node->location);
                    }
                }
            }
            // 缺陷A根治（2026-09-03，单位机 ARM64 探针发现、win-x64 同现=IR 公共层
            //   缺陷非单后端）：成员/下标/解引用目标的写回——原实现仅标识符
            //   （+类字段/顶层静态专用路径）写回，语义层已放行的成员/下标/解引用
            //   左值（p.y-- / 数组[1]++ / (*p)++）只读不写=存储静默丢弃。读-算-写回
            //   与赋值语句同款（lvalueAddress 左值地址）；宽度按操作数类型
            //   （②b B5：字段宽度禁默认 8 字节，窄整型按元素宽度写）
            if (node->operand->getType() == NodeType::MemberExpr ||
                node->operand->getType() == NodeType::IndexExpr ||
                (node->operand->getType() == NodeType::UnaryExpr &&
                 static_cast<UnaryExpr*>(node->operand.get())->op ==
                     Operator::Deref)) {
                ir::IRValue addr = lvalueAddress(node->operand.get());
                ir::IRValue incdecResult = emitResult(
                    node->op == Operator::Increment ? ir::Opcode::Add
                                                    : ir::Opcode::Sub,
                    {operand, delta}, operand.type, "", node->location);
                emit(ir::Opcode::StorePtr, {addr, incdecResult}, ir::IRValue(), "",
                     operand.type, node->location);
                lastExpr_ = incdecResult;
                break;
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
                // 缺陷修复（[&] 引用捕获 &变量）：参数槽存被捕获变量地址，
                //   &捕获变量 = Load 参数槽（取被捕获变量地址，而非参数槽自身地址）
                if (isByRefCapture(
                        static_cast<IdentifierExpr*>(node->operand.get())->name)) {
                    lastExpr_ = emitResult(ir::Opcode::Load,
                                           {ir::IRValue::var(unique, "ptr")},
                                           "ptr", unique, node->location);
                } else {
                    // 变量槽地址：AddrOf 指令（codegen 生成 lea）
                    lastExpr_ = emitResult(ir::Opcode::AddrOf,
                                           {ir::IRValue::var(unique, operand.type)},
                                           "ptr", unique, node->location);
                }
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
            } else if (node->operand->getType() == NodeType::IdentifierExpr &&
                       semantic_ != nullptr &&
                       semantic_->isGlobalStatic(
                           static_cast<IdentifierExpr*>(node->operand.get())->name)) {
                // plans/018 根治（2026-09-07，缺陷零容忍）：顶层静态变量取地址——
                //   ?gstatic_名 符号地址即变量槽地址（.data 槽本身=存储位置），直接
                //   以符号地址为取地址结果。原落 else 读值兜底（genExpr→LoadPtr 读
                //   槽内容）：引用形参实参经语义层 wrapRefArgs 包装为 &静态 后，此处
                //   取地址退化成读值——引用形参收到的是槽内容（容器=句柄值），被调
                //   方对引用形参的读取=再解引用一次（空容器句柄 0=错误码3；非空=
                //   读写容器对象头=静默错位）。v2self 语义分析多文件(6 个 & 静态
                //   容器实参) 首次踩中。Rust 对照：&static mut 的引用恒指向存储
                //   位置本身，绝不解引用重解释。
                lastExpr_ = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                       "?gstatic_" + static_cast<IdentifierExpr*>(
                                           node->operand.get())->name,
                                       node->location);
            } else {
                // 其他左值：直接使用其地址值（表达式本身是地址）
                lastExpr_ = genExpr(node->operand.get());
            }
            break;
        }
        case Operator::Deref: {
            // 解引用 *p（Task 2.4）：LoadPtr(指针值) 从指针地址加载元素
            // 元素IR类型：从操作数源码指针类型推断（整32* -> i32、浮64* -> f64）
            // 审查修复：*（p+1）等复合指针表达式原来落默认 i64 读满 8 字节，
            //   把数组相邻 4 字节栈残留垃圾一并读入（指针算术结果错误）。
            //   统一经 pointerPointeeSrcType 递归推导所指元素类型。
            std::string elemType = "i64";  // 默认按64位（指针所指值存8字节槽）
            const std::string pointeeSrc = pointerPointeeSrcType(node->operand.get());
            if (!pointeeSrc.empty()) {
                elemType = mapType(pointeeSrc);
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
void IRGenerator::visitTernaryExpr(TernaryExpr* node) {
    ir::IRValue cond = genExpr(node->condition.get());
    // 三元结果类型：IR 层类型（i64/f64/ptr/i1 等）。语义层已保证两分支类型统一
    //   （数值已宽化合并；字符串/指针/结构体一致），此处取真分支 IR 类型作为槽类型。
    // 惰性求值无法在求值前得知真值 IR 类型（真值在真块内才生成），故先分配临时槽，
    //   槽 IR 类型用"条件为真时的类型"——语义层已保证真/假分支 IR 类型一致（数值宽化
    //   由两分支各自 Cast 到公共类型，见下方 Cast 对齐）。
    // 临时槽：allocVar 分配唯一变量（Alloca + varSlots 登记），分支内 Store、汇合块 Load。
    // 注意：三元结果可能为 结构体/指针/字符串（ptr）——槽按 1 个 8 字节槽登记即可
    //   （ptr 统一 8 字节；结构体场景三元不常用，语义层已限两分支一致，此处按 ptr 处理）。
    std::string slotType = "i64";
    {
        // 先求真值到临时（惰性：真块内才求值）。为获得槽类型，这里不直接求值，
        // 而是通过语义层结果类型映射：三元结果的源码类型 = 语义层已推导（lastType_ 由
        // 语义分析器设置，但 IR 生成器无法直接读 lastType_）。改为：两分支各自
        // genExpr 后按"分支类型"统一——字符串/指针分支类型是 ptr，数值分支是 i64/f64。
        // 简便且正确：分配 ptr 槽 + 真/假值 Cast 到 ptr？不行——数值需要 8 字节槽。
        // 正确方案：真块/假块各 Store 各分支值（各自 IR 类型），汇合块 Load 时用
        //   真分支的 IR 类型（语义层保证两分支 IR 类型一致，数值已宽化合并）。
        // 故这里临时分配"通用 8 字节槽"，Store 时按分支实际类型写入（codegen 对 Store
        //   的 type 字段决定宽度），Load 时按真分支类型读。i1 布尔用 1 槽（8 字节，
        //   codegen i1 Load 读低字节）。
    }
    // 预分配临时槽（唯一内部名 __ternary$N）
    std::string tempName = "__ternary$" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "i64"),
         tempName, "i64", node->location);
    function_->varSlots[tempName] = 1;

    // 标签"随建随取"：先取 3 个标签（newBlock 内部自增计数，参照 genIf）
    std::string trueLabel = "bb" + std::to_string(blockCounter_++);
    std::string falseLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 条件跳转（条件寄存器挂在块最后指令操作数上，codegen 读取）
    endBranch(cond.toString(), trueLabel, falseLabel);

    // 真分支：求真值 -> Store 临时槽 -> 跳汇合
    setCurrentBlock(newBlock(trueLabel));
    ir::IRValue trueVal = genExpr(node->trueValue.get());
    emit(ir::Opcode::Store, {trueVal}, ir::IRValue(), tempName, trueVal.type,
         node->location);
    ir::IRValue trueTypeForLoad = trueVal;  // 记录真分支 IR 类型（汇合块 Load 用）
    if (!currentBlock_->terminated) endJump(endLabel);

    // 假分支：求假值 -> Store 临时槽 -> 跳汇合
    setCurrentBlock(newBlock(falseLabel));
    ir::IRValue falseVal = genExpr(node->falseValue.get());
    // 两分支类型对齐（数值宽化合并：真整64/假整32 -> 假 Cast 到整64；整/浮 -> 浮64）
    if (falseVal.type != trueTypeForLoad.type && !trueTypeForLoad.type.empty()) {
        // 仅当类型不同且目标非 void 时 Cast（数值宽化；字符串/指针均为 ptr 天然一致）
        falseVal = emitResult(ir::Opcode::Cast, {falseVal}, trueTypeForLoad.type, "",
                              node->location);
    }
    emit(ir::Opcode::Store, {falseVal}, ir::IRValue(), tempName, falseVal.type,
         node->location);
    if (!currentBlock_->terminated) endJump(endLabel);

    // 汇合块：Load 临时槽 作为表达式结果（类型用真分支类型，语义层已保证一致）
    setCurrentBlock(newBlock(endLabel));
    lastExpr_ = emitResult(ir::Opcode::Load, {ir::IRValue::var(tempName, "i64")},
                           trueTypeForLoad.type, "", node->location);
}

// 结构体/类整体赋值发射（46-a 根治 2026-09-09 提取的单一事实源——原机制内联
//   于 visitAssignmentExpr 下标目标分支（集成验证修复+H8 类补完），成员目标
//   分支（r.左上 = a）无此通道：genExpr(结构体标识符源) 落标量值 + StorePtr
//   只写 8 字节（源地址截断入首字段、其余字段栈残留垃圾），探针静默数据损坏
//   实锤（11/22 打出 12582296/128）。Rust place 拷贝同构：结构体赋值=整体
//   按值拷贝（POD Copy）。
bool IRGenerator::emitStructWholeAssign(const ir::IRValue& dstAddr,
                                        Expr* valueNode,
                                        const std::string& dstElemCanon,
                                        const SourceLocation& loc) {
    if (semantic_ == nullptr) return false;
    if (!(semantic_->isStructType(dstElemCanon) ||
          semantic_->isClassType(dstElemCanon))) {
        return false;
    }
    // 右值源地址三形态（与下标位既有机制逐点一致）：
    //   IndexExpr/MemberExpr -> 元素/字段内联地址（lvalueAddress）；
    //   IdentifierExpr -> 结构体 AddrOf 槽 / 类 Load 槽（槽存对象指针，
    //   AddrOf 槽会取到指向指针的指针——CopyStruct 读到指针值+栈垃圾）。
    ir::IRValue srcAddr;
    std::string srcUniqueId;  // 标识符源唯一名（拷贝构造 byRef 传参用）
    if (valueNode->getType() == NodeType::IndexExpr ||
        valueNode->getType() == NodeType::MemberExpr) {
        srcAddr = lvalueAddress(valueNode);
    } else if (valueNode->getType() == NodeType::IdentifierExpr) {
        const std::string srcName =
            static_cast<IdentifierExpr*>(valueNode)->name;
        srcUniqueId = lookupVarName(srcName);
        const std::string srcST = lookupSrcType(srcName);
        if (semantic_->isClassType(types::canonical(types::stripRef(srcST)))) {
            srcAddr = emitResult(ir::Opcode::Load,
                                 {ir::IRValue::var(srcUniqueId, "ptr")},
                                 "ptr", srcUniqueId, loc);
        } else {
            srcAddr = emitResult(ir::Opcode::AddrOf,
                                 {ir::IRValue::var(srcUniqueId, "i64")},
                                 "ptr", srcUniqueId, loc);
        }
    }
    if (srcAddr.id < 0) return false;
    // 类元素且有拷贝构造且源为类变量——调用拷贝构造深拷贝（this=内联元素
    //   地址，实参=&源槽 byRef 解引用得源对象），元素独立拥有内部数组
    //   （浅拷贝共享指针 + RAII 双释放 0xC0000374）。源为成员/下标元素
    //   （byRef 无法引用内联元素）保持 CopyStruct 浅拷贝。
    const ClassMemberInfo* copyCtor = semantic_->findCopyConstructor(dstElemCanon);
    if (copyCtor != nullptr && !srcUniqueId.empty()) {
        const std::string copyOwner =
            copyCtor->ownerClass.empty() ? dstElemCanon : copyCtor->ownerClass;
        const ir::IRValue srcRef = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(srcUniqueId, "i64")},
            "ptr", srcUniqueId, loc);
        emit(ir::Opcode::Call, {dstAddr, srcRef}, ir::IRValue(),
             methodSymbolKey(copyOwner, copyCtor->sigKey), "void", loc);
    } else {
        const int size = semantic_->typeSizeOf(dstElemCanon);
        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
             std::to_string(size), "void", loc);
    }
    return true;
}

void IRGenerator::visitAssignmentExpr(AssignmentExpr* node) {
    // 结构体字段左值：p.x = v / 指针->x = v（Task 2.7）
    // 通过 lvalueAddress 计算字段地址（FieldAddr），StorePtr 写入
    if (node->target->getType() == NodeType::MemberExpr) {
        MemberExpr* member = static_cast<MemberExpr*>(node->target.get());
        // ---- 阶段3 OOP（Task 3.1）：类字段赋值（对象.字段 = v / 类名.静态字段 = v） ----
        // 实例字段 -> 对象指针+偏移 StorePtr；静态字段 -> ?static_ 符号 StorePtr。
        if (handleClassMemberAssign(member, node->value.get(), node->location)) {
            return;
        }
        // 枚举成员赋值不合法（枚举值为只读常量；枚举类型名非指针，
        //   isDerefAccess 恒 false——v2.1 保留原防御结构）
        if (!member->isDerefAccess &&
            member->object->getType() == NodeType::IdentifierExpr) {
            IdentifierExpr* ident = static_cast<IdentifierExpr*>(member->object.get());
            std::int64_t v = 0;
            if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
                semantic_->enumValueOf(ident->name, member->memberName, v)) {
                lastExpr_ = genExpr(node->value.get());
                return;  // 语义层已报错（只读）
            }
        }
        // D2 根治（2026-09-09 第四十六轮）：构造字面量赋值（成员目标）——
        //   r.右下 = 点{ x = 11, y = 12 }（字段为结构体）同走 emitStructInitTo
        //   单点逐字段写（与下标位 Task 2.7 / 标识符位 / 声明位同构——Rust 目标
        //   place 语义，穷举纪律在语义层 visitStructInitExpr）。原无此分支：
        //   空桩常量0 落 StorePtr 只写 8 字节 -> 字段静默零填（E2E 183 探针
        //   实锤 11/12 打出 0/0）。目标地址 = lvalueAddress(成员左值)。
        if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
            node->value->getType() == NodeType::StructInitExpr) {
            StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
            // 对象所属结构体（memberObjStructType 递归+指针解引用剥离）-> 字段类型
            const std::string ownerStruct = memberObjStructType(member);
            const StructDecl* ownerDecl =
                semantic_->findStruct(types::canonical(ownerStruct));
            std::string fieldStruct;
            if (ownerDecl != nullptr) {
                for (const auto& f : ownerDecl->fields) {
                    if (f.name == member->memberName) {
                        fieldStruct = types::canonical(f.type);
                        break;
                    }
                }
            }
            if (!fieldStruct.empty() && semantic_->isStructType(fieldStruct)) {
                ir::IRValue fieldAddr = lvalueAddress(node->target.get());
                emitStructInitTo(init, fieldAddr, node->location);
                lastExpr_ = fieldAddr;   // 与整体赋值链式语义一致（值=目标地址）
                return;
            }
        }
        // 46-a 根治（2026-09-09 第四十八轮）：成员结构体作值写（r.左上 = a /
        //   r.右下 = r.左上）——字段为结构体/类时同走整体赋值单一助手
        //   emitStructWholeAssign（与下标位 H8 同构，Rust place 拷贝语义）。
        //   原无此通道：标量通用路径 StorePtr 只写 8 字节静默数据损坏。
        //   类型检查前置（与 D2 同型）：非结构体/类字段零冗余落回标量路径
        //   （lvalueAddress 有发射副作用，落回即双发）。
        if (semantic_ != nullptr && !isCompoundAssignOp(node->op)) {
            const std::string ownerStructW = memberObjStructType(member);
            const StructDecl* ownerDeclW =
                semantic_->findStruct(types::canonical(ownerStructW));
            std::string fieldStructW;
            if (ownerDeclW != nullptr) {
                for (const auto& f : ownerDeclW->fields) {
                    if (f.name == member->memberName) {
                        fieldStructW = types::canonical(f.type);
                        break;
                    }
                }
            }
            if (!fieldStructW.empty() &&
                (semantic_->isStructType(fieldStructW) ||
                 semantic_->isClassType(fieldStructW))) {
                ir::IRValue fieldAddrW = lvalueAddress(node->target.get());
                if (emitStructWholeAssign(fieldAddrW, node->value.get(),
                                          fieldStructW, node->location)) {
                    lastExpr_ = fieldAddrW;  // 值=目标地址（链式语义与 D2 一致）
                    return;
                }
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
        if (member->isDerefAccess && types::isPointer(objSrcType)) {
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
        // 72-a 根治（2026-09-11 第七十二轮）：结构体字段=x 目标字段类型为
        //   字符串 且右值为字符串变量 -> 值被外部槽持有（指针逃逸模型，与
        //   下标路径同款：数组元素借出=借用）。源变量污染退出 RAII——否则
        //   源出口释放 + 字段/后继容器浅共享 = 悬垂（v2 树 货舱解析 的
        //   `项.名称 = 键` → 依赖们.追加(项) 形态，E2E 165 回归实证）。
        if (targetType == "ptr" && !isCompoundAssignOp(node->op) &&
            node->value->getType() == NodeType::IdentifierExpr) {
            const std::string fieldFieldType = [&]() -> std::string {
                if (decl != nullptr) {
                    for (const auto& f : decl->fields) {
                        if (f.name == member->memberName) return f.type;
                    }
                }
                // 类实例字段（对象.字段，class 路径）
                if (semantic_ != nullptr && !objSrcType.empty() &&
                    semantic_->isClassType(types::canonical(objSrcType))) {
                    const std::string ft = classFieldType(
                        types::canonical(objSrcType), member->memberName);
                    if (!ft.empty()) return ft;
                }
                return "";
            }();
            if (types::canonical(fieldFieldType) == "字符串") {
                stringTainted_.insert(
                    static_cast<IdentifierExpr*>(node->value.get())->name);
            }
        }
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
        // plans/019 阶段4' 方案A：下标目标为字符串元素（字符串数组/字符串* 元素）
        //   且右值为字符串变量标识符 -> 值被外部槽持有（指针逃逸模型，38_tool
        //   姓名[]=$副本 形态）-> 右值变量污染退出 RAII（出口 free=悬垂源头）。
        {
            IndexExpr* idxX =
                node->target->getType() == NodeType::IndexExpr
                    ? static_cast<IndexExpr*>(node->target.get()) : nullptr;
            if (idxX != nullptr &&
                idxX->object->getType() == NodeType::IdentifierExpr) {
                const std::string stX = lookupSrcType(
                    static_cast<IdentifierExpr*>(idxX->object.get())->name);
                std::string elemX;
                if (types::isArray(stX)) elemX = types::arrayElemOf(stX);
                else if (types::isPointer(stX)) elemX = types::pointeeOf(stX);
                if (elemX == "字符串" &&
                    node->value->getType() == NodeType::IdentifierExpr) {
                    stringTainted_.insert(
                        static_cast<IdentifierExpr*>(node->value.get())->name);
                }
            }
        }
        // 目标类型：语义层已推导（整32 元素 / 解引用元素类型）
        std::string targetType = "i64";
        if (node->target->getType() == NodeType::IndexExpr) {
            IndexExpr* idx = static_cast<IndexExpr*>(node->target.get());
            if (idx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(st)) targetType = mapType(types::arrayElemOf(st));
                else if (types::isPointer(st)) targetType = mapType(types::pointeeOf(st));
                // A-3（2026-08）：隐式类字段对象（向量 数据[位置] = 值）——
                //   目标类型按字段所指元素类型推导（此前回退 i64 只存 8 字节）
                else if (st.empty() &&
                         isInstanceField(static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                    const std::string ft = classFieldType(
                        currentClass_,
                        static_cast<IdentifierExpr*>(idx->object.get())->name);
                    if (types::isArray(ft)) targetType = mapType(types::arrayElemOf(ft));
                    else if (types::isPointer(ft)) targetType = mapType(types::pointeeOf(ft));
                }
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
                // A-3（2026-08）：隐式类字段对象——结构体元素整体赋值（CopyStruct）
                else if (st.empty() &&
                         isInstanceField(static_cast<IdentifierExpr*>(tIdx->object.get())->name)) {
                    const std::string ft = classFieldType(
                        currentClass_,
                        static_cast<IdentifierExpr*>(tIdx->object.get())->name);
                    if (types::isArray(ft)) tElemSrc = types::arrayElemOf(ft);
                    else if (types::isPointer(ft)) tElemSrc = types::pointeeOf(ft);
                }
            }
            const std::string tElemCanon = types::canonical(tElemSrc);
            // H8 补完（2026-08-25）：类类型元素整体赋值（向量<映射<...>> 追加
            //   的 数据[元素数量] = 值）同样走 CopyStruct（56 字节）——原只处理
            //   结构体，类元素落 StorePtr 只写 8 字节指针 -> 元素数据破坏。
            //   46-a 根治（2026-09-09）：机制提取为 emitStructWholeAssign 单一
            //   事实源（成员目标分支同构复用），本分支行为逐点不变。
            if (!tElemSrc.empty() &&
                (semantic_->isStructType(tElemCanon) ||
                 semantic_->isClassType(tElemCanon)) &&
                !isCompoundAssignOp(node->op)) {
                if (emitStructWholeAssign(addr, node->value.get(),
                                          tElemCanon, node->location)) {
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
    // P3-18 补完（2026-08）：引用返回调用作赋值目标（获取() = 值 / 获取() += 值）——
    //   目标调用结果即被引用左值地址（ptr），StorePtr 写回。
    //   2026-09-04 缺陷零容忍收口：目标生成须抑制读值解引用（suppressRefDeref_——
    //   visitCallExpr 引用返回默认 lvalue-to-rvalue；赋值目标要地址不要值）。
    if (node->target->getType() == NodeType::CallExpr) {
        const bool oldSuppress = suppressRefDeref_;
        suppressRefDeref_ = true;
        ir::IRValue tgtAddr = genExpr(node->target.get());
        suppressRefDeref_ = oldSuppress;
        ir::IRValue val = genExpr(node->value.get());
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {tgtAddr}, val.type,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                val = emitResult(opcode, {current, val}, val.type, "", node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {tgtAddr, val}, ir::IRValue(), "", val.type,
             node->location);
        lastExpr_ = val;
        return;
    }
    // 标识符左值（原有路径）：Store
    if (node->target->getType() != NodeType::IdentifierExpr) {
        lastExpr_ = genExpr(node->value.get());
        return;
    }
    IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
    // ---- 阶段3 OOP（Task 3.1）：方法体内直接字段赋值（无 自身. 前缀） ----
    // 字段名 不在当前方法作用域但命中类字段表 -> this+偏移 StorePtr。
    if (handleClassFieldAssign(ident, node->value.get(), node->location)) {
        return;
    }
    // 第 9 层 Debug（P3-8）：顶层静态变量赋值——全局 .data 符号 StorePtr。
    if (semantic_ != nullptr && semantic_->isGlobalStatic(ident->name)) {
        const std::string stType = semantic_->globalStaticType(ident->name);
        const std::string irT = mapType(stType.empty() ? "整64" : stType);
        ir::IRValue value = genExpr(node->value.get());
        // 宿主根治（2026-09-01）：类静态赋值深拷贝——右值为类对象标识符时
        //   NewObject + 拷贝构造/CopyStruct（与局部类赋值一致），防共享指针
        //   悬挂（源 RAII 析构后静态槽残留 freed 指针 -> 堆损坏）；右值为构造
        //   调用/临时对象（表 = 向量<...>()）本就是新对象，直接指针入槽。
        if (!isCompoundAssignOp(node->op) &&
            semantic_->isClassType(types::canonical(stType)) &&
            node->value->getType() == NodeType::IdentifierExpr) {
            const std::string rhsName =
                static_cast<IdentifierExpr*>(node->value.get())->name;
            std::string rhsSrc = lookupSrcType(rhsName);
            const bool rhsIsStatic =
                rhsSrc.empty() && semantic_->isGlobalStatic(rhsName);
            if (rhsIsStatic) rhsSrc = semantic_->globalStaticType(rhsName);
            if (semantic_->isClassType(types::canonical(rhsSrc))) {
                const std::string canonTgt = types::canonical(stType);
                const ClassInfo* ci = semantic_->findClass(canonTgt);
                if (ci != nullptr) {
                    const std::string extra = canonTgt + "|" +
                                              std::to_string(ci->totalSize);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(canonTgt, "ptr")},
                        "ptr", extra, node->location);
                    const ClassMemberInfo* copyCtor =
                        semantic_->findCopyConstructor(canonTgt);
                    bool copied = false;
                    if (copyCtor != nullptr) {
                        const std::string copyOwner = copyCtor->ownerClass.empty()
                                                          ? canonTgt
                                                          : copyCtor->ownerClass;
                        ir::IRValue srcAddr;
                        if (rhsIsStatic) {
                            // 源为顶层类静态：槽地址 = ?gstatic_名 符号地址
                            srcAddr = emitResult(
                                ir::Opcode::ConstString, {}, "ptr",
                                "?gstatic_" + rhsName, node->location);
                        } else {
                            const std::string srcUnique = lookupVarName(rhsName);
                            if (!srcUnique.empty()) {
                                if (isByRefCapture(rhsName)) {
                                    srcAddr = emitResult(
                                        ir::Opcode::Load,
                                        {ir::IRValue::var(srcUnique, "ptr")},
                                        "ptr", srcUnique, node->location);
                                } else {
                                    srcAddr = emitResult(
                                        ir::Opcode::AddrOf,
                                        {ir::IRValue::var(srcUnique, "i64")},
                                        "ptr", srcUnique, node->location);
                                }
                            }
                        }
                        if (srcAddr.id >= 0) {
                            emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
                                 methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                                 node->location);
                            copied = true;
                        }
                    }
                    if (!copied) {
                        emit(ir::Opcode::CopyStruct, {newObj, value}, ir::IRValue(),
                             std::to_string(ci->totalSize), "void", node->location);
                    }
                    ir::IRValue slotAddr = emitResult(
                        ir::Opcode::ConstString, {}, "ptr",
                        "?gstatic_" + ident->name, node->location);
                    emit(ir::Opcode::StorePtr, {slotAddr, newObj}, ir::IRValue(),
                         "", "ptr", node->location);
                    lastExpr_ = newObj;
                    return;
                }
            }
        }
        // 复合赋值（+= 等）：先读后算再写
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue addrR = emitResult(
                ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + ident->name, node->location);
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addrR}, irT, "",
                                             node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                value = emitResult(opcode, {current, value}, irT, "", node->location);
            }
        }
        ir::IRValue addr = emitResult(
            ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + ident->name, node->location);
        emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", irT, node->location);
        lastExpr_ = value;
        return;
    }
    // 目标变量唯一内部名（后续 普通赋值/类深拷贝 均需，提前计算避免重复查找）
    const std::string unique = lookupVarName(ident->name);
    // ---- 阶段3 OOP（缺陷1 修复）：类对象赋值深拷贝 ----
    // 类对象是堆指针语义，`乙 = 甲` 若直接 Store 指针会导致两个变量槽共享
    // 同一堆地址，RAII（函数返回 DeleteObject）对两个变量重复释放 -> 堆损坏。
    // 正确语义：新建独立堆对象 + 逐字节拷贝字段（C++ 拷贝语义，含虚表指针，
    //   同类虚表指针相同，覆盖后语义不变）。
    if (semantic_ != nullptr &&
        node->value->getType() == NodeType::IdentifierExpr) {
        const std::string targetSrcType = lookupSrcType(ident->name);
        const std::string canonTarget = types::canonical(targetSrcType);
        const std::string valueSrcType = lookupSrcType(
            static_cast<IdentifierExpr*>(node->value.get())->name);
        const std::string canonValue = types::canonical(valueSrcType);
        if (semantic_->isClassType(canonTarget) &&
            semantic_->isClassType(canonValue)) {
            const ClassInfo* ci = semantic_->findClass(canonTarget);
            if (ci != nullptr && !isCompoundAssignOp(node->op)) {
                // 源对象指针 = Load 源变量槽（类变量槽存对象指针）
                ir::IRValue srcObj = genExpr(node->value.get());
                // 新建独立对象（extra=类名|大小，codegen 初始化虚表指针）
                const std::string extra = canonTarget + "|" +
                                          std::to_string(ci->totalSize);
                ir::IRValue newObj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(canonTarget, "ptr")},
                    "ptr", extra, node->location);
                // 方案A（2026-08-25）：目标类有拷贝构造（类名(类名& 其他)）时，
                //   赋值拷贝改调拷贝构造（深拷贝），而非 CopyStruct 浅拷贝
                //   （含裸指针字段浅拷贝析构双释放 0xC0000374）。与 genVarDecl
                //   初始化路径、结构体赋值路径一致；byRef ABI：源为标识符变量
                //   时传 源变量槽地址（&甲），体内经 byRef 解引用得源对象指针。
                const ClassMemberInfo* copyCtor =
                    semantic_->findCopyConstructor(canonTarget);
                if (copyCtor != nullptr) {
                    const std::string copyOwner = copyCtor->ownerClass.empty()
                                                      ? canonTarget
                                                      : copyCtor->ownerClass;
                    const std::string valueName =
                        static_cast<IdentifierExpr*>(node->value.get())->name;
                    const std::string srcUnique = lookupVarName(valueName);
                    ir::IRValue srcAddr;
                    if (isByRefCapture(valueName)) {
                        // 源为引用参数：槽内存被引用对象地址（Load 槽）
                        srcAddr = emitResult(
                            ir::Opcode::Load,
                            {ir::IRValue::var(srcUnique, "ptr")},
                            "ptr", srcUnique, node->location);
                    } else {
                        srcAddr = emitResult(ir::Opcode::AddrOf,
                                             {ir::IRValue::var(srcUnique, "i64")},
                                             "ptr", srcUnique, node->location);
                    }
                    emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
                         methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                         node->location);
                } else {
                    // 逐字节拷贝字段到新对象（深拷贝）
                    emit(ir::Opcode::CopyStruct, {newObj, srcObj}, ir::IRValue(),
                         std::to_string(ci->totalSize), "void", node->location);
                }
                // A-1（引用参数）：目标为引用参数时经指针写回（StorePtr 到槽内地址）
                if (isByRefCapture(ident->name)) {
                    ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                                     {ir::IRValue::var(unique, "ptr")},
                                                     "ptr", unique, node->location);
                    emit(ir::Opcode::StorePtr, {capAddr, newObj}, ir::IRValue(), "",
                         "ptr", node->location);
                } else {
                    emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique, "ptr",
                         node->location);
                }
                lastExpr_ = newObj;
                return;
            }
        }
    }
    // D2 根治（2026-09-09 第四十六轮）：构造字面量赋值（标识符目标）——
    //   p = 点{ x = 7, y = 9 } 与下标位（Task 2.7）/声明位同走 emitStructInitTo
    //   单点逐字段写（Rust 同构：构造字面量在任意值上下文=目标 place 逐字段
    //   初始化，穷举纪律在语义层 visitStructInitExpr）。原无此分支：genExpr
    //   空桩（常量0）落入通用 Store 只写首槽 8 字节 -> B1 零填假成功（静默
    //   错误代码，E2E 183 探针实锤 p.x 打出 0）。链式赋值 a = p = 点{...} 的
    //   外层经 isChainedAssign 路径取本分支返回的目标地址，语义一致。
    if (semantic_ != nullptr && !unique.empty() && !isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::StructInitExpr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
        const std::string tgtCanon = types::canonical(lookupSrcType(ident->name));
        if (semantic_->isStructType(tgtCanon)) {
            // A-1（引用参数）：目标为引用参数时目标地址 = Load 槽（槽内存被引用对象地址）
            ir::IRValue dstAddr;
            if (isByRefCapture(ident->name)) {
                dstAddr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(unique, "ptr")},
                                     "ptr", unique, node->location);
            } else {
                dstAddr = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(unique, "i64")},
                                     "ptr", unique, node->location);
            }
            emitStructInitTo(init, dstAddr, node->location);
            lastExpr_ = dstAddr;   // 与整体赋值链式语义一致（值=目标地址）
            return;
        }
    }
    ir::IRValue value = genExpr(node->value.get());
    // 查找变量类型（唯一内部名 unique 已在上方类深拷贝分支前计算）
    std::string targetType = lookupVarType(ident->name);
    if (targetType.empty()) targetType = value.type;
    // ---- 结构体整体赋值（Task 完善A）：b = a（C 语义逐字段拷贝 = 内存拷贝） ----
    // 目标/源均为结构体变量（源码类型是自定义结构体）时，生成 CopyStruct 指令：
    //   CopyStruct dstAddr=AddrOf(目标), srcAddr=AddrOf(源), extra=拷贝字节数
    // 结构体大小由语义层 typeSizeOf 计算（含数组字段，整体拷贝）。
    if (semantic_ != nullptr) {
        const std::string targetSrcType = lookupSrcType(ident->name);
        // 右值：标识符（b = a）或成员/下标（b = 名单[0]）或 链式赋值（b = c = a）
        //   或 结构体返回调用（读取结果 = 数据.读取(99)，Task 6.1 修复）
        std::string valueSrcType;
        bool isChainedAssign = false;  // 链式赋值：右值为内层赋值表达式
        bool isStructReturnCall = false;  // 结构体返回调用：右值为结构体返回函数/方法调用
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
        } else if (node->value->getType() == NodeType::CallExpr) {
            // 结构体返回调用（读取结果 = 数据.读取(99)）：
            //   genExpr 已生成调用（隐藏返回指针写入 retbuf），value =
            //   结构体地址（ptr 寄存器，指向 retbuf/临时对象）。
            //   目标变量是结构体时须 CopyStruct 按值拷贝——原实现漏此分支，
            //   落入默认 Store 只存 8 字节地址（读取结果 = retbuf 地址），
            //   .正常/.值 读到地址值导致越界判断失效（Task 6.1 E2E 发现）。
            isStructReturnCall = true;
            valueSrcType = targetSrcType;
        }
        if (semantic_->isStructType(types::canonical(targetSrcType)) &&
            semantic_->isStructType(types::canonical(valueSrcType))) {
            const int size = semantic_->typeSizeOf(types::canonical(targetSrcType));
            // A-1（引用参数）：目标为引用参数时目标地址 = Load 槽（槽内存被引用对象地址）
            ir::IRValue dstAddr;
            if (isByRefCapture(ident->name)) {
                dstAddr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(unique, "ptr")},
                                     "ptr", unique, node->location);
            } else {
                dstAddr = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(unique, "i64")},
                                     "ptr", unique, node->location);
            }
            // 源地址：标识符 -> AddrOf；成员/下标 -> lvalueAddress；
            // 链式赋值 -> 内层返回值（ptr 寄存器，源地址，内容与内层目标相同）；
            // 结构体返回调用 -> 调用返回的结构体地址（ptr 寄存器）
            ir::IRValue srcAddr;
            if (isChainedAssign || isStructReturnCall) {
                srcAddr = value;  // 内层 CopyStruct 返回/调用返回的结构体地址
            } else if (node->value->getType() == NodeType::IdentifierExpr) {
                const std::string srcUnique = lookupVarName(
                    static_cast<IdentifierExpr*>(node->value.get())->name);
                // A-1（引用参数）：源为引用参数时源地址 = Load 槽（槽内存地址）
                if (isByRefCapture(
                        static_cast<IdentifierExpr*>(node->value.get())->name)) {
                    srcAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(srcUnique, "ptr")},
                                         "ptr", srcUnique, node->location);
                } else {
                    srcAddr = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(srcUnique, "i64")},
                                         "ptr", srcUnique, node->location);
                }
            } else {
                srcAddr = lvalueAddress(node->value.get());
            }
            // 2026-08-25 方案A：结构体拷贝构造语义——目标类型有拷贝构造
            //   （类名(类名& 其他)）时，按值拷贝改为调用拷贝构造（深拷贝），
            //   而非 CopyStruct 浅拷贝（含裸指针结构体浅拷贝析构双释放 0xC0000374）。
            //   无拷贝构造的纯值结构体保持 CopyStruct 浅拷贝（零开销）。
            const ClassMemberInfo* copyCtor =
                semantic_->findCopyConstructor(types::canonical(targetSrcType));
            if (copyCtor != nullptr) {
                std::string copyOwner;
                // 沿继承链找到拷贝构造的所属类（ownerClass）
                copyOwner = copyCtor->ownerClass.empty()
                                ? types::canonical(targetSrcType)
                                : copyCtor->ownerClass;
                // this = 目标地址（dstAddr），实参 = 源地址（srcAddr）
                emit(ir::Opcode::Call, {dstAddr, srcAddr}, ir::IRValue(),
                     methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                     node->location);
            } else {
                emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
                     std::to_string(size), "void", node->location);
            }
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
            // A-1（引用参数）：目标为引用参数时复合赋值经 StorePtr 写回
            if (isByRefCapture(ident->name)) {
                ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                                 {ir::IRValue::var(unique, "ptr")},
                                                 "ptr", unique, node->location);
                emit(ir::Opcode::StorePtr, {capAddr, combined}, ir::IRValue(), "",
                     targetType, node->location);
            } else {
                emit(ir::Opcode::Store, {combined}, ir::IRValue(), unique, targetType,
                     node->location);
            }
            lastExpr_ = combined;
            return;
        }
    }
    // plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串赋值——三拥有形态
    //   （字面量/标识符/调用）时 free 旧值（__cn_str_free 空安全）+ 新值拥有化
    //   （字面量/标识符经 __cn_str_copy 落堆，调用返回直存）；**其余形态
    //   （解引用/成员等）整变量污染（stringTainted_）退出 RAII**——来源静态
    //   不可保证恒为堆串，free 只读段=UB，宁可放弃自动释放保安全。
    const std::string ownTargetSrcType = lookupSrcType(ident->name);
    // 72-a 根治（2026-09-11 第七十二轮，UAF 级别）：赋值位显式转移
    //   目标 = 转移(源) —— 真 move（Rust 对照：`a = b` 对 String 即 move=
    //   句柄移交 + 源失效；CN 默认深拷贝，显式 转移() 才 move）。原实现把
    //   转移(源) 当普通右值（实参值加载）走下方路径：目标与源共用同一堆句柄，
    //   而源仍在拥有名单中于作用域出口释放 -> 目标悬垂（stdlib 字符串扩展::
    //   替换 的 缓冲 = 转移(拼接) 实测输出乱码）。此处置为声明位浅交接
    //   （genVarDecl 分支）同模型：drop 旧目标 + 句柄直存 + 源槽清零
    //   （源 RAII 对零句柄空安全跳过）。自转移 甲 = 转移(甲) 语义=无操作
    //   （free 后存已释放值=UAF，直接跳过交接）。
    // 72-a 根治（2026-09-11 第七十二轮，UAF 级别）：引用参数（T& 出参）赋值
    //   拥有型串——写调用方槽后本函数出口释放源局部 = 调用方悬垂（探针 78：
    //   `输出 = 甲;` 后调用方读到垃圾；v2 树 解析货舱/拆限定名 两函数踩中，
    //   E2E 165 回归实证）。Rust 对照：`*out = s`（&mut String）对 String 即
    //   move——出参槽获得所有权、源失效。此处=句柄直写调用方槽 + 源槽清零
    //   （源 RAII 对零句柄空安全跳过）；源为污染名（借用视图）=不转移所有权
    //   （调用方不得登记 RAII，按借用视图读）。
    //   注：引用参数 entryInfo.srcType 保留 "T&" 形态（ir_decl 登记原样），
    //   判定须 canonical 剥引用后缀。
    if (types::canonical(ownTargetSrcType) == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0 &&
        isByRefCapture(ident->name) &&
        node->value->getType() == NodeType::IdentifierExpr) {
        const std::string rpSrcName =
            static_cast<const IdentifierExpr*>(node->value.get())->name;
        const std::string rpSrcUnique = lookupVarName(rpSrcName);
        const bool rpSrcTainted = stringTainted_.count(rpSrcName) > 0;
        if (!rpSrcUnique.empty() && !rpSrcTainted) {
            ir::IRValue byRefAddr = emitResult(
                ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                unique, node->location);
            emit(ir::Opcode::StorePtr, {byRefAddr, value}, ir::IRValue(), "",
                 targetType, node->location);
            ir::IRValue rpZero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                            node->location);
            emit(ir::Opcode::Store, {rpZero}, ir::IRValue(), rpSrcUnique, "i64",
                 node->location);
            lastExpr_ = value;
            return;
        }
    }
    if (ownTargetSrcType == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0 &&
        node->value->getType() == NodeType::CallExpr &&
        SemanticAnalyzer::isTransferCall(
            static_cast<const CallExpr*>(node->value.get()))) {
        const CallExpr* tr = static_cast<const CallExpr*>(node->value.get());
        if (!tr->arguments.empty() &&
            tr->arguments[0]->getType() == NodeType::IdentifierExpr) {
            const std::string srcName =
                static_cast<const IdentifierExpr*>(tr->arguments[0].get())->name;
            const std::string srcUnique = lookupVarName(srcName);
            if (!srcUnique.empty() && srcUnique != unique) {
                const bool srcTainted = stringTainted_.count(srcName) > 0;
                // drop 旧目标（拥有链维护下旧值必为堆串；空安全）——Rust 赋值
                //   语义=旧值在移交前析构
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
                // 句柄移交（value=转移展开的源槽加载值）+ 源槽清零
                emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, "ptr",
                     node->location);
                ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                              node->location);
                emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique, "i64",
                     node->location);
                // 源为借用视图（污染名）=移交的是借用句柄——目标不得登记 RAII
                if (srcTainted) stringTainted_.insert(ident->name);
                lastExpr_ = value;
                return;
            }
        }
    }
    if (ownTargetSrcType == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0) {
        const NodeType ownAt = node->value->getType();
        // 调用返回=白名单 ∪ 返回类型契约（A2 2026-09-11 方案甲）——被调者返回
        //   字符串 即拥有（语义层 retOwnedString 写回，全调用路径统一）；
        //   其余非白名单调用污染退出 RAII（借用安全方向）
        bool ownAssign = ownAt == NodeType::StringLiteral ||
                         ownAt == NodeType::IdentifierExpr;
        if (ownAt == NodeType::CallExpr) {
            const CallExpr* ace =
                static_cast<const CallExpr*>(node->value.get());
            ownAssign = ownAssign || ace->retOwnedString;
            if (!ownAssign && ace->callee->getType() == NodeType::IdentifierExpr) {
                const std::string& cn =
                    static_cast<const IdentifierExpr*>(ace->callee.get())->name;
                ownAssign = cn == "字符串复制" || cn == "字符串连接" ||
                            cn == "字符串拼接" || cn == "字符串子串" ||
                            cn == "字符串大写" || cn == "字符串小写" ||
                            cn == "字符串修剪" || cn == "字符串反转";
            }
        }
        if (ownAssign) {
            if (ownAt != NodeType::CallExpr) {
                // 先 free 旧（drop 旧再 move 新——Rust/C++ 赋值语义；调用形态
                // 的 free 由下方公共段统一发？——不：调用形态同样须 free 旧。
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
                value = emitResult(ir::Opcode::Call, {value}, "ptr",
                                   "__cn_str_copy", node->location);
            } else {
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
            }
        } else {
            stringTainted_.insert(ident->name);
        }
    }
    // 简单赋值（Task 2.3：右值类型与目标类型不同时先隐式转换 Cast，
    // 如 整8 x = 30000 需截断、整32 -> 整64 需扩展、整 -> 浮 需转换）
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "", node->location);
    }
    // 缺陷修复（[&] 引用捕获赋值写回，规格书04-一D）：参数槽存被捕获变量地址，
    //   赋值必须经指针（LoadPtr 地址 -> StorePtr 写被捕获变量），
    //   使闭包内修改反映到外部（引用语义）。值捕获（[=]）仍走本地 Store。
    if (isByRefCapture(ident->name)) {
        ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(unique, "ptr")},
                                         "ptr", unique, node->location);
        emit(ir::Opcode::StorePtr, {capAddr, value}, ir::IRValue(), "",
             targetType, node->location);
        lastExpr_ = value;
        return;
    }
    emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, targetType,
         node->location);
    lastExpr_ = value;
}
void IRGenerator::visitNullLiteral(NullLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", node->location);
}
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
            // 元素间距：按元素类型大小（缺陷③根治统一 C 布局——typeSizeOf 含
            //   结构体总大小（Task 2.7）/i128=16（BUG #5）/标量 4/2/1；原标量
            //   兜底 8 与数组 8 槽布局互洽，见 ir.cpp registerVarSlots 注）
            std::int64_t elemStride = 8;
            if (semantic_ != nullptr) {
                const int size = semantic_->typeSizeOf(elemSrc);
                if (size > 0) elemStride = size;
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
        // 自举前置 A-1（plans/004）：字符串[i] 逐字节 O(1) 访问（字符* 字节
        //   视图，步进 1；与 C 的 s[i] 一致，越界由调用方按 字符串长度 约束）
        if (!unique.empty() && types::canonical(srcType) == "字符串") {
            ir::IRValue ptr = genExpr(node->object.get());
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            ir::IRValue addr = emitResult(ir::Opcode::Add, {ptr, index}, "ptr", "",
                                          node->location);
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "i8", "",
                                   node->location);
            return;
        }
    }
    // A-3（2026-08）：隐式类字段对象（方法体内 数据[位置]，数据 是 this->字段，
    //   lookupSrcType 为空）——此前落入下方"其他对象"分支按 8 字节步进/标量 LoadPtr，
    //   结构体元素越界读（向量<点> 元素错乱实测）。此处按字段源码类型推导
    //   步进与元素形态（数组字段/结构体指针字段 -> 返回元素地址；标量 -> LoadPtr）。
    if (node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident2 = static_cast<IdentifierExpr*>(node->object.get());
        const std::string st2 = lookupSrcType(ident2->name);
        if (st2.empty() && isInstanceField(ident2->name)) {
            const std::string fieldType = classFieldType(currentClass_, ident2->name);
            ir::IRValue ptr = genExpr(node->object.get());  // 字段指针值
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            std::int64_t stride = 8;
            std::string elemSrc = fieldType;
            bool elemIsStruct = semantic_ != nullptr &&
                                semantic_->isStructType(types::canonical(fieldType));
            if (types::isArray(fieldType)) {
                // 数组字段：按元素大小步进 + 越界检查（与成员数组字段同规则，
                //   结构体内嵌数组按 C 布局紧凑排布）
                elemSrc = types::arrayElemOf(fieldType);
                elemIsStruct = semantic_ != nullptr &&
                               semantic_->isStructType(types::canonical(elemSrc));
                stride = elemIsStruct ? semantic_->typeSizeOf(elemSrc)
                         : (types::isI128(types::canonical(elemSrc)) ? 16
                                                                    : types::typeSize(elemSrc));
                emitBoundsCheck(index, types::arrayLenOf(fieldType), node->location);
            } else if (types::isPointer(fieldType)) {
                // 指针字段（T* 数据）：步进统一按所指元素 typeSizeOf（缺陷③根治
                //   2026-09-03——与 ptrElemStride/容器库紧凑分配一致；原「标量 8
                //   字节槽」与写侧按元素大小混用 -> 窄元素读写错位腐坏，30 实证）
                elemSrc = types::pointeeOf(fieldType);
                const std::string elemCanon = types::canonical(elemSrc);
                // H8 补完（2026-08-25）：类元素（向量<T> 数据 T*，T=映射/简单盒）
                //   同结构体——按类总大小步进、返回元素地址（原兜底 8+LoadPtr
                //   -> 元素错位越界 0xC0000005）
                elemIsStruct = semantic_ != nullptr &&
                               (semantic_->isStructType(elemCanon) ||
                                semantic_->isClassType(elemCanon));
                const int elemSize = (semantic_ != nullptr)
                                         ? semantic_->typeSizeOf(elemSrc) : 0;
                stride = elemSize > 0 ? elemSize
                                      : (types::isI128(elemCanon) ? 16 : 8);
            } else if (types::canonical(fieldType) == "字符串") {
                // 自举前置 A-1：字符串字段（自身.源码[i]）——字符* 字节步进 1
                elemSrc = "字符";
                stride = 1;
            } else if (elemIsStruct) {
                stride = semantic_->typeSizeOf(fieldType);
            }
            ir::IRValue scaled = emitResult(
                ir::Opcode::Mul,
                {index, ir::IRValue::constant(std::to_string(stride), "i64")},
                "i64", "", node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {ptr, scaled}, "ptr", "",
                                          node->location);
            if (elemIsStruct) {
                lastExpr_ = addr;  // 结构体元素：返回地址（结构体值语义）
                return;
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, mapType(elemSrc), "",
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
    // H8 补完（2026-08-25）：隐式类字段对象（向量 数据 T* 的 数据[位置] 读取）——
    //   lookupSrcType 为空（字段不在 IR varStack），按字段源码类型推导步进
    //   （ptrElemStride：类元素按类总大小，原兜底 8 -> 元素错位）。
    if (node->object->getType() == NodeType::IdentifierExpr) {
        const std::string objName =
            static_cast<IdentifierExpr*>(node->object.get())->name;
        std::string st = lookupSrcType(objName);
        if (st.empty() && isInstanceField(objName)) {
            st = classFieldType(currentClass_, objName);
            if (types::isPointer(st)) {
                stride = ptrElemStride(st);
            }
        }
    }
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
                // 自举前置 A-1：字符串成员（点.名[i]）——字符* 字节步进 1
                if (f.name == inner->memberName && types::canonical(f.type) == "字符串") {
                    stride = 1;
                    break;
                }
            }
        }
        // 宿主缺陷1'根治（2026-09-02）：类对象/结构体的指针与数组字段下标读侧
        //   （拷贝构造 其他.数据[索引]：其他 为类对象非 StructDecl，原兜底 8）
        if (stride == 8) {
            const std::string ftype = memberFieldSrcType(inner);
            if (types::isPointer(ftype)) {
                stride = ptrElemStride(ftype);
            } else if (types::isArray(ftype)) {
                const std::string elemSrc = types::arrayElemOf(ftype);
                stride = (semantic_->isStructType(types::canonical(elemSrc))
                              ? semantic_->typeSizeOf(elemSrc)
                              : types::typeSize(elemSrc));
                emitBoundsCheck(index, types::arrayLenOf(ftype), node->location);
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
                // 自举前置 A-1：字符串成员元素类型 字符（i8）
                if (f.name == inner->memberName && types::canonical(f.type) == "字符串") {
                    elemIrType = "i8";
                    break;
                }
            }
        }
        // 宿主缺陷1'根治（2026-09-02）：类对象/结构体的指针字段下标元素形态
        //   （拷贝构造 其他.数据[索引] 读侧：元素为结构体/类值时返回地址，
        //   原 LoadPtr 只读首 8 字节）。与 IdentifierExpr 分支（:1754）同规则。
        if (!elemIsStruct) {
            const std::string ftype = memberFieldSrcType(inner);
            if (types::isPointer(ftype)) {
                const std::string elemSrc = types::pointeeOf(ftype);
                elemIrType = mapType(elemSrc);
                const std::string elemCanon = types::canonical(elemSrc);
                if (semantic_ != nullptr &&
                    (semantic_->isStructType(elemCanon) ||
                     semantic_->isClassType(elemCanon))) {
                    elemIsStruct = true;
                }
            }
        }
    } else if (node->object->getType() == NodeType::IdentifierExpr) {
        const std::string objName =
            static_cast<IdentifierExpr*>(node->object.get())->name;
        std::string st = lookupSrcType(objName);
        // H8 补完（2026-08-25）：隐式类字段对象（向量 数据 T* 的 数据[位置]）——
        //   lookupSrcType 为空，按字段源码类型推导元素形态（类元素返回地址）
        if (st.empty() && isInstanceField(objName)) {
            st = classFieldType(currentClass_, objName);
        }
        if (types::isPointer(st)) {
            const std::string elemSrc = types::pointeeOf(st);
            elemIrType = mapType(elemSrc);
            const std::string elemCanon = types::canonical(elemSrc);
            // H8 补完（2026-08-25）：类元素（向量<T> 数据 = T*，T=映射）同结构体
            //   返回元素地址（元素是内联类值，LoadPtr 只读 8 字节错误）
            if (semantic_ != nullptr &&
                (semantic_->isStructType(elemCanon) || semantic_->isClassType(elemCanon))) {
                elemIsStruct = true;
            }
        } else if (types::canonical(st) == "字符串") {
            // 自举前置 A-1：字符串变量兜底（i8 字节视图）
            elemIrType = "i8";
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
void IRGenerator::visitInitListExpr(InitListExpr* node) {
    (void)node;
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}
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
void IRGenerator::visitStructInitExpr(StructInitExpr* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}
void IRGenerator::visitMemberExpr(MemberExpr* node) {
    // ---- 阶段3 OOP（Task 3.1）：类字段读取（对象.字段 / 类名.静态字段） ----
    // 静态字段 -> 静态字段地址 + LoadPtr；实例字段 -> 对象指针 + FieldAddr + LoadPtr。
    // 注意：方法调用（对象.方法()）由 visitCallExpr 的 handleClassCallExpr 先行拦截，
    //   此处 MemberExpr 仅处理"读取字段值"；方法引用作值（函数指针）暂不支持（语义层预留）。
    if (handleClassMemberExpr(node)) {
        return;
    }
    // 枚举引用：枚举名.成员 → 整数值（Task 2.7；枚举类型名非指针，
    //   isDerefAccess 恒 false——v2.1 保留原防御结构）
    if (!node->isDerefAccess && node->object->getType() == NodeType::IdentifierExpr) {
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
    // 地址：直接调用 lvalueAddress(node) 递归处理（值对象为变量/嵌套成员/下标；
    //        经指针对象为指针值——lvalueAddress 对 MemberExpr 已按 isDerefAccess 区分）
    // 说明：lvalueAddress(MemberExpr) 递归计算 基址+偏移，经指针访问隐含空指针检查（错误码3）
    ir::IRValue fieldAddr = lvalueAddress(node);
    // 查询字段类型（语义层布局，递归解析对象类型）
    if (semantic_ == nullptr) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 对象源码类型：值对象访问为变量源码类型（含嵌套 r.左上 的字段类型递归）；
    //               经指针对象访问（v2.1 统一 .，isDerefAccess）为指针所指类型
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
            // 宿主缺陷根治（2026-09-03，E2E 134 当场揪出）：嵌套成员链第一层为
            //   指针变量（外层* wq; wq.内.x）——findStruct("外层*") 不剥指针 ->
            //   innerDecl==null -> objSrcType 空 -> decl==null -> 字段读降级常量 0
            //   （比较恒真/打印恒 0，静默错行为）。原 -> 语法同形态同缺陷（旧用例
            //   嵌套链均隔下标层走 IndexExpr 分支未踩中）。修：剥指针一级再查
            //   （与下方 1995 行 memberObjStructType 内 isDerefAccess 剥法一致）。
            if (types::isPointer(innerObjType)) {
                innerObjType = types::pointeeOf(innerObjType);
            }
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
        // 宿主缺陷根治（2026-08-25）：结果/可选 的 .值/.错误 不是合成结构体直接字段
        //   （在联合体内）——嵌套成员（查.值.名ID）须按结果/可选成员映射推导对象类型，
        //   否则 objSrcType 空 -> decl==nullptr -> 返回常量 0（实测打印 0）。
        if (objSrcType.empty() && semantic_ != nullptr) {
            const std::string canonInner = types::canonical(innerObjType);
            if (SemanticAnalyzer::isResultType(canonInner)) {
                const std::vector<std::string> rargs =
                    SemanticAnalyzer::resultTypeArgs(canonInner);
                if (inner->memberName == "值" && rargs.size() == 2) {
                    objSrcType = rargs[0];
                } else if (inner->memberName == "错误" && rargs.size() == 2) {
                    objSrcType = rargs[1];
                } else if (inner->memberName == "正常") {
                    objSrcType = "布尔";
                }
            } else if (SemanticAnalyzer::isOptionalType(canonInner)) {
                if (inner->memberName == "值") {
                    objSrcType = SemanticAnalyzer::optionalTypeArg(canonInner);
                } else if (inner->memberName == "有值") {
                    objSrcType = "布尔";
                }
            }
        }
    } else if (node->object->getType() == NodeType::UnaryExpr) {
        // 显式解引用成员 (*q).x —— 对象经 * 解引用后为所指结构体，类型=所指类型。
        //   宿主缺陷根治（2026-09-03，E2E 134 第四节当场揪出）：原分支表不认
        //   UnaryExpr 对象 -> objSrcType 空 -> decl==null -> 降级常量 0（既有缺陷，
        //   v2.1 统一 . 后 p.字段 ≡ (*p).字段 两形态须等价可互换）。
        UnaryExpr* u = static_cast<UnaryExpr*>(node->object.get());
        if (u->op == Operator::Deref &&
            u->operand->getType() == NodeType::IdentifierExpr) {
            const std::string pType = lookupSrcType(
                static_cast<IdentifierExpr*>(u->operand.get())->name);
            if (types::isPointer(pType)) {
                objSrcType = types::pointeeOf(pType);
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
    } else if (node->isDerefAccess && node->object->getType() == NodeType::BinaryExpr) {
        // 指针算术结果成员：(名单 + (n-1)).分数 — 从左操作数（指针变量）
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
    } else if (node->object->getType() == NodeType::CallExpr) {
        // 宿主缺陷根治（2026-09-01，用户令缺陷零容忍）：对象是函数/方法调用
        //   （向量.元素(i).字段）——原只认 变量/嵌套成员/下标，CallExpr 推导空
        //   -> decl==nullptr -> 字段读取降级常量 0。补经 exprSrcType 解析返回
        //   类型（与 lvalueAddress 的同款补丁配套：地址层 + 类型/宽度层双修复）。
        objSrcType = exprSrcType(node->object.get());
    }
    if (node->isDerefAccess && types::isPointer(objSrcType)) {
        objSrcType = types::pointeeOf(objSrcType);
    }
    const StructDecl* decl = semantic_->findStruct(types::canonical(objSrcType));
    if (decl == nullptr) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 字段类型（IR类型）
    // 阶段3（Task 3.5）：结果/可选 合成结构体成员名映射（与 fieldOffsetOf 一致）——
    //   源码 .正常/.值/.错误/.有值 对应 是否正常/错误值联合/是否某些；
    //   .值/.错误 读内层联合体，类型 = 结果<T,E> 的 T/E 参数。
    std::string fieldSrcType = "";
    const std::string& srcObjType = types::canonical(objSrcType);
    if (SemanticAnalyzer::isResultType(srcObjType)) {
        const std::vector<std::string> rargs = SemanticAnalyzer::resultTypeArgs(srcObjType);
        if (node->memberName == "正常") {
            fieldSrcType = "布尔";
        } else if (node->memberName == "值" && rargs.size() == 2) {
            fieldSrcType = rargs[0];
        } else if (node->memberName == "错误" && rargs.size() == 2) {
            fieldSrcType = rargs[1];
        }
    } else if (SemanticAnalyzer::isOptionalType(srcObjType)) {
        if (node->memberName == "有值") {
            fieldSrcType = "布尔";
        } else if (node->memberName == "值") {
            fieldSrcType = SemanticAnalyzer::optionalTypeArg(srcObjType);
        }
    }
    if (fieldSrcType.empty()) {
        for (const auto& f : decl->fields) {
            if (f.name == node->memberName) {
                fieldSrcType = f.type;
                break;
            }
        }
    }
    // 修复10（数组字段退化）：结构体数组字段（如 方形.顶点）作为值表达式时，
    //   按 C 语义退化为指向首元素的指针——返回字段地址（FieldAddr）而非 LoadPtr
    //   读取字段处 8 字节当指针（垃圾值 -> 空指针错误3/访问冲突崩溃）。
    //   后续 方形.顶点[i] 的基址即此字段地址。
    // 宿主缺陷根治（2026-08-25）：结构体字段（结果.值 为 点/符号 结构体值）同样
    //   返回字段地址（值语义）——原 LoadPtr 只读首 8 字节当值，内联结构体数据
    //   拷贝给调用方时 CopyStruct(首字段值) 崩（地址 7 读取 0xC0000005）；
    //   嵌套 查.值.名ID 也依赖字段地址作基址。
    if (types::isArray(fieldSrcType) ||
        (semantic_ != nullptr &&
         semantic_->isStructType(types::canonical(fieldSrcType)))) {
        lastExpr_ = fieldAddr;
        return;
    }
    // 字段加载（LoadPtr 含空指针检查：错误码3）
    //   容器/类类型字段（函数IR.指令）：LoadPtr 读首 8 字节=堆对象指针（统一
    //   指针槽模型，与类字段 A-4/byRef 机制一致——缺陷2 根治 2026-09-02 定案：
    //   结构体容器字段=typeSizeOf 保留区+首槽指针，构造赋值 Store 指针、
    //   读取 LoadPtr、方法 this=对象指针；结构体局部零初始化保未构造槽为 null）。
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {fieldAddr},
                           mapType(fieldSrcType), "", node->location);
}
void IRGenerator::visitType(Type* node) {
    (void)node;
}
void IRGenerator::visitSizeofExpr(SizeofExpr* node) {
    // H8 补完（2026-08-25）：SizeofExpr AST 节点被泛型多实例共享，node->size
    //   是某次语义检查写入的实例特定值（多实例取同一值错）。IR 生成按当前
    //   genericTypeParams_（emitClassMethod 按 typeArgs 设置）重新解析：
    //   类型参数 T -> 本实例实参（substGenericType），再实例化具体泛型源形式
    //   （映射<整64,整64> -> 映射$整64$整64，resolveGenericTypeName），
    //   最后算类型大小。非泛型（类型大小(学生)）substGenericType 原样返回。
    int size = 8;
    if (semantic_ != nullptr) {
        const std::string t = substGenericType(node->typeName);
        const std::string resolved =
            semantic_->resolveGenericTypeName(t, node->location);
        size = semantic_->typeSizeOf(resolved);
    }
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i64",
                           std::to_string(size), node->location);
}
void IRGenerator::visitCastExpr(CastExpr* node) {
    ir::IRValue operand = genExpr(node->operand.get());
    const std::string target = mapType(node->targetType);
    // 源类型为 i128/u128 且目标为浮点：Cast 指令源类型按双槽约定
    // （codegen emitCast 已按 from==i128/u128 走辅助函数 __cn_*_to_f64）。
    // 指针 -> 整数 / 整数 -> 指针：IR 层类型均为 8 字节槽（ptr/i64），
    //   Cast 的 codegen 需按 from/to 文本区分——这里保持类型信息完整：
    //   若源是 ptr 且目标 i64：Cast(from=ptr, to=i64) 位重解释；
    //   若源是 i64 且目标 ptr：Cast(from=i64, to=ptr)。
    // 注意：IR 类型 "ptr" 与 "i64" 都是 64 位槽，Cast 指令携带完整
    //   from/to 类型，codegen emitCast 依据类型分派正确指令。
    lastExpr_ = emitResult(ir::Opcode::Cast, {operand}, target, "", node->location);
}
void IRGenerator::visitLambdaExpr(LambdaExpr* node) {
    const std::string lambdaName = "?lambda" + std::to_string(lambdaCounter_++);
    const std::string returnIrType = mapType(node->returnType.empty()
                                                 ? "空类型" : node->returnType);
    ir::IRFunction func;
    func.name = lambdaName;
    func.returnType = returnIrType;
    // 捕获变量名（语义层已回填全部捕获）：匿名函数参数前置
    std::vector<std::string> capturedNames = node->explicitCaptures;
    // 缺陷修复（[=] 快照 / [&] 引用区分，规格书04-一D）：
    //   [&] 引用捕获整体：所有捕获参数按"指针"形态传递（参数槽存被捕获变量地址），
    //     体内读取解引用（读最新值）、赋值经指针（写外部变量）。
    //   [=]/[]/[变量] 值捕获：参数槽存定义处值快照（值语义，后续外部修改不影响）。
    const bool allByRef = (node->captureKind == LambdaCaptureKind::ByRef);
    std::vector<bool> captureRefs(capturedNames.size(), allByRef);
    // 保存外层生成状态（lambda 内嵌在表达式中，生成匿名函数后须恢复主函数状态）
    ir::IRFunction* outerFunction = function_;
    ir::IRBlock* outerBlock = currentBlock_;
    const int outerBlockCounter = blockCounter_;
    const std::size_t outerVarDepth = varStack_.size();
    function_ = &func;
    varStack_.emplace_back();
    // 捕获参数：类型 = 外层变量 IR 类型（查当前 varStack 作用域链）；
    //   [&] 引用捕获参数类型为 ptr（存被捕获变量地址），体内 byRef 标记驱动解引用
    for (std::size_t ci = 0; ci < capturedNames.size(); ++ci) {
        const std::string& cap = capturedNames[ci];
        const bool byRef = captureRefs[ci];
        const std::string capType = lookupVarType(cap);
        const std::string capSrc = lookupSrcType(cap);
        // 缺陷修复（[=] 结构体值捕获）：结构体值捕获实参是"定义处深拷贝临时
        //   缓冲区的指针"（见 genVarDecl captureArgs 固化逻辑），闭包参数须标记
        //   为按值结构体参数（structParamIndexes），emitParamSetup 才会从该指针
        //   rep movsb 拷贝结构体数据到参数槽——否则参数槽存的是指针值本身，
        //   闭包体字段访问读到地址值字节=垃圾（值捕获p.x: 553448424 而非 1）。
        const bool isStructValCapture =
            !byRef && semantic_ != nullptr &&
            semantic_->isStructType(types::canonical(capSrc));
        std::string unique = cap + "$" + std::to_string(varCounter_++);
        const std::string paramType =
            byRef ? "ptr" : (capType.empty() ? "i64" : capType);
        func.params.emplace_back(cap, paramType);
        func.paramUniques.push_back(unique);
        if (isStructValCapture) {
            func.structParamIndexes.insert(static_cast<int>(ci));
        }
        // [&] 引用捕获：参数槽 1 槽（8 字节指针）；[=] 值捕获：按原类型多槽（i128 双槽）
        registerVarSlots(unique, byRef ? "" : capSrc);
        ir::IRValue reg = newReg();
        reg.type = paramType;
        VarEntry entry;
        entry.regId = reg.id;
        entry.uniqueName = unique;
        // 体内"值类型"仍为原类型：读取时经 byRef 解引用（LoadPtr）返回原类型值
        entry.type = byRef ? (capType.empty() ? "i64" : capType) : reg.type;
        entry.srcType = capSrc;
        entry.byRef = byRef;
        varStack_.back()[cap] = entry;
    }
    // 显式参数
    for (std::size_t pi = 0; pi < node->params.size(); ++pi) {
        auto& param = node->params[pi];
        // A-1（引用参数）：lambda 显式引用参数（整32&）同样按 byRef 语义
        const bool isRefParam = !param->funcPtr.isFunctionPtr() &&
                                types::isReference(param->typeName);
        const std::string ptype = (param->funcPtr.isFunctionPtr() || isRefParam)
                                      ? "ptr" : mapType(param->typeName);
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        func.params.emplace_back(param->name, ptype);
        func.paramUniques.push_back(unique);
        registerVarSlots(unique, isRefParam ? ""
                            : (param->funcPtr.isFunctionPtr() ? "" : param->typeName));
        ir::IRValue reg = newReg();
        reg.type = ptype;
        VarEntry entry;
        entry.regId = reg.id;
        entry.uniqueName = unique;
        entry.type = isRefParam ? mapType(types::stripRef(param->typeName)) : reg.type;
        entry.srcType = param->typeName;
        entry.byRef = isRefParam;
        varStack_.back()[param->name] = entry;
    }
    blockCounter_ = 0;
    ir::IRBlock* entry = newBlock("bb0");
    (void)entry;
    if (node->body != nullptr) {
        genBlock(node->body.get());
    }
    if (!function_->blocks.empty()) {
        ir::IRBlock* last = function_->blocks.back().get();
        if (!last->terminated) {
            setCurrentBlock(last);
            endReturn("");
        }
    } else {
        setCurrentBlock(entry);
        endReturn("");
    }
    varStack_.pop_back();
    module_->functions.push_back(std::move(func));
    // 恢复外层生成状态（主函数继续）
    function_ = outerFunction;
    currentBlock_ = outerBlock;
    blockCounter_ = outerBlockCounter;
    // 弹出 lambda 捕获参数作用域（仅弹出 visitLambdaExpr 自己压入的一层）
    while (varStack_.size() > outerVarDepth) varStack_.pop_back();
    // 记录最近一次 lambda 的匿名函数名与捕获列表（genVarDecl 登记闭包关联用）
    lastLambdaName_ = lambdaName;
    lastLambdaCaptures_ = capturedNames;
    lastLambdaReturnIrType_ = returnIrType;
    lastLambdaCaptureRefs_ = captureRefs;
    // 表达式结果 = 匿名函数地址（赋给 自动 变量；调用经 closureInfo_ 展开）。
    // 注意：FuncAddr 须在外层（主函数）块中生成——function_ 已恢复
    lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr", lambdaName, node->location);
}
} // namespace cn_compiler
