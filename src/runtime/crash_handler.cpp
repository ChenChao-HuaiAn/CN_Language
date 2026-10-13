// CN运行时实现：崩溃处理器 + 栈溢出防护
// 718·cn_self 第二跳侦查设施（Windows 专有）：UEF/VEH 抓 C0000005 等未处理异常，
//   stderr 直写 code/RIP/RSP/fault（WriteFile 不经 stdio 锁）。链接本 obj 的
//   编译器进程经静态初始化自动安装（宿主 cn.exe 无害）。
// 349 重构E 自 io_api.cpp 纯机械搬移·零逻辑变化：原 1019-1131 行（崩溃处理器节）——
//   本节零跨节 static 引用。
// 476：栈溢出防护（三平台·§1.1a② 确定性失败）——见下方「栈溢出防护」节；
//   Windows 侧拦截并入既有 UEF/VEH 过滤器，Linux 侧新增 SIGSEGV 处理器。
#include "runtime/runtime.hpp"

#include <atomic>
#include <cstddef>
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
#else
// 476：Linux 栈溢出防护依赖（signal/sigaction/raise、sigaltstack、ucontext SP、
//   write/_exit 直写；pthread key 供子线程备用栈生命周期）
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#endif

// ==================== 476：栈溢出防护（guard page 被动捕获·错误码 9） ====================
// 背景（任务 476·plans/001 §1.1a②）：非尾递归深度递归耗尽线程栈 → 传统段错误
//   rc=139 脏崩（违反确定性失败立法）；本设施 Rust 式被动捕获——零热路径开销
//   （仅入口安装动作一次性），栈耗尽时输出「运行时错误(错误码9): 栈溢出」
//   确定性退出 rc=1（终态对齐错误码 8 契约：stdout 打印后终止）。
// 平台分派：linux=SIGSEGV+sigaltstack+SA_ONSTACK（溢出信号必须在备用栈上处理）；
//   win=SEH EXCEPTION_STACK_OVERFLOW（专用异常码=精确判据，无地址判定面）。
// 安装点=运行时入口 entry()（runtime.cpp）——宿主与 v2 发射的程序共用同一运行时
//   （v2asm 链接宿主 libcn_runtime），双侧零分叉天然成立。
// 判定防误报（linux）：SIGSEGV 亦可能来自普通空指针/野指针访问——仅当
//   ① si_code ∈ {SEGV_MAPERR, SEGV_ACCERR}（地址类故障）
//   ② 故障地址不高于故障时 SP 一页以上（栈访问必在 SP 附近——压栈/帧内访问）
//   ③ 故障地址不低于 SP 64MiB 以上（覆盖巨型栈帧；空指针/野指针与 SP 相距
//      地址空间级被本条件排除）
//   三条全满足才判栈溢出；否则恢复默认处理（普通段错误保持原行为 rc=139）。
//   理由：栈映射内访问不产生 SIGSEGV（内核自动扩展），真溢出必发生在 SP
//   附近紧贴栈底的未映射/保护页——②③ 恰刻画该几何；win 侧
//   EXCEPTION_STACK_OVERFLOW 为专用异常码，无此判定需求。

// 栈溢出消息（错误码 9；文案锚定 io_api.cpp __cn_error_message(9) 消息表）
static const char CN_STACK_OVERFLOW_MSG[] = "运行时错误(错误码9): 栈溢出\n";

// 栈溢出退出（Linux 处理器/Windows 过滤器共用）：先尽力 flush stdout
//   （glibc/UCRT FILE 锁为递归锁·同线程重入不死锁；保住溢出前用户已缓冲的
//   输出），错误行直写标准输出（不经 stdio 缓冲——随后立即 _exit 不 flush；
//   通道对齐附录B 契约行=stdout），确定性退出码 1。
extern "C" void cn_stack_overflow_exit() {
    (void)std::fflush(stdout);
#ifdef _WIN32
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), CN_STACK_OVERFLOW_MSG,
              (DWORD)(sizeof(CN_STACK_OVERFLOW_MSG) - 1), &written, nullptr);
    ExitProcess(1);
#else
    const long long n = static_cast<long long>(
        ::write(1, CN_STACK_OVERFLOW_MSG, sizeof(CN_STACK_OVERFLOW_MSG) - 1));
    (void)n;
    ::_exit(1);
#endif
}

// 栈溢出判定纯函数（linux 判据·供 SIGSEGV 处理器与单元测试复用）
// 参数：fault=故障地址（si_addr）、sp=故障时栈指针（ucontext）、sig_code=si_code
// 返回：1=判为栈溢出；0=非栈溢出（保持默认崩溃处理）
extern "C" int __cn_stack_overflow_match(unsigned long long fault,
                                         unsigned long long sp, long long sig_code) {
#ifdef _WIN32
    // Windows 判据走 SEH 异常码（EXCEPTION_STACK_OVERFLOW），本函数仅保全平台
    // 符号面（单测/链接一致性）——语义常量与 Linux 一致：MAPERR=1/ACCERR=2
    const long long kSegvMaperr = 1;
    const long long kSegvAccerr = 2;
#else
    const long long kSegvMaperr = SEGV_MAPERR;
    const long long kSegvAccerr = SEGV_ACCERR;
#endif
    if (sig_code != kSegvMaperr && sig_code != kSegvAccerr) return 0;  // ① 非地址类故障
    if (sp == 0) return 0;                                             // SP 无效不判定
    if (fault > sp + 4096ULL) return 0;                                // ② 高于 SP 一页以上
    if (fault + (64ULL << 20) < sp) return 0;                          // ③ 低于 SP 64MiB 以上
    return 1;
}

#ifdef _WIN32
// 719：UEF 显式安装入口（runtime.cpp entry() 调用——cn_self 入口 shim 不跑
//   _initterm，auto_install 静态初始化器不执行，必须显式装）
static LONG WINAPI cn_crash_filter(EXCEPTION_POINTERS*);
extern "C" void cn_install_crash_handler() {
    SetUnhandledExceptionFilter(cn_crash_filter);
}
// 476：Windows 栈溢出防护安装（win 无需独立动作：栈溢出在下方 UEF/VEH 过滤器
//   按 EXCEPTION_STACK_OVERFLOW 异常码拦截；本入口供 entry() 全平台统一调用）
extern "C" void cn_install_stack_guard() {}
static LONG WINAPI cn_crash_filter(EXCEPTION_POINTERS* info) {
    // 476：栈溢出=错误码 9 确定性失败（专用异常码精确判定，无地址误报面）
    if (info && info->ExceptionRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        cn_stack_overflow_exit();  // 不返回
    }
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
    // 476：栈溢出=错误码 9 确定性失败——须先于一切诊断路径（VEH 挂链头必经，
//   比 UEF 更早看到 EXCEPTION_STACK_OVERFLOW）；x64 内核为栈溢出异常提供
//   受控备用栈，处理器内 WriteFile/ExitProcess 在受控栈上执行安全。
    if (info && info->ExceptionRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        cn_stack_overflow_exit();  // 不返回
    }
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
// ==================== 476：Linux 栈溢出防护（SIGSEGV + sigaltstack） ====================
// 非 Windows 平台：SEH/VEH 为 Windows 专有设施不适用；以下为 Linux 侧实现
//   （718/719/721/735 等 win 侦查设施在 linux 无对应物，不移植）。

namespace {

// 主线程备用栈（sigaltstack）：溢出信号处理器不可在已耗尽的线程栈上运行。
//   64KiB 静态 BSS（装载不占文件/物理内存；handler 栈用量 <2KiB 余量充足）。
constexpr std::size_t CN_SIG_ALTSTACK_SIZE = 64 * 1024;
unsigned char cn_sig_altstack[CN_SIG_ALTSTACK_SIZE];

// 故障时栈指针（架构分派：x86_64=REG_RSP / arm64=uc_mcontext.sp）
std::uintptr_t cn_fault_sp(void* uctx) {
#if defined(__x86_64__)
    return static_cast<std::uintptr_t>(((ucontext_t*)uctx)->uc_mcontext.gregs[REG_RSP]);
#elif defined(__aarch64__)
    return static_cast<std::uintptr_t>(((ucontext_t*)uctx)->uc_mcontext.sp);
#else
    (void)uctx;
    return 0;
#endif
}

}  // namespace

// SIGSEGV 处理器（SA_ONSTACK：在备用栈上运行；sigaction/signal/raise 均
//   async-signal-safe，handler 内不分配大栈帧）。判据见节首；非栈溢出恢复
//   默认处理（普通段错误保持原行为 rc=139：signal 还原 SIG_DFL 后 raise 重发
//   ——原信号屏蔽至处理器返回时按默认动作处理，异步段错误同样可靠终止）。
static void cn_sigsegv_handler(int, siginfo_t* info, void* uctx) {
    const unsigned long long fault = static_cast<unsigned long long>(
        reinterpret_cast<std::uintptr_t>(info != nullptr ? info->si_addr : nullptr));
    const unsigned long long sp =
        uctx != nullptr ? static_cast<unsigned long long>(cn_fault_sp(uctx)) : 0ULL;
    const long long sigCode = info != nullptr ? info->si_code : 0;
    if (__cn_stack_overflow_match(fault, sp, sigCode) != 0) {
        cn_stack_overflow_exit();  // 不返回
    }
    (void)::signal(SIGSEGV, SIG_DFL);
    (void)::raise(SIGSEGV);
}

// 子线程备用栈安装（thread_api.cpp 线程入口调用·每线程独立）：子线程无备用
//   栈则溢出时处理器在耗尽栈上运行 → 二次故障被 SIGKILL（脏崩无输出）。
//   pthread key 析构保证线程退出时释放（无泄漏）。
static pthread_key_t cn_altstack_key;
static pthread_once_t cn_altstack_once = PTHREAD_ONCE_INIT;
static int cn_altstack_key_ok = 0;

static void cn_altstack_free(void* p) { std::free(p); }
static void cn_altstack_init() {
    cn_altstack_key_ok = (pthread_key_create(&cn_altstack_key, cn_altstack_free) == 0) ? 1 : 0;
}

extern "C" void cn_install_thread_stack_guard() {
    (void)pthread_once(&cn_altstack_once, cn_altstack_init);
    if (cn_altstack_key_ok == 0) return;  // 降级：key 不可用则不装（保持默认崩溃）
    void* mem = std::malloc(CN_SIG_ALTSTACK_SIZE);
    if (mem == nullptr) return;  // 降级：不装（溢出保持默认崩溃行为）
    if (pthread_setspecific(cn_altstack_key, mem) != 0) {
        std::free(mem);
        return;
    }
    stack_t ss;
    std::memset(&ss, 0, sizeof(ss));
    ss.ss_sp = mem;
    ss.ss_size = CN_SIG_ALTSTACK_SIZE;
    if (::sigaltstack(&ss, nullptr) != 0) return;  // 失败：内存由 key 析构回收
}

// 主线程防护安装（entry() 调用）：sigaltstack + SIGSEGV 处理器。
//   安装失败=保持系统默认（降级不中断程序——宁缺毋误报）。
extern "C" void cn_install_stack_guard() {
    stack_t ss;
    std::memset(&ss, 0, sizeof(ss));
    ss.ss_sp = cn_sig_altstack;
    ss.ss_size = sizeof(cn_sig_altstack);
    if (::sigaltstack(&ss, nullptr) != 0) return;
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = cn_sigsegv_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    (void)::sigemptyset(&sa.sa_mask);
    if (::sigaction(SIGSEGV, &sa, nullptr) != 0) return;
}
#endif
