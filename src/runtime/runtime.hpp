// CN运行时（cnrt）公共接口：内存管理、IO、程序入口（规格书第十章）
// 对应CN语言内置函数：打印 / 打印行（变参，IR 层展开为 __cn_print_* 序列），以及程序入口 主
// 方案C（2026-08-14）✅ 已修复：遗留的 打印行整数/打印行浮点 已删除（旧语义"行"=换行，
//   与 打印行 不换行语义相反）；printLineInt/printLineFloat 符号保留（单元测试引用+ABI稳定）
// 注意：运行时C++实现采用英文API命名（用户要求），CN符号在汇编层映射
#pragma once

#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#define CNRT_EXPORT __declspec(dllexport)
#else
#define CNRT_EXPORT
#endif

// 内存管理API（规格书10.2，对应CN语言 分配/释放/重新分配/复制内存/置零内存）
extern "C" {
    // 分配内存（对应CN内置：分配）
    CNRT_EXPORT void* cn_alloc(std::size_t size);
    // 释放内存（对应CN内置：释放）
    CNRT_EXPORT void cn_free(void* ptr);
    // 重新分配内存（对应CN内置：重新分配）
    CNRT_EXPORT void* cn_realloc(void* ptr, std::size_t size);
    // 复制内存（对应CN内置：复制内存）
    CNRT_EXPORT void cn_memcpy(void* dst, const void* src, std::size_t size);
    // 置零内存（对应CN内置：置零内存）
    CNRT_EXPORT void cn_memset(void* dst, std::size_t size);

    // IO API（规格书10.1，Task 2.9 语义调整：打印=println 换行、打印行=print 不换行）
    // 打印/打印行 变参展开走 __cn_print_*（不换行）系列（见 string_api.cpp）；
    // 以下 printLine/printNoLine/printLineInt/printLineFloat 为防御保留的单参数路径
    //   （printLineInt/printLineFloat 仅单元测试直接调用，编译器已不再映射）
    CNRT_EXPORT void printLine(const char* text);          // 打印（字符串，println 换行）
    CNRT_EXPORT void printNoLine(const char* text);        // 打印行（字符串，print 不换行）
    CNRT_EXPORT void printLineInt(long long value);        // 打印整数（换行，防御保留）
    CNRT_EXPORT void printLineFloat(double value);         // 打印浮点（换行，防御保留）

    // 字符串API（规格书10.1 字符串操作：长度/比较/连接/复制/查找；Task 2.5 + Task 2.8 补充）
    // 对应CN内置函数：字符串长度/字符串比较/字符串连接/字符串复制/字符串查找/
    //               字符串子串/字符串字典序/字符串大写/字符串小写/字符串前缀/
    //               字符串后缀/字符串包含/字符串修剪/字符串反转/整数转字符串/
    //               浮点转字符串/字符转字符串/正数转字符串/字符串释放
    // 字符串采用 UTF-8 编码、以 \0 结尾（规格书10.3）
    CNRT_EXPORT long long __cn_str_len(const char* str);         // 字符串长度（UTF-8 字节数）
    CNRT_EXPORT long long __cn_str_eq(const char* a, const char* b); // 字符串比较（相等返回1，否则0）
    CNRT_EXPORT char* __cn_str_concat(const char* a, const char* b); // 字符串连接（动态分配）
    CNRT_EXPORT char* __cn_str_copy(const char* str);              // 字符串复制（深拷贝）
    CNRT_EXPORT long long __cn_str_find(const char* haystack, const char* needle); // 子串查找（位置，-1未找到）

    // ---- Task 2.8 补充字符串API（规格书10.1 标注"常见字符串库补充"，内存语义：调用方负责释放） ----
    CNRT_EXPORT char* __cn_str_sub(const char* str, long long start, long long len);   // 子串（字节偏移，动态分配）
    CNRT_EXPORT long long __cn_str_cmp(const char* a, const char* b); // 字典序比较（<0/0/>0，替代未定义的字符串比较运算符）
    CNRT_EXPORT char* __cn_str_upper(const char* str);               // ASCII 大写（动态分配，非ASCII字节原样保留）
    CNRT_EXPORT char* __cn_str_lower(const char* str);               // ASCII 小写（动态分配，非ASCII字节原样保留）
    CNRT_EXPORT long long __cn_str_starts_with(const char* str, const char* prefix); // 前缀判断（1=是，0=否）
    CNRT_EXPORT long long __cn_str_ends_with(const char* str, const char* suffix);   // 后缀判断（1=是，0=否）
    CNRT_EXPORT long long __cn_str_contains(const char* haystack, const char* needle); // 包含判断（1=是，0=否）
    CNRT_EXPORT char* __cn_str_trim(const char* str);                // 去首尾空白（空格/制表/换行/回车，动态分配）
    CNRT_EXPORT char* __cn_str_reverse(const char* str);             // 反转（UTF-8安全：按字符序列逆序，动态分配）
    CNRT_EXPORT char* __cn_str_from_int(long long value);            // 整数转字符串（%lld，动态分配）
    CNRT_EXPORT char* __cn_str_from_uint(unsigned long long value);  // 无符号整数转字符串（%llu，动态分配，缺陷修复）
    CNRT_EXPORT char* __cn_str_from_float(double value);             // 浮点转字符串（%f 语义，动态分配）
    CNRT_EXPORT char* __cn_str_from_char(int value);                 // 字符转字符串（单字节ASCII，动态分配）
    CNRT_EXPORT char* __cn_str_from_bool(int value);                 // 布尔转字符串（"真"/"假"，Task 2.9）
    CNRT_EXPORT void __cn_str_free(char* str);                       // 字符串释放（封装 cn_free，可安全释放nullptr）

    // ---- Task 6.5 字符串解析API（字符串扩展库；对应 CN 层 stdlib/字符串扩展.cn 包装）----
    // 成功标志：int* 输出参数（1=成功，0=非法输入/空串/范围错误），CN 层用 &成功 传参。
    // 内存语义：输入为常规 UTF-8 字符串（以 \0 结尾），无动态分配输出。
    CNRT_EXPORT long long __cn_str_to_int(const char* str, int* ok);     // 字符串转整数（字符串.转整数，strtoll 整串解析）
    CNRT_EXPORT double __cn_str_to_double(const char* str, int* ok);     // 字符串转浮点（字符串.转浮点，strtod 整串解析）
    CNRT_EXPORT long long __cn_str_to_bool(const char* str, int* ok);    // 字符串转布尔（字符串.转布尔，"真"/"假"/"true"/"false"）

    // 格式化（Task 2.9，规格书10.6）：sprintf 风格变参，返回动态分配字符串，调用方负责释放
    // 占位符：%d(整) %u(无符号) %f(浮点) %s(字符串) %c(字符) %x/%X(十六进制) %o(八进制) %p(指针)
    CNRT_EXPORT char* __cn_format(const char* fmt, ...);             // sprintf 风格格式化（动态分配）

    // 打印行多参数格式化辅助（Task 2.5）：逐段打印，最后统一换行
    // 打印行("值:", 42, 3.5) 展开为 __cn_print_str("值:") + __cn_print_int(42) +
    //                        __cn_print_float(3.5) + __cn_print_newline()
    CNRT_EXPORT void __cn_print_str(const char* text);     // 打印字符串（不换行）
    CNRT_EXPORT void __cn_print_int(long long value);      // 打印整数（不换行，%lld 语义）
    CNRT_EXPORT void __cn_print_uint(unsigned long long value); // 打印无符号整数（不换行，%llu 语义，缺陷修复）
CNRT_EXPORT void __cn_print_bool(bool value);             // 打印布尔（不换行，输出 真/假，2026-08 用户裁决）
    CNRT_EXPORT void __cn_print_float(double value);       // 打印浮点（不换行）
    CNRT_EXPORT void __cn_print_newline();                 // 打印换行

    // 128位整数运算辅助（规格书10.5，Task 完善A）
    // 128位值以 cn_i128* 指针传入/回写（布局：out[0]=低64位、out[1]=高64位，
    // 与编译器 i128 变量双槽一致），调用方负责传入指向16字节缓冲的指针
    // 平台无关改造（阶段5 Linux ARM64）：统一用 std::uint64_t（固定宽度类型），
    //   避免 LP64（Linux）下 std::uint64_t=unsigned long 与 unsigned long long 重声明冲突
    CNRT_EXPORT void __cn_add_i128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 128位加法
    CNRT_EXPORT void __cn_sub_i128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 128位减法
    CNRT_EXPORT void __cn_mul_i128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 有符号128位乘法
    CNRT_EXPORT void __cn_mul_u128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 无符号128位乘法
    CNRT_EXPORT void __cn_div_i128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 有符号128位除法
    CNRT_EXPORT void __cn_mod_i128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 有符号128位取余
    CNRT_EXPORT void __cn_div_u128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 无符号128位除法
    CNRT_EXPORT void __cn_mod_u128(const std::uint64_t* a, const std::uint64_t* b,
                                   std::uint64_t* out);            // 无符号128位取余
    CNRT_EXPORT int __cn_cmp_i128(const std::uint64_t* a, const std::uint64_t* b); // 有符号比较
    CNRT_EXPORT int __cn_cmp_u128(const std::uint64_t* a, const std::uint64_t* b); // 无符号比较
    CNRT_EXPORT double __cn_i128_to_f64(const std::uint64_t* a);   // 有符号128位转浮点
    CNRT_EXPORT double __cn_u128_to_f64(const std::uint64_t* a);   // 无符号128位转浮点
    CNRT_EXPORT double __cn_u64_to_f64(std::uint64_t v);           // 无符号64位转浮点（Task 2.10）
    CNRT_EXPORT void __cn_f64_to_i128(double value, std::uint64_t* out); // 浮点转有符号128位
    CNRT_EXPORT void __cn_print_i128(const std::uint64_t* v);      // 打印有符号128位（不换行）
    CNRT_EXPORT void __cn_print_u128(const std::uint64_t* v);      // 打印无符号128位（不换行）
    CNRT_EXPORT void printLineI128(const std::uint64_t* v);        // 打印有符号128位（换行）
    CNRT_EXPORT void printLineU128(const std::uint64_t* v);        // 打印无符号128位（换行）

    // 输入 API（Task 6.2，规格书10.6 输入；对应 CN 层 stdlib/IO.cn 函数）
    // 内存语义：__cn_read_line 返回动态分配 UTF-8 字符串（不含换行），
    //   调用方负责用 __cn_str_free 释放；EOF/失败返回 nullptr。
    // 成功标志：__cn_read_int/__cn_read_float 用 int* 输出参数（1=成功，0=EOF/非法输入），
    //   CN 层 stdlib/IO.cn 以 &成功 传参（C 风格）。
    CNRT_EXPORT char* __cn_read_line();                 // 读取行（IO.读取行，EOF 返回 nullptr）
    CNRT_EXPORT long long __cn_read_int(int* ok);       // 读取整数（IO.读取整数，strtoll 整行解析）
    CNRT_EXPORT double __cn_read_float(int* ok);        // 读取浮点（IO.读取浮点，strtod 整行解析）
    CNRT_EXPORT void __cn_print_err(const char* text);  // 打印到标准错误（IO.打印到错误，fprintf stderr 不换行）

    // 文件 API（Task 6.2，规格书阶段五「文件系统」；对应 CN 层 stdlib/文件.cn 函数）
    // 句柄类型：C 层 FILE* 以 void* 传递（CN 层对应 空类型*）。
    // 模式：1=读"rb"、2=写"wb"（截断）、3=追加"ab"；失败返回 nullptr。
    // 中文路径：UTF-8 经 MultiByteToWideChar(CP_UTF8) + _wfopen_s（lessons 已验证方案）。
    // 内存语义：__cn_file_read_line 返回动态分配字符串（含换行），调用方负责 __cn_str_free。
    CNRT_EXPORT void* __cn_file_open(const char* path, long long mode);      // 打开文件（文件.打开文件）
    CNRT_EXPORT long long __cn_file_read(void* handle, char* buffer, long long count); // 读取原始字节（文件.读取文件）
    CNRT_EXPORT long long __cn_file_write(void* handle, const char* data);   // 写入字符串（文件.写入文件）
    CNRT_EXPORT char* __cn_file_read_line(void* handle);                     // 按行读取（文件.读取文件行，EOF 返回 nullptr）
    CNRT_EXPORT long long __cn_file_size(void* handle);                      // 文件大小（文件.文件大小，失败 -1）
    CNRT_EXPORT void __cn_file_close(void* handle);                          // 关闭文件（文件.关闭文件）
    CNRT_EXPORT long long __cn_file_exists(const char* path);                // 文件存在（文件.文件存在，1=真 0=假）

    // 数学库 API（Task 6.3，规格书10.5 数学库；对应 CN 层 stdlib/数学.cn 函数）
    // 命名约定：运行时英文 API，CN 层中文函数名由编译器 IR 层映射到此符号
    // 输入输出均为 double（浮64）；P1 的对数/反三角/随机数留待后续
    CNRT_EXPORT double __cn_sqrt(double value);      // 平方根（数学.平方根）
    CNRT_EXPORT double __cn_pow(double base, double exponent); // 幂（数学.幂，底数^指数）
    CNRT_EXPORT double __cn_sin(double radians);     // 正弦（数学.正弦，弧度制）
    CNRT_EXPORT double __cn_cos(double radians);     // 余弦（数学.余弦，弧度制）
    CNRT_EXPORT double __cn_tan(double radians);     // 正切（数学.正切，弧度制）
    CNRT_EXPORT double __cn_fabs(double value);      // 绝对值（数学.绝对值）
    CNRT_EXPORT double __cn_ceil(double value);      // 向上取整（数学.向上取整）
    CNRT_EXPORT double __cn_floor(double value);     // 向下取整（数学.向下取整）

    // 运行时错误（规格书附录B错误码，Task 2.4 数组/指针运行时检查调用）
    // 错误码：1=除零、2=数组越界、3=空指针解引用（打印错误信息后终止程序）
    // 阶段3（Task 3.5）：扩展 4=内存分配失败、5=文件打开失败、6=无效参数、
    //                   7=资源未初始化、8=溢出
    CNRT_EXPORT void __cn_runtime_error(long long errorCode);
    // 错误码 -> 错误消息文本（不终止进程；供测试与诊断直接验证消息表）
    // 未注册错误码返回 "未知运行时错误"
    CNRT_EXPORT const char* __cn_error_message(long long errorCode);

    // 对象内存辅助（阶段3 Task 3.1，规格书06 类实例化/删除）
    // NewObject 展开调用：分配 size 字节堆内存（失败时报错误码4并终止），
    //   返回对象指针（虚表指针初始化由 codegen 负责——对象首地址 8 字节）
    CNRT_EXPORT void* __cn_object_new(long long size);
    // DeleteObject 展开调用：释放对象内存（安全释放 nullptr）
    CNRT_EXPORT void __cn_object_delete(void* ptr);

    // ---- Task 6.5 时间 API（时间库；对应 CN 层 stdlib/时间.cn 包装）----
    // 对标 C++ chrono/ctime；平台差异：#ifdef _WIN32（QueryPerformanceCounter/
    //   localtime_s）与 POSIX（clock_gettime/localtime_r）分支。
    // 内存语义：__cn_time_format 返回动态分配字符串，调用方用 __cn_str_free 释放。
    CNRT_EXPORT long long __cn_time();                            // 当前时间戳（时间.当前时间戳，秒级）
    CNRT_EXPORT long long __cn_clock_ms();                        // 单调时钟毫秒（时间.单调时钟毫秒）
    CNRT_EXPORT char* __cn_time_format(long long ts, const char* fmt); // 格式化时间（时间.格式化时间，失败 nullptr）

    // ---- Task 6.5 系统 API（系统库；对应 CN 层 stdlib/系统.cn 包装）----
    // 对标 C++ argv（规格书10.4 命令行参数）；entry 入口经 __cn_cache_argv 缓存。
    // 内存语义：__cn_argv 返回 CRT 持有字符串（非动态分配），调用方不得释放。
    CNRT_EXPORT long long __cn_argc();                            // 参数个数（系统.参数个数，含可执行文件名）
    CNRT_EXPORT char* __cn_argv(long long index);                 // 参数（系统.参数，越界 nullptr）
    // 缓存命令行参数：由 entry 入口调用（程序启动注入真实 argc/argv）
    CNRT_EXPORT void __cn_cache_argv(int argc, char** argv);
    // 测试专用 setter：注入固定 argc/argv（供单元测试验证缓存读写，生产不调用）
    CNRT_EXPORT void __cn_set_argv_for_test(int argc, char** argv);

    // 程序入口（crt0风格，规格书10.4：调用CN语言 主 函数）
    CNRT_EXPORT int entry(int argc, char** argv);
}
