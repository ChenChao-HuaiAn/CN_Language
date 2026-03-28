#ifndef CN_CLI_LSP_JSONRPC_H
#define CN_CLI_LSP_JSONRPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// JSON-RPC 消息最大长度（1MB），用于基本防御性检查
#define CN_LSP_JSONRPC_MAX_MESSAGE_SIZE (1024 * 1024)

// JSON-RPC 消息结构（仅保存原始 JSON 文本）
typedef struct CnLspJsonMessage {
    char *content;   // 以 '\0' 结尾的 JSON 文本
    size_t length;   // JSON 文本长度（不含终止符）
} CnLspJsonMessage;

// 从输入流中读取一条 JSON-RPC 消息
// - 解析 HTTP-style header（Content-Length）
// - 根据长度读取完整 JSON 文本
// 成功返回 true，失败或 EOF 返回 false
bool cn_lsp_jsonrpc_read_message(FILE *in, CnLspJsonMessage *out_message);

// 释放 JSON-RPC 消息占用的资源
void cn_lsp_jsonrpc_free_message(CnLspJsonMessage *message);

// 将一条 JSON 文本包装为 JSON-RPC 消息并写入输出流
// 会自动添加 Content-Length 头和空行
bool cn_lsp_jsonrpc_write_message(FILE *out, const char *json, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* CN_CLI_LSP_JSONRPC_H */
