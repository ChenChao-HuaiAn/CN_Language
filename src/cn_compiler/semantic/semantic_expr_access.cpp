// CN 语义分析器——表达式检查（D1 行数整改 116-a：自 semantic_expr.cpp 按族拆出）
//   族 = 成员/下标表达式检查（visitMemberExpr + visitIndexExpr）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

namespace {
// 728w（583 扫描·519 同族）：整数文本（已剥 L/U 后缀）超整64 承载域判定——
//   十进制 >19 位必超 / 19 位字典序超 9223372036854775807；0x 十六进制
//   >16 位必超 / 16 位首字 ≥'8'；负形态按绝对量（≥2^63 即超承载——MIN
//   本身作为数组下标亦恒越界）。与 v2 侧 下标字面量超整64域728 同口径
//   （零分叉·双侧同拒）。
bool indexTextExceedsI64(const std::string& text) {
    std::string s = text;
    bool neg = false;
    if (!s.empty() && s[0] == '-') { neg = true; s = s.substr(1); }
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        std::string h = s.substr(2);
        std::size_t i = 0;
        while (i + 1 < h.size() && h[i] == '0') i++;
        h = h.substr(i);
        if (h.size() > 16) return true;
        if (h.size() == 16 && h[0] > '7') return true;
        return false;
    }
    std::size_t i = 0;
    while (i + 1 < s.size() && s[i] == '0') i++;
    s = s.substr(i);
    if (s.size() > 19) return true;
    if (s.size() == 19) {
        if (neg) return s >= std::string("9223372036854775808");
        return s > std::string("9223372036854775807");
    }
    return false;
}
}  // namespace

void SemanticAnalyzer::visitMemberExpr(MemberExpr* node) {
    const std::string memberName = node->memberName;
    // 1360 结果化批3B：无值=非守卫可跟踪形态——value_or 显形空串回落
    //   （isChecked/markChecked 空键守卫原语义保持）
    const std::string objectVar =
        objectVarName(node->object.get()).value_or(std::string());
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
    inAddrBaseCtx_ = true;  // 010（def-init）：对象侧=地址基，抑制整体值读判定
    std::string objectType = checkExpr(node->object.get());
    inAddrBaseCtx_ = false;
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
    // #282（2026-10-08）：方法体内 自身.字段=this->字段 豁免——001 §6 立法明文
    //   （v14 采样轮 p1007_01 实锤：B9 把 this 判 p.字段 误拦·读面 Error·
    //   v2 放行=分叉；静态面已在 visitSelfExpr 精确拒绝，此处豁免安全）。
    const bool thisMemberAccess =
        node->object->getType() == NodeType::SelfExpr &&
        currentContextClass().has_value();
    if (node->isDerefAccess && assignmentTargetDepth_ == 0 &&
        !isStringSemanticType(objectType) && !thisMemberAccess) {
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
            const StructDecl* sd = findStruct(types::canonical(probeType)).value_or(nullptr);
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
            const std::vector<std::string> args = resultTypeArgs(objectType).value_or((std::vector<std::string>()));
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
                lastType_ = canonicalType(optionalTypeArg(objectType).value_or(std::string()));
                return;
            }
        }
        // 其他成员：走降级结构体字段查找（防御）
        const StructDecl* lowered = findStruct(canonicalType(objectType)).value_or(nullptr);
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
        // #284④（2026-10-08 甲案）：`枚举名::成员` 编译期拒绝——枚举成员访问
        //   点号唯一合法（001 §3.5 条文），`::` 保留给模块/包路径限定（08 章）。
        //   原实现经限定名折叠与 `.` 同通道放行（越立法·v14 采样轮 p1007 双侧
        //   甄别实锤：v2 拒宿主放）。
        if (isEnumType(enumName) && node->viaColonColon) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "枚举成员访问须用 点号（'" + enumName + "." +
                                    memberName + "'）——'::' 保留给模块路径限定");
            lastType_ = "未知";
            return;
        }
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
    if (findClass(objectType).has_value()) {
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
    if (!findClass(structType).has_value()) {
        // 588-a 集成验收修（红灯治理 §8.3）：标识符改 ASCII（win ASCII 门禁拦截
        //   非ASCII 标识符——551-a linux 绿/win 红=跨平台静补偿漏面·语义零变化）
        const std::string materializedType = resolveGenericTypeName(structType, node->location);
        if (materializedType != structType) structType = materializedType;
    }
    // 类成员访问（Task 3.1）：对象为类类型 或 类名.静态成员（标识符且是类类型名）
    const ClassInfo* cls = findClass(structType).value_or(nullptr);
    if (cls != nullptr) {
        std::string ownerClass;
        const ClassMemberInfo* member = lookupClassMember(structType, memberName, ownerClass).value_or(nullptr);
        // 122（928·用户裁决乙）：限定构造访问统一拒绝——命中形态（无参构造
        //   signatureKey 空参=纯名·按名查找巧合命中）与构造意图 miss 形态
        //   （memberName==类名=带参构造键带 # 后缀 miss）统一引导初始化列表。
        if (member != nullptr && member->isConstructor) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "构造函数 '" + memberName +
                                    "' 不能经成员访问调用（构造函数不是方法成员）——父类构造须经"
                                    "初始化列表（: " + memberName + "(实参)）调用");
            lastType_ = "未知";
            return;
        }
        if (member == nullptr && memberName == structType) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "构造函数 '" + memberName + "' 不能经成员访问调用——"
                                    "父类构造须经初始化列表（: " + memberName +
                                    "(实参)）调用");
            lastType_ = "未知";
            return;
        }
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
        const ClassInfo* ownerInfo = findClass(ownerClass).value_or(nullptr);
        if (ownerInfo != nullptr) {
            checkAccess(*ownerInfo, *member, contextClass, node->location,
                        member->isConstructor || member->isDestructor ? "方法" : "成员");
        }
        // 方法引用：类型为 方法签名（供 对象.方法() 调用检查；此处给返回类型）
        if (!member->paramTypes.empty() || member->isConstructor ||
            member->isDestructor) {
            // 方法作值（签名串）
            std::string fp = "函数指针<" + member->type + ">(";
            for (std::size_t i = 0; i < member->paramTypes.size(); ++i) {
                if (i > 0) fp += ",";
                fp += member->paramTypes[i];
            }
            fp += ")";
            // 031（#1075 一等公民化·2026-10-18 用户批）：「对象.实例方法」求值
            //   类型改判=方法值<owner类,函数指针<返回>(参数)>（plans/031 §二·②
            //   内部串）——运行时载体=栈上两槽 {this, 方法代码}（值语义·零堆
            //   分配）；调用 `cb(实参)` 零新语法。类名取方法符号 owner 类
            //   （lookupClassMember 口径——同一方法经派生实例取值类型一致）。
            //   静态方法作值（上方 isStatic 分支）=裸 fnptr 语义保持不变（98）。
            const std::string mvOwner = ownerClass.empty() ? structType : ownerClass;
            lastType_ = types::makeMethodValueType(mvOwner, fp);
            // 031 位置守卫（第一版保守面·plans/031 §二·③④）：禁写位拒绝——
            //   返回位/字段写位/下标位/语句位等（R2/R3 语法面）非「变量绑定」
            //   亦非「实参」位时编译拒绝（诊断指路绑定变量/闭包捕获）。
            //   绑定位置（声明初值/标识符赋值右值·bindDepth>0）与实参位
            //   （argDepth>0——目标形参细判移交转换检查点报 fnPtrConvertDiag）
            //   放行；同节点去重标记防双报（单诊断口径）。
            if (method_value_943::bindDepth == 0 &&
                method_value_943::argDepth == 0 &&
                !node->methodValueDiag943) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    method_value_943::positionDiag());
                node->methodValueDiag943 = true;
            }
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
        const InterfaceInfo* iface = findInterface(ifaceName).value_or(nullptr);
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
    const StructDecl* decl = findStruct(structType).value_or(nullptr);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类型 '" + structType + "' 不是结构体/联合体/类类型，无法访问成员 '" +
                            memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段存在性检查
    int offset = fieldOffsetOf(decl, memberName).value_or(-1);
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
            // 010（def-init）：结构体叶子字段读判定（写目标/链中间对象侧跳过）
            if (assignmentTargetDepth_ == 0 && !inAddrBaseCtx_) {
                std::string diName, diSuffix;
                if (defInitPlaceOf(node, diName, diSuffix)) {
                    defInitCheckReadPlace(diName, diSuffix, node->location);
                }
            }
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
    inAddrBaseCtx_ = true;  // 010（def-init）：对象侧=地址基，抑制整体值读判定
    std::string objectType = checkExpr(node->object.get());
    inAddrBaseCtx_ = false;
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
        // 114（912 轮·001 §1.1a① 分层安全第一层立法义务）：字面量下标 ×
        //   静态数组长度——编译期能证明的必须在编译期拦（原放行到运行期
        //   错误码 2 才拦）。读写两路径共用本函数（写路径
        //   assignmentTargetDepth_>0 同样拦截）；不安全函数内同拦（编译期
        //   确定性错误无条件）；常量传播面（变量持有常量初值）属后续
        //   常量折叠波。rustc 对照：`a[5]`（len 3）常量下标 deny，unsafe
        //   块内同样报。
        // 1360 结果化：has_value() 显形分流（无值=非数组·不进编译期拦截）
        const cn::Optional<int> arrLen114Opt = types::arrayLenOf(objectType);
        if (arrLen114Opt.has_value() && arrLen114Opt.value() > 0 &&
            isIntLiteralExpr(node->index.get())) {
            const int arrLen114 = arrLen114Opt.value();
            const Expr* idxE = node->index.get();
            std::int64_t idxVal = 0;
            if (idxE->getType() == NodeType::IntegerLiteral) {
                idxVal = static_cast<const IntegerLiteral*>(idxE)->value;
            } else {
                // 一元负号字面量（isIntLiteralExpr 已限定该两形态）
                const UnaryExpr* u = static_cast<const UnaryExpr*>(idxE);
                const std::int64_t v =
                    static_cast<const IntegerLiteral*>(u->operand.get())->value;
                idxVal = static_cast<std::int64_t>(
                    ~static_cast<std::uint64_t>(v) + 1);
            }
            // 728w（583 扫描·519 同族）：超整64 承载下标文本先拦——19 位
            //   ≥2^63 / ≥20 位文本在 parser 解析时已回绕毒化（idxVal 读不到
            //   真实值·实测毒化 0 放行到运行期错误码 2 才拦），文本级判域对
            //   任意数组长度（长度 ≤ 整64 域）均可证越界；负形态绝对量
            //   ≥2^63 恒越界同拦（v2 侧 语义检查初始化.cn 同口径·双侧同拒）。
            const IntegerLiteral* lit728 = nullptr;
            if (idxE->getType() == NodeType::IntegerLiteral) {
                lit728 = static_cast<const IntegerLiteral*>(idxE);
            } else {
                lit728 = static_cast<const IntegerLiteral*>(
                    static_cast<const UnaryExpr*>(idxE)->operand.get());
            }
            const std::string text728 = types::stripLiteralSuffix(lit728->raw);
            if (indexTextExceedsI64(text728)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->index->location,
                    "编译期下标越界：常量下标 " + text728 +
                    " 超出数组长度 " + std::to_string(arrLen114) +
                    "（001 §1.1a①——编译期能证必须编译期拦；运行期越界检查"
                    "错误码 2 保留兜底）");
            } else if (idxVal < 0 || idxVal >= arrLen114) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->index->location,
                    "编译期下标越界：常量下标 " + std::to_string(idxVal) +
                    " 超出数组长度 " + std::to_string(arrLen114) +
                    "（001 §1.1a①——编译期能证必须编译期拦；运行期越界检查"
                    "错误码 2 保留兜底）");
            }
        }
        // 数组对象：元素类型即结果
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::arrayElemOf(objectType);
        // 010（def-init）：数组元素读判定（写目标 assignmentTargetDepth_>0 /
        //   链中间对象侧 inAddrBaseCtx_ 跳过——外层节点精确判定）
        if (assignmentTargetDepth_ == 0 && !inAddrBaseCtx_) {
            std::string diName, diSuffix;
            if (defInitPlaceOf(node, diName, diSuffix)) {
                defInitCheckReadPlace(diName, diSuffix, node->location);
            }
        }
        return;
    }
    if (objectType == "字符*") {
        // 069 乙（829 轮·用户裁决）：字符*（借用字符串视图）下标读静默错值根除——
        //   原落下方 指针分支（所指=字符·下标按 4 字节码点步进）与视图 UTF-8
        //   字节序列语义错位（实测 [0] 对/[1..] 乱）；Rust 对照：&str 不提供
        //   下标（无 Index<usize>）——按字节读须显式转换（字符串复制 转拥有）。
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "字符*（借用字符串视图）不支持下标访问——请先 字符串复制(...) 转为拥有字符串");
        lastType_ = "整8";   // 语义上“意图为字节读”——按字节结果标注以减少连带诊断噪音
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
    //   字符*（UTF-8 字节视图），词法器逐字符遍历不再每字符一次 子串 malloc。
    //   T48 裁决甲（2026-09-20 用户批准）：下标语义=按 UTF-8 字节索引，结果
    //   类型 整8（带符号字节值——与 IR 层 i8 符号扩展装载自 A-1 起即一致；
    //   原「字符」标注与「字符=Unicode 标量值」定义矛盾，属规范空白）。
    //   完整字符经 字符转字符串（码点路径）/字符串子串（字节路径）按需构造。
    if (objectType == "字符串") {
        // 618（001 §4.4 字符串下标语义·T48/112 甲·910/596 同族收口）：
        //   自增/自减写目标=字符串下标——字符串为不可变拥有型，下标只读，
        //   自增/自减=隐式写 → 语义层编译期显式拒绝（v2 语义层同构同文）。
        //   原形态=语义层缺位：s[0]++ 落 IR 层内部错误（#648「指针步进源类型
        //   未登记 '字符串'」——非语义拒绝）；"ABCDE"[0]++ 更糟=rc=0 静默落码
        //   写字符串常量段（p618b 实证）。判定=身份比较（本下标节点即自增/
        //   自减直接操作数·嵌套下标读不误判）；lastType_ 保持元素类型语义
        //   （检链下游数值检查零连带噪音）。
        if (currentIncDecOperand_ == node) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "字符串下标只读，不可自增/自减——字符串为不可变拥有型，"
                "原地修改请用 字符 数组或不安全区指针（112 甲）");
            lastType_ = "整8";
            return;
        }
        // 596（2026-10-13·910/112 甲扩全形态）：字符串下标**写**=编译期拒绝——
        //   字符串为不可变拥有型（001 §1.1a）。910 只在赋值 target case 拦
        //   标识符形态（变量）；字面量（"ABC"[0]=x 写穿常量段+驻留共享跨表达
        //   式污染·864 轮探针实测）与调用返回串临时物（写悬垂临时）静默放行
        //   UB=安全红线。v2 侧 族②检查下标左值形态 同拒既有（双侧对齐）。
        //   写语境=assignmentTargetDepth_>0（复合赋值同拦）；标识符形态已由
        //   910 在 target case 拒（lvalueOk=false 提前返回，此处不重复报）。
        if (assignmentTargetDepth_ > 0 &&
            node->object->getType() != NodeType::IdentifierExpr) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "下标左值赋值未支持（字符串下标只读）——字符串为不可变"
                "拥有型，原地修改请用 字符 数组或不安全区指针（112 甲）");
            lastType_ = "未知";
            return;
        }
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "字符串下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = "整8";
        return;
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "下标访问要求数组或指针对象，实际为 '" + objectType + "'");
    lastType_ = "未知";
}
} // namespace cn_compiler
