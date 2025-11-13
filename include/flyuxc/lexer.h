#ifndef FLYUXC_LEXER_H
#define FLYUXC_LEXER_H

#include <stddef.h>
#include <stdio.h>

/* 词法 Token 类型 */
typedef enum TokenKind {
    TK_ERROR = 0,

    /* 标识符 / 字面量 */
    TK_IDENT,
    TK_BUILTIN_FUNC,   /* 内置函数：print, length 等 */
    TK_NUM,
    TK_STRING,

    /* 符号 */
    TK_COLON,      /* : */
    TK_SEMI,       /* ; */
    TK_COMMA,      /* , */
    TK_DOT,        /* . */
    TK_DOT_CHAIN,  /* .> */
    TK_L_PAREN,    /* ( */
    TK_R_PAREN,    /* ) */
    TK_L_BRACE,    /* { */
    TK_R_BRACE,    /* } */
    TK_L_BRACKET,  /* [ */
    TK_R_BRACKET,  /* ] */

    /* 赋值、定义、函数类型注解 */
    TK_ASSIGN,          /* = */
    TK_DEFINE,          /* := */
    TK_FUNC_TYPE_START, /* :< */
    TK_FUNC_TYPE_END,   /* >=  （当前只用于函数类型结尾） */

    /* 算术运算符 */
    TK_PLUS,    /* + */
    TK_MINUS,   /* - */
    TK_STAR,    /* * */
    TK_SLASH,   /* / */
    TK_PERCENT, /* % */

    /* 比较、布尔运算符 */
    TK_LT,        /* < */
    TK_GT,        /* > */
    TK_LE,        /* <= */
    TK_GE,        /* >= （目前通常用作 FUNC_TYPE_END，不常规使用） */
    TK_EQ_EQ,     /* == */
    TK_BANG,      /* ! */
    TK_BANG_EQ,   /* != */
    TK_AND_AND,   /* && */
    TK_OR_OR,     /* || */

    /* 关键字 */
    TK_KW_IF,      /* if */
    TK_KW_LOOP,    /* L> */
    TK_KW_RETURN,  /* R> */

    /* 类型关键字 */
    TK_TYPE_NUM,   /* num */
    TK_TYPE_STR,   /* str */
    TK_TYPE_BL,    /* bl */
    TK_TYPE_OBJ,   /* obj */
    TK_TYPE_FUNC,  /* func */

    /* 布尔/特殊字面量 */
    TK_TRUE,    /* true */
    TK_FALSE,   /* false */
    TK_NULL,    /* null */
    TK_UNDEF,   /* undef */

    /* EOF 保留（当前不生成这个 token，只保留枚举值以备将来使用） */
    TK_EOF
} TokenKind;

/* 单个 Token */
typedef struct Token {
    TokenKind kind;
    char*     lexeme;   /* 原始文本片段（拷贝出来的） */
    int       line;     /* 当前是规范化+varmap 后的行号 */
    int       column;   /* 列号 */
} Token;

/* 词法分析结果 */
typedef struct LexerResult {
    Token* tokens;
    size_t count;

    char*  error_msg;   /* 如果 error_code != 0，里面是错误信息 */
    int    error_code;  /* 0 表示成功，非 0 表示出错 */
} LexerResult;

#ifdef __cplusplus
extern "C" {
#endif

/* 对映射后的源码做词法分析 */
LexerResult lexer_tokenize(const char* source);

/* 释放 LexerResult 里所有动态内存 */
void lexer_result_free(LexerResult* result);

/* 调试输出 Token 列表 */
void lexer_print_tokens(const LexerResult* result, FILE* out);

#ifdef __cplusplus
}
#endif

#endif /* FLYUXC_LEXER_H */
