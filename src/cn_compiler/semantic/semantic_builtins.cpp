// CN 语义分析器实现（D1 行数整改 117-a：自 semantic.cpp 按族拆出）
//   族 = 内置函数注册（registerBuiltins + registerFunction + isExternFunc）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp；
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

void SemanticAnalyzer::registerBuiltins() {
    // ---- 打印 系列（Task 2.9 语义调整）----
    // 打印：println 语义（自动换行，运行时 printLine）；打印行：print 语义（不换行，运行时 printNoLine）
    // 变参：参数个数不限（IR 层逐参数展开为 __cn_print_* 序列，打印 末尾加换行）
    const auto regPrintFn = [this](const std::string& name) {
        FunctionInfo info;
        info.returnType = "空类型";
        info.paramTypes = {"字符串"};
        info.hasBody = true;
        info.variadic = true;  // 参数个数不限（visitCallExpr 特判展开）
        functions_[name] = info;
    };
    regPrintFn("打印");
    regPrintFn("打印行");

    // ---- 格式化（Task 2.9，规格书10.6）：格式化(格式字符串, 参数...) -> 字符串 ----
    // 变参：参数个数不限（IR 层按占位符展开为 __cn_format 调用）
    FunctionInfo formatInfo;
    formatInfo.returnType = "字符串";
    formatInfo.paramTypes = {"字符串"};
    formatInfo.hasBody = true;
    formatInfo.variadic = true;
    functions_["格式化"] = formatInfo;

    // ---- 字符串API（Task 2.5，规格书10.1 字符串操作：长度/比较/连接/复制/查找） ----
    // 运行时符号：字符串长度 -> __cn_str_len、字符串比较 -> __cn_str_eq、
    //           字符串连接 -> __cn_str_concat、字符串复制 -> __cn_str_copy、
    //           字符串查找 -> __cn_str_find（IR 层按函数名映射）
    FunctionInfo strLenInfo;
    strLenInfo.returnType = "整64";
    strLenInfo.paramTypes = {"字符串"};
    strLenInfo.hasBody = true;
    functions_["字符串长度"] = strLenInfo;

    FunctionInfo strEqInfo;
    strEqInfo.returnType = "布尔";
    strEqInfo.paramTypes = {"字符串", "字符串"};
    strEqInfo.hasBody = true;
    functions_["字符串比较"] = strEqInfo;

    FunctionInfo strConcatInfo;
    strConcatInfo.returnType = "字符串";
    strConcatInfo.paramTypes = {"字符串", "字符串"};
    strConcatInfo.hasBody = true;
    functions_["字符串连接"] = strConcatInfo;

    FunctionInfo strCopyInfo;
    strCopyInfo.returnType = "字符串";
    strCopyInfo.paramTypes = {"字符串"};
    strCopyInfo.hasBody = true;
    functions_["字符串复制"] = strCopyInfo;

    FunctionInfo strFindInfo;
    strFindInfo.returnType = "整64";
    strFindInfo.paramTypes = {"字符串", "字符串"};
    strFindInfo.hasBody = true;
    functions_["字符串查找"] = strFindInfo;

    // ---- 字符串驻留（自举重建 P1，2026-08-25；对标 rustc Symbol / LLVM StringPool） ----
    // 符号名/类型名/标识符 经 驻留 得唯一 整64 ID（相同内容同 ID，内容只存一份）。
    // Token/AST/IR/符号表 用 ID 引用——比较/哈希 O(1)，消除百万级重复字符串分配。
    // 运行时符号（IR 层映射）：驻留 -> __cn_intern、驻留文本 -> __cn_intern_text、
    //   驻留计数 -> __cn_intern_count。
    FunctionInfo internInfo;
    internInfo.returnType = "整64";
    internInfo.paramTypes = {"字符串"};
    internInfo.hasBody = true;
    functions_["驻留"] = internInfo;

    FunctionInfo internTextInfo;
    // plans/019 阶段4' A2（2026-09-11 方案甲）：返回类型 字符串->字符*——
    //   驻留文本 返回驻留表内部指针=借用视图（拥有→借用安全方向自动隐式）；
    //   原 字符串 返回在 A2 拥有契约下调用方会登记 free=释放驻留表条目（灾难）。
    //   调用方须持有时显式 字符串复制(驻留文本(...)) 落堆（Rust intern.get().to_string() 同款）。
    internTextInfo.returnType = "字符*";
    internTextInfo.paramTypes = {"整64"};
    internTextInfo.hasBody = true;
    functions_["驻留文本"] = internTextInfo;

    FunctionInfo internCountInfo;
    internCountInfo.returnType = "整64";
    internCountInfo.paramTypes = {};
    internCountInfo.hasBody = true;
    functions_["驻留计数"] = internCountInfo;

    // ---- 补充字符串API（Task 2.8，规格书10.1 标注"常见字符串库补充"） ----
    // 运行时符号：字符串子串 -> __cn_str_sub、字符串字典序 -> __cn_str_cmp、
    //           字符串大写 -> __cn_str_upper、字符串小写 -> __cn_str_lower、
    //           字符串前缀 -> __cn_str_starts_with、字符串后缀 -> __cn_str_ends_with、
    //           字符串包含 -> __cn_str_contains、字符串修剪 -> __cn_str_trim、
    //           字符串反转 -> __cn_str_reverse、整数转字符串 -> __cn_str_from_int、
    //           浮点转字符串 -> __cn_str_from_float、字符转字符串 -> __cn_str_from_char、
    //           布尔转字符串 -> __cn_str_from_bool、正数转字符串 -> __cn_str_from_uint、
    //           字符串释放 -> __cn_str_free（IR 层按函数名映射）
    // 内存语义：返回动态内存（子串/大写/小写/修剪/反转/整数转/浮点转/字符转/正数转），
    //           调用方负责用 字符串释放 释放；连接/复制沿用 Task 2.5 语义。
    // 注：字符串比较运算符（==/!=/</> 等）规格书未定义字符串变体（运算符表仅整型
    //     与浮点变体），不实现；等价能力由 字符串比较（相等）与 字符串字典序 提供。
    const auto regStrFn = [this](const std::string& name, const std::string& retType,
                                 const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regStrFn("字符串子串", "字符串", {"字符串", "整64", "整64"});
    regStrFn("字符串字典序", "整64", {"字符串", "字符串"});
    regStrFn("字符串大写", "字符串", {"字符串"});
    regStrFn("字符串小写", "字符串", {"字符串"});
    regStrFn("字符串前缀", "布尔", {"字符串", "字符串"});
    regStrFn("字符串后缀", "布尔", {"字符串", "字符串"});
    regStrFn("字符串包含", "布尔", {"字符串", "字符串"});
    regStrFn("字符串修剪", "字符串", {"字符串"});
    regStrFn("字符串反转", "字符串", {"字符串"});
    regStrFn("整数转字符串", "字符串", {"整64"});
    regStrFn("浮点转字符串", "字符串", {"浮64"});
    regStrFn("字符转字符串", "字符串", {"字符"});
    regStrFn("布尔转字符串", "字符串", {"布尔"});   // Task 2.9：布尔转"真"/"假"（拼接上下文）
    regStrFn("正数转字符串", "字符串", {"正64"});
    regStrFn("字符串释放", "空类型", {"字符串"});

    // ---- 数学库（Task 6.3，规格书10.5 数学库；对应运行时 math_api.cpp）----
    // 第 4 层（v2.0 决策6）：内置 key `::` 化——"数学." -> "数学::"（路径分隔
    //   由 v1.0 的 . 改为 ::）。限定名作为**核心 包 prelude 成员**（规格书08-六）：
    //   - 与 CN 层模块 stdlib/数学.cn 的公开函数（纯名 平方根 等）不冲突——
    //     模块函数注册为纯名（公开符号合并），内置函数注册为 :: 限定名
    //   - 调用方式 数学::平方根(值)（v2.0）或旧 数学.平方根(值)（第 6 层迁移前
    //     兼容）：visitCallExpr 模块限定重写时，对已注册的 数学::* 内置名特判：
    //     不重写为纯名，保留限定名走内置函数路径（prelude 无需显式导入）
    // 运行时符号：数学::平方根 -> __cn_sqrt、数学::幂 -> __cn_pow、
    //   数学::正弦 -> __cn_sin、数学::余弦 -> __cn_cos、数学::正切 -> __cn_tan、
    //   数学::绝对值 -> __cn_fabs、数学::向上取整 -> __cn_ceil、
    //   数学::向下取整 -> __cn_floor（IR 层按函数名映射）
    // 参数/返回均为 浮64（double）；P1 的对数/反三角/随机数留待后续
    const auto regMathFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMathFn("数学::平方根", "浮64", {"浮64"});
    regMathFn("数学::幂", "浮64", {"浮64", "浮64"});
    regMathFn("数学::正弦", "浮64", {"浮64"});
    regMathFn("数学::余弦", "浮64", {"浮64"});
    regMathFn("数学::正切", "浮64", {"浮64"});
    regMathFn("数学::绝对值", "浮64", {"浮64"});
    regMathFn("数学::向上取整", "浮64", {"浮64"});
    regMathFn("数学::向下取整", "浮64", {"浮64"});

    // ---- IO 输入（Task 6.2，规格书10.6 输入 API；对应运行时 input_api.cpp）----
    // 中文名带 "IO." 前缀（形如 模块.函数 限定名），与 stdlib/IO.cn 模块公开函数
    // （纯名 读取行 等）不冲突——模块函数走"公开符号合并"，内置走"限定名直调"。
    // 运行时符号：IO.读取行 -> __cn_read_line、IO.读取整数 -> __cn_read_int、
    //   IO.读取浮点 -> __cn_read_float、IO.打印到错误 -> __cn_print_err（IR 层映射）。
    // 设计说明：
    //   - 读取行 返回 字符串*（动态分配，EOF/失败返回 无/空指针）——CN 层 stdlib/IO.cn
    //     用 结果<字符串,整32> 包装（nullptr 判定 → 错误码.文件 5 / EOF 特殊语义）
    //   - 读取整数/读取浮点 为 C 风格成功标志：参数 (整32* 成功标志)，返回整64/浮64——
    //     CN 层 stdlib/IO.cn 用 &成功 传参，失败返回 错误(错误码.参数)（非 EOF 语义）
    //   - 打印到错误：单字符串参数，不换行（fprintf stderr，与 打印行 不换行语义一致）
    const auto regIoFn = [this](const std::string& name, const std::string& retType,
                                const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regIoFn("IO::读取行", "字符串", {});
    regIoFn("IO::读取整数", "整64", {"整32*"});
    regIoFn("IO::读取浮点", "浮64", {"整32*"});
    regIoFn("IO::打印到错误", "空类型", {"字符串"});

    // ---- 文件 API（Task 6.2，规格书阶段五「文件系统」；对应运行时 file_api.cpp）----
    // 中文名带 "文件." 前缀，与 stdlib/文件.cn 模块公开函数不冲突。
    // 运行时符号：文件.打开文件 -> __cn_file_open、文件.读取文件 -> __cn_file_read、
    //   文件.写入文件 -> __cn_file_write、文件.读取文件行 -> __cn_file_read_line、
    //   文件.文件大小 -> __cn_file_size、文件.关闭文件 -> __cn_file_close、
    //   文件.文件存在 -> __cn_file_exists（IR 层映射）。
    // 设计说明：
    //   - 句柄类型：空类型*（void*，C 层 FILE* 转换）
    //   - 打开文件：模式 整32（1=读/2=写/3=追加），返回 空类型*（失败 nullptr）
    //   - 读取文件/写入文件/文件大小：返回整64（实际字节数/大小，失败 -1）
    //   - 读取文件行：返回 字符串*（动态分配，EOF 返回 nullptr）——CN 层包装 结果<字符串,整32>
    //   - 文件存在：返回 布尔
    const auto regFileFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regFileFn("文件::打开文件", "空类型*", {"字符串", "整32"});
    regFileFn("文件::读取文件", "整64", {"空类型*", "字符*", "整64"});
    regFileFn("文件::写入文件", "整64", {"空类型*", "字符串"});
    regFileFn("文件::读取文件行", "字符串", {"空类型*"});
    regFileFn("文件::文件大小", "整64", {"空类型*"});
    regFileFn("文件::关闭文件", "空类型", {"空类型*"});
    regFileFn("文件::文件存在", "布尔", {"字符串"});

    // ---- 字符串扩展库（Task 6.5，对标 C++ string 解析；对应运行时 string_api.cpp）----
    // 中文名带 "解析." 前缀（形如 模块.函数 限定名），与 stdlib/字符串扩展.cn
    // 模块公开函数（纯名 字符串转整数 等）不冲突——内置走"限定名直调"（IO 库同模式）。
    // 注意：不能用 "字符串." 前缀——字符串 是类型关键字（Kw_String），词法器将
    //   "字符串.转整数" 拆为 关键字+标识符 报"预期表达式"（lessons 变量名前缀同类）。
    // 运行时符号：解析.转整数 -> __cn_str_to_int、解析.转浮点 -> __cn_str_to_double、
    //   解析.转布尔 -> __cn_str_to_bool（IR 层映射）。
    // 设计说明（Task 6.2 IO 库同模式）：
    //   - 三个解析函数均带 整32* 成功标志输出参数（1=成功，0=非法输入/空串/范围错误）
    //   - 返回整64/浮64/整64（布尔 0/1）；失败返回 0/0.0——CN 层 stdlib/字符串扩展.cn
    //     用 &成功 传参，失败返回 错误(6)
    //   - 与现有 整数转字符串/浮点转字符串/布尔转字符串 命名对称（字符串转 前缀）
    const auto regStrExtFn = [this](const std::string& name, const std::string& retType,
                                    const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regStrExtFn("解析::转整数", "整64", {"字符串", "整32*"});
    regStrExtFn("解析::转浮点", "浮64", {"字符串", "整32*"});
    regStrExtFn("解析::转布尔", "整64", {"字符串", "整32*"});

    // ---- 时间库（Task 6.5，规格书10.4 时间；对应运行时 time_api.cpp）----
    // 中文名带 "时间." 前缀，与 stdlib/时间.cn 模块公开函数不冲突（数学库同模式）。
    // 运行时符号：时间.当前时间戳 -> __cn_time、时间.单调时钟毫秒 -> __cn_clock_ms、
    //   时间.格式化时间 -> __cn_time_format（IR 层映射）。
    // 设计说明：
    //   - 当前时间戳/单调时钟毫秒：零参数，返回整64（秒/毫秒）
    //   - 格式化时间：参数 (整64 时间戳, 字符串 格式)，返回 字符串*（动态分配，
    //     失败 nullptr）——CN 层 stdlib/时间.cn 包装 结果<字符串,整32>（nullptr→错误码.参数）
    const auto regTimeFn = [this](const std::string& name, const std::string& retType,
                                  const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regTimeFn("时间::当前时间戳", "整64", {});
    regTimeFn("时间::单调时钟毫秒", "整64", {});
    regTimeFn("时间::格式化时间", "字符串", {"整64", "字符串"});

    // ---- 内存库（自举前置 C-1/C-3，2026-08；对应运行时 io_api.cpp）----
    // 中文名带 "内存." 前缀（规格书10.2 内存管理扩展）。
    // 运行时符号（IR 层映射）：
    //   内存::活动分配数     -> __cn_alloc_live  （未释放块数，泄漏检测基线）
    //   内存::总分配次数     -> __cn_alloc_total（累计分配次数）
    //   内存::竞技场分配     -> __cn_arena_alloc（一次性进程 arena，bump 分配）
    //   内存::竞技场重置     -> __cn_arena_reset（释放全部块）
    //   内存::竞技场活动字节 -> __cn_arena_bytes（已分配总字节）
    // 设计说明：
    //   - 活动分配数/总分配次数：零参数，返回整64（原子计数）
    //   - 竞技场分配：参数 (整64 大小)，返回 空类型*（失败 nullptr）
    //   - 竞技场重置/竞技场活动字节：零参数，返回 空类型/整64
    const auto regMemLibFn = [this](const std::string& name, const std::string& retType,
                                    const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMemLibFn("内存::活动分配数", "整64", {});
    regMemLibFn("内存::总分配次数", "整64", {});
    regMemLibFn("内存::竞技场分配", "空类型*", {"整64"});
    regMemLibFn("内存::竞技场重置", "空类型", {});
    regMemLibFn("内存::竞技场活动字节", "整64", {});
    regMemLibFn("内存::释放全部", "空类型", {});

    // ---- 系统库（Task 6.5，规格书10.4 命令行参数；对应运行时 system_api.cpp）----
    // 中文名带 "系统." 前缀，与 stdlib/系统.cn 模块公开函数不冲突（数学库同模式）。
    // 运行时符号：系统.参数个数 -> __cn_argc、系统.参数 -> __cn_argv（IR 层映射）。
    // 设计说明：
    //   - 参数个数：零参数，返回整64（argc，含可执行文件名本身）
    //   - 参数：参数 (整64 索引)，返回 字符串*（CRT 持有，越界 nullptr）——CN 层
    //     stdlib/系统.cn 包装 结果<字符串,整32>（nullptr→错误码.参数）
    const auto regSysFn = [this](const std::string& name, const std::string& retType,
                                 const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regSysFn("系统::参数个数", "整64", {});
    // 70-a（2026-09-11 方案A 补完）：argv 由 CRT 持有=借用视图——返回类型
    //   字符串（A2 拥有契约下调用方 free=悬垂）改 字符*（A2 C 类纯借用改法，
    //   驻留文本 同款）；stdlib 系统.cn 包装层显式 复制 装箱
    regSysFn("系统::参数", "字符*", {"整64"});

    // ---- 内存管理API（Task 6.1 核心库/容器库，规格书10.2 内存管理）----
    // 运行时符号：分配 -> cn_alloc、释放 -> cn_free、重新分配 -> cn_realloc、
    //   复制内存 -> cn_memcpy、置零内存 -> cn_memset（codegen symbolName 已有映射）
    // 参数/返回：分配/重新分配 返回 空类型*（void*）；复制内存 三个指针参数；
    //   置零内存 指针 + 整64 大小；释放 单指针（空类型返回）。
    // 说明：此前 codegen 层已为这 5 个函数保留符号映射，但语义层未注册，
    //   导致 CN 源码无法调用——本子任务补齐注册（容器库 向量/链表 依赖）。
    const auto regMemFn = [this](const std::string& name, const std::string& retType,
                                 const std::vector<std::string>& paramTypes) {
        FunctionInfo info;
        info.returnType = retType;
        info.paramTypes = paramTypes;
        info.hasBody = true;
        functions_[name] = info;
    };
    regMemFn("分配", "空类型*", {"整64"});
    regMemFn("释放", "空类型", {"空类型*"});
    regMemFn("重新分配", "空类型*", {"空类型*", "整64"});
    regMemFn("复制内存", "空类型", {"空类型*", "空类型*", "整64"});
    regMemFn("置零内存", "空类型", {"空类型*", "整64"});

    // ---- 运行时错误（Task 6.1 核心库 断言 依赖，规格书10.2 错误处理）----
    // 中文内置函数 运行时错误(整64 错误码) -> 空类型：终止进程并打印错误消息。
    // 运行时符号 __cn_runtime_error（IR 层越界检查已用，codegen 有符号映射），
    // 此处补语义层注册使 CN 源码可调用（stdlib/核心.cn 的 断言 函数用）。
    // 注意：错误码 4~8 对应运行时错误表（1=除零/2=越界/3=空指针/4=内存/
    //   5=文件/6=无效参数/7=未初始化/8=溢出），断言失败用 6（无效参数）。
    FunctionInfo rtErrInfo;
    rtErrInfo.returnType = "空类型";
    rtErrInfo.paramTypes = {"整64"};
    rtErrInfo.hasBody = true;
    functions_["运行时错误"] = rtErrInfo;
}
void SemanticAnalyzer::registerFunction(FunctionDecl* node) {
    // 阶段3（Task 3.5）：内置构造器 正常/错误/某些 用户不可重定义
    if (node->name == "正常" || node->name == "错误" || node->name == "某些") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "内置构造器 '" + node->name + "' 不可重定义");
        return;
    }
    FunctionInfo info;
    // A-2（crate 分桶）：返回类型按所属模块解析——多模块同名类型改写为限定键
    //   （模块名::类型），IR 层按改写后的类型查询语义表（findStruct/typeSizeOf）
    if (!node->returnType.empty()) {
        node->returnType = resolveTypeName(node->returnType, node->moduleName,
                                           node->location);
    }
    info.returnType = node->returnType.empty() ? "空类型" : canonicalType(node->returnType);
    info.hasBody = (node->body != nullptr);
    // C-3（FFI）：外部 函数 声明——无函数体（C 符号由链接期解析）；
    //   有函数体属误用（外部=外部定义，禁止 CN 侧实现）
    info.isExtern = node->isExtern;
    // plans/019 阶段4：不安全 函数 修饰登记（安全区边界观察期检查用）
    info.isUnsafe = node->isUnsafe;
    if (node->isExtern && node->body != nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "外部 函数 声明不能有函数体（C 符号由外部库提供）");
    }
    // P3-18 补完（2026-08）：函数返回类型支持引用（T&）——返回被引用左值的地址。
    //   引用返回不参与重载签名（仅返回类型不同不构成重载）；isRefReturn 供 IR
    //   （返回类型映射 ptr）与调用方（get() = 值 写回 / 整32& r = get() 绑定 /
    //   &get() 取址）识别。返回的 lvalue 校验在 visitReturnStmt（禁返回局部变量地址）。
    if (!node->returnType.empty() && types::isReference(node->returnType)) {
        info.isRefReturn = true;
    }
    // 默认参数规则检查：从右向左连续声明（f(a=1, b) 非法——默认参数左侧出现无默认参数；
    //   f(a, b=1, c=2) 合法——最左侧参数可无默认）。
    // 正确判定：从左到右，一旦遇到无默认参数，其后所有参数都须无默认；
    //   即"第一个有默认参数的左侧参数都须有默认"（除最左侧参数）。
    // 简化实现：反向遍历，记录右侧是否已出现默认参数；若当前无默认且
    //   右侧已有默认 且 当前不是最左侧参数 -> 非法（左侧还有参数会继承默认？不——
    //   f(a=1, b) 中 a 有默认、b 无默认：反向看 b 无默认、a(右侧)有默认、b 不是最左 -> 合法？
    //   不对——f(a=1, b)：a 有默认，b 无默认，b 在 a 右侧（更右）应无默认，合法！
    //   f(a, b=1)：b 有默认，a 无默认，a 最左，合法！
    //   f(a=1, b, c=2)：c 有默认，b 无默认（右侧有 c 默认），b 非最左 -> 非法 ✓
    //   判定：反向遍历，遇到无默认参数时，若"已见默认"且"它左侧还有参数" -> 非法。
    // 默认参数规则：从右向左连续声明——反向扫描，一旦遇到无默认参数，
    //   其左侧（更左）不能再出现有默认参数（f(a=1, b) 非法：b 无默认在右，
    //   a 有默认在左被隔断；f(a, b=1) 合法：b 有默认最右、a 无默认最左）
    bool noDefaultSeen = false;
    for (auto it = node->params.rbegin(); it != node->params.rend(); ++it) {
        // A-1（引用参数）：引用参数不能有默认值（引用须绑定调用方左值，无法预置）
        if ((*it)->hasDefault && !(*it)->funcPtr.isFunctionPtr() &&
            types::isReference((*it)->typeName)) {
            diagnostics_.report(DiagnosticLevel::Error, (*it)->location,
                                "引用参数不能有默认值（引用须绑定调用方左值）");
        }
        if ((*it)->hasDefault) {
            ++info.defaultCount;
            info.hasDefault.push_back(true);
            if (noDefaultSeen) {
                diagnostics_.report(DiagnosticLevel::Error, (*it)->location,
                                    "默认参数必须从右向左连续声明（参数 '" + (*it)->name +
                                    "' 左侧不能出现无默认值参数）");
            }
        } else {
            info.hasDefault.push_back(false);
            noDefaultSeen = true;
        }
    }
    std::reverse(info.hasDefault.begin(), info.hasDefault.end());  // 恢复参数顺序
    for (auto& param : node->params) {
        // plans/019 阶段3：常量 只读引用参数位登记（与 paramTypes 等长）
        info.constParams.push_back(param->isConstParam);
        // 函数指针参数：整32(*func)(整32, 整32) 类型存规范化字符串
        if (param->funcPtr.isFunctionPtr()) {
            info.paramTypes.push_back(param->funcPtr.toString());
        } else {
            // A-2（crate 分桶）：参数类型按所属模块解析并改写（IR 层按改写后
            //   的类型查询语义表）；A-1 引用保留 &（整32&/账户&）——签名 key/
            //   mangling 区分 按值/按引用
            param->typeName = resolveTypeName(param->typeName, node->moduleName,
                                              param->location);
            // 自举前置 A-3a（plans/004）：泛型实例化类型参数（向量<字符串> 词表）
            //   ——模板形式归一为实例符号名（向量$字符串）并触发单态化，与类方法
            //   参数（class_resolver resolveGenericTypeName 路径）一致。此前签名
            //   key/参数槽/成员访问全程用模板形式，与构造调用产生的实例名不匹配：
            //   "预期参数名，实际为 '<'"（parseParamDecl 已修）+"类型 '向量<字符串>'
            //   不是类类型，无法访问成员 '大小'"。结果/可选 等合成模板不受影响
            //   （findGeneric 未命中 -> 原样返回）。
            param->typeName = resolveGenericTypeName(param->typeName, param->location);
            info.paramTypes.push_back(types::canonicalParam(param->typeName));
        }
    }
    // 生成签名 key（名 + "#" + 参数类型串，mangling 与决议共用）
    node->sigKey = signatureKey(node->name, info.paramTypes);
    // ---- crate 模型（第 4 层，v2.0 决策4）：重复定义按模块分桶 ----
    // 跨模块同名同签名函数允许（crate 隔离：包A::工具 与 包B::工具 独立符号）；
    // 仅同模块内重名报错。moduleName 由 mergeModules 合并阶段写入 FunctionDecl。
    info.moduleName = node->moduleName;
    // plans/018 呈报二 A′（2026-09-07 用户裁决）：注册键 = 函数链接键公式键。
    //   注册侧与定义侧（ir_decl mangledName）同源单一公式——消灭旧「首注册占
    //   裸键、后注册得 模块$键」的顺序依赖（依赖先注册抢走裸键时，入口同名
    //   函数纯名调用解析到 主$键 而定义侧发射裸键 → 链接 undefined reference，
    //   base3 探针汇编实证）。Rust 对照：rustc 符号=f(def-id)，定义时即定，
    //   「同名抢裸键」结构上不存在。
    const std::string linkKey =
        functionLinkKey(node->moduleName, node->name, node->sigKey);
    auto it = functions_.find(linkKey);
    if (it != functions_.end()) {
        // 同签名重名（模块分桶判定）：原型+定义 组合须同模块才配对；
        //   公式键已内嵌模块名（除 主/空模块 裸键形态），跨模块命中同一键
        //   = 裸键形态碰撞（两模块同名同签名函数均归一为裸键，如依赖模块
        //   定义了与入口同名的 主 函数）——链接符号必然冲突，fail fast 报错。
        const bool sameModule = (it->second.moduleName == node->moduleName);
        if (!sameModule) {
            // 跨模块同名同签名且公式键归一为同一裸键：链接层面不可共存
            //   （两定义同符号），诊断拒绝（绝不静默覆盖丢函数）
            funcSigModules_[node->sigKey].insert(node->moduleName);
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "函数 '" + node->name + "' 链接符号冲突（模块 '" +
                    it->second.moduleName + "' 与 '" + node->moduleName +
                    "' 同名同签名且链接键归一为裸键，无法共存）");
            return;
        }
        // 同签名重名：允许"原型声明 + 定义"组合，其余为重复定义
        bool isProtoPlusDef = !it->second.hasBody && info.hasBody;
        if (isProtoPlusDef) {
            // 签名一致性检查：原型（已注册）与定义（当前）签名必须一致
            const FunctionInfo& proto = it->second;
            bool same = (proto.returnType == info.returnType &&
                         proto.defaultCount == info.defaultCount);
            if (!same) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "函数 '" + node->name + "' 原型声明与定义签名不一致");
            }
        } else if (it->second.hasBody || info.hasBody) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "模块 '" + node->moduleName + "' 内重复定义函数 '" + node->name +
                                "'（参数类型相同；仅返回类型不同不构成重载）");
        }
        return;
    }
    // Task 2.10 重载兼容：签名 key 未命中但同名已有其他签名——
    //   合法重载（加(整32,整32) 与 加(浮64,浮64)）；
    //   但"原型声明 + 不同签名定义"是错误（原型已锁定签名，定义须一致）。
    //   规则：同名存在 原型（无体）且 当前是定义（有体）→ 签名必须与原型一致
    //   （仅同模块内检查——跨模块原型不约束其他模块的定义）。
    //   A′：键含公式前缀（模块名$名#参数）须先剥前缀再取同名 base（与
    //   resolveOverload 的形态判定一致）。
    for (const auto& kv : functions_) {
        std::string k = kv.first;
        const std::size_t hashFirst = k.find('#');
        const std::size_t dollarPos = k.find('$');
        if (dollarPos != std::string::npos &&
            (hashFirst != std::string::npos
                 ? dollarPos < hashFirst
                 : node->name.find('$') == std::string::npos)) {
            k = k.substr(dollarPos + 1);
        }
        const std::size_t hashPos = k.find('#');
        const std::string base = (hashPos == std::string::npos) ? k
                                                                : k.substr(0, hashPos);
        if (base == node->name && kv.second.moduleName == node->moduleName &&
            !kv.second.hasBody && info.hasBody) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数 '" + node->name +
                                "' 原型声明与定义签名不一致（重载须参数类型不同）");
            return;
        }
    }
    funcSigModules_[node->sigKey].insert(node->moduleName);
    // A′：注册键 = 公式键（与定义侧 mangledName 同源，消顺序依赖）
    functions_[linkKey] = info;
}
// C-3（FFI）：查询签名 key 对应函数是否为 外部 函数（链接符号=纯名）
bool SemanticAnalyzer::isExternFunc(const std::string& sigKey) const {
    auto it = functions_.find(sigKey);
    if (it != functions_.end()) return it->second.isExtern;
    // 跨模块前缀键（模块名$sigKey，crate 隔离注册路径）：
    //   去掉模块前缀后按签名 key 比对（外部 声明均在入口文件，前缀=入口模块）
    const std::size_t dollar = sigKey.find('$');
    if (dollar != std::string::npos) {
        const std::string rest = sigKey.substr(dollar + 1);
        auto it2 = functions_.find(rest);
        if (it2 != functions_.end()) return it2->second.isExtern;
    }
    return false;
}
} // namespace cn_compiler
