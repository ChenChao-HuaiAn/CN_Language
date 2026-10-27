// IRGenerator 赋值·成员左值族实现（自 ir_expr_assign.cpp 拆分·1360 批2 行数门禁·逐字搬移零语义变化）
//   承载：assignToMemberTarget 分派及其十个子方法——memberStructLiteralAssign（构造字面量
//   原地逐字段写）/ memberClassFieldAssign（容器类字段归一化）/ canEmitClassFieldHandleCopy +
//   emitClassFieldHandleCopy（类字段句柄深拷·判定/发射分离）/ memberWholeStructAssign（结构体/
//   类字段整体赋值）/ memberGenericAssign（标量/字符串一般成员）/ memberAssignObjSrcType（对象
//   源码类型推导薄包装）/ memberStringFieldAssign（字符串字段来源归一化）/
//   canMemberClassCallProductAssign + memberClassCallProductAssign（类字段←调用产物接管）。
//   include 照抄原文件依赖子集；成员函数声明仍在 ir.hpp。
#include <cstdint>
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 族：成员左值（原 19~314 段） ====================
// 成员目标分派：类字段钩子 → 枚举只读 → 结构体构造字面量 → 类字段归一化 →
//   成员结构体作值写 → 一般（标量/字符串）成员赋值。true = 已处理完毕。
bool IRGenerator::assignToMemberTarget(AssignmentExpr* node) {
    // 结构体字段左值：p.x = v / 指针->x = v（Task 2.7）
    // 通过 lvalueAddress 计算字段地址（FieldAddr），StorePtr 写入
    MemberExpr* member = static_cast<MemberExpr*>(node->target.get());
    // ---- 阶段3 OOP（Task 3.1）：类字段赋值（对象.字段 = v / 类名.静态字段 = v） ----
    // 实例字段 -> 对象指针+偏移 StorePtr；静态字段 -> ?static_ 符号 StorePtr。
    if (handleClassMemberAssign(member, node->value.get(), node->op, node->location)) {
        return true;
    }
    // 枚举成员赋值不合法（枚举值为只读常量；枚举类型名非指针，
    //   isDerefAccess 恒 false——v2.1 保留原防御结构）
    if (!member->isDerefAccess &&
        member->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(member->object.get());
        std::int64_t v = 0;
        if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
            semantic_->enumValueOf(ident->name, member->memberName, v)) {
            lastExpr_ = genExpr(node->value.get());
            return true;  // 语义层已报错（只读）
        }
    }
    if (memberStructLiteralAssign(node, member)) return true;
    if (memberClassFieldAssign(node, member)) return true;
    if (memberWholeStructAssign(node, member)) return true;
    if (memberGenericAssign(node, member)) return true;
    return false;
}

// 构造字面量赋值（成员目标；原 38~66 段）：字段为结构体、右值为构造字面量 ->
//   emitStructInitTo 原地逐字段写（D2 根治，Rust place 语义）。
bool IRGenerator::memberStructLiteralAssign(AssignmentExpr* node, MemberExpr* member) {
    // D2 根治（2026-09-09 第四十六轮）：构造字面量赋值（成员目标）——
    //   r.右下 = 点{ x = 11, y = 12 }（字段为结构体）同走 emitStructInitTo
    //   单点逐字段写（与下标位 Task 2.7 / 标识符位 / 声明位同构——Rust 目标
    //   place 语义，穷举纪律在语义层 visitStructInitExpr）。原无此分支：
    //   空桩常量0 落 StorePtr 只写 8 字节 -> 字段静默零填（E2E 183 探针
    //   实锤 11/12 打出 0/0）。目标地址 = lvalueAddress(成员左值)。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::StructInitExpr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
        // 对象所属结构体（memberObjStructType 递归+指针解引用剥离）-> 字段类型
        // 1360 结果化：value_or("") 显形（无值=推导失败→字段查询 miss 原语义）
        const std::string ownerStruct = memberObjStructType(member).value_or("");
        const std::string fieldStruct =
            structOrClassFieldType(ownerStruct, member->memberName).value_or("");  // 973（104）统一查询
        // 静默豁免业务值: 1360 拆分随迁（存量守卫·非类字段=其他通道）非兜底
        if (!fieldStruct.empty() && semantic_->isStructType(fieldStruct)) {
            ir::IRValue fieldAddr = lvalueAddress(node->target.get());
            emitStructInitTo(init, fieldAddr, node->location);
            lastExpr_ = fieldAddr;   // 与整体赋值链式语义一致（值=目标地址）
            return true;
        }
    }
    return false;
}

// 类字段归一化赋值（成员目标·向量/栈/链表/队列族；原 67~159 段）：preFree
//   （元素释放 + DeleteObject + 清槽）→ 源分级（构造接管 / 拷贝构造深拷）。
bool IRGenerator::memberClassFieldAssign(AssignmentExpr* node, MemberExpr* member) {
    // 139-a（波 3 最小闭环；plans/020 第五十节发现四则）：成员赋值位——目标
    //   字段=**类对象（向量族；指针槽语义）**归一化赋值（写入位）：
    //     · preFree：元素释放 + DeleteObject（空跳过）+ 清槽；
    //     · 源分级：构造调用/临时=新对象直接接管（零拷贝）；值来源=NewObject
    //       + 拷贝构造（**引用实参=源左值地址〔槽地址〕**——发现二）深拷；
    //     · 结果指针 StorePtr 入槽。
    //   原路径经 46-a 通道对 8 字节指针槽做整块 CopyStruct（越界写+浅拷共享
    //   双删——形四 0xC0000374 实证）。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op)) {
        // 1360 结果化：value_or("") 显形（无值=推导失败→后续容器判定 miss 原语义）
        const std::string ownerStructC = memberObjStructType(member).value_or("");
        const std::string fieldCanonC =
            structOrClassFieldType(ownerStructC, member->memberName).value_or("");  // 973（104）统一查询
        // 142-a（泛化尝试→回退）：用户类场景实测 0xC0000374（用户类拷贝构造的
        //   this 字段初值/类析构字段级联与新设施交互未明）——按纪律回退，保留
        //   四族收窄；**泛化=专项**（plans/020 第五十四节）。
        const std::size_t dlC = fieldCanonC.find('$');
        const std::string headC =
            dlC == std::string::npos ? fieldCanonC : fieldCanonC.substr(0, dlC);
        if (!fieldCanonC.empty() &&
            (headC == "向量" || headC == "栈" || headC == "链表" ||
             headC == "队列") &&
            semantic_->isClassType(fieldCanonC)) {
                // 1360 结果化：无值=无析构（has_value 分流）
                const cn::Optional<std::string> dtorKeyC = classDestructorSymbolKey(fieldCanonC);
                if (dtorKeyC.has_value()) {
                    ir::IRValue fieldAddrC = lvalueAddress(node->target.get());
                    // 145-a+543 判据（2026-10-14·459 批甲临时合流计数实证）：
                    //   preFree ⇔ 非 this 接收者 || 非拷贝构造体内——
                    //   ①构造外/非拷贝构造体内对 this 字段赋值：序言级联**无条件**
                    //     写合法槽（NewObject calloc null 或 860-a 级联壳）→ preFree
                    //     空安全且必要（跳过=级联壳被体覆盖无人释放·每对象恒漏 1 壳·
                    //     p_n6/p_n7 探针+箱丁 asm 双 NewObject 双 StorePtr 铁证）；
                    //   ②拷贝构造体内 this 字段：序言无 860-a 级联（ir_oop.cpp 显式
                    //     排除拷贝构造）→ this 槽可能来自未清零位置（栈槽/容器
                    //     realloc 槽垃圾）→ 预释放读垃圾句柄=崩（145-a 原始防护·
                    //     1035 容器追加 0xC0000374 实证）→ 跳过；
                    //   ③非 this 接收者（如 其他.表/构造外）恒走完整 preFree。
                    //   释放后补清槽（幂等模型不变）：中途异常路径再见本槽=null。
                    bool isCtorThisField = false;
                    if (currentMethodIsCtor_ &&
                        member->object->getType() == NodeType::IdentifierExpr) {
                        const auto* objId =
                            static_cast<const IdentifierExpr*>(member->object.get());
                        if (objId->name == "自身") isCtorThisField = true;
                    }
                    if (!isCtorThisField || !currentMethodIsCopyCtor_) {
                        ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {fieldAddrC},
                                                        "ptr", "", node->location);
                        emitContainerElemFreeFor(fieldCanonC, oldObj, node->location);
                        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(),
                             fieldCanonC, "void", node->location);
                        ir::IRValue zeroC = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                                       "0", node->location);
                        emit(ir::Opcode::StorePtr, {fieldAddrC, zeroC}, ir::IRValue(),
                             "", "ptr", node->location);
                    }
                const bool srcIsCtorC =
                    node->value->getType() == NodeType::CallExpr;
                // 1360 结果化：无值=无拷贝构造（!has_value 分流·有值时 .value() 作符号键）
                const cn::Optional<std::string> copyKeyC = classCopyCtorSymbolKey(fieldCanonC);
                if (srcIsCtorC || !copyKeyC.has_value()) {
                    ir::IRValue srcPtrC = genExpr(node->value.get());
                    emit(ir::Opcode::StorePtr, {fieldAddrC, srcPtrC}, ir::IRValue(),
                         "", "ptr", node->location);
                } else {
                    // 静默豁免业务值: 1360 拆分随迁（非类字段=其他通道接管·非兜底）
                    const ir::IRValue srcRefC = lvalueAddress(node->value.get());
                    if (srcRefC.id < 0) {  // 静默豁免业务值: 1360 拆分随迁守卫（源地址无效=非类字段通道）
                        // 非左值来源（三元等）防御：对象指针直存（借用接管；
                        //   登记边界，后续轮收口）
                        ir::IRValue srcPtrC = genExpr(node->value.get());
                        emit(ir::Opcode::StorePtr, {fieldAddrC, srcPtrC},
                             ir::IRValue(), "", "ptr", node->location);
                        lastExpr_ = fieldAddrC;
                        return true;
                    }
                    const ClassInfo* ciC = semantic_->findClass(fieldCanonC).value_or(nullptr);
                    const std::string extraC =
                        fieldCanonC + "|" +
                        std::to_string(ciC != nullptr ? ciC->totalSize : 0);
                    ir::IRValue newObjC = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(fieldCanonC, "ptr")}, "ptr", extraC,
                        node->location);
                    emit(ir::Opcode::Call, {newObjC, srcRefC}, ir::IRValue(),
                         copyKeyC.value(), "void", node->location);
                    emit(ir::Opcode::StorePtr, {fieldAddrC, newObjC}, ir::IRValue(),
                         "", "ptr", node->location);
                }
                lastExpr_ = fieldAddrC;
                return true;
            }
        }
    }
    return false;
}

// 915-b（2026-10-16·任务 914 现象 B 宿主侧根治）：**类字段（句柄槽）← 标识符
//   源** 句柄语义深拷单一实现（判定/发射分离·两成员入口共用：handleClass-
//   MemberAssign 974 段与 memberWholeStructAssign）。
//   病根：原经 emitStructWholeAssign「标识符源」分支把字段槽地址当 this 传
//   拷贝构造（内联元素语义残留）：拷贝构造按对象布局写「槽地址+0..N」→ 8
//   字节句柄槽被写入对象内容首域（探针 `甲.内 = 丙` fault=内容值 0x2A 实锤），
//   后续读槽=垃圾句柄解引用崩 + 类大小>8 时写穿后继字段。
//   ★存储语义分流边界：**成员目标=类字段=句柄槽**走本通道（与标识符类变量
//   赋值 identifierClassCopyAssign 同型）；**指针下标元素=值槽/内联对象本体**
//   （容器 raw 缓冲区·emitStructWholeAssign 分支1「this=元素地址」正确——
//   915-b 首版曾误统一两形态=v2p 构建产物 C0000374 堆破坏实锤，已回退）。
//   生成=自赋值守卫 + preFree 旧壳（有析构类·空安全幂等清槽；对象分配 calloc
//   零初始化→未初始化槽 LoadPtr=0 空安全）+ NewObject + 拷贝构造(this=新壳·
//   实参=源槽地址〔引用参数=Load 捕获槽〕) + StorePtr 新壳入槽（对齐
//   identifierClassCopyAssign/183-a/728x 同族单点）。
//   判定=纯查询零发射（lvalueAddress 含空检查发射·调用方须先判后发防双发）。
bool IRGenerator::canEmitClassFieldHandleCopy(const std::string& fieldCanon,
                                              Expr* valueNode) {
    // 静默豁免业务值: 1360 拆分随迁（存量守卫：调用方契约+非类字段=其他通道接管）非兜底
    if (semantic_ == nullptr || valueNode == nullptr) return false;
    if (!semantic_->isClassType(types::canonical(fieldCanon))) return false;
    if (valueNode->getType() != NodeType::IdentifierExpr) return false;
    if (!semantic_->findCopyConstructor(types::canonical(fieldCanon)).has_value()) {
        return false;   // 无拷贝构造类字段走 emitStructWholeAssign 728x 合成拷贝
    }
    const std::string srcName = static_cast<IdentifierExpr*>(valueNode)->name;
    // 1360 结果化：has_value=已登记（原 .empty() 反判改显形）
    return lookupVarName(srcName).has_value();   // 静态源等无局部槽形态维持原路径
}

bool IRGenerator::emitClassFieldHandleCopy(const ir::IRValue& fieldAddr,
                                           const std::string& fieldCanon,
                                           Expr* valueNode,
                                           const SourceLocation& loc) {
    const std::string canon = types::canonical(fieldCanon);
    const ClassMemberInfo* copyCtor = semantic_->findCopyConstructor(canon).value_or(nullptr);
    // 1303 甄别（不可达断言·882 处置①）：三个调用点（memberWholeStructAssign
    //   :303 / handleClassMemberAssign:518 / ir_oop_field_assign.cpp:174）均先经
    //   canEmitClassFieldHandleCopy——其内 findCopyConstructor 非空判定
    //   （ir_expr_assign.cpp:225）已保证本查询非空；且本函数返回值在三调用点
    //   均被忽略（发射单点·false 无分流消费面），到达即编译器内部缺陷，报内部
    //   错误显形；返回 false 维持原控制流（driver generate() 后 hasErrors()
    //   中止，产物不落地）。
    // 静默豁免业务值: 1360 拆分随迁（存量守卫：调用方 can 判定先行·miss=非本通道）
    if (copyCtor == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类字段句柄深拷收到无拷贝构造类 '" + canon + "'（#1303）");
        return false;
    }
    const std::string srcName = static_cast<IdentifierExpr*>(valueNode)->name;
    const cn::Optional<std::string> srcUniqueOpt = lookupVarName(srcName);
    // 1303 甄别（不可达断言·882 处置①）：canEmitClassFieldHandleCopy 尾行
    //   `return lookupVarName(srcName).has_value()`（ir_expr_assign.cpp）已
    //   保证源槽名已登记；判定与发射之间为纯查询零状态变更，同名查询结果稳定，
    //   到达即内部缺陷（报内部错误显形·收尾同上）。
    if (!srcUniqueOpt.has_value()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类字段句柄深拷源槽 '" + srcName + "' 未登记（#1303）");
        return false;
    }
    const std::string& srcUnique = srcUniqueOpt.value();
    ir::IRValue srcRef;
    if (isByRefCapture(srcName)) {
        // 引用参数：槽内存=被引用对象槽地址（Load 槽·identifierClassCopyAssign
        //   :59 同款）
        srcRef = emitResult(ir::Opcode::Load, {ir::IRValue::var(srcUnique, "ptr")},
                            "ptr", srcUnique, loc);
    } else {
        // 静默豁免业务值: 1360 拆分随迁（源槽取地址·非兜底）
        srcRef = emitResult(ir::Opcode::AddrOf,
                            {ir::IRValue::var(srcUnique, "i64")}, "ptr",
                            srcUnique, loc);
    }
    const std::string skipLabel = beginSelfAssignGuard(fieldAddr, srcRef, loc);
    // 1360 结果化：无值=无析构（has_value 分流）
    if (classDestructorSymbolKey(canon).has_value()) {
        ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr",
                                        "", loc);
        emitContainerElemFreeFor(canon, oldObj, loc);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canon, "void", loc);
        ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
        emit(ir::Opcode::StorePtr, {fieldAddr, zero}, ir::IRValue(), "", "ptr", loc);
    }
    const std::string copyOwner =
        copyCtor->ownerClass.empty() ? canon : copyCtor->ownerClass;
    const ClassInfo* ci = semantic_->findClass(canon).value_or(nullptr);
    const std::string newObjExtra =
        canon + "|" + std::to_string(ci != nullptr ? ci->totalSize : 0);
    ir::IRValue newObj = emitResult(
        ir::Opcode::NewObject, {ir::IRValue::constant(canon, "ptr")}, "ptr",
        newObjExtra, loc);
    emit(ir::Opcode::Call, {newObj, srcRef}, ir::IRValue(),
         methodSymbolKey(copyOwner, copyCtor->sigKey), "void", loc);
    emit(ir::Opcode::StorePtr, {fieldAddr, newObj}, ir::IRValue(), "", "ptr", loc);
    endSelfAssignGuard(skipLabel);
    return true;
}

// 成员结构体作值写（成员目标·字段为结构体/类；原 160~190 段）：
//   emitStructWholeAssign 整体赋值（46-a 根治，Rust place 拷贝语义）。
bool IRGenerator::memberWholeStructAssign(AssignmentExpr* node, MemberExpr* member) {
    // 46-a 根治（2026-09-09 第四十八轮）：成员结构体作值写（r.左上 = a /
    //   r.右下 = r.左上）——字段为结构体/类时同走整体赋值单一助手
    //   emitStructWholeAssign（与下标位 H8 同构，Rust place 拷贝语义）。
    //   原无此通道：标量通用路径 StorePtr 只写 8 字节静默数据损坏。
    //   类型检查前置（与 D2 同型）：非结构体/类字段零冗余落回标量路径
    //   （lvalueAddress 有发射副作用，落回即双发）。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op)) {
        // 1360 结果化：value_or("") 显形（无值=推导失败→字段查询 miss 原语义）
        const std::string ownerStructW = memberObjStructType(member).value_or("");
        const std::string fieldStructW =
            structOrClassFieldType(ownerStructW, member->memberName).value_or("");  // 973（104）统一查询
        if (!fieldStructW.empty() &&
            (semantic_->isStructType(fieldStructW) ||
             semantic_->isClassType(fieldStructW))) {
            // 348-a（D11 甲方案·源优先序）：右值=一般结构体返回调用 → 先源后目标地址
            if (structWholeAssignSrcFirst(node->target.get(), node->value.get(),
                                          fieldStructW, node->location)) {
                return true;
            }
            // 915-b（任务 914 现象 B）：类字段（句柄槽）← 标识符源 → 句柄语义
            //   深拷（先判后发·与 handleClassMemberAssign 974 段共用单点）。
            if (canEmitClassFieldHandleCopy(fieldStructW, node->value.get())) {
                ir::IRValue fieldAddrB = lvalueAddress(node->target.get());
                emitClassFieldHandleCopy(fieldAddrB, fieldStructW,
                                         node->value.get(), node->location);
                lastExpr_ = fieldAddrB;
                return true;
            }
            ir::IRValue fieldAddrW = lvalueAddress(node->target.get());
            if (emitStructWholeAssign(fieldAddrW, node->value.get(),
                                      fieldStructW, node->location,
                                      /*preFree=*/true)) {
                lastExpr_ = fieldAddrW;  // 值=目标地址（链式语义与 D2 一致）
                return true;
            }
        }
    }
    return false;
}

// 一般成员赋值（标量/字符串字段；原 191~314 段）：目标类型推导 + Cast +
//   字符串字段归一化（独立拥有）+ 复合赋值 + StorePtr。
bool IRGenerator::memberGenericAssign(AssignmentExpr* node, MemberExpr* member) {
    ir::IRValue value = genExpr(node->value.get());
    // 1360 结果化：value_or("") 显形（无值=推导失败→findStruct("") miss 原语义）
    const std::string objSrcType = memberAssignObjSrcType(member).value_or("");
    std::string targetType = "i64";
    const StructDecl* decl = (semantic_ != nullptr)
                                 ? semantic_->findStruct(types::canonical(objSrcType)).value_or(nullptr)
                                 : nullptr;
    // 1059：类宿主回退——类字段（b.组[0].数 = v 的成员目标 数 属类 件）不入
    //   结构体表（findStruct 恒 miss → 原 648 误伤；v2 放行=宿主单侧拒绝分叉）。
    //   字段类型经 classFieldType（973 统一查询同源·沿继承链）。
    const bool ownerIsClass1059 =
        semantic_ != nullptr && !objSrcType.empty() &&
        semantic_->isClassType(types::canonical(objSrcType));
    std::string classFieldSrc1059;
    if (decl == nullptr && ownerIsClass1059) {
        classFieldSrc1059 =
            classFieldType(types::canonical(objSrcType), member->memberName).value_or("");
    }
    if (decl != nullptr) {
        bool fieldFound648 = false;
        for (const auto& f : decl->fields) {
            if (f.name == member->memberName) {
                targetType = mapType(f.type);
                fieldFound648 = true;
                break;
            }
        }
        if (!fieldFound648) {
            // 648 归档：错误恢复——字段名 miss 时上游语义层已报「结构体/类
            //   'X' 没有成员 'Y'」（Error），此处 i64 兜底仅为恢复路径继续
            //   发射，产物将被 driver 丢弃；非新增静默面。
        }
    } else if (!classFieldSrc1059.empty()) {
        targetType = mapType(classFieldSrc1059);
    } else if (!ownerIsClass1059) {
        // 648（miss 不可静默·110 同族）：对象结构体类型推导 miss（对象形态
        //   未覆盖·如嵌套成员目标 a.b.c = v）=合法程序内部缺陷，上游无错误
        //   短路，显形于此；driver 在 generate() 后 hasErrors() 统一中止，
        //   产物不落地。原 i64 宽写兜底保留（控制流零变更）。
        //   （类宿主的字段名 miss 落此之外=上游语义层已报「没有成员」的
        //   错误恢复，与结构体面 fieldFound648 同径——非新增静默面。）
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 成员赋值目标 '" + member->memberName +
                "' 所属结构体类型未解析（对象类型 '" + objSrcType +
                "'）——静默按整64写兜底已废除（#648）");
    }
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "", node->location);
    }
    // 字段地址 = 基址 + 偏移（FieldAddr；含空指针检查错误码3）
    ir::IRValue addr = lvalueAddress(node->target.get());
    if (memberStringFieldAssign(node, member, decl, objSrcType, addr, value)) return true;
    // 复合赋值（p.x += 1 等）
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                         "", node->location);
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            value = widenCompoundRhs(value, targetType, node->location);  // 331-a T51
            value = emitResult(opcode, {current, value}, targetType, "", node->location);
        }
    }
    emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
         node->location);
    lastExpr_ = value;
    return true;
}

// 对象结构体类型推导（成员目标·写路径）：597 根治（2026-10-14）——整体委托
//   memberObjStructType（973/1014/86-a/448/363/82-d 全形态单一实现）。
//   原为漂移简化副本（仅 标识符/下标/调用118/解引用208 四分支）——缺
//   MemberExpr 嵌套链递归等分支，嵌套成员链（盒.芯.里.a）objSrcType 空 →
//   memberGenericAssign targetType 落默认 i64 → StorePtr 恒 8B 宽写覆盖
//   相邻字段（asm 铁证 mov [rax],rcx vs 窄写 ecx·118 注释同病第三形态·
//   类字段链/局部链/泛型实例/this 直访四路全中）。重复实现漂移=病根结构：
//   读面 memberObjStructType 已全形态而写面副本 118/208 两轮各补一支仍
//   追不平——归一后新对象形态只补 memberObjStructType 一处。
// 1360 结果化：纯委托薄包装随族改（无值=无法推导）。
cn::Optional<std::string> IRGenerator::memberAssignObjSrcType(MemberExpr* member) {
    // 622 接棒 rebase 裁决（2026-10-14·合并两修）：保留 597 整体委托归一
    //   （单一实现·新对象形态只补 memberObjStructType 一处），弃深度机分支
    //   的漂移副本升级版——其 Deref 面增强（derefPointeeSrcType 递归剥层）
    //   已传导进 memberObjStructType（597 归一面），委托链自动获得全形态。
    return memberObjStructType(member);
}

// 字符串字段写入归一化（成员目标；原 247~300 段）：79-a 模型——目标字段独立拥有：
//   来源分级归一化（转移/接管/驻留/复制落堆）+ 旧值幂等释放（联合体目标跳过，164-a）。
bool IRGenerator::memberStringFieldAssign(AssignmentExpr* node, MemberExpr* member,
                                          const StructDecl* decl,
                                          const std::string& objSrcType,
                                          const ir::IRValue& addr,
                                          const ir::IRValue& value) {
    // 72-a 根治（2026-09-11 第七十二轮）：结构体字段=x 目标字段类型为
    //   字符串 且右值为字符串变量 -> 值被外部槽持有（指针逃逸模型，与
    //   下标路径同款：数组元素借出=借用）。源变量污染退出 RAII——否则
    //   源出口释放 + 字段/后继容器浅共享 = 悬垂（v2 树 货舱解析 的
    //   `项.名称 = 键` → 依赖们.追加(项) 形态，E2E 165 回归实证）。
    // 79-a（2026-09-12 第七十九轮）：字符串字段写入归一化——取代 72-a 的
    //   「源污染」模型（浅拷共享：源不释放=泄漏 + 字段释放即悬垂）。新模型=
    //   字段独立拥有：来源按 74-a 同款分级归一化（转移=move／拥有返回=接管／
    //   字面量=驻留／其余=复制落堆），目标为拥有槽时先释放旧值（幂等清槽）——
    //   源保持拥有并照常释放，零共享、零悬垂、零泄漏。
    if (!isCompoundAssignOp(node->op)) {
        const std::string fieldFieldType = [&]() -> std::string {
            if (decl != nullptr) {
                for (const auto& f : decl->fields) {
                    if (f.name == member->memberName) return f.type;
                }
            }
            // 类实例字段（对象.字段，class 路径）
            if (semantic_ != nullptr && !objSrcType.empty() &&
                semantic_->isClassType(types::canonical(objSrcType))) {
                // 1360 结果化：value_or("") 显形（miss 落末尾 return "" 原语义）
                const std::string ft = classFieldType(
                    types::canonical(objSrcType), member->memberName).value_or("");
                if (!ft.empty()) return ft;
            }
            return "";
        }();
        // #1674 乙案：手动释放<T> 字符串载荷字段同走来源归一化（新值复制落堆/
        //   接管——调用方拥有临时的结算面不知槽已接管，浅直存=临时 free 后槽
        //   悬垂·探针5 差值=1 实锤），但旧值释放跳过（覆盖即忘·ManuallyDrop
        //   ptr::write 语义·001 §5.6 ③——忘旧责任随声明归用户）。
        const std::string fieldCanon = types::canonical(fieldFieldType);
        const cn::Optional<std::string> wrapPayloadOpt =
            types::manuallyDroppedPayloadOf(fieldCanon);
        const bool strLikeField =
            types::canonicalEquals(fieldFieldType, "字符串") ||
            (wrapPayloadOpt.has_value() &&
             types::canonical(wrapPayloadOpt.value()) == "字符串");
        std::fprintf(stderr, "[PROBE]msfa ft=%s strLike=%d obj=%s\n",
                     fieldFieldType.c_str(), (int)strLikeField,
                     objSrcType.c_str());
        if (strLikeField) {
            // 顺序（自赋值安全）：值已由上方 genExpr 求值（勿二次求值——
            //   二次求值=多余分配泄漏，P6/P7 探针实证）；此处按来源分级
            //   归一化（复制落堆/接管/move），新值独立于旧值内存后才 drop 旧值
            ir::IRValue normalized = normalizeStringValueSource(
                node->value.get(), value, node->location);
            std::string objUnique;
            if (member->object->getType() == NodeType::IdentifierExpr) {
                // 1360 结果化：value_or("") 显形（未登记=空槽名原语义）
                objUnique = lookupVarName(
                    static_cast<IdentifierExpr*>(member->object.get())->name).value_or("");
            }
            // 164-a（A4·plans/023 §十二 方案A/D）：**目标对象类型为联合体**时
            //   跳过旧值释放——联合体成员共享偏移+无 tag，「旧值」身份不可判
            //   （把非活跃成员位当句柄 free=p13/p14 缺陷根因）；方案A 删除写入
            //   归一化，旧值释放=用户责任（手动释放 标注成员在 不安全 函数 内
            //   显式处理）。新值写入/来源归一化保留（值仍须可写）。
            // #1674 乙案：手动释放<T> 目标槽旧值释放跳过（覆盖即忘——级联
            //   跳过+忘旧同判据，释放由用户 摘除(槽.值) 执行）。
            const StructDecl* objDeclU = semantic_ != nullptr
                ? semantic_->findStruct(types::canonical(objSrcType)).value_or(nullptr)
                : nullptr;
            const bool unionTarget = objDeclU != nullptr && objDeclU->isUnion;
            if (!unionTarget && !wrapPayloadOpt.has_value() &&
                isOwnedFieldSlot(objUnique)) {
                emitFieldStringFreeAt(addr, node->location);  // 旧值幂等释放+清槽
            }
            emit(ir::Opcode::StorePtr, {addr, normalized}, ir::IRValue(), "",
                 "ptr", node->location);
            lastExpr_ = normalized;
            return true;
        }
    }
    return false;
}

// 629（544 呈报·成员目标面·自 handleClassMemberAssign 974 段挂点）：
//   类字段 ←「返回类对象的调用」产物（盒.槽 = 仓.产() / 盒.槽 = 货架()）
//   ——原路径落标量 StorePtr 直存新句柄：字段旧对象（连同资源字段）无人
//   释放=泄漏（探针矩阵双侧实锤宿主残留 +2）。字段=句柄槽（728x 语义）：
//   旧句柄单发 DeleteObject（095/821 同款·空句柄空安全）+ 新句柄 StorePtr
//   入槽（fieldAddr 已由调用方经 handleClassMemberLvalue 算好=单一事实源）。
//   自用右值（盒.槽 = 盒.槽.克隆()）安全：先求值后释放。排除：转移（转移
//   展开=源槽清零在 ir_call·成员转移面维持原路径）/引用返回（借用产物不得
//   释放/接管）。容器族字段由 memberClassFieldAssign 前置处理不达此。
//   返回 true=已处理；false=非该形态（零行为变更落原路径）。
// 915-b 配合（095 覆盖前 drop 旧值面补齐）：判定/发射分离——判定=纯查询
//   零发射（915-b 裸字段挂点据此先判后算目标地址·防 lvalueAddress 空检查
//   白发射），发射=求值+旧壳释放+新句柄入槽（成员/裸字段两挂点共用）。
bool IRGenerator::canMemberClassCallProductAssign(Expr* valueExpr,
                                                  const std::string& fieldCanon) {
    // 静默豁免业务值: 1360 拆分随迁（存量守卫：调用方契约+can 判定先行）非兜底
    if (semantic_ == nullptr || valueExpr == nullptr) return false;
    if (valueExpr->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(valueExpr);
    if (isTransferCall(call)) return false;
    if (call->isRefReturnCall) return false;
    // 右值调用返回类型严格等于字段类（异型/引用返回后缀自然不等=保守面）
    // 1360 结果化：value_or("") 显形（无法推导=不等→false 原语义）
    return types::canonical(exprSrcType(valueExpr).value_or("")) == fieldCanon;
}

bool IRGenerator::memberClassCallProductAssign(Expr* valueExpr,
                                               const ir::IRValue& fieldAddr,
                                               const std::string& fieldCanon,
                                               const SourceLocation& loc) {
    if (!canMemberClassCallProductAssign(valueExpr, fieldCanon)) return false;
    // 源求值（产物=新堆对象句柄——所有权随返回值移交调用方）
    ir::IRValue newObj = genExpr(valueExpr);
    // 旧字段对象释放（空句柄空安全）+ 新句柄入槽
    ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr", "",
                                    loc);
    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), fieldCanon, "void",
         loc);
    emit(ir::Opcode::StorePtr, {fieldAddr, newObj}, ir::IRValue(), "", "ptr",
         loc);
    lastExpr_ = fieldAddr;
    return true;
}

} // namespace cn_compiler
