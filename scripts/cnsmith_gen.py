#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 生成器（支柱二·plans/026 §2.2·245-a）

按规范文法生成随机但合法的 CN 程序（确定性种子可复现）。
首期生成面：整32/整64 变量、算术（+ - * / %）、位运算（& | ^ << >>）、
比较（== != < <= > >=）、逻辑（&& || !）、三元、打印行。

用法：python scripts/cnsmith_gen.py --seed 1 --count 10 --out target/cnsmith
"""
import argparse
import os
import random

NL = "\n"


class Gen:
    def __init__(self, seed):
        self.rng = random.Random(seed)
        self.vars = []          # (名称, 类型) 整32/整64
        self.tmp = 0

    def new_name(self, typ):
        self.tmp += 1
        return "变%d" % self.tmp

    def declare(self, name, typ):
        self.vars.append((name, typ))

    def expr(self, depth, typ="整32"):
        """生成一个 typ 类型表达式。"""
        if depth <= 0 or self.rng.random() < 0.35:
            return self.atom(typ)
        kind = self.rng.randrange(6)
        if typ == "整32" and kind == 0:
            # 算术二元
            op = self.rng.choice(["+", "-", "*"])
            return "(%s %s %s)" % (self.expr(depth - 1), op, self.expr(depth - 1))
        if typ == "整32" and kind == 1:
            # 位运算/移位
            op = self.rng.choice(["&", "|", "^", "<<", ">>"])
            return "(%s %s %s)" % (self.expr(depth - 1), op, self.expr(depth - 1))
        if kind == 2 and depth >= 2:
            # 除法/取模（除数恒非零保护：字面量 1..9）
            op = self.rng.choice(["/", "%"])
            return "(%s %s %d)" % (self.expr(depth - 1), op, self.rng.randint(1, 9))
        if typ == "整32" and kind == 3:
            # 比较产生布尔 -> 再转整（三元取整分支）
            ops = ["==", "!=", "<", "<=", ">", ">="]
            b = "(%s %s %s)" % (self.expr(depth - 1), self.rng.choice(ops),
                                self.expr(depth - 1))
            return "(%s ? 1 : 0)" % b
        if typ == "整32" and kind == 4 and depth >= 2:
            # 逻辑组合（操作数=比较表达式·布尔）转 0/1
            ops = ["==", "!=", "<", "<=", ">", ">="]
            op = self.rng.choice(["&&", "||"])
            cmp_op = self.rng.choice(ops)
            lhs = "(%s %s %s)" % (self.expr(depth - 2), cmp_op,
                                  self.expr(depth - 2))
            cmp_op2 = self.rng.choice(ops)
            rhs = "(%s %s %s)" % (self.expr(depth - 2), cmp_op2,
                                  self.expr(depth - 2))
            return "((%s %s %s) ? 1 : 0)" % (lhs, op, rhs)
        return self.atom(typ)

    def atom(self, typ):
        r = self.rng.random()
        if self.vars and r < 0.4:
            # 复用已声明变量（类型匹配优先，类型不同做显式转换保证合法）
            name, vt = self.rng.choice(self.vars)
            if vt == typ:
                return name
            if typ == "整32":
                return "整32(%s)" % name
            return "整64(%s)" % name
        if typ == "整64":
            return "%dL" % self.rng.randint(-100000, 100000)
        return str(self.rng.randint(-1000, 1000))

    def program(self, stmt_n=14):
        out = []
        out.append("函数 主() -> 整32 {")
        out.append('    打印行("=== CN-Smith 采样 ===");')
        for _ in range(stmt_n):
            out.append(self.statement())
        out.append('    打印行("done");')
        out.append("    返回 0;")
        out.append("}")
        return NL.join(out)

    def statement(self):
        typ = self.rng.choice(["整32", "整32", "整32", "整64"])
        name = self.new_name(typ)
        val = self.expr(self.rng.randint(2, 5), typ)  # 先生成值（未登记，避免自引用）
        self.declare(name, typ)
        head = "整64" if typ == "整64" else "整32"
        line = "    %s %s = %s;" % (head, name, val)
        if self.rng.random() < 0.6:
            line += NL + '    打印行("%s = ", %s);' % (name, name)
        return line


# 负向注入（246-a）：对合法程序注入一个随机语法/结构错误——
#   验证编译器「必有诊断、永不崩溃」（T6 族机械化）。
#   注入器=字符串级手术（生成后再破坏），不追求语义多样性（覆盖语法层防线）。
INJECTIONS = [
    "删分号",       # 语句缺少分号
    "缺右括号",     # 括号不闭合
    "缺右花括号",   # 块不闭合
    "非法字符",     # 词法层未知符号
    "截断文件",     # 结构突然中断
]


def inject_error(src, rng):
    kind = rng.choice(INJECTIONS)
    lines = src.split(NL)
    if kind == "删分号" and len(lines) > 4:
        i = rng.randrange(1, len(lines) - 1)
        lines[i] = lines[i].replace(";", "", 1)
    elif kind == "缺右括号" and len(lines) > 4:
        i = rng.randrange(1, len(lines) - 1)
        if "(" in lines[i]:
            j = lines[i].rfind(")")
            lines[i] = lines[i][:j] + lines[i][j + 1:]
    elif kind == "缺右花括号":
        # 删除最后一个 }（函数体不闭合）
        for i in range(len(lines) - 1, -1, -1):
            if lines[i].strip() == "}":
                lines.pop(i)
                break
    elif kind == "非法字符":
        i = rng.randrange(1, len(lines))
        lines[i] = lines[i] + " @#$"
    elif kind == "截断文件":
        cut = rng.randrange(len(lines) // 2, len(lines))
        lines = lines[:cut]
    return (NL.join(lines) + NL + "# 注入类型: " + kind + NL), kind


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=10)
    ap.add_argument("--out", default="target/cnsmith")
    ap.add_argument("--stmts", type=int, default=14)
    ap.add_argument("--negative", action="store_true",
                    help="负向注入模式：生成非法程序（验证必有诊断不崩溃）")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    for i in range(a.count):
        seed = a.seed + i
        g = Gen(seed)
        src = g.program(a.stmts)
        ext = ".cn"
        if a.negative:
            rng = random.Random(seed * 31 + 7)
            src, kind = inject_error(src, rng)
            ext = ".cn"
        with open(os.path.join(a.out, "s%d%s" % (seed, ext)), "w",
                  encoding="utf-8", newline="") as f:
            f.write(src)
    mode = "负向注入" if a.negative else "合法"
    print("生成 %d 个程序（%s） -> %s" % (a.count, mode, a.out))


if __name__ == "__main__":
    main()
