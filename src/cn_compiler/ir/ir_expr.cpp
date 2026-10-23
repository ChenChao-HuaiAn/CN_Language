// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
// 7. D1 行数整改 112-a：按族拆出 ir_expr_unary/assign/index/member.cpp（纯重构零行为变更，声明仍在 ir.hpp）
// 8. 728o 连接链融合：族②③（字符串连接/隐式拼接/128 位段）拆出
//    ir_expr_concat.cpp 并承载连接链融合 pass（先拆再加·>900 警戒线纪律）
#include <cstdint>
#include <string>
#include <utility>

#include <cstdio>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_string_temp_util.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitIntegerLiteral(IntegerLiteral* node) {
    // 910（超64域口径收口·759 探针实锚）：判定/物化链收口——919 提炼为
    //   emitIntLiteralConst 单点（ir_decl_extra.cpp·常量引用面共用）。原链
    //   三处读 node->value 值级判域（parser parseIntValue stoll 溢出静默回 0
    //   → 巨额立即数 A2022/textExceedsInt64 长度判据对 hex 误判）；910 修后=
    //   规范十进制文本判提升链 + 128 域检查 + ConstInt 存规范文本（i128 双半）。
    lastExpr_ = emitIntLiteralConst(node->raw, node->location);
}
void IRGenerator::visitFloatLiteral(FloatLiteral* node) {
    // 1360 结果化：浮点面无后缀契约态=浮64（001 §4.3·显形默认）
    std::string type = types::literalTypeOf(node->raw, true).value_or("浮64");
    lastExpr_ = emitResult(ir::Opcode::ConstFloat, {}, mapType(type),
                           types::stripLiteralSuffix(node->raw), node->location);
}
void IRGenerator::visitStringLiteral(StringLiteral* node) {
    // 938：注册逻辑提炼 internString（原内联·常量内联/默认值同款单点复用）
    lastExpr_ = emitResult(
        ir::Opcode::ConstString, {}, "ptr",
        "@str" + std::to_string(internString(decodeString(node->raw))),
        node->location);
}
int IRGenerator::internEmptyString() {
    // 938：并入 internString 单点（行为等价）
    return internString("");
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
    // 584-a（011-002·T47 甲「遮蔽生效」）：常量内联折叠移至变量家族（局部/静态
    //   局部/顶层静态）读取之后（下方 reg.id<0 分支内）——块内同名变量/参数
    //   遮蔽顶层常量时读点取变量（346 锚：块内 整32 上限 遮蔽 常量 上限，读点
    //   读变量槽而非内联 10）；无同名遮蔽的常量引用仍走常量内联（行为零变化）。
    // 1360 结果化：lookupVar 无值=未登记（含 regId<0 静态局部——原哨兵
    //   reg(-1,"") 的 id<0 判定同义）
    const cn::Optional<VarEntry> regVar = lookupVar(node->name);
    if (!regVar.has_value()) {
        // 320-a（T41）：函数内静态局部读取——varStack 命中但 regId=-1（无栈槽，
        //   存储为 .data 全局符号）；按 varStack 层级查 isStaticLocal 标志走
        //   ?gstatic_键 LoadPtr（局部优先序不变——同名遮蔽由层级天然处理）。
        {
            const VarEntry* ve = nullptr;
            for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
                auto fit = it->find(node->name);
                if (fit != it->end()) { ve = &fit->second; break; }
            }
            if (ve != nullptr && ve->isStaticLocal) {
                if (ve->type.empty()) {
                    // 648（miss 不可静默·110 同族）：静态局部变量 IR 类型未登记=
                    //   编译器内部缺陷（声明位登记必填类型），显形于此；driver 在
                    //   generate() 后 hasErrors() 统一中止，产物不落地。原 i32 兜底
                    //   保留（控制流零变更）。
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "内部错误: 静态局部变量 '" + node->name +
                            "' IR 类型未登记 ——静默宽度兜底已废除（#648）");
                }
                const std::string irT = ve->type.empty() ? "i32" : ve->type;
                ir::IRValue addr = emitResult(
                    ir::Opcode::ConstString, {}, "ptr",
                    "?gstatic_" + ve->uniqueName, node->location);
                // 061-b（2026-09-27 804 轮）：聚合静态局部（结构体/合成体）读=
                //   符号地址（地址语义：CopyStruct 源/成员链基址均从地址起算，
                //   与顶层静态左值 87-a 同款）——原 LoadPtr("ptr") 只读 8 字节，
                //   16 字节合成体半读致返回/赋值读截断（z3v 段错误实测）。
                //   标量静态局部维持 LoadPtr 读值（320-a 原行为）。
                if (semantic_ != nullptr && !ve->srcType.empty() &&
                    semantic_->isStructType(types::canonical(ve->srcType))) {
                    lastExpr_ = addr;
                    return;
                }
                // 852（2026-10-16·843 呈报）：静态局部**数组**——值语义=基址
                //   （259 顶层数组同款：下标读 genGenericIndexAddress/整体赋值
                //   源基址均消费 genExpr(名)=符号地址）。原落 LoadPtr 读槽首
                //   8 字节当基址（读出数据[0..1] 当地址=悬垂·S2 链路实证
                //   undefined reference 另案；本槽位面=S7 探针）。
                if (semantic_ != nullptr && !ve->srcType.empty() &&
                    types::isArray(types::canonical(ve->srcType))) {
                    lastExpr_ = addr;
                    return;
                }
                lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, irT, "",
                                       node->location);
                return;
            }
        }
        // 第 9 层 Debug（P3-8）：顶层静态变量读取——全局 .data 符号 LoadPtr。
        //   静态变量不在函数局部 varStack_，须按全局符号地址读取（?gstatic_名）；
        //   放在 lookupVar 失败后（局部变量优先，防止同名遮蔽误读全局）。
        if (semantic_ != nullptr && semantic_->isGlobalStatic(node->name)) {
            // 1360 结果化批3B：无值=未登记——value_or 显形空串回落
            const std::string stType =
                semantic_->globalStaticType(node->name).value_or(std::string());
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
            // 259（2026-10-07·#259·静态数组全链分叉根治）：数组静态——值语义=
            //   **基址**（与局部数组「数组名退化为元素指针」94-a 同构·87-a 结构体
            //   静态地址约定同款）。原无条件 LoadPtr 读 8 字节：读出 .data 首槽值
            //   当基址（零初始化=0）→ 下标读写「空指针解引用」崩溃（探针 A3 实录
            //   ·v2 侧=语义拒绝「下标对象须为字符串或指针」同病异形=分叉缺陷）。
            //   下标路径（genGenericIndexAddress genExpr(对象)=基址）与整体赋值
            //   源基址随本分支同愈。
            if (types::isArray(canonStatic)) {
                lastExpr_ = addr;
                return;
            }
            const std::string irT = mapType(stType.empty() ? "整64" : stType);
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, irT, "", node->location);
            return;
        }
        // 第 4 层（v2.0 决策9，P1-4）：顶层常量引用——编译期常量折叠（584-a：
        //   移至变量家族之后——局部/静态局部/顶层静态均未命中才内联常量）。
        //   常量名 -> 字面量值文本（语义层 globalConstValue 查询），直接生成
        //   常量加载（ConstInt/ConstFloat/ConstString），避免按变量生成栈槽。
        if (semantic_ != nullptr) {
            // 1360 结果化批3B：无值=非常量名——value_or 显形空串回落（empty 判据保持）
            const std::string constText =
                semantic_->globalConstValue(node->name).value_or(std::string());
            if (!constText.empty()) {
                if (constText.size() >= 2 &&
                    (constText.front() == '"' || constText.front() == '\'')) {
                    // 938（字符串常量全链）：字符串常量内联——原去引号后以值文本
                    //   直接作 ConstString extra → codegen `lea X, AB`（值文本被当
                    //   符号名·ml64 undefined symbol A2006·字符串常量全形态无法
                    //   构建）；与字面量路径同口径（decodeString 转义解码 +
                    //   internString 池化·938 探针实录）
                    lastExpr_ = emitResult(
                        ir::Opcode::ConstString, {}, "ptr",
                        "@str" + std::to_string(internString(decodeString(constText))),
                        node->location);
                } else if (constText == "真" || constText == "假") {
                    // 239-a：布尔常量（内建 调试模式 等）——纯值返回不发射指令（对齐
                    //   BoolLiteral 形态）：isConstant 使 如果(常量) 走 genIfConst 直取
                    lastExpr_ = ir::IRValue::constant(constText, "i1");
                } else if (constText.find_first_of(".eE") != std::string::npos) {
                    lastExpr_ = emitResult(ir::Opcode::ConstFloat, {}, "f64", constText,
                                           node->location);
                } else {  // 整数常量引用发射（域内 i64 直存/超承载 128 双半·1073 单点）
                    emitConstRefIntLiteral(constText, node->location);
                }
                return;
            }
        }
        // 未找到变量：可能是函数名（函数指针赋值）。生成函数地址。
        // 防御性：若连函数也不是（语义已报错），仍生成FuncAddr避免IR中断
        // Task 2.10 重载：函数名作值（回调 = 加）须用决议后的签名 key——
        //   定义处符号按 mangledName（名#参数串）发射，此处取首个签名保持一致；
        //   无参函数/单版本函数 sigKey 即纯名，行为不变
        std::string funcSym = node->name;
        if (semantic_ != nullptr) {
            // 1360 结果化批3B：无值=无同名条目——保持原名回落
            const cn::Optional<std::string> sig =
                semantic_->funcFirstSigKey(node->name);
            if (sig.has_value()) funcSym = sig.value();
        }
        lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr", funcSym, node->location);
        return;
    }
    // 1360 结果化：命中路径——变量必已登记（与 lookupVar 同栈查询），value_or("")
    //   显形（登记 srcType/uniqueName 为空串的合法面行为不变）
    const std::string unique = lookupVarName(node->name).value_or("");
    const std::string srcType = lookupSrcType(node->name).value_or("");
    // 1360 结果化：命中路径重建 IR 形态（regId/type 自条目——原 lookupVar 返回
    //   IRValue::reg(regId,type) 同构）
    const ir::IRValue reg =
        ir::IRValue::reg(regVar.value().regId, regVar.value().type);
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
// 933（先拆再加·本文件 903 行超 900 警戒线）：isStringTypedExpr 实现迁
//   ir_expr_concat.cpp（连接族的判据消费主文件·同轮补 CallExpr 覆盖）。
void IRGenerator::visitBinaryExpr(BinaryExpr* node) {
    // 176-a：本函数 269 行按「短路求值 / 字符串连接族 / 指针算术 / 公共类型
    //   算术」提取为 5 个族子方法（纯搬运零行为变更——逐行核验）。
    // ---- 逻辑与/或短路求值（2026-09-08 根治，缺陷零容忍；rustc 同构手法）----
    //   &&/|| 在 IR 生成期脱糖为控制流（RHS 带副作用时按短路语义执行）。
    //   And/Or 指令保留：编译器内部布尔组合（空/边界检查）不受影响。
    if (genShortCircuitBinary(node)) return;
    // ---- 728o（366 遗留·连接链融合）：整条字符串连接链一趟构建 ----
    //   须在左右操作数生成之前接管（内层 concat 一旦发射无从回退改写）；
    //   链长不足/非连接形态返回 false 交回原逐节路径（零行为变更）。
    if (genStringConcatChain(node)) return;
    ir::IRValue left = genExpr(node->left.get());
    ir::IRValue right = genExpr(node->right.get());
    // ---- 阶段3 OOP（Task 3.7）：运算符重载降级为成员方法调用 ----
    //   须在字符串连接/指针算术分支之前（语义层重载决议优先于隐式转换）。
    if (handleOperatorOverload(node, left, right)) {
        return;
    }
    bool isFloat = (left.type == "f32" || left.type == "f64" ||
                    right.type == "f32" || right.type == "f64");
    // ---- 字符串连接族（Task 2.5/2.9 + i128 段）：ptr + 数值/i128 -> 运行时拼接 ----
    if (genStringConcatBinary(node, left, right)) return;
    if (genStringConcatI128(node, left, right)) return;
    // ---- 字符串比较族（962·166 立法）：字符串×字符串 六比较符 → helper ----
    if (genStringCompareBinary(node, left, right)) return;
    // ---- 指针算术（Task 2.4）：ptr ± 整型 -> 指针（偏移量×元素大小） ----
    if (genPointerArithmetic(node, left, right)) return;
    // ---- 公共类型转换 + 算术/比较发射（92-a 浮点公共类型）----
    genCommonTypeArithmetic(node, left, right, isFloat);
}

// ==================== 176-a 族子方法（原 visitBinaryExpr 316~582 段） ====================

// 族①：逻辑与/或短路求值（原 316~369 段；rustc 同构手法）。true = 已处理。
//   a && b : 结果槽预置 假 -> a 真? 求值 b 存槽 : 直达汇合
//   a || b : 结果槽预置 真 -> a 真? 直达汇合 : 求值 b 存槽
bool IRGenerator::genShortCircuitBinary(BinaryExpr* node) {
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
        return true;
    }
    return false;
}

// ==================== 族⑤（962·166 立法）：字符串比较（==/!= 内容·四序字典序） ====================
// （001 §比较语义修订版·用户裁决 2026-10-02）字符串×字符串 的六比较符分派 runtime
//   helper：==/!= → __cn_str_eq（内容相等·布尔）；</<=/>/>= → __cn_str_cmp
//   （字典序 i64）与 0 比较。判定依据=AST 源类型双侧字符串（IR 层 ptr×ptr 与
//   真指针不可区分）；判空 s==无（右侧=空类型* 字面量）不经本族·仍走指针通道。
bool IRGenerator::genStringCompareBinary(BinaryExpr* node, const ir::IRValue& left,
                                         const ir::IRValue& right) {
    const bool isCmp = (node->op == Operator::EqualEqual ||
                        node->op == Operator::BangEqual ||
                        node->op == Operator::Less ||
                        node->op == Operator::Greater ||
                        node->op == Operator::LessEqual ||
                        node->op == Operator::GreaterEqual);
    if (!isCmp) return false;
    if (left.type != "ptr" || right.type != "ptr") return false;
    if (!isStringTypedExpr(node->left.get()) || !isStringTypedExpr(node->right.get())) {
        return false;
    }
    if (node->op == Operator::EqualEqual || node->op == Operator::BangEqual) {
        ir::IRValue eq = emitResult(ir::Opcode::Call, {left, right}, "i1",
                                    "__cn_str_eq", node->location);
        // 758（2026-10-15）：比较位连接产物消费即释——`甲+乙 == 丙+丁;` 两侧
        //   连接产物为堆串（拥有契约），唯一消费者=本 __cn_str_eq（纯函数·
        //   只读内容），调用后零引用；645/663/732 面只收语句位最终值（比较
        //   结果=布尔不命中断否）与连接生成点内层产物，比较位两侧操作数在
        //   三判定面之外=每语句净泄 2（百轮泄 200·758 立案实录）。消费点
        //   单点收口（与 732 生成点收口同哲学）；定义点溯源守卫=仅拼接产物
        //   寄存器（变量 load/转换临时/字面量天然排除防误伤）。
        //   freeIfConcatTemp 展开形态（758 附属：成员壳随 hpp 腾行降级删除
        //   ——守卫=ptr 寄存器+isConcatResultInst 定义点溯源·两调用点同构）。
        if (left.type == "ptr" && isConcatResultInst(currentBlock_, left.id)) {
            emit(ir::Opcode::Call, {left}, ir::IRValue(), "__cn_str_free", "void", node->location);
        }
        if (right.type == "ptr" && isConcatResultInst(currentBlock_, right.id)) {
            emit(ir::Opcode::Call, {right}, ir::IRValue(), "__cn_str_free", "void", node->location);
        }
        if (node->op == Operator::EqualEqual) {
            lastExpr_ = eq;
        } else {
            lastExpr_ = emitResult(ir::Opcode::Not, {eq}, "i1", "",
                                   node->location);
        }
        return true;
    }
    ir::IRValue ord = emitResult(ir::Opcode::Call, {left, right}, "i64",
                                 "__cn_str_cmp", node->location);
    // 758：四序比较符（</<=/>/>=）同面收口（__cn_str_cmp 消费·同 eq 注释）
    if (left.type == "ptr" && isConcatResultInst(currentBlock_, left.id)) {
        emit(ir::Opcode::Call, {left}, ir::IRValue(), "__cn_str_free", "void", node->location);
    }
    if (right.type == "ptr" && isConcatResultInst(currentBlock_, right.id)) {
        emit(ir::Opcode::Call, {right}, ir::IRValue(), "__cn_str_free", "void", node->location);
    }
    ir::IRValue zero = ir::IRValue::constant("0", "i64");
    ir::Opcode cmpOp = (node->op == Operator::Less) ? ir::Opcode::Lt
                     : (node->op == Operator::Greater) ? ir::Opcode::Gt
                     : (node->op == Operator::LessEqual) ? ir::Opcode::Le
                     : ir::Opcode::Ge;
    lastExpr_ = emitResult(cmpOp, {ord, zero}, "i1", "", node->location);
    return true;
}

// ==================== 族②③：字符串连接（Task 2.5/2.9/128 位段） ====================
// 728o：族实现已拆出 src/cn_compiler/ir/ir_expr_concat.cpp（按族拆分——该文件
//   同时承载连接链融合 pass）；调用点（visitBinaryExpr）与声明（ir_generator.hpp）
//   不变。拆分=纯机械搬移 + 数值转换段提取共享助手，行为零变更。
// 758 附属（2026-10-15）：freeIfConcatTemp 成员壳随 hpp 腾行降级删除——守卫
//   逻辑（ptr 寄存器+isConcatResultInst 定义点溯源）已在 758 比较位调用点
//   展开形态（genStringCompareBinary 内·逐指令等价）；判据实现=
//   ir_string_temp_util.hpp 纯自由函数（全注在该头）。

// ==================== 族④：指针算术（Task 2.4） ====================
// （原 480~515 段）ptr ± 整型 -> 指针（偏移量×元素大小）。true = 已处理。
// 1064（2026-10-18·#1040 派生缺口·表达式操作数扩展）：指针差/指针算术操作数
//   的指针源类型回溯——语义层 1040 判可面（isJudgedPtrDiffOperand1040）放行
//   的形态在此恒可解析：
//   ①标识符（指针变量/数组名退化）→ lookupSrcType 直查变量表（数组名返数组
//     全文，ptrElemStride 数组分支自管元素宽——425 同源）；
//   ②二元 ±（指针在左为主（p±n/p-q）——右指针形态（n+p）语义层已拒；仍
//     对称回溯，仅当回溯结果确为指针/数组形态才采信，防错采整数侧源文本）。
//   其余（成员链/取地址/调用返回/下标等不可判面）无值——调用方交给
//   ptrElemStride 的 648 内部错误显形（不静默取默认值）。与 v2 指针值表达式
//   元素1064 结构面同构（v2 面合判可+元素，本处仅源类型回溯）。
//   1360 结果化：无值=不可判面（原空串哨兵退役）。
cn::Optional<std::string> IRGenerator::ptrOperandSrcType(const Expr* e) const {
    if (e == nullptr) return std::nullopt;  // 空节点=非指针形态（判空谓词·调用点恒传非空子树）
    if (e->getType() == NodeType::IdentifierExpr) {
        return lookupSrcType(static_cast<const IdentifierExpr*>(e)->name);
    }
    if (e->getType() == NodeType::BinaryExpr) {
        const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
        if (b->op == Operator::Add || b->op == Operator::Subtract) {
            const cn::Optional<std::string> left = ptrOperandSrcType(b->left.get());
            if (left.has_value() && !left.value().empty() &&
                (types::isPointer(left.value()) || types::isArray(left.value()))) {
                return left;
            }
            const cn::Optional<std::string> right = ptrOperandSrcType(b->right.get());
            if (right.has_value() && !right.value().empty() &&
                (types::isPointer(right.value()) || types::isArray(right.value()))) {
                return right;
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}

bool IRGenerator::genPointerArithmetic(BinaryExpr* node, const ir::IRValue& left,
                                       const ir::IRValue& right) {
    // 1040（2026-10-18 用户批甲·001 §5.3 清单 #1）：指针差（尾-首）——语义层
    //   已限定同型非字符串语义指针+可判操作数（resolvedType=整64），此处按
    //   (addrL - addrR) / 元素大小 → i64「元素个数」发射（C ptrdiff 同款·
    //   尾在首前得负）。步进口径与下方指针±整数同源（ptrElemStride）。
    //   1064（#1040 派生·表达式操作数扩展）：操作数含 ± 链（(p+1)-p 等）——
    //   源类型经 ptrOperandSrcType 回溯根指针（与语义判可面 isJudgedPtrDiff
    //   Operand1040 同构·合法程序恒可解析）。
    if (left.type == "ptr" && right.type == "ptr") {
        // 非减法（指针比较 ==/!= 等）交上层原路径处理（比较段/数值段自管）——
        //   原双 ptr 在 genPointerArithmetic 恒 return false，此处保持零回归。
        if (node->op != Operator::Subtract) return false;
        // 1360 结果化：无值/空串回落另一侧（原空串检查同义·形态等价）
        cn::Optional<std::string> ptrSrcTypeOpt = ptrOperandSrcType(node->left.get());
        if (!ptrSrcTypeOpt.has_value() || ptrSrcTypeOpt.value().empty()) {
            ptrSrcTypeOpt = ptrOperandSrcType(node->right.get());
        }
        const std::int64_t stride = ptrElemStride(ptrSrcTypeOpt.value_or(""));
        ir::IRValue bytes = emitResult(ir::Opcode::Sub, {left, right}, "i64", "",
                                       node->location);
        lastExpr_ = emitResult(
            ir::Opcode::Div,
            {bytes, ir::IRValue::constant(std::to_string(stride), "i64")},
            "i64", "", node->location);
        return true;
    }
    if ((left.type == "ptr" && right.type != "ptr") ||
        (right.type == "ptr" && left.type != "ptr")) {
        const bool ptrLeft = (left.type == "ptr");
        ir::IRValue ptr = ptrLeft ? left : right;
        ir::IRValue delta = ptrLeft ? right : left;
        // 仅 + / - 允许指针算术（语义层已检查）
        if (node->op != Operator::Add && node->op != Operator::Subtract) {
            lastExpr_ = ptr;
            return true;
        }
        // 偏移量转 i64；步进 = 指针所指元素大小（结构体指针按结构体总大小，
        // 普通指针8字节；Task 2.7 修复）
        if (delta.type != "i64") {
            delta = emitResult(ir::Opcode::Cast, {delta}, "i64", "", node->location);
        }
        // 指针操作数的源码类型（1064：经 ptrOperandSrcType 回溯——标识符直查；
        //   ± 链（(p+1)+1 等）回溯根指针；成员/取地址等不可判面无值 →
        //   ptrElemStride 内部错误显形（648 纪律·不静默取默认值））
        //   1360 结果化：value_or("") 显形（不可判面=空串入 stride 报错通道·原语义）
        const std::string ptrSrcType =
            ptrOperandSrcType(ptrLeft ? node->left.get() : node->right.get())
                .value_or("");
        const std::int64_t stride = ptrElemStride(ptrSrcType);
        ir::IRValue scaled = emitResult(
            ir::Opcode::Mul,
            {delta, ir::IRValue::constant(std::to_string(stride), "i64")},
            "i64", "", node->location);
        // 指针 - 整数：减法（delta 取负）；指针 + 整数：加法
        lastExpr_ = emitResult(
            node->op == Operator::Add ? ir::Opcode::Add : ir::Opcode::Sub,
            {ptr, scaled}, "ptr", "", node->location);
        return true;
    }
    return false;
}

// ==================== 族⑤：公共类型转换 + 算术/比较发射（92-a 浮点公共类型） ====================
// （原 516~582 段）left/right 经 Cast 后按 opcode 发射；left/right 为引用进出。
void IRGenerator::genCommonTypeArithmetic(BinaryExpr* node, ir::IRValue& left,
                                          ir::IRValue& right, bool isFloat) {
    // 377 乙案（001 §3.7a 全程窄域·用户批乙 2026-10-10·681 探针）：窄算术
    //   判定须在混合提升**前**捕获原始类型（下方 left/right 引用会被 Cast
    //   改写）——同窄双侧行、或单侧窄+对侧 i32 整字面量（285 字面量窄参与
    //   的 IR 面同口径·语义面已同判）→算术结果 Cast 回窄（每步回绕）。
    //   条文面=+ - * / %（位/移位/比较不涉窄域条文不动）；浮点/128/i1 不在
    //   窄族。复用 emitCast 窄化域（358 判定层单点·三后端自动同构）。
    const auto isNarrowIR377 = [](const std::string& t) {
        return t == "i8" || t == "u8" || t == "i16" || t == "u16";
    };
    const auto isIntLit377 = [](const Expr* e) -> bool {
        if (e == nullptr) return false;
        if (e->getType() == NodeType::IntegerLiteral) return true;
        if (e->getType() == NodeType::UnaryExpr) {
            const auto* u = static_cast<const UnaryExpr*>(e);
            return u->op == Operator::Subtract && !u->postfix &&
                   u->operand != nullptr &&
                   u->operand->getType() == NodeType::IntegerLiteral;
        }
        return false;
    };
    // 862（#862·001 §3.7a 285/788③「字面量形态（含带后缀）」）：285 拉窄的
    //   字面量侧承载门——原为 `type == "i32"`（仅无后缀字面量），带后缀字面量
    //   （L→i64/U→u32/UL→u64）与超 i32 域无后缀字面量（i64）漏网：`甲8+3e9L`
    //   走混合提升宽算+存回截断（**无窄域溢出检查**——边界 甲127+3000000002L
    //   静默输出 1；474 立法要求结果超操作数窄域即错误码 8=静默截断漏网根治）。
    //   放宽为「≤64 位整数族字面量承载」：值面零变化（拉窄截断与宽算存回截断
    //   模算术等价），仅补全窄域溢出检查面；与语义层 285 拉窄（isIntLiteralExpr
    //   含任意后缀）+ v2 拉窄通道三口径归一。i128/u128（超 64 域 LL/ULL 物化
    //   字面量）不在列——128 通道维持既有路径（另行登记）。
    const auto isNarrowLitCarrier862 = [](const std::string& t) {
        return t == "i32" || t == "i64" || t == "u32" || t == "u64" ||
               t == "i8" || t == "i16" || t == "u8" || t == "u16";
    };
    bool narrowResult377 = false;
    std::string narrowType377;
    if (!isFloat) {
        if (isNarrowIR377(left.type) && isNarrowIR377(right.type)) {
            narrowResult377 = true;
            narrowType377 = left.type;
        } else if (isNarrowIR377(left.type) && isNarrowLitCarrier862(right.type) &&
                   isIntLit377(node->right.get())) {
            narrowResult377 = true;
            narrowType377 = left.type;
        } else if (isNarrowIR377(right.type) && isNarrowLitCarrier862(left.type) &&
                   isIntLit377(node->left.get())) {
            narrowResult377 = true;
            narrowType377 = right.type;
        }
    }
    // 公共类型（浮点优先 f64；整型取 rank 高者由语义层保证可转换）
    // 92-a：浮点算术公共类型（f32 op f32 -> f32 单精度；跨类型统一 f64）
    std::string floatCommon;
    if (narrowResult377) {
        // 任务 474 改造（377 乙案升级）：窄域算术（窄+窄 / 窄+i32 字面量）——
        //   字面量操作数 Cast 到窄域后**直接按窄域运算**（原实现 i32 域运算+
        //   尾部 Cast 回窄：回绕位模式等价，但 474 溢出检查按指令类型发射，
        //   i32 域运算检测不到窄域溢出——32767+1(整16) 静默回绕实锤）。
        //   窄域运算的截断存储与回绕行为不变（位模式等价），溢出面归 474。
        if (left.type != narrowType377) {
            left = emitResult(ir::Opcode::Cast, {left}, narrowType377,
                              "", node->location);
        }
        if (right.type != narrowType377) {
            right = emitResult(ir::Opcode::Cast, {right}, narrowType377,
                               "", node->location);
        }
    } else if (!isFloat && left.type != right.type &&
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
        // 377 乙案尾部：窄算术（+ - * / %）结果 Cast 回窄——链式中间值每步
        //   回绕（(x*2)/2 的 400 截断为 144 再除=72·立法口径）；单步赋值锚
        //   p1007_05 观测等价（存回槽截断点前移同值）。
        //   任务 474：窄域算术已按窄域发射（操作数预 Cast 同型）——结果类型
        //   已窄，仅类型不一致的防御兜底形态才补 Cast。
        if (narrowResult377 && lastExpr_.type != narrowType377 &&
            (node->op == Operator::Add || node->op == Operator::Subtract ||
             node->op == Operator::Multiply || node->op == Operator::Divide ||
             node->op == Operator::Modulo)) {
            lastExpr_ = emitResult(ir::Opcode::Cast, {lastExpr_}, narrowType377,
                                   "", node->location);
        }
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
    // 305（变量形态）：实参=局部变量标识符——语义层已按变量声明类型回填 size
    //   （isVarForm 置位），typeName 是变量名按类型名重解析无意义 → 直用 size
    if (node->isVarForm) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i64",
                               std::to_string(node->size), node->location);
        return;
    }
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
        size = semantic_->typeSizeOf(resolved).value_or(8);
    }
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i64",
                           std::to_string(size), node->location);
}
void IRGenerator::visitCastExpr(CastExpr* node) {
    ir::IRValue operand = genExpr(node->operand.get());
    const std::string target = mapType(node->targetType);
    // 274-a T15 残余面：同型显式转换=恒等透传不发射 Cast——原对 浮64(浮64
    //   表达式) 也发射 Cast(f64,f64)，曾驱动后端浮↔浮分派误走单精度舍入
    //   （7.0→7.000001·双级别一致值污染）。同型 no-op（Rust as 同型语义）；
    //   空类型文本（防御）回落原发射路径。真转换（异型）不受影响。
    if (!operand.type.empty() && operand.type == target) {
        lastExpr_ = operand;
        return;
    }
    // 533（475 批甲·2026-10-14 用户裁决）：浮→字符显式转换饱和化专用通道——
    //   超域按 Unicode 标量有效域 [0, 0x10FFFF] 钳制、NaN→0（528 浮→整族同构·
    //   runtime __cn_fXX_to_char 同源）。字符目标在 IR 类型层坍缩 i32
    //   （mapType("字符")="i32"），与 整32(浮) 目标文本不可区分——若落通用
    //   Cast 发射会按 i32 域饱和（超域→2147483647 非 0x10FFFF 域分叉）。故
    //   此处对「字符(浮源)」直发 runtime 饱和辅助调用（结果类型 i32=字符槽
    //   语义·win EXTERN 经 Call extra 自动收集）。整源→字符维持通用 Cast
    //   位模式互通（§3.7 字符↔整数·本轮不扩面）。
    if ((operand.type == "f32" || operand.type == "f64") &&
        types::canonicalEquals(node->targetType, "字符")) {
        lastExpr_ = emitResult(ir::Opcode::Call, {operand}, "i32",
                               (operand.type == "f64") ? "__cn_f64_to_char"
                                                       : "__cn_f32_to_char",
                               node->location);
        return;
    }
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
// 852 收工解堵（2026-10-16）：打印/打印行变参展开（自 visitCallExpr 纯机械搬移·
//   函数体逐字保留零语义变化）。语义（用户裁决 lessons 权重10.4）：打印=println
//   末尾换行；打印行=print 不换行。617 拥有串临时实参只读消费+语句尾结算随族走。
void IRGenerator::emitPrintLikeCall(CallExpr* node, bool isPrintNewline,
                                bool isPrintNoLine) {

    // 617（2026-10-14 伞批）：拥有串临时实参登记——打印/打印行对实参只读
    //   消费（fputs），调用完成后调用方释放（001 §10.3 拼接语义「结果由
    //   调用方负责字符串释放」的变参通道兑现）。判据=argIsOwnedStringTemp
    //   （CallExpr 且返回字符串）：字面量（只读段·free 即崩）与标识符
    //   （调用方变量自有 RAII）天然不命中——原实现 `打印("x=",
    //   整数转字符串(42))` 的临时串每语句净漏 1。
    //   662（617 三元面补全）：三元实参按「分支拥有性感知」条件释放（结算入口
    //   emitOwnedTempReleaseRec617——选中字面量分支不得 free），收集项携带来源 AST。
    std::vector<std::pair<const Expr*, ir::IRValue>> ownedStrTemps617;
    for (auto& arg : node->arguments) {
        ir::IRValue argVal = genExpr(arg.get());
        if (argIsOwnedStringTemp(arg.get()) && argVal.type == "ptr") {
            ownedStrTemps617.emplace_back(arg.get(), argVal);
        }
        // 参数类型 -> 打印函数名（字符串/字符* -> 字符串；整型 -> 整数；浮点 -> 浮点）
        std::string printFn;
        if (argVal.type == "ptr") {
            printFn = "__cn_print_str";
        } else if (argVal.type == "f32" || argVal.type == "f64") {
            // 浮32 必须先 Cast 浮64 再传 __cn_print_float(double)——
            // 否则 ABI 不匹配（xmm0 低32位垃圾被当 double 读，输出 0.000000）
            // 缺陷修复（方案C 2026-08-14）：原 打印行浮点(f32) 靠语义层隐式宽化，
            //   改 打印(f32) 变参展开后此 Cast 必须显式
            if (argVal.type == "f32") {
                argVal = emitResult(ir::Opcode::Cast, {argVal}, "f64", "",
                                    node->location);
            }
            printFn = "__cn_print_float";
        } else if (argVal.type == "i128" || argVal.type == "u128") {
            // i128/正128（Task 完善A）：传双寄存器地址（低64位槽地址），
            // 运行时辅助函数 __cn_print_i128 读 16 字节双槽
            printFn = (argVal.type == "u128") ? "__cn_print_u128"
                                              : "__cn_print_i128";
        } else if (argVal.type == "i1") {
            // 2026-08（用户裁决，布尔打印统一）：布尔（含比较/逻辑表达式
            //   结果）经 打印 统一输出 真/假（__cn_print_bool）——此前 i1
            //   走 __cn_print_int 输出 1/0，与字符串拼接 布尔转字符串 不一致
            printFn = "__cn_print_bool";
        } else {
            // 整型（含 i1 布尔）：有符号统一 Cast i64 走 __cn_print_int；
            // 无符号（正8~正64）直接传 __cn_print_uint（%llu 语义，缺陷修复：
            //   原实现统一 __cn_print_int（%lld 有符号），正64 值超 2^63 时
            //   位模式按有符号解释打印成负数）
            const bool isUnsignedArg = (argVal.type == "u8" ||
                                        argVal.type == "u16" ||
                                        argVal.type == "u32" ||
                                        argVal.type == "u64");
            if (isUnsignedArg) {
                if (argVal.type != "u64") {
                    argVal = emitResult(ir::Opcode::Cast, {argVal}, "u64", "",
                                        node->location);
                }
                printFn = "__cn_print_uint";
            } else {
                if (argVal.type != "i64") {
                    argVal = emitResult(ir::Opcode::Cast, {argVal}, "i64", "",
                                        node->location);
                }
                printFn = "__cn_print_int";
            }
        }
        emit(ir::Opcode::Call, {argVal}, ir::IRValue(), printFn, "void",
             node->location);
    }
    // 打印（println 语义）末尾统一换行；打印行（print 语义）不换行
    if (isPrintNewline) {
        emit(ir::Opcode::Call, {}, ir::IRValue(), "__cn_print_newline", "void",
             node->location);
    }
    // 617：语句尾结算——打印已完成只读消费，释放全部拥有串临时
    //   （寄存器句柄直传 free，无槽无清零义务；662 三元实参走分支拥有性感知
    //   条件释放，非三元走无条件释放原路——单一入口 Rec617）
    for (const auto& tmp617 : ownedStrTemps617) {
        emitOwnedTempReleaseRec617(tmp617.first, tmp617.second, node->location);
    }
    lastExpr_ = ir::IRValue();  // 无返回值（空类型）
    (void)isPrintNoLine;   // 对称参数：换行分支仅消费 isPrintNewline（保留签名对称）
    return;
}

} // namespace cn_compiler
