// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {

namespace ir {

// 操作码转字符串（调试/测试输出）
const char* opcodeToString(Opcode opcode) {
    switch (opcode) {
        case Opcode::ConstInt: return "常量整数";
        case Opcode::ConstFloat: return "常量浮点";
        case Opcode::ConstString: return "常量字符串";
        case Opcode::ConstBool: return "常量布尔";
        case Opcode::Add: return "加";
        case Opcode::Sub: return "减";
        case Opcode::Mul: return "乘";
        case Opcode::Div: return "除";
        case Opcode::Mod: return "取余";
        case Opcode::Eq: return "相等";
        case Opcode::Ne: return "不等";
        case Opcode::Lt: return "小于";
        case Opcode::Le: return "小于等于";
        case Opcode::Gt: return "大于";
        case Opcode::Ge: return "大于等于";
        case Opcode::And: return "逻辑与";
        case Opcode::Or: return "逻辑或";
        case Opcode::Not: return "逻辑非";
        case Opcode::Load: return "加载";
        case Opcode::Store: return "存储";
        case Opcode::Alloca: return "分配";
        case Opcode::Jump: return "跳转";
        case Opcode::Branch: return "条件跳转";
        case Opcode::Call: return "调用";
        case Opcode::Return: return "返回";
        case Opcode::Phi: return "汇合";
    }
    return "未知";
}

} // namespace ir

// ==================== 类型映射 ====================

// 源码类型 -> IR类型映射（规格书7.5类型表示）
std::string IRGenerator::mapType(const std::string& type) {
    if (type == "整8") return "i8";
    if (type == "整16") return "i16";
    if (type == "整32" || type == "整数") return "i32";
    if (type == "整64") return "i64";
    if (type == "整128") return "i128";
    if (type == "正8") return "u8";
    if (type == "正16") return "u16";
    if (type == "正32") return "u32";
    if (type == "正64") return "u64";
    if (type == "正128") return "u128";
    if (type == "浮32") return "f32";
    if (type == "浮64" || type == "小数") return "f64";
    if (type == "布尔") return "i1";
    if (type == "字符") return "i32";
    if (type == "字符串") return "ptr";
    if (type == "空类型") return "void";
    if (!type.empty() && type.back() == '*') return "ptr";
    if (!type.empty() && type != "未知") return "ptr";  // 自定义类型按指针处理
    return "void";
}

// 运算符 -> IR操作码映射（整型/浮点共用算术操作码，阶段一浮点变体后续区分）
bool IRGenerator::mapBinaryOp(Operator op, bool isFloat, ir::Opcode& out) {
    (void)isFloat;
    switch (op) {
        case Operator::Add: out = ir::Opcode::Add; return true;
        case Operator::Subtract: out = ir::Opcode::Sub; return true;
        case Operator::Multiply: out = ir::Opcode::Mul; return true;
        case Operator::Divide: out = ir::Opcode::Div; return true;
        case Operator::Modulo: out = ir::Opcode::Mod; return true;
        case Operator::EqualEqual: out = ir::Opcode::Eq; return true;
        case Operator::BangEqual: out = ir::Opcode::Ne; return true;
        case Operator::Less: out = ir::Opcode::Lt; return true;
        case Operator::LessEqual: out = ir::Opcode::Le; return true;
        case Operator::Greater: out = ir::Opcode::Gt; return true;
        case Operator::GreaterEqual: out = ir::Opcode::Ge; return true;
        case Operator::AndAnd: out = ir::Opcode::And; return true;
        case Operator::OrOr: out = ir::Opcode::Or; return true;
        default: return false;
    }
}

// 是否复合赋值运算符
bool IRGenerator::isCompoundAssignOp(Operator op) {
    switch (op) {
        case Operator::PlusAssign: case Operator::MinusAssign:
        case Operator::StarAssign: case Operator::SlashAssign:
        case Operator::PercentAssign:
        case Operator::AmpAssign: case Operator::PipeAssign:
        case Operator::CaretAssign: case Operator::LessLessAssign:
        case Operator::GreaterGreaterAssign:
            return true;
        default:
            return false;
    }
}

// 复合赋值的基础运算符（+= -> +）
Operator IRGenerator::baseOpOfCompound(Operator op) {
    switch (op) {
        case Operator::PlusAssign: return Operator::Add;
        case Operator::MinusAssign: return Operator::Subtract;
        case Operator::StarAssign: return Operator::Multiply;
        case Operator::SlashAssign: return Operator::Divide;
        case Operator::PercentAssign: return Operator::Modulo;
        case Operator::AmpAssign: return Operator::Amp;
        case Operator::PipeAssign: return Operator::Pipe;
        case Operator::CaretAssign: return Operator::Caret;
        case Operator::LessLessAssign: return Operator::LessLess;
        case Operator::GreaterGreaterAssign: return Operator::GreaterGreater;
        default: return Operator::Assign;
    }
}

// 字符串字面量解码（剥离首尾引号，阶段一保留转义原样）
std::string IRGenerator::decodeString(const std::string& raw) {
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
        return raw.substr(1, raw.size() - 2);
    }
    return raw;
}

// ==================== IR构建辅助 ====================

// 新建虚拟寄存器
ir::IRValue IRGenerator::newReg() {
    return ir::IRValue::reg(regCounter_++, "");
}

// 添加指令到当前块
void IRGenerator::emit(ir::Opcode opcode,
                       const std::vector<ir::IRValue>& operands,
                       const ir::IRValue& result,
                       const std::string& extra,
                       const std::string& type,
                       const SourceLocation& loc) {
    ir::IRInstruction inst;
    inst.opcode = opcode;
    inst.operands = operands;
    inst.result = result;
    inst.extra = extra;
    inst.type = type;
    inst.loc = loc;
    currentBlock_->instructions.push_back(std::move(inst));
}

// 添加带结果寄存器的指令
ir::IRValue IRGenerator::emitResult(ir::Opcode opcode,
                                    const std::vector<ir::IRValue>& operands,
                                    const std::string& type,
                                    const std::string& extra,
                                    const SourceLocation& loc) {
    ir::IRValue result = newReg();
    result.type = type;
    emit(opcode, operands, result, extra, type, loc);
    return result;
}

// 生成零常量（一元负号/按位非辅助）
ir::IRValue IRGenerator::zeroConst(const std::string& type) {
    return emitResult(ir::Opcode::ConstInt, {}, type, "0", SourceLocation());
}

// 终结当前块：无条件跳转
void IRGenerator::endJump(const std::string& target) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "跳转";
    currentBlock_->termTarget = target;
}

// 终结当前块：条件跳转（条件寄存器记录在最后一条指令操作数中）
void IRGenerator::endBranch(const std::string& condReg, const std::string& trueTarget,
                            const std::string& falseTarget) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "条件跳转";
    currentBlock_->termTrueTarget = trueTarget;
    currentBlock_->termFalseTarget = falseTarget;
    if (condReg.size() > 2 && condReg[0] == '%' && condReg[1] == 'v') {
        int regId = std::stoi(condReg.substr(2));
        if (!currentBlock_->instructions.empty()) {
            currentBlock_->instructions.back().operands.push_back(
                ir::IRValue::reg(regId, "i1"));
        }
    }
}

// 终结当前块：返回
void IRGenerator::endReturn(const std::string& valueReg) {
    currentBlock_->terminated = true;
    currentBlock_->termKind = "返回";
    currentBlock_->termReturnValue = valueReg;
}

// ==================== 基本块与变量管理 ====================

// 新建基本块并加入当前函数（同时设为当前块）
ir::IRBlock* IRGenerator::newBlock(const std::string& label) {
    auto block = std::make_unique<ir::IRBlock>();
    block->label = label;
    blockCounter_++;
    function_->blocks.push_back(std::move(block));
    currentBlock_ = function_->blocks.back().get();
    return currentBlock_;
}

// 设置当前生成块
void IRGenerator::setCurrentBlock(ir::IRBlock* block) {
    currentBlock_ = block;
}

// 分配变量寄存器：Alloca指令 + 登记映射
ir::IRValue IRGenerator::allocVar(const std::string& name, const std::string& irType,
                                  const SourceLocation& loc) {
    ir::IRValue reg = newReg();
    reg.type = irType;
    emit(ir::Opcode::Alloca, {}, reg, name, irType, loc);
    varRegs_[name] = reg.id;
    varTypes_[name] = irType;
    return reg;
}

// 查找变量寄存器（未找到返回id=-1）
ir::IRValue IRGenerator::lookupVar(const std::string& name) {
    auto it = varRegs_.find(name);
    if (it == varRegs_.end()) {
        return ir::IRValue::reg(-1, "");
    }
    ir::IRValue reg = ir::IRValue::reg(it->second, "");
    auto typeIt = varTypes_.find(name);
    if (typeIt != varTypes_.end()) reg.type = typeIt->second;
    return reg;
}

// ==================== 声明节点 ====================

// 程序入口：遍历顶层函数声明
void IRGenerator::visitProgram(Program* node) {
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            visitFunctionDecl(static_cast<FunctionDecl*>(decl.get()));
        }
    }
}

// 主入口：生成IR模块
ir::IRModule IRGenerator::generate(Program* program) {
    ir::IRModule module;
    module_ = &module;
    regCounter_ = 0;
    blockCounter_ = 0;
    varRegs_.clear();
    varTypes_.clear();
    loopStack_.clear();
    visitProgram(program);
    module_ = nullptr;
    return module;
}

// 函数定义 -> IRFunction
void IRGenerator::visitFunctionDecl(FunctionDecl* node) {
    ir::IRFunction func;
    func.name = node->name;
    func.returnType = mapType(node->returnType.empty() ? "空类型" : node->returnType);
    for (auto& param : node->params) {
        func.params.emplace_back(param->name, mapType(param->typeName));
    }
    function_ = &func;
    varRegs_.clear();
    varTypes_.clear();
    blockCounter_ = 0;
    // 入口基本块
    ir::IRBlock* entry = newBlock("块0");
    // 参数 -> 虚拟寄存器（阶段一：参数直接映射寄存器，无Alloca）
    for (auto& param : node->params) {
        ir::IRValue reg = newReg();
        reg.type = mapType(param->typeName);
        varRegs_[param->name] = reg.id;
        varTypes_[param->name] = reg.type;
    }
    // 函数体
    if (node->body != nullptr) {
        genBlock(node->body.get());
    }
    // 无终止指令：补充默认返回（空类型函数 / 原型声明）
    if (!function_->blocks.empty()) {
        ir::IRBlock* last = function_->blocks.back().get();
        if (!last->terminated) {
            setCurrentBlock(last);
            endReturn("");
        }
    } else if (node->body == nullptr) {
        // 原型声明：生成空入口块 + 默认返回
        setCurrentBlock(entry);
        endReturn("");
    }
    module_->functions.push_back(std::move(func));
    function_ = nullptr;
}

void IRGenerator::visitParamDecl(ParamDecl* node) {
    // 参数由 visitFunctionDecl 统一处理
    (void)node;
}

// 变量声明：Alloca + 初始值 Store
void IRGenerator::visitVarDecl(VarDecl* node) {
    genVarDecl(node);
}

// ==================== 语句生成 ====================

// 代码块：顺序生成语句
void IRGenerator::genBlock(BlockStmt* node) {
    for (auto& stmt : node->statements) {
        genStmt(stmt.get());
    }
}

// 表达式语句
void IRGenerator::visitExprStmt(ExprStmt* node) {
    genExpr(node->expr.get());
}

// 返回语句
void IRGenerator::visitReturnStmt(ReturnStmt* node) {
    if (node->value != nullptr) {
        ir::IRValue value = genExpr(node->value.get());
        endReturn(value.toString());
    } else {
        endReturn("");
    }
}

// 中断语句：跳转到循环出口块
void IRGenerator::visitBreakStmt(BreakStmt* node) {
    (void)node;
    if (!loopStack_.empty()) {
        endJump(loopStack_.back().breakTarget);
    }
}

// 继续语句：跳转到循环更新块
void IRGenerator::visitContinueStmt(ContinueStmt* node) {
    (void)node;
    if (!loopStack_.empty()) {
        endJump(loopStack_.back().continueTarget);
    }
}

// 如果语句（AstVisitor入口）：转发到genIf
void IRGenerator::visitIfStmt(IfStmt* node) {
    genIf(node);
}

// 当循环（AstVisitor入口）：转发到genWhile
void IRGenerator::visitWhileStmt(WhileStmt* node) {
    genWhile(node);
}

// 循环语句（AstVisitor入口）：转发到genFor
void IRGenerator::visitForStmt(ForStmt* node) {
    genFor(node);
}

// 如果语句：条件跳转生成分支块
void IRGenerator::genIf(IfStmt* node) {
    ir::IRValue cond = genExpr(node->condition.get());
    std::string thenLabel = "块" + std::to_string(blockCounter_);
    std::string elseLabel = "块" + std::to_string(blockCounter_ + 1);
    std::string endLabel = "块" + std::to_string(blockCounter_ + 2);
    // 记录条件并终结当前块
    endBranch(cond.toString(), thenLabel, elseLabel);
    // 真分支
    setCurrentBlock(newBlock(thenLabel));
    if (node->thenBranch != nullptr) genBlock(node->thenBranch.get());
    if (!currentBlock_->terminated) endJump(endLabel);
    // 否则分支（可能为否则如果链）
    setCurrentBlock(newBlock(elseLabel));
    if (node->elseBranch != nullptr) {
        if (node->elseBranch->getType() == NodeType::IfStmt) {
            genIf(static_cast<IfStmt*>(node->elseBranch.get()));
        } else {
            genBlock(static_cast<BlockStmt*>(node->elseBranch.get()));
        }
    }
    if (!currentBlock_->terminated) endJump(endLabel);
    // 汇合块
    setCurrentBlock(newBlock(endLabel));
}

// 当循环：条件块 -> 循环体 -> 回边
void IRGenerator::genWhile(WhileStmt* node) {
    std::string condLabel = "块" + std::to_string(blockCounter_);
    std::string bodyLabel = "块" + std::to_string(blockCounter_ + 1);
    std::string endLabel = "块" + std::to_string(blockCounter_ + 2);
    // 无条件跳入条件块
    endJump(condLabel);
    setCurrentBlock(newBlock(condLabel));
    ir::IRValue cond = genExpr(node->condition.get());
    endBranch(cond.toString(), bodyLabel, endLabel);
    // 循环体
    setCurrentBlock(newBlock(bodyLabel));
    loopStack_.push_back(LoopContext{endLabel, condLabel});  // 继续 -> 条件块
    if (node->body != nullptr) genBlock(node->body.get());
    loopStack_.pop_back();
    if (!currentBlock_->terminated) endJump(condLabel);
    // 出口块
    setCurrentBlock(newBlock(endLabel));
}

// 循环语句：初始化 -> 条件块 -> 循环体 -> 更新块 -> 回边
void IRGenerator::genFor(ForStmt* node) {
    // 初始化（入当前块）
    if (node->init != nullptr) {
        genStmt(node->init.get());
        if (currentBlock_->terminated) {
            // 初始化已终结（如返回）：后续不可达，补出口块
            setCurrentBlock(newBlock("块" + std::to_string(blockCounter_)));
            return;
        }
    }
    std::string condLabel = "块" + std::to_string(blockCounter_);
    std::string bodyLabel = "块" + std::to_string(blockCounter_ + 1);
    std::string updLabel = "块" + std::to_string(blockCounter_ + 2);
    std::string endLabel = "块" + std::to_string(blockCounter_ + 3);
    // 跳入条件块
    endJump(condLabel);
    setCurrentBlock(newBlock(condLabel));
    if (node->condition != nullptr) {
        ir::IRValue cond = genExpr(node->condition.get());
        endBranch(cond.toString(), bodyLabel, endLabel);
    } else {
        // 无限循环：无条件跳入循环体
        endJump(bodyLabel);
    }
    // 循环体
    setCurrentBlock(newBlock(bodyLabel));
    loopStack_.push_back(LoopContext{endLabel, updLabel});  // 继续 -> 更新块
    if (node->body != nullptr) genBlock(node->body.get());
    loopStack_.pop_back();
    if (!currentBlock_->terminated) endJump(updLabel);
    // 更新块
    setCurrentBlock(newBlock(updLabel));
    if (node->update != nullptr) {
        genExpr(node->update.get());
    }
    if (!currentBlock_->terminated) endJump(condLabel);
    // 出口块
    setCurrentBlock(newBlock(endLabel));
}

// 代码块语句：嵌套块顺序生成
void IRGenerator::visitBlockStmt(BlockStmt* node) {
    genBlock(node);
}

// 语句分发
void IRGenerator::genStmt(Stmt* node) {
    if (node == nullptr) return;
    switch (node->getType()) {
        case NodeType::VarDecl:
            genVarDecl(static_cast<VarDecl*>(node));
            break;
        case NodeType::ExprStmt:
            visitExprStmt(static_cast<ExprStmt*>(node));
            break;
        case NodeType::ReturnStmt:
            visitReturnStmt(static_cast<ReturnStmt*>(node));
            break;
        case NodeType::BreakStmt:
            visitBreakStmt(static_cast<BreakStmt*>(node));
            break;
        case NodeType::ContinueStmt:
            visitContinueStmt(static_cast<ContinueStmt*>(node));
            break;
        case NodeType::IfStmt:
            genIf(static_cast<IfStmt*>(node));
            break;
        case NodeType::WhileStmt:
            genWhile(static_cast<WhileStmt*>(node));
            break;
        case NodeType::ForStmt:
            genFor(static_cast<ForStmt*>(node));
            break;
        case NodeType::BlockStmt:
            genBlock(static_cast<BlockStmt*>(node));
            break;
        default:
            break;
    }
}

// 变量声明生成：Alloca + Store
void IRGenerator::genVarDecl(VarDecl* node) {
    // 类型推断：无显式类型时按初始值（阶段一简化）
    std::string irType = mapType(node->typeName.empty() ? "整32" : node->typeName);
    if (node->typeName.empty() && node->initializer != nullptr) {
        if (node->initializer->getType() == NodeType::FloatLiteral) irType = "f64";
        if (node->initializer->getType() == NodeType::StringLiteral) irType = "ptr";
        if (node->initializer->getType() == NodeType::BoolLiteral) irType = "i1";
        if (node->initializer->getType() == NodeType::CharLiteral) irType = "i32";
    }
    allocVar(node->name, irType, node->location);
    // 初始值 -> Store
    if (node->initializer != nullptr) {
        ir::IRValue value = genExpr(node->initializer.get());
        emit(ir::Opcode::Store, {value}, ir::IRValue(), node->name, irType, node->location);
    }
}

// ==================== 表达式生成 ====================

// 整数字面量
void IRGenerator::visitIntegerLiteral(IntegerLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", std::to_string(node->value),
                           node->location);
}

// 浮点字面量
void IRGenerator::visitFloatLiteral(FloatLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstFloat, {}, "f64",
                           std::to_string(node->value), node->location);
}

// 字符串字面量：收集到常量池
void IRGenerator::visitStringLiteral(StringLiteral* node) {
    std::string text = decodeString(node->raw);
    int index = -1;
    auto it = module_->stringIndex.find(text);
    if (it != module_->stringIndex.end()) {
        index = it->second;
    } else {
        index = static_cast<int>(module_->stringConstants.size());
        module_->stringConstants.push_back(text);
        module_->stringIndex[text] = index;
    }
    lastExpr_ = emitResult(ir::Opcode::ConstString, {}, "ptr",
                           "@str" + std::to_string(index), node->location);
}

// 字符字面量：按Unicode码点常量（阶段一简化：取首字节值）
void IRGenerator::visitCharLiteral(CharLiteral* node) {
    std::string text = decodeString(node->raw);
    int code = 0;
    if (!text.empty()) code = static_cast<unsigned char>(text[0]);
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", std::to_string(code),
                           node->location);
}

// 布尔字面量
void IRGenerator::visitBoolLiteral(BoolLiteral* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstBool, {}, "i1",
                           node->value ? "真" : "假", node->location);
}

// 标识符表达式：加载变量
void IRGenerator::visitIdentifierExpr(IdentifierExpr* node) {
    ir::IRValue reg = lookupVar(node->name);
    if (reg.id < 0) {
        // 未找到：生成零常量（防御性）
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    lastExpr_ = emitResult(ir::Opcode::Load,
                           {ir::IRValue::var(node->name, reg.type)},
                           reg.type, node->name, node->location);
}

// 二元运算：递归生成左右操作数，输出运算指令
void IRGenerator::visitBinaryExpr(BinaryExpr* node) {
    ir::IRValue left = genExpr(node->left.get());
    ir::IRValue right = genExpr(node->right.get());
    bool isFloat = (left.type == "f32" || left.type == "f64" ||
                    right.type == "f32" || right.type == "f64");
    ir::Opcode opcode;
    std::string resultType;
    if (mapBinaryOp(node->op, isFloat, opcode)) {
        // 比较/逻辑运算结果为i1，算术按操作数类型
        switch (node->op) {
            case Operator::EqualEqual: case Operator::BangEqual:
            case Operator::Less: case Operator::LessEqual:
            case Operator::Greater: case Operator::GreaterEqual:
            case Operator::AndAnd: case Operator::OrOr:
                resultType = "i1";
                break;
            default:
                resultType = isFloat ? "f64" : left.type;
                break;
        }
        lastExpr_ = emitResult(opcode, {left, right}, resultType, "", node->location);
    } else {
        // 不支持的操作：回退左操作数
        lastExpr_ = left;
    }
}

// 一元运算：! / - / ~ / ++ / --
void IRGenerator::visitUnaryExpr(UnaryExpr* node) {
    ir::IRValue operand = genExpr(node->operand.get());
    switch (node->op) {
        case Operator::Bang:
            lastExpr_ = emitResult(ir::Opcode::Not, {operand}, "i1", "", node->location);
            break;
        case Operator::Subtract: {
            // 一元负号：0 - 操作数
            ir::IRValue zero = zeroConst(operand.type);
            lastExpr_ = emitResult(ir::Opcode::Sub, {zero, operand}, operand.type, "",
                                   node->location);
            break;
        }
        case Operator::Tilde: {
            // 按位非：~x = -x - 1（阶段一用算术组合近似）
            ir::IRValue zero = zeroConst(operand.type);
            ir::IRValue minusOne = emitResult(ir::Opcode::ConstInt, {}, operand.type,
                                              "-1", node->location);
            ir::IRValue neg = emitResult(ir::Opcode::Sub, {zero, operand}, operand.type,
                                         "", node->location);
            lastExpr_ = emitResult(ir::Opcode::Sub, {neg, minusOne}, operand.type, "",
                                   node->location);
            break;
        }
        case Operator::Increment:
        case Operator::Decrement: {
            // 自增/自减：x = x ± 1（阶段一简化）
            ir::IRValue one = emitResult(ir::Opcode::ConstInt, {}, operand.type, "1",
                                         node->location);
            ir::IRValue result = emitResult(
                node->op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                {operand, one}, operand.type, "", node->location);
            // 仅当操作数为变量引用时写回
            if (node->operand->getType() == NodeType::IdentifierExpr) {
                IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->operand.get());
                ir::IRValue slot = lookupVar(ident->name);
                if (slot.id >= 0) {
                    emit(ir::Opcode::Store, {result}, ir::IRValue(), ident->name,
                         operand.type, node->location);
                }
            }
            lastExpr_ = result;
            break;
        }
        default:
            lastExpr_ = operand;
            break;
    }
}

// 赋值表达式：Store + 返回值
void IRGenerator::visitAssignmentExpr(AssignmentExpr* node) {
    // 阶段一：仅支持标识符左值
    if (node->target->getType() != NodeType::IdentifierExpr) {
        lastExpr_ = genExpr(node->value.get());
        return;
    }
    IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
    ir::IRValue value = genExpr(node->value.get());
    // 查找变量类型
    auto typeIt = varTypes_.find(ident->name);
    std::string targetType = (typeIt != varTypes_.end()) ? typeIt->second : value.type;
    // 复合赋值：值 = 当前值 op 右值
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = genExpr(node->target.get());
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            ir::IRValue combined = emitResult(opcode, {current, value}, targetType, "",
                                              node->location);
            emit(ir::Opcode::Store, {combined}, ir::IRValue(), ident->name, targetType,
                 node->location);
            lastExpr_ = combined;
            return;
        }
    }
    // 简单赋值
    emit(ir::Opcode::Store, {value}, ir::IRValue(), ident->name, targetType,
         node->location);
    lastExpr_ = value;
}

// 函数调用：实参寄存器 -> Call
void IRGenerator::visitCallExpr(CallExpr* node) {
    std::string calleeName;
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        calleeName = static_cast<IdentifierExpr*>(node->callee.get())->name;
    }
    std::vector<ir::IRValue> args;
    for (auto& arg : node->arguments) {
        args.push_back(genExpr(arg.get()));
    }
    // 阶段一简化：调用结果类型由语义分析保证，IR统一为i32
    lastExpr_ = emitResult(ir::Opcode::Call, args, "i32", calleeName, node->location);
}

// 成员访问：阶段一不支持
void IRGenerator::visitMemberExpr(MemberExpr* node) {
    (void)node;
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}

// 类型节点：不生成IR
void IRGenerator::visitType(Type* node) {
    (void)node;
}

// ==================== 表达式分发 ====================

// 生成表达式，返回结果寄存器
ir::IRValue IRGenerator::genExpr(Expr* node) {
    if (node == nullptr) {
        return ir::IRValue::reg(-1, "");
    }
    node->accept(*this);
    return lastExpr_;
}

} // namespace cn_compiler
