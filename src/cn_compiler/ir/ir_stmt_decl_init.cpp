// CN-IR生成器实现：变量声明·普通表达式初始化族（350 重构E2 自 ir_stmt_decl.cpp 纯机械搬移）
// 职责（函数体逐字搬移·零语义变化；成员声明仍在 ir.hpp）：
//   genExprInitializer 及其八子方法——闭包登记/转移浅交接/retbuf 整块拷/
//   结构体 ptr 拷/盒亡登记/类深拷/拆包绑定/串拥有化。
#include <cstdint>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_stmt_decl_ctx.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ---- 族⑩：普通表达式初始值 -> Store（目标用唯一内部名，保证遮蔽变量写入
//   自己的槽）。true = 已处理（命中任一初始化形态并发射完毕）。 ----
bool IRGenerator::genExprInitializer(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer != nullptr) {
        // 632（引用返回接收面·宿主侧根治）：右值=引用返回调用 → 用户类深拷。
        //   须先于下方 genExpr 求值（抑制语境取「被引用左值地址」，防双求值）。
        if (tryGenRefReturnClassCopyInit(node, ctx)) return true;
        // Task 2.10 lambda 赋值：`自动 加倍 = [...]...`——
        //   先 genExpr（生成匿名函数并记录 lastLambdaName_/lastLambdaCaptures_），
        //   再登记闭包关联（调用 `加倍(...)` 时展开捕获实参）
        ir::IRValue value = genExpr(node->initializer.get());
        // 031（#1075）：方法值初始化（对象.方法 / 方法值变量拷贝链）——两槽值
        //   整拷（从物化临时/源变量读两槽写目标两槽），须在通用单槽 Store 之前
        //   返回（单槽写会把两槽值截半）。判定=目标声明类型（ctx.srcType 回填）。
        if (tryGenMethodValueInit(node, ctx)) return true;
        // 482：lambda 字面量名先留快照（registerClosureIfNeeded 会消费清空
        //   lastLambdaName_；载体登记需要该名解析来源）。
        const std::string pendingLambda482 = lastLambdaName_;
        registerClosureIfNeeded(node);
        // 482：载体快照槽登记（声明初始化位统一——lambda 字面量/拷贝链/接收
        //   调用返回三形态；见 updateCarrierOnAssign482）。
        updateCarrierOnAssign482(node->name, node->initializer.get(),
                                 node->location, pendingLambda482);
        // 096（937·008 收官总攻第三轮）转移浅交接**前置**（见 tryGenTransferInit）
        if (tryGenTransferInit(node, value, ctx.unique)) return true;
        // 087（m85 返回面）数组 retbuf 整块 CopyStruct（见 tryGenArrayRetbufInit）
        if (tryGenArrayRetbufInit(node, value, ctx.unique)) return true;
        // 结构体变量初始化值（974/79-a/182/280，见 tryGenStructPtrInit）
        if (tryGenStructPtrInit(node, value, ctx)) return true;
        // plans/019 阶段3b 类对象深拷（见 tryGenClassCopyInit）
        if (tryGenClassCopyInit(node, value, ctx.srcType, ctx.unique)) return true;
        // 058-ⅡB 拆包绑定位值语义深拷（见 tryGenUnwrapBindInit）
        if (tryGenUnwrapBindInit(node, value, ctx.srcType, ctx.unique)) return true;
        // plans/019 阶段4'（方案A）：拥有型字符串初始化拥有化
        ownStringInitValue(node, ctx.srcType, value);
        if (value.type != ctx.irType && !ctx.irType.empty()) {
            value = emitResult(ir::Opcode::Cast, {value}, ctx.irType, "",
                               node->location);
        }
        emit(ir::Opcode::Store, {value}, ir::IRValue(),
             ctx.unique, ctx.irType, node->location);
    }
    return false;
}

// 族⑩ 子方法：lambda/实例方法作值 的闭包关联登记（Task 2.10/P3-23 补完）。
//   调用 `加倍(...)` 时展开定义处固化的捕获实参。
// 族⑩ 子方法（816·655 返工遗留乙）：拷贝形态传递登记——初始化器为**标识符**
//   且该名已在 lambda/方法值留痕集（变量 f = 取和;）时，f 同样是 lambda 变量
//   （其槽持有蹦床地址 ?lambdaN$trap / 方法地址，调用经间接通道），须与直接
//   初始化同权留痕 lambdaVarNames_。链式拷贝（h = g = 取和）因声明序保证源头
//   先登记而自然传递。现状拷贝形态的源类型串经 432/485 推断回填链可解析、
//   间接调用尾通常不掉穿（①误报用例面无实录），但留痕集语义=「曾以 lambda/
//   方法值登记过的变量名全集」，拷贝名缺席=oracle 残缺：一旦类型串链出现新
//   缺口，该名即被误判「真全域 miss」落 #655 诊断①误报（655 返工遗留呈报
//   本案）。普通变量拷贝（整32 y = x; x 不在集）不触本分支——负向拒绝面零
//   扩大。查询 miss=「来源非 lambda 变量」的语义本身（集合成员判定·非查表
//   兜底），不落 miss 不可静默面。
//
//   快径整体传递（机制级当场修·1002-⑥ 实锤）：closureInfo_ 命中时把来源
//   ClosureInfo 整体赋给拷贝名——否则拷贝调用掉进间接通道丢**绑定 this**：
//   方法值槽=裸方法地址，快径调用=直调+captureArgs（this=Load 对象槽）展开，
//   间接调用只发用户实参（被调方把首实参当 this 解引用=探针 C0000005
//   fault=6 实锤·1002 立案前存量缺陷）。lambda 值拷贝同传递（快径直调与
//   蹦床 CallIndirect 语义等价——655 返工④已论证）。跨函数安全面：lambda
//   体入口 closureInfo_ clear（485）使体内拷贝判据必 miss 快径表→只留痕
//   走蹦床通道（captureArgs 寄存器系外层函数定义，跨函数引用被 clear 纪律
//   结构性排除）。
void IRGenerator::propagateLambdaCopyBinding816(VarDecl* node) {
    if (node->initializer == nullptr ||
        node->initializer->getType() != NodeType::IdentifierExpr) {
        return;
    }
    const auto& srcName =
        static_cast<const IdentifierExpr*>(node->initializer.get())->name;
    const auto srcClosure = closureInfo_.find(srcName);
    if (srcClosure != closureInfo_.end()) {
        // 482：载体快照槽不随名字复制（槽与载体一一对应）——拷贝名的独立载体
        //   槽由 updateCarrierOnAssign482 在其后重建并整拷（防两变量共享槽=
        //   一侧重赋值污染另一侧）；元数据（lambdaName/返回类型/形参）照传。
        ClosureInfo copied = srcClosure->second;
        copied.carrierCaptures.clear();
        closureInfo_[node->name] = std::move(copied);
        lambdaVarNames_.insert(node->name);
    } else if (lambdaVarNames_.count(srcName) > 0) {
        // 快径已亡（485 clear/蹦床通道场景）：仅留痕——拷贝调用经类型串
        //   解析走蹦床间接通道，lambdaVarNames_ 保证不落 #655 诊断①。
        lambdaVarNames_.insert(node->name);
    }
}

void IRGenerator::registerClosureIfNeeded(VarDecl* node) {
    propagateLambdaCopyBinding816(node);  // 816 拷贝传递登记（独立函数·守百行线）
    // 031（#1075）：实例方法作值（对象.方法）不再是本闭包登记的消费面——方法值
    //   走两槽值通道（ir_method_value.cpp）且绑定对象随值自带（无编译期旁路表），
    //   转发/拷贝/跨函数天然安全；本函数回归纯 lambda 语义（Task 2.10）。
    if (node->initializer->getType() == NodeType::LambdaExpr &&
        !lastLambdaName_.empty()) {
        // 655 返工：lambda 变量名留痕（与 closureInfo_ 生命周期解耦——visitLambdaExpr
        //   入口 clear 会清掉快径表，但蹦床通道合法性判定仍需知道该名曾是 lambda）
        lambdaVarNames_.insert(node->name);
        ClosureInfo info;
        info.lambdaName = lastLambdaName_;
        info.captures = lastLambdaCaptures_;
        info.returnIrType = lastLambdaReturnIrType_;
        info.srcReturnType = lastLambdaSrcReturnType_;  // 485：源返回类型（sret 判定）
        info.paramTypes = lastLambdaParamTypes_;  // 337-a：用户形参类型（ABI 定标）
        {
        // 缺陷修复（[=] 快照 / [&] 引用，规格书04-一D）：
        //   捕获实参在"lambda 定义处"（即此处）求值并固化：
        //     [=]/[变量] 值捕获：genExpr(变量) 读取当前值 -> 快照
        //       （原实现在调用点读取，捕获变量后续被修改时闭包读到最新值——
        //       与规格"值捕获=捕获时复制"不符，实测 lambda值快照: 1000 而非 101）
        //     [&] 引用捕获：AddrOf(变量) 取变量地址 -> 指针，
        //       闭包内解引用读最新值、经指针写回外部（引用语义精确）
        //   调用 `加倍(...)` 时直接展开这些已固化的捕获实参（前置）。
        const std::size_t capCount = lastLambdaCaptures_.size();
        const std::size_t refCount = lastLambdaCaptureRefs_.size();
        for (std::size_t ci = 0; ci < capCount; ++ci) {
            const std::string& cap = lastLambdaCaptures_[ci];
            const bool byRef = (ci < refCount) && lastLambdaCaptureRefs_[ci] != 0;
            if (byRef) {
                // 引用捕获：&变量（AddrOf 取变量槽地址）
                const std::string capUnique = lookupVarName(cap);
                const std::string capIrType = lookupVarType(cap);
                if (capIrType.empty()) {
                    // 648（miss 不可静默·110 同族）：捕获变量 IR 类型未登记=
                    //   编译器内部缺陷（语义层回填捕获列表必对应 varStack 已登记
                    //   变量），显形于此；driver 在 generate() 后 hasErrors() 统一
                    //   中止，产物不落地。原 i64 兜底保留（控制流零变更）。
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "内部错误: 捕获变量 '" + cap + "' IR 类型未登记 ——静默宽度兜底已废除（#648）");
                }
                info.captureArgs.push_back(emitResult(
                    ir::Opcode::AddrOf,
                    {ir::IRValue::var(capUnique, capIrType.empty() ? "i64" : capIrType)},
                    "ptr", capUnique, node->location));
            } else {
                const std::string capSrc = lookupSrcType(cap);
                // 缺陷修复（[=] 结构体值捕获快照）：结构体是值类型，值捕获须
                //   在"定义处"深拷贝快照到临时缓冲区——不能仅存 AddrOf 指针：
                //   指针指向原变量，定义后修改会破坏快照；且闭包参数槽被当
                //   结构体数据本身时，字段访问读到的是地址值字节=垃圾
                //   （实测 值捕获p.x: 553448424 而非 1）。
                if (semantic_ != nullptr &&
                    semantic_->isStructType(types::canonical(capSrc))) {
                    // 940：值捕获结构体快照=定义处**深拷**（原裸 CopyStruct 位拷
                    //   仅首层字节——字符串字段句柄浅共享，源重赋值/块析构释放堆串
                    //   后同帧快径读悬垂（空串）；容器/类字段浅共享同病）。深拷=
                    //   emitStructCopyWithFields 字段级独立副本（串 __cn_str_copy
                    //   落堆·容器/类字段 NewObject+拷贝构造·嵌套递归）——源保持
                    //   拥有、快照独立（001 §一D 无悬垂面）。纯 POD 结构体内部落
                    //   CopyStruct 位拷零开销不变。
                    const std::string capCanon = types::canonical(capSrc);
                    const std::string temp =
                        "__capstruct" + std::to_string(varCounter_++);
                    emit(ir::Opcode::Alloca, {},
                         ir::IRValue::reg(regCounter_++, "ptr"),
                         temp, "ptr", node->location);
                    registerVarSlots(temp, capSrc);
                    ir::IRValue dstAddr = emitResult(
                        ir::Opcode::AddrOf, {ir::IRValue::var(temp, "i64")},
                        "ptr", temp, node->location);
                    ir::IRValue srcAddr = emitResult(
                        ir::Opcode::AddrOf,
                        {ir::IRValue::var(lookupVarName(cap), "i64")},
                        "ptr", lookupVarName(cap), node->location);
                    emitStructCopyWithFields(dstAddr, srcAddr, capCanon,
                                             node->location,
                                             /*preFree=*/false,
                                             /*deepCopy=*/true);
                    // 940：副本**不登记块出口释放**（安全优先裁决）——闭包值拷贝
                    //   传递（816 快径继承 closureInfo_）后可在快径中继续消费本
                    //   临时副本；块出口释放会令拷贝名跨块调用读已释放副本=
                    //   悬垂回归。释放归属=闭包值生命周期（CN 闭包值=裸函数
                    //   指针无析构语义·语言级缺口）→ 挂账独立任务
                    //   （双侧同规：v2 局部槽快照同不释放，零分叉）。
                    info.captureArgs.push_back(dstAddr);
                } else if (semantic_ != nullptr &&
                           (types::canonical(capSrc) == "字符串" ||
                            semantic_->isClassType(types::canonical(capSrc)))) {
                    // 940：字符串/容器/类值捕获=定义处深拷独立副本（字符串
                    //   __cn_str_copy 落堆·容器/类 NewObject+拷贝构造）——
                    //   原裸句柄浅共享，源重赋值/块析构释放后同帧快径读悬垂
                    //   （F1/F2 探针实录）。其它形态（标量/指针/数组/128 位）
                    //   走下方原路径零变化。
                    info.captureArgs.push_back(
                        evalValueCaptureCopy(cap, capSrc, node->location));
                } else {
                    // 值捕获：定义处读取变量值快照（i128 双槽 Load 生成双寄存器值）
                    info.captureArgs.push_back(genExpr(
                        std::make_unique<IdentifierExpr>(cap).get()));
                }
            }
        }
        }  // else：lambda 值/引用捕获路径（绑定方法走上方 Load 分支）
        closureInfo_[node->name] = info;
        lastLambdaName_.clear();
        lastLambdaCaptures_.clear();
        lastLambdaReturnIrType_.clear();
        lastLambdaSrcReturnType_.clear();
        lastLambdaCaptureRefs_.clear();
    }
}

// 族⑩ 子方法：096（937·008 收官总攻第三轮）转移浅交接**前置**——
//   结构体源=**整块搬迁零拷贝**（CopyStruct 位拷·拥有字段句柄直移=所有权
//   交接——078 同款设施模型）+**源槽全清零**（$s 槽命名·79-a/078 同款幂等
//   模型：源 RAII 对零句柄空安全跳过）；类/容器/字符串源=句柄直拷+单槽清零
//   （72-a 现行）。〔基准=019〕第二句：转移()=显式放弃拷贝换零拷贝。
//   79-a（2026-09-12 第七十九轮）：含串字段结构体——源为调用返回（retbuf，
//   被调方返回移出已跳过字段释放）=浅拷接管（零拷贝，句柄唯一持有者转为
//   目标）；源为标识符/成员（值语义拷贝）=深拷（字段级 __cn_str_copy 落堆，
//   源保持拥有）——浅拷共享 + 双端释放=悬垂，深拷是安全前提。
//   true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::tryGenTransferInit(VarDecl* node, const ir::IRValue& value,
                                     const std::string& unique) {
    std::string transferSrcNameIr;
    if (semantic_ != nullptr && value.type == "ptr" &&
        node->initializer != nullptr &&
        node->initializer->getType() == NodeType::IdentifierExpr &&
        semantic_->isTransferDecl(static_cast<const void*>(node),
                                  transferSrcNameIr)) {
        const std::string srcUnique = lookupVarName(transferSrcNameIr);
        const std::string tgtCanon = types::canonical(node->typeName);
        if (semantic_->isStructType(tgtCanon)) {
            ir::IRValue dstAddr = emitResult(
                ir::Opcode::AddrOf, {ir::IRValue::var(unique, "i64")}, "ptr",
                unique, node->location);
            ir::IRValue srcAddr = emitResult(
                ir::Opcode::AddrOf, {ir::IRValue::var(srcUnique, "i64")},
                "ptr", srcUnique, node->location);
            const int bytes = semantic_->typeSizeOf(tgtCanon);
            if (bytes > 0) {
                emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr},
                     ir::IRValue(), std::to_string(bytes), "void",
                     node->location);
            }
            auto slotIt = function_->varSlots.find(srcUnique);
            const int slots =
                (slotIt != function_->varSlots.end() && slotIt->second > 0)
                    ? slotIt->second : 1;
            for (int s = 0; s < slots; ++s) {
                const std::string slotName =
                    s == 0 ? srcUnique : srcUnique + "$s" + std::to_string(s);
                emit(ir::Opcode::Store,
                     {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                     slotName, "i64", node->location);
            }
            lastExpr_ = dstAddr;
            return true;
        }
        emit(ir::Opcode::Store, {value}, ir::IRValue(), unique,
             "ptr", node->location);
        ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                      node->location);
        emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique,
             "i64", node->location);
        lastExpr_ = value;
        return true;
    }
    return false;
}

// 族⑩ 子方法：087（m85 返回面）数组变量初始化值为"调用返回的 retbuf 基址
//   （ptr）"——整块 CopyStruct（bytes=typeSizeOf(数组)）到本地多槽。原通用
//   Store= w 槽存指针值（8B 句柄≠数组内容·w[i] 读垃圾——p1004_02 w=垃圾实录）。
//   true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::tryGenArrayRetbufInit(VarDecl* node, const ir::IRValue& value,
                                        const std::string& unique) {
    if (semantic_ != nullptr && value.type == "ptr" &&
        node->initializer != nullptr &&
        node->initializer->getType() == NodeType::CallExpr &&
        types::isArray(types::canonical(node->typeName))) {
        ir::IRValue dstAddr = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(unique, "i64")}, "ptr",
            unique, node->location);
        const int bytes087 =
            semantic_->typeSizeOf(types::canonical(node->typeName));
        if (bytes087 > 0) {
            emit(ir::Opcode::CopyStruct, {dstAddr, value},
                 ir::IRValue(), std::to_string(bytes087), "void",
                 node->location);
        }
        lastExpr_ = dstAddr;
        return true;
    }
    return false;
}

// 族⑩ 子方法：结构体变量初始化值为"函数返回的结构体地址（ptr）"（Task 完善A）：
//   学生 张三加 = 加分(张三) —— 值是指向返回临时结构体的指针，
//   需拷贝到本变量槽区（按值拷贝）。源=成员表达式改走 lvalueAddress（974）；
//   可选<T> = 无（NullLiteral）=逐槽零初始化（79-a 探针新证缺陷）；深拷/
//   浅拷按来源分派（79-a）+ 182/280 盒亡登记（registerPendingBoxVar）。
//   true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::tryGenStructPtrInit(VarDecl* node, ir::IRValue value,
                                      const DeclGenCtx& ctx) {
    if (semantic_ != nullptr && value.type == "ptr" &&
        semantic_->isStructType(types::canonical(node->typeName))) {
        const std::string declCanon = types::canonical(node->typeName);
        // 974（104-001 同族·声明初始化位拷出）：源=成员表达式（类聚合字段
        //   `对 复制 = h.槽`）——genExpr 对成员链的产物=字段槽**值**被再解
        //   一层（956 铁证：指针加载 多解→StructCopy 从野地址拷=稳定段错误
        //   ·样本 run_err/p0956_02）。源地址改走 lvalueAddress（字段左值
        //   地址·973 统一查询后推导正确）。
        if (node->initializer->getType() == NodeType::MemberExpr) {
            value = lvalueAddress(node->initializer.get());
        }
        ir::IRValue dstAddr = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(ctx.unique, "i64")},
                                         "ptr", ctx.unique, node->location);
        // 79-a 探针新证缺陷（既有，非本轮引入）：可选<T> 声明初始化为 无
        //   （NullLiteral）——零值语义（无值），不是结构体地址。原实现按
        //   CopyStruct 从「地址 0」拷贝 -> 段错误（可选<字符串> 空 = 无 实测
        //   mov r9,[rbp-200(=0)] 崩）。改为逐槽零初始化：条件位=0（释放面
        //   条件释放跳过）、值位=0（空安全）。
        if (node->initializer != nullptr &&
            node->initializer->getType() == NodeType::NullLiteral) {
            auto slotIt = function_->varSlots.find(ctx.unique);
            const int slots =
                (slotIt != function_->varSlots.end() && slotIt->second > 0)
                    ? slotIt->second : 1;
            for (int s = 0; s < slots; ++s) {
                const std::string slotName =
                    s == 0 ? ctx.unique : ctx.unique + "$s" + std::to_string(s);
                emit(ir::Opcode::Store,
                     {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                     slotName, "i64", node->location);
            }
            lastExpr_ = dstAddr;
            return true;
        }
        const bool srcIsCall = node->initializer != nullptr &&
                               node->initializer->getType() == NodeType::CallExpr;
        emitStructCopyWithFields(dstAddr, value, declCanon, node->location,
                                 /*preFree=*/false, /*deepCopy=*/!srcIsCall);
        // 182（1019·008 树）：结果/可选<析构类> 局部变量盒亡登记（变量级·
        //   v2 946 变量模型同构）——调用返回接收盒（装盒() 形态）889 表达式面
        //   不盖（pendingBoxCopies 仅 正常(类值) 字面量位·返回位豁免）=盒亡
        //   无析构面泄漏（551 r 盒 asm 实证）。登记 {tagAddr, fieldAddr, 载荷}
        //   ——释放面（块出口+函数尾）Call __cn_box_class_delete（tag 假=错误
        //   态垃圾句柄免疫·摘取清槽幂等）+DeleteObject 空安全。889 表达式面
        //   双登记=幂等无害（其清值字段后本面摘句柄=0）。
        registerPendingBoxVar(node, declCanon, dstAddr, value, ctx.unique);
        lastExpr_ = value;
        return true;
    }
    return false;
}

// 族⑩ 子方法：182/280 结果/可选<析构类> 局部变量盒亡登记——
//   登记面 {tagAddr, fieldAddr, 载荷} 入 pendingBoxVars_（释放面=块出口+
//   函数尾 Call __cn_box_class_delete + DeleteObject 空安全）。
void IRGenerator::registerPendingBoxVar(VarDecl* node, const std::string& declCanon,
                                        const ir::IRValue& dstAddr,
                                        const ir::IRValue& value,
                                        const std::string& unique) {
    {
        const bool declIsBox =
            isResultType(declCanon) ||
            isOptionalType(declCanon);
        // 双登记防护（577 崩实录·全量 1019 捕获）：初值=内置构造器调用
        //   （正常/某些/错误）时 889 表达式面已登记同一盒值字段——两面
        //   逆序释放后跑面 emitContainerElemFreeFor 读空对象解引用崩
        //   （C0000374）——跳过（表达式面管辖）；其余初值（调用返回
        //   装盒() 形态等）无表达式面=本面登记。
        bool ctorInitCovered = false;
        if (node->initializer->getType() == NodeType::CallExpr) {
            const CallExpr* ice = static_cast<const CallExpr*>(
                node->initializer.get());
            if (ice->callee != nullptr &&
                ice->callee->getType() == NodeType::IdentifierExpr) {
                const std::string& cn =
                    static_cast<const IdentifierExpr*>(ice->callee.get())
                        ->name;
                ctorInitCovered = cn == "正常" || cn == "某些" ||
                                  cn == "错误";
            }
        }
        // 280：两面统一本变量面登记（单一权威·返回位移交豁免=273 刀②同键）
        //   ——原 !ctorInitCovered 防护（577）令 正常() 构造盒只登记表达式面：
        //   浅拷后盒槽与源槽共享句柄→函数尾表达式面析构源槽=「返回 盒」retbuf
        //   副本死句柄双放（p273a C0000374）。ctorInitCovered 时先清源槽值字段
        //   （表达式面 LoadPtr=0 幂等跳过）——布局归一后值槽 @8·273 v1 回归根因已消。
        if (declIsBox && node->initializer != nullptr) {
            if (ctorInitCovered) {
                const StructDecl* sdZ = semantic_->findStruct(declCanon);
                const int voZ = sdZ ? semantic_->fieldOffsetOf(sdZ, "值") : -1;
                if (voZ >= 0) {
                    ir::IRValue srcField280 = emitResult(
                        ir::Opcode::FieldAddr, {value}, "ptr",
                        std::to_string(voZ), node->location);
                    ir::IRValue zero280 = emitResult(
                        ir::Opcode::ConstInt, {}, "i64", "0", node->location);
                    emit(ir::Opcode::StorePtr, {srcField280, zero280},
                         ir::IRValue(), "", "i64", node->location);
                }
            }
            {
                std::string payload280;
                if (isResultType(declCanon)) {
                    const std::vector<std::string> rargs280 =
                        resultTypeArgs(declCanon);
                    if (rargs280.size() == 2)
                        payload280 = types::canonical(rargs280[0]);
                } else {
                    payload280 = types::canonical(
                        optionalTypeArg(declCanon));
                }
                const ClassInfo* pci280 = payload280.empty()
                    ? nullptr : semantic_->findClass(payload280);
                bool pDtor280 = false;
                if (pci280 != nullptr) {
                    for (const auto& mk : pci280->methods) {
                        if (mk.second.isDestructor) { pDtor280 = true; break; }
                    }
                }
                if (pDtor280 && semantic_->findCopyConstructor(payload280) != nullptr) {
                    const StructDecl* sd280 = semantic_->findStruct(declCanon);
                    const int vo280 =
                        sd280 ? semantic_->fieldOffsetOf(sd280, "值") : -1;
                    const int co280 =
                        sd280 ? semantic_->fieldOffsetOf(
                                 sd280, isResultType(declCanon)
                                 ? "正常" : "有值") : -1;
                    if (vo280 >= 0 && co280 >= 0) {
                        ir::IRValue tagAddr280 =
                            co280 == 0 ? dstAddr
                                : emitResult(ir::Opcode::FieldAddr, {dstAddr},
                                             "ptr", std::to_string(co280),
                                             node->location);
                        ir::IRValue fieldAddr280 =
                            vo280 == 0 ? dstAddr
                                : emitResult(ir::Opcode::FieldAddr, {dstAddr},
                                             "ptr", std::to_string(vo280),
                                             node->location);
                        pendingBoxVars_.emplace_back(tagAddr280, fieldAddr280,
                                                     payload280, unique, false);
                    }
                }
            }
        }
    }
}

// 族⑩ 子方法：plans/019 阶段3b（2026-09-10）转移浅交接（性能主项）——语义层
//   已把转移(源) 改写为源标识符并登记节点；此处跳过 NewObject+拷贝构造深拷贝，
//   改为句柄直拷 + **源槽清零**：源变量 RAII 析构对零句柄走既有空安全
//   跳过（DeleteObject test/je），目标析构真句柄=恰好一次释放；条件分支
//   两路径均正确（条件假=转移未执行=句柄仍在源槽=源析构正常释放）。
// 缺陷1 修复：类对象初始化（资源 乙 = 甲）——类对象是堆指针语义，
//   直接 Store 源指针会让两个变量共享同一堆地址，RAII 重复释放堆损坏。
//   正确语义：新建独立堆对象 + 逐字段 CopyStruct 深拷贝。
//   true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::tryGenClassCopyInit(VarDecl* node, const ir::IRValue& value,
                                      const std::string& srcType,
                                      const std::string& unique) {
    if (semantic_ != nullptr && value.type == "ptr" &&
        node->initializer->getType() == NodeType::IdentifierExpr) {
        const std::string initName =
            static_cast<IdentifierExpr*>(node->initializer.get())->name;
        std::string initSrcType = lookupSrcType(initName);
        // 宿主根治（2026-09-01）：源为顶层类静态（不在 varStack_，lookupSrcType
        //   为空）——类型取 globalStaticType；拷贝构造 byRef 传静态槽地址
        //   （&?gstatic_名，解引用即对象指针——指针槽模型与局部变量槽同构）。
        //   原实现漏此分支：浅 Store 共享指针，RAII 析构双释放堆损坏（0xC0000374）。
        const bool initIsStatic =
            initSrcType.empty() && semantic_->isGlobalStatic(initName);
        if (initIsStatic) {
            initSrcType = semantic_->globalStaticType(initName);
        }
        const std::string canonSrc = types::canonical(initSrcType);
        const std::string canonTgt = types::canonical(srcType);
        if (semantic_->isClassType(canonTgt) &&
            semantic_->isClassType(canonSrc)) {
            const ClassInfo* ci = semantic_->findClass(canonTgt);
            if (ci != nullptr) {
                // value 即源对象指针（Load 源变量槽 / LoadPtr 静态槽）
                const std::string extra = canonTgt + "|" +
                                          std::to_string(ci->totalSize);
                ir::IRValue newObj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(canonTgt, "ptr")},
                    "ptr", extra, node->location);
                // 方案A（2026-08-25）：目标类有拷贝构造（类名(类名& 其他)）
                //   时，初始化拷贝改调拷贝构造（深拷贝），而非 CopyStruct 浅拷贝
                //   （含裸指针字段浅拷贝析构双释放 0xC0000374，映射 乙 = 甲 实测）。
                //   byRef ABI：拷贝构造引用参数按"被引用左值地址"传参——
                //   源为标识符变量时传 源变量槽地址（&甲），体内经 byRef
                //   解引用得源对象指针（与 ir_expr.cpp 类赋值路径一致）。
                const ClassMemberInfo* copyCtor =
                    semantic_->findCopyConstructor(canonTgt);
                if (copyCtor != nullptr) {
                    const std::string copyOwner =
                        copyCtor->ownerClass.empty() ? canonTgt
                                                     : copyCtor->ownerClass;
                    const std::string srcUnique = lookupVarName(initName);
                    ir::IRValue srcAddr;
                    if (initIsStatic) {
                        // 源为顶层类静态：槽地址 = ?gstatic_名 符号地址
                        srcAddr = emitResult(
                            ir::Opcode::ConstString, {}, "ptr",
                            "?gstatic_" + initName, node->location);
                    } else if (isByRefCapture(initName)) {
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
                    // 123（930·用户裁决甲·边界定音）：findCopyConstructor 沿链
                    //   可返回**继承来的**祖先拷贝构造（本类无自有形态）——它
                    //   只拷其链覆盖的基类字段，派生侧各层字段原被静默清零
                    //   （探针 e1 b.y=0 vs C++ b.y=7）。逐层补拷「祖先构造所
                    //   在层之下」的全部层（中间层+本类层·value=源对象指针）。
                    if (copyOwner != canonTgt) {
                        const ClassInfo* lvl = semantic_->findClass(canonTgt);
                        while (lvl != nullptr && lvl->name != copyOwner) {
                            emitOwnedFieldsCopy(newObj, value, canonTgt,
                                                lvl->name, node->location);
                            lvl = lvl->baseName.empty()
                                      ? nullptr
                                      : semantic_->findClass(lvl->baseName);
                        }
                    }
                } else {
                    // 571（2026-10-14·#571 根治）：无拷贝构造类拷贝初始化——
                    //   原裸 CopyStruct 整对象浅拷=拥有型字段（字符串/可选<字符串>/
                    //   结果<字符串,…>/容器字段）句柄共享：拷贝产物与源双主，任一侧
                    //   字段写（preFree）或析构（级联释放）=对方悬垂（探针4 实锤
                    //   UAF：乙.名称 重写后 甲.名称.值 打印已释放内存静默崩）。修=
                    //   **逐层字段级深拷**（emitOwnedFieldsCopy·123 基类链同款设施：
                    //   串 __cn_str_copy 落堆·合成体/结构体字段 emitStructCopyWithFields
                    //   载荷真值守卫条件深拷〔#280〕·类字段拷贝构造级联·标量字节拷
                    //   ——偏移按最派生类上下文 classFieldOffset 单点），C++ 隐式
                    //   拷贝语义+CN 拥有语义对齐；虚表指针由 NewObject 初始化不覆盖
                    //   （同类同表）。源 value=源对象指针（Load 源槽产物）。
                    for (const ClassInfo* lvl = ci; lvl != nullptr;
                         lvl = lvl->baseName.empty()
                                   ? nullptr
                                   : semantic_->findClass(lvl->baseName)) {
                        emitOwnedFieldsCopy(newObj, value, canonTgt, lvl->name,
                                            node->location);
                    }
                }
                emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique,
                     "ptr", node->location);
                return true;
            }
        }
    }
    return false;
}

// 族⑩ 子方法：058-ⅡB（甲案·拆包绑定位值语义深拷〔基准=019〕·795 轮）：
//   `类 b = r.值`（源=结果/可选 值字段拆包）原落通用路径=句柄浅拷+b 拥有式
//   RAII 登记→ b 与来源双主双释放（p0926_03 映射获取拆包 rc=134·双防线均不
//   达拆包形态）。修=同套上一分支深拷语义：NewObject+拷贝构造（无则 CopyStruct
//   壳）·byRef ABI 传成员链左值地址（&r.值=字段地址）。**范围=拆包本面**
//   （泛化一切 MemberExpr 曾改写 v2 树「结构体.类字段 句柄共享」存量语义→
//   91/94 回归+OOM 14.6GB 已收窄·字段链值语义随 793 归一接力·021 058 行登记）。
//   true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::tryGenUnwrapBindInit(VarDecl* node, const ir::IRValue& value,
                                       const std::string& srcType,
                                       const std::string& unique) {
    const MemberExpr* unwrapSrc = node->initializer->getType() == NodeType::MemberExpr
                                  ? static_cast<const MemberExpr*>(
                                        node->initializer.get())
                                  : nullptr;
    if (semantic_ != nullptr && value.type == "ptr" && unwrapSrc != nullptr &&
        unwrapSrc->memberName == "值") {
        const std::string baseTypeSrc =
            exprSrcType(unwrapSrc->object.get());
        const std::string baseCanon = types::canonical(baseTypeSrc);
        const bool isUnwrapSlot =
            isResultType(baseCanon) || isOptionalType(baseCanon);
        const std::string canonTgt = types::canonical(srcType);
        if (isUnwrapSlot && semantic_->isClassType(canonTgt)) {
            const ClassInfo* ci = semantic_->findClass(canonTgt);
            if (ci != nullptr) {
                const std::string extra =
                    canonTgt + "|" + std::to_string(ci->totalSize);
                ir::IRValue newObj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(canonTgt, "ptr")},
                    "ptr", extra, node->location);
                const ClassMemberInfo* copyCtor =
                    semantic_->findCopyConstructor(canonTgt);
                if (copyCtor != nullptr) {
                    const std::string copyOwner =
                        copyCtor->ownerClass.empty() ? canonTgt
                                                     : copyCtor->ownerClass;
                    ir::IRValue srcAddr =
                        lvalueAddress(node->initializer.get());
                    emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
                         methodSymbolKey(copyOwner, copyCtor->sigKey),
                         "void", node->location);
                } else {
                    emit(ir::Opcode::CopyStruct, {newObj, value},
                         ir::IRValue(), std::to_string(ci->totalSize),
                         "void", node->location);
                }
                emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique,
                     "ptr", node->location);
                return true;
            }
        }
    }
    return false;
}

// 族⑩ 子方法：632（544 呈报·声明位右值=引用返回调用的宿主侧根治）：
//   `货架 甲 = 取货(源)`（取货 返回 货架&）原落兜底 Store——引用返回调用默认
//   lvalue-to-rvalue 解引用产物=**源对象指针**被句柄直存进甲槽：甲与源共享
//   同一堆对象，块出口双方析构同一句柄=双释放（探针 pA：free(): invalid
//   pointer rc=134；成员方法形态 pC：段错误 rc=139；赋值位 pD 同根）。v2 侧
//   该形态 rc=0（544 修复轮呈报的存量分叉）。
//   规范语义（001 §3.2 元素引用「引用返回=零拷贝借用视图·只读消费」+ 类值
//   语义初始化=独立副本（719① 标识符拷贝同款深拷）·C++ copy-init 对齐）=
//   从被引用对象深拷出新对象：有拷贝构造 → NewObject+拷贝构造（byRef 传
//   「被引用左值地址」——与 tryGenClassCopyInit/719① 同 ABI）；无拷贝构造 →
//   NewObject+CopyStruct 浅壳（与 tryGenClassCopyInit 备选分支同语义——宿主
//   既有「标识符拷贝」行为面，无拷构造类的克隆契约诊断面不变）。
//   被引用左值地址 A 经 suppressRefDeref_ 抑制语境生成（与引用绑定
//   tryGenReferenceBinding 同机制）——非抑制产物已解引用（对象指针），对类
//   直接消费=共享句柄缺陷面；抑制语境 Call 产物=返回地址原样（ptr）。
//   对象化守卫：仅用户类（结构体/容器/结果/可选/接口各走既有通道）；容器/
//   映射族排除（声明位接收=容器通道既有路径，容器引用返回面另行立项）；
//   直接调用形态要求被调返回类型剥 &/* 后严格等于声明类（构造/内置 miss
//   自然排除）；方法调用形态（对象.取货()）由语义层 isRefReturnCall 标记
//   担保返回类型含 &（544 同款宽松面——IR 层重解析对象类型查成员表不扩面）。
//   true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::tryGenRefReturnClassCopyInit(VarDecl* node, const DeclGenCtx& ctx) {
    if (semantic_ == nullptr || node->initializer == nullptr) return false;
    if (node->initializer->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node->initializer.get());
    const std::string declCanon = types::canonical(node->typeName);
    if (!isRefReturnClassValueCall(call, declCanon)) return false;
    const ClassInfo* ci = semantic_->findClass(declCanon);
    if (ci == nullptr || ci->totalSize <= 0) return false;
    ir::IRValue newObj = genRefReturnClassDeepCopy(node->initializer.get(),
                                                   declCanon, ci);
    if (newObj.id < 0) return false;  // 防御：异常形态回原路
    emit(ir::Opcode::Store, {newObj}, ir::IRValue(), ctx.unique, "ptr",
         node->location);
    lastExpr_ = newObj;
    return true;
}

// 633：模板返回类型文本 → $ 实例名归一（rewriteGenericInstanceTypeName 同款
//   公式——头名 + 实参逐段 $ + canonicalizeSyntheticArgText；非模板形态原样）。
//   用途=isRefReturnClassValueCall 的 ret 比较（语义层 canonicalType 不转 $）。
static std::string refReturnInstanceName633(const std::string& type) {
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) {
        return type;
    }
    const std::string head = type.substr(0, lt);
    const std::string inner = type.substr(lt + 1, gt - lt - 1);
    std::string inst = head;
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        const std::size_t comma = inner.find(',', pos);
        std::string arg = (comma == std::string::npos)
                              ? inner.substr(pos)
                              : inner.substr(pos, comma - pos);
        const std::size_t b = arg.find_first_not_of(" 	");
        const std::size_t e = arg.find_last_not_of(" 	");
        if (b != std::string::npos && e != std::string::npos) {
            arg = arg.substr(b, e - b + 1);
        }
        inst += "$" + types::canonical(canonicalizeSyntheticArgText(arg));
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return inst;
}

// 632/633 判定（声明位 632 + 赋值位 633 共用·单一事实源）：
//   右值=引用返回调用（语义层 isRefReturnCall）&& 目标=用户类（含容器族——
//   633 放行：容器引用返回接收=深拷副本，须容器拷贝构造命中=容器恒有）
//   && 直接调用形态时被调返回类型剥 &/* 严格等于目标类（方法调用形态由
//   语义层标记担保·544 同款宽松面）。
bool IRGenerator::isRefReturnClassValueCall(const CallExpr* call,
                                            const std::string& declCanon) const {
    if (semantic_ == nullptr || call == nullptr) return false;
    if (!call->isRefReturnCall) return false;
    if (!semantic_->isClassType(declCanon)) return false;
    // 633（#633 残面④b）：容器/映射族放行——引用返回接收=深拷副本
    //   （head 判据与 ir_expr_assign memberClassFieldAssign 同款）；原 632
    //   「容器走既有通道」的暂排前提不成立（既有容器接收通道对引用返回=
    //   句柄共享双释放·探针 p4b C0000374 实录）。容器恒有容器拷贝构造
    //   （向量$T$向量$T(向量$T&)）；miss=防御回原路（不落浅壳防句柄共享）。
    const std::size_t dl = declCanon.find('$');
    const std::string head =
        dl == std::string::npos ? declCanon : declCanon.substr(0, dl);
    const bool isContainerFamily =
        head == "向量" || head == "栈" || head == "链表" ||
        head == "队列" || head == "集合" || head == "映射";
    if (isContainerFamily &&
        semantic_->findCopyConstructor(declCanon) == nullptr) {
        return false;
    }
    // 直接调用形态：被调返回类型（剥 &/*）须严格等于声明类
    if (call->callee != nullptr &&
        call->callee->getType() == NodeType::IdentifierExpr) {
        const std::string& cname =
            static_cast<const IdentifierExpr*>(call->callee.get())->name;
        std::string ret = semantic_->funcReturnTypeOf(cname);
        if (ret.empty() && !call->resolvedSignature.empty()) {
            ret = semantic_->funcReturnTypeOf(call->resolvedSignature);
        }
        if (ret.empty()) return false;
        while (!ret.empty() && (ret.back() == '&' || ret.back() == '*')) {
            ret.pop_back();
        }
        // 633（#633 残面④b 定位）：被调返回文本归一实例名——语义层
        //   funcReturnTypeOf 存 canonicalType 形态（「向量<整32>」·canonical
        //   不转 $），声明侧 declCanon 经 rewriteGenericInstanceTypeName 已是
        //   $ 实例名（「向量$整32」）——原直接 canonical 比较对容器族恒不等
        //   （类短名无 <...> 形态自然命中=632 未暴露）。归一公式=
        //   rewriteGenericInstanceTypeName 同款（语义层 instantiateGeneric
        //   注册名·实参逐段 canonicalizeSyntheticArgText）。
        ret = refReturnInstanceName633(ret);
        if (types::canonical(ret) != declCanon) return false;
    }
    return true;
}

// 632/633 深拷发射（声明位+赋值位共用）：被引用左值地址 A（抑制语境）→
//   NewObject 新壳 + 拷贝构造（byRef 传 A）/ 无拷贝构造 CopyStruct 备选。
//   返回新对象句柄（id<0=异常形态·调用方回原路）。
ir::IRValue IRGenerator::genRefReturnClassDeepCopy(Expr* rhs,
                                                   const std::string& declCanon,
                                                   const ClassInfo* ci) {
    // 被引用左值地址 A（引用返回默认解引用抑制——取地址语义）
    ir::IRValue addr;
    {
        const bool oldSuppress = suppressRefDeref_;
        suppressRefDeref_ = true;
        addr = genExpr(rhs);
        suppressRefDeref_ = oldSuppress;
    }
    if (addr.id < 0 || (addr.type != "ptr" && addr.type != "i64")) {
        return ir::IRValue();  // 防御：异常形态回原路
    }
    const std::string extra = declCanon + "|" + std::to_string(ci->totalSize);
    ir::IRValue newObj = emitResult(
        ir::Opcode::NewObject, {ir::IRValue::constant(declCanon, "ptr")}, "ptr",
        extra, rhs->location);
    const ClassMemberInfo* copyCtor = semantic_->findCopyConstructor(declCanon);
    if (copyCtor != nullptr) {
        const std::string copyOwner =
            copyCtor->ownerClass.empty() ? declCanon : copyCtor->ownerClass;
        emit(ir::Opcode::Call, {newObj, addr}, ir::IRValue(),
             methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
             rhs->location);
    } else {
        ir::IRValue objPtr = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                                        rhs->location);
        emit(ir::Opcode::CopyStruct, {newObj, objPtr}, ir::IRValue(),
             std::to_string(ci->totalSize), "void", rhs->location);
    }
    return newObj;
}

// 族⑩ 子方法：plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串初始化
//   拥有化——字面量（只读段标签）与标识符拷贝（浅共享指针）经 __cn_str_copy
//   落堆（变量一律拥有堆串，RAII 返回块释放安全；Rust "x".to_string() 同款
//   代价）；调用返回形态（拼接/复制等 runtime 串）本就堆分配直存；
//   转移初始化已在浅交接分支（句柄直拷+源清零）先行返回，不经此处。
void IRGenerator::ownStringInitValue(VarDecl* node, const std::string& srcType,
                                     ir::IRValue& value) {
    if (srcType == "字符串" && value.type == "ptr" &&
        node->initializer != nullptr) {
        const NodeType ownIt = node->initializer->getType();
        if (ownIt == NodeType::StringLiteral ||
            ownIt == NodeType::IdentifierExpr) {
            value = emitResult(ir::Opcode::Call, {value}, "ptr",
                               "__cn_str_copy", node->location);
        } else if (ownIt == NodeType::CallExpr) {
            // 调用返回拥有判定=白名单 ∪ 返回类型契约（A2 2026-09-11 方案甲）：
            //   白名单（内置 runtime 分配族：复制/连接/拼接/子串/大小写/修剪/
            //   反转）之外，被调者返回类型=字符串 即拥有（语义层决议写回
            //   retOwnedString——普通函数/类方法/接口方法/内置全路径统一；
            //   驻留文本 已改 字符* 返回=借用不登记）。Rust 签名即契约：
            //   fn f() -> String 拥有 / -> &str 借用。
            const CallExpr* ice =
                static_cast<const CallExpr*>(node->initializer.get());
            bool ownRet = ice->retOwnedString;
            if (!ownRet && ice->callee->getType() == NodeType::IdentifierExpr) {
                const std::string& cn =
                    static_cast<const IdentifierExpr*>(ice->callee.get())
                        ->name;
                // 094 判据单点化：白名单收口 ISemanticView::isOwnedStringBuiltin
                ownRet = isOwnedStringBuiltin(cn);
            }
            if (!ownRet) markStringTainted(node->name);
        }
    }
}


} // namespace cn_compiler