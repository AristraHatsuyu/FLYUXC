#ifndef FLYUXC_PARSER_H
#define FLYUXC_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "lexer.h"
#include "ast.h"

/* Parser状态结构 */
typedef struct Parser {
    Token *tokens;
    size_t token_count;
    size_t current;
    bool had_error;
    int error_count;     /* Total number of errors encountered */
    int warning_count;   /* Total number of warnings encountered */
    bool panic_mode;
    char *source;        /* Mapped source code */
    char *original_source; /* Original source code for error reporting */
} Parser;

/* 创建Parser实例 */
Parser *parser_create(Token *tokens, size_t count, char *source);

/* 设置原始源码（用于错误报告） */
void parser_set_original_source(Parser *p, const char *original_source);

/* 释放Parser实例 */
void parser_free(Parser *p);

/* 解析入口：返回AST根节点 */
ASTNode *parser_parse(Parser *p);

#endif /* FLYUXC_PARSER_H */
