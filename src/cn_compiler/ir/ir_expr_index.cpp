// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出）
//   族 = 下标（visitIndexExpr——指针下标复用途/数组名退化/越界检查/结构体元素地址语义）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   族边界勘定：族内无文件级 static/匿名命名空间依赖，见 plans/021 §3-D1）。
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitIndexExpr(IndexExpr* node) {
    // 对象：数组名（IdentifierExpr）或指针表达式
    if (node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->object.get());
        const std::string unique = lookupVarName(ident->name);
        const std::string srcType = lookupSrcType(ident->name);
        if (!unique.empty() && types::isArray(srcType)) {
            // 数组对象：基址 = AddrOf(数组槽0)；元素类型来自数组元素类型
            const std::string elemSrc = types::arrayElemOf(srcType);
            const std::string elemIrType = mapType(elemSrc);
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, elemIrType)},
                                          "ptr", unique, node->location);
            ir::IRValue index = genExpr(node->index.get());
            // 越界检查插桩：index < 0 || index >= 数组长度 -> 运行时错误(2)
            // （IR 层仅生成比较 + 条件跳转到错误块，codegen 处理）
            emitBoundsCheck(index, types::arrayLenOf(srcType), node->location);
            // 地址 = base + index*元素大小（C语义；结构体数组按总大小步进，Task 2.7）
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            // 元素间距：按元素类型大小（缺陷③根治统一 C 布局——typeSizeOf 含
            //   结构体总大小（Task 2.7）/i128=16（BUG #5）/标量 4/2/1；原标量
            //   兜底 8 与数组 8 槽布局互洽，见 ir.cpp registerVarSlots 注）
            std::int64_t elemStride = 8;
            if (semantic_ != nullptr) {
                const int size = semantic_->typeSizeOf(elemSrc);
                if (size > 0) elemStride = size;
            }
            ir::IRValue scaled = emitResult(
                ir::Opcode::Mul,
                {index, ir::IRValue::constant(std::to_string(elemStride), "i64")},
                "i64", "", node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {base, scaled}, "ptr", "",
                                          node->location);
            // 结构体元素（Task 完善A）：数组元素作为"结构体值"时返回地址（ptr），
            //   供按值传参/赋值/整体拷贝使用；普通元素 LoadPtr 加载值
            if (semantic_ != nullptr && semantic_->isStructType(types::canonical(elemSrc))) {
                lastExpr_ = addr;
                return;
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, elemIrType, "",
                                   node->location);
            return;
        }
        if (!unique.empty() && types::isPointer(srcType)) {
            // 指针对象（p[i]）：等价 *(p + i)；步进按元素大小（Task 2.7 修复）
            ir::IRValue ptr = genExpr(node->object.get());
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            const std::int64_t stride = ptrElemStride(srcType);
            ir::IRValue scaled = emitResult(ir::Opcode::Mul,
                                            {index, ir::IRValue::constant(std::to_string(stride), "i64")},
                                            "i64", "", node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {ptr, scaled}, "ptr", "",
                                          node->location);
            // 集成验证修复 Bug：结构体指针元素（如 排序(员工档案* 名单) 中
            //   名单[j]）作为"结构体值"应返回地址（供整体赋值/按值传参/字段访问），
            //   原实现统一 LoadPtr 读首 8 字节当指针 -> 排序交换读到垃圾地址崩溃
            const std::string pointee = types::pointeeOf(srcType);
            if (semantic_ != nullptr &&
                semantic_->isStructType(types::canonical(pointee))) {
                lastExpr_ = addr;
                return;
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                                   mapType(pointee), "",
                                   node->location);
            return;
        }
        // 自举前置 A-1（plans/004）：字符串[i] 逐字节 O(1) 访问（字符* 字节
        //   视图，步进 1；与 C 的 s[i] 一致，越界由调用方按 字符串长度 约束）
        if (!unique.empty() && types::canonical(srcType) == "字符串") {
            ir::IRValue ptr = genExpr(node->object.get());
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            ir::IRValue addr = emitResult(ir::Opcode::Add, {ptr, index}, "ptr", "",
                                          node->location);
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "i8", "",
                                   node->location);
            return;
        }
    }
    // A-3（2026-08）：隐式类字段对象（方法体内 数据[位置]，数据 是 this->字段，
    //   lookupSrcType 为空）——此前落入下方"其他对象"分支按 8 字节步进/标量 LoadPtr，
    //   结构体元素越界读（向量<点> 元素错乱实测）。此处按字段源码类型推导
    //   步进与元素形态（数组字段/结构体指针字段 -> 返回元素地址；标量 -> LoadPtr）。
    if (node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident2 = static_cast<IdentifierExpr*>(node->object.get());
        const std::string st2 = lookupSrcType(ident2->name);
        if (st2.empty() && isInstanceField(ident2->name)) {
            const std::string fieldType = classFieldType(currentClass_, ident2->name);
            ir::IRValue ptr = genExpr(node->object.get());  // 字段指针值
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            std::int64_t stride = 8;
            std::string elemSrc = fieldType;
            bool elemIsStruct = semantic_ != nullptr &&
                                semantic_->isStructType(types::canonical(fieldType));
            if (types::isArray(fieldType)) {
                // 数组字段：按元素大小步进 + 越界检查（与成员数组字段同规则，
                //   结构体内嵌数组按 C 布局紧凑排布）
                elemSrc = types::arrayElemOf(fieldType);
                elemIsStruct = semantic_ != nullptr &&
                               semantic_->isStructType(types::canonical(elemSrc));
                // H4 根治（99-a, 2026-09-13 第九十九轮）：元素步进统一
                //   semantic typeSizeOf（原 非结构体走 types::typeSize——该表
                //   **无「字符串」**返回 0 -> 字段数组下标不缩放，探针 fldmem2
                //   实证「再读0=乙」；同族四消费点一并核查）
                stride = (semantic_ != nullptr) ? semantic_->typeSizeOf(elemSrc) : 8;
                if (stride <= 0) stride = 8;
                emitBoundsCheck(index, types::arrayLenOf(fieldType), node->location);
            } else if (types::isPointer(fieldType)) {
                // 指针字段（T* 数据）：步进统一按所指元素 typeSizeOf（缺陷③根治
                //   2026-09-03——与 ptrElemStride/容器库紧凑分配一致；原「标量 8
                //   字节槽」与写侧按元素大小混用 -> 窄元素读写错位腐坏，30 实证）
                elemSrc = types::pointeeOf(fieldType);
                const std::string elemCanon = types::canonical(elemSrc);
                // H8 补完（2026-08-25）：类元素（向量<T> 数据 T*，T=映射/简单盒）
                //   同结构体——按类总大小步进、返回元素地址（原兜底 8+LoadPtr
                //   -> 元素错位越界 0xC0000005）
                elemIsStruct = semantic_ != nullptr &&
                               (semantic_->isStructType(elemCanon) ||
                                semantic_->isClassType(elemCanon));
                const int elemSize = (semantic_ != nullptr)
                                         ? semantic_->typeSizeOf(elemSrc) : 0;
                stride = elemSize > 0 ? elemSize
                                      : (types::isI128(elemCanon) ? 16 : 8);
            } else if (types::canonical(fieldType) == "字符串") {
                // 自举前置 A-1：字符串字段（自身.源码[i]）——字符* 字节步进 1
                elemSrc = "字符";
                stride = 1;
            } else if (elemIsStruct) {
                stride = semantic_->typeSizeOf(fieldType);
            }
            ir::IRValue scaled = emitResult(
                ir::Opcode::Mul,
                {index, ir::IRValue::constant(std::to_string(stride), "i64")},
                "i64", "", node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {ptr, scaled}, "ptr", "",
                                          node->location);
            if (elemIsStruct) {
                lastExpr_ = addr;  // 结构体元素：返回地址（结构体值语义）
                return;
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, mapType(elemSrc), "",
                                   node->location);
            return;
        }
    }
    // 其他对象形式（下标表达式等）：直接取对象值作为地址（防御性）
    // 修复10：数组字段（方形.顶点[i]）按字段数组元素大小步进（坐标[4] -> 坐标 8 字节），
    //   非数组字段按 8 字节（指针假设）
    ir::IRValue obj = genExpr(node->object.get());
    ir::IRValue index = genExpr(node->index.get());
    if (index.type != "i64") {
        index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
    }
    std::int64_t stride = 8;
    // H8 补完（2026-08-25）：隐式类字段对象（向量 数据 T* 的 数据[位置] 读取）——
    //   lookupSrcType 为空（字段不在 IR varStack），按字段源码类型推导步进
    //   （ptrElemStride：类元素按类总大小，原兜底 8 -> 元素错位）。
    if (node->object->getType() == NodeType::IdentifierExpr) {
        const std::string objName =
            static_cast<IdentifierExpr*>(node->object.get())->name;
        std::string st = lookupSrcType(objName);
        if (st.empty() && isInstanceField(objName)) {
            st = classFieldType(currentClass_, objName);
            if (types::isPointer(st)) {
                stride = ptrElemStride(st);
            }
        }
    }
    if (node->object->getType() == NodeType::MemberExpr) {
        // 修复10/10b/10c：数组字段元素步进 + 越界检查（memberObjStructType 递归处理 arrow）
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        const std::string innerType = memberObjStructType(inner);
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName && types::isArray(f.type)) {
                    const std::string elemSrc = types::arrayElemOf(f.type);
                    // H4 根治（99-a）：统一 typeSizeOf（字符串元素=8；原 typeSize=0
                    //   -> 读侧下标不缩放）
                    stride = semantic_->typeSizeOf(elemSrc);
                    if (stride <= 0) stride = 8;
                    emitBoundsCheck(index, types::arrayLenOf(f.type), node->location);
                    break;
                }
                // 自举前置 A-1：字符串成员（点.名[i]）——字符* 字节步进 1
                if (f.name == inner->memberName && types::canonical(f.type) == "字符串") {
                    stride = 1;
                    break;
                }
            }
        }
        // 宿主缺陷1'根治（2026-09-02）：类对象/结构体的指针与数组字段下标读侧
        //   （拷贝构造 其他.数据[索引]：其他 为类对象非 StructDecl，原兜底 8）
        if (stride == 8) {
            const std::string ftype = memberFieldSrcType(inner);
            if (types::isPointer(ftype)) {
                stride = ptrElemStride(ftype);
            } else if (types::isArray(ftype)) {
                const std::string elemSrc = types::arrayElemOf(ftype);
                // H4 根治（99-a）：统一 typeSizeOf（同 ②）
                stride = semantic_->typeSizeOf(elemSrc);
                if (stride <= 0) stride = 8;
                emitBoundsCheck(index, types::arrayLenOf(ftype), node->location);
            }
        }
    }
    // 元素 IR 类型（Task 完善A）：数组字段（一班.分数[i]）按字段数组元素类型；
    //   结构体元素返回地址（供按值传参），普通元素 LoadPtr 按元素类型
    std::string elemIrType = "i64";
    bool elemIsStruct = false;
    if (node->object->getType() == NodeType::MemberExpr) {
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        const std::string innerType = memberObjStructType(inner);
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName && types::isArray(f.type)) {
                    const std::string elemSrc = types::arrayElemOf(f.type);
                    elemIrType = mapType(elemSrc);
                    if (semantic_ != nullptr &&
                        semantic_->isStructType(types::canonical(elemSrc))) {
                        elemIsStruct = true;
                    }
                    break;
                }
                // 自举前置 A-1：字符串成员元素类型 字符（i8）
                if (f.name == inner->memberName && types::canonical(f.type) == "字符串") {
                    elemIrType = "i8";
                    break;
                }
            }
        }
        // 宿主缺陷1'根治（2026-09-02）：类对象/结构体的指针字段下标元素形态
        //   （拷贝构造 其他.数据[索引] 读侧：元素为结构体/类值时返回地址，
        //   原 LoadPtr 只读首 8 字节）。与 IdentifierExpr 分支（:1754）同规则。
        if (!elemIsStruct) {
            const std::string ftype = memberFieldSrcType(inner);
            if (types::isPointer(ftype)) {
                const std::string elemSrc = types::pointeeOf(ftype);
                elemIrType = mapType(elemSrc);
                const std::string elemCanon = types::canonical(elemSrc);
                if (semantic_ != nullptr &&
                    (semantic_->isStructType(elemCanon) ||
                     semantic_->isClassType(elemCanon))) {
                    elemIsStruct = true;
                }
            }
        }
    } else if (node->object->getType() == NodeType::IdentifierExpr) {
        const std::string objName =
            static_cast<IdentifierExpr*>(node->object.get())->name;
        std::string st = lookupSrcType(objName);
        // H8 补完（2026-08-25）：隐式类字段对象（向量 数据 T* 的 数据[位置]）——
        //   lookupSrcType 为空，按字段源码类型推导元素形态（类元素返回地址）
        if (st.empty() && isInstanceField(objName)) {
            st = classFieldType(currentClass_, objName);
        }
        if (types::isPointer(st)) {
            const std::string elemSrc = types::pointeeOf(st);
            elemIrType = mapType(elemSrc);
            const std::string elemCanon = types::canonical(elemSrc);
            // H8 补完（2026-08-25）：类元素（向量<T> 数据 = T*，T=映射）同结构体
            //   返回元素地址（元素是内联类值，LoadPtr 只读 8 字节错误）
            if (semantic_ != nullptr &&
                (semantic_->isStructType(elemCanon) || semantic_->isClassType(elemCanon))) {
                elemIsStruct = true;
            }
        } else if (types::canonical(st) == "字符串") {
            // 自举前置 A-1：字符串变量兜底（i8 字节视图）
            elemIrType = "i8";
        }
    }
    ir::IRValue scaled = emitResult(ir::Opcode::Mul,
                                    {index, ir::IRValue::constant(std::to_string(stride), "i64")},
                                    "i64", "", node->location);
    ir::IRValue addr = emitResult(ir::Opcode::Add, {obj, scaled}, "ptr", "",
                                  node->location);
    if (elemIsStruct) {
        lastExpr_ = addr;  // 结构体元素：返回地址（按值传参/整体拷贝用）
        return;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, elemIrType, "", node->location);
}
} // namespace cn_compiler
