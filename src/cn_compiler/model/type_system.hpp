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
#include <string_view>
#include <optional>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"

namespace cn_compiler {
namespace types {

// ==================== 类型别名与分类 ====================

// 类型别名规范化：整数 -> 整32、小数 -> 浮64（其余原样返回）
// 语义/IR层所有类型比较前必须先规范化，否则别名误报类型不匹配
// Task 2.4：递归规范化复合类型（指针 整数* -> 整32*、数组 整数[10] -> 整32[10]）
std::string canonical(const std::string& type);

// #1215：非分配 canonical 等值判定——语义等价 canonical(type) == expected，
//   但不构造临时 std::string（IR/codegen 每指令数十处「规范后等于某字面量」
//   判定：原实现每次 canonical 拷贝 + 比较，命中长类型名还会堆分配）。
//   expected 传入字面量或 string_view；内部按 canonical 的改写规则
//   （别名/尾部 & 递归剥除/* 递归追加/数组 [N] 递归追加）逐段比对。
bool canonicalEquals(const std::string& type, std::string_view expected);

// #1674 乙案（2026-10-26 用户批·001 §5.6 ③）：手动释放<T> 包装类型判定——
//   单一事实源（语义/IR 的级联跳过/赋值忘旧值/布局透明/成员 .值 各消费点共用）。
//   两形态皆认：合成名 `手动释放$T`（泛型 < > 归一后）与原文 `手动释放<T>`。
//   命中返回载荷类型 T；未命中=nullopt。对标 Rust ManuallyDrop<T>：声明槽的
//   释放权归用户（编译器级联跳过+赋值覆盖不释放旧值），与联合体成员「手动释放」
//   标注（164-a·同语义修饰形式）和谐共存——标注形式（联合体成员）管联合体面，
//   类型形式（本函数）管字段/变量面。
cn::Optional<std::string> manuallyDroppedPayloadOf(const std::string& type);

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

// 整型位宽秩（整8=1 ... 整128=5，整数别名=3）；非整数类型=无值
// 1360 结果化：原哨兵 0 与秩值域重叠（漏检即静默错秩），改 cn::Optional——
//   消费点 .has_value() 强制分流或 .value_or(显形默认)
cn::Optional<int> intRank(const std::string& type);

// 是否函数指针类型（函数指针<返回>(参数,...)，Task 2.2 规范化字符串）
bool isFuncPtr(const std::string& type);

// 从函数指针类型字符串提取形参类型列表（337-a·T53 家系）：
//   "函数指针<整32>(整32,整128)" -> ["整32","整128"]；非函数指针串/
//   无参数列表括号配对=无值；空表=合法零参函数指针 `函数指针<整32>()`。
//   **唯一实现**——语义层内部工具 funcPtrParams 与 IR 层间接调用点
//   （funcPtrParamsOfCallee）均转发至此（消除同解析两份实现）。
//   1360 结果化：原空 vector 双语义（非函数指针 vs 合法零参）两态合并，
//   改 cn::Optional 三态显形——无值=非函数指针串·空表=合法零参·非空=形参表。
cn::Optional<std::vector<std::string>> funcPtrParamsOf(const std::string& type);

// 从函数指针类型字符串提取返回类型（485·对称 funcPtrParamsOf）：
//   "函数指针<整128>(整32)" -> "整128"；返回类型可嵌套函数指针/泛型
//   （如 函数指针<函数指针<整32>(整32)>()），按 '<' '>' 深度计数取首
//   '<' 的配对 '>'；非函数指针串/无配对 '>'/返回类型空白=无值。
//   与 funcPtrParamsOf 同为**唯一实现**——IR 层间接调用点
//   funcPtrRetTypeOfCallee 转发至此。1360 结果化：原空串哨兵改 cn::Optional。
cn::Optional<std::string> funcPtrReturnTypeOf(const std::string& type);

// ==================== 绑定方法值类型（031·#1075·第一等民化） ====================
// 内部串形态：`方法值<类名,函数指针<返回类型>(参数表)>`（类名=方法符号 owner 类；
//   源码书写 方法值<类名, 返回类型(参数表)>，语义层构造时归一为内嵌函数指针串）。
//   运行时表示=栈上两槽 {this 对象引用, 方法代码}（值语义 16 字节·零堆分配）——
//   详见 plans/031 §二·①/⑤。静态方法作值保持裸函数指针语义（不入本族）。

// 是否绑定方法值类型（前缀 `方法值<`）
bool isMethodValueType(const std::string& type);

// 拼接绑定方法值类型串（类名 + 内嵌函数指针串——调用方保证 fnPtrType 为
//   `函数指针<...>(...)` 规范形态）
std::string makeMethodValueType(const std::string& className,
                                const std::string& fnPtrType);

// 提取绑定方法值的类名（`方法值<X,函数指针<...>(...)>` -> "X"；
//   非方法值串/残缺=无值）
std::optional<std::string> methodValueClassOf(const std::string& type);

// 提取绑定方法值内嵌的签名函数指针串（`方法值<X,函数指针<...>(...)>` ->
//   "函数指针<...>(...)"；非方法值串/残缺=无值）。无值=调用方按未登记
//   处置（miss 不可静默纪律——031 内部串由 makeMethodValueType 单一构造）。
std::optional<std::string> methodValueFnPtrOf(const std::string& type);

// ==================== 指针/数组复合类型（Task 2.4） ====================

// 是否指针类型（类型名以 * 结尾，如 整32* / 空类型*）
bool isPointer(const std::string& type);

// 是否数组类型（类型名形如 元素类型[长度]，如 整32[10]、整32[2][3]）
// 识别：第一个 '[' 有配对 ']'、类型以 ']' 结尾，且第一维下标为十进制数字
bool isArray(const std::string& type);

// 提取指针所指元素类型（整32* -> 整32；非指针类型原样返回）
std::string pointeeOf(const std::string& type);

// 提取数组元素类型：删掉第一个 '[...]' 维度段（#1676 多维修正）——
//   整32[10] -> 整32、整32[2][3] -> 整32[3]（保留剩余维度后缀）；
//   非数组形态原样返回
std::string arrayElemOf(const std::string& type);

// 提取数组长度：取第一维（整32[10] -> 10、整32[2][3] -> 2·#1676 多维修正）；
//   非数组类型/下标非纯数字/超 int 承载=无值
// 1360 结果化：原哨兵 -1 与合法长度值域重叠，改 cn::Optional——
//   消费点 .has_value() 分流；isArray 内部同以 has_value() 判定
cn::Optional<int> arrayLenOf(const std::string& type);

// #1459：提取数组**基元素**类型——循环剥全部维度至非数组文本
//   （整32[2][3] -> 整32、结果<字符串,整32>[2][3] -> 结果<字符串,整32>；
//   一维与 arrayElemOf 等价）。释放/布局面的「元素是否携资源」「元素尺寸」
//   判定须落在基元素上（带维度尾缀的中间文本使 isResultType 前缀判定
//   携带 [N] 尾巴、载荷字段收集 miss）。剥层数上限 8（超深=形态异常，
//   返回空串由消费方分流——不猜不静默）
std::string arrayBaseElemOf(const std::string& type);

// #1459：数组**总元素数**——各维度长度乘积（整32[2][3] -> 6；一维=长度
//   自身）。任一维非纯数字/超承载=无值（消费方显式分流，miss 不静默）
cn::Optional<long long> arrayTotalElemCount(const std::string& type);

// 剥离尾部复合后缀（061-a 合成体消费面收口）：类型文本 = 核心 + 后缀，
//   后缀 = 结尾连续的 *（指针）、&（引用）、[N]（N 为纯十进制数字）组合——
//   如 结果<整32,整32>[2] -> {"结果<整32,整32>", "[2]"}、
//      结果<盒子,整32>* -> {"结果<盒子,整32>", "*"}、整32*[3] -> {"整32", "*[3]"}。
//   无后缀时 suffix 为空串、core 原样返回。模板闭合 '>' / 限定 '::' 不属后缀。
//   用途：结果/可选 模板判定（isResultType/isOptionalType）与变量声明重组
//   （visitVarDecl）须对 core 判定、重组后接回 suffix——原实现前缀判定不校验
//   尾缀，带后缀形态被重组覆盖（数组维度/指针形态丢失，061 P3-a/d2 实测）。
void splitTypeSuffix(const std::string& type, std::string& coreOut,
                     std::string& suffixOut);

// #1215：splitTypeSuffix 的无分配视图版（语义逐字节等价）：core/suffix 为
//   type 的子串视图，不构造临时 std::string。供 isResultType/isOptionalType
//   等高频判定点使用（原实现每次双串拷贝，命中模板前缀时必然分配）。
void splitTypeSuffixView(std::string_view type, std::string_view& coreOut,
                         std::string_view& suffixOut);

// ==================== 引用类型（A-1 引用参数，2026-08） ====================

// 是否引用类型（类型名以 & 结尾，如 整32& / 账户& / T&）
bool isReference(const std::string& type);

// 剥离引用后缀（整32& -> 整32；非引用类型原样返回）
std::string stripRef(const std::string& type);

// 参数类型规范化：引用保留 &（整32& -> 整32&），其余同 canonical——
// 使重载签名（sigKey）与 mangling 能区分 按值/按引用 参数
std::string canonicalParam(const std::string& type);

// 基本类型字节大小（整8/布尔=1、整16=2、整32/浮32/字符=4、整64/浮64/指针=8、
// 整128/正128=16）；其他类型（数组/结构体等由调用方递归计算）=无值
// 1360 结果化：原哨兵 0 与合法大小值域（最小 1）相邻易混，改 cn::Optional——
//   消费点 .has_value() 强制分流
cn::Optional<int> typeSize(const std::string& type);

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
// 返回 CN 源码类型名（整32/浮64 等）。「无后缀」是 001 §4.3 合法契约态
//   （恒定级 整32/浮64，本函数内独立分支返回，与失败态无重叠）；
//   1360 结果化：非法后缀（词法层已限·理论不可达的错误恢复面）原空串哨兵
//   改 cn::Optional 无值——消费点 .has_value() 显形分流
cn::Optional<std::string> literalTypeOf(const std::string& raw, bool isFloat);

// 剥离字面量文本的后缀，返回纯数字文本（"123ULL" -> "123"，"3.5f" -> "3.5"）
std::string stripLiteralSuffix(const std::string& raw);

// ==================== 无符号后缀 + 文本非零判定（1016 四轮审查） ====================
// 无符号后缀白名单：U/UL/ULL（大小写不敏感）——与 literalTypeOf 定级口径一致；
//   LU/LLU 等非法顺序返回 false（宿主回退整32=不属无符号面）。
bool unsignedSuffixedLiteralText(const std::string& raw);

// 整数字面量文本非零判定（任意精度·不依赖 int64 承载）：剥可选 +/-、
//   0x/0b/0o 前缀、L/l/U/u 后缀后，数字段是否存在非 '0' 字节。
//   用途=parseIntValue stoll 溢出静默回 0 时区分「真零」与「溢出巨值」。
bool integerLiteralTextNonZero(const std::string& raw);

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

// 128 位整数字面量范围检查（323 v16 命中C：负边界误报根治）。
// text 为剥后缀文本（可带正负号）：整128 合法域 [-2^127, 2^127-1]——负值
// 允许到 2^127（最小界），正值只到 2^127-1；正128 合法域 [0, 2^128-1]。
// 不得用带符号原文字符串比较（负号参与长度/字典序会误判）。
bool i128TextInRange(const std::string& text, bool isSigned);

// 十进制文本是否超出 int64 范围（正值 > 9223372036854775807；用于无后缀自动提升 i128）
// text 应为已剥后缀的纯数字文本（仅十进制）
bool textExceedsInt64(const std::string& text);

// 无后缀整数字面量数学值是否超 整32 域（420 提升链求值域判据·#506甲 负号
// 形态按数学负值判）：正超上界 2^31-1 或负超下界 -2^31 → 求值域应提升 整64。
// value=带符号承载值——parser 323 把一元负号+字面量折叠为带负号 raw/负值的
// 单一 IntegerLiteral，负号形态在此即数学负值；超 int64 承载的十进制文本
// 面（|值| > 2^63-1·承载已失真）不归本判据，由调用点 textExceedsInt64
// 先行判 整128（本函数不涉）。单点归属：四处 420 提升链实现（宿主 IR 物化
// ir_expr / 推断面 semantic_decl / 回绕实参域 semantic_call / 溢出预演域
// semantic_expr_binary）共用，防判据分叉。
bool literalValueExceedsInt32Domain(long long value);

// 十进制文本是否超出 正32（无符号32位）范围（> 4294967295）
// 审查修复：无符号后缀（U）字面量值超出 2^32-1 时自动提升为 正64。
bool textExceedsU32(const std::string& text);

// 十进制文本是否超出 正64（无符号64位）范围（> 18446744073709551615）
// 审查修复：无符号后缀（UL）字面量值超出 2^64-1 时自动提升为 正128。
bool textExceedsU64(const std::string& text);

// ==================== 整数字面量文本归一与域判定（910·超64域口径收口） ====================

// 910：整数字面量文本归一化——任意进制（十进制/0x/0b/0o·大小写前缀）+
//   可选前导符号 + L/l/U/u 后缀（十六进制面 F 不剥·与 stripLiteralSuffix
//   同守卫）→ 规范十进制文本 "[-]digits"（去前导零·保留至少一位）。
//   无值（nullopt）= 超 u128 域（> 2^128-1·任意进制）或非法（无有效数字/
//   进制外字符——词法层已拒·错误恢复路径）；1303 结果化：原空串哨兵改
//   cn::Optional——调用方以 has_value() 分流或 value_or(显形默认) 处置，
//   空串值不再承担失败语义（空串与无值两态分离）。
//   存在意义：textExceedsInt64/textExceedsU32/U64 按「十进制文本长度」判域，
//   对 hex/oct/bin 文本误判（`0x8000000000000000` 18 字符 <19 判不超——759
//   探针实锚）；parseIntValue 走 stoll 对超承载文本抛异常静默回 0。本工具把
//   一切字面量文本先归一为十进制文本，后续「长度+字典序」判定大数安全且
//   单点（宿主语义/IR 两侧消费）。
cn::Optional<std::string> normalizeIntLiteralToDecimal(const std::string& raw);

// 910：规范十进制文本（可带 '-'/'+'）是否在有符号域 [-negBound, posBound]——
//   负号形态按 |digits| ≤ negBound（-2147483648=整32::MIN 域内合法）、正值
//   digits ≤ posBound。文本级比较（去前导零·大数安全·负号不参与字典序）。
bool decTextFitsSignedRange(const std::string& decText, const char* negBound,
                            const char* posBound);

// 910：规范十进制文本是否在无符号域 [0, bound]（负号形态恒 false）。
bool decTextFitsUnsignedRange(const std::string& decText, const char* bound);

// 1137（P0-1/#1076）：规范十进制文本是否在整数类型名（含「整数」别名；
//   枚举由调用方归并为 整32）的表示范围内——**类型域表单点收口**（原散于
//   semantic_literal_domain intLiteralFitsType 尾段的域表；情况标签条件域
//   判定与字面量判域共用单点，禁止第二份域表）。域口径与 910/728g/756/640
//   立法链一致：整8[-128,127]…整128[-2^127,2^127-1]、正族[0,bound]。
//   1137 审查修复 P0-C（甲案）：字符**不设独立条**——字符条件标签按整32
//   存储域判（001 §3.7/§3.7b 整源→字符位模式互通；Unicode 标量语义=乙案
//   须用户批准落 001）。
//   非整数类型名=防御恒真（调用方 isInteger/枚举归并已前置）。
bool decTextFitsTypeDomain(const std::string& decText,
                           const std::string& typeName);

// 1161（910 家族浮点面·#1076「字面量超目标类型表示范围=编译期拒」对齐）：
//   浮点字面量承载域判定——raw 文本（词法合法浮点 token：十进制段+可选
//   .小数段+可选 e 指数段+可选 f/F 后缀）数学值是否在目标浮点类型表示域内。
//   文本级判据（与 v2 自举侧 逐字同构·零分叉）：归一 尾数数字串S×10^E 与
//   上界（浮64=DBL_MAX≈1.7976931348623157e308 / 浮32=FLT_MAX≈
//   3.4028234663852886e38）按科学计数指数与有效数字字典序比较。
//   下溢 subnormal（1e-320）非超域=域内放行（宿主现状 subnormal 位模式
//   发射保真）；不用 strtod/errno（下溢 ERANGE 语义平台有差，不接触）。
//   非浮点类型名恒真（调用方 literalTypeOf 定级已前置）。
bool floatLiteralFitsTypeDomain(const std::string& raw,
                                const std::string& typeName);

// 910（788/420 提升链单点归属）：整数字面量 IR 运行时类型——带后缀按后缀
//   定级；无后缀恒定级整32 按**数学值域**提升（超 int32 双域→整64、超
//   int64 双域→整128；正32 超 u32→正64、正64 超 u64→正128）。判据全部按
//   normalizeIntLiteralToDecimal 规范文本（hex 超承载安全·替代原值级
//   literalValueExceedsInt32Domain(node->value) 与十进制文本长度判据的
//   hex 误判面）。消费点=IR 物化（ir_expr/ir_decl）、语义推断面
//   （semantic_expr）、实参推断（semantic_call）、二元折叠提升
//   （semantic_expr_binary）——四处原各自复制的提升链单点收口。
//   outStatus（可空）置域态：0=域内；1=超 整128 域；2=超 正128 域
//   （超 u128 域亦归此，按无符号后缀族分流）。返回=类型名（超域时返回可
//   诊断的定级类型：无符号后缀族→正128 / 其余→整128）。
std::string integerLiteralRuntimeType(const std::string& raw, int* outStatus);

// ==================== 十进制字面量文本保真折叠（728v·静态槽直存面共用） ====================

// 728v：十进制字面量文本 → 规范十进制文本（保真折叠·静态槽直存面共用）。
//   raw 经 stripLiteralSuffix 剥后缀后，剥可选前导 '-'/'+'，要求余下全为
//   十进制数字——成功=出参 out "[-]digits"（去前导零·保留至少一位）返回 true。
//   非纯十进制（0x/0b/0o 前缀、其它字符）、剥后空=false——调用方回退值级。
//   存在意义：parseIntValue 走 std::stoll，对超 INT64 承载的十进制文本抛
//   异常 catch 回 0（19 位 -9223372036854775808 / 2^63 实锚），值级折叠
//   输出静默错值 0；本工具保真输出源文本，发射层 `dq/.quad 文本` 按
//   mod 2^64 位模式解析——与顶层静态 raw 直存通道及 v2 位模式累加474
//   （474 分治）语义一致（零分叉）。
bool foldDecimalLiteralText(const std::string& raw, std::string& out);

// 790（569 采样轮 H1）：整数字面量文本全形态保真折叠——静态槽/裸全局直存
//   面共用（.data 直存文本的单一数值化通道）。
//   语义 = 十进制面沿用 foldDecimalLiteralText（保真·零行为变化）∨ 非十进制
//   面（0x/0b/0o + L/U 后缀 + 可选前导符号）按进制逐位 u64 累加 → 规范
//   十进制文本 "[-]digits"。
//   存在意义：0x/0b/0o/L 后缀形态原文透传 .data 直存被三平台汇编器拒绝
//   （win MASM `dq 0xFF` A2206 / `dq 111111111111L` A2048·linux GAS
//   `.quad ...L` 同病）；十进制为三后端通用文本（常量算术折叠输出同形态）。
//   超 u64 域文本（128 位字面量/超 u64 大十进制）返回 false——调用方按
//   原文保真交 128 位直存通道（parseInt128InitText 原生支持四形态解析）。
bool foldIntLiteralText(const std::string& raw, std::string& out);

// 将 i128 字面量文本拆分为 低64位:高64位 十六进制文本（如 "10000000000000000000" -> "0:8AC7230489E80000"）
// 支持 十进制/0x十六进制/0b二进制/0o八进制 前缀（文本已剥后缀）
// 无值（nullopt）=解析失败（无有效数字/非法数字字符——词法层已保证合法，
// 1303 结果化：原空串哨兵改 cn::Optional——失败不再与 "0:0" 等合法输出
// 形态共用值域，调用方 has_value()/value_or 显形处置）
cn::Optional<std::string> splitI128Text(const std::string& raw);

// ==================== 919：十进制文本大数四则（超承载常量表达式求值底座） ====================

// 919（640 侦察副产品·超承载字面量参与表达式求值双侧静默分叉根治）：
//   十进制文本四则（大数安全·平台无 128 位依赖）——纯文本进文本出，供
//   semantic_helpers.hpp cnEvalConstIntExprText919 常量表达式文本级求值消费。
//   输入约定=规范十进制文本（与 normalizeIntLiteralToDecimal 输出同形：
//   可带 '-'·无前导零·"0" 表零）；输出同形。
//   1360 结果化：原空串哨兵双语义（①超 u128 域 > 2^128-1·沿 910 口径——
//   中间与最终结果同判；②除数为零）改 cn::Optional 无值——空串不再承担
//   失败语义，消费点 .has_value() 强制分流（按 910 口径超一切整型域拒；
//   除零由求值器先行分流交既有编译期除零诊断链）。
//   存在意义：解析/折叠链（parseIntValue stoll 静默回 0、cnFoldConstIntExpr505
//   值级 int64 域）对超承载字面量（20 位十进制 99999999999999999999+0）输出
//   静默错值/回绕；文本级四则为常量表达式值域判定（intLiteralFitsType 表达式
//   扩展）提供大数安全的数学值。
cn::Optional<std::string> decTextAdd919(const std::string& a, const std::string& b);
cn::Optional<std::string> decTextSub919(const std::string& a, const std::string& b);
cn::Optional<std::string> decTextMul919(const std::string& a, const std::string& b);
cn::Optional<std::string> decTextDiv919(const std::string& a, const std::string& b);
cn::Optional<std::string> decTextMod919(const std::string& a, const std::string& b);
// 919：规范十进制文本取负（"0" → "0" 归一·"5"→"-5"·"-5"→"5"）
//   1360 结果化：空串输入=契约外（超域文本无取负语义）→ 无值；
//   合法输入恒有值（纯变换无失败面）
cn::Optional<std::string> decTextNegate919(std::string t);

} // namespace types
} // namespace cn_compiler
