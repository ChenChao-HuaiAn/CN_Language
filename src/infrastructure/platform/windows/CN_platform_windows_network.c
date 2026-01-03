/******************************************************************************
 * 文件名: CN_platform_windows_network.c
 * 功能: CN_Language Windows平台网络子系统
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows网络功能
 *  2026-01-03: 重构为符合CN_platform.h接口
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief Windows套接字句柄包装
 */
typedef struct Stru_CN_WindowsSocket_t
{
    SOCKET socket;                  /**< Windows套接字 */
    int type;                       /**< 套接字类型 */
    struct sockaddr_in local_addr;  /**< 本地地址 */
    struct sockaddr_in remote_addr; /**< 远程地址 */
} Stru_CN_WindowsSocket_t;

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 初始化Winsock
 */
static bool initialize_winsock(void)
{
    static bool initialized = false;
    static WSADATA wsa_data;
    
    if (!initialized)
    {
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0)
        {
            return false;
        }
        initialized = true;
    }
    
    return true;
}

/**
 * @brief 将Windows错误码转换为CN错误码
 */
static Eum_CN_NetworkError_t windows_error_to_cn_error(int error)
{
    switch (error)
    {
        case 0:
            return Eum_NETWORK_ERROR_NONE;
        case WSAEACCES:
            return Eum_NETWORK_ERROR_ACCESS_DENIED;
        case WSAEADDRINUSE:
            return Eum_NETWORK_ERROR_ADDRESS_IN_USE;
        case WSAEADDRNOTAVAIL:
            return Eum_NETWORK_ERROR_ADDRESS_NOT_AVAILABLE;
        case WSAEAFNOSUPPORT:
            return Eum_NETWORK_ERROR_ADDRESS_FAMILY_NOT_SUPPORTED;
        case WSAEALREADY:
            return Eum_NETWORK_ERROR_ALREADY_IN_PROGRESS;
        case WSAEBADF:
            return Eum_NETWORK_ERROR_BAD_FILE_DESCRIPTOR;
        case WSAECONNABORTED:
            return Eum_NETWORK_ERROR_CONNECTION_ABORTED;
        case WSAECONNREFUSED:
            return Eum_NETWORK_ERROR_CONNECTION_REFUSED;
        case WSAECONNRESET:
            return Eum_NETWORK_ERROR_CONNECTION_RESET;
        case WSAEDESTADDRREQ:
            return Eum_NETWORK_ERROR_DESTINATION_ADDRESS_REQUIRED;
        case WSAEFAULT:
            return Eum_NETWORK_ERROR_FAULT;
        case WSAEHOSTUNREACH:
            return Eum_NETWORK_ERROR_HOST_UNREACHABLE;
        case WSAEINPROGRESS:
            return Eum_NETWORK_ERROR_IN_PROGRESS;
        case WSAEINTR:
            return Eum_NETWORK_ERROR_INTERRUPTED;
        case WSAEINVAL:
            return Eum_NETWORK_ERROR_INVALID_ARGUMENT;
        case WSAEISCONN:
            return Eum_NETWORK_ERROR_IS_CONNECTED;
        case WSAEMFILE:
            return Eum_NETWORK_ERROR_TOO_MANY_OPEN_FILES;
        case WSAEMSGSIZE:
            return Eum_NETWORK_ERROR_MESSAGE_SIZE;
        case WSAENETDOWN:
            return Eum_NETWORK_ERROR_NETWORK_DOWN;
        case WSAENETRESET:
            return Eum_NETWORK_ERROR_NETWORK_RESET;
        case WSAENETUNREACH:
            return Eum_NETWORK_ERROR_NETWORK_UNREACHABLE;
        case WSAENOBUFS:
            return Eum_NETWORK_ERROR_NO_BUFFER_SPACE;
        case WSAENOPROTOOPT:
            return Eum_NETWORK_ERROR_NO_PROTOCOL_OPTION;
        case WSAENOTCONN:
            return Eum_NETWORK_ERROR_NOT_CONNECTED;
        case WSAENOTSOCK:
            return Eum_NETWORK_ERROR_NOT_A_SOCKET;
        case WSAEOPNOTSUPP:
            return Eum_NETWORK_ERROR_OPERATION_NOT_SUPPORTED;
        case WSAEPROTONOSUPPORT:
            return Eum_NETWORK_ERROR_PROTOCOL_NOT_SUPPORTED;
        case WSAEPROTOTYPE:
            return Eum_NETWORK_ERROR_PROTOCOL_TYPE;
        case WSAETIMEDOUT:
            return Eum_NETWORK_ERROR_TIMED_OUT;
        case WSAEWOULDBLOCK:
            return Eum_NETWORK_ERROR_WOULD_BLOCK;
        default:
            return Eum_NETWORK_ERROR_UNKNOWN;
    }
}

/**
 * @brief 将CN网络地址转换为sockaddr_in
 */
static bool cn_address_to_sockaddr(const Stru_CN_NetworkAddress_t* cn_addr, struct sockaddr_in* sock_addr)
{
    if (!cn_addr || !sock_addr) return false;
    
    memset(sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = cn_addr->port;
    
    // 只支持IPv4
    if (cn_addr->family == 2) // AF_INET
    {
        memcpy(&sock_addr->sin_addr, cn_addr->data, 4);
        return true;
    }
    
    return false;
}

/**
 * @brief 将sockaddr_in转换为CN网络地址
 */
static bool sockaddr_to_cn_address(const struct sockaddr_in* sock_addr, Stru_CN_NetworkAddress_t* cn_addr)
{
    if (!sock_addr || !cn_addr) return false;
    
    memset(cn_addr, 0, sizeof(Stru_CN_NetworkAddress_t));
    cn_addr->family = sock_addr->sin_family;
    cn_addr->port = sock_addr->sin_port;
    
    if (sock_addr->sin_family == AF_INET)
    {
        memcpy(cn_addr->data, &sock_addr->sin_addr, 4);
        return true;
    }
    
    return false;
}

/**
 * @brief 将字符串转换为CN网络地址
 */
static bool string_to_cn_address(const char* str, Stru_CN_NetworkAddress_t* addr)
{
    if (!str || !addr) return false;
    
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    
    // 解析IP地址
    if (inet_pton(AF_INET, str, &sock_addr.sin_addr) != 1)
    {
        return false;
    }
    
    // 默认端口为0
    sock_addr.sin_port = 0;
    
    return sockaddr_to_cn_address(&sock_addr, addr);
}

/**
 * @brief 将CN网络地址转换为字符串
 */
static char* cn_address_to_string(const Stru_CN_NetworkAddress_t* addr)
{
    if (!addr) return NULL;
    
    struct sockaddr_in sock_addr;
    if (!cn_address_to_sockaddr(addr, &sock_addr))
    {
        return NULL;
    }
    
    char* buffer = (char*)malloc(INET_ADDRSTRLEN);
    if (!buffer) return NULL;
    
    if (inet_ntop(AF_INET, &sock_addr.sin_addr, buffer, INET_ADDRSTRLEN) == NULL)
    {
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

// ============================================================================
// 网络接口实现（符合CN_platform.h接口）
// ============================================================================

// 网络初始化
static bool windows_network_initialize(void)
{
    return initialize_winsock();
}

// 网络清理
static void windows_network_cleanup(void)
{
    WSACleanup();
}

// 地址解析：字符串到地址
static bool windows_address_from_string(const char* str, Stru_CN_NetworkAddress_t* addr)
{
    return string_to_cn_address(str, addr);
}

// 地址解析：地址到字符串
static char* windows_address_to_string(const Stru_CN_NetworkAddress_t* addr)
{
    return cn_address_to_string(addr);
}

// 创建套接字
static CN_SocketHandle_t windows_socket_create(Eum_CN_NetworkProtocol_t protocol)
{
    if (!initialize_winsock()) return NULL;
    
    int sock_type;
    int sock_protocol;
    
    // 转换协议类型
    switch (protocol)
    {
        case Eum_NET_PROTOCOL_TCP:
            sock_type = SOCK_STREAM;
            sock_protocol = IPPROTO_TCP;
            break;
        case Eum_NET_PROTOCOL_UDP:
            sock_type = SOCK_DGRAM;
            sock_protocol = IPPROTO_UDP;
            break;
        default:
            return NULL;
    }
    
    SOCKET sock = socket(AF_INET, sock_type, sock_protocol);
    if (sock == INVALID_SOCKET)
    {
        return NULL;
    }
    
    Stru_CN_WindowsSocket_t* socket_handle = 
        (Stru_CN_WindowsSocket_t*)malloc(sizeof(Stru_CN_WindowsSocket_t));
    if (!socket_handle)
    {
        closesocket(sock);
        return NULL;
    }
    
    memset(socket_handle, 0, sizeof(Stru_CN_WindowsSocket_t));
    socket_handle->socket = sock;
    socket_handle->type = sock_type;
    
    return (CN_SocketHandle_t)socket_handle;
}

// 关闭套接字
static bool windows_socket_close(CN_SocketHandle_t socket)
{
    if (!socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    int result = closesocket(socket_handle->socket);
    free(socket_handle);
    
    return result == 0;
}

// 绑定套接字
static bool windows_socket_bind(CN_SocketHandle_t socket, const Stru_CN_NetworkAddress_t* addr)
{
    if (!socket || !addr) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    struct sockaddr_in sock_addr;
    
    if (!cn_address_to_sockaddr(addr, &sock_addr))
    {
        return false;
    }
    
    int result = bind(socket_handle->socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (result == SOCKET_ERROR)
    {
        return false;
    }
    
    // 保存本地地址
    socket_handle->local_addr = sock_addr;
    return true;
}

// 监听连接
static bool windows_socket_listen(CN_SocketHandle_t socket, int backlog)
{
    if (!socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    int result = listen(socket_handle->socket, backlog);
    return result != SOCKET_ERROR;
}

// 接受连接
static CN_SocketHandle_t windows_socket_accept(CN_SocketHandle_t server_socket, 
                                               Stru_CN_NetworkAddress_t* client_addr)
{
    if (!server_socket) return NULL;
    
    Stru_CN_WindowsSocket_t* server_handle = (Stru_CN_WindowsSocket_t*)server_socket;
    
    struct sockaddr_in client_sock_addr;
    int client_addr_len = sizeof(client_sock_addr);
    
    SOCKET client_sock = accept(server_handle->socket, 
                               (struct sockaddr*)&client_sock_addr, &client_addr_len);
    if (client_sock == INVALID_SOCKET)
    {
        return NULL;
    }
    
    // 创建新的套接字句柄
    Stru_CN_WindowsSocket_t* client_handle = 
        (Stru_CN_WindowsSocket_t*)malloc(sizeof(Stru_CN_WindowsSocket_t));
    if (!client_handle)
    {
        closesocket(client_sock);
        return NULL;
    }
    
    memset(client_handle, 0, sizeof(Stru_CN_WindowsSocket_t));
    client_handle->socket = client_sock;
    client_handle->type = server_handle->type;
    
    // 获取本地地址
    struct sockaddr_in local_addr;
    int local_addr_len = sizeof(local_addr);
    getsockname(client_sock, (struct sockaddr*)&local_addr, &local_addr_len);
    client_handle->local_addr = local_addr;
    
    // 保存远程地址
    client_handle->remote_addr = client_sock_addr;
    
    // 返回客户端地址（如果调用者需要）
    if (client_addr)
    {
        sockaddr_to_cn_address(&client_sock_addr, client_addr);
    }
    
    return (CN_SocketHandle_t)client_handle;
}

// 连接到服务器
static bool windows_socket_connect(CN_SocketHandle_t socket, const Stru_CN_NetworkAddress_t* addr)
{
    if (!socket || !addr) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    struct sockaddr_in sock_addr;
    
    if (!cn_address_to_sockaddr(addr, &sock_addr))
    {
        return false;
    }
    
    int result = connect(socket_handle->socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (result == SOCKET_ERROR)
    {
        return false;
    }
    
    // 保存远程地址
    socket_handle->remote_addr = sock_addr;
    return true;
}

// 发送数据
static int32_t windows_socket_send(CN_SocketHandle_t socket, const void* data, size_t size)
{
    if (!socket || !data || size == 0) return -1;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    
    int result = send(socket_handle->socket, (const char*)data, (int)size, 0);
    if (result == SOCKET_ERROR)
    {
        return -1;
    }
    
    return result;
}

// 接收数据
static int32_t windows_socket_receive(CN_SocketHandle_t socket, void* buffer, size_t size)
{
    if (!socket || !buffer || size == 0) return -1;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    
    int result = recv(socket_handle->socket, (char*)buffer, (int)size, 0);
    if (result == SOCKET_ERROR)
    {
        return -1;
    }
    
    return result;
}

// 发送到指定地址
static int32_t windows_socket_send_to(CN_SocketHandle_t socket, const void* data, size_t size,
                                      const Stru_CN_NetworkAddress_t* addr)
{
    if (!socket || !data || !addr || size == 0) return -1;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    struct sockaddr_in sock_addr;
    
    if (!cn_address_to_sockaddr(addr, &sock_addr))
    {
        return -1;
    }
    
    int result = sendto(socket_handle->socket, (const char*)data, (int)size, 0,
                       (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (result == SOCKET_ERROR)
    {
        return -1;
    }
    
    return result;
}

// 从指定地址接收
static int32_t windows_socket_receive_from(CN_SocketHandle_t socket, void* buffer, size_t size,
                                           Stru_CN_NetworkAddress_t* addr)
{
    if (!socket || !buffer || size == 0) return -1;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    
    struct sockaddr_in src_addr;
    int src_addr_len = sizeof(src_addr);
    
    int result = recvfrom(socket_handle->socket, (char*)buffer, (int)size, 0,
                         (struct sockaddr*)&src_addr, &src_addr_len);
    if (result == SOCKET_ERROR)
    {
        return -1;
    }
    
    // 返回源地址（如果调用者需要）
    if (addr)
    {
        sockaddr_to_cn_address(&src_addr, addr);
    }
    
    return result;
}

// 设置套接字选项
static bool windows_socket_set_option(CN_SocketHandle_t socket, int option, 
                                      const void* value, size_t size)
{
    if (!socket || !value || size == 0) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    
    int result = setsockopt(socket_handle->socket, SOL_SOCKET, option,
                           (const char*)value, (int)size);
    return result == 0;
}

// 获取套接字选项
static bool windows_socket_get_option(CN_SocketHandle_t socket, int option, 
                                      void* value, size_t* size)
{
    if (!socket || !value || !size) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    int opt_len = (int)(*size);
    
    int result = getsockopt(socket_handle->socket, SOL_SOCKET, option,
                           (char*)value, &opt_len);
    if (result == 0)
    {
        *size = (size_t)opt_len;
        return true;
    }
    
    return false;
}

// 设置套接字为非阻塞模式
static bool windows_socket_set_nonblocking(CN_SocketHandle_t socket, bool nonblocking)
{
    if (!socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    unsigned long mode = nonblocking ? 1 : 0;
    
    int result = ioctlsocket(socket_handle->socket, FIONBIO, &mode);
    return result == 0;
}

// 检查套接字是否为非阻塞模式
static bool windows_socket_is_nonblocking(CN_SocketHandle_t socket)
{
    if (!socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    unsigned long mode = 0;
    
    int result = ioctlsocket(socket_handle->socket, FIONBIO, &mode);
    if (result != 0)
    {
        return false;
    }
    
    return (mode != 0);
}

// 创建选择器（简化实现，使用select）
static void* windows_selector_create(void)
{
    // 简化实现：返回一个fd_set指针
    fd_set* set = (fd_set*)malloc(sizeof(fd_set));
    if (!set) return NULL;
    
    FD_ZERO(set);
    return set;
}

// 销毁选择器
static bool windows_selector_destroy(void* selector)
{
    if (!selector) return false;
    
    free(selector);
    return true;
}

// 向选择器添加套接字
static bool windows_selector_add(void* selector, CN_SocketHandle_t socket, void* user_data)
{
    if (!selector || !socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    fd_set* set = (fd_set*)selector;
    
    FD_SET(socket_handle->socket, set);
    return true;
}

// 从选择器移除套接字
static bool windows_selector_remove(void* selector, CN_SocketHandle_t socket)
{
    if (!selector || !socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    fd_set* set = (fd_set*)selector;
    
    FD_CLR(socket_handle->socket, set);
    return true;
}

// 等待事件
static int windows_selector_wait(void* selector, uint32_t timeout_ms)
{
    if (!selector) return -1;
    
    fd_set* set = (fd_set*)selector;
    struct timeval tv;
    
    if (timeout_ms == 0xFFFFFFFF) // 无限等待
    {
        return select(0, set, NULL, NULL, NULL);
    }
    else
    {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        return select(0, set, NULL, NULL, &tv);
    }
}

// 检查套接字是否可读
static bool windows_selector_is_readable(void* selector, CN_SocketHandle_t socket)
{
    if (!selector || !socket) return false;
    
    Stru_CN_WindowsSocket_t* socket_handle = (Stru_CN_WindowsSocket_t*)socket;
    fd_set* set = (fd_set*)selector;
    
    return FD_ISSET(socket_handle->socket, set) != 0;
}

// 检查套接字是否可写
static bool windows_selector_is_writable(void* selector, CN_SocketHandle_t socket)
{
    // 简化实现：与可读相同
    return windows_selector_is_readable(selector, socket);
}

// 获取主机名
static char* windows_get_host_name(void)
{
    char buffer[256];
    
    if (gethostname(buffer, sizeof(buffer)) != 0)
    {
        return NULL;
    }
    
    char* result = (char*)malloc(strlen(buffer) + 1);
    if (!result) return NULL;
    
    strcpy(result, buffer);
    return result;
}

// 获取主机地址列表
static bool windows_get_host_addresses(Stru_CN_NetworkAddress_t* addresses, size_t* count)
{
    if (!addresses || !count || *count == 0) return false;
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        return false;
    }
    
    struct hostent* host = gethostbyname(hostname);
    if (!host || !host->h_addr_list)
    {
        return false;
    }
    
    size_t max_count = *count;
    size_t actual_count = 0;
    
    for (int i = 0; host->h_addr_list[i] != NULL && actual_count < max_count; i++)
    {
        if (host->h_addrtype == AF_INET)
        {
            struct in_addr* addr = (struct in_addr*)host->h_addr_list[i];
            Stru_CN_NetworkAddress_t* cn_addr = &addresses[actual_count];
            
            memset(cn_addr, 0, sizeof(Stru_CN_NetworkAddress_t));
            cn_addr->family = AF_INET;
            cn_addr->port = 0;
            memcpy(cn_addr->data, addr, 4);
            
            actual_count++;
        }
    }
    
    *count = actual_count;
    return actual_count > 0;
}

// ============================================================================
// 网络接口结构体
// ============================================================================

Stru_CN_NetworkInterface_t g_windows_network_interface = {
    .network_initialize = windows_network_initialize,
    .network_cleanup = windows_network_cleanup,
    .address_from_string = windows_address_from_string,
    .address_to_string = windows_address_to_string,
    .socket_create = windows_socket_create,
    .socket_close = windows_socket_close,
    .socket_bind = windows_socket_bind,
    .socket_listen = windows_socket_listen,
    .socket_accept = windows_socket_accept,
    .socket_connect = windows_socket_connect,
    .socket_send = windows_socket_send,
    .socket_receive = windows_socket_receive,
    .socket_send_to = windows_socket_send_to,
    .socket_receive_from = windows_socket_receive_from,
    .socket_set_option = windows_socket_set_option,
    .socket_get_option = windows_socket_get_option,
    .socket_set_nonblocking = windows_socket_set_nonblocking,
    .socket_is_nonblocking = windows_socket_is_nonblocking,
    .selector_create = windows_selector_create,
    .selector_destroy = windows_selector_destroy,
    .selector_add = windows_selector_add,
    .selector_remove = windows_selector_remove,
    .selector_wait = windows_selector_wait,
    .selector_is_readable = windows_selector_is_readable,
    .selector_is_writable = windows_selector_is_writable,
    .get_host_name = windows_get_host_name,
    .get_host_addresses = windows_get_host_addresses
};

// ============================================================================
// 公共接口函数
// ============================================================================

/**
 * @brief 获取Windows平台网络接口
 * 
 * @return Windows平台网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_windows_get_network_impl(void)
{
    return &g_windows_network_interface;
}
