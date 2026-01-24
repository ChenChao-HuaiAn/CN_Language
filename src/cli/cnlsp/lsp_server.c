#include "cnlang/cli/lsp.h"
#include "cnlang/frontend/lsp_bridge.h"
#include "cnlang/cli/lsp_jsonrpc.h"
#include "cnlang/cli/lsp_document_manager.h"
#include "cnlang/cli/lsp_handlers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// LSP 服务器上下文
struct CnLspServer {
    CnLspServerOptions options;
    bool is_running;
    bool is_initialized;

    CnLspDocumentManager *document_manager;
};

// 获取默认服务器选项
CnLspServerOptions cn_lsp_server_default_options(void)
{
    CnLspServerOptions options;
    options.enable_semantic_tokens = false;  // 暂不实现
    options.enable_diagnostics = true;
    options.enable_definition = false;       // 暂不实现
    options.enable_references = false;       // 暂不实现
    options.enable_document_symbols = false; // 暂不实现
    return options;
}

// 创建 LSP 服务器实例
CnLspServer *cn_lsp_server_new(const CnLspServerOptions *options)
{
    CnLspServer *server = (CnLspServer *)malloc(sizeof(CnLspServer));
    if (!server) {
        return NULL;
    }

    if (options) {
        server->options = *options;
    } else {
        server->options = cn_lsp_server_default_options();
    }

    server->is_running = false;
    server->is_initialized = false;
    server->document_manager = cn_lsp_document_manager_new();

    if (!server->document_manager) {
        free(server);
        return NULL;
    }

    return server;
}

// 销毁 LSP 服务器实例
void cn_lsp_server_free(CnLspServer *server)
{
    if (!server) {
        return;
    }

    if (server->document_manager) {
        cn_lsp_document_manager_free(server->document_manager);
    }

    free(server);
}

// 停止 LSP 服务器
void cn_lsp_server_stop(CnLspServer *server)
{
    if (server) {
        server->is_running = false;
    }
}

// 简化的 JSON 响应输出辅助函数
static void lsp_write_response(int id, const char *result)
{
    char response[8192];
    int len = snprintf(response, sizeof(response),
        "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}",
        id, result);
    
    printf("Content-Length: %d\r\n\r\n%s", len, response);
    fflush(stdout);
}

// 处理 initialize 请求
static void handle_initialize(CnLspServer *server, int id)
{
    server->is_initialized = true;
    
    const char *capabilities = 
        "{\"capabilities\":{"
        "\"textDocumentSync\":2"  // Incremental
        "}}";
    
    lsp_write_response(id, capabilities);
}

// 处理 textDocument/didOpen 通知
static void handle_did_open(CnLspServer *server, const char *uri, const char *text, size_t length)
{
    if (!cn_lsp_document_open(server->document_manager, uri, text, length)) {
        return;
    }

    const CnLspDocumentAnalysis *analysis =
        cn_lsp_document_get_analysis(server->document_manager, uri);
    if (analysis) {
        cn_lsp_publish_diagnostics_for_document(uri, analysis);
    }
}

// 处理 textDocument/didChange 通知（当前实现：全文重载）
static void handle_did_change(CnLspServer *server, const char *uri, const char *text, size_t length)
{
    if (!cn_lsp_document_change(server->document_manager, uri, text, length)) {
        return;
    }

    const CnLspDocumentAnalysis *analysis =
        cn_lsp_document_get_analysis(server->document_manager, uri);
    if (analysis) {
        cn_lsp_publish_diagnostics_for_document(uri, analysis);
    }
}

// 处理 textDocument/didClose 通知
static void handle_did_close(CnLspServer *server, const char *uri)
{
    cn_lsp_document_close(server->document_manager, uri);

    // 按 LSP 约定：关闭文档后可发送空诊断列表清除客户端错误显示
    const CnLspDocumentAnalysis empty_analysis = {0};
    cn_lsp_publish_diagnostics_for_document(uri, &empty_analysis);
}

// 简化的消息解析（仅处理最基本的 LSP 消息）
static void process_message(CnLspServer *server, const char *message)
{
    // 极简解析：检查消息类型
    if (strstr(message, "\"method\":\"initialize\"")) {
        // 提取 id
        const char *id_str = strstr(message, "\"id\":");
        int id = 0;
        if (id_str) {
            sscanf(id_str + 5, "%d", &id);
        }
        handle_initialize(server, id);
    }
    else if (strstr(message, "\"method\":\"initialized\"")) {
        // 忽略 initialized 通知
    }
    else if (strstr(message, "\"method\":\"textDocument/didOpen\"")) {
        // 极简解析 uri 和 text
        const char *uri_start = strstr(message, "\"uri\":\"");
        const char *text_start = strstr(message, "\"text\":\"");
        
        if (uri_start && text_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');
            
            text_start += 8;
            const char *text_end = strstr(text_start, "\"}");
            
            if (uri_end && text_end) {
                size_t uri_len = uri_end - uri_start;
                size_t text_len = text_end - text_start;
                
                char *uri = (char *)malloc(uri_len + 1);
                char *text = (char *)malloc(text_len + 1);
                
                if (uri && text) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    memcpy(text, text_start, text_len);
                    text[text_len] = '\0';
                    
                    handle_did_open(server, uri, text, text_len);
                }
                
                free(uri);
                free(text);
            }
        }
    }
    else if (strstr(message, "\"method\":\"textDocument/didChange\"")) {
        // 极简解析 uri 和 text（假设使用全量同步，contentChanges[0].text）
        const char *uri_start = strstr(message, "\"uri\":\"");
        const char *text_start = strstr(message, "\"text\":\"");

        if (uri_start && text_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');

            text_start += 8;
            const char *text_end = strstr(text_start, "\"}]");

            if (uri_end && text_end) {
                size_t uri_len = uri_end - uri_start;
                size_t text_len = text_end - text_start;

                char *uri = (char *)malloc(uri_len + 1);
                char *text = (char *)malloc(text_len + 1);

                if (uri && text) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    memcpy(text, text_start, text_len);
                    text[text_len] = '\0';

                    handle_did_change(server, uri, text, text_len);
                }

                free(uri);
                free(text);
            }
        }
    }
    else if (strstr(message, "\"method\":\"textDocument/didClose\"")) {
        const char *uri_start = strstr(message, "\"uri\":\"");
        if (uri_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');
            if (uri_end) {
                size_t uri_len = uri_end - uri_start;
                char *uri = (char *)malloc(uri_len + 1);
                if (uri) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    handle_did_close(server, uri);
                    free(uri);
                }
            }
        }
    }
    else if (strstr(message, "\"method\":\"shutdown\"")) {
        const char *id_str = strstr(message, "\"id\":");
        int id = 0;
        if (id_str) {
            sscanf(id_str + 5, "%d", &id);
        }
        lsp_write_response(id, "null");
        server->is_running = false;
    }
}

// 启动 LSP 服务器主循环
int cn_lsp_server_run(CnLspServer *server)
{
    if (!server) {
        return 1;
    }

    server->is_running = true;

    while (server->is_running) {
        CnLspJsonMessage message;
        if (!cn_lsp_jsonrpc_read_message(stdin, &message)) {
            break;
        }

        // 处理消息（message.content 为完整 JSON 文本）
        process_message(server, message.content);

        cn_lsp_jsonrpc_free_message(&message);
    }

    return 0;
}
