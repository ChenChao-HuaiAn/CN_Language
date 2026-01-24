#include "cnlang/cli/lsp_handlers.h"
#include "cnlang/frontend/lsp_bridge.h"
#include "cnlang/cli/lsp_jsonrpc.h"

#include <stdio.h>
#include <string.h>

void cn_lsp_publish_diagnostics_for_document(
    const char *uri,
    const CnLspDocumentAnalysis *analysis)
{
    if (!uri || !analysis) {
        return;
    }

    // 简化实现：使用固定大小缓冲区构建 JSON 文本
    char buffer[65536];
    int written = 0;

    written = snprintf(
        buffer,
        sizeof(buffer),
        "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":\"%s\",\"diagnostics\":[",
        uri);

    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return;
    }

    // 追加诊断数组
    for (size_t i = 0; i < analysis->diagnostic_count; i++) {
        const CnLspDiagnostic *diag = &analysis->diagnostics[i];

        if (i > 0) {
            if (written + 1 >= (int)sizeof(buffer)) {
                return;
            }
            buffer[written++] = ',';
        }

        int n = snprintf(
            buffer + written,
            sizeof(buffer) - (size_t)written,
            "{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},\"severity\":%d,\"message\":\"%s\",\"source\":\"%s\"}",
            diag->range.start.line,
            diag->range.start.column,
            diag->range.end.line,
            diag->range.end.column,
            diag->severity,
            diag->message,
            diag->source);

        if (n < 0) {
            return;
        }
        written += n;
        if ((size_t)written >= sizeof(buffer)) {
            return;
        }
    }

    // 结束 JSON
    if (written + 4 >= (int)sizeof(buffer)) {
        return;
    }
    memcpy(buffer + written, "]}}", 4);
    written += 4;

    // 通过 JSON-RPC helper 输出带 Content-Length 头的消息
    cn_lsp_jsonrpc_write_message(stdout, buffer, (size_t)written);
}
