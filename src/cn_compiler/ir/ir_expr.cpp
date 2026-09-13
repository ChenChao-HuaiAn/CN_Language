// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
// 7. D1 行数整改 112-a：按族拆出 ir_expr_unary/assign/index/member.cpp（纯重构零行为变更，声明仍在 ir.hpp）
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

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
            // 95-a：字符字面量码点（单一归属 charLiteralCodePoint——转义/\u{}/UTF-8
            //   全解码，规范 01b 三；原「单引号取首字节」注释声称全解码实为未实现）
            return ir::IRValue::constant(
                std::to_string(charLiteralCodePoint(lit->raw)), "i32");
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
    // 95-a：字符字面量码点（单一归属 charLiteralCodePoint——转义/\u{}/UTF-8 全解码）
    const int code = charLiteralCodePoint(node->raw);
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
            const std::string canonStatic = types::canonical(stType);
            ir::IRValue addr = emitResult(
                ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + node->name, node->location);
            // 87-a（2026-09-12 第八十七轮）：结构体静态——值语义=**地址**（与局部
            //   结构体变量同款的「表达式值为结构体地址」约定）。原实现无条件
            //   LoadPtr 读 8 字节：读出首字段句柄当结构体地址 → 按值传参时被调方
            //   从该地址 rep movsb（探针 P50b 崩溃堆栈实证：取参 → __cn_str_copy
            //   SIGSEGV）；整体拷贝/成员链基址同源失效。
            if (semantic_->isStructType(canonStatic)) {
                lastExpr_ = addr;
                return;
            }
            const std::string irT = mapType(stType.empty() ? "整64" : stType);
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
    // 92-a：浮点算术公共类型（f32 op f32 -> f32 单精度；跨类型统一 f64）
    std::string floatCommon;
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
        // 整 + 浮：整侧转浮64（整型无损转 f64=安全方向，规范「仅允许无损或宽化」）；
        // 浮32 + 浮64：浮32转浮64
        const std::string common = "f64";
        if (left.type != common) {
            left = emitResult(ir::Opcode::Cast, {left}, common, "", node->location);
        }
        if (right.type != common) {
            right = emitResult(ir::Opcode::Cast, {right}, common, "", node->location);
        }
        floatCommon = common;
    } else if (isFloat) {
        // 92-a 根治（H2）：同类型浮点保持操作数类型——浮32 op 浮32 = 单精度运算
        //   （规格书「加浮32」变体 + Rust f32 语义）。原实现算术结果类型恒取
        //   f64（阶段一简化），与 f32 操作数的 4 字节装载错配：movss 装载清零
        //   xmm 高 96 位、addsd 把 f32 位模式当 f64 读（退化为 ~1e-315 非规格
        //   化数，加法恒得 0），且结果按 8 字节存 4 字节槽（栈溢出破坏）。
        floatCommon = left.type;
    }
    ir::Opcode opcode;
    std::string resultType;
    if (mapBinaryOp(node->op, isFloat, opcode)) {
        // 结果类型推导：
        //   比较/逻辑 -> i1
        //   位运算/移位 -> 整型（取左操作数类型，与语义 commonNumericType 一致）
        //   算术 -> 浮点取公共类型（92-a：f32/f64 精确，原恒 f64）/整型取左操作数类型
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
                resultType = isFloat ? floatCommon : left.type;
                break;
        }
        lastExpr_ = emitResult(opcode, {left, right}, resultType, "", node->location);
    } else {
        // 不支持的操作：回退左操作数
        lastExpr_ = left;
    }
}
void IRGenerator::visitNullLiteral(NullLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", node->location);
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
