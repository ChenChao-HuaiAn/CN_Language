// 语法分析器实现：声明解析族（350 重构E2 自 parser.cpp 纯机械搬移）
// 职责（函数体逐字搬移·零语义变化；声明保持在 parser.hpp 原位）：
//   parseParamDecl    —— 参数声明：类型 名称（含常量参数/函数指针参数/默认值）
//   parseFunctionDecl —— 函数声明：函数 名称(参数) [-> 返回类型] { 体 }
//   parseStructDecl   —— 结构体/联合体声明（字段/手动释放/函数指针字段）
//   parseEnumDecl     —— 枚举声明（成员 [= 值]，支持负值）
//   parseStructInit   —— 结构体/类初始化：类型名{ 字段 = 值 } / 位置初始化
#include <memory>
#include <string>
#include <utility>

#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 解析参数声明：类型 名称（CN规范；冒号后置 名称: 类型 非规范语法，A8 收口拒绝）
std::unique_ptr<ParamDecl> Parser::parseParamDecl() {
    auto param = std::make_unique<ParamDecl>();
    param->location = current().getLocation();
    // plans/019 阶段3（2026-09-10）：常量 只读引用参数前缀（常量 向量<整64>&
    //   数据）——常量 后随类型起点（类型关键字/标识符类型）时消费并置位；
    //   仅 常量 后跟 & 的形态暂不支持（须显式类型）。
    if (checkText("常量") &&
        (isTypeKeyword(peek(1).getType()) ||
         peek(1).getType() == TokenType::Identifier)) {
        param->isConstParam = true;
        advance();  // 消费 常量
    }
    // 函数指针形参统一探测（560/627·001 §5.8）：命名标量形态 `T ( * 名 ) (`——
    //   返回类型 T 可为内建类型关键字（整32(*回调)(整32)·既有）**或自定义类型名**
    //   （点(*回调)(整32)·560/627 缺陷本面：此前仅 isTypeKeyword 分支有探测，
    //   Identifier 分支无路径 -> 「预期参数名」+级联「预期顶层声明」）。
    //   funcPtrDeclAhead 纯读零消费——探测成功才调 parseFuncPtrType
    //   （其失败路径已 advance，有消费副作用）。参数位=命名标量（named+标量·
    //   001 条文明示参数位无数组形态·与既有内建分支手写探测等价）。
    if (funcPtrDeclAhead(true, false, false)) {
        if (parseFuncPtrType(param->funcPtr)) {
            // 函数指针参数：funcPtr 已填充，参数名在 funcPtr.name
            param->name = param->funcPtr.name;
            return param;
        }
    }
    if (isTypeKeyword(currentType())) {
        // 类型前置：整32 a / 整32* p / 整32[5] a（Task 2.4 复合类型参数）
        param->typeName = parseTypeNameEx();
        if (check(TokenType::Identifier)) {
            param->name = current().getValue();
            advance();
        } else {
            reportErrorHere("预期参数名");
        }
    } else if (check(TokenType::Identifier)) {
        // 自定义类型前置探测：Foo x / Foo* p / Foo[5] a（Task 2.7 集成修复——
        // 此前仅 isTypeKeyword 支持复合类型参数，结构体/枚举参数无法解析）
        // 识别模式：标识符 后跟 标识符（Foo x）、*、[ 长度
        // Task 3.1/3.8：& 引用后缀（账户& 账、T& a）——标识符 后跟 & 后跟 标识符
        // 自举前置 A-3a（plans/004）：模板类型参数（向量<字符串> 词表）——
        //   泛型实例化类型作函数参数；parseTypeNameEx 已支持 <实参> 消费，
        //   此处补识别（此前漏 Less 判定 -> "预期参数名，实际为 '<'"）
        if (peek(1).getType() == TokenType::Identifier ||
            peek(1).getType() == TokenType::Star ||
            peek(1).getType() == TokenType::LeftBracket ||
            peek(1).getType() == TokenType::Less ||
            (peek(1).getType() == TokenType::Amp &&
             peek(2).getType() == TokenType::Identifier)) {
            param->typeName = parseTypeNameEx();
            if (check(TokenType::Identifier)) {
                param->name = current().getValue();
                advance();
            } else {
                reportErrorHere("预期参数名");
            }
            return param;
        }
        // 前一个标识符是自定义类型名（Foo x，无复合后缀）；
        // 冒号后置标注（x: 整32）非规范语法（spec 03 否决），一律拒绝（A8 收口）
        param->typeName = current().getValue();
        advance();
        if (check(TokenType::Identifier)) {
            param->name = current().getValue();
            advance();
        } else {
            param->name.clear();
            reportErrorHere("预期参数名");
        }
    } else {
        reportErrorHere("预期参数声明，实际为 '" + current().getValue() + "'");
        synchronize();
    }
    // 默认参数：类型 名称 = 常量表达式（规格书04-一C，Task 2.10）
    // 解析为普通表达式（字面量/一元负号常量），常量性由语义层校验
    if (check(TokenType::Equal) && !param->name.empty()) {
        advance();
        param->hasDefault = true;
        param->defaultExpr = parseExpr();
    }
    return param;
}

// 解析函数声明：函数 名称(参数列表) [-> 返回类型] { 函数体 }
std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    auto func = std::make_unique<FunctionDecl>();
    func->location = current().getLocation();
    advance();  // 消费"函数"关键字
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期函数名");
        synchronize();
        return func;
    }
    func->name = current().getValue();
    advance();
    consume(TokenType::LeftParen, "'('");
    // 参数列表（可空）
    if (!check(TokenType::RightParen)) {
        // 1216：形参表预分配（常见 1-4 个参数）——替代几何增长多次分配
        func->params.reserve(4);
        do {
            func->params.push_back(parseParamDecl());
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "')'");
    // 返回类型（-> 类型，可省略表示无返回值；Task 2.4 支持指针返回类型）
    if (check(TokenType::Arrow)) {
        advance();
        // 040 甲案（082③·001 §5.8）：函数指针返回类型位 `-> 整32(*)(整32)`（匿名）
        //   ——规范化文本入 returnType，签名/返回检查/IR 全链按 8 字节标量指针族
        if (funcPtrDeclAhead(false, false, true)) {
            FuncPtrTypeInfo retFp;
            if (parseFuncPtrType(retFp)) {
                func->returnType = retFp.toString();
            }
        } else {
            func->returnType = parseTypeNameEx();
        }
    }
    // 函数体（可为空 = 函数原型声明）
    if (check(TokenType::LeftBrace)) {
        func->body = parseBlockStmt();
    }
    return func;
}

// 解析结构体/联合体声明：结构体 名 { 类型 字段; ... } / 联合体 名 { ... }（Task 2.7）
// 字段分隔：换行/分号均可（规格书05示例为换行，兼容分号写法）
std::unique_ptr<StructDecl> Parser::parseStructDecl(bool isUnion) {
    auto decl = std::make_unique<StructDecl>();
    decl->isUnion = isUnion;
    decl->location = current().getLocation();
    advance();  // 消费"结构体"/"联合体"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期" + std::string(isUnion ? "联合体" : "结构体") + "名");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    consume(TokenType::LeftBrace, "'{'");
    // 字段列表：类型 字段名（换行/分号分隔，直到 }
    // 1216：字段表预分配（常见 2-6 字段）——替代几何增长多次分配
    decl->fields.reserve(4);
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        // 跳过字段分隔（换行已被lexer跳过；分号/逗号为显式分隔）
        while (check(TokenType::Semicolon) || check(TokenType::Comma)) advance();
        if (check(TokenType::RightBrace)) break;
        StructField field;
        field.name.clear();
        field.location = current().getLocation();
        // 164-a（A4 方案D）：联合体成员「手动释放」前缀标注（上下文词——仅在
        //   联合体成员起始位 + 后随类型起点时识别；其余语境为普通标识符，
        //   不占命名空间——与 A5 十词上下文化同纪律）
        if (isUnion && checkText("手动释放") &&
            (isTypeKeyword(peek(1).getType()) || peek(1).getType() == TokenType::Identifier)) {
            field.manualRelease = true;
            advance();
        }
        // 字段类型：类型关键字/自定义类型名（含指针/数组后缀）
        bool fnptrField = false;
        if (isTypeKeyword(currentType()) || check(TokenType::Identifier)) {
            // 040 甲案（082①②·001 §5.8）：函数指针字段位 `整32(*回调)(整32);`——
            //   规范化文本（标量）或 函数指针<...>[N]（数组元素字段）入 field.type，
            //   走既有布局/拷贝通道（指针槽 8/8·非资源·001 §5.8 布局与拷贝语义条文）
            //   注意：fnptr 形态的名字已在 `(*名)` 内消费——须跳过下方名字消费块
            FuncPtrTypeInfo fieldFp;
            if (funcPtrDeclAhead(true, true, false) && parseFuncPtrType(fieldFp) &&
                !fieldFp.name.empty()) {
                field.name = fieldFp.name;
                field.type = fieldFp.toSymbolType();
                fnptrField = true;
            } else {
                field.type = parseTypeNameEx();
            }
        } else {
            reportErrorHere("结构体字段预期类型");
            synchronize();
            break;
        }
        if (!fnptrField) {
            if (check(TokenType::Identifier)) {
                field.name = current().getValue();
                advance();
            } else {
                reportErrorHere("结构体字段预期名称");
                synchronize();
                break;
            }
        }
        // 775（762 呈报同族·乙案双侧对齐）：结构体/联合体字段初始化器显式
        //   拒绝——001 §5.7 字段=裸声明。原缺陷=无 `=` 处理，`类型 名 = 值;`
        //   落下一轮循环在 `=` 位误报「结构体字段预期类型」（拒绝面正确但
        //   诊断失真且 break 中断后续字段报错面）；v2 侧同形态零诊断静默
        //   跳过=双向分叉，本轮双侧统一为单条精确诊断（文案共用
        //   kFieldInitRejectDiag·与类实例字段面同串）。报错后照常消费初始
        //   化器（错误恢复同步·193 抽象带体同款）——Error 诊断使编译中止。
        if (check(TokenType::Equal)) {
            reportErrorHere(kFieldInitRejectDiag);
            advance();
            if (check(TokenType::LeftBrace)) {
                parseInitList();
            } else {
                parseExpr();
            }
        }
        decl->fields.push_back(std::move(field));
        match(TokenType::Semicolon);  // 字段分隔（声明体成员分隔：可选，plans/015 语义区分）
    }
    consume(TokenType::RightBrace, "'}'");
    return decl;
}

// 解析枚举声明：枚举 名 { 成员, 成员 = 值, ... }（Task 2.7）
// 成员值：整数字面量（可为负数）/省略（自动递增，首个默认0）
namespace {

// 728u：枚举成员值文本 → 整64 承载值（两补码回绕口径，与 v2 侧
//   数字文本转数值 + 位模式取负474 逐形态一致）。
//   背景（583 扫描实证·519 同族）：parseIntValue 的 std::stoll 对
//   |数学值| > 2^63-1 的文本抛异常 catch 回 0——负号形态
//   `-parseIntValue("9223372036854775808")` 静默得 0（宿主对照输出 0
//   错值）；v2 同输入经不安全回绕得 MIN 位模式 + 安全区 0-MIN 溢出
//   （错误码 8 自举中止）。本函数以两补码回绕统一：文本数学值 mod 2^64，
//   负号形态再取负回绕（-2^63 补码不动点 = MIN）。
//   边界对齐 v2 数字文本转数值 的累加上界（第 20 个数字处返回 0）：
//   纯十进制数字 ≥20 位 → 0（不崩、双侧同值）；含进制前缀（0x/0b/0o）
//   形态回退 parseIntValue 既有通道（本任务零变化面）。
std::int64_t parseEnumMemberValue(const std::string& text, bool negate) {
    // 纯十进制判定：无进制前缀，剥 L/U/F 后缀后全数字
    bool decimal = true;
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' ||
         text[1] == 'B' || text[1] == 'o' || text[1] == 'O')) {
        decimal = false;
    }
    if (decimal) {
        std::string digits = text;
        while (!digits.empty()) {
            const char c = digits.back();
            if (c == 'L' || c == 'l' || c == 'U' || c == 'u' || c == 'F' ||
                c == 'f') {
                digits.pop_back();
            } else {
                break;
            }
        }
        bool allDigits = !digits.empty();
        for (const char c : digits) {
            if (c < '0' || c > '9') { allDigits = false; break; }
        }
        if (allDigits) {
            // v2 数字文本转数值 同界：20 位数字起返回 0（防中间值溢出）
            if (digits.size() >= 20) return 0;
            const unsigned long long uv = std::stoull(digits, nullptr, 10);
            return negate ? static_cast<std::int64_t>(0ULL - uv)
                          : static_cast<std::int64_t>(uv);
        }
    }
    // 非十进制形态（0x/0b/0o 等）：parseIntValue 既有通道 + 回绕取负
    const std::int64_t v = parseIntValue(text);
    return negate
               ? static_cast<std::int64_t>(
                     0ULL - static_cast<unsigned long long>(v))
               : v;
}

}  // namespace

std::unique_ptr<EnumDecl> Parser::parseEnumDecl() {
    auto decl = std::make_unique<EnumDecl>();
    decl->location = current().getLocation();
    advance();  // 消费"枚举"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期枚举名");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    consume(TokenType::LeftBrace, "'{'");
    // 成员列表：成员 [= 值]（逗号分隔，可带尾逗号）
    // 1216：成员表预分配（枚举成员常 3-10 个）——替代几何增长多次分配
    decl->members.reserve(8);
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        if (!check(TokenType::Identifier)) {
            reportErrorHere("枚举成员预期名称");
            synchronize();
            break;
        }
        EnumMember member;
        member.name = current().getValue();
        advance();
        // 显式赋值：= 整数字面量（支持负号）
        if (match(TokenType::Equal)) {
            member.explicitValue = true;
            if (currentType() == TokenType::IntegerLiteral) {
                // 728u：改走 parseEnumMemberValue（两补码回绕——原 parseIntValue
                //   stoll 超承载静默 0 与 v2 回绕口径分叉）
                member.value = parseEnumMemberValue(current().getValue(), false);
                advance();
            } else if (check(TokenType::Minus) &&
                       peek(1).getType() == TokenType::IntegerLiteral) {
                advance();  // 消费负号
                // 728u：-9223372036854775808 得整64 最小值（原 -parseIntValue 静默 0）
                member.value = parseEnumMemberValue(current().getValue(), true);
                advance();
            } else {
                reportErrorHere("枚举成员值必须是整数字面量");
                synchronize();
                break;
            }
        }
        decl->members.push_back(std::move(member));
        // 逗号分隔（可带尾逗号）
        if (check(TokenType::Comma)) {
            advance();
        } else {
            break;
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return decl;
}

// 解析结构体/类初始化：类型名{ 字段 = 值, ... }（Task 2.7）或 类型名{ 值1, 值2 }（位置，Task 3.7）
// 调用前提：已消费类型名（Identifier），当前为 {
// 两种形态：
//   1. 字段赋值：点{ x = 1, y = 2 }（字段名 = 值，顺序任意）
//   2. 位置初始化：复数{实部, 右.实部}（规格书06-九 运算符重载示例，无字段名按声明顺序）
//      位置元素字段名为空字符串，语义层按声明顺序对齐
std::unique_ptr<Expr> Parser::parseStructInit(const std::string& typeName) {
    auto init = std::make_unique<StructInitExpr>(typeName);
    init->location = current().getLocation();
    consume(TokenType::LeftBrace, "'{'");
    // 1216：字段初始化表预分配（常见 2-4 项）——替代几何增长多次分配
    init->fields.reserve(4);
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        // 位置初始化形态：标识符 后跟 , 或 }（无 =）→ 位置元素
        if (check(TokenType::Identifier) &&
            peek(1).getType() != TokenType::Equal) {
            // 位置初始化：值1, 值2（字段名为空）
            init->fields.emplace_back("", parseExpr());
            if (check(TokenType::Comma)) {
                advance();
                continue;
            }
            break;
        }
        if (!check(TokenType::Identifier)) {
            reportErrorHere("结构体初始化预期字段名");
            synchronize();
            break;
        }
        std::string fieldName = current().getValue();
        advance();
        consume(TokenType::Equal, "'='");
        // 字段值为 { ... } 时解析为数组初始化列表（Task 完善A：结构体数组字段初始化，
        // 如 班级{ 编号 = 1, 分数 = { 80, 90, 70 } }——此前 parseExpr 遇 { 报"预期表达式"）
        if (check(TokenType::LeftBrace)) {
            init->fields.emplace_back(fieldName, parseInitList());
        } else {
            init->fields.emplace_back(fieldName, parseExpr());
        }
        if (check(TokenType::Comma)) {
            advance();
        } else {
            break;
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return init;
}

} // namespace cn_compiler
