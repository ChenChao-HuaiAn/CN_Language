// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

namespace {
// 87-a（2026-09-12 第八十七轮）：字符字面量 raw（含单引号）→ 字符码文本。
//   95-a（2026-09-13 第九十五轮）：改走 单一归属 charLiteralCodePoint（转义 +
//   \u{XXXX} + UTF-8 全解码，规范 01b 三「4 字节 Unicode 标量值」）——原
//   「去引号取首字节」（'\n'=92 / '中'=228）违反规范且与 v2 侧分叉。
std::string charLiteralCodeText(const std::string& raw) {
    return std::to_string(charLiteralCodePoint(raw));
}
}  // namespace

void IRGenerator::visitProgram(Program* node) {
    // 第 9 层 Debug（P3-8）：登记顶层静态变量到 IRModule（codegen .data 段发射）。
    //   此前 IR 层不遍历 globals，函数体内引用全局静态名落入 FuncAddr 分支，
    //   未分配槽导致 rbp0 汇编错误（A2006）。初始值为字面量时求值存入
    //   globalStaticInits（codegen .data 段直接写初始值）。
    if (semantic_ != nullptr) {
        for (auto& g : node->globals) {
            if (g->isStatic && !g->name.empty()) {
                const std::string stType = semantic_->globalStaticType(g->name);
                if (!stType.empty()) {
                    module_->globalStatics[g->name] = stType;
                }
                const std::string canonStatic = types::canonical(stType);
                const bool isClassStatic =
                    !canonStatic.empty() && semantic_->isClassType(canonStatic);
                // 87-a（2026-09-12 第八十七轮）：结构体/字符串静态——初值不可
                //   .data 直存（结构体=多字段值语义须逐字段构造；字符串=拥有型
                //   须运行期驻留/落堆），统一经 staticCtorNames 在入口注入初始化
                //   （与类/容器静态 P3-8 机制同款；Rust 对照：非 const 构造的
                //   static 须 LazyLock 式运行期初始化）。
                const bool isStructStatic =
                    !canonStatic.empty() && semantic_->isStructType(canonStatic);
                const bool isStringStatic = (canonStatic == "字符串");
                if ((isStructStatic || isStringStatic) &&
                    g->initializer != nullptr) {
                    module_->staticCtorNames.push_back(g->name);

                    staticCtorInit_[g->name] = g->initializer.get();
                    continue;
                }
                if (isStructStatic || isStringStatic) {
                    // 无初值：结构体=.data 零（零值语义）；字符串=槽 0（空串
                    //   句柄，释放侧空安全）——两者零运行期开销，不注入。
                    continue;
                }
                if (g->initializer != nullptr) {
                    const NodeType it = g->initializer->getType();
                    if (it == NodeType::IntegerLiteral) {
                        module_->globalStaticInits[g->name] =
                            static_cast<IntegerLiteral*>(g->initializer.get())->raw;
                    } else if (it == NodeType::FloatLiteral) {
                        module_->globalStaticInits[g->name] =
                            static_cast<FloatLiteral*>(g->initializer.get())->raw;
                    } else if (it == NodeType::CharLiteral) {
                        // 87-a：字符静态初值（原缺此分支——初值恒 .quad 0 丢失）
                        module_->globalStaticInits[g->name] = charLiteralCodeText(
                            static_cast<CharLiteral*>(g->initializer.get())->raw);
                    } else if (it == NodeType::StringLiteral) {
                        module_->globalStaticInits[g->name] =
                            static_cast<StringLiteral*>(g->initializer.get())->raw;
                    } else if (it == NodeType::BoolLiteral) {
                        module_->globalStaticInits[g->name] =
                            static_cast<BoolLiteral*>(g->initializer.get())->value ? "1" : "0";
                    } else if (it == NodeType::CallExpr) {
                        // P3-8 补全（2026-08-30）：容器/类对象 静态（全局表 = 映射<...>()）——
                        //   .data 段只分配零字节，构造函数（桶数组=分配 等）须在 main 开头注入调用。
                        module_->staticCtorNames.push_back(g->name);

                        staticCtorInit_[g->name] = g->initializer.get();
                    } else if (isClassStatic) {
                        // 非字面量初始化表达式（如 向量 全局表 = 空向量 表达式）——零初始化 + 构造
                        module_->staticCtorNames.push_back(g->name);

                    } else {
                        // 87-a：其余表达式（标量非字面量初值，如 静态 整64 x = 取数()）——
                        //   原实现静默落 .data 零（初值丢失）；统一经入口注入求值一次。
                        module_->staticCtorNames.push_back(g->name);

                        staticCtorInit_[g->name] = g->initializer.get();
                    }
                } else if (isClassStatic) {
                    // P3-8 补全：无初始值的类类型静态（静态 映射<...> 模块表）——
                    //   .data 零对象无桶数组，main 注入无参构造（映射() 分配桶数组）。
                    module_->staticCtorNames.push_back(g->name);

                }
            }
        }
    }
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            visitFunctionDecl(static_cast<FunctionDecl*>(decl.get()));
        }
    }
    // 阶段3 OOP（Task 3.1）：类方法体提升。
    // 1. AST 顶层类（program->classes）逐个提升（visitClassDecl 只处理 node 对应类）
    // P2-16：AST 类键与 semantic classes() map 键一致（模块::类），保证 pass3 去重不重复发射
    std::unordered_set<std::string> astClassNames;
    for (auto& cls : node->classes) {
        astClassNames.insert(cls->moduleName.empty()
                                 ? cls->name
                                 : cls->moduleName + "::" + cls->name);
        visitClassDecl(cls.get());
    }
    // 2. 泛型内嵌类（GenericDecl.innerClass）：其类定义方法体须提升
    for (auto& gd : node->generics) {
        if (gd->innerClass != nullptr) {
            astClassNames.insert(gd->innerClass->moduleName.empty()
                                     ? gd->innerClass->name
                                     : gd->innerClass->moduleName + "::" + gd->innerClass->name);
            visitClassDecl(gd->innerClass.get());
        }
    }
    // 3. 泛型单态化实例化类（semantic classes()[名$实参]，不在 AST 顶层类列表）：
    //    实例化类符号含完整成员表（含继承并入），方法体 AST 沿用原泛型类定义。
    if (semantic_ != nullptr) {
        for (const auto& kv : semantic_->classes()) {
            if (astClassNames.count(kv.first) > 0) continue;  // 已由 AST 路径提升
            const ClassInfo& ci = kv.second;
            for (const auto& mk : ci.methods) {
                const ClassMemberInfo& mi = mk.second;
                if (!mi.hasBody) continue;
                emitClassMethod(ci.name, mi);
            }
        }
    }
    // 4. 泛型函数实例化（Task 6.1，26_generics 遗留限制打通）：
    //    语义层 visitCallExpr 对 名<类型>(实参) 调用登记 GenericFuncInstance
    //    （实例化名 + 原泛型声明 + 类型实参），此处为每个实例生成 IRFunction
    //    （类型参数 T -> 实参类型 替换，符号名 = 名$实参 与调用方一致）。
    if (semantic_ != nullptr) {
        for (const auto& gfi : semantic_->genericFuncInstances()) {
            emitGenericFuncInstance(gfi);
        }
    }
}

// 87-a（2026-09-12 第八十七轮）：顶层静态变量初始化注入（入口函数 entry 块）。
//   三类分派（性能第一、安全第二；Rust 对照）：
//     ① 结构体（初值=构造字面量）→ **原地逐字段构造**（emitStructInitTo：零临时
//        槽、零多余拷贝——Rust `static X: T = T { .. }` 运行期物化同构；负向：
//        若走「临时槽 + 整体拷贝」则多一次 memcpy）；
//     ② 结构体（初值=其他表达式）→ 整体拷贝（emitStructWholeAssign）：
//        调用返回=浅拷接管（retbuf 一次性物化槽，零拷贝）；标识符等=深拷
//        （字段级 __cn_str_copy 落堆，与局部结构体赋值位同款）；
//     ③ 字符串 → 求值 + **来源分级归一化**（normalizeStringValueSource：字面量=
//        驻留常量地址零分配；拥有返回=接管；借用来源=复制落堆）；
//     ④ 类/容器（P3-8 指针槽模型）→ NewObject + 构造 + StorePtr（原路径不变）。
//   preFree 恒假——入口首次初始化，.data 槽初始为零，无旧句柄可释放。
//   注意：多文件下仅入口模块（含 主 者）注入——与类/容器静态同限制（登记为
//     诚实边界，非本轮引入）。
void IRGenerator::emitStaticInitsAtEntry() {
    if (module_ == nullptr || semantic_ == nullptr) return;
    // 102-a（C4 核验）：`module_` 为**全量合并单模块**（宿主多文件=单 Program 模型，
    //   共享节点池/全局符号表）——本函数在 主 生成时遍历已覆盖**全部模块**的静态
    //   构造（v2 侧全树合并模型同款）。原 C4 登记「仅入口模块注入」在现架构下
    //   不成立（反证 B 实证：回退尝试仍通过）；**真实缺陷=H5**（见下 ④ 分支）。
    for (const auto& sname : module_->staticCtorNames) {
        const std::string stType = semantic_->globalStaticType(sname);
        const std::string canonStatic = types::canonical(stType);
        Expr* initExpr = nullptr;
        auto initIt = staticCtorInit_.find(sname);
        if (initIt != staticCtorInit_.end()) initExpr = initIt->second;
        // ①/② 结构体静态
        if (semantic_->isStructType(canonStatic)) {
            if (initExpr == nullptr) continue;  // .data 零 = 结构体零值
            const SourceLocation loc = initExpr->location;
            ir::IRValue symAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                             "?gstatic_" + sname, loc);
            if (initExpr->getType() == NodeType::StructInitExpr) {
                emitStructInitTo(static_cast<StructInitExpr*>(initExpr), symAddr,
                                 loc);
            } else if (initExpr->getType() == NodeType::CallExpr) {
                // 结构体返回调用：genExpr 物化返回（retbuf/临时槽），浅拷接管
                ir::IRValue src = genExpr(initExpr);
                emitStructCopyWithFields(symAddr, src, canonStatic, loc,
                                         /*preFree=*/false, /*deepCopy=*/false);
            } else if (!emitStructWholeAssign(symAddr, initExpr, canonStatic, loc,
                                              /*preFree=*/false)) {
                // 源形态未识别（语义层已诊断）：保持 .data 零（宁漏勿错）
            }
            continue;
        }
        // ③ 字符串静态
        if (canonStatic == "字符串") {
            if (initExpr == nullptr) continue;  // 槽 0 = 空串句柄（释放空安全）
            const SourceLocation loc = initExpr->location;
            ir::IRValue symAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                             "?gstatic_" + sname, loc);
            ir::IRValue val = genExpr(initExpr);
            ir::IRValue norm = normalizeStringValueSource(initExpr, val, loc);
            emit(ir::Opcode::StorePtr, {symAddr, norm}, ir::IRValue(), "", "ptr",
                 loc);
            continue;
        }
        // ④ 类/容器与标量静态（P3-8 指针槽模型 + 标量初值直存）
        ir::IRValue obj;
        const bool isScalarStatic =
            types::isInteger(canonStatic) || types::isFloat(canonStatic) ||
            canonStatic == "布尔" || canonStatic == "字符" ||
            types::isPointer(canonStatic) || semantic_->isEnumType(canonStatic);
        if (initExpr != nullptr) {
            // 有构造初始化表达式：genExpr(映射<...>()) -> NewObject + 构造调用
            obj = genExpr(initExpr);
            // 102-a（C4, 2026-09-13 第一百零二轮）**缺陷根治**：有初始化表达式
            //   路径补 **StorePtr**——原实现 `obj` 落尾端标量族判定
            //   （isScalarStatic=false）→ `continue` 跳过 StorePtr：对象已构造但
            //   指针**未写入 .data 静态槽**（槽=0）→ 读/方法调用空指针崩溃
            //   （探针 multi4：非入口模块容器静态 `静态 向量<整64> 表` 实证
            //   「初始大小=」后 运行时错误(错误码3)）。与「无初始化」路径同款
            //   指针槽模型（本缺陷对**任何模块**均存在——入口模块同形态亦崩）。
            // 110-a（2026-09-13 家机 win-x64）：102-a 版此处无条件 `continue`，
            //   使下方标量族段成为死代码——MSVC C4702 → /WX 构建失败（GCC 不报
            //   = 跨平台分叉温床），且丢失 isScalarStatic 守卫（数组等聚合会被
            //   8 字节 StorePtr 写越界 = 87-a 的安全边界）。恢复守卫：标量落下方
            //   统一 StorePtr；类/容器指针槽 StorePtr；其余聚合保持 .data 零。
            const bool obj有效 = !(obj.id < 0 && obj.isConstant == false &&
                                    obj.extra.empty());
            if (!isScalarStatic) {
                if (obj有效 && semantic_->isClassType(canonStatic)) {
                    ir::IRValue symAddr = emitResult(
                        ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + sname,
                        initExpr->location);
                    emit(ir::Opcode::StorePtr, {symAddr, obj}, ir::IRValue(), "",
                         "ptr", initExpr->location);
                }
                continue;  // 类/容器以外聚合（数组等）：保持 .data 零（宁漏勿错）
            }
            if (!obj有效) continue;
            // 标量：落入下方标量族统一 StorePtr
        } else {
            // 无初始化表达式：NewObject + 无参构造（this=新对象），
            //   StorePtr 指针入 .data 槽（指针槽模型，与局部类变量一致）
            if (!semantic_->isClassType(canonStatic)) continue;  // 非类静态无构造需求
            const ClassInfo* ci = semantic_->findClass(canonStatic);
            if (ci != nullptr) {
                const std::string extra =
                    canonStatic + "|" + std::to_string(ci->totalSize);
                ir::IRValue newObj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(canonStatic, "ptr")},
                    "ptr", extra, SourceLocation());
                const ClassMemberInfo* ctor = nullptr;
                for (const auto& mk : ci->methods) {
                    if (mk.second.isConstructor &&
                        mk.second.paramTypes.empty() &&
                        mk.second.ownerClass == canonStatic) {
                        ctor = &mk.second;
                        break;
                    }
                }
                if (ctor != nullptr) {
                    emit(ir::Opcode::Call, {newObj}, ir::IRValue(),
                         methodSymbolKey(canonStatic, ctor->sigKey), "void",
                         SourceLocation());
                }
                ir::IRValue symAddr = emitResult(
                    ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + sname,
                    SourceLocation());
                emit(ir::Opcode::StorePtr, {symAddr, newObj}, ir::IRValue(), "",
                     "ptr", SourceLocation());
            }
            continue;
        }
        // 标量族（整/浮/布/字符/指针/枚举）非字面量初值 → 入口求值 + StorePtr
        //   入 .data 槽；类/容器与数组等其他聚合已在上方分支 continue（宁漏勿错——
        //   8 字节 StorePtr 会把 >8 字节聚合写越界，安全优先）。
        // 存入 .data 符号（?gstatic_名）
        ir::IRValue symAddr = emitResult(
            ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + sname, SourceLocation());
        emit(ir::Opcode::StorePtr, {symAddr, obj}, ir::IRValue(), "", "ptr",
             SourceLocation());
    }
}

ir::IRModule IRGenerator::generate(Program* program) {
    ir::IRModule module;
    module_ = &module;
    regCounter_ = 0;
    blockCounter_ = 0;
    varCounter_ = 0;
    varStack_.clear();
    loopStack_.clear();
    switchStack_.clear();   // 72-b：复位补齐（原漏——防御跨 generate 残留）
    breakScopeSeq_ = 0;     // 72-b：中断绑定序（选择/循环压栈取号）每模块复位
    oopVarSrcTypes_.clear();
    currentClass_.clear();
    currentMethodStatic_ = false;
    currentMethodConst_ = false;
    visitProgram(program);
    module_ = nullptr;
    return module;
}
void IRGenerator::visitFunctionDecl(FunctionDecl* node) {
    if (node->body == nullptr) return;  // 函数原型声明：不生成IR函数（链接期缺失检测）
    ir::IRFunction func;
    // Task 2.10 重载：func.name 保持源码名（可读/测试契约）；
    //   mangledName 存签名 key（名#参数串），codegen 按此生成附录C符号。
    //   无参函数 sigKey 即纯名（mangledName==name，保持 主->cn_main 等映射）。
    // 第 4 层（v2.0 决策4/6，P2-6）：crate 前缀——跨模块同名函数链接符号加
    //   模块名$ 前缀（包A$函数 与 包B$函数 不冲突）。入口 主 与内置运行时符号
    //   （__cn_*）不加前缀（保持链接；主 -> cn_main 映射、内置走 __cn_ 路径）。
    func.name = node->name;
    std::string linkName = node->sigKey.empty() ? node->name : node->sigKey;
    // plans/018 呈报二 A′（2026-09-07 用户裁决）：定义侧链接键改调全编译器唯一
    //   公式 functionLinkKey（与注册侧 registerFunction 同源单一归属）——
    //   消灭旧「内联条件 vs 注册键顺序依赖」不对称（依赖+入口同名同签名 +
    //   入口纯名调用 → 链接 undefined reference 主$版本，base3 探针实证）。
    //   公式例外（主/空模块/主 函数/__cn_ 运行时 恒裸键）与 codegen
    //   symbolName 的 主->cn_main / __cn_ 直通映射对齐，注释详见 semantic.hpp。
    linkName = SemanticAnalyzer::functionLinkKey(node->moduleName, node->name,
                                                 linkName);
    func.mangledName = linkName;
    // P3-18 补完（2026-08）：引用返回（T&）——IR 返回类型映射为 指针（返回被引用
    //   左值的地址）；epilogue 走默认整型 rax 返回（无 structReturn/i128 特判）。
    const bool isRefReturnFn = !node->returnType.empty() &&
                               types::isReference(node->returnType);
    func.returnType = isRefReturnFn
                          ? "ptr"
                          : mapType(node->returnType.empty() ? "空类型"
                                                             : node->returnType);
    func.returnTypeSrc = node->returnType.empty() ? "空类型" : node->returnType;
    // Task 完善A：结构体返回值标记（返回类型为自定义结构体时走隐藏返回指针）
    // 修复（2026-08 自举检查发现）：结果/可选 返回同样走隐藏返回指针协议
    if (semantic_ != nullptr && !node->returnType.empty() &&
        semantic_->isStructType(types::canonical(node->returnType))) {
        // 结果/可选 返回同样走隐藏返回指针协议（Win x64 ABI）——IR 层对结构体
        //   返回调用预插 retbuf 地址为 operands[0]（形态A契约，见 ir_call.cpp），
        //   各后端原样传递自然落隐藏指针位（win=rcx / SysV=rdi / arm64=x0）；
        //   func.structReturn 标志供**被调方**生成接收/回写协议（paramOffset=1）。
        //   若走 __rctor 栈临时返回，调用方跨调用读 .值 悬垂（空类型结果调用处
        //   result.type=void 使仅按 result.type 的判定失效，2026-08 自举检查发现；
        //   2026-09-05 家机复核归真：win 第4路被调查询因 activeModule_ 恒 null
        //   从未生效，真正生效的一直是本处形态A预插，plans/016）
        func.structReturn = true;
        // 记录精确大小（字节）：epilogue 按此拷贝到隐藏返回缓冲区（避免 64 字节
        //   硬编码越界写破坏相邻栈变量——班级 16 字节被写 64 字节越界 48 字节）
        func.structReturnSize = semantic_->typeSizeOf(node->returnType);
    }
    // Task 完善A：提前绑定 function_，使 registerVarSlots（参数多槽登记）
    //   在参数循环内即可生效（此前 function_ 在循环后设置，结构体参数
    //   的 varSlots 登记被 registerVarSlots 的 function_==nullptr 检查跳过）
    function_ = &func;
    // 参数进入最外层作用域，生成唯一内部名（name$N）。
    // params 保留源码名（对外可读/测试契约），paramUniques 存唯一名，
    // 代码生成层按 paramUniques 登记/查询栈槽，保证遮蔽变量各自独立槽
    varStack_.emplace_back();
    for (std::size_t pi = 0; pi < node->params.size(); ++pi) {
        auto& param = node->params[pi];
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        // A-1（引用参数 整32& / 账户& / T&，2026-08 修复）：参数槽存"被引用左值
        //   的地址"（8 字节），体内读取/赋值经 byRef 机制解引用（复用 lambda
        //   [&] 捕获的成熟路径：读 LoadPtr、写 StorePtr、取地址 Load 槽）
        const bool isRefParam = !param->funcPtr.isFunctionPtr() &&
                                types::isReference(param->typeName);
        // Task 2.2：函数指针参数（整32(*func)(整32, 整32)）类型为 ptr
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(param->typeName);
        if (isRefParam) paramIrType = "ptr";  // 引用参数按地址传递（槽存地址）
        // Task 2.4：数组参数按多槽登记（varSlots）；引用参数仅 1 槽（存地址）
        registerVarSlots(unique, isRefParam ? ""
                            : (param->funcPtr.isFunctionPtr() ? "" : param->typeName));
        // Task 完善A：结构体按值参数标记（语义层查询——结构体源码类型）；
        //   引用参数（账户&）按地址传递（非按值结构体拷贝），须排除
        if (!isRefParam && semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
            semantic_->isStructType(types::canonical(param->typeName))) {
            func.structParamIndexes.insert(static_cast<int>(pi));
        }
        func.params.emplace_back(param->name, paramIrType);
        func.paramUniques.push_back(unique);
        ir::IRValue reg = newReg();
        reg.type = paramIrType;
        VarEntry entryInfo;
        entryInfo.regId = reg.id;
        entryInfo.uniqueName = unique;
        // 体内"值类型"仍为被引用基础类型：读取经 byRef 解引用（LoadPtr）返回
        // 基础类型值（与 lambda [&] 捕获 entry.type 规则一致，lambda 处注释同源）
        entryInfo.type = isRefParam ? mapType(types::stripRef(param->typeName))
                                    : reg.type;
        entryInfo.srcType = param->typeName;  // 指针/数组复合类型源码名
        entryInfo.byRef = isRefParam;         // A-1：引用参数按 byRef 语义读写
        varStack_.back()[param->name] = entryInfo;
    }
    // Task 2.10：收集尾部默认参数值（签名 key -> 默认值 IR 常量列表）。
    // 调用补全时按 名#参数串 查此表，把缺省实参精确展开为默认值常量。
    // 默认值表达式须为编译期常量（整/浮/字符串/布尔/字符字面量、一元负号）。
    //   常量求值：字面量 -> ConstInt/ConstFloat/ConstString/ConstBool；
    //   其他形态（一元负号 -1）在下方 evalConstExpr 中处理。
    {
        std::vector<ir::IRValue> defaults;
        for (auto& param : node->params) {
            if (param->hasDefault && param->defaultExpr != nullptr) {
                defaults.push_back(evalDefaultExpr(param->defaultExpr.get(), func));
            }
        }
        if (!defaults.empty()) {
            // 用 mangledName（签名 key 名#参数串）作 key——调用方按
            // node->resolvedSignature（同 sigKey）查找补全
            funcDefaultArgs_[func.mangledName] = defaults;
        }
    }
    // 注意：varCounter_ 不可重置！参数已用 varCounter_ 生成唯一名，
    // 若重置则函数体内同名遮蔽变量会生成相同唯一名（如 x$0）导致槽冲突
    // 栈帧膨胀根治（2026-09-08 v2self 锚定轮，探针实证）：regCounter_ 与
    //   blockCounter_ 同点每函数复位——虚拟寄存器是函数内 SSA 值（跨函数仅经
    //   符号名引用），原全模块递增使 computeFrameSize 按 maxRegId 定帧时后段
    //   函数帧线性膨胀（20 函数探针 80B→1152B；v2self 18 万行后段函数帧达
    //   290KB，8MB 栈深递归解析 SIGSEGV——三后端共用 IR 层同源受益）。
    //   标签唯一性由函数名前缀保证（codegen currentBlockPrefix_），
    //   块号复位既有先例同构。
    regCounter_ = 0;
    blockCounter_ = 0;
    // 入口基本块（ASCII标签 bbN：ml64 不识别中文标识符，阶段一统一 ASCII）
    ir::IRBlock* entry = newBlock("bb0");
    // P3-8 补全（2026-08-30）+ 宿主根治（2026-09-01）：顶层静态构造初始化——
    //   类/容器静态统一「指针槽模型」：.data 符号存 8 字节对象指针（与局部类
    //   变量槽同构），主 入口注入 NewObject + 无参构造 + StorePtr 指针入槽。
    //   原实现按「有无初始化表达式」分裂两种模型：有初始化 = NewObject 后
    //   StorePtr 指针入槽；无初始化 = 构造函数打在 .data 符号地址（对象内联
    //   本体）。读取路径（LoadPtr）只对指针模型正确——无初始化静态被读出
    //   首 8 字节字段（如 数据 指针）当对象指针，复制/方法调用全错（实测
    //   运行时错误3 空指针）。统一后标识符读、方法 this、拷贝构造 byRef 传参
    //   （槽地址解引用即对象指针）全部与局部类变量一致。
    if (node->name == "主" && module_ != nullptr) {
        emitStaticInitsAtEntry();
    }
    // 函数体
    if (node->body != nullptr) {
        genBlock(node->body.get());
    }
    // 无终止指令：补充默认返回（空类型函数 / 原型声明）
    if (!function_->blocks.empty()) {
        ir::IRBlock* last = function_->blocks.back().get();
        if (!last->terminated) {
            setCurrentBlock(last);
            endReturn("");
        }
    } else if (node->body == nullptr) {
        // 原型声明：生成空入口块 + 默认返回
        setCurrentBlock(entry);
        endReturn("");
    }
    // 阶段3 OOP（Task 3.1）：类类型局部变量（有析构函数）函数收尾 DeleteObject（RAII）。
    // 注意：必须在块终止补齐后调用（genClassDestructorCalls 在最后一个未终止块
    //   末尾插入 DeleteObject；若函数已有返回则不插入，避免破坏既有终止）
    genClassDestructorCalls();
    genStringFrees();
    stringTainted_.clear();
    // 72-a（2026-09-11 第七十二轮）：块级作用域名单复位（函数级状态——下一函数干净）
    ownedStringOrder_.clear();
    ownedClassOrder_.clear();
    ownedFieldOrder_.clear();
    scopeStringBase_.clear();
    scopeClassBase_.clear();
    scopeFieldBase_.clear();
    module_->functions.push_back(std::move(func));
    function_ = nullptr;
    // 修复（2026-08 自举前置 A-3a 发现）：弹出参数作用域——原实现漏 pop，
    //   每函数泄漏一层 varStack_，后续函数体内查找命中前函数的同名参数
    //   （如 总和(向量<整64> 数据) 的 数据 泄漏），类方法体内直接字段访问
    //   （向量.追加 的 数据）被误判为局部变量 -> 无槽 rbp0（A2006）实测。
    //   emitGenericFuncInstance/emitClassMethod 均已 pop，此处对齐。
    if (!varStack_.empty()) varStack_.pop_back();
}
void IRGenerator::visitParamDecl(ParamDecl* node) {
    // 参数由 visitFunctionDecl 统一处理
    (void)node;
}
void IRGenerator::visitVarDecl(VarDecl* node) {
    genVarDecl(node);
}
void IRGenerator::visitStructDecl(StructDecl* node) {
    (void)node;
}
void IRGenerator::visitEnumDecl(EnumDecl* node) {
    (void)node;
}
} // namespace cn_compiler
