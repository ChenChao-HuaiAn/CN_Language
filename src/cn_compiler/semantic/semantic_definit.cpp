// 010（2026-09-23·任务 plans/021 010·用户裁决甲）：局部值类型变量初始化状态跟踪
//   （def-init·Rust E0381「use of possibly-uninitialized variable」对标·零运行时
//   成本——纯编译期数据流判定，不生成任何运行时检查指令）。
//
// 范围口径（482-a 裁决 + 611-a 开工存量盘点实证）：
//   登记面=局部「纯值类型」：标量（整8~整128/正8~正128/浮32/浮64/布尔/字符）、
//   枚举（#1760·2026-10-28 用户裁决乙案：声明后未赋值就读=编译期拒，与布尔/
//   浮64 完全同构——「先赋值后读」「未读直接覆盖赋值」保持合法）、
//   数组（元素级 place 键，长度编译期已知）、结构体（叶子字段键递归；联合体=单键）。
//   豁免面=拥有型（类/容器/字符串/结果/可选——既有入口零初始化或默认构造防御兜
//   底）、指针/引用、静态（.data/.bss 零初始化语义）、函数参数（调用方实参初始化）。
//   数组出参惯用法豁免（存量铁证 38_tool/455）：数组名作实参=指针退化，不在读判
//   定面；仅「聚合值读」（赋值右值/返回）与元素读（a[i]）判定。
//
// 键编码："<深度>:<名>\x01<后缀>"——\x01 分隔符杜绝前缀混淆（"d:x\x01" 不匹配
//   "d:xy\x01"）；后缀段=常量下标 "[N]" / 非常量下标 "[]" / 字段 ".字段"。后缀含
//   "[]" 的写=该基数整体置位（循环填充惯用法放行·漏报方向诚实边界）；读=查该基
//   数下任一未初始化键（保守拒绝）。
//
// 控制流传播（Rust 数据流同构保守口径）：如果=两支状态 clone 后交集归并（return
//   终止支不参与·stmtGuaranteesReturn 判定）；无否则=恢复进入态（真支可能不执行）；
//   当/循环体=不外溢（可能 0 次+回边保守）；块出口按深度前缀清理（同名遮蔽串扰
//   根除）。性能：状态表 unordered_set·传播为语句级显式 clone/merge（线性）。

#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/switch_enum_gate.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {
namespace {

// 标量类型判定（值类型叶子）：布尔/字符/整N/正N/浮N
bool isDefInitScalarType(const std::string& type) {
    if (type == "布尔" || type == "字符") return true;
    static const char* kPrefixes[] = {"\xE6\x95\xB4", "\xE6\xAD\xA3", "\xE6\xB5\xAE"}; // 整/正/浮
    for (const char* prefix : kPrefixes) {
        const std::size_t plen = std::string(prefix).size();
        if (type.size() > plen && type.compare(0, plen, prefix) == 0) {
            for (std::size_t i = plen; i < type.size(); ++i) {
                if (type[i] < '0' || type[i] > '9') return false;
            }
            return true;
        }
    }
    return false;
}

} // namespace

bool SemanticAnalyzer::isDefInitTrackedType(const std::string& type) const {
    if (type.empty()) return false;
    if (type.back() == '*') return false;                 // 指针豁免
    if (types::isReference(type)) return false;           // 引用豁免
    // 1155：结果/可选判定加 '<' 前缀门（原每次变量声明无条件 splitTypeSuffix 双串拷贝）
    if (type == "字符串" || (type.rfind("结果<", 0) == 0 && isResultType(type)) ||
        (type.rfind("可选<", 0) == 0 && isOptionalType(type))) {
        return false;  // 拥有型
    }
    // 1155：isClassType 恒等于 findClass(type)!=nullptr——原双调用在未命中路径
    //   做两次相同索引查询，删去重复。
    if (isClassType(type)) return false;  // 类/容器实例
    // 953：函数指针（含数组）=指针族豁免（§3.9 豁免面·换防条款同指针：声明点
    //   零初始化+间接调用点判空防线 093/§5.8）——C 风格签名声明的类型文本统一为
    //   「函数指针<…>」形态（探针 p1 实证），带数组后缀（[2]）时 isFuncPtrType
    //   不命中=曾被数组面误纳入登记面（循环填充后读取 E0381 误拒·v2 放行=分叉）。
    //   剥数组后缀后判定；嵌套数组逐层剥。
    {
        std::string probe = type;
        while (types::arrayLenOf(probe).has_value()) {
            probe = types::arrayElemOf(probe);
        }
        if (probe != type && isFuncPtrType(probe)) return false;
    }
    if (types::arrayLenOf(type).has_value()) return true;  // 数组
    // 结构体/联合体豁免：局部结构体裸声明=编译器入口零初始化注入（257 形二
    //   「字段数组零初始化」断言 r.分数[0]==0 实锤·两侧既有防御）——字段读有
    //   定义行为，不入 def-init 登记面（与拥有型同通道）
    if (findStruct(type).has_value()) return false;
    // #1760（2026-10-28 用户裁决乙案）：枚举纳入登记面（001 §3.9 标量清单补
    //   枚举）——声明后未赋值就读与布尔/浮64 同路径同文案拒绝；限定名（甲::颜色）
    //   由 isEnumType 既有 A-2 判定覆盖。枚举数组经上方数组分支逐元素跟踪，
    //   元素叶键在 defInitCollectLeaves 落单键（findStruct 不命中枚举）。
    return isDefInitScalarType(type) || isEnumType(type);  // 标量/枚举
}

// 1360 结果化批3A：无值=未声明（原 -1 哨兵退役）
cn::Optional<int> SemanticAnalyzer::defInitLookupDepth(const std::string& name) const {
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) > 0) return static_cast<int>(i);
    }
    return std::nullopt;
}

void SemanticAnalyzer::defInitCollectLeaves(const std::string& type, const std::string& key,
                                            std::vector<std::string>& out) const {
    // 1360 结果化：has_value() 显形分流
    const cn::Optional<int> lenOpt = types::arrayLenOf(type);
    if (lenOpt.has_value()) {
        const int len = lenOpt.value();
        const std::string elem = types::arrayElemOf(type);
        for (int i = 0; i < len; ++i) {
            defInitCollectLeaves(elem, key + "[" + std::to_string(i) + "]", out);
        }
        return;
    }
    if (const StructDecl* sd = findStruct(type).value_or(nullptr)) {
        if (sd->isUnion) { out.push_back(key); return; }  // 联合体一次一活跃成员=单键
        for (const auto& field : sd->fields) {
            defInitCollectLeaves(field.type, key + "." + field.name, out);
        }
        return;
    }
    out.push_back(key);  // 标量叶子
}

void SemanticAnalyzer::defInitRegisterDecl(const std::string& name, const std::string& type) {
    if (!isDefInitTrackedType(type)) return;
    const int depth = defInitLookupDepth(name).value_or(-1);  // 1360 结果化批3A：无值(未声明)→-1 显形回落，下方 <0 分流保持
    if (depth < 0) return;
    const std::string base = std::to_string(depth) + ":" + name + "\x01";
    std::vector<std::string> leaves;
    defInitCollectLeaves(type, base, leaves);
    for (const auto& k : leaves) uninitPlaces_.insert(k);
}

void SemanticAnalyzer::defInitEraseBasePrefix(const std::string& prefix) {
    for (auto it = uninitPlaces_.begin(); it != uninitPlaces_.end();) {
        if (it->rfind(prefix, 0) == 0) it = uninitPlaces_.erase(it);
        else ++it;
    }
}

// 联合体键归并：place 后缀沿声明类型链逐段解析（[N]"/"[]" 剥一维、".名" 查
//   字段类型），遇联合体类型即把后缀截断到该联合体级——联合体一次一活跃成员
//   （001），写/读任一成员均以联合体整体为初始化单位，与 defInitCollectLeaves
//   的联合体单键登记同口径（否则写「组[0].l」标记键与登记键粒度错位，联合体
//   数组元素字段写后读被误拒 E0381）。无联合体前缀=原样返回（结构体叶子键、
//   标量键行为不变）。
//   类型链与后缀不一致（前置字段存在性/数组下标检查已保证一致，不一致=编译器
//   内部缺陷）→ 报内部错误诊断显形后返回原后缀（诊断已记 Error 编译必失败，
//   控制流继续仅为级联诊断质量——#648 先例同款形态）。
std::string SemanticAnalyzer::defInitSuffixCutAtUnion(const std::string& declType,
                                                      const std::string& suffix,
                                                      const std::string& varName) const {
    std::string type = declType;
    std::size_t pos = 0;
    while (pos < suffix.size()) {
        const char lead = suffix[pos];
        if (lead == '[') {
            // 下标段（"[N]"/"[]"）：数组剥一维（段闭合由 defInitPlaceOf 生成面保证）；
            //   指针下标（p[0] 等价 *(p+0)·150-a 通道）剥一层所指。两者皆非=真不一致。
            const std::size_t close = suffix.find(']', pos);
            if (close == std::string::npos) {
                diagnostics_.report(
                    DiagnosticLevel::Error, SourceLocation(),
                    "内部错误: def-init 后缀 '" + suffix + "' 段未闭合");
                return suffix;
            }
            const bool isArraySeg = types::arrayLenOf(type).has_value();
            const bool isPtrSeg = !isArraySeg && isPointerType(type);
            if (!isArraySeg && !isPtrSeg) {
                // 非数组/非指针根遇 "[]"：取地址整体置位哨兵（1771 checkAddressOfUnary
                //   对 &槽[0]/&签.字段 硬编码后缀 "[]"=「置位到变量根」·无 place 链
                //   语义）——整体置位面无须归并，原样返回保持既有键行为（真实下标
                //   段的类型-后缀一致性由前置数组下标检查保证，联合体/结构体根不产
                //   生下标段）
                return suffix;
            }
            type = isArraySeg ? types::arrayElemOf(type) : types::pointeeOf(type);
            pos = close + 1;
            continue;
        }
        if (lead == '.') {
            // 字段段 ".名"：名界=下一 '.'/'[' 或串尾（defInitPlaceOf 以 AST 成员名
            //   拼接，成员名不含 '.'/'['）；指针对象自动解引用一级（v2.1 p.字段≡
            //   (*p).字段·MemberExpr isDerefAccess 同口径），剥一层所指再查字段。
            std::size_t next = pos + 1;
            while (next < suffix.size() && suffix[next] != '.' && suffix[next] != '[') {
                ++next;
            }
            const std::string fieldName = suffix.substr(pos + 1, next - pos - 1);
            if (isPointerType(type)) {
                type = types::pointeeOf(type);
            }
            // 结果/可选合成成员（Task 3.5 规则2/3 同口径）：.正常/.有值=布尔（叶）、
            //   .值=载荷 T、.错误=错误载荷 E——拥有型豁免面自身无登记键，但链中
            //   载荷可能嵌联合体/结构体（r.值.字段），类型链须能走通
            if (isResultType(type)) {
                const std::vector<std::string> args =
                    resultTypeArgs(type).value_or(std::vector<std::string>());
                if (fieldName == "正常") { type = "布尔"; pos = next; continue; }
                if (fieldName == "值" && args.size() == 2) { type = args[0]; pos = next; continue; }
                if (fieldName == "错误" && args.size() == 2) { type = args[1]; pos = next; continue; }
            }
            if (isOptionalType(type)) {
                if (fieldName == "有值") { type = "布尔"; pos = next; continue; }
                if (fieldName == "值") {
                    type = optionalTypeArg(type).value_or(std::string());
                    pos = next;
                    continue;
                }
            }
            const StructDecl* sd = findStruct(type).value_or(nullptr);
            if (sd == nullptr) {
                // 类实例字段（拥有型豁免面自身无登记键·链中字段可嵌结构体/联合体
                //   ——b.值.字段 等形态类型链须能走通）
                const ClassInfo* ci = findClass(type).value_or(nullptr);
                if (ci == nullptr) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, SourceLocation(),
                        "内部错误: def-init 后缀 '" + suffix + "' 与声明类型 '" + declType +
                        "'（变量 '" + varName + "'）不一致（'" + type +
                        "' 非结构体/联合体/类却有字段段 '" + fieldName + "'）");
                    return suffix;
                }
                const auto cit = ci->fields.find(fieldName);
                if (cit == ci->fields.end()) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, SourceLocation(),
                        "内部错误: def-init 后缀 '" + suffix + "' 与声明类型 '" + declType +
                        "'（变量 '" + varName + "'）不一致（类 '" + type + "' 无字段 '" +
                        fieldName + "'）");
                    return suffix;
                }
                type = cit->second.type;
                pos = next;
                continue;
            }
            if (sd->isUnion) {
                return suffix.substr(0, pos);  // 联合体=单键：截到联合体级
            }
            bool fieldFound = false;
            for (const auto& field : sd->fields) {
                if (field.name == fieldName) {
                    type = field.type;
                    fieldFound = true;
                    break;
                }
            }
            if (!fieldFound) {
                diagnostics_.report(
                    DiagnosticLevel::Error, SourceLocation(),
                    "内部错误: def-init 后缀 '" + suffix + "' 与声明类型 '" + declType +
                    "'（变量 '" + varName + "'）不一致（'" + type + "' 无字段 '" + fieldName + "'）");
                return suffix;
            }
            pos = next;
            continue;
        }
        diagnostics_.report(
            DiagnosticLevel::Error, SourceLocation(),
            "内部错误: def-init 后缀 '" + suffix + "' 段非法（起始字符非 '['/'.'）");
        return suffix;
    }
    return suffix;
}

// 声明类型获取：defInitLookupDepth 命中层内同名条目（同数据源同层，depth 命中
//   后本查必命中）。1360 结果化：无值=作用域状态错位的内部缺陷（内部错误已报
//   显形）——消费点 value_or 空串分流放弃本轮判定（诊断已记 Error 编译必失败，
//   控制流继续仅为级联诊断质量）
cn::Optional<std::string> SemanticAnalyzer::defInitDeclTypeAtDepth(
        int depth, const std::string& name) const {
    const auto& scopeMap = scopes_[static_cast<std::size_t>(depth)];
    const auto it = scopeMap.find(name);
    if (it == scopeMap.end()) {
        diagnostics_.report(
            DiagnosticLevel::Error, SourceLocation(),
            "内部错误: def-init 取变量 '" + name + "' 声明类型落空（作用域状态错位）");
        return std::nullopt;
    }
    return it->second;
}

void SemanticAnalyzer::defInitMarkInitPlace(const std::string& name, const std::string& suffix) {
    const int depth = defInitLookupDepth(name).value_or(-1);  // 1360 结果化批3A：无值(未声明)→-1 显形回落，下方 <0 分流保持
    if (depth < 0) return;
    const std::string declType =
        defInitDeclTypeAtDepth(depth, name).value_or(std::string());
    if (declType.empty()) return;  // 无值=取型落空（内部错误已报显形·放弃本轮判定）
    const std::string base = std::to_string(depth) + ":" + name + "\x01";
    // 联合体键归并：写任一成员=联合体整体置位（与登记单键同口径）
    const std::string cut = defInitSuffixCutAtUnion(declType, suffix, name);
    // 非常量下标写（后缀含 "[]"）=该基数整体置位（循环填充惯用法放行）
    std::string prefix = base + cut;
    const std::size_t dynIdx = cut.find("[]");
    if (dynIdx != std::string::npos) {
        prefix = base + cut.substr(0, dynIdx);
        // 1771：循环体内 [] 写=循环填充惯用法（001 §3.9 诚实边界·漏报方向）——
        //   记录基数前缀供循环体恢复后重应用（体内写过的基数外溢·嵌套循环冒泡）；
        //   循环体外写点即时生效无恢复面，不记录（防跨语句残留累积）。
        // 静默豁免业务值: 循环嵌套深度谓词（loopDepth_>0=在循环体内·门控记录面，
        //   非查表 miss 无默认值通道；深度无自然上界=开区间本质）
        if (loopDepth_ > 0) {
            defInitLoopDynWrites_.push_back(prefix);
        }
    }
    defInitEraseBasePrefix(prefix);
}

void SemanticAnalyzer::defInitCheckReadPlace(const std::string& name, const std::string& suffix,
                                             const SourceLocation& loc) {
    const int depth = defInitLookupDepth(name).value_or(-1);  // 1360 结果化批3A：无值(未声明)→-1 显形回落，下方 <0 分流保持
    if (depth < 0) return;
    const std::string declType =
        defInitDeclTypeAtDepth(depth, name).value_or(std::string());
    if (declType.empty()) return;  // 无值=取型落空（内部错误已报显形·放弃本轮判定）
    const std::string base = std::to_string(depth) + ":" + name + "\x01";
    // 联合体键归并：读任一成员=查联合体整体键（诊断文案仍报用户书写的完整 place）
    const std::string cut = defInitSuffixCutAtUnion(declType, suffix, name);
    std::string place = name + suffix;
    const std::size_t dynIdx = cut.find("[]");
    if (dynIdx == std::string::npos) {
        // 精确读：标量键 / 常量下标 / 字段链 / 联合体整体键
        if (uninitPlaces_.count(base + cut)) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "变量 '" + place +
                                "' 未初始化，不能读取（声明后须先赋值——Rust E0381 对照编译期拒绝）");
        }
        return;
    }
    // 非常量下标读：该基数下任一未初始化键即报（保守拒绝）
    const std::string prefix = base + cut.substr(0, dynIdx) + "[";
    for (const auto& k : uninitPlaces_) {
        if (k.rfind(prefix, 0) == 0) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "变量 '" + name + cut.substr(0, dynIdx) +
                                "[…]' 可能未初始化，不能读取（非常量下标按保守判定——Rust E0381 对照）");
            return;
        }
    }
}

void SemanticAnalyzer::defInitDropScopeDepth(std::size_t depth) {
    const std::string prefix = std::to_string(depth) + ":";
    for (auto it = uninitPlaces_.begin(); it != uninitPlaces_.end();) {
        if (it->rfind(prefix, 0) == 0) it = uninitPlaces_.erase(it);
        else ++it;
    }
}

bool SemanticAnalyzer::defInitPlaceOf(const Expr* e, std::string& name,
                                      std::string& suffix) const {
    CN_SWITCH_ENUM_EXEMPT_BEGIN
    // 存量清偿挂#2074：漏 case 靠 default 挡存量（枚举穷举基线·逐处显式补 case 后删包裹）
    switch (e->getType()) {
        case NodeType::IdentifierExpr:
            name = static_cast<const IdentifierExpr*>(e)->name;
            suffix.clear();
            return true;
        case NodeType::IndexExpr: {
            const auto* idx = static_cast<const IndexExpr*>(e);
            if (!defInitPlaceOf(idx->object.get(), name, suffix)) return false;
            if (idx->index->getType() == NodeType::IntegerLiteral) {
                suffix += "[" +
                          std::to_string(static_cast<const IntegerLiteral*>(idx->index.get())->value) +
                          "]";
            } else {
                suffix += "[]";  // 非常量下标
            }
            return true;
        }
        case NodeType::MemberExpr: {
            const auto* mem = static_cast<const MemberExpr*>(e);
            if (!defInitPlaceOf(mem->object.get(), name, suffix)) return false;
            suffix += "." + mem->memberName;
            return true;
        }
        default:
            return false;  // 调用返回/解引用等临时值基=无未初始化概念
    }
    CN_SWITCH_ENUM_EXEMPT_END
}

void SemanticAnalyzer::defInitMarkAssignTarget(Expr* target) {
    std::string name, suffix;
    bool ok = defInitPlaceOf(target, name, suffix);
    if (!ok) return;
    defInitMarkInitPlace(name, suffix);
}

void SemanticAnalyzer::defInitCheckReadAssignTarget(Expr* target, const SourceLocation& loc) {
    std::string name, suffix;
    if (!defInitPlaceOf(target, name, suffix)) return;
    defInitCheckReadPlace(name, suffix, loc);
}

void SemanticAnalyzer::defInitCheckIf(IfStmt* node) {
    const std::unordered_set<std::string> base = uninitPlaces_;
    // 980 波7（任务 007 NLL）：moved 集随同一次遍历快照-恢复-合流（886 假阳性
    //   根治——标记不得跨互斥分支泄漏；口径与 movedCheckBranches 一致）。
    const MovedSet movedBase = snapshotMoved();
    if (node->thenBranch != nullptr) checkBlock(node->thenBranch.get());
    std::unordered_set<std::string> afterThen = uninitPlaces_;
    const MovedSet movedAfterThen = snapshotMoved();
    const bool thenExits = node->thenBranch != nullptr &&
                           stmtGuaranteesReturn(node->thenBranch.get());
    if (node->elseBranch == nullptr) {
        // 无否则：真支可能不执行——汇合=进入态（真支置位不外溢）
        uninitPlaces_ = base;
        restoreMoved(movedBase);
        (void)afterThen;
        return;
    }
    uninitPlaces_ = base;
    restoreMoved(movedBase);
    checkStmt(node->elseBranch.get());
    std::unordered_set<std::string> afterElse = uninitPlaces_;
    const MovedSet movedAfterElse = snapshotMoved();
    const bool elseExits = stmtGuaranteesReturn(node->elseBranch.get());
    if (thenExits && elseExits) {
        uninitPlaces_ = base;  // 后续不可达，状态取任意一致视图
        restoreMoved(movedBase);
        return;
    }
    if (thenExits) {
        uninitPlaces_ = std::move(afterElse);
        restoreMoved(movedAfterElse);
        return;
    }
    if (elseExits) {
        uninitPlaces_ = std::move(afterThen);
        restoreMoved(movedAfterThen);
        return;
    }
    // 并集归并（def-init=must analysis）：已初始化须「所有路径都初始化」——
    //   未初始化集汇合=并集（真支赋值+否则未赋→否则路径仍未初始化=保持未初始化）
    std::unordered_set<std::string> merged = afterElse;
    for (const auto& k : afterThen) {
        merged.insert(k);
    }
    uninitPlaces_ = std::move(merged);
    // moved=may analysis：任一路径转移即 moved（Rust E0382 同款·保守不放宽）
    restoreMoved(movedAfterElse);
    mergeMovedOr(movedAfterThen);
}

void SemanticAnalyzer::defInitCheckLoopBody(BlockStmt* body) {
    const std::unordered_set<std::string> base = uninitPlaces_;
    // 1771：本层循环体的 [] 写记录与本层前的外层记录隔离（体内记录只归本层
    //   消费+冒泡，外层挂起记录不受体内检查影响）
    std::vector<std::string> savedDynWrites = std::move(defInitLoopDynWrites_);
    defInitLoopDynWrites_.clear();
    checkBlock(body);
    // 体内「[] 写」基数前缀=本层填充面（常量下标/标量/字段写不记录——001 §3.9
    //   「循环体内赋值不外溢」保守口径对它们维持）
    std::vector<std::string> bodyDynWrites = std::move(defInitLoopDynWrites_);
    // 体可能 0 次执行+回边保守——体内置位先回滚进入态
    uninitPlaces_ = base;
    // 1771：循环填充惯用法放行（001 §3.9 诚实边界·漏报方向）——体内 [] 写
    //   过的基数重应用整体置位（循环执行即填充的惯用语义；循环 0 次的漏报=
    //   设计接受的诚实边界）。v2 侧同面放行（零分叉）。
    for (const auto& prefix : bodyDynWrites) {
        defInitEraseBasePrefix(prefix);
    }
    // 冒泡口径（loopDepth_ 进入本函数前已含本层）：嵌套循环（>1）本层记录并入
    //   挂起集冒泡——内层填充=外层填充的一部分，外层恢复时同样重应用；最外层
    //   （==1）本层记录消费完即弃（恢复挂起集=循环外不记录守卫下恒空），不跨
    //   语句残留。
    // 静默豁免业务值: 循环嵌套深度谓词（loopDepth_>1=嵌套循环内·门控冒泡面，
    //   非查表 miss 无默认值通道；深度无自然上界=开区间本质）
    if (loopDepth_ > 1) {
        for (const auto& prefix : bodyDynWrites) {
            savedDynWrites.push_back(prefix);
        }
    }
    defInitLoopDynWrites_ = std::move(savedDynWrites);
}

} // namespace cn_compiler
