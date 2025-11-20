#ifndef FLYUXC_AST_H
#define FLYUXC_AST_H

#include <stddef.h>
#include <stdbool.h>
#include "lexer.h"
#include "normalize.h"

/* ============================================================================
 * AST (Abstract Syntax Tree) 节点定义
 * 
 * FLYUX语言的抽象语法树节点类型和数据结构定义
 * ============================================================================ */

/* AST节点类型枚举 */
typedef enum ASTNodeKind {
    /* 程序根节点 */
    AST_PROGRAM,
    
    /* ===== 语句 (Statements) ===== */
    AST_VAR_DECL,        /* 变量声明: x := 123 或 x :[num]= 123 */
    AST_CONST_DECL,      /* 常量声明: X :(num)= 123 */
    AST_FUNC_DECL,       /* 函数声明: f := (a,b) { ... } */
    AST_EXPR_STMT,       /* 表达式语句: a + b; */
    AST_ASSIGN_STMT,     /* 赋值语句: x = 456 */
    AST_IF_STMT,         /* if语句 */
    AST_LOOP_STMT,       /* L>循环语句 */
    AST_RETURN_STMT,     /* R>返回语句 */
    AST_TRY_STMT,        /* T>异常处理语句 */
    AST_BLOCK,           /* 代码块: { ... } */
    
    /* ===== 表达式 (Expressions) ===== */
    AST_BINARY_EXPR,     /* 二元表达式: a + b */
    AST_UNARY_EXPR,      /* 一元表达式: !a, -b */
    AST_CALL_EXPR,       /* 函数调用: f(a, b) */
    AST_MEMBER_EXPR,     /* 成员访问: obj.prop */
    AST_INDEX_EXPR,      /* 索引访问: arr[0] */
    AST_CHAIN_EXPR,      /* 链式调用: obj.>method */
    
    /* ===== 字面量 (Literals) ===== */
    AST_NUM_LITERAL,     /* 数字: 123, 3.14 */
    AST_STRING_LITERAL,  /* 字符串: "hello" */
    AST_BOOL_LITERAL,    /* 布尔: true, false */
    AST_NULL_LITERAL,    /* null */
    AST_UNDEF_LITERAL,   /* undef */
    AST_ARRAY_LITERAL,   /* 数组: [1, 2, 3] */
    AST_OBJECT_LITERAL,  /* 对象: {a: 1, b: 2} */
    
    /* ===== 其他 ===== */
    AST_IDENTIFIER,      /* 标识符: x, foo, 🐶 */
    AST_TYPE_ANNOTATION  /* 类型标注: :[num], :(str) */
} ASTNodeKind;

/* 循环类型 */
typedef enum LoopType {
    LOOP_REPEAT,    /* L> [n] { ... } */
    LOOP_FOR,       /* L> (init; cond; update) { ... } */
    LOOP_FOREACH    /* L> (arr : item) { ... } */
} LoopType;

/* 前置声明 */
typedef struct ASTNode ASTNode;

/* ============================================================================
 * AST节点基类
 * ============================================================================ */

struct ASTNode {
    ASTNodeKind kind;
    SourceLocation loc;  /* 源码位置信息 */
    void *data;          /* 指向具体节点数据的指针 */
};

/* ============================================================================
 * 程序根节点
 * ============================================================================ */

typedef struct ASTProgram {
    ASTNode **statements;  /* 顶层语句数组 */
    size_t stmt_count;     /* 语句数量 */
} ASTProgram;

/* ============================================================================
 * 语句节点
 * ============================================================================ */

/* 变量声明: x := 123 或 x :[num]= 123 */
typedef struct ASTVarDecl {
    char *name;                   /* 变量名 */
    ASTNode *type_annotation;     /* 类型标注（可为NULL表示类型推断） */
    bool is_const;                /* 是否为常量（括号标注） */
    ASTNode *init_expr;           /* 初始化表达式（可为NULL） */
} ASTVarDecl;

/* 函数声明: f := (a, b) { ... } 或 f :<num>= (a, b) { ... } */
typedef struct ASTFuncDecl {
    char *name;              /* 函数名 */
    char **params;           /* 参数名数组 */
    size_t param_count;      /* 参数数量 */
    ASTNode *return_type;    /* 返回类型标注（可为NULL） */
    ASTNode *body;           /* 函数体（AST_BLOCK） */
} ASTFuncDecl;

/* 赋值语句: x = 456 */
typedef struct ASTAssignStmt {
    ASTNode *target;         /* 赋值目标（IDENTIFIER/MEMBER_EXPR/INDEX_EXPR） */
    ASTNode *value;          /* 赋值的值 */
} ASTAssignStmt;

/* if语句: if (cond) { ... } { ... } */
typedef struct ASTIfStmt {
    ASTNode **conditions;    /* 条件表达式数组（支持多条件） */
    ASTNode **then_blocks;   /* 对应的then块数组 */
    ASTNode *else_block;     /* else块（可为NULL） */
    size_t cond_count;       /* 条件数量 */
} ASTIfStmt;

/* L>循环语句 */
typedef struct ASTLoopStmt {
    LoopType loop_type;
    
    union {
        /* LOOP_REPEAT: L> [n] { ... } */
        ASTNode *repeat_count;
        
        /* LOOP_FOR: L> (init; cond; update) { ... } */
        struct {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *update;
        } for_loop;
        
        /* LOOP_FOREACH: L> (arr : item) { ... } */
        struct {
            ASTNode *iterable;
            char *item_var;
        } foreach_loop;
    } loop_data;
    
    ASTNode *body;           /* 循环体 */
} ASTLoopStmt;

/* R>返回语句: R> value 或 R> */
typedef struct ASTReturnStmt {
    ASTNode *value;          /* 返回值（可为NULL表示返回undef） */
} ASTReturnStmt;

/* T>异常处理: T> { try } (error) { catch } { finally } */
typedef struct ASTTryStmt {
    ASTNode *try_block;      /* try代码块（必需） */
    char *catch_param;       /* catch参数名（可为NULL） */
    ASTNode *catch_block;    /* catch代码块（可为NULL） */
    ASTNode *finally_block;  /* finally代码块（可为NULL） */
} ASTTryStmt;

/* 代码块: { stmt1; stmt2; ... } */
typedef struct ASTBlock {
    ASTNode **statements;
    size_t stmt_count;
} ASTBlock;

/* 表达式语句: expr; */
typedef struct ASTExprStmt {
    ASTNode *expr;
} ASTExprStmt;

/* ============================================================================
 * 表达式节点
 * ============================================================================ */

/* 二元表达式: a + b, a && b, a < b */
typedef struct ASTBinaryExpr {
    TokenKind op;            /* 运算符类型 */
    ASTNode *left;
    ASTNode *right;
} ASTBinaryExpr;

/* 一元表达式: !a, -b, +c */
typedef struct ASTUnaryExpr {
    TokenKind op;            /* 运算符类型（TK_BANG, TK_MINUS, TK_PLUS, TK_PLUS_PLUS, TK_MINUS_MINUS） */
    ASTNode *operand;
    bool is_postfix;         /* true: i++, false: ++i */
} ASTUnaryExpr;

/* 函数调用: f(a, b, c) 或 f(a, b, c)! */
typedef struct ASTCallExpr {
    ASTNode *callee;         /* 被调用的函数（通常是IDENTIFIER） */
    ASTNode **args;          /* 参数数组 */
    size_t arg_count;        /* 参数数量 */
    int throw_on_error;      /* ! 后缀：1表示出错抛异常，0表示出错返回带类型的null */
} ASTCallExpr;

/* 成员访问: obj.prop */
typedef struct ASTMemberExpr {
    ASTNode *object;         /* 对象 */
    char *property;          /* 属性名 */
    bool is_computed;        /* false表示点访问，true表示[]访问 */
} ASTMemberExpr;

/* 索引访问: arr[i] */
typedef struct ASTIndexExpr {
    ASTNode *object;
    ASTNode *index;
} ASTIndexExpr;

/* 链式调用元素 */
typedef struct ChainElement {
    char *method_name;       /* 方法名 */
    ASTNode **args;          /* 参数数组（不包括第一个参数） */
    size_t arg_count;
} ChainElement;

/* 链式调用: obj.>method.>call(x) */
typedef struct ASTChainExpr {
    ASTNode *object;         /* 起始对象 */
    ChainElement *chain;     /* 链式调用数组 */
    size_t chain_count;
} ASTChainExpr;

/* ============================================================================
 * 字面量节点
 * ============================================================================ */

/* 数字字面量: 123, 3.14, 1.5e10 */
typedef struct ASTNumLiteral {
    double value;            /* 统一用double存储 */
    char *raw;               /* 原始字符串（保留用于输出） */
} ASTNumLiteral;

/* 字符串字面量: "hello" */
typedef struct ASTStringLiteral {
    char *value;
    size_t length;  /* 字符串实际长度（支持包含\0的字符串） */
} ASTStringLiteral;

/* 布尔字面量: true, false */
typedef struct ASTBoolLiteral {
    bool value;
} ASTBoolLiteral;

/* 数组字面量: [1, 2, 3] */
typedef struct ASTArrayLiteral {
    ASTNode **elements;
    size_t elem_count;
} ASTArrayLiteral;

/* 对象属性 */
typedef struct ASTObjectProperty {
    char *key;               /* 属性键 */
    ASTNode *value;          /* 属性值 */
} ASTObjectProperty;

/* 对象字面量: {a: 1, b: 2} */
typedef struct ASTObjectLiteral {
    ASTObjectProperty *properties;
    size_t prop_count;
} ASTObjectLiteral;

/* 标识符: x, foo, 🐶 */
typedef struct ASTIdentifier {
    char *name;
} ASTIdentifier;

/* 类型标注: :[num], :(str), :<func> */
typedef struct ASTTypeAnnotation {
    TokenKind type_token;    /* TK_TYPE_NUM, TK_TYPE_STR等 */
    bool is_const;           /* true表示():常量, false表示[]:变量 */
} ASTTypeAnnotation;

/* ============================================================================
 * AST辅助函数
 * ============================================================================ */

/* 创建AST节点 */
ASTNode *ast_node_create(ASTNodeKind kind, SourceLocation loc);

/* 释放AST节点（递归释放） */
void ast_node_free(ASTNode *node);

/* 打印AST（用于调试） */
void ast_print(ASTNode *node, int indent);

/* 获取节点类型名称（用于调试） */
const char *ast_kind_name(ASTNodeKind kind);

/* ============================================================================
 * 特定节点创建函数
 * ============================================================================ */

/* 创建程序节点 */
ASTNode *ast_program_create(ASTNode **statements, size_t count, SourceLocation loc);

/* 创建变量声明节点 */
ASTNode *ast_var_decl_create(char *name, ASTNode *type_ann, bool is_const, 
                              ASTNode *init, SourceLocation loc);

/* 创建函数声明节点 */
ASTNode *ast_func_decl_create(char *name, char **params, size_t param_count,
                               ASTNode *return_type, ASTNode *body, SourceLocation loc);

/* 创建赋值语句节点 */
ASTNode *ast_assign_stmt_create(ASTNode *target, ASTNode *value, SourceLocation loc);

/* 创建if语句节点 */
ASTNode *ast_if_stmt_create(ASTNode **conditions, ASTNode **then_blocks,
                             size_t cond_count, ASTNode *else_block, SourceLocation loc);

/* 创建循环语句节点 */
ASTNode *ast_loop_stmt_create(LoopType type, ASTNode *body, SourceLocation loc);

/* 创建返回语句节点 */
ASTNode *ast_return_stmt_create(ASTNode *value, SourceLocation loc);

/* 创建异常处理节点 */
ASTNode *ast_try_stmt_create(ASTNode *try_block, char *catch_param, 
                              ASTNode *catch_block, ASTNode *finally_block, SourceLocation loc);

/* 创建代码块节点 */
ASTNode *ast_block_create(ASTNode **statements, size_t count, SourceLocation loc);

/* 创建表达式语句节点 */
ASTNode *ast_expr_stmt_create(ASTNode *expr, SourceLocation loc);

/* 创建二元表达式节点 */
ASTNode *ast_binary_expr_create(TokenKind op, ASTNode *left, ASTNode *right, 
                                 SourceLocation loc);

/* 创建一元表达式节点 */
ASTNode *ast_unary_expr_create(TokenKind op, ASTNode *operand, SourceLocation loc);

/* 创建函数调用节点 */
ASTNode *ast_call_expr_create(ASTNode *callee, ASTNode **args, size_t arg_count,
                               int throw_on_error, SourceLocation loc);

/* 创建成员访问节点 */
ASTNode *ast_member_expr_create(ASTNode *object, char *property, bool is_computed,
                                 SourceLocation loc);

/* 创建索引访问节点 */
ASTNode *ast_index_expr_create(ASTNode *object, ASTNode *index, SourceLocation loc);

/* 创建链式调用节点 */
ASTNode *ast_chain_expr_create(ASTNode *object, ChainElement *chain, size_t chain_count,
                                SourceLocation loc);

/* 创建数字字面量节点 */
ASTNode *ast_num_literal_create(double value, char *raw, SourceLocation loc);

/* 创建字符串字面量节点 */
ASTNode *ast_string_literal_create(char *value, size_t length, SourceLocation loc);

/* 创建布尔字面量节点 */
ASTNode *ast_bool_literal_create(bool value, SourceLocation loc);

/* 创建null字面量节点 */
ASTNode *ast_null_literal_create(SourceLocation loc);

/* 创建undef字面量节点 */
ASTNode *ast_undef_literal_create(SourceLocation loc);

/* 创建数组字面量节点 */
ASTNode *ast_array_literal_create(ASTNode **elements, size_t count, SourceLocation loc);

/* 创建对象字面量节点 */
ASTNode *ast_object_literal_create(ASTObjectProperty *properties, size_t count,
                                    SourceLocation loc);

/* 创建标识符节点 */
ASTNode *ast_identifier_create(char *name, SourceLocation loc);

/* 创建类型标注节点 */
ASTNode *ast_type_annotation_create(TokenKind type_token, bool is_const, 
                                     SourceLocation loc);

/* 创建一元表达式节点 */
ASTNode *ast_unary_expr_create(TokenKind op, ASTNode *operand, SourceLocation loc);

/* 创建成员访问表达式节点 */
ASTNode *ast_member_expr_create(ASTNode *object, char *property, bool is_computed,
                                 SourceLocation loc);

/* 创建索引访问表达式节点 */
ASTNode *ast_index_expr_create(ASTNode *object, ASTNode *index, SourceLocation loc);

/* 创建循环语句节点 */
ASTNode *ast_loop_stmt_create(LoopType type, ASTNode *body, SourceLocation loc);

/* 创建for循环节点 */
ASTNode *ast_for_loop_create(ASTNode *init, ASTNode *cond, ASTNode *update, 
                              ASTNode *body, SourceLocation loc);

/* 创建重复循环节点 */
ASTNode *ast_repeat_loop_create(ASTNode *count_expr, ASTNode *body, SourceLocation loc);

/* 创建foreach循环节点 */
ASTNode *ast_foreach_loop_create(ASTNode *iterable, char *item_var, ASTNode *body, SourceLocation loc);

#endif /* FLYUXC_AST_H */
