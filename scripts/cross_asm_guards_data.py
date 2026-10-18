#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""887 交叉汇编守卫序列对照门禁——数据面（探针源码 + 语义期望表 + 映射表）。

逻辑面见 check_cross_asm_guards.py（本模块只放数据/表·单一职责拆分使
主脚本 ≤1000 行红线）。数据改动（探针/期望/映射）等效于门禁语义改动——
须同步自检正反例样本与 001 条文（887）。
"""

# ============ 内嵌探针源码 ============
# 设计要点：①每个守卫一个「安全函数」（不安全 函数内不插检查）；②函数参数
#   形态防常量折叠（O0/O2 均保留守卫）；③驱动段用 不安全 主 免除杂散守卫；
#   ④「读值」触发 FieldAddr+PtrLoad 空指针守卫（错误码 3）；⑤盒构造与主调用
#   点附带 3 个判零守卫（构造器字段写 2 + 调用点 1）——共 18 守卫期望。
#   908 补面：原探针缺「正64（u64）安全乘法」——887 u32 修复同族缺口未站岗
#   （linux_x64 u64 乘法重发段「双乘」缺陷由此潜伏：折叠形态 v2p 崩溃实锤）。
#   诚实边界：本门禁对照**守卫谓词**（条件码级）；乘法「输入数据流」面
#   （重发装载取错寄存器等）无判据——由 E2E 值断言用例承担（908 新用例）。
探针源码 = """函数 窄加8(整8 甲, 整8 乙) -> 整8 { 返回 甲 + 乙; }
函数 窄减8(整8 甲, 整8 乙) -> 整8 { 返回 甲 - 乙; }
函数 窄加16(整16 甲, 整16 乙) -> 整16 { 返回 甲 + 乙; }
函数 正加8(正8 甲, 正8 乙) -> 正8 { 返回 甲 + 乙; }
函数 正减8(正8 甲, 正8 乙) -> 正8 { 返回 甲 - 乙; }
函数 宽加32(整32 甲, 整32 乙) -> 整32 { 返回 甲 + 乙; }
函数 宽减64(整64 甲, 整64 乙) -> 整64 { 返回 甲 - 乙; }
函数 乘32(整32 甲, 整32 乙) -> 整32 { 返回 甲 * 乙; }
函数 乘32正(正32 甲, 正32 乙) -> 正32 { 返回 甲 * 乙; }
函数 乘64(整64 甲, 整64 乙) -> 整64 { 返回 甲 * 乙; }
函数 乘64正(正64 甲, 正64 乙) -> 正64 { 返回 甲 * 乙; }
函数 位移32(整32 甲, 整32 乙) -> 整32 { 返回 甲 << 乙; }
函数 位移64(整64 甲, 整64 乙) -> 整64 { 返回 甲 << 乙; }
类 盒 {
公开:
    整32 值
    函数 盒(整32 v) { 值 = v; }
}
函数 读值(盒 甲) -> 整32 { 返回 甲.值; }
不安全 函数 主() -> 整32 {
    打印("1=", 整32(窄加8(100, 27)));
    打印("2=", 整32(窄减8(100, 27)));
    打印("3=", 整32(窄加16(100, 27)));
    打印("4=", 整32(正加8(100, 27)));
    打印("5=", 整32(正减8(100, 27)));
    打印("6=", 宽加32(100, 27));
    打印("7=", 宽减64(100, 27));
    打印("8=", 乘32(100, 27));
    打印("9=", 整32(乘32正(100, 27)));
    打印("10=", 乘64(100, 27));
    打印("10b=", 整64(乘64正(100, 27)));
    打印("11=", 位移32(100, 3));
    打印("12=", 位移64(100, 3));
    盒 b = 盒(7);
    打印("13=", 读值(b));
    返回 0;
}
"""

# ============ 期望表（语言语义·001 §3.7a/§1.1a） ============
# (类别名, 错误码, 判定函数 x -> 该采样点是否应进入错误块, 裁剪域 or None)
# 裁剪域：产物以 32 位窗口（cmp wN/cmp eax）承载的窄域/移位守卫——判定对象
#   的可达数学域在该窗口内（窄和/差/积 |x| < 2^31；移位量按 32 位承载现状），
#   采样点超出窗口的值会被产物截断而与数学期望偏离——按可达域裁剪采样
#   （机器窗口内无截断，等价性成立）。标志/恒等式类原子以数学值直接判定，
#   不裁剪（对任意数学值语义自洽）。移位量超 32 位面不在本门禁覆盖（文件头声明）。
_域32 = (-(2 ** 31), 2 ** 31 - 1)
期望类别 = [
    ("有符号8溢出", 8, lambda x: not (-128 <= x <= 127), _域32),
    ("有符号16溢出", 8, lambda x: not (-32768 <= x <= 32767), _域32),
    ("无符号8溢出", 8, lambda x: not (0 <= x <= 255), _域32),
    ("有符号32溢出", 8, lambda x: not (-(2 ** 31) <= x <= 2 ** 31 - 1), None),
    ("无符号32溢出", 8, lambda x: not (0 <= x <= 2 ** 32 - 1), None),
    ("有符号64溢出", 8, lambda x: not (-(2 ** 63) <= x <= 2 ** 63 - 1), None),
    ("无符号64溢出", 8, lambda x: not (0 <= x <= 2 ** 64 - 1), None),
    ("移位量域32", 8, lambda k: not (0 <= k <= 31), _域32),
    ("移位量域64", 8, lambda k: not (0 <= k <= 63), _域32),
    ("判零(空指针)", 3, lambda p: p == 0, None),
]
期望计数 = {
    "有符号8溢出": 2, "有符号16溢出": 1, "无符号8溢出": 2,
    "有符号32溢出": 2, "无符号32溢出": 1, "有符号64溢出": 2,
    "无符号64溢出": 1,
    "移位量域32": 1, "移位量域64": 1, "判零(空指针)": 5,
}


_边界 = [-1, 0, 1, 31, 32, 63, 64, 127, 128, 255, 256,
         -129, -128, -32768, -32769, 32766, 32767, 32768,
         2 ** 31 - 1, 2 ** 31, -(2 ** 31) - 1, -(2 ** 31),
         2 ** 32 - 1, 2 ** 32, 2 ** 32 + 1,
         2 ** 63 - 1, 2 ** 63, -(2 ** 63) - 1, -(2 ** 63), -(2 ** 63) + 1]
采样点 = sorted({b + d for b in _边界 for d in (-2, -1, 0, 1, 2)})

# ============ 条件码映射表 ============
比较码 = {  # 条件成立时的常量比较语义（cmp 前驱）
    "jg": ("s", ">"), "jge": ("s", ">="), "jl": ("s", "<"), "jle": ("s", "<="),
    "ja": ("u", ">"), "jae": ("u", ">="), "jb": ("u", "<"), "jbe": ("u", "<="),
    "b.gt": ("s", ">"), "b.ge": ("s", ">="), "b.lt": ("s", "<"), "b.le": ("s", "<="),
    "b.hi": ("u", ">"), "b.hs": ("u", ">="), "b.lo": ("u", "<"), "b.ls": ("u", "<="),
}
相等码 = {"je", "jz", "b.eq"}      # 条件成立 = 操作数 == 0
不等码 = {"jne", "jnz", "b.ne"}    # 条件成立 = 操作数 != 0
# 标志类条件码（运算前驱）：(标志, 条件成立时标志的期望值)
标志码 = {
    "jno": ("of", False), "jo": ("of", True),
    "jnc": ("cf", False), "jc": ("cf", True),
    "jb": ("cf", True), "jae": ("cf", False),  # 运算前驱时 jb==jc / jae==jnc
    "b.vc": ("of", False), "b.vs": ("of", True),
    "b.cc": ("cf", False), "b.cs": ("cf", True),
    "b.lo": ("cf", False), "b.hs": ("cf", True),  # arm64 运算前驱时 lo==cc / hs==cs
}
条件跳转集 = (set(比较码) | 相等码 | 不等码 | set(标志码) |
              {"cbz", "cbnz"})
运算类 = {"adds", "subs", "add", "sub", "imul", "mul",
          "smull", "umull", "smulh", "umulh", "shr", "sar", "shl", "neg"}
标志指令集 = 运算类 | {"cmp", "cmn", "test", "tst", "cbz", "cbnz"}




# ============ 自检样本表（908 迁入·先拆再加：check 主脚本近 900 警戒线解压） ============
# (名称, 汇编文本, 类别名, 应通过)：check_cross_asm_guards.自检() 消费（探测器自测）。
#   正样本=三平台正确守卫形态；负样本（应通过=False）=形态回归/笔误回放（须判红）。
自检样本 = [
    ("arm64 窄加8", """
    cmp w9, #127
    b.gt Lovf1
    cmn w9, #128
    b.ge Lovf_ok1
Lovf1:
    movz x0, #8
    bl __cn_runtime_error
Lovf_ok1:
""", "有符号8溢出", True),
    ("win 窄加8", """
    add eax, ecx
    cmp eax, 127
    jg @ovf0
    cmp eax, -128
    jge @ovf_ok0
@ovf0:
    mov rcx, 8
    call __cn_runtime_error
@ovf_ok0:
""", "有符号8溢出", True),
    ("lx64 窄加8（8 位标志）", """
    add r10b, r9b
    jno Lovf0_ok
Lovf0:
    mov rdi, 8
    call __cn_runtime_error
Lovf0_ok:
""", "有符号8溢出", True),
    ("lx64 无符号加（jnc）", """
    add r10b, r9b
    jnc Lovf3_ok
Lovf3:
    mov rdi, 8
    call __cn_runtime_error
Lovf3_ok:
""", "无符号8溢出", True),
    ("win 无符号加（单界）", """
    add eax, ecx
    cmp eax, 255
    jbe @ovf_ok3
    mov rcx, 8
    call __cn_runtime_error
@ovf_ok3:
""", "无符号8溢出", True),
    ("win 32 位加（标志）", """
    add eax, ecx
    jno @ovf_ok5
@ovf5:
    mov rcx, 8
    call __cn_runtime_error
@ovf_ok5:
""", "有符号32溢出", True),
    ("arm64 i32 乘法恒等式", """
    smull x12, w9, w11
    cmp x12, w12, sxtw
    b.eq Lovf10_ok
Lovf10:
    movz x0, #8
    bl __cn_runtime_error
Lovf10_ok:
""", "有符号32溢出", True),
    ("arm64 i64 乘法恒等式", """
    smulh x12, x10, x11
    mul x10, x10, x11
    cmp x12, x10, asr #63
    b.eq Lovf12_ok
Lovf12:
    movz x0, #8
    bl __cn_runtime_error
Lovf12_ok:
""", "有符号64溢出", True),
    ("win u32 乘法（shr+jz 修复形态）", """
    imul rax, rcx
    mov r11, rax
    shr r11, 32
    jz @ovf0_ok
@ovf0:
    mov rcx, 8
    call __cn_runtime_error
@ovf_ok0:
""", "无符号32溢出", True),
    ("lx64 u64 乘法（mul+jnc）", """
    imul r10, r9
    mov rax, qword ptr [rbp-8]
    mov r9, qword ptr [rbp-16]
    mul r9
    jnc Lovf20_ok
Lovf20:
    mov rdi, 8
    call __cn_runtime_error
Lovf20_ok:
    mov r10, rax
""", "无符号64溢出", True),
    ("win u64 乘法（mul rcx+jnc）", """
    imul rax, rcx
    mov rax, qword ptr [rbp-8]
    mov rcx, qword ptr [rbp-16]
    mul rcx
    jnc @ovf20_ok
@ovf20:
    mov rcx, 8
    call __cn_runtime_error
@ovf20_ok:
""", "无符号64溢出", True),
    ("arm64 u64 乘法（umulh+cbz）", """
    umulh x12, x10, x11
    cbz x12, Lovf20_ok
Lovf20:
    movz x0, #8
    bl __cn_runtime_error
Lovf20_ok:
""", "无符号64溢出", True),
    ("判零（test/jne）", """
    test rax, rax
    jne @field_ok2
    mov rcx, 3
    call __cn_runtime_error
@field_ok2:
""", "判零(空指针)", True),
    ("判零（cmp#0/b.ne）", """
    cmp x9, #0
    b.ne Lfield_ok15
    movz x0, #3
    bl __cn_runtime_error
Lfield_ok15:
""", "判零(空指针)", True),
    ("移位域（cmp/jb）", """
    cmp ecx, 32
    jb @ovf2_ok
@ovf2:
    mov rcx, 8
    call __cn_runtime_error
@ovf2_ok:
""", "移位量域32", True),
    ("835 反转（b.le 序）", """
    cmp w9, #127
    b.le Lovf_ok1
    b Lovf1
    cmn w9, #128
    b.ge Lovf_ok1
Lovf1:
    movz x0, #8
    bl __cn_runtime_error
Lovf_ok1:
""", "有符号8溢出", False),
    ("判零反转（je）", """
    test rax, rax
    je @field_ok2
    mov rcx, 3
    call __cn_runtime_error
@field_ok2:
""", "判零(空指针)", False),
    ("移位界笔误（32→31）", """
    cmp ecx, 31
    jb @ovf2_ok
@ovf2:
    mov rcx, 8
    call __cn_runtime_error
@ovf2_ok:
""", "移位量域32", False),
    ("u32 双落错误块（修复前形态回放）", """
    imul rax, rcx
    mov r11, rax
    shr r11, 32
    jz @ovf0
@ovf0:
    mov rcx, 8
    call __cn_runtime_error
@ovf_ok0:
""", "无符号32溢出", False),
    ("u64 乘法守卫极性反转（jnc→jc）", """
    mov rax, qword ptr [rbp-8]
    mov r9, qword ptr [rbp-16]
    mul r9
    jc Lovf20_ok
Lovf20:
    mov rdi, 8
    call __cn_runtime_error
Lovf20_ok:
""", "无符号64溢出", False),
    ("标志家族混用（jno→jnc）", """
    add eax, ecx
    jnc @ovf1_ok
@ovf1:
    mov rcx, 8
    call __cn_runtime_error
@ovf1_ok:
""", "有符号32溢出", False),
    ("arm64 边界笔误（127→126）", """
    cmp w9, #126
    b.gt Lovf1
    cmn w9, #128
    b.ge Lovf_ok1
Lovf1:
    movz x0, #8
    bl __cn_runtime_error
Lovf_ok1:
""", "有符号8溢出", False),
]
