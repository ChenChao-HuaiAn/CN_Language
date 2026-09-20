// 类型系统工具子模块（Task 2.3 抽取）：CN语言类型识别/转换/推导
// 职责：
//   1. 类型别名规范化（整数->整32、小数->浮64）
//   2. 类型分类（数值/整数/浮点/函数指针）
//   3. 隐式转换检查（规格书3.7：字符↔整数、整型宽化、浮点宽化、整数->浮点）
//   4. 数值运算结果类型推导（整型取宽者、含浮点取浮点）
//   5. 整型位宽秩（rank）比较（整8=1 ... 整128=5）
//   6. 字面量后缀解析（f/L/LL/U/UL/ULL，规格书4.3）
// 设计：纯静态工具（无状态），供语义分析器（semantic）与IR生成器（ir）共用，
//       避免两处各写一套类型逻辑导致别名/秩不一致
// 规范：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#pragma once
#include <string>
#include <vector>

namespace cn_compiler {
namespace types {

// ==================== 类型别名与分类 ====================

// 类型别名规范化：整数 -> 整32、小数 -> 浮64（其余原样返回）
// 语义/IR层所有类型比较前必须先规范化，否则别名误报类型不匹配
// Task 2.4：递归规范化复合类型（指针 整数* -> 整32*、数组 整数[10] -> 整32[10]）
std::string canonical(const std::string& type);

// 是否整数类型（整8~整128/正8~正128/整数，规范后判断）
bool isInteger(const std::string& type);

// 静态标量初值可否直存 .data 的类型判定（331-a·T50 根治·三后端单一归属）：
//   中文类型名（整8..正64/字符/布尔）与 IR 类型名（i8..u64/i1）双口径——
//   函数内静态局部经 mapType 落 IR 名（"i64"），顶层静态落中文名（"整64"），
//   原判定只认中文名（isInteger），导致函数内静态标量初值在 linux_x64/arm64
//   恒 .quad 0（静默丢初值；win 侧用「非浮点文本」宽松判定而幸存=三后端
//   口径分叉）。i128/u128 不在本判定（走双 .quad 分支），字符串句柄不可直存。
bool isStaticScalarInitType(const std::string& type);

// 是否 128 位整数类型（T46·467-a·单一归属）：中文（整128/正128）与 IR 名
//   （i128/u128）双口径——原三后端数据段各自四连 ‖ 判定（同串重复四处），
//   收敛至此供 IR 层与三后端共用。
bool isInt128Type(const std::string& type);

// 128 位整数是否有符号（整128/i128=有符号；正128/u128=无符号）——
//   供 parseInt128InitText 范围判定（有符号 |v| ≤ 2^127；无符号 < 2^128）。
bool isInt128Signed(const std::string& type);

// 128 位静态初值文本 → two's complement 双 quad（T46·467-a·单一归属）：
//   text 为字面量 raw 文本（IntegerLiteral::raw——可能超出 int64 表示域，
//   唯一完整信息源），支持十进制（可带 +/-）、0x/0X、0b/0B、0o/0O 前缀。
//   纯 uint64 实现（MSVC 无 __int128，跨 MSVC/GCC 单一口径）。
//   范围：isSigned 时 |v| ≤ 2^127（最小值 −2^127 特例合法）；无符号 < 2^128。
//   超范围/非法文本返回 false（调用方保持零占位——超界拒绝归 T9 方案 D 辖区）。
bool parseInt128InitText(const std::string& text, bool isSigned,
                         unsigned long long& loOut, unsigned long long& hiOut);

// 是否浮点类型（浮32/浮64/小数，规范后判断）
bool isFloat(const std::string& type);

// 是否数值类型（整/正/浮，规范后判断）
bool isNumeric(const std::string& type);

// 是否无符号整数类型（正N）
bool isUnsigned(const std::string& type);

// 整型位宽秩（整8=1 ... 整128=5）；非整数类型返回0
int intRank(const std::string& type);

// 是否函数指针类型（函数指针<返回>(参数,...)，Task 2.2 规范化字符串）
bool isFuncPtr(const std::string& type);

// 从函数指针类型字符串提取形参类型列表（337-a·T53 家系）：
//   "函数指针<整32>(整32,整128)" -> ["整32","整128"]；非函数指针串/
//   无参数列表返回空列表。**唯一实现**——语义层内部工具 funcPtrParams 与
//   IR 层间接调用点（funcPtrParamsOfCallee）均转发至此（消除同解析两份实现）。
std::vector<std::string> funcPtrParamsOf(const std::string& type);

// ==================== 指针/数组复合类型（Task 2.4） ====================

// 是否指针类型（类型名以 * 结尾，如 整32* / 空类型*）
bool isPointer(const std::string& type);

// 是否数组类型（类型名形如 元素类型[长度]，如 整32[10]）
// 识别：最后一个 '[' 位于末尾 ']' 前，且下标部分为十进制数字
bool isArray(const std::string& type);

// 提取指针所指元素类型（整32* -> 整32；非指针类型原样返回）
std::string pointeeOf(const std::string& type);

// 提取数组元素类型（整32[10] -> 整32；数组类型中最后一个 '[' 之前的部分）
std::string arrayElemOf(const std::string& type);

// 提取数组长度（整32[10] -> 10；非数组类型返回 -1）
int arrayLenOf(const std::string& type);

// ==================== 引用类型（A-1 引用参数，2026-08） ====================

// 是否引用类型（类型名以 & 结尾，如 整32& / 账户& / T&）
bool isReference(const std::string& type);

// 剥离引用后缀（整32& -> 整32；非引用类型原样返回）
std::string stripRef(const std::string& type);

// 参数类型规范化：引用保留 &（整32& -> 整32&），其余同 canonical——
// 使重载签名（sigKey）与 mangling 能区分 按值/按引用 参数
std::string canonicalParam(const std::string& type);

// 基本类型字节大小（整8/布尔=1、整16=2、整32/浮32/字符=4、整64/浮64/指针=8、
// 整128/正128=16）；其他类型返回0（数组/结构体等由调用方递归计算）
int typeSize(const std::string& type);

// ==================== 隐式转换 ====================

// 能否隐式转换（规格书3.7 + 整型宽化/浮点宽化/整数->浮点）
// from 源类型、to 目标类型（均先做别名规范化）
bool canConvert(const std::string& from, const std::string& to);

// 数值运算结果类型：整型取秩高者，含浮点取较宽浮点（先做别名规范化）
std::string commonNumericType(const std::string& a, const std::string& b);

// ==================== 字面量后缀（规格书4.3） ====================

// 解析整数/浮点字面量原始文本的后缀，返回推断类型
// 整数字面量：无后缀 -> 整32；f -> 浮32；L -> 整64；LL -> 整128；
//            U -> 正32；UL -> 正64；ULL -> 正128
// 浮点字面量：无后缀 -> 浮64；f/F -> 浮32
// 返回 CN 源码类型名（整32/浮64 等），非法后缀返回空串
std::string literalTypeOf(const std::string& raw, bool isFloat);

// 剥离字面量文本的后缀，返回纯数字文本（"123ULL" -> "123"，"3.5f" -> "3.5"）
std::string stripLiteralSuffix(const std::string& raw);

// ==================== 字符串元素容器/映射判定（语义与IR共用，单一事实源） ====================
// 第七十七轮（A21 借出视图生命周期检查）：判定原在 IR 层 isStringElemContainer/
//   isStringValuedMap（74-a/76-a 容器元素释放面确立），语义层新增借出视图生命周期
//   检查需要同一口径——上提 types:: 共享，IR 层委托调用（避免双实现分叉）。

// 字符串元素容器判定：实例化名 前缀$元素类型（向量/链表/栈/队列/集合），元素类型恰为 字符串。
//   严格口径：嵌套形态（向量$映射$整64$字符串）元素是容器对象非字符串，不予匹配。
bool isStringElemContainer(const std::string& canonClass);

// 字符串值映射判定：映射$K$字符串——值侧恰为 字符串（键侧任意）。
bool isStringValuedMap(const std::string& canonClass);

// ==================== 128位整数文本工具（Task 完善A：i128 完整支持） ====================

// 是否 128 位整数类型（整128/正128）
bool isI128(const std::string& type);

// 十进制文本是否超出 int64 范围（正值 > 9223372036854775807；用于无后缀自动提升 i128）
// text 应为已剥后缀的纯数字文本（仅十进制）
bool textExceedsInt64(const std::string& text);

// 十进制文本是否超出 正32（无符号32位）范围（> 4294967295）
// 审查修复：无符号后缀（U）字面量值超出 2^32-1 时自动提升为 正64。
bool textExceedsU32(const std::string& text);

// 十进制文本是否超出 正64（无符号64位）范围（> 18446744073709551615）
// 审查修复：无符号后缀（UL）字面量值超出 2^64-1 时自动提升为 正128。
bool textExceedsU64(const std::string& text);

// 将 i128 字面量文本拆分为 低64位:高64位 十六进制文本（如 "10000000000000000000" -> "0:8AC7230489E80000"）
// 支持 十进制/0x十六进制/0b二进制/0o八进制 前缀（文本已剥后缀）
// 解析失败返回空串（词法层已保证合法，防御性）
std::string splitI128Text(const std::string& raw);

} // namespace types
} // namespace cn_compiler
