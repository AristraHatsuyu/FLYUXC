# FLYUX Parser 设计文档 V2.0 - 现代化生产级实现

**设计理念**: 参考 Rust Compiler (rustc) 和 Go Compiler (gc) 的现代架构  
**更新日期**: 2025-11-17  
**目标**: 生产级、高性能、优秀的错误诊断

---

## 📋 目录

1. [设计目标](#设计目标)
2. [整体架构](#整体架构)
3. [核心数据结构](#核心数据结构)
4. [错误处理系统](#错误处理系统)
5. [AST设计](#AST设计)
6. [Parser实现策略](#Parser实现策略)
7. [性能优化](#性能优化)
8. [测试策略](#测试策略)

---

## 🎯 设计目标

### 1. 现代编译器标准

参考业界最佳实践：

| 特性 | Rust rustc | Go gc | FLYUX Parser |
|------|-----------|-------|--------------|
| 错误恢复 | ✅ Panic/Recovery | ✅ Error Recovery | ✅ Sync Points |
| 增量解析 | ✅ Query-based | ✅ Partial | ✅ Module-level |
| 丰富诊断 | ✅ Multi-span | ✅ Suggestions | ✅ Context + Hints |
| 零拷贝 | ✅ Interned strings | ✅ String refs | ✅ Arena allocation |
| 并发安全 | ✅ Send/Sync | ✅ Goroutines | ✅ Immutable AST |

### 2. 核心设计原则

```
┌─────────────────────────────────────────────────────────┐
│  1. 健壮性 (Robustness)                                  │
│     • 永不崩溃，即使输入有误                              │
│     • 错误恢复继续解析                                    │
│     • 提供部分AST用于IDE                                 │
│                                                         │
│  2. 性能 (Performance)                                  │
│     • O(n) 线性时间复杂度                                │
│     • 零拷贝字符串处理                                    │
│     • Arena内存分配                                      │
│     • 懒加载和增量解析                                    │
│                                                         │
│  3. 诊断质量 (Diagnostics)                               │
│     • 精确的错误位置                                      │
│     • 上下文相关的建议                                    │
│     • 多语言支持                                         │
│     • 颜色高亮输出                                       │
│                                                         │
│  4. 可维护性 (Maintainability)                           │
│     • 清晰的模块划分                                      │
│     • 完善的测试覆盖                                      │
│     • 文档齐全                                           │
│     • 易于扩展                                           │
└─────────────────────────────────────────────────────────┘
```

---

## 🏗️ 整体架构

### 分层设计

```
┌─────────────────────────────────────────────────────────┐
│                     Parser Frontend                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │   Lexer     │→│ Token Buffer │→│ Parser Core  │  │
│  │  (词法器)    │  │  (Token流)   │  │  (语法分析)   │  │
│  └─────────────┘  └──────────────┘  └──────────────┘  │
│         ↓                                    ↓          │
│  ┌─────────────┐                    ┌──────────────┐  │
│  │ String Pool │                    │     AST      │  │
│  │ (字符串池)   │                    │  (语法树)     │  │
│  └─────────────┘                    └──────────────┘  │
│         ↓                                    ↓          │
│  ┌─────────────────────────────────────────────────┐  │
│  │          Diagnostic Engine (诊断引擎)            │  │
│  │   • Error Collection                            │  │
│  │   • Warning Management                          │  │
│  │   • Hint Generation                             │  │
│  └─────────────────────────────────────────────────┘  │
│         ↓                                              │
│  ┌─────────────────────────────────────────────────┐  │
│  │        Error Recovery (错误恢复)                  │  │
│  │   • Panic Mode                                  │  │
│  │   • Synchronization Points                      │  │
│  │   • Partial AST Construction                    │  │
│  └─────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                         ↓
            ┌────────────────────────┐
            │   Semantic Analyzer    │
            │     (语义分析器)         │
            └────────────────────────┘
```

### 模块划分

```c
src/
├── core/
│   ├── lexer.c          // 词法分析 (已完成)
│   ├── parser.c         // 核心Parser (待实现)
│   ├── ast.c            // AST构造和操作 (待实现)
│   ├── diagnostic.c     // 诊断系统 (待实现)
│   └── arena.c          // 内存管理 (待实现)
├── parser/
│   ├── expr.c           // 表达式解析
│   ├── stmt.c           // 语句解析
│   ├── decl.c           // 声明解析
│   ├── type.c           // 类型解析
│   └── recovery.c       // 错误恢复
└── include/flyuxc/
    ├── ast.h
    ├── parser.h
    ├── diagnostic.h
    └── arena.h
```

---

## 🧱 核心数据结构

### 1. Parser State (解析器状态)

```c
/* Parser核心状态 - 线程局部 */
typedef struct Parser {
    /* ===== Token流管理 ===== */
    Token* tokens;              // Token数组
    size_t token_count;         // Token总数
    size_t current;             // 当前位置
    
    /* ===== 上下文追踪 ===== */
    ParserContext* context;     // 当前上下文栈
    int depth;                  // 嵌套深度
    
    /* ===== 内存管理 ===== */
    Arena* ast_arena;           // AST节点内存池
    StringPool* string_pool;    // 字符串池
    
    /* ===== 诊断系统 ===== */
    DiagnosticEngine* diag;     // 诊断引擎
    bool had_error;             // 是否有错误
    bool panic_mode;            // 是否在panic模式
    
    /* ===== 配置选项 ===== */
    ParserConfig config;        // 配置参数
    
    /* ===== 统计信息 ===== */
    ParseStats stats;           // 性能统计
} Parser;

/* 解析上下文 */
typedef enum ParserContext {
    CTX_TOP_LEVEL,      // 顶层
    CTX_FUNCTION,       // 函数体内
    CTX_BLOCK,          // 块语句内
    CTX_LOOP,           // 循环内
    CTX_IF,             // if语句内
    CTX_EXPRESSION,     // 表达式内
} ParserContext;

/* 配置选项 */
typedef struct ParserConfig {
    bool error_recovery;        // 是否启用错误恢复
    bool partial_parsing;       // 是否生成部分AST
    bool collect_comments;      // 是否保留注释
    int max_errors;             // 最大错误数
    bool colored_output;        // 彩色输出
} ParserConfig;
```

### 2. Arena内存分配器

```c
/* Arena分配器 - 零碎片，快速释放 */
typedef struct Arena {
    uint8_t* buffer;        // 内存块
    size_t capacity;        // 容量
    size_t used;            // 已用
    Arena* next;            // 链表下一个
} Arena;

/* Arena API */
Arena* arena_create(size_t initial_size);
void* arena_alloc(Arena* arena, size_t size);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);

/* AST节点分配 - 快速且缓存友好 */
#define ALLOC_NODE(parser, type) \
    ((type*)arena_alloc((parser)->ast_arena, sizeof(type)))
```

### 3. 字符串池 (String Interning)

```c
/* 字符串池 - 去重和快速比较 */
typedef struct StringPool {
    char** strings;         // 字符串数组
    size_t* hashes;         // 哈希值
    size_t count;
    size_t capacity;
} StringPool;

/* 字符串API */
const char* intern_string(StringPool* pool, const char* str, size_t len);
bool string_equal(const char* a, const char* b);  // O(1) 指针比较
```

---

## 🚨 错误处理系统

### 1. 诊断引擎设计

```c
/* 诊断级别 */
typedef enum DiagLevel {
    DIAG_ERROR,         // 错误 (编译失败)
    DIAG_WARNING,       // 警告 (可能的问题)
    DIAG_NOTE,          // 提示 (补充信息)
    DIAG_HELP,          // 帮助 (建议修复)
} DiagLevel;

/* 诊断信息 */
typedef struct Diagnostic {
    DiagLevel level;
    
    /* 主要位置 */
    SourceLocation primary_loc;
    const char* message;
    
    /* 次要标注 (多span) */
    SourceLabel* labels;
    size_t label_count;
    
    /* 修复建议 */
    Suggestion* suggestions;
    size_t suggestion_count;
    
    /* 上下文代码 */
    const char* source_snippet;
} Diagnostic;

/* 源码位置 */
typedef struct SourceLocation {
    const char* file;
    uint32_t line;
    uint32_t column;
    uint32_t offset;        // 字节偏移
    uint32_t length;        // 错误span长度
} SourceLocation;

/* 标注 */
typedef struct SourceLabel {
    SourceLocation loc;
    const char* message;
    DiagLevel level;
} SourceLabel;

/* 修复建议 */
typedef struct Suggestion {
    SourceLocation loc;
    const char* replacement;
    const char* message;
} Suggestion;
```

### 2. 错误报告示例

```
error: expected ')' after parameter list
  ┌─ test.fx:5:15
  │
5 │ add := (a, b {
  │              ^ expected ')'
  │
  = help: try adding ')' before '{'
  = note: function parameters must be enclosed in parentheses

error: unexpected token in expression
  ┌─ test.fx:12:9
  │
12│     x = := 5
  │         ^^ unexpected ':='
  │
  = help: did you mean to use '=' instead of ':='?
  = note: ':=' is for variable declaration, use '=' for assignment

warning: unused variable 'result'
  ┌─ test.fx:20:5
  │
20│     result := calculate()
  │     ^^^^^^ unused variable
  │
  = help: consider using '_' if the value is intentionally unused
  = help: or remove this variable declaration
```

### 3. 错误恢复策略

```c
/* 同步点 - 在这些token处恢复 */
static const TokenKind SYNC_TOKENS[] = {
    TK_SEMICOLON,       // ;
    TK_RBRACE,          // }
    TK_EOF,             // 文件结束
    TK_KEYWORD_IF,      // if
    TK_KEYWORD_L,       // L>
    TK_KEYWORD_R,       // R>
    TK_IDENTIFIER,      // 标识符
};

/* 错误恢复函数 */
void parser_synchronize(Parser* p) {
    p->panic_mode = false;
    
    while (!at_end(p)) {
        // 在语句边界处停止
        if (previous(p)->kind == TK_SEMICOLON) return;
        
        // 在同步token处停止
        TokenKind current = peek(p)->kind;
        for (size_t i = 0; i < ARRAY_SIZE(SYNC_TOKENS); i++) {
            if (current == SYNC_TOKENS[i]) return;
        }
        
        advance(p);
    }
}

/* 错误报告辅助 */
void parser_error_at(Parser* p, Token* token, const char* message) {
    if (p->panic_mode) return;  // 避免错误雪崩
    
    p->panic_mode = true;
    p->had_error = true;
    
    // 创建诊断信息
    Diagnostic diag = {
        .level = DIAG_ERROR,
        .primary_loc = token_to_location(token),
        .message = message,
    };
    
    // 添加到诊断引擎
    diagnostic_emit(p->diag, &diag);
    
    // 错误恢复
    if (p->config.error_recovery) {
        parser_synchronize(p);
    }
}
```

---

## 🌳 AST设计

### 1. AST节点类型系统

```c
/* AST节点基类 */
typedef struct ASTNode {
    ASTNodeKind kind;
    SourceLocation loc;
    
    union {
        /* 表达式 */
        struct {
            BinaryExpr binary;
            UnaryExpr unary;
            CallExpr call;
            IndexExpr index;
            MemberExpr member;
            LiteralExpr literal;
            IdentExpr ident;
            ArrayExpr array;
            ObjectExpr object;
            FunctionExpr function;
        } expr;
        
        /* 语句 */
        struct {
            BlockStmt block;
            IfStmt if_stmt;
            LoopStmt loop;
            ReturnStmt return_stmt;
            ExprStmt expr_stmt;
            VarDeclStmt var_decl;
        } stmt;
        
        /* 声明 */
        struct {
            FunctionDecl func_decl;
            VarDecl var_decl;
        } decl;
    } as;
} ASTNode;

/* AST节点种类 */
typedef enum ASTNodeKind {
    /* ===== 表达式 (26种) ===== */
    
    // 字面量
    AST_LITERAL_NUM,        // 123, 3.14
    AST_LITERAL_STR,        // "hello"
    AST_LITERAL_CHAR,       // 'a'
    AST_LITERAL_BOOL,       // true, false
    AST_LITERAL_NULL,       // null
    AST_LITERAL_UNDEF,      // undef
    
    // 标识符
    AST_IDENTIFIER,         // variable_name
    
    // 二元运算
    AST_BINARY_ADD,         // a + b
    AST_BINARY_SUB,         // a - b
    AST_BINARY_MUL,         // a * b
    AST_BINARY_DIV,         // a / b
    AST_BINARY_MOD,         // a % b
    AST_BINARY_POW,         // a ** b
    AST_BINARY_EQ,          // a == b
    AST_BINARY_NE,          // a != b
    AST_BINARY_LT,          // a < b
    AST_BINARY_LE,          // a <= b
    AST_BINARY_GT,          // a > b
    AST_BINARY_GE,          // a >= b
    AST_BINARY_AND,         // a && b
    AST_BINARY_OR,          // a || b
    AST_BINARY_BW_AND,      // a & b
    AST_BINARY_BW_OR,       // a | b
    AST_BINARY_BW_XOR,      // a ^ b
    
    // 一元运算
    AST_UNARY_NEG,          // -a
    AST_UNARY_NOT,          // !a
    
    // 复杂表达式
    AST_CALL,               // func(args)
    AST_INDEX,              // arr[idx]
    AST_MEMBER,             // obj.prop
    AST_METHOD_CHAIN,       // obj.>method()
    AST_ARRAY,              // [1, 2, 3]
    AST_OBJECT,             // {a: 1, b: 2}
    AST_FUNCTION,           // (a, b) { body }
    AST_CHAIN_COMPARE,      // a < b <= c
    
    /* ===== 语句 (10种) ===== */
    AST_BLOCK,              // { stmts }
    AST_IF,                 // if (cond) { } { }
    AST_LOOP_REPEAT,        // L> [10] { }
    AST_LOOP_FOR,           // L> (init; cond; update) { }
    AST_LOOP_FOREACH,       // L> (arr : item) { }
    AST_RETURN,             // R> value
    AST_EXPR_STMT,          // expression;
    AST_VAR_DECL,           // a := 123
    AST_VAR_ASSIGN,         // a = 456
    AST_CONST_DECL,         // PI :(num)= 3.14
    
    /* ===== 声明 (2种) ===== */
    AST_FUNCTION_DECL,      // func := (params) { }
    AST_PROGRAM,            // 顶层节点
} ASTNodeKind;
```

### 2. 具体AST结构

```c
/* 二元表达式 */
typedef struct BinaryExpr {
    ASTNode* left;
    ASTNode* right;
    TokenKind op;
} BinaryExpr;

/* 函数调用 */
typedef struct CallExpr {
    ASTNode* callee;        // 被调用的表达式
    ASTNode** args;         // 参数列表
    size_t arg_count;
    bool is_method_chain;   // 是否是 .> 调用
} CallExpr;

/* if语句 */
typedef struct IfStmt {
    ASTNode* condition;
    ASTNode* then_block;
    ASTNode* else_block;    // 可为NULL
    
    // 链式if支持
    ASTNode** chain_conditions;
    ASTNode** chain_blocks;
    size_t chain_count;
} IfStmt;

/* 循环语句 */
typedef struct LoopStmt {
    enum {
        LOOP_REPEAT,        // L> [n] { }
        LOOP_FOR,           // L> (init; cond; update) { }
        LOOP_FOREACH,       // L> (arr : item) { }
    } loop_kind;
    
    union {
        struct {
            ASTNode* count;     // 重复次数
        } repeat;
        
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
        } for_loop;
        
        struct {
            ASTNode* iterable;
            const char* item_name;
        } foreach;
    } as;
    
    ASTNode* body;
} LoopStmt;

/* 变量声明 */
typedef struct VarDeclStmt {
    const char* name;
    const char* type;       // 可为NULL (类型推断)
    ASTNode* initializer;   // 可为NULL
    bool is_const;          // 是否是常量
    bool explicit_type;     // 是否显式类型
} VarDeclStmt;

/* 函数声明 */
typedef struct FunctionDecl {
    const char* name;
    const char** params;    // 参数名
    size_t param_count;
    const char* return_type; // 可为NULL
    ASTNode* body;
} FunctionDecl;
```

---

## 🔧 Parser实现策略

### 1. 递归下降 + Pratt解析

```c
/* Pratt解析器 - 用于表达式 */
typedef struct {
    PrefixParseFn prefix;   // 前缀解析函数
    InfixParseFn infix;     // 中缀解析函数
    Precedence precedence;  // 优先级
} ParseRule;

/* 解析规则表 */
static ParseRule rules[] = {
    [TK_PLUS]    = {NULL, parse_binary, PREC_TERM},
    [TK_MINUS]   = {parse_unary, parse_binary, PREC_TERM},
    [TK_STAR]    = {NULL, parse_binary, PREC_FACTOR},
    [TK_SLASH]   = {NULL, parse_binary, PREC_FACTOR},
    [TK_POWER]   = {NULL, parse_binary, PREC_POWER},
    [TK_LT]      = {NULL, parse_binary, PREC_COMPARISON},
    [TK_NUMBER]  = {parse_number, NULL, PREC_NONE},
    [TK_IDENT]   = {parse_identifier, NULL, PREC_NONE},
    // ... 更多规则
};

/* 核心解析函数 */
ASTNode* parse_precedence(Parser* p, Precedence precedence) {
    Token* token = advance(p);
    ParseRule* rule = &rules[token->kind];
    
    if (rule->prefix == NULL) {
        parser_error_at(p, token, "expected expression");
        return NULL;
    }
    
    ASTNode* left = rule->prefix(p, token);
    
    while (precedence <= get_precedence(peek(p))) {
        token = advance(p);
        InfixParseFn infix = rules[token->kind].infix;
        left = infix(p, left, token);
    }
    
    return left;
}
```

### 2. 语句解析

```c
/* 解析语句 */
ASTNode* parse_statement(Parser* p) {
    // 进入语句上下文
    push_context(p, CTX_STATEMENT);
    
    ASTNode* stmt = NULL;
    
    if (match(p, TK_KEYWORD_IF)) {
        stmt = parse_if_statement(p);
    } else if (match(p, TK_KEYWORD_L)) {
        stmt = parse_loop_statement(p);
    } else if (match(p, TK_KEYWORD_R)) {
        stmt = parse_return_statement(p);
    } else if (match(p, TK_LBRACE)) {
        stmt = parse_block_statement(p);
    } else if (check(p, TK_IDENTIFIER) && check_ahead(p, 1, TK_DEFINE)) {
        stmt = parse_var_declaration(p);
    } else {
        stmt = parse_expression_statement(p);
    }
    
    pop_context(p);
    return stmt;
}

/* if语句解析 - 支持链式if */
ASTNode* parse_if_statement(Parser* p) {
    consume(p, TK_LPAREN, "expected '(' after 'if'");
    ASTNode* condition = parse_expression(p);
    consume(p, TK_RPAREN, "expected ')' after condition");
    
    ASTNode* then_block = parse_block(p);
    
    IfStmt* if_stmt = ALLOC_NODE(p, IfStmt);
    if_stmt->condition = condition;
    if_stmt->then_block = then_block;
    
    // 支持链式if: if (a) {} (b) {} (c) {}
    while (match(p, TK_LPAREN)) {
        ASTNode* chain_cond = parse_expression(p);
        consume(p, TK_RPAREN, "expected ')' after condition");
        ASTNode* chain_block = parse_block(p);
        
        add_chain_condition(if_stmt, chain_cond, chain_block);
    }
    
    // else分支
    if (match(p, TK_LBRACE)) {
        if_stmt->else_block = parse_block_impl(p);
    }
    
    return wrap_if_stmt(if_stmt);
}
```

### 3. 增量解析支持

```c
/* 增量解析 - 用于IDE */
typedef struct ParseCache {
    uint64_t file_hash;         // 文件哈希
    ASTNode** function_asts;    // 函数级缓存
    size_t* function_offsets;   // 偏移位置
    size_t count;
} ParseCache;

/* 智能重解析 */
ASTNode* parse_incremental(Parser* p, ParseCache* cache, 
                          size_t changed_offset, size_t changed_len) {
    // 1. 找到受影响的函数
    size_t affected_func = find_affected_function(cache, changed_offset);
    
    // 2. 只重新解析该函数
    ASTNode* new_func = parse_function_at(p, affected_func);
    
    // 3. 更新缓存
    cache->function_asts[affected_func] = new_func;
    
    // 4. 重组完整AST
    return reconstruct_ast(cache);
}
```

---

## ⚡ 性能优化

### 1. 零拷贝Token处理

```c
/* Token不拷贝字符串 - 只存指针和长度 */
typedef struct Token {
    TokenKind kind;
    const char* start;      // 指向源码
    size_t length;
    uint32_t line;
    uint32_t column;
} Token;

/* 快速Token比较 */
bool token_equals(Token* tok, const char* str) {
    size_t len = strlen(str);
    return tok->length == len && 
           memcmp(tok->start, str, len) == 0;
}
```

### 2. Arena内存布局

```
┌────────────────────────────────────────┐
│ Arena 1 (64KB)                         │
├────────────────────────────────────────┤
│ [AST Node] [AST Node] [AST Node] ...   │
│     ↓          ↓          ↓            │
│  Used: 45KB                            │
│  Free: 19KB ────────────────→          │
└────────────────────────────────────────┘
         │ (满了则分配新Arena)
         ↓
┌────────────────────────────────────────┐
│ Arena 2 (128KB)                        │
├────────────────────────────────────────┤
│ [AST Node] [AST Node] ...              │
└────────────────────────────────────────┘

优点:
• 分配O(1)，只需移动指针
• 释放O(1)，整块释放
• 缓存友好，内存连续
• 无碎片化
```

### 3. 性能基准

```c
/* 性能统计 */
typedef struct ParseStats {
    uint64_t tokens_parsed;
    uint64_t nodes_created;
    uint64_t memory_used;
    uint64_t parse_time_us;     // 微秒
} ParseStats;

/* 目标性能 */
// 文件大小: 100KB (约3000行)
// 解析时间: < 10ms
// 内存使用: < 2MB
// 吞吐量: > 10MB/s
```

---

## 🧪 测试策略

### 1. 单元测试

```c
/* 测试框架 */
void test_parse_binary_expr(void) {
    const char* source = "a + b * c";
    Parser* p = parser_create(source);
    ASTNode* ast = parse_expression(p);
    
    assert(ast->kind == AST_BINARY_ADD);
    assert(ast->as.expr.binary.right->kind == AST_BINARY_MUL);
    
    parser_destroy(p);
}

void test_error_recovery(void) {
    const char* source = "a + + b; c = 5;";  // 错误
    Parser* p = parser_create(source);
    p->config.error_recovery = true;
    
    ASTNode* ast = parse_program(p);
    
    // 应该恢复并解析到 c = 5
    assert(p->had_error == true);
    assert(ast != NULL);  // 部分AST
    
    parser_destroy(p);
}
```

### 2. 模糊测试

```c
/* Fuzzer集成 */
#ifdef FUZZING_BUILD_MODE
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    Parser* p = parser_create_from_buffer(data, size);
    p->config.error_recovery = true;
    
    // 不应崩溃
    ASTNode* ast = parse_program(p);
    
    parser_destroy(p);
    return 0;
}
#endif
```

### 3. 基准测试

```bash
# 性能测试套件
tests/
├── bench/
│   ├── small_file.fx       # 100行
│   ├── medium_file.fx      # 1000行
│   ├── large_file.fx       # 10000行
│   └── stress_test.fx      # 深度嵌套
```

---

## 📐 完整的BNF语法

```bnf
(* ===== 程序结构 ===== *)
program         ::= declaration* EOF

declaration     ::= function_decl
                  | var_decl
                  | const_decl
                  | statement

(* ===== 声明 ===== *)
function_decl   ::= IDENTIFIER ":=" function_expr
                  | IDENTIFIER ":" "(" "func" ")" "=" function_expr

function_expr   ::= "(" parameters? ")" block

parameters      ::= IDENTIFIER ("," IDENTIFIER)*

var_decl        ::= IDENTIFIER ":=" expression
                  | IDENTIFIER ":" "[" type "]" "=" expression
                  | IDENTIFIER ":" "[" type "]"  (* 无初始值 *)

const_decl      ::= IDENTIFIER ":" "(" type ")" "=" expression

type            ::= "num" | "str" | "bl" | "obj" | "func"

(* ===== 语句 ===== *)
statement       ::= expr_stmt
                  | block
                  | if_stmt
                  | loop_stmt
                  | return_stmt

expr_stmt       ::= expression

block           ::= "{" statement* "}"

if_stmt         ::= "if" "(" expression ")" block
                    ("(" expression ")" block)*
                    ("{" statement* "}")?

loop_stmt       ::= "L>" loop_kind block

loop_kind       ::= "[" expression "]"                           (* 重复循环 *)
                  | "(" var_decl ";" expression ";" expression ")" (* for循环 *)
                  | "(" expression ":" IDENTIFIER ")"            (* foreach *)

return_stmt     ::= "R>" expression?

(* ===== 表达式 ===== *)
expression      ::= assignment

assignment      ::= IDENTIFIER "=" expression
                  | logical_or

logical_or      ::= logical_and ("||" logical_and)*

logical_and     ::= bitwise_or ("&&" bitwise_or)*

bitwise_or      ::= bitwise_xor ("|" bitwise_xor)*

bitwise_xor     ::= bitwise_and ("^" bitwise_and)*

bitwise_and     ::= comparison ("&" comparison)*

comparison      ::= term (("<" | ">" | "<=" | ">=" | "==" | "!=") term)*

term            ::= factor (("+" | "-") factor)*

factor          ::= power (("*" | "/" | "%") power)*

power           ::= unary ("**" unary)*

unary           ::= ("-" | "!") unary
                  | postfix

postfix         ::= primary
                  | postfix "(" arguments? ")"        (* 函数调用 *)
                  | postfix "[" expression "]"        (* 索引 *)
                  | postfix "." IDENTIFIER            (* 属性访问 *)
                  | postfix ".>" IDENTIFIER           (* 方法链 *)

arguments       ::= expression ("," expression)*

primary         ::= NUMBER
                  | STRING
                  | CHAR
                  | "true" | "false"
                  | "null" | "undef"
                  | IDENTIFIER
                  | "(" expression ")"
                  | array_literal
                  | object_literal
                  | function_expr

array_literal   ::= "[" (expression ("," expression)*)? "]"

object_literal  ::= "{" (object_entry ("," object_entry)*)? "}"

object_entry    ::= (IDENTIFIER | STRING | "[" expression "]") ":" expression
```

---

## 🎯 实现路线图

### Phase 1: 核心基础设施 (1-2天)

```
✅ 任务清单:
□ 实现Arena内存分配器
□ 实现StringPool字符串池
□ 创建基础AST节点结构
□ 实现Token缓冲区
□ 编写单元测试

📊 验收标准:
• Arena分配/释放性能测试通过
• StringPool去重功能正常
• 内存泄漏检测通过
```

### Phase 2: 诊断系统 (2-3天)

```
✅ 任务清单:
□ 实现DiagnosticEngine
□ 实现错误格式化输出
□ 实现多span标注
□ 实现修复建议生成
□ 添加彩色输出支持

📊 验收标准:
• 错误信息清晰易懂
• 多语言支持(中英文)
• 修复建议准确率>80%
```

### Phase 3: 表达式解析 (3-4天)

```
✅ 任务清单:
□ 实现Pratt解析器
□ 解析所有运算符
□ 解析函数调用
□ 解析数组/对象字面量
□ 支持链式比较

📊 验收标准:
• 运算符优先级正确
• 复杂嵌套表达式正确
• 所有测试用例通过
```

### Phase 4: 语句解析 (3-4天)

```
✅ 任务清单:
□ 解析变量声明
□ 解析if语句(含链式)
□ 解析循环语句(3种)
□ 解析return语句
□ 解析块语句

📊 验收标准:
• 所有语句类型支持
• 嵌套语句正确
• 错误恢复正常
```

### Phase 5: 错误恢复 (2-3天)

```
✅ 任务清单:
□ 实现panic模式
□ 实现同步点恢复
□ 生成部分AST
□ 错误限制机制

📊 验收标准:
• 错误不雪崩
• 恢复后继续解析
• 部分AST可用
```

### Phase 6: 优化与测试 (3-5天)

```
✅ 任务清单:
□ 性能优化
□ 内存优化
□ 完善测试覆盖
□ 模糊测试
□ 基准测试

📊 验收标准:
• 解析速度>10MB/s
• 内存使用合理
• 测试覆盖率>90%
• 无内存泄漏
```

---

## 📚 参考资料

### 现代编译器设计

- **Rust Compiler (rustc)**
  - [Parser实现](https://github.com/rust-lang/rust/tree/master/compiler/rustc_parse)
  - [错误诊断系统](https://github.com/rust-lang/rust/tree/master/compiler/rustc_errors)
  - Arena分配器使用

- **Go Compiler (gc)**
  - [语法解析器](https://github.com/golang/go/tree/master/src/go/parser)
  - [AST设计](https://github.com/golang/go/tree/master/src/go/ast)
  - 增量编译支持

- **LLVM/Clang**
  - [Diagnostic系统](https://clang.llvm.org/docs/InternalsManual.html#the-diagnostic-subsystem)
  - FixIt hints实现

### 解析技术

- **Pratt Parsing**
  - "Top Down Operator Precedence" by Vaughan Pratt
  - Matklad's blog on Pratt parsers

- **Error Recovery**
  - "Crafting Interpreters" by Robert Nystrom
  - Dragon Book Chapter 4.5

---

## 🎉 总结

这个Parser设计达到了**现代生产级编译器**的标准：

✅ **健壮性**: 永不崩溃，优雅的错误恢复  
✅ **性能**: 线性时间复杂度，零拷贝优化  
✅ **诊断**: 清晰的错误信息，智能修复建议  
✅ **可维护**: 模块化设计，完善的测试  
✅ **可扩展**: 增量解析，IDE友好  

**与Rust/Go编译器对比**:

| 特性 | Rust rustc | Go gc | FLYUX Parser |
|------|-----------|-------|--------------|
| 错误恢复 | ✅ | ✅ | ✅ |
| 丰富诊断 | ✅ | ✅ | ✅ |
| 性能优化 | ✅ | ✅ | ✅ |
| Arena分配 | ✅ | ✅ | ✅ |
| 增量解析 | ✅ | ✅ | ✅ |
| 并发安全 | ✅ | ✅ | ✅ |

**预期性能指标**:
- 解析速度: **10-20 MB/s**
- 错误恢复率: **>95%**
- 内存效率: **<2MB per 100KB source**
- 诊断质量: **接近rustc水平**

---

**文档版本**: 2.0  
**最后更新**: 2025-11-17  
**状态**: ✅ 设计完成，等待实现
