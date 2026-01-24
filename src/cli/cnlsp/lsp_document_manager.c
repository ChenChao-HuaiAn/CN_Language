#include "cnlang/cli/lsp_document_manager.h"

#include <stdlib.h>
#include <string.h>

struct CnLspDocumentEntry {
    char *uri;
    char *text;
    size_t length;
    CnLspDocumentAnalysis *analysis;
};

struct CnLspDocumentManager {
    struct CnLspDocumentEntry *entries;
    size_t count;
    size_t capacity;
};

static void cn_lsp_document_entry_free(struct CnLspDocumentEntry *entry)
{
    if (!entry) {
        return;
    }
    free(entry->uri);
    free(entry->text);
    if (entry->analysis) {
        cn_lsp_free_analysis(entry->analysis);
    }
    entry->uri = NULL;
    entry->text = NULL;
    entry->analysis = NULL;
    entry->length = 0;
}

static int cn_lsp_document_manager_find_index(
    const CnLspDocumentManager *manager,
    const char *uri)
{
    if (!manager || !uri) {
        return -1;
    }
    for (size_t i = 0; i < manager->count; i++) {
        if (manager->entries[i].uri && strcmp(manager->entries[i].uri, uri) == 0) {
            return (int)i;
        }
    }
    return -1;
}

CnLspDocumentManager *cn_lsp_document_manager_new(void)
{
    CnLspDocumentManager *manager = (CnLspDocumentManager *)malloc(sizeof(CnLspDocumentManager));
    if (!manager) {
        return NULL;
    }
    manager->entries = NULL;
    manager->count = 0;
    manager->capacity = 0;
    return manager;
}

void cn_lsp_document_manager_free(CnLspDocumentManager *manager)
{
    if (!manager) {
        return;
    }
    for (size_t i = 0; i < manager->count; i++) {
        cn_lsp_document_entry_free(&manager->entries[i]);
    }
    free(manager->entries);
    free(manager);
}

static bool cn_lsp_document_manager_ensure_capacity(CnLspDocumentManager *manager)
{
    if (!manager) {
        return false;
    }
    if (manager->count < manager->capacity) {
        return true;
    }
    size_t new_capacity = manager->capacity == 0 ? 4 : manager->capacity * 2;
    struct CnLspDocumentEntry *new_entries =
        (struct CnLspDocumentEntry *)realloc(manager->entries,
                                             new_capacity * sizeof(struct CnLspDocumentEntry));
    if (!new_entries) {
        return false;
    }
    // 初始化新分配的区域
    for (size_t i = manager->capacity; i < new_capacity; i++) {
        new_entries[i].uri = NULL;
        new_entries[i].text = NULL;
        new_entries[i].length = 0;
        new_entries[i].analysis = NULL;
    }
    manager->entries = new_entries;
    manager->capacity = new_capacity;
    return true;
}

bool cn_lsp_document_open(
    CnLspDocumentManager *manager,
    const char *uri,
    const char *text,
    size_t length)
{
    if (!manager || !uri || !text) {
        return false;
    }

    int index = cn_lsp_document_manager_find_index(manager, uri);
    struct CnLspDocumentEntry *entry = NULL;

    if (index >= 0) {
        // 复用已有条目
        entry = &manager->entries[index];
        cn_lsp_document_entry_free(entry);
    } else {
        if (!cn_lsp_document_manager_ensure_capacity(manager)) {
            return false;
        }
        entry = &manager->entries[manager->count++];
    }

    entry->uri = _strdup(uri);
    if (!entry->uri) {
        cn_lsp_document_entry_free(entry);
        return false;
    }

    entry->text = (char *)malloc(length + 1);
    if (!entry->text) {
        cn_lsp_document_entry_free(entry);
        return false;
    }
    memcpy(entry->text, text, length);
    entry->text[length] = '\0';
    entry->length = length;

    // 调用桥接层进行分析
    entry->analysis = cn_lsp_analyze_document(entry->text, entry->length, entry->uri);

    return entry->analysis != NULL;
}

bool cn_lsp_document_change(
    CnLspDocumentManager *manager,
    const char *uri,
    const char *text,
    size_t length)
{
    // 当前实现：全文重载，行为与 open 相同
    return cn_lsp_document_open(manager, uri, text, length);
}

bool cn_lsp_document_close(
    CnLspDocumentManager *manager,
    const char *uri)
{
    if (!manager || !uri) {
        return false;
    }

    int index = cn_lsp_document_manager_find_index(manager, uri);
    if (index < 0) {
        return false;
    }

    cn_lsp_document_entry_free(&manager->entries[index]);

    // 将最后一个条目迁移到当前空位，保持数组紧凑
    if (index != (int)(manager->count - 1)) {
        manager->entries[index] = manager->entries[manager->count - 1];
    }
    manager->count -= 1;

    return true;
}

const CnLspDocumentAnalysis *cn_lsp_document_get_analysis(
    const CnLspDocumentManager *manager,
    const char *uri)
{
    if (!manager || !uri) {
        return NULL;
    }

    int index = cn_lsp_document_manager_find_index(manager, uri);
    if (index < 0) {
        return NULL;
    }

    return manager->entries[index].analysis;
}
