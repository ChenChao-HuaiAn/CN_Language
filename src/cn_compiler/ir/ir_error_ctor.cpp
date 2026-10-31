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
    // #1884（#1466 遗留①甲案·跨函数甄别）：**被调方返回位登记载荷所有权主张**
    //   ——调用方装盒时据此判定盒亡是否释放载荷（p8 装盒()/p12 装盒() 拥有、
    //   p16 包形参(引用参数) 借用）。同一函数多返回语句：任一借用=整体借用
    //   （保守不登记=只残留不双删）。
    if (inReturnExpr_ && function_ != nullptr && !function_->name.empty() &&
        (name == "正常" || name == "某些")) {
        registerBoxReturnClaim(function_->name,
                               resultCtorReturnPayloadOwned(node));
    }
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


// ==================== #1884：结果/可选盒的类载荷所有权族 ====================
// 背景（#1466 遗留①②③ 宿主侧三面，与 v2 946/1605 模型的零分叉对齐面）：
//   · p8/p12：盒初值=调用返回（装盒()）且载荷 dtor-only——盒亡判据只认
//     「有拷贝构造 ∨ 正常/某些(类构造临时)」，普通调用返回盒不登记 → 净漏 1。
//     001 #1784 R1（返回值=值副本·所有权归调用方）与 85-a 聚合返回契约下，
//     调用返回盒的载荷归接收盒所有——判据补「初值=非引用返回调用」。
//   · p18：可选 无 初值盒（NullLiteral 零写早退）漏盒亡登记 → 后续赋值接管
//     的载荷无人释放（残留 1）。零值盒同样登记（tag 假 → 释放空跳过）。
//   · p19：赋值位 r = 正常(g) 的 889 表达式面（pendingBoxCopies_ 深拷副本）
//     与盒亡面释放同一副本 → 双删 C0000374。修=目标盒接管后清源 rctor 值字段
//     （与声明位 280 同模型·move 归一）。
//   · p22：循环赋值覆盖旧载荷无人释放 → 中间载荷净漏；修=赋值位 preFree
//     条件释放旧载荷（盒在册时——不在册=借用盒，旧载荷归源槽，禁删）。
//   · p20：盒变量互拷 b = a 位拷共享类句柄 → 两盒亡面双删 C0000374；修=
//     类载荷 tag 条件深拷（源值字段槽地址=拷贝构造「其他」契约·与 v2 1605
//     类分支同构）。
// 布局/符号键单点=boxClassPayloadOf；释放/深拷/登记三面共用，避免各自推导
// 偏移与键（#1716 同语义单名）。

// 识别结果/可选盒的类载荷（无值=非合成体/载荷非类/类无析构）。
//   载荷名从**源形态文本**解（resultTypeArgs/optionalTypeArg 只认模板文本；
//   $ 合成名边界=宁漏勿错——与 collectOwnedStrFields 571 同口径）。
cn::Optional<BoxClassPayload> IRGenerator::boxClassPayloadOf(
    const std::string& canonRaw) const {
    if (semantic_ == nullptr) return std::nullopt;
    const std::string canon = canonicalizeSyntheticArgText(canonRaw);
    const bool isResult = canon.rfind("结果$", 0) == 0;
    const bool isOptional = canon.rfind("可选$", 0) == 0;
    if (!isResult && !isOptional) return std::nullopt;
    std::string payloadCanon;
    if (isResult) {
        const std::vector<std::string> args =
            resultTypeArgs(canonRaw).value_or((std::vector<std::string>()));
        if (args.size() == 2) payloadCanon = types::canonical(args[0]);
    } else {
        payloadCanon = types::canonical(
            optionalTypeArg(canonRaw).value_or(std::string()));
    }
    if (payloadCanon.empty() || !semantic_->isClassType(payloadCanon)) {
        return std::nullopt;
    }
    // 有析构才存在盒亡释放义务（无析构类字段无资源语义，盒不承担删除）
    const ClassInfo* ci = semantic_->findClass(payloadCanon).value_or(nullptr);
    if (ci == nullptr) return std::nullopt;
    bool hasDtor = false;
    for (const auto& mk : ci->methods) {
        if (mk.second.isDestructor) { hasDtor = true; break; }
    }
    if (!hasDtor) return std::nullopt;
    const StructDecl* sd = semantic_->findStruct(canon).value_or(nullptr);
    if (sd == nullptr) return std::nullopt;
    const cn::Optional<int> tagOff =
        semantic_->fieldOffsetOf(sd, isResult ? "正常" : "有值");
    const cn::Optional<int> valOff = semantic_->fieldOffsetOf(sd, "值");
    if (!tagOff.has_value() || !valOff.has_value()) return std::nullopt;
    BoxClassPayload pay;
    pay.tagOffset = tagOff.value();
    pay.valueOffset = valOff.value();
    pay.classCanon = payloadCanon;
    pay.copyCtorKey = classCopyCtorSymbolKey(payloadCanon).value_or(std::string());
    return pay;
}

// 盒旧载荷条件释放（preFree 面）：tag 假（错误态/无值态）免疫——值位是错误码
//   整数/0，按句柄删即 UB。runtime 单调用 __cn_box_class_delete（946 单点·
//   免块切割）摘句柄+清槽幂等；DeleteObject 空句柄空安全，重复经过=读 0 跳过。
void IRGenerator::emitBoxClassPayloadPreFree(const ir::IRValue& dstBase,
                                             const BoxClassPayload& pay,
                                             const SourceLocation& loc) {
    ir::IRValue tagAddr = pay.tagOffset == 0
        ? dstBase
        : emitResult(ir::Opcode::FieldAddr, {dstBase}, "ptr",
                     std::to_string(pay.tagOffset), loc);
    ir::IRValue fieldAddr = pay.valueOffset == 0
        ? dstBase
        : emitResult(ir::Opcode::FieldAddr, {dstBase}, "ptr",
                     std::to_string(pay.valueOffset), loc);
    ir::IRValue objPtr = emitResult(ir::Opcode::Call, {tagAddr, fieldAddr}, "ptr",
                                    "__cn_box_class_delete", loc);
    emitContainerElemFreeFor(pay.classCanon, objPtr, loc);
    emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(), pay.classCanon,
         "void", loc);
}

// 盒载荷条件深拷（postCopy 面）：位拷已把 src 的句柄搬进 dst —— tag 真时改写成
//   独立副本（NewObject + 拷贝构造·实参=**源值字段槽地址**（存句柄的槽，copy
//   契约「其他」两层 Load 的第一层）），tag 假保留位拷的错误码。
//   无拷贝构造=共享边界：不发（源槽拥有，登记面不登记该盒）。
//   分支块形态与 beginSelfAssignGuard 同款（Eq+Branch 已验证的块模型）。
void IRGenerator::emitBoxClassPayloadPostCopy(const ir::IRValue& dstBase,
                                              const ir::IRValue& srcBase,
                                              const BoxClassPayload& pay,
                                              const SourceLocation& loc) {
    if (pay.copyCtorKey.empty()) return;
    ir::IRValue tagAddr = pay.tagOffset == 0
        ? srcBase
        : emitResult(ir::Opcode::FieldAddr, {srcBase}, "ptr",
                     std::to_string(pay.tagOffset), loc);
    ir::IRValue tagVal = emitResult(ir::Opcode::LoadPtr, {tagAddr}, "i1", "", loc);
    const std::string bodyLabel = "bb" + std::to_string(blockCounter_++);
    const std::string mergeLabel = "bb" + std::to_string(blockCounter_++);
    endBranch(tagVal.toString(), bodyLabel, mergeLabel);
    setCurrentBlock(newBlock(bodyLabel));
    const ClassInfo* ci = semantic_->findClass(pay.classCanon).value_or(nullptr);
    const std::string extra =
        pay.classCanon + "|" +
        std::to_string(ci != nullptr ? ci->totalSize : 0);
    ir::IRValue newObj = emitResult(
        ir::Opcode::NewObject, {ir::IRValue::constant(pay.classCanon, "ptr")},
        "ptr", extra, loc);
    ir::IRValue srcFieldAddr = emitResult(
        ir::Opcode::FieldAddr, {srcBase}, "ptr",
        std::to_string(pay.valueOffset), loc);
    emit(ir::Opcode::Call, {newObj, srcFieldAddr}, ir::IRValue(), pay.copyCtorKey,
         "void", loc);
    ir::IRValue dstFieldAddr = emitResult(
        ir::Opcode::FieldAddr, {dstBase}, "ptr",
        std::to_string(pay.valueOffset), loc);
    emit(ir::Opcode::StorePtr, {dstFieldAddr, newObj}, ir::IRValue(), "", "ptr",
         loc);
    endJump(mergeLabel);
    setCurrentBlock(newBlock(mergeLabel));
}

// #1884：盒初值=普通函数/方法调用（非引用返回）→ **查被调方返回位所有权主张
//   表**（#1466 遗留①甲案·跨函数甄别）：被调方返回 正常/某些(...) 时按载荷
//   来源登记（resultCtorReturnPayloadOwned），调用点命中「移交」才判盒拥有。
//   · 内置构造器（正常/错误/某些）不查表：拥有关系取决于实参形态（临时移交=
//     boxInitMovesTempPayload；place 无拷贝构造=借用共享·p13 双删录）。
//   · 方法调用（callee=成员）面外=保守借用（跨类方法表判定另案；宁漏不双删）。
//   · 表 miss（被调函数在调用点之后生成/跨翻译单元）=未知=保守借用。
//   纯谓词（真假判定·#1360 谓词免除结果化）。
bool IRGenerator::boxInitFromOwnedCall(const Expr* init) const {
    // 单出口形态（#1360 谓词免结果化·非失败通道）：形态不适用=保持假，不是
    //   「静默默认值继续编译」——全判定链显式枚举，无提前 return 短路。
    bool owned = false;
    if (init != nullptr && init->getType() == NodeType::CallExpr) {
        const auto* call = static_cast<const CallExpr*>(init);
        const bool calleeIsFreeFunction =
            call->callee != nullptr && !call->isRefReturnCall &&
            call->callee->getType() == NodeType::IdentifierExpr;
        if (calleeIsFreeFunction) {
            const std::string& name =
                static_cast<const IdentifierExpr*>(call->callee.get())->name;
            const bool isBuiltinCtor = name == "正常" || name == "错误" ||
                                       name == "某些";
            if (!isBuiltinCtor) {
                const auto it = boxReturnOwnership_.find(name);
                owned = it != boxReturnOwnership_.end() && it->second;
            }
        }
    }
    return owned;
}

// #1884：返回位「正常/某些(载荷)」载荷所有权判定——移交=真（调用方盒可拥有）：
//   · 实参=调用（非引用返回）=无左值槽临时（本函数 NewObject 产物·C++ rvalue
//     move 同构）；转移(...)=显式 move 交接；复制(...)=新副本——均拥有。
//   · 实参=标识符且为**拥有局部**（ownedClassOrder_ 在册=有析构类局部·返回位
//     emitResultCtorReturnMoveForClass 已清源槽）→ 拥有。
//   · 形参/类字段裸名/全局静态（lookupVarName miss 或不在册）/成员/下标/
//     解引用/三元/自身/字符串 → 借用（p16 引用参数实锚）。
bool IRGenerator::resultCtorReturnPayloadOwned(const CallExpr* node) const {
    // 单出口形态（#1360 谓词免结果化）：空实参/不可判形态=不拥有（假），
    //   属形态判定出口而非「静默默认值」——全链显式枚举无提前 return。
    bool owned = false;
    if (node != nullptr && !node->arguments.empty()) {
        const Expr* arg = node->arguments[0].get();
        if (arg->getType() == NodeType::CallExpr) {
            owned = !static_cast<const CallExpr*>(arg)->isRefReturnCall;
        } else if (arg->getType() == NodeType::IdentifierExpr) {
            const std::string& nm = static_cast<const IdentifierExpr*>(arg)->name;
            const cn::Optional<std::string> slot = lookupVarName(nm);
            if (slot.has_value()) {
                owned = std::find(ownedClassOrder_.begin(), ownedClassOrder_.end(),
                                  slot.value()) != ownedClassOrder_.end();
            }
        }
    }
    return owned;
}

// #1884：盒返回所有权主张登记（被调方返回位）——同函数多返回语句语义=任一
//   借用则整体按借用（保守：调用方不登记释放=不漏删；漏放残留可观测不崩）。
void IRGenerator::registerBoxReturnClaim(const std::string& fnName, bool owned) {
    const auto it = boxReturnOwnership_.find(fnName);
    if (it == boxReturnOwnership_.end()) {
        boxReturnOwnership_.emplace(fnName, owned);
        return;
    }
    if (!owned) it->second = false;
}

// 盒变量是否已在盒亡登记表（赋值位「旧载荷可释放 / 补登记」判据）。
bool IRGenerator::boxVarInRegistry(const std::string& unique) const {
    bool found = false;
    for (const auto& ent : pendingBoxVars_) {
        // 空名不匹配任何登记项（登记面 unique 恒非空）——形态判定非失败通道
        if (unique == std::get<3>(ent)) {
            found = true;
            break;
        }
    }
    return found;
}

// 盒亡释放登记发射（声明位/赋值位共用）：tag/值字段地址由盒子基址+布局算，
//   条目格式与 emitPendingBoxVarsRelease 契约一致（tag 假免疫·摘句柄清槽幂等）。
void IRGenerator::emitBoxVarReleaseRegister(const std::string& declCanon,
                                            const ir::IRValue& dstAddr,
                                            const std::string& unique,
                                            const SourceLocation& loc) {
    const cn::Optional<BoxClassPayload> pay = boxClassPayloadOf(declCanon);
    if (!pay.has_value()) return;
    ir::IRValue tagAddr = pay.value().tagOffset == 0
        ? dstAddr
        : emitResult(ir::Opcode::FieldAddr, {dstAddr}, "ptr",
                     std::to_string(pay.value().tagOffset), loc);
    ir::IRValue fieldAddr = pay.value().valueOffset == 0
        ? dstAddr
        : emitResult(ir::Opcode::FieldAddr, {dstAddr}, "ptr",
                     std::to_string(pay.value().valueOffset), loc);
    pendingBoxVars_.emplace_back(tagAddr, fieldAddr, pay.value().classCanon,
                                 unique, false);
}

// 盒值字段清零（内置构造器临时盒的 move 归一·声明位 280 与赋值位共用）：
//   目标盒 CopyStruct 接管句柄后清源临时盒值字段——889 表达式面
//   （pendingBoxCopies_·valAddr=源值字段）释放面读 0 空跳过，防同一副本
//   被表达式面与盒亡面双删（p19 C0000374 实锚）。
void IRGenerator::zeroBoxPayloadField(const ir::IRValue& boxBase,
                                      const std::string& boxCanon,
                                      const SourceLocation& loc) {
    if (semantic_ == nullptr) return;
    const StructDecl* sd =
        semantic_->findStruct(canonicalizeSyntheticArgText(boxCanon))
            .value_or(nullptr);
    if (sd == nullptr) return;
    const cn::Optional<int> vo = semantic_->fieldOffsetOf(sd, "值");
    if (!vo.has_value()) return;
    ir::IRValue fieldAddr = vo.value() == 0
        ? boxBase
        : emitResult(ir::Opcode::FieldAddr, {boxBase}, "ptr",
                     std::to_string(vo.value()), loc);
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    emit(ir::Opcode::StorePtr, {fieldAddr, zero}, ir::IRValue(), "", "i64", loc);
}


} // namespace cn_compiler
