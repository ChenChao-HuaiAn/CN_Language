#ifndef CN_CLI_LSP_DOCUMENT_MANAGER_H
#define CN_CLI_LSP_DOCUMENT_MANAGER_H

#include <stddef.h>
#include <stdbool.h>

#include "cnlang/frontend/lsp_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

// 文档管理器：负责缓存文档内容与分析结果
// 当前实现为简单版本，支持多文档，使用线性表存储

typedef struct CnLspDocumentManager CnLspDocumentManager;

// 创建 / 销毁文档管理器
CnLspDocumentManager *cn_lsp_document_manager_new(void);
void cn_lsp_document_manager_free(CnLspDocumentManager *manager);

// 打开或重新加载文档
// - uri: 文档唯一标识
// - text: 文本内容
// - length: 文本长度
// 调用内部桥接层执行分析并缓存结果
// 返回：true 表示成功
bool cn_lsp_document_open(
    CnLspDocumentManager *manager,
    const char *uri,
    const char *text,
    size_t length);

// 文档内容变更（当前实现等价于重新加载全文）
bool cn_lsp_document_change(
    CnLspDocumentManager *manager,
    const char *uri,
    const char *text,
    size_t length);

// 关闭文档并释放其缓存
bool cn_lsp_document_close(
    CnLspDocumentManager *manager,
    const char *uri);

// 根据 URI 获取文档的分析结果
// 成功时返回非 NULL 指针，失败返回 NULL
const CnLspDocumentAnalysis *cn_lsp_document_get_analysis(
    const CnLspDocumentManager *manager,
    const char *uri);

#ifdef __cplusplus
}
#endif

#endif /* CN_CLI_LSP_DOCUMENT_MANAGER_H */
