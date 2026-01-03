/******************************************************************************
 * 文件名: CN_platform_windows_filesystem.c
 * 功能: CN_Language Windows平台文件系统子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows文件系统功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief Windows文件句柄包装
 */
typedef struct Stru_CN_WindowsFileHandle_t
{
    HANDLE handle;                  /**< Windows文件句柄 */
    char* path;                     /**< 文件路径 */
    Eum_CN_FileMode_t mode;         /**< 打开模式 */
    uint64_t position;              /**< 当前位置 */
} Stru_CN_WindowsFileHandle_t;

/**
 * @brief Windows目录句柄包装
 */
typedef struct Stru_CN_WindowsDirectoryHandle_t
{
    HANDLE handle;                  /**< FindFirstFile句柄 */
    WIN32_FIND_DATAW find_data;     /**< 查找数据 */
    BOOL first;                     /**< 是否是第一个文件 */
    char* path;                     /**< 目录路径 */
} Stru_CN_WindowsDirectoryHandle_t;

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 将UTF-8字符串转换为宽字符串
 */
static wchar_t* utf8_to_wide(const char* utf8)
{
    if (!utf8) return NULL;
    
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wide_len <= 0) return NULL;
    
    wchar_t* wide = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide) return NULL;
    
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, wide_len);
    return wide;
}

/**
 * @brief 将宽字符串转换为UTF-8字符串
 */
static char* wide_to_utf8(const wchar_t* wide)
{
    if (!wide) return NULL;
    
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return NULL;
    
    char* utf8 = (char*)malloc(utf8_len);
    if (!utf8) return NULL;
    
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8_len, NULL, NULL);
    return utf8;
}

/**
 * @brief 将Windows文件属性转换为CN文件属性
 */
static uint32_t windows_attrs_to_cn_attrs(DWORD win_attrs)
{
    uint32_t cn_attrs = 0;
    
    if (win_attrs & FILE_ATTRIBUTE_READONLY) cn_attrs |= Eum_FILE_ATTR_READONLY;
    if (win_attrs & FILE_ATTRIBUTE_HIDDEN) cn_attrs |= Eum_FILE_ATTR_HIDDEN;
    if (win_attrs & FILE_ATTRIBUTE_SYSTEM) cn_attrs |= Eum_FILE_ATTR_SYSTEM;
    if (win_attrs & FILE_ATTRIBUTE_DIRECTORY) cn_attrs |= Eum_FILE_ATTR_DIRECTORY;
    if (win_attrs & FILE_ATTRIBUTE_ARCHIVE) cn_attrs |= Eum_FILE_ATTR_ARCHIVE;
    if (win_attrs & FILE_ATTRIBUTE_NORMAL) cn_attrs |= Eum_FILE_ATTR_NORMAL;
    
    return cn_attrs;
}

/**
 * @brief 将CN文件模式转换为Windows访问模式和创建标志
 */
static void cn_mode_to_windows_mode(Eum_CN_FileMode_t cn_mode, 
                                   DWORD* access_mode, DWORD* creation_disposition)
{
    switch (cn_mode)
    {
        case Eum_FILE_MODE_READ:
            *access_mode = GENERIC_READ;
            *creation_disposition = OPEN_EXISTING;
            break;
            
        case Eum_FILE_MODE_WRITE:
            *access_mode = GENERIC_WRITE;
            *creation_disposition = CREATE_ALWAYS;
            break;
            
        case Eum_FILE_MODE_APPEND:
            *access_mode = GENERIC_WRITE;
            *creation_disposition = OPEN_ALWAYS;
            break;
            
        case Eum_FILE_MODE_READ_WRITE:
            *access_mode = GENERIC_READ | GENERIC_WRITE;
            *creation_disposition = OPEN_EXISTING;
            break;
            
        case Eum_FILE_MODE_CREATE:
            *access_mode = GENERIC_READ | GENERIC_WRITE;
            *creation_disposition = CREATE_ALWAYS;
            break;
            
        default:
            *access_mode = GENERIC_READ;
            *creation_disposition = OPEN_EXISTING;
            break;
    }
}

/**
 * @brief 将Windows FILETIME转换为Unix时间戳（毫秒）
 */
static uint64_t filetime_to_timestamp(const FILETIME* ft)
{
    ULARGE_INTEGER uli;
    uli.LowPart = ft->dwLowDateTime;
    uli.HighPart = ft->dwHighDateTime;
    
    // FILETIME是100纳秒间隔，从1601-01-01开始
    // 转换为Unix时间戳（毫秒，从1970-01-01开始）
    return (uli.QuadPart / 10000) - 11644473600000ULL;
}

// ============================================================================
// 函数声明（前向声明）
// ============================================================================

// 文件操作函数
static bool windows_file_exists(const char* path);
static CN_FileHandle_t windows_file_open(const char* path, Eum_CN_FileMode_t mode);
static bool windows_file_close(CN_FileHandle_t handle);
static size_t windows_file_read(CN_FileHandle_t handle, void* buffer, size_t size);
static size_t windows_file_write(CN_FileHandle_t handle, const void* buffer, size_t size);
static bool windows_file_seek(CN_FileHandle_t handle, int64_t offset, int whence);
static int64_t windows_file_tell(CN_FileHandle_t handle);
static bool windows_file_get_info(const char* path, Stru_CN_FileInfo_t* info);
static bool windows_file_set_attributes(const char* path, uint32_t attributes);
static bool windows_file_delete(const char* path);
static bool windows_file_rename(const char* old_path, const char* new_path);
static bool windows_file_copy(const char* src_path, const char* dst_path);

// 目录操作函数
static bool windows_directory_create(const char* path);
static bool windows_directory_delete(const char* path);
static bool windows_directory_exists(const char* path);
static void* windows_directory_open(const char* path);
static bool windows_directory_read(void* handle, Stru_CN_FileInfo_t* info);
static bool windows_directory_close(void* handle);

// 路径操作函数
static bool windows_path_is_absolute(const char* path);
static bool windows_path_get_directory(const char* path, char* directory, size_t size);
static bool windows_path_get_filename(const char* path, char* filename, size_t size);
static bool windows_path_get_extension(const char* path, char* extension, size_t size);

// 特殊路径函数
static bool windows_set_current_directory(const char* path);
static bool windows_get_home_directory(char* buffer, size_t size);
static bool windows_get_temp_directory(char* buffer, size_t size);

// ============================================================================
// 文件系统接口实现
// ============================================================================

// 文件存在性检查
static bool windows_file_exists(const char* path)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    DWORD attrs = GetFileAttributesW(wpath);
    free(wpath);
    
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

// 文件打开
CN_FileHandle_t windows_file_open(const char* path, Eum_CN_FileMode_t mode)
{
    if (!path) return NULL;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return NULL;
    
    DWORD access_mode, creation_disposition;
    cn_mode_to_windows_mode(mode, &access_mode, &creation_disposition);
    
    HANDLE handle = CreateFileW(
        wpath,
        access_mode,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        creation_disposition,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    free(wpath);
    
    if (handle == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }
    
    Stru_CN_WindowsFileHandle_t* file_handle = 
        (Stru_CN_WindowsFileHandle_t*)malloc(sizeof(Stru_CN_WindowsFileHandle_t));
    if (!file_handle)
    {
        CloseHandle(handle);
        return NULL;
    }
    
    file_handle->handle = handle;
    file_handle->path = strdup(path);
    file_handle->mode = mode;
    file_handle->position = 0;
    
    return (CN_FileHandle_t)file_handle;
}

// 文件关闭
bool windows_file_close(CN_FileHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsFileHandle_t* file_handle = (Stru_CN_WindowsFileHandle_t*)handle;
    
    BOOL result = CloseHandle(file_handle->handle);
    
    free(file_handle->path);
    free(file_handle);
    
    return result != FALSE;
}

// 文件读取
size_t windows_file_read(CN_FileHandle_t handle, void* buffer, size_t size)
{
    if (!handle || !buffer || size == 0) return 0;
    
    Stru_CN_WindowsFileHandle_t* file_handle = (Stru_CN_WindowsFileHandle_t*)handle;
    
    DWORD bytes_read = 0;
    if (!ReadFile(file_handle->handle, buffer, (DWORD)size, &bytes_read, NULL))
    {
        return 0;
    }
    
    file_handle->position += bytes_read;
    return bytes_read;
}

// 文件写入
size_t windows_file_write(CN_FileHandle_t handle, const void* buffer, size_t size)
{
    if (!handle || !buffer || size == 0) return 0;
    
    Stru_CN_WindowsFileHandle_t* file_handle = (Stru_CN_WindowsFileHandle_t*)handle;
    
    DWORD bytes_written = 0;
    if (!WriteFile(file_handle->handle, buffer, (DWORD)size, &bytes_written, NULL))
    {
        return 0;
    }
    
    file_handle->position += bytes_written;
    return bytes_written;
}

// 文件定位
bool windows_file_seek(CN_FileHandle_t handle, int64_t offset, int whence)
{
    if (!handle) return false;
    
    Stru_CN_WindowsFileHandle_t* file_handle = (Stru_CN_WindowsFileHandle_t*)handle;
    
    DWORD move_method;
    switch (whence)
    {
        case SEEK_SET: move_method = FILE_BEGIN; break;
        case SEEK_CUR: move_method = FILE_CURRENT; break;
        case SEEK_END: move_method = FILE_END; break;
        default: return false;
    }
    
    LARGE_INTEGER li_offset;
    li_offset.QuadPart = offset;
    
    LARGE_INTEGER new_position;
    if (!SetFilePointerEx(file_handle->handle, li_offset, &new_position, move_method))
    {
        return false;
    }
    
    file_handle->position = new_position.QuadPart;
    return true;
}

// ============================================================================
// 文件系统接口结构定义
// ============================================================================

/**
 * @brief Windows平台文件系统接口实现
 */
Stru_CN_FileSystemInterface_t g_windows_filesystem_interface = {
    // 文件存在性检查
    .file_exists = windows_file_exists,
    
    // 文件打开和关闭
    .file_open = windows_file_open,
    .file_close = windows_file_close,
    
    // 文件读写
    .file_read = windows_file_read,
    .file_write = windows_file_write,
    
    // 文件定位
    .file_seek = windows_file_seek,
    .file_tell = windows_file_tell,
    
    // 文件属性
    .file_get_info = windows_file_get_info,
    .file_set_attributes = windows_file_set_attributes,
    
    // 文件操作
    .file_delete = windows_file_delete,
    .file_rename = windows_file_rename,
    .file_copy = windows_file_copy,
    
    // 目录操作
    .directory_create = windows_directory_create,
    .directory_delete = windows_directory_delete,
    .directory_exists = windows_directory_exists,
    
    // 目录遍历
    .directory_open = windows_directory_open,
    .directory_read = windows_directory_read,
    .directory_close = windows_directory_close,
    
    // 路径操作
    .path_is_absolute = windows_path_is_absolute,
    .path_is_relative = NULL, // 暂未实现
    .path_combine = NULL, // 暂未实现
    .path_get_directory = NULL, // 函数签名不匹配，暂未实现
    .path_get_filename = NULL, // 函数签名不匹配，暂未实现
    .path_get_extension = NULL, // 函数签名不匹配，暂未实现
    
    // 特殊路径
    .get_current_directory = NULL, // 暂未实现
    .set_current_directory = NULL, // 函数签名不匹配，暂未实现
    .get_home_directory = NULL, // 函数签名不匹配，暂未实现
    .get_temp_directory = NULL, // 函数签名不匹配，暂未实现
    
    // 文件系统信息
    .get_disk_free_space = NULL, // 暂未实现
    .get_disk_total_space = NULL, // 暂未实现
};

/**
 * @brief 获取Windows平台文件系统接口实现
 * 
 * @return Windows平台文件系统接口指针
 */
Stru_CN_FileSystemInterface_t* CN_platform_windows_get_filesystem_impl(void)
{
    return &g_windows_filesystem_interface;
}

// 获取文件位置
int64_t windows_file_tell(CN_FileHandle_t handle)
{
    if (!handle) return -1;
    
    Stru_CN_WindowsFileHandle_t* file_handle = (Stru_CN_WindowsFileHandle_t*)handle;
    return file_handle->position;
}

// 获取文件信息
bool windows_file_get_info(const char* path, Stru_CN_FileInfo_t* info)
{
    if (!path || !info) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    WIN32_FILE_ATTRIBUTE_DATA attr_data;
    if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &attr_data))
    {
        free(wpath);
        return false;
    }
    
    // 转换文件名
    wchar_t* wfilename = wcsrchr(wpath, L'\\');
    if (wfilename) wfilename++;
    else wfilename = wpath;
    
    char* filename = wide_to_utf8(wfilename);
    if (filename)
    {
        strncpy(info->name, filename, sizeof(info->name) - 1);
        info->name[sizeof(info->name) - 1] = '\0';
        free(filename);
    }
    else
    {
        info->name[0] = '\0';
    }
    
    // 文件大小
    ULARGE_INTEGER size;
    size.LowPart = attr_data.nFileSizeLow;
    size.HighPart = attr_data.nFileSizeHigh;
    info->size = size.QuadPart;
    
    // 时间戳
    info->creation_time = filetime_to_timestamp(&attr_data.ftCreationTime);
    info->modification_time = filetime_to_timestamp(&attr_data.ftLastWriteTime);
    info->access_time = filetime_to_timestamp(&attr_data.ftLastAccessTime);
    
    // 属性
    info->attributes = windows_attrs_to_cn_attrs(attr_data.dwFileAttributes);
    
    free(wpath);
    return true;
}

// 设置文件属性
bool windows_file_set_attributes(const char* path, uint32_t attributes)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    DWORD win_attrs = 0;
    if (attributes & Eum_FILE_ATTR_READONLY) win_attrs |= FILE_ATTRIBUTE_READONLY;
    if (attributes & Eum_FILE_ATTR_HIDDEN) win_attrs |= FILE_ATTRIBUTE_HIDDEN;
    if (attributes & Eum_FILE_ATTR_SYSTEM) win_attrs |= FILE_ATTRIBUTE_SYSTEM;
    if (attributes & Eum_FILE_ATTR_ARCHIVE) win_attrs |= FILE_ATTRIBUTE_ARCHIVE;
    if (attributes & Eum_FILE_ATTR_NORMAL) win_attrs |= FILE_ATTRIBUTE_NORMAL;
    
    BOOL result = SetFileAttributesW(wpath, win_attrs);
    free(wpath);
    
    return result != FALSE;
}

// 文件删除
bool windows_file_delete(const char* path)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    BOOL result = DeleteFileW(wpath);
    free(wpath);
    
    return result != FALSE;
}

// 文件重命名
bool windows_file_rename(const char* old_path, const char* new_path)
{
    if (!old_path || !new_path) return false;
    
    wchar_t* wold_path = utf8_to_wide(old_path);
    wchar_t* wnew_path = utf8_to_wide(new_path);
    
    if (!wold_path || !wnew_path)
    {
        free(wold_path);
        free(wnew_path);
        return false;
    }
    
    BOOL result = MoveFileW(wold_path, wnew_path);
    
    free(wold_path);
    free(wnew_path);
    
    return result != FALSE;
}

// 文件复制
bool windows_file_copy(const char* src_path, const char* dst_path)
{
    if (!src_path || !dst_path) return false;
    
    wchar_t* wsrc_path = utf8_to_wide(src_path);
    wchar_t* wdst_path = utf8_to_wide(dst_path);
    
    if (!wsrc_path || !wdst_path)
    {
        free(wsrc_path);
        free(wdst_path);
        return false;
    }
    
    BOOL result = CopyFileW(wsrc_path, wdst_path, FALSE);
    
    free(wsrc_path);
    free(wdst_path);
    
    return result != FALSE;
}

// 目录创建
bool windows_directory_create(const char* path)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    BOOL result = CreateDirectoryW(wpath, NULL);
    free(wpath);
    
    return result != FALSE;
}

// 目录删除
bool windows_directory_delete(const char* path)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    BOOL result = RemoveDirectoryW(wpath);
    free(wpath);
    
    return result != FALSE;
}

// 目录存在性检查
bool windows_directory_exists(const char* path)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    DWORD attrs = GetFileAttributesW(wpath);
    free(wpath);
    
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

// 目录打开
void* windows_directory_open(const char* path)
{
    if (!path) return NULL;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return NULL;
    
    // 构建搜索模式
    size_t wpath_len = wcslen(wpath);
    wchar_t* search_pattern = (wchar_t*)malloc((wpath_len + 3) * sizeof(wchar_t));
    if (!search_pattern)
    {
        free(wpath);
        return NULL;
    }
    
    wcscpy(search_pattern, wpath);
    if (wpath_len > 0 && search_pattern[wpath_len - 1] != L'\\')
    {
        wcscat(search_pattern, L"\\");
    }
    wcscat(search_pattern, L"*");
    
    Stru_CN_WindowsDirectoryHandle_t* dir_handle = 
        (Stru_CN_WindowsDirectoryHandle_t*)malloc(sizeof(Stru_CN_WindowsDirectoryHandle_t));
    if (!dir_handle)
    {
        free(search_pattern);
        free(wpath);
        return NULL;
    }
    
    dir_handle->handle = FindFirstFileW(search_pattern, &dir_handle->find_data);
    dir_handle->first = TRUE;
    dir_handle->path = strdup(path);
    
    free(search_pattern);
    free(wpath);
    
    if (dir_handle->handle == INVALID_HANDLE_VALUE)
    {
        free(dir_handle->path);
        free(dir_handle);
        return NULL;
    }
    
    return dir_handle;
}

// 目录读取
bool windows_directory_read(void* handle, Stru_CN_FileInfo_t* info)
{
    if (!handle || !info) return false;
    
    Stru_CN_WindowsDirectoryHandle_t* dir_handle = (Stru_CN_WindowsDirectoryHandle_t*)handle;
    
    // 跳过 "." 和 ".." 目录
    while (1)
    {
        if (dir_handle->first)
        {
            dir_handle->first = FALSE;
        }
        else
        {
            if (!FindNextFileW(dir_handle->handle, &dir_handle->find_data))
            {
                return false;
            }
        }
        
        // 检查是否是 "." 或 ".."
        if (wcscmp(dir_handle->find_data.cFileName, L".") != 0 && 
            wcscmp(dir_handle->find_data.cFileName, L"..") != 0)
        {
            break;
        }
    }
    
    // 转换文件名
    char* filename = wide_to_utf8(dir_handle->find_data.cFileName);
    if (!filename) return false;
    
    strncpy(info->name, filename, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    free(filename);
    
    // 设置文件大小
    ULARGE_INTEGER size;
    size.LowPart = dir_handle->find_data.nFileSizeLow;
    size.HighPart = dir_handle->find_data.nFileSizeHigh;
    info->size = size.QuadPart;
    
    // 设置时间戳
    info->creation_time = filetime_to_timestamp(&dir_handle->find_data.ftCreationTime);
    info->modification_time = filetime_to_timestamp(&dir_handle->find_data.ftLastWriteTime);
    info->access_time = filetime_to_timestamp(&dir_handle->find_data.ftLastAccessTime);
    
    // 设置属性
    info->attributes = windows_attrs_to_cn_attrs(dir_handle->find_data.dwFileAttributes);
    
    return true;
}

// 目录关闭
bool windows_directory_close(void* handle)
{
    if (!handle) return false;
    
    Stru_CN_WindowsDirectoryHandle_t* dir_handle = (Stru_CN_WindowsDirectoryHandle_t*)handle;
    
    BOOL result = FindClose(dir_handle->handle);
    
    free(dir_handle->path);
    free(dir_handle);
    
    return result != FALSE;
}

// 路径规范化
bool windows_path_normalize(const char* path, char* normalized, size_t size)
{
    if (!path || !normalized || size == 0) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    wchar_t* wnormalized = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
    if (!wnormalized)
    {
        free(wpath);
        return false;
    }
    
    // 获取完整路径
    DWORD result = GetFullPathNameW(wpath, MAX_PATH, wnormalized, NULL);
    
    free(wpath);
    
    if (result == 0 || result > MAX_PATH)
    {
        free(wnormalized);
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_normalized = wide_to_utf8(wnormalized);
    free(wnormalized);
    
    if (!utf8_normalized) return false;
    
    // 复制到输出缓冲区
    strncpy(normalized, utf8_normalized, size - 1);
    normalized[size - 1] = '\0';
    
    free(utf8_normalized);
    return true;
}

// 获取当前工作目录
bool windows_get_current_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0) return false;
    
    wchar_t* wbuffer = (wchar_t*)malloc(size * sizeof(wchar_t));
    if (!wbuffer) return false;
    
    DWORD result = GetCurrentDirectoryW((DWORD)size, wbuffer);
    
    if (result == 0 || result > size)
    {
        free(wbuffer);
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_buffer = wide_to_utf8(wbuffer);
    free(wbuffer);
    
    if (!utf8_buffer) return false;
    
    strncpy(buffer, utf8_buffer, size - 1);
    buffer[size - 1] = '\0';
    
    free(utf8_buffer);
    return true;
}

// 设置当前工作目录
bool windows_set_current_directory(const char* path)
{
    if (!path) return false;
    
    wchar_t* wpath = utf8_to_wide(path);
    if (!wpath) return false;
    
    BOOL result = SetCurrentDirectoryW(wpath);
    free(wpath);
    
    return result != FALSE;
}

// 获取用户主目录
bool windows_get_home_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0) return false;
    
    wchar_t wpath[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, wpath);
    
    if (FAILED(result))
    {
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_path = wide_to_utf8(wpath);
    if (!utf8_path) return false;
    
    strncpy(buffer, utf8_path, size - 1);
    buffer[size - 1] = '\0';
    
    free(utf8_path);
    return true;
}

// 获取临时目录
bool windows_get_temp_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0) return false;
    
    wchar_t wpath[MAX_PATH];
    DWORD result = GetTempPathW(MAX_PATH, wpath);
    
    if (result == 0 || result > MAX_PATH)
    {
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_path = wide_to_utf8(wpath);
    if (!utf8_path) return false;
    
    strncpy(buffer, utf8_path, size - 1);
    buffer[size - 1] = '\0';
    
    free(utf8_path);
    return true;
}

// 获取应用程序数据目录
bool windows_get_appdata_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0) return false;
    
    wchar_t wpath[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wpath);
    
    if (FAILED(result))
    {
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_path = wide_to_utf8(wpath);
    if (!utf8_path) return false;
    
    strncpy(buffer, utf8_path, size - 1);
    buffer[size - 1] = '\0';
    
    free(utf8_path);
    return true;
}

// 获取本地应用程序数据目录
bool windows_get_local_appdata_directory(char* buffer, size_t size)
{
    if (!buffer || size == 0) return false;
    
    wchar_t wpath[MAX_PATH];
    HRESULT result = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, wpath);
    
    if (FAILED(result))
    {
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_path = wide_to_utf8(wpath);
    if (!utf8_path) return false;
    
    strncpy(buffer, utf8_path, size - 1);
    buffer[size - 1] = '\0';
    
    free(utf8_path);
    return true;
}

// 检查路径是否为绝对路径
bool windows_path_is_absolute(const char* path)
{
    if (!path || strlen(path) == 0) return false;
    
    // Windows绝对路径可以是：
    // 1. 驱动器路径：C:\path
    // 2. UNC路径：\\server\share\path
    // 3. 设备路径：\\.\device\path
    
    if (strlen(path) >= 2)
    {
        // 检查驱动器路径
        if (isalpha((unsigned char)path[0]) && path[1] == ':')
        {
            return true;
        }
        
        // 检查UNC路径
        if (path[0] == '\\' && path[1] == '\\')
        {
            return true;
        }
    }
    
    return false;
}

// 获取路径的目录部分
bool windows_path_get_directory(const char* path, char* directory, size_t size)
{
    if (!path || !directory || size == 0) return false;
    
    // 查找最后一个路径分隔符
    const char* last_sep = strrchr(path, '\\');
    if (!last_sep)
    {
        last_sep = strrchr(path, '/');
    }
    
    if (!last_sep)
    {
        // 没有目录部分，返回当前目录
        return windows_get_current_directory(directory, size);
    }
    
    // 计算目录部分长度
    size_t dir_len = last_sep - path;
    if (dir_len >= size)
    {
        return false;
    }
    
    // 复制目录部分
    strncpy(directory, path, dir_len);
    directory[dir_len] = '\0';
    
    return true;
}

// 获取路径的文件名部分
bool windows_path_get_filename(const char* path, char* filename, size_t size)
{
    if (!path || !filename || size == 0) return false;
    
    // 查找最后一个路径分隔符
    const char* last_sep = strrchr(path, '\\');
    if (!last_sep)
    {
        last_sep = strrchr(path, '/');
    }
    
    const char* start = last_sep ? last_sep + 1 : path;
    
    // 计算文件名长度
    size_t name_len = strlen(start);
    if (name_len >= size)
    {
        return false;
    }
    
    // 复制文件名
    strncpy(filename, start, name_len);
    filename[name_len] = '\0';
    
    return true;
}

// 路径连接
bool windows_path_join(const char* dir, const char* file, char* result, size_t size)
{
    if (!dir || !file || !result || size == 0) return false;
    
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    
    if (dir_len + file_len + 2 >= size) // +2 用于分隔符和空字符
    {
        return false;
    }
    
    // 复制目录部分
    strcpy(result, dir);
    
    // 添加分隔符（如果需要）
    if (dir_len > 0 && dir[dir_len - 1] != '\\' && dir[dir_len - 1] != '/')
    {
        strcat(result, "\\");
    }
    
    // 添加文件名
    strcat(result, file);
    
    return true;
}

// 获取文件扩展名
bool windows_path_get_extension(const char* path, char* extension, size_t size)
{
    if (!path || !extension || size == 0) return false;
    
    // 查找最后一个点号
    const char* last_dot = strrchr(path, '.');
    if (!last_dot)
    {
        extension[0] = '\0';
        return true;
    }
    
    // 查找最后一个路径分隔符
    const char* last_sep = strrchr(path, '\\');
    if (!last_sep)
    {
        last_sep = strrchr(path, '/');
    }
    
    // 确保点号在文件名部分（在最后一个分隔符之后）
    if (last_sep && last_dot < last_sep)
    {
        extension[0] = '\0';
        return true;
    }
    
    // 计算扩展名长度
    size_t ext_len = strlen(last_dot);
    if (ext_len >= size)
    {
        return false;
    }
    
    // 复制扩展名
    strncpy(extension, last_dot, ext_len);
    extension[ext_len] = '\0';
    
    return true;
}

// 更改文件扩展名
bool windows_path_change_extension(const char* path, const char* new_ext, char* result, size_t size)
{
    if (!path || !result || size == 0) return false;
    
    // 查找最后一个点号
    const char* last_dot = strrchr(path, '.');
    
    // 查找最后一个路径分隔符
    const char* last_sep = strrchr(path, '\\');
    if (!last_sep)
    {
        last_sep = strrchr(path, '/');
    }
    
    // 确定扩展名的起始位置
    const char* ext_start = NULL;
    if (last_dot && (!last_sep || last_dot > last_sep))
    {
        ext_start = last_dot;
    }
    
    // 计算基础部分长度
    size_t base_len = ext_start ? (size_t)(ext_start - path) : strlen(path);
    
    // 计算新扩展名长度
    size_t new_ext_len = new_ext ? strlen(new_ext) : 0;
    
    // 检查缓冲区大小
    if (base_len + new_ext_len + 1 >= size) // +1 用于空字符
    {
        return false;
    }
    
    // 复制基础部分
    strncpy(result, path, base_len);
    result[base_len] = '\0';
    
    // 添加新扩展名（如果有）
    if (new_ext && new_ext_len > 0)
    {
        strcat(result, new_ext);
    }
    
    return true;
}
