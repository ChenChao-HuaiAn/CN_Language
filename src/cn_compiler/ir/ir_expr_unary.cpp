// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出）
//   族 = 一元/三元运算（visitUnaryExpr + visitTernaryExpr——91-a 浮点分派、C-1 错误传播、位非/取负/自增自减）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   族边界勘定：族内无文件级 static/匿名命名空间依赖，见 plans/021 §3-D1）。
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

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
            // 269-c/273-a T16：浮点操作数改 x * (-1.0) —— IEEE 符号翻转语义
            //   （-(+0.0)=-0.0；原「0 - x」降级=0−(+0)=+0 丢负零·深度机浮点
            //   矩阵 T16 立案）；整型保持 0 - x（两补码取负无负零面）
            if (operand.type == "f64" || operand.type == "f32") {
                ir::IRValue negOne = emitResult(ir::Opcode::ConstFloat, {},
                                                ir::IRValue::constant(
                                                    "-1.0", operand.type).type,
                                                "-1.0", node->location);
                lastExpr_ = emitResult(ir::Opcode::Mul, {operand, negOne},
                                       operand.type, "", node->location);
                break;
            }
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
            // 320-a（T41 写面完备）：函数内静态局部自增自减——读-算-写回
            //   （?gstatic_键 符号）。原实现落下方普通路径：lookupVar 对静态局部
            //   返回 regId=-1，`slot.id>=0` 写回守卫不成立=静默不写回（桩++ 恒旧值）。
            //   置于类字段/顶层静态分支之前=varStack 命中优先（遮蔽语义对齐读路径：
            //   同名局部静态遮蔽顶层静态/类字段）。
            if (node->operand->getType() == NodeType::IdentifierExpr) {
                const VarEntry* slVe = findVarEntry(
                    static_cast<IdentifierExpr*>(node->operand.get())->name);
                if (slVe != nullptr && slVe->isStaticLocal) {
                    const std::string irT = slVe->type;
                    ir::IRValue gsAddr = emitResult(
                        ir::Opcode::ConstString, {}, "ptr",
                        "?gstatic_" + slVe->uniqueName, node->location);
                    ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {gsAddr}, irT, "",
                                                 node->location);
                    ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, "i64", "1",
                                                   node->location);
                    ir::IRValue res = emitResult(
                        node->op == Operator::Increment ? ir::Opcode::Add
                                                        : ir::Opcode::Sub,
                        {cur, delta}, irT, "", node->location);
                    emit(ir::Opcode::StorePtr, {gsAddr, res}, ir::IRValue(), "", irT,
                         node->location);
                    // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
                    lastExpr_ = node->postfix ? cur : res;
                    break;
                }
            }
            // 缺陷5 修复：类字段（静态/实例）自增自减——字段名不在 varStack_，
            //   须走"读-算-写回"专用路径（原实现只读不写，静态字段 总数++ 恒 0）
            if (node->operand->getType() == NodeType::IdentifierExpr &&
                handleClassFieldIncDec(
                    static_cast<IdentifierExpr*>(node->operand.get()),
                    node->op, node->location, node->postfix)) {
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
                    // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
                    lastExpr_ = node->postfix ? cur : res;
                    break;
                }
            }
            // 自增/自减：数值 x = x ± 1；指针 x = x ± 元素大小（Task 2.4）
            // 92-a 根治（H2）：结果类型=操作数类型（原恒 "ptr"——浮点自增走整型
            //   Add，只改 IEEE754 位模式最低位使 ++ 对浮点静默失效；写回按 8 字节
            //   存 4 字节槽溢出）。浮点 delta 用同精度 1.0（ConstFloat 按类型入
            //   常量池，浮32 得 1.0f）。
            const bool floatIncOperand = (operand.type == "f32" ||
                                          operand.type == "f64");
            std::string deltaText = floatIncOperand ? "1.0" : "1";
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
            ir::IRValue delta = emitResult(
                floatIncOperand ? ir::Opcode::ConstFloat : ir::Opcode::ConstInt, {},
                floatIncOperand ? operand.type : "i64", deltaText, node->location);
            // 331-a（T51）：128 位操作数的 delta 须宽化——原恒 i64 → Add/Sub(i128, i64)
            //   未宽化 → -O0 发射静默不生效（O1+ 优化层掩盖=级别分叉）；复用复合
            //   赋值同一辅助（单一归属）。
            delta = widenCompoundRhs(delta, operand.type, node->location);
            ir::IRValue result = emitResult(
                node->op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                {operand, delta}, operand.type, "", node->location);
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
                             ir::IRValue(), "", operand.type, node->location);
                    } else {
                        emit(ir::Opcode::Store, {result}, ir::IRValue(),
                             lookupVarName(ident->name), operand.type, node->location);
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
                // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
                lastExpr_ = node->postfix ? operand : incdecResult;
                break;
            }
            // T24（297-a·根治）：后缀 i++ 表达式值=自增前的旧值（C/C++ 语义；
            //   前缀 ++i=自增后的新值）——原实现忽略 postfix 一律取新值
            lastExpr_ = node->postfix ? operand : result;
            break;
        }
        case Operator::AddressOf: {
            // 取地址 &变量（Task 2.4）：AddrOf(变量引用) -> 变量地址
            // 数组名作 & 操作数：数组名已是首元素地址，&数组 与 数组名 等价
            // （C语义；此处直接生成 AddrOf 取变量槽地址）
            const std::string operandName =
                node->operand->getType() == NodeType::IdentifierExpr
                    ? static_cast<IdentifierExpr*>(node->operand.get())->name
                    : "";
            const std::string unique = lookupVarName(operandName);
            if (node->operand->getType() == NodeType::IdentifierExpr && !unique.empty()) {
                // 缺陷修复（[&] 引用捕获 &变量）：参数槽存被捕获变量地址，
                //   &捕获变量 = Load 参数槽（取被捕获变量地址，而非参数槽自身地址）
                if (node->refWrapAddr) {
                    // 784-a（550 阶段二·A-1 wrap 契约精确化·分支提前）：语义层
                    //   wrapRefArgs 包装节点=内部引用机制形态——产物=「操作数所
                    //   绑定 place 的地址」：①操作数=普通变量（调用方直传）→
                    //   AddrOf(变量槽)=place 地址；②操作数=引用形参（被调方实参
                    //   再传递）→Load(源槽)=源槽内容=被绑定 place 地址（AddrOf
                    //   会错成源槽自身地址·776-h 实验② v2p 符号表 设置 链崩实锤）。
                    //   须先于 isByRefCapture 判定（wrap 节点操作数恒为引用形参·
                    //   二者同时命中·契约优先）。
                    if (isByRefCapture(operandName)) {
                        lastExpr_ = emitResult(ir::Opcode::Load,
                                               {ir::IRValue::var(unique, "ptr")},
                                               "ptr", unique, node->location);
                    } else {
                        lastExpr_ = emitResult(ir::Opcode::AddrOf,
                                               {ir::IRValue::var(unique, operand.type)},
                                               "ptr", unique, node->location);
                    }
                } else if (isByRefCapture(operandName)) {
                    // 776 终（550 阶段二·甲案）：用户显式 &引用（引用形参/引用
                    //   局部·byRef 槽存被引用 place 地址）的 & 求值=**堆对象地址**
                    //   （两跳：Load 槽=place 地址→LoadPtr place=容器/类句柄·标量
                    //   引用单跳即 place 地址不变）——p3/p8/p10 迭代器指针字段
                    //   形态根治点（&引用形参 存指针字段→指针成员调用 this=
                    //   堆对象地址·大小=2）；闭包捕获（非引用类型）单跳槽地址
                    //   语义字节级保持（[&] 契约）。
                    const std::string capSrc =
                        types::canonical(types::stripRef(lookupSrcType(operandName)));
                    const bool capIsObj =
                        (semantic_ != nullptr &&
                         (semantic_->isClassType(capSrc) ||
                          capSrc.rfind("向量$", 0) == 0 ||
                          capSrc.rfind("映射$", 0) == 0 ||
                          capSrc.rfind("链表$", 0) == 0 ||
                          capSrc.rfind("栈$", 0) == 0 ||
                          capSrc.rfind("队列$", 0) == 0 ||
                          capSrc.rfind("集合$", 0) == 0));
                    ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                                     {ir::IRValue::var(unique, "ptr")},
                                                     "ptr", unique, node->location);
                    if (capIsObj) {
                        // 784-a（550 阶段二·codegen 槽语义对齐）：第二跳必须
                        //   **LoadPtr**（解引用语义+空检查）而非 Load——codegen
                        //   emitLoadStore 对「Load 寄存器操作数（id>=0）」发射
                        //   寄存器槽复制（不解引用），q2 得 place 栈地址（p10
                        //   大小=栈垃圾·gdb q2=0x7ffd… 实锤）；与
                        //   visitIdentifierExpr byRef 读值路径（Load+LoadPtr）
                        //   同构·与 A 链形态一致（Rust &mut transparent·堆对象
                        //   地址）。
                        capAddr = emitResult(ir::Opcode::LoadPtr, {capAddr},
                                             "ptr", "", node->location);
                    }
                    lastExpr_ = capAddr;
                } else if (types::isReference(lookupSrcType(operandName))) {
                    // T94（550-a）：引用变量取地址（&引用参数/&引用局部）=目标地址——
                    //   引用槽存目标地址（别名语义），&v = Load 槽内容（Rust 对照：
                    //   引用为首类指针，&alias 得目标地址而非槽地址）；若走 AddrOf
                    //   (lea 槽) 则槽地址被存入指针字段/传为 this，后续成员方法调用
                    //   读错位（p1/p3 探针 rc=223 实锤）。与 byRef 捕获分支同构；
                    //   列 refWrapAddr 之后=内部引用机制链路（A-1 契约）不受影响。
                    //   判据=语义层源类型文本尾 & 判定（与 588-a 判据同源）。
                    lastExpr_ = emitResult(ir::Opcode::Load,
                                           {ir::IRValue::var(unique, "ptr")},
                                           "ptr", unique, node->location);
                } else if (semantic_ != nullptr &&
                           [&] {
                               // 772 轮（550 阶段二·甲案清单②·用户 2026-09-25 裁决
                               //   甲=&容器变量=对象地址）：**容器类型并入 Load 域**——
                               //   容器（向量/映射/链表/栈/队列/集合）不在 classes_ 表
                               //   （stdlib 内置合成·isClassType 恒假），&容器变量/
                               //   &容器引用 落兜底 AddrOf(槽)=二级语义→`向量<整64>*
                               //   q = &表; q.大小()` 的 this=栈槽地址→方法读槽+8=
                               //   静默错值（p3/p8 探针 大小=0·gdb rdi=栈地址铁证；
                               //   与 588-a 类分支同一机理同款修复·容器=句柄槽模型
                               //   Load 槽即对象地址·C++/Rust &container 同构）。
                               const std::string addrSrc =
                                   types::canonical(types::stripRef(
                                       lookupSrcType(operandName)));
                               if (semantic_->isClassType(addrSrc)) return true;
                               return addrSrc.rfind("向量$", 0) == 0 ||
                                      addrSrc.rfind("映射$", 0) == 0 ||
                                      addrSrc.rfind("链表$", 0) == 0 ||
                                      addrSrc.rfind("栈$", 0) == 0 ||
                                      addrSrc.rfind("队列$", 0) == 0 ||
                                      addrSrc.rfind("集合$", 0) == 0;
                           }()) {
                    // 588-a（005〔原B-T81〕·用户 2026-09-21 裁决甲=取地址语义
                    //   一致化）：用户显式 &类变量 产**对象地址**——类=指针槽模型
                    //   （变量槽存句柄），Load 槽即对象地址，与 点* 类型标注一级
                    //   语义对齐（C++/Rust 对照：&obj 即对象地址）。原无条件
                    //   AddrOf(槽)=指向槽本身（二级语义），`点* p2 = &p1; p2.读x()`
                    //   把槽地址当对象地址解引用=静默垃圾值（probe_f1 实锤·E2E
                    //   零覆盖）。判据与整体赋值源分支同款（ir_expr_member.cpp：
                    //   结构体 AddrOf 槽 / 类 Load 槽）。
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
                       semantic_->isGlobalStatic(operandName)) {
                // plans/018 根治（2026-09-07，缺陷零容忍）：顶层静态变量取地址——
                //   ?gstatic_名 符号地址即变量槽地址（.data 槽本身=存储位置），直接
                //   以符号地址为取地址结果。原落 else 读值兜底（genExpr→LoadPtr 读
                //   槽内容）：引用形参实参经语义层 wrapRefArgs 包装为 &静态 后，此处
                //   取地址退化成读值——引用形参收到的是槽内容（容器=句柄值），被调
                //   方对引用形参的读取=再解引用一次（空容器句柄 0=错误码3；非空=
                //   读写容器对象头=静默错位）。v2self 语义分析多文件(6 个 & 静态
                //   容器实参) 首次踩中。Rust 对照：&static mut 的引用恒指向存储
                //   位置本身，绝不解引用重解释。
                // 588-a（005 甲案）：用户显式 &静态**类**变量 与局部同口径语义
                //   一致化——静态槽存句柄，LoadPtr 符号槽=对象地址（一级语义）；
                //   包装节点（refWrapAddr）与非类静态维持符号地址（槽地址）不变。
                const std::string stSrc = semantic_->globalStaticType(operandName);
                const std::string stCanon =
                    types::canonical(types::stripRef(stSrc));
                if (!node->refWrapAddr &&
                    (semantic_->isClassType(stCanon) ||
                     // 772 轮（550 阶段二）：静态容器变量与局部同口径（容器不在
                     //   classes_ 表·甲案 LoadPtr 符号槽=对象地址）
                     stCanon.rfind("向量$", 0) == 0 ||
                     stCanon.rfind("映射$", 0) == 0 ||
                     stCanon.rfind("链表$", 0) == 0 ||
                     stCanon.rfind("栈$", 0) == 0 ||
                     stCanon.rfind("队列$", 0) == 0 ||
                     stCanon.rfind("集合$", 0) == 0)) {
                    const ir::IRValue sym = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                                       "?gstatic_" + operandName,
                                                       node->location);
                    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {sym}, "ptr",
                                           "", node->location);
                } else {
                    lastExpr_ = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                           "?gstatic_" + operandName,
                                           node->location);
                }
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
    // 559-a（T96b）：槽类型/槽数按语义层注记分配——m44_01 实弹：i128 真值写入
    //   硬编码 i64 单槽=越槽写 8 字节，汇合块 Load 16 字节读出栈垃圾（O0 巧合
    //   对/O3 布局重排读垃圾）。i128/u128=双槽（对齐 ir.cpp:423 i128 变量模型）；
    //   注记为空（语义未跑到/未知）保持 i64 单槽回退。
    std::string tempName = "__ternary$" + std::to_string(varCounter_++);
    std::string slotIRType = "i64";
    int slotCount = 1;
    if (!node->semanticType.empty()) {
        const std::string mapped = mapType(node->semanticType);
        if (!mapped.empty() && mapped != "未知") {
            slotIRType = mapped;
            slotCount = (mapped == "i128" || mapped == "u128") ? 2 : 1;
        }
    }
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, slotIRType),
         tempName, slotIRType, node->location);
    function_->varSlots[tempName] = slotCount;

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
    lastExpr_ = emitResult(ir::Opcode::Load,
                           {ir::IRValue::var(tempName, slotIRType)},
                           trueTypeForLoad.type, "", node->location);
}
} // namespace cn_compiler
