// AST打印器实现：递归打印AST树（Task 1.10 ast 命令调试输出）
// 实现要点：
//   1. 每行输出：缩进 + 节点类型 + 源码位置 + 关键字段（值/名/运算符）
//   2. 递归访问子节点前 depth_ 递增，输出后递减（树形缩进）
//   3. 运算符映射输出中文符号文本，便于阅读
//   4. #1555 验证面补打：凡「序列化写侧有、打印无」的解析期字段一律注记补打
//      （前缀注记风格同 [常量]/[静态] 先例），使 CN_AST_CACHE_VERIFY 打印比对
//      通道能抓缓存命中后的字段漂移；语义回填字段（缓存写入点=解析后语义前，
//      两侧同为默认值）不打——豁免边界与完整清单登记于 ast_serialize_impl.hpp
//      文件头（#1555）；可选注记统一「非默认值才打」，默认值两侧同构无漂移面。
#include "cn_compiler/model/ast_printer.hpp"

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
        case Operator::AddressOf: return "&";
        case Operator::Deref: return "*";
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
        case Operator::Propagate: return "?";
        case Operator::Dot: return ".";  // v2.1：成员访问统一 .（Arrow 已废除）
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
    // D25（500-a）：顶层常量/静态变量声明打印——原 visitProgram 漏遍历
    //   globals（ast_printer 覆盖率 14.6% 的构成性缺口·471-a 缺口实证：
    //   源码含「静态 整32 全局计数 = 7;」而输出声明数不含该声明）。
    for (const auto& g : node->globals) {
        g->accept(*this);  // 走 visitVarDecl（含 [静态]/[常量] 标注+初始化器遍历）
    }
    for (const auto& s : node->structs) {
        s->accept(*this);
    }
    for (const auto& e : node->enums) {
        e->accept(*this);
    }
    // 阶段3：导入/类/接口/泛型 声明（Task 3.1/3.3/3.6/3.8）
    for (const auto& imp : node->imports) {
        imp->accept(*this);
    }
    for (const auto& cls : node->classes) {
        cls->accept(*this);
    }
    for (const auto& itf : node->interfaces) {
        itf->accept(*this);
    }
    for (const auto& gen : node->generics) {
        gen->accept(*this);
    }
    for (const auto& decl : node->declarations) {
        decl->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitStructDecl(StructDecl* node) {
    std::string detail = node->name + " 字段数=" + std::to_string(node->fields.size());
    // #1555：explicitPortable（217 便携豁免声明位·解析期）+ 公开可见性注记补打
    if (node->explicitPortable) detail += " [可搬运]";
    if (node->access == AccessSpecifier::Public) detail += " [公开]";
    printHeader(node->isUnion ? "联合体声明" : "结构体声明", node->location, detail);
    ++depth_;
    for (const auto& field : node->fields) {
        // #1555：manualRelease（164-a 联合体手动释放标注）+ 字段自身声明位置
        //   （field.location·164-a 诊断精确位置，原误用声明节点位置）补打
        std::string fieldDetail = field.type + " " + field.name;
        if (field.manualRelease) fieldDetail += " [手动释放]";
        printHeader("字段", field.location, fieldDetail);
    }
    --depth_;
}

void AstPrinter::visitEnumDecl(EnumDecl* node) {
    std::string detail = node->name + " 成员数=" + std::to_string(node->members.size());
    if (node->access == AccessSpecifier::Public) detail += " [公开]";  // #1555
    printHeader("枚举声明", node->location, detail);
    ++depth_;
    for (const auto& member : node->members) {
        // #1555：explicitValue（解析期显式赋值标记）补打——自动递增与显式赋值
        //   在缓存命中丢失时会静默互混，VERIFY 通道须可见
        std::string memberDetail = member.name + " = " + std::to_string(member.value);
        if (member.explicitValue) memberDetail += " [显式值]";
        printHeader("成员", node->location, memberDetail);
    }
    --depth_;
}

void AstPrinter::visitFunctionDecl(FunctionDecl* node) {
    std::string detail = "函数 " + node->name;
    if (!node->returnType.empty()) detail += " -> " + node->returnType;
    // #1555：解析期声明位注记补打（外部/不安全/公开可见性）；sigKey 实测为语义层
    //   写回字段（签名注册时填·解析后两侧同空），非空才打=VERIFY 无噪音、
    //   语义阶段后调试可见
    if (node->isExtern) detail += " [外部]";
    if (node->isUnsafe) detail += " [不安全]";
    if (node->access == AccessSpecifier::Public) detail += " [公开]";
    if (!node->sigKey.empty()) detail += " 签名=" + node->sigKey;
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
    std::string detail = node->typeName + " " + node->name;
    // #1555：注记补打——hasDefault/isConstParam 解析期实弹面；ownedConsumed 为
    //   语义层消耗置位（102 甲案）、funcPtr 非空表示函数指针参数（解析期），
    //   后两者非默认才打（解析后同默认=VERIFY 无噪音）
    if (node->hasDefault) detail += " [有默认]";
    if (node->isConstParam) detail += " [常量引用]";
    if (node->ownedConsumed) detail += " [消耗]";
    if (node->funcPtr.isFunctionPtr()) detail += " [函数指针 " + node->funcPtr.toString() + "]";
    printHeader("参数", node->location, detail);
    // #1555：defaultExpr 子树遍历（解析期默认值表达式·缓存命中丢失即显形）
    if (node->defaultExpr != nullptr) {
        ++depth_;
        node->defaultExpr->accept(*this);
        --depth_;
    }
}

void AstPrinter::visitVarDecl(VarDecl* node) {
    std::string detail = node->name;
    if (!node->typeName.empty()) detail += " : " + node->typeName;
    if (node->isConst) detail += " [常量]";
    if (node->isStatic) detail += " [静态]";
    // #1555：注记补打——access/funcPtr 解析期实弹面；constInitText 为语义层
    //   回填（569 顶层常量求值文本·解析后两侧同空），非空才打
    if (node->access == AccessSpecifier::Public) detail += " [公开]";
    if (node->funcPtr.isFunctionPtr()) detail += " [函数指针 " + node->funcPtr.toString() + "]";
    if (!node->constInitText.empty()) detail += " 常量初始=" + node->constInitText;
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

void AstPrinter::visitRangeForStmt(RangeForStmt* node) {
    printHeader("遍历...中每个 迭代语句", node->location);
    ++depth_;
    printHeader("变量: " + node->varName, node->location);
    if (node->iterable != nullptr) {
        node->iterable->accept(*this);
    }
    if (node->body != nullptr) {
        node->body->accept(*this);
    }
    // #1555：desugared 子树遍历（语义层降级树·解析后恒空非空才打=VERIFY 无噪音，
    //   语义阶段后调试可见）
    if (node->desugared != nullptr) {
        printHeader("降级树", node->desugared->location);
        ++depth_;
        node->desugared->accept(*this);
        --depth_;
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
    // #1554/#1555：value/decValue/isString/strValue/isEnumMember 补打——
    //   decValue（1137 规范十进制文本·超 i64 标签权威值）解析期非空才打；
    //   strValue 为语义层解码回填（仅字符串情况打·解析后同空=VERIFY 无噪音）
    std::string detail = node->rawValue + " 值=" + std::to_string(node->value);
    if (!node->decValue.empty()) detail += " 十进制=" + node->decValue;
    if (node->isString) detail += " [字符串 串值=" + node->strValue + "]";
    if (node->isEnumMember) detail += " [枚举成员]";
    printHeader("情况标签", node->location, detail);
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

void AstPrinter::visitTernaryExpr(TernaryExpr* node) {
    printHeader("三元表达式", node->location);
    ++depth_;
    if (node->condition != nullptr) {
        node->condition->accept(*this);
    }
    if (node->trueValue != nullptr) {
        node->trueValue->accept(*this);
    }
    if (node->falseValue != nullptr) {
        node->falseValue->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitCastExpr(CastExpr* node) {
    printHeader("强制转换", node->location, node->targetType);
    ++depth_;
    if (node->operand != nullptr) {
        node->operand->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitSizeofExpr(SizeofExpr* node) {
    // #1555：isVarForm（解析期变量形态）/size（语义层布局回填·解析后 0 两侧同值
    //   恒打=无 VERIFY 噪音，语义阶段后调试可见）补打
    std::string detail = node->typeName + " 大小=" + std::to_string(node->size);
    if (node->isVarForm) detail += " [变量形态]";
    printHeader("类型大小", node->location, detail);
}

void AstPrinter::visitLambdaExpr(LambdaExpr* node) {
    std::string capture;
    switch (node->captureKind) {
        case LambdaCaptureKind::None: capture = "[]"; break;
        case LambdaCaptureKind::ByValue: capture = "[=]"; break;
        case LambdaCaptureKind::ByRef: capture = "[&]"; break;
        case LambdaCaptureKind::Explicit: {
            capture = "[";
            for (std::size_t i = 0; i < node->explicitCaptures.size(); ++i) {
                if (i > 0) capture += ", ";
                capture += node->explicitCaptures[i];
            }
            capture += "]";
            break;
        }
    }
    std::string detail = capture + " 返回=" +
        (node->returnType.empty() ? "推导" : node->returnType);
    printHeader("lambda表达式", node->location, detail);
    ++depth_;
    for (auto& param : node->params) {
        param->accept(*this);
    }
    if (node->body != nullptr) {
        node->body->accept(*this);
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
    std::string detail = "." + node->memberName;
    // #1555：注记补打——isDerefAccess/viaColonColon 解析期实弹面；isMethodValue
    //   实测语义层置位（方法值化·解析后同默认），true 才打=VERIFY 无噪音
    if (node->isDerefAccess) detail += " [解引用]";
    if (node->viaColonColon) detail += " [双冒号]";
    if (node->isMethodValue) detail += " [方法值]";
    printHeader("成员访问", node->location, detail);
    ++depth_;
    if (node->object != nullptr) {
        node->object->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitType(Type* node) {
    printHeader("类型", node->location, node->name);
}

// ==================== 阶段3 OOP/错误处理/模块/泛型 节点打印 ====================

// #1555：导入声明注记补打（模块声明/导入声明两分支共用）——importPath/access
//   解析期实弹面（公开 导入 前缀置位）；fromImport 恒 false 兼容字段；
//   ownerModule 为 mergeModules 合并期写入（缓存写入点同空=VERIFY 无噪音）
static void appendImportNotes(std::string& detail, ImportDecl* node) {
    if (node->access == AccessSpecifier::Public) detail += " [公开]";
    if (!node->importPath.empty()) detail += " 路径=" + node->importPath;
    if (node->fromImport) detail += " [从导入]";
    if (!node->ownerModule.empty()) detail += " 归属=" + node->ownerModule;
}

void AstPrinter::visitImportDecl(ImportDecl* node) {
    std::string detail;
    if (node->isModuleDecl) {
        // 模块声明：模块 标识符（v2.0）
        detail = "模块 " + node->importPath;
        appendImportNotes(detail, node);
        printHeader("模块声明", node->location, detail);
        return;
    }
    // 导入声明（v2.0 全形式）
    detail = "导入 ";
    for (std::size_t i = 0; i < node->segments.size(); ++i) {
        if (i > 0) detail += "::";
        detail += node->segments[i];
    }
    if (!node->alias.empty()) {
        detail += " 作为 " + node->alias;
    } else if (node->wildcard) {
        detail += "::*";
    } else if (!node->names.empty()) {
        detail += "::{";
        for (std::size_t i = 0; i < node->names.size(); ++i) {
            if (i > 0) detail += ", ";
            detail += node->names[i].name;
            if (!node->names[i].alias.empty()) {
                detail += " 作为 " + node->names[i].alias;
            }
        }
        detail += "}";
    }
    appendImportNotes(detail, node);
    printHeader("导入声明", node->location, detail);
}

void AstPrinter::visitInterfaceDecl(InterfaceDecl* node) {
    std::string detail = node->name + " 方法数=" + std::to_string(node->members.size());
    if (node->access == AccessSpecifier::Public) detail += " [公开]";  // #1555
    printHeader("接口声明", node->location, detail);
    ++depth_;
    for (const auto& member : node->members) {
        member->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitClassDecl(ClassDecl* node) {
    std::string detail = node->name;
    if (!node->baseName.empty()) detail += " : " + node->baseName;
    for (const auto& itf : node->interfaces) {
        detail += ", " + itf;
    }
    detail += " 成员数=" + std::to_string(node->members.size());
    // #1555：explicitPortable（217 便携豁免·与 StructDecl 同构）+ 公开可见性补打
    if (node->explicitPortable) detail += " [可搬运]";
    if (node->access == AccessSpecifier::Public) detail += " [公开]";
    printHeader("类声明", node->location, detail);
    ++depth_;
    for (const auto& member : node->members) {
        member->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitClassMember(ClassMember* node) {
    // 访问标签文本
    const char* accessText = "公开";
    if (node->access == AccessSpecifier::Protected) accessText = "保护";
    if (node->access == AccessSpecifier::Private) accessText = "私有";
    std::string detail = std::string(accessText) + ":";
    // 按成员种类输出细节
    switch (node->kind) {
        case ClassMemberKind::Field:
            detail += " 字段 " + node->typeName + " " + node->name;
            if (node->isStatic) detail += " [静态]";
            printHeader("类成员", node->location, detail);
            if (node->initializer != nullptr) {
                ++depth_;
                node->initializer->accept(*this);
                --depth_;
            }
            return;
        case ClassMemberKind::Constructor:
            detail += " 构造 " + node->name;
            break;
        case ClassMemberKind::Destructor:
            detail += " 析构 ~" + node->name;
            break;
        case ClassMemberKind::Operator:
            detail += " 运算符重载 " + node->operatorSym;
            break;
        case ClassMemberKind::Friend:
            detail += std::string(" 友元 ") + (node->isFriendClass ? "类 " : "函数 ") + node->name;
            printHeader("类成员", node->location, detail);
            return;
        case ClassMemberKind::Method:
        default:
            detail += " 方法 " + node->name;
            break;
    }
    // 方法修饰符
    if (node->isVirtual) detail += " [虚拟]";
    if (node->isOverride) detail += " [重写]";
    if (node->isAbstract) detail += " [抽象]";
    if (node->isConstMethod) detail += " [常量]";
    if (node->isStatic) detail += " [静态]";
    if (node->isUnsafe) detail += " [不安全]";  // #1555：解析期修饰（019 安全区边界）
    // #1555：ctorInitBase/ctorInitArgs（P3-20 父类构造初始化列表·解析期实弹面）
    if (!node->ctorInitBase.empty()) detail += " [基类初始化=" + node->ctorInitBase + "]";
    if (!node->returnType.empty()) detail += " -> " + node->returnType;
    printHeader("类成员", node->location, detail);
    ++depth_;
    for (const auto& param : node->params) {
        param->accept(*this);
    }
    // #1555：父类构造实参子树（解析期·缓存命中丢失即显形）
    if (!node->ctorInitArgs.empty()) {
        printHeader("基类构造实参", node->location,
                    "个数=" + std::to_string(node->ctorInitArgs.size()));
        ++depth_;
        for (const auto& arg : node->ctorInitArgs) {
            arg->accept(*this);
        }
        --depth_;
    }
    if (node->body != nullptr) {
        node->body->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitGenericDecl(GenericDecl* node) {
    std::string detail = "<";
    for (std::size_t i = 0; i < node->typeParams.size(); ++i) {
        if (i > 0) detail += ", ";
        detail += "类型 " + node->typeParams[i];
        if (i < node->constraints.size() && !node->constraints[i].empty()) {
            detail += " : " + node->constraints[i];
        }
    }
    detail += ">";
    printHeader("泛型声明", node->location, detail);
    ++depth_;
    if (node->innerClass != nullptr) {
        node->innerClass->accept(*this);
    }
    if (node->innerFunc != nullptr) {
        node->innerFunc->accept(*this);
    }
    --depth_;
}

void AstPrinter::visitSelfExpr(SelfExpr* node) {
    printHeader("自身", node->location);
}

void AstPrinter::visitSuperExpr(SuperExpr* node) {
    printHeader("父类", node->location);
}

} // namespace cn_compiler
