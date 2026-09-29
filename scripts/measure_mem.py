"""074 侦查③：进程峰值工作集精确采样（Windows PROCESS_MEMORY_COUNTERS.PeakWorkingSetSize）。

用法: python scripts/measure_mem.py <标签> <工作目录> <exe> <args...>
输出: [标签] TIME_S=.. PEAK_MB=.. (轮询峰值 ..) EXIT=..
口径与 851/872 一致（WorkingSet64 峰值·MB）。
"""
import ctypes
import subprocess
import sys
import time
from ctypes import wintypes

PROCESS_QUERY_LIMITED_INFORMATION = 0x1000


class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
    _fields_ = [
        ("cb", wintypes.DWORD),
        ("PageFaultCount", wintypes.DWORD),
        ("PeakWorkingSetSize", ctypes.c_size_t),
        ("WorkingSetSize", ctypes.c_size_t),
        ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
        ("QuotaPagedPoolUsage", ctypes.c_size_t),
        ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
        ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
        ("PagefileUsage", ctypes.c_size_t),
        ("PeakPagefileUsage", ctypes.c_size_t),
    ]


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi", use_last_error=True)
kernel32.OpenProcess.restype = wintypes.HANDLE
psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(PROCESS_MEMORY_COUNTERS), wintypes.DWORD]
psapi.GetProcessMemoryInfo.restype = wintypes.BOOL


def 采样(句柄):
    c = PROCESS_MEMORY_COUNTERS()
    c.cb = ctypes.sizeof(c)
    if not psapi.GetProcessMemoryInfo(句柄, ctypes.byref(c), c.cb):
        return None, None
    return c.WorkingSetSize, c.PeakWorkingSetSize


def main():
    标签, 工作目录, exe, *参数 = sys.argv[1:]
    日志 = rf"{工作目录}\{标签}.log"
    with open(日志, "wb") as out, open(日志 + ".err", "wb") as err:
        开始 = time.time()
        p = subprocess.Popen([exe, *参数], cwd=工作目录, stdout=out, stderr=err)
        句柄 = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, p.pid)
        轮询峰值 = 0
        if not 句柄:
            print(f"[{标签}] OpenProcess 失败（错误 {ctypes.get_last_error()}）")
        while p.poll() is None:
            ws, _ = 采样(句柄)
            if ws and ws > 轮询峰值:
                轮询峰值 = ws
            time.sleep(0.1)
        _, 系统峰值 = 采样(句柄)
        if 句柄:
            kernel32.CloseHandle(句柄)
        try:
            p.wait(timeout=10)
        except subprocess.TimeoutExpired:
            pass
        耗时 = time.time() - 开始
        峰值 = max(轮询峰值, 系统峰值 or 0)
        print(f"[{标签}] TIME_S={耗时:.1f} PEAK_MB={峰值 / 1048576:.1f} "
              f"(轮询 {轮询峰值 / 1048576:.1f}) EXIT={p.returncode}")


if __name__ == "__main__":
    main()
