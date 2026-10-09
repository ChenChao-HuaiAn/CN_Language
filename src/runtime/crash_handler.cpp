// CN运行时实现：崩溃处理器（718·cn_self 第二跳侦查设施·Windows 专有）
// UEF 抓 C0000005 等未处理异常，stderr 直写 code/RIP/RSP/fault（WriteFile 不经
//   stdio 锁）。链接本 obj 的编译器进程经静态初始化自动安装（宿主 cn.exe 无害）。
// 349 重构E 自 io_api.cpp 纯机械搬移·零逻辑变化：原 1019-1131 行（崩溃处理器节）——
//   本节零跨节 static 引用；全部设施位于 _WIN32 门内，非 Windows 平台本 TU 刻意
//   保持空（仅占位别名，不产生任何代码/符号）。
#include "runtime/runtime.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _WIN32
#include <malloc.h>  // _heapmin（堆压缩，归还空闲页）
#include <windows.h>  // ExitProcess/SetUnhandledExceptionFilter（718 崩溃处理器）
#endif
#ifdef _WIN32
// 719：UEF 显式安装入口（runtime.cpp entry() 调用——cn_self 入口 shim 不跑
//   _initterm，auto_install 静态初始化器不执行，必须显式装）
static LONG WINAPI cn_crash_filter(EXCEPTION_POINTERS*);
extern "C" void cn_install_crash_handler() {
    SetUnhandledExceptionFilter(cn_crash_filter);
}
static LONG WINAPI cn_crash_filter(EXCEPTION_POINTERS* info) {
    if (info && info->ExceptionRecord) {
        char buf[256];
        void* fault = (info->ExceptionRecord->NumberParameters >= 2)
                          ? (void*)info->ExceptionRecord->ExceptionInformation[1]
                          : nullptr;
        int n = std::snprintf(buf, sizeof(buf),
                              "[crash] code=%08X addr=%p RIP=%p RSP=%p fault=%p\n",
                              (unsigned)info->ExceptionRecord->ExceptionCode,
                              (void*)info->ExceptionRecord->ExceptionAddress,
                              (void*)info->ContextRecord->Rip,
                              (void*)info->ContextRecord->Rsp, fault);
        if (n > 0) { DWORD written; WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, (DWORD)n, &written, nullptr); }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
// 719b：VEH 版（第一顺位）——cn_self 的 C0000005 现场 UEF 未触发（原因待查），
//   VEH 挂异常分发链头必经。打印 code/RIP/RSP/fault 后 CONTINUE_SEARCH。
static LONG WINAPI cn_veh_filter(EXCEPTION_POINTERS* info) {
    if (info && info->ExceptionRecord) {
        char buf[256];
        void* fault = (info->ExceptionRecord->NumberParameters >= 2)
                          ? (void*)info->ExceptionRecord->ExceptionInformation[1]
                          : nullptr;
        // 721：ASLR 下绝对 RIP 不可对位——打印 RVA（RIP-模块基址）
        HMODULE mod = nullptr;
        uintptr_t base = 0;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)info->ExceptionRecord->ExceptionAddress, &mod);
        if (mod) { base = (uintptr_t)mod; }
        // 735：实参寄存器并入（追踪入参 NULL 产出方向——rcx=this/rdx=值形参…）
        int n = std::snprintf(buf, sizeof(buf),
                              "[veh] code=%08X RIP=%p RSP=%p RBP=%p fault=%p base=%p RVA=%llx RAX=%p RCX=%p RDX=%p R8=%p R9=%p\n",
                              (unsigned)info->ExceptionRecord->ExceptionCode,
                              (void*)info->ExceptionRecord->ExceptionAddress,
                              (void*)info->ContextRecord->Rsp,
                              (void*)info->ContextRecord->Rbp, fault, (void*)base,
                              (unsigned long long)(info->ContextRecord->Rip - base),
                              (void*)info->ContextRecord->Rax, (void*)info->ContextRecord->Rcx,
                              (void*)info->ContextRecord->Rdx, (void*)info->ContextRecord->R8,
                              (void*)info->ContextRecord->R9);
        if (n > 0) { DWORD written; WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, (DWORD)n, &written, nullptr); }
        // 990（171 深水·栈顶转储）：RSP 起 16 个四字——崩点 rdi/rsi=-1 且
        //   返回地址可能被写坏跳入随机符号时 rbp 链不可靠；栈顶原始数据供
        //   对位 map 手工解析真调用链。
        {
            uintptr_t rsp = info->ContextRecord ? info->ContextRecord->Rsp : 0;
            HMODULE mod3 = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)(info->ContextRecord ? info->ContextRecord->Rip : 0), &mod3);
            uintptr_t base3 = mod3 ? (uintptr_t)mod3 : 0;
            for (int q = 0; q < 16 && rsp; q++) {
                uintptr_t v = 0;
                SIZE_T rd = 0;
                if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(rsp + q * 8), &v, 8, &rd) || rd != 8) break;
                char b3[96];
                int n3 = std::snprintf(b3, sizeof(b3), "[veh] stk%02d %p%s", q, (void*)v,
                                       (v > base3 && v < base3 + 0x400000) ? " [mod]" : "");
                if (n3 > 0) { DWORD w; WriteFile(GetStdHandle(STD_ERROR_HANDLE), b3, (DWORD)n3, &w, nullptr); }
            }
        }
    }
    // 721：rbp 链回溯（CN 生成代码 push rbp/mov rbp,rsp 帧链）——打印各层返回地址 RVA
    {
        uintptr_t rbp = info->ContextRecord ? info->ContextRecord->Rbp : 0;
        HMODULE mod2 = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)(info->ContextRecord ? info->ContextRecord->Rip : 0), &mod2);
        uintptr_t base2 = mod2 ? (uintptr_t)mod2 : 0;
        for (int depth = 0; depth < 16 && rbp; depth++) {
            uintptr_t ret = 0, next = 0;
            SIZE_T rd = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(rbp + 8), &ret, 8, &rd) || rd != 8) break;
            ReadProcessMemory(GetCurrentProcess(), (LPCVOID)rbp, &next, 8, &rd);
            char b2[128];
            int n2 = std::snprintf(b2, sizeof(b2), "[veh] frame%02d ret RVA=%llx\n", depth,
                                   (unsigned long long)(ret > base2 ? ret - base2 : ret));
            if (n2 > 0) { DWORD w; WriteFile(GetStdHandle(STD_ERROR_HANDLE), b2, (DWORD)n2, &w, nullptr); }
            if (next <= rbp) break;
            rbp = next;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
extern "C" void cn_install_veh() {
    AddVectoredExceptionHandler(1, cn_veh_filter);
}
namespace {
// 718~735 自举侦查链设施：静态安装 UEF（崩溃时走 cn_crash_filter 输出诊断）。
//   866 清理（0928 审计第 10 条·用户裁决机制级当场修）：撤销启动横幅
//   「[crash] UEF installed」——侦查链已收尾，每程序启动必打的残留污染用户 stderr；
//   安装与崩溃诊断路径保留（诊断契约见 plans/001 §10.2a）。
struct cn_crash_auto_install {
    cn_crash_auto_install() {
        SetUnhandledExceptionFilter(cn_crash_filter);
    }
};
static const cn_crash_auto_install cn_crash_auto_install_instance;
}
#else
// 非 Windows 平台：SEH/VEH 为 Windows 专有设施，本 TU 刻意保持空——
//   以下占位别名仅满足编译器对非空翻译单元的保守要求，不产生任何代码/符号。
using cn_crash_handler_placeholder_t = int;
#endif
