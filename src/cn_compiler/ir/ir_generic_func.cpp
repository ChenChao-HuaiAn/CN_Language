// 泛型函数实例化 IR 生成（Task 6.1 打通泛型函数调用链路）
// 背景：26_generics 遗留限制「泛型函数调用单态化注册未接入」——语义层此前
//   只对泛型类构造（名<实参>(...)）触发单态化，泛型函数调用（最小<整32>(3,7)）
//   落入"无法调用非函数类型"错误。本子任务打通：
//   1. 语义层 visitCallExpr 识别 名<类型>(实参) -> instantiateGeneric 注册
//      名$实参 函数符号 + 登记 GenericFuncInstance（原泛型声明 + 类型实参）
//   2. IR 层 visitProgram 遍历 genericFuncInstances()，为每个实例化函数
//      生成 IRFunction（类型参数 T -> 实参类型 替换）
// 实现：
//   - 泛型函数实例化函数体从 GenericDecl.innerFunc 生成，参数/返回/局部
//     变量类型经 substGenericType 替换类型参数（T -> 整32 等）
//   - 符号名 = 名$实参（与语义层 instantiateGeneric mangling 一致），
//     调用方 IR 层按 resolvedSignature（名$实参）生成 Call 符号匹配
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 替换源码类型中的类型参数（T/T*/结果<T,整32> 等；非参数原样返回）
// 递归处理：裸类型参数（T）、指针（T*）、模板类型（结果<T,E>）
std::string IRGenerator::substGenericType(const std::string& type) const {
    if (genericTypeParams_.empty()) return type;
    // 裸类型参数
    auto it = genericTypeParams_.find(type);
    if (it != genericTypeParams_.end()) return it->second;
    // 引用：尾字符 '&'（T& -> 实参&，泛型引用参数 A-1；与语义 substTypeParam 同步）
    if (!type.empty() && type.back() == '&') {
        const std::string elem = substGenericType(type.substr(0, type.size() - 1));
        return elem + "&";
    }
    // 指针：尾字符 '*'（T* -> 整32*）
    if (!type.empty() && type.back() == '*') {
        const std::string elem = substGenericType(type.substr(0, type.size() - 1));
        return elem + "*";
    }
    // 模板类型：结果<...>/可选<...> 内部替换
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
        const std::string inner = type.substr(lt + 1, gt - lt - 1);
        std::string newInner;
        std::size_t pos = 0;
        while (pos <= inner.size()) {
            const std::size_t comma = inner.find(',', pos);
            const std::string part = (comma == std::string::npos)
                ? inner.substr(pos) : inner.substr(pos, comma - pos);
            std::size_t b = part.find_first_not_of(" \t");
            std::size_t e = part.find_last_not_of(" \t");
            const std::string trimmed = (b != std::string::npos && e != std::string::npos)
                ? part.substr(b, e - b + 1) : part;
            if (!newInner.empty()) newInner += ",";
            newInner += substGenericType(trimmed);
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        // Debug 子任务修复（泛型 + 函数指针回调）：保留 <...> 之后的后缀
        //   （函数指针参数列表 (T,T)）并递归替换内嵌类型参数——原实现丢弃
        //   后缀导致函数指针类型不完整（与语义层 substTypeParam 同步修复）。
        std::string suffix = (gt + 1 < type.size()) ? type.substr(gt + 1) : "";
        if (!suffix.empty() && suffix.front() == '(' && suffix.back() == ')') {
            const std::string plist = suffix.substr(1, suffix.size() - 2);
            std::string newPlist;
            std::size_t pos2 = 0;
            while (pos2 <= plist.size()) {
                const std::size_t comma = plist.find(',', pos2);
                std::string part = (comma == std::string::npos)
                    ? plist.substr(pos2) : plist.substr(pos2, comma - pos2);
                std::size_t b2 = part.find_first_not_of(" \t");
                std::size_t e2 = part.find_last_not_of(" \t");
                if (b2 != std::string::npos && e2 != std::string::npos) {
                    part = part.substr(b2, e2 - b2 + 1);
                }
                if (!newPlist.empty()) newPlist += ",";
                newPlist += substGenericType(part);
                if (comma == std::string::npos) break;
                pos2 = comma + 1;
            }
            suffix = "(" + newPlist + ")";
        } else if (!suffix.empty()) {
            suffix = substGenericType(suffix);
        }
        return type.substr(0, lt) + "<" + newInner + ">" + suffix;
    }
    return type;
}

// 提升泛型函数实例化函数体（名$实参）：从 泛型声明 innerFunc 生成 IRFunction
// 实现要点：
//   1. 类型参数映射：T -> 实参类型（genericTypeParams_），函数体生成期间生效
//   2. 符号名 = 实例化名（名$实参），与语义层注册的 名$实参 函数符号一致
//   3. 参数/返回类型经 substGenericType 替换；局部变量/解引用同理（mapType
//      调用点见 ir.cpp genVarDecl/visitUnaryExpr——mapType 前先替换）
void IRGenerator::emitGenericFuncInstance(const GenericFuncInstance& gfi) {
    const FunctionDecl* node = gfi.gen->innerFunc.get();
    if (node == nullptr || node->body == nullptr) return;
    const GenericDecl* gen = gfi.gen;

    // 1. 构建类型参数映射（T -> 实参；与语义层 substTypeParam 对应）
    std::unordered_map<std::string, std::string> savedParams = genericTypeParams_;
    genericTypeParams_.clear();
    for (std::size_t i = 0; i < gen->typeParams.size() && i < gfi.args.size(); ++i) {
        genericTypeParams_[gen->typeParams[i]] = gfi.args[i];
    }

    // 2. 构建 IRFunction（符号名 = 实例化名 名$实参）
    ir::IRFunction func;
    func.name = gfi.instanceName;
    func.mangledName = gfi.instanceName;
    const std::string retSrc = substGenericType(
        node->returnType.empty() ? "空类型" : node->returnType);
    func.returnType = mapType(retSrc);
    func.returnTypeSrc = retSrc;
    // 结构体返回值标记（隐藏返回指针）
    // 修复（2026-08）：结果/可选 返回同样走隐藏返回指针协议（与 ir_decl/ir_oop
    //   一致）——调用方 emitCall 按被调 structReturn 标志传返回缓冲
    if (semantic_ != nullptr && !retSrc.empty() &&
        semantic_->isStructType(types::canonical(retSrc))) {
        func.structReturn = true;
        func.structReturnSize = semantic_->typeSizeOf(retSrc);
    }
    function_ = &func;

    // 3. 参数装载（类型经替换）
    varStack_.emplace_back();
    for (std::size_t pi = 0; pi < node->params.size(); ++pi) {
        const ParamDecl* param = node->params[pi].get();
        const std::string paramSrc = substGenericType(param->typeName);
        // A-1（引用参数 泛型 T& -> 整32&）：参数槽存被引用左值地址，
        //   体内读写经 byRef 解引用（与普通函数 visitFunctionDecl 一致）
        const bool isRefParam = !param->funcPtr.isFunctionPtr() &&
                                types::isReference(paramSrc);
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(paramSrc);
        if (isRefParam) paramIrType = "ptr";
        registerVarSlots(unique, isRefParam ? ""
                            : (param->funcPtr.isFunctionPtr() ? "" : paramSrc));
        if (!isRefParam && semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
            semantic_->isStructType(types::canonical(paramSrc))) {
            func.structParamIndexes.insert(static_cast<int>(pi));
        }
        func.params.emplace_back(param->name, paramIrType);
        func.paramUniques.push_back(unique);
        ir::IRValue reg = newReg();
        reg.type = paramIrType;
        VarEntry entryInfo;
        entryInfo.regId = reg.id;
        entryInfo.uniqueName = unique;
        entryInfo.type = isRefParam ? mapType(types::stripRef(paramSrc)) : reg.type;
        entryInfo.srcType = paramSrc;
        entryInfo.byRef = isRefParam;
        varStack_.back()[param->name] = entryInfo;
    }

    // 4. 函数体生成（入口块 + 语句）
    // 栈帧膨胀根治（2026-09-08 v2self 锚定轮）：寄存器号每函数复位，与
    //   visitFunctionDecl 同点同构（详见 ir_decl.cpp 注记）
    regCounter_ = 0;
    blockCounter_ = 0;
    newBlock("bb0");
    if (node->body != nullptr) {
        genBlock(node->body.get());
    }
    if (!function_->blocks.empty()) {
        ir::IRBlock* last = function_->blocks.back().get();
        if (!last->terminated) {
            setCurrentBlock(last);
            endReturn("");
        }
    }
    // 类类型局部变量析构（RAII，与普通函数一致）
    genClassDestructorCalls();
    genStringFrees();  // plans/019 阶段4'：拥有型字符串 RAII
    // 72-a 收尾（2026-09-11）：函数级状态复位（与 visitFunctionDecl 出口对称）。
    //   泛型实例由 visitProgram 第 4 步模块级批量生成（所有普通函数之后），
    //   不嵌套于其他函数体生成中——clear 不会破坏宿主函数活状态；补齐可防
    //   实例间名单/基线栈/污染集的串扰残留（行为原由 genBlock 出口截断自愈）。
    stringTainted_.clear();
    ownedStringOrder_.clear();
    ownedClassOrder_.clear();
    scopeStringBase_.clear();
    scopeClassBase_.clear();
    module_->functions.push_back(std::move(func));
    function_ = nullptr;
    if (!varStack_.empty()) varStack_.pop_back();

    // 5. 恢复类型参数映射
    genericTypeParams_ = savedParams;
}

} // namespace cn_compiler
