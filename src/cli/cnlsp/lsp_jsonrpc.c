#include "cnlang/cli/lsp_jsonrpc.h"

#include <stdlib.h>
#include <string.h>

bool cn_lsp_jsonrpc_read_message(FILE *in, CnLspJsonMessage *out_message)
{
    if (!in || !out_message) {
        return false;
    }

    char header_line[256];
    int content_length = -1;

    // 读取 header 行，直到遇到空行
    while (fgets(header_line, sizeof(header_line), in)) {
        // 空行表示 header 结束
        if (strcmp(header_line, "\r\n") == 0 || strcmp(header_line, "\n") == 0) {
            break;
        }

        // 解析 Content-Length
        if (strncmp(header_line, "Content-Length:", 15) == 0) {
            content_length = atoi(header_line + 15);
        }
    }

    if (content_length <= 0 || content_length > CN_LSP_JSONRPC_MAX_MESSAGE_SIZE) {
        return false;
    }

    char *buffer = (char *)malloc((size_t)content_length + 1);
    if (!buffer) {
        return false;
    }

    size_t total_read = 0;
    while (total_read < (size_t)content_length) {
        size_t n = fread(buffer + total_read, 1,
                         (size_t)content_length - total_read, in);
        if (n == 0) {
            free(buffer);
            return false;
        }
        total_read += n;
    }
    buffer[content_length] = '\0';

    out_message->content = buffer;
    out_message->length = (size_t)content_length;
    return true;
}

void cn_lsp_jsonrpc_free_message(CnLspJsonMessage *message)
{
    if (!message) {
        return;
    }

    free(message->content);
    message->content = NULL;
    message->length = 0;
}

bool cn_lsp_jsonrpc_write_message(FILE *out, const char *json, size_t length)
{
    if (!out || !json) {
        return false;
    }

    if (length == 0) {
        length = strlen(json);
    }

    if (length > CN_LSP_JSONRPC_MAX_MESSAGE_SIZE) {
        return false;
    }

    if (fprintf(out, "Content-Length: %zu\r\n\r\n", length) < 0) {
        return false;
    }

    if (fwrite(json, 1, length, out) != length) {
        return false;
    }

    fflush(out);
    return true;
}
