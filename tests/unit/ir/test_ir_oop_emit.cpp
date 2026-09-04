// 阶段3 IR 层 OOP 指令发射单元测试（Task 3.1/3.2，串联集成子任务）
// 覆盖：
//   1. 类方法体提升为独立 IRFunction（this 指针为第一个参数，静态方法无 this）
//   2. 构造调用 类名(实参) -> NewObject（extra="类名|大小字节"）+ 构造体 Call
//   3. 虚函数调用 -> VirtualCall（extra="类名.虚方法名"，operand[0]=this）
//   4. 非虚/静态/父类 方法调用 -> 直接 Call（符号 类名$sigKey）
//   5. 自身/父类 表达式 -> Load this 指针
//   6. 实例字段访问（自身.字段/对象.字段）-> FieldAddr + LoadPtr/StorePtr
//   7. 静态字段访问（类名.字段）-> 静态字段符号地址
//   8. 类类型局部变量（有析构函数）函数收尾 -> DeleteObject（RAII）
// 测试方式：全链路 Lexer + Parser + SemanticAnalyzer + IRGenerator（真实流水线）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::Opcode;

namespace {

// 辅助：全链路生成 IR 模块（语义分析成功才继续）
struct IrResult {
    bool ok = false;
    IRModule module;
    std::string messages;
};

IrResult generateIr(const std::string& source) {
    IrResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "OOP IR发射测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer semantic(diagnostics);
    if (!semantic.analyze(program.get())) {
        result.messages = diagnostics.format();
        return result;
    }
    IRGenerator irGen(diagnostics, &semantic);
    result.module = irGen.generate(program.get());
    result.ok = !diagnostics.hasErrors();
    result.messages = diagnostics.format();
    return result;
}

// 辅助：查找函数（按 func.name 前缀匹配）
const cn_compiler::ir::IRFunction* findFunction(
    const IRModule& module, const std::string& namePrefix) {
    for (const auto& fn : module.functions) {
        if (fn.name == namePrefix || fn.name.rfind(namePrefix, 0) == 0) {
            return &fn;
        }
    }
    return nullptr;
}

// 辅助：在函数中查找第一条指定操作码的指令
const cn_compiler::ir::IRInstruction* findInst(
    const cn_compiler::ir::IRFunction& fn, Opcode opcode) {
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == opcode) return &inst;
        }
    }
    return nullptr;
}

// 辅助：统计函数中指定操作码的指令数量
int countInst(const cn_compiler::ir::IRFunction& fn, Opcode opcode) {
    int count = 0;
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == opcode) ++count;
        }
    }
    return count;
}

} // namespace

// ==================== 类方法体提升（this 第一参数） ====================

// 类方法提升：有体方法生成独立 IRFunction；this 指针为第一个参数（自身 : ptr）
TEST(IrOopEmitTest, ClassMethodPromotedWithThis) {
    auto r = generateIr(R"CN(
类 动物 {
公开:
    整32 年龄;
    函数 设置年龄(整32 值) -> 空类型 {
        年龄 = 值;
    }
    函数 获取年龄() -> 整32 {
        返回 年龄;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    // 方法体提升为独立 IRFunction（类名.方法名）
    const auto* setter = findFunction(r.module, "动物.设置年龄");
    ASSERT_NE(setter, nullptr);
    // this 指针为第一个参数（参数名 自身，IR 类型 ptr）
    ASSERT_FALSE(setter->params.empty());
    EXPECT_EQ(setter->params[0].first, "自身");
    EXPECT_EQ(setter->params[0].second, "ptr");
    EXPECT_EQ(setter->params[1].first, "值");
    EXPECT_EQ(setter->params[1].second, "i32");
    // 方法链接符号：类名$sigKey（codegen 生成符号的依据）
    EXPECT_EQ(setter->mangledName, "动物$设置年龄#整32");
    // 方法体内字段写入：Load 自身 + FieldAddr + StorePtr（实例字段 StorePtr）
    EXPECT_GT(countInst(*setter, Opcode::FieldAddr), 0);
    EXPECT_GT(countInst(*setter, Opcode::StorePtr), 0);
    const auto* getter = findFunction(r.module, "动物.获取年龄");
    ASSERT_NE(getter, nullptr);
    EXPECT_EQ(getter->mangledName, "动物$获取年龄");
    EXPECT_GT(countInst(*getter, Opcode::LoadPtr), 0);
}

// 静态方法提升：无 this 参数
TEST(IrOopEmitTest, StaticMethodNoThis) {
    auto r = generateIr(R"CN(
类 计数器 {
公开:
    静态 整32 总数 = 0;
    静态 函数 获取总数() -> 整32 {
        返回 总数;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "计数器.获取总数");
    ASSERT_NE(fn, nullptr);
    // 静态方法无 this：params 为空
    EXPECT_TRUE(fn->params.empty());
    // 静态字段读取：静态字段符号地址（ConstString 携带 ?static_ 符号）
    bool hasStaticSym = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::ConstString &&
                inst.extra.rfind("?static_", 0) == 0) {
                hasStaticSym = true;
            }
        }
    }
    EXPECT_TRUE(hasStaticSym);
}

// ==================== 构造调用 NewObject ====================

// 构造调用 类名(实参)：NewObject（extra="类名|大小字节"）+ 构造体 Call（this 前置）
TEST(IrOopEmitTest, ConstructorNewObject) {
    auto r = generateIr(R"CN(
类 点 {
公开:
    整32 x;
    整32 y;
    函数 点(整32 初始x, 整32 初始y) {
        x = 初始x;
        y = 初始y;
    }
    函数 获取X() -> 整32 {
        返回 x;
    }
}
函数 主() -> 整32 {
    点 p = 点(1, 2);
    返回 p.获取X();
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    // NewObject 指令：extra = "点|大小字节"
    const auto* newObj = findInst(*main, Opcode::NewObject);
    ASSERT_NE(newObj, nullptr);
    EXPECT_NE(newObj->extra.find('|'), std::string::npos);
    EXPECT_EQ(newObj->extra.substr(0, newObj->extra.find('|')), "点");
    // 构造体 Call：符号 = 点$点#整32,整32（this=NewObject 结果前置）
    bool hasCtorCall = false;
    for (const auto& block : main->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == "点$点#整32,整32") {
                hasCtorCall = true;
                // 实参：operand[0] = 对象指针（NewObject 结果），operand[1..] = 实参
                ASSERT_GE(inst.operands.size(), 3u);
            }
        }
    }
    EXPECT_TRUE(hasCtorCall);
}

// 无构造函数类：NewObject 仅分配（无构造体 Call）
TEST(IrOopEmitTest, DefaultConstructorNewObject) {
    auto r = generateIr(R"CN(
类 盒子 {
公开:
    整32 内容;
}
函数 主() -> 整32 {
    盒子 b = 盒子();
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    EXPECT_GT(countInst(*main, Opcode::NewObject), 0);
    // 无构造体调用（类名$类名 符号不应出现）
    for (const auto& block : main->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == "盒子$盒子") {
                FAIL() << "默认构造不应发射构造体 Call";
            }
        }
    }
}

// ==================== 虚调用 VirtualCall ====================

// 虚函数调用 对象.虚方法()：VirtualCall（extra="类名.虚方法名"，operand[0]=this）
TEST(IrOopEmitTest, VirtualCallDispatch) {
    auto r = generateIr(R"CN(
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
函数 主() -> 整32 {
    动物* 实例 = 无;
    字符串 声 = 实例.叫声();
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    const auto* vcall = findInst(*main, Opcode::VirtualCall);
    ASSERT_NE(vcall, nullptr);
    // extra = "类名.虚方法名"（codegen 契约）
    EXPECT_EQ(vcall->extra, "动物.叫声");
    // operand[0] = this（对象指针）
    ASSERT_FALSE(vcall->operands.empty());
    // 返回类型为字符串（ptr）
    EXPECT_EQ(vcall->type, "ptr");
    // 虚表地址引用（VtableAddr 供 codegen 取虚表符号）
    // （本测试仅验证 VirtualCall 契约；VtableAddr 由 codegen 内部展开）
}

// 方法体内 自身.虚方法() 调用：VirtualCall（this=自身 指针）
TEST(IrOopEmitTest, VirtualCallFromMethod) {
    auto r = generateIr(R"CN(
类 动物 {
公开:
    虚拟 函数 叫声() -> 字符串 {
        返回 "...";
    }
    函数 介绍() -> 字符串 {
        返回 自身.叫声();
    }
}
类 狗 : 动物 {
公开:
    重写 函数 叫声() -> 字符串 {
        返回 "汪汪";
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* intro = findFunction(r.module, "动物.介绍");
    ASSERT_NE(intro, nullptr);
    const auto* vcall = findInst(*intro, Opcode::VirtualCall);
    ASSERT_NE(vcall, nullptr);
    EXPECT_EQ(vcall->extra, "动物.叫声");
    // operand[0] = this 指针（自身 Load）
    ASSERT_FALSE(vcall->operands.empty());
}

// ==================== 父类.方法 直接调用（非虚分派） ====================

// 父类.方法()：直接 Call 父类方法符号（非虚分派）
TEST(IrOopEmitTest, SuperCallDirect) {
    auto r = generateIr(R"CN(
类 基类 {
公开:
    函数 方法() -> 整32 {
        返回 10;
    }
}
类 派生 : 基类 {
公开:
    重写 函数 方法() -> 整32 {
        返回 父类.方法() + 1;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* sub = findFunction(r.module, "派生.方法");
    ASSERT_NE(sub, nullptr);
    // 父类.方法() -> 直接 Call 父类符号（非 VirtualCall）
    bool hasSuperCall = false;
    for (const auto& block : sub->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == "基类$方法") {
                hasSuperCall = true;
            }
            if (inst.opcode == Opcode::VirtualCall) {
                FAIL() << "父类.方法() 不应发射 VirtualCall（非虚分派）";
            }
        }
    }
    EXPECT_TRUE(hasSuperCall);
}

// ==================== 自身 / 父类 表达式 ====================

// 自身 表达式：加载 this 指针（方法第一个参数 自身 的栈槽）
TEST(IrOopEmitTest, SelfExprLoadsThis) {
    auto r = generateIr(R"CN(
类 账户 {
公开:
    整32 余额;
    函数 查看() -> 整32 {
        返回 自身.余额;
    }
}
函数 主() -> 整32 {
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* fn = findFunction(r.module, "账户.查看");
    ASSERT_NE(fn, nullptr);
    // 自身.余额：Load 自身（this）+ FieldAddr + LoadPtr
    bool hasSelfLoad = false;
    for (const auto& block : fn->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Load &&
                (inst.operands.empty() ||
                 inst.extra.rfind("自身$", 0) == 0 ||
                 inst.result.type == "ptr")) {
                // Load 自身参数槽（extra=唯一名 自身$N 或 type=ptr）
                if (inst.extra.rfind("自身$", 0) == 0) hasSelfLoad = true;
            }
        }
    }
    EXPECT_TRUE(hasSelfLoad);
    EXPECT_GT(countInst(*fn, Opcode::FieldAddr), 0);
    EXPECT_GT(countInst(*fn, Opcode::LoadPtr), 0);
}

// ==================== 实例/静态字段访问 ====================

// 实例字段读写：自身.字段 / 对象.字段 -> FieldAddr + LoadPtr/StorePtr
TEST(IrOopEmitTest, InstanceFieldAccess) {
    auto r = generateIr(R"CN(
类 点 {
公开:
    整32 x;
    整32 y;
    函数 设置(整32 新x) -> 空类型 {
        自身.x = 新x;
    }
    函数 读取() -> 整32 {
        返回 自身.y;
    }
}
函数 主() -> 整32 {
    点 p = 点();
    p.x = 5;
    整32 值 = p.y;
    返回 值;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* setter = findFunction(r.module, "点.设置");
    ASSERT_NE(setter, nullptr);
    // 自身.x = 新x：Load 自身 + FieldAddr + StorePtr
    EXPECT_GT(countInst(*setter, Opcode::FieldAddr), 0);
    EXPECT_GT(countInst(*setter, Opcode::StorePtr), 0);
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    // p.x = 5：对象指针 + FieldAddr + StorePtr
    EXPECT_GT(countInst(*main, Opcode::FieldAddr), 0);
    // p.y 读取：LoadPtr
    EXPECT_GT(countInst(*main, Opcode::LoadPtr), 0);
}

// 静态字段读写：类名.字段 -> 静态字段符号地址 + LoadPtr/StorePtr
TEST(IrOopEmitTest, StaticFieldAccess) {
    auto r = generateIr(R"CN(
类 计数器 {
公开:
    静态 整32 总数 = 0;
}
函数 主() -> 整32 {
    计数器.总数 = 10;
    整32 当前 = 计数器.总数;
    返回 当前;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    // 静态字段符号：ConstString 携带 ?static_类名_字段名（codegen lea 取地址）
    int staticSymCount = 0;
    for (const auto& block : main->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::ConstString &&
                inst.extra == "?static_计数器$总数") {
                ++staticSymCount;
            }
        }
    }
    EXPECT_GE(staticSymCount, 1);
    EXPECT_GT(countInst(*main, Opcode::StorePtr), 0);
    EXPECT_GT(countInst(*main, Opcode::LoadPtr), 0);
}

// ==================== DeleteObject（类局部变量析构，RAII） ====================

// 类类型局部变量（有析构函数）：函数收尾 DeleteObject（extra=类名）
TEST(IrOopEmitTest, DeleteObjectRaii) {
    auto r = generateIr(R"CN(
类 资源 {
公开:
    函数 资源() {
    }
    函数 ~资源() {
    }
}
函数 主() -> 整32 {
    资源 r = 资源();
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    const auto* del = findInst(*main, Opcode::DeleteObject);
    ASSERT_NE(del, nullptr);
    // extra = 类名（codegen 契约：调用 类名$~类名 析构 + __cn_object_delete）
    EXPECT_EQ(del->extra, "资源");
    ASSERT_FALSE(del->operands.empty());
}

// 无析构函数的类局部变量：不发射 DeleteObject（无资源需释放）
TEST(IrOopEmitTest, NoDeleteWithoutDtor) {
    auto r = generateIr(R"CN(
类 纯数据 {
公开:
    整32 值;
}
函数 主() -> 整32 {
    纯数据 d = 纯数据();
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    EXPECT_EQ(countInst(*main, Opcode::DeleteObject), 0);
}

// ==================== 运算符重载降级 ====================

// 左操作数为类实例 + 类有 运算符+ 成员：降级为成员方法调用（Call 运算符+ 符号）
TEST(IrOopEmitTest, OperatorOverloadCall) {
    auto r = generateIr(R"CN(
类 复数 {
公开:
    浮64 实部;
    浮64 虚部;
    函数 复数(浮64 实, 浮64 虚) {
        实部 = 实;
        虚部 = 虚;
    }
    函数 运算符+(复数 右) -> 复数 {
        返回 复数(实部 + 右.实部, 虚部 + 右.虚部);
    }
}
函数 主() -> 整32 {
    复数 a = 复数(1.0, 2.0);
    复数 b = 复数(3.0, 4.0);
    复数 c = a + b;
    返回 0;
}
)CN");
    ASSERT_TRUE(r.ok) << r.messages;
    const auto* main = findFunction(r.module, "主");
    ASSERT_NE(main, nullptr);
    // a + b -> Call 复数$+#复数（this=a 指针，实参=b）
    // 说明：运算符重载成员名 = 运算符符号（+），sigKey = +#复数（与语义层一致）
    bool hasOpCall = false;
    for (const auto& block : main->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call &&
                inst.extra == "复数$+#复数") {
                hasOpCall = true;
            }
        }
    }
    EXPECT_TRUE(hasOpCall);
    // 运算符重载方法提升为 IRFunction
    const auto* opFn = findFunction(r.module, "复数.+");
    ASSERT_NE(opFn, nullptr);
    EXPECT_EQ(opFn->mangledName, "复数$+#复数");
    ASSERT_FALSE(opFn->params.empty());
    EXPECT_EQ(opFn->params[0].first, "自身");  // this 第一参数
    EXPECT_EQ(opFn->params[1].first, "右");
}
