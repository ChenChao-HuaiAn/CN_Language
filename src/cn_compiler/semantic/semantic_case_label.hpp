// 情况标签条件域判与重复检测（1137 第三轮审查 P2-1：自 SemanticAnalyzer
//   成员声明拆出到小头文件——纯函数无分析器状态依赖；semantic_stmt.cpp
//   调用、semantic_literal_domain.cpp 实现）。
//   语义：标签键=归一十进制文本（整数 decValue；枚举/字符值 to_string）；
//   条件域=条件类型域（枚举/字符归并整32 存储域·甲案）；超域与重复返回
//   诊断文案（空=通过·调用方负责 diagnostics_.report——本函数纯函数）。
#pragma once
#include <string>
#include <unordered_set>

namespace cn_compiler {

class CaseLabel;

struct CaseLabelCheck1137 {
    std::string domainError;  // 非空=超出条件类型表示范围（编译期拒·#1076）
    std::string dupError;     // 非空=情况值重复
};

CaseLabelCheck1137 checkCaseLabel1137(const CaseLabel* caseNode,
                                      const std::string& condType, bool enumCond,
                                      std::unordered_set<std::string>& seenTexts);

}  // namespace cn_compiler
