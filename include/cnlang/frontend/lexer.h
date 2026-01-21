#ifndef CN_FRONTEND_LEXER_H
#define CN_FRONTEND_LEXER_H

#include <stddef.h>
#include <stdbool.h>

#include "cnlang/frontend/token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CnLexer {
    const char *source;
    const char *filename;
    size_t length;
    size_t offset;
    int line;
    int column;
} CnLexer;

void cn_frontend_lexer_init(CnLexer *lexer, const char *source, size_t length, const char *filename);
bool cn_frontend_lexer_next_token(CnLexer *lexer, CnToken *out_token);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_LEXER_H */
