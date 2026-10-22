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
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/model/type_system.hpp"

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
            // 569 甲案（2026-10-14 用户裁决·001 §5.2a）：条件放宽——顶层裸全局
            //   （!isStatic 且非常量）与顶层静态同通道发射存储（.data/.bss +
            //   初始化器）；三后端消费 module.globalStatics/globalStaticInits
            //   零改动。语义层已同表登记（globalStaticType 可查）。
            if (!g->isConst && !g->name.empty()) {
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
                if (!g->isStatic && !g->constInitText.empty()) {
                    // 569 d 款（001 §5.2a）：裸全局常量初始化器直存——语义层
                    //   cnEvalConstExpr 折叠文本（覆盖常量引用/常量算术等非字面量
                    //   节点形态·否则落下方「其他表达式」分支=运行期求值，违反
                    //   constinit 常量初始化意图；字面量节点文本同口径。布尔已转
                    //   1/0·字符已转码点·i128 文本由发射层 parseInt128InitText
                    //   解析。字符串/结构体形态已被上方 isStructStatic/
                    //   isStringStatic 分支分流至入口注入，不达此处）。
                    module_->globalStaticInits[g->name] = g->constInitText;
                } else if (g->initializer != nullptr) {
                    const NodeType it = g->initializer->getType();
                    if (it == NodeType::IntegerLiteral) {
                        // 478（473 采样轮）：顶层静态字面量补 128 位域检查——
                        //   原直存通道绕过 visitIntegerLiteral 域检查，无后缀
                        //   u128::MAX 赋 正128 漏网放行（483 实锚），与 540
                        //   「双侧同拒」裁决（001 §4.3 后缀映射）矛盾。
                        // 910：判定收口 types::integerLiteralRuntimeType（全形态
                        //   归一——原 textExceedsInt64/i128TextInRange 十进制文本
                        //   判据对 hex 面误判：`0x8000000000000000` 长 18 判不超/
                        //   hex 文本比较失真；759 探针实锚）。
                        const auto* lit =
                            static_cast<IntegerLiteral*>(g->initializer.get());
                        int st910 = 0;
                        (void)types::integerLiteralRuntimeType(lit->raw, &st910);
                        if (st910 != 0) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, g->location,
                                "整数字面量超出" +
                                    std::string(st910 == 2
                                                    ? "正128（2^128-1）"
                                                    : "整128（2^127-1）") +
                                    "范围");
                        }
                        // 790（569 采样轮 H1）：字面量数值化直存——原 lit->raw
                        //   原文透传（0x/0b/0o/L 后缀四形态 .data 直存被三平台
                        //   汇编器拒绝：win MASM dq 0xFF A2206 / dq 111111111111L
                        //   A2048·linux GAS .quad ...L 同病）。十进制规范文本为
                        //   三后端通用形态（常量算术折叠输出同口径）；超 u64 域
                        //   文本（128 位字面量）保真原文交 128 位双 quad 通道
                        //   parseInt128InitText 解析（原生支持四形态·T46）。
                        std::string initLitText;
                        if (!types::foldIntLiteralText(lit->raw, initLitText)) {
                            initLitText = types::stripLiteralSuffix(lit->raw);
                        }
                        module_->globalStaticInits[g->name] = initLitText;
                    } else if (it == NodeType::UnaryExpr &&
                               types::isInt128Type(canonStatic)) {
                        // T46（467-a）缺口①（IR 侧·Rust const-eval 直存 .data
                        //   同构）：128 位标量的 ±字面量折叠进直存通道（文本=
                        //   符号×raw·发射层 parseInt128InitText 解析双 quad）——
                        //   原落 staticCtor 运行时通道且 StorePtr 类型误标
                        //   "ptr" 半槽写（静态 整128 b = -5; 实测读出 2^64−5）。
                        //   仅 128 位走新折叠（≤64 位现状 StorePtr 写 8 字节
                        //   正确·零回归面）。
                        const Expr* folded = g->initializer.get();
                        int sign = 1;
                        while (folded->getType() == NodeType::UnaryExpr) {
                            const auto* un = static_cast<const UnaryExpr*>(folded);
                            if (un->postfix) { sign = 0; break; }  // i-- 等非纯符号
                            if (un->op == Operator::Subtract) {
                                sign = -sign;
                            } else if (un->op != Operator::Add) {
                                sign = 0;  // 非 ± 符号链（! ~ 等）：不折叠
                                break;
                            }
                            folded = un->operand.get();
                        }
                        if (sign != 0 &&
                            folded->getType() == NodeType::IntegerLiteral) {
                            const std::string foldedText =
                                std::string(sign < 0 ? "-" : "") +
                                static_cast<const IntegerLiteral*>(folded)->raw;
                            // 478（473 采样轮）：±字面量链同补域检查（原超域
                            //   静默落零占位——parseInt128InitText 域判定失败
                            //   ok=false 无诊断）。符号域按目标类型（与发射层
                            //   判定同源 isInt128Signed）——负字面量赋 正128
                            //   =域外拒绝；负界 −2^127 合法。
                            const bool tgtSigned =
                                types::isInt128Signed(canonStatic);
                            if (!types::i128TextInRange(foldedText,
                                                        tgtSigned)) {
                                diagnostics_.report(
                                    DiagnosticLevel::Error, g->location,
                                    "整数字面量超出" +
                                        std::string(tgtSigned
                                                        ? "整128（2^127-1）"
                                                        : "正128（2^128-1）") +
                                        "范围");
                            }
                            module_->globalStaticInits[g->name] = foldedText;
                        } else {
                            // 非 ±字面量链（调用等运行期初值）：保持 staticCtor
                            //   入口注入通道（注入处 StorePtr 对 128 位传 IR 名）
                            module_->staticCtorNames.push_back(g->name);
                            staticCtorInit_[g->name] = g->initializer.get();
                        }
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
    // D23 根治（248-a）：类构造默认值**预收集**——类方法体提升（下方步骤 2）
    //   晚于本函数的 FunctionDecl 循环（主函数生成即调用点查表），生成期收集
    //   会 miss；此处先于全部函数生成对全部类（AST+泛型实例）预收集，
    //   emitClassMethod 内同款收集保留（幂等跳过）作兜底。
    if (semantic_ != nullptr) {
        for (const auto& kv : semantic_->classes()) {
            for (const auto& mk : kv.second.methods) {
                collectCtorDefaults(kv.second.name, mk.second);
            }
        }
    }
    // 762 根治：自由函数默认参数值**预收集**——原收集点在 visitFunctionDecl
    //   （按声明顺序单遍发射时），同文件后置定义（001 两遍编译立法合法形态）
    //   的带默认参数函数，其调用点（在先发射）查 funcDefaultArgs_ miss →
    //   缺省实参静默不补=静默错值（探针实录：带缺省() 应 6 静默返 0·
    //   前置定义即愈=570 轮三案实录同机制）。D23 类构造预收集同款根治：
    //   先于全部函数生成对全部顶层有体函数预收集；visitFunctionDecl 内原
    //   收集保留（幂等覆盖同 key）作兜底。
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            auto* fd = static_cast<FunctionDecl*>(decl.get());
            collectFreeFunctionDefaults(fd);
            // 880（980 红治理 v2）：函数声明表预收集——registerOopRaii 判
            //   调用接收别名的返回来源面（见 ir_generator.hpp 成员注释）。
            if (!fd->name.empty()) freeFuncDecls880_[fd->name] = fd;
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
    //    317-a（T19②）：改为不动点循环——实例体生成前的重放检查
    //    （recheckGenericFuncBody）会触发体内嵌套泛型调用注册新 GFI（推断
    //    形态），range-for 快照遍历会漏掉新实例且迭代器失效。
    if (semantic_ != nullptr) {
        const std::vector<GenericFuncInstance>& all =
            semantic_->genericFuncInstances();
        std::size_t gfiIdx = 0;
        while (gfiIdx < all.size()) {
            // 317-a：值拷贝传入——recheck（体内嵌套实例化注册）会向
            // genericFuncInstances_ 追加元素，vector 扩容搬家使 all[idx]
            // 与按引用收参的 gfi 悬垂（段错误 0xC0000005 实锤）。
            emitGenericFuncInstance(GenericFuncInstance(all[gfiIdx]));
            ++gfiIdx;
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
                // 061-c（2026-09-27 804 轮）：顶层静态初值 内置构造器 上下文类型——
                //   正常(5)/错误(码)/某些(v) 的脱糖（handleResultCtor）依赖
                //   node->resolvedType；局部变量声明由语义层推导赋值，顶层静态
                //   初值无此推导 → resolvedType 空 + 主函数 returnTypeSrc=整32
                //   → 脱糖放弃退化普通外部调用（链接 undefined reference to
                //   _E6ADA3E5B8B8 实测）。此处按静态类型补上下文（与语义层
                //   visitVarDecl 推导同效），脱糖产物=临时合成体槽 → 下方
                //   emitStructCopyWithFields 写入静态槽。
                {
                    CallExpr* initCall = static_cast<CallExpr*>(initExpr);
                    if (initCall->callee->getType() == NodeType::IdentifierExpr &&
                        initCall->resolvedType.empty()) {
                        const std::string calleeName =
                            static_cast<IdentifierExpr*>(initCall->callee.get())
                                ->name;
                        if (calleeName == "正常" || calleeName == "错误" ||
                            calleeName == "某些") {
                            initCall->resolvedType = stType;
                        }
                    }
                }
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
        // 040（001 §5.8 甲案·2026-10-05）：函数指针静态槽=标量族（8 字节指针槽，
        //   初值=函数地址入口注入 StorePtr——原落「其他聚合保持 .data 零」分支
        //   =初值静默丢失·probe082/g2 实证面）
        const bool isScalarStatic =
            types::isInteger(canonStatic) || types::isFloat(canonStatic) ||
            canonStatic == "布尔" || canonStatic == "字符" ||
            types::isPointer(canonStatic) || semantic_->isEnumType(canonStatic) ||
            isFuncPtrTypeStr(canonStatic);
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
            const bool objValid = !(obj.id < 0 && obj.isConstant == false &&
                                    obj.extra.empty());
            if (!isScalarStatic) {
                if (objValid && semantic_->isClassType(canonStatic)) {
                    ir::IRValue symAddr = emitResult(
                        ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + sname,
                        initExpr->location);
                    emit(ir::Opcode::StorePtr, {symAddr, obj}, ir::IRValue(), "",
                         "ptr", initExpr->location);
                }
                continue;  // 类/容器以外聚合（数组等）：保持 .data 零（宁漏勿错）
            }
            if (!objValid) continue;
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
        // T46（467-a）缺口②：128 位标量 StorePtr 类型须传 IR 名——原恒传
        //   "ptr" 使三后端落 64 位存储分支只写低 8 字节（半槽写：静态 整128
        //   b = -5; 实测读出 2^64−5）。三后端 StorePtr 的 i128 双槽写分支已在
        //   位（结构体字段写 i128 引入），此处对齐触达；≤64 位标量保持 "ptr"
        //   （槽 8 字节·零回归面）。常量源由后端 i128 分支的常量子分支直写。
        std::string storeType = "ptr";
        if (types::isInt128Type(canonStatic)) {
            storeType = types::isInt128Signed(canonStatic) ? "i128" : "u128";
        }
        // 919（超承载字面量参与表达式·求值链收口）：表达式值为 128 域承载
        //   （i128/u128——638 甲案「无后缀大字面量提升链」表达式面归整128）
        //   而槽为 ≤64 位整数时，原直接 StorePtr 类型错配（8 字节槽 + i128
        //   值）→ 后端落 8 字节存储分支读值高半（探针实录：`静态 整64 甲 =
        //   99999999999999999999 + 0;` 出 5＝高位半字，应为
        //   7766279631452241919＝低 64 位截断；19 位形态出 0）。修法=与局部
        //   声明初始化（genExprInitializer）同款：值类型≠槽类型时插 Cast
        //   （§3.7a ④ 128→64 表达式源窄化特例＝低 N 位截断；128 槽
        //   i128/u128 直存路径不受影响）。
        if ((obj.type == "i128" || obj.type == "u128") &&
            !types::isInt128Type(canonStatic) && types::isInteger(canonStatic)) {
            const std::string slotIr919 = mapType(canonStatic);
            if (!slotIr919.empty() && slotIr919 != obj.type) {
                obj = emitResult(ir::Opcode::Cast, {obj}, slotIr919, "",
                                 initExpr != nullptr ? initExpr->location
                                                     : SourceLocation());
            }
        }
        emit(ir::Opcode::StorePtr, {symAddr, obj}, ir::IRValue(), "", storeType,
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
    // 485：闭包关联表按函数上下文清空——closureInfo_ 键=变量源码名（无函数
    //   作用域），跨函数同名变量误命中快径直调 ?lambdaN（捕获实参缺失=输出
    //   垃圾/崩溃·742a 拆面实录：造值 内「取」登记后 主 内同名「取」取()
    //   被展开为无捕获实参的直调）。闭包变量本就是函数局部概念，入口清空
    //   即隔离；lambda 体上下文由 visitLambdaExpr 入口同样清空。
    closureInfo_.clear();
    // 482：返回来源跟踪按函数复位（上一函数残留零泄漏；lambda 体由
    //   visitLambdaExpr save/restore 另行隔离）。
    currentReturnOrigin482_.clear();
    carrierAssigned482_.clear();
    ir::IRFunction func;
    // 191-a：本函数 171 行按「注册流水线」提取为 2 个族子方法（纯搬运零行为
    //   变更——多重集核验先行于构建）：参数登记+默认参数收集 / RAII 收尾+入列。
    if (node->body == nullptr) return;  // 函数原型声明：不生成IR函数（链接期缺失检测）
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
    linkName = functionLinkKey(node->moduleName, node->name,
                                                 linkName);
    func.mangledName = linkName;
    // 482：当前函数签名键登记（返回来源表收尾写入用——键与调用点
    //   resolvedSignature 同源「名#参数串」）。
    currentSigKey482_ = linkName;
    // P3-18 补完（2026-08）：引用返回（T&）——IR 返回类型映射为 指针（返回被引用
    //   左值的地址）；epilogue 走默认整型 rax 返回（无 structReturn/i128 特判）。
    const bool isRefReturnFn = !node->returnType.empty() &&
                               types::isReference(node->returnType);
    func.returnType = isRefReturnFn
                          ? "ptr"
                          : mapType(node->returnType.empty() ? "空类型"
                                                             : node->returnType);
    func.returnTypeSrc = node->returnType.empty() ? "空类型" : node->returnType;
    // 任务 474：不安全 修饰传入 IR 函数（发射层溢出检查豁免判据·判空/越界同源纪律）
    func.isUnsafe = node->isUnsafe;
    // Task 完善A：结构体返回值标记（返回类型为自定义结构体时走隐藏返回指针）
    // 修复（2026-08 自举检查发现）：结果/可选 返回同样走隐藏返回指针协议
    if (semantic_ != nullptr && !node->returnType.empty() &&
        (semantic_->isStructType(types::canonical(node->returnType)) ||
         types::isArray(node->returnType))) {
        // 结果/可选 返回同样走隐藏返回指针协议（Win x64 ABI）——IR 层对结构体
        //   返回调用预插 retbuf 地址为 operands[0]（形态A契约，见 ir_call.cpp），
        //   各后端原样传递自然落隐藏指针位（win=rcx / SysV=rdi / arm64=x0）；
        //   func.structReturn 标志供**被调方**生成接收/回写协议（paramOffset=1）。
        //   若走 __rctor 栈临时返回，调用方跨调用读 .值 悬垂（空类型结果调用处
        //   result.type=void 使仅按 result.type 的判定失效，2026-08 自举检查发现；
        //   2026-09-05 家机复核归真：win 第4路被调查询因 activeModule_ 恒 null
        //   从未生效，真正生效的一直是本处形态A预插，plans/016）
        //   087（m85 返回面）：定长数组返回（整32[3]）同通道——原 mapType 塌缩
        //   i32（8B 返回槽装 12B 数组=静默截断垃圾）·sret 隐藏指针+被调 epilogue
        //   按 structReturnSize 拷贝（001 §3.5 数组值语义·Rust [T;N] 返回
        //   memcpy 同构）
        func.structReturn = true;
        // 记录精确大小（字节）：epilogue 按此拷贝到隐藏返回缓冲区（避免 64 字节
        //   硬编码越界写破坏相邻栈变量——班级 16 字节被写 64 字节越界 48 字节）
        func.structReturnSize = semantic_->typeSizeOf(node->returnType);
    }
    // Task 完善A：提前绑定 function_，使 registerVarSlots（参数多槽登记）
    //   在参数循环内即可生效（此前 function_ 在循环后设置，结构体参数
    //   的 varSlots 登记被 registerVarSlots 的 function_==nullptr 检查跳过）
    function_ = &func;
    // 参数登记（参数循环+varStack_ 压栈+默认参数收集）
    registerFunctionParams(node, func);
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
    // 102 甲案（2026-09-29 用户裁决·〔基准=019〕）：**消耗形参 prologue 深拷**
    //   ——字符串值形参被语义层标记 ownedConsumed（体内赋值/转移()/字符串释放）
    //   时，入口把借用句柄经 __cn_str_copy 落堆为独立拥有副本（090 丙案
    //   「默认=值副本」兑现：被调方重赋值/出口释放只作用于自身副本，不再释放
    //   调用方缓冲区——885 实证 132 次重复释放面归零）。
    //   第四句拷贝消除：非消耗形参直传零拷贝（字符串无原地写·只读借用别名
    //   不可观察——语义层 checkIdentifierAssignTarget/checkTransferCall 判据）。
    //   注册 ownedStringOrder_＝出口 RAII 释放自身副本（重赋值释放旧值同理正确）。
    //   644 rebase 收口（2026-10-15）：644 分支曾在此平行登记 oopVarSrcTypes_
    //   （消费面=genStringFrees 形参扫描段·无清零）——develop 616 版以
    //   ownedParamCopySlots_ 名单承担同责（free+清零幂等·普通/方法/泛型三面），
    //   按「develop 已有等价修复以 develop 为准」弃 644 登记（其唯一消费段
    //   已弃·模块级表留无消费死数据=账面分裂源）。
    for (std::size_t pi = 0; pi < node->params.size() && pi < func.paramUniques.size();
         ++pi) {
        const auto& param = node->params[pi];
        if (!param->ownedConsumed) continue;
        const std::string& unique = func.paramUniques[pi];
        ir::IRValue cur = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(unique, "ptr")}, "ptr",
                                     unique, node->location);
        ir::IRValue cp = emitResult(ir::Opcode::Call, {cur}, "ptr",
                                    "__cn_str_copy", node->location);
        emit(ir::Opcode::Store, {cp}, ir::IRValue(), unique, "ptr",
             node->location);
        ownedStringOrder_.push_back(unique);
        // 616（2026-10-14 伞批）：同步登记 prologue 副本槽名单——genStringFrees
        //   返回块按名单注入出口释放（free+清零幂等）。原只登记 ownedStringOrder_
        //   而形参在函数体块基线内（块出口不管）+槽无 Alloca（函数级扫描看不到）
        //   =副本终身无人释放（每次调用净漏 1）。
        ownedParamCopySlots_.push_back(unique);
    }
    // P3-8 补全（2026-08-30）+ 宿主根治（2026-09-01）：顶层静态构造初始化——
    //   类/容器静态统一「指针槽模型」：.data 符号存 8 字节对象指针（与局部类
    //   变量槽同构），主 入口注入 NewObject + 无参构造 + StorePtr 指针入槽。
    //   原实现按「有无初始化表达式」分裂两种模型：有初始化 = NewObject 后
    //   StorePtr 指针入槽；无初始化 = 构造函数打在 .data 符号地址（对象内联
    //   本体）。读取路径（LoadPtr）只对指针模型正确——无初始化静态被读出
    //   首 8 字节字段（如 数据/容器指针）当对象指针，复制/方法调用全错（实测
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
    // 482：本函数返回来源收尾入表（唯一已知来源才登记——冲突/未知哨兵 "?"
    //   与空=不登记=未知；接收点 resolveLambdaOrigin482 消费）。
    if (!currentSigKey482_.empty() && !currentReturnOrigin482_.empty() &&
        currentReturnOrigin482_ != "?") {
        funcReturnLambda482_[currentSigKey482_] = currentReturnOrigin482_;
    }
    // RAII 收尾（析构 DeleteObject/串释放/名单复位）+ IR 函数入列 + 作用域弹出
    finishFunctionEmit(func);
}

// ==================== 191-a 流水线族子方法（原 visitFunctionDecl 361~480 段） ====================

// 族①：参数登记（原 361~418 段）——参数进入最外层作用域（varStack_ 压栈，
//   与 finishFunctionEmit 的 pop 配对），生成唯一内部名（name$N）；引用参数
//   按地址传递（byRef 机制）；结构体按值参数标记；默认参数值收集
//   （funcDefaultArgs_，签名 key -> 默认值 IR 常量列表）。
void IRGenerator::registerFunctionParams(FunctionDecl* node, ir::IRFunction& func) {
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
        // 087（m85 族·数组值语义 ABI）：定长数组形参（整32[4] a）=按值聚合——
        //   复用 056 结构体按值通道（调用点传源数组地址·被调 emitParamSetup 从
        //   指针 rep movsb 拷贝 varSlots*8 字节到本地多槽·体内 a[i] 按本地数组
        //   模型读本地拷贝）。原 mapType 塌缩 i32（签名登记 4B 槽）+调用点退化
        //   传指针+被调按本地数组无解引用=三处错位读栈垃圾（p1004_02 sum=
        //   13237248 应 10 实录·O0=O3 同错）。001 §3.5 数组=值聚合（赋值拷贝
        //   语义）·Rust [T;N] 按值=memcpy 同构。
        const bool isArrayParam = !param->funcPtr.isFunctionPtr() &&
                                  types::isArray(param->typeName);
        // Task 2.2：函数指针参数（整32(*func)(整32, 整32)）类型为 ptr
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(param->typeName);
        if (isRefParam || isArrayParam) paramIrType = "ptr";  // 引用/数组按值参数按地址传递（槽存地址）
        // Task 2.4：数组参数按多槽登记（varSlots）；引用参数仅 1 槽（存地址）
        registerVarSlots(unique, isRefParam ? ""
                            : (param->funcPtr.isFunctionPtr() ? "" : param->typeName));
        // 031（#1075）：绑定方法值形参——源码级 1 个（语义层 arity/调用检查零
        //   变更），物理级摊平为两个连续形参位（this、code）与调用方两槽实参
        //   两位展开对位（〔一值=两位〕单点·plans/031 §二·⑤；codegen prologue
        //   按位收寄存器/栈参——三后端零协议新增）。物理起始位=当前 params 数
        //   （后续 structParamIndexes 须按物理位登记防偏移）。
        const bool isMvParam =
            !isRefParam && !param->funcPtr.isFunctionPtr() &&
            !param->typeName.empty() &&
            types::isMethodValueType(types::canonical(param->typeName));
        const int physBase = static_cast<int>(func.params.size());
        // Task 完善A：结构体按值参数标记（语义层查询——结构体源码类型）；
        //   引用参数（账户&）按地址传递（非按值结构体拷贝），须排除
        //   087：数组按值参数同通道标记（emitParamSetup 拷贝判定同款）
        if (!isRefParam && semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
            (semantic_->isStructType(types::canonical(param->typeName)) ||
             isArrayParam)) {
            func.structParamIndexes.insert(physBase);
        }
        func.params.emplace_back(param->name, paramIrType);
        func.paramUniques.push_back(unique);
        if (isMvParam) {
            // 二槽物理形参：code 位（$s1 命名与 varSlots 多槽登记同规）
            func.params.emplace_back(param->name + "$s1", "ptr");
            func.paramUniques.push_back(unique + "$s1");
        }
        ir::IRValue reg = newReg();
        reg.type = paramIrType;
        VarEntry entryInfo;
        entryInfo.regId = reg.id;
        entryInfo.uniqueName = unique;
        // 体内"值类型"仍为被引用基础类型：读取经 byRef 解引用（LoadPtr）返回
        // 基础类型值（与 lambda [&] 捕获 entry.type 规则一致，lambda 处注释同源）
        entryInfo.type = isRefParam ? mapType(types::stripRef(param->typeName))
                                    : reg.type;
        // 337-a（T53 家系）：函数指针参数登记完整规范串（`函数指针<返回>(参数,...)`）
        //   ——与局部变量声明位同口径（funcPtrParamsOfCallee 的形参来源）；原登记
        //   param->typeName（C 风格声明 `整32(*f)(整128)` 在 funcPtr 形态下为空）
        //   → 间接调用实参宽化无据可判（同 ir_stmt_decl.cpp 注释）。
        //   泛型实例体：T→实参类型替换（`函数指针<整32>(T,T)` → `...<整32>(整128,整128)`）
        //   ——否则实例体内间接调用解析出形参 "T"、128 位判定落空（探针 n 实证：
        //   泛型实例体 应用$整128 的形参未替换，字面量实参仍窄传）。
        entryInfo.srcType = substGenericType(paramSrcTypeOf(*param.get()));
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
                // 1150：按形参声明类型落位（128 形参的 <=64 位默认值须拆双半）
                defaults.push_back(
                    evalDefaultExpr(param->defaultExpr.get(), param->typeName));
            }
        }
        if (!defaults.empty()) {
            // 用 mangledName（签名 key 名#参数串）作 key——调用方按
            // node->resolvedSignature（同 sigKey）查找补全
            funcDefaultArgs_[func.mangledName] = defaults;
            // D23（248-a）：同步登记参数总数（补缺点精确判定缺省个数，
            //   防回退口径把显式传满参的调用误补）
            funcDefaultTotal_[func.mangledName] = node->params.size();
        }
    }
}

// 762：自由函数默认参数值预收集（visitProgram 在全部函数生成前调用）——
//   与 registerFunctionParams 尾段同款收集（默认值常量求值+参数总数登记），
//   key=定义侧链接键（functionLinkKey 与 visitFunctionDecl 的 func.mangledName
//   同源公式·语义层已先跑，node->sigKey 必已填充）。原型声明（body==nullptr）
//   不收集——原型不发射 IR 函数，与 visitFunctionDecl 的跳过口径一致；有体
//   函数无条件收集（无默认值参数时 defaults 空=不登记，零行为差异）。
void IRGenerator::collectFreeFunctionDefaults(FunctionDecl* node) {
    if (node->body == nullptr) return;  // 函数原型声明：不发射（同 visitFunctionDecl）
    std::vector<ir::IRValue> defaults;
    for (auto& param : node->params) {
        if (param->hasDefault && param->defaultExpr != nullptr) {
            // 1150：按形参声明类型落位（128 形参的 <=64 位默认值须拆双半）
            defaults.push_back(
                evalDefaultExpr(param->defaultExpr.get(), param->typeName));
        }
    }
    if (defaults.empty()) return;
    const std::string linkName = functionLinkKey(
        node->moduleName, node->name,
        node->sigKey.empty() ? node->name : node->sigKey);
    funcDefaultArgs_[linkName] = std::move(defaults);
    // D23（248-a）同款：参数总数同步登记（补缺点精确判定缺省个数）
    funcDefaultTotal_[linkName] = node->params.size();
}

// 族②：函数发射收尾（原 460~480 段）——类类型局部变量析构 DeleteObject
//   （RAII，须在块终止补齐后调用）/串释放/函数级名单复位/IR 函数入列/
//   参数作用域弹出（A-3a：漏 pop 修复）。
void IRGenerator::finishFunctionEmit(ir::IRFunction& func) {
    // 阶段3 OOP（Task 3.1）：类类型局部变量（有析构函数）函数收尾 DeleteObject（RAII）。
    // 注意：必须在块终止补齐后调用（genClassDestructorCalls 在最后一个未终止块
    //   末尾插入 DeleteObject；若函数已有返回则不插入，避免破坏既有终止）
    genClassDestructorCalls();
    genStringFrees();
    // 100（939）：盒内类值副本函数尾兜底（返回/跳出路径未达块出口的存量
    //   登记——幂等：块出口已释放的条目值字段=0 空安全跳过）
    emitPendingBoxCopiesRelease(0);
    // 182（1019）：结果/可选<析构类> 变量盒亡函数尾兜底（返回/跳出路径未达块
    //   出口的存量登记——runtime 摘取幂等·已释放条目句柄=0 空安全跳过）
    emitPendingBoxVarsRelease(0);
    stringTainted_.clear();
    // 72-a（2026-09-11 第七十二轮）：块级作用域名单复位（函数级状态——下一函数干净）
    ownedStringOrder_.clear();
    // 616：prologue 副本槽名单复位（正常路径 genStringFrees 已清；防御性再清）
    ownedParamCopySlots_.clear();
    // 662（617 三元面补全）：三元条件寄存器表复位（节点指针键——防跨函数累积）
    ternaryCondRegs617_.clear();
    ownedClassOrder_.clear();
    ownedFieldOrder_.clear();
    pendingBoxCopies_.clear();
    scopeBoxCopiesBase_.clear();
    pendingBoxVars_.clear();
    scopeBoxVarsBase_.clear();
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
