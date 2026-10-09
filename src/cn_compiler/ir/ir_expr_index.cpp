// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出）
//   族 = 下标（visitIndexExpr——指针下标复用途/数组名退化/越界检查/结构体元素地址语义）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   族边界勘定：族内无文件级 static/匿名命名空间依赖，见 plans/021 §3-D1）。
#include <cstdint>
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitIndexExpr(IndexExpr* node) {
    // 对象：数组名（IdentifierExpr）或指针表达式
    if (node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->object.get());
        const std::string unique = lookupVarName(ident->name);
        const std::string srcType = lookupSrcType(ident->name);
        // 259（2026-10-07·#259 静态数组）：顶层/静态局部数组标识符（unique 空·
        //   lookupSrcType 兜底命中数组类型）同走数组路径——原落兜底分支按
        //   stride=8/i64 宽读（乙[2] 错位+打包宽读=大数错值·探针 A5 实录）。
        //   基址=genExpr(静态名)=静态分支返回的符号地址（ir_expr 259/87-a 同款）。
        const bool isArrayObject = types::isArray(srcType);
        if (isArrayObject) {
            // 数组对象：基址 = AddrOf(数组槽0)；元素类型来自数组元素类型
            const std::string elemSrc = types::arrayElemOf(srcType);
            const std::string elemIrType = mapType(elemSrc);
            ir::IRValue base;
            if (!unique.empty()) {
                base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(unique, elemIrType)},
                                  "ptr", unique, node->location);
            } else {
                base = genExpr(node->object.get());
            }
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
            // 955（008 总攻·六位置余三前置根治）：**类容器元素同款地址语义**——
            //   数组元素为 24B/56B 内联对象本体（C 布局·首字段=数据指针），原落
            //   LoadPtr 把「首字段值」当地址读=多解一层：`槽[0].大小()` 的 this
            //   装配=数据指针→读错（p2c 打 0）；传值/赋值消费位同理。与 746-a
            //   「元素(i)=内联元素地址」契约对齐（消费位自行副本化/拷贝构造）。
            if (semantic_ != nullptr &&
                (semantic_->isStructType(types::canonical(elemSrc)) ||
                 semantic_->isClassType(types::canonical(elemSrc)))) {
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
            // T4（306-a）：越界检查（原「由调用方约束」=防线不对称·越界读 UB）
            emitStrBoundsCheck(index, ptr, node->location);
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
            const std::string fieldTypeRaw = classFieldType(currentClass_, ident2->name);
            ir::IRValue ptr = genExpr(node->object.get());  // 字段指针值
            ir::IRValue index = genExpr(node->index.get());
            if (index.type != "i64") {
                index = emitResult(ir::Opcode::Cast, {index}, "i64", "", node->location);
            }
            std::int64_t stride = 8;
            // 331-a（T53 根治）：泛型方法内的字段类型可能是泛型名（`T`/`T*`）——
            //   先做单态化替换（T → 实参类型）再参与步进/分类判定；否则
            //   typeSizeOf("T") 兜底 8（整64 实例巧合正确·i128 需 16 → 元素步进
            //   错位 → 堆越界段错误·实弹 run_err/m53_01）。非泛型上下文原样返回。
            const std::string fieldType = substGenericType(fieldTypeRaw);
            std::string elemSrc = fieldType;
            bool elemIsStruct = semantic_ != nullptr &&
                                semantic_->isStructType(types::canonical(fieldType));
            bool isStringView = false;   // D30 根治（255-a）：发射宽走 i8（字节视图）
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
                // 自举前置 A-1：字符串字段（自身.源码[i]）——字符* 字节视图
                // D30 根治（255-a）：加载宽=i8——原走 mapType("字符")="i32"
                //   -> 4 字节打包读（内容[0]="ax" 得 30817）；mapType 无 "i8"
                //   直通（落自定义->ptr 8 字节），故发射端特判传 "i8"
                elemSrc = "字符";
                stride = 1;
                isStringView = true;
                // T4（306-a）：越界检查（ptr=112 行的字符串指针值）
                emitStrBoundsCheck(index, ptr, node->location);
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
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                                   (isStringView ? std::string("i8") : mapType(elemSrc)), "",
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
            } else if (types::canonical(ftype) == "字符串") {
                // D30 根治（255-a）：拥有型字符串字段下标——字符* 字节视图步进 1
                //   （与 IdentifierExpr 分支的「字符串变量兜底」对称；原兜底 8
                //   -> 内容[i] 地址偏移 i*8=打包/越界读）
                stride = 1;
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
            } else if (types::canonical(ftype) == "字符串") {
                // D30 根治（255-a）：字符串字段元素=i8（字节视图·与字符串变量
                //   兜底及「字符串成员 i8」分支对称；原默认 i64 -> 4/8 字节打包读）
                elemIrType = "i8";
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
    // T4（306-a）：字符串视图形态（成员链字符串/字符串变量兜底——elemIrType=i8
    //   且步进 1）统一越界检查；obj 即字符串指针值（字段槽存的 char*/变量值）。
    //   「字符* 指针」形态 elemIrType=i32（mapType 无 i8 直通）不进入=不误伤。
    if (elemIrType == "i8" && stride == 1) {
        emitStrBoundsCheck(index, obj, node->location);
    }
    ir::IRValue addr = emitResult(ir::Opcode::Add, {obj, scaled}, "ptr", "",
                                  node->location);
    if (elemIsStruct) {
        lastExpr_ = addr;  // 结构体元素：返回地址（按值传参/整体拷贝用）
        return;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, elemIrType, "", node->location);
}
// 259（2026-10-07·#259 静态数组·冻结线 242 迁移）：族③ 通用下标地址自 ir.cpp 机械搬移
//   （纯搬运零行为变更·下标族归属 ir_expr_index；ir.cpp 冻结线禁净增）。
// 族③：通用下标地址（原 725~805 段）——其他对象（指针 p[i] / 数组字段
//   方形.顶点[i]）：地址 = 基址 + index*元素大小（结构体指针/数组字段按
//   元素大小，普通指针8字节；Task 2.7/修复10）。
ir::IRValue IRGenerator::genGenericIndexAddress(IndexExpr* idx) {
        // 其他对象（指针 p[i] / 数组字段 方形.顶点[i]）：地址 = 基址 + index*元素大小
        // （结构体指针/数组字段按元素大小，普通指针8字节；Task 2.7/修复10）
        ir::IRValue index = genExpr(idx->index.get());
        ir::IRValue obj = genExpr(idx->object.get());
        if (index.type != "i64") {
            index = emitResult(ir::Opcode::Cast, {index}, "i64", "", idx->location);
        }
        // 元素步进：数组字段（方形.顶点 / 方形指针->顶点，坐标[4]）按字段数组元素
        //   类型大小；普通指针按 ptrElemStride（结构体指针按总大小）
        std::int64_t stride = 8;
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            // A-3（2026-08）：隐式类字段对象（方法体内 数据[位置] = 值 赋值目标，
            //   lookupSrcType 为空）——按字段源码类型推导步进（向量 数据 T* 的
            //   结构体元素 24 字节，此前固定 8 导致元素错位/越界）
            if (st.empty() && isInstanceField(
                    static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                st = classFieldType(currentClass_,
                                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(st)) {
                    // 数组字段：按 C 布局元素大小（与成员数组字段同规则）
                    const std::string elemSrc = types::arrayElemOf(st);
                    // H4 根治（99-a, 2026-09-13 第九十九轮）：元素大小统一走
                    //   semantic typeSizeOf（原 非结构体走 types::typeSize——该表
                    //   **无「字符串」**（指针类）返回 0 -> 步进 0 -> 字段数组元素
                    //   下标不缩放（`r.名[1]` 读写落元素 0，探针 fldmem2「再读0=乙」
                    //   实证；局部数组路径本就用 typeSizeOf=正确，v2 侧同源正确=
                    //   宿主单侧分叉）。typeSizeOf 对 字符串=8/结构体=总大小/
                    //   标量=自然大小全正确。
                    stride = (semantic_ != nullptr)
                                 ? semantic_->typeSizeOf(elemSrc) : 8;
                    if (stride <= 0) stride = 8;
                    emitBoundsCheck(index, types::arrayLenOf(st), idx->location);
                } else {
                    stride = ptrElemStride(st);
                }
            } else {
                // 259（2026-10-07·#259 静态数组）：顶层/静态局部数组标识符
                //   （lookupSrcType 兜底命中数组类型·unique 空）——按元素大小
                //   步进+越界检查（原落 ptrElemStride 兜底 8：乙[2] 写基址+16
                //   越界槽·探针 A3 实录）。基址=genExpr(静态名)=符号地址。
                if (types::isArray(st)) {
                    const std::string elemSrc259 = types::arrayElemOf(st);
                    stride = (semantic_ != nullptr)
                                 ? semantic_->typeSizeOf(elemSrc259) : 8;
                    if (stride <= 0) stride = 8;
                    emitBoundsCheck(index, types::arrayLenOf(st), idx->location);
                } else {
                    stride = ptrElemStride(st);
                    // 908（任务 092·读侧 306-a emitStrBoundsCheck 对称）：字符串下标
                    //   **写**路径检查（s[i]='x' 形态经此族——读侧 visitIndexExpr 字符串
                    //   分支已有、写侧缺失=防线不对称：w2 探针 s[9]='x' 静默越界堆写
                    //   实证；错误码 2 与读侧/数组防线同码）。
                    if (types::canonical(st) == "字符串") {
                        emitStrBoundsCheck(index, obj, idx->location);
                    }
                }
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 修复10/10b/10c：方形.顶点[0] / 方形指针->顶点[0] — object 为数组字段成员，
            //   元素步进 = 字段数组元素类型大小（memberObjStructType 递归处理 arrow）；
            //   越界检查 = 字段数组长度（错误码2，修复10c）
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            const std::string innerType = memberObjStructType(inner);
            const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType));
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName && types::isArray(f.type)) {
                        const std::string elemSrc = types::arrayElemOf(f.type);
                        // H4 根治（99-a）：同 ①——统一 typeSizeOf（字符串元素=8；
                        //   原 types::typeSize 对字符串=0 -> 步进 0）
                        stride = semantic_->typeSizeOf(elemSrc);
                        if (stride <= 0) stride = 8;
                        emitBoundsCheck(index, types::arrayLenOf(f.type), idx->location);
                        break;
                    }
                }
            }
            // 宿主缺陷1'根治（2026-09-02）：类对象/结构体的指针与数组字段下标
            //   （拷贝构造 其他.数据[索引] 写侧；其他 为类对象非 StructDecl，
            //   原兜底 8 -> T*>8字节元素错位）。按字段源码类型推导：
            //   指针字段 ptrElemStride（结构体/类元素按总大小）、数组字段按元素大小。
            if (stride == 8) {
                const std::string ftype = memberFieldSrcType(inner);
                if (types::isPointer(ftype)) {
                    stride = ptrElemStride(ftype);
                } else if (types::isArray(ftype)) {
                    const std::string elemSrc = types::arrayElemOf(ftype);
                    // H4 根治（99-a）：同 ①/②——统一 typeSizeOf
                    stride = semantic_->typeSizeOf(elemSrc);
                    if (stride <= 0) stride = 8;
                    emitBoundsCheck(index, types::arrayLenOf(ftype), idx->location);
                }
            }
        }
        ir::IRValue scaled = emitResult(
            ir::Opcode::Mul, {index, ir::IRValue::constant(std::to_string(stride), "i64")},
            "i64", "", idx->location);
        return emitResult(ir::Opcode::Add, {obj, scaled}, "ptr", "", idx->location);
}

} // namespace cn_compiler
