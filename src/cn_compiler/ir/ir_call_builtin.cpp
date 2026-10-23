// CN-IR生成器实现：调用内置族（954 拆族：自 ir_call.cpp visitCallExpr 巨函数
//   纯机械搬移·逐字保留零语义变化；函数≤100 行/文件<900 行纪律）
//   族 = 转移展开（emitTransferCallExpand + 结构体源物化 emitTransferStructSourceCopy）
//   成员函数声明仍在 ir_generator.hpp（954 拆分=成员实现搬移）。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ---- plans/019 阶段1（2026-09-10）：显式转移 转移(变量) 展开 ----
// 语义层已检查放行（声明初始化位在 visitVarDecl 已改写为标识符，不会到这；
// 此处=表达式位指针/字符串值交接）。展开=实参标识符的值加载（转移() 零运行
// 时指令，纯编译期标记语义）。resolvedType 非空且 callee 为 转移 双重判定，
// 防用户经占位注册签名外的同名调用误入。
// 954（visitCallExpr 拆族·机械搬移）：展开体自 visitCallExpr 转移块逐字迁出，
//   零语义变化（调用点保留 lvalueAddress/求值序完全一致）。
void IRGenerator::emitTransferCallExpand(CallExpr* node) {
    (void)genExpr(node->arguments[0].get());  // lastExpr_=实参求值结果（句柄直拷）
    // plans/022 波 4 首件（459-a·206-d）：源槽清零（moved-from 标记到 IR 槽位）
    //   ——所有权已移交接收位；源变量 RAII 析构对零句柄走既有空安全跳过
    //   （字符串 __cn_str_free(nullptr) 忽略 / 容器 __cn_vector_free_strings
    //   obj==nullptr return / 类 DeleteObject test/je 空安全）——「释放+清零
    //   幂等模型」同构（74-a 纪律 7·与声明初始化位 ir_stmt_decl 浅交接分派
    //   同构）。原实现缺此步：实参位转移后源槽仍持句柄，双 free 靠
    //   cn_free_tracked 注册表兜底侥幸安全（结构性正确升级）。
    // 959（008 总攻·转移位余二②）：**成员源**（乙 = 转移(h.桶)）——句柄
    //   加载（成员地址→LoadPtr）+**成员槽清零**（成员地址 StorePtr 0——
    //   宿主所有者出口对清零成员空安全跳过=所有权交接）。成员地址=
    //   lvalueAddress（标识符/成员链基座通用）。
    if (node->arguments[0]->getType() == NodeType::MemberExpr) {
        ir::IRValue memAddr959 = lvalueAddress(
            node->arguments[0].get());
        if (memAddr959.id >= 0 || memAddr959.isConstant) {
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {memAddr959},
                                   "ptr", "", node->location);
            emit(ir::Opcode::StorePtr,
                 {memAddr959, ir::IRValue::constant("0", "i64")},
                 ir::IRValue(), "", "ptr", node->location);
        }
        return;
    }
    if (node->arguments[0]->getType() == NodeType::IdentifierExpr) {
        const std::string srcName =
            static_cast<IdentifierExpr*>(node->arguments[0].get())->name;
        const cn::Optional<std::string> srcUniqueOpt = lookupVarName(srcName);
        if (srcUniqueOpt.has_value()) {
            const std::string& srcUnique = srcUniqueOpt.value();
            const std::string srcCanon =
                types::canonical(lookupSrcType(srcName).value_or(""));
            // 结构体源=先物化副本再清零源(族体见 emitTransferStructSourceCopy·096)
            if (semantic_ != nullptr && semantic_->isStructType(srcCanon)) {
                emitTransferStructSourceCopy(node, srcUnique, srcCanon);
                return;
            }
            // 096（937）：数组源**跳过清零**——数组名=退化借用指针（C 聚合
            //   语义·栈上无单句柄所有权），清首槽=破坏栈聚合内容（p9 实测
            //   转移(组) 后读 0·撤改动基线同崩=459-a 既有缺陷）；数组传参
            //   本就退化为按址共享（零拷贝天然成立·转移 无额外增益·语义
            //   层 459-a 放行面维持统一语法）。
            // 096（937）：数组源跳过清零（数组名=退化借用指针·清首槽=破坏
            //   栈聚合内容）。1360 结果化：value_or("") 显形（miss=非数组·原语义）
            if (semantic_ != nullptr &&
                types::isArray(types::canonical(
                    lookupSrcType(srcName).value_or("")))) {
                return;
            }
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique,
                 "i64", node->location);
        }
    }
    return;
}

// 096（937·008 收官总攻第三轮）：结构体源=**先物化副本再清零源**
//   （078 LUE 同款模式：复槽分配+CopyStruct 整块位拷〔拥有字段
//   句柄直移〕+lastExpr_=副本基址+源槽全清零）——实参消费方按
//   槽地址读结构体（清零后读源=0·p7 形态2 实测 0 实锤）。标量/
//   类/容器/字符串=值物化天然（句柄在寄存器）·单槽清零（459-a
//   现行·幂等模型：源 RAII 对零句柄空安全跳过）。
// 954（visitCallExpr 拆族）：结构体源物化子段（原 L73-107）逐字迁出。
void IRGenerator::emitTransferStructSourceCopy(CallExpr* node,
                                               const std::string& srcUnique,
                                               const std::string& srcCanon) {
    ir::IRValue srcAddr = emitResult(
        ir::Opcode::AddrOf, {ir::IRValue::var(srcUnique, "i64")},
        "ptr", srcUnique, node->location);
    const std::string tmp =
        "__xfertmp" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {},
         ir::IRValue::reg(regCounter_++, "ptr"), tmp, "ptr",
         node->location);
    registerVarSlots(tmp, srcCanon);
    ir::IRValue tmpAddr = emitResult(
        ir::Opcode::AddrOf, {ir::IRValue::var(tmp, "i64")},
        "ptr", tmp, node->location);
    const int bytes = semantic_->typeSizeOf(srcCanon).value_or(8);
    if (bytes > 0) {
        emit(ir::Opcode::CopyStruct, {tmpAddr, srcAddr},
             ir::IRValue(), std::to_string(bytes), "void",
             node->location);
    }
    auto slotIt = function_->varSlots.find(srcUnique);
    const int slots =
        (slotIt != function_->varSlots.end() &&
         slotIt->second > 0) ? slotIt->second : 1;
    for (int s = 0; s < slots; ++s) {
        const std::string slotName =
            s == 0 ? srcUnique
                   : srcUnique + "$s" + std::to_string(s);
        ir::IRValue zero = emitResult(
            ir::Opcode::ConstInt, {}, "i64", "0",
            node->location);
        emit(ir::Opcode::Store, {zero}, ir::IRValue(), slotName,
             "i64", node->location);
    }
    lastExpr_ = tmpAddr;
}


// ---- 任务 474：回绕内置 回绕加/回绕减/回绕乘（显式两补码回绕算术）----
// 语义层已检查放行并推导 resolvedType（=与 + - * 完全一致的类型规则，
// 含 285 窄域字面量豁免）。展开=两实参 Cast 对齐公共宽度 + WrapAdd/WrapSub/
// WrapMul 指令（发射层无溢出检查——回绕语义的显式豁免面）。
// 954（visitCallExpr 拆族·机械搬移）：整体迁出;命中修饰=return true（调用点即 return）。
bool IRGenerator::emitWrapBuiltinCall(CallExpr* node) {
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
            return true;
        }
    }
    return false;
}

// ---- 打印 / 打印行 展开（Task 2.5 + Task 2.9 语义调整）----
// 新语义（用户裁决，lessons.md 权重10.4）：
//   打印   = println（末尾自动换行）——运行时 printLine/__cn_print_* + newline
//   打印行 = print（末尾不换行）——运行时 __cn_print_*（fputs 语义，无换行）
// 统一展开：逐参数按类型调用运行时打印函数（字符串/整数/浮点/i128/布尔），
//   打印 最后加 __cn_print_newline；打印行 不加。
// 注意：打印函数返回 void，用 emit 直接发射（不分配结果寄存器，
//       codegen 对 id<0 的结果不生成返回值存储）
// 954（visitCallExpr 拆族·机械搬移）：判定+发射薄封装(打印族展开体=852 emitPrintLikeCall)。
bool IRGenerator::tryEmitPrintLikeCall(CallExpr* node, bool isDirect,
                                       const std::string& calleeName) {
    bool isPrintNewline = (isDirect && calleeName == "打印");      // 打印：换行
    bool isPrintNoLine = (isDirect && calleeName == "打印行");     // 打印行：不换行
    if (isPrintNewline || isPrintNoLine) {
        // 852 收工解堵（2026-10-16）：打印族展开迁出 emitPrintLikeCall
        //   （visitCallExpr 983 行巨函数首刀瘦身——纯机械搬移零语义；
        //   单函数 ≤100 行纪律的既有欠账另案清偿）
        emitPrintLikeCall(node, isPrintNewline, isPrintNoLine);
        return true;
    }
    return false;
}

// 617（2026-10-14 伞批）：内置字符串消费族判定（映射前中文名）——分配族
//   拷贝内容、谓词族只读、格式化 sprintf 拷贝，均不收下实参句柄=调用完成后
//   其「拥有串临时」实参（argIsOwnedStringTemp）可安全结算。拼接链中间临时
//   同点归零：t1=连接(a,b) 作为 连接(t1,c) 实参时在内层发射完释放。
//   驻留（表可能持指针视图）保守不入集合。
// 954（visitCallExpr 拆族·机械搬移）：判定表达式提为函数返回值（逐字）。
bool IRGenerator::isBuiltinStrConsumer617(const std::string& calleeName,
                                         bool isDirect) const {
    return isDirect && (calleeName == "字符串长度" || calleeName == "字符串比较" ||
                        calleeName == "字符串连接" || calleeName == "字符串复制" ||
                        calleeName == "字符串查找" || calleeName == "字符串子串" ||
                        calleeName == "字符串字典序" || calleeName == "字符串大写" ||
                        calleeName == "字符串小写" || calleeName == "字符串前缀" ||
                        calleeName == "字符串后缀" || calleeName == "字符串包含" ||
                        calleeName == "字符串修剪" || calleeName == "字符串反转" ||
                        calleeName == "字符串拼接" || calleeName == "格式化");
}

// 954（visitCallExpr 拆族·机械搬移）：内置名映射段一（字符串API/驻留/格式化/
//   数学/运行时错误/IO/文件）——原单链按段机械二拆;两段名字集互斥=恰一命中语义不变。
void IRGenerator::mapBuiltinCalleeNameApi(std::string& calleeName) {
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
    //   文件::文件存在 -> __cn_file_exists、文件::创建目录 -> __cn_file_mkdir
    //   （file_api.cpp）
    else if (calleeName == "文件::打开文件" || calleeName == "文件.打开文件") calleeName = "__cn_file_open";
    else if (calleeName == "文件::读取文件" || calleeName == "文件.读取文件") calleeName = "__cn_file_read";
    else if (calleeName == "文件::写入文件" || calleeName == "文件.写入文件") calleeName = "__cn_file_write";
    else if (calleeName == "文件::读取文件行" || calleeName == "文件.读取文件行") calleeName = "__cn_file_read_line";
    else if (calleeName == "文件::文件大小" || calleeName == "文件.文件大小") calleeName = "__cn_file_size";
    else if (calleeName == "文件::关闭文件" || calleeName == "文件.关闭文件") calleeName = "__cn_file_close";
    else if (calleeName == "文件::文件存在" || calleeName == "文件.文件存在") calleeName = "__cn_file_exists";
    else if (calleeName == "文件::创建目录" || calleeName == "文件.创建目录") calleeName = "__cn_file_mkdir";
}

// 954（visitCallExpr 拆族）：内置名映射段二（线程/解析/时间/系统/内存/构建器）——
//   段首 else if -> if（链断开点;前段已描述语义不变）。
void IRGenerator::mapBuiltinCalleeNameLib(std::string& calleeName) {
    // 996（027 波 1·线程库）：线程::新 -> __cn_thread_new、线程::并入 ->
    //   __cn_thread_join、线程::互斥锁新建/上锁/解锁/释放 -> __cn_mutex_*
    //   （thread_api.cpp；linux=pthread 真实现·win=错误码 9 桩随波 3）
    if (calleeName == "线程::新" || calleeName == "线程.新") calleeName = "__cn_thread_new";
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

// 954（visitCallExpr 拆族·机械搬移）：内置符号结果类型表——原 visitCallExpr
//   内置分派链逐字迁出（赋值分支写 resultType;空体分支保持 i32=挡语义层查询）;
//   链尾原「else if (semantic_ != nullptr) { 用户查询 }」替换为「else { hit=false; }」
//   （用户查询移交调用点·两级判据等价:未命中内置且语义层在场才查语义层）。
//   命中（含空体分支）=true;未命中=false。
bool IRGenerator::resolveBuiltinCallResultType(const std::string& calleeName,
                                               std::string& resultType) {
    bool hit = true;
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
    } else if (calleeName == "__cn_file_mkdir") {
        resultType = "i1";        // 创建目录 -> 布尔（1=成功/已存在 0=失败）
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
    } else {
        hit = false;
    }
    return hit;
}

} // namespace cn_compiler
