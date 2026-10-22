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
#include <cstdio>
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

// 940（#940 值捕获快照悬垂根治·单一归属）：值捕获快照值求值——字符串/
//   容器/用户类=定义处深拷独立副本（源重赋值/块析构释放后快照不悬垂·
//   001 §一D「值捕获快照语义无悬垂面」）；标量/指针/数组原样直取。
//   字符串：__cn_str_copy 落堆（genCopyBuiltin ① 同通道）；
//   容器/用户类：NewObject+拷贝构造（genCopyBuiltin ③ 同通道·引用实参=
//   源变量槽地址·「发现二」约定）；无拷贝构造类=语义层负测拒绝面，
//   此处防御直取原句柄（不可达路径，非静默兜底：语义检查已拦）。
ir::IRValue IRGenerator::evalValueCaptureCopy(const std::string& capName,
                                              const std::string& capSrc,
                                              const SourceLocation& loc) {
    ir::IRValue val = genExpr(std::make_unique<IdentifierExpr>(capName).get());
    if (semantic_ == nullptr) return val;
    const std::string canon = types::canonical(capSrc);
    if (canon == "字符串") {
        return emitResult(ir::Opcode::Call, {val}, "ptr", "__cn_str_copy", loc);
    }
    if (semantic_->isClassType(canon)) {
        const std::string copyKey = classCopyCtorSymbolKey(canon);
        if (copyKey.empty()) return val;  // 语义层已拒绝（无拷贝构造负测面）
        const std::string capUnique = lookupVarName(capName);
        ir::IRValue srcSlot = emitResult(
            ir::Opcode::AddrOf,
            {ir::IRValue::var(capUnique, "i64")}, "ptr", capUnique, loc);
        const ClassInfo* ciC = semantic_->findClass(canon);
        const std::string extra =
            canon + "|" + std::to_string(ciC != nullptr ? ciC->totalSize : 0);
        ir::IRValue newObj = emitResult(
            ir::Opcode::NewObject, {ir::IRValue::constant(canon, "ptr")}, "ptr",
            extra, loc);
        emit(ir::Opcode::Call, {newObj, srcSlot}, ir::IRValue(), copyKey, "void",
             loc);
        return newObj;
    }
    return val;
}

// lambda 用户参数段构建（?lambdaN 与蹦床 ?lambdaN$trap 共用——纪律二当场归纳；
//   原为 visitLambdaExpr 内联段·432 提炼）。前置条件：function_ 已指向 func、
//   varStack_ 已压入函数作用域。引用参数（T&）按 ptr 槽+byRef 形态（A-1 同款）。
void IRGenerator::buildLambdaUserParams(ir::IRFunction& func,
                                        LambdaExpr* node) {
    std::vector<const ParamDecl*> params;
    params.reserve(node->params.size());
    for (const auto& p : node->params) params.push_back(p.get());
    buildClosureParams943(func, params);
}

// 943（绑定方法值赋值位合法化·蹦床闭包）：闭包参数段构建单点——lambda 蹦床
//   与 方法值蹦床 共用（A-1 引用参数按 ptr+byRef；函数指针参数按 ptr；其余
//   按 mapType——与 ?lambdaN 完全同构=蹦床可作 fnptr 的前提）。
void IRGenerator::buildClosureParams943(
    ir::IRFunction& func, const std::vector<const ParamDecl*>& params) {
    for (std::size_t pi = 0; pi < params.size(); ++pi) {
        const ParamDecl* param = params[pi];
        // A-1（引用参数）：lambda 显式引用参数（整32&）同样按 byRef 语义
        const bool isRefParam = !param->funcPtr.isFunctionPtr() &&
                                types::isReference(param->typeName);
        const std::string ptype =
            (param->funcPtr.isFunctionPtr() || isRefParam)
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
        entry.type = isRefParam ? mapType(types::stripRef(param->typeName))
                    : reg.type;
        // 337-a（T53 家系）：lambda 形参同款——函数指针取完整规范串
        //   （paramSrcTypeOf 单一归属，与 ir_decl/ir_generic_func 登记点同口径）。
        entry.srcType = paramSrcTypeOf(*param);
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
            // 结构体值捕获：定义处**深拷快照**到槽（940：#632/#617/#644 深拷通道
            //   emitStructCopyWithFields）——原裸 CopyStruct 仅位拷首层字节：
            //   字符串字段=句柄浅共享（源重赋值/块析构释放堆串后快照悬垂·闭包读
            //   空串/垃圾）、容器/类字段=指针浅共享（源释放后快照读大小 0/元素(0)
            //   越界——违反 001 §一D「值捕获快照语义无悬垂面」）。深拷=字段级
            //   独立拥有副本（串 __cn_str_copy 落堆·容器/类字段 NewObject+拷贝
            //   构造·嵌套递归），快照与源完全解耦。纯 POD 结构体（无拥有字段）
            //   内部自动落 CopyStruct 位拷（零开销不变）。
            //   preFree=false（940 裁决）：释放旧副本为本任务外延（重固化=槽
            //   覆盖语义旧副本成孤儿·与 v2 固化端口径一致——v2 固化点无
            //   函数级清旧设施，双侧同「不释放」零分叉）；副本释放归属=
            //   闭包值生命周期（CN 闭包值=裸函数指针无析构语义·语言级缺口）
            //   → 挂账独立任务。悬垂（安全目标②）优先于残留（性能面）。
            const std::string canon = types::canonical(capSrc);
            ir::IRValue srcAddr = emitResult(
                ir::Opcode::AddrOf,
                {ir::IRValue::var(lookupVarName(cap), "i64")},
                "ptr", lookupVarName(cap), loc);
            emitStructCopyWithFields(slotAddr, srcAddr, canon, loc,
                                     /*preFree=*/false, /*deepCopy=*/true);
        } else {
            // 标量/字符串/容器/类值捕获：定义处快照——字符串与容器/类经
            //   evalValueCaptureCopy 深拷独立副本（940：原裸句柄浅共享，
            //   源重赋值/释放后悬垂）；标量直取（原路径零变化）。
            ir::IRValue val = evalValueCaptureCopy(cap, capSrc, loc);
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
    std::vector<const ParamDecl*> params;
    params.reserve(node->params.size());
    for (const auto& p : node->params) params.push_back(p.get());
    buildClosureTrampoline943(lambdaName + "$trap", lambdaName, lambdaName,
                              node->returnType, returnIrType, params,
                              capturedNames, captureRefs, loc);
}

// 943：通用闭包蹦床构建单点（432 lambda 蹦床 与 方法值蹦床 共用——杜绝复制）。
//   trapName=蹦床函数名；calleeName=转发目标（lambda=?lambdaN·方法值=方法链接
//   符号）；closureId=快照静态槽键前缀（?gstatic_cap$<id>$<捕获名>）；params=
//   蹦床用户参数（lambda 用户形参 / 方法形参——与 callee 的参数序一致）。
void IRGenerator::buildClosureTrampoline943(
    const std::string& trapName, const std::string& calleeName,
    const std::string& closureId, const std::string& returnSrcType,
    const std::string& returnIrType,
    const std::vector<const ParamDecl*>& params,
    const std::vector<std::string>& capturedNames,
    const std::vector<char>& captureRefs,
    const SourceLocation& loc) {
    ir::IRFunction trap;
    trap.name = trapName;
    trap.returnType = returnIrType;
    // 485：蹦床同置 structReturn（与被调同协议——蹦床作 fnptr 逃逸后
    //   经形态A retbuf 预插间接调用，被调方须按隐藏返回指针回写）。
    //   判定在 visitLambdaExpr 与本函数各算一次（返回类型源码同源幂等）。
    if (semantic_ != nullptr && !returnSrcType.empty() &&
        (semantic_->isStructType(types::canonical(returnSrcType)) ||
         types::isArray(returnSrcType))) {
        trap.structReturn = true;
        trap.structReturnSize = semantic_->typeSizeOf(returnSrcType);
    }
    // 保存外层生成状态（trap 构建独立于当前生成上下文·visitLambdaExpr 同框架）
    ir::IRFunction* outerFunction = function_;
    ir::IRBlock* outerBlock = currentBlock_;
    const int outerBlockCounter = blockCounter_;
    const std::size_t outerVarDepth = varStack_.size();
    function_ = &trap;
    varStack_.emplace_back();
    // 用户参数段（与被调完全同构——ABI 一致是蹦床可作 fnptr 的前提）
    buildClosureParams943(trap, params);
    blockCounter_ = 0;
    ir::IRBlock* entry = newBlock("bb0");
    (void)entry;
    // 捕获实参还原 = LoadPtr 快照槽（顺序与被调捕获参数序严格一致）
    std::vector<ir::IRValue> callArgs;
    callArgs.reserve(capturedNames.size() + params.size());
    for (std::size_t ci = 0; ci < capturedNames.size(); ++ci) {
        const std::string& cap = capturedNames[ci];
        const bool byRef = captureRefs[ci] != 0;
        ir::IRValue slotAddr = emitResult(
            ir::Opcode::ConstString, {}, "ptr",
            "?gstatic_" + lambdaSlotKey(closureId, cap), loc);
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
        if (byRef) {
            // 引用捕获：槽值即变量地址（被调捕获参数按 ptr 收地址）
            callArgs.push_back(emitResult(ir::Opcode::LoadPtr, {slotAddr},
                                          "ptr", "", loc));
        } else if (!capSrc.empty() && semantic_ != nullptr &&
                   semantic_->isStructType(types::canonical(capSrc))) {
            // 结构体值捕获：实参=槽地址（被调结构体值捕获参数经
            //   structParamIndexes 从指针 rep movsb——槽即固化缓冲）
            callArgs.push_back(slotAddr);
        } else {
            // 标量值捕获：LoadPtr 还原快照值
            callArgs.push_back(emitResult(ir::Opcode::LoadPtr, {slotAddr},
                                          capIrType.empty() ? "i64" : capIrType,
                                          "", loc));
        }
    }
    // 用户实参 = 参数槽值经 Load 读出（与被调体内读参数同通道——
    //   参数槽引用必须走 var(唯一名) 形态 Load：lookupVar 返回的 reg(regId)
    //   是占位寄存器 id，codegen 会把该寄存器分到普通栈槽而非参数槽
    //   （面2 实弹：trap 读 [rbp-80] 未初始化垃圾·参数槽在 [rbp-112]））
    for (const auto* param : params) {
        const VarEntry* pe = findVarEntry(param->name);
        if (pe != nullptr) {
            // 943：按「槽内实际内容宽度」装载转发——引用形参/地址透传形参
            //   （结构体/数组）槽值=地址（ptr 宽），按声明窄宽装载会把地址
            //   截断（457 捕码截断 SEGV 同族）；其余按值宽装载。
            const std::string loadType =
                (pe->byRef || pe->type == "ptr") ? "ptr" : pe->type;
            callArgs.push_back(emitResult(
                ir::Opcode::Load,
                {ir::IRValue::var(pe->uniqueName, pe->type)},
                loadType, pe->uniqueName, loc));
        } else {
            callArgs.push_back(lookupVar(param->name));
        }
    }
    // 转发直调被调（返回=透传）
    // 485（结构体/数组返回）：被调已置 structReturn（第一参数位=sret
    //   隐藏指针 rcx），转发调用点须预插蹦床内 retbuf 为首实参（形态A）——
    //   i128 返回经 emitCall result.type=i128 自动走 sret（p10 链已通），
    //   结构体返回被 mapType 塌缩 ptr 无法同判，此处显式补齐；返回透传=
    //   epilogue 按 structReturnSize 把 retbuf 拷到蹦床自己的 sret 槽。
    const bool fwdStructRet = semantic_ != nullptr && !returnSrcType.empty() &&
                              (semantic_->isStructType(
                                   types::canonical(returnSrcType)) ||
                               types::isArray(returnSrcType));
    if (returnIrType.empty() || returnIrType == "void") {
        emit(ir::Opcode::Call, callArgs, ir::IRValue(), calleeName, "void", loc);
        endReturn("");
    } else if (fwdStructRet) {
        const std::string temp = "__retbuf" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             temp, "ptr", loc);
        // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 拷贝上限一致
        function_->varSlots[temp] = 8;
        registerVarSlots(temp, returnSrcType);
        ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(temp, "i64")},
                                     "ptr", temp, loc);
        callArgs.insert(callArgs.begin(), buf);  // 隐藏返回指针=第一参数（rcx）
        emit(ir::Opcode::Call, callArgs, ir::IRValue(), calleeName, "void", loc);
        endReturn(buf.toString());
    } else {  // 标量返回：直调结果透传
        ir::IRValue result = emitResult(ir::Opcode::Call, callArgs, returnIrType,
                                        calleeName, loc);
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

// 943（绑定方法值赋值位合法化·2026-10-17）：方法值蹦床合成已废弃（031·#1075
//   一等公民化——`对象.方法` 求值改走两槽值通道 ir_method_value.cpp：值自带
//   对象引用，转发/拷贝/跨函数天然安全，不再需要「蹦床+静态槽+编译期旁路」；
//   plans/031 §二·③⑥「#943 过渡路径废弃」）。buildClosureTrampoline943 保留
//   ——lambda 蹦床（?lambdaN$trap·上方 visitLambdaExpr 215 行调用）共用。

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
    // 482：本函数返回来源跟踪同上下文隔离（lambda 体内的「返回」不污染外层
    //   函数返回来源表；lambda 自身的返回来源无消费点=丢弃）。
    const std::string outerReturnOrigin482 = currentReturnOrigin482_;
    currentReturnOrigin482_.clear();
    const std::unordered_set<std::string> outerAssigned482 = carrierAssigned482_;
    carrierAssigned482_.clear();
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
    // 482：同步收集捕获源类型（载体快照槽布局元数据·此刻仍在外层作用域可查）。
    std::vector<std::string> capturedSrcTypes482;
    capturedSrcTypes482.reserve(capturedNames.size());
    for (std::size_t ci = 0; ci < capturedNames.size(); ++ci) {
        const std::string& cap = capturedNames[ci];
        const bool byRef = captureRefs[ci] != 0;
        const std::string capType = lookupVarType(cap);
        const std::string capSrc = lookupSrcType(cap);
        capturedSrcTypes482.push_back(capSrc);
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
    // 482：恢复外层函数返回来源跟踪（lambda 体内返回不污染外层·见入口注释）。
    currentReturnOrigin482_ = outerReturnOrigin482;
    carrierAssigned482_ = outerAssigned482;
    // 482：捕获元数据落表（载体快照槽布局单一事实源——名/引用/源类型）。
    recordLambdaCaptureMeta482(lambdaName, capturedNames, captureRefs,
                               capturedSrcTypes482);
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
    // 482：静态调用元数据持久副本（接收点跨函数重建载体登记用——lastLambda*
    //   为一次性通道，随后被 genVarDecl 消费清空）。
    {
        LambdaStaticInfo482 si482;
        si482.returnIrType = returnIrType;
        si482.srcReturnType = node->returnType;
        si482.paramTypes = lastLambdaParamTypes_;
        lambdaStaticInfo482_[lambdaName] = std::move(si482);
    }
    // 表达式结果 = 蹦床函数地址（432：原 ?lambdaN 裸地址——捕获实参无载体，
    //   经返回/实参逃逸后调用读到垃圾参数=悬垂面根治）。
    // 注意：FuncAddr 须在外层（主函数）块中生成——function_ 已恢复
    lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr", lambdaName + "$trap",
                           node->location);
}

// ==================== 482（#482 多实例独立快照）载体快照槽族 ====================
// 缺陷（核验波2 实证）：lambda 值捕获快照固化在「同一定义点共享的静态槽」
//   （emitLambdaSnapshotStores·蹦床还原读同槽）——同一定义点在循环内/跨函数
//   多次求值产生多实例共存时后写覆盖先值（p482b 7/107/107、p482e 107/107
//   实测双侧同病）；赋值位还遗留「重赋值后调用仍直调旧定义点」陈旧登记
//   （p482 宿主 0/0）。根治模型（快照随「值载体变量」独立存储）：
//   ① 载体槽=每「lambda 变量」一套隐藏栈槽（每捕获一槽·registerVarSlots 定宽）；
//   ② 快照流四处维护：定义点（共享槽已写后随批拷入载体槽）/ 接收点
//      （变量 甲 = 造(7)——被调返回点已写共享槽→拷入甲槽）/ 拷贝链
//      （g = f——槽对槽整拷）/ 返回点（返回 f——载体槽写回共享槽，保证
//      接收方按「函数返回来源」拷到的是 f 的当前实例而非定义点最近值）；
//   ③ 调用点：登记为槽式时从载体槽装载捕获实参直调 ?lambdaN（各实例互不影响）；
//   ④ 来源解析（resolveLambdaOrigin482）：lambda 字面量/登记变量/直接调用
//      （函数返回来源表）——来源未知=清除登记退化蹦床通道（共享槽=最近实例，
//      与现状同界不再更差；数组/字段/参数等载体另域）。
int IRGenerator::captureBytes482(const std::string& capSrc) {
    // 641-fix 静态槽宽度口径：lambda 捕获槽恒 8 字节槽宽（固化/还原走
    //   T_整64 全宽——641 记述「lambda/捕获路径不经静态值码」）；i128=16、
    //   结构体/数组=typeSizeOf（CopyStruct 整块拷，浅字节口径与
    //   emitLambdaSnapshotStores 结构体快照一致）。
    if (semantic_ != nullptr && !capSrc.empty()) {
        if (types::isI128(types::canonical(capSrc))) return 16;
        const std::string canon = types::canonical(capSrc);
        if (semantic_->isStructType(canon) || types::isArray(capSrc)) {
            const int bytes = semantic_->typeSizeOf(capSrc);
            if (bytes > 0) return bytes;
        }
    }
    return 8;
}

void IRGenerator::recordLambdaCaptureMeta482(
    const std::string& lambdaName,
    const std::vector<std::string>& capturedNames,
    const std::vector<char>& captureRefs,
    const std::vector<std::string>& srcTypes) {
    LambdaCaptureMeta482 meta;
    meta.names = capturedNames;
    meta.refs = captureRefs;
    meta.srcTypes = srcTypes;
    lambdaCaptureMeta482_[lambdaName] = std::move(meta);
}

std::vector<ClosureInfo::CarrierCapture> IRGenerator::makeCarrierSlots482(
    const std::string& varName, const std::string& lambdaName,
    const SourceLocation& loc) {
    std::vector<ClosureInfo::CarrierCapture> slots;
    auto metaIt = lambdaCaptureMeta482_.find(lambdaName);
    if (metaIt == lambdaCaptureMeta482_.end()) return slots;
    for (std::size_t ci = 0; ci < metaIt->second.names.size(); ++ci) {
        ClosureInfo::CarrierCapture cc;
        cc.unique = varName + "$snap482$" + metaIt->second.names[ci] + "$" +
                    std::to_string(varCounter_++);
        cc.srcType = ci < metaIt->second.srcTypes.size()
                         ? metaIt->second.srcTypes[ci] : std::string();
        registerVarSlots(cc.unique, cc.srcType);
        // 登记锚 Alloca（codegen 两轮槽登记以 Alloca/变量引用为据——载体槽
        //   在返回点整拷场景可能仅被 AddrOf 消费，须显式锚定防 #836）。
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             cc.unique, "ptr", loc);
        slots.push_back(std::move(cc));
    }
    return slots;
}

void IRGenerator::emitSharedToCarrier482(
    const std::string& lambdaName,
    const std::vector<ClosureInfo::CarrierCapture>& slots,
    const SourceLocation& loc) {
    auto metaIt = lambdaCaptureMeta482_.find(lambdaName);
    if (metaIt == lambdaCaptureMeta482_.end()) return;
    const std::size_t n = slots.size() < metaIt->second.names.size()
                              ? slots.size() : metaIt->second.names.size();
    for (std::size_t ci = 0; ci < n; ++ci) {
        const std::string& cap = metaIt->second.names[ci];
        const int bytes = captureBytes482(slots[ci].srcType);
        ir::IRValue srcAddr = emitResult(
            ir::Opcode::ConstString, {}, "ptr",
            "?gstatic_" + lambdaSlotKey(lambdaName, cap), loc);
        ir::IRValue dstAddr = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(slots[ci].unique, "i64")},
            "ptr", slots[ci].unique, loc);
        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
             std::to_string(bytes), "void", loc);
    }
}

void IRGenerator::emitCarrierToShared482(
    const std::string& lambdaName,
    const std::vector<ClosureInfo::CarrierCapture>& slots,
    const SourceLocation& loc) {
    auto metaIt = lambdaCaptureMeta482_.find(lambdaName);
    if (metaIt == lambdaCaptureMeta482_.end()) return;
    const std::size_t n = slots.size() < metaIt->second.names.size()
                              ? slots.size() : metaIt->second.names.size();
    for (std::size_t ci = 0; ci < n; ++ci) {
        const std::string& cap = metaIt->second.names[ci];
        const int bytes = captureBytes482(slots[ci].srcType);
        ir::IRValue dstAddr = emitResult(
            ir::Opcode::ConstString, {}, "ptr",
            "?gstatic_" + lambdaSlotKey(lambdaName, cap), loc);
        ir::IRValue srcAddr = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(slots[ci].unique, "i64")},
            "ptr", slots[ci].unique, loc);
        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
             std::to_string(bytes), "void", loc);
    }
}

void IRGenerator::emitCarrierToCarrier482(
    const std::vector<ClosureInfo::CarrierCapture>& srcSlots,
    const std::vector<ClosureInfo::CarrierCapture>& dstSlots,
    const SourceLocation& loc) {
    const std::size_t n = srcSlots.size() < dstSlots.size()
                              ? srcSlots.size() : dstSlots.size();
    for (std::size_t ci = 0; ci < n; ++ci) {
        const int bytes = captureBytes482(dstSlots[ci].srcType);
        ir::IRValue srcAddr = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(srcSlots[ci].unique, "i64")},
            "ptr", srcSlots[ci].unique, loc);
        ir::IRValue dstAddr = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(dstSlots[ci].unique, "i64")},
            "ptr", dstSlots[ci].unique, loc);
        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
             std::to_string(bytes), "void", loc);
    }
}

std::string IRGenerator::resolveLambdaOrigin482(Expr* rhs,
                                                const std::string& literalName) {
    if (rhs == nullptr) return "";
    switch (rhs->getType()) {
        case NodeType::LambdaExpr:
            // 调用者须在 genExpr(rhs) 之后立即调用（lastLambdaName_ 尚未被
            //   消费清空——registerClosureIfNeeded 场景由调用方先行快照）。
            return !literalName.empty() ? literalName : lastLambdaName_;
        case NodeType::IdentifierExpr: {
            const std::string& name = static_cast<IdentifierExpr*>(rhs)->name;
            auto it = closureInfo_.find(name);
            return it != closureInfo_.end() ? it->second.lambdaName
                                            : std::string();
        }
        case NodeType::CallExpr: {
            const CallExpr* call = static_cast<const CallExpr*>(rhs);
            if (call->callee != nullptr &&
                call->callee->getType() == NodeType::IdentifierExpr) {
                std::string key = call->resolvedSignature;
                if (key.empty()) {
                    key = static_cast<const IdentifierExpr*>(
                              call->callee.get())->name;
                }
                auto it = funcReturnLambda482_.find(key);
                if (it != funcReturnLambda482_.end()) return it->second;
            }
            return "";
        }
        default:
            return "";
    }
}

void IRGenerator::updateCarrierOnAssign482(const std::string& varName, Expr* rhs,
                                           const SourceLocation& loc,
                                           const std::string& literalName,
                                           bool fromPlainAssign) {
    if (rhs == nullptr) return;
    // 方法值（成员表达式）走既有登记通道（P3-23/816/104）——载体族零介入
    //   （否则「来源未知」分支会把方法值登记误清——1002-⑥ 回归）。
    if (rhs->getType() == NodeType::MemberExpr &&
        static_cast<MemberExpr*>(rhs)->isMethodValue) {
        return;
    }
    // 干预面判定（不用目标类型串）：宿主「变量 f = lambda」的 VarEntry.srcType
    //   为空白（lambda 初始化类型未回填——类型串仅在间接调用定标面消费），
    //   改以「右值来源 / 历史登记」判定——origin 非空=lambda 来源（建/复用
    //   载体槽）；origin 空但有历史登记=清除陈旧；普通变量（整型/普通 fnptr
    //   无历史登记）零介入=零行为变化。
    const std::string origin = resolveLambdaOrigin482(rhs, literalName);
    if (origin.empty()) {
        // 来源未知：清除陈旧登记（p482 根因：重赋值后调用仍展开旧定义点）；
        //   lambdaVarNames_ 留痕保留（间接调用诊断判据用·655 返工）。
        if (closureInfo_.erase(varName) > 0 && fromPlainAssign) {
            carrierAssigned482_.insert(varName);
        }
        return;
    }
    auto metaIt = lambdaCaptureMeta482_.find(origin);
    if (metaIt == lambdaCaptureMeta482_.end()) {
        // 元数据缺失（非 lambda 定义点来源——方法值符号等）：保持既有登记不动。
        return;
    }
    // 复用/新建载体槽：同变量同定义点重复赋值=复用槽（循环体内赋值发射一次，
    //   运行多轮各自重写槽=互不干扰）；换定义点/换变量=新建槽集合。
    auto oldIt = closureInfo_.find(varName);
    const bool reuse = oldIt != closureInfo_.end() &&
                       oldIt->second.lambdaName == origin &&
                       !oldIt->second.carrierCaptures.empty() &&
                       oldIt->second.carrierCaptures.size() ==
                           metaIt->second.names.size();
    ClosureInfo info;
    if (reuse) {
        info = oldIt->second;
    } else {
        info.lambdaName = origin;
        info.captures = metaIt->second.names;
        auto stIt = lambdaStaticInfo482_.find(origin);
        if (stIt != lambdaStaticInfo482_.end()) {
            info.returnIrType = stIt->second.returnIrType;
            info.srcReturnType = stIt->second.srcReturnType;
            info.paramTypes = stIt->second.paramTypes;
        }
        info.carrierCaptures = makeCarrierSlots482(varName, origin, loc);
    }
    // 快照→载体槽（源按右值形态分派）：标识符=源载体槽整拷（816 拷贝链升级为
    //   独立快照）；lambda 字面量=定义点已写共享槽、接收调用=被调返回点已写
    //   共享槽——统一从共享槽拷入载体槽（时间窗=紧随求值，无中间 lambda 求值）。
    if (!info.carrierCaptures.empty()) {
        bool copied = false;
        if (rhs->getType() == NodeType::IdentifierExpr) {
            const std::string& srcVar =
                static_cast<IdentifierExpr*>(rhs)->name;
            auto srcIt = closureInfo_.find(srcVar);
            if (srcIt != closureInfo_.end() &&
                srcIt->second.carrierCaptures.size() ==
                    info.carrierCaptures.size()) {
                emitCarrierToCarrier482(srcIt->second.carrierCaptures,
                                        info.carrierCaptures, loc);
                copied = true;
            }
        }
        if (!copied) {
            emitSharedToCarrier482(origin, info.carrierCaptures, loc);
        }
    }
    closureInfo_[varName] = std::move(info);
    // 482×815：纯赋值位更新集（穿透块级回滚——循环内重赋值外层闭包变量时块外
    //   调用须见新实例）；声明初始化位从集移除（块内遮蔽声明=随作用域回滚）。
    if (fromPlainAssign) {
        carrierAssigned482_.insert(varName);
    } else {
        carrierAssigned482_.erase(varName);
    }
    lambdaVarNames_.insert(varName);
}

void IRGenerator::emitReturnOriginSnapshot482(Expr* rhs) {
    if (rhs == nullptr) return;
    const std::string origin = resolveLambdaOrigin482(rhs);
    if (origin.empty()) {
        // 未知来源返回：已有已知痕迹即置冲突哨兵（同一函数多返回路径
        //   来源不一致=接收点静态解析不可信）。
        if (!currentReturnOrigin482_.empty()) currentReturnOrigin482_ = "?";
        return;
    }
    // 槽式载体来源：载体槽→共享槽整拷（保证接收方按返回来源拷到当前实例）。
    if (rhs->getType() == NodeType::IdentifierExpr) {
        const std::string& name = static_cast<IdentifierExpr*>(rhs)->name;
        auto it = closureInfo_.find(name);
        if (it != closureInfo_.end() && it->second.lambdaName == origin &&
            !it->second.carrierCaptures.empty()) {
            emitCarrierToShared482(origin, it->second.carrierCaptures,
                                   rhs->location);
        }
    }
    // lambda 字面量（定义点已写共享槽）/调用返回（被调返回点已写）=无动作。
    if (currentReturnOrigin482_.empty()) {
        currentReturnOrigin482_ = origin;
    } else if (currentReturnOrigin482_ != origin) {
        currentReturnOrigin482_ = "?";
    }
}
} // namespace cn_compiler
