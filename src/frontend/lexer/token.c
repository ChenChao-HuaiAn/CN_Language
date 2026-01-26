#include "cnlang/frontend/token.h"

#include <stdio.h>

const char *cn_frontend_token_kind_name(CnTokenKind kind)
{
    switch (kind) {
    case CN_TOKEN_INVALID:
        return "INVALID";
    case CN_TOKEN_IDENT:
        return "IDENT";
    case CN_TOKEN_INTEGER:
        return "INTEGER";
    case CN_TOKEN_FLOAT_LITERAL:
        return "FLOAT_LITERAL";
    case CN_TOKEN_STRING_LITERAL:
        return "STRING_LITERAL";
    case CN_TOKEN_KEYWORD_IF:
        return "KEYWORD_IF";
    case CN_TOKEN_KEYWORD_ELSE:
        return "KEYWORD_ELSE";
    case CN_TOKEN_KEYWORD_FN:
        return "KEYWORD_FN";
    case CN_TOKEN_KEYWORD_RETURN:
        return "KEYWORD_RETURN";
    case CN_TOKEN_KEYWORD_AND:
        return "KEYWORD_AND";
    case CN_TOKEN_KEYWORD_OR:
        return "KEYWORD_OR";
    case CN_TOKEN_KEYWORD_VAR:
        return "KEYWORD_VAR";
    case CN_TOKEN_KEYWORD_INT:
        return "KEYWORD_INT";
    case CN_TOKEN_KEYWORD_FLOAT:
        return "KEYWORD_FLOAT";
    case CN_TOKEN_KEYWORD_STRING:
        return "KEYWORD_STRING";
    case CN_TOKEN_KEYWORD_BOOL:
        return "KEYWORD_BOOL";
    case CN_TOKEN_KEYWORD_ARRAY:
        return "KEYWORD_ARRAY";
    case CN_TOKEN_KEYWORD_STRUCT:
        return "KEYWORD_STRUCT";
    case CN_TOKEN_KEYWORD_ENUM:
        return "KEYWORD_ENUM";
    case CN_TOKEN_KEYWORD_WHILE:
        return "KEYWORD_WHILE";
    case CN_TOKEN_KEYWORD_FOR:
        return "KEYWORD_FOR";
    case CN_TOKEN_KEYWORD_BREAK:
        return "KEYWORD_BREAK";
    case CN_TOKEN_KEYWORD_CONTINUE:
        return "KEYWORD_CONTINUE";
    case CN_TOKEN_KEYWORD_SWITCH:
        return "KEYWORD_SWITCH";
    case CN_TOKEN_KEYWORD_CASE:
        return "KEYWORD_CASE";
    case CN_TOKEN_KEYWORD_DEFAULT:
        return "KEYWORD_DEFAULT";
    case CN_TOKEN_KEYWORD_MAIN:
        return "KEYWORD_MAIN";
    case CN_TOKEN_KEYWORD_TRUE:
        return "KEYWORD_TRUE";
    case CN_TOKEN_KEYWORD_FALSE:
        return "KEYWORD_FALSE";
    case CN_TOKEN_KEYWORD_NULL:
        return "KEYWORD_NULL";
    case CN_TOKEN_KEYWORD_VOID:
        return "KEYWORD_VOID";
    case CN_TOKEN_KEYWORD_MODULE:
        return "KEYWORD_MODULE";
    case CN_TOKEN_KEYWORD_IMPORT:
        return "KEYWORD_IMPORT";
    case CN_TOKEN_KEYWORD_NAMESPACE:
        return "KEYWORD_NAMESPACE";
    case CN_TOKEN_KEYWORD_INTERFACE:
        return "KEYWORD_INTERFACE";
    case CN_TOKEN_KEYWORD_CLASS:
        return "KEYWORD_CLASS";
    case CN_TOKEN_KEYWORD_TEMPLATE:
        return "KEYWORD_TEMPLATE";
    case CN_TOKEN_KEYWORD_CONST:
        return "KEYWORD_CONST";
    case CN_TOKEN_KEYWORD_STATIC:
        return "KEYWORD_STATIC";
    case CN_TOKEN_KEYWORD_PUBLIC:
        return "KEYWORD_PUBLIC";
    case CN_TOKEN_KEYWORD_PRIVATE:
        return "KEYWORD_PRIVATE";
    case CN_TOKEN_KEYWORD_PROTECTED:
        return "KEYWORD_PROTECTED";
    case CN_TOKEN_KEYWORD_VIRTUAL:
        return "KEYWORD_VIRTUAL";
    case CN_TOKEN_KEYWORD_OVERRIDE:
        return "KEYWORD_OVERRIDE";
    case CN_TOKEN_KEYWORD_ABSTRACT:
        return "KEYWORD_ABSTRACT";
    case CN_TOKEN_PLUS:
        return "PLUS";
    case CN_TOKEN_PLUS_PLUS:
        return "PLUS_PLUS";
    case CN_TOKEN_MINUS:
        return "MINUS";
    case CN_TOKEN_MINUS_MINUS:
        return "MINUS_MINUS";
    case CN_TOKEN_STAR:
        return "STAR";
    case CN_TOKEN_SLASH:
        return "SLASH";
    case CN_TOKEN_PERCENT:
        return "PERCENT";
    case CN_TOKEN_EQUAL:
        return "EQUAL";
    case CN_TOKEN_EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case CN_TOKEN_BANG:
        return "BANG";
    case CN_TOKEN_BANG_EQUAL:
        return "BANG_EQUAL";
    case CN_TOKEN_LESS:
        return "LESS";
    case CN_TOKEN_LESS_EQUAL:
        return "LESS_EQUAL";
    case CN_TOKEN_GREATER:
        return "GREATER";
    case CN_TOKEN_GREATER_EQUAL:
        return "GREATER_EQUAL";
    case CN_TOKEN_LOGICAL_AND:
        return "LOGICAL_AND";
    case CN_TOKEN_LOGICAL_OR:
        return "LOGICAL_OR";
    case CN_TOKEN_AMPERSAND:
        return "AMPERSAND";
    case CN_TOKEN_LPAREN:
        return "LPAREN";
    case CN_TOKEN_RPAREN:
        return "RPAREN";
    case CN_TOKEN_LBRACE:
        return "LBRACE";
    case CN_TOKEN_RBRACE:
        return "RBRACE";
    case CN_TOKEN_LBRACKET:
        return "LBRACKET";
    case CN_TOKEN_RBRACKET:
        return "RBRACKET";
    case CN_TOKEN_SEMICOLON:
        return "SEMICOLON";
    case CN_TOKEN_COMMA:
        return "COMMA";
    case CN_TOKEN_DOT:
        return "DOT";
    case CN_TOKEN_COLON:
        return "COLON";
    case CN_TOKEN_ARROW:
        return "ARROW";
    case CN_TOKEN_EOF:
        return "EOF";
        
    default:
        return "UNKNOWN";
    }
}

void cn_frontend_token_print(const CnToken *token)
{
    const char *kind_name;

    if (token == NULL) {
        return;
    }

    kind_name = cn_frontend_token_kind_name(token->kind);

    printf("Token(%s, line=%d, column=%d)\n",
           kind_name,
           token->line,
           token->column);
}
