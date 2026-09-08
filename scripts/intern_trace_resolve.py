#!/usr/bin/env python3
# 灰色点取证：驻留轨迹 ra 反解（nm 排序表二分 -> CN 函数名规范化）
# 对比口径：仅函数名主干（剥 L_/bb 标签；hex 段解码为 UTF-8 中文）。
#   ra 常落局部标签（bb0/objdel_N_M 等）内——向上回退最近的函数符号。
# 用法: resolve_ra.py <二进制> <trace.log> <out.log>
import subprocess, sys, bisect, re

def is_local_label(name):
    # 宿主 bb 标签 / v2 局部标签（小写开头、非 CN mangling）
    if re.match(r"^(bb\d+|objdel_\d+_\d+|objinit|loc_[0-9a-f]+)$", name):
        return True
    return False

def is_func_symbol(name):
    # CN mangling：剥 L_ 后以大写 hex 段开头（E 开头 UTF-8 首字节）或保留的
    #   运行时 C 符号（__cn_* / main 等）
    return bool(re.match(r"^(L_)?_?[0-9A-F]{8}", name)) or name.startswith("__cn_") or name == "main"

def decode_cn_name(name):
    name = re.sub(r"^(L_)+", "", name)
    def repl(m):
        h = m.group(0)
        if len(h) % 2:
            return h
        try:
            s = bytes.fromhex(h).decode("utf-8")
        except Exception:
            return h
        if any(ord(c) > 127 for c in s):
            return s
        return h
    name = re.sub(r"[0-9A-F]{8,}", repl, name)
    name = re.sub(r"_?bb\d+$", "", name)
    return name

def load_symbols(binary):
    out = subprocess.run(["nm", "--defined-only", binary],
                         capture_output=True, text=True).stdout
    syms = []           # (addr, name, is_func)
    for line in out.splitlines():
        parts = line.split(" ", 2)
        if len(parts) < 3:
            continue
        addr_str, typ, name = parts
        try:
            addr = int(addr_str, 16)
        except ValueError:
            continue
        syms.append((addr, name, is_func_symbol(name) and not is_local_label(name)))
    syms.sort()
    return syms

def resolve(syms, addr):
    addrs = [s[0] for s in syms]
    i = bisect.bisect_right(addrs, addr) - 1
    if i < 0:
        return "?"
    if not syms[i][2]:
        # 局部标签：向前回退最近函数符号
        j = i
        while j >= 0 and not syms[j][2]:
            j -= 1
        if j >= 0:
            i = j
    return decode_cn_name(syms[i][1])

def main():
    binary, trace, outp = sys.argv[1], sys.argv[2], sys.argv[3]
    syms = load_symbols(binary)
    with open(trace, encoding="utf-8") as f, open(outp, "w", encoding="utf-8") as o:
        for line in f:
            m = re.match(r"^\[intern\] (\d+) (.*) ra=(0x[0-9a-f]+)", line.rstrip("\n"))
            if m:
                iid, text, ra = m.group(1), m.group(2), int(m.group(3), 16)
                o.write(f"[intern] {iid} {text} @ {resolve(syms, ra)}\n")
            else:
                o.write(line.rstrip("\n") + "\n")

if __name__ == "__main__":
    main()
