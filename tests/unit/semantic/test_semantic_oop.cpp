// 阶段3 语义分析器 OOP 单元测试（Task 3.1~3.4, 3.7, 3.9）
// 覆盖：类符号表/继承/构造析构判定、虚函数槽位分配/重写签名校验、
//       接口实现验证、访问控制（公开/保护/私有）正反用例、运算符重载决议、
//       静态成员、常量成员函数、友元
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 SemanticAnalyzer 分析（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;

namespace {

// 辅助：解析+语义分析源码，返回是否成功与诊断引擎
struct SemanticResult {
    bool ok = false;         // 语义分析是否成功
    int errorCount = 0;      // 错误数量
    int warningCount = 0;    // 警告数量
    std::string messages;    // 全部诊断消息（拼接）
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "语义OOP测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    result.warningCount = diagnostics.getWarningCount();
    for (const auto& d : diagnostics.getAll()) {
        result.messages += d.message + "\n";
    }
    return result;
}

} // namespace

// ==================== 类符号表与继承（Task 3.1） ====================

// 类定义 + 字段 + 方法：合法
TEST(SemanticOopTest, ClassBasicDecl) {
    auto r = analyzeSource(R"CN(
类 动物 {
公开:
    字符串 名称;
    整32 年龄;
    不安全 函数 设置名称(字符串 名) -> 空类型 {
        名称 = 名;
    }
}
不安全 函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 单继承：父类成员并入子类作用域（子类可访问父类字段/方法）
TEST(SemanticOopTest, ClassInheritance) {
    auto r = analyzeSource(R"CN(
类 动物 {
公开:
    整32 年龄;
    不安全 函数 获取年龄() -> 整32 {
        返回 年龄;
    }
}
类 狗 : 动物 {
公开:
    不安全 函数 狗() {
        年龄 = 3;
    }
}
不安全 函数 主() -> 整32 {
    狗 实例 = 狗();
    整32 岁 = 实例.获取年龄();
    返回 岁;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 构造函数判定：函数名 == 类名；构造返回对象
TEST(SemanticOopTest, ConstructorDetection) {
    auto r = analyzeSource(R"CN(
类 点 {
公开:
    整32 x;
    整32 y;
    不安全 函数 点(整32 初始x, 整32 初始y) {
        x = 初始x;
        y = 初始y;
    }
    不安全 函数 获取X() -> 整32 {
        返回 x;
    }
}
不安全 函数 主() -> 整32 {
    点 p = 点(1, 2);
    返回 p.获取X();
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 父类未声明：报错
TEST(SemanticOopTest, MissingBaseClass) {
    auto r = analyzeSource(R"CN(
类 狗 : 不存在类 {
公开:
    整32 年龄;
}
不安全 函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("不存在类"), std::string::npos);
}

// ==================== 虚函数与多态（Task 3.2） ====================

// 虚函数槽位分配：父类虚函数 1 槽，子类重写覆盖同槽，新虚函数追加
TEST(SemanticOopTest, VtableSlotAssignment) {
    auto r = analyzeSource(R"CN(
类 动物 {
公开:
    虚拟 函数 叫声() -> 字符串 {
        返回 "...";
    }
}
类 狗 : 动物 {
公开:
    重写 函数 叫声() -> 字符串 {
        返回 "汪汪";
    }
}
不安全 函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 重写签名不匹配：父类返回 字符串，子类返回 整32 -> 报错
TEST(SemanticOopTest, OverrideSignatureMismatch) {
    auto r = analyzeSource(R"CN(
类 动物 {
公开:
    虚拟 函数 叫声() -> 字符串 {
        返回 "...";
    }
}
类 狗 : 动物 {
公开:
    重写 函数 叫声() -> 整32 {
        返回 1;
    }
}
不安全 函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("签名与父类不一致"), std::string::npos);
}

// ==================== 接口（Task 3.3） ====================

// 接口实现：类实现接口全部虚方法 -> 合法
TEST(SemanticOopTest, InterfaceImplementation) {
    auto r = analyzeSource(R"CN(
接口 可绘制 {
    虚拟 函数 绘制() -> 空类型
}
类 圆形 : 可绘制 {
公开:
    浮64 半径;
    重写 函数 绘制() -> 空类型 {
        半径 = 1.0;
    }
}
不安全 函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 接口实现缺失：类未实现接口方法 -> 报错
TEST(SemanticOopTest, InterfaceMissingMethod) {
    auto r = analyzeSource(R"CN(
接口 可绘制 {
    虚拟 函数 绘制() -> 空类型
    虚拟 函数 清除() -> 空类型
}
类 圆形 : 可绘制 {
公开:
    重写 函数 绘制() -> 空类型 {
    }
}
不安全 函数 主() -> 整32 {
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("未实现接口"), std::string::npos);
}

// ==================== 访问控制（Task 3.4） ====================

// 私有成员类内访问：合法
TEST(SemanticOopTest, PrivateAccessInside) {
    auto r = analyzeSource(R"CN(
类 账户 {
私有:
    整64 余额;
公开:
    不安全 函数 账户(整64 初始余额) {
        余额 = 初始余额;
    }
    不安全 函数 获取余额() -> 整64 {
        返回 余额;
    }
}
不安全 函数 主() -> 整32 {
    账户 a = 账户(100);
    整64 余额 = a.获取余额();
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 私有成员类外访问：报错
TEST(SemanticOopTest, PrivateAccessOutside) {
    auto r = analyzeSource(R"CN(
类 账户 {
私有:
    整64 余额;
公开:
    不安全 函数 账户(整64 初始余额) {
        余额 = 初始余额;
    }
}
不安全 函数 主() -> 整32 {
    账户 a = 账户(100);
    整64 余额 = a.余额;
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("私有"), std::string::npos);
}

// 保护成员子类访问：合法（子类可访问保护）
TEST(SemanticOopTest, ProtectedAccessInSubclass) {
    auto r = analyzeSource(R"CN(
类 动物 {
保护:
    整32 年龄;
公开:
    不安全 函数 动物() {
        年龄 = 0;
    }
}
类 狗 : 动物 {
公开:
    不安全 函数 设置年龄(整32 岁) -> 空类型 {
        年龄 = 岁;
    }
}
不安全 函数 主() -> 整32 {
    狗 d = 狗();
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 私有成员子类访问：报错（子类方法体内访问父类私有成员）
TEST(SemanticOopTest, PrivateAccessInSubclass) {
    auto r = analyzeSource(R"CN(
类 动物 {
私有:
    整32 内部ID;
公开:
    不安全 函数 动物() {
        内部ID = 0;
    }
}
类 狗 : 动物 {
公开:
    不安全 函数 获取ID() -> 整32 {
        返回 自身.内部ID;
    }
}
不安全 函数 主() -> 整32 {
    狗 d = 狗();
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// ==================== 运算符重载（Task 3.7） ====================

// 运算符重载：类定义 运算符+，对象相加 -> 合法
TEST(SemanticOopTest, OperatorOverload) {
    auto r = analyzeSource(R"CN(
类 复数 {
公开:
    浮64 实部;
    浮64 虚部;
    不安全 函数 复数(浮64 实, 浮64 虚) {
        实部 = 实;
        虚部 = 虚;
    }
    不安全 函数 运算符+(复数 右) -> 复数 {
        返回 复数(实部 + 右.实部, 虚部 + 右.虚部);
    }
}
不安全 函数 主() -> 整32 {
    复数 a = 复数(1.0, 2.0);
    复数 b = 复数(3.0, 4.0);
    复数 c = a + b;
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 运算符无匹配：普通数值类型不触发重载路径（内置运算仍可用）
TEST(SemanticOopTest, OperatorBuiltinStillWorks) {
    auto r = analyzeSource(R"CN(
不安全 函数 主() -> 整32 {
    整32 a = 1;
    整32 b = 2;
    整32 c = a + b;
    返回 c;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 静态成员（Task 3.9） ====================

// 静态成员：类内直接访问 + 类名.静态成员访问
TEST(SemanticOopTest, StaticMember) {
    auto r = analyzeSource(R"CN(
类 计数器 {
公开:
    静态 整32 总数 = 0;
    不安全 函数 计数器() {
        总数++;
    }
    静态 函数 获取总数() -> 整32 {
        返回 总数;
    }
}
不安全 函数 主() -> 整32 {
    计数器.总数 = 10;
    整32 当前 = 计数器.获取总数();
    返回 当前;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 成员静态性不匹配拒绝（第九十三轮 93-a） ====================
// B2 立案缺陷根治：类名.实例成员 / 实例.静态成员 / 接口名.方法 —— 编译期拒绝
//   （修复前：运行期段错误〈NULL 间接调用〉或静默错误〈读 this 首 8 字节/写不生效〉）。
//   纪律依据：规范 06-十「类外访问用 类名.静态成员」+ Rust（E0061/E0599）。

// ① 类名.实例方法() —— 拒绝
TEST(SemanticOopTest, ClassNameInstanceMethodRejected) {
    auto r = analyzeSource(R"CN(
类 盒子 {
公开:
    整64 值;
    不安全 函数 盒子(整64 v) {
        值 = v;
    }
    不安全 函数 读值() -> 整64 {
        返回 值;
    }
}
不安全 函数 主() -> 整32 {
    盒子.读值();
    返回 0;
}
)CN");
    EXPECT_GT(r.errorCount, 0) << r.messages;
    EXPECT_NE(r.messages.find("须经实例调用"), std::string::npos) << r.messages;
}

// ② 类名.实例字段（读）—— 拒绝（既有纪律，回归锚定）
TEST(SemanticOopTest, ClassNameInstanceFieldRejected) {
    auto r = analyzeSource(R"CN(
类 盒子 {
公开:
    整64 值;
    不安全 函数 盒子(整64 v) {
        值 = v;
    }
}
不安全 函数 主() -> 整32 {
    整64 x = 盒子.值;
    返回 0;
}
)CN");
    EXPECT_GT(r.errorCount, 0) << r.messages;
    EXPECT_NE(r.messages.find("要求成员为静态"), std::string::npos) << r.messages;
}

// ③ 实例.静态方法() —— 拒绝（修复前段错误）
TEST(SemanticOopTest, InstanceStaticMethodRejected) {
    auto r = analyzeSource(R"CN(
类 盒子 {
公开:
    整64 值;
    静态 整64 计数 = 0;
    不安全 函数 盒子(整64 v) {
        值 = v;
    }
    静态 函数 取计数() -> 整64 {
        返回 计数;
    }
}
不安全 函数 主() -> 整32 {
    盒子 甲 = 盒子(7);
    整64 计数读 = 甲.取计数();
    返回 0;
}
)CN");
    EXPECT_GT(r.errorCount, 0) << r.messages;
    EXPECT_NE(r.messages.find("须经类名调用"), std::string::npos) << r.messages;
}

// ④ 实例.静态字段 —— 拒绝（修复前静默读 0 / 写不生效）
TEST(SemanticOopTest, InstanceStaticFieldRejected) {
    auto r = analyzeSource(R"CN(
类 盒子 {
公开:
    整64 值;
    静态 整64 计数 = 0;
    不安全 函数 盒子(整64 v) {
        值 = v;
    }
}
不安全 函数 主() -> 整32 {
    盒子 甲 = 盒子(7);
    整64 计数直读 = 甲.计数;
    返回 0;
}
)CN");
    EXPECT_GT(r.errorCount, 0) << r.messages;
    EXPECT_NE(r.messages.find("不能经实例访问"), std::string::npos) << r.messages;
}

// ⑤ 接口名.方法() —— 拒绝（修复前段错误）
TEST(SemanticOopTest, InterfaceNameMethodRejected) {
    auto r = analyzeSource(R"CN(
接口 形状 {
    不安全 函数 面积() -> 整64;
}
类 方形 : 形状 {
公开:
    整64 边;
    不安全 函数 方形(整64 s) {
        边 = s;
    }
    重写 函数 面积() -> 整64 {
        返回 边 * 边;
    }
}
不安全 函数 主() -> 整32 {
    形状.面积();
    返回 0;
}
)CN");
    EXPECT_GT(r.errorCount, 0) << r.messages;
    EXPECT_NE(r.messages.find("须经接口对象调用"), std::string::npos) << r.messages;
}

// ⑥ 子类名.父类实例方法() —— 拒绝（同族形态：成员查找沿继承链命中）
TEST(SemanticOopTest, SubClassNameBaseInstanceMethodRejected) {
    auto r = analyzeSource(R"CN(
类 基 {
公开:
    整64 值;
    不安全 函数 基(整64 v) {
        值 = v;
    }
    不安全 函数 读基() -> 整64 {
        返回 值;
    }
}
类 子 : 基 {
公开:
    不安全 函数 子(整64 v) {
        值 = v;
    }
}
不安全 函数 主() -> 整32 {
    子.读基();
    返回 0;
}
)CN");
    EXPECT_GT(r.errorCount, 0) << r.messages;
    EXPECT_NE(r.messages.find("须经实例调用"), std::string::npos) << r.messages;
}

// 合法形态回归锚定：类名.静态成员（方法/字段）+ 实例.实例成员 不受影响
TEST(SemanticOopTest, StaticAccessLegalFormsUnaffected) {
    auto r = analyzeSource(R"CN(
类 盒子 {
公开:
    整64 值;
    静态 整64 计数 = 0;
    不安全 函数 盒子(整64 v) {
        值 = v;
    }
    不安全 函数 读值() -> 整64 {
        返回 值;
    }
    静态 函数 取计数() -> 整64 {
        返回 计数;
    }
}
不安全 函数 主() -> 整32 {
    盒子 甲 = 盒子(7);
    盒子.计数 = 3;
    整64 计数读 = 盒子.取计数();
    整64 甲读值 = 甲.读值();
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 常量成员函数（Task 3.9） ====================

// 常量成员函数：只读成员 -> 合法
TEST(SemanticOopTest, ConstMethodReadOnly) {
    auto r = analyzeSource(R"CN(
类 只读包装 {
私有:
    整32 值;
公开:
    不安全 函数 只读包装(整32 v) {
        值 = v;
    }
    常量 函数 获取() -> 整32 {
        返回 值;
    }
}
不安全 函数 主() -> 整32 {
    只读包装 w = 只读包装(42);
    返回 w.获取();
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 常量成员函数内修改成员：报错
TEST(SemanticOopTest, ConstMethodModifyField) {
    auto r = analyzeSource(R"CN(
类 只读包装 {
私有:
    整32 值;
公开:
    不安全 函数 只读包装(整32 v) {
        值 = v;
    }
    常量 函数 修改() -> 空类型 {
        值 = 10;
    }
}
不安全 函数 主() -> 整32 {
    只读包装 w = 只读包装(42);
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("常量成员函数内不能修改成员"), std::string::npos);
}

// ==================== 友元（Task 3.9） ====================

// 友元函数：可访问私有成员 -> 合法
TEST(SemanticOopTest, FriendFunctionAccess) {
    auto r = analyzeSource(R"CN(
类 账户 {
私有:
    整64 余额;
公开:
    友元 函数 审计余额(账户 账);
    不安全 函数 账户(整64 初始) {
        余额 = 初始;
    }
}
不安全 函数 审计余额(账户 账) -> 整64 {
    返回 账.余额;
}
不安全 函数 主() -> 整32 {
    账户 a = 账户(100);
    整64 余额 = 审计余额(a);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 自身（this）访问（Task 3.1，规格书06-七）
// 自身.字段 读写在类方法体内合法
TEST(SemanticOopTest, SelfMemberAccess) {
    auto r = analyzeSource(R"CN(
类 动物 {
公开:
    整32 年龄;
    不安全 函数 动物() {
        自身.年龄 = 3;
    }
    不安全 函数 获取年龄() -> 整32 {
        返回 自身.年龄;
    }
}
不安全 函数 主() -> 整32 {
    动物 a = 动物();
    返回 a.获取年龄();
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 父类限定调用：父类.方法()（Task 3.2，规格书06-七）
TEST(SemanticOopTest, SuperMethodCall) {
    auto r = analyzeSource(R"CN(
类 动物 {
公开:
    不安全 函数 叫声() -> 字符串 {
        返回 "...";
    }
}
类 狗 : 动物 {
公开:
    不安全 函数 介绍() -> 字符串 {
        返回 父类.叫声();
    }
}
不安全 函数 主() -> 整32 {
    狗 d = 狗();
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
