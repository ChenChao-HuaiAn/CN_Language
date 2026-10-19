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
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"

namespace cn_compiler {

namespace {
// 1143：空桶共享实例（函数名分桶索引 miss 返回——调用方循环自然零次）
const std::vector<const FuncEntryView*>& emptyFuncNameBucket() {
    static const std::vector<const FuncEntryView*> kEmpty;
    return kEmpty;
}
}  // namespace

// 648：miss 返回空串是接口信号（消费点遍布全树），禁止在本函数改报错；
//   消费点须按「miss 不可静默」立法处置（先例=ir_call.cpp 用户函数返回类型
//   两级查询收口·内置/泛型回退命中不涉）。
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
// 662（方法链消费位）：函数返回类对象拥有形态查询——键域=funcReturnTypeOf 同款
//   （含 模块$前缀 剥离回退）；miss（含泛型类方法面）=保守 false（不析构）。
bool SemanticAnalyzer::funcReturnOwnsClassOf(const std::string& funcName) const {
    auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        const std::size_t dollar = funcName.find('$');
        if (dollar != std::string::npos) it = functions_.find(funcName.substr(dollar + 1));
    }
    if (it == functions_.end()) return false;
    return it->second.returnsOwnedClass662;
}
// 648：miss 返回 false 是接口信号（非引用返回的合法缺省+查无此函数双义），
//   禁止在本函数改报错；消费点须按「miss 不可静默」立法处置
//   （先例=ir_call.cpp 用户函数返回类型收口）。
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
// 648：miss 返回空表是接口信号（已登记无参函数的空形参表与未登记 miss 双义，
//   消费点须以 funcFirstSigKey 等区分——先例=ir_call.cpp 形参表收口豁免③），
//   禁止在本函数改报错；消费点须按「miss 不可静默」立法处置。
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

bool SemanticAnalyzer::hasFunctionName(const std::string& name) const {
    if (functions_.find(name) != functions_.end()) return true;  // 内置纯名 key
    // 1143：分桶索引——同名候选桶（两形态按 name 是否含 '$' 选表；桶内条目
    //   base 恒等于 name，逐条等值判定与原全表逐条剥离匹配等价）
    const bool nameNoDollar = name.find('$') == std::string::npos;
    for (const FuncEntryView* view : funcNameBucket(name)) {
        const std::string_view base =
            nameNoDollar ? view->baseNoDollar : view->baseWithDollar;
        if (base == name) return true;
    }
    return false;
}
std::string SemanticAnalyzer::funcFirstSigKey(const std::string& name) const {
    std::string best;
    // 1143：分桶索引——最小键选取与遍历顺序无关（纯 min 运算），过滤等价
    const bool nameNoDollar = name.find('$') == std::string::npos;
    for (const FuncEntryView* view : funcNameBucket(name)) {
        const std::string_view base =
            nameNoDollar ? view->baseNoDollar : view->baseWithDollar;
        if (base != name) continue;  // 桶已保证同名；保留判定防索引偏差静默
        const std::string& key = view->entry->first;
        // 997 案丁口径（原实现注释保留结论）：与 hasFunctionName 同一套跨模块
        //   base 剥离（52_library 第 8 层键形态=模块$名#参数 / 模块$名）——
        //   泛型实例键（排序$整32·name 含 '$'）不剥离，与 hasFunctionName 判据一致。
        if (best.empty() || key < best) best = key;
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
        // 234（2026-10-06·91303）：类 → 基类链·仅引用形参的决议形状放行——
        //   决议层按 A-1 已剥 & 比较形状，此处放行须原形参为引用（绑定=同地址
        //   零拷贝·与接口 A1 对仗）；值形参不放行=按值拷贝即切片（切片拒绝由
        //   reportSliceReject 在赋值/传参层执行·立法 234）
        if (types::isReference(paramTypeRaw) &&
            isClassType(arg) && isClassType(param) && !isInterfaceType(param) &&
            isDerivedFrom(arg, param)) {
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
    // 1143：分桶索引遍历——只走同名条目（原全表扫描+逐条现场剥离）；桶内顺序
    //   =构建时 functions_ 遍历顺序（同名子序列与原全表扫描一致，决议/诊断等价）
    const std::vector<const FuncEntryView*>& bucket = funcNameBucket(name);
    for (const FuncEntryView* view : bucket) {
        const FunctionInfo& info = view->entry->second;
        const std::string& sigEntryKey = view->entry->first;
        if (!overloadEntryMatch(*view, name, info, moduleFilter, currentHasName)) {
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
            bestKey = sigEntryKey;
            bestTotal = total;
            ambiguous = false;
        } else if (total == bestTotal && sigEntryKey != bestKey) {
            ambiguous = true;  // 两个签名同样优
            ambiguousDetail = sigEntryKey + " 与 " + bestKey;
        }
    }
    if (ambiguous && !bestKey.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "函数 '" + name + "' 调用存在歧义（" + ambiguousDetail +
                            " 均可匹配），请使用显式类型转换消除歧义");
        return "";
    }
    if (bestKey.empty()) {
        // 031（#1075）：含方法值实参的调用决议失败——优先报专用文案（指路
        //   绑定变量/闭包捕获）。判据=无任何同名候选在该实参位的形参是方法值
        //   类型（有=方法值位本身不是决议失败主因，走通用报错保持原诊断面）。
        bool anyMvTarget = false;
        for (const FuncEntryView* view : bucket) {
            const FunctionInfo& info = view->entry->second;
            if (!overloadEntryMatch(*view, name, info, moduleFilter, currentHasName)) {
                continue;
            }
            const std::size_t cap = std::min(argTypes.size(), info.paramTypes.size());
            for (std::size_t ai = 0; ai < cap; ++ai) {
                if (types::isMethodValueType(argTypes[ai]) &&
                    types::isMethodValueType(info.paramTypes[ai])) {
                    anyMvTarget = true;
                }
            }
        }
        if (!anyMvTarget) {
            for (std::size_t ai = 0; ai < argTypes.size(); ++ai) {
                if (types::isMethodValueType(argTypes[ai])) {
                    diagnostics_.report(DiagnosticLevel::Error, loc,
                                        method_value_943::fnPtrConvertDiag());
                    return "";
                }
            }
        }
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
// 1143：遍历改分桶索引（只走同名条目）；key 剥离取视图预解析字段（原逐条
//   substr 现场构造）——按 name 含/不含 '$' 两形态选对应字段，规则逐字同原实现。
bool SemanticAnalyzer::hasCurrentModuleDefinition(const std::string& name) {
    // 静默豁免业务值: currentModuleName_ 空=无模块上下文（单文件管线/未入模块体），
    //   「当前模块是否定义该名」语义恒假——业务定义非错误默认（调用方同守卫）
    if (currentModuleName_.empty()) return false;
    const bool nameNoDollar = name.find('$') == std::string::npos;
    for (const FuncEntryView* view : funcNameBucket(name)) {
        const std::string_view base =
            nameNoDollar ? view->baseNoDollar : view->baseWithDollar;
        if (base != name) continue;  // 桶已保证同名；保留判定防索引偏差静默
        const std::string_view mod =
            nameNoDollar ? view->moduleNoDollar : view->moduleWithDollar;
        const std::string_view entryModule =
            mod.empty() ? std::string_view(view->entry->second.moduleName) : mod;
        if (entryModule == currentModuleName_) return true;
    }
    return false;
}

// 206-a：单条签名候选的模块过滤匹配（原 key 形态剥离族；1143 改用索引视图）——
//   剥离规则：形态由调用名 name 是否含 '$' 决定（含 '$'=泛型调用形态，'$' 前缀
//   仅 '#' 前剥；不含 '$'=普通调用形态，无 '#' 的 模块名$名 亦剥——第 8 层
//   52_library 实测）；精确/父前缀/末段三级模块匹配（52_library 实测缺陷+挂账1
//   包前缀化）；A-5 纯名遮蔽。返回 false=跳过该条目（原 continue 语义），true=参与决议。
bool SemanticAnalyzer::overloadEntryMatch(const FuncEntryView& view,
                                          const std::string& name,
                                          const FunctionInfo& info,
                                          const std::string& moduleFilter,
                                          bool currentHasName) {
    // 1143：key 形态剥离已由索引预解析（视图字段）——此处按 name 形态取对应
    //   base/模块前缀，规则与原现场 find/substr 逐字一致。
    const bool nameNoDollar = name.find('$') == std::string::npos;
    const std::string_view base = nameNoDollar ? view.baseNoDollar : view.baseWithDollar;
    if (base != name) return false;  // 仅同名的签名参与决议（桶外条目恒不命中）
    // 第 4 层（crate 隔离）：限定调用按模块过滤——跨模块同名函数各自独立，
    //   仅匹配调用模块的签名（数学::双倍 只解析 数学.cn 的双倍）。
    // A-5（2026-08）：纯名调用（moduleFilter 空）当前模块条目优先——
    //   crate 隔离同名函数纯名调用不再一律歧义（作用域遮蔽导入语义，
    //   与 52_library 的 主::版本() 自限定等价）。
    const std::string_view keyModule =
        nameNoDollar ? view.moduleNoDollar : view.moduleWithDollar;
    const std::string_view entryModule =
        keyModule.empty() ? std::string_view(info.moduleName) : keyModule;
    if (!moduleFilter.empty()) {
        // 模块过滤：普通条目按 info.moduleName，跨模块条目按 key 前缀模块
        //   呈报一B 补充（2026-09-07）：moduleName 空 = 单文件管线
        //   （runPipeline 未合并 / mergeModules singleModule）——全部声明
        //   同属唯一模块，限定调用过滤器（自导入 主::版本 等）恒命中本
        //   模块；多文件管线声明恒带模块名，不受影响。
        // 精确匹配：同包限定调用（网络::传输控制::发送 -> 网络::传输控制）——恒
        //   直接放行；非精确才做 A-5 父模块前缀/末段两级松匹配
        const bool moduleExact = (entryModule == moduleFilter) || entryModule.empty();
        if (!moduleExact) {
            // A-5（父模块名限定调用子模块函数）：entryModule（网络::传输控制）
            //   以 moduleFilter + "::"（网络::）为前缀即视为同一模块——
            //   子模块属于父模块命名空间（网络::连接() 解析到 网络::传输控制::连接）
            //   （1143：前缀判定改无分配等价式——长度下限+前缀段+双冒号，
            //   替代 moduleFilter+"::" 临时串构造与 compare）
            const std::size_t fsz = moduleFilter.size();
            const bool isSubModule = entryModule.size() >= fsz + 2 &&
                                     entryModule.compare(0, fsz, moduleFilter) == 0 &&
                                     entryModule[fsz] == ':' && entryModule[fsz + 1] == ':';
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
                const std::string_view filterLast =
                    (lastColon == std::string::npos)
                        ? std::string_view(moduleFilter)
                        : std::string_view(moduleFilter).substr(lastColon + 2);
                const std::size_t entryLastColon = entryModule.rfind("::");
                const std::string_view entryLast =
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

// 1143：函数名分桶索引惰性构建——size 变化（含首查）时按 functions_ 遍历顺序
//   整表重建（O(条目数)·每 key 一次性剥离+双桶插入）；size 不变期间零重建、
//   查询零临时串。functions_ 无删除路径（全树核验零 erase/clear·写入皆新键
//   插入/同键覆盖）⇒ size 相等即索引与表一致；单次编译单实例（driver 每编译
//   局部构造分析器）⇒ 跨程序复用不进入本缓存。桶内追加顺序=functions_ 遍历
//   顺序 ⇒ 同名子序列处理顺序与原全表扫描一致（决议遍历/歧义诊断等价）。
const FuncNameIndex& SemanticAnalyzer::ensureFuncNameIndex() const {
    if (funcNameIndex_.builtSize == functions_.size()) {
        return funcNameIndex_;
    }
    FuncNameIndex& idx = funcNameIndex_;
    idx.views.clear();
    idx.byBase.clear();
    idx.byBaseDollar.clear();
    for (const auto& kv : functions_) {
        const std::string& key = kv.first;
        const std::size_t hashPos = key.find('#');
        const std::size_t dollarPos = key.find('$');
        idx.views.emplace_back();
        FuncEntryView& view = idx.views.back();
        view.entry = &kv;
        const std::string_view keyView(key);
        // 形态 A（查询名不含 '$'）：默认未剥=剥 '#' 后缀为 base；含 '$' 且（无 '#'
        //   或 '$' 在 '#' 前）⇒ 覆盖为剥 模块$（剥后取到 '#' 前——第 4 层
        //   模块名$名#参数；无 '#' 取到串尾——第 8 层 模块名$名·52_library 实测）
        view.baseNoDollar = (hashPos == std::string::npos)
                                ? keyView
                                : keyView.substr(0, hashPos);
        if (dollarPos != std::string::npos &&
            (hashPos == std::string::npos || dollarPos < hashPos)) {
            view.moduleNoDollar = keyView.substr(0, dollarPos);
            view.baseNoDollar = (hashPos == std::string::npos)
                                    ? keyView.substr(dollarPos + 1)
                                    : keyView.substr(dollarPos + 1, hashPos - dollarPos - 1);
        }
        // 形态 B（查询名含 '$'）：默认未剥；含 '$' 且含 '#' 且 '$' 在 '#' 前 ⇒
        //   覆盖为剥 模块$（泛型实例名 排序$整32 无 '#' 保持整键）
        view.baseWithDollar = (hashPos == std::string::npos)
                                  ? keyView
                                  : keyView.substr(0, hashPos);
        if (dollarPos != std::string::npos && hashPos != std::string::npos &&
            dollarPos < hashPos) {
            view.moduleWithDollar = keyView.substr(0, dollarPos);
            view.baseWithDollar = keyView.substr(dollarPos + 1, hashPos - dollarPos - 1);
        }
        idx.byBase[view.baseNoDollar].push_back(&view);
        idx.byBaseDollar[view.baseWithDollar].push_back(&view);
    }
    idx.builtSize = functions_.size();
    return idx;
}

// 1143：按名取同名额桶——查询名含 '$' 与否选两分表（形态 A/B）；无同名条目
//   返回共享空表。桶向量在下次「size 变化后的重建」前恒有效（单线程遍历期内
//   无插入；views 用 deque 持稳定地址）。
const std::vector<const FuncEntryView*>& SemanticAnalyzer::funcNameBucket(
    const std::string& name) const {
    const FuncNameIndex& idx = ensureFuncNameIndex();
    const auto& table =
        (name.find('$') == std::string::npos) ? idx.byBase : idx.byBaseDollar;
    const auto it = table.find(std::string_view(name));
    if (it == table.end()) {
        return emptyFuncNameBucket();
    }
    return it->second;
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
    // 106（890 残余·泛型方法实参面）：$ 合成名形态（可选$整32——泛型/容器实例
    //   形参替换后的形态）同款豁免。原判据 isOptionalType 只认模板文本
    //   （可选<整32>），泛型方法实参位传 无 被拒并报「无法将 '空类型*' 隐式转换
    //   为 '可选$整32'」（探针 F1/E2 实证；v2 侧同形态放行=宿主单侧拒绝分叉，
    //   规范 07-三 空指针常量→可选 无语境限制）。后缀（* / &）存在时不豁免
    //   ——与 toOpt（isOptionalType 见到后缀即假）口径对齐。
    {
        std::string toCore, toSuffix;
        types::splitTypeSuffix(to, toCore, toSuffix);
        const bool toSynOpt = toSuffix.empty() && toCore.rfind("可选$", 0) == 0;
        if (from == "空类型*" && (toOpt || toSynOpt)) return true;
    }
    // 061-d（2026-09-27 804 轮）：合成体名 ↔ 模板文本 同一性（A-3b 泛型类实例
    //   同构）——容器克隆体签名用合成体名（结果$整32$整32，实参经
    //   canonicalizeSyntheticArgText 统一 $ 形态），用户源码用模板文本
    //   （结果<整32, 整32>），双向视为同一类型。指针/数组后缀保留比对。
    {
        const std::string fromSyn = canonicalizeSyntheticArgText(from);
        const std::string toSyn = canonicalizeSyntheticArgText(to);
        if (fromSyn != from || toSyn != to) return fromSyn == toSyn;
    }
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
    // 234（2026-10-06·用户批准甲案·91303 采样实锤）：类 → 基类链 隐式转换
    //   （指针/引用形态·同地址语义=接口 A1 扩展）——装的是原对象零拷贝；
    //   经基类指针/引用的虚拟调用按动态类型虚表分派（365 三级链实证）；
    //   访问控制按静态类型视角（既有 lookupClassMember ownerClass 体系）。
    //   值形态不在此放行=切片拒绝（专用诊断 reportSliceReject·立法 234）。
    {
        // 形态判定用原始文本（canonical 会剥 &——引用后缀到此处已丢失）；
        // 指针星号 canonical 保留、判定可用规范化形态。
        const bool toIsPtr = types::isPointer(toRaw);
        const bool toIsRef = types::isReference(toRaw);
        if (toIsPtr || toIsRef) {
            std::string toBase = canonicalType(toIsPtr ? types::pointeeOf(toRaw)
                                                       : types::stripRef(toRaw));
            if (isClassType(toBase) && !isInterfaceType(toBase)) {
                std::string fromBase = types::isPointer(from)
                                           ? types::pointeeOf(from)
                                           : canonicalType(types::stripRef(fromRaw));
                if (isClassType(fromBase) && isDerivedFrom(fromBase, toBase)) {
                    return true;
                }
            }
        }
    }
    return types::canConvert(from, to);
}

// 234：类→基类值形态切片拒绝专用诊断（立法 234·防 C++ 经典事故源——值形态
//   截掉派生类字段）。from 为派生类值、to 为其基类值类型时报专用消息并返回
//   true；否则返回 false 交由调用方走通用诊断。
bool SemanticAnalyzer::reportSliceReject(const std::string& fromRaw,
                                         const std::string& toRaw,
                                         const SourceLocation& loc) {
    const std::string from = canonicalType(fromRaw);
    const std::string to = canonicalType(toRaw);
    if (types::isPointer(to) || types::isReference(to)) return false;
    if (!isClassType(to) || isInterfaceType(to)) return false;
    if (!isClassType(from)) return false;
    if (!isDerivedFrom(from, to) || from == to) return false;
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "禁止类切片：'" + from + "' 不能隐式转换为基类值 '" + to +
                            "'——值形态会截掉派生类字段（立法 234·防切片）；"
                            "请用基类指针 '" + to + "*' 或引用同地址持有");
    return true;
}

// 55-c 方案A（2026-09-10 用户裁决，Rust E0308 对齐）：canConvertType 拒绝时的
//   整数字面量豁免——源/目标均为整数族且值表达式为整数字面量形态（含 -1）
//   时放行。赋值初始化（visitVarDecl）/传参（visitCallExpr 逐参）/返回
//   （visitReturnStmt）三面统一走本函数；二元运算面豁免在 visitBinaryExpr
//   内联（isIntLiteralExpr 判定后跳过混合符号检查）。
bool SemanticAnalyzer::canConvertWithLiteral(const Expr* value,
                                             const std::string& fromRaw,
                                             const std::string& toRaw) const {
    if (canConvertType(fromRaw, toRaw)) {
        // 558（506甲·用户裁决 2026-10-13）：可隐式转换面（含同型）整数字面量
        //   值域门槛——无后缀字面量恒定级整32（literalTypeOf 与值大小无关）而
        //   值可超域，同型转换短路原零检查放行=编译期静默回绕（c506 实锤·
        //   Rust E0308「同型亦查域」同款）；负号形态按负值判域
        //  （-2147483648=整32::MIN 域内合法·存量零翻转）。赋值/返回/传参/
        //   数组元素/字段各面共用本入口·一次补检全链生效。
        // 919 扩面（640 侦察副产品·超承载字面量参与表达式求值双侧静默分叉
        //   根治）：谓词经 cnIsConstIntExpr919 覆盖常量表达式同面——`返回
        //   99999999999999999999/3;` 修前宿主静默错值放行 vs v2 修正后拒的
        //   分叉面（返回/赋值/实参/元素/字段五消费面随本单点统一对齐；值域
        //   判定主体 intLiteralFitsType 表达式扩展见 semantic_literal_domain）。
        // 919-final（2026-10-18 用户裁决「字面量单点判域」）：谓词收拢为
        //   cnExprHasIntLiteral919（含变量形态同判——子树扫描取代 isIntLiteralExpr
        //   ∪ cnIsConstIntExpr919 两形态拼合，单谓词单判据）。
        if (value != nullptr && types::isInteger(fromRaw) &&
            types::isInteger(toRaw) && cnExprHasIntLiteral919(value) &&
            !intLiteralFitsType(value, toRaw)) {
            return false;
        }
        return true;
    }
    // 字面量豁免仅整数族→整数族（浮点/字符串等其它拒绝面不豁免）
    if (!types::isInteger(fromRaw) || !types::isInteger(toRaw)) return false;
    // 319-a（T36·方案甲）：值域门槛——豁免=字面量且值在目标类型域内
    //（整16 甲=40000 超域拒绝，域内 整8 a=10 保留豁免；表达式面口径统一）
    return isIntLiteralExpr(value) && intLiteralFitsType(value, toRaw);
}
// 285（001 §3.7a 窄算术回绕·甲案 2026-10-08）：实参位窄字面量回绕放行——
//   T36 的传参值域检查被 §3.7a 容器元素窄域口径显式覆盖（p1007_05 验收基线
//   五值含 向量<整8> 追加(200) 读回 -56·v2 零改动锚=放行回绕；发射层窄槽
//   存回截断天然承载回绕）。仅实参面：字面量→窄整数目标放行；声明/赋值/
//   返回面维持 T36 值域检查不变（整16 甲=40000 拒仍在）。
bool SemanticAnalyzer::canConvertArgNarrow(const Expr* value,
                                           const std::string& fromRaw,
                                           const std::string& toRaw) const {
    if (canConvertWithLiteral(value, fromRaw, toRaw)) return true;
    if (!types::isInteger(fromRaw) || !types::isInteger(toRaw)) return false;
    if (!isIntLiteralExpr(value)) return false;
    const std::string to = types::canonical(toRaw);
    return to == "整8" || to == "整16" || to == "正8" || to == "正16";
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
