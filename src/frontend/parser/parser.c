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

static Token *previous(Parser *p) {
    if (p->current > 0) {
        return &p->tokens[p->current - 1];
    }
    return &p->tokens[0];
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
    
    // 如果长度为1，只显示单个位置；否则显示范围
    if (token->orig_length <= 1) {
        fprintf(stderr, "\033[31mError\033[0m at line %d, column %d: %s\n", 
                display_line, display_column, message);
    } else {
        fprintf(stderr, "\033[31mError\033[0m at line %d, column %d-%d: %s\n", 
                display_line, display_column, display_column + token->orig_length - 1, message);
    }
    
    // 如果有原始源码，显示错误上下文
    if (p->original_source && display_line > 0) {
        const char *src = p->original_source;
        int current_line = 1;
        const char *line_start = src;
        const char *line_end = NULL;
        
        // 找到错误所在行
        while (*src && current_line < display_line) {
            if (*src == '\n') {
                current_line++;
                line_start = src + 1;
            }
            src++;
        }
        
        // 找到行尾
        line_end = line_start;
        while (*line_end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }
        
        // 显示行号和源代码行，但用红色高亮错误区间
        fprintf(stderr, "\033[36m%5d |\033[0m ", display_line);
        
        // 输出源代码，高亮错误区间
        const char *ptr = line_start;
        int byte_pos = 1;
        int error_start_byte = display_column;
        int error_end_byte = display_column + token->orig_length - 1;
        
        while (ptr < line_end) {
            // 判断当前位置是否在错误区间内
            if (byte_pos >= error_start_byte && byte_pos <= error_end_byte) {
                if (byte_pos == error_start_byte) {
                    fprintf(stderr, "\033[31m");  // 开始红色高亮
                }
            }
            
            fputc(*ptr, stderr);
            ptr++;
            byte_pos++;
            
            // 判断是否需要结束红色高亮
            if (byte_pos > error_end_byte && byte_pos - 1 <= error_end_byte) {
                fprintf(stderr, "\033[0m");  // 结束红色高亮
            }
        }
        
        // 如果高亮区间在行尾，确保关闭颜色
        if (error_end_byte >= byte_pos - 1) {
            fprintf(stderr, "\033[0m");
        }
        fprintf(stderr, "\n");
        
        // 显示箭头指示器
        fprintf(stderr, "      \033[36m|\033[0m ");
        
        // 计算箭头位置和长度（考虑 UTF-8 多字节字符）
        ptr = line_start;
        int visual_column = 1;
        byte_pos = 1;
        
        // 找到错误起始的视觉列
        while (ptr < line_end && byte_pos < display_column) {
            unsigned char c = *ptr;
            if (c < 0x80) {
                visual_column++;
                ptr++;
                byte_pos++;
            } else if ((c & 0xE0) == 0xC0) {
                visual_column++;
                ptr += 2;
                byte_pos++;
            } else if ((c & 0xF0) == 0xE0) {
                visual_column++;
                ptr += 3;
                byte_pos++;
            } else if ((c & 0xF8) == 0xF0) {
                visual_column += 2; // Emoji 通常占两个显示宽度
                ptr += 4;
                byte_pos++;
            } else {
                ptr++;
                byte_pos++;
            }
        }
        
        // 输出空格到错误起始位置
        for (int i = 1; i < visual_column; i++) {
            fprintf(stderr, " ");
        }
        
        // 计算错误区间的视觉长度
        int error_visual_length = 0;
        int remaining_bytes = token->orig_length;
        while (ptr < line_end && remaining_bytes > 0) {
            unsigned char c = *ptr;
            if (c < 0x80) {
                error_visual_length++;
                ptr++;
                remaining_bytes--;
            } else if ((c & 0xE0) == 0xC0) {
                error_visual_length++;
                ptr += 2;
                remaining_bytes--;
            } else if ((c & 0xF0) == 0xE0) {
                error_visual_length++;
                ptr += 3;
                remaining_bytes--;
            } else if ((c & 0xF8) == 0xF0) {
                error_visual_length += 2; // Emoji 占两个显示宽度
                ptr += 4;
                remaining_bytes--;
            } else {
                ptr++;
                remaining_bytes--;
            }
        }
        
        // 显示红色波浪线
        fprintf(stderr, "\033[31m");
        if (error_visual_length > 0) {
            for (int i = 0; i < error_visual_length && i < 50; i++) {
                fprintf(stderr, "^");
            }
        } else {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\033[0m\n");
    }
    
    p->had_error = true;
    p->error_count++;
}

// 警告函数：类似 error_at 但用黄色显示
static void warning_at(Parser *p, Token *token, const char *message) {
    // 只使用原始源码位置
    int display_line = token->orig_line;
    int display_column = token->orig_column;
    
    // 如果没有原始位置信息（合成token），使用映射后位置
    if (display_line == 0) {
        display_line = token->line;
        display_column = token->column;
    }
    
    // 如果长度为1，只显示单个位置；否则显示范围
    if (token->orig_length <= 1) {
        fprintf(stderr, "\033[33mWarning\033[0m at line %d, column %d: %s\n", 
                display_line, display_column, message);
    } else {
        fprintf(stderr, "\033[33mWarning\033[0m at line %d, column %d-%d: %s\n", 
                display_line, display_column, display_column + token->orig_length - 1, message);
    }
    
    // 如果有原始源码，显示警告上下文
    if (p->original_source && display_line > 0) {
        const char *src = p->original_source;
        int current_line = 1;
        const char *line_start = src;
        const char *line_end = NULL;
        
        // 找到警告所在行
        while (*src && current_line < display_line) {
            if (*src == '\n') {
                current_line++;
                line_start = src + 1;
            }
            src++;
        }
        
        // 找到行尾
        line_end = line_start;
        while (*line_end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }
        
        // 显示行号和源代码行，用黄色高亮警告区间
        fprintf(stderr, "\033[36m%5d |\033[0m ", display_line);
        
        // 输出源代码，高亮警告区间
        const char *ptr = line_start;
        int byte_pos = 1;
        int error_start_byte = display_column;
        int error_end_byte = display_column + token->orig_length - 1;
        
        while (ptr < line_end) {
            if (byte_pos >= error_start_byte && byte_pos <= error_end_byte) {
                if (byte_pos == error_start_byte) {
                    fprintf(stderr, "\033[33m");  // 开始黄色高亮
                }
            }
            fputc(*ptr, stderr);
            ptr++;
            byte_pos++;
            
            if (byte_pos > error_end_byte && byte_pos - 1 == error_end_byte) {
                fprintf(stderr, "\033[0m");  // 结束高亮
            }
        }
        fprintf(stderr, "\n");
        
        // 显示指示器
        fprintf(stderr, "\033[36m      |\033[0m ");
        
        // 计算到达错误位置的视觉列数
        ptr = line_start;
        int visual_column = 1;
        while (ptr < line_end && visual_column < display_column) {
            unsigned char c = *ptr;
            if (c < 0x80) {
                visual_column++;
                ptr++;
            } else if ((c & 0xE0) == 0xC0) {
                visual_column++;
                ptr += 2;
            } else if ((c & 0xF0) == 0xE0) {
                visual_column++;
                ptr += 3;
            } else if ((c & 0xF8) == 0xF0) {
                visual_column += 2;
                ptr += 4;
            } else {
                ptr++;
            }
        }
        
        for (int i = 1; i < visual_column; i++) {
            fputc(' ', stderr);
        }
        
        // 计算警告区间的视觉长度
        int error_visual_length = 0;
        int remaining_bytes = token->orig_length;
        while (ptr < line_end && remaining_bytes > 0) {
            unsigned char c = *ptr;
            if (c < 0x80) {
                error_visual_length++;
                ptr++;
                remaining_bytes--;
            } else if ((c & 0xE0) == 0xC0) {
                error_visual_length++;
                ptr += 2;
                remaining_bytes -= 2;
            } else if ((c & 0xF0) == 0xE0) {
                error_visual_length++;
                ptr += 3;
                remaining_bytes -= 3;
            } else if ((c & 0xF8) == 0xF0) {
                error_visual_length += 2;
                ptr += 4;
                remaining_bytes -= 4;
            } else {
                ptr++;
                remaining_bytes--;
            }
        }
        
        if (error_visual_length < 1) error_visual_length = 1;
        
        for (int i = 0; i < error_visual_length; i++) {
            fputc('^', stderr);
        }
        fprintf(stderr, "\n");
    }
    
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

static ASTNode *parse_primary(Parser *p) {
    Token *token = current_token(p);
    
    // 前缀一元运算符: !, -, +, ++, --
    if (match(p, TK_BANG) || match(p, TK_MINUS) || match(p, TK_PLUS) || 
        match(p, TK_PLUS_PLUS) || match(p, TK_MINUS_MINUS)) {
        Token *op_token = &p->tokens[p->current - 1];
        ASTNode *operand = parse_primary(p);
        
        // 如果操作数解析失败，返回 NULL 避免创建无效节点
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
                } while (match(p, TK_COMMA) && !check(p, TK_R_PAREN));
                
                // 检查是否在逗号后直接遇到右括号（尾随逗号）
                if (p->current > 0 && p->tokens[p->current - 1].kind == TK_COMMA && check(p, TK_R_PAREN)) {
                    warning_at(p, previous(p), "Trailing comma in function call");
                }
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
                    size_t len = strlen(key_token->lexeme);
                    key = (char *)malloc(len - 1);
                    strncpy(key, key_token->lexeme + 1, len - 2);
                    key[len - 2] = '\0';
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
                    } while (match(p, TK_COMMA) && !check(p, TK_R_PAREN));
                    
                    // 检查是否在逗号后直接遇到右括号（尾随逗号）
                    if (p->current > 0 && p->tokens[p->current - 1].kind == TK_COMMA && check(p, TK_R_PAREN)) {
                        warning_at(p, previous(p), "Trailing comma in function call");
                    }
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
    
    // 检测类型关键字
    if (check(p, TK_TYPE_NUM) || check(p, TK_TYPE_STR) || 
        check(p, TK_TYPE_BL) || check(p, TK_TYPE_OBJ) || check(p, TK_TYPE_FUNC)) {
        error_at(p, current_token(p), "Type keywords (num, str, bl, obj, func) cannot be used as variable names");
        advance(p);
        return NULL;
    }
    
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
    
    // 检查是否是无类型声明的函数: name := (params){body}
    if (check(p, TK_L_PAREN)) {
        Token *peek_tok = peek(p, 1);
        // 如果 ( 后面是 ) 或 标识符，很可能是函数定义
        if (peek_tok && (peek_tok->kind == TK_R_PAREN || peek_tok->kind == TK_IDENT)) {
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
    }
    
    // 普通变量赋值
    ASTNode *init = parse_expression(p);
    
    // 如果表达式解析失败，返回 NULL
    if (init == NULL) {
        free(name);
        return NULL;
    }
    
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
    
    // 检测类型关键字被误用作变量名
    if (check(p, TK_TYPE_NUM) || check(p, TK_TYPE_STR) || 
        check(p, TK_TYPE_BL) || check(p, TK_TYPE_OBJ) || check(p, TK_TYPE_FUNC)) {
        error_at(p, current_token(p), "Type keywords (num, str, bl, obj, func) cannot be used as variable names");
        advance(p);
        return NULL;
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
