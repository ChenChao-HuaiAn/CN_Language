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
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

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
    // 转移(变量) 展开（plans/019 阶段1·954 族体见 emitTransferCallExpand）
    if (isTransferCall(node) && !node->resolvedType.empty()) {
        emitTransferCallExpand(node);
        return;
    }
    // ---- 206-b（波 4·plans/022 §四.5）：复制(表达式) 泛型克隆内置 ----
    //   语义层已推导 resolvedType（=实参类型，genCopyBuiltin 分派依据）。
    //   与 转移(x) 构成显式「复制/移动」双内置（C++ copy/move 对照物）。
    if (node->callee->getType() == NodeType::IdentifierExpr &&
        !node->resolvedType.empty() &&
        static_cast<IdentifierExpr*>(node->callee.get())->name == "复制" &&
        node->arguments.size() == 1) {
        genCopyBuiltin(node, node->location);
        return;
    }
    // ---- 任务 474：回绕内置 回绕加/回绕减/回绕乘（显式两补码回绕算术）----
    // 语义层已检查放行并推导 resolvedType（=与 + - * 完全一致的类型规则，
    // 含 285 窄域字面量豁免）。展开=两实参 Cast 对齐公共宽度 + WrapAdd/WrapSub/
    // WrapMul 指令（发射层无溢出检查——回绕语义的显式豁免面）。
    if (node->callee->getType() == NodeType::IdentifierExpr &&
        !node->resolvedType.empty() && node->arguments.size() == 2) {
        const std::string& wrapName =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        ir::Opcode wrapOp;
        if (wrapName == "回绕加") wrapOp = ir::Opcode::WrapAdd;
        else if (wrapName == "回绕减") wrapOp = ir::Opcode::WrapSub;
        else if (wrapName == "回绕乘") wrapOp = ir::Opcode::WrapMul;
        else wrapOp = ir::Opcode::WrapAdd;  // 不可达（ callee 名三选一）；防御占位
        const bool isWrapBuiltin = (wrapName == "回绕加" || wrapName == "回绕减" ||
                                    wrapName == "回绕乘");
        if (isWrapBuiltin) {
            ir::IRValue l = genExpr(node->arguments[0].get());
            ir::IRValue r = genExpr(node->arguments[1].get());
            // 结果类型：语义层 resolvedType（源码类型）-> IR 类型
            const std::string resTy = mapType(node->resolvedType);
            // 操作数对齐（与 visitBinaryExpr 混合宽度 Cast 同口径）：同型跳过，
            // 异型按结果类型 Cast（宽侧窄落仅出现于 285 窄域字面量豁免形态——
            // 字面量 Cast 回绕与现行 377 行为一致）
            if (l.type != resTy && !l.type.empty()) {
                l = emitResult(ir::Opcode::Cast, {l}, resTy, "", node->location);
            }
            if (r.type != resTy && !r.type.empty()) {
                r = emitResult(ir::Opcode::Cast, {r}, resTy, "", node->location);
            }
            lastExpr_ = emitResult(wrapOp, {l, r}, resTy, "", node->location);
            return;
        }
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
        // Task 2.10 lambda 闭包调用（速径展开·族体见 genClosureCallExpr·ir_call_closure.cpp）
        if (genClosureCallExpr(node, calleeName)) return;
        if (lookupVar(calleeName).id < 0) {
            isDirect = true;  // 不在变量表 -> 函数名
            // 040（001 §5.8 甲案·2026-10-05）：静态槽函数指针调用——lookupVar 只查
            //   函数局部表，静态槽 miss 曾误判直调（LNK2019 静态回调 实证）；静态
            //   类型为 fnptr 时保持间接（genExpr 标识符=静态槽 LoadPtr→CallIndirect
            //   与局部 fnptr 同链）
            if (semantic_ != nullptr && semantic_->isGlobalStatic(calleeName) &&
                semantic_->globalStaticType(calleeName).rfind("函数指针<", 0) == 0) {
                isDirect = false;
            }
        }
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
            // 参数总数：优先 funcDefaultTotal_（D23：两侧收集点同步登记，构造
            //   不入 functions_ 回退口径 given+defaults 会把显式传满参的调用误补）；
            //   次选语义层注册的 paramTypes 长度（普通函数）。
            std::size_t totalParams = 0;
            auto totIt = funcDefaultTotal_.find(node->resolvedSignature);
            if (totIt != funcDefaultTotal_.end()) {
                totalParams = totIt->second;
            } else if (semantic_ != nullptr) {
                totalParams = semantic_->funcParamTypesOf(node->resolvedSignature).size();
            }
            if (totalParams == 0) totalParams = given;
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
        // 852 收工解堵（2026-10-16）：打印族展开迁出 emitPrintLikeCall
        //   （visitCallExpr 983 行巨函数首刀瘦身——纯机械搬移零语义；
        //   单函数 ≤100 行纪律的既有欠账另案清偿）
        emitPrintLikeCall(node, isPrintNewline, isPrintNoLine);
        return;
    }

    // ---- 字符串API名称映射（Task 2.5 + Task 2.8）：中文函数名 -> 运行时符号 ----
    // 617（2026-10-14 伞批）：内置字符串消费族判定（映射前中文名）——分配族
    //   拷贝内容、谓词族只读、格式化 sprintf 拷贝，均不收下实参句柄=调用完成后
    //   其「拥有串临时」实参（argIsOwnedStringTemp）可安全结算。拼接链中间临时
    //   同点归零：t1=连接(a,b) 作为 连接(t1,c) 实参时在内层发射完释放。
    //   驻留（表可能持指针视图）保守不入集合。
    const bool isStrConsumer617 =
        isDirect && (calleeName == "字符串长度" || calleeName == "字符串比较" ||
                     calleeName == "字符串连接" || calleeName == "字符串复制" ||
                     calleeName == "字符串查找" || calleeName == "字符串子串" ||
                     calleeName == "字符串字典序" || calleeName == "字符串大写" ||
                     calleeName == "字符串小写" || calleeName == "字符串前缀" ||
                     calleeName == "字符串后缀" || calleeName == "字符串包含" ||
                     calleeName == "字符串修剪" || calleeName == "字符串反转" ||
                     calleeName == "字符串拼接" || calleeName == "格式化");
    // 662：收集项携带来源 AST（三元实参结算走分支拥有性感知条件释放）
    std::vector<std::pair<const Expr*, ir::IRValue>> consumedArgTemps617;
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

        // 996（027 波 1·线程库）：线程::新 -> __cn_thread_new、线程::并入 ->
        //   __cn_thread_join、线程::互斥锁新建/上锁/解锁/释放 -> __cn_mutex_*
        //   （thread_api.cpp；linux=pthread 真实现·win=错误码 9 桩随波 3）
        else if (calleeName == "线程::新" || calleeName == "线程.新") calleeName = "__cn_thread_new";
        else if (calleeName == "线程::并入" || calleeName == "线程.并入") calleeName = "__cn_thread_join";
        else if (calleeName == "线程::互斥锁新建" || calleeName == "线程.互斥锁新建") calleeName = "__cn_mutex_new";
        else if (calleeName == "线程::上锁" || calleeName == "线程.上锁") calleeName = "__cn_mutex_lock";
        else if (calleeName == "线程::解锁" || calleeName == "线程.解锁") calleeName = "__cn_mutex_unlock";
        else if (calleeName == "线程::互斥锁释放" || calleeName == "线程.互斥锁释放") calleeName = "__cn_mutex_free";
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
        else if (calleeName == "内存::总分配字节" || calleeName == "内存.总分配字节") calleeName = "__cn_alloc_bytes";
        else if (calleeName == "内存::总分配次数" || calleeName == "内存.总分配次数") calleeName = "__cn_alloc_total";
        else if (calleeName == "内存::竞技场分配" || calleeName == "内存.竞技场分配") calleeName = "__cn_arena_alloc";
        else if (calleeName == "内存::竞技场重置" || calleeName == "内存.竞技场重置") calleeName = "__cn_arena_reset";
        else if (calleeName == "内存::竞技场活动字节" || calleeName == "内存.竞技场活动字节") calleeName = "__cn_arena_bytes";
        else if (calleeName == "内存::释放全部" || calleeName == "内存.释放全部") calleeName = "__cn_alloc_reset";
        // 波2（074·872）：字符串构建器四件套（string_api.cpp·Rust String::push_str 同款）
        else if (calleeName == "字符串构建器::新建" || calleeName == "字符串构建器.新建") calleeName = "__cn_strbuf_new";
        else if (calleeName == "字符串构建器::追加" || calleeName == "字符串构建器.追加") calleeName = "__cn_strbuf_append";
        else if (calleeName == "字符串构建器::追加字节" || calleeName == "字符串构建器.追加字节") calleeName = "__cn_strbuf_append_char";
        else if (calleeName == "字符串构建器::完成" || calleeName == "字符串构建器.完成") calleeName = "__cn_strbuf_finish";
        // 366（2026-10-13）：第五件 弃置（错误路径中途放弃释放缓冲·防泄漏）
        else if (calleeName == "字符串构建器::弃置" || calleeName == "字符串构建器.弃置") calleeName = "__cn_strbuf_drop";
    }

    // ---- 442（判据统一·421 审查 P2-2/P2-3）：有效形参表（实参定标唯一判据源）----
    //   直接调用=语义层形参表；间接（函数指针）调用=callee 源码类型串形参表
    //   （funcPtrTypeOfCallee -> funcPtrParamsOf·与语义层同口径）。原间接路径
    //   paramIs128 恒假/无 promoteF32 依据 → 两处定标判据误判：
    //   ①已 128 位实参被 (argIs128 && !paramIs128) 误截断为 i64（高半丢失 →
    //     再宽化按 i64 符号扩展=静默错值。探针 442pe2/442pe3 实锤：fnptr 实参
    //     2^64+1 -> 1；v2 侧同形正确=分叉）；
    //   ②浮32 实参对浮32 形参被无条件提升 f64（探针 442pk 实锤：指32(2.5f) ->
    //     1.5；v2 侧 4.0 正确）。
    //   两侧同判据（v2 回扫结构体与引用实参 / 发射间接调用480 皆签名表驱动）：
    //   形参表逐位对号——128 按值形参位定标、引用位不宽化、浮点形参不提升。
    //   个数不匹配（变参等）保守回落现状=无表（零行为变更）。
    const std::string calleeFnPtrType442 = isDirect
                                               ? std::string()
                                               : funcPtrTypeOfCallee(node->callee.get());
    std::vector<std::string> effectiveParamTypes;
    if (isDirect && semantic_ != nullptr) {
        effectiveParamTypes = semantic_->funcParamTypesOf(calleeName);
        // 655（648 余量①·712 返回类型两级查询同构）：外部 函数（FFI）调用符号
        //   =纯名（C 符号无 mangling），而 functions_ 表键=签名 key（名#参数串）
        //   ——纯名 miss 曾止步于此（648 豁免④只免报不回填），128 定标/浮点
        //   提升/引用宽化三判据全跳过=实参 ABI 静默错值（探针 p655-① 实锤：
        //   sqrtf(浮32 2.25) 实参被恒提升 f64 传 8 字节，被调按 4 字节读=0.0；
        //   修复后两级命中按形参表逐位判定=1.5）。miss 回退查签名 key，与
        //   712 返回类型查询（ir_call.cpp resolvedSignature 回退）完全同构。
        if (effectiveParamTypes.empty() && !node->resolvedSignature.empty()) {
            effectiveParamTypes =
                semantic_->funcParamTypesOf(node->resolvedSignature);
        }
    } else if (!calleeFnPtrType442.empty()) {
        std::vector<std::string> fnPtrParams442 =
            types::funcPtrParamsOf(calleeFnPtrType442);
        if (fnPtrParams442.size() == node->arguments.size()) {
            effectiveParamTypes = std::move(fnPtrParams442);
        }
    }
    // 648 miss 不可静默：直接调用形参表 miss=终态（128 定标/浮点提升/引用宽化
    //   三判据全跳过·实参 ABI 错配=静默错值面）。报诊断后保留无表回落现状
    //   （控制流零变更）。豁免三面=既有合法回落契约（495 注释）：
    //   ①__cn_ 内置改写名不在 functions_（内置实参语义层已检查·定标不依赖此表）；
    //   ②含 '$' 的泛型实例/跨模块符号（funcParamTypesOf 无泛型方法回退·实例化
    //   调用为海量合法面）；③已登记无参函数（paramTypes 空表=非 miss·经
    //   funcFirstSigKey 区分）。原豁免④（签名 key 可命中）已由上方 655 两级
    //   回退收编——签名 hit 时 effectiveParamTypes 非空不再进入本分支。
    //   间接调用回落（类型串 miss/形参个数不匹配=变参合法）不在此报：
    //   类型串整体 miss 由 851 终态收口报告，此处不双报。
    if (isDirect && semantic_ != nullptr && effectiveParamTypes.empty() &&
        calleeName.rfind("__cn_", 0) != 0 &&
        calleeName.find('$') == std::string::npos) {
        const bool knownFunc = !semantic_->funcFirstSigKey(calleeName).empty();
        if (!knownFunc) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "内部错误: 函数形参表未登记 '" + calleeName + "'（#648）");
        }
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
        std::string param128Kind;  // 316-a（C23/T45 甲）：形参 128 位具体类型（i128/u128）
        // 442：形参表来源统一为 effectiveParamTypes（直调=语义层/间接=函数指针
        //   签名——间接位原恒 miss → 已 128 位实参被误截断，见上方判据源注释）
        if (ai < effectiveParamTypes.size()) {
            const std::string canon = types::canonical(effectiveParamTypes[ai]);
            // 466（wt466·p3 探针 IR+汇编实锤）：**128 引用形参位不进按值定标
            //   通道**——canonical 剥 '&' 后 整128& 与 整128 同判 → 引用实参
            //   已被语义层 wrapRefArgs 取地址（ptr=调用方左值地址），此处再
            //   Cast 成 i128 值 → emitCall i128 分支改传「Cast 临时双槽地址」
            //   → 被调引用槽收临时槽地址，体内 16 字节读写全落在临时槽
            //   （调用方变量不变·标量/数组元素/读侧/自增四面全红实证）。
            //   引用位保持 ptr 原样传（rcx=左值地址=被调 A-1 契约）。
            if (!types::isReference(effectiveParamTypes[ai]) &&
                (canon == "整128" || canon == "正128")) {
                paramIs128 = true;
                param128Kind = (canon == "正128") ? "u128" : "i128";
            }
        }
        // 316-a（C23/T45 甲·用户裁决）：i128/u128 形参的**窄整实参**统一定标
        //   128 位——字面量（物理 i64 寄存器形态）/表达式/折叠产物原样保留时
        //   emitCall 走通用值传分支（mov rdx, [槽] 低 64 位直传），被调方按
        //   i128 指针 ABI 解引用 = 契约分叉（m45_01 SIGSEGV / m45_02 静默错值 /
        //   m45_03 类构造字面实参·深度机 314-a 立案）。经宽化 Cast（源符号
        //   扩展/零扩展双半）定标后走 emitCall i128 分支（lea 取地址 = 与
        //   变量/表达式实参同 ABI）。
        // 442（判据统一）：**不再排除浮点实参**——浮点→128 同走转换通道
        //   （Cast 浮源→128 经 __cn_f64_to_i128 全宽辅助；v2 物化128位操作数
        //   浮域分支同构），消除「宿主排除/v2 转换」微差。语义层对 浮→整
        //   隐式转换本就拒绝（001 §3.7），本判据=IR 层安全网（不可达面同构）。
        if (paramIs128 && !argIs128) {
            argVal = emitResult(ir::Opcode::Cast, {argVal}, param128Kind, "",
                                node->location);
            argIs128 = true;
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
            // 442：判据源统一 effectiveParamTypes——间接调用原无表 → 恒走「未知
            //   →提升」分支，浮32 形参的 fnptr 调用被误提升 f64（探针 442pk：
            //   指32(2.5f) -> 1.5；v2 正确 4.0=分叉）。有表（含间接签名表）时
            //   按形参逐位判定。
            bool promoteF32 = true;
            if (ai < effectiveParamTypes.size()) {
                promoteF32 = (types::canonical(effectiveParamTypes[ai]) == "浮64");
            }
            if (promoteF32) {
                argVal = emitResult(ir::Opcode::Cast, {argVal}, "f64", "",
                                    node->location);
            }
        }
        args.push_back(argVal);
        // 617：消费族拥有串临时实参收集（调用发射完成后结算）——ptr 前置挡
        //   整数加法形态（BinaryExpr+Add 判定的类型护栏）；662：收集项携带
        //   来源 AST（三元实参结算走条件释放）
        if (isStrConsumer617 && argVal.type == "ptr" &&
            argIsOwnedStringTemp(node->arguments[ai].get())) {
            consumedArgTemps617.emplace_back(node->arguments[ai].get(), argVal);
            // 644 rebase 交叉收口（2026-10-15）：本实参若已被 81-a 纯串登记
            //   （genArgValueWithCleanup 644/662 分支·同在此循环求值），从待清理
            //   名单移交本通道——释放权单点归 617 结算（防 flushPendingArg
            //   Cleanups 与 617 结算对同一句柄双 __cn_str_free）。实参求值期
            //   间内层调用的 flush 已按各自 base 回卷，该登记项此刻必在名单内。
            for (auto it = pendingArgCleanups_.begin();
                 it != pendingArgCleanups_.end(); ++it) {
                if (std::get<1>(*it) == "字符串" &&
                    std::get<0>(*it).id == argVal.id &&
                    !std::get<0>(*it).isConstant) {
                    pendingArgCleanups_.erase(it);
                    break;
                }
            }
        }
    }
    // 106：实参位「无 → 可选<T>」空可选装箱（canConvert 豁免面 IR 配套——
    //   内置名映射后 funcParamTypesOf 查无=no-op；「无→可选」仅用户函数形参面）
    if (isDirect && semantic_ != nullptr) {
        boxNullOptionalArgs(node, semantic_->funcParamTypesOf(calleeName), args);
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
                   calleeName == "__cn_arena_bytes" || calleeName == "__cn_alloc_bytes") {
            resultType = "i64";       // 内存库（自举前置 C-1/C-3）：活动分配数/总分配次数/竞技场字节 -> 整64
        } else if (calleeName == "__cn_arena_alloc") {
            resultType = "ptr";       // 竞技场分配 -> 空类型*（失败 nullptr）
        } else if (calleeName == "__cn_arena_reset" || calleeName == "__cn_alloc_reset") {
            // 竞技场重置/释放全部：空类型返回（与 __cn_str_free 同惯例）
        } else if (calleeName == "__cn_thread_new" || calleeName == "__cn_mutex_new") {
            resultType = "ptr";       // 996/027 线程库：线程句柄/互斥锁句柄 -> 空类型*——262 根治：
                                      //   本链缺线程系=默认 i32 读回截断 32 位（mov %eax）：主堆
                                      //   低位地址侥幸存活、glibc 线程独立 arena 高位必炸
                                      //   （0x7ffff0000880→0xf0000880 SIGSEGV 实证·586/610 同mask）
                                      //   v2 侧 IR布局 1364 本就正确=宿主单侧缺漏
        } else if (calleeName == "__cn_thread_join") {
            resultType = "i64";       // 并入 -> 线程返回值整64（同缺位截断面）
        } else if (calleeName == "__cn_mutex_lock" || calleeName == "__cn_mutex_unlock" ||
                   calleeName == "__cn_mutex_free") {
            // 上锁/解锁/毁锁：空类型返回（结果无人消费·与 __cn_str_free 同惯例保持 i32）
        } else if (calleeName == "__cn_strbuf_new") {
            resultType = "i64";       // 波2（074·872）：构建器句柄 -> 整64
        } else if (calleeName == "__cn_strbuf_finish") {
            resultType = "ptr";       // 波2（074·872）：完成 -> 字符串
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
                std::string ret = semantic_->funcReturnTypeOf(calleeName);
                // 110（外部 函数 整64 返回值塌缩根治）：外部 函数 调用符号=纯名
                //   （C 符号无 mangling·上方 FFI 分支改写 calleeName），而语义层
                //   functions_ 表键=签名 key（名#参数串）——纯名 miss 回退 i32，
                //   三后端 emitCall 恒按 32 位读返回值（eax/w0+符号扩展），整64
                //   大值（>2^32）低 32 截断（llabs(4294967297)→1 实测：arm64 与
                //   linux x64 双后端运行级同塌·win x64 同构按 result.type 分派
                //   同病；v2 侧双后端 str x0/mov rax 全读本即正确=宿主单侧分叉）。
                //   miss 时回退查签名 key（funcReturnTypeOf 内置 模块$签名 剥前
                //   缀·#435 fnptr 间接调用返回类型同族根治先例：IR 层一处修，
                //   三后端 emitCall 按 result.type 分派同受益）。
                if (ret.empty() && !node->resolvedSignature.empty()) {
                    ret = semantic_->funcReturnTypeOf(node->resolvedSignature);
                }
                if (ret.empty() && calleeName.rfind("__cn_", 0) != 0) {
                    // 648 miss 不可静默（2026-10-14 立法·110 同族终态）：纯名+签名
                    //   key 两级查询均 miss=终态——原静默保持初始 "i32"（按 32 位
                    //   读返回值=三平台错值源）。报诊断后保留 i32 兜底值与控制流
                    //   （driver generate() 后 hasErrors() 中止）。
                    //   豁免 "__cn_" 前缀：内置/运行时 API 已由上方内置分派覆盖
                    //   （理论上走不到此分支，保险豁免）。
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "内部错误: 函数返回类型未登记 '" + calleeName +
                            "' —— 静默 i32 兜底已废除（#648·110 同族终态）");
                }
                if (!ret.empty()) {
                    const std::string mapped = mapType(ret);
                    if (mapped != "void" && mapped != "") resultType = mapped;
                }
            }
        }
        // 结构体返回值函数（Task 完善A）：调用方分配返回缓冲区（结构体临时变量），
        //   以隐藏指针（rcx，Win x64 ABI）传给被调方；被调方写入缓冲区。
        //   结果 = 缓冲区地址（ptr），供调用方 CopyStruct 到目标。
        //   087（m85 返回面）：定长数组返回（整32[3]）同通道——被调 ir_decl 已置
        //   structReturn（隐藏指针+epilogue 按 structReturnSize 拷贝）·调用点
        //   原 isStructType 判定不含数组=retbuf 不预插→被调读空 retbuf 段错误
        //   （m85c 实录 RAX=0）。resultType 显式升 ptr（mapType 数组塌缩 i32·
        //   不动 mapType 本体——数组名退化=元素指针模型全树依赖元素类型）。
        const std::string retSrc087 = semantic_ != nullptr
                                          ? semantic_->funcReturnTypeOf(calleeName)
                                          : std::string();
        const bool isArrayRet087 = semantic_ != nullptr && types::isArray(retSrc087);
        if (isArrayRet087) resultType = "ptr";
        if (resultType == "ptr" && semantic_ != nullptr &&
            (semantic_->isStructType(types::canonical(
                 semantic_->funcReturnTypeOf(calleeName))) ||
             isArrayRet087)) {
            // 缓冲构造=allocAggRetbufBuf（936 提炼·公共单点）：槽数经
            //   registerVarSlots 按源码类型实际大小登记（原「64 字节 8 槽」
            //   固定假设=被调 epilogue 精确大小拷贝（structReturnSize）时代
            //   的安全面欠账，936 归纳时统一为多槽登记）。
            ir::IRValue buf = allocAggRetbufBuf(
                semantic_->funcReturnTypeOf(calleeName), node->location);
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
            // 616（2026-10-14 伞批）：字符串释放(变量槽) 释放后清零——「释放+清零
            //   幂等模型」（74-a 纪律·emitStringFreeFor 同款）：块出口/返回块
            //   兜底再经过该槽时 __cn_str_free(nullptr) 空安全跳过，杜绝「显式
            //   释放 + RAII 兜底」裸 double free（原靠 runtime cn_free_tracked
            //   注册表兜底侥幸安全）。消耗形参体内显式释放后出口释放同理受益。
            if (calleeName == "__cn_str_free" && !node->arguments.empty() &&
                node->arguments[0]->getType() == NodeType::IdentifierExpr) {
                const std::string freedUnique = lookupVarName(
                    static_cast<IdentifierExpr*>(node->arguments[0].get())->name);
                if (!freedUnique.empty()) {
                    ir::IRValue zero616 = emitResult(
                        ir::Opcode::ConstInt, {}, "i64", "0", node->location);
                    emit(ir::Opcode::Store, {zero616}, ir::IRValue(),
                         freedUnique, "ptr", node->location);
                }
            }
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
        // 617：消费族调用发射完成——内容已拷贝/只读消费毕，结算拥有串临时实参
        //   （662：三元实参走分支拥有性感知条件释放·单一入口 Rec617）
        for (const auto& tmp617 : consumedArgTemps617) {
            emitOwnedTempReleaseRec617(tmp617.first, tmp617.second,
                                       node->location);
        }
        return;
    }
    // 337-a（T53 家系·间接调用路径）：i128/u128 形参的窄整实参宽化——函数指针
    //   调用（如 `整32(*指)(整128)` 的 `指(200000000000)`）原缺宽化 → 字面量实参
    //   物理 i64 直传 → 被调方按 i128 指针 ABI 解引用 SIGSEGV（探针 p_fnptr rc=139）。
    //   形参列表来自 callee 表达式的**源码类型串**（`函数指针<返回>(参数,...)`，
    //   与语义层 funcPtrParamsOf 同口径、同一规范化格式）——声明位/参数位
    //   登记时即写完整串（「登记字面量 `函数指针` 丢形参」=半截机制的根治）。
    //   树形态覆盖：标识符（局部变量/函数指针参数/全局）、成员访问（对象字段
    //   函数指针）；无法解析出形参列表时保守跳过（零行为变更）。
    // 485（间接调用返回面根治·探针 p8/p10 C0000005 实锤）：callee 源码类型串
    //   单次解析（funcPtrTypeOfCallee），形参宽化与返回类型同源消费——原返回
    //   类型硬编码 "i32"：i128 返回=调用点不传 sret 隐藏指针且按 eax 取值，
    //   蹦床（432 ?lambdaN$trap）按 sret 协议写 rcx 垃圾地址=访问违例 C0000005
    //   （[=] 闭包经返回真逃逸后跨函数调用立案形态·#435 轮探针）；整64 返回=
    //   eax 低 32 截断（10000000000→1410065408 实测）；浮点返回=eax 垃圾；
    //   结构体/数组返回=retbuf 预插缺失。三后端 emitCall 均按 result.type
    //   分派（i128 sret/浮点 xmm/整型位宽）——IR 层一处修三后端同受益。
    //   解析失败保守兜底 "i32"（现状零行为变更）。
    // 442：callee 源码类型串在循环前已单次解析（calleeFnPtrType442·实参定标
    //   判据源同源消费）——此处直接复用，不重复解析。
    const std::string& calleeFnPtrType = calleeFnPtrType442;
    if (!calleeFnPtrType.empty()) {
        const std::vector<std::string> fnPtrParams =
            types::funcPtrParamsOf(calleeFnPtrType);
        if (!fnPtrParams.empty() && fnPtrParams.size() == args.size()) {
            widenI128Args(args, fnPtrParams, node->location);
        }
        // 结构体/数组返回 fnptr：形态A同构（直接调用 087 同款）——retbuf 预插，
        //   operands=[fnptr, retbuf, 实参...]（operand[0]=call 目标·operand[1]=
        //   隐藏返回指针落 rcx），result=void，lastExpr_=retbuf 地址（调用方
        //   CopyStruct 消费与直接调用同链）。
        const std::string retSrc = types::funcPtrReturnTypeOf(calleeFnPtrType);
        if (retSrc.empty()) {
            // 648 miss 不可静默：函数指针类型串已解析但返回类型未登记——原静默
            //   落终态 "i32"（上方 801 行注释自认保守兜底；485 修的本链已登记
            //   面，此报错收口未登记面）。报诊断后保留 i32 兜底与控制流
            //   （driver generate() 后 hasErrors() 中止）。
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "内部错误: 函数指针返回类型未登记 '" + calleeFnPtrType + "'（#648）");
        }
        if (!retSrc.empty() && semantic_ != nullptr &&
            (semantic_->isStructType(types::canonical(retSrc)) ||
             types::isArray(retSrc))) {
            ir::IRValue calleeVal = genExpr(node->callee.get());
            const std::string temp = "__retbuf" + std::to_string(varCounter_++);
            emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
                 temp, "ptr", node->location);
            // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 的 64 字节
            //   拷贝上限一致（结构体实际大小 ≤64 字节场景）
            function_->varSlots[temp] = 8;
            registerVarSlots(temp, retSrc);
            ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(temp, "i64")},
                                         "ptr", temp, node->location);
            args.insert(args.begin(), buf);        // operand[1]=隐藏返回指针
            args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
            emit(ir::Opcode::CallIndirect, args, ir::IRValue(), "", "void",
                 node->location);
            lastExpr_ = buf;
            return;
        }
        // 标量返回：返回类型按 fnptr 声明映射（空类型维持 "i32" 兜底——result
        //   无消费者·eax 装载无害·零行为变更）
        if (!retSrc.empty()) {
            const std::string mapped = mapType(retSrc);
            if (mapped != "void" && mapped != "") {
                ir::IRValue calleeVal = genExpr(node->callee.get());
                args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
                lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, mapped, "",
                                       node->location);
                return;
            }
        }
    }
    // 间接调用：先求被调者表达式（函数指针变量），再 CallIndirect
    // 655（648 余量②收口）：静态槽 fnptr 豁免已由 funcPtrTypeOfCallee 窄放宽
    //   根治。655 返工（720/859/939/942 连锁红根治）：本尾到达条件实为四种，
    //   原诊断把整个掉穿面误当全域 miss 面报错，现仅①报——
    //   ①calleeFnPtrType 空且非闭包变量=真全域 miss（签名终态缺失·静默错值
    //     面）→ 显形（探针 t1~t4 实证解析链无缺口·形参/局部/静态登记全通）；
    //   ②返回=空类型（retSrc 非空·mapped=="void" 不 return 掉穿）=合法面：
    //     result 无消费者·i32 兜底 eax 装载无害（stdlib/测试框架.cn:55 实锤·859/939/942 红根因）；
    //   ③retSrc 空已由上方 #648 报过，再报=双报；
    //   ④lambda 变量但 closureInfo_ 快径 miss（lambdaVarNames_ 留痕）=蹦床通道
    //     合法面：432 lambda 值=蹦床地址·CallIndirect 与快径语义等价——快径受
    //     visitLambdaExpr 入口 clear（485）影响（720 实录·取和走蹦床正确）。
    //   各形态发射同链零行为变化；若合法程序落①=新 miss 形态，须补签名解析
    //   而非回撤诊断（110/861-a 同教训；v2 侧 发射间接调用480 无此病）。
    const bool lambdaTrampoline655r = lambdaVarNames_.count(calleeName) > 0;
    if (calleeFnPtrType.empty() && !lambdaTrampoline655r) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 间接调用被调者类型串未解析（funcPtrTypeOfCallee 全域 miss·"
            "#655）");
    }
    ir::IRValue calleeVal = genExpr(node->callee.get());
    args.insert(args.begin(), calleeVal);  // operand[0]=指针寄存器
    lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, "i32", "", node->location);
}


} // namespace cn_compiler
