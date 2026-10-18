// CN-IR生成器——阶段3 OOP（D1 行数整改 113-a：自 ir_oop.cpp 按族拆出）
//   族 = 类字段访问（classFieldType + memberFieldSrcType + genInstanceFieldAddr + genStaticFieldAddr + isInstanceField + handleClassFieldRead + handleClassMemberExpr + emitClassFieldDefaults）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp）。
//   861（先拆再加）：handleClassFieldAssign/handleClassFieldIncDec（类字段赋值/
//   自增减族）已迁至 ir_oop_field_assign.cpp（机械搬迁零行为变更）。
//   915-b（任务 914）：handleClassMemberLvalue/classMemberBasePtr/handleClass-
//   MemberAssign（类成员左值地址与赋值族）已迁至 ir_expr_assign_ident_class.cpp
//   （机械搬迁零行为变更）。
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 查询类字段源码类型（沿继承链；未找到返回空串）
std::string IRGenerator::classFieldType(const std::string& className,
                                        const std::string& fieldName) const {
    if (semantic_ == nullptr) return "";
    const ClassInfo* ci = semantic_->findClass(types::canonical(className));
    while (ci != nullptr) {
        auto f = ci->fields.find(fieldName);
        if (f != ci->fields.end()) return f->second.type;
        ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
    }
    return "";
}

// 成员字段源码类型（结构体/类对象统一；宿主缺陷1'根治 2026-09-02）：
//   下标 MemberExpr 分支原只查 StructDecl（类对象查不到 -> 步长兜底 8），
//   致 拷贝构造 其他.数据[索引]（T*>8字节）读源错位（p9 实证重叠写入形态）。
std::string IRGenerator::memberFieldSrcType(MemberExpr* node) const {
    if (semantic_ == nullptr || node == nullptr) return "";
    const std::string objType = memberObjStructType(node);
    if (objType.empty()) return "";
    const std::string canon = types::canonical(objType);
    const StructDecl* decl = semantic_->findStruct(canon);
    if (decl != nullptr) {
        for (const auto& f : decl->fields) {
            if (f.name == node->memberName) return f.type;
        }
        return "";
    }
    if (semantic_->isClassType(canon)) {
        return classFieldType(canon, node->memberName);
    }
    return "";
}

// 缺陷3 根治（2026-09-02）：类字段级联析构注入（571 轮自 ir_oop.cpp 机械搬移——
//   该文件 1036 行过 1000 冻结线·「先拆再加」纪律；搬移=纯机械零行为变更，
//   571 盒载荷字段清理注入随体·字段族归属=本文件 classFieldType/handleClassField* 同域）——~类名() 体生成后、默认返回前，
//   按字段逆序对本类「有析构类」字段发射 DeleteObject（对标 C++ 成员析构语义）：
//     this 指针 → FieldAddr(字段偏移) → LoadPtr（字段槽存堆对象指针）→ DeleteObject
//   （空安全：未构造字段经 __cn_object_new 清零为 null，codegen test/je 跳过——
//   原 CN 无字段析构原语，容器字段只能泄漏，stdlib 全库被迫用裸指针+分配/释放 规避）。
//   边界：父类字段由父类析构链负责；析构体内提前 返回 不覆盖（C++ 同语义不保证）；
//   静态字段与无析构类字段不注入。
void IRGenerator::injectFieldCascadeDestroy(const ClassMember* member) {
    if (semantic_ == nullptr || member == nullptr) return;
    ir::IRFunction* fn = function_;
    if (fn == nullptr || fn->blocks.empty()) return;
    ir::IRBlock* last = fn->blocks.back().get();
    if (last->terminated) return;  // 体尾已终止（提前 返回）：不注入（见上边界）
    setCurrentBlock(last);
    const ClassInfo* ci = semantic_->findClass(currentClass_);
    if (ci == nullptr) return;
    const std::string thisUnique = lookupVarName("自身");
    if (thisUnique.empty()) return;
    for (auto it = ci->fieldOrder.rbegin(); it != ci->fieldOrder.rend(); ++it) {
        const auto f = ci->fields.find(*it);
        if (f == ci->fields.end() || f->second.isStatic) continue;
        const std::string ftype = classFieldType(currentClass_, *it);
        const std::string fcanon = types::canonical(ftype);
        // 100（939·008 收官总攻）：**字符串字段清理注入**——结构体串字段
        //   三件套（79-a）对类不对称缺失（类字段串=编译器不管→tracked 串
        //   泄漏·p104 定性：副本串差 1；驻留句柄/空句柄 free=忽略/空安全）。
        //   纯增益：类串字段无既有自动清理路径（容器字段=级联析构在下）。
        if (fcanon == "字符串") {
            const int offS = semantic_->classFieldOffset(currentClass_, *it);
            if (offS >= 0) {
                ir::IRValue thisS = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(thisUnique, "ptr")},
                    "ptr", thisUnique, member->body->location);
                ir::IRValue addrS = emitResult(
                    ir::Opcode::FieldAddr, {thisS}, "ptr",
                    std::to_string(offS), member->body->location);
                ir::IRValue strPtr = emitResult(
                    ir::Opcode::LoadPtr, {addrS}, "ptr", "",
                    member->body->location);
                emit(ir::Opcode::Call, {strPtr}, ir::IRValue(),
                     "__cn_str_free", "void", member->body->location);
                emit(ir::Opcode::StorePtr, {addrS, ir::IRValue::constant("0", "i64")},
                     ir::IRValue(), "", "ptr", member->body->location);
            }
            continue;
        }
        // 571（2026-10-14·#571 连带补齐）：**盒载荷字段清理注入**——可选<
        //   字符串>/结果<字符串,整32>（合成体）字段原不在此级联（仅 字符串/
        //   类字段 两分支）→ tracked 载荷泄漏（904 形四五六 残留位实证：深拷
        //   副本差 1——与字符串字段 939 同款不对称）。收集器驱动条件释放
        //   （ownedStrFieldsOf→#280 载荷真值守卫→emitFieldStringFreeIf 单调用
        //   ：tag 假=错误态垃圾句柄免疫·真=摘句柄+free+清槽幂等）；值≠字符串
        //   的合成体收集空=零注入（与无串字段类零开销同口径）。
        {
            const std::string fcanonNorm = canonicalizeSyntheticArgText(fcanon);
            if (fcanonNorm.rfind("可选$", 0) == 0 ||
                fcanonNorm.rfind("结果$", 0) == 0) {
                const int offB = semantic_->classFieldOffset(currentClass_, *it);
                if (offB >= 0 && !ownedStrFieldsOf(ftype).empty()) {
                    ir::IRValue thisB = emitResult(
                        ir::Opcode::Load, {ir::IRValue::var(thisUnique, "ptr")},
                        "ptr", thisUnique, member->body->location);
                    ir::IRValue baseB = emitResult(
                        ir::Opcode::FieldAddr, {thisB}, "ptr",
                        std::to_string(offB), member->body->location);
                    emitOwnedStrFieldFreesAt(baseB, ftype,
                                             member->body->location);
                }
                continue;
            }
        }
        if (ftype.empty() || !semantic_->isClassType(fcanon)) continue;
        const ClassInfo* fci = semantic_->findClass(fcanon);
        bool hasDtor = false;
        if (fci != nullptr) {
            for (const auto& mk : fci->methods) {
                if (mk.second.isDestructor) { hasDtor = true; break; }
            }
        }
        // 658②（2026-10-14·#658）：无析构类字段不再跳过——C++ 隐式析构完整
        // 语义（外层有无用户析构器≠字段不析构）：递归 emitNoDtorFieldCascade
        // （字符串/盒载荷/类字段级联·v2 发射资源字段释放于地址 用户类分支
        // 同轮同案）。原 continue=嵌套无析构类字段 tracked 泄漏面。
        if (!hasDtor) {
            const int offN = semantic_->classFieldOffset(currentClass_, *it);
            if (offN >= 0) {
                ir::IRValue thisN = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(thisUnique, "ptr")},
                    "ptr", thisUnique, member->body->location);
                ir::IRValue addrN = emitResult(
                    ir::Opcode::FieldAddr, {thisN}, "ptr",
                    std::to_string(offN), member->body->location);
                ir::IRValue innerN = emitResult(ir::Opcode::LoadPtr, {addrN},
                                                "ptr", "", member->body->location);
                emitNoDtorFieldCascade(innerN, fcanon, member->body->location, 1);
            }
            continue;
        }
        const int offset = semantic_->classFieldOffset(currentClass_, *it);
        if (offset < 0) continue;
        ir::IRValue thisPtr = emitResult(
            ir::Opcode::Load, {ir::IRValue::var(thisUnique, "ptr")},
            "ptr", thisUnique, member->body->location);
        ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {thisPtr}, "ptr",
                                      std::to_string(offset), member->body->location);
        ir::IRValue objPtr = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                                        member->body->location);
        emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(),
             fcanon, "void", member->body->location);
    }
}

// 658②（2026-10-14·#658）：**无析构类**出口 RAII 字段级联释放——C++ 隐式析构
//   同语义：类无用户析构器时，其字段资源仍须出口释放（宿主同病 差=1 实证：
//   无析构类字符串字段 函数出口 tracked 泄漏——genBlockExitDestruct/genJump
//   共用的 emitClassDeleteFor 只 DeleteObject 释放对象本体、genClassDestructor
//   Calls 更是对无析构类整只跳过）。发射面与 injectFieldCascadeDestroy 同域
//   （this→FieldAddr→释放）但接收任意对象指针（出口点=变量槽 Load 出的堆指针）：
//     · 字符串字段 → emitFieldStringFreeAt（free+清槽幂等）；
//     · 盒载荷字段（可选/结果·T=字符串）→ emitOwnedStrFieldFreesAt（571 收集器
//       驱动条件释放·__cn_box_class_delete 摘句柄错误态免疫+清槽幂等）；
//     · 类字段（有析构）→ LoadPtr+DeleteObject（其析构体已含自身字段级联）
//       +清槽；类字段（无析构）→ 递归（深度限 4 防环·对齐 v2 发射资源字段
//       释放于地址 同款上限）。
//   容器字段不注入（宿主有析构类的 injectFieldCascadeDestroy 同缺=既有面·
//   两侧同病不单侧扩权，呈报台账另行清偿）。有析构类调用方不进（勿双释放）。
void IRGenerator::emitNoDtorFieldCascade(const ir::IRValue& objPtr,
                                         const std::string& classCanon,
                                         const SourceLocation& loc, int depth) {
    if (semantic_ == nullptr || depth > 4) return;
    const ClassInfo* ci = semantic_->findClass(types::canonical(classCanon));
    if (ci == nullptr) return;
    for (auto it = ci->fieldOrder.rbegin(); it != ci->fieldOrder.rend(); ++it) {
        const auto f = ci->fields.find(*it);
        if (f == ci->fields.end() || f->second.isStatic) continue;
        const std::string ftype = classFieldType(types::canonical(classCanon), *it);
        if (ftype.empty()) continue;
        const std::string fcanon = types::canonical(ftype);
        const int offset = semantic_->classFieldOffset(
            types::canonical(classCanon), *it);
        if (offset < 0) continue;
        ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {objPtr}, "ptr",
                                      std::to_string(offset), loc);
        // 字符串字段：free+清槽（幂等——块出口/函数级兜底多路径共享槽）
        if (fcanon == "字符串") {
            emitFieldStringFreeAt(addr, loc);
            continue;
        }
        // 盒载荷字段（可选/结果·T=字符串）：收集器驱动条件释放（571 单点）
        {
            const std::string fcanonNorm = canonicalizeSyntheticArgText(fcanon);
            if (fcanonNorm.rfind("可选$", 0) == 0 ||
                fcanonNorm.rfind("结果$", 0) == 0) {
                if (!ownedStrFieldsOf(ftype).empty()) {
                    emitOwnedStrFieldFreesAt(addr, ftype, loc);
                }
                continue;
            }
        }
        // 类字段：有析构=DeleteObject（其体内级联自含）；无析构=递归级联
        if (ftype.empty() || !semantic_->isClassType(fcanon)) continue;
        if (classDestructorSymbolKey(fcanon).empty()) {
            ir::IRValue innerPtr = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr",
                                              "", loc);
            emitNoDtorFieldCascade(innerPtr, fcanon, loc, depth + 1);
            continue;
        }
        ir::IRValue objFieldPtr = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr",
                                             "", loc);
        emit(ir::Opcode::DeleteObject, {objFieldPtr}, ir::IRValue(), fcanon,
             "void", loc);
        ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
        emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "", "ptr", loc);
    }
}

// ==================== 实例/静态字段地址 ====================

// 生成实例字段地址：this 指针（Load 自身参数槽）+ FieldAddr（类字段偏移）。
// 语义层 classFieldOffset 已含虚表指针偏移调整（实例首地址起算）。
ir::IRValue IRGenerator::genInstanceFieldAddr(const std::string& fieldName,
                                              const SourceLocation& loc) {
    const std::string unique = lookupVarName("自身");
    ir::IRValue thisPtr;
    if (!unique.empty() && !currentMethodStatic_) {
        thisPtr = emitResult(ir::Opcode::Load,
                             {ir::IRValue::var(unique, "ptr")},
                             "ptr", unique, loc);
    } else {
        thisPtr = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", loc);
    }
    const int offset = (semantic_ != nullptr)
        ? semantic_->classFieldOffset(currentClass_, fieldName) : -1;
    if (offset < 0) return thisPtr;
    return emitResult(ir::Opcode::FieldAddr, {thisPtr}, "ptr",
                      std::to_string(offset), loc);
}

// 生成静态字段地址：ConstString 常量携带 ?static_类名_字段名 链接符号。
// codegen emitConstLoad 对 ConstString 生成 lea rax, 符号（符号原样输出），
// 结果 = 静态字段地址（.data 段全类共享）。
ir::IRValue IRGenerator::genStaticFieldAddr(const std::string& className,
                                            const std::string& fieldName,
                                            const SourceLocation& loc) {
    return emitResult(ir::Opcode::ConstString, {}, "ptr",
                      "?static_" + className + "$" + fieldName, loc);
}

// 名称是否为当前类的实例字段（非静态方法内、未被局部变量/参数遮蔽）
// 判定：当前方法非静态 且 名称在类字段表（沿继承链）且 不在当前方法作用域。
// 说明：语义层把类字段并入方法作用域（checkClassMethods declareVar），
//   IR 层 setupMethodParams 未并入字段，故用"不在 varStack_ 但命中字段表"判定。
bool IRGenerator::isInstanceField(const std::string& name) const {
    if (currentMethodStatic_ || currentClass_.empty()) return false;
    if (semantic_ == nullptr) return false;
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        if (it->find(name) != it->end()) return false;
    }
    return !classFieldType(currentClass_, name).empty();
}

// ==================== 方法体内直接字段访问 ====================

// 方法体内直接字段读取（无 自身. 前缀）：
//   静态字段（currentClass_ 的静态字段）-> 静态字段符号地址 + LoadPtr；
//   实例字段 -> this + FieldAddr(偏移) + LoadPtr（字段 IR 类型）
bool IRGenerator::handleClassFieldRead(IdentifierExpr* node) {
    // 静态字段直接访问（静态方法/普通方法内均可，Task 3.9）：字段名在类字段表且 isStatic
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(node->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                ir::IRValue addr = genStaticFieldAddr(currentClass_, node->name,
                                                      node->location);
                if (types::isArray(fieldType)) {
                    lastExpr_ = addr;  // 数组字段退化：返回地址
                    return true;
                }
                lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                                       mapType(fieldType), "", node->location);
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(node->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, node->name);
    ir::IRValue addr = genInstanceFieldAddr(node->name, node->location);
    // 数组字段退化：返回字段地址（供 字段[i] 下标访问）
    if (types::isArray(fieldType)) {
        lastExpr_ = addr;
        return true;
    }
    // A-4（2026-08）：类类型字段读取返回"对象指针"（LoadPtr）而非字段地址——
    //   字段存对象指针，方法调用（字段.方法()）与值读取（类变量赋值）需要指针
    //   本身；此前返回字段地址导致 向量 类字段的 追加/元素 以字段地址为 this，
    //   实例字段读偏移全部错位（追加 分配失败/元素 读垃圾，52_library 边界复现）
    if (semantic_ != nullptr &&
        semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                               node->location);
        return true;
    }
    // 106（890 残余·泛型方法实参面·连带根治）：**聚合字段（结构体/结果/可选）
    //   值读 → 返回字段地址（值语义）**——方法内直访路径（`返回 值槽`/`局=值槽`）
    //   原落行末 LoadPtr：字段内联值首 8 字节被读作「值/地址」（D9 汇编铁证
    //   `mov r9,[r10];mov r10,r9;mov r9,[r10+0]`——返回位把它当地址再解引用，
    //   段错误 rc=139）；与 handleClassMemberExpr / visitMemberExpr 结构体字段
    //   分支（缺陷根治 2026-08-25 判例）同款值语义。双形态归一（模板文本
    //   可选<整32> → $ 合成名 可选$整32·lowerResultOptionalTypes 注册形态）。
    if (semantic_ != nullptr) {
        const std::string canonFieldType = canonicalizeSyntheticArgText(
            types::canonical(fieldType));
        if (semantic_->isStructType(canonFieldType) ||
            isResultType(canonFieldType) || isOptionalType(canonFieldType)) {
            lastExpr_ = addr;
            return true;
        }
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                           mapType(fieldType), "", node->location);
    return true;
}

// ==================== 类成员表达式（读取） ====================

// 类字段读取（visitMemberExpr 钩子）：
//   静态字段（类名.字段）：静态字段地址 + LoadPtr
//   实例字段（对象.字段 / 自身.字段 / 父类.字段）：
//     对象指针（自身 -> this；类变量 -> 变量值（对象指针））+ FieldAddr + LoadPtr
bool IRGenerator::handleClassMemberExpr(MemberExpr* node) {
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(node->object.get())->name;
    }
    const std::string objSrcType = memberObjectSrcType(node->object.get());  // 622：支持 (*类指针)
    const std::string canonObj = types::canonical(objSrcType);

    // P3-23：静态方法作值（类名.静态方法 用作函数指针值）→ FuncAddr 方法链接符号
    if (!objName.empty() && semantic_->isClassType(objName)) {
        const ClassInfo* ciStatic = semantic_->findClass(objName);
        if (ciStatic != nullptr) {
            auto smit = ciStatic->methods.find(node->memberName);
            if (smit != ciStatic->methods.end() && smit->second.isStatic) {
                lastExpr_ = emitResult(
                    ir::Opcode::FuncAddr, {}, "ptr",
                    methodSymbolKey(ciStatic->name, smit->second.sigKey),
                    node->location);
                return true;
            }
        }
    }

    // ---- 静态字段：类名.字段（对象是类类型名，非变量） ----
    if (!objName.empty() && semantic_->isClassType(objName)) {
        const std::string fieldType = classFieldType(objName, node->memberName);
        if (fieldType.empty()) return false;  // 非字段（静态方法等，交给调用钩子）
        ir::IRValue addr = genStaticFieldAddr(objName, node->memberName, node->location);
        if (types::isArray(fieldType)) {
            lastExpr_ = addr;  // 数组字段退化：返回地址
            return true;
        }
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                               mapType(fieldType), "", node->location);
        return true;
    }

    // 031（#1075·2026-10-18 用户批·一等公民化）：`对象.实例方法` 求值=绑定方法值
    //   ——栈上两槽 {this, code}（值语义·零堆分配·零运行期检查·plans/031 §二·①）。
    //   表达式通道物化为临时两槽变量（__mvN），消费点（声明位/赋值位/实参位/
    //   调用位）按「一值=两位」单点读取（ir_method_value.cpp·031 风险表第一条）。
    //   旧 943 蹦床闭包通道已废弃（plans/031 §二·③⑥）：此前的「蹦床+静态槽 +
    //   编译期旁路（closureInfo_ 快径）」在转发语境丢绑定（p82d 段错误病灶），
    //   两槽值随值自带对象引用——转发/拷贝/跨函数天然安全，同帧快径不再是
    //   正确性依据。code 槽=方法链接符号；虚方法值=创建时快照（lookupClassMember
    //   从对象实际类沿链解析——对象类终生不变=与运行期分派恒等价，§二·①④）。
    if (node->isMethodValue && semantic_->isClassType(canonObj)) {
        std::string owner;
        const ClassMemberInfo* method =
            semantic_->lookupClassMember(canonObj, node->memberName, owner);
        if (method != nullptr && !method->isStatic) {
            const std::string ownerSym = owner.empty() ? canonObj : owner;
            const std::string methodSym = methodSymbolKey(ownerSym, method->sigKey);
            // this 槽：对象指针求值（标识符/自身/成员链/指针形态——classMemberBasePtr
            //   622 统一助手）；code 槽：方法符号地址。两槽物化=单点发射。
            ir::IRValue objPtr = classMemberBasePtr(node->object.get());
            ir::IRValue codeVal = emitResult(ir::Opcode::FuncAddr, {}, "ptr",
                                             methodSym, node->location);
            const std::string temp =
                emitMethodValuePair(objPtr, codeVal, node->location);
            lastExpr_ = ir::IRValue::var(temp, "ptr");
            return true;
        }
    }

    // ---- 实例字段：对象为类实例（源码类型是类或类指针——v2.1 . 自动解引用一级） ----
    // 588-a（005〔原B-T81〕甲案）：类指针对象（`点* p2 = &p1; p2.x`）剥指针取
    //   类名（与 emitInstanceMethodCall 类型驱动剥法同款·泛型合成名尾 * 先判
    //   类直接用）——原不识别指针形态 → findStruct 失败 → 字段读降级常量 0
    //   （静默错值·probe3 实锤）。
    std::string canonObjField = canonObj;
    if (!semantic_->isClassType(canonObjField) && types::isPointer(canonObjField)) {
        canonObjField = types::canonical(types::pointeeOf(canonObjField));
    }
    if (!semantic_->isClassType(canonObjField)) return false;
    const std::string fieldType = classFieldType(canonObjField, node->memberName);
    if (fieldType.empty()) return false;
    ir::IRValue base = classMemberBasePtr(node->object.get());  // 对象指针（622 统一助手·内含 728g 指针下标分支）
    const int offset = semantic_->classFieldOffset(canonObjField, node->memberName);
    if (offset < 0) return false;
    ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                  std::to_string(offset), node->location);
    // 数组字段退化：返回字段地址（供 字段[i] 下标访问）
    if (types::isArray(fieldType)) {
        lastExpr_ = addr;
        return true;
    }
    // 宿主缺陷1根治（2026-09-02）：类类型字段读取返回"对象指针"（LoadPtr）——
    //   对齐方法内直访路径（handleClassFieldRead 的 A-4 语义）：字段槽存堆对象
    //   指针，链式字段.方法()（e.表.大小()）的 this 与按值传参需要指针本身；
    //   原返回字段地址致 this=槽地址（p2 空指针崩溃、p6 其他.表.大小() 静默 0
    //   -> 深拷贝失效）。下标访问（其他.数据[索引]）走 lvalueAddress/visitIndexExpr
    //   的成员路径，不受影响。
    if (semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                               node->location);
        return true;
    }
    // 106（890 残余·泛型方法实参面·连带根治）：**聚合字段（结构体/结果/可选）
    //   值读 → 返回字段地址（值语义）**——与 visitMemberExpr 结构体字段分支
    //   （缺陷根治 2026-08-25 判例）同款。原路径落行末 LoadPtr：字段内联
    //   值首 8 字节被读作「值/地址」（类方法「返回 值槽」/「局=值槽」→ 被调
    //   方把字段首 8 字节当指针再解引用——D9/D1b 汇编铁证
    //   `mov r9,[r10];mov r10,r9;mov r9,[r10+0]`，段错误 rc=139；宿主
    //   D1/D2/D6/D9/D10 探针族实证）。值语义下调用方（返回位 CopyStruct/
    //   声明位拷贝）均按地址源消费，与本分支返回 addr 衔接。
    {
        // 双形态归一：模板文本（可选<整32>）→ $ 合成名（可选$整32·lowerResult
        //   OptionalTypes 注册形态），findStruct 单键命中；结构体名原样。
        const std::string canonFieldType = canonicalizeSyntheticArgText(
            types::canonical(fieldType));
        if (semantic_->isStructType(canonFieldType) ||
            isResultType(canonFieldType) || isOptionalType(canonFieldType)) {
            lastExpr_ = addr;
            return true;
        }
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                           mapType(fieldType), "", node->location);
    return true;
}
// 775（甲案·2026-10-15 用户裁决）：实例字段默认值注入——layerClass 的本层字段
//   （ownerClass==layerClass）逐个写 defaultInitText（语义层 468 同构面折叠：
//   整数规范十进制文本/布尔 "1"/"0"）。偏移按 objClass 布局查（fieldOffset
//   沿继承链·含虚表/接口分派区调整——派生对象上写基类层字段时 objClass=派生类）；
//   写宽=字段 IR 类型（窄字段按宽写·emitPtrLoadStore 宽度分派）。声明序输出
//   （fieldOrder 保序·确定性）。显式构造赋值在构造体内随后覆盖（C++ NSDMI 语义）。
void IRGenerator::emitClassFieldDefaults(const std::string& layerClass,
                                          const std::string& objClass,
                                          const ir::IRValue& objPtr,
                                          const SourceLocation& loc) {
    if (semantic_ == nullptr) return;
    const ClassInfo* ci = semantic_->findClass(layerClass);
    if (ci == nullptr) {
        // 648（miss 不可静默·110 同族）：调用点=类创建路径（上游语义已保证
        //   类已注册）——此 miss=内部缺陷显形（与 v2 登记内部未登记 同构）；
        //   driver hasErrors() 统一中止·产物不落地。原静默 return=漏发默认值
        //   对象字段错值（648 红线本面）。
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类字段默认值注入·类未登记 '" + layerClass + "'（#648）");
        return;
    }
    for (const std::string& fn : ci->fieldOrder) {
        auto it = ci->fields.find(fn);
        if (it == ci->fields.end()) continue;
        const ClassMemberInfo& fm = it->second;
        if (fm.isStatic || fm.defaultInitText.empty()) continue;
        if (!fm.ownerClass.empty() && fm.ownerClass != layerClass) continue;
        const std::string irType = mapType(fm.type);
        if (irType.empty()) continue;   // 648：类型未登记面在字段访问处显形·注入保守跳过
        const int offset = semantic_->classFieldOffset(objClass, fn);
        if (offset < 0) continue;
        ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {objPtr}, "ptr",
                                      std::to_string(offset), loc);
        ir::IRValue val = emitResult(ir::Opcode::ConstInt, {}, irType,
                                     fm.defaultInitText, loc);
        emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", irType, loc);
    }
}

} // namespace cn_compiler
