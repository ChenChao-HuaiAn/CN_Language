# -*- coding: utf-8 -*-
# B-1（2026-08）：semantic.cpp 按成员函数切分子文件（纯移动，不改逻辑）
# 用法：python tools/split_semantic.py
import re, os

SRC = "src/cn_compiler/semantic/semantic.cpp"

with open(SRC, encoding="utf-8") as f:
    lines = f.readlines()

def find_end(start_idx):
    """从函数签名行 start_idx 起做大括号匹配，返回函数结束行号（含）。"""
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

# 成员函数定义行（返回类型 前缀 + SemanticAnalyzer::名）
fn_pat = re.compile(r'^([A-Za-z_][A-Za-z0-9_:<>*&\[\] ]*\s+)SemanticAnalyzer::(\w+)\(')
fns = []  # (name, start_line, end_line)
for idx, line in enumerate(lines):
    m = fn_pat.match(line)
    if m:
        # 排除声明（无 body：行尾是 ;）与内联
        end = find_end(idx)
        fns.append((m.group(2), idx, end))

print("member functions:", len(fns))

# 分组：函数名 -> 目标文件
GROUPS = {
    "semantic_expr.cpp": [
        "visitIntegerLiteral","visitFloatLiteral","visitStringLiteral","visitCharLiteral",
        "visitBoolLiteral","visitNullLiteral","visitIdentifierExpr","visitBinaryExpr",
        "visitUnaryExpr","visitTernaryExpr","visitAssignmentExpr","visitMemberExpr",
        "visitIndexExpr","visitInitListExpr","visitStructInitExpr","visitSelfExpr",
        "visitSuperExpr","visitType","collectLambdaCaptures","visitSizeofExpr",
        "visitCastExpr","visitLambdaExpr",
    ],
    "semantic_call.cpp": ["wrapRefArgs","visitCallExpr"],
    "semantic_stmt.cpp": [
        "visitBlockStmt","visitExprStmt","visitIfStmt","visitWhileStmt","visitForStmt",
        "visitReturnStmt","visitBreakStmt","visitContinueStmt","visitSwitchStmt",
        "visitCaseLabel","visitDefaultLabel",
    ],
    "semantic_decl.cpp": [
        "visitVarDecl","visitImportDecl","visitClassDecl","visitClassMember",
        "visitInterfaceDecl","visitGenericDecl","checkFunctionBody","visitFunctionDecl",
        "visitParamDecl",
    ],
}

# 公共头部（includes + namespace + 匿名命名空间 static 辅助）
# 找到第一个成员函数前的所有内容（含头部注释 + static 辅助）
first_fn_start = fns[0][1]
header = lines[:first_fn_start]
# header 以 "namespace cn_compiler {" 结束；static 辅助在其后
ns_idx = next(i for i, l in enumerate(header) if l.strip() == "namespace cn_compiler {")
header_core = header[:ns_idx + 1]          # includes + namespace {
anon_ns_start = ns_idx + 1                  # namespace { 行
anon_ns_end = next(i for i in range(ns_idx + 1, len(header)) if header[i].strip() == "} // namespace")
static_helpers = header[anon_ns_start:anon_ns_end + 1]

def make_header():
    return header_core + ["\n"] + static_helpers + ["\n"]

def make_footer():
    return ["} // namespace cn_compiler\n"]

# 按函数名索引
fn_by_name = {name: (s, e) for name, s, e in fns}

# 主文件保留集
kept = [name for name, _, _ in fns if name not in {g for vals in GROUPS.values() for g in vals}]
print("kept in main:", len(kept))

# 生成子文件
out_dir = os.path.dirname(SRC)
for fname, names in GROUPS.items():
    body = []
    for name in names:
        s, e = fn_by_name[name]
        body.extend(lines[s:e + 1])
    content = make_header() + body + make_footer()
    path = os.path.join(out_dir, fname)
    with open(path, "w", encoding="utf-8") as f:
        f.writelines(content)
    print("wrote", fname, len(body), "lines")

# 主文件：保留函数 + 尾部
main_body = []
for name in kept:
    s, e = fn_by_name[name]
    main_body.extend(lines[s:e + 1])
main_content = header_core + ["\n"] + static_helpers + ["\n"] + main_body + ["} // namespace cn_compiler\n"]
with open(SRC, "w", encoding="utf-8") as f:
    f.writelines(main_content)
print("main semantic.cpp now", len(main_body), "function lines")
