/******************************************************************************
 * 文件名: CN_platform_windows_special.c
 * 功能: CN_Language Windows平台特有功能子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows特有功能（注册表、GUI、COM）
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * @brief 将注册表路径字符串转换为HKEY根键
 */
static HKEY get_root_key_from_path(const char* path, const char** subpath)
{
    if (!path || !subpath) return NULL;
    
    *subpath = path;
    
    // 检查根键
    if (strncmp(path, "HKEY_CLASSES_ROOT\\", 18) == 0)
    {
        *subpath = path + 18;
        return HKEY_CLASSES_ROOT;
    }
    else if (strncmp(path, "HKEY_CURRENT_USER\\", 18) == 0)
    {
        *subpath = path + 18;
        return HKEY_CURRENT_USER;
    }
    else if (strncmp(path, "HKEY_LOCAL_MACHINE\\", 19) == 0)
    {
        *subpath = path + 19;
        return HKEY_LOCAL_MACHINE;
    }
    else if (strncmp(path, "HKEY_USERS\\", 11) == 0)
    {
        *subpath = path + 11;
        return HKEY_USERS;
    }
    else if (strncmp(path, "HKEY_CURRENT_CONFIG\\", 20) == 0)
    {
        *subpath = path + 20;
        return HKEY_CURRENT_CONFIG;
    }
    
    // 默认使用HKEY_CURRENT_USER
    return HKEY_CURRENT_USER;
}

// ============================================================================
// 注册表接口实现
// ============================================================================

// 打开注册表键
void* windows_reg_open_key(const char* path, uint32_t access)
{
    if (!path) return NULL;
    
    // 获取根键和子路径
    const char* subpath = NULL;
    HKEY root_key = get_root_key_from_path(path, &subpath);
    
    // 转换为宽字符
    wchar_t* wsubpath = utf8_to_wide(subpath);
    if (!wsubpath) return NULL;
    
    // 转换访问权限
    REGSAM sam = 0;
    if (access & 0x01) sam |= KEY_READ;
    if (access & 0x02) sam |= KEY_WRITE;
    if (access & 0x04) sam |= KEY_ALL_ACCESS;
    
    HKEY hKey = NULL;
    LONG result = RegOpenKeyExW(root_key, wsubpath, 0, sam, &hKey);
    
    free(wsubpath);
    
    if (result != ERROR_SUCCESS)
    {
        return NULL;
    }
    
    return (void*)hKey;
}

// 关闭注册表键
bool windows_reg_close_key(void* key)
{
    if (!key) return false;
    
    HKEY hKey = (HKEY)key;
    LONG result = RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS;
}

// 获取字符串值
bool windows_reg_get_value_string(void* key, const char* name, char* buffer, size_t size)
{
    if (!key || !buffer || size == 0) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    if (!wname) return false;
    
    wchar_t wbuffer[4096];
    DWORD buffer_size = sizeof(wbuffer);
    DWORD type = 0;
    
    LONG result = RegQueryValueExW(hKey, wname, NULL, &type, 
                                  (LPBYTE)wbuffer, &buffer_size);
    
    free(wname);
    
    if (result != ERROR_SUCCESS || type != REG_SZ)
    {
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_value = wide_to_utf8(wbuffer);
    if (!utf8_value) return false;
    
    strncpy(buffer, utf8_value, size - 1);
    buffer[size - 1] = '\0';
    
    free(utf8_value);
    return true;
}

// 获取DWORD值
bool windows_reg_get_value_dword(void* key, const char* name, uint32_t* value)
{
    if (!key || !name || !value) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    if (!wname) return false;
    
    DWORD data = 0;
    DWORD data_size = sizeof(data);
    DWORD type = 0;
    
    LONG result = RegQueryValueExW(hKey, wname, NULL, &type, 
                                  (LPBYTE)&data, &data_size);
    
    free(wname);
    
    if (result != ERROR_SUCCESS || type != REG_DWORD)
    {
        return false;
    }
    
    *value = data;
    return true;
}

// 获取二进制值
bool windows_reg_get_value_binary(void* key, const char* name, void* buffer, size_t* size)
{
    if (!key || !name || !buffer || !size || *size == 0) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    if (!wname) return false;
    
    DWORD type = 0;
    DWORD buffer_size = (DWORD)*size;
    
    LONG result = RegQueryValueExW(hKey, wname, NULL, &type, 
                                  (LPBYTE)buffer, &buffer_size);
    
    free(wname);
    
    if (result != ERROR_SUCCESS || type != REG_BINARY)
    {
        return false;
    }
    
    *size = buffer_size;
    return true;
}

// 设置字符串值
bool windows_reg_set_value_string(void* key, const char* name, const char* value)
{
    if (!key || !name || !value) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    wchar_t* wvalue = utf8_to_wide(value);
    
    if (!wname || !wvalue)
    {
        free(wname);
        free(wvalue);
        return false;
    }
    
    LONG result = RegSetValueExW(hKey, wname, 0, REG_SZ, 
                                (const BYTE*)wvalue, 
                                (DWORD)((wcslen(wvalue) + 1) * sizeof(wchar_t)));
    
    free(wname);
    free(wvalue);
    
    return result == ERROR_SUCCESS;
}

// 设置DWORD值
bool windows_reg_set_value_dword(void* key, const char* name, uint32_t value)
{
    if (!key || !name) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    if (!wname) return false;
    
    DWORD data = value;
    
    LONG result = RegSetValueExW(hKey, wname, 0, REG_DWORD, 
                                (const BYTE*)&data, sizeof(data));
    
    free(wname);
    
    return result == ERROR_SUCCESS;
}

// 设置二进制值
bool windows_reg_set_value_binary(void* key, const char* name, const void* value, size_t size)
{
    if (!key || !name || !value || size == 0) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    if (!wname) return false;
    
    LONG result = RegSetValueExW(hKey, wname, 0, REG_BINARY, 
                                (const BYTE*)value, (DWORD)size);
    
    free(wname);
    
    return result == ERROR_SUCCESS;
}

// 枚举子键
bool windows_reg_enum_keys(void* key, uint32_t index, char* name_buffer, size_t name_size)
{
    if (!key || !name_buffer || name_size == 0) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t wname[256];
    DWORD name_length = sizeof(wname) / sizeof(wchar_t);
    
    LONG result = RegEnumKeyExW(hKey, index, wname, &name_length, 
                               NULL, NULL, NULL, NULL);
    
    if (result != ERROR_SUCCESS)
    {
        return false;
    }
    
    // 转换为UTF-8
    char* utf8_name = wide_to_utf8(wname);
    if (!utf8_name) return false;
    
    strncpy(name_buffer, utf8_name, name_size - 1);
    name_buffer[name_size - 1] = '\0';
    
    free(utf8_name);
    return true;
}

// 枚举值
bool windows_reg_enum_values(void* key, uint32_t index, char* name_buffer, size_t name_size,
                            uint32_t* type, void* data_buffer, size_t* data_size)
{
    if (!key || !name_buffer || name_size == 0) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t wname[256];
    DWORD name_length = sizeof(wname) / sizeof(wchar_t);
    DWORD win_type = 0;
    DWORD buffer_size = data_buffer ? (DWORD)*data_size : 0;
    
    LONG result = RegEnumValueW(hKey, index, wname, &name_length,
                               NULL, &win_type,
                               data_buffer ? (LPBYTE)data_buffer : NULL,
                               data_buffer ? &buffer_size : NULL);
    
    if (result != ERROR_SUCCESS)
    {
        return false;
    }
    
    // 转换名称
    char* utf8_name = wide_to_utf8(wname);
    if (!utf8_name) return false;
    
    strncpy(name_buffer, utf8_name, name_size - 1);
    name_buffer[name_size - 1] = '\0';
    
    free(utf8_name);
    
    // 设置类型和数据大小
    if (type) *type = win_type;
    if (data_size) *data_size = buffer_size;
    
    return true;
}

// 创建键
bool windows_reg_create_key(const char* path)
{
    if (!path) return false;
    
    // 获取根键和子路径
    const char* subpath = NULL;
    HKEY root_key = get_root_key_from_path(path, &subpath);
    
    // 转换为宽字符
    wchar_t* wsubpath = utf8_to_wide(subpath);
    if (!wsubpath) return false;
    
    HKEY hKey = NULL;
    DWORD disposition = 0;
    
    LONG result = RegCreateKeyExW(root_key, wsubpath, 0, NULL, 0,
                                 KEY_ALL_ACCESS, NULL, &hKey, &disposition);
    
    free(wsubpath);
    
    if (result != ERROR_SUCCESS)
    {
        return false;
    }
    
    RegCloseKey(hKey);
    return true;
}

// 删除键
bool windows_reg_delete_key(const char* path)
{
    if (!path) return false;
    
    // 获取根键和子路径
    const char* subpath = NULL;
    HKEY root_key = get_root_key_from_path(path, &subpath);
    
    // 转换为宽字符
    wchar_t* wsubpath = utf8_to_wide(subpath);
    if (!wsubpath) return false;
    
    LONG result = RegDeleteKeyW(root_key, wsubpath);
    
    free(wsubpath);
    
    return result == ERROR_SUCCESS;
}

// 删除值
bool windows_reg_delete_value(void* key, const char* name)
{
    if (!key || !name) return false;
    
    HKEY hKey = (HKEY)key;
    wchar_t* wname = utf8_to_wide(name);
    if (!wname) return false;
    
    LONG result = RegDeleteValueW(hKey, wname);
    
    free(wname);
    
    return result == ERROR_SUCCESS;
}

// ============================================================================
// GUI接口实现（简化版本）
// ============================================================================

// 窗口创建
void* windows_window_create(const char* title, int width, int height)
{
    // 简化实现：返回NULL
    // 实际实现需要创建窗口类、注册窗口、创建窗口等
    return NULL;
}

// 窗口销毁
bool windows_window_destroy(void* window)
{
    // 简化实现
    return false;
}

// 窗口显示
bool windows_window_show(void* window)
{
    // 简化实现
    return false;
}

// 窗口隐藏
bool windows_window_hide(void* window)
{
    // 简化实现
    return false;
}

// 处理消息
bool windows_process_messages(void* window)
{
    // 简化实现
    return false;
}

// 设置消息处理器
void windows_set_message_handler(void* window, void (*handler)(void* msg))
{
    // 简化实现
}

// 创建按钮
void* windows_create_button(void* parent, const char* text, int x, int y, int width, int height)
{
    // 简化实现
    return NULL;
}

// 创建标签
void* windows_create_label(void* parent, const char* text, int x, int y, int width, int height)
{
    // 简化实现
    return NULL;
}

// 创建文本框
void* windows_create_textbox(void* parent, int x, int y, int width, int height)
{
    // 简化实现
    return NULL;
}

// 创建列表框
void* windows_create_listbox(void* parent, int x, int y, int width, int height)
{
    // 简化实现
    return NULL;
}

// 设置控件文本
bool windows_set_control_text(void* control, const char* text)
{
    // 简化实现
    return false;
}

// 获取控件文本
char* windows_get_control_text(void* control)
{
    // 简化实现
    return NULL;
}

// 添加列表项
bool windows_add_list_item(void* listbox, const char* item)
{
    // 简化实现
    return false;
}

// 清空列表
bool windows_clear_list(void* listbox)
{
    // 简化实现
    return false;
}

// 显示消息框
bool windows_show_message_box_gui(const char* title, const char* message, uint32_t type)
{
    // 使用系统消息框
    wchar_t* wtitle = utf8_to_wide(title);
    wchar_t* wmessage = utf8_to_wide(message);
    
    if (!wtitle || !wmessage)
    {
        free(wtitle);
        free(wmessage);
        return false;
    }
    
    UINT flags = MB_OK;
    if (type & 0x01) flags = MB_OKCANCEL;
    if (type & 0x02) flags = MB_YESNO;
    if (type & 0x04) flags |= MB_ICONINFORMATION;
    if (type & 0x08) flags |= MB_ICONWARNING;
    if (type & 0x10) flags |= MB_ICONERROR;
    if (type & 0x20) flags |= MB_ICONQUESTION;
    
    int result = MessageBoxW(NULL, wmessage, wtitle, flags);
    
    free(wtitle);
    free(wmessage);
    
    return result != 0;
}

// 显示打开文件对话框
char* windows_show_open_file_dialog(const char* title, const char* filter)
{
    // 简化实现
    return NULL;
}

// 显示保存文件对话框
char* windows_show_save_file_dialog(const char* title, const char* filter)
{
    // 简化实现
    return NULL;
}

// 显示选择文件夹对话框
char* windows_show_select_folder_dialog(const char* title)
{
    // 简化实现
    return NULL;
}

// ============================================================================
// COM接口实现（简化版本）
// ============================================================================

// COM初始化
bool windows_com_initialize(void)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    return SUCCEEDED(hr);
}

// COM反初始化
void windows_com_uninitialize(void)
{
    CoUninitialize();
}

// 创建COM实例
void* windows_com_create_instance(const char* clsid, const char* iid)
{
    // 简化实现
    return NULL;
}

// 释放COM实例
bool windows_com_release_instance(void* com_object)
{
    // 简化实现
    return false;
}

// 调用COM方法
bool windows_com_invoke_method(void* com_object, const char* method_name, 
                              void* params, uint32_t param_count, void* result)
{
    // 简化实现
    return false;
}

// 获取COM属性
bool windows_com_get_property(void* com_object, const char* property_name, void* value)
{
    // 简化实现
    return false;
}

// 设置COM属性
bool windows_com_set_property(void* com_object, const char* property_name, void* value)
{
    // 简化实现
    return false;
}

// 调用COM方法（自动化）
bool windows_com_call_method(void* com_object, const char* method_name, 
                            void* args, uint32_t arg_count, void* result)
{
    // 简化实现
    return false;
}

// ============================================================================
// 注册表接口结构体
// ============================================================================

Stru_CN_WindowsRegistryInterface_t g_windows_registry_interface = {
    .reg_open_key = windows_reg_open_key,
    .reg_close_key = windows_reg_close_key,
    .reg_get_value_string = windows_reg_get_value_string,
    .reg_get_value_dword = windows_reg_get_value_dword,
    .reg_get_value_binary = windows_reg_get_value_binary,
    .reg_set_value_string = windows_reg_set_value_string,
    .reg_set_value_dword = windows_reg_set_value_dword,
    .reg_set_value_binary = windows_reg_set_value_binary,
    .reg_enum_keys = windows_reg_enum_keys,
    .reg_enum_values = windows_reg_enum_values,
    .reg_create_key = windows_reg_create_key,
    .reg_delete_key = windows_reg_delete_key,
    .reg_delete_value = windows_reg_delete_value
};

// ============================================================================
// GUI接口结构体
// ============================================================================

Stru_CN_WindowsGUIInterface_t g_windows_gui_interface = {
    .window_create = windows_window_create,
    .window_destroy = windows_window_destroy,
    .window_show = windows_window_show,
    .window_hide = windows_window_hide,
    .process_messages = windows_process_messages,
    .set_message_handler = windows_set_message_handler,
    .create_button = windows_create_button,
    .create_label = windows_create_label,
    .create_textbox = windows_create_textbox,
    .create_listbox = windows_create_listbox,
    .set_control_text = windows_set_control_text,
    .get_control_text = windows_get_control_text,
    .add_list_item = windows_add_list_item,
    .clear_list = windows_clear_list,
    .show_message_box = windows_show_message_box_gui,
    .show_open_file_dialog = windows_show_open_file_dialog,
    .show_save_file_dialog = windows_show_save_file_dialog,
    .show_select_folder_dialog = windows_show_select_folder_dialog
};

// ============================================================================
// COM接口结构体
// ============================================================================

Stru_CN_WindowsCOMInterface_t g_windows_com_interface = {
    .com_initialize = windows_com_initialize,
    .com_uninitialize = windows_com_uninitialize,
    .com_create_instance = windows_com_create_instance,
    .com_release_instance = windows_com_release_instance,
    .com_invoke_method = windows_com_invoke_method,
    .com_get_property = windows_com_get_property,
    .com_set_property = windows_com_set_property,
    .com_call_method = windows_com_call_method
};

// ============================================================================
// 公共接口函数
// ============================================================================

/**
 * @brief 获取Windows注册表接口实现
 * 
 * @return Windows注册表接口指针
 */
Stru_CN_WindowsRegistryInterface_t* CN_platform_windows_get_registry_impl(void)
{
    return &g_windows_registry_interface;
}

/**
 * @brief 获取Windows GUI接口实现
 * 
 * @return Windows GUI接口指针
 */
Stru_CN_WindowsGUIInterface_t* CN_platform_windows_get_gui_impl(void)
{
    return &g_windows_gui_interface;
}

/**
 * @brief 获取Windows COM接口实现
 * 
 * @return Windows COM接口指针
 */
Stru_CN_WindowsCOMInterface_t* CN_platform_windows_get_com_impl(void)
{
    return &g_windows_com_interface;
}
