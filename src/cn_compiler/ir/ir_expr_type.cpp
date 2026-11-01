// CN-IR生成器——表达式源码类型推导族（728g 批红修复轮自 ir_oop.cpp 拆出：
//   该文件 1011 行超 §3 1000 硬红线，按「先拆再加」先拆本族再落
//   776 指针下标通道对齐修正——纯机械搬移零行为变更，成员函数实现
//   搬迁·声明仍在 ir_generator.hpp）。
//   族 = exprSrcType（对象.方法/对象.字段 的类类型判定与字段类型查询的
//   统一表达式源码类型推导入口）。
#include <string>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

namespace {
// 1360 结果化：出口单点转换——空串=未推导 → 无值（原空串哨兵不外泄）。
//   move 传入零拷贝；SSO 短类型串无堆分配（热路径开销可忽略）。
cn::Optional<std::string> optOfSrc(std::string s) {
    if (s.empty()) return std::nullopt;
    return cn::Optional<std::string>(std::move(s));
}
}  // namespace

// ==================== 表达式源码类型推导 ====================

// 推导表达式源码类型（供 对象.方法/对象.字段 的类类型判定与字段类型查询）：
//   标识符 -> 变量源码类型；自身 -> 当前类；父类 -> 当前类父类；
//   成员访问 -> 字段类型；1360 结果化：其余/无法推导=无值（原空串哨兵退役，
//   各 return 出口经 optOfSrc 单点转换）。
cn::Optional<std::string> IRGenerator::exprSrcType(Expr* node) const {
    if (node == nullptr) return std::nullopt;
    switch (node->getType()) {
        case NodeType::IdentifierExpr: {
            const std::string name = static_cast<IdentifierExpr*>(node)->name;
            const cn::Optional<std::string> st = lookupSrcType(name);
            if (st.has_value() && !st.value().empty()) return st.value();
            if (semantic_ != nullptr && semantic_->isClassType(name)) return name;
            // 第 9 层 Debug（P3-8）：顶层静态变量——exprSrcType 须返回全局静态的
            //   源码类型（如 向量$整64），否则 全局表.大小() 的对象类型解析失败
            //   （无值 -> isClassType 判定 false -> 走通用间接调用路径，callee
            //   MemberExpr 被当字段访问生成空 extra 的 AddrOf -> [rbp0] A2006）。
            if (semantic_ != nullptr && semantic_->isGlobalStatic(name)) {
                return semantic_->globalStaticType(name);
            }
            // Task 6.1（泛型嵌套容器 栈<T> 组合 向量<T>）：方法体内直接字段名
            //   （数据）——exprSrcType 需返回字段源码类型（向量$整32），否则
            //   handleClassCallExpr 对 数据.设置() 的对象类型解析失败（无值 ->
            //   isClassType 判定 false -> 走通用间接调用路径，生成空取地址）。
            if (!currentClass_.empty() && !currentMethodStatic_ &&
                isInstanceField(name)) {
                // 1472：与 isInstanceField 同锚（方法声明类链·最派生命中）
                return optOfSrc(classFieldType(fieldScopeClass(), name).value_or(""));
            }
            return std::nullopt;
        }
        case NodeType::BinaryExpr: {
            // 缺陷2 修复：链式运算符重载（甲+乙+丙）内层 BinaryExpr 的源码类型
            //   由语义层写回 resolvedType（重载方法返回类型，如 复数）。否则
            //   exprSrcType 无值，外层 + 被误判为 ptr+ptr 字符串连接。
            return optOfSrc(static_cast<BinaryExpr*>(node)->resolvedType);
        }
        case NodeType::UnaryExpr: {
            // 465（2026-10-12）：单目运算符重载（- ! ~）结果源码类型——语义层
            //   visitUnaryExpr 命中重载时写回 resolvedType（与 BinaryExpr 同构）。
            //   原缺此分支 → 接收者=单目表达式形态（(-点(5)).x）类型推导无值 →
            //   handleClassMemberExpr 实例字段路径 isClassType 假 → return false →
            //   结构体路径 decl 空 → 字段读降级常量 0（静默错值）。
            // #1699：解引用读值（*指）源码类型=操作数指针所指类型——类值借用来源
            //   深拷通道（声明初始化/赋值）按 exprSrcType 判类类型，缺此分支=
            //   通道 miss→对象指针句柄直存双主（探针 pO 改动前 10/10 C0000374）。
            //   剥星口径同 derefPointeeSrcType（. 左侧指针自动解引用一级）。
            {
                const UnaryExpr* ue = static_cast<const UnaryExpr*>(node);
                if (!ue->resolvedType.empty()) return optOfSrc(ue->resolvedType);
                if (ue->op == Operator::Deref) {
                    return derefPointeeSrcType(ue->operand.get());
                }
                return std::nullopt;
            }
        }
        case NodeType::SelfExpr:
            return optOfSrc(currentClass_);
        case NodeType::TernaryExpr: {
            // 86-a（2026-09-12 复审缺陷①）：三元结果的源码类型——递归真/假分支
            //   （语义层已保证两分支类型统一；防御：真分支无值时取假分支）。
            //   原缺此分支 → 三元聚合右值的类型推导为空 → 赋值/声明/返回等消费位
            //   退化（P46 赋值位落标量 8 字节路径 = 静默数据损坏；P41 返回位不深拷
            //   = 悬垂）。与 84-d「类型推导失败→静默降级」同族范式，第三次复发。
            const TernaryExpr* te = static_cast<const TernaryExpr*>(node);
            const cn::Optional<std::string> t = exprSrcType(te->trueValue.get());
            if (t.has_value()) return t;
            return exprSrcType(te->falseValue.get());
        }
        case NodeType::SuperExpr:
            if (semantic_ != nullptr && !currentClass_.empty()) {
                const ClassInfo* ci = semantic_->findClass(currentClass_).value_or(nullptr);
                if (ci != nullptr) return optOfSrc(ci->baseName);
            }
            return std::nullopt;
        case NodeType::CallExpr: {
            // 宿主缺陷根治（2026-08-25）：函数/方法调用结果源码类型——优先语义层写回的
            // resolvedType（正常/错误 等内置），普通函数调用 resolvedType 为空时
            //   按被调函数返回类型解析（安全除法(a,b) -> 结果<整32,整32>）。原缺
            //   CallExpr 分支返回无值，? 运算符读偏移按操作数类型推导失败（默认 8 读错位）。
            const CallExpr* call = static_cast<CallExpr*>(node);
            if (!call->resolvedType.empty()) return call->resolvedType;
            // 465 根治（2026-10-12）：构造调用 类名(实参)——callee 名经泛型实例名
            //   映射（resolveGenericCtorInstanceName，与 emitConstructorCall 同一
            //   判据单一事实源）后 findClass 命中即构造调用 → 返回实例化类名。
            //   原缺此分支，构造调用临时对象的源码类型恒空 → 单目/二元运算符
            //   重载分派（handleUnaryOperatorOverload/handleOperatorOverload 的
            //   isClassType 判据）全 miss 静默落内置通道：!温度(0) 对对象指针发射
            //   逻辑非（指针非零恒得假）、点(1)+点(2) 退化为指针相加（436 同族
            //   「表达式位类型推导缺分支→下游分派 miss」）；先存变量再运算
            //   （标识符有 lookupSrcType 路径）不受影响=两侧分叉铁证。
            //   判据次序对齐语义层 visitCallExpr（类名优先于普通函数名）。
            if (call->callee->getType() == NodeType::IdentifierExpr &&
                semantic_ != nullptr) {
                const std::string ctorName = resolveGenericCtorInstanceName(
                    static_cast<const IdentifierExpr*>(call->callee.get())->name);
                if (!ctorName.empty() && semantic_->findClass(ctorName).has_value()) {
                    return ctorName;
                }
            }
            if (call->callee->getType() == NodeType::IdentifierExpr &&
                semantic_ != nullptr) {
                const std::string fn =
                    static_cast<const IdentifierExpr*>(call->callee.get())->name;
                const std::string sig = semantic_->funcFirstSigKey(fn).value_or("");
                // 845（零分叉·569 采样轮）：函数返回泛型容器实例（造() ->
                //   向量<整32>）的返回文本=canonicalType 形态（含 '<'，未转 $
                //   实例名）——变量声明路径 lookupSrcType 取物化名，调用路径
                //   原样返回 → 下游（emitMemberMethodCall 对象识别 / 662 临时
                //   析构 / memberObjStructType 链）isClassType/findClass 全 miss
                //   （造().大小() 宿主逸出「无法调用非函数类型」/648 内部错误；
                //   v2 全绿）。此处与变量路径对齐物化（resolveGenericInstanceType
                //   幂等：非泛型/已物化原样返回）。
                return optOfSrc(resolveGenericInstanceType(
                    semantic_->funcReturnTypeOf(sig.empty() ? fn : sig).value_or("")));
            }
            // 宿主缺陷根治（2026-08-25）：对象.方法() 调用——解析方法返回类型
            //   （表.元素(0) -> 映射$整64$整64）。原缺此分支导致链式方法调用
            //   （元素(0).设置()）对象类型推导失败 -> handleClassCallExpr 未识别
            //   类方法 -> 通用间接调用（this=0）崩溃 0xC0000005 实测。
            if (call->callee->getType() == NodeType::MemberExpr &&
                semantic_ != nullptr) {
                const MemberExpr* mem =
                    static_cast<const MemberExpr*>(call->callee.get());
                const cn::Optional<std::string> objType = exprSrcType(mem->object.get());
                if (objType.has_value()) {
                    std::string owner;
                    const ClassMemberInfo* m = semantic_->lookupClassMember(
                        types::canonical(objType.value()), mem->memberName, owner).value_or(nullptr);
                    // 845：方法返回文本同款物化（容器读方法返回 T 替换后已为
                    //   实例名；泛型文本残余形态归一——幂等）
                    if (m != nullptr && !m->type.empty()) {
                        return optOfSrc(resolveGenericInstanceType(m->type));
                    }
                }
            }
            return std::nullopt;
        }
        case NodeType::IndexExpr: {
            // 955（008 总攻·六位置余三前置根治）：数组元素源码类型——`槽[0].大小()`
            //   的接收者=IndexExpr 原缺分支 → objSrcType 空 → isClassType 假 →
            //   走通用间接调用路径被调=0 → call *r11 SIGSEGV（p2c 实锤·与转移
            //   无关的既有缺陷）。数组名 lookupSrcType → arrayElemOf 元素类型。
            // #1699：形态判据统一到 memberIndexObjElemType 单点（448 提炼）——
            //   原本分支自持 Identifier/Index 两子分支，缺 Member 对象形态（类
            //   字段数组元素 `甲.数据[0]`，探针 pS 声明初始化→通道 miss 双主
            //   C0000374）且与单点漂移；统一后 Identifier（含 stripRef 引用数组
            //   形参/静态回退）/Member/Index 三形态同源。
            //   memberIndexObjElemType 形参非 const——与下方 Index 递归同款去
            //   const 直调（成员函数本体 const 不受影响）。
            return memberIndexObjElemType(const_cast<IndexExpr*>(
                static_cast<const IndexExpr*>(node)));
        }
        case NodeType::MemberExpr: {
            MemberExpr* mem = static_cast<MemberExpr*>(node);
            const cn::Optional<std::string> objType = exprSrcType(mem->object.get());
            if (!objType.has_value()) return std::nullopt;
            // 宿主缺陷根治（2026-08-25）：结果/可选 成员映射——结果<T,E>.值/.错误 ->
            //   T/E；.正常 -> 布尔；可选<T>.值/.有值 -> T/布尔。原缺此映射导致
            //   exprSrcType(查.值) 返回空（值 不是合成结构体直接字段），链式
            //   （结果<类>.值.字段 / 结果.值.方法）的对象类型推导失败 -> 类字段
            //   访问/方法调用未识别 -> findStruct(类)=null 防御返回 0（打印 0 实测）。
            const std::string canonObj = types::canonical(objType.value());
            // #1562（零分叉·与下方结构体字段分支同族先例）：结果/可选 载荷为
            //   泛型容器时，T/E 取出的可能是**模板文本**（向量<整32>）而非物化
            //   实例名（向量$整32）——下游 findClass/lookupClassMember 全 miss →
            //   宿主报「不是结构体/联合体/类类型」误拒（r.值.大小() 实锚）。
            //   与结构体字段面同法物化（resolveGenericTypeName 幂等：非泛型原样）。
            auto payloadType = [&](const std::string& t) {
                if (semantic_ == nullptr || t.empty()) return t;
                return types::canonical(
                    semantic_->resolveGenericTypeName(t, node->location));
            };
            if (isResultType(canonObj)) {
                const std::vector<std::string> rargs =
                    resultTypeArgs(canonObj).value_or((std::vector<std::string>()));
                if (mem->memberName == "值" && rargs.size() == 2)
                    return payloadType(rargs[0]);
                if (mem->memberName == "错误" && rargs.size() == 2)
                    return payloadType(rargs[1]);
                if (mem->memberName == "正常") return "布尔";
            } else if (isOptionalType(canonObj)) {
                if (mem->memberName == "值")
                    return optOfSrc(payloadType(
                        optionalTypeArg(canonObj).value_or(std::string())));
                if (mem->memberName == "有值") return "布尔";
            }
            // 363 根治（2026-10-10·与 v2 321 同构）：链上指针字段中间跳——
            //   objType 带星（如 持有*）时 classFieldType/findStruct 恒 miss →
            //   三级链（自身.内.表.追加 的 .表）对象类型断链 → 方法符号解析 0
            //   （asm 铁证 mov r11,0; call r11）。. 左侧指针自动解引用一级
            //   （001 立法）——剥星限本消费点不入公共查询入口（362 教训：
            //   无差别剥星误伤哨兵消费）。
            std::string lookupType363 = objType.value();
            if (types::isPointer(lookupType363)) {
                lookupType363 = types::pointeeOf(lookupType363);
            }
            const cn::Optional<std::string> fieldType =
                classFieldType(lookupType363, mem->memberName);
            if (fieldType.has_value() && !fieldType.value().empty()) return fieldType.value();
            if (semantic_ != nullptr) {
                const StructDecl* decl =
                    semantic_->findStruct(types::canonical(lookupType363)).value_or(nullptr);
                if (decl != nullptr) {
                    for (const auto& f : decl->fields) {
                        // 宿主缺陷根治（2026-08-25）：结构体字段为泛型容器
                        //   （结构体 { 映射<整64,整64> 表 }）时返回实例名（映射$整64$整64）
                        //   ——原返回模板形式（映射<整64,整64>），handleClassCallExpr
                        //   isClassType 未命中 -> 对象.方法() 降级为间接调用 this=0
                        //   崩溃 0xC0000005（与语义层字段类型归一一致）。
                        if (f.name == mem->memberName) {
                            return types::canonical(
                                semantic_->resolveGenericTypeName(f.type, node->location));
                        }
                    }
                }
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
    }
}


// ==================== 指针所指源码类型推导族（#1889 自 ir.cpp 拆入） ====================
//   pointerPointeeSrcType：解引用/成员访问/指针算术消费点的「操作数所指元素」
//   源码类型单一事实源（1360 结果化：无值=无法推导）。搬移=纯机械（ir.cpp 破
//   1000 文件红线·先拆再加），仅 Deref 精确剥一层 / AddressOf 返回操作数类型
//   两处本轮语义修复。
// 1360 结果化：B 族源码类型推导函数统一无值=无法推导（原空串哨兵退役）。
cn::Optional<std::string> IRGenerator::pointerPointeeSrcType(Expr* node) const {
    if (node == nullptr) return std::nullopt;
    if (node->getType() == NodeType::MemberExpr) {
        // 655 复核轮（648 余量④d 同族补全·成员链指针解引用）：*b.游标 的
        //   操作数=成员链——原实现无 MemberExpr 分支恒无值，648 显形诊断
        //   误伤合法程序（探针 v2_992a 实锤：*b.游标 被拒编译中止）。所指
        //   元素类型=字段指针类型的 pointee，与 655 ④d 自增分支（visitUnary
        //   Expr 内联 memberFieldSrcType）同口径；字段为数组形态时=数组元素
        //   类型（425 数组名退化同源）。
        const cn::Optional<std::string> fieldType = memberFieldSrcType(
            static_cast<MemberExpr*>(node));
        if (fieldType.has_value() && types::isPointer(fieldType.value())) {
            return types::pointeeOf(fieldType.value());
        }
        if (fieldType.has_value() && types::isArray(fieldType.value())) {
            return types::arrayElemOf(fieldType.value());
        }
        return std::nullopt;
    }
    if (node->getType() == NodeType::IdentifierExpr) {
        const cn::Optional<std::string> srcType = lookupSrcType(
            static_cast<IdentifierExpr*>(node)->name);
        // 425 同轮补（09_integration 实录）：数组名标识符——所指元素类型=
        //   数组元素类型（数组名退化语义·与 ptrElemStride 数组分支同源）。
        //   原实现 isPointer(学生[5])=假 直返无值：`(名单 + 4).分数` 括号
        //   直读成员对象源类型空 → 字段偏移错位读垃圾（探针 5/6 实录）。
        if (srcType.has_value() && types::isArray(srcType.value())) {
            return types::arrayElemOf(srcType.value());
        }
        if (srcType.has_value() && types::isPointer(srcType.value())) {
            return types::pointeeOf(srcType.value());
        }
        return std::nullopt;
    }
    if (node->getType() == NodeType::BinaryExpr) {
        BinaryExpr* bin = static_cast<BinaryExpr*>(node);
        if (bin->op == Operator::Add || bin->op == Operator::Subtract) {
            // 指针算术：指针可能在左或右（p+1 / 1+p 罕见但支持）
            const cn::Optional<std::string> left = pointerPointeeSrcType(bin->left.get());
            if (left.has_value()) return left;
            const cn::Optional<std::string> right = pointerPointeeSrcType(bin->right.get());
            if (right.has_value()) return right;
        }
        return std::nullopt;
    }
    if (node->getType() == NodeType::UnaryExpr) {
        UnaryExpr* un = static_cast<UnaryExpr*>(node);
        // 自增/自减 p++/p--：表达式值仍是指针 p 本体（类型=p 的类型），所指=
        //   递归 operand（`*p++` = `*(p++)` 的 elemType 由此正确）
        if (un->op == Operator::Increment || un->op == Operator::Decrement) {
            return pointerPointeeSrcType(un->operand.get());
        }
        // #1889（2026-10-31·多级指针外层解引用精确剥一层）：`*x` 的类型=
        //   `pointerPointeeSrcType(x)`（x 的所指），而**本函数求的是 `*x` 自身
        //   的所指**——须在 x 的所指上再剥一层（宿主同轮与 v2 解析所指类型
        //   一元分支同修）。原 Deref 与自增合并「直接递归 operand」=只剥 x
        //   一层：`**pp` 的 elemType 得指针类型（8 字节宽读），`字符**` 探针
        //   修前读 8 字节吃相邻字节（v2 -3689348818173476287 / 宿主
        //   -4290559423 分叉实锚），`整32**` 同宽巧合盖住。inner 非指针/
        //   数组（`*(*非指针)` 上游语义层已拒）=无值交 648 显形，不静默按
        //   基类型宽读。
        if (un->op == Operator::Deref) {
            const cn::Optional<std::string> inner =
                pointerPointeeSrcType(un->operand.get());
            if (!inner.has_value()) return std::nullopt;
            if (types::isPointer(inner.value())) {
                return types::pointeeOf(inner.value());
            }
            // 数组退化（多维/数组名的所指仍是数组时）：再退化一维=元素类型
            //   （与 425 数组名退化同口径）
            if (types::isArray(inner.value())) {
                return types::arrayElemOf(inner.value());
            }
            return std::nullopt;
        }
        // #1889：`&x` 的类型=x 的类型+一层指针 → **所指=x 的类型本体**。
        //   原与 Deref 合并递归=返回 x 的所指：x 为指针时少一层、x 为标量时
        //   无值（`整32 v = *&n;` 合法程序误落 648 内部错误）。逐级解析
        //   （exprSrcType 覆盖标识符/静态/成员/调用/下标/三元 → 标识符直查
        //   变量表 → 成员字段类型）；仍无值=不可推导，交 648 显形。
        if (un->op == Operator::AddressOf) {
            Expr* innerExpr = un->operand.get();
            const cn::Optional<std::string> t = exprSrcType(innerExpr);
            if (t.has_value() && !t.value().empty()) return t;
            if (innerExpr->getType() == NodeType::IdentifierExpr) {
                return lookupSrcType(
                    static_cast<IdentifierExpr*>(innerExpr)->name);
            }
            if (innerExpr->getType() == NodeType::MemberExpr) {
                return memberFieldSrcType(
                    static_cast<MemberExpr*>(innerExpr));
            }
            return std::nullopt;
        }
        return std::nullopt;
    }
    if (node->getType() == NodeType::CallExpr) {
        // 813（144_v2 云端红归因·v2 对称面）：解引用调用返回指针
        //   （*容器.元素(0)）——返回源码类型经 exprSrcType（其 CallExpr 分支
        //   已支持 resolvedType/函数返回/方法返回三级），指针剥星=pointee。
        //   修前无 CallExpr 分支恒无值 → 648 显形诊断误伤合法程序（v2 侧
        //   修前兜底 T_整64 对 整64* 返回碰巧宽度正确·813 显形后双侧同修）。
        const cn::Optional<std::string> retSrc = exprSrcType(node);
        if (retSrc.has_value() && types::isPointer(retSrc.value())) {
            return types::pointeeOf(retSrc.value());
        }
        return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace cn_compiler
