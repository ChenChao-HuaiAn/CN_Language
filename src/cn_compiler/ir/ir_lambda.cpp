// CN-IR生成器实现：lambda 闭包族（432 拆分自 ir_expr.cpp·单一职责）
// 职责：
//   visitLambdaExpr —— lambda 字面量 -> 匿名函数 ?lambdaN + 蹦床 ?lambdaN$trap
//     + 快照静态槽固化（432·#276 波3）。
//   闭包值模型（432 蹦床·001 §5.6「lambda 表达式」+spec 04-一D）：
//     lambda 值 = 蹦床函数地址（签名=用户签名·可安全作函数指针返回/实参传递——
//     高阶两面零新增调用面：逃逸调用走既有 fnptr 间接调用通道）；
//     捕获快照固化 .data 静态槽（?gstatic_cap$?lambdaN$<捕获名>：值捕获=定义处
//     值快照；[&]=变量地址）——蹦床体内 LoadPtr 槽还原捕获实参直调 ?lambdaN。
//     同函数闭包变量调用仍走 closureInfo_ 直调 ?lambdaN（captureArgs 定义处值·
//     与蹦床语义等价的内联快径，ir_call.cpp 展开通道零变化）。
//     生命周期：值捕获快照随槽全局存活=闭包逃逸栈帧安全（悬垂零容忍·安全目标②）；
//     [&] 逃逸=悬垂域（#435/#468 逃逸检查面·本波只做值捕获高阶形态·呈报记录）。
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// lambda 快照静态槽键：?gstatic_ 前缀外的注册键（codegen 发射
//   "?gstatic_" + nameMangle(键)——键纯 ASCII 直通）。lambdaName 全局唯一
//   （lambdaCounter_ 递增）保证跨 lambda/跨函数不撞键。
std::string IRGenerator::lambdaSlotKey(const std::string& lambdaName,
                                       const std::string& cap) const {
    return "cap$" + lambdaName + "$" + cap;
}

// lambda 用户参数段构建（?lambdaN 与蹦床 ?lambdaN$trap 共用——纪律二当场归纳；
//   原为 visitLambdaExpr 内联段·432 提炼）。前置条件：function_ 已指向 func、
//   varStack_ 已压入函数作用域。引用参数（T&）按 ptr 槽+byRef 形态（A-1 同款）。
void IRGenerator::buildLambdaUserParams(ir::IRFunction& func,
                                        LambdaExpr* node) {
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
        // 337-a（T53 家系）：lambda 形参同款——函数指针取完整规范串
        //   （paramSrcTypeOf 单一归属，与 ir_decl/ir_generic_func 登记点同口径）。
        entry.srcType = paramSrcTypeOf(param.get());
        entry.byRef = isRefParam;
        varStack_.back()[param->name] = entry;
    }
}

// 快照静态槽注册 + 定义处固化（外层块发射·调用点=visitLambdaExpr 恢复外层后）。
//   值捕获标量：StorePtr(槽地址, 定义处值)；
//   [&] 引用捕获：StorePtr(槽地址, AddrOf(变量))（槽存地址·蹦床还原即引用实参）；
//   结构体值捕获：CopyStruct(槽地址, 源地址)（槽即固化缓冲·与 genVarDecl
//     __capstruct 临时同构——第 2 处重复留标记：固化逻辑与 ir_stmt_decl_init
//     registerClosureIfNeeded 固化段互为镜像，数据流终点不同——captureArgs=局部
//     值列表（同函数内联快径），本处=.data 静态槽（逃逸蹦床径），暂不机械合并）。
//   数组值捕获：槽存基址（数组名退化=元素指针·与 captureArgs 现状同语义）。
void IRGenerator::emitLambdaSnapshotStores(
    const std::string& lambdaName,
    const std::vector<std::string>& capturedNames,
    const std::vector<char>& captureRefs,
    const SourceLocation& loc) {
    for (std::size_t ci = 0; ci < capturedNames.size(); ++ci) {
        const std::string& cap = capturedNames[ci];
        const bool byRef = captureRefs[ci] != 0;
        const std::string capSrc = lookupSrcType(cap);
        const std::string capIrType = lookupVarType(cap);
        if (capIrType.empty()) {
            // 648（miss 不可静默·110 同族）：捕获变量 IR 类型未登记=编译器内部
            //   缺陷（语义层回填捕获列表必对应 varStack 已登记变量），显形于此；
            //   driver 在 generate() 后 hasErrors() 统一中止，产物不落地。覆盖
            //   本函数下方 byRef AddrOf 与标量快照 StorePtr 两处 i64 兜底 miss
            //   （同变量只报一次），原兜底表达式保留（控制流零变更）。
            diagnostics_.report(DiagnosticLevel::Error, loc,
                "内部错误: 捕获变量 '" + cap + "' IR 类型未登记 ——静默宽度兜底已废除（#648）");
        }
        const std::string slotSym = "?gstatic_" + lambdaSlotKey(lambdaName, cap);
        const bool isStructValCap =
            !byRef && semantic_ != nullptr &&
            semantic_->isStructType(types::canonical(capSrc));
        // 槽注册：globalStatics 值=源类型文本（codegen 按类型分派槽宽：
        //   结构体按 typeSizeOf / i128 双槽 / 其余 8 字节——与顶层静态同通道）
        module_->globalStatics[lambdaSlotKey(lambdaName, cap)] = capSrc;
        ir::IRValue slotAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                          slotSym, loc);
        if (byRef) {
            // 引用捕获：槽存被捕获变量地址（AddrOf 取变量槽地址——数组/结构体/
            //   标量统一地址形态，蹦床 LoadPtr 还原=引用实参）
            const std::string capUnique = lookupVarName(cap);
            ir::IRValue srcAddr = emitResult(
                ir::Opcode::AddrOf,
                {ir::IRValue::var(capUnique,
                                  capIrType.empty() ? "i64" : capIrType)},
                "ptr", capUnique, loc);
            emit(ir::Opcode::StorePtr, {slotAddr, srcAddr}, ir::IRValue(), "",
                 "ptr", loc);
        } else if (isStructValCap) {
            // 结构体值捕获：定义处深拷快照到槽（槽地址作目的）
            const int size = semantic_->typeSizeOf(types::canonical(capSrc));
            ir::IRValue srcAddr = emitResult(
                ir::Opcode::AddrOf,
                {ir::IRValue::var(lookupVarName(cap), "i64")},
                "ptr", lookupVarName(cap), loc);
            emit(ir::Opcode::CopyStruct, {slotAddr, srcAddr}, ir::IRValue(),
                 std::to_string(size), "void", loc);
        } else {
            // 标量值捕获：定义处读取变量值快照（与 captureArgs 同一求值路径）
            ir::IRValue val = genExpr(
                std::make_unique<IdentifierExpr>(cap).get());
            emit(ir::Opcode::StorePtr, {slotAddr, val}, ir::IRValue(), "",
                 capIrType.empty() ? "i64" : capIrType, loc);
        }
    }
}

// 蹦床合成函数 ?lambdaN$trap 构建（432 高阶两面核心）：
//   签名=用户签名（参数段=buildLambdaUserParams 同构）；
//   体 = LoadPtr 快照槽还原捕获实参 + Call ?lambdaN(捕获实参..., 用户实参...)。
//   逃逸调用方经既有 fnptr 间接调用通道进蹦床（T69·i128 ABI 440 已备），
//   蹦床转发 ?lambdaN 直调——闭包展开通道单点归一。
void IRGenerator::buildLambdaTrampoline(
    const std::string& lambdaName, const std::string& returnIrType,
    LambdaExpr* node,
    const std::vector<std::string>& capturedNames,
    const std::vector<char>& captureRefs,
    const SourceLocation& loc) {
    ir::IRFunction trap;
    trap.name = lambdaName + "$trap";
    trap.returnType = returnIrType;
    // 485：蹦床同置 structReturn（与 ?lambdaN 同协议——蹦床作 fnptr 逃逸后
    //   经形态A retbuf 预插间接调用，被调方须按隐藏返回指针回写）。
    //   判定在 visitLambdaExpr 与本函数各算一次（返回类型源码同源幂等）。
    if (semantic_ != nullptr && !node->returnType.empty() &&
        (semantic_->isStructType(types::canonical(node->returnType)) ||
         types::isArray(node->returnType))) {
        trap.structReturn = true;
        trap.structReturnSize = semantic_->typeSizeOf(node->returnType);
    }
    // 保存外层生成状态（trap 构建独立于当前生成上下文·visitLambdaExpr 同框架）
    ir::IRFunction* outerFunction = function_;
    ir::IRBlock* outerBlock = currentBlock_;
    const int outerBlockCounter = blockCounter_;
    const std::size_t outerVarDepth = varStack_.size();
    function_ = &trap;
    varStack_.emplace_back();
    // 用户参数段（与 ?lambdaN 完全同构——ABI 一致是蹦床可作 fnptr 的前提）
    buildLambdaUserParams(trap, node);
    blockCounter_ = 0;
    ir::IRBlock* entry = newBlock("bb0");
    (void)entry;
    // 捕获实参还原 = LoadPtr 快照槽（顺序与 ?lambdaN 捕获参数序严格一致）
    std::vector<ir::IRValue> callArgs;
    callArgs.reserve(capturedNames.size() + node->params.size());
    for (std::size_t ci = 0; ci < capturedNames.size(); ++ci) {
        const std::string& cap = capturedNames[ci];
        const bool byRef = captureRefs[ci] != 0;
        const std::string capSrc = lookupSrcType(cap);
        const std::string capIrType = lookupVarType(cap);
        if (capIrType.empty()) {
            // 648（miss 不可静默·110 同族）：捕获变量 IR 类型未登记=编译器内部
            //   缺陷（与 emitLambdaSnapshotStores 同源 miss 的蹦床侧独立防御点），
            //   显形于此；driver 在 generate() 后 hasErrors() 统一中止，产物不
            //   落地。覆盖下方标量 LoadPtr 的 i64 兜底 miss，原兜底表达式保留
            //   （控制流零变更）。
            diagnostics_.report(DiagnosticLevel::Error, loc,
                "内部错误: 捕获变量 '" + cap + "' IR 类型未登记 ——静默宽度兜底已废除（#648）");
        }
        ir::IRValue slotAddr = emitResult(
            ir::Opcode::ConstString, {}, "ptr",
            "?gstatic_" + lambdaSlotKey(lambdaName, cap), loc);
        if (byRef) {
            // 引用捕获：槽值即变量地址（?lambdaN 捕获参数按 ptr 收地址）
            callArgs.push_back(emitResult(ir::Opcode::LoadPtr, {slotAddr},
                                          "ptr", "", loc));
        } else if (!capSrc.empty() && semantic_ != nullptr &&
                   semantic_->isStructType(types::canonical(capSrc))) {
            // 结构体值捕获：实参=槽地址（?lambdaN 结构体值捕获参数经
            //   structParamIndexes 从指针 rep movsb——槽即固化缓冲）
            callArgs.push_back(slotAddr);
        } else {
            // 标量值捕获：LoadPtr 还原快照值
            callArgs.push_back(emitResult(ir::Opcode::LoadPtr, {slotAddr},
                                          capIrType.empty() ? "i64" : capIrType,
                                          "", loc));
        }
    }
    // 用户实参 = 参数槽值经 Load 读出（与 ?lambda0 体内读参数同通道——
    //   参数槽引用必须走 var(唯一名) 形态 Load：lookupVar 返回的 reg(regId)
    //   是占位寄存器 id，codegen 会把该寄存器分到普通栈槽而非参数槽
    //   （面2 实弹：trap 读 [rbp-80] 未初始化垃圾·参数槽在 [rbp-112]））
    for (const auto& param : node->params) {
        const VarEntry* pe = findVarEntry(param->name);
        if (pe != nullptr) {
            callArgs.push_back(emitResult(
                ir::Opcode::Load,
                {ir::IRValue::var(pe->uniqueName, pe->type)},
                pe->type, pe->uniqueName, loc));
        } else {
            callArgs.push_back(lookupVar(param->name));
        }
    }
    // 转发直调 ?lambdaN（返回=透传）
    // 485（结构体/数组返回）：?lambdaN 已置 structReturn（第一参数位=sret
    //   隐藏指针 rcx），转发调用点须预插蹦床内 retbuf 为首实参（形态A）——
    //   i128 返回经 emitCall result.type=i128 自动走 sret（p10 链已通），
    //   结构体返回被 mapType 塌缩 ptr 无法同判，此处显式补齐；返回透传=
    //   epilogue 按 structReturnSize 把 retbuf 拷到蹦床自己的 sret 槽。
    const bool fwdStructRet = semantic_ != nullptr && !node->returnType.empty() &&
                              (semantic_->isStructType(
                                   types::canonical(node->returnType)) ||
                               types::isArray(node->returnType));
    if (returnIrType.empty() || returnIrType == "void") {
        emit(ir::Opcode::Call, callArgs, ir::IRValue(), lambdaName, "void", loc);
        endReturn("");
    } else if (fwdStructRet) {
        const std::string temp = "__retbuf" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             temp, "ptr", loc);
        // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 拷贝上限一致
        function_->varSlots[temp] = 8;
        registerVarSlots(temp, node->returnType);
        ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(temp, "i64")},
                                     "ptr", temp, loc);
        callArgs.insert(callArgs.begin(), buf);  // 隐藏返回指针=第一参数（rcx）
        emit(ir::Opcode::Call, callArgs, ir::IRValue(), lambdaName, "void", loc);
        endReturn(buf.toString());
    } else {
        ir::IRValue result = emitResult(ir::Opcode::Call, callArgs, returnIrType,
                                        lambdaName, loc);
        endReturn(result.toString());
    }
    varStack_.pop_back();
    module_->functions.push_back(std::move(trap));
    // 恢复外层生成状态（主函数继续）
    function_ = outerFunction;
    currentBlock_ = outerBlock;
    blockCounter_ = outerBlockCounter;
    while (varStack_.size() > outerVarDepth) varStack_.pop_back();
}

void IRGenerator::visitLambdaExpr(LambdaExpr* node) {
    // 815（485 clear 纪律生命周期改造·save/restore）：lambda 体=独立函数上下文，
    //   闭包关联表须与体外隔离（485：防内外同名闭包变量跨上下文误命中快径——
    //   键=变量源码名无函数限定）。原实现入口 clear 出口不恢复：同函数后定义的
    //   lambda 会清掉先前全部登记（655 返工④·720 实录：每组第一个定义的 lambda
    //   调用 miss 掉 432 蹦床通道 CallIndirect、最后一个 hit——语义等价但快径
    //   覆盖收缩=性能缺陷，且掉穿面曾致 655 首轮全域 miss 误报连锁）。改法=
    //   入口整表 save+清空（体内隔离纪律保持），出口整表 restore（外层先前登记
    //   复活·先前闭包变量调用重新命中快径直调 ?lambdaN——ir_call.cpp 展开通道
    //   与蹦床语义等价）；本 lambda 自己的登记由 genVarDecl 的
    //   registerClosureIfNeeded 在本函数返回后落表（时序天然落在恢复后的表上）。
    //   跨函数隔离仍由 visitFunctionDecl 入口 clear 承担（815 不动）。
    //   v2 无此病（零分叉辨析）：v2 闭包绑定表（lambda闭包变量们）从无 clear
    //   动作、可蹦床形态恒走 T69 间接调用（432 蹦床归一主动设计）——本修复为
    //   宿主单侧优化通道的覆盖恢复，运行行为两侧等价不变。
    std::unordered_map<std::string, ClosureInfo> outerClosures =
        std::move(closureInfo_);
    closureInfo_.clear();  // move 后显式清空：不依赖移动后源的未规定状态
    const std::string lambdaName = "?lambda" + std::to_string(lambdaCounter_++);
    const std::string returnIrType = mapType(node->returnType.empty()
                                                 ? "空类型" : node->returnType);
    // 485（结构体/数组返回 lambda 的 sret 协议）：合成函数同普通函数声明通道
    //   置 structReturn（被调方隐藏返回指针接收/回写·paramOffset=1·epilogue 按
    //   structReturnSize 拷贝）——原恒按 rax 返回栈上临时地址：同帧展开通道读
    //   被调帧悬垂垃圾、间接调用（retbuf 预插形态A）写 rcx 未接收=错值/崩溃
    //   （探针 p15 实锤）。返回类型经 mapType 已塌缩 ptr，判定用源码类型。
    const bool structRetLambda = semantic_ != nullptr && !node->returnType.empty() &&
                                 (semantic_->isStructType(
                                      types::canonical(node->returnType)) ||
                                  types::isArray(node->returnType));
    const int structRetSize =
        structRetLambda && semantic_ != nullptr
            ? semantic_->typeSizeOf(node->returnType) : 0;
    ir::IRFunction func;
    func.name = lambdaName;
    func.returnType = returnIrType;
    func.structReturn = structRetLambda;
    func.structReturnSize = structRetSize;
    // 捕获变量名（语义层已回填全部捕获）：匿名函数参数前置
    std::vector<std::string> capturedNames = node->explicitCaptures;
    // 缺陷修复（[=] 快照 / [&] 引用区分，规格书04-一D）：
    //   [&] 引用捕获整体：所有捕获参数按"指针"形态传递（参数槽存被捕获变量地址），
    //     体内读取解引用（读最新值）、赋值经指针（写外部变量）。
    //   [=]/[]/[变量] 值捕获：参数槽存定义处值快照（值语义，后续外部修改不影响）。
    const bool allByRef = (node->captureKind == LambdaCaptureKind::ByRef);
    std::vector<char> captureRefs(capturedNames.size(), allByRef ? 1 : 0);
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
        const bool byRef = captureRefs[ci] != 0;
        const std::string capType = lookupVarType(cap);
        const std::string capSrc = lookupSrcType(cap);
        if (capType.empty()) {
            // 648（miss 不可静默·110 同族）：捕获变量 IR 类型未登记=编译器内部
            //   缺陷（语义层回填捕获列表必对应 varStack 已登记变量），显形于此；
            //   driver 在 generate() 后 hasErrors() 统一中止，产物不落地。覆盖
            //   本循环下方捕获参数 paramType 与 VarEntry.type 两处 i64 兜底 miss
            //   （同源同变量只报一次），原兜底表达式保留（控制流零变更）。
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                "内部错误: 捕获变量 '" + cap + "' IR 类型未登记 ——静默宽度兜底已废除（#648）");
        }
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
    // 显式参数（432：提炼 buildLambdaUserParams——蹦床参数段同构共用）
    buildLambdaUserParams(func, node);
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
    // 815：恢复外层闭包关联表（save/restore 出口·见函数头注释）——先于蹦床段
    //   发射与 lastLambda* 收尾（蹦床段不消费闭包表，位置取语义聚合：与上方
    //   四状态恢复同块收尾）。
    closureInfo_ = std::move(outerClosures);
    // ---- 432 蹦床段（外层块发射）----
    // 快照静态槽注册 + 定义处固化（值捕获快照/[&] 地址——逃逸生命周期根基）
    emitLambdaSnapshotStores(lambdaName, capturedNames, captureRefs,
                             node->location);
    // 蹦床 ?lambdaN$trap 构建（lambda 值=蹦床地址——高阶两面载体）
    buildLambdaTrampoline(lambdaName, returnIrType, node, capturedNames,
                          captureRefs, node->location);
    // 记录最近一次 lambda 的匿名函数名与捕获列表（genVarDecl 登记闭包关联用；
    //   同函数闭包变量调用仍直调 ?lambdaN——closureInfo_ 内联快径与蹦床语义等价）
    lastLambdaName_ = lambdaName;
    lastLambdaCaptures_ = capturedNames;
    lastLambdaReturnIrType_ = returnIrType;
    lastLambdaSrcReturnType_ = node->returnType;  // 485：源返回类型（sret 判定）
    lastLambdaCaptureRefs_ = captureRefs;
    // 337-a（T53 家系）：lambda 用户形参类型列表（闭包调用展开用户实参的 ABI
    //   定标依据；函数指针形参登记规范串，与声明位同口径）。
    lastLambdaParamTypes_.clear();
    for (const auto& param : node->params) {
        lastLambdaParamTypes_.push_back(param->funcPtr.isFunctionPtr()
                                            ? param->funcPtr.toString()
                                            : param->typeName);
    }
    // 表达式结果 = 蹦床函数地址（432：原 ?lambdaN 裸地址——捕获实参无载体，
    //   经返回/实参逃逸后调用读到垃圾参数=悬垂面根治）。
    // 注意：FuncAddr 须在外层（主函数）块中生成——function_ 已恢复
    lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr", lambdaName + "$trap",
                           node->location);
}
} // namespace cn_compiler
