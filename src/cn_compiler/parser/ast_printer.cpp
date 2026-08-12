// AST打印器实现：递归打印AST树（Task 1.10 ast 命令调试输出）
// 实现要点：
//   1. 每行输出：缩进 + 节点类型 + 源码位置 + 关键字段（值/名/运算符）
//   2. 递归访问子节点前 depth_ 递增，输出后递减（树形缩进）
//   3. 运算符映射输出中文符号文本，便于阅读
#include "cn_compiler/parser/ast_printer.hpp"

#include <memory>
#include <string>
#include <utility>

namespace cn_compiler {

// 运算符 -> 中文符号文本（调试输出用）
const char* AstPrinter::operatorText(Operator op) {
    switch (op) {
        case Operator::Add: return "+";
        case Operator::Subtract: return "-";
        case Operator::Multiply: return "*";
        case Operator::Divide: return "/";
        case Operator::Modulo: return "%";
        case Operator::EqualEqual: return "==";
        case Operator::BangEqual: return "!=";
        case Operator::Less: return "<";
        case Operator::Greater: return ">";
        case Operator::LessEqual: return "<=";
        case Operator::GreaterEqual: return ">=";
        case Operator::AndAnd: return "&&";
        case Operator::OrOr: return "||";
        case Operator::Bang: return "!";
        case Operator::Amp: return "&";
        case Operator::Pipe: return "|";
        case Operator::Caret: return "^";
        case Operator::Tilde: return "~";
        case Operator::LessLess: return "<<";
        case Operator::GreaterGreater: return ">>";
        case Operator::Assign: return "=";
        case Operator::PlusAssign: return "+=";
        case Operator::MinusAssign: return "-=";
        case Operator::StarAssign: return "*=";
        case Operator::SlashAssign: return "/=";
        case Operator::PercentAssign: return "%=";
        case Operator::AmpAssign: return "&=";
        case Operator::PipeAssign: return "|=";
        case Operator::CaretAssign: return "^=";
        case Operator::LessLessAssign: return "<<=";
        case Operator::GreaterGreaterAssign: return ">>=";
        case Operator::Increment: return "++";
        case Operator::Decrement: return "--";
        case Operator::Dot: return ".";
        case Operator::Arrow: return "->";
    }
    return "?";
}

// 打印带缩进与位置的节点头
void AstPrinter::printHeader(const std::string& typeName, const SourceLocation& loc,
                             const std::string& detail) {
    for (int i = 0; i < depth_; ++i) out_ << "  ";
    out_ << typeName;
    if (!detail.empty()) out_ << " " << detail;
    out_ << " @" << loc.toString() << "\n";
}

// 主入口：打印完整程序AST
void AstPrinter::print(Program* program) {
    program->accept(*this);
}

void AstPrinter::visitProgram(Program* node) {
    printHeader("程序", node->location, "声明数=" + std::to_string(node->declarations.size()));
    ++depth_;
    for (const auto& s : node->structs) {
        s->accept(*this);
    }
    for (const auto& e : node->enums) {
        e->accept(*this);
    }
    for (const auto& decl : node->declarations) {
        decl->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitStructDecl(StructDecl* node) {
    printHeader(node->isUnion ? "联合体声明" : "结构体声明", node->location,
                node->name + " 字段数=" + std::to_string(node->fields.size()));
    ++depth_;
    for (const auto& field : node->fields) {
        printHeader("字段", node->location, field.type + " " + field.name);
    }
    --depth_;
}

void AstPrinter::visitEnumDecl(EnumDecl* node) {
    printHeader("枚举声明", node->location,
                node->name + " 成员数=" + std::to_string(node->members.size()));
    ++depth_;
    for (const auto& member : node->members) {
        printHeader("成员", node->location,
                    member.name + " = " + std::to_string(member.value));
    }
    --depth_;
}

void AstPrinter::visitFunctionDecl(FunctionDecl* node) {
    std::string detail = "函数 " + node->name;
    if (!node->returnType.empty()) detail += " -> " + node->returnType;
    printHeader("函数声明", node->location, detail);
    ++depth_;
    for (const auto& param : node->params) {
        param->accept(*this);
    }
    if (node->body != nullptr) {
        node->body->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitParamDecl(ParamDecl* node) {
    printHeader("参数", node->location, node->typeName + " " + node->name);
}

void AstPrinter::visitVarDecl(VarDecl* node) {
    std::string detail = node->name;
    if (!node->typeName.empty()) detail += " : " + node->typeName;
    if (node->isConst) detail += " [常量]";
    if (node->isStatic) detail += " [静态]";
    printHeader("变量声明", node->location, detail);
    if (node->initializer != nullptr) {
        ++depth_;
        node->initializer->accept(*this);
        --depth_;
    }
}

void AstPrinter::visitBlockStmt(BlockStmt* node) {
    printHeader("代码块", node->location, "语句数=" + std::to_string(node->statements.size()));
    ++depth_;
    for (const auto& stmt : node->statements) {
        stmt->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitExprStmt(ExprStmt* node) {
    printHeader("表达式语句", node->location);
    if (node->expr != nullptr) {
        ++depth_;
        node->expr->accept(*this);
        --depth_;
    }
}

void AstPrinter::visitIfStmt(IfStmt* node) {
    printHeader("如果语句", node->location);
    ++depth_;
    if (node->condition != nullptr) {
        node->condition->accept(*this);
    }
    if (node->thenBranch != nullptr) {
        node->thenBranch->accept(*this);
    }
    if (node->elseBranch != nullptr) {
        printHeader("否则分支", node->elseBranch->location);
        ++depth_;
        node->elseBranch->accept(*this);
        --depth_;
    }
    --depth_;
}

void AstPrinter::visitWhileStmt(WhileStmt* node) {
    printHeader("当循环", node->location);
    ++depth_;
    if (node->condition != nullptr) {
        node->condition->accept(*this);
    }
    if (node->body != nullptr) {
        node->body->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitForStmt(ForStmt* node) {
    printHeader("循环语句", node->location);
    ++depth_;
    if (node->init != nullptr) {
        printHeader("初始化", node->init->location);
        ++depth_;
        node->init->accept(*this);
        --depth_;
    }
    if (node->condition != nullptr) {
        printHeader("条件", node->condition->location);
        ++depth_;
        node->condition->accept(*this);
        --depth_;
    }
    if (node->update != nullptr) {
        printHeader("更新", node->update->location);
        ++depth_;
        node->update->accept(*this);
        --depth_;
    }
    if (node->body != nullptr) {
        node->body->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitReturnStmt(ReturnStmt* node) {
    printHeader("返回语句", node->location);
    if (node->value != nullptr) {
        ++depth_;
        node->value->accept(*this);
        --depth_;
    }
}

void AstPrinter::visitBreakStmt(BreakStmt* node) {
    printHeader("中断语句", node->location);
}

void AstPrinter::visitContinueStmt(ContinueStmt* node) {
    printHeader("继续语句", node->location);
}

void AstPrinter::visitSwitchStmt(SwitchStmt* node) {
    printHeader("选择语句", node->location,
                "情况数=" + std::to_string(node->cases.size()) +
                (node->defaultCase != nullptr ? " [有默认]" : " [无默认]"));
    ++depth_;
    if (node->condition != nullptr) {
        printHeader("选择表达式", node->condition->location);
        ++depth_;
        node->condition->accept(*this);
        --depth_;
    }
    for (const auto& caseNode : node->cases) {
        caseNode->accept(*this);
    }
    if (node->defaultCase != nullptr) {
        node->defaultCase->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitCaseLabel(CaseLabel* node) {
    printHeader("情况标签", node->location, node->rawValue);
    ++depth_;
    for (const auto& stmt : node->statements) {
        stmt->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitDefaultLabel(DefaultLabel* node) {
    printHeader("默认标签", node->location);
    ++depth_;
    for (const auto& stmt : node->statements) {
        stmt->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitIntegerLiteral(IntegerLiteral* node) {
    printHeader("整数字面量", node->location, node->raw);
}

void AstPrinter::visitFloatLiteral(FloatLiteral* node) {
    printHeader("浮点字面量", node->location, node->raw);
}

void AstPrinter::visitStringLiteral(StringLiteral* node) {
    printHeader("字符串字面量", node->location, node->raw);
}

void AstPrinter::visitCharLiteral(CharLiteral* node) {
    printHeader("字符字面量", node->location, node->raw);
}

void AstPrinter::visitBoolLiteral(BoolLiteral* node) {
    printHeader("布尔字面量", node->location, node->raw);
}

void AstPrinter::visitNullLiteral(NullLiteral* node) {
    printHeader("空指针字面量", node->location, "无");
}

void AstPrinter::visitIndexExpr(IndexExpr* node) {
    printHeader("下标访问", node->location);
    ++depth_;
    if (node->object != nullptr) {
        node->object->accept(*this);
    }
    if (node->index != nullptr) {
        node->index->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitInitListExpr(InitListExpr* node) {
    printHeader("初始化列表", node->location,
                "元素数=" + std::to_string(node->elements.size()));
    ++depth_;
    for (const auto& elem : node->elements) {
        elem->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitStructInitExpr(StructInitExpr* node) {
    printHeader("结构体初始化", node->location,
                node->typeName + " 字段数=" + std::to_string(node->fields.size()));
    ++depth_;
    for (const auto& field : node->fields) {
        printHeader("字段赋值", node->location, field.first);
        ++depth_;
        if (field.second != nullptr) {
            field.second->accept(*this);
        }
        --depth_;
    }
    --depth_;
}

void AstPrinter::visitIdentifierExpr(IdentifierExpr* node) {
    printHeader("标识符", node->location, node->name);
}

void AstPrinter::visitBinaryExpr(BinaryExpr* node) {
    printHeader("二元运算", node->location, operatorText(node->op));
    ++depth_;
    if (node->left != nullptr) {
        node->left->accept(*this);
    }
    if (node->right != nullptr) {
        node->right->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitUnaryExpr(UnaryExpr* node) {
    std::string detail = std::string(operatorText(node->op));
    if (node->postfix) detail += "（后缀）";
    printHeader("一元运算", node->location, detail);
    ++depth_;
    if (node->operand != nullptr) {
        node->operand->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitAssignmentExpr(AssignmentExpr* node) {
    printHeader("赋值运算", node->location, operatorText(node->op));
    ++depth_;
    if (node->target != nullptr) {
        node->target->accept(*this);
    }
    if (node->value != nullptr) {
        node->value->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitCallExpr(CallExpr* node) {
    printHeader("函数调用", node->location,
                "实参数=" + std::to_string(node->arguments.size()));
    ++depth_;
    if (node->callee != nullptr) {
        node->callee->accept(*this);
    }
    for (const auto& arg : node->arguments) {
        arg->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitMemberExpr(MemberExpr* node) {
    printHeader("成员访问", node->location,
                (node->isArrow ? "->" : ".") + node->memberName);
    ++depth_;
    if (node->object != nullptr) {
        node->object->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitType(Type* node) {
    printHeader("类型", node->location, node->name);
}

} // namespace cn_compiler
