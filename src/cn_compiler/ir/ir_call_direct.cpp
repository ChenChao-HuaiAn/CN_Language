// CN-IR生成器实现：直接调用族（954 拆族：自 ir_call.cpp visitCallExpr 巨函数纯机械
//   搬移·逐字保留零语义变化;函数≤100 行纪律）
//   族 = 直调性/符号名决议（resolveCalleeDirectness·040/FFI 纯名/重载签名）、
//   默认实参（prepareDefaultArgValues）、args 收尾（finalizeCallArgs）、
//   直调发射（emitDirectCall 编排 + 用户函数返回类型 resolveUserFuncCallResultType
//   + 聚合返回 emitAggStructReturnCall + 空类型直发 emitVoidBuiltinCall +
//   引用返回右值消费 needsRefReturnDeref/emitRefReturnDeref〔986：先 Call
//   后解引用·对齐 v2〕）。成员函数声明仍在 ir_generator.hpp。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 954（visitCallExpr 拆族·机械搬移）：直调性判定（040 静态槽 fnptr 豁免·原注释随行）
//   + 重载/FFI 符号名决议整体迁出;返回 isDirect（调用点接收）。
bool IRGenerator::resolveCalleeDirectness(CallExpr* node,
                                          std::string& calleeName) {
    bool isDirect = false;
    if (lookupVar(calleeName).id < 0) {
        isDirect = true;  // 不在变量表 -> 函数名
        // 040（001 §5.8 甲案·2026-10-05）：静态槽函数指针调用——lookupVar 只查
        //   函数局部表，静态槽 miss 曾误判直调（LNK2019 静态回调 实证）；静态
        //   类型为 fnptr 时保持间接（genExpr 标识符=静态槽 LoadPtr→CallIndirect
        //   与局部 fnptr 同链）
        if (semantic_ != nullptr && semantic_->isGlobalStatic(calleeName) &&
            semantic_->globalStaticType(calleeName).rfind("函数指针<", 0) == 0) {
            isDirect = false;
        }
    }
    // Task 2.10 重载：语义层决议结果（签名 key 名#参数串）优先作为符号名——
    //   定义/调用三处一致（codegen 按此生成 mangled 符号）。
    //   内置函数（打印/字符串API）无重载，resolvedSignature 为空，保持原名映射。
    if (isDirect && !node->resolvedSignature.empty()) {
        // C-3（FFI）：外部 函数 调用——链接符号 = 纯名（C 符号无重载
        //   mangling、无模块前缀；codegen 自动 EXTERN，链接期由库解析）
        if (semantic_ != nullptr &&
            semantic_->isExternFunc(node->resolvedSignature)) {
            calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        } else {
            calleeName = node->resolvedSignature;
        }
    }
    return isDirect;
}

// ---- 默认实参补全（Task 2.10）----
// 决议已按"实参个数+可补全"匹配。缺省参数按 函数定义侧收集的默认值常量
// （funcDefaultArgs_[签名key]，visitFunctionDecl 时由 defaultExpr 求值）精确展开。
// 收集规则：defaults 列表按函数参数顺序仅含带默认值的尾部参数；
//   调用方缺 N 个实参时取 defaults 最后 N 个（从右向左连续声明）。
// 954（visitCallExpr 拆族·机械搬移）：整体迁出（写入 defaultArgValues_ 成员）。
void IRGenerator::prepareDefaultArgValues(const CallExpr* node, bool isDirect) {
    if (isDirect && !node->resolvedSignature.empty()) {
        auto defIt = funcDefaultArgs_.find(node->resolvedSignature);
        if (defIt != funcDefaultArgs_.end()) {
            const auto& defaults = defIt->second;
            const std::size_t given = node->arguments.size();
            // 参数总数：优先 funcDefaultTotal_（D23：两侧收集点同步登记，构造
            //   不入 functions_ 回退口径 given+defaults 会把显式传满参的调用误补）；
            //   次选语义层注册的 paramTypes 长度（普通函数）。
            std::size_t totalParams = 0;
            auto totIt = funcDefaultTotal_.find(node->resolvedSignature);
            if (totIt != funcDefaultTotal_.end()) {
                totalParams = totIt->second;
            } else if (semantic_ != nullptr) {
                totalParams = semantic_->funcParamTypesOf(node->resolvedSignature).size();
            }
            if (totalParams == 0) totalParams = given;
            // 缺省个数 = 参数总数 - 实参个数（0 ~ defaults.size()）
            if (given < totalParams) {
                const std::size_t missing = totalParams - given;
                defaultArgValues_.clear();
                // 取 defaults 中 末尾 missing 个（尾部参数默认值，从右向左连续声明）
                for (std::size_t k = defaults.size() - missing; k < defaults.size(); ++k) {
                    defaultArgValues_.push_back(defaults[k]);
                }
            }
        }
    }
}

// 954（visitCallExpr 拆族·机械搬移）：args 收尾（106 空可选装箱 + 默认实参追加）。
void IRGenerator::finalizeCallArgs(CallExpr* node, const std::string& calleeName,
                                   bool isDirect, std::vector<ir::IRValue>& args) {
    // 106：实参位「无 → 可选<T>」空可选装箱（canConvert 豁免面 IR 配套——
    //   内置名映射后 funcParamTypesOf 查无=no-op；「无→可选」仅用户函数形参面）
    if (isDirect && semantic_ != nullptr) {
        boxNullOptionalArgs(node, semantic_->funcParamTypesOf(calleeName), args);
    }
    // 追加默认实参（Task 2.10）：缺省参数在实参之后按参数顺序补全
    for (const auto& defVal : defaultArgValues_) {
        args.push_back(defVal);
    }
    defaultArgValues_.clear();
}

// 直接调用：extra=函数名；按函数返回类型设置结果类型（Task 2.5 字符串API；
// Task 2.7 集成修复：用户自定义函数经语义层查真实返回类型，避免浮64
// 结果被误标 i32 导致 codegen 用 eax 读 xmm0 返回值）
// 954（visitCallExpr 拆族·机械搬移）：直调发射编排（结果类型两级 -> 三形态 -> 主发射 + 617 结算）。
// 986（954 呈报根治·对齐 v2）：引用返回右值消费路径改为「先发被调 Call 再
//   解引用」——判定提前为纯谓词（needsRefReturnDeref），Call 发射与 617
//   结算共享主发射单点，解引用输入=显式 Call 结果寄存器。原实现在主发射前
//   return（Call 静默不发射=副作用丢失）且 LoadPtr 消费 lastExpr_ 残留。
void IRGenerator::emitDirectCall(CallExpr* node, const std::string& calleeName,
                                 const std::vector<ir::IRValue>& args,
                                 const std::vector<std::pair<const Expr*, ir::IRValue>>&
                                     consumedArgTemps617) {
    std::string resultType = "i32";
    if (!resolveBuiltinCallResultType(calleeName, resultType) &&
        semantic_ != nullptr) {
        resultType = resolveUserFuncCallResultType(node, calleeName);
    }
    if (emitAggStructReturnCall(node, calleeName, args, resultType)) return;
    if (emitVoidBuiltinCall(node, calleeName, args)) return;
    // 986：引用返回右值消费=纯谓词（Call 前判定·与原实现判定时机一致）
    const bool refDeref986 = needsRefReturnDeref(calleeName, resultType);
    // 986：被调 Call 恒发射（对齐 v2「先发 IR_调用参数化再引用返回加载」）
    const ir::IRValue callRet986 =
        emitResult(ir::Opcode::Call, args, resultType, calleeName,
                   node->location);
    lastExpr_ = callRet986;
    // 617：消费族调用发射完成——内容已拷贝/只读消费毕，结算拥有串临时实参
    //   （662：三元实参走分支拥有性感知条件释放·单一入口 Rec617）
    for (const auto& tmp617 : consumedArgTemps617) {
        emitOwnedTempReleaseRec617(tmp617.first, tmp617.second,
                                   node->location);
    }
    // 986：右值消费=对 Call 结果（=被引用左值地址）解引用读值
    if (refDeref986) {
        emitRefReturnDeref(node, calleeName, callRet986);
    }
}

    // 用户函数：查询语义层返回类型（未映射（空/未知）回退 i32）
    // P3-18 补完：引用返回函数调用结果 = 被引用左值地址（ptr）
// 954（visitCallExpr 拆族·机械搬移）：用户函数返回类型查询（110/648 两级回退）;
//   进入前提=内置未命中且 semantic_ 非空（调用点保证）;「引用返回 -> ptr」提前 return。
std::string IRGenerator::resolveUserFuncCallResultType(CallExpr* node,
                                                       const std::string& calleeName) {
    if (semantic_->funcReturnsRef(calleeName)) return "ptr";
    std::string resultType = "i32";
    std::string ret = semantic_->funcReturnTypeOf(calleeName);
    // 110（外部 函数 整64 返回值塌缩根治）：外部 函数 调用符号=纯名
    //   （C 符号无 mangling·上方 FFI 分支改写 calleeName），而语义层
    //   functions_ 表键=签名 key（名#参数串）——纯名 miss 回退 i32，
    //   三后端 emitCall 恒按 32 位读返回值（eax/w0+符号扩展），整64
    //   大值（>2^32）低 32 截断（llabs(4294967297)→1 实测：arm64 与
    //   linux x64 双后端运行级同塌·win x64 同构按 result.type 分派
    //   同病；v2 侧双后端 str x0/mov rax 全读本即正确=宿主单侧分叉）。
    //   miss 时回退查签名 key（funcReturnTypeOf 内置 模块$签名 剥前
    //   缀·#435 fnptr 间接调用返回类型同族根治先例：IR 层一处修，
    //   三后端 emitCall 按 result.type 分派同受益）。
    if (ret.empty() && !node->resolvedSignature.empty()) {
        ret = semantic_->funcReturnTypeOf(node->resolvedSignature);
    }
    if (ret.empty() && calleeName.rfind("__cn_", 0) != 0) {
        // 648 miss 不可静默（2026-10-14 立法·110 同族终态）：纯名+签名
        //   key 两级查询均 miss=终态——原静默保持初始 "i32"（按 32 位
        //   读返回值=三平台错值源）。报诊断后保留 i32 兜底值与控制流
        //   （driver generate() 后 hasErrors() 中止）。
        //   豁免 "__cn_" 前缀：内置/运行时 API 已由上方内置分派覆盖
        //   （理论上走不到此分支，保险豁免）。
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 函数返回类型未登记 '" + calleeName +
                "' —— 静默 i32 兜底已废除（#648·110 同族终态）");
    }
    if (!ret.empty()) {
        const std::string mapped = mapType(ret);
        if (mapped != "void" && mapped != "") resultType = mapped;
    }
    return resultType;
}

// 954（visitCallExpr 拆族·机械搬移）：聚合返回 retbuf 预插（Task 完善A/087/936）。
bool IRGenerator::emitAggStructReturnCall(CallExpr* node,
                                          const std::string& calleeName,
                                          const std::vector<ir::IRValue>& args,
                                          std::string& resultType) {
    // 结构体返回值函数（Task 完善A）：调用方分配返回缓冲区（结构体临时变量），
    //   以隐藏指针（rcx，Win x64 ABI）传给被调方；被调方写入缓冲区。
    //   结果 = 缓冲区地址（ptr），供调用方 CopyStruct 到目标。
    //   087（m85 返回面）：定长数组返回（整32[3]）同通道——被调 ir_decl 已置
    //   structReturn（隐藏指针+epilogue 按 structReturnSize 拷贝）·调用点
    //   原 isStructType 判定不含数组=retbuf 不预插→被调读空 retbuf 段错误
    //   （m85c 实录 RAX=0）。resultType 显式升 ptr（mapType 数组塌缩 i32·
    //   不动 mapType 本体——数组名退化=元素指针模型全树依赖元素类型）。
    const std::string retSrc087 = semantic_ != nullptr
                                      ? semantic_->funcReturnTypeOf(calleeName)
                                      : std::string();
    const bool isArrayRet087 = semantic_ != nullptr && types::isArray(retSrc087);
    if (isArrayRet087) resultType = "ptr";
    if (resultType == "ptr" && semantic_ != nullptr &&
        (semantic_->isStructType(types::canonical(
             semantic_->funcReturnTypeOf(calleeName))) ||
         isArrayRet087)) {
        // 缓冲构造=allocAggRetbufBuf（936 提炼·公共单点）：槽数经
        //   registerVarSlots 按源码类型实际大小登记（原「64 字节 8 槽」
        //   固定假设=被调 epilogue 精确大小拷贝（structReturnSize）时代
        //   的安全面欠账，936 归纳时统一为多槽登记）。
        ir::IRValue buf = allocAggRetbufBuf(
            semantic_->funcReturnTypeOf(calleeName), node->location);
        // 隐藏返回指针作为第一个参数（rcx）
        std::vector<ir::IRValue> hiddenArgs = args;
        hiddenArgs.insert(hiddenArgs.begin(), buf);
        emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(), calleeName, "void",
             node->location);
        lastExpr_ = buf;
        return true;
    }
    return false;
}

// 954（visitCallExpr 拆族·机械搬移）：空类型返回内置直发（str_free/arena_reset/alloc_reset）。
bool IRGenerator::emitVoidBuiltinCall(CallExpr* node, const std::string& calleeName,
                                      const std::vector<ir::IRValue>& args) {
    // 字符串释放/竞技场重置/释放全部（空类型返回）：用 emit 直接发射，不分配结果寄存器
    // （与打印行 void 展开一致；codegen 对 result.id<0 不生成返回值存储）
    if (calleeName == "__cn_str_free" || calleeName == "__cn_arena_reset" ||
        calleeName == "__cn_alloc_reset") {
        emit(ir::Opcode::Call, args, ir::IRValue(), calleeName, "void",
             node->location);
        // 616（2026-10-14 伞批）：字符串释放(变量槽) 释放后清零——「释放+清零
        //   幂等模型」（74-a 纪律·emitStringFreeFor 同款）：块出口/返回块
        //   兜底再经过该槽时 __cn_str_free(nullptr) 空安全跳过，杜绝「显式
        //   释放 + RAII 兜底」裸 double free（原靠 runtime cn_free_tracked
        //   注册表兜底侥幸安全）。消耗形参体内显式释放后出口释放同理受益。
        if (calleeName == "__cn_str_free" && !node->arguments.empty() &&
            node->arguments[0]->getType() == NodeType::IdentifierExpr) {
            const std::string freedUnique = lookupVarName(
                static_cast<IdentifierExpr*>(node->arguments[0].get())->name);
            if (!freedUnique.empty()) {
                ir::IRValue zero616 = emitResult(
                    ir::Opcode::ConstInt, {}, "i64", "0", node->location);
                emit(ir::Opcode::Store, {zero616}, ir::IRValue(),
                     freedUnique, "ptr", node->location);
            }
        }
        lastExpr_ = ir::IRValue();
        return true;
    }
    return false;
}

// 986（954 呈报根治）：引用返回右值消费谓词——resultType=ptr（resolveUser
//   FuncCallResultType 对 funcReturnsRef 的约定）且被调为引用返回用户函数
//   且非抑制语境。抑制=赋值目标/复合赋值/引用绑定/声明位深拷=取地址语义
//   （Call 结果原样即被引用左值地址，由消费方按引用处理）。纯判定零发射。
bool IRGenerator::needsRefReturnDeref(const std::string& calleeName,
                                      const std::string& resultType) const {
    return resultType == "ptr" && semantic_ != nullptr &&
           semantic_->funcReturnsRef(calleeName) && !suppressRefDeref_;
}

// 986（954 呈报根治·对齐 v2）：引用返回读值后处理——lvalue-to-rvalue 语义
//   （2026-09-04 缺陷零容忍收口）不变：引用返回调用作为右值须解引用读值
//   （原返回裸地址被右值消费=静默错误代码）。Call 已由调用方发射（副作用
//   保真），此处只把调用结果（=被引用左值地址）解引用。
//   根因（954 探针三形态实锤）：原实现（原 ir_call.cpp L845-859·2026-09-04
//   引入）把 LoadPtr 放在主发射之前并提前 return——①主发射不可达=被调 Call
//   静默不发射（函数副作用全丢；v2 侧「先发 IR_调用参数化再引用返回加载」
//   正确=宿主单侧分叉）；②LoadPtr 地址操作数消费 lastExpr_（=最后实参求值
//   残留）——恒等引用返回（返回 源;）巧合正确（102/1012 用例未覆盖），非恒等
//   （返回 甲;）读出最后实参残留值，无实参读出更早残留/垃圾。
//   修复=解引用输入显式取 Call 结果寄存器（不再依赖 lastExpr_ 状态巧合）。
void IRGenerator::emitRefReturnDeref(CallExpr* node, const std::string& calleeName,
                                     const ir::IRValue& callResult) {
    std::string baseRet = semantic_->funcReturnTypeOf(calleeName);
    if (!baseRet.empty() && baseRet.back() == '&') {
        baseRet.pop_back();
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {callResult},
                           mapType(baseRet), "", node->location);
}


} // namespace cn_compiler
