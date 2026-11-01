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
        // 1360 结果化：无值=无拷贝构造（has_value 分流·语义层已拒绝负测面）
        const cn::Optional<std::string> copyKeyOpt = classCopyCtorSymbolKey(canon);
        const ClassInfo* ciC = semantic_->findClass(canon).value_or(nullptr);
        bool srcHasDtor = false;
        if (ciC != nullptr) {
            for (const auto& mk : ciC->methods) {
                if (mk.second.isDestructor) { srcHasDtor = true; break; }
            }
        }
        // 1367（940 残面·768 对称）：带析构且无拷贝构造的类值捕获——位拷贝
        //   快照在析构面双放行（源与快照各自析构共享资源=悬垂/双放行），拒绝
        //   （v2 生成类尾/值捕获段同判据同文案·零分叉）。
        if (!copyKeyOpt.has_value() && srcHasDtor) {
            diagnostics_.report(
                DiagnosticLevel::Error, loc,
                "类 '" + canon +
                    "' 带析构且无拷贝构造，不能作值捕获快照（位拷贝=析构双放"
                    "行悬垂）；为类补拷贝构造，或改用显式复制后捕获");
            return val;
        }
        // 1367（768 双分·无析构纯数据类）：位拷贝快照回退——原 copyKey 空
        //   直取原句柄浅共享（快照独立性与多字段读面双缺口；旧注释「语义层
        //   已拒绝」与实测不符已更正——语义层无此拒绝面）。无析构=无用户
        //   释放面，浅位拷与 C++ 隐式拷贝语义同（拥有字段以 拷贝构造/析构
        //   成对声明承担）；NewObject 堆副本句柄单槽（拷贝构造路径同模型）。
        //   位拷源=val（类标识符读=对象句柄·槽存堆对象指针——1321 引用源
        //   透明化由读路径承担）。
        if (!copyKeyOpt.has_value()) {
            const int totalBytes = ciC != nullptr ? ciC->totalSize : 0;
            ir::IRValue newObj = emitResult(
                ir::Opcode::NewObject, {ir::IRValue::constant(canon, "ptr")}, "ptr",
                canon + "|" + std::to_string(totalBytes), loc);
            if (totalBytes != 0) {  // 空类（totalSize 0）跳过位拷（NewObject 空壳即快照）
                emit(ir::Opcode::CopyStruct, {newObj, val},
                     ir::IRValue(), std::to_string(totalBytes), "void", loc);
            }
            return newObj;
        }
        // 1321：拷贝构造源地址走 genIdentifierLvalue 单点——被捕获变量是
        //   类引用（计数器& c）时 Load(槽)=对象地址（原 AddrOf(槽)=引用槽
        //   自身地址，构造从别名槽读字段=错值）；普通类变量行为与原等价。
        const std::string capUnique = lookupVarName(capName).value_or("");
        ir::IRValue srcSlot = genIdentifierLvalue(
            std::make_unique<IdentifierExpr>(capName).get());
        const std::string extra =
            canon + "|" + std::to_string(ciC != nullptr ? ciC->totalSize : 0);
        ir::IRValue newObj = emitResult(
            ir::Opcode::NewObject, {ir::IRValue::constant(canon, "ptr")}, "ptr",
            extra, loc);
        emit(ir::Opcode::Call, {newObj, srcSlot}, ir::IRValue(), copyKeyOpt.value(),
             "void",
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
//   数组值捕获：槽=整组快照（CopyStruct typeSizeOf(数组) 字节位拷·1408）。
void IRGenerator::emitLambdaSnapshotStores(
    const std::string& lambdaName,
    const std::vector<std::string>& capturedNames,
    const std::vector<char>& captureRefs,
    const SourceLocation& loc) {
    for (std::size_t ci = 0; ci < capturedNames.size(); ++ci) {
        const std::string& cap = capturedNames[ci];
        const bool byRef = captureRefs[ci] != 0;
        // 1360 结果化：value_or("") 显形（miss 落 648 显形/宽度回退链·原语义）
        std::string capSrc = lookupSrcType(cap).value_or("");
        std::string capIrType = lookupVarType(cap).value_or("");
        // #1178：字段捕获回填（[=]/[&]/[字段] 显式声明=001 合法面）——字段名
        //   IR varStack 无登记，回填单源=generateLambda 已落 meta.srcTypes（482
        //   元数据表·含字段类型·与本函数同调用链先行落表）。meta miss（方法值
        //   蹦床复用径等）保持原 miss 面。与生成声明快照段/蹦床段同式三现·待
        //   归纳（ir_generator.hpp 1000 硬线成员提炼不可行·拆头另案挂账）。
        if (capIrType.empty() || capSrc.empty()) {
            const auto metaIt = lambdaCaptureMeta482_.find(lambdaName);
            if (metaIt != lambdaCaptureMeta482_.end() &&
                ci < metaIt->second.srcTypes.size()) {
                if (capSrc.empty()) capSrc = metaIt->second.srcTypes[ci];
                if (capIrType.empty()) capIrType = mapType(capSrc);
            }
        }
        if (capIrType.empty()) {
            // 648（miss 不可静默·110 同族）：捕获变量 IR 类型未登记=编译器内部
            //   缺陷（语义层回填捕获列表必对应 varStack 已登记变量或类字段），
            //   显形于此；driver 在 generate() 后 hasErrors() 统一中止，产物不
            //   落地。覆盖本函数下方 byRef AddrOf 与标量快照 StorePtr 两处 i64
            //   兜底 miss（同变量只报一次），原兜底表达式保留（控制流零变更）。
            diagnostics_.report(DiagnosticLevel::Error, loc,
                "内部错误: 捕获变量 '" + cap + "' IR 类型未登记 ——静默宽度兜底已废除（#648）");
        }
        const std::string slotSym = "?gstatic_" + lambdaSlotKey(lambdaName, cap);
        const bool isStructValCap =
            !byRef && semantic_ != nullptr &&
            semantic_->isStructType(types::canonical(capSrc));
        // 1408（形态①）：数组值捕获=聚合值语义——槽=整组快照（原「槽存基址
        //   （数组名退化=元素指针）」=槽 8 字节只存源数组首元素地址：浅共享
        //   +蹦床还原指针值传多槽捕获参数（槽 1..N-1 未初始化）+体内下标读
        //   参数槽本身=地址低 32 位当元素（h1 探针 1610174149 实录·#959 同族
        //   比浅共享更深）。槽宽=globalStatics 注册数组文本（codegen 按类型
        //   分派多 qword·259 静态数组同通道）；元素全标量=CopyStruct 位拷
        //   完整快照；元素含字符串/容器/类=句柄浅共享面另域（呈报挂账）。
        const bool isArrayValCap =
            !byRef && semantic_ != nullptr && types::isArray(capSrc);
        // 槽注册：globalStatics 值=源类型文本（codegen 按类型分派槽宽：
        //   结构体按 typeSizeOf / i128 双槽 / 其余 8 字节——与顶层静态同通道）
        module_->globalStatics[lambdaSlotKey(lambdaName, cap)] = capSrc;
        ir::IRValue slotAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                          slotSym, loc);
        if (byRef) {
            // 引用捕获：槽存被捕获变量地址（数组/结构体/标量统一地址形态，蹦床
            //   LoadPtr 还原=引用实参）。1321：固化地址取 genIdentifierLvalue
            //   四分流单点（#1023 同理念）——被捕获变量自身是引用变量（引用
            //   形参/局部引用，VarEntry.byRef=true）时 Load(槽)=被引用左值
            //   地址（别名透明），原无条件 AddrOf(槽) 固化外层帧别名槽自身
            //   地址：蹦床还原后体内读=别名槽内容当值（静默错值）、写=毁外层
            //   别名槽（m13/s4 错值·s6 段错误实录）；普通变量/静态兜底分支
            //   行为与原 AddrOf 逐字等价。1360 结果化：value_or("") 显形
            //   （语义层保证捕获已登记·形态兜面）。
            ir::IRValue srcAddr = genIdentifierLvalue(
                std::make_unique<IdentifierExpr>(cap).get());
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
            // 1321：深拷源地址同走 genIdentifierLvalue 单点——被捕获变量是
            //   结构体引用（点& r）时 Load(槽)=被引用对象基址（原 AddrOf(槽)
            //   =引用槽自身地址，位拷把「目标地址值」当结构体字段=静默错值
            //   s1 实录）；普通结构体变量行为与原 AddrOf 等价。1360 结果化：
            //   value_or("") 显形（语义层保证捕获已登记·形态兜面）。
            ir::IRValue srcAddr = genIdentifierLvalue(
                std::make_unique<IdentifierExpr>(cap).get());
            emitStructCopyWithFields(slotAddr, srcAddr, canon, loc,
                                     /*preFree=*/false, /*deepCopy=*/true);
        } else if (isArrayValCap) {
            // 1408（形态①）：数组值捕获快照=定义处整组拷入槽——源地址同走
            //   genIdentifierLvalue 单点（普通数组变量=AddrOf(槽0)=数组本体）；
            //   宽度=typeSizeOf(数组) 结果化分流（无值=编译器内部缺陷显形——
            //   数组已语义登记而大小查表 miss，禁静默取默认继续编译）。
            ir::IRValue srcAddr = genIdentifierLvalue(
                std::make_unique<IdentifierExpr>(cap).get());
            const cn::Optional<int> bytesOpt = semantic_->typeSizeOf(capSrc);
            if (!bytesOpt.has_value()) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                    "内部错误: 捕获数组 '" + cap + "' 大小查表 miss（#1408）");
            }
            // 业务路独立 if（else 新规：业务双路分流禁落否则块·显形后产物
            //   由 driver hasErrors 统一丢弃）
            if (bytesOpt.has_value()) {
                emit(ir::Opcode::CopyStruct, {slotAddr, srcAddr},
                     ir::IRValue(), std::to_string(bytesOpt.value()), "void",
                     loc);
            }
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
        trap.structReturnSize = semantic_->typeSizeOf(returnSrcType).value_or(8);
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
        // 1360 结果化：value_or("") 显形（miss 落 648 显形/宽度回退链·原语义）
        std::string capSrc = lookupSrcType(cap).value_or("");
        std::string capIrType = lookupVarType(cap).value_or("");
        // #1178：字段捕获回填（同 emitLambdaSnapshotStores——meta.srcTypes 单源
        //   ·closureId=lambda 名=meta 键；方法值蹦床复用径 meta miss=保持原 miss
        //   面·零行为变化）。同式三现·待归纳（同上注）。
        if (capIrType.empty() || capSrc.empty()) {
            const auto metaIt = lambdaCaptureMeta482_.find(closureId);
            if (metaIt != lambdaCaptureMeta482_.end() &&
                ci < metaIt->second.srcTypes.size()) {
                if (capSrc.empty()) capSrc = metaIt->second.srcTypes[ci];
                if (capIrType.empty()) capIrType = mapType(capSrc);
            }
        }
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
                   (semantic_->isStructType(types::canonical(capSrc)) ||
                    types::isArray(capSrc))) {
            // 结构体/数组值捕获：实参=槽地址（被调聚合值捕获参数经
            //   structParamIndexes 从指针 rep movsb——槽即固化缓冲；
            //   1408：数组并入同通道，原 LoadPtr 还原 8 字节指针值传多槽
            //   聚合参数=ABI 错位链根治）
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
        // 1360 结果化：has_value=条目已登记（原 nullptr 判定显形）
        const cn::Optional<const VarEntry*> peOpt = findVarEntry(param->name);
        if (peOpt.has_value()) {
            const VarEntry* pe = peOpt.value();
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
            // 1360 结果化：pe 与 lookupVar 同栈遍历——此分支 lookupVar 恒无值
            //   （原返回哨兵 reg(-1,"")·形态等价直写）
            // 1672 批10 甄别（#1360 处置①）：参数表由下方同一 params 经
            //   buildClosureParams943 构建（同源同序），findVarEntry miss=内部
            //   缺陷——原静默 push 空操作数实参（reg(-1,"")=渲染空串）会产出
            //   不可信 IR；补内部错误显形，占位 push 保留（控制流/产物丢弃口径不变）。
            diagnostics_.report(DiagnosticLevel::Error, loc,
                "内部错误: 蹦床转发参数 '" + param->name +
                    "' 条目未登记（#1360·1672）");
            callArgs.push_back(ir::IRValue::reg(-1, ""));
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
    // 750 复审返工：lambda 体=独立函数上下文——三枚累积提升函数级状态在入口
    //   save（随后 beginAccumScope 清空+按 lambda 体扫描），出口 restore：
    //   不清空则体内扫描结果污染外层（外层 lambda 先于循环出现 → 后续循环误用
    //   体内结果=错值）；不恢复则外层逃逸/别名面丢失=死循环。保存声明置于函数
    //   外层作用域（恢复点在函数尾）。
    std::vector<std::string> outerAliasEscaped750 =
        std::move(stringAliasEscaped_);
    const bool outerHasLambda750 = funcHasLambda_;
    const bool outerHasAlias750 = funcHasAlias_;
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
            ? semantic_->typeSizeOf(node->returnType).value_or(8) : 0;
    ir::IRFunction func;
    func.name = lambdaName;
    func.returnType = returnIrType;
    // 1571：lambda 体返回位契约判定须要源码返回类型（A2 拥有契约/85-a 聚合
    //   返回/933 引用返回深拷共用 function_->returnTypeSrc）。原实现只置 IR
    //   类型 returnType，returnTypeSrc 恒空 → 体内「返回 字符串字面量」不落堆
    //   （调用方 free 只读段指针=UB；v2 同形态已复制=分叉），本处补齐与普通
    //   函数（ir_decl.cpp 同字段）同口径。未标注=空类型。
    func.returnTypeSrc = node->returnType.empty() ? "空类型" : node->returnType;
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
        // 1360 结果化：value_or("") 显形（miss 落 648 显形/宽度回退链·原语义）
        std::string capType = lookupVarType(cap).value_or("");
        // #1178（2026-10-31）：类字段捕获类型回填——[=]/[&]/[字段] 显式声明
        //   捕获类字段（001 §lambda 捕获条合法面）。字段名在语义层 scopes_ 注
        //   入但 IR varStack 无登记（方法体 IR 只登记参数/局部）→ lookupVarType
        //   miss。isInstanceField 判定（varStack 各层 miss+类字段表命中·1472
        //   声明类锚）→ 回填字段源类型（快照槽布局/参数槽宽度全链消费·并经
        //   recordLambdaCaptureMeta482 落 meta.srcTypes 供固化/蹦床两处同源）。
        //   真未知名仍落 648 内部错误显形（miss 不可静默纪律不变）。
        if (capType.empty() && isInstanceField(cap)) {
            capType = mapType(classFieldType(fieldScopeClass(), cap).value_or(""));
        }
        std::string capSrc = lookupSrcType(cap).value_or("");
        // #1178：字段捕获源类型同步回填（结构体值捕获深拷/isStructValCapture/
        //   482 载体槽布局判定消费 capSrc——字段名 lookupSrcType 同样 miss）
        if (capSrc.empty() && isInstanceField(cap)) {
            capSrc = classFieldType(fieldScopeClass(), cap).value_or("");
        }
        capturedSrcTypes482.push_back(capSrc);
        if (capType.empty()) {
            // 648（miss 不可静默·110 同族）：捕获变量 IR 类型未登记=编译器内部
            //   缺陷（语义层回填捕获列表必对应 varStack 已登记变量或类字段），
            //   显形于此；driver 在 generate() 后 hasErrors() 统一中止，产物
            //   不落地。覆盖本循环下方捕获参数 paramType 与 VarEntry.type 两处
            //   i64 兜底 miss（同源同变量只报一次），原兜底表达式保留（控制流
            //   零变更）。
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
        // 1408（形态①）：数组值捕获并入聚合值传参通道——参数按 ptr 型收
        //   「快照槽地址」（structParamIndexes 位·prologue rep movsb 整组拷入
        //   参数槽）。原数组参数 paramType=capType（mapType(数组)=元素 IR 型
        //   如 i32）=1 槽收 8 字节指针值+槽 1..N-1 未初始化+体内下标读参数槽
        //   本身=ABI 错位链（h1 探针实录）。结构体分支原判据不变。
        const bool isArrayValCapture =
            !byRef && semantic_ != nullptr && types::isArray(capSrc);
        std::string unique = cap + "$" + std::to_string(varCounter_++);
        const std::string paramType =
            byRef ? "ptr"
                  : (isStructValCapture || isArrayValCapture
                         ? "ptr"
                         : (capType.empty() ? "i64" : capType));
        func.params.emplace_back(cap, paramType);
        func.paramUniques.push_back(unique);
        if (isStructValCapture || isArrayValCapture) {
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
    // 1571：lambda 体捕获名帧入栈（返回位拥有契约判定——捕获快照返回须复制
    //   落堆；见 ir_stmt.cpp visitReturnStmt A2 块）。体生成完毕即弹（嵌套
    //   lambda 逐层进出·与 varStack_ 层同序压弹）。
    lambdaCaptureFrames_.emplace_back(capturedNames.begin(),
                                      capturedNames.end());
    if (node->body != nullptr) {
        // 750 复审返工：lambda 体函数级别名逃逸/lambda 预扫描（P0-1/P0-3）。
        //   三枚状态 save 在函数入口、restore 在函数尾（见入口注释）。
        beginAccumScope(node->body.get());
        genBlock(node->body.get());
    }
    lambdaCaptureFrames_.pop_back();  // 1571：与上方便携入栈成对（体生成完毕）
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
    // 750 复审返工：恢复外层累积提升函数级状态（与入口 save 成对·见 genBlock 前注释）。
    stringAliasEscaped_ = std::move(outerAliasEscaped750);
    funcHasLambda_ = outerHasLambda750;
    funcHasAlias_ = outerHasAlias750;
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

} // namespace cn_compiler
