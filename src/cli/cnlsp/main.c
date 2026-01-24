#include "cnlang/cli/lsp.h"
#include <stdio.h>
#include <stdlib.h>

// CN_Language LSP 服务器主程序
// 用法: cnlsp
// 通过 stdin/stdout 与编辑器进行 JSON-RPC 通信

int main(void)
{
    // 创建默认配置的 LSP 服务器
    CnLspServerOptions options = cn_lsp_server_default_options();
    CnLspServer *server = cn_lsp_server_new(&options);
    
    if (!server) {
        fprintf(stderr, "错误：无法创建 LSP 服务器\n");
        return 1;
    }

    // 启动服务器主循环（阻塞）
    int result = cn_lsp_server_run(server);

    // 清理资源
    cn_lsp_server_free(server);

    return result;
}
