// IRGenerator 成员串字段赋值「接收者 place 拥有」谓词族（自 ir_fields.cpp 拆分·
//   #1933 行数整改：承载文件 1052 行超 1000 冻结线先拆再加——谓词块整体迁出
//   逐字零语义变化；声明仍在 ir_generator.hpp·消费点=ir_expr_assign_member.cpp
//   memberStringFieldAssign / ir_expr_assign_ident_class.cpp handleClassMemberAssign）
//   结构=Depth 主函数（护栏+形态分派）+标识符根判定+成员链递归三函数；递归
//   深度经 depth 参数逐层 +1 传播·护栏 [0,4] 双边界在主函数单点。
#include <algorithm>
#include <string>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {


// #1933：成员串字段赋值「接收者 place 拥有」判定（单点·类面
//   handleClassMemberAssign 与通用面 memberStringFieldAssign 两消费点同源）——
//   真=接收者对象（obj）的字符串字段槽归本函数，写入位 drop 旧值安全。
//   判据=链根拥有 + 链上中段全为字段访问（值/句柄语义传递·串归根）：
//   · 根=标识符：①含串字段聚合局部（ownedFieldOrder_·结构体/结果/可选）——
//     其中结果/可选盒须同时在盒亡登记表（boxVarInRegistry·#1884 甲案：借用盒
//     载荷=源槽句柄共享，drop 其字段串=源槽析构双删）；②有析构类局部
//     （ownedClassOrder_·类字段串归堆对象·堆对象归本函数）。形参/静态/未登记
//     =假（保守不释放·宁漏勿错）。
//   · 中段=MemberExpr：宿主类型可解析且非联合体（164-a 联合体成员共享偏移
//     无 tag·活跃成员身份不可判）→ 递归根；指针成员跳（isDerefAccess·所指
//     无拥有语义）→ 假。
//   · 其余形态（下标=借出视图 72-a/解引用/调用/三元=右值 place）=假（保守）。
//   纯谓词（真假判定·#1360 免结果化）；深度护栏 [0,4] 双边界（越界=形态异常
//   按假保守·1880 同款）。
bool IRGenerator::memberStringRecvPlaceOwned(const Expr* obj) const {
    return memberStringRecvPlaceOwnedDepth(obj, 0);
}

bool IRGenerator::memberStringRecvPlaceOwnedDepth(const Expr* obj, int depth) const {
    // 深度护栏（1979 arrayElemsOwnResource 同款双边界 [0,4]：>4=链上 5 层字段跳
    //   仍接末端串赋值的现实不可达形态；返假=保守不释放（宁漏勿错·漏释放残留
    //   可观测、误释放=双删崩溃），非静默取默认值继续编译。登记条 miss_registry
    // 静默豁免闭集穷举: 双边界护栏返假=形态异常按保守假（1979/1880 同界同形态）
    if (obj == nullptr || depth < 0 || depth > 4) return false;
    if (obj->getType() == NodeType::IdentifierExpr) {
        return memberStringRecvOwnedIdentRoot(obj);
    }
    if (obj->getType() == NodeType::MemberExpr) {
        return memberStringRecvOwnedMemberChain(obj, depth);
    }
    // 下标/解引用/调用/三元=右值或借出 place（保守不释放）
    return false;
}

// 根=标识符分支（2089 嵌套≤3 拆分件·原 Depth 内嵌 4 层抽此）——判据逐字保留：
bool IRGenerator::memberStringRecvOwnedIdentRoot(const Expr* obj) const {
    const std::string& name =
        static_cast<const IdentifierExpr*>(obj)->name;
    const cn::Optional<std::string> slot = lookupVarName(name);
    if (!slot.has_value()) return false;  // 静态/未登记=保守假（87-a 静态面走顶层通道不在本面）
    const std::string& unique = slot.value();
    if (isOwnedFieldSlot(unique)) {
        // 结果/可选盒须盒拥有载荷（借用盒宁漏勿错——#1884 甲案判据复用）；
        // 类型查询 miss=非盒（保守分支不触）·1360 显形分流非缺省
        const cn::Optional<std::string> srcOpt = lookupSrcType(name);
        const bool borrowBox = srcOpt.has_value() &&
            (isResultType(types::canonical(srcOpt.value())) ||
             isOptionalType(types::canonical(srcOpt.value()))) &&
            !boxVarInRegistry(unique);
        return !borrowBox;
    }
    // #1933 补：结果/可选盒变量=盒亡登记在册（boxVarInRegistry·#1884 单点）
    //   =盒拥有载荷——类载荷盒（载荷=类·串在堆对象内·析构级联 free）不入
    //   ownedFieldOrder_（collectOwnedStrFields 对类载荷不展开·138 行面外），
    //   盒亡登记单点即其拥有权威；借用盒不在册=假（宁漏勿错防双删）。
    if (boxVarInRegistry(unique)) {
        const cn::Optional<std::string> srcBoxOpt = lookupSrcType(name);
        if (srcBoxOpt.has_value()) {
            const std::string canonBox = types::canonical(srcBoxOpt.value());
            if (isResultType(canonBox) || isOptionalType(canonBox)) {
                return true;
            }
        }
    }
    // 有析构类局部（类字段串归堆对象·堆对象归本函数·析构级联 free 该槽）
    return std::find(ownedClassOrder_.begin(), ownedClassOrder_.end(),
                     unique) != ownedClassOrder_.end();
}

// 链上中段=MemberExpr 分支（2089 拆分件）——联合体拒/指针成员跳/递归根：
bool IRGenerator::memberStringRecvOwnedMemberChain(const Expr* obj, int depth) const {
    const MemberExpr* m = static_cast<const MemberExpr*>(obj);
    if (m->isDerefAccess) return false;  // 指针成员（所指无拥有语义）保守假
    // 中段宿主类型联合体拒（164-a）：链穿联合体字段=活跃成员身份不可判
    const Expr* host = m->object.get();
    std::string hostCanon;
    cn::Optional<std::string> hostOpt;
    if (host->getType() == NodeType::IdentifierExpr) {
        hostOpt = lookupSrcType(
            static_cast<const IdentifierExpr*>(host)->name);
    } else if (host->getType() == NodeType::MemberExpr) {
        // memberObjStructType 为 const 只读查询（const 方法）——入参签名
        //   非 const 为历史形态，此处 const_cast 纯查询零改写安全。
        hostOpt = memberObjStructType(const_cast<MemberExpr*>(
            static_cast<const MemberExpr*>(host)));
    }
    // 推导 miss=链宿主类型不可解（非联合体拒面·递归根判定继续）——
    //   显形分流非缺省（1360 结果化口径）
    if (hostOpt.has_value()) {
        hostCanon = types::canonical(hostOpt.value());
    }
    if (!hostCanon.empty() && semantic_ != nullptr) {
        const cn::Optional<const StructDecl*> hostDeclOpt =
            semantic_->findStruct(hostCanon);
        // 布局 miss=宿主非结构体（类/合成体·非联合体拒面）——显形分流
        if (hostDeclOpt.has_value() && hostDeclOpt.value()->isUnion) {
            return false;
        }
    }
    return memberStringRecvPlaceOwnedDepth(host, depth + 1);
}


}  // namespace cn_compiler
