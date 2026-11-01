// CN 语义分析器实现：类型引用存在性检查族（#1949·自 semantic_types.cpp 拆分）
//   职责=类型引用存在性判定/诊断单面（对齐 v2 语义分析中 检查类型引用·
//   plans/018 类型分桶）：变量/参数/返回类型引用的未声明名编译期拒——
//   「先拆再加」纪律落点（semantic_types.cpp 撞 1000 冻结线·本文件承接
//   #1949 新增三函数·宿主新代码归属速查：语义检查→semantic）。
#include <string>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

// 类型引用递归深度上限（环防护·CN 无自引用值语义；标识符禁嵌任务号纪律·
//   isPortableType 同款 12 层窗口契约）
constexpr int kTypeRefMaxDepth = 12;

// ==================== #1949：类型引用存在性检查（对齐 v2 检查类型引用） ====================
// 背景（plans/018 类型分桶·v2 语义分析中 同族）：此前宿主变量/参数/返回类型
//   引用零存在性检查——用户拼错类型名（幽灵 g;）静默放行产出错误程序，v2 同位
//   报「未声明的类型: X」（n10 探针双侧对拍实锤=零分叉缺陷）。
// 判定序与 v2 检查类型引用同构：空/泛型参数名/指针引用后缀/基础文本/函数指针/
//   方法值/枚举/数组放行；泛型模板源形态（名<实参,...>）头名合法+实参递归；
//   限定名/纯名查表 miss=未声明。挂点=visitVarDecl（非静态·v2 检查声明类型
//   节点同构）/registerFunction 返回/resolveParamTypes（v2 语义检查尾 参数/
//   返回挂点同构）。诊断落递归 miss 内层（v2 递归文案同构：幽灵[3] 报 幽灵）。
bool SemanticAnalyzer::checkTypeReferenceInner(const std::string& type,
                                               const SourceLocation& loc,
                                               bool diagnose, int depth) const {
    // miss 报点：内层文本（v2 递归检查类型引用在 miss 层报该层文本同构）
    const auto reject = [&](const std::string& text) {
        if (diagnose) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "未声明的类型: " + text);
        }
        return false;
    };
    if (type.size() == 0) {
        return type.empty();   // 空文本提前返回（契约：空文本无可查引用）
    }
    if (depth >= kTypeRefMaxDepth) {
        return true;   // 环防护豁免窗口（CN 无自引用值语义·isPortableType 同款契约）
    }
    // 泛型类方法体上下文：类型参数名（T/K/V…）按实参绑定放行（v2 是泛型参数名豁免同构）
    const bool genericParamBound = genericTypeParams_.count(type) != 0;
    if (genericParamBound) {
        return genericParamBound;   // 泛型参数名豁免（方法体上下文按实参绑定·上游已声明域）
    }
    // 指针/引用后缀：剥净判基名（v2 同款循环剥·剥空=纯后缀异常形态放行）
    std::string core = type;
    while (!core.empty() && (core.back() == '*' || core.back() == '&')) core.pop_back();
    if (core.size() == 0) {
        return core.empty();   // 剥空=纯后缀异常形态提前返回（调用方保证形态合法）
    }
    // 基础类型文本（v2 是基础类型文本同集合·「指针」=通用指针别名 001 条文同列）
    static const char* const kBasicTexts[] = {
        "整8", "整16", "整32", "整64", "整128",
        "正8", "正16", "正32", "正64", "正128",
        "布尔", "字符", "字符串", "浮32", "浮64", "空类型", "指针"};
    for (const char* basic : kBasicTexts) {
        if (core == basic) return true;
    }
    // 类型位关键字/内置词豁免（词法关键字与内置模板头有自己的语义处理通道·
    //   只查登记表会把语言内置词误判未声明——预验 274/1586 打红实证）：
    //   「自动」=162-a 已删关键字·类型推断哨兵词面（Kw_Auto 枚举已删但语义
    //   通道保留·274 负测期望「隐式转换为 '自动'」既有诊断锚）；
    //   「整数/小数」=Kw_Int/Kw_Double 默认类型别名（001 §3.1·canonical 归一
    //   整32/浮64·isNumeric 兜底之外的字面别名形态显式豁免）；
    //   「结果/可选」=Kw_Result/Kw_Optional 合成模板头裸名形态（实参形态由
    //   模板分支递归）。
    static const char* const kTypeKeywordTexts[] = {
        "自动", "整数", "小数", "结果", "可选"};
    for (const char* keyword : kTypeKeywordTexts) {
        if (core == keyword) return true;
    }
    if (types::isNumeric(core)) return true;   // 别名族（canonical 口径）兜底放行
    // 函数指针类型文本：返回/各实参递归检查（v2 T69 同构；形态本身合法放行）
    if (isFuncPtrType(core)) {
        const cn::Optional<std::string> ret = types::funcPtrReturnTypeOf(core);
        if (ret.has_value() &&
            !checkTypeReferenceInner(ret.value(), loc, diagnose, depth + 1)) {
            return false;
        }
        const cn::Optional<std::vector<std::string>> params = types::funcPtrParamsOf(core);
        if (params.has_value()) {
            for (const std::string& param : params.value()) {
                const bool paramDeclared = checkTypeReferenceInner(param, loc, diagnose, depth + 1);
                if (paramDeclared == false) {
                    return paramDeclared;   // 上游已诊断（内层 reject 已报诊断·此处仅传播）
                }
            }
        }
        return true;
    }
    // 绑定方法值类型文本：类名与签名递归检查（v2 031 同构；形态本身合法放行）
    if (types::isMethodValueType(core)) {
        const std::optional<std::string> cls = types::methodValueClassOf(core);
        if (cls.has_value() &&
            !checkTypeReferenceInner(cls.value(), loc, diagnose, depth + 1)) {
            return false;
        }
        const std::optional<std::string> fnp = types::methodValueFnPtrOf(core);
        if (fnp.has_value() &&
            !checkTypeReferenceInner(fnp.value(), loc, diagnose, depth + 1)) {
            return false;
        }
        return true;
    }
    // 枚举名作类型放行（值域=整32·v2 418-a 同构）
    if (isEnumType(core)) return true;
    // 数组类型：剥维度递归元素检查（v2 94-a 同构；长度已在语法层限定纯数字）
    if (types::isArray(core)) {
        return checkTypeReferenceInner(types::arrayElemOf(core), loc, diagnose,
                                       depth + 1);
    }
    // 泛型模板源形态（名<实参,...>）：头名合法 + 实参递归存在性——
    //   向量<幽灵>/盒子<幽灵> 实参面与 幽灵<整32> 头名面（v2 语法层已归一
    //   $ 形态故检查类型引用不可见；宿主挂点在 resolveGenericTypeName 之前
    //   持源形态·实参检查此处承载）
    if (core.find('<') != std::string::npos && core.back() == '>') {
        const std::size_t lt = core.find('<');
        const std::string head = core.substr(0, lt);
        static const char* const kContainerHeads[] = {
            "向量", "映射", "集合", "链表", "栈", "队列", "互斥锁"};
        // 内置合成模板头（非用户声明表登记·只查登记表会误判未声明——预验
        //   1586 打红实证）：结果/可选（Kw_Result/Kw_Optional）/手动释放
        //   （#1674 内置包装模板头·001 §5.6③·1586 零泄漏正例形态；实参位
        //   完整形态判定=types::manuallyDroppedPayloadOf 手动释放$/<T> 双口径）
        bool headKnown = (head == "结果") || (head == "可选") ||
                         (head == "手动释放") ||
                         isResultType(head) || isOptionalType(head) ||
                         types::manuallyDroppedPayloadOf(core).has_value();
        for (const char* container : kContainerHeads) {
            if (head == container) headKnown = true;
        }
        headKnown = headKnown || isStructType(head) || isClassType(head) ||
                    isEnumType(head) || isInterfaceType(head) ||
                    findGeneric(head).has_value();
        if (!headKnown &&
            !checkTypeReferenceInner(head, loc, diagnose, depth + 1)) {
            return false;   // 头名未声明（幽灵<整32> 形态·诊断已在内层报防双报）
        }
        // 实参逐段递归（findTopLevelComma 平衡尖括号·resolveTypeName 同款解析）
        const std::string inner = core.substr(lt + 1, core.size() - lt - 2);
        std::size_t pos = 0;
        while (pos <= inner.size()) {
            const std::size_t comma = findTopLevelComma(inner, pos);
            const std::string part = (comma == std::string::npos)
                                         ? inner.substr(pos)
                                         : inner.substr(pos, comma - pos);
            const std::size_t b = part.find_first_not_of(" \t");
            const std::size_t e = part.find_last_not_of(" \t");
            const std::string trimmed = (b != std::string::npos && e != std::string::npos)
                                            ? part.substr(b, e - b + 1) : part;
            const bool argSegmentDeclared = checkTypeReferenceInner(trimmed, loc, diagnose, depth + 1);
            if (argSegmentDeclared == false) {
                return argSegmentDeclared;   // 上游已诊断（内层 reject 已报诊断·不重复报）
            }
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        return true;
    }
    // 限定名（甲::记录）/纯名：查表（谓词均支持限定键·A-2 口径）
    if (isStructType(core) || isClassType(core) ||
        isEnumType(core) || isInterfaceType(core)) {
        return true;
    }
    return reject(core);
}

void SemanticAnalyzer::checkTypeReference(const std::string& type,
                                          const SourceLocation& loc) {
    checkTypeReferenceInner(type, loc, true, 0);
}

bool SemanticAnalyzer::isTypeReferenceDeclared(const std::string& type) const {
    static const SourceLocation kNoLoc;
    return checkTypeReferenceInner(type, kNoLoc, false, 0);
}

} // namespace cn_compiler
