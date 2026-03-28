#ifndef CN_CLI_LSP_H
#define CN_CLI_LSP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// LSP 服务器上下文（不透明类型）
typedef struct CnLspServer CnLspServer;

// LSP 服务器初始化选项
typedef struct CnLspServerOptions {
    bool enable_semantic_tokens;  // 是否启用语义高亮
    bool enable_diagnostics;      // 是否启用诊断信息
    bool enable_definition;       // 是否启用跳转到定义
    bool enable_references;       // 是否启用查找引用
    bool enable_document_symbols; // 是否启用文档符号
} CnLspServerOptions;

// 创建 LSP 服务器实例
// 参数：
//   options: 服务器选项（NULL 表示使用默认配置）
// 返回：
//   服务器实例，失败时返回 NULL
CnLspServer *cn_lsp_server_new(const CnLspServerOptions *options);

// 销毁 LSP 服务器实例
void cn_lsp_server_free(CnLspServer *server);

// 启动 LSP 服务器主循环（阻塞调用）
// 参数：
//   server: 服务器实例
// 返回：
//   0 表示正常退出，非 0 表示错误
int cn_lsp_server_run(CnLspServer *server);

// 停止 LSP 服务器（通常由 shutdown 请求触发）
void cn_lsp_server_stop(CnLspServer *server);

// 获取默认服务器选项
CnLspServerOptions cn_lsp_server_default_options(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_CLI_LSP_H */
