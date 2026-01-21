#include "cnlang/frontend/parser.h"

#include <stdlib.h>
#include <string.h>

typedef struct CnParser {
    CnLexer *lexer;
    CnToken current;
    int has_current;
    int error_count;
} CnParser;

static void parser_advance(CnParser *parser);
static int parser_match(CnParser *parser, CnTokenKind kind);
static int parser_expect(CnParser *parser, CnTokenKind kind);

static CnAstProgram *parse_program_internal(CnParser *parser);
static CnAstFunctionDecl *parse_function_decl(CnParser *parser);
static CnAstBlockStmt *parse_block(CnParser *parser);
static CnAstStmt *parse_statement(CnParser *parser);
static CnAstExpr *parse_expression(CnParser *parser);
static CnAstExpr *parse_assignment(CnParser *parser);
static CnAstExpr *parse_logical_or(CnParser *parser);
static CnAstExpr *parse_logical_and(CnParser *parser);
static CnAstExpr *parse_comparison(CnParser *parser);
static CnAstExpr *parse_additive(CnParser *parser);
static CnAstExpr *parse_term(CnParser *parser);
static CnAstExpr *parse_unary(CnParser *parser);
static CnAstExpr *parse_factor(CnParser *parser);

static CnAstExpr *make_integer_literal(long value);
static CnAstExpr *make_identifier(const char *name, size_t length);
static CnAstExpr *make_binary(CnAstBinaryOp op, CnAstExpr *left, CnAstExpr *right);
static CnAstExpr *make_logical(CnAstLogicalOp op, CnAstExpr *left, CnAstExpr *right);
static CnAstExpr *make_unary(CnAstUnaryOp op, CnAstExpr *operand);
static CnAstExpr *make_assign(CnAstExpr *target, CnAstExpr *value);
static CnAstStmt *make_expr_stmt(CnAstExpr *expr);
static CnAstStmt *make_return_stmt(CnAstExpr *expr);
static CnAstStmt *make_if_stmt(CnAstExpr *condition, CnAstBlockStmt *then_block, CnAstBlockStmt *else_block);
static CnAstStmt *make_while_stmt(CnAstExpr *condition, CnAstBlockStmt *body);
static CnAstStmt *make_for_stmt(CnAstStmt *init, CnAstExpr *condition, CnAstExpr *update, CnAstBlockStmt *body);
static CnAstStmt *make_break_stmt(void);
static CnAstStmt *make_continue_stmt(void);
static CnAstBlockStmt *make_block(void);
static void block_add_stmt(CnAstBlockStmt *block, CnAstStmt *stmt);
static CnAstProgram *make_program(void);
static void program_add_function(CnAstProgram *program, CnAstFunctionDecl *function_decl);

CnParser *cn_frontend_parser_new(CnLexer *lexer)
{
    CnParser *parser;

    if (!lexer) {
        return NULL;
    }

    parser = (CnParser *)malloc(sizeof(CnParser));
    if (!parser) {
        return NULL;
    }

    parser->lexer = lexer;
    parser->has_current = 0;
    parser->error_count = 0;

    return parser;
}

void cn_frontend_parser_free(CnParser *parser)
{
    free(parser);
}

bool cn_frontend_parse_program(CnParser *parser, CnAstProgram **out_program)
{
    CnAstProgram *program;

    if (!parser || !out_program) {
        return false;
    }

    program = parse_program_internal(parser);
    if (!program) {
        return false;
    }

    *out_program = program;
    return parser->error_count == 0;
}

static void parser_advance(CnParser *parser)
{
    if (!parser) {
        return;
    }

    if (!cn_frontend_lexer_next_token(parser->lexer, &parser->current)) {
        parser->current.kind = CN_TOKEN_EOF;
    }

    parser->has_current = 1;
}

static int parser_match(CnParser *parser, CnTokenKind kind)
{
    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == kind) {
        parser_advance(parser);
        return 1;
    }

    return 0;
}

static int parser_expect(CnParser *parser, CnTokenKind kind)
{
    if (parser_match(parser, kind)) {
        return 1;
    }

    parser->error_count++;
    return 0;
}

static CnAstProgram *parse_program_internal(CnParser *parser)
{
    CnAstProgram *program = make_program();

    parser_advance(parser);

    while (parser->current.kind != CN_TOKEN_EOF) {
        CnAstFunctionDecl *fn = parse_function_decl(parser);
        if (!fn) {
            break;
        }
        program_add_function(program, fn);
    }

    return program;
}

static CnAstFunctionDecl *parse_function_decl(CnParser *parser)
{
    CnAstFunctionDecl *fn;
    CnAstBlockStmt *body;

    if (!parser_expect(parser, CN_TOKEN_KEYWORD_FN)) {
        return NULL;
    }

    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind != CN_TOKEN_IDENT) {
        parser->error_count++;
        return NULL;
    }

    fn = (CnAstFunctionDecl *)malloc(sizeof(CnAstFunctionDecl));
    if (!fn) {
        return NULL;
    }

    fn->name = parser->current.lexeme_begin;
    fn->name_length = parser->current.lexeme_length;
    fn->body = NULL;

    parser_advance(parser);

    parser_expect(parser, CN_TOKEN_LPAREN);
    parser_expect(parser, CN_TOKEN_RPAREN);

    body = parse_block(parser);
    fn->body = body;

    return fn;
}

static CnAstBlockStmt *parse_block(CnParser *parser)
{
    CnAstBlockStmt *block;

    if (!parser_expect(parser, CN_TOKEN_LBRACE)) {
        return NULL;
    }

    block = make_block();

    while (parser->current.kind != CN_TOKEN_RBRACE &&
           parser->current.kind != CN_TOKEN_EOF) {
        CnAstStmt *stmt = parse_statement(parser);
        if (!stmt) {
            break;
        }
        block_add_stmt(block, stmt);
    }

    parser_expect(parser, CN_TOKEN_RBRACE);

    return block;
}

static CnAstStmt *parse_statement(CnParser *parser)
{
    CnAstExpr *expr;

    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_RETURN) {
        CnAstExpr *value = NULL;

        parser_advance(parser);

        if (parser->current.kind != CN_TOKEN_SEMICOLON) {
            value = parse_expression(parser);
        }

        parser_expect(parser, CN_TOKEN_SEMICOLON);

        return make_return_stmt(value);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_IF) {
        CnAstExpr *condition;
        CnAstBlockStmt *then_block;
        CnAstBlockStmt *else_block = NULL;

        parser_advance(parser);

        parser_expect(parser, CN_TOKEN_LPAREN);
        condition = parse_expression(parser);
        parser_expect(parser, CN_TOKEN_RPAREN);

        then_block = parse_block(parser);

        if (parser->current.kind == CN_TOKEN_KEYWORD_ELSE) {
            parser_advance(parser);
            else_block = parse_block(parser);
        }

        return make_if_stmt(condition, then_block, else_block);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_WHILE) {
        CnAstExpr *condition;
        CnAstBlockStmt *body;

        parser_advance(parser);

        parser_expect(parser, CN_TOKEN_LPAREN);
        condition = parse_expression(parser);
        parser_expect(parser, CN_TOKEN_RPAREN);

        body = parse_block(parser);

        return make_while_stmt(condition, body);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_FOR) {
        CnAstStmt *init = NULL;
        CnAstExpr *condition = NULL;
        CnAstExpr *update = NULL;
        CnAstBlockStmt *body;

        parser_advance(parser);

        parser_expect(parser, CN_TOKEN_LPAREN);

        if (parser->current.kind != CN_TOKEN_SEMICOLON) {
            init = parse_statement(parser);
        } else {
            parser_advance(parser);
        }

        if (parser->current.kind != CN_TOKEN_SEMICOLON) {
            condition = parse_expression(parser);
        }
        parser_expect(parser, CN_TOKEN_SEMICOLON);

        if (parser->current.kind != CN_TOKEN_RPAREN) {
            update = parse_expression(parser);
        }
        parser_expect(parser, CN_TOKEN_RPAREN);

        body = parse_block(parser);

        return make_for_stmt(init, condition, update, body);
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_BREAK) {
        parser_advance(parser);
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        return make_break_stmt();
    }

    if (parser->current.kind == CN_TOKEN_KEYWORD_CONTINUE) {
        parser_advance(parser);
        parser_expect(parser, CN_TOKEN_SEMICOLON);
        return make_continue_stmt();
    }

    expr = parse_expression(parser);
    if (!expr) {
        return NULL;
    }

    parser_expect(parser, CN_TOKEN_SEMICOLON);

    return make_expr_stmt(expr);
}

static CnAstExpr *parse_expression(CnParser *parser)
{
    return parse_assignment(parser);
}

static CnAstExpr *parse_assignment(CnParser *parser)
{
    CnAstExpr *expr = parse_logical_or(parser);

    if (parser->current.kind == CN_TOKEN_EQUAL) {
        parser_advance(parser);
        CnAstExpr *value = parse_assignment(parser);  // 右结合
        return make_assign(expr, value);
    }

    return expr;
}

static CnAstExpr *parse_logical_or(CnParser *parser)
{
    CnAstExpr *left = parse_logical_and(parser);

    while (parser->current.kind == CN_TOKEN_LOGICAL_OR) {
        parser_advance(parser);
        CnAstExpr *right = parse_logical_and(parser);
        left = make_logical(CN_AST_LOGICAL_OP_OR, left, right);
    }

    return left;
}

static CnAstExpr *parse_logical_and(CnParser *parser)
{
    CnAstExpr *left = parse_comparison(parser);

    while (parser->current.kind == CN_TOKEN_LOGICAL_AND) {
        parser_advance(parser);
        CnAstExpr *right = parse_comparison(parser);
        left = make_logical(CN_AST_LOGICAL_OP_AND, left, right);
    }

    return left;
}

static CnAstExpr *parse_comparison(CnParser *parser)
{
    CnAstExpr *left = parse_additive(parser);

    while (parser->current.kind == CN_TOKEN_EQUAL_EQUAL ||
           parser->current.kind == CN_TOKEN_BANG_EQUAL ||
           parser->current.kind == CN_TOKEN_LESS ||
           parser->current.kind == CN_TOKEN_GREATER ||
           parser->current.kind == CN_TOKEN_LESS_EQUAL ||
           parser->current.kind == CN_TOKEN_GREATER_EQUAL) {
        CnAstBinaryOp op;

        switch (parser->current.kind) {
        case CN_TOKEN_EQUAL_EQUAL:
            op = CN_AST_BINARY_OP_EQ;
            break;
        case CN_TOKEN_BANG_EQUAL:
            op = CN_AST_BINARY_OP_NE;
            break;
        case CN_TOKEN_LESS:
            op = CN_AST_BINARY_OP_LT;
            break;
        case CN_TOKEN_GREATER:
            op = CN_AST_BINARY_OP_GT;
            break;
        case CN_TOKEN_LESS_EQUAL:
            op = CN_AST_BINARY_OP_LE;
            break;
        case CN_TOKEN_GREATER_EQUAL:
            op = CN_AST_BINARY_OP_GE;
            break;
        default:
            parser->error_count++;
            return left;
        }

        parser_advance(parser);
        left = make_binary(op, left, parse_additive(parser));
    }

    return left;
}

static CnAstExpr *parse_additive(CnParser *parser)
{
    CnAstExpr *left = parse_term(parser);

    while (parser->current.kind == CN_TOKEN_PLUS ||
           parser->current.kind == CN_TOKEN_MINUS) {
        CnAstBinaryOp op = (parser->current.kind == CN_TOKEN_PLUS)
                               ? CN_AST_BINARY_OP_ADD
                               : CN_AST_BINARY_OP_SUB;
        parser_advance(parser);
        left = make_binary(op, left, parse_term(parser));
    }

    return left;
}

static CnAstExpr *parse_term(CnParser *parser)
{
    CnAstExpr *left = parse_unary(parser);

    while (parser->current.kind == CN_TOKEN_STAR ||
           parser->current.kind == CN_TOKEN_SLASH) {
        CnAstBinaryOp op = (parser->current.kind == CN_TOKEN_STAR)
                               ? CN_AST_BINARY_OP_MUL
                               : CN_AST_BINARY_OP_DIV;
        parser_advance(parser);
        left = make_binary(op, left, parse_unary(parser));
    }

    return left;
}

static CnAstExpr *parse_unary(CnParser *parser)
{
    if (parser->current.kind == CN_TOKEN_BANG) {
        parser_advance(parser);
        CnAstExpr *operand = parse_unary(parser);  // 递归处理多个 !
        return make_unary(CN_AST_UNARY_OP_NOT, operand);
    }

    return parse_factor(parser);
}

static CnAstExpr *parse_factor(CnParser *parser)
{
    CnAstExpr *expr = NULL;

    if (!parser->has_current) {
        parser_advance(parser);
    }

    if (parser->current.kind == CN_TOKEN_INTEGER) {
        long value = strtol(parser->current.lexeme_begin, NULL, 10);
        expr = make_integer_literal(value);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_IDENT) {
        expr = make_identifier(parser->current.lexeme_begin, parser->current.lexeme_length);
        parser_advance(parser);
    } else if (parser->current.kind == CN_TOKEN_LPAREN) {
        parser_advance(parser);
        expr = parse_expression(parser);
        parser_expect(parser, CN_TOKEN_RPAREN);
    } else {
        parser->error_count++;
        parser_advance(parser);
    }

    return expr;
}

static CnAstExpr *make_integer_literal(long value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_INTEGER_LITERAL;
    expr->as.integer_literal.value = value;
    return expr;
}

static CnAstExpr *make_identifier(const char *name, size_t length)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_IDENTIFIER;
    expr->as.identifier.name = name;
    expr->as.identifier.name_length = length;
    return expr;
}

static CnAstExpr *make_binary(CnAstBinaryOp op, CnAstExpr *left, CnAstExpr *right)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_BINARY;
    expr->as.binary.op = op;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    return expr;
}

static CnAstExpr *make_assign(CnAstExpr *target, CnAstExpr *value)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_ASSIGN;
    expr->as.assign.target = target;
    expr->as.assign.value = value;
    return expr;
}

static CnAstExpr *make_logical(CnAstLogicalOp op, CnAstExpr *left, CnAstExpr *right)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_LOGICAL;
    expr->as.logical.op = op;
    expr->as.logical.left = left;
    expr->as.logical.right = right;
    return expr;
}

static CnAstExpr *make_unary(CnAstUnaryOp op, CnAstExpr *operand)
{
    CnAstExpr *expr = (CnAstExpr *)malloc(sizeof(CnAstExpr));
    if (!expr) {
        return NULL;
    }

    expr->kind = CN_AST_EXPR_UNARY;
    expr->as.unary.op = op;
    expr->as.unary.operand = operand;
    return expr;
}

static CnAstStmt *make_expr_stmt(CnAstExpr *expr)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_EXPR;
    stmt->as.expr.expr = expr;
    return stmt;
}

static CnAstStmt *make_return_stmt(CnAstExpr *expr)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_RETURN;
    stmt->as.return_stmt.expr = expr;
    return stmt;
}

static CnAstStmt *make_if_stmt(CnAstExpr *condition,
                               CnAstBlockStmt *then_block,
                               CnAstBlockStmt *else_block)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_IF;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_block = then_block;
    stmt->as.if_stmt.else_block = else_block;
    return stmt;
}

static CnAstStmt *make_while_stmt(CnAstExpr *condition, CnAstBlockStmt *body)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_WHILE;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    return stmt;
}

static CnAstStmt *make_for_stmt(CnAstStmt *init,
                                CnAstExpr *condition,
                                CnAstExpr *update,
                                CnAstBlockStmt *body)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_FOR;
    stmt->as.for_stmt.init = init;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.update = update;
    stmt->as.for_stmt.body = body;
    return stmt;
}

static CnAstStmt *make_break_stmt(void)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_BREAK;
    return stmt;
}

static CnAstStmt *make_continue_stmt(void)
{
    CnAstStmt *stmt = (CnAstStmt *)malloc(sizeof(CnAstStmt));
    if (!stmt) {
        return NULL;
    }

    stmt->kind = CN_AST_STMT_CONTINUE;
    return stmt;
}

static CnAstBlockStmt *make_block(void)
{
    CnAstBlockStmt *block = (CnAstBlockStmt *)malloc(sizeof(CnAstBlockStmt));
    if (!block) {
        return NULL;
    }

    block->stmt_count = 0;
    block->stmts = NULL;
    return block;
}

static void block_add_stmt(CnAstBlockStmt *block, CnAstStmt *stmt)
{
    size_t new_count;
    CnAstStmt **new_array;

    if (!block || !stmt) {
        return;
    }

    new_count = block->stmt_count + 1;
    new_array = (CnAstStmt **)realloc(block->stmts, new_count * sizeof(CnAstStmt *));
    if (!new_array) {
        return;
    }

    block->stmts = new_array;
    block->stmts[block->stmt_count] = stmt;
    block->stmt_count = new_count;
}

static CnAstProgram *make_program(void)
{
    CnAstProgram *program = (CnAstProgram *)malloc(sizeof(CnAstProgram));
    if (!program) {
        return NULL;
    }

    program->function_count = 0;
    program->functions = NULL;
    return program;
}

static void program_add_function(CnAstProgram *program, CnAstFunctionDecl *function_decl)
{
    size_t new_count;
    CnAstFunctionDecl **new_array;

    if (!program || !function_decl) {
        return;
    }

    new_count = program->function_count + 1;
    new_array = (CnAstFunctionDecl **)realloc(program->functions,
                                              new_count * sizeof(CnAstFunctionDecl *));
    if (!new_array) {
        return;
    }

    program->functions = new_array;
    program->functions[program->function_count] = function_decl;
    program->function_count = new_count;
}
