// 996（027 波 1·008 销账后波8 启动·方案甲已批 2026-09-27）：CN 线程运行时层——
//   linux-x64=pthread 真实现（波 1·宿主先行）；win=编译桩（调用即运行时错误(9)
//   波 3 预留——宁严勿松：未实现的平台不得静默空转）。
// CN 语言符号映射（stdlib/线程.cn 消费）：
//   __cn_thread_new(entry, arg) -> 句柄     入口=函数指针 void*(*)(void*)
//   __cn_thread_join(句柄) -> void*          并入（阻塞·取返回值）
//   __cn_mutex_new() -> 句柄                 互斥锁创建
//   __cn_mutex_lock(句柄)                    上锁（阻塞）
//   __cn_mutex_unlock(句柄)                  解锁
//   __cn_mutex_free(句柄)                    销毁（须未加锁态）
// 内存序默认顺序一致（027 §3.3·原子族=plans/026 L4 波 2 承载，不在本层）。
//
// 生命周期纪律：线程句柄 join 后由实现释放（pthread_detach 语义经 join 吸收）；
//   互斥锁句柄须显式 __cn_mutex_free（stdlib 锁守卫 RAII 析构链）。

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
// ---- win 桩（波 3 预留·宁严勿松：调用即运行时错误(9)） ----
extern "C" {

void* __cn_thread_new(void* (*entry)(void*), void* arg) {
    (void)entry;
    (void)arg;
    std::fprintf(stderr, "运行时错误(9): 线程层 win 实现随 027 波 3 提供（当前仅 linux-x64）\n");
    std::exit(9);
}

void* __cn_thread_join(void* handle) {
    (void)handle;
    std::fprintf(stderr, "运行时错误(9): 线程层 win 实现随 027 波 3 提供（当前仅 linux-x64）\n");
    std::exit(9);
}

void* __cn_mutex_new(void) {
    std::fprintf(stderr, "运行时错误(9): 线程层 win 实现随 027 波 3 提供（当前仅 linux-x64）\n");
    std::exit(9);
}

void __cn_mutex_lock(void* handle) {
    (void)handle;
    std::fprintf(stderr, "运行时错误(9): 线程层 win 实现随 027 波 3 提供（当前仅 linux-x64）\n");
    std::exit(9);
}

void __cn_mutex_unlock(void* handle) {
    (void)handle;
    std::fprintf(stderr, "运行时错误(9): 线程层 win 实现随 027 波 3 提供（当前仅 linux-x64）\n");
    std::exit(9);
}

void __cn_mutex_free(void* handle) {
    (void)handle;
    std::fprintf(stderr, "运行时错误(9): 线程层 win 实现随 027 波 3 提供（当前仅 linux-x64）\n");
    std::exit(9);
}

} // extern "C"

#elif defined(__linux__)

#include <pthread.h>
#include <vector>

namespace {

// 入口包装：CN 函数指针（返回值+实参）统一经 void* 盒传递——
//   stdlib/线程.cn 的入口包装函数（CN 版）负责「解盒→调 CN 函数→结果装盒」，
//   本层只管 pthread 生命周期（零 CN 语义知识=分层纪律）。
struct ThreadBox {
    void* (*entry)(void*);
    void* arg;
    void* result;
};

void* thread_trampoline(void* raw) {
    ThreadBox* box = static_cast<ThreadBox*>(raw);
    void* result = box->entry(box->arg);
    box->result = result;
    return raw;  // 盒指针经 pthread_join 归还（结果由 join 侧转交后弃盒）
}

} // namespace

extern "C" {

// 新线程：返回 pthread 句柄（入堆·join 侧负责释放）；失败=运行时错误(4)
void* __cn_thread_new(void* (*entry)(void*), void* arg) {
    ThreadBox* box = static_cast<ThreadBox*>(std::malloc(sizeof(ThreadBox)));
    if (box == nullptr) {
        std::fprintf(stderr, "运行时错误(4): 线程盒内存分配失败\n");
        std::exit(4);
    }
    box->entry = entry;
    box->arg = arg;
    box->result = nullptr;
    pthread_t tid = 0;
    const int rc = pthread_create(&tid, nullptr, thread_trampoline, box);
    if (rc != 0) {
        std::fprintf(stderr, "运行时错误(4): pthread_create 失败（%d）\n", rc);
        std::exit(4);
    }
    // 句柄堆化（pthread_t 不透明·join 侧释放）
    pthread_t* handle = static_cast<pthread_t*>(std::malloc(sizeof(pthread_t)));
    if (handle == nullptr) {
        std::fprintf(stderr, "运行时错误(4): 线程句柄分配失败\n");
        std::exit(4);
    }
    *handle = tid;
    return handle;
}

// 并入：阻塞取返回值（盒指针转交结果后弃盒·句柄释放）
void* __cn_thread_join(void* handle) {
    pthread_t tid = *static_cast<pthread_t*>(handle);
    void* raw = nullptr;
    const int rc = pthread_join(tid, &raw);
    if (rc != 0) {
        std::fprintf(stderr, "运行时错误(4): pthread_join 失败（%d）\n", rc);
        std::exit(4);
    }
    std::free(handle);
    ThreadBox* box = static_cast<ThreadBox*>(raw);
    void* result = box->result;
    std::free(box);
    return result;
}

void* __cn_mutex_new(void) {
    pthread_mutex_t* m = static_cast<pthread_mutex_t*>(
        std::malloc(sizeof(pthread_mutex_t)));
    if (m == nullptr) {
        std::fprintf(stderr, "运行时错误(4): 互斥锁分配失败\n");
        std::exit(4);
    }
    const int rc = pthread_mutex_init(m, nullptr);
    if (rc != 0) {
        std::fprintf(stderr, "运行时错误(4): pthread_mutex_init 失败（%d）\n", rc);
        std::exit(4);
    }
    return m;
}

void __cn_mutex_lock(void* handle) {
    const int rc = pthread_mutex_lock(static_cast<pthread_mutex_t*>(handle));
    if (rc != 0) {
        std::fprintf(stderr, "运行时错误(4): pthread_mutex_lock 失败（%d）\n", rc);
        std::exit(4);
    }
}

void __cn_mutex_unlock(void* handle) {
    const int rc = pthread_mutex_unlock(static_cast<pthread_mutex_t*>(handle));
    if (rc != 0) {
        std::fprintf(stderr, "运行时错误(4): pthread_mutex_unlock 失败（%d）\n", rc);
        std::exit(4);
    }
}

void __cn_mutex_free(void* handle) {
    pthread_mutex_destroy(static_cast<pthread_mutex_t*>(handle));
    std::free(handle);
}

} // extern "C"

#else
#error "027 波 1：仅支持 linux（pthread）与 win（桩）——其它平台随波 3"
#endif
