// CN-IR生成器——阶段3 OOP 调用/删除 指令发射（Task 3.1/3.2，串联集成子任务）
// 职责：
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>


#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_string_temp_util.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 74-a：容器元素所有权（入容器位归一化） ====================
// 语义依据（plans/019 §2 设计哲学 + Rust Vec<String> 对照）：

// 入容器位方法名（stdlib/容器.cn 中「把值交给容器」的全部入口）
bool IRGenerator::isContainerInsertMethod(const std::string& name) {
    return name == "追加" || name == "插入" || name == "设置" ||
           name == "压入" || name == "入队" || name == "头部追加" ||
           name == "添加";
}

// 值实参下标：插入(位置,值)/设置(位置,值) 的值在 1；其余（追加/压入/入队/
//   头部追加/添加）值在 0。1360 结果化：无值=非入容器位方法（原 -1 哨兵退役）。
cn::Optional<int> IRGenerator::containerInsertValueArgIndex(const std::string& name) {
    if (name == "插入" || name == "设置") return 1;
    if (isContainerInsertMethod(name)) return 0;
    return std::nullopt;
}

bool IRGenerator::isStringElemContainer(const std::string& canonClass) {
    // 77-a（第七十七轮）：判定上提 types:: 共享（语义层借出视图生命周期检查
    //   同一口径），此处委托调用——原实现与完整注释见 semantic/type_system.cpp
    //   （含 76-a 集合扩面、嵌套形态严格口径两处沿革）。
    return types::isStringElemContainer(canonClass);
}

// 76-a：字符串值映射判定（映射$K$字符串）——入容器位归一化用（映射值在实参下标1）。
//   释放面由 ir_oop.cpp 的映射分支处理（__cn_map_free_strings/_slot——键/值两数组
bool IRGenerator::isStringValuedMap(const std::string& canonClass) {
    // 77-a：同 上提 types::（原实现与沿革注释见 semantic/type_system.cpp）。
    return types::isStringValuedMap(canonClass);
}

// 容器元素数组字段名：向量/栈=数据（平铺数组）；链表/队列=值表（槽+下一索引链）
// 1360 结果化：无值=非容器（原空串哨兵退役）。
cn::Optional<std::string> IRGenerator::containerElemArrayField(const std::string& canonClass) {
    const std::size_t dl = canonClass.find('$');
    if (dl == std::string::npos) return std::nullopt;
    const std::string head = canonClass.substr(0, dl);
    if (head == "向量" || head == "栈") return "数据";
    if (head == "链表" || head == "队列") return "值表";
    if (head == "集合") return "数据数组";   // 76-a（元素独立平铺数组）
    return std::nullopt;
}

// 容器元素串释放辅助函数名（按容器模型分派）——1360 结果化：非字符串元素
//   容器=无值（原空串哨兵退役）。
cn::Optional<std::string> IRGenerator::containerElemFreeFn(const std::string& canonClass) const {
    if (!isStringElemContainer(canonClass)) return std::nullopt;
    const std::string head = canonClass.substr(0, canonClass.find('$'));
    if (head == "向量" || head == "栈") return "__cn_vector_free_strings";
    if (head == "集合") return "__cn_vector_free_strings";   // 76-a 平铺同款
    return "__cn_chain_free_strings";   // 链表/队列（链游释放，见运行时注释）
}

// 发射容器元素串释放（三处释放路径共用单点事实源）
//   平铺模型（向量/栈）：__cn_vector_free_strings(obj, 数据偏移, 元素数量偏移)
void IRGenerator::emitContainerElemFreeFor(const std::string& canonClass,
                                          const ir::IRValue& objPtr,
                                          const SourceLocation& loc) {
    // 1360 结果化：无值=非字符串元素容器（has_value 分流）
    const cn::Optional<std::string> freeFn = containerElemFreeFn(canonClass);
    if (!freeFn.has_value() || semantic_ == nullptr) return;
    const std::string head = canonClass.substr(0, canonClass.find('$'));
    const bool chainModel = (head == "链表" || head == "队列");
    const int dataOff =
        semantic_->classFieldOffset(canonClass, containerElemArrayField(canonClass).value_or(""))
            .value_or(-1);
    const int countOff = semantic_->classFieldOffset(canonClass, "元素数量").value_or(-1);
    if (dataOff < 0 || countOff < 0) return;
    std::vector<ir::IRValue> args{objPtr};
    if (chainModel) {
        const int nextOff = semantic_->classFieldOffset(canonClass, "下一索引").value_or(-1);
        const int headOff = semantic_->classFieldOffset(canonClass, "头索引").value_or(-1);
        if (nextOff < 0 || headOff < 0) return;
        args.push_back(ir::IRValue::constant(std::to_string(dataOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(nextOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(headOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(countOff), "整64"));
    } else {
        args.push_back(ir::IRValue::constant(std::to_string(dataOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(countOff), "整64"));
    }
    emit(ir::Opcode::Call, args, ir::IRValue(), freeFn.value(), "void", loc);
}

// 槽是否为本函数拥有串局部（真 move 判据）：登记在拥有名单且未被污染
//   （污染名=借用视图，所有权不归本函数——转移只能复制不能移交）。
bool IRGenerator::isOwnedStringSlot(const std::string& unique,
                                    const std::string& srcName) const {
    if (unique.empty()) return false;
    if (stringTainted_.count(srcName) > 0) return false;
    return std::find(ownedStringOrder_.begin(), ownedStringOrder_.end(), unique) !=
           ownedStringOrder_.end();
}

// 入容器位实参所有权归一化（方案A 核心）——按来源分级：
//   ① 字面量               → 驻留借用（只读段常量，释放侧空安全，零复制开销）
ir::IRValue IRGenerator::normalizeStringValueSource(Expr* arg,
                                                    const ir::IRValue& value,
                                                    const SourceLocation& loc) {
    if (arg == nullptr) return value;
    const NodeType kind = arg->getType();
    // ① 字面量：驻留借用（进程生命周期，释放对其空安全）
    if (kind == NodeType::StringLiteral) return value;
    // ②/③/④ 调用形态：转移 特判 + 拥有契约
    if (kind == NodeType::CallExpr) {
        CallExpr* ce = static_cast<CallExpr*>(arg);
        if (isTransferCall(ce) && !ce->arguments.empty() &&
            ce->arguments[0]->getType() == NodeType::IdentifierExpr) {
            const std::string srcName =
                static_cast<IdentifierExpr*>(ce->arguments[0].get())->name;
            // 1360 结果化：value_or("") 显形（miss=非拥有槽判定假·原语义）
            const std::string srcUnique = lookupVarName(srcName).value_or("");
            if (isOwnedStringSlot(srcUnique, srcName)) {
                // ③ 真 move：源槽清零（源 RAII 出口 free(nullptr) 空安全）
                ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                              loc);
                emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique, "ptr",
                     loc);
                return value;
            }
            // ④ 借用来源：复制（安全方向——借用不能移交所有权）
            return emitResult(ir::Opcode::Call, {value}, "ptr", "__cn_str_copy",
                              loc);
        }
        // ② 拥有返回（A2 契约：签名即契约）→ 直接接管；借用返回 → 复制
        bool ownRet = ce->retOwnedString;
        if (!ownRet && ce->callee->getType() == NodeType::IdentifierExpr) {
            const std::string& cn =
                static_cast<IdentifierExpr*>(ce->callee.get())->name;
            ownRet = cn == "字符串复制" || cn == "字符串连接" ||
                     cn == "字符串拼接" || cn == "字符串子串" ||
                     cn == "字符串大写" || cn == "字符串小写" ||
                     cn == "字符串修剪" || cn == "字符串反转";
        }
        if (ownRet) return value;
        return emitResult(ir::Opcode::Call, {value}, "ptr", "__cn_str_copy", loc);
    }
    // ⑥ 二元连接（+）产物=新堆串（拥有契约·无人别名）——直存接管（不复制）。
    //   861（732 同口径·零分叉纪律）：判据=**定义点溯源** isConcatResultInst
    //   （寄存器在块内倒序首条定义=__cn_str_concat/__cn_strbuf_finish——645/
    //   663/732/758 链单一事实源，比较/指针算术/转换临时/变量 load 定义非该
    //   符号集天然排除；v2 发射拥有串赋值②「节点_二元运算」同款语义）。原落
    //   ⑤ 一律复制=拼接产物线程孤儿（每写入净泄 1 个中间串：探针 p9② 复合
    //   赋值百轮泄 100 实录）。**不用 AST 类型判据**：方法内裸字段名不入
    //   变量类型表（isStringTypedExpr 对 `字段 + 串` 左操作数盲判 false——
    //   732 判据在本面不复核），IR 溯源对字段/变量/成员各形态统一命中。
    if (value.type == "ptr" && isConcatResultInst(currentBlock_, value.id)) {
        return value;
    }
    // ⑤ 标识符（拥有局部/形参/静态）与借出视图（成员/下标/解引用）——一律复制：
    //   拥有局部=值语义深拷贝（源照常拥有并释放）；借用来源=容器独立拥有
    //   （源由他人持有，容器不得与之共享句柄）
    return emitResult(ir::Opcode::Call, {value}, "ptr", "__cn_str_copy", loc);
}

// 入容器位实参归一化（求值 + 分级）——旧入口保持（求值一次后委托分级助手）
ir::IRValue IRGenerator::normalizeContainerInsertArg(Expr* arg,
                                                     const SourceLocation& loc) {
    if (arg == nullptr) return ir::IRValue();
    // 966：转移实参入容器位（表.追加(转移(甲))）——元素深拷（stdlib 容器
    //   方法体 数据[n] = 值 写入深拷·运行期）完成后源句柄=孤儿壳：转移展开
    //   已清源槽（句柄交接语义·459-a）+容器元素=深拷独立拥有（630）→源无
    //   人持有必须释放（v2 净 0 对齐面·探针斜率 泄1壳/调用 实录）。登记
    //   pendingArgCleanups（用户类）→ArgCleanupGuard 在追加调用发射完成后
    //   flush DeleteObject 级联全清（时序=元素已拷毕·安全）。转移源=标识符
    //   主形态；成员源（959 形态 h.桶）/结构体值语义源不在本面（挂账）。
    if (arg->getType() == NodeType::CallExpr &&
        isTransferCall(static_cast<const CallExpr*>(arg)) &&
        semantic_ != nullptr) {
        Expr* xferSrc = static_cast<CallExpr*>(arg)->arguments[0].get();
        if (xferSrc != nullptr && xferSrc->getType() == NodeType::IdentifierExpr) {
            const std::string srcT = types::canonical(
                lookupSrcType(static_cast<IdentifierExpr*>(xferSrc)->name)
                    .value_or(""));
            // 218（云端预验红实录）：求值**恒一次**提分支外——分支内先行求值+
            //   串源不命中类面再落分支外二次 genExprForOop=转移展开两遍（第一
            //   遍清源槽·第二遍读 0=元素空串·bit4 丢实录）。
            const ir::IRValue v = genExprForOop(arg);
            if (semantic_->isClassType(srcT)) {
                pendingArgCleanups_.emplace_back(v, "966壳$" + srcT, nullptr);
                // 966：类源直接返回句柄——不得落 normalizeStringValueSource
                //   （串归一对非串槽源走 __cn_str_copy 借用复制=对类句柄做
                //   串拷贝·错误码 3 实录）。
                return v;
            }
            return normalizeStringValueSource(arg, v, loc);
        }
    }
    const ir::IRValue v = genExprForOop(arg);
    return normalizeStringValueSource(arg, v, loc);
}

// 方法调用实参构建：入容器位（对象类型=字符串元素容器 或 字符串值映射 且 方法∈
//   入容器位）时对值实参做所有权归一化，其余实参/其余调用等价 buildCallArgsOop。
std::vector<ir::IRValue> IRGenerator::buildCallArgsForMethod(
    CallExpr* node, const std::string& canonObj,
    const std::string& methodName) {
    std::vector<ir::IRValue> out;
    // 966 返工②：入容器位逐参两分——串容器（ownsStrElem）=串面归一化原域
    //   （76-a 原判据·normalizeStringValueSource 对类句柄走 __cn_str_copy=毁·
    //   不得扩域）；非串容器=类面 966 独立入口（只对 转移(类源) 登记释放·
    //   其余 genExprForOop 原样零变化）——向量<货品>.追加(转移(甲)) 原 走
    //   buildCallArgsOop 旁路=966 登记永不可达（斜率 3→151 不变实录）。
    const bool ownsStrElem = isStringElemContainer(canonObj) ||
                             isStringValuedMap(canonObj);
    // 1360 结果化：无值=非入容器位（has_value 分流·原 -1 哨兵判定显形）
    const cn::Optional<int> valueIdxOpt = containerInsertValueArgIndex(methodName);
    if (!valueIdxOpt.has_value() ||
        static_cast<std::size_t>(valueIdxOpt.value()) >= node->arguments.size()) {
        return buildCallArgsOop(node->arguments, node->location);
    }
    const std::size_t vi = static_cast<std::size_t>(valueIdxOpt.value());
    // 81-a：结构体元素容器（含拥有型串字段的结构体）**不在此归一化**——元素槽
    //   字段串的独立性由 stdlib 容器方法体的 `数据[n] = 值` 结构体写入深拷（79-a
    for (std::size_t i = 0; i < node->arguments.size(); ++i) {
        // 966 返工②显式三分（else 新规：业务分支禁 else 兜底——三区间闭式枚举）
        const bool valStrFace = (i == vi) && ownsStrElem;
        const bool valClassFace = (i == vi) && !ownsStrElem;
        if (valStrFace) {
            out.push_back(normalizeContainerInsertArg(node->arguments[i].get(),
                                                      node->location));
        }
        if (valClassFace) {
            out.push_back(
                genTransferClassArgWithCleanup966(node->arguments[i].get(),
                                                  node->location));
        }
        if (i != vi) {
            out.push_back(genArgValueWithCleanup(node->arguments[i].get(),
                                                 node->location));
        }
    }
    return out;
}

// 966：非串容器的入容器位类面入口——只对 转移(类标识符源) 登记源壳释放
//   （pendingArgCleanups "966壳$" 打标·flush 侧 662 同款整壳析构）；其余
//   形态（临时/拷贝/非类）genExprForOop 原样零变化。
// 331-a+#1888：方法/构造/接口/闭包/间接调用面实参 ABI 定标（i128 宽化+浮32
//   提升）单一归属=calibrateCallArgsByParams·定义在 ir_call_args.cpp 实参族
//   （先拆再加：本文件超 900 警戒线·#1888 新增浮32 面随迁；原 widenI128Args）。

// ==================== 类方法调用/构造调用 ====================

// 类调用处理（visitCallExpr 钩子）：
//   返回 true 表示已处理（lastExpr_ 已设置），false 表示非类调用（交回原路径）
bool IRGenerator::handleClassCallExpr(CallExpr* node) {
    if (semantic_ == nullptr) return false;
    if (node->callee->getType() != NodeType::MemberExpr &&
        node->callee->getType() != NodeType::IdentifierExpr) {
        return false;
    }

    // ---- P3-19：接口对象方法调用（图形.方法(实参)）——B1 全局槽位运行时分派 ----
    if (tryInterfaceMethodCall(node)) return true;

    // ---- 情形A：构造调用 类名(实参) ----
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        return emitConstructorCall(node);
    }

    // ---- 情形B：成员方法调用 对象.方法(实参) / 类名.静态方法(实参) / 父类.方法(实参) ----
    return emitMemberMethodCall(node);
}


// 情形A 构造调用族（原 handleClassCallExpr 情形A 段迁移）：
//   泛型实例名解析 -> findClass -> NewObject -> 构造查找 -> 构造 Call 发射。
bool IRGenerator::emitConstructorCall(CallExpr* node) {
    std::string className =
        static_cast<IdentifierExpr*>(node->callee.get())->name;
    className = resolveGenericCtorInstanceName(className);
        const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
        if (ci == nullptr) return false;
        // 抽象类不可实例化（语义层已报错，防御跳过）
        if (ci->isAbstract) return false;
        // 生成 NewObject：extra = "类名|大小字节"
        const std::string extra = className + "|" + std::to_string(ci->totalSize);
        ir::IRValue obj = emitResult(ir::Opcode::NewObject,
                                     {ir::IRValue::constant(className, "ptr")},
                                     "ptr", extra, node->location);

    // 构造函数查找（isConstructor 成员；无构造函数 -> 默认构造（仅分配））：
    // 1360 结果化：无值=无显式构造（has_value 分流）
    const cn::Optional<const ClassMemberInfo*> ctorOpt =
        findCtorMember(ci, className, node);
    if (ctorOpt.has_value()) {
        emitCtorInvoke(node, className, ctorOpt.value(), obj);
    } else {
        // 117：类自身无构造的实例化点——NewObject 零初始化后基类构造链仍须跑
        //   （C++ 隐式默认构造语义；B1 形态：子类不写构造时基类构造原被整跳）。
        //   有自有构造的类走 emitClassMethod 序言注入，不经此分支。
        emitBaseCtorChain(className, obj, node->location);
        // #283 甲案（2026-10-08）：无构造类实例化点字段级联——「仅分配」路径
        //   自此对类类型字段级联默认构造（001 §3.9 条文·p1007_02 容器字段
        //   悬空空指针根治；与 860-a 用户构造序言共用单字段发射）
        cascadeConstructFields(className, obj, node->location);
    }
    lastExpr_ = obj;
    return true;
}

// 泛型实例化构造 盒子<整32>(42) 的实例名映射（原情形A 内联段迁移）：
//   名<实参> -> 类名$实参1$实参2（语义层 instantiateGeneric mangling 对齐）；
std::string IRGenerator::resolveGenericCtorInstanceName(std::string className) const {
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化构造 盒子<整32>(42)——callee
    //   为 名<实参>（IdentifierExpr 名字含 <），语义层已单态化注册实例化类
    //   （盒子$整32），此处把 名<实参> 映射到实例化类符号名。
    const std::size_t genLt = className.find('<');
    // 2026-08-25 H3：平衡扫描找配对 '>'（嵌套泛型 向量<映射<整64,整64>> 的
    //   inner 若用 rfind 最后 > 会缺内层闭合，实例名含 '<' -> findClass 失败
    //   -> 构造调用回退普通 Call（无 NewObject/this）-> 运行段错误）
    std::size_t genGt = std::string::npos;
    if (genLt != std::string::npos) {
        int depth = 0;
        for (std::size_t i = genLt; i < className.size(); ++i) {
            if (className[i] == '<') depth++;
            else if (className[i] == '>') {
                depth--;
                if (depth == 0) { genGt = i; break; }
            }
        }
    }
    if (genLt != std::string::npos && genGt != std::string::npos &&
        genGt > genLt) {
        const std::string head = className.substr(0, genLt);
        const std::string inner =
            className.substr(genLt + 1, genGt - genLt - 1);
        // 平衡逗号分割（嵌套内层 < 中 , 非外层分隔）
        std::vector<std::string> args;
        std::size_t pos = 0;
        int angleDepth = 0;
        std::size_t segStart = 0;
        while (pos <= inner.size()) {
            if (pos == inner.size() ||
                (inner[pos] == ',' && angleDepth == 0)) {
                std::string seg = inner.substr(segStart, pos - segStart);
                // 318-a（T19①）：泛型函数体内构造 向量<T>()——callee AST 保留
                //   <T> 原文（语义 recheck 的重写只改 className 引用不改
                seg = substGenericType(seg);
                args.push_back(seg);
                segStart = pos + 1;
                if (pos == inner.size()) break;
            } else if (inner[pos] == '<') {
                angleDepth++;
            } else if (inner[pos] == '>') {
                angleDepth--;
            }
            pos++;
        }
        for (auto& a : args) {
            const std::size_t b = a.find_first_not_of(" \t");
            const std::size_t e = a.find_last_not_of(" \t");
            if (b != std::string::npos && e != std::string::npos) {
                a = a.substr(b, e - b + 1);
            }
            // H3：嵌套实参（含 '<'）转为实例化名（映射<整64,整64> -> 映射$整64$整64）
            if (a.find('<') != std::string::npos) {
                std::string ninst = a.substr(0, a.find('<'));
                std::string ninner = a.substr(a.find('<') + 1, a.rfind('>') - a.find('<') - 1);
                std::size_t npos = 0;
                while (npos <= ninner.size()) {
                    const std::size_t ncomma = ninner.find(',', npos);
                    if (ncomma == std::string::npos) { ninst += "$" + ninner.substr(npos); break; }
                    ninst += "$" + ninner.substr(npos, ncomma - npos);
                    npos = ncomma + 1;
                }
                a = ninst;
            }
            // 061-d（2026-09-27 804 轮）：合成模板实参（结果<T,E>/可选<T>）统一
            //   合成体名 $ 形态（与语义层 instantiateGeneric 注册名一致——原裸拼
            a = canonicalizeSyntheticArgText(a);
        }
                    std::string inst = head;
        for (const auto& a : args) {
            inst += "$" + types::canonical(a);
        }
        // 语义层实例化类符号名 = 类名$实参（instantiateGeneric mangling）
        if (semantic_->findClass(inst).has_value()) className = inst;
    }
    return className;
}

// 构造函数查找（原情形A 内联段迁移）——以下策略注释为原文：
        // 查找构造函数（isConstructor 成员）；无构造函数 -> 默认构造（仅分配）。
        // 1360 结果化：无值=未找到（原 nullptr 哨兵退役·内部指针状态机不变）。
cn::Optional<const ClassMemberInfo*> IRGenerator::findCtorMember(const ClassInfo* ci,
                                                   const std::string& className,
                                                   CallExpr* node) {
    const ClassMemberInfo* ctor = nullptr;
    // Debug 子任务修复（构造函数重载）：优先用语义层记录的选中构造
    //   （node->resolvedSignature = 类名$构造sigKey，visitCallExpr 已按实参匹配），
    //   精确对应 无参/带参 重载；未记录时遍历 methods 按 实参个数 匹配兜底。
    if (!node->resolvedSignature.empty()) {
        const std::size_t ds = node->resolvedSignature.find('$');
        if (ds != std::string::npos) {
            const std::string wantSig = node->resolvedSignature.substr(ds + 1);
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.sigKey == wantSig &&
                    mk.second.ownerClass == className) {
                    ctor = &mk.second;
                    break;
                }
            }
        }
    }
    if (ctor == nullptr) {
        // 兜底：按 实参个数 匹配本类构造（与语义层一致的 ownerClass 限定）
        //   933 防御纵深：跳过「引用形参 × 实参不可寻址」组合——语义层未记录
        const std::size_t givenArgs = node->arguments.size();
        const ClassMemberInfo* fallback = nullptr;
        for (const auto& mk : ci->methods) {
            if (mk.second.isConstructor && mk.second.hasBody &&
                mk.second.ownerClass == className &&
                mk.second.paramTypes.size() == givenArgs) {
                bool refMismatch = false;
                for (std::size_t i = 0; i < mk.second.paramTypes.size(); ++i) {
                    if (types::isReference(mk.second.paramTypes[i]) &&
                        node->arguments[i]->getType() != NodeType::IdentifierExpr &&
                        node->arguments[i]->getType() != NodeType::MemberExpr &&
                        node->arguments[i]->getType() != NodeType::IndexExpr) {
                        refMismatch = true;
                        break;
                    }
                }
                if (refMismatch) continue;
                fallback = &mk.second;
                break;
            }
        }
        if (fallback == nullptr) {
            // 648 miss 不可静默：语义层选中构造（resolvedSignature 精确层）与
            //   实参个数匹配层两级全 miss，落到「取第一个构造函数」强凑终态——
            //   重载/缺省实参形态下可能绑错构造（静默错构造初始化）。报诊断后
            //   保留取第一个兜底与控制流（driver generate() 后 hasErrors() 中止）；
            //   循环无命中（类无显式构造=默认构造合法面）返回 nullptr，不报。
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.hasBody &&
                    mk.second.ownerClass == className) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "内部错误: 构造函数重载决议失败 '" + className + "'（#648）");
                    fallback = &mk.second;
                    break;
                }
            }
        }
        ctor = fallback;
    }
    if (ctor == nullptr) return std::nullopt;
    return ctor;
}

// 构造 Call 发射（原情形A 内联段迁移）：实参 = [obj(this)] + 实参（i128 宽化+
//   缺省实参补全 D23 机制原文随迁）。
// （D1 拆分 456-a：本函数自 handleClassCallExpr 整段迁移，逻辑逐字保留·零行为变更）
void IRGenerator::emitCtorInvoke(CallExpr* node, const std::string& className,
                                 const ClassMemberInfo* ctor, const ir::IRValue& obj) {
        // 构造体 Call：符号 = 类名$构造sigKey，实参 = [obj(this)] + 实参
        //   （构造实参不是容器元素所有权入口——不做入容器位归一化）
        // 775（甲案·2026-10-15 用户裁决）：本类层字段默认值注入——构造体
        //   （Call）执行前写入，构造体内显式赋值随后覆盖（C++ NSDMI 语义）。
        emitClassFieldDefaults(className, className, obj, node->location);
        std::vector<ir::IRValue> args;
        args.push_back(obj);  // this（对象指针）
        std::vector<ir::IRValue> userArgs =
            buildCallArgsOop(node->arguments, node->location);
        // 316-a（C23/T45 甲）：i128/u128 构造形参的窄整实参定标——与 ir_call
        //   直调路径同款（类构造字面实参 ABI 契约分叉 m45_03）。
        // 442（判据统一·421 审查 P2-2/P2-3）：内联副本归并 单一归属
        //   calibrateCallArgsByParams——原副本三微差（无 466 引用形参守卫/排除浮点实参/
        //   判据文本与直调路径分叉）随归并消除。
        calibrateCallArgsByParams(userArgs, ctor->paramTypes, node->location);
        for (auto& a : userArgs) args.push_back(a);
        // D23 根治（248-a）：构造缺省实参补全——构造调用经 handleClassCallExpr
        //   提前展开（visitCallExpr 通用补缺段不可达），此处按语义层选中的
        if (!node->resolvedSignature.empty()) {
            auto defIt = funcDefaultArgs_.find(node->resolvedSignature);
            if (defIt != funcDefaultArgs_.end()) {
                auto totIt = funcDefaultTotal_.find(node->resolvedSignature);
                std::size_t totalParams = node->arguments.size();
                if (totIt != funcDefaultTotal_.end()) {
                    totalParams = totIt->second;
                }
                const std::size_t given = node->arguments.size();
                if (given < totalParams &&
                    totalParams - given <= defIt->second.size()) {
                    const std::size_t missing = totalParams - given;
                    const auto& defaults = defIt->second;
                    for (std::size_t k = defaults.size() - missing;
                         k < defaults.size(); ++k) {
                        // 1150：默认值物化为寄存器（与显式实参同通道）
                        args.push_back(materializeDefaultArg(defaults[k],
                                                            node->location));
                    }
                }
            }
        }
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(className, ctor->sigKey), "void", node->location);
}

// 情形B 成员方法调用族（原 handleClassCallExpr 情形B 段迁移）：
//   静态方法（类名.静态方法）三态分派 + 实例方法（虚/非虚）。
// （D1 拆分 456-a：本函数自 handleClassCallExpr 整段迁移，逻辑逐字保留·零行为变更）
bool IRGenerator::emitMemberMethodCall(CallExpr* node) {
    MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
    const std::string methodName = mem->memberName;
    // 1360 结果化：value_or("") 显形（无法推导=canonical("") miss 链·原语义）// 622：支持 (*类指针).方法()
    const std::string objSrcType = memberObjectSrcType(mem->object.get()).value_or("");
    const std::string canonObj = types::canonical(objSrcType);

    const int stat = emitStaticMethodCall(node, mem, methodName);
    if (stat >= 0) return stat == 1;  // 1=已处理；0=交回原路径
    return emitInstanceMethodCall(node, mem, methodName, canonObj);
}

// 类名.静态方法族（原情形B 静态段迁移）：
//   返回 1=已处理；0=交回原路径（非方法/非静态——原 return false 语义不变）；
int IRGenerator::emitStaticMethodCall(CallExpr* node, MemberExpr* mem,
                                      const std::string& methodName) {
    // 类名.静态方法：对象标识符本身是类类型名
    std::string staticClassName;
    if (mem->object->getType() == NodeType::IdentifierExpr) {
        const std::string objName =
            static_cast<IdentifierExpr*>(mem->object.get())->name;
        if (semantic_->isClassType(objName)) staticClassName = objName;
    }
    if (!staticClassName.empty()) {
        std::string owner;
        // 1360 结果化：无值=非方法（has_value 分流）
        const cn::Optional<const ClassMemberInfo*> mOpt =
            findClassMethod(staticClassName, methodName, owner);
        if (!mOpt.has_value()) return 0;  // 非方法（静态字段等，交回原路径）
        const ClassMemberInfo* m = mOpt.value();
        if (!m->isStatic) return 0;  // 语义层已报错，防御跳过
        std::vector<ir::IRValue> args =
            buildCallArgsOop(node->arguments, node->location);
        // 331-a（T53 家系·静态方法路径）：i128/u128 形参的窄整实参宽化——同族
        //   同修（实例方法/虚调用已接·静态方法原缺 → 字面量实参 i64 直传 →
        //   被调方按 i128 指针解引用 SIGSEGV·探针 p_static 实证）。
        normalizeMethodArgs(node, m->paramTypes, args);
        // 936（912 宿主对应面·1036 探针实锚 C0000005）：静态方法直调（类名.静态
        //   方法()/类名::静态方法()·两分隔符共用本 MemberExpr 通道）聚合返回
        //   （结构体/数组/结果<>合成结构体）——与虚调用 410/实例直调 Task 6.1/
        //   普通调用 087 同款形态A 预插 retbuf 隐藏返回指针（参数位 0=rcx）。
        //   原缺预插：被调方 epilogue 把垃圾 rcx 当 retbuf rep movsb 野写
        //   （C0000005 非确定崩·读侧 rax 亦为垃圾）、调用方无缓冲可拷——宿主
        //   为唯一红侧（v2 侧 912 同族已修·1036_v2 PASS），产物曾首跑假绿复跑
        //   红（1036 双编译实录）。
        if (semantic_ != nullptr && !m->type.empty() && !m->isRefReturn &&
            (semantic_->isStructType(types::canonical(m->type)) ||
             types::isArray(m->type))) {
            ir::IRValue buf = allocAggRetbufBuf(m->type, node->location);
            std::vector<ir::IRValue> hiddenArgs;
            hiddenArgs.push_back(buf);  // 形态A：retbuf（位 0=rcx）
            for (auto& a : args) hiddenArgs.push_back(a);
            emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(),
                 methodSymbolKey(owner, m->sigKey), "void", node->location);
            lastExpr_ = buf;
            return 1;
        }
        // 118（929）：引用返回方法（-> T&）——resultType=ptr（被引用左值地址）
        const std::string resultType = m->isRefReturn
                                           ? "ptr"
                                           : mapType(m->type.empty() ? "空类型" : m->type);
        // 静态方法返回 空类型 时用 emit（不分配结果寄存器）
        if (resultType == "void" || resultType.empty()) {
            emit(ir::Opcode::Call, args, ir::IRValue(),
                 methodSymbolKey(owner, m->sigKey), "void", node->location);
            lastExpr_ = ir::IRValue();
        } else {
            lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                                   methodSymbolKey(owner, m->sigKey), node->location);
            // 118（929）：引用返回读值默认 lvalue-to-rvalue（与直接函数调用
            //   ir_call P3-18 同构）；赋值目标/复合赋值/引用绑定语境经
            if (m->isRefReturn && !suppressRefDeref_ && !refReturnAggInline(m)) {
                lastExpr_ = emitResult(ir::Opcode::LoadPtr, {lastExpr_},
                                       mapType(m->type.empty() ? "空类型" : m->type),
                                       "", node->location);
            }
        }
        return 1;
    }
    return -1;  // 非静态方法形态——fall-through 实例方法路径（原控制流）
}

// 实例方法调用族（原情形B 实例段迁移）：对象类类型解析 -> 方法查找 ->
//   虚表槽位判定 -> 虚调用（VirtualCall）/非虚直接 Call（含结构体隐藏返回指针）。
// （D1 拆分 456-a：本函数自 handleClassCallExpr 整段迁移，逻辑逐字保留·零行为变更）
bool IRGenerator::emitInstanceMethodCall(CallExpr* node, MemberExpr* mem,
                                         const std::string& methodName,
                                         const std::string& canonObj) {
    // 实例方法调用：对象为类实例（源码类型是类）。
    // v2.1 统一 .：对象源码类型为 类名*（指针）时剥指针取类名（与语义层
    std::string canonObjForMethod = canonObj;
    if (!semantic_->isClassType(canonObjForMethod) &&
        types::isPointer(canonObjForMethod)) {
        canonObjForMethod = types::canonical(types::pointeeOf(canonObjForMethod));
    }
    if (!semantic_->isClassType(canonObjForMethod)) {
        return false;
    }
    // 查方法成员（沿继承链；owner=声明类）
    std::string owner;
    // 1360 结果化：无值=非方法（has_value 分流）
    const cn::Optional<const ClassMemberInfo*> mOpt =
        findClassMethod(canonObjForMethod, methodName, owner);
    if (!mOpt.has_value()) return false;  // 非方法（字段访问等，交回原路径）
    const ClassMemberInfo* m = mOpt.value();
    if (m->isStatic) return false;   // 实例.静态方法 语义允许，但走静态路径（防御）

    // this 实参：自身/父类 -> this 指针；类变量 -> 变量值（对象指针）
    // 宿主根治（2026-09-01）：顶层静态对象方法调用（全局表.大小()）不再特判——
    ir::IRValue thisArg;
    bool elemMutable794 = false;
    if (mem->object->getType() == NodeType::CallExpr) {
        CallExpr* elemCall794 = static_cast<CallExpr*>(mem->object.get());
        if (elemCall794->callee->getType() == NodeType::MemberExpr) {
            MemberExpr* elemMem794 =
                static_cast<MemberExpr*>(elemCall794->callee.get());
            const std::string elemName794 = elemMem794->memberName;
            const bool isMutable794 =
                methodName == "设置" || methodName == "删除" ||
                methodName == "清空" || methodName == "插入" ||
                methodName == "弹出";
            if (isMutable794 && elemName794 == "元素" &&
                elemCall794->arguments.size() == 1) {
                ir::IRValue contShell794 =
                    genExpr(elemMem794->object.get());
                ir::IRValue faddr794 = emitResult(
                    ir::Opcode::FieldAddr, {contShell794}, "ptr", "0",
                    node->location);
                ir::IRValue dataPtr794 = emitResult(
                    ir::Opcode::LoadPtr, {faddr794}, "ptr", "", node->location);
                ir::IRValue idxVal794 = genExpr(elemCall794->arguments[0].get());
                const std::string elemCanon794 =
                    types::canonical(elemMem794->semanticType.empty()
                                         ? elemMem794->semanticType
                                         : elemMem794->semanticType);
                const int elemSize794 =
                    semantic_->typeSizeOf(types::canonical(canonObjForMethod)).value_or(8);
                const std::int64_t stride794 =
                    elemSize794 > 0 ? elemSize794 : 8;
                // 任务 119（927）：ConstInt 值文本必须走 extra（与全库其他发射点
                //   同构的规范形态）。原写法把值放 operands[0]（IRValue::constant）
                ir::IRValue strideConst794 = emitResult(
                    ir::Opcode::ConstInt, {}, "i64",
                    std::to_string(stride794), node->location);
                ir::IRValue scaled794 = emitResult(
                    ir::Opcode::Mul, {idxVal794, strideConst794}, "i64", "",
                    node->location);
                ir::IRValue elemAddr794 = emitResult(
                    ir::Opcode::Add, {dataPtr794, scaled794}, "ptr", "",
                    node->location);
                thisArg = elemAddr794;
                elemMutable794 = true;
            }
        }
    }
    if (!elemMutable794) {
        // 622（2026-10-14）：this 生成走 classMemberBasePtr 统一助手——显式解引用
        //   对象（(*类指针).方法()）this=指针值本身；原 genExpr(*p) 对 Deref 多一层
        //   LoadPtr（读对象首字段当 this）→ 被调方 this 错位段错误（m 探针 rc=139）。
        thisArg = classMemberBasePtr(mem->object.get());
    }

    // ---- 虚调用：方法在虚表中有槽位（虚拟 或 重写，vtableIndex>=0）且非 父类. 限定调用 ----
    // 重写方法 isVirtual=false 但 vtableIndex>=0（覆盖父类槽位），同样须虚分派。
    const bool isSuperCall = (mem->object->getType() == NodeType::SuperExpr);
    const int vtableSlot =
        isSuperCall ? -1
                    : semantic_->classVtableIndex(canonObjForMethod, methodName).value_or(-1);
    if (vtableSlot >= 0) {
        emitVirtualMethodCall(node, m, owner, methodName, thisArg);
    } else {
        emitDirectMethodCall(node, m, owner, methodName, canonObjForMethod, thisArg);
    }
    // 662（方法链消费位·2026-10-16）：接收者=调用返回类对象「拥有形态」临时——
    //   方法调用发射完成后析构（判定+发射实现=ir_oop_call_release.cpp 释放族
    //   emitOwnedRecvTempDestruct662：构造新建/返回本地移出才析构·借用返回不析构）
    emitOwnedRecvTempDestruct662(mem->object.get(), thisArg, node->location);
    return true;
}

// 虚方法发射（原实例段虚调用分支迁移）：VirtualCall.extra = "类名.虚方法名"。
// （D1 拆分 456-a：本函数自 handleClassCallExpr 整段迁移，逻辑逐字保留·零行为变更）
void IRGenerator::emitVirtualMethodCall(CallExpr* node, const ClassMemberInfo* m,
                                        const std::string& owner,
                                        const std::string& methodName,
                                        const ir::IRValue& thisArg) {
    std::vector<ir::IRValue> args;
    args.push_back(thisArg);  // operand[0] = this
    std::vector<ir::IRValue> userArgs =
        buildCallArgsOop(node->arguments, node->location);
    // 331-a（T53 根治·同族同修）：虚调用路径同样须做 i128/u128 形参宽化
    //   （buildCallArgsOop 不感知形参类型；此处有 m->paramTypes）。
    normalizeMethodArgs(node, m->paramTypes, userArgs);
    for (auto& a : userArgs) args.push_back(a);
    // 410（虚调用 bigRet 形态A·emitDirectMethodCall Task 6.1 同构）：结构体/
    //   数组返回方法——IR 层预插 retbuf 地址为 operands[0]（this 退位 1），
    //   result.type=void，extra 加 "&" 前缀供后端识别（emitCall 的 struct 面
    //   即此形态A 契约；虚调用此前缺预插=被调方把返回值写进 this 对象内存=
    //   堆写穿·调用方从 [rax=this] 拷回仅值侥幸对）。i128 返回不预插（后端
    //   按 result.type 自建缓冲·emitCall hasBigRet 同款）。
    //   缓冲构造=allocAggRetbufBuf（936 提炼·公共单点）。
    if (semantic_ != nullptr && !m->type.empty() && !m->isRefReturn &&
        (semantic_->isStructType(types::canonical(m->type)) ||
         types::isArray(m->type))) {
        ir::IRValue buf = allocAggRetbufBuf(m->type, node->location);
        std::vector<ir::IRValue> hiddenArgs;
        hiddenArgs.push_back(buf);      // 形态A：retbuf（位 0）
        hiddenArgs.push_back(thisArg);  // this（位 1）
        for (auto& a : userArgs) hiddenArgs.push_back(a);
        emit(ir::Opcode::VirtualCall, hiddenArgs, ir::IRValue(),
             std::string("&") + owner + "." + methodName, "void", node->location);
        lastExpr_ = buf;
        return;
    }
    // 118（929）：引用返回方法（-> T&）——resultType=ptr（被引用左值地址）
    const std::string resultType = m->isRefReturn
                                       ? "ptr"
                                       : mapType(m->type.empty() ? "空类型" : m->type);
    const std::string extra = owner + "." + methodName;  // "类名.虚方法名"
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::VirtualCall, args, ir::IRValue(),
             extra, "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::VirtualCall, args, resultType,
                               extra, node->location);
        // 118（929）：引用返回读值默认 lvalue-to-rvalue（suppressRefDeref_
        //   抑制=赋值目标/绑定语境取地址——与直接函数调用同构）。
        if (m->isRefReturn && !suppressRefDeref_ && !refReturnAggInline(m)) {
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {lastExpr_},
                                   mapType(m->type.empty() ? "空类型" : m->type),
                                   "", node->location);
        }
    }
    return;
}

// 非虚方法/父类.方法 发射（原实例段非虚分支迁移）：直接 Call + 结构体返回
//   隐藏返回指针（Task 6.1 retbuf 机制原文随迁）。
// （D1 拆分 456-a：本函数自 handleClassCallExpr 整段迁移，逻辑逐字保留·零行为变更）
void IRGenerator::emitDirectMethodCall(CallExpr* node, const ClassMemberInfo* m,
                                       const std::string& owner,
                                       const std::string& methodName,
                                       const std::string& canonObjForMethod,
                                       const ir::IRValue& thisArg) {
    // ---- 非虚方法 / 父类.方法（直接 Call，非虚分派） ----
    std::vector<ir::IRValue> args;
    args.push_back(thisArg);  // this 为第一个实参（参数位 0）
    std::vector<ir::IRValue> userArgs =
        buildCallArgsForMethod(node, canonObjForMethod, methodName);
    // 331-a（T53 根治）：i128/u128 形参的窄整实参（字面量等）统一宽化——与构造
    //   路径同款（单一归属 calibrateCallArgsByParams）；原缺 → 按 i64 值直传 → 被调方按
    //   i128 指针解引用段错误（实弹 m53_01 容器 追加(100000000000)）。
    normalizeMethodArgs(node, m->paramTypes, userArgs);
    for (auto& a : userArgs) args.push_back(a);
    // 118（929）：引用返回方法（-> T&）——resultType=ptr（被引用左值地址）
    const std::string resultType = m->isRefReturn
                                       ? "ptr"
                                       : mapType(m->type.empty() ? "空类型" : m->type);
    // Task 6.1（容器库 追加/读取 返回 结果<空类型,整32> 合成结构体）：方法返回
    //   结构体时须走隐藏返回指针（与 visitCallExpr 普通函数 structReturn 一致）——
    //   缓冲构造=allocAggRetbufBuf（936 提炼·公共单点；原注释「返回缓冲区槽数按
    //   结构体实际大小 ceil(size/8)」的槽登记由 registerVarSlots 统一承担）。
    if (semantic_ != nullptr && !m->type.empty() && !m->isRefReturn &&
        (semantic_->isStructType(types::canonical(m->type)) ||
         types::isArray(m->type))) {
        ir::IRValue buf = allocAggRetbufBuf(m->type, node->location);
        std::vector<ir::IRValue> hiddenArgs;
        hiddenArgs.push_back(buf);   // 隐藏返回指针（参数位 0，rcx）
        hiddenArgs.push_back(thisArg);  // this（参数位 1，rdx）
        for (auto& a : userArgs) hiddenArgs.push_back(a);
        emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = buf;
        return;  // （结构体返回分支终止——D1 拆分 void 化，原 return true 由调用点 emitInstanceMethodCall 无条件承接）
    }
    // 符号：父类.方法() 用父类（owner）符号；普通调用用声明类（owner）符号
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                               methodSymbolKey(owner, m->sigKey), node->location);
        // 118（929）：引用返回读值默认 lvalue-to-rvalue（suppressRefDeref_
        //   抑制=赋值目标/绑定语境取地址——与直接函数调用同构）。
        if (m->isRefReturn && !suppressRefDeref_ && !refReturnAggInline(m)) {
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {lastExpr_},
                                   mapType(m->type.empty() ? "空类型" : m->type),
                                   "", node->location);
        }
    }
    return;
}


// ==================== 运算符重载族 → 936 拆分迁 ir_oop_op_overload.cpp ====================
// （D1 行数整改先例 114-a/456-a：纯搬迁零行为变更 + 聚合返回 retbuf 预插同族修复）


// 方法参数装载：this（自身）+ 显式参数进入 varStack_ 最外层作用域。
// 与 visitFunctionDecl 的参数处理一致（唯一内部名 name$N、数组/结构体多槽登记）。
void IRGenerator::setupMethodParams(ir::IRFunction& func, const ClassMemberInfo& mi) {
    varStack_.emplace_back();
    // this 指针（非静态方法）：参数名 自身（与语义层 declareVar("自身") 一致），
    // IR 类型 ptr（类实例地址）。codegen emitParamSetup 按 paramUniques 登记栈槽，
    // 参数位 0（rcx）——与 VirtualCall/构造调用 的 this 参数位契约一致。
    if (!mi.isStatic) {
        const std::string thisUnique = "自身$" + std::to_string(varCounter_++);
        func.params.emplace_back("自身", "ptr");
        func.paramUniques.push_back(thisUnique);
        registerVarSlots(thisUnique, "");
        ir::IRValue reg = newReg();
        reg.type = "ptr";
        VarEntry entry;
        entry.regId = reg.id;
        entry.uniqueName = thisUnique;
        entry.type = "ptr";
        entry.srcType = mi.ownerClass.empty() ? currentClass_ : mi.ownerClass;
        varStack_.back()["自身"] = entry;
    }
    // 显式参数（AST 节点取 ParamDecl 指针）
    const ClassMember* member = mi.ast;
    if (member == nullptr) return;
    for (std::size_t pi = 0; pi < member->params.size(); ++pi) {
        const ParamDecl* param = member->params[pi].get();
        // 缺陷3 修复（泛型单态化参数类型）：泛型方法体提升时，AST 参数的 typeName
        //   仍是原始类型参数（T/U），未替换为实参类型（整32/浮64）。mi.paramTypes
        //   已在 instantiateGeneric 中按实参替换（canonical 后），必须优先采用，
        //   否则 浮64/多类型参数 被 mapType(T) 误判为 ptr -> 参数位模式错乱（读 0/垃圾）。
        //   函数指针参数 mi.paramTypes 存 funcPtr.toString()，仍按 ptr 处理。
        std::string paramType =
            (pi < mi.paramTypes.size()) ? mi.paramTypes[pi] : param->typeName;
        // H6 根治（2026-08-25）：类方法引用参数（简单盒& / 盒子$整64& 其他）——
        //   原实现未设 byRef 标志，体内读取/成员访问把"被引用变量地址"当值用
        //   （源.值 读到对象指针而非字段值，实测 b: 2227986720736 而非 7）。
        //   与 visitFunctionDecl 引用参数（ir_decl.cpp）同规则：参数槽存"被引用
        //   左值地址"（8 字节），体内读取经 byRef 机制解引用（LoadPtr 得对象
        //   指针/基础值），类/结构体/标量引用参数统一按此语义。
        const bool isRefParam = !param->funcPtr.isFunctionPtr() &&
                                types::isReference(paramType);
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(paramType);
        if (isRefParam) paramIrType = "ptr";  // 引用参数按地址传递（槽存地址）
        // 087b 捎带（宿主方法面·ir_decl 087 isArrayParam 同构）：数组形参=按值
        //   聚合——paramIrType=ptr+structParamIndexes（056 通道 prologue rep
        //   movsb 拷贝）；原 mapType 塌缩元素类型 i32（597 方法面 法=垃圾值实录）
        const bool isArrayParam087b = !isRefParam &&
                                      !param->funcPtr.isFunctionPtr() &&
                                      types::isArray(paramType);
        if (isArrayParam087b) paramIrType = "ptr";
        registerVarSlots(unique, isRefParam ? ""
                            : (param->funcPtr.isFunctionPtr() ? "" : paramType));
        // 结构体按值参数标记：引用参数（账户&）按地址传递（非按值结构体拷贝），
        //   须排除（与 ir_decl.cpp 同规则）
        //   087b：数组按值参数同通道标记（emitParamSetup 拷贝判定同款）
        if (!isRefParam && semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
            (semantic_->isStructType(types::canonical(paramType)) ||
             isArrayParam087b)) {
            func.structParamIndexes.insert(static_cast<int>(pi) + (mi.isStatic ? 0 : 1));
        }
        func.params.emplace_back(param->name, paramIrType);
        func.paramUniques.push_back(unique);
        ir::IRValue reg = newReg();
        reg.type = paramIrType;
        VarEntry entry;
        entry.regId = reg.id;
        entry.uniqueName = unique;
        // 体内"值类型"仍为被引用基础类型/对象指针：读取经 byRef 解引用返回
        entry.type = isRefParam ? mapType(types::stripRef(paramType)) : reg.type;
        entry.srcType = paramType;
        entry.byRef = isRefParam;  // H6：引用参数按 byRef 语义读写
        varStack_.back()[param->name] = entry;
    }
}

} // namespace cn_compiler// 118（929）：引用返回方法的读值分叉判据（semantic 判定聚合内联）——