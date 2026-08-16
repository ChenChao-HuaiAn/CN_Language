// CN运行时实现：文件 API（Task 6.2，规格书阶段五「文件系统」/ 10.6 文件读写）
// 对标 C++ fstream，提供文件打开/读取/写入/按行读取/大小/关闭/存在性：
//   __cn_file_open       打开文件（模式 1=读"r"、2=写"w"（截断）、3=追加"a"），失败返回 nullptr
//   __cn_file_read       读取原始字节（fread，返回实际读取字节数）
//   __cn_file_write      写入字符串数据（fwrite 数据字节数，返回写入字节数）
//   __cn_file_read_line  按行读取（fgets 动态分配，含换行；EOF 返回 nullptr，调用方负责释放）
//   __cn_file_size       文件大小（fseek end + ftell + 恢复原位置），失败返回 -1
//   __cn_file_close      关闭文件（fclose，安全处理 nullptr）
//   __cn_file_exists     文件是否存在（_wfopen_s 试读，成功即关闭返回真）
// 中文路径：UTF-8 窄路径经 MultiByteToWideChar(CP_UTF8) 转 UTF-16 宽路径，
//   用 _wfopen_s 打开（lessons.md 已验证方案：std::filesystem::u8path 对中文路径挂起，弃用）。
// 句柄类型：C 层 FILE* 强制转为 void* 传递；CN 层对应 空类型*。
// 注意：__cn_file_read_line 返回的字符串由调用方负责用 __cn_str_free 释放。
#include "runtime/runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
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
