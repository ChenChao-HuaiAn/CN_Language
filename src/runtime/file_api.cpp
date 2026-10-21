// CN运行时实现：文件 API（Task 6.2，规格书阶段五「文件系统」/ 10.6 文件读写）
// 对标 C++ fstream，提供文件打开/读取/写入/按行读取/大小/关闭/存在性：
//   __cn_file_open       打开文件（模式 1=读"r"、2=写"w"（截断）、3=追加"a"），失败返回 nullptr
//   __cn_file_read       读取原始字节（fread，返回实际读取字节数）
//   __cn_file_write      写入字符串数据（fwrite 数据字节数，返回写入字节数）
//   __cn_file_read_line  按行读取（fgets 动态分配，含换行；EOF 返回 nullptr，调用方负责释放）
//   __cn_file_size       文件大小（fseek end + ftell + 恢复原位置），失败返回 -1
//   __cn_file_close      关闭文件（fclose，安全处理 nullptr）
//   __cn_file_exists     文件是否存在（_wfopen_s 试读，成功即关闭返回真）
//   __cn_file_mkdir      递归创建目录（逐级创建；已存在=成功；中段是文件/权限不足=失败）
// 中文路径：UTF-8 窄路径经 MultiByteToWideChar(CP_UTF8) 转 UTF-16 宽路径，
//   用 _wfopen_s 打开（lessons.md 已验证方案：std::filesystem::u8path 对中文路径挂起，弃用）。
// 句柄类型：C 层 FILE* 强制转为 void* 传递；CN 层对应 空类型*。
// 注意：__cn_file_read_line 返回的字符串由调用方负责用 __cn_str_free 释放。
#include "runtime/runtime.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

// 模式码 -> fopen 模式串（1=读"r"、2=写"w"、3=追加"a"；其余返回 nullptr）
const char* modeToFopen(int mode) {
    switch (mode) {
        case 1: return "rb";      // 读（文本二进制统一，按字节处理）
        case 2: return "wb";      // 写（截断）
        case 3: return "ab";      // 追加
        default: return nullptr;
    }
}

// 打开文件（内部实现）：UTF-8 路径 -> 宽路径（_wfopen_s），失败返回 nullptr
// Windows：MultiByteToWideChar(CP_UTF8) 转 UTF-16 后 _wfopen_s（中文路径可靠）；
//   窄路径直试（兼容 GBK 命令行传入路径，与 module.cpp readSourceFile 策略一致）。
// 非 Windows：直接 fopen。
FILE* fileOpenImpl(const char* path, const char* mode) {
    if (path == nullptr || mode == nullptr) return nullptr;
#ifdef _WIN32
    FILE* fp = nullptr;
    if (fopen_s(&fp, path, mode) == 0 && fp != nullptr) return fp;
    // 窄字符失败：UTF-8 路径 -> UTF-16 宽路径（中文文件名）
    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (wideLen <= 0) return nullptr;
    std::vector<wchar_t> widePath(static_cast<std::size_t>(wideLen));
    MultiByteToWideChar(CP_UTF8, 0, path, -1, widePath.data(), wideLen);
    std::vector<wchar_t> wideMode(wideLen);  // 模式串同为 ASCII，宽度复用
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wideMode.data(), wideLen);
    if (_wfopen_s(&fp, widePath.data(), wideMode.data()) != 0 || fp == nullptr) return nullptr;
    return fp;
#else
    return std::fopen(path, mode);
#endif
}

// 递归创建目录（内部实现）：逐级创建，已存在且确为目录=成功。
//   语义对齐宿主 cn_main.cpp ensureDirExists（#1138 双侧零分叉），但走进程内
//   mkdir/CreateDirectoryW（不经 shell——路径含引号/空格安全，交叉构建不受
//   `mkdir -p "..."` 引号规则影响）；中段同名文件/权限不足=false（显式失败，
//   禁静默继续）。
bool makeDirsImpl(const char* path) {
    bool ok = path != nullptr && *path != '\0';
    if (!ok) {
        return ok;  // 空路径=失败哨兵（调用方 stdlib 创建目录 转 错误(错误码.文件)）
    }
#ifdef _WIN32
    // UTF-8 -> UTF-16（中文路径；与 fileOpenImpl 同口径）；API 失败返回 0
    const int wideLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (wideLen == 0) {
        ok = false;
        return ok;
    }
    std::wstring dir(static_cast<std::size_t>(wideLen), L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, &dir[0], wideLen) == 0) {
        ok = false;
        return ok;
    }
    dir.resize(static_cast<std::size_t>(wideLen - 1));  // 去掉结尾 NUL
    // 去尾部多余分隔符（根路径保留）
    while (dir.size() > 1 && (dir.back() == L'\\' || dir.back() == L'/')) dir.pop_back();
    ok = !dir.empty();
    if (!ok) {
        return ok;
    }
    const auto isSep = [](wchar_t ch) { return ch == L'\\' || ch == L'/'; };
    const auto makeOne = [](const std::wstring& one) {
        if (CreateDirectoryW(one.c_str(), nullptr)) return true;
        if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
        const DWORD attrs = GetFileAttributesW(one.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    };
    // 根前缀长度：盘符（C:\）/ UNC（\\server\share）/ POSIX 风格绝对路径
    // （find==1/find_first_of==2 判据等价 size>=2/>=3 且字符位命中，避开单边区间）
    std::size_t start = 0;
    if (dir.find(L':') == 1) {
        start = 2;
        if (dir.find_first_of(L"\\/", 2) == 2) start = 3;
    } else if (isSep(dir[0]) && dir.find_first_of(L"\\/", 1) == 1) {
        std::size_t components = 0;
        std::size_t i = 2;
        while (i < dir.size() && components < 2) {  // server 与 share 两段
            if (isSep(dir[i])) components++;
            i++;
        }
        start = i;
    } else if (isSep(dir[0])) {
        start = 1;
    }
    if (start >= dir.size()) return makeOne(dir);  // 根本身：只核验目录身份
    for (std::size_t i = start + 1; i < dir.size(); ++i) {
        if (!isSep(dir[i])) continue;
        if (!makeOne(dir.substr(0, i))) {
            ok = false;
            return ok;
        }
    }
    return makeOne(dir);
#else
    std::string dir(path);
    while (dir.size() > 1 && dir.back() == '/') dir.pop_back();
    ok = !dir.empty();
    if (!ok) {
        return ok;
    }
    const auto makeOne = [](const std::string& one) {
        if (::mkdir(one.c_str(), 0777) == 0) return true;
        if (errno != EEXIST) return false;
        struct stat st;
        return ::stat(one.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    };
    for (std::size_t i = 1; i < dir.size(); ++i) {
        if (dir[i] != '/') continue;
        if (!makeOne(dir.substr(0, i))) {
            ok = false;
            return ok;
        }
    }
    return makeOne(dir);
#endif
}

} // namespace

// 打开文件：模式 1=读"rb"、2=写"wb"、3=追加"ab"；失败返回 nullptr
extern "C" void* __cn_file_open(const char* path, long long mode) {
    const char* fmode = modeToFopen(static_cast<int>(mode));
    if (fmode == nullptr) return nullptr;
    return fileOpenImpl(path, fmode);
}

// 读取文件原始字节：fread 到缓冲，返回实际读取字节数（EOF/失败时 < 数量）
// 句柄、缓冲必须非空（非法调用返回 0）；缓冲容量由调用方保证 >= 数量
extern "C" long long __cn_file_read(void* handle, char* buffer, long long count) {
    if (handle == nullptr || buffer == nullptr || count < 0) return 0;
    const std::size_t got = std::fread(buffer, 1, static_cast<std::size_t>(count),
                                       static_cast<FILE*>(handle));
    return static_cast<long long>(got);
}

// 写入文件字符串数据：fwrite 数据长度（不含结尾 \0），返回写入字节数
// 句柄、数据必须非空（非法调用返回 0）
extern "C" long long __cn_file_write(void* handle, const char* data) {
    if (handle == nullptr || data == nullptr) return 0;
    const std::size_t len = std::strlen(data);
    const std::size_t written = std::fwrite(data, 1, len, static_cast<FILE*>(handle));
    return static_cast<long long>(written);
}

// 按行读取文件：fgets 动态分配一整行（含换行；文本文件 \n 结尾）
// 返回动态分配字符串（调用方负责 __cn_str_free）；EOF/失败返回 nullptr
extern "C" char* __cn_file_read_line(void* handle) {
    if (handle == nullptr) return nullptr;
    FILE* fp = static_cast<FILE*>(handle);
    std::size_t cap = 128;
    char* buf = static_cast<char*>(cn_alloc_tracked(cap));
    if (buf == nullptr) return nullptr;
    std::size_t len = 0;
    int c = std::fgetc(fp);
    while (c != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char* nb = static_cast<char*>(cn_realloc_tracked(buf, cap));
            if (nb == nullptr) {
                cn_free_tracked(buf);
                return nullptr;
            }
            buf = nb;
        }
        buf[len++] = static_cast<char>(c);
        if (c == '\n') break;
        c = std::fgetc(fp);
    }
    if (len == 0) {
        cn_free_tracked(buf);  // EOF 且无数据
        return nullptr;
    }
    buf[len] = '\0';
    return buf;
}

// 文件大小：fseek end + ftell + 恢复原位置；失败返回 -1
extern "C" long long __cn_file_size(void* handle) {
    if (handle == nullptr) return -1;
    FILE* fp = static_cast<FILE*>(handle);
    const long cur = std::ftell(fp);
    if (cur < 0) return -1;
    if (std::fseek(fp, 0, SEEK_END) != 0) return -1;
    const long size = std::ftell(fp);
    // 恢复原位置（失败仅影响后续读写，不影响本次结果）
    std::fseek(fp, cur, SEEK_SET);
    return size < 0 ? -1 : static_cast<long long>(size);
}

// 关闭文件：fclose（安全处理 nullptr；指针不再使用）
extern "C" void __cn_file_close(void* handle) {
    if (handle != nullptr) {
        std::fclose(static_cast<FILE*>(handle));
    }
}

// 文件是否存在：_wfopen_s 试读（模式"rb"），成功即关闭返回真；失败返回假
extern "C" long long __cn_file_exists(const char* path) {
    FILE* fp = fileOpenImpl(path, "rb");
    if (fp == nullptr) return 0;
    std::fclose(fp);
    return 1;
}

// 递归创建目录：逐级创建，成功（含已存在且确为目录）返回 1；失败返回 0。
// 调用方（stdlib/文件.cn 创建目录 包装）据此返回 结果<空类型,整32>，
// 失败路径不静默（#1138：v2 写 asm 前建输出目录失败须显式诊断中止）。
extern "C" long long __cn_file_mkdir(const char* path) {
    return makeDirsImpl(path) ? 1 : 0;
}

// ==================== 程序缓存支撑原语（任务 1272） ====================
namespace {

// UTF-8 窄路径 -> UTF-16 宽路径（Windows）。失败返回 false。
#ifdef _WIN32
bool utf8ToWide(const char* text, std::wstring& out) {
    // 静默豁免运行时API: 空路径=转换失败信号（调用方禁用缓存/显式返回）
    if (text == nullptr) return false;
    const int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    // 静默豁免运行时API: MultiByteToWideChar 失败（<=0）=平台错误信号
    if (len <= 0) return false;
    std::vector<wchar_t> buf(static_cast<std::size_t>(len));
    // 静默豁免运行时API: 宽字符转换失败=平台错误信号（调用方显式分流）
    if (MultiByteToWideChar(CP_UTF8, 0, text, -1, buf.data(), len) <= 0) return false;
    out.assign(buf.data());  // 以 NUL 终止，构造时自然截断
    return true;
}
#endif

// 目录递归创建：已存在视为成功；任一级失败且最终不是目录=失败。
long long ensureDirImpl(const char* path) {
    if (path == nullptr || path[0] == '\0') return 1;
#ifdef _WIN32
    std::wstring wide;
    if (!utf8ToWide(path, wide)) return 1;
    // 逐段 CreateDirectoryW：跳过盘符（"C:"）与重复分隔符；已存在=成功。
    for (std::size_t i = 1; i < wide.size(); ++i) {
        if (wide[i] == L'/' || wide[i] == L'\\') {
            const std::wstring part = wide.substr(0, i);
            if (!part.empty() && part.back() != L':') {
                CreateDirectoryW(part.c_str(), nullptr);
            }
        }
    }
    CreateDirectoryW(wide.c_str(), nullptr);
    const DWORD attrs = GetFileAttributesW(wide.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES &&
            (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) ? 0 : 1;
#else
    std::string current(path);
    // 逐段 mkdir；绝对路径首个 '/' 前的空段跳过
    for (std::size_t i = 1; i <= current.size(); ++i) {
        if (i == current.size() || current[i] == '/') {
            const std::string part = current.substr(0, i);
            if (!part.empty()) {
                if (mkdir(part.c_str(), 0755) != 0 && errno != EEXIST) return 1;
            }
        }
    }
    struct stat st {};
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) return 1;
    return 0;
#endif
}

}  // namespace

namespace {

// FNV-1a 128（双 64 位通道；非加密——缓存指纹/身份）。返回 0=成功 1=打开失败 2=读失败。
long long hashFileImpl(const char* path, std::uint64_t& a, std::uint64_t& b) {
    FILE* fp = fileOpenImpl(path, "rb");
    if (fp == nullptr) return 1;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    a = 14695981039346656037ULL;
    b = 1469598103934665603ULL;
    unsigned char buf[65536];
    bool readFailed = false;
    for (;;) {
        const std::size_t got = std::fread(buf, 1, sizeof(buf), fp);
        for (std::size_t i = 0; i < got; ++i) {
            const std::uint64_t byte = static_cast<std::uint64_t>(buf[i]);
            a = (a ^ byte) * kPrime;
            b = (b ^ (byte + 0x9eULL)) * kPrime;
        }
        if (got < sizeof(buf)) {
            if (std::ferror(fp) != 0) readFailed = true;
            break;
        }
    }
    std::fclose(fp);
    return readFailed ? 2 : 0;
}

#ifdef _WIN32
// 当前可执行文件绝对路径（GetModuleFileNameW；截断/失败=假）
bool currentExeWide(std::wstring& wide) {
    std::vector<wchar_t> buf(32768);
    const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= static_cast<DWORD>(buf.size())) return false;
    wide.assign(buf.data(), static_cast<std::size_t>(n));
    return true;
}
#else
// 当前可执行文件路径（/proc/self/exe；失败=假）
bool currentExeNarrow(std::string& path) {
    char buf[4096];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n < 1 || n > static_cast<ssize_t>(sizeof(buf) - 2)) return false;
    buf[n] = '\0';
    path.assign(buf, static_cast<std::size_t>(n));
    return true;
}
#endif

}  // namespace

// 流式文件摘要：FNV-1a 128。返回 0=成功；1=打开失败；2=读取失败。
extern "C" long long __cn_file_digest_utf8(const char* path, long long* a, long long* b) {
    if (a == nullptr || b == nullptr) return 1;
    *a = 0;
    *b = 0;
    std::uint64_t laneA = 0;
    std::uint64_t laneB = 0;
    const long long status = hashFileImpl(path, laneA, laneB);
    if (status == 0) {
        *a = static_cast<long long>(laneA);
        *b = static_cast<long long>(laneB);
    }
    return status;
}

// 自身可执行文件内容摘要（GetModuleFileNameW / /proc/self/exe 直接定位，
//   不经 argv[0]——POSIX 裸名经 PATH + cwd 同名文件会摘要错对象）。0=成功，1=失败。
extern "C" long long __cn_exe_identity(long long* a, long long* b) {
    if (a == nullptr || b == nullptr) return 1;
    std::uint64_t laneA = 0;
    std::uint64_t laneB = 0;
#ifdef _WIN32
    std::wstring wide;
    if (!currentExeWide(wide)) return 1;
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, wide.c_str(), L"rb") != 0 || fp == nullptr) return 1;
    // 复用流式 FNV（窄路径接口不适用宽路径；此处内联同算法）
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    laneA = 14695981039346656037ULL;
    laneB = 1469598103934665603ULL;
    unsigned char buf[65536];
    bool readFailed = false;
    for (;;) {
        const std::size_t got = std::fread(buf, 1, sizeof(buf), fp);
        for (std::size_t i = 0; i < got; ++i) {
            const std::uint64_t byte = static_cast<std::uint64_t>(buf[i]);
            laneA = (laneA ^ byte) * kPrime;
            laneB = (laneB ^ (byte + 0x9eULL)) * kPrime;
        }
        if (got < sizeof(buf)) {
            if (std::ferror(fp) != 0) readFailed = true;
            break;
        }
    }
    const int closeRc = std::fclose(fp);
    if (readFailed || closeRc != 0) return 1;
#else
    std::string path;
    if (!currentExeNarrow(path)) return 1;
    const long long status = hashFileImpl(path.c_str(), laneA, laneB);
    if (status != 0) return 1;
#endif
    *a = static_cast<long long>(laneA);
    *b = static_cast<long long>(laneB);
    return 0;
}

// 进程 ID（缓存 tmp 名唯一化）。失败返回 0（调用方以序号/时间兜底不允许——
//   0 与合法 PID 不重叠，调用方须检查；此处失败面=平台 API 契约）。
extern "C" long long __cn_process_id() {
#ifdef _WIN32
    return static_cast<long long>(GetCurrentProcessId());
#else
    return static_cast<long long>(getpid());
#endif
}

// 原子替换重命名：目标已存在时覆盖。返回 0=成功，非 0=失败。
extern "C" long long __cn_file_rename_replace_utf8(const char* from, const char* to) {
    if (from == nullptr || to == nullptr) return 1;
#ifdef _WIN32
    std::wstring wideFrom;
    std::wstring wideTo;
    if (!utf8ToWide(from, wideFrom) || !utf8ToWide(to, wideTo)) return 1;
    const BOOL ok = MoveFileExW(wideFrom.c_str(), wideTo.c_str(),
                                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    return ok ? 0 : 2;
#else
    return std::rename(from, to) == 0 ? 0 : 2;
#endif
}

// 删除文件（缓存 tmp 清理）：0=成功/本不存在，非 0=失败。
extern "C" long long __cn_file_delete_utf8(const char* path) {
    if (path == nullptr) return 1;
#ifdef _WIN32
    std::wstring wide;
    if (!utf8ToWide(path, wide)) return 1;
    if (DeleteFileW(wide.c_str()) != 0) return 0;
    return GetLastError() == ERROR_FILE_NOT_FOUND ? 0 : 2;
#else
    if (std::remove(path) == 0) return 0;
    return errno == ENOENT ? 0 : 2;
#endif
}

// 目录递归创建（缓存目录）。返回 0=成功/已存在，非 0=失败。
extern "C" long long __cn_dir_ensure_utf8(const char* path) { return ensureDirImpl(path); }

// 内存文本 128 位摘要（与 __cn_file_digest_utf8 同算法：FNV-1a 双 64 位通道）。
// 返回 0=成功（文本非空指针；nullptr 按空串处理）。
extern "C" long long __cn_str_hash128(const char* text, long long* a, long long* b) {
    if (a == nullptr || b == nullptr) return 1;
    if (text == nullptr) text = "";
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t laneA = 14695981039346656037ULL;
    std::uint64_t laneB = 1469598103934665603ULL;
    const std::size_t len = std::strlen(text);
    for (std::size_t i = 0; i < len; ++i) {
        const std::uint64_t byte = static_cast<std::uint64_t>(
            static_cast<unsigned char>(text[i]));
        laneA = (laneA ^ byte) * kPrime;
        laneB = (laneB ^ (byte + 0x9eULL)) * kPrime;
    }
    *a = static_cast<long long>(laneA);
    *b = static_cast<long long>(laneB);
    return 0;
}

// 整文件读取（二进制全文 + NUL 终止）：0=成功（out=拥有串，__cn_str_free；
//   空文件返回长度为 0 的空串），1=打开失败，2=定位失败，3=超上限（2GB），
//   4=分配失败，5=读取/关闭失败。64 位文件位置（_ftelli64/ftello）。
extern "C" long long __cn_file_read_all_utf8(const char* path, char** out,
                                              unsigned long long* outLen) {
    if (path == nullptr || out == nullptr || outLen == nullptr) return 2;
    *outLen = 0;
    FILE* fp = fileOpenImpl(path, "rb");
    if (fp == nullptr) return 1;
#ifdef _WIN32
    if (_fseeki64(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        return 2;
    }
    // 静默豁免运行时API: ftell 失败=文件定位错误（状态码 2 显式返回）
    const long long size = _ftelli64(fp);
#else
    if (fseeko(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        return 2;
    }
    // 静默豁免运行时API: ftell 失败=文件定位错误（状态码 2 显式返回）
    const off_t size = ftello(fp);
#endif
    // 静默豁免运行时API: ftell 失败=文件定位错误（状态码 2 显式返回）
    if (size < 0) {
        std::fclose(fp);
        return 2;
    }
    const unsigned long long limit = 2ULL << 30;  // 2GB 上限（缓存产物远小于此）
    if (static_cast<unsigned long long>(size) > limit) {
        std::fclose(fp);
        return 3;
    }
#ifdef _WIN32
    if (_fseeki64(fp, 0, SEEK_SET) != 0) {
#else
    if (fseeko(fp, 0, SEEK_SET) != 0) {
#endif
        std::fclose(fp);
        return 2;
    }
    const std::size_t len = static_cast<std::size_t>(size);
    char* buf = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (buf == nullptr) {
        std::fclose(fp);
        return 4;
    }
    const std::size_t got = std::fread(buf, 1, len, fp);
    const bool readOk = (got == len && std::ferror(fp) == 0);
    const int closeRc = std::fclose(fp);
    if (!readOk || closeRc != 0) {
        cn_free_tracked(buf);
        return 5;
    }
    buf[len] = '\0';
    *out = buf;
    *outLen = static_cast<unsigned long long>(len);
    return 0;
}
