#include "cnlang/cli/lsp.h"
#include "cnlang/frontend/lsp_bridge.h"
#include "cnlang/frontend/keywords.h"
#include "cnlang/cli/lsp_jsonrpc.h"
#include "cnlang/cli/lsp_document_manager.h"
#include "cnlang/cli/lsp_handlers.h"
#include "cnlang/frontend/lexer.h"
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
        "\"textDocumentSync\":2,"  /* Incremental */
        "\"definitionProvider\":true,"
        "\"referencesProvider\":true,"
        "\"completionProvider\":{\"triggerCharacters\":[]},"
        "\"semanticTokensProvider\":{"
        "\"legend\":{"
        "\"tokenTypes\":[\"keyword\",\"variable\",\"function\",\"type\"],"
        "\"tokenModifiers\":[]"
        "},"
        "\"full\":true"
        "}"
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

// 处理 textDocument/definition 请求
static void handle_definition(CnLspServer *server, int id, const char *uri, int line, int character)
{
    if (!server || !uri) {
        return;
    }

    CnLspDocumentAnalysis *analysis = (CnLspDocumentAnalysis *)cn_lsp_document_get_analysis(
        server->document_manager, uri);
    CnLspSymbolInfo symbol_info;

    char response[1024];

    if (!analysis) {
        snprintf(response, sizeof(response), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":null}", id);
        cn_lsp_jsonrpc_write_message(stdout, response, 0);
        return;
    }

    CnLspPosition pos;
    pos.line = line;
    pos.column = character;

    if (cn_lsp_find_definition(analysis, pos, &symbol_info)) {
        int len = snprintf(
            response,
            sizeof(response),
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}]}",
            id,
            uri,
            symbol_info.definition_range.start.line,
            symbol_info.definition_range.start.column,
            symbol_info.definition_range.end.line,
            symbol_info.definition_range.end.column);
        if (len > 0) {
            cn_lsp_jsonrpc_write_message(stdout, response, (size_t)len);
        }
    } else {
        int len = snprintf(
            response,
            sizeof(response),
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":null}",
            id);
        if (len > 0) {
            cn_lsp_jsonrpc_write_message(stdout, response, (size_t)len);
        }
    }
}

// 处理 textDocument/references 请求
static void handle_references(CnLspServer *server, int id, const char *uri, int line, int character)
{
    if (!server || !uri) {
        return;
    }

    CnLspDocumentAnalysis *analysis = (CnLspDocumentAnalysis *)cn_lsp_document_get_analysis(
        server->document_manager, uri);

    char *json = NULL;

    if (!analysis) {
        char response[128];
        int len = snprintf(response, sizeof(response),
                           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[]}", id);
        if (len > 0) {
            cn_lsp_jsonrpc_write_message(stdout, response, (size_t)len);
        }
        return;
    }

    CnLspPosition pos;
    pos.line = line;
    pos.column = character;

    CnLspRange *ranges = NULL;
    size_t count = 0;

    if (!cn_lsp_find_references(analysis, pos, &ranges, &count) || count == 0) {
        free(ranges);
        char response[128];
        int len = snprintf(response, sizeof(response),
                           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[]}", id);
        if (len > 0) {
            cn_lsp_jsonrpc_write_message(stdout, response, (size_t)len);
        }
        return;
    }

    // 构造 JSON 数组
    size_t capacity = 1024;
    json = (char *)malloc(capacity);
    if (!json) {
        free(ranges);
        return;
    }

    int written = snprintf(json, capacity,
                           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":[",
                           id);
    if (written < 0) {
        free(json);
        free(ranges);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        const CnLspRange *r = &ranges[i];
        if ((size_t)written + 256 >= capacity) {
            capacity *= 2;
            char *new_json = (char *)realloc(json, capacity);
            if (!new_json) {
                free(json);
                free(ranges);
                return;
            }
            json = new_json;
        }

        if (i > 0) {
            json[written++] = ',';
        }

        int n = snprintf(json + written, capacity - (size_t)written,
                         "{\"uri\":\"%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}",
                         uri,
                         r->start.line, r->start.column,
                         r->end.line, r->end.column);
        if (n < 0) {
            free(json);
            free(ranges);
            return;
        }
        written += n;
    }

    if ((size_t)written + 2 >= capacity) {
        capacity += 2;
        char *new_json = (char *)realloc(json, capacity);
        if (!new_json) {
            free(json);
            free(ranges);
            return;
        }
        json = new_json;
    }

    json[written++] = ']';
    json[written++] = '}';

    cn_lsp_jsonrpc_write_message(stdout, json, (size_t)written);

    free(json);
    free(ranges);
}

// 处理 textDocument/semanticTokens/full 请求
static void handle_semantic_tokens_full(CnLspServer *server, int id, const char *uri)
{
    if (!server || !uri) {
        return;
    }

    CnLspDocumentAnalysis *analysis = (CnLspDocumentAnalysis *)cn_lsp_document_get_analysis(
        server->document_manager, uri);

    if (!analysis || !analysis->source) {
        char response[256];
        int len = snprintf(response, sizeof(response),
                           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"data\":[]}}",
                           id);
        if (len > 0) {
            cn_lsp_jsonrpc_write_message(stdout, response, (size_t)len);
        }
        return;
    }

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, analysis->source, analysis->source_length, uri);

    CnToken token;
    size_t data_capacity = 128;
    size_t data_count = 0;
    int *data = (int *)malloc(data_capacity * sizeof(int));
    if (!data) {
        return;
    }

    int last_line = 0;
    int last_start = 0;
    int has_prev = 0;

    while (cn_frontend_lexer_next_token(&lexer, &token)) {
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }

        int token_type = -1;
        if (token.kind == CN_TOKEN_IDENT) {
            token_type = 1; // variable
        } else if (token.kind >= CN_TOKEN_KEYWORD_IF && token.kind <= CN_TOKEN_KEYWORD_VOID) {
            token_type = 0; // keyword
        }

        if (token_type < 0) {
            continue;
        }

        int line0 = (token.line > 0) ? token.line - 1 : 0;
        int col0 = (token.column > 0) ? token.column - 1 : 0;
        int length = (int)token.lexeme_length;

        int delta_line = has_prev ? (line0 - last_line) : line0;
        int delta_start = (delta_line == 0 && has_prev) ? (col0 - last_start) : col0;

        if (data_count + 5 > data_capacity) {
            data_capacity *= 2;
            int *new_data = (int *)realloc(data, data_capacity * sizeof(int));
            if (!new_data) {
                free(data);
                return;
            }
            data = new_data;
        }

        data[data_count++] = delta_line;
        data[data_count++] = delta_start;
        data[data_count++] = length;
        data[data_count++] = token_type;
        data[data_count++] = 0; // modifiers

        last_line = line0;
        last_start = col0;
        has_prev = 1;
    }

    size_t json_capacity = 256 + data_count * 12;
    char *json = (char *)malloc(json_capacity);
    if (!json) {
        free(data);
        return;
    }

    int written = snprintf(json, json_capacity,
                           "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"data\":[",
                           id);
    if (written < 0) {
        free(json);
        free(data);
        return;
    }

    for (size_t i = 0; i < data_count; i++) {
        if ((size_t)written + 16 >= json_capacity) {
            json_capacity *= 2;
            char *new_json = (char *)realloc(json, json_capacity);
            if (!new_json) {
                free(json);
                free(data);
                return;
            }
            json = new_json;
        }

        if (i > 0) {
            json[written++] = ',';
        }

        int n = snprintf(json + written, json_capacity - (size_t)written, "%d", data[i]);
        if (n < 0) {
            free(json);
            free(data);
            return;
        }
        written += n;
    }

    if ((size_t)written + 4 >= json_capacity) {
        json_capacity += 4;
        char *new_json = (char *)realloc(json, json_capacity);
        if (!new_json) {
            free(json);
            free(data);
            return;
        }
        json = new_json;
    }

    memcpy(json + written, "]}}", 4);
    written += 4;

    cn_lsp_jsonrpc_write_message(stdout, json, (size_t)written);

    free(json);
    free(data);
}

// 处理 textDocument/completion 请求
static void handle_completion(CnLspServer *server, int id, const char *uri, int line, int character)
{
    if (!server || !uri) {
        return;
    }

    // 使用统一的关键字定义
    size_t keyword_count = 0;
    const CnKeywordEntry *keywords = cn_frontend_get_keywords(&keyword_count);

    // 构建 JSON 响应
    size_t json_capacity = 4096;
    char *json = (char *)malloc(json_capacity);
    if (!json) {
        return;
    }

    int written = snprintf(json, json_capacity,
        "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"isIncomplete\":false,\"items\":[",
        id);
    if (written < 0) {
        free(json);
        return;
    }

    // 添加关键字补全项
    for (size_t i = 0; i < keyword_count; i++) {
        if (i > 0) {
            if ((size_t)written + 1 >= json_capacity) {
                json_capacity *= 2;
                char *new_json = (char *)realloc(json, json_capacity);
                if (!new_json) {
                    free(json);
                    return;
                }
                json = new_json;
            }
            json[written++] = ',';
        }

        // 使用关键字表中的分类信息
        int kind = 14; // Keyword
        const char *detail = keywords[i].category;

        // 构建补全项 JSON
        int n = snprintf(json + written, json_capacity - (size_t)written,
            "{\"label\":\"%s\",\"kind\":%d,\"detail\":\"%s\",\"insertText\":\"%s\"}",
            keywords[i].text, kind, detail, keywords[i].text);
        
        if (n < 0) {
            free(json);
            return;
        }
        written += n;
        if ((size_t)written >= json_capacity) {
            json_capacity *= 2;
            char *new_json = (char *)realloc(json, json_capacity);
            if (!new_json) {
                free(json);
                return;
            }
            json = new_json;
        }
    }

    // 结束 JSON
    if ((size_t)written + 4 >= json_capacity) {
        json_capacity += 4;
        char *new_json = (char *)realloc(json, json_capacity);
        if (!new_json) {
            free(json);
            return;
        }
        json = new_json;
    }
    memcpy(json + written, "]}}", 4);
    written += 4;

    cn_lsp_jsonrpc_write_message(stdout, json, (size_t)written);
    free(json);
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
    else if (strstr(message, "\"method\":\"textDocument/definition\"")) {
        const char *id_str = strstr(message, "\"id\":");
        const char *uri_start = strstr(message, "\"uri\":\"");
        const char *line_start = strstr(message, "\"line\":");
        const char *char_start = strstr(message, "\"character\":");
        int id = 0;
        int line = 0;
        int character = 0;
        if (id_str) {
            sscanf(id_str + 5, "%d", &id);
        }
        if (line_start) {
            sscanf(line_start + 8, "%d", &line);
        }
        if (char_start) {
            sscanf(char_start + 12, "%d", &character);
        }
        if (uri_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');
            if (uri_end) {
                size_t uri_len = uri_end - uri_start;
                char *uri = (char *)malloc(uri_len + 1);
                if (uri) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    handle_definition(server, id, uri, line, character);
                    free(uri);
                }
            }
        }
    }
    else if (strstr(message, "\"method\":\"textDocument/references\"")) {
        const char *id_str = strstr(message, "\"id\":");
        const char *uri_start = strstr(message, "\"uri\":\"");
        const char *line_start = strstr(message, "\"line\":");
        const char *char_start = strstr(message, "\"character\":");
        int id = 0;
        int line = 0;
        int character = 0;
        if (id_str) {
            sscanf(id_str + 5, "%d", &id);
        }
        if (line_start) {
            sscanf(line_start + 8, "%d", &line);
        }
        if (char_start) {
            sscanf(char_start + 12, "%d", &character);
        }
        if (uri_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');
            if (uri_end) {
                size_t uri_len = uri_end - uri_start;
                char *uri = (char *)malloc(uri_len + 1);
                if (uri) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    handle_references(server, id, uri, line, character);
                    free(uri);
                }
            }
        }
    }
    else if (strstr(message, "\"method\":\"textDocument/semanticTokens/full\"")) {
        const char *id_str = strstr(message, "\"id\":");
        const char *uri_start = strstr(message, "\"uri\":\"");
        int id = 0;
        if (id_str) {
            sscanf(id_str + 5, "%d", &id);
        }
        if (uri_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');
            if (uri_end) {
                size_t uri_len = uri_end - uri_start;
                char *uri = (char *)malloc(uri_len + 1);
                if (uri) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    handle_semantic_tokens_full(server, id, uri);
                    free(uri);
                }
            }
        }
    }
    else if (strstr(message, "\"method\":\"textDocument/completion\"")) {
        const char *id_str = strstr(message, "\"id\":");
        const char *uri_start = strstr(message, "\"uri\":\"");
        const char *line_start = strstr(message, "\"line\":");
        const char *char_start = strstr(message, "\"character\":");
        int id = 0;
        int line = 0;
        int character = 0;
        if (id_str) {
            sscanf(id_str + 5, "%d", &id);
        }
        if (line_start) {
            sscanf(line_start + 8, "%d", &line);
        }
        if (char_start) {
            sscanf(char_start + 12, "%d", &character);
        }
        if (uri_start) {
            uri_start += 7;
            const char *uri_end = strchr(uri_start, '"');
            if (uri_end) {
                size_t uri_len = uri_end - uri_start;
                char *uri = (char *)malloc(uri_len + 1);
                if (uri) {
                    memcpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    handle_completion(server, id, uri, line, character);
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
