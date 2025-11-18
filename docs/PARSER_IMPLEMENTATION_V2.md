# FLYUX Parser 实现计划 V2.0

**基于**: PARSER_DESIGN_V2.md  
**目标**: 生产级Parser，对标Rust/Go编译器  
**预计时间**: 14-21天

---

## 📋 实现清单

### Phase 1: 核心基础设施 (Day 1-2)

#### 1.1 Arena内存分配器

**文件**: `src/core/arena.c`, `include/flyuxc/arena.h`

```c
/* arena.h */
#ifndef FLYUXC_ARENA_H
#define FLYUXC_ARENA_H

#include <stddef.h>
#include <stdint.h>

/* Arena块 - 链表结构 */
typedef struct ArenaBlock {
    uint8_t* buffer;
    size_t capacity;
    size_t used;
    struct ArenaBlock* next;
} ArenaBlock;

/* Arena分配器 */
typedef struct Arena {
    ArenaBlock* current;
    ArenaBlock* first;
    size_t total_allocated;
    size_t block_size;      // 默认64KB
} Arena;

/* API */
Arena* arena_create(size_t initial_size);
void* arena_alloc(Arena* arena, size_t size);
void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);

/* 统计信息 */
size_t arena_total_memory(Arena* arena);
size_t arena_used_memory(Arena* arena);

#endif
```

**实现要点**:
- 初始块64KB，后续倍增
- 8字节对齐保证
- 无锁设计(单线程)
- 性能目标: 分配<10ns

#### 1.2 字符串池

**文件**: `src/core/string_pool.c`, `include/flyuxc/string_pool.h`

```c
/* string_pool.h */
#ifndef FLYUXC_STRING_POOL_H
#define FLYUXC_STRING_POOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* 字符串池 - 基于哈希表 */
typedef struct StringPool {
    char** strings;
    uint64_t* hashes;
    size_t count;
    size_t capacity;
    Arena* arena;           // 存储字符串数据
} StringPool;

/* API */
StringPool* string_pool_create(Arena* arena);
const char* string_pool_intern(StringPool* pool, const char* str, size_t len);
bool string_pool_contains(StringPool* pool, const char* str, size_t len);
void string_pool_destroy(StringPool* pool);

/* 工具函数 */
uint64_t hash_string(const char* str, size_t len);
bool strings_equal_ptr(const char* a, const char* b);  // O(1)指针比较

#endif
```

**实现要点**:
- FNV-1a哈希算法
- 线性探测冲突解决
- 自动扩容(负载因子0.75)
- 去重率>99%

#### 1.3 基础AST结构

**文件**: `src/core/ast.c`, `include/flyuxc/ast.h`

```c
/* ast.h - 核心定义 */
#ifndef FLYUXC_AST_H
#define FLYUXC_AST_H

#include <stddef.h>
#include <stdint.h>
#include "arena.h"

/* 源码位置 */
typedef struct SourceLoc {
    const char* file;
    uint32_t line;
    uint32_t column;
    uint32_t offset;
    uint32_t length;
} SourceLoc;

/* AST节点种类 (38种) */
typedef enum ASTKind {
    /* 字面量 (6) */
    AST_LIT_NUM,
    AST_LIT_STR,
    AST_LIT_CHAR,
    AST_LIT_BOOL,
    AST_LIT_NULL,
    AST_LIT_UNDEF,
    
    /* 标识符 (1) */
    AST_IDENT,
    
    /* 二元运算 (17) */
    AST_BIN_ADD, AST_BIN_SUB, AST_BIN_MUL, AST_BIN_DIV, AST_BIN_MOD,
    AST_BIN_POW, AST_BIN_EQ, AST_BIN_NE, AST_BIN_LT, AST_BIN_LE,
    AST_BIN_GT, AST_BIN_GE, AST_BIN_AND, AST_BIN_OR,
    AST_BIN_BW_AND, AST_BIN_BW_OR, AST_BIN_BW_XOR,
    
    /* 一元运算 (2) */
    AST_UN_NEG, AST_UN_NOT,
    
    /* 复杂表达式 (7) */
    AST_CALL, AST_INDEX, AST_MEMBER, AST_METHOD_CHAIN,
    AST_ARRAY, AST_OBJECT, AST_FUNCTION,
    
    /* 语句 (10) */
    AST_BLOCK, AST_IF, AST_LOOP_REPEAT, AST_LOOP_FOR,
    AST_LOOP_FOREACH, AST_RETURN, AST_EXPR_STMT,
    AST_VAR_DECL, AST_VAR_ASSIGN, AST_CONST_DECL,
    
    /* 顶层 (1) */
    AST_PROGRAM,
} ASTKind;

/* AST节点基类 */
typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTKind kind;
    SourceLoc loc;
    
    union {
        /* 字面量 */
        struct { double value; } num;
        struct { const char* value; size_t len; } str;
        struct { char value; } chr;
        struct { bool value; } boolean;
        
        /* 标识符 */
        struct { const char* name; } ident;
        
        /* 二元运算 */
        struct {
            ASTNode* left;
            ASTNode* right;
        } binary;
        
        /* 一元运算 */
        struct {
            ASTNode* operand;
        } unary;
        
        /* 函数调用 */
        struct {
            ASTNode* callee;
            ASTNode** args;
            size_t arg_count;
        } call;
        
        /* 数组索引 */
        struct {
            ASTNode* object;
            ASTNode* index;
        } index;
        
        /* 成员访问 */
        struct {
            ASTNode* object;
            const char* member;
        } member;
        
        /* 数组字面量 */
        struct {
            ASTNode** elements;
            size_t count;
        } array;
        
        /* 对象字面量 */
        struct {
            const char** keys;
            ASTNode** values;
            size_t count;
        } object;
        
        /* 函数表达式 */
        struct {
            const char** params;
            size_t param_count;
            ASTNode* body;
        } function;
        
        /* 块语句 */
        struct {
            ASTNode** stmts;
            size_t count;
        } block;
        
        /* if语句 */
        struct {
            ASTNode* condition;
            ASTNode* then_branch;
            ASTNode* else_branch;
            
            /* 链式if */
            ASTNode** chain_conds;
            ASTNode** chain_blocks;
            size_t chain_count;
        } if_stmt;
        
        /* 循环语句 */
        struct {
            enum {
                LOOP_REPEAT,
                LOOP_FOR,
                LOOP_FOREACH,
            } kind;
            
            union {
                struct { ASTNode* count; } repeat;
                struct {
                    ASTNode* init;
                    ASTNode* cond;
                    ASTNode* update;
                } for_loop;
                struct {
                    ASTNode* iterable;
                    const char* item;
                } foreach;
            } as;
            
            ASTNode* body;
        } loop;
        
        /* return语句 */
        struct {
            ASTNode* value;  // 可为NULL
        } return_stmt;
        
        /* 变量声明 */
        struct {
            const char* name;
            const char* type;  // 可为NULL
            ASTNode* init;     // 可为NULL
            bool is_const;
        } var_decl;
        
        /* 赋值语句 */
        struct {
            const char* target;
            ASTNode* value;
        } assign;
        
        /* 顶层程序 */
        struct {
            ASTNode** decls;
            size_t count;
        } program;
    } as;
};

/* AST构造函数 */
ASTNode* ast_create_num(Arena* arena, double value, SourceLoc loc);
ASTNode* ast_create_str(Arena* arena, const char* value, size_t len, SourceLoc loc);
ASTNode* ast_create_ident(Arena* arena, const char* name, SourceLoc loc);
ASTNode* ast_create_binary(Arena* arena, ASTKind op, ASTNode* left, ASTNode* right, SourceLoc loc);
ASTNode* ast_create_call(Arena* arena, ASTNode* callee, ASTNode** args, size_t arg_count, SourceLoc loc);
// ... 更多构造函数

/* AST工具 */
void ast_print(ASTNode* node, int indent);
void ast_free_recursive(ASTNode* node);  // 仅用于非Arena情况

#endif
```

---

### Phase 2: 诊断系统 (Day 3-5)

#### 2.1 诊断引擎

**文件**: `src/core/diagnostic.c`, `include/flyuxc/diagnostic.h`

```c
/* diagnostic.h */
#ifndef FLYUXC_DIAGNOSTIC_H
#define FLYUXC_DIAGNOSTIC_H

#include "ast.h"
#include <stdbool.h>

/* 诊断级别 */
typedef enum DiagLevel {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_HELP,
} DiagLevel;

/* 诊断消息 */
typedef struct Diagnostic {
    DiagLevel level;
    SourceLoc primary;
    const char* message;
    
    /* 次要标注 */
    SourceLoc* secondary;
    const char** secondary_msgs;
    size_t secondary_count;
    
    /* 修复建议 */
    const char* suggestion;
    SourceLoc suggestion_loc;
} Diagnostic;

/* 诊断引擎 */
typedef struct DiagEngine {
    Diagnostic* diagnostics;
    size_t count;
    size_t capacity;
    
    const char* source_code;
    const char* filename;
    
    /* 配置 */
    bool colored_output;
    bool show_context;
    int max_errors;
    
    /* 统计 */
    size_t error_count;
    size_t warning_count;
} DiagEngine;

/* API */
DiagEngine* diag_create(const char* filename, const char* source);
void diag_emit(DiagEngine* engine, const Diagnostic* diag);
void diag_error(DiagEngine* engine, SourceLoc loc, const char* fmt, ...);
void diag_warning(DiagEngine* engine, SourceLoc loc, const char* fmt, ...);
void diag_note(DiagEngine* engine, SourceLoc loc, const char* fmt, ...);
void diag_suggest(DiagEngine* engine, SourceLoc loc, const char* replacement, const char* msg);

/* 输出 */
void diag_print_all(DiagEngine* engine);
bool diag_has_errors(DiagEngine* engine);
void diag_destroy(DiagEngine* engine);

#endif
```

**实现功能**:
- 彩色终端输出(ANSI codes)
- 多span标注
- 源码上下文显示
- 智能建议生成
- 中英文双语支持

**示例输出**:
```
error: expected ')' after parameter list
  ┌─ test.fx:5:15
  │
5 │ add := (a, b {
  │              ^ expected ')'
  │
  = help: try adding ')' before '{'
```

#### 2.2 错误恢复机制

**文件**: `src/parser/recovery.c`

```c
/* recovery.c - 错误恢复策略 */

/* 同步点token */
static const TokenKind SYNC_TOKENS[] = {
    TK_SEMICOLON,
    TK_RBRACE,
    TK_KEYWORD_IF,
    TK_KEYWORD_L,
    TK_KEYWORD_R,
    TK_EOF,
};

/* Panic模式恢复 */
void parser_synchronize(Parser* p) {
    p->panic_mode = false;
    
    while (!parser_at_end(p)) {
        // 语句边界
        if (parser_previous(p)->kind == TK_SEMICOLON) {
            return;
        }
        
        // 同步token
        TokenKind current = parser_peek(p)->kind;
        for (size_t i = 0; i < ARRAY_LEN(SYNC_TOKENS); i++) {
            if (current == SYNC_TOKENS[i]) {
                return;
            }
        }
        
        parser_advance(p);
    }
}

/* 智能跳过错误部分 */
void parser_skip_until_sync(Parser* p) {
    int brace_depth = 0;
    
    while (!parser_at_end(p)) {
        Token* tok = parser_peek(p);
        
        if (tok->kind == TK_LBRACE) brace_depth++;
        if (tok->kind == TK_RBRACE) {
            if (brace_depth > 0) {
                brace_depth--;
            } else {
                return;  // 找到匹配的}
            }
        }
        
        // 在同级别找到同步点
        if (brace_depth == 0 && is_sync_token(tok->kind)) {
            return;
        }
        
        parser_advance(p);
    }
}
```

---

### Phase 3: 表达式解析 (Day 6-9)

#### 3.1 Pratt解析器核心

**文件**: `src/parser/expr.c`

```c
/* expr.c - 表达式解析 */

/* 优先级 */
typedef enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // ||
    PREC_AND,           // &&
    PREC_BW_OR,         // |
    PREC_BW_XOR,        // ^
    PREC_BW_AND,        // &
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * / %
    PREC_POWER,         // **
    PREC_UNARY,         // ! -
    PREC_CALL,          // () [] .
    PREC_PRIMARY,
} Precedence;

/* 解析函数类型 */
typedef ASTNode* (*PrefixFn)(Parser* p);
typedef ASTNode* (*InfixFn)(Parser* p, ASTNode* left);

/* 解析规则 */
typedef struct ParseRule {
    PrefixFn prefix;
    InfixFn infix;
    Precedence precedence;
} ParseRule;

/* 规则表 */
static ParseRule parse_rules[] = {
    [TK_LPAREN]   = {parse_grouping, parse_call, PREC_CALL},
    [TK_MINUS]    = {parse_unary, parse_binary, PREC_TERM},
    [TK_PLUS]     = {NULL, parse_binary, PREC_TERM},
    [TK_STAR]     = {NULL, parse_binary, PREC_FACTOR},
    [TK_SLASH]    = {NULL, parse_binary, PREC_FACTOR},
    [TK_PERCENT]  = {NULL, parse_binary, PREC_FACTOR},
    [TK_POWER]    = {NULL, parse_binary, PREC_POWER},
    [TK_BANG]     = {parse_unary, NULL, PREC_NONE},
    [TK_LT]       = {NULL, parse_comparison, PREC_COMPARISON},
    [TK_GT]       = {NULL, parse_comparison, PREC_COMPARISON},
    [TK_LE]       = {NULL, parse_comparison, PREC_COMPARISON},
    [TK_GE]       = {NULL, parse_comparison, PREC_COMPARISON},
    [TK_EQ]       = {NULL, parse_binary, PREC_EQUALITY},
    [TK_NE]       = {NULL, parse_binary, PREC_EQUALITY},
    [TK_AND]      = {NULL, parse_binary, PREC_AND},
    [TK_OR]       = {NULL, parse_binary, PREC_OR},
    [TK_BW_AND]   = {NULL, parse_binary, PREC_BW_AND},
    [TK_BW_OR]    = {NULL, parse_binary, PREC_BW_OR},
    [TK_BW_XOR]   = {NULL, parse_binary, PREC_BW_XOR},
    [TK_NUMBER]   = {parse_number, NULL, PREC_NONE},
    [TK_STRING]   = {parse_string, NULL, PREC_NONE},
    [TK_TRUE]     = {parse_literal, NULL, PREC_NONE},
    [TK_FALSE]    = {parse_literal, NULL, PREC_NONE},
    [TK_NULL]     = {parse_literal, NULL, PREC_NONE},
    [TK_UNDEF]    = {parse_literal, NULL, PREC_NONE},
    [TK_IDENTIFIER] = {parse_identifier, NULL, PREC_NONE},
    [TK_LBRACKET] = {parse_array, parse_index, PREC_CALL},
    [TK_LBRACE]   = {parse_object, NULL, PREC_NONE},
    [TK_DOT]      = {NULL, parse_member, PREC_CALL},
    [TK_METHOD_CHAIN] = {NULL, parse_method_chain, PREC_CALL},
};

/* Pratt解析核心 */
ASTNode* parse_precedence(Parser* p, Precedence precedence) {
    Token* token = parser_advance(p);
    ParseRule* rule = &parse_rules[token->kind];
    
    if (rule->prefix == NULL) {
        diag_error(p->diag, token_loc(token), "expected expression");
        parser_synchronize(p);
        return NULL;
    }
    
    ASTNode* left = rule->prefix(p);
    if (left == NULL) return NULL;
    
    while (precedence <= get_current_precedence(p)) {
        token = parser_advance(p);
        InfixFn infix = parse_rules[token->kind].infix;
        left = infix(p, left);
        if (left == NULL) return NULL;
    }
    
    return left;
}

/* 入口 */
ASTNode* parse_expression(Parser* p) {
    return parse_precedence(p, PREC_ASSIGNMENT);
}
```

#### 3.2 复杂表达式

```c
/* 函数调用 */
ASTNode* parse_call(Parser* p, ASTNode* callee) {
    SourceLoc loc = token_loc(parser_previous(p));
    
    ASTNode** args = NULL;
    size_t arg_count = 0;
    size_t capacity = 0;
    
    if (!parser_check(p, TK_RPAREN)) {
        do {
            if (arg_count >= 255) {
                diag_error(p->diag, loc, "too many arguments (max 255)");
                return NULL;
            }
            
            ASTNode* arg = parse_expression(p);
            if (arg == NULL) return NULL;
            
            ARRAY_PUSH(args, arg_count, capacity, arg);
        } while (parser_match(p, TK_COMMA));
    }
    
    if (!parser_consume(p, TK_RPAREN, "expected ')' after arguments")) {
        return NULL;
    }
    
    return ast_create_call(p->ast_arena, callee, args, arg_count, loc);
}

/* 数组字面量 */
ASTNode* parse_array(Parser* p) {
    SourceLoc loc = token_loc(parser_previous(p));
    
    ASTNode** elements = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    if (!parser_check(p, TK_RBRACKET)) {
        do {
            ASTNode* elem = parse_expression(p);
            if (elem == NULL) return NULL;
            
            ARRAY_PUSH(elements, count, capacity, elem);
        } while (parser_match(p, TK_COMMA));
    }
    
    if (!parser_consume(p, TK_RBRACKET, "expected ']' after array elements")) {
        return NULL;
    }
    
    return ast_create_array(p->ast_arena, elements, count, loc);
}
```

---

### Phase 4: 语句解析 (Day 10-13)

#### 4.1 变量声明

```c
/* 变量声明解析 */
ASTNode* parse_var_declaration(Parser* p) {
    Token* name = parser_consume(p, TK_IDENTIFIER, "expected variable name");
    if (name == NULL) return NULL;
    
    const char* var_name = intern_token_string(p, name);
    const char* type_name = NULL;
    bool is_const = false;
    ASTNode* initializer = NULL;
    
    if (parser_match(p, TK_COLON)) {
        // 显式类型
        if (parser_match(p, TK_LPAREN)) {
            // 常量: name :(type)= value
            is_const = true;
            type_name = parse_type(p);
            parser_consume(p, TK_RPAREN, "expected ')' after type");
            parser_consume(p, TK_ASSIGN, "expected '=' after type");
            initializer = parse_expression(p);
        } else if (parser_match(p, TK_LBRACKET)) {
            // 变量: name :[type]= value 或 name :[type]
            type_name = parse_type(p);
            parser_consume(p, TK_RBRACKET, "expected ']' after type");
            
            if (parser_match(p, TK_ASSIGN)) {
                initializer = parse_expression(p);
            }
        }
    } else if (parser_consume(p, TK_DEFINE, "expected ':=' or ':'")) {
        // 类型推断: name := value
        initializer = parse_expression(p);
        if (initializer == NULL) {
            diag_error(p->diag, token_loc(name), 
                      "variable declaration requires initializer");
            return NULL;
        }
    }
    
    return ast_create_var_decl(p->ast_arena, var_name, type_name, 
                               initializer, is_const, token_loc(name));
}
```

#### 4.2 if语句(链式支持)

```c
/* if语句解析 */
ASTNode* parse_if_statement(Parser* p) {
    SourceLoc loc = token_loc(parser_previous(p));
    
    // 第一个条件
    parser_consume(p, TK_LPAREN, "expected '(' after 'if'");
    ASTNode* condition = parse_expression(p);
    parser_consume(p, TK_RPAREN, "expected ')' after condition");
    
    ASTNode* then_branch = parse_block(p);
    
    // 链式if: (cond2) { } (cond3) { }
    ASTNode** chain_conds = NULL;
    ASTNode** chain_blocks = NULL;
    size_t chain_count = 0;
    size_t capacity = 0;
    
    while (parser_check(p, TK_LPAREN) && 
           !parser_check_ahead(p, 1, TK_IDENTIFIER)) {
        parser_advance(p);  // consume '('
        
        ASTNode* chain_cond = parse_expression(p);
        parser_consume(p, TK_RPAREN, "expected ')' after condition");
        ASTNode* chain_block = parse_block(p);
        
        ARRAY_PUSH(chain_conds, chain_count, capacity, chain_cond);
        ARRAY_PUSH(chain_blocks, chain_count, capacity, chain_block);
        capacity = (capacity == 0) ? capacity : capacity / 2;  // 调整
    }
    
    // else分支
    ASTNode* else_branch = NULL;
    if (parser_match(p, TK_LBRACE)) {
        else_branch = parse_block_impl(p);
    }
    
    return ast_create_if(p->ast_arena, condition, then_branch, else_branch,
                        chain_conds, chain_blocks, chain_count, loc);
}
```

#### 4.3 循环语句

```c
/* 循环语句解析 */
ASTNode* parse_loop_statement(Parser* p) {
    SourceLoc loc = token_loc(parser_previous(p));
    
    if (parser_match(p, TK_LBRACKET)) {
        // 重复循环: L> [n] { }
        ASTNode* count = parse_expression(p);
        parser_consume(p, TK_RBRACKET, "expected ']' after count");
        ASTNode* body = parse_block(p);
        
        return ast_create_loop_repeat(p->ast_arena, count, body, loc);
        
    } else if (parser_consume(p, TK_LPAREN, "expected '(' or '[' after 'L>'")) {
        
        // 区分for和foreach
        size_t checkpoint = p->current;
        bool is_foreach = false;
        
        // 向前看，寻找 ':'
        int paren_depth = 1;
        while (paren_depth > 0 && !parser_at_end(p)) {
            Token* tok = parser_peek(p);
            if (tok->kind == TK_LPAREN) paren_depth++;
            if (tok->kind == TK_RPAREN) paren_depth--;
            if (tok->kind == TK_COLON && paren_depth == 1) {
                is_foreach = true;
                break;
            }
            parser_advance(p);
        }
        
        // 回退
        p->current = checkpoint;
        
        if (is_foreach) {
            // foreach: L> (arr : item) { }
            ASTNode* iterable = parse_expression(p);
            parser_consume(p, TK_COLON, "expected ':'");
            Token* item = parser_consume(p, TK_IDENTIFIER, "expected item name");
            parser_consume(p, TK_RPAREN, "expected ')'");
            ASTNode* body = parse_block(p);
            
            const char* item_name = intern_token_string(p, item);
            return ast_create_loop_foreach(p->ast_arena, iterable, 
                                          item_name, body, loc);
        } else {
            // for: L> (init; cond; update) { }
            ASTNode* init = parse_var_declaration(p);
            parser_consume(p, TK_SEMICOLON, "expected ';' after init");
            ASTNode* cond = parse_expression(p);
            parser_consume(p, TK_SEMICOLON, "expected ';' after condition");
            ASTNode* update = parse_expression(p);
            parser_consume(p, TK_RPAREN, "expected ')'");
            ASTNode* body = parse_block(p);
            
            return ast_create_loop_for(p->ast_arena, init, cond, 
                                      update, body, loc);
        }
    }
    
    return NULL;
}
```

---

### Phase 5: 集成与测试 (Day 14-17)

#### 5.1 Parser主入口

```c
/* parser.c - 主入口 */

/* 创建Parser */
Parser* parser_create(const char* source, const char* filename) {
    Parser* p = malloc(sizeof(Parser));
    
    // Lexer
    p->tokens = lex_source(source);
    p->token_count = token_array_count(p->tokens);
    p->current = 0;
    
    // 内存管理
    p->ast_arena = arena_create(64 * 1024);  // 64KB
    p->string_pool = string_pool_create(p->ast_arena);
    
    // 诊断
    p->diag = diag_create(filename, source);
    p->had_error = false;
    p->panic_mode = false;
    
    // 配置
    p->config = (ParserConfig){
        .error_recovery = true,
        .partial_parsing = true,
        .collect_comments = false,
        .max_errors = 50,
        .colored_output = isatty(STDERR_FILENO),
    };
    
    return p;
}

/* 解析程序 */
ASTNode* parser_parse(Parser* p) {
    ASTNode** decls = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    while (!parser_at_end(p)) {
        ASTNode* decl = parse_declaration(p);
        
        if (decl != NULL) {
            ARRAY_PUSH(decls, count, capacity, decl);
        }
        
        // 错误恢复
        if (p->panic_mode) {
            parser_synchronize(p);
        }
        
        // 错误限制
        if (p->diag->error_count >= p->config.max_errors) {
            diag_error(p->diag, (SourceLoc){0}, 
                      "too many errors, stopping");
            break;
        }
    }
    
    // 打印诊断
    if (diag_has_errors(p->diag)) {
        diag_print_all(p->diag);
    }
    
    return ast_create_program(p->ast_arena, decls, count);
}

/* 销毁Parser */
void parser_destroy(Parser* p) {
    arena_destroy(p->ast_arena);
    string_pool_destroy(p->string_pool);
    diag_destroy(p->diag);
    token_array_free(p->tokens);
    free(p);
}
```

#### 5.2 测试套件

```c
/* tests/test_parser.c */

#include "test_framework.h"
#include "parser.h"

/* 表达式测试 */
TEST(parse_binary_expr) {
    Parser* p = parser_create("a + b * c", "test.fx");
    ASTNode* ast = parse_expression(p);
    
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->kind, AST_BIN_ADD);
    ASSERT_EQ(ast->as.binary.right->kind, AST_BIN_MUL);
    
    parser_destroy(p);
}

TEST(parse_power_precedence) {
    Parser* p = parser_create("2 ** 3 ** 4", "test.fx");
    ASTNode* ast = parse_expression(p);
    
    // 应该是右结合: 2 ** (3 ** 4)
    ASSERT_EQ(ast->kind, AST_BIN_POW);
    ASSERT_EQ(ast->as.binary.right->kind, AST_BIN_POW);
    
    parser_destroy(p);
}

TEST(parse_function_call) {
    Parser* p = parser_create("print(1, 2, 3)", "test.fx");
    ASTNode* ast = parse_expression(p);
    
    ASSERT_EQ(ast->kind, AST_CALL);
    ASSERT_EQ(ast->as.call.arg_count, 3);
    
    parser_destroy(p);
}

/* 语句测试 */
TEST(parse_if_statement) {
    const char* src = "if (x > 0) { print(x) }";
    Parser* p = parser_create(src, "test.fx");
    ASTNode* ast = parse_statement(p);
    
    ASSERT_EQ(ast->kind, AST_IF);
    ASSERT_NOT_NULL(ast->as.if_stmt.condition);
    ASSERT_NOT_NULL(ast->as.if_stmt.then_branch);
    
    parser_destroy(p);
}

TEST(parse_chain_if) {
    const char* src = "if (x < 10) { a } (x < 20) { b } { c }";
    Parser* p = parser_create(src, "test.fx");
    ASTNode* ast = parse_statement(p);
    
    ASSERT_EQ(ast->kind, AST_IF);
    ASSERT_EQ(ast->as.if_stmt.chain_count, 1);
    ASSERT_NOT_NULL(ast->as.if_stmt.else_branch);
    
    parser_destroy(p);
}

/* 错误恢复测试 */
TEST(error_recovery_missing_paren) {
    const char* src = "add := (a, b { R> a + b }; x := 5;";
    Parser* p = parser_create(src, "test.fx");
    ASTNode* ast = parser_parse(p);
    
    // 应该有错误
    ASSERT_TRUE(p->had_error);
    
    // 但应该恢复并解析到 x := 5
    ASSERT_NOT_NULL(ast);
    ASSERT_GT(ast->as.program.count, 0);
    
    parser_destroy(p);
}

/* 性能测试 */
TEST(benchmark_large_file) {
    // 加载10000行测试文件
    char* source = load_file("tests/fixtures/large.fx");
    
    uint64_t start = get_microseconds();
    
    Parser* p = parser_create(source, "large.fx");
    ASTNode* ast = parser_parse(p);
    
    uint64_t end = get_microseconds();
    uint64_t elapsed = end - start;
    
    // 应该在100ms内完成
    ASSERT_LT(elapsed, 100000);
    
    printf("Parsed 10000 lines in %lu us\n", elapsed);
    
    parser_destroy(p);
    free(source);
}

/* 运行所有测试 */
int main(void) {
    RUN_TEST(parse_binary_expr);
    RUN_TEST(parse_power_precedence);
    RUN_TEST(parse_function_call);
    RUN_TEST(parse_if_statement);
    RUN_TEST(parse_chain_if);
    RUN_TEST(error_recovery_missing_paren);
    RUN_TEST(benchmark_large_file);
    
    PRINT_TEST_SUMMARY();
    return TEST_FAILED_COUNT > 0 ? 1 : 0;
}
```

---

## 📊 验收标准

### 功能完整性

- ✅ 所有表达式类型解析正确
- ✅ 所有语句类型解析正确
- ✅ 运算符优先级符合规范
- ✅ 错误恢复机制工作正常
- ✅ 诊断信息清晰准确

### 性能指标

- ✅ 解析速度 > 10 MB/s
- ✅ 内存使用 < 2MB per 100KB
- ✅ 10000行代码 < 100ms

### 代码质量

- ✅ 测试覆盖率 > 90%
- ✅ 无内存泄漏(Valgrind检查)
- ✅ 无未定义行为(UBSan检查)
- ✅ 文档完整

---

## 🎯 下一步

Parser实现完成后，进入**语义分析**阶段：

1. 符号表构建
2. 类型检查
3. 作用域解析
4. 常量折叠
5. 死代码检测

**最终目标**: 生成LLVM IR，完成AOT编译流程！

---

**文档版本**: 2.0  
**最后更新**: 2025-11-17  
**预计完成**: 2025-12-08
