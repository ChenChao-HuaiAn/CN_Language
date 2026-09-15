// 借出视图生命周期检查（plans/019 阶段3 扩展 · plans/020 矩阵 A21 格靶子）
// 第七十七轮（2026-09-12）：借出视图 × 容器移除 = UAF 的编译期拦截。
//
// 语义模型（Rust 借用检查器同构，NLL 顺序近似）：
//   借出视图 = 容器内元素句柄的**浅拷**（字符串元素容器的 元素/读取/栈顶/队首/
//   头部元素/读取头部/读取尾部/获取 的返回值——容器拥有元素模型下，该句柄与
//   容器槽共享同一所有权）。容器释放该元素（失效方法）或容器先亡（析构释放
//   元素）后，借出视图即悬垂。
//   判据 = 「引用活跃区间 = 绑定点 → 最后一次使用点」（Rust 非词法生命周期），
//   活跃区间内出现容器失效点即拒绝（E0502 对标）；Rust 靠类型系统携带借用信息，
//   CN 无借用类型 → 靠**方法名清单 + 来源登记**（诚实边界见下）。
//
// 两个子形态（探针 77/77-2 实证，宿主侧内容损坏）：
//   ①同作用域失效：容器失效方法调用落在（绑定行, 最后使用行）之间；
//   ②跨作用域逃逸（容器先亡）：借出视图在容器声明作用域之外仍被使用。
//
// 诚实边界（第一版）：
//   · 接收者为标识符的容器（成员链/指针接收者 = 后续轮）；
//   · 跨函数别名（借出视图经实参传入函数、由被调函数修改容器）检不出（Rust 靠
//     生命周期参数检查，CN 靠纪律）；
//   · 元素为值语义类型（结构体/类/整型）的容器不在面内（元素() 返回深拷贝=非借用）；
//   · 非字符串元素容器（向量<整64> 等）不在面内（无所有权语义）。
#include <algorithm>
#include <string>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 借出方法名判定：容器内元素句柄的读出接口（stdlib/容器.cn + 映射集合.cn）。
//   向量/链表=元素/读取/头部元素/读取头部/读取尾部；栈=栈顶；队列=队首；
//   映射=获取（结果<V,整32> 装箱携带值句柄浅拷=借用）。
bool SemanticAnalyzer::isBorrowViewMethod(const std::string& m) {
    return m == "元素" || m == "读取" || m == "栈顶" || m == "队首" ||
           m == "头部元素" || m == "读取头部" || m == "读取尾部" || m == "获取";
}

// 容器失效方法判定（使借出视图句柄失效/所有权变更的方法）——按实例化头分派：
//   名称相同语义不同的 清空 在此区分（76-a 全量释放面口径）：
//     链表/队列/映射 清空=按链/平铺全量释放（元素句柄消失→悬垂）；
//     向量 清空=仅 元素数量归零（底层数组与串均不释放）→ 不入面（泄漏面另计）。
//   追加/插入/压入/入队/头部追加/添加（扩容类）不入面：字符串元素是独立堆块，
//     槽位搬移不使句柄失效（C++ vector 迭代器失效语义在此收窄为「串释放」事实）。
bool SemanticAnalyzer::isContainerInvalidateCall(const std::string& canonClass,
                                                 const std::string& m) {
    const std::size_t dl = canonClass.find('$');
    const std::string head =
        (dl == std::string::npos) ? canonClass : canonClass.substr(0, dl);
    if (head == "向量") return m == "删除" || m == "设置";
    if (head == "集合") return m == "删除";
    if (head == "链表") {
        return m == "删除头部" || m == "删除尾部" || m == "清空";
    }
    if (head == "队列") return m == "出队" || m == "清空";
    if (head == "栈") return m == "弹出";
    if (head == "映射") {
        return m == "删除" || m == "设置" || m == "清空" || m == "释放内部数组";
    }
    return false;
}

// 借出来源容器判定：字符串元素容器（向量/链表/栈/队列/集合$字符串）或
//   字符串值映射（映射$K$字符串）——与 IR 层容器元素释放面同一口径（types:: 共享）。
bool SemanticAnalyzer::isBorrowSourceContainer(const std::string& canonType) {
    return types::isStringElemContainer(canonType) ||
           types::isStringValuedMap(canonType);
}

// 表达式路径文本重建（标识符/成员/下标/解引用）——容器引用键的成员链形态用。
//   下标统一记 `[]`（不展开下标表达式：`p[0].表` 与 `p[1].表` 同键——保守合并，
//   不同下标的区分列为诚实边界）；解引用记 `*` 前缀；未知形态记 `?`（调用方拒绝）。
void SemanticAnalyzer::buildPathText(const Expr* e, std::string& out) {
    if (e == nullptr) {
        out += "?";
        return;
    }
    switch (e->getType()) {
        case NodeType::IdentifierExpr:
            out += static_cast<const IdentifierExpr*>(e)->name;
            break;
        case NodeType::MemberExpr: {
            const MemberExpr* m = static_cast<const MemberExpr*>(e);
            buildPathText(m->object.get(), out);
            out += ".";
            out += m->memberName;
            break;
        }
        case NodeType::IndexExpr:
            buildPathText(static_cast<const IndexExpr*>(e)->object.get(), out);
            out += "[]";
            break;
        case NodeType::UnaryExpr: {
            const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
            if (u->op == Operator::Deref) out += "*";
            buildPathText(u->operand.get(), out);
            break;
        }
        default:
            out += "?";
            break;
    }
}

// 表达式 → 容器引用键（77-a 扩展②：成员链接收者支持）。
//   标识符 → 键=变量名；成员链/下标链/解引用 → 键=基础名.成员路径文本（基础名经
//   refReturnLvalueBase 解剖，ID 取基础名）；其余形态（调用/字面量/二元等）→ false。
bool SemanticAnalyzer::resolveContainerRef(const Expr* e, std::string& outKey,
                                           int& outVarId) const {
    outKey.clear();
    outVarId = 0;
    if (e == nullptr) return false;
    const NodeType t = e->getType();
    if (t == NodeType::IdentifierExpr) {
        outKey = static_cast<const IdentifierExpr*>(e)->name;
        if (outKey.empty()) return false;
        outVarId = lookupVarId(outKey, nullptr);
        return true;
    }
    if (t == NodeType::MemberExpr || t == NodeType::IndexExpr ||
        (t == NodeType::UnaryExpr &&
         static_cast<const UnaryExpr*>(e)->op == Operator::Deref)) {
        std::string base;
        if (!refReturnLvalueBase(e, base) || base.empty()) return false;
        std::string path;
        buildPathText(e, path);
        if (path.empty() || path.find('?') != std::string::npos) return false;
        outKey = path;
        outVarId = lookupVarId(base, nullptr);
        return true;
    }
    return false;
}

// 按名解析活跃借出登记（与 lookupVar 同序：从内到外第一层含该名的层——内层同名的
//   重新声明=新变量，遮蔽语义正确）。SIZE_MAX=未命中。
std::size_t SemanticAnalyzer::findActiveBorrowView(const std::string& name) const {
    if (borrowViews_.empty() || borrowViewScopes_.empty()) {
        return static_cast<std::size_t>(-1);
    }
    for (std::size_t i = borrowViewScopes_.size(); i-- > 0;) {
        auto it = borrowViewScopes_[i].find(name);
        if (it != borrowViewScopes_[i].end()) return it->second;
    }
    return static_cast<std::size_t>(-1);
}

// 变量身份 ID：沿作用域链解析（与 lookupVar 同序——内层遮蔽正确）。
//   返回 -1=不可见（未声明/已出作用域），0=可见但无 ID（未追踪绑定，如全局层），
//   >0=身份 ID。ID 由 declareVar 分配（同名不同声明=不同 ID，防跨作用域误配）。
int SemanticAnalyzer::lookupVarId(const std::string& name,
                                  int* outScopeIndex) const {
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) == 0) continue;
        if (outScopeIndex != nullptr) *outScopeIndex = static_cast<int>(i);
        if (i < scopeVarIds_.size()) {
            auto it = scopeVarIds_[i].find(name);
            if (it != scopeVarIds_[i].end()) return it->second;
        }
        return 0;
    }
    if (outScopeIndex != nullptr) *outScopeIndex = -1;
    return -1;
}

// 方法调用点登记（visitCallExpr 方法分支）：
//   · 借出方法（字符串元素容器）→ 置 lastExprIsBorrowView_ 供绑定位（声明/赋值）
//     消费（结果变量与字符串变量同处理：结果.值 即借出句柄）；
//   · 失效方法 → 登记容器失效点（函数级结算与借出活跃区间比对）。
//   接收者须为标识符（第一版诚实边界：成员链/指针接收者不登记）。
void SemanticAnalyzer::noteBorrowCallSite(const MemberExpr& mem,
                                         const std::string& clsName,
                                         const std::string& methodName,
                                         const CallExpr* callNode) {
    lastExprIsBorrowView_ = false;
    lastBorrowCallNode_ = nullptr;
    if (!isBorrowSourceContainer(clsName)) return;
    // 接收者 → 容器引用键（标识符=名；成员链=基础名.成员路径——77-a 扩展②；
    //   调用/字面量接收者不登记=诚实边界）
    std::string recv;
    int id = 0;
    if (!resolveContainerRef(mem.object.get(), recv, id)) return;
    if (isBorrowViewMethod(methodName)) {
        lastExprIsBorrowView_ = true;
        lastBorrowCallNode_ = callNode;   // 仅「顶层即该调用」的绑定位才登记
        lastBorrowContainer_ = recv;
        lastBorrowContainerId_ = id;
        lastBorrowContainerType_ = clsName;
        return;
    }
    if (isContainerInvalidateCall(clsName, methodName)) {
        ContainerMutationInfo mu;
        mu.container = recv;
        mu.containerVarId = id;
        mu.line = mem.location.getLine();
        mu.method = methodName;
        mu.containerType = clsName;
        containerMutations_.push_back(mu);
    }
}

// 借出绑定登记（visitVarDecl 初始化位 / visitAssignmentExpr 赋值位消费
//   lastExprIsBorrowView_）。登记层 = **借出视图变量的声明层**（不是赋值发生层——
//   赋值位形态 `借出 = 表.元素(0)` 中 借出 可能声明在更外层作用域：容器先亡的
//   逃生形态正需该记录存活到使用点，探针 77-2 形态G 实证）。同层同名=重新绑定
//   （NLL 语义：新借用取代旧借用，活跃区间从新绑定点重新计），更新既有记录。
void SemanticAnalyzer::registerBorrowView(const std::string& viewVar,
                                         const SourceLocation& loc) {
    if (!lastExprIsBorrowView_) return;
    int layerIdx = -1;
    lookupVarId(viewVar, &layerIdx);
    if (layerIdx < 0 ||
        static_cast<std::size_t>(layerIdx) >= borrowViewScopes_.size()) {
        layerIdx = static_cast<int>(scopes_.size()) - 1;   // 兜底：当前层
    }
    if (layerIdx < 0) return;
    auto& layer = borrowViewScopes_[static_cast<std::size_t>(layerIdx)];
    auto it = layer.find(viewVar);
    if (it != layer.end() && it->second < borrowViews_.size()) {
        BorrowViewInfo& bv = borrowViews_[it->second];
        bv.container = lastBorrowContainer_;
        bv.containerVarId = lastBorrowContainerId_;
        bv.containerType = lastBorrowContainerType_;
        bv.bindLine = loc.getLine();
        bv.bindLoc = loc;
        bv.lastUseLine = 0;
        bv.reported = false;
        return;
    }
    BorrowViewInfo bv;
    bv.viewVar = viewVar;
    bv.container = lastBorrowContainer_;
    bv.containerVarId = lastBorrowContainerId_;
    bv.containerType = lastBorrowContainerType_;
    bv.bindLine = loc.getLine();
    bv.bindLoc = loc;
    borrowViews_.push_back(bv);
    layer[viewVar] = borrowViews_.size() - 1;
}

// 借出视图使用登记（visitIdentifierExpr 读值根拦截点——表达式/实参/返回/
//   成员与下标对象侧/操作数均经此）。两件事：
//   ①更新活跃区间右端（NLL：最后一次使用点）；
//   ②跨作用域逃逸实时判定——使用发生时容器已不可见（或已换绑同名变量）
//     = 容器先亡（析构已释放元素），立即报错（位置=使用点，体验优于函数尾）。
void SemanticAnalyzer::noteBorrowViewUse(const std::string& name,
                                        const SourceLocation& loc) {
    const std::size_t idx = findActiveBorrowView(name);
    if (idx == static_cast<std::size_t>(-1) || idx >= borrowViews_.size()) return;
    BorrowViewInfo& bv = borrowViews_[idx];
    if (loc.getLine() > bv.lastUseLine) bv.lastUseLine = loc.getLine();
    if (bv.reported) return;
    // 容器可见性判定：全局静态=进程生命周期（永远可见，豁免）；
    //   -1=不可见（已出作用域）→ 逃逸；>0 且与绑定 ID 不同=指到别的同名变量 → 逃逸。
    // 逃逸判定只对「标识符容器」形态（成员链容器的可见性判定复杂——键含 `.`/`[`/`*`
    //   时跳过，登记为诚实边界；成员链仍参与失效点判定）。
    const bool plainName = bv.container.find_first_of(".[*") == std::string::npos;
    if (!plainName) return;
    const bool isGlobalStatic =
        globalStatics_.count(bv.container) > 0 ||
        globalStaticsQualified_.count(bv.container) > 0;
    if (!isGlobalStatic) {
        const int curId = lookupVarId(bv.container, nullptr);
        if (curId == -1) {
            bv.reported = true;
            diagnostics_.report(
                DiagnosticLevel::Error, loc,
                "借出视图 '" + name + "'（来源容器 '" + bv.container +
                    "'，绑定 行" + std::to_string(bv.bindLine) +
                    "）在容器作用域结束后仍被使用——容器析构已释放该元素"
                    "（容器先亡=悬垂）；须在容器作用域内 字符串复制(...) "
                    "取拥有副本（Rust 借用检查器同类拒绝）");
            return;
        }
        if (curId > 0 && bv.containerVarId > 0 && curId != bv.containerVarId) {
            bv.reported = true;
            diagnostics_.report(
                DiagnosticLevel::Error, loc,
                "借出视图 '" + name + "'（来源容器 '" + bv.container +
                    "'，绑定 行" + std::to_string(bv.bindLine) +
                    "）与当前作用域同名容器不是同一对象，已失效；"
                    "须重新借出或先 字符串复制(...) 取拥有副本");
        }
    }
}

// 调用点同源互斥（77-a 扩展①，跨函数别名窄面）：同一调用的实参列表中，
//   「容器引用键 = C」与「来源为 C 的借出视图实参」并存 → 拒绝。
//   动机（探针 A 实证 UAF）：`帮忙改(表, 借出)` —— 被调函数形参 表 与调用方容器
//   共享句柄（CN 容器形参=借用），被调函数若删除元素，调用方的借出视图即悬垂
//   （实测长度 3 vs 期望 13）；编译器无法知道被调者是否修改容器 → 保守拒绝
//   （与阶段 3b「同调用可变×只读互斥」同哲学）。Rust 对照：`f(&mut v, &v[0])`
//   被借用检查器拒绝（同调用容器与其视图不可并存）。
//   覆盖面=「实参标识符已登记为借出视图」（探针 A 形态）；内联形态
//   `f(表, 表.元素(0))` 与经结构体/全局中转的别名列诚实边界。
void SemanticAnalyzer::checkBorrowViewCallArgs(CallExpr* node) {
    if (node == nullptr || node->arguments.size() < 2) return;
    if (borrowViews_.empty()) return;
    for (std::size_t i = 0; i < node->arguments.size(); ++i) {
        const Expr* a = node->arguments[i].get();
        if (a == nullptr || a->getType() != NodeType::IdentifierExpr) continue;
        const std::string viewName =
            static_cast<const IdentifierExpr*>(a)->name;
        const std::size_t vi = findActiveBorrowView(viewName);
        if (vi == static_cast<std::size_t>(-1) || vi >= borrowViews_.size()) {
            continue;
        }
        if (borrowViews_[vi].reported) continue;
        // 同调用其他实参中查找同源容器
        for (std::size_t j = 0; j < node->arguments.size(); ++j) {
            if (j == i) continue;
            std::string ckey;
            int cid = 0;
            if (!resolveContainerRef(node->arguments[j].get(), ckey, cid)) {
                continue;
            }
            if (ckey != borrowViews_[vi].container) continue;
            if (borrowViews_[vi].containerVarId > 0 && cid > 0 &&
                cid != borrowViews_[vi].containerVarId) {
                continue;
            }
            borrowViews_[vi].reported = true;
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "实参 '" + viewName + "' 是容器 '" + borrowViews_[vi].container +
                    "' 的借出视图（绑定 行" +
                    std::to_string(borrowViews_[vi].bindLine) +
                    "），与该容器同时作为实参传入——被调函数若修改容器该视图即悬垂；"
                    "须先 字符串复制(...) 取拥有副本"
                    "（Rust 借用检查器同类拒绝：同调用容器与其视图不可并存）");
            break;
        }
    }
}

// 函数级结算（checkFunctionBody / 类方法体 / lambda 体尾部）：
//   同作用域失效形态——容器失效点落在（绑定行, 最后使用行）之间 → 借出视图悬垂。
//   诊断按（绑定行, 失效行）排序输出（跨嵌套函数定义顺序稳定）。
void SemanticAnalyzer::checkBorrowViewLifetimes() {
    if (!borrowViews_.empty() && !containerMutations_.empty()) {
        struct Hit {
            int bindLine;
            int mutLine;
            int useLine;
            std::string message;
        };
        std::vector<Hit> hits;
        for (const auto& bv : borrowViews_) {
            if (bv.reported || bv.lastUseLine <= bv.bindLine) continue;
            for (const auto& mu : containerMutations_) {
                if (mu.line <= bv.bindLine || mu.line >= bv.lastUseLine) continue;
                // 容器身份：引用键相同 ∧（ID 相同或任一为 0——成员链基础名/全局）
                const bool same =
                    (bv.container == mu.container) &&
                    ((bv.containerVarId > 0 && mu.containerVarId > 0)
                         ? (bv.containerVarId == mu.containerVarId)
                         : true);
                if (!same) continue;
                Hit h;
                h.bindLine = bv.bindLine;
                h.mutLine = mu.line;
                h.useLine = bv.lastUseLine;
                h.message =
                    "借出视图 '" + bv.viewVar + "'（来源容器 '" + bv.container +
                    "'，绑定 行" + std::to_string(bv.bindLine) +
                    "）在容器失效点（行" + std::to_string(mu.line) + " " +
                    mu.method + "）之后仍被使用（行" +
                    std::to_string(bv.lastUseLine) +
                    "）——该元素已被释放/移出（悬垂）；须先 字符串复制(...) "
                    "取拥有副本，或调整使用顺序（Rust 借用检查器同类拒绝）";
                hits.push_back(h);
                break;
            }
        }
        std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
            if (a.bindLine != b.bindLine) return a.bindLine < b.bindLine;
            return a.mutLine < b.mutLine;
        });
        for (const auto& h : hits) {
            diagnostics_.report(DiagnosticLevel::Error,
                                SourceLocation(std::string(), h.useLine, 1),
                                h.message);
        }
    }
    clearBorrowViewState();
}

void SemanticAnalyzer::clearBorrowViewState() {
    borrowViews_.clear();
    containerMutations_.clear();
    // 与 scopes_ 保持逐层平行（清空为同数量空层——否则后续 pushScope 的
    //   emplace 会与 scopes_ 层错位）
    borrowViewScopes_.assign(scopes_.size(), {});
}

} // namespace cn_compiler
