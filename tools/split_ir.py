# -*- coding: utf-8 -*-
# B-1（2026-08）：ir.cpp 按成员函数切分子文件（纯移动，不改逻辑）
import re, os

SRC = "src/cn_compiler/ir/ir.cpp"
with open(SRC, encoding="utf-8") as f:
    lines = f.readlines()

def find_end(start_idx):
    depth = 0
    i = start_idx
    in_str = False
    in_char = False
    in_line_comment = False
    in_block_comment = False
    while i < len(lines):
        line = lines[i]
        j = 0
        while j < len(line):
            c = line[j]
            if in_line_comment:
                pass
            elif in_block_comment:
                if c == '*' and j + 1 < len(line) and line[j+1] == '/':
                    in_block_comment = False
                    j += 1
            elif in_str:
                if c == '\\':
                    j += 1
                elif c == '"':
                    in_str = False
            elif in_char:
                if c == '\\':
                    j += 1
                elif c == "'":
                    in_char = False
            else:
                if c == '/' and j + 1 < len(line) and line[j+1] == '/':
                    in_line_comment = True
                    j += 1
                elif c == '/' and j + 1 < len(line) and line[j+1] == '*':
                    in_block_comment = True
                    j += 1
                elif c == '"':
                    in_str = True
                elif c == "'":
                    in_char = True
                elif c == '{':
                    depth += 1
                elif c == '}':
                    depth -= 1
                    if depth == 0:
                        return i
            j += 1
        in_line_comment = False
        i += 1
    raise RuntimeError("unbalanced at line %d" % start_idx)

fn_pat = re.compile(r'^([A-Za-z_][A-Za-z0-9_:<>*&\[\] ]*\s+)IRGenerator::(\w+)\(')
fns = []
for idx, line in enumerate(lines):
    m = fn_pat.match(line)
    if m:
        end = find_end(idx)
        fns.append((m.group(2), idx, end))
print("member functions:", len(fns))

GROUPS = {
    "ir_expr.cpp": [
        "visitIntegerLiteral","visitFloatLiteral","visitStringLiteral","internEmptyString",
        "evalDefaultExpr","visitCharLiteral","visitBoolLiteral","visitIdentifierExpr",
        "isStringTypedExpr","visitBinaryExpr","visitUnaryExpr","visitTernaryExpr",
        "visitAssignmentExpr","visitNullLiteral","visitIndexExpr","visitInitListExpr",
        "emitStructInitTo","visitStructInitExpr","visitMemberExpr","visitType",
        "visitSizeofExpr","visitCastExpr","visitLambdaExpr",
    ],
    "ir_call.cpp": ["visitCallExpr"],
    "ir_stmt.cpp": [
        "visitExprStmt","visitReturnStmt","visitBreakStmt","visitContinueStmt",
        "visitIfStmt","visitWhileStmt","visitForStmt","visitSwitchStmt",
        "visitCaseLabel","visitDefaultLabel","genIf","genWhile","genFor","genSwitch",
        "visitBlockStmt","genStmt","genVarDecl","genBlock",
    ],
    "ir_decl.cpp": [
        "visitProgram","generate","visitFunctionDecl","visitParamDecl","visitVarDecl",
        "visitStructDecl","visitEnumDecl",
    ],
}

first_fn_start = fns[0][1]
header = lines[:first_fn_start]
# ir.cpp 无匿名 namespace：子文件头部 = includes + 两个 namespace 打开行
# （opcodeToString 等自由函数留在主文件，ir.hpp 有声明，子文件可链接使用）
ns_idx = next(i for i, l in enumerate(header) if l.strip() == "namespace cn_compiler {")
# 成员函数定义在 namespace cn_compiler 直接作用域（ir.cpp 的 } // namespace ir
# 在第一个成员函数之前已闭合）——子文件 header 只需 includes + namespace cn_compiler {
header_core = header[:ns_idx + 1]

def make_header():
    return header_core + ["\n"]

def make_footer():
    return ["} // namespace cn_compiler\n"]

fn_by_name = {name: (s, e) for name, s, e in fns}
kept = [name for name, _, _ in fns if name not in {g for vals in GROUPS.values() for g in vals}]
print("kept in main:", len(kept))

out_dir = os.path.dirname(SRC)
for fname, names in GROUPS.items():
    body = []
    for name in names:
        s, e = fn_by_name[name]
        body.extend(lines[s:e + 1])
    with open(os.path.join(out_dir, fname), "w", encoding="utf-8") as f:
        f.writelines(make_header() + body + make_footer())
    print("wrote", fname, len(body), "lines")

main_body = []
for name in kept:
    s, e = fn_by_name[name]
    main_body.extend(lines[s:e + 1])
# 主文件：完整头部（含 opcodeToString 自由函数与 } // namespace ir 闭合）+
#   保留函数（namespace cn_compiler 直接作用域）
with open(SRC, "w", encoding="utf-8") as f:
    f.writelines(header + main_body + ["} // namespace cn_compiler\n"])
print("main ir.cpp now", len(main_body), "function lines")
