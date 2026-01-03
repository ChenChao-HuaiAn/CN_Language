/******************************************************************************
 * 文件名: CN_platform_linux_filesystem.c
 * 功能: CN_Language Linux平台文件系统子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Linux文件系统功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_linux.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 将stat结构转换为CN文件信息结构
 */
static void stat_to_file_info(const struct stat* st, const char* name, 
                              Stru_CN_FileInfo_t* info)
{
    if (!st || !info) return;
    
    // 复制文件名
    if (name)
    {
        strncpy(info->name, name, sizeof(info->name) - 1);
        info->name[sizeof(info->name) - 1] = '\0';
    }
    else
    {
        info->name[0] = '\0';
    }
    
    // 设置文件大小
    info->size = (uint64_t)st->st_size;
    
    // 设置时间（转换为毫秒）
    info->creation_time = (uint64_t)st->st_ctime * 1000ULL;
    info->modification_time = (uint64_t)st->st_mtime * 1000ULL;
    info->access_time = (uint64_t)st->st_atime * 1000ULL;
    
    // 设置文件属性
    info->attributes = 0;
    
    if (S_ISDIR(st->st_mode))
    {
        info->attributes |= Eum_FILE_ATTR_DIRECTORY;
    }
    else if (S_ISREG(st->st_mode))
    {
        info->attributes |= Eum_FILE_ATTR_NORMAL;
    }
    
    if ((st->st_mode & S_IWUSR) == 0)
    {
        info->attributes |= Eum_FILE_ATTR_READONLY;
    }
}

/**
 * @brief 打开文件句柄包装
 */
typedef struct Stru_CN_LinuxFileHandle_t
{
    int fd;                     /**< 文件描述符 */
    char* path;                 /**< 文件路径（用于调试） */
    Eum_CN_FileMode_t mode;     /**< 打开模式 */
} Stru_CN_LinuxFileHandle_t;

/**
 * @brief 目录句柄包装
 */
typedef struct Stru_CN_LinuxDirectoryHandle_t
{
    DIR* dir;                   /**< 目录流 */
    char* path;                 /**< 目录路径 */
} Stru_CN_LinuxDirectoryHandle_t;

// ============================================================================
// 文件系统接口实现
// ============================================================================

// 检查文件是否存在
bool linux_file_exists(const char* path)
{
    if (!path) return false;
    
    struct stat st;
    return (stat(path, &st) == 0);
}

// 打开文件
CN_FileHandle_t linux_file_open(const char* path, Eum_CN_FileMode_t mode)
{
    if (!path) return NULL;
    
    int flags = 0;
    mode_t permissions = 0644; // 默认权限
    
    // 根据模式设置打开标志
    switch (mode)
    {
        case Eum_FILE_MODE_READ:
            flags = O_RDONLY;
            break;
        case Eum_FILE_MODE_WRITE:
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;
        case Eum_FILE_MODE_APPEND:
            flags = O_WRONLY | O_CREAT | O_APPEND;
            break;
        case Eum_FILE_MODE_READ_WRITE:
            flags = O_RDWR | O_CREAT;
            break;
        case Eum_FILE_MODE_CREATE:
            flags = O_WRONLY | O_CREAT | O_EXCL;
            break;
        default:
            return NULL;
    }
    
    // 打开文件
    int fd = open(path, flags, permissions);
    if (fd == -1)
    {
        return NULL;
    }
    
    // 创建文件句柄包装
    Stru_CN_LinuxFileHandle_t* handle = (Stru_CN_LinuxFileHandle_t*)malloc(sizeof(Stru_CN_LinuxFileHandle_t));
    if (!handle)
    {
        close(fd);
        return NULL;
    }
    
    handle->fd = fd;
    handle->path = strdup(path);
    handle->mode = mode;
    
    return (CN_FileHandle_t)handle;
}

// 关闭文件
bool linux_file_close(CN_FileHandle_t handle)
{
    if (!handle) return false;
    
    Stru_CN_LinuxFileHandle_t* linux_handle = (Stru_CN_LinuxFileHandle_t*)handle;
    
    // 关闭文件描述符
    if (close(linux_handle->fd) != 0)
    {
        // 关闭失败，但仍然释放内存
    }
    
    // 释放资源
    if (linux_handle->path)
    {
        free(linux_handle->path);
    }
    
    free(linux_handle);
    return true;
}

// 读取文件
size_t linux_file_read(CN_FileHandle_t handle, void* buffer, size_t size)
{
    if (!handle || !buffer || size == 0) return 0;
    
    Stru_CN_LinuxFileHandle_t* linux_handle = (Stru_CN_LinuxFileHandle_t*)handle;
    
    ssize_t bytes_read = read(linux_handle->fd, buffer, size);
    if (bytes_read == -1)
    {
        return 0;
    }
    
    return (size_t)bytes_read;
}

// 写入文件
size_t linux_file_write(CN_FileHandle_t handle, const void* buffer, size_t size)
{
    if (!handle || !buffer || size == 0) return 0;
    
    Stru_CN_LinuxFileHandle_t* linux_handle = (Stru_CN_LinuxFileHandle_t*)handle;
    
    ssize_t bytes_written = write(linux_handle->fd, buffer, size);
    if (bytes_written == -1)
    {
        return 0;
    }
    
    return (size_t)bytes_written;
}

// 文件定位
bool linux_file_seek(CN_FileHandle_t handle, int64_t offset, int whence)
{
    if (!handle) return false;
    
    Stru_CN_LinuxFileHandle_t* linux_handle = (Stru_CN_LinuxFileHandle_t*)handle;
    
    int linux_whence;
    switch (whence)
    {
        case 0: // SEEK_SET
            linux_whence = SEEK_SET;
            break;
        case 1: // SEEK_CUR
            linux_whence = SEEK_CUR;
            break;
        case 2: // SEEK_END
            linux_whence = SEEK_END;
            break;
        default:
            return false;
    }
    
    off_t result = lseek(linux_handle->fd, (off_t)offset, linux_whence);
    return (result != (off_t)-1);
}

// 获取文件位置
int64_t linux_file_tell(CN_FileHandle_t handle)
{
    if (!handle) return -1;
    
    Stru_CN_LinuxFileHandle_t* linux_handle = (Stru_CN_LinuxFileHandle_t*)handle;
    
    off_t position = lseek(linux_handle->fd, 0, SEEK_CUR);
    if (position == (off_t)-1)
    {
        return -1;
    }
    
    return (int64_t)position;
}

// 获取文件信息
bool linux_file_get_info(const char* path, Stru_CN_FileInfo_t* info)
{
    if (!path || !info) return false;
    
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }
    
    // 提取文件名
    const char* filename = strrchr(path, '/');
    if (filename)
    {
        filename++; // 跳过'/'
    }
    else
    {
        filename = path;
    }
    
    stat_to_file_info(&st, filename, info);
    return true;
}

// 设置文件属性
bool linux_file_set_attributes(const char* path, uint32_t attributes)
{
    if (!path) return false;
    
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }
    
    mode_t mode = st.st_mode;
    
    // 设置只读属性
    if (attributes & Eum_FILE_ATTR_READONLY)
    {
        mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH); // 移除写权限
    }
    else
    {
        mode |= S_IWUSR; // 添加用户写权限
    }
    
    return (chmod(path, mode) == 0);
}

// 删除文件
bool linux_file_delete(const char* path)
{
    if (!path) return false;
    return (unlink(path) == 0);
}

// 重命名文件
bool linux_file_rename(const char* old_path, const char* new_path)
{
    if (!old_path || !new_path) return false;
    return (rename(old_path, new_path) == 0);
}

// 复制文件（简化实现）
bool linux_file_copy(const char* src_path, const char* dst_path)
{
    if (!src_path || !dst_path) return false;
    
    // 打开源文件
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) return false;
    
    // 创建目标文件
    int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1)
    {
        close(src_fd);
        return false;
    }
    
    // 复制数据
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    bool success = true;
    
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            success = false;
            break;
        }
    }
    
    if (bytes_read == -1)
    {
        success = false;
    }
    
    // 关闭文件
    close(src_fd);
    close(dst_fd);
    
    return success;
}

// 创建目录
bool linux_directory_create(const char* path)
{
    if (!path) return false;
    return (mkdir(path, 0755) == 0);
}

// 删除目录
bool linux_directory_delete(const char* path)
{
    if (!path) return false;
    return (rmdir(path) == 0);
}

// 检查目录是否存在
bool linux_directory_exists(const char* path)
{
    if (!path) return false;
    
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }
    
    return S_ISDIR(st.st_mode);
}

// 打开目录
void* linux_directory_open(const char* path)
{
    if (!path) return NULL;
    
    DIR* dir = opendir(path);
    if (!dir) return NULL;
    
    Stru_CN_LinuxDirectoryHandle_t* handle = (Stru_CN_LinuxDirectoryHandle_t*)malloc(sizeof(Stru_CN_LinuxDirectoryHandle_t));
    if (!handle)
    {
        closedir(dir);
        return NULL;
    }
    
    handle->dir = dir;
    handle->path = strdup(path);
    
    return handle;
}

// 读取目录条目
bool linux_directory_read(void* dir_handle, Stru_CN_FileInfo_t* info)
{
    if (!dir_handle || !info) return false;
    
    Stru_CN_LinuxDirectoryHandle_t* handle = (Stru_CN_LinuxDirectoryHandle_t*)dir_handle;
    
    struct dirent* entry = readdir(handle->dir);
    if (!entry) return false;
    
    // 跳过"."和".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
        return linux_directory_read(dir_handle, info); // 递归读取下一个
    }
    
    // 构建完整路径
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", handle->path, entry->d_name);
    
    // 获取文件信息
    struct stat st;
    if (stat(full_path, &st) != 0)
    {
        return false;
    }
    
    stat_to_file_info(&st, entry->d_name, info);
    return true;
}

// 关闭目录
bool linux_directory_close(void* dir_handle)
{
    if (!dir_handle) return false;
    
    Stru_CN_LinuxDirectoryHandle_t* handle = (Stru_CN_LinuxDirectoryHandle_t*)dir_handle;
    
    bool success = (closedir(handle->dir) == 0);
    
    if (handle->path)
    {
        free(handle->path);
    }
    
    free(handle);
    return success;
}

// 检查路径是否为绝对路径
bool linux_path_is_absolute(const char* path)
{
    if (!path || path[0] == '\0') return false;
    return (path[0] == '/');
}

// 检查路径是否为相对路径
bool linux_path_is_relative(const char* path)
{
    if (!path || path[0] == '\0') return true;
    return (path[0] != '/');
}

// 合并路径
char* linux_path_combine(const char* path1, const char* path2)
{
    if (!path1 || !path2) return NULL;
    
    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);
    
    // 分配内存
    char* result = (char*)malloc(len1 + len2 + 2); // +2 for '/' and null terminator
    if (!result) return NULL;
    
    // 复制第一部分
    strcpy(result, path1);
    
    // 添加分隔符（如果需要）
    if (len1 > 0 && path1[len1 - 1] != '/' && len2 > 0 && path2[0] != '/')
    {
        strcat(result, "/");
    }
    else if (len1 > 0 && path1[len1 - 1] == '/' && len2 > 0 && path2[0] == '/')
    {
        // 如果两边都有分隔符，移除一个
        result[len1 - 1] = '\0';
    }
    
    // 添加第二部分
    strcat(result, path2);
    
    return result;
}

// 获取目录部分
char* linux_path_get_directory(const char* path)
{
    if (!path) return NULL;
    
    const char* last_slash = strrchr(path, '/');
    if (!last_slash)
    {
        // 没有目录部分，返回当前目录
        return strdup(".");
    }
    
    size_t dir_len = last_slash - path;
    if (dir_len == 0)
    {
        // 根目录
        dir_len = 1;
    }
    
    char* result = (char*)malloc(dir_len + 1);
    if (!result) return NULL;
    
    strncpy(result, path, dir_len);
    result[dir_len] = '\0';
    
    return result;
}

// 获取文件名部分
char* linux_path_get_filename(const char* path)
{
    if (!path) return NULL;
    
    const char* last_slash = strrchr(path, '/');
    if (!last_slash)
    {
        return strdup(path);
    }
    
    return strdup(last_slash + 1);
}

// 获取文件扩展名
char* linux_path_get_extension(const char* path)
{
    if (!path) return NULL;
    
    const char* last_dot = strrchr(path, '.');
    if (!last_dot)
    {
        return strdup("");
    }
    
    // 确保点号在最后一个斜杠之后（不是目录名中的点号）
    const char* last_slash = strrchr(path, '/');
    if (last_slash && last_dot < last_slash)
    {
        return strdup("");
    }
    
    return strdup(last_dot);
}

// 获取当前工作目录
char* linux_get_current_directory(void)
{
    char* buffer = (char*)malloc(PATH_MAX);
    if (!buffer) return NULL;
    
    if (getcwd(buffer, PATH_MAX) == NULL)
    {
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

// 设置当前工作目录
bool linux_set_current_directory(const char* path)
{
    if (!path) return false;
    return (chdir(path) == 0);
}

// 获取用户主目录
char* linux_get_home_directory(void)
{
    const char* home = getenv("HOME");
    if (!home) return NULL;
    
    return strdup(home);
}

// 获取临时目录
char* linux_get_temp_directory(void)
{
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir)
    {
        tmpdir = "/tmp";
    }
    
    return strdup(tmpdir);
}

// 获取磁盘可用空间
uint64_t linux_get_disk_free_space(const char* path)
{
    if (!path) return 0;
    
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0)
    {
        return 0;
    }
    
    return (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize;
}

// 获取磁盘总空间
uint64_t linux_get_disk_total_space(const char* path)
{
    if (!path) return 0;
    
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0)
    {
        return 0;
    }
    
    return (uint64_t)vfs.f_blocks * (uint64_t)vfs.f_frsize;
}

// ============================================================================
// 文件系统接口结构体
// ============================================================================

Stru_CN_FileSystemInterface_t g_linux_filesystem_interface = {
    .file_exists = linux_file_exists,
    .file_open = linux_file_open,
    .file_close = linux_file_close,
    .file_read = linux_file_read,
    .file_write = linux_file_write,
    .file_seek = linux_file_seek,
    .file_tell = linux_file_tell,
    .file_get_info = linux_file_get_info,
    .file_set_attributes = linux_file_set_attributes,
    .file_delete = linux_file_delete,
    .file_rename = linux_file_rename,
    .file_copy = linux_file_copy,
    .directory_create = linux_directory_create,
    .directory_delete = linux_directory_delete,
    .directory_exists = linux_directory_exists,
    .directory_open = linux_directory_open,
    .directory_read = linux_directory_read,
    .directory_close = linux_directory_close,
    .path_is_absolute = linux_path_is_absolute,
    .path_is_relative = linux_path_is_relative,
    .path_combine = linux_path_combine,
    .path_get_directory = linux_path_get_directory,
    .path_get_filename = linux_path_get_filename,
    .path_get_extension = linux_path_get_extension,
    .get_current_directory = linux_get_current_directory,
    .set_current_directory = linux_set_current_directory,
    .get_home_directory = linux_get_home_directory,
    .get_temp_directory = linux_get_temp_directory,
    .get_disk_free_space = linux_get_disk_free_space,
    .get_disk_total_space = linux_get_disk_total_space
};
