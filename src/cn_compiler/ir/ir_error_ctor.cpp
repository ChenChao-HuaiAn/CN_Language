// CN-IR生成器——阶段3 内置构造器降级（Task 3.5，规格书07-五）
// 职责：
//   正常(值)/错误(值)/某些(值) 内置构造器 -> 降级为结果/可选合成结构体构造：
//     结果<T,E> -> 结构体 { 布尔 是否正常; 联合体 { T 值; E 错误值 } }
//     可选<T>   -> 结构体 { 布尔 是否某些; T 值 }
//   分配临时结构体槽（Alloca 按语义层 typeSizeOf 字节数）、写入字段
//     （是否正常/是否某些 = 1/0，值/错误值 = 实参），返回结构体地址（ptr）。
//   调用方（返回语句/变量声明/赋值）按既有结构体路径 CopyStruct 到目标。
// 语义层 visitCallExpr 已推导实际类型并写回 node->resolvedType
//   （如 结果<整32,整32>/可选<字符串>），本模块按此降级。
// 规范：英文API命名，中文仅注释；函数<=100行
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 内置构造器降级入口（visitCallExpr 钩子）：
//   正常(值) -> 结果<T,E> { 是否正常=1; 值 }
//   错误(值) -> 结果<T,E> { 是否正常=0; 错误值 }
//   某些(值) -> 可选<T> { 是否某些=1; 值 }
// 返回 true 表示已处理；false 表示非内置构造器（交回原路径）
bool IRGenerator::handleResultCtor(CallExpr* node) {
    if (semantic_ == nullptr || function_ == nullptr) return false;
    if (node->callee->getType() != NodeType::IdentifierExpr) return false;
    const std::string name =
        static_cast<IdentifierExpr*>(node->callee.get())->name;
    if (name != "正常" && name != "错误" && name != "某些") return false;
    std::string structName;   // 合成结构体名（结果$T$E / 可选$T）——209-a 提升至主函数
    std::string valueType;    // 值字段类型（T/E）
    // 209-a（2026-09-15 第两百零九轮，D1 行数整改）：目标类型解析迁
    //   resolveResultCtorTargetType（resolvedType 修复+结果/可选 推导——false=交回
    //   原路径）；实参值写入+70-a 装箱 move 迁 emitResultCtorValue。宿主纯重构。
    if (!resolveResultCtorTargetType(node, name, structName, valueType)) {
        return false;
    }

    // 合成结构体须已注册（语义层 lowerResultOptionalTypes 已加入 program->structs）
    // Task 6.1（泛型类实例化方法体）：实例化后类型（结果$整32$整32）可能在
    //   lowerResultOptionalTypes 之后才出现——查不到时触发 ensureLoweredType 再查。
    const StructDecl* decl = semantic_->findStruct(structName).value_or(nullptr);
    if (decl == nullptr) {
        semantic_->ensureLoweredType(node->resolvedType);
        decl = semantic_->findStruct(structName).value_or(nullptr);
        if (decl == nullptr) return false;
    }
    const int structSize = semantic_->typeSizeOf(structName).value_or(8);

    // ---- 分配临时结构体槽（多槽登记：类型大小 -> 槽数）----
    const std::string temp = "__rctor" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
         temp, "ptr", node->location);
    registerVarSlots(temp, structName);
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(temp, "i64")},
                                  "ptr", temp, node->location);

    // ---- C19 根治（338-a·构造端载荷零初始化）----
    //   结果<T,E> 的 值/错误值 **共用联合体偏移**：`错误(码)` 构造只写判别位 +
    //   错误值，值槽（T）保持栈垃圾 → 既有的**无条件**释放路径（块出口/跳出/
    //   赋值位的 preFree 归一化）把垃圾当字符串指针释放——valgrind 8 errors/
    //   4 contexts 实证（「读源文件」结果变量值槽 uninit → __cn_str_free →
    //   trackedUnregister）。语义=C 联合体「未激活成员显式置零」＝Rust drop-flag
    //   「未初始化成员不得参与 drop」的等价实现（free(0) 天然安全）。
    //   落点=**构造端**（爆炸半径最小）：236-a 与 338-a 两次「释放面条件化」
    //   方案均致 76 号用例内存失控（条件释放让**赋值位 preFree 跳过释放旧值**
    //   → O(n²) 泄漏 → 28.9GB OOM 连坐 ZCode），故释放面保持原样零改动。
    //   性能：仅 **错误路径** 多 ceil(structSize/8) 次 8 字节零写（结果体通常
    //   16B=2 次）；成功路径零额外指令（值由 emitResultCtorValue 写满）。
    if (name == "错误") {
        for (int off = 0; off < structSize; off += 8) {
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            ir::IRValue addr = base;
            if (off > 0) {
                addr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                  std::to_string(off), node->location);
            }
            emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "", "ptr",
                 node->location);
        }
    }

    // ---- 写 是否正常/是否某些（结果首字段偏移0；可选首字段偏移0）----
    // 结构体布局（computeLayout）：布尔 1 字节 + 对齐填充后 联合体/值
    // 标志位常量用 i32（mov rcx, 1/0 存 4 字节；.正常/.有值 读取 i1 取低字节，
    //   避免 i1 常量在 codegen 截断/规范化异常——实测 i1 ConstInt "1" 生成 mov rcx,0）
    ir::IRValue flagAddr = base;
    ir::IRValue flagVal = emitResult(
        ir::Opcode::ConstInt, {}, "i32",
        ((name == "正常") || (name == "某些")) ? "1" : "0", node->location);
    emit(ir::Opcode::StorePtr, {flagAddr, flagVal}, ir::IRValue(), "",
         "i32", node->location);

    // ---- 写 值/错误值（联合体/值字段偏移 = 对齐后的偏移）----
    // 结果<T,E>：值/错误值 共用联合体，偏移 = 对齐(布尔=1)后 -> 8；
    // 可选<T>：值字段偏移 = 对齐(布尔=1)后 -> 8。
    // 1672 批10（#1360 处置①）：偏移查询用 Optional 贯穿——原 value_or(-1)+
    //   `if (valueOffset < 0)` 的负值哨兵与「默认 8」双形态退役，has_value()
    //   显式分流（无值=布局/降级漂移内部缺陷），value_or(8) 显形保留原控制流。
    cn::Optional<int> valueOffsetOpt;   // 有值=结果/可选合成体字段偏移已登记
    // 209-a：值偏移段本地推导（isResult/isOptional 声明已随推导分支迁族①）
    const bool isResult = isResultType(node->resolvedType);
    for (const auto& f : decl->fields) {
        if (isResult) {
            if (f.name == "错误值联合") { valueOffsetOpt = semantic_->fieldOffsetOf(decl, f.name); }
        // 静默豁免业务值: 1360 批3A 结果化取值路径——结果/值联合两形态的字段偏移分流·非兜底
        } else {
            if (f.name == "值") { valueOffsetOpt = semantic_->fieldOffsetOf(decl, f.name); }
        }
    }
    // 宿主缺陷根治（2026-08-25）：valueOffset 取真实布局（computeLayout）——
    //   整64/结构体 联合体偏移 8、整32 偏移 4。原强制 8 使 结果<整32,整32>（联合体
    //   真实偏移 4）写/读偏移错位（坏.错误 读 0 实测）。仅未找到字段时防御用 8。
    // 1672 批10 甄别（#1360 处置①）：结果/可选合成体布局必含「错误值联合」/「值」
    //   字段（lowerResultOptionalTypes 单点降级），无值=布局/降级漂移内部缺陷——
    //   补内部错误显形，value_or(8) 控制流不变（driver hasErrors 丢弃产物）。
    if (!valueOffsetOpt.has_value()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 结果/可选构造器字段偏移未登记（'" + structName +
                "'·构造器 '" + name + "'·#1360·1672）");
    }
    const int valueOffset = valueOffsetOpt.value_or(8);
    ir::IRValue valAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                     std::to_string(valueOffset), node->location);

    emitResultCtorValue(node, name, valAddr, valueType);

    // 结果 = 结构体地址（ptr）；调用方按结构体路径 CopyStruct
    lastExpr_ = base;
    (void)structSize;
    return true;
}


// 209-a：族① 构造器目标类型解析——resolvedType 修复（泛型类方法体 AST 共享缺陷根治：
//   returnTypeSrc 优先）+ 结果<T,E>/可选<T> 推导（错误() 存 E 正常() 存 T——2026-08-25
//   CopyStruct 崩溃根治）。false=非构造器语境（交回原路径）。
bool IRGenerator::resolveResultCtorTargetType(CallExpr* node, const std::string& name,
                                              std::string& structName,
                                              std::string& valueType) {
    // 宿主缺陷根治（2026-08-25）：泛型类方法体 AST 共享——node->resolvedType 被
    //   多实例检查覆盖（最后实例残留，如 映射$整64$整64.获取 残留 结果<符号,整32>，
    //   CopyStruct 32 溢出崩溃）。优先用当前函数返回类型（returnTypeSrc，
    //   emitClassMethod 按实例 mi.type 设置）——返回 正常()/错误(码) 的 结果<T,E>
    //   与函数返回类型一致；仅当 returnTypeSrc 为空才回退共享 resolvedType。
    // 081 嵌套合成体守卫（2026-09-29·p0928_08~11 四向跨函数段错误根治）：嵌套
    //   返回类型（可选<结果<整32,整32>> 等）函数体内的**内层**构造器（如 正常(7)
    //   的目标=内层 结果<整32,整32>）曾被无条件覆盖成外层类型——标量实参随
    //   valueType=结构体 走 CopyStruct（值寄存器被当源地址解引用拷贝，gdb 实锤
    //   `mov (%r9)` r9=7 段错误）。守卫双条件，任一命中即放弃覆盖（退回语义层
    //   resolvedType——声明位/实参位推导准确）：①**名不匹配**=构造器名与返回类型
    //   顶层语义不符（某些/无↔可选、正常/错误↔结果——正常(x) 不可能是可选<T> 的
    //   顶层构造）；②**存储聚合×实参非聚合**=覆盖后存储类型（正常/某些取首层 T·
    //   错误取第二层 E）为结构体/合成体而实参为标量或类型未知（字面量无语义层
    //   resolvedType）。泛型残留治理面（存储标量、或实参同为聚合的标识符）覆盖
    //   行为不变——两场景互斥不冲突。
    if (function_ != nullptr && !function_->returnTypeSrc.empty()) {
        const std::string retCanon = types::canonical(function_->returnTypeSrc);
        const bool retIsResult = isResultType(retCanon);
        const bool retIsOptional = isOptionalType(retCanon);
        if (retIsResult || retIsOptional) {
            const bool nameMatched = (retIsOptional && (name == "某些" || name == "无")) ||
                                (retIsResult && (name == "正常" || name == "错误"));
            bool abandonOverride = !nameMatched;
            if (!abandonOverride && !node->arguments.empty() && semantic_ != nullptr) {
                std::string storeType;
                if (retIsResult) {
                    const std::vector<std::string> layerArgs =
                        resultTypeArgs(retCanon).value_or((std::vector<std::string>()));
                    if (layerArgs.size() == 2)
                        storeType = (name == "错误") ? layerArgs[1] : layerArgs[0];
                } else {
                    storeType = optionalTypeArg(retCanon).value_or(std::string());
                }
                if (!storeType.empty() &&
                    semantic_->isStructType(types::canonical(storeType))) {
                    const std::string argType =
                        types::canonical(exprSrcType(node->arguments[0].get()).value_or(""));
                    if (argType.empty() ||
                        !semantic_->isStructType(argType)) {
                        abandonOverride = true;
                    }
                }
            }
            // ③#1588 回归修复（549_嵌套合成体跨函数返回·云端预验片3 红实证）：
            //   语义层 resolvedType 已与实参**自洽**（当前目标的存储层类型 ==
            //   实参类型）→ 放弃覆盖。覆盖源=函数返回类型只对「AST 共享残留/
            //   缺失」兜底；声明/赋值位的目标类型由语义层标注权威。缺此条：
            //   外层函数返回更深嵌套时（返回 可选<可选<可选<整32>>>，体内声明
            //   可选<可选<整32>> 中 = 某些(某些(21))），外层构造器被覆盖成返回
            //   类型 → 载荷层数比实参深一层 → 结构体载荷守卫误报内部错误。
            if (!abandonOverride && !node->resolvedType.empty() &&
                !node->arguments.empty()) {
                // ⚠ 零实参构造器（正常()/错误()/无）必须在此早退——实参位
                //   node->arguments[0] 越界=vector subscript out of range
                //   （1371/1064 cn ir 调试断言实录·549 云预验未覆盖零参面）。
                const std::string curCanon = types::canonical(node->resolvedType);
                std::string curStore;
                if (isResultType(curCanon)) {
                    const std::vector<std::string> curArgs =
                        resultTypeArgs(curCanon).value_or((std::vector<std::string>()));
                    if (curArgs.size() == 2)
                        curStore = (name == "错误") ? curArgs[1] : curArgs[0];
                } else if (isOptionalType(curCanon)) {
                    curStore = optionalTypeArg(curCanon).value_or(std::string());
                } else {
                    // 非合成体残留形态：交给原覆盖（保留泛型残留治理面）
                }
                if (!curStore.empty()) {
                    const std::string argCanon = types::canonical(
                        exprSrcType(node->arguments[0].get()).value_or(""));
                    if (!argCanon.empty() &&
                        types::canonicalEquals(types::canonical(curStore), argCanon)) {
                        abandonOverride = true;
                    }
                }
            }
            if (!abandonOverride) node->resolvedType = function_->returnTypeSrc;
        }
    }
    if (node->resolvedType.empty()) {
        // Task 6.1（泛型类实例化方法体 向量$整32.追加 等）：语义层对实例化类
        //   方法体的 resolvedType 推导可能缺失（泛型上下文），回退用当前函数
        //   返回类型（function_->returnTypeSrc，如 结果<空类型,整32>）——
        //   返回 正常()/返回 错误(码) 的 结果<T,E> 与函数返回类型一致。
        if (function_ == nullptr || function_->returnTypeSrc.empty()) return false;
        node->resolvedType = function_->returnTypeSrc;
        // 错误(码)：错误值类型 = 实参类型（如 错误码.内存 -> 整32 枚举）
        if (name == "错误" && !node->arguments.empty()) {
            // 保持 结果<T,E> 的 E = 实参推导；若返回类型 E 与实参不匹配，
            //   canConvert 已检查；此处直接用函数返回类型（E 一致）。
        }
    }

    // 解析推导类型：结果<T,E> / 可选<T>
    bool isResult = isResultType(node->resolvedType);
    bool isOptional = isOptionalType(node->resolvedType);
    if (isResult) {
        const std::vector<std::string> args =
            resultTypeArgs(node->resolvedType)
                .value_or((std::vector<std::string>()));
        if (args.size() != 2) return false;
        const std::string t = types::canonical(args[0]);
        const std::string e = types::canonical(args[1]);
        structName = resultStructName(t, e);
        // 宿主缺陷根治（2026-08-25）：错误(码) 存 E（错误值类型，如 整32），
        //   正常(值) 存 T——原恒用 T 导致 错误() 分支把 T（结构体）当存储类型，
        //   CopyStruct 从错误码值（如 7）读 32 字节 -> 访问地址 7 崩溃 0xC0000005。
        valueType = (name == "错误") ? e : t;
    } else if (isOptional) {
        const std::string t =
            types::canonical(optionalTypeArg(node->resolvedType)
                                 .value_or(std::string()));
        if (t.empty()) return false;
        structName = optionalStructName(t);
        valueType = t;
    } else {
        return false;
    }
    return true;
}

// 209-a：族② 实参值写入入口——求值（结构体字面量物化/普通表达式）→ 类值深拷
//   分叉 → 载荷按**目标载荷类型**宽度写入（结构体 CopyStruct / 标量 Cast+StorePtr）；
//   70-a 装箱 move（正常/某些+字符串值+非污染 → 源槽清零，与 转移() 浅交接同模型）。
//   #1588：原单函数 130 行超 100 行红线，按「先拆再加」拆出类深拷/结构体载荷/
//   标量载荷三族；结构体载荷新增同型守卫（禁静默按位拷贝，见子函数注释）。
void IRGenerator::emitResultCtorValue(CallExpr* node, const std::string& name,
                                      ir::IRValue valAddr, const std::string& valueType) {
    // 1611（2026-10-25·1025 采样轮 d5 实锚）：载荷类名**先物化**再判定（声明
    //   上提函数体层——1611 move 转交段[块外]与深拷判定[块内]共用）。
    const std::string canonValueType =
        resolveGenericInstanceType(types::canonical(valueType));
    bool deepCopied100 = false;
    // 实参值（构造器单参数）
    if (!node->arguments.empty()) {
        ir::IRValue val;
        // 067-002 治本（合成体消费面同族·d3 最小复现）：结构体字面量实参须
        //   物化（复用 81-a materializeStructInitArg 单点设施）——原 genExpr
        //   直通字面量节点返回空值 0 → CopyStruct 源=NULL 运行崩溃
        //   （宿主 asm 铁证 `mov rsi,0` + `rep movsb`，d3/d2/546 实测
        //   0xC0000005）。普通调用（46-a D3）与方法调用（81-a）路径早已物化，
        //   内置构造器路径遗漏。Rust 对照：值上下文临时 place 物化。
        if (node->arguments[0]->getType() == NodeType::StructInitExpr) {
            val = materializeStructInitArg(
                static_cast<StructInitExpr*>(node->arguments[0].get()),
                node->location);
        } else {
            val = genExpr(node->arguments[0].get());
        }
        // 1611（2026-10-25·1025 采样轮 d5 实锚）：载荷类名**先物化**再判定——
        //   valueType=载荷类型文本（字符串映射<整32>），canonical 不转 $ 实例名，
        //   isClassType/findClass/findCopyConstructor 按精确实例名查表恒 miss →
        // 任务 100（008 树波 4）：类值装箱统一深拷（详见子函数注释·1611 物化
        //   与 deepCopied100 声明上提至函数体层——1611 move 转交段共用）。
        if (semantic_ != nullptr && semantic_->isClassType(canonValueType)) {
            deepCopied100 =
                emitResultCtorClassDeepCopy(node, name, valAddr, canonValueType, val);
        }
        // 归属分派：结构体内联拷贝 / 标量按自然宽度（详见证型分派注释）——
        //   两个显式互补分支（无 else 兜底：布尔枚举完备，门禁 else 业务新规）
        const bool payloadIsStruct =
            semantic_ != nullptr && semantic_->isStructType(canonValueType);
        if (payloadIsStruct) {
            emitResultCtorStructPayload(node, valAddr, canonValueType, val);
        }
        if (!payloadIsStruct) {
            emitResultCtorScalarPayload(valAddr, canonValueType, val,
                                        node->location);
        }
    }

    // 70-a（2026-09-11 方案A 用户裁决）：装箱 move 语义——正常(拥有字符串局部) 的
    //   所有权转入结果/可选结构体（Rust Ok(s) move 同款零拷贝）：值字段写入后
    //   源槽清零（与 转移() 浅交接同模型），函数返回块 genStringFrees 对已清零
    //   槽空安全跳过——根治「装箱浅共享×局部 RAII 释放=悬垂」家族第五实例
    //   （stdlib 文件/IO 读取行包装链实证：装箱值随源 free 悬垂，v2p 读源文件
    //   全空）。借用装箱（污染名单内）不清零——指针共享随源存活。
    // 1604：扩 错误(拥有字符串局部)——E 载荷释放面接通后错误槽将摘句柄释放，
    //   源不清零=借用共享+槽释放=提前释放源的串（UAF）；与 正常/某些 同款
    //   move（错误() 的 valueType=E 载荷·resolveResultCtorTargetType 口径）。
    if ((name == "正常" || name == "某些" || name == "错误") &&
        !node->arguments.empty() &&
        node->arguments[0]->getType() == NodeType::IdentifierExpr &&
        types::canonicalEquals(valueType, "字符串")) {
        const std::string srcName = static_cast<const IdentifierExpr*>(
            node->arguments[0].get())->name;
        if (stringTainted_.count(srcName) == 0) {
            const cn::Optional<std::string> slotOpt = lookupVarName(srcName);
            if (slotOpt.has_value()) {
                const std::string& slot = slotOpt.value();
                emit(ir::Opcode::Store, {ir::IRValue::constant("0", "i64")},
                     ir::IRValue(), slot, "ptr", node->location);
            }
        }
    }
    // 1611：无拷贝构造类返回位 move 转交（详注见函数体·子函数单点）
    emitResultCtorReturnMoveForClass(node, name, deepCopied100, canonValueType);
}

// #1588：结构体载荷写入——CopyStruct 内联拷贝（2026-08-25 根治：原 StorePtr 只存
//   8 字节地址，结果.值 读到地址而非结构体数据）。
//   守卫（禁静默按位拷贝）：载荷=结构体时实参必须是**同一结构体类型**（canonical
//   归一比较）——异型/标量实参按目标结构体大小 CopyStruct = 字段错位/越界读栈
//   垃圾（1585 审计同族：窄载荷×宽目标）。语义层在目标已知时已按载荷可转换性
//   报用户错误（载荷不可转换=单一可行动诊断，编译终止不产出可执行文件）；本
//   守卫只在**无上游错误**时报内部错误（语义层漏标/共享 AST 残留=编译器 bug
//   显形，#648 miss 不可静默）——有上游错误时保持原发射（产物不链接，无静默面）。
//   结构体字面量实参（StructInitExpr 物化为盒）豁免：其类型即字面量类型名，
//   语义层已按目标载荷检查（异型=已报错）。
void IRGenerator::emitResultCtorStructPayload(CallExpr* node, ir::IRValue valAddr,
                                             const std::string& valueType,
                                             ir::IRValue val) {
    const bool argIsStructInit =
        node->arguments[0]->getType() == NodeType::StructInitExpr;
    const std::string argSrc =
        types::canonical(exprSrcType(node->arguments[0].get()).value_or(""));
    const bool payloadSameStruct =
        !argSrc.empty() && semantic_->isStructType(argSrc) &&
        types::canonicalEquals(argSrc, valueType);
    // 实参类型可解析且明确不匹配=编译器内部不一致（语义层漏检）；不可解析
    //   （下标/成员/调用等 IR 层 exprSrcType 无值形态）=信任语义层载荷检查
    //   （checkExpr 返回真实类型并已按目标载荷校验），不误报。
    if (!argIsStructInit && !argSrc.empty() && !payloadSameStruct &&
        !diagnostics_.hasErrors()) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 内置构造器结构体载荷类型不一致（载荷 '" + valueType +
                "'，实参 '" + (argSrc.empty() ? std::string("未知") : argSrc) +
                "'）——语义层漏检（共享 AST 残留/目标校准遗漏），已报编译器缺陷");
    }
    const int size = semantic_->typeSizeOf(valueType).value_or(8);
    // #1588 回归修复（678）：正常(无) 且目标载荷=结构体/合成体（如
    //   结果<可选<整32>,整32> 的外层 T=可选<整32>）——无 是 NullLiteral，
    //   genExpr 产物=空指针值 0，不是结构体地址；按位 CopyStruct 会从地址 0
    //   读 size 字节（实测 C0000005）。语义=空载荷（合成体判别位 0+载荷 0）
    //   → 按目标宽度零写（与 错误() 构造端零初始化同一设施形态；合成体布局
    //   按 8 字节槽圆整，逐槽 8 字节清零不越界）。
    if (node->arguments[0]->getType() == NodeType::NullLiteral) {
        for (int off = 0; off < size; off += 8) {
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            ir::IRValue addr = valAddr;
            if (off != 0) {
                addr = emitResult(ir::Opcode::FieldAddr, {valAddr}, "ptr",
                                  std::to_string(off), node->location);
            }
            emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "", "ptr",
                 node->location);
        }
        return;
    }
    emit(ir::Opcode::CopyStruct, {valAddr, val}, ir::IRValue(),
         std::to_string(size), "void", node->location);
}

// #1588：标量/指针载荷写入——按值类型自然宽度存储（2026-08-25 根治：整32 -> i32
//   4 字节、整64/字符串/指针 -> i64/ptr 8 字节、布尔 -> i8 1 字节；原强制 i64
//   使 结果<整32,整32> 联合体 4 字节写入越界）。宽窄差异由 Cast 补齐（如
//   某些(整32 字面量) 进 可选<整64>：i32 → i64 后写满 8 字节，#1588 主缺陷面）。
void IRGenerator::emitResultCtorScalarPayload(ir::IRValue valAddr,
                                             const std::string& valueType,
                                             ir::IRValue val,
                                             const SourceLocation& loc) {
    const std::string valIrType = mapType(valueType.empty() ? "整32" : valueType);
    std::string storeType = valIrType;
    if (storeType == "i1") storeType = "i8";
    if (val.type != storeType && !storeType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, storeType, "", loc);
    }
    emit(ir::Opcode::StorePtr, {valAddr, val}, ir::IRValue(), "",
         storeType, loc);
}

// 任务 100（2026-09-29·008 树波 4）：类值装箱**统一深拷**——有析构∧有拷贝构造类
//   实参：NewObject+拷贝构造（源=实参左值地址）产出独立副本句柄存盒。原路径
//   （genExpr 句柄浅存+源 RAII 析构照发）=盒携悬垂句柄跨作用域（p8 IR 铁证：
//   装箱→删除对象→返回悬垂）——深拷后盒恒独立副本，收集面
//   （collectOwnedStrFields 类值条件打开）配套盒亡条件析构，别名/悬垂/泄漏三面
//   同根除；〔基准=019〕第 1 句默认值语义（未显式 转移() 不自动 move·源照常
//   可用），热路径零拷由第 4 句 LUE/转移() 承接。无拷贝构造类=不纳管。
bool IRGenerator::emitResultCtorClassDeepCopy(CallExpr* node, const std::string& name,
                                              ir::IRValue valAddr,
                                              const std::string& valueType,
                                              ir::IRValue& val) {
    const std::string& clsCanon = valueType;
    // #1466 根治（宿主 C0000005·v2 正常·零分叉）：实参=无左值槽的 rvalue 临时
    //   （构造调用 `货物("x")` 产物）——emitResultCtorValue 已用 genExpr 求值一次
    //   （val=新堆对象句柄）。原实现此处再经 lvalueAddress 求值一次，双重缺陷：
    //   ①二次 NewObject+构造调用=首个对象无人持有（每装箱净漏 1 壳+字段）；
    //   ②lvalueAddress 对调用表达式的兜底分支是「按表达式值处理」=genExpr，产物
    //   是**对象句柄**而非拷贝构造契约要的「存句柄的槽地址」——callee 内
    //   [其他]=[句柄]=虚表指针，再按字段偏移解引用=野指针读 C0000005
    //   （fault=串字节·p3/p4/p5b 实锚）。临时对象无源槽可共享、无出口析构面 =
    //   C++ rvalue 临时 move / v2 107「T_对象 源=句柄直移交」同构：句柄原样
    //   留盒（盒=唯一拥有者），不发拷贝构造、不登记 889 深拷面。
    //   左值 place 实参（标识符/下标/成员/解引用/引用返回调用）维持既有深拷。
    if (builtinCtorArgIsRvalueTemp(node)) {
        return false;
    }
    const ClassInfo* ci = semantic_->findClass(clsCanon).value_or(nullptr);
    bool hasDtor = false;
    if (ci != nullptr) {
        for (const auto& mk : ci->methods) {
            if (mk.second.isDestructor) { hasDtor = true; break; }
        }
    }
    const ClassMemberInfo* copyCtor =
        semantic_->findCopyConstructor(clsCanon).value_or(nullptr);
    // 1611：返回值=是否实际深拷——显式双值契约返回（非查表 miss 静默面）：
    //   真=盒内独立副本·源照常可用照常析构；假=浅存路径**提前返回**（无析构
    //   或无拷贝构造·深拷无从起），调用方 1611 返回位 move 转交段按契约接管。
    const bool deepCopyApplicable = hasDtor && copyCtor != nullptr;
    if (!deepCopyApplicable) {
        return deepCopyApplicable;
    }
    const std::string copyOwner =
        copyCtor->ownerClass.empty() ? clsCanon : copyCtor->ownerClass;
    ir::IRValue newObj = emitResult(
        ir::Opcode::NewObject, {ir::IRValue::constant(clsCanon, "ptr")}, "ptr",
        clsCanon + "|" + std::to_string(ci->totalSize), node->location);
    // 898（2026-09-30·116 红灯根治）：按实参形态分派源拷贝——拷贝构造 callee
    //   契约=「其他=存句柄的槽地址」（callee 内两层 Load：[槽]=句柄→[句柄]=字段）。
    //   ①实参=类变量（Identifier）：lvalueAddress 产出=槽地址，直调拷贝构造 ✓；
    //   ②实参=下标/成员（容器内联元素·类内联字段）：lvalueAddress 产出=**对象本体
    //   地址**（一步即字段），直调=被调方 [对象]=首字段值再解引用=以字段值当地址
    //   C0000005（116 栈.弹出/队.出队 `正常(数据[i])` 崩点实证）——改盒内字段拷
    //   （emitStructCopyWithFields srcAddr=对象地址契约 ✓·拥有型串字段深拷）；
    //   类句柄字段浅拷=诚实边界（随任务 096 拥有位统一解），较直调崩已纯改善。
    const NodeType argKind = node->arguments[0]->getType();
    if (argKind == NodeType::IndexExpr || argKind == NodeType::MemberExpr) {
        ir::IRValue srcObj = lvalueAddress(node->arguments[0].get());
        emitStructCopyWithFields(newObj, srcObj, clsCanon, node->location,
                                 /*preFree=*/false, /*deepCopy=*/true);
    } else {
        ir::IRValue srcAddr = lvalueAddress(node->arguments[0].get());
        emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
             methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
             node->location);
    }
    val = newObj;
    // 100（939·008 收官总攻·泄漏面根治）：**实例级登记**——深拷分叉的盒内独立
    //   副本（盒=唯一拥有者）登记块出口条件释放（tag 真 DeleteObject+清槽幂等）。
    //   绕开收集面（183-a 116 双删回退史：898 容器内联元素浅拷分叉**不登记**=
    //   不误伤容器元素管理）。valAddr=盒值字段地址（调用方 StorePtr 同址·虚拟
    //   寄存器跨块恒定）。登记条件=结果盒（正常(类值)）：可选盒由收集面外壳路径
    //   覆盖（551 基线绿·双登记=双删）；返回表达式内豁免（盒移交调用方——浅拷
    //   共享句柄·被调方释放=悬垂）。
    if (name == "正常" && !inReturnExpr_) {
        pendingBoxCopies_.push_back({valAddr, clsCanon});
    }
    return true;
}

// #1466：内置构造器首个实参「无左值槽 rvalue 临时」判定——装箱所有权链单点。
//   true=调用表达式的类对象产物（构造调用/方法返回的堆句柄）：无源槽可传拷贝
//   构造、无出口析构面 → 句柄移交盒（move·盒唯一拥有者）。
//   false=可寻址 place 形态（标识符/下标/成员/解引用/引用返回调用）或非调用
//   实参——维持既有深拷/借用分流（引用返回调用=被引用对象的借用 place，
//   lvalueAddress 取其地址语义正确，禁按临时接管）。
//   纯谓词（真假语义判定·非失败通道·#1360 谓词免除结果化）。
bool IRGenerator::builtinCtorArgIsRvalueTemp(const CallExpr* ctor) const {
    return ctor != nullptr && !ctor->arguments.empty() &&
           ctor->arguments[0]->getType() == NodeType::CallExpr &&
           !static_cast<const CallExpr*>(ctor->arguments[0].get())
                ->isRefReturnCall;
}

// #1466：盒初值「正常/某些(类构造临时)」判定——盒载荷=被移交的临时句柄
//   （盒=唯一拥有者·emitResultCtorClassDeepCopy 句柄移交面）。
//   只认 正常/某些（错误() 的载荷=E 且判别位为假，盒亡释放面不删）；非构造器
//   初值恒 false（调用返回盒等形态维持既有声明位判据）。纯谓词（真假判定·
//   非失败通道）。
bool IRGenerator::boxInitMovesTempPayload(const Expr* init) const {
    const bool isCall = init != nullptr && init->getType() == NodeType::CallExpr;
    const auto* call = isCall ? static_cast<const CallExpr*>(init) : nullptr;
    const bool isNormalOrSomeCtor =
        call != nullptr && call->callee != nullptr &&
        call->callee->getType() == NodeType::IdentifierExpr &&
        (static_cast<const IdentifierExpr*>(call->callee.get())->name == "正常" ||
         static_cast<const IdentifierExpr*>(call->callee.get())->name == "某些");
    return isNormalOrSomeCtor && builtinCtorArgIsRvalueTemp(call);
}


// 1611（2026-10-25·1025 采样轮 d5 实锚·与 70-a 同构）：**无拷贝构造类**
//   返回位装箱=move 转交——深拷（任务 100）只覆盖「有析构∧有拷贝构造」类
//   （独立副本·源照常可用）；无拷贝构造类（stdlib 字符串映射<整32> 实锚）
//   原浅存源句柄，返回后源 RAII 析构（genClassDestructorCalls 返回块
//   DeleteObject）=返回聚合携悬垂句柄，调用方 .值.方法() 读已释放对象
//   （d5 取表() 返回 可选<字符串映射<整32>> 后 甲.值.获取 miss——落变量
//   基线同缺=非链式专属·函数返回聚合+类载荷即中）。v2 侧同域立法=71-a
//   「装箱（正常/某些(局部)）=move 转交，出口析构跳过」——宿主补齐同语义：
//   返回表达式内（return 后源不可达·无 019 第 1 句「源照常可用」冲突）
//   源槽清零，出口析构空安全跳过（codegen test/je 在案形态），所有权随
//   聚合移交调用方。仅对 RAII 拥有面（ownedClassOrder_ 在册）生效——
//   借用视图/非拥有句柄不在册，清零=破坏视图源，不清。
void IRGenerator::emitResultCtorReturnMoveForClass(
    CallExpr* node, const std::string& name, bool deepCopied100,
    const std::string& canonValueType) {
    if (!node->arguments.empty() && !deepCopied100 && inReturnExpr_ &&
        (name == "正常" || name == "某些") &&
        node->arguments[0]->getType() == NodeType::IdentifierExpr &&
        semantic_ != nullptr && semantic_->isClassType(canonValueType)) {
        const std::string srcVarName = static_cast<const IdentifierExpr*>(
            node->arguments[0].get())->name;
        const cn::Optional<std::string> srcSlotOpt = lookupVarName(srcVarName);
        if (srcSlotOpt.has_value() &&
            std::find(ownedClassOrder_.begin(), ownedClassOrder_.end(),
                      srcSlotOpt.value()) != ownedClassOrder_.end()) {
            emit(ir::Opcode::Store, {ir::IRValue::constant("0", "i64")},
                 ir::IRValue(), srcSlotOpt.value(), "ptr", node->location);
        }
    }
}


} // namespace cn_compiler
