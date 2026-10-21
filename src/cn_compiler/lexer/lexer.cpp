// 词法分析器实现：UTF-8字符处理、关键字匹配、字面量与运算符识别（Task 1.2）
// 覆盖：46个中文保留字（162-a 关键字体系收口：删 自动/实现，原始/多行 与 10 词上下文化后；plans/024 §七/§十）、整数/浮点/字符串/字符字面量（含原始/多行前缀）、
//       39个运算符（含++/--）、9个分隔符（含 ::）、注释（块注释嵌套）、错误诊断
#include <cstddef>
#include <string>

#include "cn_compiler/lexer/lexer.hpp"

namespace cn_compiler {

namespace {

// 从 pos 处解码一个UTF-8字符，返回Unicode码点，len 输出该字符的字节长度
// #103（1143 批·解析层分配点零分配改造）：ASCII 快路径——ASCII 单字节字符
//   （空白/标点/数字/ASCII 标识符）占源码字节大头，显式短路省多字节分支链
//   与续字节循环（语义零变化：len=1、cp=首字节；多字节路径逐字节同原实现）。
char32_t decodeUtf8(const std::string& s, std::size_t pos, int& len) {
    unsigned char first = static_cast<unsigned char>(s[pos]);
    if ((first & 0x80) == 0) {  // ASCII 区段=高位清零（0x00-0x7F 穷举判定）
        len = 1;
        return first;
    }
    len = 1;
    char32_t cp = first;
    if (first >= 0xF0) {
        len = 4;
        cp = first & 0x07;
    } else if (first >= 0xE0) {
        len = 3;
        cp = first & 0x0F;
    } else if (first >= 0xC0) {
        len = 2;
        cp = first & 0x1F;
    }
    for (int i = 1; i < len && pos + static_cast<std::size_t>(i) < s.size(); i++) {
        cp = (cp << 6) | (static_cast<unsigned char>(s[pos + static_cast<std::size_t>(i)]) & 0x3F);
    }
    return cp;
}

// ASCII数字判断（避免 cctype 对 char32_t 的不安全转换）
bool isAsciiDigit(char32_t c) { return c >= U'0' && c <= U'9'; }

// ASCII字母判断
bool isAsciiAlpha(char32_t c) {
    return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z');
}

// 46个保留字 -> TokenType 映射表（162-a 关键字体系收口后，plans/024 §七/§十）
// 控制流(10)：如果/否则/当/循环/返回/中断/继续/选择/情况/默认
// 类型(21)：整数/小数/整8~整128/正8~正128/浮32/浮64/布尔/字符/字符串/空类型/结构体/联合体/枚举
// 声明(3)：函数/变量/导入
// 常量(3)：真/假/无
// 安全区边界(1)：不安全
// OOP(5)：类/接口/自身/父类/友元
// 错误处理(2)：结果/可选
// 泛型(1)：泛型
// 注：以下词已移出保留字表（2026-09-14 用户裁决，plans/024）：
//   - 删除：自动（与 变量 二合一）/ 实现（死关键字）——Kw 枚举同轮删除
//   - 上下文化（parser 固定语法位置按 Identifier+文本识别，语法零变化）：
//     原始/多行（字面量前缀位）、保护/虚拟/重写/抽象（类成员修饰位）、
//     公开/私有（冒号标签位）、静态/常量（声明前缀位）、作为（重命名导入位）、
//     模块（顶层声明位）
//   - 既有上下文词：运算符/遍历/每个/属于/对（规范 §2.2 先例）
//
// #1215 阶段A：运行时 unordered_map 改为 constexpr 有序表 + 首码点分派。
//   原实现每次进程启动构建 46 项哈希表（堆节点分配 + 静态初始化），每次
//   标识符查找计算整串哈希并按桶逐字节比较。静态表零启动分配、零哈希：
//   首码点二分（34 组整数比较）+ 组内逐条精确比较，非关键字中文标识符多数
//   在整数二分阶段即被排除（微基准 5.14M 次查找：unordered_map 19.90ms /
//   全表字符串二分 26.84ms / 本方案 13.42ms）。
//   排序口径 = UTF-8 字节无符号序（keywordByteLess），static_assert 机械
//   保证表序——新增关键字插错位置或漏改分组表=编译期失败。
struct KeywordEntry {
    std::string_view text;
    TokenType type;
};

// UTF-8 字节序严格小于（无符号字节；与 std::string_view 语义一致但显式定序）
constexpr bool keywordByteLess(std::string_view a, std::string_view b) {
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();
    for (std::size_t i = 0; i < n; ++i) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[i]);
        if (ca != cb) return ca < cb;
    }
    return a.size() < b.size();
}

// 按 UTF-8 字节序升序排列（维护：新增条目必须插入正确位置，否则 static_assert 失败）
constexpr KeywordEntry kKeywordTable[] = {
    {"不安全", TokenType::Kw_Unsafe},
    {"中断", TokenType::Kw_Break},
    {"假", TokenType::Kw_False},
    {"函数", TokenType::Kw_Function},
    {"友元", TokenType::Kw_Friend},
    {"变量", TokenType::Kw_Var},
    {"可选", TokenType::Kw_Optional},
    {"否则", TokenType::Kw_Else},
    {"如果", TokenType::Kw_If},
    {"字符", TokenType::Kw_Char},
    {"字符串", TokenType::Kw_String},
    {"导入", TokenType::Kw_Import},
    {"小数", TokenType::Kw_Double},
    {"布尔", TokenType::Kw_Bool},
    {"当", TokenType::Kw_While},
    {"循环", TokenType::Kw_For},
    {"情况", TokenType::Kw_Case},
    {"接口", TokenType::Kw_Interface},
    {"整128", TokenType::Kw_Int128},
    {"整16", TokenType::Kw_Int16},
    {"整32", TokenType::Kw_Int32},
    {"整64", TokenType::Kw_Int64},
    {"整8", TokenType::Kw_Int8},
    {"整数", TokenType::Kw_Int},
    {"无", TokenType::Kw_None},
    {"枚举", TokenType::Kw_Enum},
    {"正128", TokenType::Kw_UInt128},
    {"正16", TokenType::Kw_UInt16},
    {"正32", TokenType::Kw_UInt32},
    {"正64", TokenType::Kw_UInt64},
    {"正8", TokenType::Kw_UInt8},
    {"泛型", TokenType::Kw_Generic},
    {"浮32", TokenType::Kw_Float32},
    {"浮64", TokenType::Kw_Float64},
    {"父类", TokenType::Kw_Super},
    {"真", TokenType::Kw_True},
    {"空类型", TokenType::Kw_Void},
    {"类", TokenType::Kw_Class},
    {"结构体", TokenType::Kw_Struct},
    {"结果", TokenType::Kw_Result},
    {"继续", TokenType::Kw_Continue},
    {"联合体", TokenType::Kw_Union},
    {"自身", TokenType::Kw_Self},
    {"返回", TokenType::Kw_Return},
    {"选择", TokenType::Kw_Switch},
    {"默认", TokenType::Kw_Default},
};

// 排序核验：返回逆序数（0=严格升序）。用计数式而非「if 逆序 return false」：
//   后者被静默形态门禁误判为默认值兜底；计数式编译期断言语义更直白。
constexpr std::size_t keywordTableOrderViolations() {
    constexpr std::size_t n = sizeof(kKeywordTable) / sizeof(kKeywordTable[0]);
    std::size_t violations = 0;
    for (std::size_t i = 1; i < n; ++i) {
        if (!keywordByteLess(kKeywordTable[i - 1].text, kKeywordTable[i].text)) {
            ++violations;
        }
    }
    return violations;
}
static_assert(keywordTableOrderViolations() == 0,
              "关键字表必须按 UTF-8 字节序严格升序排列");

// 首码点分组表：关键字表按 UTF-8 字节序 = Unicode 码点序排列 ⇒ 同首字符条目
//   连续。查询先二分首码点（整数比较），再在组内逐条精确比较——非关键字
//   中文标识符多数在二分阶段即被排除（微基准 5.14M 次查找：unordered_map
//   19.90ms / 全表二分 26.84ms / 本方案 13.42ms）。
//   维护：新增关键字须插入 kKeywordTable 正确位置并同步本表区间——下方
//   keywordFirstGroupsValid 机械核验（区间连续覆盖、组内首码点一致、组间
//   严格升序），漏改=编译期失败。
struct KeywordFirstGroup {
    char32_t first;      // 组首码点（组内全部关键字首字符相同）
    std::size_t begin;   // kKeywordTable 起始下标
    std::size_t count;   // 组内条目数
};

constexpr KeywordFirstGroup kKeywordFirstGroups[] = {
    {0x4E0D, 0, 1},   // 不安全
    {0x4E2D, 1, 1},   // 中断
    {0x5047, 2, 1},   // 假
    {0x51FD, 3, 1},   // 函数
    {0x53CB, 4, 1},   // 友元
    {0x53D8, 5, 1},   // 变量
    {0x53EF, 6, 1},   // 可选
    {0x5426, 7, 1},   // 否则
    {0x5982, 8, 1},   // 如果
    {0x5B57, 9, 2},   // 字符/字符串
    {0x5BFC, 11, 1},  // 导入
    {0x5C0F, 12, 1},  // 小数
    {0x5E03, 13, 1},  // 布尔
    {0x5F53, 14, 1},  // 当
    {0x5FAA, 15, 1},  // 循环
    {0x60C5, 16, 1},  // 情况
    {0x63A5, 17, 1},  // 接口
    {0x6574, 18, 6},  // 整128/整16/整32/整64/整8/整数
    {0x65E0, 24, 1},  // 无
    {0x679A, 25, 1},  // 枚举
    {0x6B63, 26, 5},  // 正128/正16/正32/正64/正8
    {0x6CDB, 31, 1},  // 泛型
    {0x6D6E, 32, 2},  // 浮32/浮64
    {0x7236, 34, 1},  // 父类
    {0x771F, 35, 1},  // 真
    {0x7A7A, 36, 1},  // 空类型
    {0x7C7B, 37, 1},  // 类
    {0x7ED3, 38, 2},  // 结构体/结果
    {0x7EE7, 40, 1},  // 继续
    {0x8054, 41, 1},  // 联合体
    {0x81EA, 42, 1},  // 自身
    {0x8FD4, 43, 1},  // 返回
    {0x9009, 44, 1},  // 选择
    {0x9ED8, 45, 1},  // 默认
};

// 解出文本首码点（与 Lexer::decodeUtf8 同口径：各自节循环、越界即停）
constexpr char32_t firstCodepoint(std::string_view text) {
    const unsigned char b0 = static_cast<unsigned char>(text[0]);
    if ((b0 & 0x80) == 0) return b0;
    const int len = b0 >= 0xF0 ? 4 : (b0 >= 0xE0 ? 3 : (b0 >= 0xC0 ? 2 : 1));
    char32_t cp = b0 & (len == 4 ? 0x07u : (len == 3 ? 0x0Fu : (len == 2 ? 0x1Fu : 0x7Fu)));
    for (int i = 1; i < len && static_cast<std::size_t>(i) < text.size(); ++i) {
        cp = (cp << 6) | (static_cast<unsigned char>(text[i]) & 0x3Fu);
    }
    return cp;
}

// 分组表机械核验（返回违规数，0=合法）：区间从 0 连续覆盖全表、组内首码点
//   一致、组间严格升序。计数式避免「if 违规 return false」触静默形态门禁。
constexpr std::size_t keywordFirstGroupViolations() {
    constexpr std::size_t tableSize = sizeof(kKeywordTable) / sizeof(kKeywordTable[0]);
    constexpr std::size_t groupCount =
        sizeof(kKeywordFirstGroups) / sizeof(kKeywordFirstGroups[0]);
    std::size_t violations = 0;
    std::size_t covered = 0;
    for (std::size_t g = 0; g < groupCount; ++g) {
        const KeywordFirstGroup& group = kKeywordFirstGroups[g];
        if (group.begin != covered || group.count == 0) ++violations;
        if (group.begin + group.count > tableSize) ++violations;
        if (firstCodepoint(kKeywordTable[group.begin].text) != group.first) ++violations;
        for (std::size_t i = 1; i < group.count; ++i) {
            if (firstCodepoint(kKeywordTable[group.begin + i].text) != group.first) {
                ++violations;
            }
        }
        if (g != 0 && !(kKeywordFirstGroups[g - 1].first < group.first)) ++violations;
        covered += group.count;
    }
    if (covered != tableSize) ++violations;
    return violations;
}
static_assert(keywordFirstGroupViolations() == 0,
              "关键字首码点分组表须连续覆盖全表且组序严格升序");

// 关键字域与映射唯一性核验（返回违规数，0=合法）：
//   ① 首码点 >= 0x80——lookupKeyword 对 ASCII 起始文本直接短路拒绝；新增
//      ASCII 关键字而未同步放宽快筛时，该词会被静默当标识符。确需 ASCII
//      关键字时删除本断言并同步放宽 lookupKeyword（维护点显式化，防静默）。
//   ② 词→TokenType 映射唯一（两词同类型=词法识别必错一路；词与类型的正确
//      对应由 tests/unit/lexer/test_keyword_table.cpp 逐项对照）。
constexpr char32_t kKeywordAsciiLimit = 0x80;
constexpr std::size_t keywordTableDomainViolations() {
    constexpr std::size_t n = sizeof(kKeywordTable) / sizeof(kKeywordTable[0]);
    std::size_t violations = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (firstCodepoint(kKeywordTable[i].text) < kKeywordAsciiLimit) ++violations;
        for (std::size_t j = i + 1; j < n; ++j) {
            if (static_cast<int>(kKeywordTable[i].type) ==
                static_cast<int>(kKeywordTable[j].type)) {
                ++violations;
            }
        }
    }
    return violations;
}
static_assert(keywordTableDomainViolations() == 0,
              "关键字首码点须 >= 0x80（ASCII 关键字须同步放宽词法快筛）且词→TokenType 映射唯一");

} // namespace

// 查找关键字：命中返回true并设置类型
// #103（1143 批）：快筛——46 保留字首字符全为汉字（多字节头字节），ASCII 起始
//   文本直接拒绝（命中域不变；非关键字中文标识符走首码点分派兜底）。
//   ★维护警示：新增 ASCII 关键字须同步放宽本快筛（keywordTableDomainViolations
//   的 static_assert 会把漏改变成编译期失败）。
// #1215 阶段A：constexpr 有序表 + 首码点分派（无哈希、无启动分配、无堆节点）。
bool Lexer::lookupKeyword(std::string_view text, TokenType& type) {
    const unsigned char b0 = text.empty() ? 0u : static_cast<unsigned char>(text[0]);
    if ((b0 & 0x80) == 0) return false;  // ASCII 区段=高位清零
    const char32_t first = firstCodepoint(text);
    std::size_t lo = 0;
    std::size_t hi = sizeof(kKeywordFirstGroups) / sizeof(kKeywordFirstGroups[0]);
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (kKeywordFirstGroups[mid].first < first) {
            lo = mid + 1;
            continue;  // 本组首码点小于查询首码点：右半区
        }
        hi = mid;
    }
    if (lo >= sizeof(kKeywordFirstGroups) / sizeof(kKeywordFirstGroups[0]) ||
        kKeywordFirstGroups[lo].first != first) {
        return false;  // 首码点不在关键字域（非关键字中文标识符主路径）
    }
    const KeywordFirstGroup& group = kKeywordFirstGroups[lo];
    for (std::size_t i = 0; i < group.count; ++i) {
        const KeywordEntry& entry = kKeywordTable[group.begin + i];
        if (entry.text == text) {
            type = entry.type;
            return true;
        }
    }
    return false;
}

// 报告一条词法错误到诊断引擎
void Lexer::reportError(const SourceLocation& loc, const std::string& message) {
    diagnostics_.report(DiagnosticLevel::Error, loc, message);
}

// 是否到达源码末尾
bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

// 前进一个UTF-8字符，返回其Unicode码点（\n换行时更新行号/列号）
char32_t Lexer::advance() {
    if (isAtEnd()) return 0;
    int len = 1;
    char32_t cp = decodeUtf8(source_, pos_, len);
    pos_ += static_cast<std::size_t>(len);
    if (cp == U'\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return cp;
}

// 查看当前字符（不前进）
char32_t Lexer::peek() const {
    if (isAtEnd()) return 0;
    int len = 0;
    return decodeUtf8(source_, pos_, len);
}

// 查看向后第n个字符（不前进，n>=1；越界返回0）
// 注意：必须跳过当前位置再前进n个，peekNext(1) 返回下一个字符
char32_t Lexer::peekNext(int n) const {
    std::size_t p = pos_;
    char32_t cp = 0;
    // 共前进 n+1 个字符：第1次跳过当前位置，后n次定位到目标字符
    for (int i = 0; i <= n; i++) {
        if (p >= source_.size()) return 0;
        int len = 1;
        cp = decodeUtf8(source_, p, len);
        p += static_cast<std::size_t>(len);
    }
    return cp;
}

// 返回当前位置字符的UTF-8字节串（不前进；越界返回空串）
std::string Lexer::currentUtf8() const {
    if (isAtEnd()) return "";
    int len = 1;
    decodeUtf8(source_, pos_, len);
    return source_.substr(pos_, static_cast<std::size_t>(len));
}

// 当前位置（文件名/行/列）
SourceLocation Lexer::currentLocation() const {
    // #1154：文件名已在 Lexer 构造时驻留一次——逐 Token 只拷指针，免重复驻留
    return SourceLocation::fromInternedFileName(*internedFileName_, line_, column_);
}

// 跳过空白与注释：//单行注释、/* */块注释（支持嵌套），循环直到无空白/注释
void Lexer::skipWhitespaceAndComments() {
    bool progressed = true;
    while (progressed) {
        progressed = false;
        // 空白（换行由 advance 更新行号）
        while (!isAtEnd()) {
            char32_t c = peek();
            if (c == U' ' || c == U'\t' || c == U'\r' || c == U'\n') {
                advance();
                progressed = true;
            } else {
                break;
            }
        }
        // 单行注释 // 到行尾
        if (peek() == U'/' && peekNext(1) == U'/') {
            advance();
            advance();
            while (!isAtEnd() && peek() != U'\n') advance();
            progressed = true;
        }
        // 块注释 /* */（支持嵌套）
        if (peek() == U'/' && peekNext(1) == U'*') {
            SourceLocation loc = currentLocation();
            advance();
            advance();
            int depth = 1;
            while (!isAtEnd() && depth > 0) {
                if (peek() == U'/' && peekNext(1) == U'*') {
                    advance();
                    advance();
                    depth++;
                } else if (peek() == U'*' && peekNext(1) == U'/') {
                    advance();
                    advance();
                    depth--;
                } else {
                    advance();
                }
            }
            if (depth > 0) reportError(loc, "未闭合的块注释");
            progressed = true;
        }
    }
}

// T21（306-a 波次2·方案甲）：标识符字符判定单点化——
//   「中文」=汉字区段（CJK 统一表意 U+4E00~U+9FFF+扩展A U+3400~U+4DBF+
//   扩展B U+20000~U+2A6DF）；拒绝全角形式拉丁/中文标点/假名/谚文等。
//   **主分发与收集循环必须共用本判定**：原 306-a 首版只收紧收集循环而
//   主分发仍 `c >= 0x80` 全放行 -> 区段外字符进收集立即 break 返回空
//   token 且不消费 -> 主分发无限空转（每轮一个空 Identifier token）
//   -> 诊断/AST 无限膨胀（实测 28.7GB 触发系统 OOM 连坐 ZCode）。
bool Lexer::isIdentifierStartChar(char32_t c) const {
    return isAsciiAlpha(c) || c == U'_' ||
           (c >= 0x4E00 && c <= 0x9FFF) ||    // CJK 统一表意
           (c >= 0x3400 && c <= 0x4DBF) ||    // CJK 扩展A
           (c >= 0x20000 && c <= 0x2A6DF);    // CJK 扩展B
}
bool Lexer::isIdentifierContChar(char32_t c) const {
    return isIdentifierStartChar(c) || isAsciiDigit(c);
}

// 读取标识符或关键字（UTF-8中文/ASCII字母/数字/下划线连续收集，最长匹配）
// 特殊处理：原始/多行前缀后紧跟引号时转入带前缀字符串读取
Token Lexer::readIdentifierOrKeyword() {
    SourceLocation loc = currentLocation();
    // #103（1143 批）：标识符字节区间单次截取——原逐字符 `text += currentUtf8()`
    //   每字符构造一个临时 std::string；advance 按解码长度推进字节，扫描所得区间
    //   [start, pos_) 与逐字符累积串逐字节相同（损坏 UTF-8 尾亦等价：substr 自动
    //   截到串尾=原 currentUtf8 越界保护同口径；字符判定循环体不变）。
    const std::size_t idStart = pos_;
    while (!isAtEnd()) {
        char32_t c = peek();
        // T21（306-a 波次2·方案甲）：「中文」限定=汉字区段——CJK 统一表意
        //   U+4E00~U+9FFF + 扩展A U+3400~U+4DBF + 扩展B U+20000~U+2A6DF；
        //   拒绝全角形式拉丁/中文标点/假名/谚文等非表意字符入标识符
        //   （对照 Rust XIDStart/XIDContinue 业界标准；ASCII 面不变）。
        //   原实现 `c >= 0x80` 对全部非 ASCII 放行=字符集过宽（行59/60 立案）。
        if (!isIdentifierContChar(c)) break;
        advance();
    }
    std::string text = source_.substr(idStart, pos_ - idStart);
    // 原始/多行 前缀上下文识别（162-a 非保留化，plans/024 §7.3）：词后紧跟
    //   引号才是字符串字面量；前缀后仅空白再跟引号=前缀误用，报友好诊断；
    //   其余语境回退普通标识符（原始/多行 可作变量名/字段名/函数名）
    if (text == "原始" || text == "多行") {
        if (peek() == U'"') {
            return readPrefixedString(text, loc);
        }
        if (prefixAbuseNextQuote()) {
            reportError(loc, "字符串前缀后必须紧跟引号");
        }
        return Token(TokenType::Identifier, text, loc);
    }
    TokenType type;
    if (lookupKeyword(text, type)) {
        return Token(type, text, loc);
    }
    // 组合前缀：原始多行 / 多行原始（连续无空白）后紧跟引号 → 字符串字面量
    if (peek() == U'"' && (text == "原始多行" || text == "多行原始")) {
        return readPrefixedString(text, loc);
    }
    return Token(TokenType::Identifier, text, loc);
}

// 前缀误用探测（162-a）：当前位置起跳过空格/制表后是否紧跟引号——
//   命中则由调用方报「字符串前缀后必须紧跟引号」（诊断后不消费，回退标识符）
bool Lexer::prefixAbuseNextQuote() const {
    std::size_t p = pos_;
    while (p < source_.size() &&
           (source_[p] == ' ' || source_[p] == '	')) {
        p++;
    }
    return p < source_.size() && source_[p] == '"';
}

// 读取带前缀的字符串字面量：支持 原始 / 多行 / 原始多行 / 多行原始 组合
// 字符串Token的value保留完整原始文本（含前缀与引号），供语法分析器统一解码
// prefixText 已由调用方收集完整前缀（含组合前缀，如"原始多行"），find 判断即可
Token Lexer::readPrefixedString(const std::string& prefixText, const SourceLocation& loc) {
    const bool raw = (prefixText.find("原始") != std::string::npos);
    const bool multiLine = (prefixText.find("多行") != std::string::npos);
    if (peek() != U'"') {
        reportError(loc, "字符串前缀后必须紧跟引号");
        return Token(TokenType::StringLiteral, "", loc);
    }
    return Token(TokenType::StringLiteral, prefixText + readStringBody(multiLine, raw), loc);
}

// 读取字符串体：返回含引号的原始文本
// multiLine=true 使用三引号"""..."""（可跨行）；raw=true 不处理转义
std::string Lexer::readStringBody(bool multiLine, bool raw) {
    // 多行前缀但实际只有单引号时，降级为普通字符串
    const bool isTripleQuote = peek() == U'"' && peekNext(1) == U'"' && peekNext(2) == U'"';
    if (multiLine && !isTripleQuote) multiLine = false;

    std::string text;
    if (multiLine) {
        // 消费三引号 """
        for (int i = 0; i < 3 && peek() == U'"'; i++) {
            text += currentUtf8();
            advance();
        }
        // 读取到下一个 """ 为止（可跨行，\n由 advance 更新行号）
        while (!isAtEnd()) {
            if (peek() == U'"' && peekNext(1) == U'"' && peekNext(2) == U'"') {
                text += "\"\"\"";
                advance();
                advance();
                advance();
                return text;
            }
            if (peek() == U'\\' && !raw) {
                text += currentUtf8();
                advance();
                if (isAtEnd()) break;
                // 282-a T5 多行段：\u{ 结构感知+三态校验（对齐普通段 278-a
                //   同款——原宽松 +1 字符收集不识别结构；raw 多行不处理转义
                //   保持 raw 语义）
                if (peek() == U'u') {
                    text += currentUtf8();
                    advance();
                    if (peek() != U'{') {
                        reportError(currentLocation(), "无效的Unicode转义：缺少 {");
                        text += currentUtf8();
                        advance();
                        continue;
                    }
                    text += currentUtf8();
                    advance();
                    const std::size_t hexStart = text.size();
                    while (!isAtEnd() && peek() != U'}') {
                        text += currentUtf8();
                        advance();
                    }
                    const std::string hexBody = text.substr(hexStart);
                    if (peek() == U'}') {
                        text += currentUtf8();
                        advance();
                    } else {
                        reportError(currentLocation(), "无效的Unicode转义：缺少 }");
                    }
                    validateUnicodeEscape(hexBody, currentLocation());
                    continue;
                }
                text += currentUtf8();
                advance();
                continue;
            }
            text += currentUtf8();
            advance();
        }
        reportError(currentLocation(), "未闭合的多行字符串");
        return text;
    }

    // 普通或原始字符串：单引号包裹
    text += currentUtf8();
    advance(); // 消费开引号 "
    while (!isAtEnd()) {
        char32_t c = peek();
        if (c == U'"') {
            text += currentUtf8();
            advance();
            return text;
        }
        if (c == U'\n') {
            reportError(currentLocation(), "字符串字面量不能跨行");
            return text;
        }
        if (c == U'\\' && !raw) {
            text += currentUtf8();
            advance();
            if (isAtEnd()) break;
            // 278-a T5：\u{ 结构感知+三态校验（原宽松 +1 字符收集不识别结构）
            if (peek() == U'u') {
                text += currentUtf8();
                advance();
                if (peek() == U'{') {
                    text += currentUtf8();
                    advance();
                    const std::size_t hexStart = text.size();
                    while (!isAtEnd() && peek() != U'}') {
                        text += currentUtf8();
                        advance();
                    }
                    const std::string hexBody = text.substr(hexStart);
                    if (peek() == U'}') {
                        text += currentUtf8();
                        advance();
                    } else {
                        reportError(currentLocation(), "无效的Unicode转义：缺少 }");
                    }
                    validateUnicodeEscape(hexBody, currentLocation());
                    continue;
                }
                reportError(currentLocation(), "无效的Unicode转义：缺少 {");
                text += currentUtf8();
                advance();
                continue;
            }
            text += currentUtf8();
            advance();
            continue;
        }
        text += currentUtf8();
        advance();
    }
    reportError(currentLocation(), "未闭合的字符串字面量");
    return text;
}

// 收集整数/浮点后缀：f/L/LL/U/UL/ULL（规格书4.3节）
// 注意：不在数字收集阶段处理a-f，避免与十六进制数字冲突
void Lexer::collectNumberSuffix(std::string& text) {
    while (!isAtEnd()) {
        const char32_t c = peek();
        if (c == U'f' || c == U'F' || c == U'l' || c == U'L' ||
            c == U'u' || c == U'U') {
            text += currentUtf8();
            advance();
        } else {
            break;
        }
    }
}

// 读取数字字面量：十进制/十六进制(0x)/二进制(0b)/八进制(0o)/浮点（小数与科学计数法）
Token Lexer::readNumber() {
    SourceLocation loc = currentLocation();
    std::string text;
    bool isFloat = false;

    // ---- 前缀进制 ----
    if (peek() == U'0' && (peekNext(1) == U'x' || peekNext(1) == U'X')) {
        text += currentUtf8();
        advance();
        text += currentUtf8();
        advance();
        while (!isAtEnd()) {
            const char32_t c = peek();
            if (isAsciiDigit(c) || (c >= U'a' && c <= U'f') || (c >= U'A' && c <= U'F')) {
                text += currentUtf8();
                advance();
            } else {
                break;
            }
        }
        collectNumberSuffix(text);
        return Token(TokenType::IntegerLiteral, text, loc);
    }
    if (peek() == U'0' && (peekNext(1) == U'b' || peekNext(1) == U'B')) {
        text += currentUtf8();
        advance();
        text += currentUtf8();
        advance();
        while (!isAtEnd()) {
            const char32_t c = peek();
            if (c == U'0' || c == U'1') {
                text += currentUtf8();
                advance();
            } else {
                break;
            }
        }
        collectNumberSuffix(text);
        return Token(TokenType::IntegerLiteral, text, loc);
    }
    if (peek() == U'0' && (peekNext(1) == U'o' || peekNext(1) == U'O')) {
        text += currentUtf8();
        advance();
        text += currentUtf8();
        advance();
        while (!isAtEnd()) {
            const char32_t c = peek();
            if (c >= U'0' && c <= U'7') {
                text += currentUtf8();
                advance();
            } else {
                break;
            }
        }
        collectNumberSuffix(text);
        return Token(TokenType::IntegerLiteral, text, loc);
    }

    // ---- 十进制整数部分 ----
    while (!isAtEnd()) {
        const char32_t c = peek();
        if (isAsciiDigit(c)) {
            text += currentUtf8();
            advance();
        } else {
            break;
        }
    }
    // ---- 浮点：小数点后必须跟数字 ----
    if (peek() == U'.' && isAsciiDigit(peekNext(1))) {
        isFloat = true;
        text += currentUtf8();
        advance(); // 消费 .
        while (!isAtEnd() && isAsciiDigit(peek())) {
            text += currentUtf8();
            advance();
        }
    }
    // ---- 浮点：科学计数法 e/E 后跟数字或符号 ----
    if (peek() == U'e' || peek() == U'E') {
        const char32_t next = peekNext(1);
        if (isAsciiDigit(next) || next == U'+' || next == U'-') {
            isFloat = true;
            text += currentUtf8();
            advance(); // 消费 e/E
            if (peek() == U'+' || peek() == U'-') {
                text += currentUtf8();
                advance();
            }
            while (!isAtEnd() && isAsciiDigit(peek())) {
                text += currentUtf8();
                advance();
            }
        }
    }
    collectNumberSuffix(text);
    return Token(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral, text, loc);
}

// 278-a T5：\u{...} 十六进制体三态校验（空转义/超码点>U+10FFFF/代理区
//   U+D800~U+DFFF）+非法十六进制字符——四态均 reportError（词法期拒绝，
//   消除 decodeEscapes 生成非法 UTF-8 字节的面）。
bool Lexer::validateUnicodeEscape(const std::string& hexBody, const SourceLocation& loc) {
    if (hexBody.empty()) {
        reportError(loc, "无效的Unicode转义：\\u{} 空转义（须为 Unicode 码点）");
        return false;
    }
    char32_t cp = 0;
    for (char h : hexBody) {
        cp *= 16;
        if (h >= '0' && h <= '9') cp += h - '0';
        else if (h >= 'a' && h <= 'f') cp += h - 'a' + 10;
        else if (h >= 'A' && h <= 'F') cp += h - 'A' + 10;
        else {
            reportError(loc, "无效的Unicode转义：非十六进制字符");
            return false;
        }
    }
    if (cp > 0x10FFFF) {
        reportError(loc, "无效的Unicode转义：码点超过 U+10FFFF 上限");
        return false;
    }
    if (cp >= 0xD800 && cp <= 0xDFFF) {
        reportError(loc, "无效的Unicode转义：代理区码点（U+D800~U+DFFF）不可直接使用");
        return false;
    }
    return true;
}

// 读取字符字面量：'X'、'\n'、'\t'、'\0'、'\\'、'\''、'\"'、'\u{XXXX}'
Token Lexer::readChar() {
    SourceLocation loc = currentLocation();
    advance(); // 消费开引号（单引号）
    std::string text = "'";
    if (isAtEnd()) {
        reportError(loc, "未闭合的字符字面量");
        return Token(TokenType::CharLiteral, text, loc);
    }
    if (peek() == U'\\') {
        // 转义序列
        text += currentUtf8();
        advance(); // 消费反斜杠
        if (isAtEnd()) {
            reportError(loc, "未闭合的字符字面量");
            return Token(TokenType::CharLiteral, text, loc);
        }
        const char32_t esc = peek();
        if (esc == U'u') {
            // Unicode转义 \u{XXXX}——278-a T5：结构严格化（缺 { 拒绝）+
            //   收集 hex 体后三态校验（空/超限/代理区）
            text += currentUtf8();
            advance(); // 消费 u
            if (peek() != U'{') {
                reportError(loc, "无效的Unicode转义：缺少 {");
            } else {
                text += currentUtf8();
                advance(); // 消费 {
                const std::size_t hexStart = text.size();
                while (!isAtEnd() && peek() != U'}') {
                    text += currentUtf8();
                    advance();
                }
                const std::string hexBody = text.substr(hexStart);
                if (peek() == U'}') {
                    text += currentUtf8();
                    advance();
                } else {
                    reportError(loc, "无效的Unicode转义：缺少 }");
                }
                validateUnicodeEscape(hexBody, loc);
            }
        } else if (esc == U'n' || esc == U't' || esc == U'r' || esc == U'0' ||
                   esc == U'\\' || esc == U'\'' || esc == U'"') {
            text += currentUtf8();
            advance();
        } else {
            reportError(loc, "无效的转义序列");
            text += currentUtf8();
            advance();
        }
    } else {
        // 普通字符（含多字节中文）
        text += currentUtf8();
        advance();
    }
    // 闭引号
    if (peek() == U'\'') {
        text += currentUtf8();
        advance();
    } else {
        reportError(loc, "未闭合的字符字面量");
    }
    return Token(TokenType::CharLiteral, text, loc);
}

// 读取运算符或分隔符：三字符(<<= >>=) > 双字符(++ -- += -= ...) > 单字符（贪婪最长匹配）
Token Lexer::readOperatorOrDelimiter() {
    SourceLocation loc = currentLocation();
    const char32_t c = peek();

    // ---- 三字符运算符：<<= >>= ----
    if ((c == U'<' || c == U'>') && peekNext(1) == c && peekNext(2) == U'=') {
        const bool isShiftLeft = (c == U'<');
        advance();
        advance();
        advance();
        return Token(isShiftLeft ? TokenType::LessLessEqual : TokenType::GreaterGreaterEqual,
                     isShiftLeft ? "<<=" : ">>=", loc);
    }

    // ---- 双字符运算符（贪婪匹配） ----
    switch (c) {
        case U'+':
            if (peekNext(1) == U'+') { advance(); advance(); return Token(TokenType::PlusPlus, "++", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::PlusEqual, "+=", loc); }
            break;
        case U'-':
            if (peekNext(1) == U'-') { advance(); advance(); return Token(TokenType::MinusMinus, "--", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::MinusEqual, "-=", loc); }
            if (peekNext(1) == U'>') { advance(); advance(); return Token(TokenType::Arrow, "->", loc); }
            break;
        case U'*':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::StarEqual, "*=", loc); }
            break;
        case U'/':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::SlashEqual, "/=", loc); }
            break;
        case U'%':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::PercentEqual, "%=", loc); }
            break;
        case U'=':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::EqualEqual, "==", loc); }
            break;
        case U'!':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::BangEqual, "!=", loc); }
            break;
        case U':':
            // 模块路径分隔符 ::（v2.0）：贪婪匹配优先于单字符 Colon
            if (peekNext(1) == U':') { advance(); advance(); return Token(TokenType::ColonColon, "::", loc); }
            break;
        case U'<':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::LessEqual, "<=", loc); }
            if (peekNext(1) == U'<') { advance(); advance(); return Token(TokenType::LessLess, "<<", loc); }
            break;
        case U'>':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::GreaterEqual, ">=", loc); }
            if (peekNext(1) == U'>') { advance(); advance(); return Token(TokenType::GreaterGreater, ">>", loc); }
            break;
        case U'&':
            if (peekNext(1) == U'&') { advance(); advance(); return Token(TokenType::AndAnd, "&&", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::AmpEqual, "&=", loc); }
            break;
        case U'|':
            if (peekNext(1) == U'|') { advance(); advance(); return Token(TokenType::OrOr, "||", loc); }
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::PipeEqual, "|=", loc); }
            break;
        case U'^':
            if (peekNext(1) == U'=') { advance(); advance(); return Token(TokenType::CaretEqual, "^=", loc); }
            break;
        default:
            break;
    }

    // ---- 单字符运算符/分隔符 ----
    switch (c) {
        case U'+': advance(); return Token(TokenType::Plus, "+", loc);
        case U'-': advance(); return Token(TokenType::Minus, "-", loc);
        case U'*': advance(); return Token(TokenType::Star, "*", loc);
        case U'/': advance(); return Token(TokenType::Slash, "/", loc);
        case U'%': advance(); return Token(TokenType::Percent, "%", loc);
        case U'=': advance(); return Token(TokenType::Equal, "=", loc);
        case U'<': advance(); return Token(TokenType::Less, "<", loc);
        case U'>': advance(); return Token(TokenType::Greater, ">", loc);
        case U'!': advance(); return Token(TokenType::Bang, "!", loc);
        case U'&': advance(); return Token(TokenType::Amp, "&", loc);
        case U'|': advance(); return Token(TokenType::Pipe, "|", loc);
        case U'^': advance(); return Token(TokenType::Caret, "^", loc);
        case U'~': advance(); return Token(TokenType::Tilde, "~", loc);
        case U'(': advance(); return Token(TokenType::LeftParen, "(", loc);
        case U')': advance(); return Token(TokenType::RightParen, ")", loc);
        case U'{': advance(); return Token(TokenType::LeftBrace, "{", loc);
        case U'}': advance(); return Token(TokenType::RightBrace, "}", loc);
        case U'[': advance(); return Token(TokenType::LeftBracket, "[", loc);
        case U']': advance(); return Token(TokenType::RightBracket, "]", loc);
        case U';': advance(); return Token(TokenType::Semicolon, ";", loc);
        case U',': advance(); return Token(TokenType::Comma, ",", loc);
        case U'.': advance(); return Token(TokenType::Dot, ".", loc);
        case U':': advance(); return Token(TokenType::Colon, ":", loc);  // 单冒号（:: 已在双字符分支处理）
        case U'?': advance(); return Token(TokenType::Question, "?", loc);
        default: {
            // 未知字符：报告诊断并跳过（错误恢复）
            const std::string bad = currentUtf8();
            reportError(loc, "非法字符: " + bad);
            advance();
            return Token(TokenType::Unknown, bad, loc);
        }
    }
}

// 主入口：分析源码返回Token流（末尾含文件结束Token）
// Task 6.6 条件编译：先经 Preprocessor 裁剪（保留行号），再切分 Token。
//   裁剪后的非激活行替换为空白，行号/列号与原始源码一致，诊断定位不偏移。
std::vector<Token> Lexer::tokenize() {
    // 预处理：条件编译指令裁剪（#定义/#如果定义/#否则/#结束如果）
    Preprocessor preprocessor(source_, fileName_, diagnostics_, macros_);
    source_ = preprocessor.process();
    pos_ = 0;
    line_ = 1;
    column_ = 1;

    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (isAtEnd()) {
            tokens.push_back(Token(TokenType::EndOfFile, "", currentLocation()));
            break;
        }
        const char32_t c = peek();
        // 标识符/关键字（含中文·T21 区段口径——与收集循环共用判定，
        //   区段外字符落 readOperatorOrDelimiter 的未知字符错误恢复分支）
        if (isIdentifierStartChar(c)) {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }
        // 数字字面量
        if (isAsciiDigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }
        // 字符串字面量
        if (c == U'"') {
            SourceLocation loc = currentLocation();
            tokens.push_back(Token(TokenType::StringLiteral, readStringBody(false, false), loc));
            continue;
        }
        // 字符字面量
        if (c == U'\'') {
            tokens.push_back(readChar());
            continue;
        }
        // 运算符/分隔符/未知字符
        tokens.push_back(readOperatorOrDelimiter());
    }
    return tokens;
}

} // namespace cn_compiler
