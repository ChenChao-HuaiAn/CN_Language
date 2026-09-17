#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""famscan——缺陷族面扩散探针（B9 制度工具·plans/026 §2.11·2026-09-17 用户批准转正）。

制度定位（与 B8 面台账互补：B8=构件面自顶向下巡视，本工具=根因面自底向上扩散）：
  已知点缺陷 → 提炼根因模式 → 按扩散轴（求值位置×类型位宽×消费指令×符号性×优化级别）
  枚举同族全形态 → 双 oracle（gcc 同语义 C 对拍 + python C 语义模拟互检）
  → CN O0/O3 实测比对 → 跨后端 asm 静态定性（asm 由 --keep 保留可人工审）。

用法（cwd=仓库根或任意目录均可）：
  python3 scripts/famscan.py                     # 默认 target/cn·输出 /tmp/famscan
  python3 scripts/famscan.py --cn target/cn --out /tmp/famscan_verify --keep
判据：
  命中数=CN 输出与 oracle 不一致的同族形态数（缺陷在=预期非零；根治轮复跑应→0）。
  退出码恒 0（工具自身成功）；gcc 缺失时 C 对拍自动降级（单 oracle·输出标注）。
首案基线（T23·2026-09-17·linux_x64）：16 命中——287-a 修复轮复验判据=同命令 0 命中。
"""
import argparse, subprocess, os, re, json, shutil, sys

# ---------- 缺陷族用例库（T23 首案固化：4 族 47 用例） ----------
def i(v, w):   return (v + (1 << (w-1))) % (1 << w) - (1 << (w-1))
def u(v, w):   return v % (1 << w)
def tdiv(a, b): q = abs(a)//abs(b); return q if (a<0)==(b<0) else -q
def tmod(a, b): return a - tdiv(a, b)*b
def sar(a, n, w): return i(a, w) >> (n % w)
def shr(a, n, w): return u(a, w) >> (n % w)
def shl(a, n, w): return i(i(a, w) << (n % w), w)
U64MAX = 18446744073709551615

PRELUDE_CN = [
    "静态 整32 静甲 = -426;",
    "静态 整16 静窄 = -426;",
    "函数 取负() -> 整32 { 返回 -426; }",
    "函数 取移(整32 左, 整32 量) -> 整32 { 返回 左 >> 量; }",
]
PRELUDE_CN_BODY = [
    "整32 甲 = -426; 整32 乙 = -7; 整32 丙 = 13; 整32 丁 = 4; 整32 戊 = -5875;",
    "整32[2] 阵; 阵[0] = -426; 阵[1] = 13;",
    "整32 己 = -426; 己 >>= 13;",
    "整16 窄 = -426; 整8 微 = -100; 整64 长 = -426L;",
    "整32 边 = -2147483648; 整16 窄边 = -32768;",
    "正32 甲无 = 3000000000; 正32 无极大 = 4294967295; 正32 乙无 = 3;",
    "正64 甲大 = 18446744073709551615;",
    "点 p = 点{x = -426}; 点 p2 = 点{x = -426};",
]
PRELUDE_C_HEAD = [
    "#include <stdio.h>", "#include <stdint.h>", "#include <limits.h>",
    "struct P { int32_t x; };",
    "static int32_t sa = -426;",
    "static int16_t sn = -426;",
    "int32_t fneg(void) { return -426; }",
    "int32_t fsh(int32_t l, int32_t r) { return l >> r; }",
]
PRELUDE_C_BODY = [
    "int32_t a=-426, b=-7, c=13, d=4, e=-5875;",
    "int32_t arr[2]; arr[0]=-426; arr[1]=13;",
    "int32_t j1=-426; j1 >>= 13;",
    "int16_t n16=-426; int8_t n8=-100; int64_t l64=-426;",
    "int32_t mn=INT32_MIN; int16_t m16=INT16_MIN;",
    "uint32_t ua=3000000000U, um=4294967295U, ub2=3U;",
    "uint64_t big=18446744073709551615ULL;",
    "struct P p={-426}, p2={-426};",
]

CASES = []
def add(tag, cn_expr, c_expr, exp, cfmt="%lld", group=""):
    CASES.append((tag, cn_expr, c_expr, str(exp), cfmt, group))

g = "F1"  # 右移零扩展装载·位置面（点=s2026091705·287-a）
add("F1_01局部",        "甲 >> 丙",                 "a >> c",            sar(-426,13,32), group=g)
add("F1_02静态",        "静甲 >> 13",               "sa >> 13",          sar(-426,13,32), group=g)
add("F1_03数组",        "阵[0] >> 阵[1]",           "arr[0] >> arr[1]",  sar(-426,13,32), group=g)
add("F1_04字段",        "p.x >> 13",                "p.x >> 13",         sar(-426,13,32), group=g)
add("F1_05返回值",      "取负() >> 13",             "fneg() >> 13",      sar(-426,13,32), group=g)
add("F1_06实参",        "取移(-426, 13)",           "fsh(-426, 13)",     sar(-426,13,32), group=g)
add("F1_08三元",        "(真 ? 甲 : 乙) >> 丙",     "(1 ? a : b) >> c",  sar(-426,13,32), group=g)
add("F1_09复合赋值",    "己",                       "j1",                sar(-426,13,32), group=g)
add("F1_10负移位量",    "甲 >> 戊",                 "a >> e",            sar(-426,-5875%32,32), group=g)
add("F1_11左移后比较",  "(甲 << 丁) < 0 ? 1 : 0",   "(a << d) < 0 ? 1 : 0", 1, group=g)
add("F1_12左移转整64",  "整64(甲 << 丁)",           "(int64_t)(a << d)", shl(-426,4,32), group=g)
add("F1_13左移值",      "甲 << 丁",                 "a << d",            shl(-426,4,32), group=g)

g = "F2"  # 右移·类型位宽面（点=T13）
add("F2_01整16",        "窄 >> 3",                  "n16 >> 3",          sar(-426,3,16), group=g)
add("F2_02整8",         "微 >> 2",                  "(int32_t)(n8 >> 2)", sar(-100,2,8), group=g)
add("F2_03整64",        "长 >> 13",                 "l64 >> 13",         sar(-426,13,64), group=g)
add("F2_04整32边界",    "边 >> 3",                  "mn >> 3",           sar(-2147483648,3,32), group=g)
add("F2_05整16边界",    "窄边 >> 3",                "(int32_t)(m16 >> 3)", sar(-32768,3,16), group=g)
add("F2_06折叠负字面量","(-426) >> 13",             "(-426) >> 13",      sar(-426,13,32), group=g)
add("F2_07折叠T13原点", "(-12) >> 3",               "(-12) >> 3",        sar(-12,3,32), group=g)
add("F2_08静态整16",    "静窄 >> 3",                "sn >> 3",           sar(-426,3,16), group=g)

g = "F3"  # 符号扩展装载·消费指令面（点=T14）
add("F3_01转整64",      "整64(甲)",                 "(int64_t)a",        i(-426,64), group=g)
add("F3_02转浮64",      "浮64(甲)",                 "(double)a",         "-426.000000", cfmt="%.6f", group=g)
add("F3_03转浮32",      "浮32(甲)",                 "(double)(float)a",  "-426.000000", cfmt="%.6f", group=g)
add("F3_04除法",        "甲 / 乙",                  "a / b",             tdiv(-426,-7), group=g)
add("F3_05取模",        "甲 % 乙",                  "a % b",             tmod(-426,-7), group=g)
add("F3_06乘法",        "甲 * 乙",                  "a * b",             i(-426*-7,32), group=g)
add("F3_07比较",        "甲 < 乙 ? 1 : 0",          "a < b ? 1 : 0",     1, group=g)  # -426 < -7 为真（首案 python 期望曾写反·C 对拍纠正）
add("F3_08位与",        "甲 & 乙",                  "a & b",             i(-426 & -7,32), group=g)
add("F3_09位或",        "甲 | 乙",                  "a | b",             i(-426 | -7,32), group=g)
add("F3_10异或",        "甲 ^ 乙",                  "a ^ b",             i(-426 ^ -7,32), group=g)
add("F3_11按位非",      "~甲",                      "~a",                i(~(-426),32), group=g)
add("F3_12判真",        "甲 != 0 ? 1 : 0",          "a != 0 ? 1 : 0",    1, group=g)
add("F3_13加法",        "甲 + 乙",                  "a + b",             i(-433,32), group=g)
add("F3_14静态转浮64",  "浮64(静甲)",               "(double)sa",        "-426.000000", cfmt="%.6f", group=g)
add("F3_15字段转浮64",  "浮64(p2.x)",               "(double)p2.x",      "-426.000000", cfmt="%.6f", group=g)

g = "F4"  # 无符号算术分派姊妹面（由 T13/T14 联想扩散·首案全对=修复边界证据）
add("F4_01正32右移",    "甲无 >> 4",                "ua >> 4",           shr(3000000000,4,32), cfmt="%llu", group=g)
add("F4_02正32除法",    "甲无 / 3",                 "ua / 3",            u(tdiv(3000000000,3),32), cfmt="%llu", group=g)
add("F4_03正32取模",    "甲无 % 7",                 "ua % 7",            u(tmod(3000000000,7),32), cfmt="%llu", group=g)
add("F4_04正32边界加回绕","甲无 + 1294967296",      "ua + 1294967296U",  u(4294967296,32), cfmt="%llu", group=g)
add("F4_05正32极大右移","无极大 >> 28",             "um >> 28",          shr(4294967295,28,32), cfmt="%llu", group=g)
add("F4_06正64超大除法","甲大 / 7",                 "big / 7",           u(tdiv(U64MAX,7),64), cfmt="%llu", group=g)
add("F4_07正64超大取模","甲大 % 7",                 "big % 7",           u(tmod(U64MAX,7),64), cfmt="%llu", group=g)
add("F4_08正64超大右移","甲大 >> 4",                "big >> 4",          shr(U64MAX,4,64), cfmt="%llu", group=g)
add("F4_09正64超大比较","甲大 > 9223372036854775807 ? 1 : 0", "big > 9223372036854775807LL ? 1 : 0", 1, group=g)
add("F4_10正32转整64",  "整64(甲无)",               "(int64_t)ua",       u(3000000000,32), group=g)
add("F4_11正32转浮64",  "浮64(甲无)",               "(double)ua",        "3000000000.000000", cfmt="%.6f", group=g)
add("F4_12正32乘法回绕","甲无 * 乙无",              "ua * ub2",          u(9000000000,32), cfmt="%llu", group=g)


def gen_cn():
    cn = ["结构体 点 { 整32 x }"] + PRELUDE_CN + [
        "函数 主() -> 整32 {"] + ["    " + s for s in PRELUDE_CN_BODY] + [
        '    打印行("%s=", %s);' % (t, e) for t, e, _, _, _, _ in CASES] + [
        "    返回 0;", "}"]
    return "\n".join(cn) + "\n"

def gen_c():
    body = []
    for t, _, ce, _, f, _ in CASES:
        if f in ("%lld", "%llu"):
            cast = "(unsigned long long)(" if f == "%llu" else "(long long)("
            body.append('    printf("' + t + '=' + f + '\\n", ' + cast + ce + '));')
        else:  # 浮点 %.6f：实参本身为 double，禁止整型提升包装
            body.append('    printf("' + t + '=' + f + '\\n", ' + ce + ');')
    c = PRELUDE_C_HEAD + ["int main(void) {"] + ["    " + s for s in PRELUDE_C_BODY] + body + [
        "    return 0;", "}"]
    return "\n".join(c) + "\n"


def parse(s):
    # CN 打印行 不带换行：值按纯数字提取，防吞后续 tag
    return dict(re.findall(r'(F\d_\d+[^\s=]*)=(-?\d+(?:\.\d+)?)', s))


def main():
    ap = argparse.ArgumentParser(description="缺陷族面扩散探针（B9·T23 复验基准）")
    ap.add_argument("--cn", default=None, help="CN 编译器路径（默认 <仓库根>/target/cn）")
    ap.add_argument("--out", default="/tmp/famscan", help="工作输出目录")
    ap.add_argument("--keep", action="store_true", help="保留 .s/.asm 产物供人工审（默认仅留 .out/.json）")
    args = ap.parse_args()

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    cn = args.cn or os.path.join(root, "target", "cn")
    if not os.path.isfile(cn):
        print("错误: CN 编译器不存在：%s（先构建或用 --cn 指定）" % cn); return 2
    have_gcc = shutil.which("gcc") is not None
    if not have_gcc:
        print("警告: 无 gcc——C 对拍降级（单 oracle=python C 语义模拟）")
    out = args.out; os.makedirs(out, exist_ok=True)

    open(os.path.join(out, "fam.cn"), "w", encoding="utf-8").write(gen_cn())

    def run(cmd, t=180):
        return subprocess.run(cmd, capture_output=True, text=True, timeout=t, cwd=root)

    c_out = None
    if have_gcc:
        open(os.path.join(out, "fam.c"), "w", encoding="utf-8").write(gen_c())
        r = run(["gcc", "-O0", "-w", os.path.join(out, "fam.c"), "-o", os.path.join(out, "fam_c")])
        if r.returncode != 0:
            print("C 参考编译失败（降级单 oracle）:\n", r.stderr[:800])
        else:
            c_out = subprocess.run([os.path.join(out, "fam_c")], capture_output=True, text=True, timeout=15).stdout
            open(os.path.join(out, "rt_c.out"), "w").write(c_out)

    outs = {}
    for lvl, suf in (("-O0", "o0"), ("-O3", "o3")):
        exe = os.path.join(out, "fam_" + suf)
        b = run([cn, "build", os.path.join(out, "fam.cn"), "--target", "linux-x86_64", lvl, "--output", exe])
        if b.returncode != 0:
            print("CN", lvl, "编译失败:\n", b.stderr[-2000:]); return 2
        s3 = os.path.join(out, "fam_" + suf + ".s")
        if os.path.isfile(s3) and not args.keep:
            os.rename(s3, os.path.join(out, "keep_" + suf + ".s"))  # build 副产 .s 移出防覆盖判断
        o = subprocess.run([exe], capture_output=True, text=True, timeout=15).stdout
        open(os.path.join(out, "rt_" + suf + ".out"), "w").write(o)
        outs[lvl] = parse(o)

    C = parse(c_out) if c_out else {}
    O0, O3 = outs["-O0"], outs["-O3"]
    mode = "双 oracle" if c_out else "单 oracle(C 降级)"
    print("模式=%s | 条数 C=%d O0=%d O3=%d" % (mode, len(C), len(O0), len(O3)))

    hits = []
    for tag, _, _, exp, cfmt, g_ in CASES:
        cv, v0, v3 = C.get(tag), O0.get(tag), O3.get(tag)
        if c_out and cv is None or v0 is None or v3 is None:
            hits.append([tag, g_, "缺输出", str(cv), str(v0), str(v3), exp]); continue
        flags = []
        if c_out and cv != exp: flags.append("oracle分叉(C与python)")
        if v0 != exp: flags.append("O0错")
        if v3 != exp: flags.append("O3错")
        if flags:
            hits.append([tag, g_, "+".join(flags), str(cv), str(v0), str(v3), exp])

    print("\n===== 命中清单（共 %d 用例·%s）=====" % (len(CASES), mode))
    for h in hits:
        print("  [%s][%s] %s | CN-O0=%s CN-O3=%s C=%s 期望=%s" % (h[0], h[1], h[2], h[4], h[5], h[3], h[6]))
    print("\n命中 %d / %d" % (len(hits), len(CASES)))
    json.dump(hits, open(os.path.join(out, "fam_hits.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print("产物：%s（rt_*.out 原始输出+fam_hits.json%s）" % (out, "+asm" if args.keep else ""))
    return 0

if __name__ == "__main__":
    sys.exit(main())
