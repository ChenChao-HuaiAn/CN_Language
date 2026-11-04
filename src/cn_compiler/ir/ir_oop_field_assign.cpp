// CN-IR生成器——类字段写入族（861 拆分：自 ir_oop_field.cpp 按族机械搬移——>900 警戒线先拆再加纪律）
//   族 = 方法体内直接字段赋值/自增减（handleClassFieldAssign + handleClassFieldIncDec）；
//   纯重构零行为变更（成员函数实现搬迁——声明仍在 ir_generator.hpp）。
#include <string>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 方法体内直接字段赋值（无 自身. 前缀）：
//   静态字段 -> 静态字段符号 StorePtr；实例字段 -> this+偏移 StorePtr
// 915-a（任务 084）：op 形参新增——复合赋值先读旧值再算（见两处复合分支）
bool IRGenerator::handleClassFieldAssign(IdentifierExpr* ident, Expr* value,
                                         Operator op, const SourceLocation& loc) {
    // 静态字段直接赋值（静态方法/普通方法内均可，Task 3.9）
    // #1445：起点=fieldScopeClass()（1472 名字隐藏同锚）+符号类名=声明类
    //   （findStaticFieldAlongChain 归一）——写路径与读路径/限定名通道落同一
    //   声明类槽（?static_声明类$字段），禁发射类并入槽（读写跨通道分裂·#1310）。
    if (semantic_ != nullptr && !fieldScopeClass().empty()) {
        std::string declClass;
        const cn::Optional<const ClassMemberInfo*> fieldOpt =
            findStaticFieldAlongChain(*semantic_, fieldScopeClass(), ident->name,
                                      declClass);
        if (fieldOpt.has_value()) {
            const ClassMemberInfo* field = fieldOpt.value();
            const std::string fieldType = field->type;
            std::string targetIrType = mapType(fieldType);
            if (targetIrType.empty()) {
                // 648（miss 不可静默·110 同族）：类字段类型未登记=内部缺陷显形；driver hasErrors 统一中止。
                diagnostics_.report(DiagnosticLevel::Error, loc,
                    "内部错误: 类字段类型未登记 '" + ident->name + "' —— 静默 i64 兜底已废除（#648）");
                targetIrType = "i64";  // 648：保留原兜底（中止前路径）
            }
            // 861（644-b 静态段对称补完·顶层静态 87-a 同律）：**静态字符串
            //   字段直赋归一化 + 旧值释放**——原静态段与 644-b 前的实例段
            //   同病：genExpr 产物句柄 StorePtr 裸写。借用来源（局部/形参）
            //   直存句柄 → 函数出口局部 RAII 释放后字段悬垂（探针 p9 实录：
            //   「静态字段 = 局部串」函数返回后宿主读空、v2 正确）；重复
            //   赋值不 free 旧值=旧串泄漏。静态槽长期存活恒拥有（无块出口
            //   RAII）——赋值点=唯一 drop 时机（顶层静态 87-a 同律）。
            //   顺序纪律=先归一化后 free 旧（自赋值 静态串=静态串 安全：
            //   新值已独立于旧句柄）。复合赋值 += 经语义层
            //   rewriteStringConcatCompoundAssign 重析为 BinaryExpr(Add) 落
            //   本分支（normalize 对拼接产物=直存接管——732 同口径）；
            //   非 += 的复合符对字符串已被语义层拒绝（不可拼接右值），
            //   复合段字符串不可达——数值复合原路径保留不动。
            if (types::canonicalEquals(fieldType, "字符串") &&
                !isCompoundAssignOp(op)) {
                ir::IRValue strVal = genExpr(value);
                strVal = normalizeStringValueSource(value, strVal, loc);
                ir::IRValue strAddr =
                    genStaticFieldAddr(declClass, ident->name, loc);
                // 旧值释放（幂等·空安全）：.data 零初始化槽首赋=null 安全
                emitFieldStringFreeAt(strAddr, loc);
                emit(ir::Opcode::StorePtr, {strAddr, strVal}, ir::IRValue(), "",
                     "ptr", loc);
                lastExpr_ = strVal;
                return true;
            }
            ir::IRValue val = genExpr(value);
            if (val.type != targetIrType && !targetIrType.empty()) {
                val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
            }
            ir::IRValue addr = genStaticFieldAddr(declClass, ident->name, loc);
            // 915-a（任务 084）：静态字段复合赋值先读旧值再算（原丢读取侧）
            if (isCompoundAssignOp(op)) {
                ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr},
                                                 targetIrType, "", loc);
                ir::Opcode opcode;
                Operator baseOp = baseOpOfCompound(op);
                if (mapBinaryOp(baseOp, false, opcode)) {
                    val = widenCompoundRhs(val, targetIrType, loc);  // 331-a T51
                    val = emitResult(opcode, {current, val}, targetIrType, "",
                                     loc);
                }
            }
            emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "",
                 targetIrType, loc);
            lastExpr_ = val;
            return true;
        }
    }
    if (!isInstanceField(ident->name)) return false;
    // 1360 结果化：value_or("") 显形（isInstanceField 已先行判定·空串落 648 显形原语义）
    // 1472：类型查询与 isInstanceField 同锚（方法声明类链·最派生命中）
    const std::string fieldType = classFieldType(fieldScopeClass(), ident->name).value_or("");
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) {
        // 648（miss 不可静默·110 同族）：类字段类型未登记=内部缺陷显形；driver hasErrors 统一中止。
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类字段类型未登记 '" + ident->name + "' —— 静默 i64 兜底已废除（#648）");
        targetIrType = "i64";  // 648：保留原兜底（中止前路径）
    }
    // 180-a（甲通道实装·对齐 memberClassFieldAssign 形态 b）：方法体内直接
    //   字段赋值（无 自身. 前缀=隐式 this）的**拥有型容器字段**深拷——
    //   149-a 定位（plans/020 第六十八节）：拷贝构造内 表 = 其他.表 原生成
    //   「读源槽指针 → 存本槽」= 指针共享 → 出口双重释放（uF 0xC0000374 铁证）。
    //   四族收窄（142-a 教训：用户类泛化未明交互，不扩）+ 构造内初始化豁免。
    if (semantic_ != nullptr) {
        const std::string fieldCanon = types::canonical(fieldType);
        const std::size_t dl = fieldCanon.find('$');
        const std::string head =
            dl == std::string::npos ? fieldCanon : fieldCanon.substr(0, dl);
        const bool isOwnedContainer =
            (head == "向量" || head == "栈" || head == "链表" || head == "队列") &&
            semantic_->isClassType(fieldCanon) &&
            classDestructorSymbolKey(fieldCanon).has_value();
        if (isOwnedContainer) {
            ir::IRValue fieldAddr = genInstanceFieldAddr(ident->name, loc);
            // 543 预释放判据（2026-10-14·459 批甲临时合流计数实证；初版撤尽
            //   145-a 跳过致 1035 容器追加崩 0xC0000374·本处精确化）：
            //   preFree ⇔ 非构造体内 || 非拷贝构造体内——
            //   ①构造外/非拷贝构造体内：旧值恒合法——__cn_object_new calloc
            //     清零（null）或 860-a 序言级联壳（非拷贝构造的序言**无条件**写
            //     槽：NewObject+默认构造+StorePtr）→ preFree 空安全且必要（跳过=
            //     级联壳被体覆盖无人释放·每对象恒漏 1 壳·p_n6/p_n7 探针+箱丁
            //     asm 双 NewObject 双 StorePtr 铁证）；
            //   ②拷贝构造体内：860-a 字段级联**显式排除拷贝构造**（ir_oop.cpp
            //     序言判定 isCopyConstructor——字段初始化=用户全权）→ this 槽
            //     可能来自未清零位置（栈槽/容器扩容 realloc 数组槽=垃圾）——
            //     预释放读垃圾句柄 DeleteObject=崩（145-a 原始防护场景·1035
            //     向量<货架>.追加 槽 realloc 后调拷贝构造实证）→ 跳过。
            if (!currentMethodIsCtor_ || !currentMethodIsCopyCtor_) {
                ir::IRValue oldObj =
                    emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr", "", loc);
                emitContainerElemFreeFor(fieldCanon, oldObj, loc);
                emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), fieldCanon,
                     "void", loc);
                ir::IRValue zero =
                    emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
                emit(ir::Opcode::StorePtr, {fieldAddr, zero}, ir::IRValue(), "",
                     "ptr", loc);
            }
            // 源分级（形态 b 同款）：构造调用/临时=新对象直接接管（零拷贝）；
            //   值来源=NewObject + 拷贝构造（引用实参=源左值槽地址〔发现二〕）。
            const bool srcIsCtor = value->getType() == NodeType::CallExpr;
            // 1360 结果化：无值=无拷贝构造（has_value 分流）
            const cn::Optional<std::string> copyKeyOpt =
                classCopyCtorSymbolKey(fieldCanon);
            if (srcIsCtor || !copyKeyOpt.has_value()) {
                ir::IRValue srcPtr = genExpr(value);
                emit(ir::Opcode::StorePtr, {fieldAddr, srcPtr}, ir::IRValue(), "",
                     "ptr", loc);
            } else {
                const ir::IRValue srcRef = lvalueAddress(value);
                if (srcRef.id < 0) {
                    // 非左值来源（三元等）防御：对象指针直存（借用接管）
                    ir::IRValue srcPtr = genExpr(value);
                    emit(ir::Opcode::StorePtr, {fieldAddr, srcPtr}, ir::IRValue(),
                         "", "ptr", loc);
                // 静默豁免业务值: 1360 批3A 结果化取值路径——类查找 miss 即非类字段回落·非兜底
                } else {
                    const ClassInfo* ci = semantic_->findClass(fieldCanon).value_or(nullptr);
                    // 1672 批10 甄别（#1360 处置①·B 类：哨兵 0 与合法 totalSize=0
                    //   重叠且消费点原未检查）：isOwnedContainer 判定已含
                    //   isClassType(fieldCanon)（≡findClass 非空机械等价）——miss
                    //   不可达；原三元 `ci != nullptr ? totalSize : 0` 把 miss 静默
                    //   写成分配尺寸 0（NewObject 零尺寸=堆破坏面），补内部错误显形，
                    //   extra 组装保持原控制流（产物丢弃）。
                    if (ci == nullptr) {
                        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "内部错误: 类字段深拷分配尺寸类布局表缺失 '" +
                                fieldCanon + "'（#1360·1672）");
                    }
                    const std::string extra =
                        fieldCanon + "|" +
                        std::to_string(ci != nullptr ? ci->totalSize : 0);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(fieldCanon, "ptr")}, "ptr", extra,
                        loc);
                    emit(ir::Opcode::Call, {newObj, srcRef}, ir::IRValue(),
                         copyKeyOpt.value(), "void", loc);
                    emit(ir::Opcode::StorePtr, {fieldAddr, newObj}, ir::IRValue(),
                         "", "ptr", loc);
                }
            }
            lastExpr_ = fieldAddr;
            return true;
        }
    }
    // 915-b（任务 914·m11 实证·与 v2 生成语句赋值 915-b 裸字段面零分叉）：
    //   **用户类字段（裸字段名·隐式 this）← 标识符源**深拷——原落下方标量通道
    //   指针存储=句柄浅共享双主（源变量与字段同对象：源出口 RAII + 父析构级联
    //   双 DeleteObject = C0000374 堆破坏·探针 m11 双侧实锤）。修=成员链面
    //   emitClassFieldHandleCopy 同型（新建壳+拷贝构造+旧壳释放+新句柄入槽·
    //   判定同单点=有拷贝构造类才分流：无拷贝构造类两侧维持无释放面现状）。
    //   位置=genExpr 前（helper 自发射源地址·防双发副作用）。
    if (!isCompoundAssignOp(op) && value->getType() == NodeType::IdentifierExpr) {
        const std::string fieldCanonZ = types::canonical(fieldType);
        if (canEmitClassFieldHandleCopy(fieldCanonZ, value)) {
            ir::IRValue fieldAddrZ = genInstanceFieldAddr(ident->name, loc);
            emitClassFieldHandleCopy(fieldAddrZ, fieldCanonZ, value, loc);
            lastExpr_ = fieldAddrZ;
            return true;
        }
    }
    // 915-b 配合（095 覆盖前 drop 旧值面补齐·与 v2 843-a 同款·m10b 实证）：
    //   **用户类字段（裸字段名）← 调用产物**（构造调用/返回类对象调用·接管
    //   语义）——原路径句柄直存覆盖：旧壳（连同资源字段）无人释放=每轮泄漏
    //   1（m10b 析构数 1<51 双侧实锤）。复用 629 成员面单点（转移/引用返回
    //   排除同口径·先求值后释放=自用右值安全）。容器字段由 180-a 前段处理
    //   不达此；判定先于地址计算（零发射守卫）。
    //   ★isClassType 守卫（915-b 收口修正·REBASE 后官网示例回归实证）：
    //   canMemberClassCallProductAssign=「类字段」判定（原唯一消费点 974 段
    //   外层已限 isStructType/isClassType），裸字段面裸调时**标量字段也会
    //   命中**（exprSrcType==fieldCanon·如「整64」==「整64」）→ 对整64字段
    //   发 DeleteObject(数值当句柄 free)=堆破坏（stdlib/随机数.cn 播种
    //   `状态0 = 自身.扩散混合(x)` → 官网 random/猜数示例 C0000374 实证·
    //   develop 对照绿）。与 canEmitClassFieldHandleCopy 内建 isClassType
    //   同口径显式前置。
    if (!isCompoundAssignOp(op) && value->getType() == NodeType::CallExpr &&
        semantic_ != nullptr) {
        const std::string fieldCanonY = types::canonical(fieldType);
        if (semantic_->isClassType(fieldCanonY) &&
            canMemberClassCallProductAssign(value, fieldCanonY)) {
            ir::IRValue fieldAddrY = genInstanceFieldAddr(ident->name, loc);
            memberClassCallProductAssign(value, fieldAddrY, fieldCanonY, loc);
            lastExpr_ = fieldAddrY;
            return true;
        }
    }
    // #1700 乙案+#1696 随批（2026-10-27）：类字段 ← 字符串源——归一化
    //   （79-a 字段独立拥有：标识符=复制落堆/字面量=驻留/调用=按 A2 白名单
    //   接管·normalizeStringValueSource 单点）+pre-free（普通字符串字段旧值
    //   释放+清槽）/覆盖即忘（手动释放<T> 字段跳过——忘旧责任随声明归用户，
    //   释放由 摘除(槽.值) 执行）。原落标量 StorePtr 直存=浅共享+调用方临时
    //   结算悬垂（探针简乙 实锤：形参直赋后 .值 读空——memberStringFieldAssign
    //   只覆盖结构体面·类路径（裸字段名）漏同款）。
    if (!isCompoundAssignOp(op) && semantic_ != nullptr) {
        const std::string fieldCanonS = types::canonical(fieldType);
        const std::string payloadS =
            types::manuallyDroppedPayloadOf(fieldCanonS).value_or(std::string());
        const bool strLikeS = (fieldCanonS == "字符串") ||
                              (types::canonical(payloadS) == "字符串");
        if (strLikeS) {
            ir::IRValue fieldAddrS = genInstanceFieldAddr(ident->name, loc);
            ir::IRValue valS = genExpr(value);
            ir::IRValue normalizedS =
                normalizeStringValueSource(value, valS, loc);
            if (payloadS.empty()) {
                emitFieldStringFreeAt(fieldAddrS, loc);   // 旧值幂等释放+清槽
            }
            emit(ir::Opcode::StorePtr, {fieldAddrS, normalizedS},
                 ir::IRValue(), "", "ptr", loc);
            lastExpr_ = normalizedS;
            return true;
        }
    }
    ir::IRValue val = genExpr(value);
    if (val.type != targetIrType && !targetIrType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
    }
    // 512-a（T80-host 根治·第二缺口·与 v2 侧 508-a 同源）：**类字段（结构体
    //   类型）整体赋值**——目标=字段地址（genInstanceFieldAddr=对象指针+字段
    //   偏移），源按右值形态取地址（调用=求值产物即结构体地址〔retbuf/物化槽〕；
    //   标识符/成员/下标=左值地址），CopyStruct 整体拷贝（深拷=真·旧字段串
    //   preFree=真）。原落下方标量 StorePtr（8 字节）→ 结构体字段只写首 8 字节
    //   （探针 `内 = p` + 方法调用后 读=0 实证）。
    {
        const std::string fieldCanon2 = types::canonical(fieldType);
        if (semantic_ != nullptr && !fieldCanon2.empty() &&
            semantic_->isStructType(fieldCanon2)) {
            ir::IRValue fieldAddr2 = genInstanceFieldAddr(ident->name, loc);
            // 1651：`字段 = 无`（结果/可选 空载荷）——按合成体布局零写；原落
            //   下方整块拷贝（lvalueAddress(无)=常量 0 被当源地址→16 字节从地址
            //   0 拷 C0000005 fault=0，宿主+v2 同病）。preFree=覆盖前释放旧载荷
            //   （tag 守卫·与下方聚合整块赋值的 preFree 契约同口径）。
            if (value->getType() == NodeType::NullLiteral &&
                isResultOrOptionalText(fieldCanon2)) {
                emitSyntheticNullWrite(fieldAddr2, fieldType, loc,
                                       /*preFree=*/true);
                lastExpr_ = fieldAddr2;
                return true;
            }
            ir::IRValue srcAddr2;
            if (value->getType() == NodeType::CallExpr) {
                srcAddr2 = genExpr(value);
            } else {
                srcAddr2 = lvalueAddress(value);
                if (srcAddr2.id < 0) { srcAddr2 = genExpr(value); }
            }
            if (fieldAddr2.id >= 0 && srcAddr2.id >= 0) {
                emitStructCopyWithFields(fieldAddr2, srcAddr2, fieldCanon2, loc,
                                         /*preFree=*/true, /*deepCopy=*/true);
                lastExpr_ = fieldAddr2;
                return true;
            }
        }
    }
    // 644-b（2026-10-16·52_library 云端红根治·宿主/v2 同构）：字符串实例字段
    //   直访赋值（方法/构造体内裸字段名=隐式 this）79-a 同款来源分级归一化——
    //   原形态 genExpr 产物句柄直存（StorePtr 裸写）=字段接管右值句柄。与 644
    //   传参位串临时 flush 冲突（探针 p3/p8 铁证）：构造器 ISBN编号 = 编号
    //   句柄直存 + 调用方 flush = 字段悬空（52_library 三本图书 ISBN 全空）；
    //   基线则靠「调用方不 flush」侥幸配对（每调用净泄 1·644 立项原病）。
    //   归一化后字段独立拥有（79-a 立法：零共享零悬垂零泄漏·与 自身.字段 =
    //   显式形态 memberStringFieldAssign 同构）：字面量=驻留直存（释放侧空
    //   安全）；拥有返回调用=接管直存（新串无主）；其余（形参/局部/成员链
    //   借出）=__cn_str_copy 落堆深拷。旧值释放：非构造内=emitFieldStringFreeAt
    //   幂等清槽（重赋值语义·字段槽 calloc 零初始化+空安全双保险）；构造内=
    //   初始化语义豁免（145-a 同规：目标槽未初始化不 preFree）。归一化在释放
    //   前——自赋值 新值已独立于旧句柄。
    if (!isCompoundAssignOp(op) && types::canonicalEquals(fieldType, "字符串")) {
        val = normalizeStringValueSource(value, val, loc);
        ir::IRValue strFieldAddr = genInstanceFieldAddr(ident->name, loc);
        if (!currentMethodIsCtor_) {
            emitFieldStringFreeAt(strFieldAddr, loc);
        }
        emit(ir::Opcode::StorePtr, {strFieldAddr, val}, ir::IRValue(), "",
             "ptr", loc);
        lastExpr_ = val;
        return true;
    }
    ir::IRValue addr = genInstanceFieldAddr(ident->name, loc);
    // 915-a（任务 084·与 memberGenericAssign:276 同构）：复合赋值先读旧值
    //   再算（原丢读取侧——方法体裸字段 `存量 += n` 实测存 n 应 存量+n）
    if (isCompoundAssignOp(op)) {
        ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetIrType,
                                         "", loc);
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            val = widenCompoundRhs(val, targetIrType, loc);  // 331-a T51
            val = emitResult(opcode, {current, val}, targetIrType, "", loc);
        }
    }
    emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = val;
    return true;
}

// 方法体内直接字段自增/自减（无 自身. 前缀，Task 3.9 缺陷5 修复）：
//   静态字段 / 实例字段 的 ++/-- 需"读-算-写回"三段（原 visitUnaryExpr 只对
//   varStack_ 命中的普通变量写回，类字段不在 varStack_ 中，导致只读不写，
//   静态字段 总数++ 实测计数恒 0）。
bool IRGenerator::handleClassFieldIncDec(IdentifierExpr* ident, Operator op,
                                         const SourceLocation& loc,
                                         bool postfix) {
    // 静态字段直接访问（静态方法/普通方法内均可，Task 3.9）
    // #1445：起点=fieldScopeClass()（1472 名字隐藏同锚）+符号类名=声明类
    //   （findStaticFieldAlongChain 归一）——读算写回三段同落声明类槽。
    if (semantic_ != nullptr && !fieldScopeClass().empty()) {
        std::string declClass;
        const cn::Optional<const ClassMemberInfo*> fieldOpt =
            findStaticFieldAlongChain(*semantic_, fieldScopeClass(), ident->name,
                                      declClass);
        if (fieldOpt.has_value()) {
            const std::string fieldType = fieldOpt.value()->type;
            std::string targetIrType = mapType(fieldType);
            if (targetIrType.empty()) {
                // 648（miss 不可静默·110 同族）：类字段类型未登记=内部缺陷显形；driver hasErrors 统一中止。
                diagnostics_.report(DiagnosticLevel::Error, loc,
                    "内部错误: 类字段类型未登记 '" + ident->name + "' —— 静默 i64 兜底已废除（#648）");
                targetIrType = "i64";  // 648：保留原兜底（中止前路径）
            }
            ir::IRValue addr = genStaticFieldAddr(declClass, ident->name, loc);
            ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {addr},
                                         targetIrType, "", loc);
            ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, targetIrType,
                                           "1", loc);
            ir::IRValue result = emitResult(
                op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                {cur, delta}, targetIrType, "", loc);
            emit(ir::Opcode::StorePtr, {addr, result}, ir::IRValue(), "",
                 targetIrType, loc);
            // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
            lastExpr_ = postfix ? cur : result;
            return true;
        }
    }
    if (!isInstanceField(ident->name)) return false;
    // 1360 结果化：value_or("") 显形（isInstanceField 已先行判定·空串落 648 显形原语义）
    // 1472：类型查询与 isInstanceField 同锚（方法声明类链·最派生命中）
    const std::string fieldType = classFieldType(fieldScopeClass(), ident->name).value_or("");
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) {
        // 648（miss 不可静默·110 同族）：类字段类型未登记=内部缺陷显形；driver hasErrors 统一中止。
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类字段类型未登记 '" + ident->name + "' —— 静默 i64 兜底已废除（#648）");
        targetIrType = "i64";  // 648：保留原兜底（中止前路径）
    }
    ir::IRValue addr = genInstanceFieldAddr(ident->name, loc);
    ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {addr}, targetIrType, "", loc);
    ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, targetIrType, "1", loc);
    ir::IRValue result = emitResult(
        op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
        {cur, delta}, targetIrType, "", loc);
    emit(ir::Opcode::StorePtr, {addr, result}, ir::IRValue(), "", targetIrType, loc);
    // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
    lastExpr_ = postfix ? cur : result;
    return true;
}

// ==================== 族：类字段聚合整体赋值（974 提取 + #1979 新面） ====================

// #1979：数组元素句柄型判定（元素级深拷触发面·闭集穷举递归）——字符串/容器/
//   用户类/嵌套数组（递归到底）/含资源字段结构体；纯标量=位拷已正确零义务
//   （闭集终点·显式枚举余项）。深度护栏 4 层（与 emitValueCaptureArrayDeepCopy
//   同界——值语义结构体不可自包含，护栏=形态异常防线）。
static bool arrayElemsOwnResource(const ISemanticView& semantic,
                          const std::string& elemCanonRaw, int depth) {
    // 深度护栏合法域=[0,4] 双边界闭区间（1880 v2 同款·单边禁令）——越界=调用
    //   链形态异常，返假=按纯标量零义务处理（递归深度 5 层+数组=现实不可达）。
    // 静默豁免闭集穷举: 深度护栏返假=按纯标量零义务（1880 emitValueCaptureArrayDeepCopy
    //   基线位同形态·>5 维嵌套藏句柄=现实不可达·登记 miss_registry #1979）
    if (depth > 4 || depth < 0) return false;
    const std::string elemCanon = types::canonical(elemCanonRaw);
    if (types::isArray(elemCanon)) {
        return arrayElemsOwnResource(semantic, types::arrayElemOf(elemCanon), depth + 1);
    }
    if (types::canonicalEquals(elemCanon, "字符串")) return true;
    if (semantic.isClassType(elemCanon)) return true;
    if (semantic.isStructType(elemCanon)) {
        // 1672 同甄别：isStructType 命中 ≡ findStruct 非空（机械等价·1672 批10
        //   先例口径）——miss 不可达，不设冗余判空。
        const StructDecl* decl = semantic.findStruct(elemCanon).value_or(nullptr);  // 静默豁免闭集穷举: isStructType 命中≡findStruct 非空（1672 机械等价）miss 不可达
        for (const auto& f : decl->fields) {
            if (arrayElemsOwnResource(semantic, f.type, depth + 1)) return true;
        }
        return false;
    }
    return false;
}

// #1979（#1882 探针派生）：**类静态数组字段整体赋值**——整块拷（元素×长度全部
//   复制·259 顶层静态数组 assignToGlobalStatic 同款立法投影·001 §3.5 数组=值
//   聚合赋值拷贝语义）。病根：974 查询链（structOrClassFieldType→
//   memberObjectSrcType）对「类名.静态字段」基座恒 miss（类名非变量非全局
//   静态），数组 canon 亦非 struct/class → 落 handleClassMemberAssign 尾段
//   通用 StorePtr：targetIrType=mapType(数组)=剥一维元素类型，val=genExpr(数组
//   名)=源栈基址 Cast——只写槽首 8 字节且写入值=栈地址（双侧实测
//   [0]=140737417564416 垃圾值）。#1882 已修下标读写面+静态槽=元素×长度本体
//   布局，本函数补整体赋值面（直接整块拷）。
//   形态穷举（双侧零分叉·与 v2 070 波G 数组通道同域）：源=同型数组标识符
//   →源 place 地址+整块拷+元素句柄型逐元素深拷替换（#1880
//   emitValueCaptureArrayDeepCopy 设施·静态槽长期存活恒拥有——浅拷共享源
//   局部句柄=源块退出 RAII 后目标悬垂，E2E 1743 实锤）。其余源形态（构造
//   字面量/调用/成员链/一般算术）=648 显形中止（v2 同域同拒），禁落 8 字节
//   静默错值写。
//   true=已处理；false=非类静态数组字段形态（交调用方后续通道）。
bool IRGenerator::emitStaticArrayFieldWholeCopy(MemberExpr* target,
                                                Expr* valueExpr,
                                                const ir::IRValue& addr,
                                                const SourceLocation& loc) {
    // 契约：semantic_ 非空由调用链保证（handleClassMemberAssign 532 行前置分流）。
    const cn::Optional<std::string> staticFieldType =
        classStaticFieldTypeOfMember(*semantic_, target);
    if (!staticFieldType.has_value()) return false;
    const std::string staticCanon = types::canonical(staticFieldType.value());
    // 静默豁免业务值: 静态命中但非数组（标量/字符串/类静态字段）=既有通道接管
    //   让位（struct/class 通道/通用 StorePtr·handleClassMemberAssign 后续段），
    //   非兜底——登记 miss_registry（#1979）
    if (!types::isArray(staticCanon)) return false;
    // ---- 源 place 地址（穷举=数组标识符源·三形态同 emitStructWholeAssign 约定）----
    ir::IRValue srcArr;
    if (valueExpr->getType() == NodeType::IdentifierExpr) {
        const std::string srcName =
            static_cast<IdentifierExpr*>(valueExpr)->name;
        const cn::Optional<const VarEntry*> veSrc = findVarEntry(srcName);
        // 87-a：顶层静态源=?gstatic_名 符号地址（lvalueAddress 静态分支同口径）
        // 1337：普通局部/参数遮蔽全局时名字直查截胡=错源——局部优先序同款守卫
        if (semantic_->isGlobalStatic(srcName) && !veSrc.has_value()) {
            srcArr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                "?gstatic_" + srcName, loc);
        } else if (veSrc.has_value() && veSrc.value()->isStaticLocal) {
            // 852：函数内静态局部——?gstatic_唯一名 符号地址直引（320-a 同口径）
            srcArr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                "?gstatic_" + veSrc.value()->uniqueName, loc);
        } else if (veSrc.has_value() && !veSrc.value()->isStaticLocal) {
            // 局部/参数数组：槽本体地址=数组基址（AddrOf·94-a 基址槽模型）
            const cn::Optional<std::string> srcUniq = lookupVarName(srcName);
            if (!srcUniq.has_value()) {
                // 648（miss 不可静默·110 同族）：数组源变量未登记=内部缺陷显形
                diagnostics_.report(DiagnosticLevel::Error, loc,
                    "内部错误: 类静态数组字段整体赋值源变量 '" + srcName +
                        "' 槽未登记 ——静默按整64写兜底已废除（#648/#1979）");
                return true;
            }
            srcArr = emitResult(ir::Opcode::AddrOf,
                                {ir::IRValue::var(srcUniq.value(), "i64")},
                                "ptr", srcUniq.value(), loc);
        }
        // 补集（变量表与静态表全 miss）=srcArr 保持无效哨兵 → 下方等值判定显形
        //   （三态穷举终点·非 else 兜底）
    } else {
        // 648（miss 不可静默）：源形态未支持（构造字面量/调用/成员链/算术等
        //   非穷举形态）——合法程序内部缺陷显形；禁落 8 字节静默错值写
        //   （#1979 缺陷本体·v2 同域 IR层诊断+中止=双侧零分叉）。
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类静态数组字段整体赋值源形态未支持（类型 '" +
                staticCanon + "'·穷举=同型数组标识符）——静默按整64写兜底已废除（#648/#1979）");
        return true;
    }
    // 三态穷举补集收口（v2 887b != 0 等值哨兵同族形态）：三产物形态
    //   （ConstString/AddrOf）恒发有效寄存器，-1=仅补集（未登记变量）可达。
    // 静默豁免闭集穷举: 三态穷举补集收口（v2 887b != 0 等值哨兵同族）——三产物
    //   恒发有效寄存器，-1=仅未登记变量补集可达·块内显形中止（非静默继续·登记 #1979）
    if (srcArr.id == -1) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类静态数组字段整体赋值源地址未解析 ——静默按整64写兜底"
            "已废除（#648/#1979）");
        return true;
    }
    // 整块拷：元素×长度全部复制（emitStructCopyWithFields 单点——数组 canon
    //   的 ownedStrFieldsOf 恒空=纯 CopyStruct typeSizeOf 字节·259 同形态）。
    //   preFree=false（259 同款：静态槽重复覆盖不逐元素 drop 旧句柄——与顶层
    //   静态数组通道同边界）。
    emitStructCopyWithFields(addr, srcArr, staticCanon, loc,
                             /*preFree=*/false, /*deepCopy=*/false);
    // 元素句柄型 → 整块后逐元素深拷替换独立副本（#1880 设施同构）：字符串=
    //   __cn_str_copy 落堆/容器类=拷贝构造内联壳/结构体=字段级递归/嵌套数组
    //   递归到底；纯标量=位拷已正确零义务（emitValueCaptureArrayDeepCopy 内
    //   闭集穷举终点）。
    if (arrayElemsOwnResource(*semantic_, types::arrayElemOf(staticCanon), 0)) {
        emitValueCaptureArrayDeepCopy(addr, srcArr, staticCanon, loc, 0);
    }
    lastExpr_ = addr;
    return true;
}

// 974（104-001 根治）+ #1979：类字段聚合整体赋值单助手（自 handleClassMemberAssign
//   提取——本文件 897 行贴 900 警戒线·先拆再加纪律；974 四通道逐字搬移零语义变化）。
//   字段=结构体/类 → 整体赋值（emitStructWholeAssign 单一助手·Rust place 拷贝
//   语义）；字段=类静态数组字段 → #1979 整块拷通道（emitStaticArrayFieldWholeCopy）。
//   addr=handleClassMemberLvalue 已算好的目标地址（静态槽/实例字段偏移）。
//   契约：semantic_ 非空且 op 非复合——调用点 handleClassMemberAssign
//   （532 行 semantic_ 前置分流+本调用处于 !isCompoundAssignOp(op) 块内）保证。
bool IRGenerator::tryClassFieldWholeAssign(MemberExpr* target, Expr* valueExpr,
                                           const ir::IRValue& addr,
                                           const SourceLocation& loc) {
    // 1360 结果化：value_or("") 显形（miss=非聚合判定假·原语义）
    const std::string fieldTypeAgg =
        structOrClassFieldType(
            memberObjectSrcType(target->object.get()).value_or(""),  // 静默豁免闭集穷举: 1360 结果化显形（miss=非聚合判定假·原语义保持）
            target->memberName).value_or("");  // 静默豁免闭集穷举: 1360 结果化显形（miss=非聚合判定假·原语义保持）
    const std::string canonAgg = types::canonical(fieldTypeAgg);
    if (!canonAgg.empty() &&
        (semantic_->isStructType(canonAgg) ||
         semantic_->isClassType(canonAgg))) {
        if (structWholeAssignSrcFirst(target, valueExpr, canonAgg, loc)) {
            return true;
        }
        // 915-b（任务 914 现象 B 根治）：类字段（句柄槽）← 标识符源 →
        //   句柄语义深拷（新建壳+拷贝构造+旧壳释放+新句柄入槽）——原落
        //   下方 emitStructWholeAssign 标识符分支把字段槽地址当 this 传
        //   拷贝构造（内联元素语义残留·探针 甲.内=丙 fault=内容值实锤）。
        //   判定/发射单点=emitClassFieldHandleCopy（成员族·两入口共用）。
        if (canEmitClassFieldHandleCopy(canonAgg, valueExpr)) {
            emitClassFieldHandleCopy(addr, canonAgg, valueExpr, loc);
            lastExpr_ = addr;
            return true;
        }
        if (emitStructWholeAssign(addr, valueExpr, canonAgg, loc,
                                  /*preFree=*/true)) {
            return true;
        }
        // 629（544 呈报·成员目标面）：类字段 ←「返回类对象的调用」产物
        //   （盒.槽 = 仓.产() / 盒.槽 = 货架()）——原落下方标量 StorePtr
        //   直存新句柄：字段旧对象（连同资源字段）无人释放=泄漏（探针矩阵
        //   双侧实锤宿主残留 +2；与变量面 identifierClassCallProductAssign
        //   同族同修）。字段=句柄槽（728x 语义）：旧句柄单发 DeleteObject
        //   （095/821 同款·空句柄空安全——calloc 新对象字段零=构造体初赋值
        //   天然空安全）+ 新句柄 StorePtr。排除：转移（转移展开=源槽清零在
        //   ir_call·成员转移面维持原路径）/引用返回（借用产物不得释放）。
        if (memberClassCallProductAssign(valueExpr, addr, canonAgg,
                                         loc)) {
            return true;
        }
    }
    // #1979：类静态数组字段整体赋值（整块拷·详见 emitStaticArrayFieldWholeCopy）
    if (emitStaticArrayFieldWholeCopy(target, valueExpr, addr, loc)) {
        return true;
    }
    return false;
}

} // namespace cn_compiler
