// CN-IR生成器——阶段3 OOP 调用/析构发射（D1 行数整改 114-a：自 ir_oop_call.cpp 按族拆出）
//   族 = 类类型局部变量析构与拥有型字符串释放（genStringFrees + genClassDestructorCalls + emitStringFreeFor + emitClassDeleteFor + genBlockExitDestruct + emitStrArrayElemFreesFor + genJumpDestructFrom + static blockExitSrcName）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   文件级 static blockExitSrcName 随族——族内唯一引用点）。
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/types/type_system.hpp"

namespace cn_compiler {

// ==================== 类类型局部变量析构（DeleteObject 发射） ====================

// 函数收尾钩子：扫描当前函数 Alloca 的变量，源码类型为类且类有析构函数 ->
//   在函数返回前发射 DeleteObject（extra=类名，operand[0]=变量地址）。
// 说明：类对象在 CN 中为堆对象（NewObject 分配），变量槽存对象指针；
//   RAII 风格：函数退出时自动释放（调用析构 + __cn_object_delete）。
// 实现：仅在函数最后块（未终止）前插入；简化版不做异常安全（阶段三范围）。
// plans/019 阶段4'（2026-09-10 方案A 用户裁决）：拥有型字符串 RAII——与类
// RAII（genClassDestructorCalls）同模型：①入口块最前零初始化（未执行分支/
// 裸声明槽垃圾防线）；②每个返回块指令末尾注入 __cn_str_free（多返回点全覆
// 盖；空安全——零句柄/已清零槽安全跳过）；③返回值=该槽 Load 跳过（所有权
// 移出）。名单=Alloca 槽源类型为字符串 ∩ 语义层 isOwnedStringLocal（非拥有
// 形态赋值与返回已在语义层剔除——free 只读段=UB 的静态防线）。
void IRGenerator::genStringFrees() {
    if (semantic_ == nullptr || function_ == nullptr) return;
    if (function_->blocks.empty()) return;
    // 收集：本函数 Alloca 指令（函数内天然隔离——oopVarSrcTypes_ 为模块级表，
    //   直接扫表会跨函数串槽）∩ oopVarSrcTypes_ 源类型=字符串 ∩ 非污染
    //   （stringTainted_：赋值右值非拥有形态的变量整剔——IR 期自持）
    std::vector<std::string> ownedSlots;
    for (const auto& block : function_->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode != ir::Opcode::Alloca) continue;
            const std::string& unique = inst.extra;
            if (unique.empty()) continue;
            auto srcIt = oopVarSrcTypes_.find(unique);
            if (srcIt == oopVarSrcTypes_.end() || srcIt->second != "字符串")
                continue;
            const std::size_t dl = unique.rfind('$');
            const std::string srcName =
                (dl == std::string::npos) ? unique : unique.substr(0, dl);
            if (stringTainted_.count(srcName) > 0) continue;
            ownedSlots.push_back(unique);
        }
    }
    // 81-a（2026-09-12 第八十一轮）：原此处 `if (ownedSlots.empty()) return;`
    //   提前返回会**连带跳过**下方 79-a 的「含串字段聚合局部」返回块兜底段——
    //   函数内无字符串局部但有含串字段结构体局部（如 `存入(表, 值)` 内
    //   `盒子 局 = 盒子{名=值}`，值=形参）时，局 的字段串只在块出口释放；
    //   函数带 `返回 X;`（块尾部终止）时块出口析构不发射 → 兜底段被挡 → 泄漏 1
    //   （探针 P12/P13/P14 变体矩阵实证：无返回语句的变体残留 0、带返回的残留 1；
    //   asm 对照：存入甲函数体零 __cn_str_free）。修法：字符串段改为条件包裹，
    //   字段兜底段无条件执行。
    // 98-a（C9）：字符串元素数组——函数级兜底（返回块全量逐元素释放）。
    //   收集方式与字符串段同款=**扫本函数 Alloca ∩ oopVarSrcTypes_**（不可用
    //   ownedStrArrayOrder_ 名单：块出口析构已把名单截断到基线——函数顶层块
    //   出口即清空，此处看不到；mem6 探针「后=2」实证）。入口零初始化由数组
    //   声明路径既有逐槽零初始化覆盖。
    std::vector<std::string> ownedStrArrays;
    for (const auto& block : function_->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode != ir::Opcode::Alloca) continue;
            const std::string& unique = inst.extra;
            if (unique.empty()) continue;
            auto srcIt2 = oopVarSrcTypes_.find(unique);
            if (srcIt2 == oopVarSrcTypes_.end()) continue;
            const std::string st2 = types::canonical(srcIt2->second);
            if (types::isArray(st2) && types::arrayElemOf(st2) == "字符串") {
                ownedStrArrays.push_back(unique);
            }
        }
    }
    if (!ownedStrArrays.empty()) {
        // 206-c 宿主缺陷根治（H 级·堆损坏 0xC0000374）：**字符串元素数组入口块
        //   零初始化**——原注释「入口零初始化由数组声明路径既有逐槽零初始化
        //   覆盖」只对**无初始化器**数组成立（声明路径零初始化条件，见
        //   ir_stmt_decl.cpp 缺陷2 根治）；有初始化器数组在提前返回路径上
        //   元素槽为栈垃圾 → 返回块逐元素释放读垃圾指针。收集口径与释放段
        //   一致（扫本函数 Alloca ∩ oopVarSrcTypes_，名单在块出口已截断不可用）。
        {
            ir::IRBlock* entryBlock = function_->blocks.front().get();
            for (auto it = ownedStrArrays.rbegin(); it != ownedStrArrays.rend();
                 ++it) {
                auto slotIt = function_->varSlots.find(*it);
                if (slotIt == function_->varSlots.end() || slotIt->second <= 0)
                    continue;
                for (int s = slotIt->second - 1; s >= 0; --s) {
                    const std::string slotName =
                        s == 0 ? *it : *it + "$s" + std::to_string(s);
                    ir::IRInstruction zeroInst;
                    zeroInst.opcode = ir::Opcode::Store;
                    zeroInst.operands.push_back(ir::IRValue::constant("0", "i64"));
                    zeroInst.result = ir::IRValue();
                    zeroInst.extra = slotName;
                    zeroInst.type = "i64";
                    entryBlock->instructions.insert(
                        entryBlock->instructions.begin(), zeroInst);
                }
            }
        }
        for (const auto& block : function_->blocks) {
            if (!block->terminated) continue;
            if (block->termKind != "返回") continue;
            setCurrentBlock(block.get());
            for (std::size_t i = ownedStrArrays.size(); i > 0; --i) {
                emitStrArrayElemFreesFor(ownedStrArrays[i - 1]);
            }
        }
    }
    // 206-c 宿主缺陷根治（H 级·堆损坏 0xC0000374）：**聚合局部（含拥有型字段的
    //   结构体/结果/可选）入口块零初始化**——字段释放设施（ownedFieldOrder_
    //   名单，登记点 genVarDecl；块出口/跳出/返回块三路释放）此前**只**依赖
    //   「声明点零初始化」，而声明点零初始化仅在无初始化器声明发射；提前返回
    //   （语法/语义错误中止等）路径上声明点未执行 → 槽为栈垃圾 → 释放设施读
    //   垃圾对象指针 → __cn_object_delete(垃圾) 堆损坏 0xC0000374（v2p 错误
    //   路径实测：修复前 rc=0xC0000374，补零初始化后 rc=1/2 正常）。最小复现
    //   =结构体声明置于 返回 之后的同帧形态（d3 探针：bb1 析构读未初始化槽，
    //   新鲜栈页侥幸为 0，叠加前序调用污染栈即崩）。与 字符串/类 局部同款
    //   「入口块最前」前置插入；收集=扫本函数 Alloca ∩ oopVarSrcTypes_ ∩
    //   判据（聚合=ownedStrFieldsOf 非空，与释放设施精确对齐；类/字符串/
    //   字符串元素数组各有专段不再重复）。
    {
        std::vector<std::string> ownedAggSlots;
        for (const auto& block : function_->blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode != ir::Opcode::Alloca) continue;
                const std::string& unique = inst.extra;
                if (unique.empty()) continue;
                auto srcIt3 = oopVarSrcTypes_.find(unique);
                if (srcIt3 == oopVarSrcTypes_.end()) continue;
                const std::string canon3 = types::canonical(srcIt3->second);
                if (canon3 == "字符串") continue;
                if (semantic_->isClassType(canon3)) continue;
                if (types::isArray(canon3)) continue;  // 数组段已覆盖
                if (ownedStrFieldsOf(canon3).empty()) continue;
                ownedAggSlots.push_back(unique);
            }
        }
        if (!ownedAggSlots.empty()) {
            ir::IRBlock* entryBlock = function_->blocks.front().get();
            for (auto it = ownedAggSlots.rbegin(); it != ownedAggSlots.rend();
                 ++it) {
                auto slotIt = function_->varSlots.find(*it);
                if (slotIt == function_->varSlots.end() || slotIt->second <= 0)
                    continue;
                for (int s = slotIt->second - 1; s >= 0; --s) {
                    const std::string slotName =
                        s == 0 ? *it : *it + "$s" + std::to_string(s);
                    ir::IRInstruction zeroInst;
                    zeroInst.opcode = ir::Opcode::Store;
                    zeroInst.operands.push_back(ir::IRValue::constant("0", "i64"));
                    zeroInst.result = ir::IRValue();
                    zeroInst.extra = slotName;
                    zeroInst.type = "i64";
                    entryBlock->instructions.insert(
                        entryBlock->instructions.begin(), zeroInst);
                }
            }
        }
    }
    if (!ownedSlots.empty()) {
        // 入口块零初始化（与类 RAII 同款前置插入）
        ir::IRBlock* entryBlock = function_->blocks.front().get();
        for (const auto& unique : ownedSlots) {
            ir::IRInstruction zeroInst;
            zeroInst.opcode = ir::Opcode::Store;
            zeroInst.operands.push_back(ir::IRValue::constant("0", "i64"));
            zeroInst.result = ir::IRValue();
            zeroInst.extra = unique;
            zeroInst.type = "ptr";
            entryBlock->instructions.insert(entryBlock->instructions.begin(),
                                            zeroInst);
        }
        // 返回块末尾注入 __cn_str_free（返回值=该槽 Load 时跳过——IR 识别双保险）
        for (const auto& block : function_->blocks) {
            if (!block->terminated) continue;
            if (block->termKind != "返回") continue;
            std::unordered_set<std::string> returnedSlots;
            const std::string& rv = block->termReturnValue;
            if (rv.size() > 2 && rv[0] == '%' && rv[1] == 'v') {
                const int retId = std::stoi(rv.substr(2));
                for (const auto& inst : block->instructions) {
                    if (inst.result.id == retId && inst.opcode == ir::Opcode::Load &&
                        !inst.operands.empty() && inst.operands[0].id < 0 &&
                        !inst.operands[0].isConstant) {
                        returnedSlots.insert(inst.operands[0].extra);
                        break;
                    }
                }
            }
            setCurrentBlock(block.get());
            for (const auto& unique : ownedSlots) {
                if (returnedSlots.count(unique) > 0) continue;
                ir::IRValue strPtr = emitResult(
                    ir::Opcode::Load,
                    {ir::IRValue::var(unique, "ptr")}, "ptr", unique,
                    SourceLocation());
                emit(ir::Opcode::Call, {strPtr}, ir::IRValue(),
                     "__cn_str_free", "void", SourceLocation());
            }
        }
    }
    // 79-a（2026-09-12 第七十九轮）：聚合拥有型字符串字段函数尾兜底——
    //   含串字段的结构体/结果/可选局部，逐字段 free+清槽（单点事实源
    //   emitOwnedFieldFreesFor）。返回移出豁免：返回块返回值为 AddrOf(槽)
    //   （结构体按值返回 = 地址 + 被调方 epilogue memcpy 到调用方缓冲）时，
    //   该槽字段所有权移交调用方（接收方登记接管）——跳过，否则调用方
    //   拿到已释放句柄（跨函数 UAF）。
    if (!ownedFieldOrder_.empty()) {
        std::unordered_set<std::string> returnedFieldSlots;
        for (const auto& block : function_->blocks) {
            if (!block->terminated || block->termKind != "返回") continue;
            const std::string& rv = block->termReturnValue;
            if (rv.size() <= 2 || rv[0] != '%' || rv[1] != 'v') continue;
            const int retId = std::stoi(rv.substr(2));
            for (const auto& inst : block->instructions) {
                if (inst.result.id == retId && inst.opcode == ir::Opcode::AddrOf &&
                    !inst.operands.empty()) {
                    returnedFieldSlots.insert(inst.operands[0].extra);
                    break;
                }
            }
        }
        for (const auto& block : function_->blocks) {
            if (!block->terminated || block->termKind != "返回") continue;
            setCurrentBlock(block.get());
            for (const auto& unique : ownedFieldOrder_) {
                if (returnedFieldSlots.count(unique) > 0) continue;
                auto it = oopVarSrcTypes_.find(unique);
                if (it == oopVarSrcTypes_.end()) continue;
                emitOwnedFieldFreesFor(unique, types::canonical(it->second),
                                       SourceLocation());
            }
        }
    }
}

void IRGenerator::genClassDestructorCalls() {
    if (semantic_ == nullptr || function_ == nullptr) return;
    if (function_->blocks.empty()) return;
    // 从块指令收集 Alloca（extra=唯一内部名），源码类型查 oopVarSrcTypes_
    struct ObjVar {
        std::string unique;   // 唯一内部名
        std::string srcType;  // 类名
    };
    std::vector<ObjVar> objVars;
    for (const auto& block : function_->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode != ir::Opcode::Alloca) continue;
            const std::string& unique = inst.extra;
            auto srcIt = oopVarSrcTypes_.find(unique);
            if (srcIt == oopVarSrcTypes_.end()) continue;
            const std::string canon = types::canonical(srcIt->second);
            const ClassInfo* ci = semantic_->findClass(canon);
            if (ci == nullptr) continue;
            bool hasDtor = false;
            for (const auto& mk : ci->methods) {
                if (mk.second.isDestructor) { hasDtor = true; break; }
            }
            if (!hasDtor) continue;
            objVars.push_back(ObjVar{unique, canon});
        }
    }
    if (objVars.empty()) return;
    // 宿主根治（2026-09-01，缺陷：分支未执行时类局部被无条件析构）：
    //   函数入口块最前统一零初始化全部类局部槽（Store 空指针）——类局部声明
    //   可能位于未执行的分支内，声明处初始化不运行、槽为栈垃圾（新鲜栈零页
    //   时碰巧为空属侥幸，实测叠加前序调用污染栈后 100% 崩溃）；入口零初始化
    //   保证任意执行路径下未构造槽为确定性 空指针，配合 codegen DeleteObject
    //   空安全跳过（x64/arm64），RAII 收尾不再触碰野指针。
    {
        ir::IRBlock* entryBlock = function_->blocks.front().get();
        for (auto it = objVars.rbegin(); it != objVars.rend(); ++it) {
            ir::IRInstruction zeroInst;
            zeroInst.opcode = ir::Opcode::Store;
            zeroInst.operands.push_back(ir::IRValue::constant("0", "i64"));
            zeroInst.result = ir::IRValue();
            zeroInst.extra = it->unique;   // Store 目标：变量槽（唯一内部名）
            zeroInst.type = "ptr";
            entryBlock->instructions.insert(entryBlock->instructions.begin(),
                                            zeroInst);
        }
    }
    // 在每个"返回终止"块的指令列表末尾追加 DeleteObject（RAII：return 前释放）。
    // 说明：IRBlock 终止信息是块级属性（termKind/termReturnValue），指令列表不含
    //   return 指令；codegen emitBlock 输出顺序为 指令序列 + 终止——在返回块
    //   instructions 末尾追加 DeleteObject 即保证其先于 ret 执行。
    // 多返回点场景：每个返回块都会释放（重复释放同一对象——CN 类对象为显式堆管理，
    //   本子任务按"函数退出自动释放"语义实现，后续模块系统子任务可完善作用域级释放）。
    for (const auto& block : function_->blocks) {
        if (!block->terminated) continue;
        if (block->termKind != "返回") continue;
        // 自举前置 A-3b（plans/004）：返回块若返回"类局部变量的对象指针"
        //   （返回 表，构建词表() -> 向量<字符串>），对象所有权随返回值转移给
        //   调用方——该变量的 DeleteObject 必须跳过。此前一律析构，函数返回后
        //   局部析构释放对象、调用方退出再次析构 -> 双重释放堆损坏（0xC0000374
        //   STATUS_HEAP_CORRUPTION，返回容器/自定义泛型实测崩溃）。
        //   识别：块返回值为 %vN 且 %vN 由 Load 该变量槽 产生（直接返回标识符）。
        std::unordered_set<std::string> returnedVars;
        const std::string& rv = block->termReturnValue;
        if (rv.size() > 2 && rv[0] == '%' && rv[1] == 'v') {
            const int retId = std::stoi(rv.substr(2));
            for (const auto& inst : block->instructions) {
                if (inst.result.id == retId && inst.opcode == ir::Opcode::Load &&
                    !inst.operands.empty() && inst.operands[0].id < 0 &&
                    !inst.operands[0].isConstant) {
                    returnedVars.insert(inst.operands[0].extra);
                    break;
                }
            }
        } else if (!rv.empty() && rv.rfind("%v", 0) != 0) {
            returnedVars.insert(rv);  // 防御：返回值为变量名直传
        }
        setCurrentBlock(block.get());
        for (const auto& ov : objVars) {
            if (returnedVars.count(ov.unique) > 0) continue;  // 所有权转移：跳过析构
            // 变量槽地址 -> Load 对象指针
            ir::IRValue objPtr = emitResult(
                ir::Opcode::Load,
                {ir::IRValue::var(ov.unique, "ptr")},
                "ptr", ov.unique, SourceLocation());
            // 方案A RAII（2026-08-25，学习 C++ vector<string>）：容器局部变量
            //   析构时先释放元素字符串，使字符串随局部容器离开作用域自动清理，
            //   降低 78/79 组件链每模块百万级字符串在 reset 前的峰值累积。
            //   74-a（2026-09-11）：单点事实源 emitContainerElemFreeFor——从
            //   向量 扩到 栈（平铺模型）+ 链表/队列（链式模型，探针 N 实证原三
            //   容器元素串从不释放）。
            const std::string canonSrc = types::canonical(ov.srcType);
            emitContainerElemFreeFor(canonSrc, objPtr, SourceLocation());
            emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(),
                 ov.srcType, "void", SourceLocation());
        }
    }
}

// ==================== 72-a 块级作用域 RAII（2026-09-11 第七十二轮） ====================
// 槽唯一内部名（名$槽序）-> 源码名：污染集 stringTainted_ 按源码名键控
//   （genVarDecl/赋值位/下标借出/字段借出登记），此处统一剥离 '$' 后缀。
static std::string blockExitSrcName(const std::string& unique) {
    const std::size_t dl = unique.rfind('$');
    return (dl == std::string::npos) ? unique : unique.substr(0, dl);
}
// 背景：宿主 RAII 原为函数级（genStringFrees/genClassDestructorCalls 仅在返回块
//   注入释放）——循环体内声明的资源只有末次迭代被释放，中间迭代永久泄漏
//   （探针 66 实测：循环体字符串 4 轮残留 3、容器 4 轮残留 3）。v2 侧已有
//   块出口析构模型（生成块 出口对基线后新增项逆序释放），此处对齐（Rust 作用域
//   drop 同构）：genBlock 进入记录基线，出口释放本块新增项并截断名单。
// 跳出路径：返回=返回块全量兜底（既有）；中断/继续=跳出前先发循环体基线的
//   块级释放（drop-on-jump），随后函数级兜底覆盖剩余。
// 释放+清零（幂等）：块出口/跳出/函数级兜底多路径共用同一槽——清零后
//   后续路径对该槽 free(nullptr) 空安全，杜绝「条件释放 + 兜底释放」双重释放
//   （循环体反复声明同槽、中断跳出与落空出口并存等场景）。
void IRGenerator::emitStringFreeFor(const std::string& unique) {
    ir::IRValue strPtr = emitResult(ir::Opcode::Load,
                                    {ir::IRValue::var(unique, "ptr")},
                                    "ptr", unique, SourceLocation());
    emit(ir::Opcode::Call, {strPtr}, ir::IRValue(), "__cn_str_free", "void",
         SourceLocation());
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                  SourceLocation());
    emit(ir::Opcode::Store, {zero}, ir::IRValue(), unique, "ptr",
         SourceLocation());
}
void IRGenerator::emitClassDeleteFor(const std::string& unique,
                                     const std::string& canon) {
    ir::IRValue objPtr = emitResult(ir::Opcode::Load,
                                    {ir::IRValue::var(unique, "ptr")},
                                    "ptr", unique, SourceLocation());
    // 容器元素串释放（方案A RAII；74-a 单点事实源——从 向量 扩到 栈/链表/队列，
    //   探针 N 实证原三容器元素串从不释放，各残留 1）
    emitContainerElemFreeFor(canon, objPtr, SourceLocation());
    emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(), canon, "void",
         SourceLocation());
    ir::IRValue z = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                               SourceLocation());
    emit(ir::Opcode::Store, {z}, ir::IRValue(), unique, "ptr", SourceLocation());
}

// 块出口析构：释放本块新增的拥有串/类对象（逆序=后声明先析构）并截断名单。
//   调用点=genBlock 正常出口（未终止路径）；已终止（返回/中断/继续 已跳）时
//   不调用（不可达，且各自跳转路径已/将由兜底释放覆盖）。
void IRGenerator::genBlockExitDestruct() {
    while (ownedStringOrder_.size() > scopeStringBase_.back()) {
        const std::string unique = ownedStringOrder_.back();
        ownedStringOrder_.pop_back();
        if (stringTainted_.count(blockExitSrcName(unique)) > 0) continue;  // 借用视图不释放
        emitStringFreeFor(unique);
    }
    while (ownedClassOrder_.size() > scopeClassBase_.back()) {
        const std::string unique = ownedClassOrder_.back();
        ownedClassOrder_.pop_back();
        auto it = oopVarSrcTypes_.find(unique);
        if (it == oopVarSrcTypes_.end()) continue;
        emitClassDeleteFor(unique, types::canonical(it->second));
    }
    // 79-a：含串字段聚合局部——块出口逆序释放字段串（free+清槽，幂等）
    while (ownedFieldOrder_.size() > scopeFieldBase_.back()) {
        const std::string unique = ownedFieldOrder_.back();
        ownedFieldOrder_.pop_back();
        auto it = oopVarSrcTypes_.find(unique);
        if (it == oopVarSrcTypes_.end()) continue;
        emitOwnedFieldFreesFor(unique, types::canonical(it->second),
                               SourceLocation());
    }
    // 98-a（C9）：字符串元素数组——块出口逆序逐元素释放 + 名单截断
    while (ownedStrArrayOrder_.size() > scopeStrArrayBase_.back()) {
        const std::string unique = ownedStrArrayOrder_.back();
        ownedStrArrayOrder_.pop_back();
        emitStrArrayElemFreesFor(unique);
    }
}

// 98-a（C9, 2026-09-13 第九十八轮）：字符串元素数组元素释放发射——编译期展开
//   逐元素（长度已知）：地址 = 数组基址 + i×元素大小 → LoadPtr → __cn_str_free
//   → StorePtr 0（释放+清槽幂等模型：块出口/跳出/函数尾多路径共享数组区，
//   重复经过的释放点对已清槽空安全跳过）。数组基址 = AddrOf(unique)（数组变量
//   槽区最深槽=基址，宿主 C 布局 registerVarSlots）。
void IRGenerator::emitStrArrayElemFreesFor(const std::string& unique) {
    if (semantic_ == nullptr) return;
    auto it = oopVarSrcTypes_.find(unique);
    if (it == oopVarSrcTypes_.end()) return;
    const std::string srcType = types::canonical(it->second);
    if (!types::isArray(srcType)) return;
    if (types::arrayElemOf(srcType) != "字符串") return;
    const int len = types::arrayLenOf(srcType);
    if (len <= 0) return;
    int elemStride = semantic_->typeSizeOf(types::arrayElemOf(srcType));
    if (elemStride <= 0) elemStride = 8;
    const SourceLocation loc;
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(unique, "ptr")}, "ptr",
                                  unique, loc);
    for (int i = 0; i < len; ++i) {
        ir::IRValue addr = base;
        if (i > 0) {
            ir::IRValue off = emitResult(
                ir::Opcode::ConstInt, {}, "i64",
                std::to_string(static_cast<long long>(i) * elemStride), loc);
            addr = emitResult(ir::Opcode::Add, {base, off}, "ptr", "", loc);
        }
        ir::IRValue handle = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "", loc);
        emitResult(ir::Opcode::Call, {handle}, "i32", "__cn_str_free", loc);
        emitResult(ir::Opcode::StorePtr, {addr, ir::IRValue::constant("0", "i64")},
                   "", "", loc);
    }
}

// 中断/继续 跳出循环体或选择分支：按进入该分支时记录的基线释放新增资源
//   （drop-on-jump）。调用方：循环=LoopContext 基线、选择=SwitchContext 基线。
// 注意：不截断编译期名单（落空路径的块出口析构仍须覆盖）——只发射释放+清零，
//   释放后槽=0，落空路径/函数级兜底再释放即空安全（幂等）。
void IRGenerator::genJumpDestructFrom(std::size_t stringBase, std::size_t classBase,
                                     std::size_t fieldBase, std::size_t strArrayBase) {
    // 98-a（C9）：字符串元素数组——按基线逐元素释放（drop-on-jump，不截断名单）
    for (std::size_t i = strArrayBase; i < ownedStrArrayOrder_.size(); ++i) {
        emitStrArrayElemFreesFor(ownedStrArrayOrder_[i]);
    }
    const std::size_t strEnd = ownedStringOrder_.size();
    for (std::size_t i = stringBase; i < strEnd; ++i) {
        const std::string& unique = ownedStringOrder_[i];
        if (stringTainted_.count(blockExitSrcName(unique)) > 0) continue;
        emitStringFreeFor(unique);
    }
    const std::size_t clsEnd = ownedClassOrder_.size();
    for (std::size_t i = classBase; i < clsEnd; ++i) {
        const std::string& unique = ownedClassOrder_[i];
        auto it = oopVarSrcTypes_.find(unique);
        if (it == oopVarSrcTypes_.end()) continue;
        emitClassDeleteFor(unique, types::canonical(it->second));
    }
    // 79-a：含串字段聚合局部——跳出前按基线释放字段串（drop-on-jump，
    //   不截断名单：同块其他路径仍须各自发射释放；清槽保证幂等）
    const std::size_t fldEnd = ownedFieldOrder_.size();
    for (std::size_t i = fieldBase; i < fldEnd; ++i) {
        const std::string& unique = ownedFieldOrder_[i];
        auto it = oopVarSrcTypes_.find(unique);
        if (it == oopVarSrcTypes_.end()) continue;
        emitOwnedFieldFreesFor(unique, types::canonical(it->second),
                               SourceLocation());
    }
}
} // namespace cn_compiler
