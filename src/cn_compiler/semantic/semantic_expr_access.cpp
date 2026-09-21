// CN 语义分析器——表达式检查（D1 行数整改 116-a：自 semantic_expr.cpp 按族拆出）
//   族 = 成员/下标表达式检查（visitMemberExpr + visitIndexExpr）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {

void SemanticAnalyzer::visitMemberExpr(MemberExpr* node) {
    const std::string memberName = node->memberName;
    const std::string objectVar = objectVarName(node->object.get());
    // 188-a（D6·plans/023 B11 变量常量传播）：`无.字段` 字面量形态（编译期常量
    //   空指针成员访问）——硬错误（plans/023 §九 负例 `点* q = 无; 返回 q.x;` 的
    //   字面量孪生形态；原实现只报「类型 '空类型' 不是结构体…」间接错误）。
    if (node->object->getType() == NodeType::NullLiteral) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "编译期常量空指针成员访问（确定性错误；plans/023 B11）");
        lastType_ = "未知";
        return;
    }
    std::string objectType = checkExpr(node->object.get());
    // v2.1（2026-09-03，用户裁决废除 ->）：成员访问统一 .——对象为指针时
    //   自动解引用一级（≡ (*对象).成员，Go 先例）。isDerefAccess 按对象类型
    //   写回（IR 层据此选基址：指针值 / 对象地址）；解析层恒 false。
    //   结果/可选/枚举/接口对象均非此指针语义或各自先行处理，统一置位无害。
    node->isDerefAccess = types::isPointer(objectType);
    // 188-a（D6 B11 变量常量传播）：成员访问基对象为标识符 → 使用点登记
    //   （仅指针基对象=自动解引用一级；判定在函数体检查收尾统一做）
    if (node->isDerefAccess &&
        node->object->getType() == NodeType::IdentifierExpr) {
        noteNullUse(static_cast<IdentifierExpr*>(node->object.get())->name,
                    node->location, "成员访问");
    }
    // 150-a（plans/023 B9 实施）：指针成员访问（p.字段 自动解引用一级）观察期
    //   警告（排除字符串语义=字符串视图；赋值场景的写面由 B7/A2 族承担）。
    if (node->isDerefAccess && assignmentTargetDepth_ == 0 &&
        !isStringSemanticType(objectType)) {
        reportUnsafeBoundary(node->location, "指针成员访问", "指针成员访问（p.字段）");
    }
    // plans/019 阶段4（2026-09-10）：安全区边界观察期——联合体字段访问（共享
    //   内存无 tag=类型安全结构性缺口，Rust union 同为 unsafe-only）应在
    //   不安全 函数 内
    {
        std::string probeType = types::isPointer(objectType)
                                    ? types::pointeeOf(objectType)
                                    : objectType;
        if (!probeType.empty()) {
            const StructDecl* sd = findStruct(types::canonical(probeType));
            if (sd != nullptr && sd->isUnion) {
                reportUnsafeBoundary(node->location, "联合体字段访问",
                                   probeType + "." + memberName);
            }
        }
    }
    // 结果/可选成员检查（Task 3.5 规则2/3）：.正常/.有值/.值/.错误
    if (isResultType(objectType) || isOptionalType(objectType)) {
        // 结果<T,E> / 可选<T> 经降级为合成结构体，其成员 .正常/.有值/.值/.错误
        // 在此处做强制检查规则分析；成员类型按降级结构体字段推导。
        checkResultMember(objectType, memberName, node->location, objectVar);
        // 推导成员类型：结果.正常 -> 布尔；可选.有值 -> 布尔；结果.值 -> T；可选.值 -> T；
        // 结果.错误 -> E
        if (isResultType(objectType)) {
            const std::vector<std::string> args = resultTypeArgs(objectType);
            if (memberName == "正常") {
                lastType_ = "布尔";
                return;
            }
            if (memberName == "值" && args.size() == 2) {
                lastType_ = canonicalType(args[0]);
                return;
            }
            if (memberName == "错误" && args.size() == 2) {
                lastType_ = canonicalType(args[1]);
                return;
            }
        }
        if (isOptionalType(objectType)) {
            if (memberName == "有值") {
                lastType_ = "布尔";
                return;
            }
            if (memberName == "值") {
                lastType_ = canonicalType(optionalTypeArg(objectType));
                return;
            }
        }
        // 其他成员：走降级结构体字段查找（防御）
        const StructDecl* lowered = findStruct(canonicalType(objectType));
        if (lowered != nullptr) {
            for (const auto& f : lowered->fields) {
                if (f.name == memberName) {
                    lastType_ = canonicalType(
                        resolveGenericTypeName(f.type, node->location));
                    return;
                }
            }
        }
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结果/可选 类型没有成员 '" + memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 枚举值引用：枚举名.成员（如 颜色.红，Task 2.7）
    // object 为标识符且其类型是枚举类型名 → 求值为枚举成员整数值
    // （v2.1：枚举类型名非指针，原 !isArrow 守卫随 -> 废除删除）
    if (node->object->getType() == NodeType::IdentifierExpr) {
        const std::string enumName = objectType;
        std::int64_t enumValue = 0;
        if (isEnumType(enumName) && enumValueOf(enumName, memberName, enumValue)) {
            lastType_ = enumName;  // 枚举值类型为枚举类型名（可与整型互转）
            return;
        }
        if (isEnumType(enumName)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "枚举 '" + enumName + "' 没有成员 '" + memberName + "'");
            lastType_ = "未知";
            return;
        }
    }
    std::string structType;  // 承载字段的结构体/类类型名
    // v2.1（2026-09-03，用户裁决废除 ->）：成员访问统一 .——对象为指针时
    //   自动解引用**一级**（≡ (*对象).成员，Go 先例；自身 this 指针同此剥法，
    //   原专门分支并入）；多级指针不继续解引用，报错可见化。
    // 簇⑥根治（2026-09-04，宿主缺陷优先纪律）：泛型实例名可含实参星号
    //   （盒子<整64*> -> 盒子$整64*——合成名按 canonical 保留尾 *），尾 * 是
    //   实例名一部分而非对象指针语义——原 isPointer 按尾 * 判定会把实例名剥成
    //   盒子$整64（findClass 失败，「不是结构体/联合体/类类型」，最小复现=
    //   泛型<类型 T> 类 盒子 + 盒子<整64*> b; b.值，探针打印实证 varType 全程
    //   正确、丢点在此消费侧）。修复：先按**全名**查类命中即用原名（真指针
    //   盒子$整64** 不会命中类表，自然落入下方解引用分支——行为不回退）。
    if (findClass(objectType) != nullptr) {
        structType = canonicalType(objectType);
    } else if (types::isPointer(objectType)) {
        const std::string pointee = types::pointeeOf(objectType);
        if (types::isPointer(pointee)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "多级指针 '" + objectType +
                                    "' 不自动解引用：请显式 * 解引用一级后再访问成员 '" +
                                    memberName + "'");
            lastType_ = "未知";
            return;
        }
        structType = canonicalType(pointee);
    } else {
        structType = canonicalType(objectType);
    }
    // T95（551-a）：泛型方法体内引用参数（向量<T>& v）的类型文本经
    //   substTypeParam 文本替换保留尖括号形态（向量<整64>）——未经实例物化，
    //   findClass miss 误报「不是结构体/联合体/类类型」连带返回类型未知
    //   （p2 探针：返回 v.元素() 报「无法将 '未知' 隐式转换为返回类型」）。
    //   类表未命中时先 resolveGenericTypeName 物化为合成名（向量$整64）再查
    //   （幂等：非泛型/已物化/类表已命中路径零行为变化）。
    if (findClass(structType) == nullptr) {
        // 588-a 集成验收修（红灯治理 §8.3）：标识符改 ASCII（win ASCII 门禁拦截
        //   非ASCII 标识符——551-a linux 绿/win 红=跨平台静补偿漏面·语义零变化）
        const std::string materializedType = resolveGenericTypeName(structType, node->location);
        if (materializedType != structType) structType = materializedType;
    }
    // 类成员访问（Task 3.1）：对象为类类型 或 类名.静态成员（标识符且是类类型名）
    const ClassInfo* cls = findClass(structType);
    if (cls != nullptr) {
        std::string ownerClass;
        const ClassMemberInfo* member = lookupClassMember(structType, memberName, ownerClass);
        if (member == nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类 '" + structType + "' 没有成员 '" + memberName + "'");
            lastType_ = "未知";
            return;
        }
        // 静态成员访问检查（Task 3.9）：类名.静态成员 允许；实例.静态成员 也允许；
        //   非静态成员经 类名. 访问 -> 错误（无实例）
        // 判断"类名.成员"：标识符本身是已注册类名（非类类型变量！变量 a 类型为
        //   账户 时 a.余额 是实例访问，不应误判为 类名.静态访问）
        bool objectIsTypeName = false;
        if (node->object->getType() == NodeType::IdentifierExpr) {
            const std::string& objName =
                static_cast<IdentifierExpr*>(node->object.get())->name;
            objectIsTypeName = isClassType(objName);
        }
        if (objectIsTypeName && !member->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "静态访问 '类名." + memberName +
                                    "' 要求成员为静态（非静态成员须经实例访问）");
            lastType_ = "未知";
            return;
        }
        // 缺陷根治（第九十三轮，2026-09-13 B2 立案复现）：实例.静态成员 —— 静态
        //   成员须经类名访问（Rust E0599 同款：关联常量/函数不经实例访问；规范
        //   06-十「类外访问用 类名.静态成员」）。原实现静默按实例路径生成
        //   （this+字段偏移）：静态字段不在实例字段表 → classFieldOffset=-1 →
        //   读 this 首 8 字节/暗写错位（探针 M5b/M5c 实证：读恒 0、写不生效）。
        if (!objectIsTypeName && member->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "静态成员 '" + memberName + "' 须经类名访问（" +
                                    structType + "." + memberName +
                                    "），不能经实例访问");
            lastType_ = "未知";
            return;
        }
        // 静态成员引用：直接给类型（供 IR 层取静态字段/静态方法地址）
        if (member->isStatic) {
            // P3-23：静态方法作值（函数指针）——类型为方法签名
            if (cls->methods.find(memberName) != cls->methods.end()) {
                std::string fp = "函数指针<" + member->type + ">(";
                for (std::size_t i = 0; i < member->paramTypes.size(); ++i) {
                    if (i > 0) fp += ",";
                    fp += member->paramTypes[i];
                }
                fp += ")";
                lastType_ = fp;
                return;
            }
            lastType_ = member->type;
            return;
        }
        // 访问控制检查（Task 3.4）：非类上下文访问 私有/保护 成员 -> 错误
        const std::string contextClass = contextClassStack_.empty()
                                             ? ""
                                             : contextClassStack_.back();
        // 实例成员访问控制（自身.私有字段 在子类访问父类私有 -> 报错）
        const ClassInfo* ownerInfo = findClass(ownerClass);
        if (ownerInfo != nullptr) {
            checkAccess(*ownerInfo, *member, contextClass, node->location,
                        member->isConstructor || member->isDestructor ? "方法" : "成员");
        }
        // 方法引用：类型为 方法签名（供 对象.方法() 调用检查；此处给返回类型）
        if (!member->paramTypes.empty() || member->isConstructor ||
            member->isDestructor) {
            // 方法作值（函数指针类型）
            std::string fp = "函数指针<" + member->type + ">(";
            for (std::size_t i = 0; i < member->paramTypes.size(); ++i) {
                if (i > 0) fp += ",";
                fp += member->paramTypes[i];
            }
            fp += ")";
            lastType_ = fp;
            // P3-23 补完：实例方法作值（对象.实例方法）标记——IR 合成"绑定 this"
            //   闭包（须先赋给变量经闭包变量调用；不可直接作裸 fnptr 实参，D2 诊断）
            node->isMethodValue = true;
            return;
        }
        lastType_ = member->type;
        return;
    }
    // P3-19：接口类型对象成员访问（图形.方法，图形 静态类型为接口）——
    //   解析为接口方法（公开；类型=签名返回类型；槽位由 IR 经 interfaceSlot 查询）
    {
        const std::string ifaceName = types::isPointer(objectType)
            ? canonicalType(types::pointeeOf(objectType))
            : canonicalType(objectType);
        const InterfaceInfo* iface = findInterface(ifaceName);
        if (iface != nullptr) {
            const auto imit = iface->methods.find(memberName);
            if (imit == iface->methods.end()) {
                if (structType != ifaceName) {
                    // 结构体名正好也是接口名等异常情形，走底层逻辑
                }
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "接口 '" + ifaceName + "' 没有成员 '" + memberName + "'");
                lastType_ = "未知";
                return;
            }
            lastType_ = imit->second.type;
            return;
        }
    }
    // 结构体/联合体字段访问（Task 2.7）
    const StructDecl* decl = findStruct(structType);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类型 '" + structType + "' 不是结构体/联合体/类类型，无法访问成员 '" +
                            memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段存在性检查
    int offset = fieldOffsetOf(decl, memberName);
    if (offset < 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结构体 '" + decl->name + "' 没有成员 '" + memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段类型（从声明中查找）——宿主缺陷根治（2026-08-25）：泛型容器字段
    //   （结构体 { 向量<整64> 数据 }）类型须归一为实例名（向量$整64）——原仅
    //   canonicalType（模板形式 向量<整64>），后续 数据.方法() 的 findClass
    //   （类表存 向量$整64）未命中 -> 误报"不是结构体/联合体/类类型，无法访问
    //   成员"（与函数参数 resolveGenericTypeName 归一一致）。
    for (const auto& f : decl->fields) {
        if (f.name == memberName) {
            lastType_ = canonicalType(
                resolveGenericTypeName(f.type, node->location));
            return;
        }
    }
    lastType_ = "未知";
}
void SemanticAnalyzer::visitIndexExpr(IndexExpr* node) {
    // 188-a（D6·plans/023 B11 变量常量传播）：`无[i]` 字面量形态（编译期常量
    //   空指针下标访问）——硬错误（原实现只报后续类型转换间接错误）。
    if (node->object->getType() == NodeType::NullLiteral) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "编译期常量空指针下标访问（确定性错误；plans/023 B11）");
        lastType_ = "未知";
        return;
    }
    std::string objectType = checkExpr(node->object.get());
    std::string indexType = checkExpr(node->index.get());
    if (objectType == "未知") {
        lastType_ = "未知";
        return;
    }
    // 188-a（D6 B11 变量常量传播）：下标基对象为标识符（指针/数组/字符串）→
    //   使用点登记（读写两路径共用本函数——写路径 assignmentTargetDepth_>0 仅
    //   抑制观察期警告，空指针下标=确定性 UB 两侧同判）
    if (node->object->getType() == NodeType::IdentifierExpr &&
        isNullConstEligibleType(objectType)) {
        noteNullUse(static_cast<IdentifierExpr*>(node->object.get())->name,
                    node->location, "下标访问");
    }
    // 数组退化：数组名作下标对象（数据[i]）按元素类型处理
    if (isArrayType(objectType)) {
        // 数组对象：元素类型即结果
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::arrayElemOf(objectType);
        return;
    }
    if (isPointerType(objectType)) {
        // 指针对象（p[i] 等价 *(p+i)）：结果类型为所指元素类型
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::pointeeOf(objectType);
        // 150-a（plans/023 B8 实施）：指针下标读观察期警告——写路径由赋值
        //   target case 的 A2「指针下标写」报（assignmentTargetDepth_>0 抑制本处）；
        //   排除字符串语义（字符串/字符*=字符串视图）。
        if (assignmentTargetDepth_ == 0 && !isStringSemanticType(objectType)) {
            reportUnsafeBoundary(node->location, "指针下标读", "指针下标（p[i]）");
        }
        return;
    }
    // 自举前置 A-1（plans/004）：字符串[i] 逐字节 O(1) 访问——字符串即
    //   字符*（UTF-8 字节视图），下标结果类型 字符。词法器逐字符遍历
    //   不再每字符一次 子串 malloc（百万级分配不可接受）。
    if (objectType == "字符串") {
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "字符串下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = "字符";
        return;
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "下标访问要求数组或指针对象，实际为 '" + objectType + "'");
    lastType_ = "未知";
}
} // namespace cn_compiler
