// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <cstdio>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitCallExpr(CallExpr* node) {
    // 81-a：调用返回结构体实参临时清理（RAII 守卫，单点覆盖全部 return 路径）。
    //   实参求值期登记（genArgValueWithCleanup），本函数返回（=调用已发射）后
    //   统一释放——元素槽/形参槽已持有独立副本，释放安全；幂等清零。
    //   基准式隔离：嵌套调用（实参本身是调用）各自持基准，内层只清理内层新增项。
    struct ArgCleanupGuard {
        IRGenerator* gen;
        std::size_t base;
        ~ArgCleanupGuard() { gen->flushPendingArgCleanups(base); }
    } argCleanupGuard{this, pendingArgCleanups_.size()};
    (void)argCleanupGuard;
    // ---- plans/019 阶段1（2026-09-10）：显式转移 转移(变量) 展开 ----
    // 语义层已检查放行（声明初始化位在 visitVarDecl 已改写为标识符，不会到这；
    // 此处=表达式位指针/字符串值交接）。展开=实参标识符的值加载（转移() 零运行
    // 时指令，纯编译期标记语义）。resolvedType 非空且 callee 为 转移 双重判定，
    // 防用户经占位注册签名外的同名调用误入。
    if (SemanticAnalyzer::isTransferCall(node) && !node->resolvedType.empty()) {
        (void)genExpr(node->arguments[0].get());  // lastExpr_=实参求值结果（值交接）
        return;
    }
    // ---- 阶段3 OOP（Task 3.1/3.2）：构造调用/成员方法调用/虚调用 ----
    // 构造调用 类名(实参)：callee 为类类型名（NewObject + 构造体 Call）；
    // 成员方法调用 对象.方法(实参)：callee 为 MemberExpr（虚 -> VirtualCall，
    //   非虚/父类/静态 -> 直接 Call）。命中即返回，未命中（普通函数调用）走原路径。
    if (handleClassCallExpr(node)) {
        return;
    }
    // ---- 阶段3 错误处理（Task 3.5）：内置构造器 正常/错误/某些 降级 ----
    // 语义层已推导 结果<T,E>/可选<T> 类型（resolvedType），此处分配合成结构体
    // 临时槽 + 写 是否正常/是否某些 + 值/错误值，返回结构体地址（ptr）。
    // 命中即返回，未命中（普通函数调用）走原路径。
    if (handleResultCtor(node)) {
        return;
    }
    // 判断是否为直接函数名调用（函数名不在变量表中）
    bool isDirect = false;
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
        // Task 2.10 lambda 闭包调用：`加倍(21)`——callee 是登记过的闭包变量，
        //   展开为：捕获实参（前置，定义处已固化的值快照/引用指针，缺陷修复——
        //   原实现在此 genExpr(变量) 于调用点取值，[=] 值捕获读到最新值而非快照）
        //           + 显式实参 + Call 匿名函数
        auto closureIt = closureInfo_.find(calleeName);
        if (closureIt != closureInfo_.end()) {
            std::vector<ir::IRValue> closureArgs;
            for (const auto& capArg : closureIt->second.captureArgs) {
                closureArgs.push_back(capArg);
            }
            for (auto& arg : node->arguments) {
                closureArgs.push_back(genExpr(arg.get()));
            }
            // 结果类型：匿名函数返回 IR 类型（非空）；空（如 空类型）用 void
            const std::string retType = closureIt->second.returnIrType.empty()
                                            ? "void" : closureIt->second.returnIrType;
            lastExpr_ = emitResult(ir::Opcode::Call, closureArgs, retType,
                                   closureIt->second.lambdaName, node->location);
            return;
        }
        if (lookupVar(calleeName).id < 0) isDirect = true;  // 不在变量表 -> 函数名
        // Task 2.10 重载：语义层决议结果（签名 key 名#参数串）优先作为符号名——
        //   定义/调用三处一致（codegen 按此生成 mangled 符号）。
        //   内置函数（打印/字符串API）无重载，resolvedSignature 为空，保持原名映射。
        if (isDirect && !node->resolvedSignature.empty()) {
            // C-3（FFI）：外部 函数 调用——链接符号 = 纯名（C 符号无重载
            //   mangling、无模块前缀；codegen 自动 EXTERN，链接期由库解析）
            if (semantic_ != nullptr &&
                semantic_->isExternFunc(node->resolvedSignature)) {
                calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
            } else {
                calleeName = node->resolvedSignature;
            }
        }
    }
    // ---- 默认实参补全（Task 2.10）----
    // 决议已按"实参个数+可补全"匹配。缺省参数按 函数定义侧收集的默认值常量
    // （funcDefaultArgs_[签名key]，visitFunctionDecl 时由 defaultExpr 求值）精确展开。
    // 收集规则：defaults 列表按函数参数顺序仅含带默认值的尾部参数；
    //   调用方缺 N 个实参时取 defaults 最后 N 个（从右向左连续声明）。
    if (isDirect && !node->resolvedSignature.empty()) {
        auto defIt = funcDefaultArgs_.find(node->resolvedSignature);
        if (defIt != funcDefaultArgs_.end()) {
            const auto& defaults = defIt->second;
            const std::size_t given = node->arguments.size();
            // 参数总数：语义层注册的 paramTypes 长度（含默认参数）
            const std::size_t totalParams = semantic_ != nullptr
                ? semantic_->funcParamTypesOf(node->resolvedSignature).size()
                : given + defaults.size();
            // 缺省个数 = 参数总数 - 实参个数（0 ~ defaults.size()）
            if (given < totalParams) {
                const std::size_t missing = totalParams - given;
                defaultArgValues_.clear();
                // 取 defaults 中 末尾 missing 个（尾部参数默认值，从右向左连续声明）
                for (std::size_t k = defaults.size() - missing; k < defaults.size(); ++k) {
                    defaultArgValues_.push_back(defaults[k]);
                }
            }
        }
    }

    // ---- 打印 / 打印行 展开（Task 2.5 + Task 2.9 语义调整）----
    // 新语义（用户裁决，lessons.md 权重10.4）：
    //   打印   = println（末尾自动换行）——运行时 printLine/__cn_print_* + newline
    //   打印行 = print（末尾不换行）——运行时 __cn_print_*（fputs 语义，无换行）
    // 统一展开：逐参数按类型调用运行时打印函数（字符串/整数/浮点/i128/布尔），
    //   打印 最后加 __cn_print_newline；打印行 不加。
    // 注意：打印函数返回 void，用 emit 直接发射（不分配结果寄存器，
    //       codegen 对 id<0 的结果不生成返回值存储）
    bool isPrintNewline = (isDirect && calleeName == "打印");      // 打印：换行
    bool isPrintNoLine = (isDirect && calleeName == "打印行");     // 打印行：不换行
    if (isPrintNewline || isPrintNoLine) {
        for (auto& arg : node->arguments) {
            ir::IRValue argVal = genExpr(arg.get());
            // 参数类型 -> 打印函数名（字符串/字符* -> 字符串；整型 -> 整数；浮点 -> 浮点）
            std::string printFn;
            if (argVal.type == "ptr") {
                printFn = "__cn_print_str";
            } else if (argVal.type == "f32" || argVal.type == "f64") {
                // 浮32 必须先 Cast 浮64 再传 __cn_print_float(double)——
                // 否则 ABI 不匹配（xmm0 低32位垃圾被当 double 读，输出 0.000000）
                // 缺陷修复（方案C 2026-08-14）：原 打印行浮点(f32) 靠语义层隐式宽化，
                //   改 打印(f32) 变参展开后此 Cast 必须显式
                if (argVal.type == "f32") {
                    argVal = emitResult(ir::Opcode::Cast, {argVal}, "f64", "",
                                        node->location);
                }
                printFn = "__cn_print_float";
            } else if (argVal.type == "i128" || argVal.type == "u128") {
                // i128/正128（Task 完善A）：传双寄存器地址（低64位槽地址），
                // 运行时辅助函数 __cn_print_i128 读 16 字节双槽
                printFn = (argVal.type == "u128") ? "__cn_print_u128"
                                                  : "__cn_print_i128";
            } else if (argVal.type == "i1") {
                // 2026-08（用户裁决，布尔打印统一）：布尔（含比较/逻辑表达式
                //   结果）经 打印 统一输出 真/假（__cn_print_bool）——此前 i1
                //   走 __cn_print_int 输出 1/0，与字符串拼接 布尔转字符串 不一致
                printFn = "__cn_print_bool";
            } else {
                // 整型（含 i1 布尔）：有符号统一 Cast i64 走 __cn_print_int；
                // 无符号（正8~正64）直接传 __cn_print_uint（%llu 语义，缺陷修复：
                //   原实现统一 __cn_print_int（%lld 有符号），正64 值超 2^63 时
                //   位模式按有符号解释打印成负数）
                const bool isUnsignedArg = (argVal.type == "u8" ||
                                            argVal.type == "u16" ||
                                            argVal.type == "u32" ||
                                            argVal.type == "u64");
                if (isUnsignedArg) {
                    if (argVal.type != "u64") {
                        argVal = emitResult(ir::Opcode::Cast, {argVal}, "u64", "",
                                            node->location);
                    }
                    printFn = "__cn_print_uint";
                } else {
                    if (argVal.type != "i64") {
                        argVal = emitResult(ir::Opcode::Cast, {argVal}, "i64", "",
                                            node->location);
                    }
                    printFn = "__cn_print_int";
                }
            }
            emit(ir::Opcode::Call, {argVal}, ir::IRValue(), printFn, "void",
                 node->location);
        }
        // 打印（println 语义）末尾统一换行；打印行（print 语义）不换行
        if (isPrintNewline) {
            emit(ir::Opcode::Call, {}, ir::IRValue(), "__cn_print_newline", "void",
                 node->location);
        }
        lastExpr_ = ir::IRValue();  // 无返回值（空类型）
        return;
    }

    // ---- 字符串API名称映射（Task 2.5 + Task 2.8）：中文函数名 -> 运行时符号 ----
    if (isDirect) {
        if (calleeName == "字符串长度") calleeName = "__cn_str_len";
        else if (calleeName == "字符串比较") calleeName = "__cn_str_eq";
        else if (calleeName == "字符串连接") calleeName = "__cn_str_concat";
        else if (calleeName == "字符串复制") calleeName = "__cn_str_copy";
        else if (calleeName == "字符串查找") calleeName = "__cn_str_find";
        // Task 2.8 补充API（规格书10.1 标注"常见字符串库补充"）
        else if (calleeName == "字符串子串") calleeName = "__cn_str_sub";
        else if (calleeName == "字符串字典序") calleeName = "__cn_str_cmp";
        else if (calleeName == "字符串大写") calleeName = "__cn_str_upper";
        else if (calleeName == "字符串小写") calleeName = "__cn_str_lower";
        else if (calleeName == "字符串前缀") calleeName = "__cn_str_starts_with";
        else if (calleeName == "字符串后缀") calleeName = "__cn_str_ends_with";
        else if (calleeName == "字符串包含") calleeName = "__cn_str_contains";
        else if (calleeName == "字符串修剪") calleeName = "__cn_str_trim";
        else if (calleeName == "字符串反转") calleeName = "__cn_str_reverse";
        else if (calleeName == "整数转字符串") calleeName = "__cn_str_from_int";
        else if (calleeName == "浮点转字符串") calleeName = "__cn_str_from_float";
        else if (calleeName == "字符转字符串") calleeName = "__cn_str_from_char";
        else if (calleeName == "布尔转字符串") calleeName = "__cn_str_from_bool";  // Task 2.9
        else if (calleeName == "正数转字符串") calleeName = "__cn_str_from_uint";
        else if (calleeName == "字符串释放") calleeName = "__cn_str_free";
        // 字符串驻留（自举重建 P1，2026-08-25；对标 rustc Symbol / LLVM StringPool）
        //   驻留(字符串) -> 整64 Symbol ID；驻留文本(ID) -> 字符串；驻留计数 -> 整64
        else if (calleeName == "驻留") calleeName = "__cn_intern";
        else if (calleeName == "驻留文本") calleeName = "__cn_intern_text";
        else if (calleeName == "驻留计数") calleeName = "__cn_intern_count";
        // Task 2.9：格式化（格式字符串, 参数...）-> 字符串（sprintf 风格）
        else if (calleeName == "格式化") calleeName = "__cn_format";
        // Task 6.3 数学库：中文限定名 -> 运行时符号（math_api.cpp）。第 4 层
        //   （v2.0 决策6）内置 key `::` 化：语义层注册 数学::平方根，visitCallExpr
        //   重写为 IdentifierExpr("数学::平方根")；此处映射为 __cn_sqrt。
        //   旧点号名（数学.平方根，v1.0）兼容映射保留——第 6 层迁移前 E2E
        //   25_math 等仍用 数学.平方根(值)（语义层 builtinQualified 双判定）。
        else if (calleeName == "数学::平方根" || calleeName == "数学.平方根") calleeName = "__cn_sqrt";
        else if (calleeName == "数学::幂" || calleeName == "数学.幂") calleeName = "__cn_pow";
        else if (calleeName == "数学::正弦" || calleeName == "数学.正弦") calleeName = "__cn_sin";
        else if (calleeName == "数学::余弦" || calleeName == "数学.余弦") calleeName = "__cn_cos";
        else if (calleeName == "数学::正切" || calleeName == "数学.正切") calleeName = "__cn_tan";
        else if (calleeName == "数学::绝对值" || calleeName == "数学.绝对值") calleeName = "__cn_fabs";
        else if (calleeName == "数学::向上取整" || calleeName == "数学.向上取整") calleeName = "__cn_ceil";
        else if (calleeName == "数学::向下取整" || calleeName == "数学.向下取整") calleeName = "__cn_floor";
        // Task 6.1 核心库：运行时错误（断言 依赖）-> __cn_runtime_error
        //   （IR 层数组越界检查已直接发射该符号，codegen 有映射）
        else if (calleeName == "运行时错误") calleeName = "__cn_runtime_error";
        // Task 6.2 IO 输入（stdlib/IO.cn 包装的内置，语义层注册为 IO:: 限定名）：
        //   IO::读取行 -> __cn_read_line、IO::读取整数 -> __cn_read_int、
        //   IO::读取浮点 -> __cn_read_float、IO::打印到错误 -> __cn_print_err（input_api.cpp）
        else if (calleeName == "IO::读取行" || calleeName == "IO.读取行") calleeName = "__cn_read_line";
        else if (calleeName == "IO::读取整数" || calleeName == "IO.读取整数") calleeName = "__cn_read_int";
        else if (calleeName == "IO::读取浮点" || calleeName == "IO.读取浮点") calleeName = "__cn_read_float";
        else if (calleeName == "IO::打印到错误" || calleeName == "IO.打印到错误") calleeName = "__cn_print_err";
        // Task 6.2 文件 API（stdlib/文件.cn 包装的内置，语义层注册为 文件:: 限定名）：
        //   文件::打开文件 -> __cn_file_open、文件::读取文件 -> __cn_file_read、
        //   文件::写入文件 -> __cn_file_write、文件::读取文件行 -> __cn_file_read_line、
        //   文件::文件大小 -> __cn_file_size、文件::关闭文件 -> __cn_file_close、
        //   文件::文件存在 -> __cn_file_exists（file_api.cpp）
        else if (calleeName == "文件::打开文件" || calleeName == "文件.打开文件") calleeName = "__cn_file_open";
        else if (calleeName == "文件::读取文件" || calleeName == "文件.读取文件") calleeName = "__cn_file_read";
        else if (calleeName == "文件::写入文件" || calleeName == "文件.写入文件") calleeName = "__cn_file_write";
        else if (calleeName == "文件::读取文件行" || calleeName == "文件.读取文件行") calleeName = "__cn_file_read_line";
        else if (calleeName == "文件::文件大小" || calleeName == "文件.文件大小") calleeName = "__cn_file_size";
        else if (calleeName == "文件::关闭文件" || calleeName == "文件.关闭文件") calleeName = "__cn_file_close";
        else if (calleeName == "文件::文件存在" || calleeName == "文件.文件存在") calleeName = "__cn_file_exists";
        // Task 6.5 字符串扩展库（stdlib/字符串扩展.cn 包装的内置，语义层注册为
        //   解析:: 限定名——"字符串" 是类型关键字不能作限定名前缀）：
        //   解析::转整数 -> __cn_str_to_int、解析::转浮点 -> __cn_str_to_double、
        //   解析::转布尔 -> __cn_str_to_bool（string_api.cpp；成功标志经整32* 输出参数）
        else if (calleeName == "解析::转整数" || calleeName == "解析.转整数") calleeName = "__cn_str_to_int";
        else if (calleeName == "解析::转浮点" || calleeName == "解析.转浮点") calleeName = "__cn_str_to_double";
        else if (calleeName == "解析::转布尔" || calleeName == "解析.转布尔") calleeName = "__cn_str_to_bool";
        // Task 6.5 时间库（stdlib/时间.cn 包装的内置，语义层注册为 时间:: 限定名）：
        //   时间::当前时间戳 -> __cn_time、时间::单调时钟毫秒 -> __cn_clock_ms、
        //   时间::格式化时间 -> __cn_time_format（time_api.cpp）
        else if (calleeName == "时间::当前时间戳" || calleeName == "时间.当前时间戳") calleeName = "__cn_time";
        else if (calleeName == "时间::单调时钟毫秒" || calleeName == "时间.单调时钟毫秒") calleeName = "__cn_clock_ms";
        else if (calleeName == "时间::格式化时间" || calleeName == "时间.格式化时间") calleeName = "__cn_time_format";
        // Task 6.5 系统库（stdlib/系统.cn 包装的内置，语义层注册为 系统:: 限定名）：
        //   系统::参数个数 -> __cn_argc、系统::参数 -> __cn_argv（system_api.cpp）
        else if (calleeName == "系统::参数个数" || calleeName == "系统.参数个数") calleeName = "__cn_argc";
        else if (calleeName == "系统::参数" || calleeName == "系统.参数") calleeName = "__cn_argv";
        // 自举前置 C-1/C-3 内存库（io_api.cpp）：
        //   内存::活动分配数 -> __cn_alloc_live、内存::总分配次数 -> __cn_alloc_total、
        //   内存::竞技场分配 -> __cn_arena_alloc、内存::竞技场重置 -> __cn_arena_reset、
        //   内存::竞技场活动字节 -> __cn_arena_bytes、内存::释放全部 -> __cn_alloc_reset
        else if (calleeName == "内存::活动分配数" || calleeName == "内存.活动分配数") calleeName = "__cn_alloc_live";
        else if (calleeName == "内存::总分配次数" || calleeName == "内存.总分配次数") calleeName = "__cn_alloc_total";
        else if (calleeName == "内存::竞技场分配" || calleeName == "内存.竞技场分配") calleeName = "__cn_arena_alloc";
        else if (calleeName == "内存::竞技场重置" || calleeName == "内存.竞技场重置") calleeName = "__cn_arena_reset";
        else if (calleeName == "内存::竞技场活动字节" || calleeName == "内存.竞技场活动字节") calleeName = "__cn_arena_bytes";
        else if (calleeName == "内存::释放全部" || calleeName == "内存.释放全部") calleeName = "__cn_alloc_reset";
    }

    std::vector<ir::IRValue> args;
    for (std::size_t ai = 0; ai < node->arguments.size(); ++ai) {
        // D3 根治（2026-09-09 第四十六轮）：构造字面量实参位——求和(点{x=3, y=4})：
        //   结构体实参按 byRef ABI 传地址，字面量须先物化到调用方临时变量再传
        //   地址（与返回位 __ret 临时 ir_stmt.cpp 同构；Rust：值上下文临时
        //   place 物化）。原直接 genExpr -> 空桩常量0 被 Cast 成 i64 当地址传
        //   -> 被调方解引用空指针段错误（探针 rc=139 实锤，静默内存违例）。
        //   81-a：物化逻辑提取为 materializeStructInitArg 共享助手（方法调用
        //   路径 buildCallArgsOop 同用——原缺失致同一崩形，探针 P3 形态六实证）。
        if (node->arguments[ai]->getType() == NodeType::StructInitExpr) {
            StructInitExpr* argInit =
                static_cast<StructInitExpr*>(node->arguments[ai].get());
            const std::string argStruct = types::canonical(argInit->typeName);
            if (semantic_ != nullptr && semantic_->isStructType(argStruct)) {
                args.push_back(materializeStructInitArg(argInit, node->location));
                continue;
            }
        }
        // 81-a：实参求值 + 调用返回结构体临时清理登记（见 genArgValueWithCleanup）
        ir::IRValue argVal = genArgValueWithCleanup(node->arguments[ai].get(),
                                                    node->location);
        // Task 2.3：8/16位整数实参先扩展为 i64；i128/u128 实参截断为 i64
        // （Win x64 ABI 整参按64位传递；否则 emitCall 的 mov rax, op 读到槽中高位垃圾）
        // 所有 <64位 整数实参统一 Cast 到 i64（Win x64 ABI 整参按64位传递；
        // u32/i32 等若直接 mov rcx, [槽] 会读到槽高位栈残留垃圾）
        // 集成验证修复：i128/u128 实参传给 i128/u128 参数时不得截断（如
        //   `月薪(调后.年薪)`、`调整年薪(员工[0], 250000000000000000LL)`）——
        //   原实现无条件截断为 i64，被调方按 16 字节读参数槽读到垃圾高位。
        bool argIs128 = (argVal.type == "i128" || argVal.type == "u128");
        bool paramIs128 = false;
        if (argIs128 && isDirect && semantic_ != nullptr) {
            const auto paramTypes = semantic_->funcParamTypesOf(calleeName);
            if (ai < paramTypes.size()) {
                const std::string canon = types::canonical(paramTypes[ai]);
                paramIs128 = (canon == "整128" || canon == "正128");
            }
        }
        if (argVal.type == "i8" || argVal.type == "i16" ||
            argVal.type == "u8" || argVal.type == "u16" ||
            argVal.type == "i32" || argVal.type == "u32" ||
            (argIs128 && !paramIs128)) {
            argVal = emitResult(ir::Opcode::Cast, {argVal}, "i64", "", node->location);
        } else if (argVal.type == "f32") {
            // 92-a 根治（H2）：f32 实参**不再无条件提升 f64**——原注释「Win x64
            //   ABI 浮点参数按 xmm 传双精度」是错误假设：SysV AMD64 / Win64 /
            //   AAPCS64 的 float 形参均在 xmm 低 32 位（仅 double 用满 64 位）。
            //   无条件提升 → 被调方按 f32 读 xmm 低位 = double 位模式的低 32 位
            //   （1.5 的 double 低 32 位为 0）→ 浮32 参数恒 0。
            //   现按形参类型分派：形参 浮64 → 提升（隐式宽化，规范 §七）；
            //   形参 浮32 → 原样单精度传递；变参/未知（实参超出形参表 = 变参
            //   调用按 C 默认实参提升；内置 runtime 浮点形参为 double）→ 提升。
            bool promoteF32 = true;
            if (isDirect && semantic_ != nullptr) {
                const auto paramTypes = semantic_->funcParamTypesOf(calleeName);
                if (ai < paramTypes.size()) {
                    promoteF32 = (types::canonical(paramTypes[ai]) == "浮64");
                }
            }
            if (promoteF32) {
                argVal = emitResult(ir::Opcode::Cast, {argVal}, "f64", "",
                                    node->location);
            }
        }
        args.push_back(argVal);
    }
    // 追加默认实参（Task 2.10）：缺省参数在实参之后按参数顺序补全
    for (const auto& defVal : defaultArgValues_) {
        args.push_back(defVal);
    }
    defaultArgValues_.clear();
    if (isDirect) {
        // 直接调用：extra=函数名；按函数返回类型设置结果类型（Task 2.5 字符串API；
        // Task 2.7 集成修复：用户自定义函数经语义层查真实返回类型，避免浮64
        // 结果被误标 i32 导致 codegen 用 eax 读 xmm0 返回值）
        std::string resultType = "i32";
        if (calleeName == "__cn_str_len" || calleeName == "__cn_str_find" ||
            calleeName == "__cn_str_cmp" ||
            calleeName == "__cn_intern" || calleeName == "__cn_intern_count") {
            resultType = "i64";       // 字符串长度/查找/字典序 + 驻留/驻留计数 -> 整64
        } else if (calleeName == "__cn_intern_text") {
            resultType = "ptr";       // 驻留文本(ID) -> 字符串（驻留表指针，只读不释放）
        } else if (calleeName == "__cn_str_eq" || calleeName == "__cn_str_starts_with" ||
                   calleeName == "__cn_str_ends_with" || calleeName == "__cn_str_contains") {
            resultType = "i1";        // 字符串比较/前缀/后缀/包含 -> 布尔
        } else if (calleeName == "__cn_str_concat" || calleeName == "__cn_str_copy" ||
                   calleeName == "__cn_str_sub" || calleeName == "__cn_str_upper" ||
                   calleeName == "__cn_str_lower" || calleeName == "__cn_str_trim" ||
                   calleeName == "__cn_str_reverse" || calleeName == "__cn_str_from_int" ||
                   calleeName == "__cn_str_from_float" || calleeName == "__cn_str_from_char" ||
                   calleeName == "__cn_str_from_bool" || calleeName == "__cn_str_from_uint" ||
                   calleeName == "__cn_format") {
            resultType = "ptr";       // 连接/复制/子串/大写/小写/修剪/反转/数字/字符/布尔转换、格式化 -> 字符串（指针）
        } else if (calleeName == "__cn_sqrt" || calleeName == "__cn_pow" ||
                   calleeName == "__cn_sin" || calleeName == "__cn_cos" ||
                   calleeName == "__cn_tan" || calleeName == "__cn_fabs" ||
                   calleeName == "__cn_ceil" || calleeName == "__cn_floor") {
            resultType = "f64";       // 数学库（Task 6.3）：平方根/幂/正弦/余弦/正切/绝对值/向上取整/向下取整 -> 浮64
        } else if (calleeName == "__cn_read_line") {
            resultType = "ptr";       // IO 输入（Task 6.2）：读取行 -> 字符串（动态分配，EOF 返回 nullptr）
        } else if (calleeName == "__cn_read_int") {
            resultType = "i64";       // 读取整数 -> 整64（成功标志经整32* 输出参数）
        } else if (calleeName == "__cn_read_float") {
            resultType = "f64";       // 读取浮点 -> 浮64（成功标志经整32* 输出参数）
        } else if (calleeName == "__cn_file_open") {
            resultType = "ptr";       // 文件 API（Task 6.2）：打开文件 -> 空类型*（FILE*，失败 nullptr）
        } else if (calleeName == "__cn_file_read" || calleeName == "__cn_file_write" ||
                   calleeName == "__cn_file_size") {
            resultType = "i64";       // 读取/写入字节数、文件大小 -> 整64（失败 -1）
        } else if (calleeName == "__cn_file_read_line") {
            resultType = "ptr";       // 读取文件行 -> 字符串（动态分配，EOF 返回 nullptr）
        } else if (calleeName == "__cn_file_exists") {
            resultType = "i1";        // 文件存在 -> 布尔
        } else if (calleeName == "__cn_str_to_int") {
            resultType = "i64";       // 字符串扩展（Task 6.5）：转整数 -> 整64（成功标志经整32*）
        } else if (calleeName == "__cn_str_to_double") {
            resultType = "f64";       // 转浮点 -> 浮64（成功标志经整32*）
        } else if (calleeName == "__cn_str_to_bool") {
            resultType = "i64";       // 转布尔 -> 整64（0/1，成功标志经整32*；CN 层 Cast 布尔）
        } else if (calleeName == "__cn_time" || calleeName == "__cn_clock_ms") {
            resultType = "i64";       // 时间库（Task 6.5）：当前时间戳/单调时钟毫秒 -> 整64
        } else if (calleeName == "__cn_time_format") {
            resultType = "ptr";       // 格式化时间 -> 字符串（动态分配，失败 nullptr）
        } else if (calleeName == "__cn_argc") {
            resultType = "i64";       // 系统库（Task 6.5）：参数个数 -> 整64
        } else if (calleeName == "__cn_argv") {
            resultType = "ptr";       // 参数 -> 字符串（CRT 持有，越界 nullptr）
        } else if (calleeName == "__cn_alloc_live" || calleeName == "__cn_alloc_total" ||
                   calleeName == "__cn_arena_bytes") {
            resultType = "i64";       // 内存库（自举前置 C-1/C-3）：活动分配数/总分配次数/竞技场字节 -> 整64
        } else if (calleeName == "__cn_arena_alloc") {
            resultType = "ptr";       // 竞技场分配 -> 空类型*（失败 nullptr）
        } else if (calleeName == "__cn_arena_reset" || calleeName == "__cn_alloc_reset") {
            // 竞技场重置/释放全部：空类型返回（与 __cn_str_free 同惯例）
        } else if (calleeName == "__cn_str_free") {
            // 字符串释放：空类型返回，resultType 保持 i32（与用户 void 函数调用一致：
            // 语义层"空类型"->mapType "void" 被下方过滤，emitResult 结果寄存器写入
            // eax 无害且符合现有 void 调用惯例）
        } else if (semantic_ != nullptr) {
            // 用户函数：查询语义层返回类型（未映射（空/未知）回退 i32）
            // P3-18 补完：引用返回函数调用结果 = 被引用左值地址（ptr）
            if (semantic_->funcReturnsRef(calleeName)) {
                resultType = "ptr";
            } else {
                const std::string ret = semantic_->funcReturnTypeOf(calleeName);
                if (!ret.empty()) {
                    const std::string mapped = mapType(ret);
                    if (mapped != "void" && mapped != "") resultType = mapped;
                }
            }
        }
        // 结构体返回值函数（Task 完善A）：调用方分配返回缓冲区（结构体临时变量），
        //   以隐藏指针（rcx，Win x64 ABI）传给被调方；被调方写入缓冲区。
        //   结果 = 缓冲区地址（ptr），供调用方 CopyStruct 到目标。
        if (resultType == "ptr" && semantic_ != nullptr &&
            semantic_->isStructType(types::canonical(
                semantic_->funcReturnTypeOf(calleeName)))) {
            const std::string retType = semantic_->funcReturnTypeOf(calleeName);
            const std::string temp = "__retbuf" + std::to_string(varCounter_++);
            emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
                 temp, "ptr", node->location);
            // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 的 64 字节
            //   拷贝上限一致（结构体实际大小 ≤64 字节场景）
            function_->varSlots[temp] = 8;
            registerVarSlots(temp, retType);
            ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(temp, "i64")},
                                         "ptr", temp, node->location);
            // 隐藏返回指针作为第一个参数（rcx）
            std::vector<ir::IRValue> hiddenArgs = args;
            hiddenArgs.insert(hiddenArgs.begin(), buf);
            emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(), calleeName, "void",
                 node->location);
            lastExpr_ = buf;
            return;
        }
        // 字符串释放/竞技场重置/释放全部（空类型返回）：用 emit 直接发射，不分配结果寄存器
        // （与打印行 void 展开一致；codegen 对 result.id<0 不生成返回值存储）
        if (calleeName == "__cn_str_free" || calleeName == "__cn_arena_reset" ||
            calleeName == "__cn_alloc_reset") {
            emit(ir::Opcode::Call, args, ir::IRValue(), calleeName, "void",
                 node->location);
            lastExpr_ = ir::IRValue();
            return;
        }
        // 引用返回读值默认 lvalue-to-rvalue（2026-09-04 缺陷零容忍收口）：
        //   引用返回调用（resultType="ptr" 且被调为引用返回用户函数）作为右值
        //   须解引用读值（C++ 语义）——原返回裸地址被右值消费=静默错误代码
        //   （整64 a = 取值(p) 实测读出地址）；赋值目标/复合赋值/引用绑定
        //   上下文经 suppressRefDeref_ 抑制（取地址语义）
        if (resultType == "ptr" && semantic_ != nullptr &&
            semantic_->funcReturnsRef(calleeName) && !suppressRefDeref_) {
            std::string baseRet = semantic_->funcReturnTypeOf(calleeName);
            if (!baseRet.empty() && baseRet.back() == '&') {
                baseRet.pop_back();
            }
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {lastExpr_},
                                   mapType(baseRet), "", node->location);
            return;
        }
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType, calleeName,
                               node->location);
        return;
    }
    // 间接调用：先求被调者表达式（函数指针变量），再 CallIndirect
    ir::IRValue calleeVal = genExpr(node->callee.get());
    args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
    lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, "i32", "", node->location);
}

// 结构体字面量实参物化（D3 根治逻辑提取为共享助手，81-a）：按值结构体参数的 ABI
//   为「传地址」（callee 侧 rep movsb 拷贝），故字面量须先物化到调用方临时变量再
//   传地址——原仅普通调用路径有物化，方法调用路径（buildCallArgsOop）无此处理 →
//   实参寄存器为空值（汇编 mov rdx, 0）→ 被调方解引用空指针段错误（探针 P3 形态六
//   实锤：向量<盒子>.追加(盒子{...}) rc=139）。Rust 对照：值上下文临时 place 物化。
ir::IRValue IRGenerator::materializeStructInitArg(StructInitExpr* init,
                                                  const SourceLocation& loc) {
    if (init == nullptr || semantic_ == nullptr || function_ == nullptr) {
        return ir::IRValue::reg(-1, "");
    }
    const std::string argStruct = types::canonical(init->typeName);
    if (!semantic_->isStructType(argStruct)) return ir::IRValue::reg(-1, "");
    const std::string temp = "__arginit" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"), temp, "ptr",
         loc);
    function_->varSlots[temp] = 8;
    registerVarSlots(temp, argStruct);
    // 81-a：物化临时登记为「含串字段聚合局部」→ 块出口按字段偏移释放（字面量字段
    //   在构造时已归一化落堆或为驻留借用，释放面空安全）——原实现只物化不登记 =
    //   含串字段的结构体字面量实参每调用泄漏其字段串（E2E 235 ②③⑦⑧ 实证：帧内
    //   临时无人释放）；与 79-a 的聚合字段 RAII 同名单同机制（块基线 scopeFieldBase_）。
    oopVarSrcTypes_[temp] = argStruct;
    if (!ownedStrFieldsOf(argStruct).empty()) {
        ownedFieldOrder_.push_back(temp);
    }
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(temp, "i64")}, "ptr", temp, loc);
    emitStructInitTo(init, base, loc);
    return base;
}

// 81-a：实参求值 + 「调用返回结构体临时」清理登记（泄漏面见 ir.hpp 声明注释）
ir::IRValue IRGenerator::genArgValueWithCleanup(Expr* arg,
                                                const SourceLocation& loc) {
    (void)loc;   // 位置仅用于诊断归属（清理发射用默认位置，与块出口释放同款）
    ir::IRValue v = genExprForOop(arg);
    if (arg == nullptr || semantic_ == nullptr) return v;
    if (arg->getType() != NodeType::CallExpr) return v;
    // 结构体字面量实参走 materializeStructInitArg（已物化，无临时泄漏面）
    CallExpr* ce = static_cast<CallExpr*>(arg);
    std::string retCanon = types::canonical(ce->resolvedType);
    if (retCanon.empty() && ce->callee != nullptr &&
        ce->callee->getType() == NodeType::IdentifierExpr) {
        // 语义层 resolvedType 只对内置构造器写回——普通调用按「重载决议签名键 /
        //   裸名」两路回退查返回类型（functions_ 键为 名#签名）
        const std::string nm =
            static_cast<IdentifierExpr*>(ce->callee.get())->name;
        const std::string keys[2] = {ce->resolvedSignature, nm};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            const std::string rt = types::canonical(semantic_->funcReturnTypeOf(key));
            if (!rt.empty()) { retCanon = rt; break; }
        }
    }
    if (retCanon.empty()) return v;
    if (ownedStrFieldsOf(retCanon).empty()) return v;
    pendingArgCleanups_.emplace_back(v, retCanon);
    return v;
}

// 调用发射后统一清理（只清理 base 之后新增项；释放幂等清零，嵌套调用各自持基准）
void IRGenerator::flushPendingArgCleanups(std::size_t base) {
    if (base >= pendingArgCleanups_.size()) {
        pendingArgCleanups_.resize(base);   // 防御：截断到基准
        return;
    }
    for (std::size_t i = base; i < pendingArgCleanups_.size(); ++i) {
        emitOwnedStrFieldFreesAt(pendingArgCleanups_[i].first,
                                 pendingArgCleanups_[i].second, SourceLocation());
    }
    pendingArgCleanups_.resize(base);
}
} // namespace cn_compiler
