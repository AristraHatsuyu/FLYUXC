#include "flyuxc/frontend/parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Parser辅助函数
 * ============================================================================ */

static Token *current_token(Parser *p) {
    if (p->current >= p->token_count) {
        return &p->tokens[p->token_count - 1];  // 返回最后一个token (应该是 EOF)
    }
    return &p->tokens[p->current];
}

static Token *peek(Parser *p, int offset) {
    size_t pos = p->current + offset;
    if (pos >= p->token_count) {
        return &p->tokens[p->token_count - 1];
    }
    return &p->tokens[pos];
}

static bool check(Parser *p, TokenKind kind) {
    if (p->current >= p->token_count) {
        return kind == TK_EOF;  // 越界时只有检查 EOF 才返回 true
    }
    return current_token(p)->kind == kind;
}

static bool match(Parser *p, TokenKind kind) {
    if (check(p, kind)) {
        if (p->current < p->token_count) {  // 只在未越界时前进
            p->current++;
        }
        return true;
    }
    return false;
}

static Token *advance(Parser *p) {
    if (p->current < p->token_count) {
        p->current++;
    }
    return &p->tokens[p->current - 1];
}

static void error_at(Parser *p, Token *token, const char *message) {
    fprintf(stderr, "Error at line %d, column %d: %s\n", 
            token->line, token->column, message);
    p->had_error = true;
}

static SourceLocation token_to_loc(Token *token) {
    SourceLocation loc;
    loc.orig_line = token->orig_line;
    loc.orig_column = token->orig_column;
    loc.orig_length = token->orig_length;
    loc.is_synthetic = 0;
    return loc;
}

/* ============================================================================
 * 前向声明
 * ============================================================================ */

static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_expression(Parser *p);
static ASTNode *parse_primary(Parser *p);
static ASTNode *parse_assignment(Parser *p);
static ASTNode *parse_additive(Parser *p);
static ASTNode *parse_multiplicative(Parser *p);
static ASTNode *parse_equality(Parser *p);
static ASTNode *parse_comparison(Parser *p);
static ASTNode *parse_logical_and(Parser *p);
static ASTNode *parse_logical_or(Parser *p);
static ASTNode *parse_var_declaration(Parser *p);

/* ============================================================================
 * Parser创建和销毁
 * ============================================================================ */

Parser *parser_create(Token *tokens, size_t count, char *source) {
    Parser *p = (Parser *)malloc(sizeof(Parser));
    if (!p) return NULL;
    
    p->tokens = tokens;
    p->token_count = count;
    p->current = 0;
    p->had_error = false;
    p->panic_mode = false;
    p->source = source;
    
    return p;
}

void parser_free(Parser *p) {
    if (p) {
        free(p);
    }
}

/* ============================================================================
 * 表达式解析
 * ============================================================================ */

static ASTNode *parse_postfix(Parser *p);

static ASTNode *parse_primary(Parser *p) {
    Token *token = current_token(p);
    
    // 前缀一元运算符: !, -, +, ++, --
    if (match(p, TK_BANG) || match(p, TK_MINUS) || match(p, TK_PLUS) || 
        match(p, TK_PLUS_PLUS) || match(p, TK_MINUS_MINUS)) {
        Token *op_token = &p->tokens[p->current - 1];
        ASTNode *operand = parse_primary(p);
        ASTNode *node = ast_node_create(AST_UNARY_EXPR, token_to_loc(op_token));
        ASTUnaryExpr *unary = (ASTUnaryExpr *)malloc(sizeof(ASTUnaryExpr));
        unary->op = op_token->kind;
        unary->operand = operand;
        unary->is_postfix = false;  // 前缀运算符
        node->data = unary;
        return node;
    }
    
    // 数字字面量
    if (match(p, TK_NUM)) {
        Token *t = &p->tokens[p->current - 1];
        double value = atof(t->lexeme);
        return ast_num_literal_create(value, (char *)t->lexeme, token_to_loc(t));
    }
    
    // 字符串字面量
    if (match(p, TK_STRING)) {
        Token *t = &p->tokens[p->current - 1];
        // 去掉引号
        size_t len = strlen(t->lexeme);
        char *value = (char *)malloc(len - 1);
        strncpy(value, t->lexeme + 1, len - 2);
        value[len - 2] = '\0';
        ASTNode *node = ast_string_literal_create(value, token_to_loc(t));
        free(value);
        return node;
    }
    
    // 标识符或函数调用
    if (match(p, TK_IDENT) || match(p, TK_BUILTIN_FUNC)) {
        Token *t = &p->tokens[p->current - 1];
        ASTNode *id = ast_identifier_create((char *)t->lexeme, token_to_loc(t));
        
        // 检查是否是函数调用
        if (match(p, TK_L_PAREN)) {
            // 解析参数
            ASTNode **args = NULL;
            size_t arg_count = 0;
            size_t arg_capacity = 0;
            
            if (!check(p, TK_R_PAREN)) {
                do {
                    if (arg_count >= arg_capacity) {
                        arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                        args = (ASTNode **)realloc(args, arg_capacity * sizeof(ASTNode *));
                    }
                    args[arg_count++] = parse_expression(p);
                } while (match(p, TK_COMMA));
            }
            
            if (!match(p, TK_R_PAREN)) {
                error_at(p, current_token(p), "Expected ')' after arguments");
            }
            
            return ast_call_expr_create(id, args, arg_count, token_to_loc(t));
        }
        
        return id;
    }
    
    // 括号表达式
    if (match(p, TK_L_PAREN)) {
        ASTNode *expr = parse_expression(p);
        if (!match(p, TK_R_PAREN)) {
            error_at(p, current_token(p), "Expected ')' after expression");
        }
        return expr;
    }
    
    // 数组字面量
    if (match(p, TK_L_BRACKET)) {
        ASTNode **elements = NULL;
        size_t elem_count = 0;
        size_t elem_capacity = 0;
        
        if (!check(p, TK_R_BRACKET)) {
            do {
                if (elem_count >= elem_capacity) {
                    elem_capacity = elem_capacity == 0 ? 4 : elem_capacity * 2;
                    elements = (ASTNode **)realloc(elements, elem_capacity * sizeof(ASTNode *));
                }
                elements[elem_count++] = parse_expression(p);
            } while (match(p, TK_COMMA));
        }
        
        if (!match(p, TK_R_BRACKET)) {
            error_at(p, current_token(p), "Expected ']' after array elements");
        }
        
        return ast_array_literal_create(elements, elem_count, token_to_loc(token));
    }
    
    // 对象字面量: {a: 1, b: 2}
    if (match(p, TK_L_BRACE)) {
        ASTObjectProperty *properties = NULL;
        size_t prop_count = 0;
        size_t prop_capacity = 0;
        
        if (!check(p, TK_R_BRACE)) {
            do {
                // 解析键（标识符或字符串）
                Token *key_token = current_token(p);
                char *key = NULL;
                if (match(p, TK_IDENT) || match(p, TK_BUILTIN_FUNC)) {
                    key = strdup(key_token->lexeme);
                } else if (match(p, TK_STRING)) {
                    size_t len = strlen(key_token->lexeme);
                    key = (char *)malloc(len - 1);
                    strncpy(key, key_token->lexeme + 1, len - 2);
                    key[len - 2] = '\0';
                } else {
                    error_at(p, key_token, "Expected property key");
                    break;
                }
                
                if (!match(p, TK_COLON)) {
                    error_at(p, current_token(p), "Expected ':' after property key");
                    free(key);
                    break;
                }
                
                ASTNode *value = parse_expression(p);
                
                if (prop_count >= prop_capacity) {
                    prop_capacity = prop_capacity == 0 ? 4 : prop_capacity * 2;
                    properties = (ASTObjectProperty *)realloc(properties, 
                                prop_capacity * sizeof(ASTObjectProperty));
                }
                properties[prop_count].key = key;
                properties[prop_count].value = value;
                prop_count++;
                
            } while (match(p, TK_COMMA));
        }
        
        if (!match(p, TK_R_BRACE)) {
            error_at(p, current_token(p), "Expected '}' after object properties");
        }
        
        return ast_object_literal_create(properties, prop_count, token_to_loc(token));
    }

    error_at(p, token, "Expected expression");
    return NULL;
}

// 解析后缀表达式: obj.prop, obj[index], obj.>method()
static ASTNode *parse_postfix(Parser *p) {
    ASTNode *expr = parse_primary(p);
    
    while (true) {
        // 数组索引: arr[index]
        if (match(p, TK_L_BRACKET)) {
            ASTNode *index = parse_expression(p);
            if (!match(p, TK_R_BRACKET)) {
                error_at(p, current_token(p), "Expected ']' after index");
            }
            expr = ast_index_expr_create(expr, index, expr->loc);
        }
        // 成员访问: obj.prop
        else if (match(p, TK_DOT)) {
            Token *prop_token = current_token(p);
            if (!match(p, TK_IDENT) && !match(p, TK_BUILTIN_FUNC)) {
                error_at(p, prop_token, "Expected property name after '.'");
                break;
            }
            expr = ast_member_expr_create(expr, strdup(prop_token->lexeme), false, expr->loc);
        }
                // 链式调用: obj.>method(args) 或 obj.>property
        else if (match(p, TK_DOT_CHAIN)) {
            Token *method_token = current_token(p);
            if (!match(p, TK_IDENT) && !match(p, TK_BUILTIN_FUNC)) {
                error_at(p, method_token, "Expected method name after '.>'");
                break;
            }
            char *method_name = strdup(method_token->lexeme);
            
            // 检查是否有参数（有括号才是方法调用）
            if (match(p, TK_L_PAREN)) {
                // 有括号 - 方法调用: obj.>method(args)
                ASTNode **args = NULL;
                size_t arg_count = 0;
                size_t arg_capacity = 0;
                
                if (!check(p, TK_R_PAREN)) {
                    do {
                        if (arg_count >= arg_capacity) {
                            arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                            args = (ASTNode **)realloc(args, arg_capacity * sizeof(ASTNode *));
                        }
                        args[arg_count++] = parse_expression(p);
                    } while (match(p, TK_COMMA));
                }
                
                if (!match(p, TK_R_PAREN)) {
                    error_at(p, current_token(p), "Expected ')' after arguments");
                }
                
                // 创建方法调用: method(obj, args...)
                size_t total_args = arg_count + 1;
                ASTNode **all_args = (ASTNode **)malloc(total_args * sizeof(ASTNode *));
                all_args[0] = expr;  // 第一个参数是对象本身
                for (size_t i = 0; i < arg_count; i++) {
                    all_args[i + 1] = args[i];
                }
                if (args) free(args);
                
                ASTNode *callee = ast_identifier_create(method_name, expr->loc);
                expr = ast_call_expr_create(callee, all_args, total_args, expr->loc);
            } else {
                // 无括号 - 属性访问: obj.>property (等同于 obj.property)
                expr = ast_member_expr_create(expr, method_name, false, expr->loc);
            }
        }
        // 后缀 ++ 和 --
        else if (match(p, TK_PLUS_PLUS) || match(p, TK_MINUS_MINUS)) {
            Token *op_token = &p->tokens[p->current - 1];
            // 创建后缀一元运算符节点
            ASTNode *node = ast_node_create(AST_UNARY_EXPR, expr->loc);
            ASTUnaryExpr *unary = (ASTUnaryExpr *)malloc(sizeof(ASTUnaryExpr));
            unary->op = op_token->kind;
            unary->operand = expr;
            unary->is_postfix = true;  // 标记为后缀
            node->data = unary;
            expr = node;
        }
        else {
            break;
        }
    }
    
    return expr;
}

static ASTNode *parse_multiplicative(Parser *p) {
    ASTNode *left = parse_postfix(p);
    
    while (match(p, TK_STAR) || match(p, TK_SLASH) || match(p, TK_PERCENT)) {
        Token *op_token = &p->tokens[p->current - 1];
        TokenKind op = op_token->kind;
        ASTNode *right = parse_postfix(p);
        left = ast_binary_expr_create(op, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_additive(Parser *p) {
    ASTNode *left = parse_multiplicative(p);
    
    while (match(p, TK_PLUS) || match(p, TK_MINUS)) {
        Token *op_token = &p->tokens[p->current - 1];
        TokenKind op = op_token->kind;
        ASTNode *right = parse_multiplicative(p);
        left = ast_binary_expr_create(op, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_comparison(Parser *p) {
    ASTNode *left = parse_additive(p);
    
    while (match(p, TK_LT) || match(p, TK_GT) || match(p, TK_LE) || match(p, TK_GE)) {
        Token *op_token = &p->tokens[p->current - 1];
        TokenKind op = op_token->kind;
        ASTNode *right = parse_additive(p);
        left = ast_binary_expr_create(op, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_equality(Parser *p) {
    ASTNode *left = parse_comparison(p);
    
    while (match(p, TK_EQ_EQ) || match(p, TK_BANG_EQ)) {
        Token *op_token = &p->tokens[p->current - 1];
        TokenKind op = op_token->kind;
        ASTNode *right = parse_comparison(p);
        left = ast_binary_expr_create(op, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_logical_and(Parser *p) {
    ASTNode *left = parse_equality(p);
    
    while (match(p, TK_AND_AND)) {
        Token *op_token = &p->tokens[p->current - 1];
        ASTNode *right = parse_equality(p);
        left = ast_binary_expr_create(TK_AND_AND, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_logical_or(Parser *p) {
    ASTNode *left = parse_logical_and(p);
    
    while (match(p, TK_OR_OR)) {
        Token *op_token = &p->tokens[p->current - 1];
        ASTNode *right = parse_logical_and(p);
        left = ast_binary_expr_create(TK_OR_OR, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_expression(Parser *p) {
    return parse_logical_or(p);
}

/* ============================================================================
 * 语句解析
 * ============================================================================ */

static ASTNode *parse_block(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_L_BRACE)) {
        error_at(p, start, "Expected '{'");
        return NULL;
    }
    
    ASTNode **statements = NULL;
    size_t stmt_count = 0;
    size_t stmt_capacity = 0;
    
    while (!check(p, TK_R_BRACE) && !check(p, TK_EOF)) {
        if (stmt_count >= stmt_capacity) {
            stmt_capacity = stmt_capacity == 0 ? 8 : stmt_capacity * 2;
            statements = (ASTNode **)realloc(statements, stmt_capacity * sizeof(ASTNode *));
        }
        
        ASTNode *stmt = parse_statement(p);
        if (stmt) {
            statements[stmt_count++] = stmt;
        }
        
        // 跳过可选的分号
        match(p, TK_SEMI);
    }
    
    if (!match(p, TK_R_BRACE)) {
        error_at(p, current_token(p), "Expected '}'");
    }
    
    return ast_block_create(statements, stmt_count, token_to_loc(start));
}

static ASTNode *parse_if_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_IF)) {
        return NULL;
    }
    
    // 解析条件
    if (!match(p, TK_L_PAREN)) {
        error_at(p, current_token(p), "Expected '(' after 'if'");
        return NULL;
    }
    
    ASTNode *condition = parse_expression(p);
    
    if (!match(p, TK_R_PAREN)) {
        error_at(p, current_token(p), "Expected ')' after condition");
        return NULL;
    }
    
    // 解析 then 块
    ASTNode *then_block = parse_block(p);
    
    // 解析可选的 else 块
    ASTNode *else_block = NULL;
    if (match(p, TK_L_BRACE)) {
        p->current--; // 回退
        else_block = parse_block(p);
    }
    
    ASTNode **conditions = (ASTNode **)malloc(sizeof(ASTNode *));
    ASTNode **then_blocks = (ASTNode **)malloc(sizeof(ASTNode *));
    conditions[0] = condition;
    then_blocks[0] = then_block;
    
    return ast_if_stmt_create(conditions, then_blocks, 1, else_block, token_to_loc(start));
}

static ASTNode *parse_return_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_RETURN)) {
        return NULL;
    }
    
    ASTNode *value = NULL;
    if (!check(p, TK_SEMI) && !check(p, TK_R_BRACE)) {
        value = parse_expression(p);
    }
    
    return ast_return_stmt_create(value, token_to_loc(start));
}

static ASTNode *parse_loop_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_LOOP)) {
        return NULL;
    }
    
    if (!match(p, TK_L_PAREN)) {
        error_at(p, current_token(p), "Expected '(' after 'L>'");
        return NULL;
    }
    
    // 解析 for 循环: L>(init; cond; update) { ... }
    ASTNode *init = NULL;
    ASTNode *cond = NULL;
    ASTNode *update = NULL;
    
    // 初始化部分 - 可以是变量声明或赋值
    if (!check(p, TK_SEMI)) {
        // 检查是否是变量声明 (identifier followed by :=)
        if (check(p, TK_IDENT)) {
            Token *next = peek(p, 1);
            if (next->kind == TK_DEFINE) {
                init = parse_var_declaration(p);
            } else if (next->kind == TK_ASSIGN) {
                // 赋值语句：转换为 AST_ASSIGN_STMT
                Token *name_token = current_token(p);
                advance(p); // 跳过标识符
                advance(p); // 跳过 =
                ASTNode *target = ast_identifier_create((char *)name_token->lexeme, token_to_loc(name_token));
                ASTNode *value = parse_expression(p);
                init = ast_assign_stmt_create(target, value, token_to_loc(name_token));
            }
        }
    }
    if (!match(p, TK_SEMI)) {
        error_at(p, current_token(p), "Expected ';' after loop init");
    }
    
    // 条件部分
    if (!check(p, TK_SEMI)) {
        cond = parse_expression(p);
    }
    if (!match(p, TK_SEMI)) {
        error_at(p, current_token(p), "Expected ';' after loop condition");
    }
    
    // 更新部分 - 通常是赋值表达式
    if (!check(p, TK_R_PAREN)) {
        // 检查是否是赋值
        if (check(p, TK_IDENT)) {
            Token *name_token = current_token(p);
            Token *next = peek(p, 1);
            if (next->kind == TK_ASSIGN) {
                advance(p); // 跳过标识符
                advance(p); // 跳过 =
                ASTNode *target = ast_identifier_create((char *)name_token->lexeme, token_to_loc(name_token));
                ASTNode *value = parse_expression(p);
                update = ast_assign_stmt_create(target, value, token_to_loc(name_token));
            } else {
                update = parse_expression(p);
            }
        } else {
            update = parse_expression(p);
        }
    }
    
    if (!match(p, TK_R_PAREN)) {
        error_at(p, current_token(p), "Expected ')' after loop header");
        return NULL;
    }
    
    // 循环体
    ASTNode *body = parse_block(p);
    
    return ast_for_loop_create(init, cond, update, body, token_to_loc(start));
}

static ASTNode *parse_var_declaration(Parser *p) {
    Token *name_token = current_token(p);
    
    if (!match(p, TK_IDENT)) {
        return NULL;
    }
    
    char *name = strdup(name_token->lexeme);
    
    // 函数声明: name:<type>=(params){body}
    if (match(p, TK_FUNC_TYPE_START)) {
        // 跳过返回类型
        while (!match(p, TK_FUNC_TYPE_END) && !check(p, TK_EOF)) {
            advance(p);
        }
        
        // 期望参数列表
        if (!match(p, TK_L_PAREN)) {
            error_at(p, current_token(p), "Expected '(' after function type");
            free(name);
            return NULL;
        }
        
        // 解析参数列表
        char **params = NULL;
        size_t param_count = 0;
        size_t param_capacity = 0;
        
        while (!check(p, TK_R_PAREN) && !check(p, TK_EOF)) {
            if (!check(p, TK_IDENT)) {
                error_at(p, current_token(p), "Expected parameter name");
                break;
            }
            
            if (param_count >= param_capacity) {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                params = (char **)realloc(params, param_capacity * sizeof(char *));
            }
            
            params[param_count++] = strdup(current_token(p)->lexeme);
            advance(p);
            
            if (!match(p, TK_COMMA)) {
                break;
            }
        }
        
        if (!match(p, TK_R_PAREN)) {
            error_at(p, current_token(p), "Expected ')' after parameters");
            free(name);
            for (size_t i = 0; i < param_count; i++) free(params[i]);
            free(params);
            return NULL;
        }
        
        // 解析函数体
        ASTNode *body = parse_block(p);
        
        // 创建函数声明节点
        ASTNode *func_node = ast_node_create(AST_FUNC_DECL, token_to_loc(name_token));
        ASTFuncDecl *func = (ASTFuncDecl *)malloc(sizeof(ASTFuncDecl));
        func->name = name;
        func->params = params;
        func->param_count = param_count;
        func->return_type = NULL; // 暂不处理返回类型
        func->body = body;
        func_node->data = func;
        
        return func_node;
    }
    
    // 普通变量声明: name := expr
    if (!match(p, TK_DEFINE)) {
        error_at(p, current_token(p), "Expected ':=' in variable declaration");
        free(name);
        return NULL;
    }
    
    ASTNode *init = parse_expression(p);
    
    return ast_var_decl_create(name, NULL, false, init, token_to_loc(name_token));
}

static ASTNode *parse_statement(Parser *p) {
    // 跳过分号
    while (match(p, TK_SEMI)) {
    }
    
    // EOF或其他无法解析的token
    if (check(p, TK_EOF) || check(p, TK_R_BRACE)) {
        return NULL;
    }
    
    // Return 语句
    if (check(p, TK_KW_RETURN)) {
        return parse_return_statement(p);
    }
    
    // Loop 语句 (for循环)
    if (check(p, TK_KW_LOOP)) {
        return parse_loop_statement(p);
    }
    
    // If 语句
    if (check(p, TK_KW_IF)) {
        return parse_if_statement(p);
    }
    
    // 代码块
    if (check(p, TK_L_BRACE)) {
        return parse_block(p);
    }
    
    // 变量声明或赋值或函数调用
    if (check(p, TK_IDENT) || check(p, TK_BUILTIN_FUNC)) {
        Token *name_token = current_token(p);
        Token *next = peek(p, 1);
        
        // 变量声明: name := expr
        if (next->kind == TK_DEFINE || next->kind == TK_FUNC_TYPE_START) {
            return parse_var_declaration(p);
        }
        
        // 赋值: name = expr 或 arr[i] = expr
        if (next->kind == TK_ASSIGN) {
            advance(p); // 跳过标识符
            advance(p); // 跳过 =
            ASTNode *target = ast_identifier_create((char *)name_token->lexeme, token_to_loc(name_token));
            ASTNode *value = parse_expression(p);
            return ast_assign_stmt_create(target, value, token_to_loc(name_token));
        }
        
        // 检查是否是索引赋值: arr[i] = expr 或成员赋值: obj.prop = expr
        if (next->kind == TK_L_BRACKET || next->kind == TK_DOT) {
            // 解析可能的后缀表达式（arr[i] 或 obj.prop 等）
            ASTNode *target = parse_postfix(p);
            
            // 检查后面是否跟着 =
            if (match(p, TK_ASSIGN)) {
                ASTNode *value = parse_expression(p);
                return ast_assign_stmt_create(target, value, target->loc);
            }
            
            // 如果不是赋值，则作为表达式语句
            ASTNode *node = ast_node_create(AST_EXPR_STMT, target->loc);
            ASTExprStmt *stmt = (ASTExprStmt *)malloc(sizeof(ASTExprStmt));
            stmt->expr = target;
            node->data = stmt;
            return node;
        }
        
        // 可能是函数调用等表达式语句
        ASTNode *expr = parse_expression(p);
        if (expr) {
            ASTNode *node = ast_node_create(AST_EXPR_STMT, expr->loc);
            ASTExprStmt *stmt = (ASTExprStmt *)malloc(sizeof(ASTExprStmt));
            stmt->expr = expr;
            node->data = stmt;
            return node;
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 主解析入口
 * ============================================================================ */

ASTNode *parser_parse(Parser *p) {
    ASTNode **statements = NULL;
    size_t stmt_count = 0;
    size_t stmt_capacity = 0;
    
    
    while (!check(p, TK_EOF)) {
        if (stmt_count >= stmt_capacity) {
            stmt_capacity = stmt_capacity == 0 ? 16 : stmt_capacity * 2;
            statements = (ASTNode **)realloc(statements, stmt_capacity * sizeof(ASTNode *));
        }
        
        size_t old_pos = p->current;
        ASTNode *stmt = parse_statement(p);
        
        if (stmt) {
            statements[stmt_count++] = stmt;
        }
        
        // 如果位置没有前进，强制前进以避免无限循环
        if (p->current == old_pos) {
            if (!check(p, TK_EOF)) {
                advance(p);
            } else {
                break;
            }
        }
        
        // 如果有致命错误，退出
        if (p->had_error && stmt == NULL) {
            break;
        }
        p->had_error = false;
    }
    
    SourceLocation loc;
    loc.orig_line = 1;
    loc.orig_column = 1;
    loc.orig_length = 0;
    loc.is_synthetic = 0;
    return ast_program_create(statements, stmt_count, loc);
}
