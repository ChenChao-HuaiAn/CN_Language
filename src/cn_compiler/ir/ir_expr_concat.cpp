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

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

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
        emitOwnedTempReleaseRec617(node->left.get(), left, node->location);
        emitOwnedTempReleaseRec617(node->right.get(), right, node->location);
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
        emitOwnedTempReleaseRec617(node->left.get(), left, node->location);
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
        emitOwnedTempReleaseRec617(node->left.get(), left, node->location);
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
//   边界声明（#648 精神·判据 miss 面）：查不到返回类型的调用=保守返回 false
//   （不判字符串 -> 不融合 / 不结算）——方向安全（漏报=慢或泄漏面归 A2 族，
//   无错值/误释放面）；成员方法调用形态（callee=MemberExpr）仅认语义层写回的
//   retOwnedString，未写回时保守不认。
bool IRGenerator::chainCallReturnsString(CallExpr* node) const {
    if (node == nullptr) return false;
    if (node->retOwnedString) return true;  // A2 契约（语义层写回·全调用路径）
    if (node->callee == nullptr ||
        node->callee->getType() != NodeType::IdentifierExpr) {
        return false;
    }
    const std::string name =
        static_cast<IdentifierExpr*>(node->callee.get())->name;
    if (isOwnedStringBuiltin(name)) return true;  // 094 单点白名单
    if (semantic_ != nullptr) {
        const std::string keys[2] = {node->resolvedSignature, name};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            const std::string rt =
                types::canonical(semantic_->funcReturnTypeOf(key));
            if (!rt.empty()) return rt == "字符串";
        }
    }
    return false;
}

// 段「拥有临时」判定（617 口径·融合段追加完成后的结算依据）：
//   CallExpr 返回字符串 -> 新拥有串（A2 契约）；
//   连接子链结果（BinaryExpr Add 且左为字符串）-> 新拥有串（concat/融合完成产物）；
//   其余（字面量=驻留借用、标识符=调用方变量 RAII 自负、下标/成员=视图）-> 非拥有。
//   结算时机=追加完成后（append 已拷贝内容·与 617「concat 只读消费后结算」同点）。
bool IRGenerator::chainSegOwnedTemp(Expr* segAst) const {
    if (segAst == nullptr) return false;
    if (segAst->getType() == NodeType::CallExpr) {
        return chainCallReturnsString(static_cast<CallExpr*>(segAst));
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
//   诚实边界：exprSrcType 对「对象为表达式/链式」的方法调用与未知形态返回
//   空串 → false（保守不判字符串·零行为变更；主形态=标识符对象/直接调用）。
bool IRGenerator::isStringTypedExpr(Expr* node) const {
    if (node == nullptr) return false;
    switch (node->getType()) {
        case NodeType::StringLiteral:
            return true;
        case NodeType::IdentifierExpr: {
            const std::string st = types::canonical(lookupSrcType(
                static_cast<IdentifierExpr*>(node)->name).value_or(""));
            return (st == "字符串" || st == "字符*");
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* bin = static_cast<BinaryExpr*>(node);
            if (bin->op == Operator::Add) {
                // 连接/拼接结果：左字符串（含数值拼接）即结果字符串
                return isStringTypedExpr(bin->left.get());
            }
            return false;
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
            return false;
        }
        case NodeType::CallExpr: {
            // 933：调用返回类型经 exprSrcType 推导（含引用返回），剥尾 '&' 判定
            // 1360 结果化：value_or("") 显形（无法推导=非字符串判定假·原语义）
            std::string ct = exprSrcType(node).value_or("");
            if (!ct.empty() && ct.back() == '&') ct.pop_back();
            const std::string ccanon = types::canonical(ct);
            return (ccanon == "字符串" || ccanon == "字符*");
        }
        default:
            return false;
    }
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
            emitOwnedTempReleaseRec617(segAst, segVal, loc);
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
