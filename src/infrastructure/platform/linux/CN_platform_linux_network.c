/******************************************************************************
 * 文件名：CN_platform_linux_network.c
 * 功能：Linux平台网络接口实现
 * 描述：基于BSD套接字API实现的网络接口，提供套接字操作、数据传输、
 *       地址转换、主机名解析等功能。
 * 作者：CN_Language开发团队
 * 创建日期：2026年1月3日
 * 修改历史：
 *   [2026-01-03] 初始版本创建
 * 版权：MIT许可证
 ******************************************************************************/

#include "CN_platform_linux.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* 内部数据结构定义 */

/**
 * @brief Linux套接字句柄包装结构
 */
typedef struct {
    int sockfd;                   /**< 套接字文件描述符 */
    int domain;                   /**< 协议域 */
    int type;                     /**< 套接字类型 */
    bool is_connected;            /**< 是否已连接 */
    bool is_listening;            /**< 是否在监听 */
} Stru_CN_LinuxSocketHandle_t;

/* 静态函数声明 */

static bool linux_network_initialize(void);
static void linux_network_cleanup(void);
static Stru_CN_NetworkInterface_t* linux_network_get_interface(void);

static int linux_socket_create(int domain, int type, int protocol);
static int linux_socket_close(int sockfd);
static int linux_socket_bind(int sockfd, const Stru_CN_SockAddr_t* addr, socklen_t addrlen);
static int linux_socket_listen(int sockfd, int backlog);
static int linux_socket_accept(int sockfd, Stru_CN_SockAddr_t* addr, socklen_t* addrlen);
static int linux_socket_connect(int sockfd, const Stru_CN_SockAddr_t* addr, socklen_t addrlen);

static ssize_t linux_socket_send(int sockfd, const void* buf, size_t len, int flags);
static ssize_t linux_socket_recv(int sockfd, void* buf, size_t len, int flags);
static ssize_t linux_socket_sendto(int sockfd, const void* buf, size_t len, int flags,
                                   const Stru_CN_SockAddr_t* dest_addr, socklen_t addrlen);
static ssize_t linux_socket_recvfrom(int sockfd, void* buf, size_t len, int flags,
                                     Stru_CN_SockAddr_t* src_addr, socklen_t* addrlen);

static int linux_socket_get_option(int sockfd, int level, int optname,
                                   void* optval, socklen_t* optlen);
static int linux_socket_set_option(int sockfd, int level, int optname,
                                   const void* optval, socklen_t optlen);

static int linux_inet_pton(int af, const char* src, void* dst);
static const char* linux_inet_ntop(int af, const void* src, char* dst, socklen_t size);

static int linux_gethostname(char* name, size_t len);
static void* linux_gethostbyname(const char* name);
static int linux_getaddrinfo(const char* node, const char* service,
                             const Stru_CN_AddrInfo_t* hints, Stru_CN_AddrInfo_t** res);
static void linux_freeaddrinfo(Stru_CN_AddrInfo_t* res);

/* 全局网络接口变量 */

static Stru_CN_NetworkInterface_t g_linux_network_interface = {
    .socket_create = linux_socket_create,
    .socket_close = linux_socket_close,
    .socket_bind = linux_socket_bind,
    .socket_listen = linux_socket_listen,
    .socket_accept = linux_socket_accept,
    .socket_connect = linux_socket_connect,
    .socket_send = linux_socket_send,
    .socket_recv = linux_socket_recv,
    .socket_sendto = linux_socket_sendto,
    .socket_recvfrom = linux_socket_recvfrom,
    .socket_get_option = linux_socket_get_option,
    .socket_set_option = linux_socket_set_option,
    .inet_pton = linux_inet_pton,
    .inet_ntop = linux_inet_ntop,
    .gethostname = linux_gethostname,
    .gethostbyname = linux_gethostbyname,
    .getaddrinfo = linux_getaddrinfo,
    .freeaddrinfo = linux_freeaddrinfo
};

/* 模块初始化状态 */

static bool g_network_module_initialized = false;

/******************************************************************************
 * 函数名：linux_network_initialize
 * 功能：初始化Linux网络模块
 * 参数：无
 * 返回值：初始化成功返回true，失败返回false
 ******************************************************************************/
static bool linux_network_initialize(void)
{
    if (g_network_module_initialized) {
        return true;
    }
    
    // 检查网络功能是否可用
    // 这里可以添加更详细的系统功能检测
    
    g_network_module_initialized = true;
    return true;
}

/******************************************************************************
 * 函数名：linux_network_cleanup
 * 功能：清理Linux网络模块
 * 参数：无
 * 返回值：无
 ******************************************************************************/
static void linux_network_cleanup(void)
{
    if (!g_network_module_initialized) {
        return;
    }
    
    g_network_module_initialized = false;
}

/******************************************************************************
 * 函数名：linux_network_get_interface
 * 功能：获取Linux网络接口
 * 参数：无
 * 返回值：网络接口指针
 ******************************************************************************/
static Stru_CN_NetworkInterface_t* linux_network_get_interface(void)
{
    if (!g_network_module_initialized) {
        if (!linux_network_initialize()) {
            return NULL;
        }
    }
    
    return &g_linux_network_interface;
}

/******************************************************************************
 * 套接字操作函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_socket_create
 * 功能：创建套接字
 * 参数：
 *   domain - 协议域（AF_INET, AF_INET6, AF_UNIX等）
 *   type - 套接字类型（SOCK_STREAM, SOCK_DGRAM等）
 *   protocol - 协议（0表示默认）
 * 返回值：套接字描述符，失败返回-1
 ******************************************************************************/
static int linux_socket_create(int domain, int type, int protocol)
{
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0) {
        return -1;
    }
    
    // 设置套接字为非阻塞模式（可选）
    // int flags = fcntl(sockfd, F_GETFL, 0);
    // fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    return sockfd;
}

/******************************************************************************
 * 函数名：linux_socket_close
 * 功能：关闭套接字
 * 参数：
 *   sockfd - 套接字描述符
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_socket_close(int sockfd)
{
    if (sockfd < 0) {
        return -1;
    }
    
    return close(sockfd);
}

/******************************************************************************
 * 函数名：linux_socket_bind
 * 功能：绑定套接字
 * 参数：
 *   sockfd - 套接字描述符
 *   addr - 地址结构体指针
 *   addrlen - 地址结构体长度
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_socket_bind(int sockfd, const Stru_CN_SockAddr_t* addr, socklen_t addrlen)
{
    if (sockfd < 0 || !addr) {
        return -1;
    }
    
    return bind(sockfd, (const struct sockaddr*)addr, addrlen);
}

/******************************************************************************
 * 函数名：linux_socket_listen
 * 功能：监听套接字
 * 参数：
 *   sockfd - 套接字描述符
 *   backlog - 等待连接队列的最大长度
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_socket_listen(int sockfd, int backlog)
{
    if (sockfd < 0) {
        return -1;
    }
    
    return listen(sockfd, backlog);
}

/******************************************************************************
 * 函数名：linux_socket_accept
 * 功能：接受连接
 * 参数：
 *   sockfd - 套接字描述符
 *   addr - 客户端地址结构体指针
 *   addrlen - 地址结构体长度指针
 * 返回值：新连接套接字描述符，失败返回-1
 ******************************************************************************/
static int linux_socket_accept(int sockfd, Stru_CN_SockAddr_t* addr, socklen_t* addrlen)
{
    if (sockfd < 0) {
        return -1;
    }
    
    return accept(sockfd, (struct sockaddr*)addr, addrlen);
}

/******************************************************************************
 * 函数名：linux_socket_connect
 * 功能：连接服务器
 * 参数：
 *   sockfd - 套接字描述符
 *   addr - 服务器地址结构体指针
 *   addrlen - 地址结构体长度
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_socket_connect(int sockfd, const Stru_CN_SockAddr_t* addr, socklen_t addrlen)
{
    if (sockfd < 0 || !addr) {
        return -1;
    }
    
    return connect(sockfd, (const struct sockaddr*)addr, addrlen);
}

/******************************************************************************
 * 数据传输函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_socket_send
 * 功能：发送数据
 * 参数：
 *   sockfd - 套接字描述符
 *   buf - 发送缓冲区
 *   len - 发送数据长度
 *   flags - 发送标志
 * 返回值：实际发送的字节数，失败返回-1
 ******************************************************************************/
static ssize_t linux_socket_send(int sockfd, const void* buf, size_t len, int flags)
{
    if (sockfd < 0 || !buf) {
        return -1;
    }
    
    return send(sockfd, buf, len, flags);
}

/******************************************************************************
 * 函数名：linux_socket_recv
 * 功能：接收数据
 * 参数：
 *   sockfd - 套接字描述符
 *   buf - 接收缓冲区
 *   len - 接收缓冲区长度
 *   flags - 接收标志
 * 返回值：实际接收的字节数，失败返回-1
 ******************************************************************************/
static ssize_t linux_socket_recv(int sockfd, void* buf, size_t len, int flags)
{
    if (sockfd < 0 || !buf) {
        return -1;
    }
    
    return recv(sockfd, buf, len, flags);
}

/******************************************************************************
 * 函数名：linux_socket_sendto
 * 功能：发送到指定地址
 * 参数：
 *   sockfd - 套接字描述符
 *   buf - 发送缓冲区
 *   len - 发送数据长度
 *   flags - 发送标志
 *   dest_addr - 目标地址结构体指针
 *   addrlen - 地址结构体长度
 * 返回值：实际发送的字节数，失败返回-1
 ******************************************************************************/
static ssize_t linux_socket_sendto(int sockfd, const void* buf, size_t len, int flags,
                                   const Stru_CN_SockAddr_t* dest_addr, socklen_t addrlen)
{
    if (sockfd < 0 || !buf || !dest_addr) {
        return -1;
    }
    
    return sendto(sockfd, buf, len, flags, 
                  (const struct sockaddr*)dest_addr, addrlen);
}

/******************************************************************************
 * 函数名：linux_socket_recvfrom
 * 功能：从指定地址接收
 * 参数：
 *   sockfd - 套接字描述符
 *   buf - 接收缓冲区
 *   len - 接收缓冲区长度
 *   flags - 接收标志
 *   src_addr - 源地址结构体指针
 *   addrlen - 地址结构体长度指针
 * 返回值：实际接收的字节数，失败返回-1
 ******************************************************************************/
static ssize_t linux_socket_recvfrom(int sockfd, void* buf, size_t len, int flags,
                                     Stru_CN_SockAddr_t* src_addr, socklen_t* addrlen)
{
    if (sockfd < 0 || !buf) {
        return -1;
    }
    
    return recvfrom(sockfd, buf, len, flags, 
                    (struct sockaddr*)src_addr, addrlen);
}

/******************************************************************************
 * 套接字选项函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_socket_get_option
 * 功能：获取套接字选项
 * 参数：
 *   sockfd - 套接字描述符
 *   level - 选项级别（SOL_SOCKET, IPPROTO_TCP等）
 *   optname - 选项名称
 *   optval - 选项值缓冲区
 *   optlen - 选项值长度指针
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_socket_get_option(int sockfd, int level, int optname,
                                   void* optval, socklen_t* optlen)
{
    if (sockfd < 0 || !optval || !optlen) {
        return -1;
    }
    
    return getsockopt(sockfd, level, optname, optval, optlen);
}

/******************************************************************************
 * 函数名：linux_socket_set_option
 * 功能：设置套接字选项
 * 参数：
 *   sockfd - 套接字描述符
 *   level - 选项级别
 *   optname - 选项名称
 *   optval - 选项值指针
 *   optlen - 选项值长度
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_socket_set_option(int sockfd, int level, int optname,
                                   const void* optval, socklen_t optlen)
{
    if (sockfd < 0 || !optval) {
        return -1;
    }
    
    return setsockopt(sockfd, level, optname, optval, optlen);
}

/******************************************************************************
 * 地址转换函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_inet_pton
 * 功能：将IP地址字符串转换为二进制格式
 * 参数：
 *   af - 地址族（AF_INET, AF_INET6）
 *   src - IP地址字符串
 *   dst - 二进制地址缓冲区
 * 返回值：成功返回1，失败返回0或-1
 ******************************************************************************/
static int linux_inet_pton(int af, const char* src, void* dst)
{
    if (!src || !dst) {
        return -1;
    }
    
    return inet_pton(af, src, dst);
}

/******************************************************************************
 * 函数名：linux_inet_ntop
 * 功能：将二进制IP地址转换为字符串格式
 * 参数：
 *   af - 地址族
 *   src - 二进制地址指针
 *   dst - 字符串缓冲区
 *   size - 缓冲区大小
 * 返回值：成功返回字符串指针，失败返回NULL
 ******************************************************************************/
static const char* linux_inet_ntop(int af, const void* src, char* dst, socklen_t size)
{
    if (!src || !dst) {
        return NULL;
    }
    
    return inet_ntop(af, src, dst, size);
}

/******************************************************************************
 * 主机名和地址解析函数实现
 ******************************************************************************/

/******************************************************************************
 * 函数名：linux_gethostname
 * 功能：获取主机名
 * 参数：
 *   name - 主机名缓冲区
 *   len - 缓冲区长度
 * 返回值：成功返回0，失败返回-1
 ******************************************************************************/
static int linux_gethostname(char* name, size_t len)
{
    if (!name || len == 0) {
        return -1;
    }
    
    return gethostname(name, len);
}

/******************************************************************************
 * 函数名：linux_gethostbyname
 * 功能：获取主机信息
 * 参数：
 *   name - 主机名
 * 返回值：主机信息结构体指针，失败返回NULL
 ******************************************************************************/
static void* linux_gethostbyname(const char* name)
{
    if (!name) {
        return NULL;
    }
    
    return gethostbyname(name);
}

/******************************************************************************
 * 函数名：linux_getaddrinfo
 * 功能：获取地址信息
 * 参数：
 *   node - 节点名（主机名或IP地址）
 *   service - 服务名或端口号
 *   hints - 提示结构体指针
 *   res - 结果链表指针
 * 返回值：成功返回0，失败返回错误码
 ******************************************************************************/
static int linux_getaddrinfo(const char* node, const char* service,
                             const Stru_CN_AddrInfo_t* hints, Stru_CN_AddrInfo_t** res)
{
    if (!node && !service) {
        return EAI_NONAME;
    }
    
    return getaddrinfo(node, service, (const struct addrinfo*)hints, 
                       (struct addrinfo**)res);
}

/******************************************************************************
 * 函数名：linux_freeaddrinfo
 * 功能：释放地址信息
 * 参数：
 *   res - 地址信息链表
 * 返回值：无
 ******************************************************************************/
static void linux_freeaddrinfo(Stru_CN_AddrInfo_t* res)
{
    if (!res) {
        return;
    }
    
    freeaddrinfo((struct addrinfo*)res);
}

/******************************************************************************
 * 外部接口函数
 ******************************************************************************/

/**
 * @brief 获取Linux网络接口
 * 
 * @return Stru_CN_NetworkInterface_t* 网络接口指针
 */
Stru_CN_NetworkInterface_t* CN_platform_linux_get_network(void)
{
    return linux_network_get_interface();
}

/**
 * @brief 初始化Linux网络模块
 * 
 * @return bool 初始化成功返回true，失败返回false
 */
bool CN_platform_linux_network_initialize(void)
{
    return linux_network_initialize();
}

/**
 * @brief 清理Linux网络模块
 */
void CN_platform_linux_network_cleanup(void)
{
    linux_network_cleanup();
}
