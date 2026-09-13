// AST节点定义：语法分析器的输出、语义分析器的输入（Task 1.3/1.5）
// D1 125-a：按层拆分（ast_base/ast_expr/ast_stmt/ast_decl）——
//   本文件保留为聚合入口（转发 include 四层；包含面零改动）。
#pragma once

#include "cn_compiler/parser/ast_base.hpp"
#include "cn_compiler/parser/ast_expr.hpp"
#include "cn_compiler/parser/ast_stmt.hpp"
#include "cn_compiler/parser/ast_decl.hpp"
