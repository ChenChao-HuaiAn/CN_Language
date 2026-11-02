// CN-IR生成器实现（#1882 行数整改：自 ir_expr_index.cpp 按族拆出·先拆再加纪律）
//   族 = 通用下标地址（genGenericIndexAddress——族③·259 冻结线 242 自 ir.cpp 迁入 ir_expr_index
//   后再迁本文件；写地址面：指针 p[i] / 数组字段 方形.顶点[i] / 类名.静态数组字段[i] / 嵌套
//   下标 m[i][j] 的 基址+index*元素大小 计算）；纯重构零行为变更（成员函数实现搬迁——声明仍在
//   ir_generator.hpp；族边界勘定：族内无文件级 static/匿名命名空间依赖）。
#include <cstdint>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

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
            // 1360 结果化：value_or("") 显形（miss=空→类字段回落链·原语义）
            std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name).value_or("");
            // A-3（2026-08）：隐式类字段对象（方法体内 数据[位置] = 值 赋值目标，
            //   lookupSrcType 无值）——按字段源码类型推导步进（向量 数据 T* 的
            //   结构体元素 24 字节，此前固定 8 导致元素错位/越界）
            if (st.empty() && isInstanceField(
                    static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                st = classFieldType(fieldScopeClass(),
                                    static_cast<IdentifierExpr*>(idx->object.get())->name).value_or("");  // 1472 同锚
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
                    // 655（648 余量④-b 显形·同 ④-a 写法）：实例字段数组元素
                    //   宽度 typeSizeOf miss=内部缺陷，报诊断后保留 8 兜底。
                    stride = (semantic_ != nullptr)
                                 ? semantic_->typeSizeOf(elemSrc).value_or(8) : 8;
                    if (stride <= 0) {
                        if (semantic_ != nullptr) {
                            diagnostics_.report(DiagnosticLevel::Error,
                                idx->location,
                                "内部错误: 数组字段元素类型 '" + elemSrc +
                                    "' 宽度未登记，步进终态 8 兜底已不可静默（#655）");
                        }
                        stride = 8;
                    }
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
                    const std::string topLevelStaticElemSrc = types::arrayElemOf(st);
                    // 655（648 余量④-b 显形·同上）：静态数组元素宽度 miss 显形。
                    stride = (semantic_ != nullptr)
                                 ? semantic_->typeSizeOf(topLevelStaticElemSrc).value_or(8) : 8;
                    if (stride <= 0) {
                        if (semantic_ != nullptr) {
                            diagnostics_.report(DiagnosticLevel::Error,
                                idx->location,
                                "内部错误: 静态数组元素类型 '" + topLevelStaticElemSrc +
                                    "' 宽度未登记，步进终态 8 兜底已不可静默（#655）");
                        }
                        stride = 8;
                    }
                    emitBoundsCheck(index, types::arrayLenOf(st), idx->location);
                } else {
                    stride = ptrElemStride(st);
                    // 908（任务 092·读侧 306-a emitStrBoundsCheck 对称）：字符串下标
                    //   **写**路径检查（s[i]='x' 形态经此族——读侧 visitIndexExpr 字符串
                    //   分支已有、写侧缺失=防线不对称：w2 探针 s[9]='x' 静默越界堆写
                    //   实证；错误码 2 与读侧/数组防线同码）。
                    // #1215：此处保留 canonical 比较（不再改为无分配判定）——
                    //   本 else 块文本是 miss 门禁 else业务 的存量基线锚点，改动
                    //   块内文本会使存量豁免失配（门禁=只减不增，禁新增位置）。
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
            // 1360 结果化：value_or("") 显形（miss=findStruct("") 恒 null·原语义）
            const std::string innerType = memberObjStructType(inner).value_or("");
            const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerType)).value_or(nullptr);
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName && types::isArray(f.type)) {
                        const std::string elemSrc = types::arrayElemOf(f.type);
                        // H4 根治（99-a）：同 ①——统一 typeSizeOf（字符串元素=8；
                        //   原 types::typeSize 对字符串=0 -> 步进 0）
                        // 655（648 余量④-b 显形·同上）：成员对象数组字段元素
                        //   宽度 miss 显形。
                        stride = semantic_->typeSizeOf(elemSrc).value_or(8);
                        // 静默豁免业务值: 单边守卫——宽度 miss 先报内部错误诊断（#655 显形）后步进终态 8，非静默默认值
                        if (stride <= 0) {
                            diagnostics_.report(DiagnosticLevel::Error,
                                idx->location,
                                "内部错误: 成员数组字段元素类型 '" + elemSrc +
                                    "' 宽度未登记，步进终态 8 兜底已不可静默（#655）");
                            stride = 8;
                        }
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
                // 1360 结果化：value_or("") 显形（miss=非指针/非字符串判定假·原语义）
            const std::string ftype = memberFieldSrcType(inner).value_or("");
                if (types::isPointer(ftype)) {
                    stride = ptrElemStride(ftype);
                } else if (types::isArray(ftype)) {
                    const std::string elemSrc = types::arrayElemOf(ftype);
                    // H4 根治（99-a）：同 ①/②——统一 typeSizeOf
                    stride = semantic_->typeSizeOf(elemSrc).value_or(8);
                    if (stride <= 0) stride = 8;
                    emitBoundsCheck(index, types::arrayLenOf(ftype), idx->location);
                }
            }
            // #1882（#1445 第二形态·写地址面）：下标对象=类名.静态成员——
            //   classStaticFieldTypeOfMember 沿基类链解析。数组=元素 typeSizeOf 步进
            //   （Optional 分流·无值=显形·648 禁默认 8 踩槽）+静态长度越界检查
            //   （基址=genExpr(类名.静态数组字段)=符号地址·handleClassMemberExpr
            //   数组分支同款本体语义——与 v2 写侧静态通道对称）。字符串下标写=
            //   语义层 112 甲已拒不入此。原双推导链对类名基座 miss → stride 兜 8。
            if (stride == 8) {
                const cn::Optional<std::string> staticFieldTypeAddr =
                    classStaticFieldTypeOfMember(*semantic_, inner);
                if (staticFieldTypeAddr.has_value() &&
                    types::isArray(staticFieldTypeAddr.value())) {
                    const std::string staticArrayElemSrcAddr =
                        types::arrayElemOf(staticFieldTypeAddr.value());
                    const cn::Optional<int> staticArrayElemSizeAddr =
                        semantic_->typeSizeOf(staticArrayElemSrcAddr);
                    if (staticArrayElemSizeAddr.has_value()) {
                        stride = staticArrayElemSizeAddr.value();
                    } else {
                        diagnostics_.report(DiagnosticLevel::Error, idx->location,
                            "内部错误: 静态数组字段元素类型 '" + staticArrayElemSrcAddr +
                                "' 宽度未登记 ——静默步进8兜底已废除（#648/#1882）");
                    }
                    emitBoundsCheck(index, types::arrayLenOf(staticFieldTypeAddr.value()),
                                    idx->location);
                }
            }
        } else if (idx->object->getType() == NodeType::IndexExpr) {
            // #1687（2026-10-26·嵌套下标写 m[i][j] = v 地址面）：对象=子数组
            //   首地址——步长按对象源码类型剥维推导 + 外层长度越界检查（m[i] ->
            //   整32[3]：步长 4·界 3；写 基地址+i*12+j*4）。原落兜底 8=j*8 错位
            //   写（样本1 648 拒编译的伴生地址面）。对象无值/标量=恢复路径。
            const cn::Optional<std::string> objT = exprSrcType(idx->object.get());
            if (objT.has_value() && types::isArray(objT.value())) {
                const std::string elemSrcNest = types::arrayElemOf(objT.value());
                stride = (semantic_ != nullptr)
                             ? semantic_->typeSizeOf(elemSrcNest).value_or(0) : 0;
                // 静默豁免业务值: 单边守卫——宽度 miss 先报内部错误诊断（#648 显形）后步进终态 8，非静默默认值
                if (stride <= 0) {
                    // 648（miss 不可静默）：宽度未登记=内部缺陷显形后按 8 兜底
                    if (semantic_ != nullptr) {
                        diagnostics_.report(DiagnosticLevel::Error, idx->location,
                            "内部错误: 嵌套下标元素类型 '" + elemSrcNest +
                                "' 宽度未登记 ——静默步进8兜底已废除（#648）");
                    }
                    stride = 8;
                }
                emitBoundsCheck(index, types::arrayLenOf(objT.value()), idx->location);
            } else if (objT.has_value() && types::isPointer(objT.value())) {
                stride = ptrElemStride(objT.value());
            }
        }
        ir::IRValue scaled = emitResult(
            ir::Opcode::Mul, {index, ir::IRValue::constant(std::to_string(stride), "i64")},
            "i64", "", idx->location);
        return emitResult(ir::Opcode::Add, {obj, scaled}, "ptr", "", idx->location);
}

} // namespace cn_compiler
