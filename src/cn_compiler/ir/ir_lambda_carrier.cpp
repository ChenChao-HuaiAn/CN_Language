// 482 lambda 多实例独立快照·载体快照槽族（#1178 轮拆分·2026-10-31）
// 职责：#482 载体快照槽族单点——captureBytes482/recordLambdaCaptureMeta482/
//   makeCarrierSlots482/emitSharedToCarrier482/emitCarrierToCarrier482/
//   resolveLambdaOrigin482/updateCarrierOnAssign482/emitReturnOriginSnapshot482。
//   自 ir_lambda.cpp 机械搬移（896 行·警戒线先拆再加·纯搬移零行为变更——
//   1415/1472 拆分先例同款）。
#include <string>
#include <vector>

#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

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
cn::Optional<int> IRGenerator::captureBytes482(const std::string& capSrc) {
    // 641-fix 静态槽宽度口径：lambda 捕获槽恒 8 字节槽宽（固化/还原走
    //   T_整64 全宽——641 记述「lambda/捕获路径不经静态值码」）；i128=16、
    //   结构体/数组=typeSizeOf（CopyStruct 整块拷，浅字节口径与
    //   emitLambdaSnapshotStores 结构体快照一致）。
    // 1360 结果化（边界件·报告标注）：查表 miss=无值——但原哨兵 8 与合法
    //   「标量/字符串捕获槽宽恰为 8」值域重叠，Optional 无法分离两面；
    //   消费点 value_or(8) 显形（8=静态槽缺省宽·语义在调用点写出），
    //   查表命中面（i128=16/结构体/数组）行为逐点不变。
    if (semantic_ != nullptr && !capSrc.empty()) {
        if (types::isI128(types::canonical(capSrc))) return 16;
        const std::string canon = types::canonical(capSrc);
        if (semantic_->isStructType(canon) || types::isArray(capSrc)) {
            const int bytes = semantic_->typeSizeOf(capSrc).value_or(8);
            // 静默豁免业务值: size 合法域恒正守卫——typeSizeOf 结果非正=上游异常值不采用（落无值·消费点 value_or(8) 显形·1360 既有口径），非查表默认值
            if (bytes > 0) return bytes;
        }
    }
    return std::nullopt;  // 查表 miss=无值（非 128/结构体/数组形态）
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
        // 1360 结果化：value_or(8) 显形——8=静态槽缺省宽（641-fix 口径·
        //   查表 miss 与标量捕获同落 8，行为逐点不变）
        const int bytes = captureBytes482(slots[ci].srcType).value_or(8);
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
        // 1360 结果化：value_or(8) 显形——8=静态槽缺省宽（641-fix 口径·
        //   查表 miss 与标量捕获同落 8，行为逐点不变）
        const int bytes = captureBytes482(slots[ci].srcType).value_or(8);
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
        // 1360 结果化：value_or(8) 显形——8=静态槽缺省宽（641-fix 口径）
        const int bytes = captureBytes482(dstSlots[ci].srcType).value_or(8);
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

// 1360 结果化：无值=来源未知（原空串哨兵退役·出口空串→无值单点转换）。
cn::Optional<std::string> IRGenerator::resolveLambdaOrigin482(Expr* rhs,
                                                const std::string& literalName) {
    if (rhs == nullptr) return std::nullopt;
    switch (rhs->getType()) {
        case NodeType::LambdaExpr:
            // 调用者须在 genExpr(rhs) 之后立即调用（lastLambdaName_ 尚未被
            //   消费清空——registerClosureIfNeeded 场景由调用方先行快照）。
            // 1360 结果化：两侧空串→无值（原空串=来源未知同义）。
            if (!literalName.empty()) return literalName;
            if (lastLambdaName_.empty()) return std::nullopt;
            return lastLambdaName_;
        case NodeType::IdentifierExpr: {
            const std::string& name = static_cast<IdentifierExpr*>(rhs)->name;
            auto it = closureInfo_.find(name);
            if (it == closureInfo_.end()) return std::nullopt;
            if (it->second.lambdaName.empty()) return std::nullopt;
            return it->second.lambdaName;
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
                if (it != funcReturnLambda482_.end() && !it->second.empty()) {
                    return it->second;
                }
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
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
    // 1360 结果化：无值=来源未知（has_value 分流）
    const cn::Optional<std::string> originOpt = resolveLambdaOrigin482(rhs, literalName);
    if (!originOpt.has_value()) {
        // 来源未知：清除陈旧登记（p482 根因：重赋值后调用仍展开旧定义点）；
        //   lambdaVarNames_ 留痕保留（间接调用诊断判据用·655 返工）。
        // 静默豁免运行时API: erase 删除计数消费——>0=有陈旧登记被清除（标准容器 API 返回值·非失败通道哨兵）
        if (closureInfo_.erase(varName) > 0 && fromPlainAssign) {
            carrierAssigned482_.insert(varName);
        }
        return;
    }
    const std::string& origin = originOpt.value();
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
    // 静默豁免业务值: 复用/新建二元互斥枚举——else=新建载体槽合法分支（reuse 条件不满足即新建·非兜底）
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
    // 静默豁免业务值: 纯赋值/声明初始化二元互斥枚举——else=声明初始化位从集移除（482×815 块级回滚协议·非兜底）
    } else {
        carrierAssigned482_.erase(varName);
    }
    lambdaVarNames_.insert(varName);
}

void IRGenerator::emitReturnOriginSnapshot482(Expr* rhs) {
    if (rhs == nullptr) return;
    // 1360 结果化：无值=来源未知（has_value 分流）
    const cn::Optional<std::string> originOpt = resolveLambdaOrigin482(rhs);
    if (!originOpt.has_value()) {
        // 未知来源返回：已有已知痕迹即置冲突哨兵（同一函数多返回路径
        //   来源不一致=接收点静态解析不可信）。
        if (!currentReturnOrigin482_.empty()) currentReturnOrigin482_ = "?";
        return;
    }
    const std::string& origin = originOpt.value();
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
