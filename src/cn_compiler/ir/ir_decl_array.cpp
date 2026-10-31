// IR·数组初始化列表写链（#1769 轮拆分 自 ir_stmt_decl.cpp 族⑧ + 多维改造：
//   一维行为逐字保持，多维递归展开新增）。
//
// #1769（2026-10-31 用户批甲案·二维嵌套初始化列表 {{...}}）：
//   ①嵌套大括号初始化列表——数组初始化列表的元素本身可为初始化列表（嵌套
//     深度=数组维度），IR 侧逐元素写链按元素类型递归展开（行主序·1473 甲案
//     口径：左起第一下标=行=外层）。
//   ②缺格补「类型默认态」（1473 已批甲案·同构推广到每层内层）：数值/浮/布/
//     字符=0、字符串=空串态、指针=空指针、结果<..>=异常态（零位 tag0·001
//     §3.5 条文写明）、可选=无值、结构体=逐字段零位（字段无默认值语法·
//     §5.7 裸声明）、**类=默认构造**（既有入口默认构造通道=775 字段默认值
//     注入+无参构造调用，内联槽展开·与 975 数组元素构造链同构）。
//   ③维度不齐=编译期拒（语义层 semantic_decl_init.cpp 裁定·IR 只接合法形态；
//     IR 层对不合法形态到达=内部缺陷显形，禁静默）。
//   立法条文=plans/001 §3.5（随本任务分支落盘〔通告〕）。
#include <cstdint>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_stmt_decl_ctx.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ---- 族⑧：数组初始化列表 { 1, 2, 3 } / { { 1, 2 }, { 3, 4 } } ----
//   true = 已处理（genVarDecl 返回）。
bool IRGenerator::genArrayInitListDecl(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::InitListExpr &&
        types::isArray(ctx.srcType)) {
        InitListExpr* initList =
            static_cast<InitListExpr*>(node->initializer.get());
        // #1769：多维递归展开（一维=单层·发射序列与原实现逐指令一致）。
        //   每层缺格尾部由递归尾统一补默认态（含类缺格默认构造）。
        emitArrayInitElements(node, initList, ctx.srcType, ctx.unique, 0,
                              node->location);
        return true;
    }
    return false;
}

// 当前层元素步进（字节）：typeSizeOf 无值=内部缺陷显形（语义层已保证声明类型
//   合法，此 miss=降级/登记遗漏·648 红线本面禁静默——本路径不继承
//   value_or(8) 静默语义）。有值 0=零长维（合法·循环不消费）。
std::int64_t IRGenerator::arrayInitStrideOf(const std::string& elemType,
                                            const SourceLocation& loc) {
    if (semantic_ != nullptr) {
        const cn::Optional<int> size = semantic_->typeSizeOf(elemType);
        if (!size.has_value()) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "内部错误: 数组元素类型 '" + elemType +
                                    "' 尺寸未解析（初始化列表展开中止）");
            return 0;
        }
        return size.value();
    }
    return 8;
}

// 元素地址发射：数组基址 + 绝对字节偏移（与原实现同构——每元素独立
//   AddrOf+ConstInt+Add，一维发射序列逐指令一致）。
ir::IRValue IRGenerator::emitArrayElemAddr(std::int64_t byteOffset,
                                           const std::string& varIrType,
                                           const std::string& unique,
                                           const SourceLocation& loc) {
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(unique, varIrType)},
                                  "ptr", unique, loc);
    ir::IRValue offset = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                    std::to_string(byteOffset), loc);
    return emitResult(ir::Opcode::Add, {base, offset}, "ptr", "", loc);
}

// 单元素写入（原 storeArrayInitElements 循环体逐字搬移·分派序不变）：
//   结构体元素逐字段 / 类元素构造到内联槽（975）/ 结果可选合成体整块拷贝
//   （1415）/ 无元素零填 / 普通元素 Cast+StorePtr。
void IRGenerator::emitArrayElementWrite(
    Expr* elem, const std::string& elemType, const std::string& elemIrType,
    std::int64_t elemStride, const ir::IRValue& addr,
    const SourceLocation& loc) {
    // 结构体元素（学生{...}）：逐字段写入（Task 2.7 修复——此前只写占位0）
    if (elem->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
        emitStructInitTo(static_cast<StructInitExpr*>(elem), addr, loc);
        return;
    }
    // 975（130 根治）：类元素×构造调用 → **构造到内联槽地址**
    if (semantic_ != nullptr &&
        semantic_->isClassType(types::canonical(elemType)) &&
        elem->getType() == NodeType::CallExpr) {
        CallExpr* elemCall = static_cast<CallExpr*>(elem);
        if (elemCall->callee->getType() == NodeType::IdentifierExpr) {
            std::string ctorName =
                static_cast<IdentifierExpr*>(elemCall->callee.get())->name;
            ctorName = resolveGenericCtorInstanceName(ctorName);
            const ClassInfo* ciC =
                semantic_->findClass(ctorName).value_or(nullptr);
            if (ciC != nullptr && !ciC->isAbstract) {
                const cn::Optional<const ClassMemberInfo*> ctorOptC =
                    findCtorMember(ciC, ctorName, elemCall);
                // else 新规机械合规：互斥二态双 if 显式枚举
                if (ctorOptC.has_value()) {
                    emitCtorInvoke(elemCall, ctorName, ctorOptC.value(), addr);
                }
                if (!ctorOptC.has_value()) {
                    // #1769：无显式构造=默认构造语义（类名() 展开·与
                    //   genClassDefaultConstruct 类变量通道同构）——本类层
                    //   775 字段默认值注入+祖先链（原漏本类层注入=无构造类
                    //   元素槽栈垃圾·r 探针实证·1473 甲案「类对象=既有入口
                    //   默认构造通道」同口径）
                    emitClassFieldDefaults(ctorName, ctorName, addr, loc);
                    emitBaseCtorChain(ctorName, addr, loc);
                }
                return;
            }
        }
    }
    // 1415（#1415 P1-2·零分叉）：结果/可选 **合成体元素**=内联体（tag+载荷
    //   联合体）——genExpr 返回物化槽地址（ptr）；整块拷贝（元素步进字节）
    //   与结构体元素同语义。
    if (isResultOrOptionalText(elemType)) {
        // 1415（复审 P0·零分叉）：**无元素**（NullLiteral）=空可选值
        //   （tag=0/载荷=0）——零填充元素槽（1602/1651 统一走 emitZeroFillAt）。
        if (elem->getType() == NodeType::NullLiteral) {
            emitZeroFillAt(addr, elemStride, loc);
            return;
        }
        ir::IRValue elemValue = genExpr(elem);
        emit(ir::Opcode::CopyStruct, {addr, elemValue}, ir::IRValue(),
             std::to_string(elemStride), "void", loc);
        return;
    }
    // 普通元素：生成值 + Cast + StorePtr
    ir::IRValue elemVal = genExpr(elem);
    if (elemVal.type != elemIrType) {
        elemVal = emitResult(ir::Opcode::Cast, {elemVal}, elemIrType, "", loc);
    }
    emit(ir::Opcode::StorePtr, {addr, elemVal}, ir::IRValue(), "", elemIrType,
         loc);
}

// 缺格补「类型默认态」（1473 甲案·#1769 每层递归）：startOffset=缺格首元素
//   绝对字节偏移；count=缺格元素个数；elemType=缺格元素类型（可为数组=整行
//   缺格递归 / 类=默认构造 / 其他=零位准确宽度分段）。
void IRGenerator::emitArrayTailDefaults(VarDecl* node,
                                        const std::string& elemType,
                                        std::int64_t startOffset,
                                        long long count,
                                        const std::string& unique,
                                        const SourceLocation& loc) {
    if (count == 0) return;  // 无缺格（数量域=层长-已写>=0 恒非负）
    const std::string canon =
        semantic_ != nullptr ? types::canonical(elemType) : elemType;
    const std::string elemIrType = mapType(elemType);
    const std::int64_t stride = arrayInitStrideOf(elemType, loc);
    // 类缺格=默认构造（775 字段默认值注入+无参构造调用·内联槽展开——
    //   语义=类名() 与 975 元素构造链同构·1473 甲案「类对象=既有入口默认
    //   构造通道」。一维/多维同口径零分叉）
    if (semantic_ != nullptr && semantic_->isClassType(canon)) {
        const ClassInfo* ci = semantic_->findClass(canon).value_or(nullptr);
        if (ci == nullptr || ci->isAbstract) {
            // 语义层已拒（抽象类元素数组=930 负测面）——到达=内部缺陷显形
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "内部错误: 数组缺格元素类 '" + canon +
                                    "' 未解析或为抽象类");
            return;
        }
        for (long long k = 0; k < count; ++k) {
            ir::IRValue addr = emitArrayElemAddr(
                startOffset + k * stride, elemIrType, unique, loc);
            emitClassFieldDefaults(canon, canon, addr, loc);
            bool invoked = false;
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.hasBody &&
                    mk.second.ownerClass == canon &&
                    mk.second.paramTypes.empty()) {
                    std::vector<ir::IRValue> args;
                    args.push_back(addr);  // this（内联槽地址）
                    emit(ir::Opcode::Call, args, ir::IRValue(),
                         methodSymbolKey(canon, mk.second.sigKey), "void",
                         loc);
                    invoked = true;
                    break;
                }
            }
            if (!invoked) {
                // 无自有零参构造=emitBaseCtorChain（975 无显式构造同款·含
                //   祖先链字段默认值注入）
                emitBaseCtorChain(canon, addr, loc);
            }
        }
        return;
    }
    // 数组元素（行缺格）：递归内层整行补默认态（每行=内层全缺格）
    if (types::isArray(elemType)) {
        const std::string inner = types::arrayElemOf(elemType);
        const cn::Optional<int> innerLen = types::arrayLenOf(elemType);
        if (!innerLen.has_value()) {
            // isArray 守卫下首维恒纯数字（多维声明收口保证）——无值=内部缺陷
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "内部错误: 数组行类型 '" + elemType +
                                    "' 首维长度缺失（缺格补齐中止）");
            return;
        }
        for (long long k = 0; k < count; ++k) {
            emitArrayTailDefaults(node, inner, startOffset + k * stride,
                                  innerLen.value(), unique, loc);
        }
        return;
    }
    // 零位：准确宽度分段（8/4/2/1——修原「stride>8 逐 8B」对非 8 倍数步进
    //   过写相邻元素面；零位模式与元素类型位模式无关，宽度对齐 stride）
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    for (long long k = 0; k < count; ++k) {
        ir::IRValue addr = emitArrayElemAddr(startOffset + k * stride,
                                             elemIrType, unique, loc);
        std::int64_t off = 0;
        while (off < stride) {
            const std::int64_t remain = stride - off;
            const std::int64_t w = remain >= 8 ? 8 : remain;
            const std::string wIr =
                w >= 8 ? "i64" : (w == 4 ? "i32" : (w == 2 ? "i16" : "i8"));
            ir::IRValue a = addr;
            if (off != 0) {
                ir::IRValue subOff = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                                std::to_string(off), loc);
                a = emitResult(ir::Opcode::Add, {addr, subOff}, "ptr", "", loc);
            }
            emit(ir::Opcode::StorePtr, {a, zero}, ir::IRValue(), "", wIr, loc);
            off += w;
        }
    }
}

// 多维递归展开：arrayType=当前层**数组类型**（顶层 整32[2][2] / 内层 整32[2]）；
//   baseOffset=当前层首元素相对数组变量基址的绝对字节偏移。
//   行主序（1473 甲案）：元素序=写序=行优先逐元素。每层裁定：
//   ①元素类型是数组：元素为嵌套列表→递归下一维；否则=拉平（语义层已拒，
//     到达=内部缺陷显形禁静默）。②元素类型非数组：嵌套列表=维度过深（同上
//     显形）；否则单元素写入。③层尾缺格→emitArrayTailDefaults 递归补默认态。
void IRGenerator::emitArrayInitElements(VarDecl* node, InitListExpr* list,
                                        const std::string& arrayType,
                                        const std::string& unique,
                                        std::int64_t baseOffset,
                                        const SourceLocation& loc) {
    const std::string elemType = types::arrayElemOf(arrayType);
    const cn::Optional<int> lenOpt = types::arrayLenOf(arrayType);
    if (!lenOpt.has_value()) {
        // isArray 守卫下首维恒纯数字（语义层多维声明收口保证）——无值=内部缺陷
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "内部错误: 数组类型 '" + arrayType +
                                "' 首维长度缺失（初始化列表展开中止）");
        return;
    }
    const int len = lenOpt.value();
    const std::string elemIrType = mapType(elemType);
    const std::int64_t stride = arrayInitStrideOf(elemType, loc);
    const bool elemIsArray = types::isArray(elemType);
    for (std::size_t i = 0; i < list->elements.size(); ++i) {
        Expr* elem = list->elements[i].get();
        const std::int64_t elemOffset =
            baseOffset + static_cast<std::int64_t>(i) * stride;
        const bool elemIsList = elem->getType() == NodeType::InitListExpr;
        if (elemIsArray && !elemIsList) {
            // 拉平禁入（语义层已拒·到达=内部缺陷显形）
            diagnostics_.report(
                DiagnosticLevel::Error, elem->location,
                "内部错误: 数组初始化拉平形态未被语义层拦截（'" + elemType +
                    "' 仍为数组）");
            continue;
        }
        if (!elemIsArray && elemIsList) {
            // 嵌套塞一维（语义层已拒·到达=内部缺陷显形）
            diagnostics_.report(
                DiagnosticLevel::Error, elem->location,
                "内部错误: 一维数组元素位嵌套列表未被语义层拦截（'" + elemType +
                    "' 非数组）");
            continue;
        }
        if (elemIsArray && elemIsList) {
            // 嵌套合法：递归下一维（行基址偏移=elemOffset）
            emitArrayInitElements(node, static_cast<InitListExpr*>(elem),
                                  elemType, unique, elemOffset, loc);
            continue;
        }
        ir::IRValue addr =
            emitArrayElemAddr(elemOffset, elemIrType, unique, loc);
        emitArrayElementWrite(elem, elemType, elemIrType, stride, addr, loc);
    }
    // 层尾缺格补默认态（部分初始化·1473 甲案）
    const long long written = static_cast<long long>(list->elements.size());
    if (written < len) {
        emitArrayTailDefaults(node, elemType, baseOffset + written * stride,
                              len - written, unique, loc);
    }
}

} // namespace cn_compiler
