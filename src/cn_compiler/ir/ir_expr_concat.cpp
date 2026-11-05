// CN-IR生成器实现（728o：字符串连接族——自 ir_expr.cpp 按族拆出并承载连接链融合）
//   搬移 = genStringConcatBinary + genStringConcatI128（176-a 族②③·纯搬运零行为
//   变更；数值转换段同时提取为单点事实源 emitConcatNumToStr，两消费位共用）。
//   新增 = 连接链融合（#366 提交「遗留」段如实挂账项）：一次性连接链
//   `甲 + 乙 + 丙 + …` 编译器识别整链、改写为「新建缓冲 -> 逐段追加 -> 完成」
//   的构建器序列（复用 366 的 runtime __cn_strbuf_* 设施·001 §10.3）——朴素
//   加号链原每次「+」独立分配整串拷贝（k 段链 O(k²) 字节拷贝），融合后一趟
//   构建 O(L)。
//   语义等价铁律（用户口径）：①终串逐字节等值（含 UTF-8 多字节）②链上段
//   「求值顺序 / 求值次数 / 内容读取时点」与原逐节左结合链逐一镜像（内容读取
//   时点=原内层 concat 的执行时点——防副作用求值期间修改段内容的语义漂移）
//   ③拥有权/RAII 同逐节连接（出串=finish 产物新拥有串·段拥有临时（调用返回
//   串/连接子链结果）追加后同点结算释放·数值转换器产物同点释放——617 口径）
//   ④链长不足/形态不确定一律回退原逐节路径（零行为变更）。
//   拆分依据：原 ir_expr.cpp 841 行，新增融合族逼近 1000 行红线（D1 纪律
//   先拆再加·112-a/174-a 同款按族拆分先例）。
#include <cstddef>
#include <string>
#include <vector>

#include "cn_compiler/common/switch_enum_gate.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

namespace {
// 源码类型文本判定「字符串语义类型」——拼接/比较共用判据的单点入口：
//   剥尾 '&'（引用返回后缀·右值读值后类型；与下方调用形态豁免同口径）后按
//   model 层单一事实源 isStringSemanticType（字符串/字符*）判定；'*' 不剥
//   （字符* 是字符串视图类型本体）。无值/空串=无法推导 → 假（保守不判字符串·
//   禁默认猜型；漏判方向=退回整值比较/指针算术，由调用点各自的兜底纪律处理）。
bool isStringSemanticSrcText(const cn::Optional<std::string>& srcType) {
    if (!srcType.has_value() || srcType.value().empty()) return false;
    std::string t = srcType.value();
    if (t.back() == '&') t.pop_back();
    return isStringSemanticType(types::canonical(t));
}
}  // namespace

// 728o：连接链融合最小段数阈值。链长 2（`甲+乙`）时原路径「一次分配+两段
//   拷贝」已是最优（融合反而多出缓冲分配与完成拷贝=负优化），故不融合；
//   链长 >=3 起逐节中间串的反复整串拷贝被消除（k 越大收益越大——详见
//   tests/bench/b6 连接链形态基准与提交信息量化对拍）。
static constexpr std::size_t kMinConcatChainSegs = 3;

// ==================== 共享：数值/字符/布尔 -> 字符串转换器发射 ====================
// Task 2.9 分派（原 genStringConcatBinary 分支②逐字提取·两消费位共用单点）：
//   整数/枚举 -> __cn_str_from_int（整32 先 Cast i64）
//   无符号（正8~正64） -> __cn_str_from_uint（先 Cast u64·%llu 语义）
//   浮点 -> __cn_str_from_float（f32 先 Cast f64）
//   字符 -> __cn_str_from_char（i32 值；右操作数形态=CharLiteral 直判 +
//       变量源码类型查表「字符」——IndexExpr 等其余 i32 形态走整数转换，
//       与融合前原判定逐形态一致）
//   布尔 -> __cn_str_from_bool（i1 -> "真"/"假"）
ir::IRValue IRGenerator::emitConcatNumToStr(Expr* operandAst,
                                            const ir::IRValue& value,
                                            const SourceLocation& loc) {
    ir::IRValue strArg = value;
    std::string convFn;
    if (strArg.type == "f32" || strArg.type == "f64") {
        if (strArg.type != "f64") {
            strArg = emitResult(ir::Opcode::Cast, {strArg}, "f64", "", loc);
        }
        convFn = "__cn_str_from_float";
    } else if (strArg.type == "i1") {
        convFn = "__cn_str_from_bool";  // 布尔转 "真"/"假"
    } else if (strArg.type == "i32" || strArg.type == "u32") {
        // 字符字面量/字符变量（IR i32）走字符转换；整32/枚举 走整数转换。
        // 区分依据：右操作数 AST 形态（CharLiteral 直判）+ 变量源码类型查表
        bool isChar = false;
        if (operandAst != nullptr &&
            operandAst->getType() == NodeType::CharLiteral) {
            isChar = true;
        } else if (operandAst != nullptr &&
                   operandAst->getType() == NodeType::IdentifierExpr) {
            const std::string st = types::canonical(lookupSrcType(
                static_cast<IdentifierExpr*>(operandAst)->name).value_or(""));
            isChar = (st == "字符");
        }
        if (isChar) {
            convFn = "__cn_str_from_char";
        } else {
            if (strArg.type != "i64") {
                strArg = emitResult(ir::Opcode::Cast, {strArg}, "i64", "", loc);
            }
            convFn = "__cn_str_from_int";
        }
    } else {
        // 整数（i64/u64/i8/i16 等）：缺陷修复——无符号（正8~正64）走
        //   __cn_str_from_uint（%llu 语义，值超 2^63 正确显示正数），
        //   有符号统一转 i64 走 __cn_str_from_int（%lld）
        const bool isUnsignedNum = (strArg.type == "u8" ||
                                    strArg.type == "u16" ||
                                    strArg.type == "u32" ||
                                    strArg.type == "u64");
        if (isUnsignedNum) {
            if (strArg.type != "u64") {
                strArg = emitResult(ir::Opcode::Cast, {strArg}, "u64", "", loc);
            }
            convFn = "__cn_str_from_uint";
        } else {
            if (strArg.type != "i64") {
                strArg = emitResult(ir::Opcode::Cast, {strArg}, "i64", "", loc);
            }
            convFn = "__cn_str_from_int";
        }
    }
    return emitResult(ir::Opcode::Call, {strArg}, "ptr", convFn, loc);
}

// ==================== 族②：字符串连接（Task 2.5）+ 字符串+数值隐式拼接（Task 2.9） ====================
// 说明：字符串类型在IR层映射为 ptr；两操作数均为 ptr 且运算符为 + 时视为连接
//       （指针+整数 仍走指针算术分支；指针+指针 由语义层保证为字符串连接）
// 多操作数左结合："a" + 1 + 2 = ("a"+1)+2：内层结果为 ptr（连接产物），外层再拼。
//   连接/转换产物为动态内存，调用方负责 字符串释放（与 Task 2.5 语义一致）。
// 注意：Call 有副作用，CSE 已排除（cse.cpp isSideEffect），不会被错误合并。
// 关键区分：ptr 左操作数可能是 字符串（拼接）或 普通指针（指针算术 q + 1）。
//   拼接仅当 左操作数源码类型为 字符串/字符*（字面量 或 字符串变量/数组元素）；
//   普通指针 + 整数 必须走指针算术分支（05_array_pointer 回归教训）。
// 展开：右操作数先转字符串（__cn_str_from_int/float/char/bool），再 __cn_str_concat。
bool IRGenerator::genStringConcatBinary(BinaryExpr* node, const ir::IRValue& left,
                                        const ir::IRValue& right) {
    if (left.type == "ptr" && right.type == "ptr" && node->op == Operator::Add) {
        lastExpr_ = emitResult(ir::Opcode::Call, {left, right}, "ptr",
                               "__cn_str_concat", node->location);
        // 617（2026-10-14 伞批）：concat 只读消费实参——发射完成后结算实参中
        //   的拥有串临时（拼接链中间临时归零点：t2=连接(t1,c) 发射完释放 t1；
        //   字面量/标识符形态由 argIsOwnedStringTemp 判据天然排除）
        //   662（617 三元面补全）：三元操作数走分支拥有性感知条件释放
        //   （单一入口 Rec617·选中字面量分支不得 free——无块计数上下文残余清）
        emitOwnedTempReleaseRec(node->left.get(), left, node->location);
        emitOwnedTempReleaseRec(node->right.get(), right, node->location);
        return true;
    }
    const bool leftIsString = isStringTypedExpr(node->left.get());
    if (leftIsString && left.type == "ptr" && node->op == Operator::Add &&
        right.type != "ptr" && right.type != "i128" && right.type != "u128") {
        ir::IRValue rightStr = emitConcatNumToStr(node->right.get(), right,
                                                  node->location);
        lastExpr_ = emitResult(ir::Opcode::Call, {left, rightStr}, "ptr",
                               "__cn_str_concat", node->location);
        // 617（2026-10-14 伞批）：转换器（from_int/float/char/bool/uint）产物
        //   仅被本次 concat 消费——发射完成即释放；左操作数中拥有串临时
        //   （如 整数转字符串(1) + 2 的左临时）同点结算（662 三元走条件释放）。
        emit(ir::Opcode::Call, {rightStr}, ir::IRValue(), "__cn_str_free",
             "void", node->location);
        emitOwnedTempReleaseRec(node->left.get(), left, node->location);
        return true;
    }
    // 1960（执法一·2026-11-02）：左=ptr 但字符串形态两面都推不出且右侧非
    //   指针算术形态（ptr/128 位段已分流）=语义层拼接放行而 IR 分派面形态
    //   未登记——原静默 return false 落指针算术分支（撞 #648「指针步进源
    //   类型未登记」错位文案·步进 8 错值）。判据=源码类型两面（exprSrcType
    //   ∪ ptrOperandSrcType——后者防 (p+1)+1 指针算术链误伤·1064 同源）均
    //   无值才显形：普通指针+整数=指针算术合法路径零变更（05_array_pointer
    //   回归教训·真指针变量 lookupSrcType/± 链回溯恒可解析）。显形口径同
    //   #648 ptrElemStride：报内部错误后占位发射（lastExpr_=布尔假·中止前
    //   合法 IR 值），driver generate() 后 hasErrors() 中止·产物不落盘。
    if (!leftIsString && left.type == "ptr" && node->op == Operator::Add &&
        right.type != "ptr" && right.type != "i128" && right.type != "u128" &&
        !exprSrcType(node->left.get()).has_value() &&
        !ptrOperandSrcType(node->left.get()).has_value()) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 字符串拼接形态未登记（分派面：拼接·缺形态：exprSrcType "
            "左侧无值）");
        lastExpr_ = ir::IRValue::constant("假", "i1");  // 占位（中止前布尔假）
        return true;
    }
    return false;
}

// ==================== 族③：字符串 + 整128/正128 ====================
// i128 值在 IR 层为"指向16字节双槽内存的 ptr"（低64位槽+高64位槽），
// right 本身就是该地址（变量槽地址或临时双槽地址）。取低64位 LoadPtr 转整64
//（值域≤2^63 语义正确；超范围拼接场景后续 Task 再支持全量转换）。
// true = 已处理。
bool IRGenerator::genStringConcatI128(BinaryExpr* node, const ir::IRValue& left,
                                      const ir::IRValue& right) {
    if (isStringTypedExpr(node->left.get()) && left.type == "ptr" &&
        node->op == Operator::Add &&
        (right.type == "i128" || right.type == "u128")) {
        // i128 值是"双槽寄存器值"（IRValue.id=高64位槽、id+1=低64位槽），
        // 而非指针地址——不能用 LoadPtr 解引用（会把槽值当地址访问导致
        // 0xC0000005）。正确做法：Cast i128 -> i64 取低64位（emitCast 已有
        // i128/u128->i64 分支，值域≤2^63 语义正确）。
        ir::IRValue lo = emitResult(ir::Opcode::Cast, {right}, "i64", "",
                                    node->location);
        ir::IRValue rightStr = emitResult(ir::Opcode::Call, {lo}, "ptr",
                                          "__cn_str_from_int", node->location);
        lastExpr_ = emitResult(ir::Opcode::Call, {left, rightStr}, "ptr",
                               "__cn_str_concat", node->location);
        // 617：转换产物仅被本次 concat 消费——发射完成即释放（族②同口径；
        //   左操作数拥有串临时同点结算，662 三元走条件释放）
        emit(ir::Opcode::Call, {rightStr}, ir::IRValue(), "__cn_str_free",
             "void", node->location);
        emitOwnedTempReleaseRec(node->left.get(), left, node->location);
        return true;
    }
    // 1960（执法一·2026-11-02·族②显形同款扩段）：左=ptr 但字符串形态两面
    //   都推不出且右侧=128 位段=拼接分派面形态未登记——原静默 return false
    //   落指针算术分支（ptrElemStride 错位文案·步进错值）。两面判据与占位
    //   口径同族②（exprSrcType ∪ ptrOperandSrcType 均无值——防 (p+1)+大数
    //   指针算术链误伤·合法面零变更），报内部错误后 driver 收尾中止。
    if (left.type == "ptr" && node->op == Operator::Add &&
        (right.type == "i128" || right.type == "u128") &&
        !exprSrcType(node->left.get()).has_value() &&
        !ptrOperandSrcType(node->left.get()).has_value()) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 字符串拼接形态未登记（分派面：拼接128·缺形态：exprSrcType "
            "左侧无值）");
        lastExpr_ = ir::IRValue::constant("假", "i1");  // 占位（中止前布尔假）
        return true;
    }
    return false;
}

// ==================== 728o：连接链融合（366 遗留·a+b+c+… 一趟构建） ====================

// 链操作数「是字符串值」判定（融合判定的保守面）：语义 = 「该表达式的 IR 值
//   为 ptr 且按 C 串内容语义参与连接」。在 isStringTypedExpr 之上两处收窄/扩展：
//   ① BinaryExpr(Add) 链节点——递归其左操作数（下钻到最左叶子）；
//   ② CallExpr 返回字符串（chainCallReturnsString）——链首段为
//      `整数转字符串(x) + "-" + …` 等调用返回串形态（原 isStringTypedExpr
//      不认 CallExpr，此类最常见链首段形态不融合=性能覆盖大打折扣）。
//   收窄（与 isStringTypedExpr 的差异·关键）：数组元素仅认「字符串」元素
//   ——「字符」数组元素 isStringTypedExpr 认之（服务右操作数拼接面：`"x" + 表[i]`
//   按码点转字符/整数），但其 IR 值为 i32 而非 ptr；若作链左端点融合会把 i32
//   当 ptr 追加（错值）。此类形态一律不融合（交回原路径/语义层既有处置=零
//   行为变更）。保守性：判定 miss 一律 false（不融合 -> 原路径），无错值面。
bool IRGenerator::chainStringOperand(Expr* node) const {
    if (node == nullptr) return false;
    if (node->getType() == NodeType::BinaryExpr) {
        BinaryExpr* bin = static_cast<BinaryExpr*>(node);
        if (bin->op == Operator::Add) {
            return chainStringOperand(bin->left.get());
        }
        return false;
    }
    if (node->getType() == NodeType::CallExpr) {
        return chainCallReturnsString(static_cast<CallExpr*>(node));
    }
    if (node->getType() == NodeType::IndexExpr) {
        IndexExpr* idx = static_cast<IndexExpr*>(node);
        if (idx->object != nullptr &&
            idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string st = types::canonical(lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name).value_or(""));
            if (types::isArray(st)) {
                return types::canonicalEquals(types::arrayElemOf(st), "字符串");
            }
        }
        return false;
    }
    return isStringTypedExpr(node);
}

// 调用返回类型「字符串」判定（645 口径：返回类型字符串=拥有契约——A2 语义层
//   写回 retOwnedString ∪ 094 白名单 isOwnedStringBuiltin ∪ 返回类型查表
//   （决议签名键 -> 裸名两级；用户函数 / 模块限定内置））。
//   本函数只回答「内容语义是不是字符串」（融合/链段类型判定），**不回答拥有性**
//   ——方法调用面拥有性判据见 callReturnsOwnedStringTemp（#1572：类型判据≠
//   所有权，方法可合法返回借用视图，见该函数注释）。
//   边界声明（#648 精神·判据 miss 面）：查不到返回类型的调用=保守返回 false
//   （不判字符串 -> 不融合）——方向安全（漏报=慢，无错值/误释放面）。
bool IRGenerator::chainCallReturnsString(const CallExpr* node) const {
    if (node == nullptr) return false;
    if (node->retOwnedString) return true;  // A2 契约（语义层写回·全调用路径）
    if (node->callee == nullptr ||
        node->callee->getType() != NodeType::IdentifierExpr) {
        return false;
    }
    const std::string name =
        static_cast<const IdentifierExpr*>(node->callee.get())->name;
    if (isOwnedStringBuiltin(name)) return true;  // 094 单点白名单
    // 1578（#1578·2026-10-30 伞批·#1571 同族）：**闭包/functor 变量被调**——
    //   被调名不是已知函数（下方 funcReturnTypeOf 两键均 miss），而是声明为
    //   函数指针的变量（lambda 闭包变量/回调形参）时，原判据 miss=段拥有性
    //   漏判：融合/保守路径都按借用段处理，返回串追加后不结算=每段净泄 1
    //   （1517 形二值/形二引/形二写 活差 2 应为 1 实录·v2 同源）。判定=变量源
    //   类型的函数指针返回段**精确**等于 "字符串"（拥有 A2 契约）；"字符串&"
    //   （引用返回借用视图）与标量返回不命中——'&' 不剥，借用句柄不得 free。
    const std::string calleeSrcType =
        types::canonical(lookupSrcType(name).value_or(std::string()));
    if (types::isFuncPtr(calleeSrcType)) {
        const cn::Optional<std::string> fpRet =
            types::funcPtrReturnTypeOf(calleeSrcType);
        // 无值=非函数指针形态的合法否定（isFuncPtr 命中即必有返回段）
        if (fpRet.has_value() && fpRet.value() == "字符串") return true;
    }
    if (semantic_ != nullptr) {
        const std::string keys[2] = {node->resolvedSignature, name};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            // 1360 结果化批3B：无值=查询 miss——value_or 显形空串回落
            const std::string rt = types::canonical(
                semantic_->funcReturnTypeOf(key).value_or(std::string()));
            if (!rt.empty()) return rt == "字符串";
        }
    }
    return false;
}

// ==================== #1572：调用返回串「拥有性」按返回表达式形态判定 ====================
// 为什么不能直接用 retOwnedString：A2 契约位由语义层按「返回类型=字符串 ∧ 非
//   引用返回 ∧ 非泛型实例」写回，**方法面不区分返回表达式形态**——方法合法返回
//   借用视图（`返回 名;` 字段、`返回 表.元素(i);` 容器元素、`返回 局部借用句柄;`）
//   时该位同样置位。按该位释放临时=释放借用句柄=源对象悬垂/越界（#1696 三处扩面
//   于 859 JSON `文本(节)`（返回 文本们.元素(节) 借用视图）崩后被整体回退之实录）。
//   自由函数面语义层已按 210 负测族拒绝借用返回（参数/成员/下标/字符*/静态），
//   故自由函数/内置维持「类型=字符串即拥有」判据（现状零变更）；**方法面**改按
//   返回表达式形态判定（本函数族）。
// 判定谱系（拥有=堆串所有权随返回移交调用方）：
//   字面量 -> 拥有（驻留串非 tracked 分配，释放空安全无副作用）
//   二元 + -> 拥有（连接/拼接产物）
//   三元   -> 两分支皆拥有才拥有（运行时二选一）
//   调用   -> 自由函数/内置=类型判据（上）；转移=显式句柄交接；方法=递归形态判定
//   标识符 -> 本体内有同名变量声明且各声明初值皆拥有才拥有（局部移出）；
//            无形参/字段裸名/全局静态命中=借用（写穿别名）
//   其余（成员读/下标读/解引用/自增等）-> 借用视图
// 实现取向：分类函数=布尔两态正常分流（否定=借用面·安全方向「漏放=泄漏非错值」），
//   非「查表 miss 取默认值续译」——故以互斥穷举（switch 全列 + default=未列形态）
//   与布尔累积表达，不做逐点诊断（与 v2 返回表达式串拥有判定 同构）。
namespace {

// 体内全部 return 表达式收集（语句树全形态：块/分支/三类循环/选择分支）——
//   任一 return 借用即整体按借用处理（多出口方法不得只判首个 return）。
//   嵌套 lambda 体是表达式非语句，不下钻（其 return 属另一函数）。
void collectReturnExprsOwned(const Stmt* s, std::vector<const Expr*>& out) {
    if (s == nullptr) return;   // 空语句=该形态无体（递归出口）
    CN_SWITCH_ENUM_EXEMPT_BEGIN
    // 存量清偿挂#2074：漏 case 靠 default 挡存量（枚举穷举基线·逐处显式补 case 后删包裹）
    switch (s->getType()) {
        case NodeType::ReturnStmt:
            out.push_back(static_cast<const ReturnStmt*>(s)->value.get());
            return;
        case NodeType::BlockStmt:
            for (const auto& st : static_cast<const BlockStmt*>(s)->statements) {
                collectReturnExprsOwned(st.get(), out);
            }
            return;
        case NodeType::IfStmt: {
            const auto* ifs = static_cast<const IfStmt*>(s);
            collectReturnExprsOwned(ifs->thenBranch.get(), out);
            collectReturnExprsOwned(ifs->elseBranch.get(), out);
            return;
        }
        case NodeType::WhileStmt:
            collectReturnExprsOwned(static_cast<const WhileStmt*>(s)->body.get(), out);
            return;
        case NodeType::ForStmt:
            collectReturnExprsOwned(static_cast<const ForStmt*>(s)->body.get(), out);
            return;
        case NodeType::RangeForStmt:
            collectReturnExprsOwned(static_cast<const RangeForStmt*>(s)->body.get(), out);
            return;
        case NodeType::SwitchStmt: {
            const auto* sw = static_cast<const SwitchStmt*>(s);
            for (const auto& c : sw->cases) {
                if (c == nullptr) continue;
                for (const auto& st : c->statements) collectReturnExprsOwned(st.get(), out);
            }
            if (sw->defaultCase != nullptr) {
                for (const auto& st : sw->defaultCase->statements) {
                    collectReturnExprsOwned(st.get(), out);
                }
            }
            return;
        }
        default:
            return;
    }
    CN_SWITCH_ENUM_EXEMPT_END
}

// 体内同名变量声明初值收集（任意嵌套深度）：命中数=声明个数（形参/字段/全局
//   不是本体内 变量声明 语句，天然不命中=借用面）。
void collectVarInitExprsOwned(const Stmt* s, const std::string& name,
                              std::vector<const Expr*>& out) {
    if (s == nullptr) return;
    CN_SWITCH_ENUM_EXEMPT_BEGIN
    // 存量清偿挂#2074：漏 case 靠 default 挡存量（枚举穷举基线·逐处显式补 case 后删包裹）
    switch (s->getType()) {
        case NodeType::VarDecl: {
            const auto* vd = static_cast<const VarDecl*>(s);
            if (vd->name == name) out.push_back(vd->initializer.get());
            return;
        }
        case NodeType::BlockStmt:
            for (const auto& st : static_cast<const BlockStmt*>(s)->statements) {
                collectVarInitExprsOwned(st.get(), name, out);
            }
            return;
        case NodeType::IfStmt: {
            const auto* ifs = static_cast<const IfStmt*>(s);
            collectVarInitExprsOwned(ifs->thenBranch.get(), name, out);
            collectVarInitExprsOwned(ifs->elseBranch.get(), name, out);
            return;
        }
        case NodeType::WhileStmt:
            collectVarInitExprsOwned(static_cast<const WhileStmt*>(s)->body.get(), name, out);
            return;
        case NodeType::ForStmt: {
            const auto* fs = static_cast<const ForStmt*>(s);
            collectVarInitExprsOwned(fs->init.get(), name, out);
            collectVarInitExprsOwned(fs->body.get(), name, out);
            return;
        }
        case NodeType::RangeForStmt:
            collectVarInitExprsOwned(static_cast<const RangeForStmt*>(s)->body.get(), name, out);
            return;
        case NodeType::SwitchStmt: {
            const auto* sw = static_cast<const SwitchStmt*>(s);
            for (const auto& c : sw->cases) {
                if (c == nullptr) continue;
                for (const auto& st : c->statements) {
                    collectVarInitExprsOwned(st.get(), name, out);
                }
            }
            if (sw->defaultCase != nullptr) {
                for (const auto& st : sw->defaultCase->statements) {
                    collectVarInitExprsOwned(st.get(), name, out);
                }
            }
            return;
        }
        default:
            return;
    }
    CN_SWITCH_ENUM_EXEMPT_END
}

}  // namespace

// 单个返回表达式形态 -> 拥有性（递归；判定谱系见上方族注释）。
//   互斥穷举：case 全列合法拥有形态，default=未列形态（成员读/下标读/解引用/
//   赋值/自增等）→ 借用视图。空表达式（语句 `返回;`）=借用面。
//   #1578：captureNames=当前 lambda 捕获名集——捕获名在返回位已由 1571 自动
//   复制成拥有副本（捕获槽=只读快照/外层变量地址，所有权不能搬出），故命中
//   => 拥有；非 lambda 面（方法体）传空，捕获名天然不存在。
bool IRGenerator::stringReturnFormOwned(const Expr* e, const BlockStmt* body,
                                       const std::vector<std::string>& paramNames,
                                       int depth,
                                       const std::vector<std::string>* captureNames) const {
    const NodeType kind = (e != nullptr) ? e->getType() : NodeType::Program;
    CN_SWITCH_ENUM_EXEMPT_BEGIN
    // 存量清偿挂#2074：漏 case 靠 default 挡存量（枚举穷举基线·逐处显式补 case 后删包裹）
    switch (kind) {
        case NodeType::StringLiteral:
            return true;   // 驻留串（非 tracked·释放空安全无副作用）
        case NodeType::BinaryExpr:
            return static_cast<const BinaryExpr*>(e)->op == Operator::Add;  // 连接产物
        case NodeType::TernaryExpr: {
            const auto* te = static_cast<const TernaryExpr*>(e);
            return stringReturnFormOwned(te->trueValue.get(), body, paramNames, depth,
                                         captureNames) &&
                   stringReturnFormOwned(te->falseValue.get(), body, paramNames, depth,
                                         captureNames);
        }
        case NodeType::CallExpr: {
            const auto* ce = static_cast<const CallExpr*>(e);
            bool owned = false;
            if (ce != nullptr && ce->callee != nullptr) {
                const NodeType calleeKind = ce->callee->getType();
                if (calleeKind == NodeType::IdentifierExpr) {
                    const std::string nm =
                        static_cast<const IdentifierExpr*>(ce->callee.get())->name;
                    // 转移=显式句柄交接（源槽已清零）；其余自由函数/内置=类型判据
                    owned = (nm == "转移") || callReturnsStringType(ce);
                } else if (calleeKind == NodeType::MemberExpr) {
                    owned = callReturnsOwnedStringTemp(ce, depth - 1);   // 方法面递归
                }
            }
            return owned;   // 间接/方法值等被调形态=借用面（无返回形态可判）
        }
        case NodeType::IdentifierExpr: {
            const std::string& n = static_cast<const IdentifierExpr*>(e)->name;
            std::vector<const Expr*> inits;
            if (body != nullptr) collectVarInitExprsOwned(body, n, inits);
            // #1578：lambda 捕获名=返回位自动复制成拥有副本（1571 visitReturnStmt
            //   同点·值捕获/引用捕获同判）——**无同名局部声明遮蔽**时方能作数
            //   （体内重声明同名变量=返回的是该局部，按局部移出判据处置）。
            if (inits.empty() && captureNames != nullptr) {
                for (const auto& c : *captureNames) {
                    if (c == n) return true;
                }
            }
            bool paramHit = false;
            for (const auto& p : paramNames) {
                if (p == n) paramHit = true;   // 形参=写穿别名（210 已被语义拒·防御留）
            }
            bool initsOwned = !inits.empty();   // 无同名声明=字段裸名/全局静态=借用
            for (const Expr* init : inits) {
                if (!stringReturnFormOwned(init, body, paramNames, depth - 1,
                                           captureNames)) {
                    initsOwned = false;   // 任一同名声明初值借用=局部可能持借用句柄
                }
            }
            return !paramHit && initsOwned;   // 局部变量=返回位移出（返回块豁免析构）
        }
        default:
            return false;   // 成员读/下标读/解引用/自增等=借用视图
    }
    CN_SWITCH_ENUM_EXEMPT_END
}

// 方法体/lambda 体返回位拥有性：全部 return 皆拥有形态才=拥有（无体/无 return=借用面）。
bool IRGenerator::stringReturnBodyOwned(const BlockStmt* body,
                                        const std::vector<std::string>& paramNames,
                                        int depth,
                                        const std::vector<std::string>* captureNames) const {
    std::vector<const Expr*> rets;
    if (body != nullptr) collectReturnExprsOwned(body, rets);
    bool allOwned = !rets.empty();   // 无 返回 语句=非字符串返回形态（借用面）
    for (const Expr* rv : rets) {
        if (!stringReturnFormOwned(rv, body, paramNames, depth, captureNames)) allOwned = false;
    }
    return allOwned;
}

// 调用返回串拥有性（判据单点·临时清理/链段结算共用）。
//   调用形态四路互斥穷举（#1572 方法面 + #1578 变量面）：
//   ① 被调=成员（`对象.方法()`）：非引用返回 ∧ 递归预算未耗尽；接收者静态类型
//      （自身=当前发射类、标识符=语义层写回类型）→ 方法 AST 返回形态判定；
//      其余接收者（表达式/链式）/类表 miss/无体 = 借用面。
//   ② 被调=闭包变量（lambda）：**定义处登记的体返回形态**（ClosureInfo::
//      returnBodyOwned）。为什么不用返回类型：lambda 体可合法返回借用句柄
//      （形参透传——lambda 体 A2 尚未执法；探针实测按类型释放后源串变为空串
//      =#1696 同族误释面）；捕获名返回已由 1571 返回位复制成拥有副本（判拥有）。
//   ③ 被调=lambda 留痕但闭包登记不可达（跨函数/来源重赋值）：借用面（宁漏勿崩）。
//   ④ 被调=函数指针变量/自由函数/内置：返回类型判据（自由函数 A2 已拒借用
//      返回→类型即所有权；函数指针变量=声明返回段精确"字符串"，'&' 不剥）。
bool IRGenerator::callReturnsOwnedStringTemp(const CallExpr* ce, int depth) const {
    // 形态守卫（谓词否定回答面·非失败通道）：空节点/无被调者=非调用形态——此处
    //   不设早退（静默形态禁令），以 callShape 合取把非调用形态交末尾统一 return。
    const bool callShape = ce != nullptr && ce->callee != nullptr;
    const NodeType calleeKind =
        callShape ? ce->callee->getType() : NodeType::Program;
    // ---- ②③④ 变量被调面（#1578）----
    if (callShape && calleeKind == NodeType::IdentifierExpr) {
        const std::string nm =
            static_cast<const IdentifierExpr*>(ce->callee.get())->name;
        const auto closureIt = closureInfo_.find(nm);
        if (closureIt != closureInfo_.end()) {
            // ②闭包变量：体返回形态（仅返回类型=字符串 时该项才有串临时面）
            return closureIt->second.returnBodyOwned &&
                   types::canonicalEquals(
                       types::canonical(closureIt->second.srcReturnType), "字符串");
        }
        // ③lambda 留痕但快径登记已亡（485 clear / 重赋值来源未知）——无体可判
        //   =借用面；此处不得退到类型判据（借用返回按类型释放=误释崩面）
        if (lambdaVarNames_.count(nm) != 0) return false;
        // ④函数指针变量：声明返回段精确 "字符串"（引用返回 "字符串&" 不命中）
        const std::string calleeSrcType =
            types::canonical(lookupSrcType(nm).value_or(std::string()));
        if (types::isFuncPtr(calleeSrcType)) {
            const cn::Optional<std::string> fpRet =
                types::funcPtrReturnTypeOf(calleeSrcType);
            // 无值=非函数指针形态的合法否定（isFuncPtr 命中即必有返回段）
            if (fpRet.has_value()) return fpRet.value() == "字符串";
        }
    }
    // ①方法调用：返回表达式形态判定（借用视图返回不结算·859 实录口径）
    if (callShape && calleeKind == NodeType::MemberExpr) {
        const bool methodForm = semantic_ != nullptr && !ce->isRefReturnCall &&
                                depth != 0;
        const MemberExpr* mem =
            methodForm ? static_cast<const MemberExpr*>(ce->callee.get()) : nullptr;
        const Expr* recv = (mem != nullptr) ? mem->object.get() : nullptr;
        std::string objType;
        if (recv != nullptr && recv->getType() == NodeType::SelfExpr) {
            objType = types::canonical(currentClass_);
        } else if (recv != nullptr && recv->getType() == NodeType::IdentifierExpr) {
            objType = types::canonical(recv->semanticType);
        }
        const bool classForm = mem != nullptr && !objType.empty() &&
                           objType.find('$') == std::string::npos &&
                           semantic_->isClassType(objType);
        std::string owner;
        const ClassMemberInfo* m =
            classForm ? semantic_->lookupClassMember(objType, mem->memberName, owner)
                         .value_or(nullptr)
                   : nullptr;
        const bool resolvable = m != nullptr && m->ast != nullptr &&
                            m->ast->body != nullptr && !m->isRefReturn &&
                            types::canonicalEquals(types::canonical(m->type), "字符串");
        bool owned = false;
        if (resolvable) {
            std::vector<std::string> pn;
            for (const auto& p : m->ast->params) {
                if (p != nullptr) pn.push_back(p->name);
            }
            owned = stringReturnBodyOwned(m->ast->body.get(), pn, depth);
        }
        return owned;
    }
    // 自由函数/内置面：A2 契约位（语义层写回）∪ 返回类型判据；非调用形态=false
    return callShape && (ce->retOwnedString || callReturnsStringType(ce));
}

// 调用返回类型「字符串」判定（类型判据单点：resolvedType → 094 白名单 →
//   决议签名键/裸名两级查表）。自由函数/内置面拥有性判据与形态判定共用本函数
//   （#1572：自由函数借用返回已被语义层 210 族拒绝→类型判据即拥有判据）。
bool IRGenerator::callReturnsStringType(const CallExpr* ce) const {
    std::string retCanon =
        (ce != nullptr) ? types::canonical(ce->resolvedType) : std::string();
    const bool calleeIsIdent = ce != nullptr && ce->callee != nullptr &&
                          ce->callee->getType() == NodeType::IdentifierExpr;
    const std::string nm =
        calleeIsIdent ? static_cast<const IdentifierExpr*>(ce->callee.get())->name
                   : std::string();
    if (calleeIsIdent && retCanon.empty()) {
        if (isOwnedStringBuiltin(nm)) retCanon = "字符串";   // 094 单点白名单
        if (retCanon.empty() && semantic_ != nullptr) {
            const std::string keys[2] = {ce->resolvedSignature, nm};
            for (const std::string& key : keys) {
                if (key.empty()) continue;
                // 1360 结果化批3B：无值=查询 miss——value_or 显形空串回落
                const std::string rt = types::canonical(
                    semantic_->funcReturnTypeOf(key).value_or(std::string()));
                if (!rt.empty()) { retCanon = rt; break; }
            }
        }
    }
    return retCanon == "字符串";
}

// 段「拥有临时」判定（617 口径·融合段追加完成后的结算依据）：
//   调用返回拥有串（拥有性判据单点 callReturnsOwnedStringTemp：自由函数/内置=
//   类型判据，方法=返回形态判定，lambda/functor 变量=闭包体返回形态/函数指针
//   返回段——#1572/#1578）；
//   连接子链结果（BinaryExpr Add 且左为字符串）-> 新拥有串（concat/融合完成产物）；
//   其余（字面量=驻留借用、标识符=调用方变量 RAII 自负、下标/成员=视图）-> 非拥有。
//   结算时机=追加完成后（append 已拷贝内容·与 617「concat 只读消费后结算」同点）。
bool IRGenerator::chainSegOwnedTemp(Expr* segAst) const {
    if (segAst == nullptr) return false;
    if (segAst->getType() == NodeType::CallExpr) {
        return callReturnsOwnedStringTemp(static_cast<CallExpr*>(segAst));
    }
    if (segAst->getType() == NodeType::BinaryExpr) {
        BinaryExpr* bin = static_cast<BinaryExpr*>(segAst);
        return bin->op == Operator::Add && chainStringOperand(bin->left.get());
    }
    return false;
}


// 判断 AST 表达式是否字符串语义类型（Task 2.9 拼接判定 / 962 比较判定共用）。
//   字符串字面量 / 字符串变量（字符串|字符*）→ true；连接结果沿左递归。
//   933（先拆再加·自 ir_expr.cpp 机械搬迁＋调用形态覆盖）：原定义在 ir_expr.cpp
//   （该文件 903 行超 900 警戒线），随「调用返回字符串」覆盖面补齐迁至本文件
//   （连接族判据消费主文件）。新增 CallExpr 分支=962 立法（字符串 ==/!= 内容
//   比较）在「调用返回串」形态的判定缺口：`拼后(x) == "名后"`（值返回）与
//   `b.读名() == "盒名"`（引用返回 字符串&）原落指针地址比较（同内容异地址
//   恒假·933 探针实录）；同时覆盖「调用返回串 + 数值」拼接判定（左串侧）。
//   类型来源=exprSrcType 统一推导（985 面：自由函数 funcReturnTypeOf ∪ 方法
//   调用 lookupClassMember），剥尾 '&'（引用返回后缀·右值读值后类型）后判定
//   （'*' 不剥——`字符*` 是字符串视图类型本体）。
//   1925（成员/解引用形态补齐·零分叉红线）：原判定只认 字面量/标识符/连接链/
//   数组元素/调用 五形态——**成员访问（结构体字段/类字段/结果载荷）与解引用
//   读值不在判据面** → 整值指针比较：`甲卡.内容 != "甲"`（字段存堆副本·
//   与字面量异地址）恒判不等（001 §比较语义要求内容比较·探针 9001b/9002/9004/
//   9006 实录），且 `字段 + 数值` 被误发射为指针算术（语义层已按拼接分派=
//   双侧分叉）。现改为「已知形态显式判 + 其余统一回落 exprSrcType 单点推导」
//   （成员访问→字段类型[含结构体/类/结果载荷/泛型物化]/三元/自身/单目重载
//   结果全覆盖）；解引用读值（*p）单列——类型=操作数所指（pointerPointeeSrcType
//   单点·指针/数组退化同口径）。
//   诚实边界：exprSrcType 对「对象为表达式/链式」的方法调用与未知形态返回
//   无值 → false（保守不判字符串；主形态=标识符对象/直接调用/字段访问）。
bool IRGenerator::isStringTypedExpr(Expr* node) const {
    if (node == nullptr) return false;
    CN_SWITCH_ENUM_EXEMPT_BEGIN
    // 存量清偿挂#2074：漏 case 靠 default 挡存量（枚举穷举基线·逐处显式补 case 后删包裹）
    switch (node->getType()) {
        case NodeType::StringLiteral:
            return true;
        case NodeType::IdentifierExpr: {
            const std::string st = types::canonical(lookupSrcType(
                static_cast<IdentifierExpr*>(node)->name).value_or(""));
            if (st == "字符串" || st == "字符*") {
                return true;
            }
            // 1960（云端预验第 4 轮·1357 误伤实证·2026-11-03）：顶层常量字符串
            //   引用（`常量 S = "AB"; … S == "AB"`）——语义层 visitIdentifierExpr
            //   按 checkConstIdentifier 折叠 lastType_=字符串（值文本内联发射·
            //   584-a 第 4 层），但 IR 层 lookupSrcType 变量表与语义层常量折叠表
            //   不同源（常量名 miss）→ 标识符分支假 → exprSrcType 同 miss →
            //   1960 比较面显形拦死合法程序。补回退：常量值文本（globalConstValue
            //   ·值文本形态=带引号字面量）引号形态→字符串语义（值文本以
            //   ConstString 内联·与字面量同 IR 形态——比较分派面 __cn_str_eq
            //   内容比较正确）；非常量/非引号形态=原 miss 不变（真未登记形态
            //   维持显形·不放宽）。
            if (semantic_ != nullptr) {
                const std::string constText = semantic_->globalConstValue(
                    static_cast<IdentifierExpr*>(node)->name).value_or(std::string());
                if (constText.size() >= 2 &&
                    (constText.front() == '"' || constText.front() == '\'')) {
                    return true;
                }
            }
            return false;
        }
    CN_SWITCH_ENUM_EXEMPT_END
        case NodeType::BinaryExpr: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(node);
            if (bin->op == Operator::Add) {
                // 连接/拼接结果：左字符串（含数值拼接）即结果字符串
                return isStringTypedExpr(bin->left.get());
            }
            break;  // 其余二元（运算符重载结果等）→ 统一回落 exprSrcType
        }
        case NodeType::IndexExpr: {
            // 数组元素：元素类型为字符串/字符（字符串[i] -> 字符 仍可参与拼接）
            IndexExpr* idx = static_cast<IndexExpr*>(node);
            if (idx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = types::canonical(lookupSrcType(
                    static_cast<IdentifierExpr*>(idx->object.get())->name).value_or(""));
                if (types::isArray(st)) {
                    const std::string elem = types::canonical(types::arrayElemOf(st));
                    return (elem == "字符串" || elem == "字符");
                }
            }
            break;  // 指针下标/嵌套下标/容器元素等 → 统一回落 exprSrcType
        }
        case NodeType::UnaryExpr: {
            const UnaryExpr* un = static_cast<const UnaryExpr*>(node);
            if (un->op == Operator::Deref) {
                // 解引用读值（*p）：类型=操作数所指（指针剥星/数组退化单点）
                return isStringSemanticSrcText(
                    pointerPointeeSrcType(un->operand.get()));
            }
            break;  // 其余单目（运算符重载结果）→ 统一回落 exprSrcType
        }
        default:
            break;  // 成员访问/三元/自身等 → 统一回落 exprSrcType
    }
    // 统一回落：exprSrcType 单点推导（成员访问字段/结果载荷/三元/调用返回/
    //   运算符重载结果等）；无法推导=无值 → false（保守不判字符串）。
    return isStringSemanticSrcText(exprSrcType(node));
}

// 段值 -> 字符串值（融合段追加前转换单点入口）：
//   ptr（字符串/字符*）直通；i128/u128 取低 64 位后 from_int（族③同口径）；
//   其余数值/字符/布尔走 emitConcatNumToStr（族②共享转换器）。
//   返回的转换产物=拥有串（完成即被追加消费·调用方随后释放）。
ir::IRValue IRGenerator::emitChainSegToStr(Expr* segAst, const ir::IRValue& segVal,
                                           const SourceLocation& loc) {
    if (segVal.type == "ptr") return segVal;
    if (segVal.type == "i128" || segVal.type == "u128") {
        ir::IRValue lo = emitResult(ir::Opcode::Cast, {segVal}, "i64", "", loc);
        return emitResult(ir::Opcode::Call, {lo}, "ptr", "__cn_str_from_int", loc);
    }
    return emitConcatNumToStr(segAst, segVal, loc);
}

// 单段追加发射：追加 + 拥有临时结算（字符串段）或转换产物释放（数值段）
//   ——内容已入缓冲（append 内拷贝），此后的释放对终串零影响。
//   662（617 三元面补全）：三元段走分支拥有性感知条件释放（emitOwnedTemp-
//   ReleaseRec617·选中字面量/标识符借用分支不得 free）；其余形态维持
//   chainSegOwnedTemp 原判据（零回归面）。
void IRGenerator::emitChainSegAppend(const ir::IRValue& handle, Expr* segAst,
                                     const ir::IRValue& segVal,
                                     const SourceLocation& loc) {
    if (segVal.type == "ptr") {
        emit(ir::Opcode::Call, {handle, segVal}, ir::IRValue(),
             "__cn_strbuf_append", "void", loc);
        if (segAst != nullptr &&
            segAst->getType() == NodeType::TernaryExpr) {
            emitOwnedTempReleaseRec(segAst, segVal, loc);
        } else if (chainSegOwnedTemp(segAst)) {
            emit(ir::Opcode::Call, {segVal}, ir::IRValue(), "__cn_str_free",
                 "void", loc);
        }
        return;
    }
    ir::IRValue segStr = emitChainSegToStr(segAst, segVal, loc);
    emit(ir::Opcode::Call, {handle, segStr}, ir::IRValue(),
         "__cn_strbuf_append", "void", loc);
    emit(ir::Opcode::Call, {segStr}, ir::IRValue(), "__cn_str_free", "void", loc);
}

// 连接链融合入口（visitBinaryExpr 顶部调用·须在左右操作数生成之前接管整链）。
//   true = 已按融合路径发射（lastExpr_ 已设置）；false = 交回原逐节路径。
//   判定与展开（保守·语义等价铁律）：
//   ① 运算符 + 且左操作数静态判定为字符串（连接节点）——与语义层「字符串+
//      字符串 / 字符串+数值隐式拼接」同边界；左非字符串（数值/指针/类实例
//      运算符重载）一律不融合；
//   ② 左结合链展平为段序列（左到右）：链节点=Add 且其左操作数为字符串；
//      遇到右嵌套/括号子链作为「段」原样保留（其自身递归生成时独立判定融合）
//      ——不假设链的右结合形态，防链外形态误改写；
//   ③ 段数 >= kMinConcatChainSegs 才融合（链长 2 原路径已最优）。
//   发射序列（镜像原逐节链·见文件头语义等价铁律②）：
//     new -> [段0求值, 段1求值, append(段0), append(段1)]
//         -> 段 i(>=2): 求值 -> append -> finish
//   即：段 0/1 的内容读取时点=原内层 concat 的执行时点（段 2 求值之前）；
//   段 i>=2 的读取时点=原第 i 次 concat 的执行时点（段 i+1 求值之前）。
//   段求值次数与左到右次序与原逐节链逐一相同（副作用的顺序与次数不变）。
bool IRGenerator::genStringConcatChain(BinaryExpr* node) {
    if (node == nullptr || node->op != Operator::Add) return false;
    if (!chainStringOperand(node->left.get())) return false;
    // ---- 展平左结合链（迭代；段序=左到右）----
    std::vector<Expr*> segs;
    std::vector<BinaryExpr*> spine;  // 链节点（根 -> 最深）
    {
        BinaryExpr* cur = node;
        while (true) {
            spine.push_back(cur);
            Expr* leftExpr = cur->left.get();
            if (leftExpr != nullptr &&
                leftExpr->getType() == NodeType::BinaryExpr) {
                BinaryExpr* inner = static_cast<BinaryExpr*>(leftExpr);
                if (inner->op == Operator::Add &&
                    chainStringOperand(inner->left.get())) {
                    cur = inner;
                    continue;
                }
            }
            segs.push_back(leftExpr);  // 首段（链最左操作数）
            break;
        }
        for (auto it = spine.rbegin(); it != spine.rend(); ++it) {
            segs.push_back((*it)->right.get());
        }
    }
    if (segs.size() < kMinConcatChainSegs) return false;
    const SourceLocation& loc = node->location;
    // 句柄（整64 不透明·001 §10.3）；错误路径（new 失败=nullptr）经 runtime
    //   append/完成 的空安全通道自然退化（不泄漏·不崩——366 弃置契约同源）。
    ir::IRValue handle = emitResult(ir::Opcode::Call, {}, "i64",
                                    "__cn_strbuf_new", loc);
    // 前两段先行求值（内容读取时点镜像：原 concat_1 于段 2 求值前执行）
    ir::IRValue v0 = genExpr(segs[0]);
    ir::IRValue v1 = genExpr(segs[1]);
    if (v0.type != "ptr") {
        // 判定与 IR 不一致=编译器内部缺陷（字符串类型恒映射 ptr）——#648
        //   miss 不可静默：显形诊断（driver 收尾中止·产物不落地），随后仍按
        //   数值段转换发射保控制流不崩（原静默路径无对应行为可回退）。
        diagnostics_.report(
            DiagnosticLevel::Error, loc,
            "内部错误: 连接链首段 IR 类型非字符串（'" + v0.type +
                "'）——静默续编已废除（#728o）");
    }
    emitChainSegAppend(handle, segs[0], v0, loc);
    emitChainSegAppend(handle, segs[1], v1, loc);
    for (std::size_t i = 2; i < segs.size(); ++i) {
        ir::IRValue vi = genExpr(segs[i]);
        emitChainSegAppend(handle, segs[i], vi, loc);
    }
    // 完成：出串=新拥有串（cn_alloc_tracked·与 __cn_str_concat 同所有权——
    //   赋值/返回/打印等消费位所有权契约零变化）
    lastExpr_ = emitResult(ir::Opcode::Call, {handle}, "ptr",
                           "__cn_strbuf_finish", loc);
    return true;
}

} // namespace cn_compiler
