// 语义·变量声明初始化器检查族（#1769 轮拆分 自 semantic_decl.cpp visitVarDecl：
//   数组初始化列表/非数组初始化列表/结构体初始化 三分支+多维嵌套递归检查。
//   先拆再加纪律——semantic_decl.cpp 已 950 行撞警戒线；本文件承接多维新逻辑）。
//
// #1769（2026-10-31 用户批甲案·二维嵌套初始化列表 {{...}} 立法+实施）：
//   ①嵌套大括号初始化列表（整32[2][2] 表 = { { 1, 2 }, { 3, 4 } }）——数组初
//     始化列表的元素本身可为初始化列表（嵌套深度=数组维度）；结果<..>/类/容器
//     元素形态按元素类型递归展开。
//   ②缺格沿用 #1473 已批甲案「补类型默认态」（字符串=空串态·整/浮=0·类=默认
//     构造·结果<..>=异常态〔零位模式·001 §3.5 条文写明〕）——检查面放行，
//     IR 面补齐。
//   ③维度不齐（内层元素个数>该维声明长度）=编译期拒·逐维同文案。
//   ④拉平禁入——一维清单塞多维数组=编译期拒（堵静默重排·诊断指路嵌套写法）。
//   立法条文=plans/001 §3.5（随本任务分支落盘〔通告〕）。
#include <string>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 变量声明初始化器形态分派（visitVarDecl 搬出段·原行为逐字保持）：
//   ①数组类型 × 初始化列表 → checkArrayInitListDim 递归裁定（一维行为与原
//     单层检查逐字一致：元素转换检查/个数超长拒文案不变）
//   ②初始化列表 × 非数组类型 → 独立表达式检查（visitInitListExpr 诊断）
//   ③结构体/联合体初始化 → 类型匹配检查
//   其余（非初始化列表 initializer）不进本函数（visitVarDecl 原分支序保持）。
void SemanticAnalyzer::checkVarDeclInitList(VarDecl* node,
                                            const std::string& varType) {
    if (node->initializer->getType() == NodeType::InitListExpr &&
        types::isArray(varType)) {
        InitListExpr* initList =
            static_cast<InitListExpr*>(node->initializer.get());
        // #1769：多维递归裁定（一维=单层·行为与原实现一致）
        checkArrayInitListDim(initList, varType,
                              node->initializer->location);
        return;
    }
    if (node->initializer->getType() == NodeType::InitListExpr &&
        !types::isArray(varType)) {
        // 初始化列表用于非数组类型：报错
        checkExpr(node->initializer.get());
        return;
    }
    if (node->initializer->getType() == NodeType::StructInitExpr) {
        // 结构体/联合体初始化列表：类型须匹配（Task 2.7）
        std::string initType = checkExpr(node->initializer.get());
        if (varType != "未知" && !canConvertType(initType, varType)) {
            diagnostics_.report(DiagnosticLevel::Error,
                                node->initializer->location,
                                "无法将 '" + initType + "' 隐式转换为 '" +
                                    varType + "'");
        }
    }
}

// #1769：数组初始化列表维度递归检查。
//   arrayType=当前层数组类型（顶层 整32[2][2] / 内层 整32[2]）；elemType=剥
//   首维元素类型（多维时仍为数组=递归一层）。诊断文案双侧（宿主/v2）逐字一致：
//   ①拉平禁入 ②嵌塞一维 ③逐维超界（847 既有锚逐字·多维同文案）④标量元素
//   转换检查（原一维逻辑·869 切片专用诊断保留）。缺格（元素数<该维长度）=
//   合法（补类型默认态·1473 甲案）。
void SemanticAnalyzer::checkArrayInitListDim(InitListExpr* list,
                                             const std::string& arrayType,
                                             const SourceLocation& loc) {
    const std::string elemType = types::arrayElemOf(arrayType);
    // 1360 结果化：isArray 守卫下首维恒纯数字（多维声明收口保证）——无值=
    //   内部缺陷（多维声明漏检）显形，禁静默按 0 继续（648 红线本面）
    const cn::Optional<int> lenOpt = types::arrayLenOf(arrayType);
    if (!lenOpt.has_value()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "内部错误: 数组声明首维长度缺失（'" + arrayType +
                                "' 应为已收口的多维数组类型）");
        return;
    }
    const int len = lenOpt.value();
    const bool elemIsArray = types::isArray(elemType);
    for (auto& elem : list->elements) {
        const bool elemIsList = elem->getType() == NodeType::InitListExpr;
        if (elemIsArray && !elemIsList) {
            // 拉平禁入：一维清单塞多维数组（静默重排拒绝·诊断指路嵌套写法）
            diagnostics_.report(
                DiagnosticLevel::Error, elem->location,
                "数组维度不匹配: 元素须为嵌套初始化列表 { ... }（声明元素类型 '" +
                    elemType + "' 仍为数组——拉平写法禁入，请按维度嵌套）");
            checkExpr(elem.get());  // 下钻子树暴露后续错误（级联防）
            continue;
        }
        if (!elemIsArray && elemIsList) {
            // 嵌套列表塞一维数组（维度过深）
            diagnostics_.report(
                DiagnosticLevel::Error, elem->location,
                "数组维度不匹配: 元素不接受嵌套初始化列表 { ... }（声明元素类型 '" +
                    elemType + "' 非数组）");
            checkExpr(elem.get());
            continue;
        }
        if (elemIsArray && elemIsList) {
            // 嵌套合法：递归下一维（维度=数组维度逐层对应）
            checkArrayInitListDim(static_cast<InitListExpr*>(elem.get()),
                                  elemType, elem->location);
            continue;
        }
        // 标量元素：原一维转换检查（55-c 字面量豁免/869 切片专用诊断·逐字保持）
        // #1588：元素位目标=元素类型（构造器按目标载荷宽度发射）
        ctorTargetStack_.push_back(elemType);
        std::string elemInitType = checkExpr(elem.get());
        ctorTargetStack_.pop_back();
        // 869（234 立法执法面收口·v2 侧 869 同轮对齐）：值形态类切片
        //   （派生类值 → 基类数组元素类型）=专用切片诊断；切片判定真=
        //   已报，不再叠加「数组元素无法将」通用诊断。
        if (!canConvertWithLiteral(elem.get(), elemInitType, elemType) &&
            !reportSliceReject(elemInitType, elemType, elem->location)) {
            diagnostics_.report(DiagnosticLevel::Error, elem->location,
                                "数组元素无法将 '" + elemInitType +
                                    "' 隐式转换为 '" + elemType + "'");
        }
    }
    // 逐维超界：元素个数不得超过该维声明长度（部分初始化允许=缺格补默认态）。
    //   文案与 847 一维锚逐字一致（多维内层同文案·双侧一致）。
    if (len > 0 && static_cast<int>(list->elements.size()) > len) {  // 静默豁免业务值: 层长0=零长数组（811 立法无禁令·塞元素不报超界）——847 文案逐字锚同款守卫（visitVarDecl 原行搬移·v2 同步态）
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "数组初始化元素个数 " +
                            std::to_string(list->elements.size()) +
                            " 超过数组长度 " + std::to_string(len));
    }
}

} // namespace cn_compiler
