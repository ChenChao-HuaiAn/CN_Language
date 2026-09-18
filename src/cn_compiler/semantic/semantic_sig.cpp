// CN 语义分析器实现（D1 行数整改 117-a：自 semantic.cpp 按族拆出）
//   族 = 函数签名与重载/隐式转换（funcReturnTypeOf → genericClassInstanceName）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp；
//   共享 helper 已由 115-a 头化在 semantic_internal.hpp）。
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {

std::string SemanticAnalyzer::funcReturnTypeOf(const std::string& funcName) const {
    // 第 4 层（P2-6）：支持 模块名$签名key（resolvedSignature 带 crate 前缀）
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) {
        // 泛型类方法（自举重建 ABI 修复 2026-08-25）：调用符号形如
        //   向量$Token$元素#整64（实例化类名$方法名#参数签名）。泛型类方法实例化
        //   注册在 classes_（非 functions_），此处回退解析：
        //   取最后一个'$'之后为方法名（元素#整64 -> 元素），剩余（向量$Token）为
        //   实例化类名，查 classes_ 方法表得返回类型（Token）——使调用方正确走
        //   结构体返回缓冲（隐藏返回指针），否则 16 字节结构体按寄存器返回字段错乱。
        const std::size_t lastDollar = funcName.rfind('$');
        if (lastDollar != std::string::npos) {
            const std::string classPart = funcName.substr(0, lastDollar);
            std::string methodPart = funcName.substr(lastDollar + 1);
            const std::size_t hash = methodPart.find('#');
            if (hash != std::string::npos) methodPart = methodPart.substr(0, hash);
            const ClassInfo* ci = findClass(classPart);
            if (ci != nullptr) {
                const auto mit = ci->methods.find(methodPart);
                if (mit != ci->methods.end()) return mit->second.type;
                for (const auto& mk : ci->methods) {
                    if (mk.first == methodPart || mk.first.rfind(methodPart, 0) == 0) {
                        return mk.second.type;
                    }
                }
            }
        }
        return "";
    }
    return it->second.returnType;
}
bool SemanticAnalyzer::funcReturnsRef(const std::string& funcName) const {
    // P3-18 补完：函数返回类型是否为引用（T&）——调用点把结果当"被引用左值的地址"
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) return false;
    return it->second.isRefReturn;
}
std::vector<std::string> SemanticAnalyzer::funcParamTypesOf(const std::string& funcName) const {
    // 第 4 层（P2-6）：支持 模块名$签名key（resolvedSignature 带 crate 前缀）
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) return {};
    return it->second.paramTypes;
}
std::string SemanticAnalyzer::signatureKey(const std::string& name,
                                           const std::vector<std::string>& paramTypes) {
    std::string key = name;
    if (!paramTypes.empty()) {
        key += "#";
        for (std::size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) key += ",";
            key += paramTypes[i];
        }
    }
    return key;
}
// plans/018 呈报二 A′：函数链接键公式实现（唯一归属，注释见 semantic.hpp 声明处）。
//   例外三形态与 codegen symbolName 映射对齐：
//   ① moduleName 空（单文件编译，mergeModules singleModule 不写 crate 名）；
//   ② moduleName=="主"（入口 crate 根文件——主->cn_main 映射基于纯名）；
//   ③ funcName=="主"（入口函数本身，codegen 映射 cn_main）；
//   ④ moduleName 以 __cn_ 开头（内置运行时符号直通）。
std::string SemanticAnalyzer::functionLinkKey(const std::string& moduleName,
                                              const std::string& funcName,
                                              const std::string& sigKey) {
    if (moduleName.empty() || moduleName == "主" || funcName == "主" ||
        moduleName.rfind("__cn_", 0) == 0) {
        return sigKey;
    }
    return moduleName + "$" + sigKey;
}
bool SemanticAnalyzer::hasFunctionName(const std::string& name) const {
    if (functions_.find(name) != functions_.end()) return true;  // 内置纯名 key
    for (const auto& kv : functions_) {
        // 用户函数 key 形如 名#参数串；纯名内置 key 无 '#'
        const std::size_t hashPos = kv.first.find('#');
        const std::size_t dollarPos = kv.first.find('$');
        // 第 8 层（52_library 实测缺陷）：无参跨模块条目（模块名$函数名，
        //   如 格式化$版本）剥离 '$' 前缀后 base 才匹配纯名；含 '#' 的
        //   跨模块条目（模块名$名#参数）同样剥离。泛型实例名（排序$整32）
        //   不剥离（base 保持 排序$整32 匹配泛型调用）——name 含 '$' 时跳过。
        const bool crossModuleNoHash =
            (hashPos == std::string::npos && dollarPos != std::string::npos &&
             name.find('$') == std::string::npos);
        const bool crossModuleWithHash =
            (hashPos != std::string::npos && dollarPos != std::string::npos &&
             dollarPos < hashPos);
        if (crossModuleNoHash || crossModuleWithHash) {
            const std::size_t hashP2 = kv.first.find('#');
            const std::string base2 = (hashP2 == std::string::npos)
                                          ? kv.first.substr(dollarPos + 1)
                                          : kv.first.substr(dollarPos + 1, hashP2 - dollarPos - 1);
            if (base2 == name) return true;
        } else {
            const std::string base = (hashPos == std::string::npos)
                                         ? kv.first
                                         : kv.first.substr(0, hashPos);
            if (base == name) return true;
        }
    }
    return false;
}
std::string SemanticAnalyzer::funcFirstSigKey(const std::string& name) const {
    std::string best;
    for (const auto& kv : functions_) {
        const std::size_t hashPos = kv.first.find('#');
        const std::string base = (hashPos == std::string::npos) ? kv.first
                                                                : kv.first.substr(0, hashPos);
        if (base != name) continue;
        if (best.empty() || kv.first < best) best = kv.first;
    }
    return best;
}
int SemanticAnalyzer::conversionLevel(const std::string& argTypeRaw,
                                      const std::string& paramTypeRaw,
                                      bool argIsLiteral) {
    // A-1（引用参数）：按值/按引用双方均剥 & 后比较——引用参数绑定左值实参，
    //   实参与参数的数据形状一致（都是被引用类型的值）；引用 vs 按值 的区分
    //   由签名 key（& 保留在 paramTypes）完成，决议只看形状
    const std::string arg = types::stripRef(canonicalType(argTypeRaw));
    const std::string param = types::stripRef(canonicalType(paramTypeRaw));
    if (arg == param) return 0;
    if (!canConvertType(arg, param)) {
        // 55-c 方案A（2026-09-10 用户裁决）：整数字面量实参豁免——源/目标均为
        //   整数族时按宽化级参与决议（字面量按目标类型解释，Rust 字面量推断
        //   惯例；`读值(100)` 传 正32 形参等形态保留）
        if (argIsLiteral && types::isInteger(arg) && types::isInteger(param)) {
            return 1;
        }
        return -1;
    }
    // 枚举 -> 整数：按宽化处理（枚举本质为整32，值域不损失）
    if (isEnumType(arg) && !isEnumType(param) && types::isInteger(param)) return 1;
    // 整数族内部：同符号宽化（整8->整16->整32->整64、正8->正16->...）
    if (types::isInteger(arg) && types::isInteger(param)) {
        const bool argUnsigned = types::isUnsigned(arg);
        const bool paramUnsigned = types::isUnsigned(param);
        if (argUnsigned == paramUnsigned) {
            if (types::intRank(arg) <= types::intRank(param)) return 1;  // 同符号向宽
        }
        // 符号混合：一律按隐式转换（保守，避免有符号->无符号宽化的值域陷阱）
        return 2;
    }
    // 浮点族内部：浮32 -> 浮64 宽化
    if (types::isFloat(arg) && types::isFloat(param)) {
        if (arg == "浮32" && param == "浮64") return 1;
        return 2;  // 浮64 -> 浮32 窄化：隐式转换（有损）
    }
    // 整数 -> 浮点 / 字符 -> 整数 等：隐式转换
    return 2;
}
std::string SemanticAnalyzer::resolveOverload(const std::string& name,
                                              const std::vector<std::string>& argTypes,
                                              const SourceLocation& loc,
                                              const std::string& moduleFilter,
                                              const std::vector<bool>& argIsLiteral) {
    std::string bestKey;
    int bestTotal = INT32_MAX;
    bool ambiguous = false;
    std::string ambiguousDetail;
    // 206-a（2026-09-15 第两百零六轮，D1 行数整改）：三段提取——A-5 前置扫描迁
    //   hasCurrentModuleDefinition；主循环内 key 剥离+模块过滤迁 overloadEntryMatch
    //   （continue → return false 转换）；主函数保留状态变量+循环骨架+参数个数/
    //   转换等级+最优更新+歧义/未命中诊断（宿主纯重构零行为变更）。
    const bool currentHasName = moduleFilter.empty() && !currentModuleName_.empty() &&
                                hasCurrentModuleDefinition(name);
    for (const auto& kv : functions_) {
        const FunctionInfo& info = kv.second;
        if (!overloadEntryMatch(kv.first, name, info, moduleFilter, currentHasName)) {
            continue;
        }
        // 参数个数匹配：实参个数 + 可补全的默认参数数 >= 参数总数
        const int required = static_cast<int>(info.paramTypes.size()) - info.defaultCount;
        const int given = static_cast<int>(argTypes.size());
        if (given < required || given > static_cast<int>(info.paramTypes.size())) {
            continue;  // 参数个数不匹配
        }
        // 逐参数转换等级（只检查实际提供的参数；缺省部分由默认值补全）
        int total = 0;
        bool ok = true;
        for (int i = 0; i < given; ++i) {
            const bool lit = argIsLiteral.size() > static_cast<std::size_t>(i) &&
                             argIsLiteral[static_cast<std::size_t>(i)];
            const int level = conversionLevel(argTypes[i], info.paramTypes[i], lit);
            if (level < 0) { ok = false; break; }
            total += level;
        }
        if (!ok) continue;
        if (bestKey.empty() || total < bestTotal) {
            bestKey = kv.first;
            bestTotal = total;
            ambiguous = false;
        } else if (total == bestTotal && kv.first != bestKey) {
            ambiguous = true;  // 两个签名同样优
            ambiguousDetail = kv.first + " 与 " + bestKey;
        }
    }
    if (ambiguous && !bestKey.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "函数 '" + name + "' 调用存在歧义（" + ambiguousDetail +
                            " 均可匹配），请使用显式类型转换消除歧义");
        return "";
    }
    if (bestKey.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "未找到匹配的函数 '" + name + "'（参数个数或类型不匹配）");
        return "";
    }
    return bestKey;
}


// 206-a：A-5 前置扫描（crate 隔离纯名调用）——当前模块是否定义过该函数名；
//   定义过则其他模块同名条目不参与纯名决议（作用域遮蔽导入语义：跨模块同名
//   函数纯名调用不再一律报歧义，当前模块版本优先）；调用方守卫 moduleFilter 空
//   与 currentModuleName_ 非空（206-a：条件留主函数，族① 语义纯粹）。
bool SemanticAnalyzer::hasCurrentModuleDefinition(const std::string& name) {
    bool currentHasName = false;
    if (!currentModuleName_.empty()) {
        for (const auto& kv : functions_) {
            std::string k2 = kv.first;
            const std::size_t h2 = k2.find('#');
            const std::size_t d2 = k2.find('$');
            std::string km2;
            if (d2 != std::string::npos && h2 != std::string::npos && d2 < h2) {
                km2 = k2.substr(0, d2);
                k2 = k2.substr(d2 + 1);
            } else if (h2 == std::string::npos && d2 != std::string::npos &&
                       name.find('$') == std::string::npos) {
                km2 = k2.substr(0, d2);
                k2 = k2.substr(d2 + 1);
            }
            const std::size_t hp = k2.find('#');
            const std::string b2 = (hp == std::string::npos) ? k2 : k2.substr(0, hp);
            if (b2 != name) continue;
            const std::string em2 = km2.empty() ? kv.second.moduleName : km2;
            if (em2 == currentModuleName_) {
                currentHasName = true;
                break;
            }
        }
    }
    return currentHasName;
}

// 206-a：单条签名候选的 key 形态剥离 + crate 隔离模块过滤——
//   普通/跨模块/无参/泛型形态剥离（第 4/8 层）；精确/父前缀/末段三级模块匹配
//   （52_library 实测缺陷+挂账1 包前缀化）；A-5 纯名遮蔽。返回 false=跳过该条目
//   （原 continue 语义），true=参与决议。
bool SemanticAnalyzer::overloadEntryMatch(const std::string& rawKey,
                                          const std::string& name,
                                          const FunctionInfo& info,
                                          const std::string& moduleFilter,
                                          bool currentHasName) {
        // 第 4 层：key 形态兼容——普通签名（名#参数）与跨模块条目
        //   （模块名$名#参数）。模块条目 key 含 '$' 前缀（模块名$），
        //   base 提取须剥离 模块名$ 前缀。
        // 注意：泛型实例名（排序$整32）与重载签名（名#参数）不含模块前缀——
        //   仅当 '#' 存在且 '$' 位于 '#' 之前（模块名$名#参数）才剥离；
        //   排序$整32 无 '#' -> 不剥离（base 保持 排序$整32 匹配泛型调用）。
        std::string key = rawKey;
        std::string keyModule;  // key 携带的模块名（跨模块条目）
        const std::size_t hashFirst = key.find('#');
        const std::size_t dollarPos = key.find('$');
        if (dollarPos != std::string::npos && hashFirst != std::string::npos &&
            dollarPos < hashFirst) {
            keyModule = key.substr(0, dollarPos);
            key = key.substr(dollarPos + 1);
        } else if (hashFirst == std::string::npos && dollarPos != std::string::npos &&
                   name.find('$') == std::string::npos) {
            // 第 8 层（52_library 实测缺陷）：无参函数（sigKey 无 '#'）跨模块
            //   注册 key = 模块名$函数名（如 格式化$版本）。调用 name 不含 '$'
            //   （普通函数调用，区别于泛型实例名 排序$整32）——剥离 '$' 前缀。
            keyModule = key.substr(0, dollarPos);
            key = key.substr(dollarPos + 1);
        }
        const std::size_t hashPos = key.find('#');
        const std::string base = (hashPos == std::string::npos) ? key
                                                                : key.substr(0, hashPos);
        if (base != name) return false;  // 仅同名的签名参与决议
        // 第 4 层（crate 隔离）：限定调用按模块过滤——跨模块同名函数各自独立，
        //   仅匹配调用模块的签名（数学::双倍 只解析 数学.cn 的双倍）。
        // A-5（2026-08）：纯名调用（moduleFilter 空）当前模块条目优先——
        //   crate 隔离同名函数纯名调用不再一律歧义（作用域遮蔽导入语义，
        //   与 52_library 的 主::版本() 自限定等价）。
        const std::string entryModule = keyModule.empty() ? info.moduleName : keyModule;
        if (!moduleFilter.empty()) {
            // 模块过滤：普通条目按 info.moduleName，跨模块条目按 key 前缀模块
            //   呈报一B 补充（2026-09-07）：moduleName 空 = 单文件管线
            //   （runPipeline 未合并 / mergeModules singleModule）——全部声明
            //   同属唯一模块，限定调用过滤器（自导入 主::版本 等）恒命中本
            //   模块；多文件管线声明恒带模块名，不受影响。
            if (entryModule == moduleFilter || entryModule.empty()) {
                // 精确匹配：同包限定调用（网络::传输控制::发送 -> 网络::传输控制）
            } else {
                // A-5（父模块名限定调用子模块函数）：entryModule（网络::传输控制）
                //   以 moduleFilter + "::"（网络::）为前缀即视为同一模块——
                //   子模块属于父模块命名空间（网络::连接() 解析到 网络::传输控制::连接）
                const std::string filterPrefix = moduleFilter + "::";
                const bool isSubModule =
                    entryModule.compare(0, filterPrefix.size(), filterPrefix) == 0;
                if (!isSubModule) {
                    // 第 8 层（52_library 实测缺陷）：跨 crate 限定调用
                    //   （工具库::格式化::版本）——外部依赖模块注册 moduleName =
                    //   文件主干（格式化），而调用路径 subModule = 工具库::格式化。
                    //   最后段匹配：moduleFilter 末段（:: 之后）== entryModule 即视为
                    //   同一模块（跨 crate 限定调用解析到依赖包内同名模块）。
                    //   挂账1 包前缀化扩展（2026-09-08）：目录包/父挂子成员模块名
                    //   带包前缀（语义::内置）——条目名**末段**与 filterLast 对齐即
                    //   命中（v1 自举组件 语义::语义::内置::行类型 实测：条目
                    //   语义::内置 末段 内置 == filterLast 内置）。与跨 crate 末段
                    //   按名匹配同族：精确/前缀匹配优先，末段为松匹配兜底。
                    const std::size_t lastColon = moduleFilter.rfind("::");
                    const std::string filterLast = (lastColon == std::string::npos)
                                                        ? moduleFilter
                                                        : moduleFilter.substr(lastColon + 2);
                    const std::size_t entryLastColon = entryModule.rfind("::");
                    const std::string entryLast =
                        (entryLastColon == std::string::npos)
                            ? entryModule
                            : entryModule.substr(entryLastColon + 2);
                    if (entryModule != filterLast && entryLast != filterLast) return false;
                }
            }
        } else if (currentHasName && !entryModule.empty() &&
                   entryModule != currentModuleName_) {
            // A-5（crate 隔离纯名调用）：当前模块有同名函数时，其他模块条目
            //   不参与决议（作用域遮蔽导入；52_library 的 主::版本() 自限定
            //   语义等价，纯名 版本() 现在直接命中当前模块版本）
            return false;
        }
    return true;
}

bool SemanticAnalyzer::canConvertType(const std::string& fromRaw,
                                      const std::string& toRaw) const {
    const std::string from = canonicalType(fromRaw);
    const std::string to = canonicalType(toRaw);
    // 320-a（T42·方案甲·Rust () 同款）：空类型不与任何类型互转——空类型
    //   调用结果参与赋值/传参/返回一律拒绝（「无返回值却有值」违反类型
    //   安全）；算术/比较面由 visitBinaryExpr 操作数检查覆盖。
    if ((from == "空类型") != (to == "空类型")) return false;
    // 模板类型兼容（Task 3.5）：结果<T,E> 与 结果<T,E> 须完全一致；
    //   结果<A,B> 与 结果<C,D>（A可转C 且 B可转D）允许（错误码类型宽化）
    const bool fromResult = isResultType(from);
    const bool toResult = isResultType(to);
    if (fromResult && toResult) {
        const std::vector<std::string> fa = resultTypeArgs(from);
        const std::vector<std::string> ta = resultTypeArgs(to);
        if (fa.size() == 2 && ta.size() == 2) {
            return canConvertType(fa[0], ta[0]) && canConvertType(fa[1], ta[1]);
        }
        return false;
    }
    const bool fromOpt = isOptionalType(from);
    const bool toOpt = isOptionalType(to);
    if (fromOpt && toOpt) {
        const std::string fa = optionalTypeArg(from);
        const std::string ta = optionalTypeArg(to);
        return !fa.empty() && !ta.empty() && canConvertType(fa, ta);
    }
    // 空指针常量 -> 可选<T>：视为空可选值（无）（Task 3.5，规格书07-三）
    if (from == "空类型*" && toOpt) return true;
    if (fromResult || toResult || fromOpt || toOpt) return false;  // 模板与非模板不可转
    const bool fromEnum = isEnumType(from);
    const bool toEnum = isEnumType(to);
    // 枚举 ↔ 整数：双向允许（枚举值可赋给整型变量；整数可赋给枚举变量）
    if (fromEnum && !toEnum) return types::isInteger(to);
    if (!fromEnum && toEnum) return types::isInteger(from);
    // 枚举 → 枚举：须同一枚举类型
    if (fromEnum && toEnum) return from == to;
    // 结构体 → 结构体：须同一类型（值类型拷贝）
    const bool fromStruct = isStructType(from);
    const bool toStruct = isStructType(to);
    if (fromStruct || toStruct) return from == to;
    // 自举前置 A-3b（plans/004）：自定义泛型类实例 类型名统一——源码模板形式
    //   （向量<字符串>）与实例化符号名（向量$字符串，instantiateGeneric 生成）
    //   视为同一类型。此前 返回 表（表=向量$字符串）与返回类型 向量<字符串>
    //   比较失败 -> "无法将 '向量$字符串' 隐式转换为 '向量<字符串>'"
    //   （返回容器/自定义泛型不可用，自举源码大面积受阻）。
    const std::string fromInst = genericClassInstanceName(from);
    const std::string toInst = genericClassInstanceName(to);
    if (fromInst != from || toInst != to) {
        return fromInst == toInst;
    }
    // P3-19：类 → 接口 隐式转换（值或指针；接口类型可持有任何实现类对象）
    {
        std::string toIface = to;
        if (types::isPointer(to) && isInterfaceType(types::pointeeOf(to))) {
            toIface = types::pointeeOf(to);
        }
        if (isInterfaceType(toIface)) {
            std::string fromBase = types::isPointer(from) ? types::pointeeOf(from) : from;
            if (isClassType(fromBase) && classImplementsInterface(fromBase, toIface)) {
                return true;
            }
        }
    }
    return types::canConvert(from, to);
}
// 55-c 方案A（2026-09-10 用户裁决，Rust E0308 对齐）：canConvertType 拒绝时的
//   整数字面量豁免——源/目标均为整数族且值表达式为整数字面量形态（含 -1）
//   时放行。赋值初始化（visitVarDecl）/传参（visitCallExpr 逐参）/返回
//   （visitReturnStmt）三面统一走本函数；二元运算面豁免在 visitBinaryExpr
//   内联（isIntLiteralExpr 判定后跳过混合符号检查）。
bool SemanticAnalyzer::canConvertWithLiteral(const Expr* value,
                                             const std::string& fromRaw,
                                             const std::string& toRaw) const {
    if (canConvertType(fromRaw, toRaw)) return true;
    // 字面量豁免仅整数族→整数族（浮点/字符串等其它拒绝面不豁免）
    if (!types::isInteger(fromRaw) || !types::isInteger(toRaw)) return false;
    // 319-a（T36·方案甲）：值域门槛——豁免=字面量且值在目标类型域内
    //（整16 甲=40000 超域拒绝，域内 整8 a=10 保留豁免；表达式面口径统一）
    return isIntLiteralExpr(value) && intLiteralFitsType(value, toRaw);
}
// 混合符号赋值专用诊断（55-c 方案A，2026-09-10 用户裁决·Rust E0308 对齐）——
//   消息风格与二元面「混合符号二元运算禁止」对仗；主三面（声明初始化/赋值/
//   返回）接入，函数指针/构造/接口/方法等次要面维持通用消息（拒绝语义已生效）
bool SemanticAnalyzer::reportMixedSignAssign(const Expr* value,
                                             const std::string& from,
                                             const std::string& to,
                                             const SourceLocation& loc) {
    if (types::isInteger(from) && types::isInteger(to) &&
        types::isUnsigned(from) != types::isUnsigned(to) &&
        !isIntLiteralExpr(value)) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "混合符号赋值禁止：'" + from + "' 与 '" + to +
                            "' —— 须显式转换（如 整64(表达式)/正64(表达式)）；"
                            "字面量豁免（Rust 对齐，2026-09-10 方案A）");
        return true;
    }
    return false;
}
std::string SemanticAnalyzer::genericClassInstanceName(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    const std::size_t lt = type.find('<');
    if (lt == std::string::npos || type.empty() || type.back() != '>') return type;
    const std::string head = type.substr(0, lt);
    // 仅自定义泛型类（向量/映射/链表/栈/队列 等）参与统一；结果/可选 是
    //   合成模板（另有降级路径），函数指针/其他 名<...> 形态不在此列
    if (findGeneric(head) == nullptr || findGeneric(head)->ast->innerClass == nullptr) {
        return type;
    }
    std::string instance = head;
    std::string inner = type.substr(lt + 1, type.size() - lt - 2);
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        const std::size_t comma = inner.find(',', pos);
        const std::string part = (comma == std::string::npos)
                                     ? inner.substr(pos)
                                     : inner.substr(pos, comma - pos);
        std::size_t b = part.find_first_not_of(" \t");
        std::size_t e = part.find_last_not_of(" \t");
        instance += "$" + (b == std::string::npos ? "" : part.substr(b, e - b + 1));
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return instance;
}
} // namespace cn_compiler
