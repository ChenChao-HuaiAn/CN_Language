#ifndef CN_CLI_LSP_HANDLERS_H
#define CN_CLI_LSP_HANDLERS_H

#include "cnlang/frontend/lsp_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

// 针对单个文档，构造并发送 textDocument/publishDiagnostics 通知
// - uri: 文档 URI
// - analysis: 文档的分析结果（包含诊断信息）
void cn_lsp_publish_diagnostics_for_document(
    const char *uri,
    const CnLspDocumentAnalysis *analysis);

#ifdef __cplusplus
}
#endif

#endif /* CN_CLI_LSP_HANDLERS_H */
