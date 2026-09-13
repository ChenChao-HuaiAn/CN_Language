// CN-IR生成器——阶段3 OOP（D1 行数整改 113-a：自 ir_oop.cpp 按族拆出）
//   族 = 容器<T> 元素自动析构注入（Feature 2 完整版：injectContainerElemDestroy + containerElemTypeOf + isOwnedStrFieldElemContainer + emitContainerElemWalk + emitSingleElemRelease）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp）。
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 容器<T> 元素自动析构注入（Feature 2 完整版） ====================

// Feature 2 完整版（2026-08-25）：容器<T>（向量/链表/栈/队列）元素自动析构——
//   编译器级注入，使容器自动管理内联类元素生命周期（对标 C++ STL 容器析构语义）。
//   容器 持有内联类元素（T* 数据/值表）。当实例化元素 T 为有析构类（如 映射$整64$符号）
//   时：
//     ~类名()/清空()  -> 注入全量元素析构循环（销毁 数组[0..元素数量) 后原体释放数组）
//     向量 删除(位置) -> 注入单元素析构（守卫 位置<元素数量 后销毁 数据[位置]）
//     链表 删除头部/删除尾部 -> 注入单元素析构（守卫 索引<0 后销毁 值表[头/尾索引]）
//     栈 弹出 / 队列 出队  -> 返回元素（所有权转移给调用方），不析构
//   元素析构 = Call T$析构函数（this=元素内联地址）——析构方法释放元素自身堆资源
//   （如 映射 的 键/值/链/桶 数组），元素内存本身由容器数组持有（释放(数据) 归还）。
//   效果：作用域栈 弹出 只需 删除(末尾) 即自动销毁 映射 元素（原需显式调
//   stdlib 释放内部数组()）；任意 容器<有析构类> 的 清空/销毁 均自动清理元素。
//   无析构类元素（标量/结构体）不注入（无资源需释放，保持原语义）。
// 注入时机：emitClassMethod 生成 容器$T 方法体前（入口块之后、原方法体之前）。
void IRGenerator::injectContainerElemDestroy(const std::string& className,
                                             const ClassMemberInfo& mi,
                                             const SourceLocation& loc) {
    if (semantic_ == nullptr || function_ == nullptr) return;
    const std::string canonClass = types::canonical(className);
    // ---- 75-a（2026-09-12 第七十五轮）：映射<K,V> 字符串键/值释放 ----
    //   缺口（探针实证，矩阵 A17 同族）：映射的 ~映射/清空 只释放四个内部数组
    //   （键/值/链/桶），不释放**字符串键/值本身**=每个字符串元素永久泄漏；
    //   嵌套场景（向量<映射<…,字符串>>）同样由本注入覆盖（元素的析构经 ~映射）。
    //   元素模型：映射删除用「与末元素交换」→ 有效元素恒为 键/值数组的
    //   [0, 元素数量) → 平铺释放安全（同 向量/栈 模型）。仅当 K/V 至少一侧为
    //   字符串时注入（其它类型无堆资源）。
    //   时机：~映射/清空/释放内部数组 方法体前（清空的 元素数量=0 在方法体内
    //   ——先释放后归零；释放内部数组 的数组释放也在方法体内——先释放字符串
    //   后释放数组，顺序不可倒）。
    // ---- 76-a（2026-09-12 第七十六轮）：映射三路径补齐（探针 76 实证）----
    //   ① ~映射/清空 → 全量平铺释放（75-a）；**释放内部数组** 同为「容器元素
    //      显式销毁」路径（H8-5），漏注入=字符串键/值泄漏（探针 76-D）——并入全量；
    //   ② **析构键值(索引)** ＝ 单槽释放（键+值）：映射.删除 在链摘除后、与
    //      末元素交换前调用（交换会覆盖被删槽——不先释放即泄漏，探针 76-A/B）；
    //   ③ **析构值(索引)** ＝ 单槽释放（仅值）：映射.设置 覆盖已有键时写入新值
    //      前调用（旧值句柄被覆盖即泄漏，探针 76-C；键保留=仍是有效元素）。
    //   K/V 为字符串时才注入（其余类型无堆资源，空体调用零开销）。
    if (canonClass.rfind("映射$", 0) == 0) {
        injectMapElemDestroy(canonClass, mi, loc);
        return;
    }
    // 容器识别 + 数组字段名（向量/栈=数据；链表/队列=值表；集合=数据数组）
    std::string arrayField;
    if (canonClass.rfind("向量$", 0) == 0) arrayField = "数据";
    else if (canonClass.rfind("链表$", 0) == 0) arrayField = "值表";
    else if (canonClass.rfind("栈$", 0) == 0) arrayField = "数据";
    else if (canonClass.rfind("队列$", 0) == 0) arrayField = "值表";
    else if (canonClass.rfind("集合$", 0) == 0) arrayField = "数据数组";  // 76-a
    else return;
    const std::string base = canonClass.substr(0, canonClass.find('$'));
    // 方法匹配：
    //   全量析构：~类名 / 清空
    //   单元素析构：
    //     向量 析构元素(索引)（编译器注入辅助，stdlib 删除 移位循环调用）-> 参数 索引
    //     链表 删除头部/删除尾部 -> 头/尾索引 字段
    //   向量 删除(位置)：移位槽泄漏已由 stdlib 显式 析构元素() 根治（move 语义），
    //     不再方法体前注入（被移除元素由 析构元素(移动=位置) 析构）。
    //   栈 弹出 / 队列 出队：所有权转移，不析构（不匹配即不注入）
    // ---- 76-a（2026-09-12 第七十六轮）：字符串元素路径（探针 76 实证）----
    //   字符串不是「有析构类」（内置类型），但元素是堆串（有堆资源）——原有注入
    //   只覆盖「T 有析构类」，字符串元素容器的移除路径全部静默泄漏：
    //   向量 删除(位置)/设置覆盖（探针 76-V1/V2）、链表 删除头部/删除尾部（76-L1/L2）、
    //   链表/队列 清空（76-L3/Q1）、集合 析构/清空/删除（76-E/S1）。
    //   本分支与「有析构类」路径一一对应：全量（~容器/清空）经 emitContainerElemFreeFor
    //   （按元素模型分派 __cn_vector/__cn_chain_free_strings）；单槽（析构元素/删除
    //   头部/删除尾部）经 __cn_seq_free_slot（释放 + 槽清零，幂等不变量）。
    const bool isFull = (mi.name == ("~" + base) || mi.name == "清空");
    bool isSingle = false;
    bool isMove = false;   // 81-a：元素移动挂点（移位循环单点，按元素类型注入）
    std::string indexParam;  // 单元素析构的索引参数名（析构元素=索引）；空=从字段
    std::string indexField;  // 单元素析构的索引字段名（链表 头索引/尾索引）；空=从参数
    if (mi.name == "析构元素") { isSingle = true; indexParam = "索引"; }
    else if (mi.name == "析构被移除") { isSingle = true; indexParam = "索引"; }
    else if (mi.name == "删除头部") { isSingle = true; indexField = "头索引"; }
    else if (mi.name == "删除尾部") { isSingle = true; indexField = "尾索引"; }
    else if (mi.name == "移动元素") { isMove = true; }   // 81-a：元素移动挂点
    if (!isFull && !isSingle && !isMove) return;
    // 76-a：字符串元素的单槽释放只服务「**唯一持有者**槽」（被移除槽）——
    //   析构被移除（向量/集合 删除前释放的被移除槽）、删除头部/删除尾部
    //   （链表 被摘槽：出链后无其他槽引用）；析构元素（移位目标槽/末尾移出槽）
    //   对字符串是**重复引用**（下标赋值=浅拷句柄），释放即悬垂（探针 76-V1
    //   修复首版实证）→ 不注入。有析构类元素反之（析构元素链负责，见下）。
    const bool strOwnedSlot = (mi.name == "析构被移除" ||
                               mi.name == "删除头部" || mi.name == "删除尾部");
    // 元素类型 T（emitClassMethod 已按本实例实参设置 genericTypeParams_）
    std::string elemType;
    auto tit = genericTypeParams_.find("T");
    if (tit != genericTypeParams_.end()) elemType = tit->second;
    if (elemType.empty()) return;
    const std::string elemCanon = types::canonical(elemType);
    // ---- 81-a：元素移动挂点（移位循环单点，所有元素类型统一注入）----
    //   注入：CopyStruct(数据[目标] ← 数据[源], 步长) + **源槽资源清零**——含拥有型
    //   串字段的元素：字段串句柄随 memcpy 移交目标槽，源槽清零使目标槽成为唯一
    //   持有者（Rust `ptr::copy` + 源失效同款，零拷贝）。无串字段元素 = 纯 memcpy
    //   （标量/字符串/类对象，零行为变化）。调用方（stdlib 移位循环）保证目标槽
    //   旧值已清（删除=被移除槽先行释放+清零；插入/前移=上一步移动已清零源槽）。
    //   背景（探针 P8 实证）：原移位写成 `数据[移动] = 数据[移动 + 1]` 走 79-a 的
    //   结构体整体赋值深拷（preFree + memcpy + __cn_str_copy）——既多一次分配，
    //   又把源槽句柄留在超范围槽（容器析构不遍历）=泄漏 1/元素。
    if (mi.name == "移动元素") {
        injectMoveElemDestroy(canonClass, mi, loc, arrayField, elemCanon);
        return;
    }
    // ---- 字符串元素：单槽/全量释放经运行时辅助（槽清零=幂等，同 75-a 模型）----
    if (elemCanon == "字符串") {
        injectStrElemDestroy(canonClass, loc, arrayField, elemCanon, isFull, strOwnedSlot,
                               indexParam, indexField);
        return;
    }
    // ---- 81-a（第八十一轮）：元素 = 含拥有型串字段的结构体（含嵌套结构体/结果/可选）----
    //   元素槽的字段串由 stdlib `数据[n] = 值` 结构体写入深拷（79-a）保证「容器
    //   独有或驻留常量」——本分支补**容器消亡/移除路径的释放面**（原缺口：每元素
    //   泄漏其字段串，宿主/v2 同缺，探针 P1/P2 实证）：
    //     · 全量（~容器/清空）：按元素模型遍历（平铺/链游）逐元素释放字段串
    //       （嵌套结构体递归 + 结果/可选 联合体条件释放——emitOwnedStrFieldFreesAt）；
    //     · 单槽（析构被移除/删除头部/删除尾部）：唯一持有者槽单次释放；
    //   释放体自带「释放+清槽」幂等（多路径共享槽安全，plans/020 移植纪律 7）。
    if (isOwnedStrFieldElemContainer(canonClass)) {
        injectOwnedStrFieldElemDestroy(canonClass, loc, arrayField, elemCanon, isFull, isSingle,
                                          strOwnedSlot, indexParam, indexField);
        return;
    }
    if (injectClassElemDestroy(canonClass, mi, loc, arrayField, elemCanon, isFull, isSingle,
                               indexParam, indexField)) {
        return;
    }
}

// D1 133-a：有析构类元素释放（析构方法调用 + 全量/单槽遍历；
//   自 injectContainerElemDestroy 按族拆出——顺序尾形态：bool 模式（真=已处理/提前结束）
//   各段原顺序/条件/语句逐字不动；纯重构零行为变更）
bool IRGenerator::injectClassElemDestroy(const std::string& canonClass,
                                            const ClassMemberInfo& mi,
                                            const SourceLocation& loc,
                                            const std::string& arrayField,
                                            const std::string& elemCanon, bool isFull,
                                            bool isSingle, const std::string& indexParam,
                                            const std::string& indexField) {
    // T 须为有析构类：类 + 析构方法（沿继承链合并后的 methods 表）
    //   76-a：析构被移除（唯一持有者槽）只服务字符串——有析构类该槽的析构由
    //   析构元素 链（向量 删除循环首步）/删除头部/删除尾部 原路径负责，零行为变化
    if (mi.name == "析构被移除") return true;
    const ClassInfo* eci = semantic_->findClass(elemCanon);
    if (eci == nullptr) return true;
    std::string dtorSig;
    for (const auto& mk : eci->methods) {
        if (mk.second.isDestructor) { dtorSig = mk.second.sigKey; break; }
    }
    if (dtorSig.empty()) return true;  // 无析构：不注入（标量/结构体元素）
    const std::string thisUnique = lookupVarName("自身");
    if (thisUnique.empty()) return true;
    // 容器字段：数组（T*）/ 元素数量（整64）
    const int arrayOff = semantic_->classFieldOffset(canonClass, arrayField);
    const int countOff = semantic_->classFieldOffset(canonClass, "元素数量");
    if (arrayOff < 0 || countOff < 0) return true;
    const int stride = semantic_->typeSizeOf(elemCanon);
    if (stride <= 0) return true;
    const std::string dtorSym = methodSymbolKey(elemCanon, dtorSig);
    // this + 数组/元素数量 字段加载（循环外只读一次，循环内不修改）
    ir::IRValue selfPtr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(thisUnique, "ptr")},
                                     "ptr", thisUnique, loc);
    ir::IRValue arrayAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                       std::to_string(arrayOff), loc);
    ir::IRValue arrayPtr = emitResult(ir::Opcode::LoadPtr, {arrayAddr}, "ptr", "", loc);
    ir::IRValue countAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                       std::to_string(countOff), loc);
    ir::IRValue count = emitResult(ir::Opcode::LoadPtr, {countAddr}, "i64", "", loc);
    // 81-a：全量/单槽统一经元素遍历/单槽助手——**链式容器（链表/队列）按链游**
    //   （原实现全量平铺：删除头部/尾部 摘链后槽内容仍在且槽序号可能 >= 元素数量，
    //   平铺会漏释有效元素 + 误触越界槽二次析构，探针 P7 实证：链表<向量<字符串>>
    //   三元素删头×2 → 有效元素泄漏 1 + 越界槽二次调用 ~T）。
    const std::function<void(const ir::IRValue&)> dtorBody =
        [&](const ir::IRValue& elemAddr) {
            emit(ir::Opcode::Call, {elemAddr}, ir::IRValue(), dtorSym, "void", loc);
        };
    if (isFull) {
        emitContainerElemWalk(canonClass, selfPtr, arrayPtr, count, stride, loc, dtorBody);
    } else if (isSingle) {
        emitSingleElemRelease(canonClass, selfPtr, arrayPtr, count, stride, indexParam,
                              indexField, loc, dtorBody);
    }
    return false;
}


// D1 132-a：元素 = 含拥有型串字段的结构体——
//   字段串释放（递归字段释放 + 全量/单槽遍历）；自 injectContainerElemDestroy 按族拆出
//   （“if 包 + return”模式；纯重构零行为变更）
void IRGenerator::injectOwnedStrFieldElemDestroy(const std::string& canonClass,
                                                      const SourceLocation& loc,
                                                      const std::string& arrayField,
                                                      const std::string& elemCanon, bool isFull,
                                                      bool isSingle, bool strOwnedSlot,
                                                      const std::string& indexParam,
                                                      const std::string& indexField) {
    if (isOwnedStrFieldElemContainer(canonClass)) {
        const std::string thisUniqueE = lookupVarName("自身");
        if (thisUniqueE.empty()) return;
        const int arrayOffE = semantic_->classFieldOffset(canonClass, arrayField);
        const int countOffE = semantic_->classFieldOffset(canonClass, "元素数量");
        const int strideE = semantic_->typeSizeOf(elemCanon);
        if (arrayOffE < 0 || countOffE < 0 || strideE <= 0) return;
        ir::IRValue selfPtrE = emitResult(ir::Opcode::Load,
                                          {ir::IRValue::var(thisUniqueE, "ptr")},
                                          "ptr", thisUniqueE, loc);
        ir::IRValue arrayAddrE = emitResult(ir::Opcode::FieldAddr, {selfPtrE}, "ptr",
                                            std::to_string(arrayOffE), loc);
        ir::IRValue arrayPtrE = emitResult(ir::Opcode::LoadPtr, {arrayAddrE}, "ptr", "",
                                          loc);
        const std::function<void(const ir::IRValue&)> releaseElem =
            [&](const ir::IRValue& elemAddr) {
                emitOwnedStrFieldFreesAt(elemAddr, elemCanon, loc);
            };
        if (isFull) {
            ir::IRValue countAddrE = emitResult(ir::Opcode::FieldAddr, {selfPtrE},
                                                "ptr", std::to_string(countOffE), loc);
            ir::IRValue countE = emitResult(ir::Opcode::LoadPtr, {countAddrE}, "i64",
                                            "", loc);
            emitContainerElemWalk(canonClass, selfPtrE, arrayPtrE, countE, strideE, loc,
                                  releaseElem);
            return;
        }
        if (!isSingle || !strOwnedSlot) return;
        ir::IRValue countAddrE2 = emitResult(ir::Opcode::FieldAddr, {selfPtrE}, "ptr",
                                             std::to_string(countOffE), loc);
        ir::IRValue countE2 = emitResult(ir::Opcode::LoadPtr, {countAddrE2}, "i64", "",
                                         loc);
        emitSingleElemRelease(canonClass, selfPtrE, arrayPtrE, countE2, strideE,
                              indexParam, indexField, loc, releaseElem);
        return;
    }
}


// D1 131-a：字符串元素释放（全量通循环 / 单槽 __cn_seq_free_slot；
//   自 injectContainerElemDestroy 按族拆出——“if 包 + return”模式；纯重构零行为变更）
void IRGenerator::injectStrElemDestroy(const std::string& canonClass,
                                            const SourceLocation& loc,
                                            const std::string& arrayField,
                                            const std::string& elemCanon, bool isFull,
                                            bool strOwnedSlot, const std::string& indexParam,
                                            const std::string& indexField) {
    if (elemCanon == "字符串") {
        const std::string thisUniqueS = lookupVarName("自身");
        if (thisUniqueS.empty()) return;
        ir::IRValue selfPtrS = emitResult(ir::Opcode::Load,
                                          {ir::IRValue::var(thisUniqueS, "ptr")},
                                          "ptr", thisUniqueS, loc);
        if (isFull) {
            // 全量（~容器/清空；清空的 元素数量=0 在方法体内——先释放后归零）；
            //   链表/队列 按链游（清空前链有效，已摘槽不在链上=不重复释放）
            emitContainerElemFreeFor(canonClass, selfPtrS, loc);
            return;
        }
        // 76-a：字符串元素只注入「唯一持有者槽」的释放（析构被移除/删除头部/
        //   删除尾部）；析构元素（移位目标槽/末尾移出槽）**不注入**——字符串
        //   下标赋值=浅拷句柄，源槽与有效槽重复引用，释放即悬垂（探针 76-V1 实证）
        if (!strOwnedSlot) return;
        // 单槽：索引来自 参数（析构元素=索引）或 字段（链表 头索引/尾索引——
        //   注入在方法体前，字段仍是删除前旧值）
        ir::IRValue posS;
        if (!indexParam.empty()) {
            const std::string posUniqueS = lookupVarName(indexParam);
            if (posUniqueS.empty()) return;
            posS = emitResult(ir::Opcode::Load,
                              {ir::IRValue::var(posUniqueS, "i64")},
                              "i64", posUniqueS, loc);
        } else {
            const int idxOffS = semantic_->classFieldOffset(canonClass, indexField);
            if (idxOffS < 0) return;
            ir::IRValue idxAddrS = emitResult(ir::Opcode::FieldAddr, {selfPtrS}, "ptr",
                                              std::to_string(idxOffS), loc);
            posS = emitResult(ir::Opcode::LoadPtr, {idxAddrS}, "i64", "", loc);
        }
        const int arrayOffS = semantic_->classFieldOffset(canonClass, arrayField);
        if (arrayOffS < 0) return;
        emit(ir::Opcode::Call,
             {selfPtrS,
              ir::IRValue::constant(std::to_string(arrayOffS), "整64"),
              posS},
             ir::IRValue(), "__cn_seq_free_slot", "void", loc);
        return;
    }
}


// D1 130-a：移动元素挂点（移位循环单点：memcpy + 源槽资源清零；
//   自 injectContainerElemDestroy 按族拆出——“if 包 + return”模式；纯重构零行为变更）
void IRGenerator::injectMoveElemDestroy(const std::string& canonClass,
                                             const ClassMemberInfo& mi,
                                             const SourceLocation& loc,
                                             const std::string& arrayField,
                                             const std::string& elemCanon) {
    if (mi.name == "移动元素") {
        const std::string dstUniqueM = lookupVarName("目标");
        const std::string srcUniqueM = lookupVarName("源");
        const std::string thisUniqueMv = lookupVarName("自身");
        if (dstUniqueM.empty() || srcUniqueM.empty() || thisUniqueMv.empty()) return;
        const int arrayOffM = semantic_->classFieldOffset(canonClass, arrayField);
        const int strideM = semantic_->typeSizeOf(elemCanon);
        if (arrayOffM < 0 || strideM <= 0) return;
        ir::IRValue selfPtrMv = emitResult(ir::Opcode::Load,
                                           {ir::IRValue::var(thisUniqueMv, "ptr")},
                                           "ptr", thisUniqueMv, loc);
        ir::IRValue arrayAddrM = emitResult(ir::Opcode::FieldAddr, {selfPtrMv}, "ptr",
                                            std::to_string(arrayOffM), loc);
        ir::IRValue arrayPtrM = emitResult(ir::Opcode::LoadPtr, {arrayAddrM}, "ptr", "",
                                           loc);
        ir::IRValue dstPos = emitResult(ir::Opcode::Load,
                                        {ir::IRValue::var(dstUniqueM, "i64")}, "i64",
                                        dstUniqueM, loc);
        ir::IRValue srcPos = emitResult(ir::Opcode::Load,
                                        {ir::IRValue::var(srcUniqueM, "i64")}, "i64",
                                        srcUniqueM, loc);
        ir::IRValue dstAddr = emitResult(
            ir::Opcode::Add,
            {arrayPtrM,
             emitResult(ir::Opcode::Mul,
                        {dstPos, ir::IRValue::constant(std::to_string(strideM), "i64")},
                        "i64", "", loc)},
            "ptr", "", loc);
        ir::IRValue srcAddr = emitResult(
            ir::Opcode::Add,
            {arrayPtrM,
             emitResult(ir::Opcode::Mul,
                        {srcPos, ir::IRValue::constant(std::to_string(strideM), "i64")},
                        "i64", "", loc)},
            "ptr", "", loc);
        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
             std::to_string(strideM), "void", loc);
        // 源槽资源清零（句柄已随 memcpy 移交目标槽；只清点字段槽不释放——联合体安全）
        for (const auto& f : ownedStrFieldsOf(elemCanon)) {
            ir::IRValue fAddr = emitResult(ir::Opcode::FieldAddr, {srcAddr}, "ptr",
                                           std::to_string(f.offset), loc);
            ir::IRValue zeroM = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
            emit(ir::Opcode::StorePtr, {fAddr, zeroM}, ir::IRValue(), "", "ptr", loc);
        }
        return;
    }
}


// D1 128-a：映射<K,V> 字符串键／值释放注入（自 injectContainerElemDestroy 按族拆出；
//   族内所有路径均 return（原语义等价：主函数外层 if 块接管返回）。
void IRGenerator::injectMapElemDestroy(const std::string& canonClass,
                                          const ClassMemberInfo& mi,
                                          const SourceLocation& loc) {
        std::string kType;
        std::string vType;
        auto kit = genericTypeParams_.find("K");
        auto vit = genericTypeParams_.find("V");
        if (kit != genericTypeParams_.end()) kType = kit->second;
        if (vit != genericTypeParams_.end()) vType = vit->second;
        const bool kStr = (types::canonical(kType) == "字符串");
        const bool vStr = (types::canonical(vType) == "字符串");
        if (!kStr && !vStr) return;
        const std::string thisUniqueM = lookupVarName("自身");
        if (thisUniqueM.empty()) return;
        const int keysOff = semantic_->classFieldOffset(canonClass, "键数组");
        const int valsOff = semantic_->classFieldOffset(canonClass, "值数组");
        const int cntOffM = semantic_->classFieldOffset(canonClass, "元素数量");
        if (keysOff < 0 || valsOff < 0 || cntOffM < 0) return;
        ir::IRValue selfPtrM = emitResult(ir::Opcode::Load,
                                          {ir::IRValue::var(thisUniqueM, "ptr")},
                                          "ptr", thisUniqueM, loc);
        const bool isMapFull = (mi.name == "~映射" || mi.name == "清空" ||
                                mi.name == "释放内部数组");
        const bool isMapSlot = (mi.name == "析构键值" || mi.name == "析构值");
        if (!isMapFull && !isMapSlot) {
            return;   // 映射其余方法不注入
        }
        if (isMapFull) {
            emit(ir::Opcode::Call,
                 {selfPtrM,
                  ir::IRValue::constant(std::to_string(keysOff), "整64"),
                  ir::IRValue::constant(std::to_string(valsOff), "整64"),
                  ir::IRValue::constant(std::to_string(cntOffM), "整64"),
                  ir::IRValue::constant(kStr ? "1" : "0", "整64"),
                  ir::IRValue::constant(vStr ? "1" : "0", "整64")},
                 ir::IRValue(), "__cn_map_free_strings", "void", loc);
            return;   // 映射不参与「有析构类元素循环」注入（元素是 K/V 值，非 T）
        }
        // ---- 单槽释放（析构键值/析构值）----
        const std::string posUniqueM = lookupVarName("索引");
        if (posUniqueM.empty()) return;
        ir::IRValue posM = emitResult(ir::Opcode::Load,
                                      {ir::IRValue::var(posUniqueM, "i64")},
                                      "i64", posUniqueM, loc);
        const bool freeK = (mi.name == "析构键值" && kStr);
        const bool freeV = vStr;
        if (!freeK && !freeV) return;
        emit(ir::Opcode::Call,
             {selfPtrM,
              ir::IRValue::constant(std::to_string(keysOff), "整64"),
              ir::IRValue::constant(std::to_string(valsOff), "整64"),
              posM,
              ir::IRValue::constant(freeK ? "1" : "0", "整64"),
              ir::IRValue::constant(freeV ? "1" : "0", "整64")},
             ir::IRValue(), "__cn_map_free_slot", "void", loc);
        return;
}


// ==================== 81-a：容器元素遍历/单槽释放助手 ====================
// 背景（探针 P1~P7 实证，plans/020 矩阵靶子 #1）：元素槽资源（含串字段结构体的
//   字段串 / 有析构类元素的对象）在容器消亡（~容器/清空）与移除路径需按**元素模型**
//   释放——平铺数组（向量/栈/集合）与链式槽（链表/队列）的「有效元素集合」不同：
//   链式容器的 删除头部/删除尾部 摘链后槽内容仍在（且槽序号可能 >= 元素数量），
//   平铺遍历会漏释有效元素 + 误触越界槽（探针 P7：链表<向量<字符串>> 三元素删头
//   ×2 → 有效元素泄漏 1 + 越界槽二次析构）。故全量释放统一经本遍历循环：
//     · 平铺：idx=0..元素数量；
//     · 链式：idx=头索引 沿 下一索引 游走（步数上限=元素数量，链损坏时不死循环）。
// 不变量：循环计数器用可变 Alloca 槽（每次迭代 Load/Store）；数组==无 守卫跳过；
//   body 内的释放体自身「释放+清槽」幂等（多路径共享槽，plans/020 移植纪律 7）。

// 容器元素类型（实例化实参 canonical；非容器/无实参返回空串）
std::string IRGenerator::containerElemTypeOf(const std::string& canonClass) const {
    const std::size_t dl = canonClass.find('$');
    if (dl == std::string::npos) return std::string();
    const std::string head = canonClass.substr(0, dl);
    if (head != "向量" && head != "链表" && head != "栈" && head != "队列" &&
        head != "集合") {
        return std::string();  // 映射=双实参 K$V，非单元素数组模型（另路径）
    }
    return types::canonical(canonClass.substr(dl + 1));
}

// 元素含拥有型串字段判定（结构体/结果/可选，递归展开；字符串元素走既有路径）
bool IRGenerator::isOwnedStrFieldElemContainer(const std::string& canonClass) const {
    const std::string elem = containerElemTypeOf(canonClass);
    if (elem.empty() || elem == "字符串") return false;
    return !ownedStrFieldsOf(elem).empty();
}

// 元素遍历循环（全量释放路径单点事实源；body 由调用方发射）
void IRGenerator::emitContainerElemWalk(
    const std::string& canonClass, const ir::IRValue& selfPtr,
    const ir::IRValue& arrayPtr, const ir::IRValue& count, int stride,
    const SourceLocation& loc,
    const std::function<void(const ir::IRValue&)>& body) {
    if (stride <= 0 || semantic_ == nullptr) return;
    const std::string head = canonClass.substr(0, canonClass.find('$'));
    const bool chain = (head == "链表" || head == "队列");
    ir::IRValue nextPtr = ir::IRValue::reg(-1, "ptr");
    ir::IRValue headIdx = ir::IRValue::reg(-1, "i64");
    if (chain) {
        const int nextOff = semantic_->classFieldOffset(canonClass, "下一索引");
        const int headOff = semantic_->classFieldOffset(canonClass, "头索引");
        if (nextOff < 0 || headOff < 0) return;
        ir::IRValue nextAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                          std::to_string(nextOff), loc);
        nextPtr = emitResult(ir::Opcode::LoadPtr, {nextAddr}, "ptr", "", loc);
        ir::IRValue headAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                          std::to_string(headOff), loc);
        headIdx = emitResult(ir::Opcode::LoadPtr, {headAddr}, "i64", "", loc);
    }
    ir::IRValue zeroC = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    const std::string idxUnique = "?ewi" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "i64"), idxUnique,
         "i64", loc);
    emit(ir::Opcode::Store, {chain ? headIdx : zeroC}, ir::IRValue(), idxUnique,
         "i64", loc);
    std::string cntUnique;  // 链式：链游步数（上限=元素数量，防环）
    if (chain) {
        cntUnique = "?ewn" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "i64"),
             cntUnique, "i64", loc);
        emit(ir::Opcode::Store, {zeroC}, ir::IRValue(), cntUnique, "i64", loc);
    }
    const std::string loopL = "bb" + std::to_string(blockCounter_++);
    const std::string bodyL = "bb" + std::to_string(blockCounter_++);
    const std::string doneL = "bb" + std::to_string(blockCounter_++);
    // 数组==无 守卫（空容器/未分配：跳过循环）
    ir::IRValue arrayIsNull = emitResult(ir::Opcode::Eq, {arrayPtr, zeroC}, "i1", "",
                                         loc);
    endBranch(arrayIsNull.toString(), doneL, loopL);
    setCurrentBlock(newBlock(loopL));
    ir::IRValue idx = emitResult(ir::Opcode::Load,
                                 {ir::IRValue::var(idxUnique, "i64")}, "i64",
                                 idxUnique, loc);
    ir::IRValue cond;
    if (chain) {
        // 有效 = !(idx < 0 || n >= 元素数量)
        ir::IRValue neg = emitResult(ir::Opcode::Lt, {idx, zeroC}, "i1", "", loc);
        ir::IRValue n = emitResult(ir::Opcode::Load,
                                   {ir::IRValue::var(cntUnique, "i64")}, "i64",
                                   cntUnique, loc);
        ir::IRValue over = emitResult(ir::Opcode::Ge, {n, count}, "i1", "", loc);
        ir::IRValue bad = emitResult(ir::Opcode::Or, {neg, over}, "i1", "", loc);
        cond = emitResult(ir::Opcode::Not, {bad}, "i1", "", loc);
    } else {
        cond = emitResult(ir::Opcode::Lt, {idx, count}, "i1", "", loc);
    }
    endBranch(cond.toString(), bodyL, doneL);
    setCurrentBlock(newBlock(bodyL));
    ir::IRValue scaled = emitResult(
        ir::Opcode::Mul,
        {idx, ir::IRValue::constant(std::to_string(stride), "i64")}, "i64", "", loc);
    ir::IRValue elemAddr = emitResult(ir::Opcode::Add, {arrayPtr, scaled}, "ptr", "",
                                      loc);
    body(elemAddr);
    ir::IRValue one = emitResult(ir::Opcode::ConstInt, {}, "i64", "1", loc);
    ir::IRValue next;
    if (chain) {
        // 下一索引 数组按 8 字节步长（整64 槽）
        ir::IRValue idxScaled = emitResult(ir::Opcode::Mul,
                                           {idx, ir::IRValue::constant("8", "i64")},
                                           "i64", "", loc);
        ir::IRValue nAddr = emitResult(ir::Opcode::Add, {nextPtr, idxScaled}, "ptr",
                                       "", loc);
        next = emitResult(ir::Opcode::LoadPtr, {nAddr}, "i64", "", loc);
    } else {
        next = emitResult(ir::Opcode::Add, {idx, one}, "i64", "", loc);
    }
    emit(ir::Opcode::Store, {next}, ir::IRValue(), idxUnique, "i64", loc);
    if (chain) {
        ir::IRValue n2 = emitResult(ir::Opcode::Load,
                                    {ir::IRValue::var(cntUnique, "i64")}, "i64",
                                    cntUnique, loc);
        ir::IRValue nInc = emitResult(ir::Opcode::Add, {n2, one}, "i64", "", loc);
        emit(ir::Opcode::Store, {nInc}, ir::IRValue(), cntUnique, "i64", loc);
    }
    endJump(loopL);
    setCurrentBlock(newBlock(doneL));
}

// 单槽元素释放（守卫 + 体块；索引来源：参数 索引 / 字段 头索引·尾索引）
void IRGenerator::emitSingleElemRelease(
    const std::string& canonClass, const ir::IRValue& selfPtr,
    const ir::IRValue& arrayPtr, const ir::IRValue& count, int stride,
    const std::string& indexParam, const std::string& indexField,
    const SourceLocation& loc,
    const std::function<void(const ir::IRValue&)>& body) {
    if (stride <= 0 || semantic_ == nullptr) return;
    ir::IRValue pos;
    if (!indexParam.empty()) {
        const std::string posUnique = lookupVarName(indexParam);
        if (posUnique.empty()) return;
        pos = emitResult(ir::Opcode::Load, {ir::IRValue::var(posUnique, "i64")},
                         "i64", posUnique, loc);
    } else {
        const int idxOff = semantic_->classFieldOffset(canonClass, indexField);
        if (idxOff < 0) return;
        ir::IRValue idxAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                         std::to_string(idxOff), loc);
        pos = emitResult(ir::Opcode::LoadPtr, {idxAddr}, "i64", "", loc);
    }
    const std::string releaseL = "bb" + std::to_string(blockCounter_++);
    const std::string skipL = "bb" + std::to_string(blockCounter_++);
    // 守卫：析构元素/析构被移除（向量/集合，参数索引）= 索引<0 或 >=元素数量；
    //   链表 删头/删尾（字段索引）只用 索引<0（链式槽序号可能 >= 元素数量）
    ir::IRValue zeroC = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    ir::IRValue skipCond;
    if (!indexParam.empty()) {
        ir::IRValue neg = emitResult(ir::Opcode::Lt, {pos, zeroC}, "i1", "", loc);
        ir::IRValue ge = emitResult(ir::Opcode::Ge, {pos, count}, "i1", "", loc);
        skipCond = emitResult(ir::Opcode::Or, {neg, ge}, "i1", "", loc);
    } else {
        skipCond = emitResult(ir::Opcode::Lt, {pos, zeroC}, "i1", "", loc);
    }
    endBranch(skipCond.toString(), skipL, releaseL);
    setCurrentBlock(newBlock(releaseL));
    ir::IRValue scaled = emitResult(
        ir::Opcode::Mul,
        {pos, ir::IRValue::constant(std::to_string(stride), "i64")}, "i64", "", loc);
    ir::IRValue elemAddr = emitResult(ir::Opcode::Add, {arrayPtr, scaled}, "ptr", "",
                                      loc);
    body(elemAddr);
    endJump(skipL);
    setCurrentBlock(newBlock(skipL));
}
} // namespace cn_compiler
