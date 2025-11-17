#ifndef FLYUXC_PARSER_H
#define FLYUXC_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "lexer.h"
#include "ast.h"

/* ============================================================================
 * Parser (语法分析器)
 * 
 * 将Lexer产生的Token序列转换为抽象语法树(AST)
 * ============================================================================ */

/* Parser状态结构 */
typedef struct Parser {
    Token *tokens;           /* Token数组 */
    size_t token_count;      /* Token总数 */
    size_t current;          /* 当前Token索引 */
    
    bool had_error;          /* 是否发生错误 */
    bool panic_mode;         /* 是否处于panic模式 */
    
    char *source;            /* 源代码（用于错误报告） */
} Parser;

/* ============================================================================
 * Parser生命周期
 * ============================================================================ */

/* 创建Parser实例 */
Parser *parser_create(Token *tokens, size_t count, char *source);

/* 释放Parser实例 */
void parser_free(Parser *p);

/* 解析入口：返回AST根节点 */
ASTNode *parser_parse(Parser *p);

/* ============================================================================
 * Token操作
 * ============================================================================ */

/* 获取当前Token */
Token *current_token(Parser *p);

/* 获取前一个Token */
Token *previous_token(Parser *p);

/* 向前查看Token（lookahead=1表示下一个） */
Token *peek_token(Parser *p, size_t lookahead);

/* 检查当前Token是否为指定类型 */
bool check(Parser *p, TokenKind kind);

/* 检查当前Token是否匹配，匹配则前进 */
bool match(Parser *p, TokenKind kind);

/* 前进到下一个Token */
Token *advance(Parser *p);

/* 期望当前Token为指定类型，否则报错 */
Token *expect(Parser *p, TokenKind kind, const char *message);

/* 检查是否到达Token流末尾 */
bool is_at_end(Parser *p);

/* ============================================================================
 * 错误处理
 * ============================================================================ */

/* 在指定Token位置报告错误 */
void parser_error(Parser *p, Token *token, const char *message);

/* 在当前Token位置报告错误 */
void parser_error_at_current(Parser *p, const char *message);

/* 错误恢复：同步到下一个语句边界 */
void synchronize(Parser *p);

/* ============================================================================
 * 解析函数 - 顶层
 * ============================================================================ */

/* 解析程序（顶层） */
ASTNode *parse_program(Parser *p);

/* 解析单个语句 */
ASTNode *parse_statement(Parser *p);

/* ============================================================================
 * 解析函数 - 声明
 * ============================================================================ */

/* 解析变量声明: x := 123 或 x :[num]= 123 */
ASTNode *parse_var_decl(Parser *p);

/* 解析常量声明: X :(num)= 123 */
ASTNode *parse_const_decl(Parser *p);

/* 解析函数声明: f := (a, b) { ... } */
ASTNode *parse_func_decl(Parser *p);

/* 解析函数声明（已知函数名之后） */
ASTNode *parse_func_decl_after_name(Parser *p, Token *name_tok);

/* 解析参数列表: (a, b, c) */
char **parse_param_list(Parser *p, size_t *out_count);

/* 解析类型标注: :[num] 或 :(str) 或 :<func> */
ASTNode *parse_type_annotation(Parser *p);

/* ============================================================================
 * 解析函数 - 语句
 * ============================================================================ */

/* 解析if语句: if (cond) { ... } { ... } */
ASTNode *parse_if_stmt(Parser *p);

/* 解析循环语句: L> [...] { ... } */
ASTNode *parse_loop_stmt(Parser *p);

/* 解析返回语句: R> value */
ASTNode *parse_return_stmt(Parser *p);

/* 解析代码块: { ... } */
ASTNode *parse_block(Parser *p);

/* 解析表达式语句: expr; */
ASTNode *parse_expr_stmt(Parser *p);

/* 解析赋值语句: target = value */
ASTNode *parse_assign_stmt(Parser *p, ASTNode *target);

/* ============================================================================
 * 解析函数 - 表达式（按优先级从低到高）
 * ============================================================================ */

/* 解析表达式（入口） */
ASTNode *parse_expr(Parser *p);

/* 逻辑或: a || b */
ASTNode *parse_logical_or(Parser *p);

/* 逻辑与: a && b */
ASTNode *parse_logical_and(Parser *p);

/* 位或: a | b */
ASTNode *parse_bitwise_or(Parser *p);

/* 位异或: a ^ b */
ASTNode *parse_bitwise_xor(Parser *p);

/* 位与: a & b */
ASTNode *parse_bitwise_and(Parser *p);

/* 相等性: a == b, a != b */
ASTNode *parse_equality(Parser *p);

/* 关系运算: a < b, a > b, a <= b, a >= b */
ASTNode *parse_relational(Parser *p);

/* 加减法: a + b, a - b */
ASTNode *parse_additive(Parser *p);

/* 乘除模: a * b, a / b, a % b */
ASTNode *parse_multiplicative(Parser *p);

/* 幂运算: a ** b */
ASTNode *parse_power(Parser *p);

/* 一元运算: !a, -b, +c */
ASTNode *parse_unary(Parser *p);

/* 后缀运算: f(), arr[0], obj.prop, obj.>method */
ASTNode *parse_postfix(Parser *p);

/* 基础表达式: 字面量、标识符、括号表达式 */
ASTNode *parse_primary(Parser *p);

/* ============================================================================
 * 解析函数 - 后缀操作
 * ============================================================================ */

/* 解析函数调用（已知callee）: f(a, b) */
ASTNode *parse_call_expr(Parser *p, ASTNode *callee);

/* 解析索引访问（已知object）: arr[i] */
ASTNode *parse_index_expr(Parser *p, ASTNode *object);

/* 解析成员访问（已知object）: obj.prop */
ASTNode *parse_member_expr(Parser *p, ASTNode *object, bool is_computed);

/* 解析链式调用（已知起始对象）: obj.>method.>call */
ASTNode *parse_chain_expr(Parser *p, ASTNode *object);

/* 解析参数列表: (expr1, expr2, ...) */
ASTNode **parse_arg_list(Parser *p, size_t *out_count);

/* ============================================================================
 * 解析函数 - 字面量
 * ============================================================================ */

/* 解析数组字面量: [1, 2, 3] */
ASTNode *parse_array_literal(Parser *p);

/* 解析对象字面量: {a: 1, b: 2} */
ASTNode *parse_object_literal(Parser *p);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/* 检查当前Token是否为二元运算符 */
bool is_binary_op(TokenKind kind);

/* 检查当前Token是否为一元运算符 */
bool is_unary_op(TokenKind kind);

/* 检查当前Token是否为类型关键字 */
bool is_type_keyword(TokenKind kind);

/* 检查Token是否为语句开始 */
bool is_statement_start(TokenKind kind);

/* 复制字符串（用于AST节点） */
char *parser_strdup(const char *str);

/* ============================================================================
 * 调试函数
 * ============================================================================ */

/* 打印当前解析状态（调试用） */
void parser_debug_print_state(Parser *p);

/* 打印Token流（调试用） */
void parser_debug_print_tokens(Parser *p, size_t count);

#endif /* FLYUXC_PARSER_H */
