#include "flyuxc/frontend/parser.h"
#include "flyuxc/error.h"
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

static Token *previous(Parser *p) {
    if (p->current > 0) {
        return &p->tokens[p->current - 1];
    }
    return &p->tokens[0];
}

/**
 * 检查当前位置是否是函数定义语法: (params) { body }
 * 需要向前查看，跳过括号内容，检查 ) 后是否是 {
 * 函数参数只能是标识符和逗号，不能包含运算符
 */
static bool is_function_definition(Parser *p) {
    // 当前应该在 ( 位置
    if (!check(p, TK_L_PAREN)) {
        return false;
    }
    
    int offset = 1; // 从 ( 后面开始
    int paren_depth = 1;
    bool valid_params = true;
    
    // 扫描括号内容
    while (paren_depth > 0) {
        Token *tok = peek(p, offset);
        if (tok->kind == TK_EOF) {
            return false; // 未闭合的括号
        }
        
        if (tok->kind == TK_L_PAREN) {
            paren_depth++;
            // 如果有嵌套括号，这不可能是函数参数列表
            valid_params = false;
        } else if (tok->kind == TK_R_PAREN) {
            paren_depth--;
        } else if (paren_depth == 1) {
            // 在最外层括号内，检查是否只有合法的参数语法
            // 合法：标识符、逗号、空
            // 非法：运算符、数字、字符串等
            if (tok->kind != TK_IDENT && tok->kind != TK_COMMA) {
                valid_params = false;
            }
        }
        offset++;
    }
    
    // 现在 offset 指向 ) 的下一个位置
    // 检查 ) 后面是否是 {
    Token *after_paren = peek(p, offset);
    
    // 只有当括号后面是 { 且参数合法时，才是函数定义
    return after_paren->kind == TK_L_BRACE && valid_params;
}

static void error_at(Parser *p, Token *token, const char *message) {
    // 只使用原始源码位置
    int display_line = token->orig_line;
    int display_column = token->orig_column;
    
    // 如果没有原始位置信息（合成token），使用映射后位置
    if (display_line == 0) {
        display_line = token->line;
        display_column = token->column;
    }
    
    // 使用统一的错误报告接口
    report_error_at(ERR_ERROR, PHASE_PARSER,
                    p->original_source,
                    display_line, display_column, 
                    token->orig_length > 0 ? token->orig_length : 1,
                    message);
    
    p->had_error = true;
    p->error_count++;
}

// 警告函数
static void warning_at(Parser *p, Token *token, const char *message) {
    // 只使用原始源码位置
    int display_line = token->orig_line;
    int display_column = token->orig_column;
    
    // 如果没有原始位置信息（合成token），使用映射后位置
    if (display_line == 0) {
        display_line = token->line;
        display_column = token->column;
    }
    
    // 使用统一的错误报告接口
    report_error_at(ERR_WARNING, PHASE_PARSER,
                    p->original_source,
                    display_line, display_column,
                    token->orig_length > 0 ? token->orig_length : 1,
                    message);
    
    p->warning_count++;
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
static ASTNode *parse_block(Parser *p);

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
    p->error_count = 0;
    p->panic_mode = false;
    p->source = source;
    p->original_source = NULL;
    
    return p;
}

void parser_set_original_source(Parser *p, const char *original_source) {
    if (p) {
        p->original_source = (char *)original_source;
    }
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
static ASTNode *parse_unary(Parser *p);

static ASTNode *parse_primary(Parser *p) {
    Token *token = current_token(p);
    
    // 数字字面量
    if (match(p, TK_NUM)) {
        Token *t = &p->tokens[p->current - 1];
        char *endptr;
        double value = strtod(t->lexeme, &endptr);
        // 检查转换是否成功
        if (endptr == t->lexeme) {
            error_at(p, t, "Invalid number format");
            return NULL;
        }
        return ast_num_literal_create(value, (char *)t->lexeme, token_to_loc(t));
    }
    
    // 字符串字面量（lexer已处理转义并去除引号）
    if (match(p, TK_STRING)) {
        Token *t = &p->tokens[p->current - 1];
        // lexer已经去除引号并处理转义，直接传递lexeme（ast_string_literal_create会复制）
        ASTNode *node = ast_string_literal_create(t->lexeme, t->lexeme_length, token_to_loc(t));
        return node;
    }
    
    // 布尔字面量: true, false
    if (match(p, TK_TRUE)) {
        Token *t = &p->tokens[p->current - 1];
        return ast_bool_literal_create(true, token_to_loc(t));
    }
    
    if (match(p, TK_FALSE)) {
        Token *t = &p->tokens[p->current - 1];
        return ast_bool_literal_create(false, token_to_loc(t));
    }
    
    // null字面量
    if (match(p, TK_NULL)) {
        Token *t = &p->tokens[p->current - 1];
        ASTNode *node = ast_node_create(AST_NULL_LITERAL, token_to_loc(t));
        return node;
    }
    
    // undef字面量
    if (match(p, TK_UNDEF)) {
        Token *t = &p->tokens[p->current - 1];
        ASTNode *node = ast_node_create(AST_UNDEF_LITERAL, token_to_loc(t));
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
                } while (match(p, TK_COMMA) && !check(p, TK_R_PAREN));
                
                // 检查是否在逗号后直接遇到右括号（尾随逗号）
                if (p->current > 0 && p->tokens[p->current - 1].kind == TK_COMMA && check(p, TK_R_PAREN)) {
                    warning_at(p, previous(p), "Trailing comma in function call");
                }
            }
            
            if (!match(p, TK_R_PAREN)) {
                error_at(p, current_token(p), "Expected ')' after arguments");
            }
            
            // 检测 ! 后缀（表示错误时抛出异常）
            int throw_on_error = 0;
            if (match(p, TK_BANG)) {  // ! 是 TK_BANG
                throw_on_error = 1;
            }
            
            return ast_call_expr_create(id, args, arg_count, throw_on_error, token_to_loc(t));
        }
        
        return id;
    }
    
    // 括号表达式或匿名函数
    if (match(p, TK_L_PAREN)) {
        Token *lparen = &p->tokens[p->current - 1];
        
        // 检查是否是匿名函数: (params) { body }
        // 通过向前查看来判断
        size_t saved_pos = p->current;
        bool is_anonymous_func = false;
        
        // 尝试跳过参数列表
        int paren_depth = 1;
        while (paren_depth > 0 && !check(p, TK_EOF)) {
            if (match(p, TK_L_PAREN)) paren_depth++;
            else if (match(p, TK_R_PAREN)) paren_depth--;
            else advance(p);
        }
        
        // 如果 ) 后面紧跟 {，那就是匿名函数
        if (check(p, TK_L_BRACE)) {
            is_anonymous_func = true;
        }
        
        // 回溯到 ( 后面
        p->current = saved_pos;
        
        if (is_anonymous_func) {
            // 解析匿名函数: (a, b, c) { body }
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
                for (size_t i = 0; i < param_count; i++) free(params[i]);
                free(params);
                return NULL;
            }
            
            // 解析函数体
            ASTNode *body = parse_block(p);
            
            // 创建匿名函数节点 - 使用空名称
            static int anon_func_counter = 0;
            char anon_name[64];
            snprintf(anon_name, sizeof(anon_name), "_anon_%d", anon_func_counter++);
            
            ASTNode *func_node = ast_node_create(AST_FUNC_DECL, token_to_loc(lparen));
            ASTFuncDecl *func = (ASTFuncDecl *)malloc(sizeof(ASTFuncDecl));
            func->name = strdup(anon_name);
            func->params = params;
            func->param_count = param_count;
            func->return_type = NULL;
            func->body = body;
            func_node->data = func;
            
            return func_node;
        }
        
        // 普通括号表达式
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
            } while (match(p, TK_COMMA) && !check(p, TK_R_BRACKET));
            
            // 检查是否在逗号后直接遇到右括号（尾随逗号）
            if (p->current > 0 && p->tokens[p->current - 1].kind == TK_COMMA && check(p, TK_R_BRACKET)) {
                warning_at(p, previous(p), "Trailing comma in array literal");
            }
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
        bool had_error_in_object = false;
        
        if (!check(p, TK_R_BRACE)) {
            do {
                // 解析键（标识符或字符串）
                Token *key_token = current_token(p);
                char *key = NULL;
                if (match(p, TK_IDENT) || match(p, TK_BUILTIN_FUNC)) {
                    key = strdup(key_token->lexeme);
                } else if (match(p, TK_STRING)) {
                    // lexer已经去除引号并处理转义
                    key = strdup(key_token->lexeme);
                } else {
                    error_at(p, key_token, "Expected property key");
                    had_error_in_object = true;
                    // 错误恢复：跳到右花括号结束对象解析
                    while (!check(p, TK_R_BRACE) && !check(p, TK_EOF)) {
                        advance(p);
                    }
                    break;
                }
                
                if (!match(p, TK_COLON)) {
                    error_at(p, current_token(p), "Expected ':' after property key");
                    free(key);
                    had_error_in_object = true;
                    // 错误恢复：跳到右花括号结束对象解析
                    while (!check(p, TK_R_BRACE) && !check(p, TK_EOF)) {
                        advance(p);
                    }
                    break;
                }
                
                ASTNode *value = parse_expression(p);
                
                // 如果解析值失败，跳到右花括号结束对象解析
                if (value == NULL) {
                    free(key);
                    had_error_in_object = true;
                    while (!check(p, TK_R_BRACE) && !check(p, TK_EOF)) {
                        advance(p);
                    }
                    break;
                }
                
                if (prop_count >= prop_capacity) {
                    prop_capacity = prop_capacity == 0 ? 4 : prop_capacity * 2;
                    properties = (ASTObjectProperty *)realloc(properties, 
                                prop_capacity * sizeof(ASTObjectProperty));
                }
                properties[prop_count].key = key;
                properties[prop_count].value = value;
                prop_count++;
                
            } while (match(p, TK_COMMA) && !check(p, TK_R_BRACE));
            
            // 检查是否在逗号后直接遇到右花括号（尾随逗号）
            if (p->current > 0 && p->tokens[p->current - 1].kind == TK_COMMA && check(p, TK_R_BRACE)) {
                warning_at(p, previous(p), "Trailing comma in object literal");
            }
        }
        
        if (!match(p, TK_R_BRACE)) {
            error_at(p, current_token(p), "Expected '}' after object properties");
            had_error_in_object = true;
        }
        
        // 如果对象解析过程中有错误，返回 NULL 而不是部分对象
        if (had_error_in_object) {
            // 释放已分配的属性
            for (size_t i = 0; i < prop_count; i++) {
                if (properties[i].key) free(properties[i].key);
            }
            if (properties) free(properties);
            return NULL;
        }
        
        return ast_object_literal_create(properties, prop_count, token_to_loc(token));
    }

    error_at(p, token, "Expected expression");
    return NULL;
}

// 解析后缀表达式: obj.prop, obj[index], obj.>method()
static ASTNode *parse_postfix(Parser *p) {
    ASTNode *expr = parse_primary(p);
    
    // 如果 parse_primary 失败，立即返回 NULL 避免死循环
    if (expr == NULL) {
        return NULL;
    }
    
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
                // 链式调用: obj.>method(args) 或 obj.>function
        else if (match(p, TK_DOT_CHAIN)) {
            Token *method_token = current_token(p);
            if (!match(p, TK_IDENT) && !match(p, TK_BUILTIN_FUNC)) {
                error_at(p, method_token, "Expected method name after '.>'");
                break;
            }
            char *method_name = strdup(method_token->lexeme);
            
            // 检查是否有参数（有括号）
            if (match(p, TK_L_PAREN)) {
                // 有括号 - 方法调用: obj.>method(args) → method(obj, args)
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
                    } while (match(p, TK_COMMA) && !check(p, TK_R_PAREN));
                    
                    // 检查是否在逗号后直接遇到右括号（尾随逗号）
                    if (p->current > 0 && p->tokens[p->current - 1].kind == TK_COMMA && check(p, TK_R_PAREN)) {
                        warning_at(p, previous(p), "Trailing comma in function call");
                    }
                }
                
                if (!match(p, TK_R_PAREN)) {
                    error_at(p, current_token(p), "Expected ')' after arguments");
                }
                
                // 检测 ! 后缀（表示错误时抛出异常）
                int throw_on_error = 0;
                if (match(p, TK_BANG)) {  // ! 是 TK_BANG
                    throw_on_error = 1;
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
                expr = ast_call_expr_create(callee, all_args, total_args, throw_on_error, expr->loc);
            } else {
                // 无括号 - 零参数函数调用: obj.>func → func(obj)
                // 根据 FLYUX_SYNTAX.md: array.>len.>🐮🐴(2)  # 链式调用,左边作为第一个参数
                
                // 检测 ! 后缀（表示错误时抛出异常）
                int throw_on_error = 0;
                if (match(p, TK_BANG)) {  // ! 是 TK_BANG
                    throw_on_error = 1;
                }
                
                ASTNode **all_args = (ASTNode **)malloc(1 * sizeof(ASTNode *));
                all_args[0] = expr;  // 左边的值作为唯一参数
                
                ASTNode *callee = ast_identifier_create(method_name, expr->loc);
                expr = ast_call_expr_create(callee, all_args, 1, throw_on_error, expr->loc);
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

// 一元运算符: !, -, +, ++, --
// 优先级高于幂运算，但低于后缀运算（成员访问、数组索引）
// 所以 !cell.hot 解析为 !(cell.hot)，而不是 (!cell).hot
static ASTNode *parse_unary(Parser *p) {
    // 前缀一元运算符: !, -, +, ++, --
    if (match(p, TK_BANG) || match(p, TK_MINUS) || match(p, TK_PLUS) || 
        match(p, TK_PLUS_PLUS) || match(p, TK_MINUS_MINUS)) {
        Token *op_token = &p->tokens[p->current - 1];
        // 递归调用 parse_unary 处理连续前缀运算符如 !!x 或 --x
        ASTNode *operand = parse_unary(p);
        
        if (operand == NULL) {
            error_at(p, current_token(p), "Expected operand after unary operator");
            return NULL;
        }
        
        ASTNode *node = ast_node_create(AST_UNARY_EXPR, token_to_loc(op_token));
        ASTUnaryExpr *unary = (ASTUnaryExpr *)malloc(sizeof(ASTUnaryExpr));
        unary->op = op_token->kind;
        unary->operand = operand;
        unary->is_postfix = false;  // 前缀运算符
        node->data = unary;
        return node;
    }
    
    // 不是前缀运算符，解析后缀表达式
    return parse_postfix(p);
}

// 幂运算 (右结合: 2**3**2 = 2**(3**2) = 512)
static ASTNode *parse_power(Parser *p) {
    ASTNode *left = parse_unary(p);  // 修改：调用 parse_unary 而不是 parse_postfix
    
    if (match(p, TK_POWER)) {
        Token *op_token = &p->tokens[p->current - 1];
        ASTNode *right = parse_power(p);  // 右结合：递归调用自身
        return ast_binary_expr_create(op_token->kind, left, right, token_to_loc(op_token));
    }
    
    return left;
}

static ASTNode *parse_multiplicative(Parser *p) {
    ASTNode *left = parse_power(p);  // 修改为调用 parse_power
    
    while (match(p, TK_STAR) || match(p, TK_SLASH) || match(p, TK_PERCENT)) {
        Token *op_token = &p->tokens[p->current - 1];
        TokenKind op = op_token->kind;
        ASTNode *right = parse_power(p);  // 修改为调用 parse_power
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
    
    // 检查是否有比较运算符
    if (!check(p, TK_LT) && !check(p, TK_GT) && !check(p, TK_LE) && !check(p, TK_GE)) {
        return left;
    }
    
    // 收集链式比较：a < b < c 转换为 a < b && b < c
    ASTNode *result = NULL;
    ASTNode *prev_left = left;
    
    while (match(p, TK_LT) || match(p, TK_GT) || match(p, TK_LE) || match(p, TK_GE)) {
        Token *op_token = &p->tokens[p->current - 1];
        TokenKind op = op_token->kind;
        ASTNode *right = parse_additive(p);
        
        // 创建当前的比较：prev_left op right
        ASTNode *current_cmp = ast_binary_expr_create(op, prev_left, right, token_to_loc(op_token));
        
        if (result == NULL) {
            // 第一个比较
            result = current_cmp;
        } else {
            // 链式比较：用 && 连接
            result = ast_binary_expr_create(TK_AND_AND, result, current_cmp, token_to_loc(op_token));
        }
        
        // 下一次比较的左操作数：
        // 如果 right 是标识符，创建一个新的标识符节点（避免节点复用）
        // 否则，链式比较可能会有问题（需要临时变量，暂时不支持复杂表达式的链式比较）
        if (right->kind == AST_IDENTIFIER) {
            ASTIdentifier *id = (ASTIdentifier *)right->data;
            prev_left = ast_identifier_create(id->name, right->loc);
        } else if (right->kind == AST_NUM_LITERAL) {
            ASTNumLiteral *num = (ASTNumLiteral *)right->data;
            prev_left = ast_num_literal_create(num->value, num->raw, right->loc);
        } else {
            // 复杂表达式在链式比较中需要临时变量，暂时直接复用节点
            // 注意：这可能导致问题，但对于简单情况应该可以工作
            prev_left = right;
        }
    }
    
    return result ? result : left;
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
        
        size_t old_pos = p->current;
        ASTNode *stmt = parse_statement(p);
        if (stmt) {
            statements[stmt_count++] = stmt;
        }
        
        // 如果位置没有前进，强制前进避免死循环
        if (p->current == old_pos) {
            if (!check(p, TK_R_BRACE) && !check(p, TK_EOF)) {
                error_at(p, current_token(p), "Unexpected token in block");
                advance(p);
            } else {
                break;
            }
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
    
    // 收集所有条件和对应的块
    ASTNode **conditions = (ASTNode **)malloc(sizeof(ASTNode *));
    ASTNode **then_blocks = (ASTNode **)malloc(sizeof(ASTNode *));
    size_t cond_count = 1;
    size_t cond_capacity = 1;
    
    conditions[0] = condition;
    then_blocks[0] = then_block;
    
    // 解析链式条件: } (condition) { block }
    while (check(p, TK_L_PAREN)) {
        advance(p); // 跳过 (
        
        ASTNode *next_cond = parse_expression(p);
        if (!next_cond) {
            error_at(p, current_token(p), "Expected condition after '('");
            break;
        }
        
        if (!match(p, TK_R_PAREN)) {
            error_at(p, current_token(p), "Expected ')' after condition");
            break;
        }
        
        ASTNode *next_block = parse_block(p);
        
        // 扩展数组
        if (cond_count >= cond_capacity) {
            cond_capacity *= 2;
            conditions = (ASTNode **)realloc(conditions, cond_capacity * sizeof(ASTNode *));
            then_blocks = (ASTNode **)realloc(then_blocks, cond_capacity * sizeof(ASTNode *));
        }
        
        conditions[cond_count] = next_cond;
        then_blocks[cond_count] = next_block;
        cond_count++;
    }
    
    // 解析可选的 else 块（最后一个 {} 没有条件）
    ASTNode *else_block = NULL;
    if (match(p, TK_L_BRACE)) {
        p->current--; // 回退
        else_block = parse_block(p);
    }
    
    return ast_if_stmt_create(conditions, then_blocks, cond_count, else_block, token_to_loc(start));
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

static ASTNode *parse_break_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_BREAK)) {
        return NULL;
    }
    
    // 检查是否有目标标签: B> label
    char *target_label = NULL;
    if (check(p, TK_IDENT)) {
        Token *label_token = current_token(p);
        advance(p);
        target_label = strdup(label_token->lexeme);
    }
    
    // 创建带标签的 break 节点
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->kind = AST_BREAK_STMT;
    node->loc = token_to_loc(start);
    
    ASTBreakStmt *break_stmt = (ASTBreakStmt *)malloc(sizeof(ASTBreakStmt));
    break_stmt->target_label = target_label;
    node->data = break_stmt;
    
    return node;
}

static ASTNode *parse_next_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_NEXT)) {
        return NULL;
    }
    
    // 检查是否有目标标签: N> label
    char *target_label = NULL;
    if (check(p, TK_IDENT)) {
        Token *label_token = current_token(p);
        advance(p);
        target_label = strdup(label_token->lexeme);
    }
    
    // 创建带标签的 next 节点
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->kind = AST_NEXT_STMT;
    node->loc = token_to_loc(start);
    
    ASTNextStmt *next_stmt = (ASTNextStmt *)malloc(sizeof(ASTNextStmt));
    next_stmt->target_label = target_label;
    node->data = next_stmt;
    
    return node;
}

static ASTNode *parse_try_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_TRY)) {
        return NULL;
    }
    
    // 必须有try块: T> { ... }
    if (!check(p, TK_L_BRACE)) {
        error_at(p, current_token(p), "Expected '{' after 'T>'");
        return NULL;
    }
    
    ASTNode *try_block = parse_block(p);
    if (!try_block) {
        return NULL;
    }
    
    // 可选的catch块: (error) { ... }
    char *catch_param = NULL;
    ASTNode *catch_block = NULL;
    
    if (check(p, TK_L_PAREN)) {
        advance(p);  // 消耗 (
        
        if (current_token(p)->kind != TK_IDENT) {
            error_at(p, current_token(p), "Expected parameter name in catch clause");
            ast_node_free(try_block);
            return NULL;
        }
        
        Token *param_tok = current_token(p);
        catch_param = strdup(param_tok->lexeme);
        advance(p);
        
        if (!match(p, TK_R_PAREN)) {
            error_at(p, current_token(p), "Expected ')' after catch parameter");
            free(catch_param);
            ast_node_free(try_block);
            return NULL;
        }
        
        if (!check(p, TK_L_BRACE)) {
            error_at(p, current_token(p), "Expected '{' for catch block");
            free(catch_param);
            ast_node_free(try_block);
            return NULL;
        }
        
        catch_block = parse_block(p);
        if (!catch_block) {
            free(catch_param);
            ast_node_free(try_block);
            return NULL;
        }
    }
    
    // 可选的finally块: { ... } (没有参数括号)
    ASTNode *finally_block = NULL;
    if (check(p, TK_L_BRACE)) {
        finally_block = parse_block(p);
        if (!finally_block) {
            if (catch_param) free(catch_param);
            ast_node_free(catch_block);
            ast_node_free(try_block);
            return NULL;
        }
    }
    
    return ast_try_stmt_create(try_block, catch_param, catch_block, finally_block, token_to_loc(start));
}

/* 辅助函数：解析可选的循环标签 :label */
static char *parse_optional_loop_label(Parser *p) {
    // 检查是否有 :label 语法（注意：不是 := 或 :[ 或 :(）
    if (check(p, TK_COLON)) {
        Token *next = peek(p, 1);
        // 必须紧跟标识符，且后面是 { 才是标签
        if (next && next->kind == TK_IDENT) {
            Token *after = peek(p, 2);
            if (after && after->kind == TK_L_BRACE) {
                advance(p);  // 跳过 :
                Token *label_token = current_token(p);
                advance(p);  // 跳过标识符
                return strdup(label_token->lexeme);
            }
        }
    }
    return NULL;
}

static ASTNode *parse_loop_statement(Parser *p) {
    Token *start = current_token(p);
    
    if (!match(p, TK_KW_LOOP)) {
        return NULL;
    }
    
    // 所有循环都必须以 ( 开头
    // L> (n) { } - repeat 循环（仅表达式，无 : 无 ;）
    // L> (array : item) { } - foreach 循环（含单个 :）
    // L> (init; cond; update) { } - for 循环（含 ;）
    
    if (!match(p, TK_L_PAREN)) {
        error_at(p, current_token(p), "Expected '(' after 'L>'");
        return NULL;
    }
    
    // 向前看，判断循环类型
    // 检查是否有 ; 或 : （不是 := 的 :）
    int has_semi = 0;
    int has_colon = 0;
    int lookahead = 0;
    int paren_depth = 1;  // 已经匹配了一个 (
    
    while (lookahead < 50 && paren_depth > 0) {
        Token *t = peek(p, lookahead);
        if (t->kind == TK_L_PAREN) {
            paren_depth++;
        } else if (t->kind == TK_R_PAREN) {
            paren_depth--;
        } else if (paren_depth == 1) {  // 只在第一层括号内检查
            if (t->kind == TK_SEMI) {
                has_semi = 1;
                break;  // 有分号一定是 for 循环
            }
            if (t->kind == TK_COLON) {
                // 检查下一个token，如果是 = 则是 := 不是 foreach
                Token *next = peek(p, lookahead + 1);
                if (next->kind != TK_ASSIGN && next->kind != TK_L_BRACKET && next->kind != TK_L_PAREN) {
                    has_colon = 1;
                }
            }
        }
        lookahead++;
    }
    
    if (has_semi) {
        // for 循环: L> (init; cond; update):label { }
        ASTNode *init = NULL;
        ASTNode *cond = NULL;
        ASTNode *update = NULL;
        
        // 初始化部分
        if (!check(p, TK_SEMI)) {
            if (check(p, TK_IDENT)) {
                Token *next = peek(p, 1);
                if (next->kind == TK_DEFINE) {
                    init = parse_var_declaration(p);
                } else if (next->kind == TK_ASSIGN) {
                    Token *name_token = current_token(p);
                    advance(p);
                    advance(p);
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
        
        // 更新部分
        if (!check(p, TK_R_PAREN)) {
            if (check(p, TK_IDENT)) {
                Token *name_token = current_token(p);
                Token *next = peek(p, 1);
                if (next->kind == TK_ASSIGN) {
                    advance(p);
                    advance(p);
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
        
        char *label = parse_optional_loop_label(p);
        ASTNode *body = parse_block(p);
        ASTNode *node = ast_for_loop_create(init, cond, update, body, token_to_loc(start));
        if (label) {
            ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
            loop->label = label;
        }
        return node;
    }
    
    if (has_colon) {
        // foreach 循环: L> (array : item):label { }
        ASTNode *iterable = parse_expression(p);
        if (!iterable) {
            error_at(p, current_token(p), "Expected iterable expression");
            return NULL;
        }
        if (!match(p, TK_COLON)) {
            error_at(p, current_token(p), "Expected ':' in foreach loop");
            return NULL;
        }
        Token *item_token = current_token(p);
        if (!match(p, TK_IDENT)) {
            error_at(p, item_token, "Expected variable name after ':'");
            return NULL;
        }
        char *item_var = strdup(item_token->lexeme);
        if (!match(p, TK_R_PAREN)) {
            error_at(p, current_token(p), "Expected ')' after foreach header");
            return NULL;
        }
        
        char *label = parse_optional_loop_label(p);
        ASTNode *body = parse_block(p);
        ASTNode *node = ast_foreach_loop_create(iterable, item_var, body, token_to_loc(start));
        if (label) {
            ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
            loop->label = label;
        }
        return node;
    }
    
    // repeat 循环: L> (n):label { }
    ASTNode *count_expr = parse_expression(p);
    if (!count_expr) {
        error_at(p, current_token(p), "Expected count expression in repeat loop");
        return NULL;
    }
    if (!match(p, TK_R_PAREN)) {
        error_at(p, current_token(p), "Expected ')' after repeat count");
        return NULL;
    }
    
    char *label = parse_optional_loop_label(p);
    ASTNode *body = parse_block(p);
    ASTNode *node = ast_repeat_loop_create(count_expr, body, token_to_loc(start));
    if (label) {
        ASTLoopStmt *loop = (ASTLoopStmt *)node->data;
        loop->label = label;
    }
    return node;
}

static ASTNode *parse_var_declaration(Parser *p) {
    Token *name_token = current_token(p);
    
    // 检查是否使用了保留关键字作为变量名
    if (name_token->kind == TK_TYPE_NUM || name_token->kind == TK_TYPE_STR ||
        name_token->kind == TK_TYPE_BL || name_token->kind == TK_TYPE_OBJ ||
        name_token->kind == TK_TYPE_FUNC || name_token->kind == TK_TRUE ||
        name_token->kind == TK_FALSE || name_token->kind == TK_NULL ||
        name_token->kind == TK_UNDEF || name_token->kind == TK_KW_IF) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "Cannot use reserved keyword '%s' as variable name", 
                name_token->lexeme);
        error_at(p, name_token, error_msg);
        return NULL;
    }
    
    if (!match(p, TK_IDENT)) {
        return NULL;
    }
    
    char *name = strdup(name_token->lexeme);
    
    // 跳过可选的类型注解: :[type]= 或 :<type>=
    // TypeScript 风格：类型注解用于辅助检查但不控制实际类型
    ASTNode *type_annotation = NULL;
    if (check(p, TK_COLON)) {
        advance(p); // 跳过 :
        
        // 检查是否是 [ 或 < 开头的类型注解
        if (match(p, TK_L_BRACKET)) {
            // :[type]= 格式
            Token *type_tok = current_token(p);
            TokenKind type_kind = type_tok->kind;
            
            // 记录类型token（可能是 TK_TYPE_NUM, TK_TYPE_STR 等）
            if (!match(p, TK_TYPE_NUM) && !match(p, TK_TYPE_STR) && !match(p, TK_TYPE_BL) && 
                !match(p, TK_TYPE_OBJ) && !match(p, TK_TYPE_FUNC)) {
                // 可能是自定义类型，跳过
                advance(p);
            }
            
            if (!match(p, TK_R_BRACKET)) {
                error_at(p, current_token(p), "Expected ']' after type annotation");
            }
            
            // 创建类型注解AST节点
            type_annotation = ast_node_create(AST_TYPE_ANNOTATION, token_to_loc(type_tok));
            ASTTypeAnnotation *type_ann = (ASTTypeAnnotation *)malloc(sizeof(ASTTypeAnnotation));
            type_ann->type_token = type_kind;
            type_annotation->data = type_ann;
            
        } else if (match(p, TK_LT)) {
            // :<type>= 格式
            Token *type_tok = current_token(p);
            TokenKind type_kind = type_tok->kind;
            
            // 跳过类型 token
            if (!match(p, TK_TYPE_NUM) && !match(p, TK_TYPE_STR) && !match(p, TK_TYPE_BL) && 
                !match(p, TK_TYPE_OBJ) && !match(p, TK_TYPE_FUNC)) {
                advance(p);
            }
            
            if (!match(p, TK_GT)) {
                error_at(p, current_token(p), "Expected '>' after type annotation");
            }
            
            // 创建类型注解AST节点
            type_annotation = ast_node_create(AST_TYPE_ANNOTATION, token_to_loc(type_tok));
            ASTTypeAnnotation *type_ann = (ASTTypeAnnotation *)malloc(sizeof(ASTTypeAnnotation));
            type_ann->type_token = type_kind;
            type_annotation->data = type_ann;
        }
        // 如果不是 [ 或 <，可能是对象字面量的冒号，回退
        // 但在变量声明上下文中，这不应该发生
    }
    
    // 函数声明: name:<type>=(params){body} 或已经跳过类型注解
    if (match(p, TK_FUNC_TYPE_START)) {
        // 跳过返回类型，直到遇到 >= (TK_GE) 或 >= (TK_FUNC_TYPE_END)
        while (!match(p, TK_GE) && !match(p, TK_FUNC_TYPE_END) && !check(p, TK_EOF)) {
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
    
    // 普通变量声明: name := expr 或 name :[type]= expr
    // 如果有类型注解，期待 = 而不是 :=
    TokenKind expected_assign = type_annotation ? TK_ASSIGN : TK_DEFINE;
    if (!match(p, expected_assign)) {
        if (type_annotation) {
            error_at(p, current_token(p), "Expected '=' after type annotation in variable declaration");
        } else {
            error_at(p, current_token(p), "Expected ':=' in variable declaration");
        }
        free(name);
        return NULL;
    }
    
    // 检查是否是无类型声明的函数: name := (params){body}
    // 使用 is_function_definition 来正确区分函数定义和括号表达式
    if (check(p, TK_L_PAREN) && is_function_definition(p)) {
        // 解析为函数定义
        if (!match(p, TK_L_PAREN)) {
            error_at(p, current_token(p), "Expected '('");
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
        func->return_type = NULL;
        func->body = body;
        func_node->data = func;
        
        return func_node;
    }
    
    // 普通变量赋值
    ASTNode *init = parse_expression(p);
    
    // 如果表达式解析失败，返回 NULL
    if (init == NULL) {
        free(name);
        return NULL;
    }
    
    // 检查：如果是 := null 则报错（null需要显式类型注解）
    if (!type_annotation && init->kind == AST_NULL_LITERAL) {
        error_at(p, name_token, "Cannot infer type from null value. Use explicit type annotation like 'name:[type]=null'");
        free(name);
        return NULL;
    }
    
    return ast_var_decl_create(name, type_annotation, false, init, token_to_loc(name_token));
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
    
    // Break 语句
    if (check(p, TK_KW_BREAK)) {
        return parse_break_statement(p);
    }
    
    // Next 语句 (continue)
    if (check(p, TK_KW_NEXT)) {
        return parse_next_statement(p);
    }
    
    // Try 语句
    if (check(p, TK_KW_TRY)) {
        return parse_try_statement(p);
    }
    
    // Loop 语句 (for循环)
    if (check(p, TK_KW_LOOP)) {
        return parse_loop_statement(p);
    }
    
    // If 语句
    if (check(p, TK_KW_IF)) {
        return parse_if_statement(p);
    }
    
    // 代码块 或 对象字面量表达式
    // 需要区分: { stmt } 是代码块, { key: value } 是对象字面量
    if (check(p, TK_L_BRACE)) {
        // 使用lookahead判断是否为对象字面量
        // 对象字面量: { } 或 { key: value } 或 { "key": value }
        // 代码块: { stmt; } 或 { var := value }
        Token *next = peek(p, 1);
        
        // 空花括号 - 可能是空对象或空代码块,优先当作对象字面量
        if (next->kind == TK_R_BRACE) {
            ASTNode *expr = parse_expression(p);
            if (expr) {
                ASTNode *node = ast_node_create(AST_EXPR_STMT, expr->loc);
                ASTExprStmt *stmt = (ASTExprStmt *)malloc(sizeof(ASTExprStmt));
                stmt->expr = expr;
                node->data = stmt;
                return node;
            }
        }
        
        // 检查 { identifier/string : ... } 模式 - 对象字面量
        if ((next->kind == TK_IDENT || next->kind == TK_STRING || next->kind == TK_BUILTIN_FUNC)) {
            Token *after_key = peek(p, 2);
            if (after_key && after_key->kind == TK_COLON) {
                // 这是对象字面量
                ASTNode *expr = parse_expression(p);
                if (expr) {
                    ASTNode *node = ast_node_create(AST_EXPR_STMT, expr->loc);
                    ASTExprStmt *stmt = (ASTExprStmt *)malloc(sizeof(ASTExprStmt));
                    stmt->expr = expr;
                    node->data = stmt;
                    return node;
                }
            }
        }
        
        // 否则是代码块
        return parse_block(p);
    }
    
    // Note: 类型关键字(num, str, bl, obj, func)不能作为变量名
    // 但这个检查应该在词法分析或normalize阶段完成，而不是在这里
    // 因为在这个位置，我们无法区分是类型关键字还是恰好同名的标识符
    // 已移除此检查，类型关键字会在classify_identifier中被识别为TYPE tokens
    // 如果它们出现在变量名位置，parse_var_declaration会处理
    
    // 检查是否使用了保留关键字作为变量名（在变量声明位置）
    Token *cur_tok = current_token(p);
    Token *next_tok = peek(p, 1);
    if (next_tok && (next_tok->kind == TK_DEFINE || next_tok->kind == TK_FUNC_TYPE_START || next_tok->kind == TK_COLON)) {
        // 这是一个变量声明，检查变量名
        if (cur_tok->kind == TK_TYPE_NUM || cur_tok->kind == TK_TYPE_STR ||
            cur_tok->kind == TK_TYPE_BL || cur_tok->kind == TK_TYPE_OBJ ||
            cur_tok->kind == TK_TYPE_FUNC || cur_tok->kind == TK_TRUE ||
            cur_tok->kind == TK_FALSE || cur_tok->kind == TK_NULL ||
            cur_tok->kind == TK_UNDEF || cur_tok->kind == TK_KW_IF) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), 
                    "Cannot use reserved keyword '%s' as variable name", 
                    cur_tok->lexeme);
            error_at(p, cur_tok, error_msg);
            return NULL;
        }
    }
    
    // 变量声明或赋值或函数调用
    if (check(p, TK_IDENT) || check(p, TK_BUILTIN_FUNC)) {
        Token *name_token = current_token(p);
        Token *next = peek(p, 1);
        
        // 变量声明: name := expr 或 name :[type]= expr 或 name :<type>=(params){}
        // 需要检查第二个token是否是 := 或 : (后面跟类型注解)
        if (next->kind == TK_DEFINE || next->kind == TK_FUNC_TYPE_START) {
            return parse_var_declaration(p);
        }
        
        // 检查是否是带类型注解的声明: name :[type]= 或 name :<type>=
        // 即 next是 : 且不是对象字面量的开始
        if (next->kind == TK_COLON) {
            // 需要lookahead更多token判断是否是类型注解
            Token *third = peek(p, 2);
            if (third->kind == TK_L_BRACKET || third->kind == TK_LT) {
                // 这是类型注解: :[...] 或 :<...>
                return parse_var_declaration(p);
            }
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
            // 检查是否有 := 跟随（无效的左值赋值）
            if (check(p, TK_DEFINE)) {
                error_at(p, current_token(p), "Invalid left-hand side in variable declaration");
                advance(p); // 跳过 :=
                return NULL;
            }
            
            ASTNode *node = ast_node_create(AST_EXPR_STMT, expr->loc);
            ASTExprStmt *stmt = (ASTExprStmt *)malloc(sizeof(ASTExprStmt));
            stmt->expr = expr;
            node->data = stmt;
            return node;
        }
    }
    
    // 处理其他类型开头的表达式语句 (例如: 字符串字面量.>方法)
    // 包括: TK_NUM, TK_STRING, TK_TRUE, TK_FALSE, TK_NULL, TK_UNDEF, TK_L_PAREN, TK_L_BRACKET, TK_L_BRACE
    ASTNode *expr = parse_expression(p);
    if (expr) {
        ASTNode *node = ast_node_create(AST_EXPR_STMT, expr->loc);
        ASTExprStmt *stmt = (ASTExprStmt *)malloc(sizeof(ASTExprStmt));
        stmt->expr = expr;
        node->data = stmt;
        return node;
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
        // 重置 per-statement error flag，但保留 error_count
        p->had_error = false;
    }
    
    SourceLocation loc;
    loc.orig_line = 1;
    loc.orig_column = 1;
    loc.orig_length = 0;
    loc.is_synthetic = 0;
    return ast_program_create(statements, stmt_count, loc);
}
