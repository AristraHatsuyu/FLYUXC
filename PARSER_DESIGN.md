# FLYUX Parser 设计文档

**创建日期**: 2025-11-17  
**状态**: 设计阶段

---

## 📋 目录

1. [概述](#概述)
2. [AST节点设计](#ast节点设计)
3. [语法规则（BNF）](#语法规则bnf)
4. [Parser架构](#parser架构)
5. [递归下降解析策略](#递归下降解析策略)
6. [错误处理](#错误处理)
7. [实现计划](#实现计划)

---

## 🎯 概述

### Parser的职责

Parser（语法分析器）的主要任务是：

1. **Token流 → AST**: 将Lexer产生的Token序列转换为抽象语法树
2. **语法验证**: 检查Token序列是否符合FLYUX语法规则
3. **结构化表示**: 构建便于后续语义分析和代码生成的树形结构
4. **错误报告**: 提供清晰的语法错误信息

### 输入输出

```
输入: Token[] from Lexer
      [TK_IDENT("x"), TK_DEFINE, TK_NUM("123"), TK_SEMI, ...]

输出: AST (Abstract Syntax Tree)
      Program
      └── VarDecl
          ├── name: "x"
          ├── type: null (inferred)
          ├── is_const: false
          └── init: NumLiteral(123)
```

---

## 🌳 AST节点设计

### 基础节点结构

```c
/* AST节点类型枚举 */
typedef enum ASTNodeKind {
    /* 程序根节点 */
    AST_PROGRAM,
    
    /* 语句 (Statements) */
    AST_VAR_DECL,        // 变量声明: x := 123
    AST_CONST_DECL,      // 常量声明: X :(num)= 123
    AST_FUNC_DECL,       // 函数声明: f := (a,b) { ... }
    AST_EXPR_STMT,       // 表达式语句: a + b;
    AST_ASSIGN_STMT,     // 赋值语句: x = 456
    AST_IF_STMT,         // if语句
    AST_LOOP_STMT,       // L>循环语句
    AST_RETURN_STMT,     // R>返回语句
    AST_BLOCK,           // 代码块: { ... }
    
    /* 表达式 (Expressions) */
    AST_BINARY_EXPR,     // 二元表达式: a + b
    AST_UNARY_EXPR,      // 一元表达式: !a, -b
    AST_CALL_EXPR,       // 函数调用: f(a, b)
    AST_MEMBER_EXPR,     // 成员访问: obj.prop
    AST_INDEX_EXPR,      // 索引访问: arr[0]
    AST_CHAIN_EXPR,      // 链式调用: obj.>method.>call(x)
    
    /* 字面量 (Literals) */
    AST_NUM_LITERAL,     // 数字: 123, 3.14
    AST_STRING_LITERAL,  // 字符串: "hello"
    AST_BOOL_LITERAL,    // 布尔: true, false
    AST_NULL_LITERAL,    // null
    AST_UNDEF_LITERAL,   // undef
    AST_ARRAY_LITERAL,   // 数组: [1, 2, 3]
    AST_OBJECT_LITERAL,  // 对象: {a: 1, b: 2}
    
    /* 其他 */
    AST_IDENTIFIER,      // 标识符: x, foo
    AST_TYPE_ANNOTATION  // 类型标注: :[num], :(str)
} ASTNodeKind;

/* AST节点基类 */
typedef struct ASTNode {
    ASTNodeKind kind;
    SourceLocation loc;  // 源码位置信息
    void *data;          // 指向具体节点数据的指针
} ASTNode;
```

### 程序根节点

```c
/* 程序根节点 */
typedef struct ASTProgram {
    ASTNode **statements;  // 顶层语句数组
    size_t stmt_count;     // 语句数量
} ASTProgram;
```

### 语句节点

```c
/* 变量声明: x := 123 或 x :[num]= 123 */
typedef struct ASTVarDecl {
    char *name;              // 变量名
    ASTNode *type_annotation; // 类型标注（可为NULL表示类型推断）
    bool is_const;           // 是否为常量（括号标注）
    ASTNode *init_expr;      // 初始化表达式（可为NULL）
} ASTVarDecl;

/* 函数声明: f := (a, b) { ... } 或 f :<num>= (a, b) { ... } */
typedef struct ASTFuncDecl {
    char *name;              // 函数名
    char **params;           // 参数名数组
    size_t param_count;      // 参数数量
    ASTNode *return_type;    // 返回类型标注（可为NULL）
    ASTNode *body;           // 函数体（AST_BLOCK）
} ASTFuncDecl;

/* 赋值语句: x = 456 */
typedef struct ASTAssignStmt {
    ASTNode *target;         // 赋值目标（IDENTIFIER/MEMBER_EXPR/INDEX_EXPR）
    ASTNode *value;          // 赋值的值
} ASTAssignStmt;

/* if语句: if (cond) { ... } { ... } */
typedef struct ASTIfStmt {
    ASTNode **conditions;    // 条件表达式数组（支持多条件）
    ASTNode **then_blocks;   // 对应的then块数组
    ASTNode *else_block;     // else块（可为NULL）
    size_t cond_count;       // 条件数量
} ASTIfStmt;

/* L>循环语句 */
typedef struct ASTLoopStmt {
    enum {
        LOOP_REPEAT,    // L> [n] { ... }
        LOOP_FOR,       // L> (init; cond; update) { ... }
        LOOP_FOREACH    // L> (arr : item) { ... }
    } loop_type;
    
    union {
        // LOOP_REPEAT
        ASTNode *repeat_count;
        
        // LOOP_FOR
        struct {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *update;
        } for_loop;
        
        // LOOP_FOREACH
        struct {
            ASTNode *iterable;
            char *item_var;
        } foreach_loop;
    } loop_data;
    
    ASTNode *body;           // 循环体
} ASTLoopStmt;

/* R>返回语句: R> value 或 R> */
typedef struct ASTReturnStmt {
    ASTNode *value;          // 返回值（可为NULL表示返回undef）
} ASTReturnStmt;

/* 代码块: { stmt1; stmt2; ... } */
typedef struct ASTBlock {
    ASTNode **statements;
    size_t stmt_count;
} ASTBlock;

/* 表达式语句: expr; */
typedef struct ASTExprStmt {
    ASTNode *expr;
} ASTExprStmt;
```

### 表达式节点

```c
/* 二元表达式: a + b, a && b, a < b */
typedef struct ASTBinaryExpr {
    TokenKind op;            // 运算符类型
    ASTNode *left;
    ASTNode *right;
} ASTBinaryExpr;

/* 一元表达式: !a, -b, +c */
typedef struct ASTUnaryExpr {
    TokenKind op;            // 运算符类型（TK_BANG, TK_MINUS, TK_PLUS）
    ASTNode *operand;
} ASTUnaryExpr;

/* 函数调用: f(a, b, c) */
typedef struct ASTCallExpr {
    ASTNode *callee;         // 被调用的函数（通常是IDENTIFIER）
    ASTNode **args;          // 参数数组
    size_t arg_count;        // 参数数量
} ASTCallExpr;

/* 成员访问: obj.prop */
typedef struct ASTMemberExpr {
    ASTNode *object;         // 对象
    char *property;          // 属性名
    bool is_computed;        // false表示点访问，true表示[]访问
} ASTMemberExpr;

/* 索引访问: arr[i] */
typedef struct ASTIndexExpr {
    ASTNode *object;
    ASTNode *index;
} ASTIndexExpr;

/* 链式调用: obj.>method.>call(x) */
typedef struct ASTChainExpr {
    ASTNode *object;         // 起始对象
    ASTNode **chain;         // 链式调用数组
    size_t chain_count;
} ASTChainExpr;
```

### 字面量节点

```c
/* 数字字面量: 123, 3.14, 1.5e10 */
typedef struct ASTNumLiteral {
    double value;            // 统一用double存储
    char *raw;               // 原始字符串（保留用于输出）
} ASTNumLiteral;

/* 字符串字面量: "hello" */
typedef struct ASTStringLiteral {
    char *value;
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

/* 对象字面量: {a: 1, b: 2} */
typedef struct ASTObjectProperty {
    char *key;               // 属性键
    ASTNode *value;          // 属性值
};

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
    TokenKind type_token;    // TK_TYPE_NUM, TK_TYPE_STR等
    bool is_const;           // true表示():常量, false表示[]:变量
} ASTTypeAnnotation;
```

---

## 📝 语法规则（BNF）

### 程序结构

```bnf
Program         → Statement*

Statement       → VarDecl
                | ConstDecl
                | FuncDecl
                | AssignStmt
                | IfStmt
                | LoopStmt
                | ReturnStmt
                | ExprStmt
                | Block
```

### 声明语句

```bnf
VarDecl         → IDENT ':=' Expr ';'?
                | IDENT ':[' Type ']' '=' Expr ';'?
                | IDENT ':[' Type ']' ';'?

ConstDecl       → IDENT ':(' Type ')' '=' Expr ';'?

FuncDecl        → IDENT ':=' '(' ParamList? ')' Block
                | IDENT ':<' Type '>=' '(' ParamList? ')' Block

ParamList       → IDENT (',' IDENT)*

Type            → 'num' | 'str' | 'bl' | 'obj' | 'func'
```

### 赋值与表达式语句

```bnf
AssignStmt      → LValue '=' Expr ';'?

LValue          → IDENT
                | MemberExpr
                | IndexExpr

ExprStmt        → Expr ';'?
```

### 控制流语句

```bnf
IfStmt          → 'if' '(' Expr ')' Block ElseIfChain? ElseBlock?

ElseIfChain     → ('(' Expr ')' Block)+

ElseBlock       → Block

LoopStmt        → 'L>' '[' Expr ']' Block                    # 重复循环
                | 'L>' '(' ForInit ';' Expr ';' Expr ')' Block  # for循环
                | 'L>' '(' Expr ':' IDENT ')' Block          # foreach循环

ForInit         → VarDecl | AssignStmt | Expr

ReturnStmt      → 'R>' Expr? ';'?

Block           → '{' Statement* '}'
```

### 表达式（按优先级从低到高）

```bnf
Expr            → AssignExpr

AssignExpr      → LogicalOr

LogicalOr       → LogicalAnd ('||' LogicalAnd)*

LogicalAnd      → BitwiseOr ('&&' BitwiseOr)*

BitwiseOr       → BitwiseXor ('|' BitwiseXor)*

BitwiseXor      → BitwiseAnd ('^' BitwiseAnd)*

BitwiseAnd      → Equality ('&' Equality)*

Equality        → Relational (('==' | '!=') Relational)*

Relational      → Additive (('<' | '>' | '<=' | '>=') Additive)*

Additive        → Multiplicative (('+' | '-') Multiplicative)*

Multiplicative  → Power (('*' | '/' | '%') Power)*

Power           → Unary ('**' Unary)*

Unary           → ('!' | '-' | '+') Unary
                | Postfix

Postfix         → Primary PostfixOp*

PostfixOp       → '(' ArgList? ')'        # 函数调用
                | '[' Expr ']'            # 索引访问
                | '.' IDENT               # 成员访问
                | '.>' IDENT              # 链式调用

Primary         → NumLiteral
                | StringLiteral
                | BoolLiteral
                | NullLiteral
                | UndefLiteral
                | ArrayLiteral
                | ObjectLiteral
                | IDENT
                | '(' Expr ')'

ArgList         → Expr (',' Expr)*

ArrayLiteral    → '[' (Expr (',' Expr)*)? ']'

ObjectLiteral   → '{' (Property (',' Property)*)? '}'

Property        → IDENT ':' Expr
                | STRING ':' Expr
                | '[' Expr ']' ':' Expr
```

---

## 🏗️ Parser架构

### 核心数据结构

```c
/* Parser状态 */
typedef struct Parser {
    Token *tokens;           // Token数组
    size_t token_count;      // Token总数
    size_t current;          // 当前Token索引
    
    bool had_error;          // 是否发生错误
    bool panic_mode;         // 是否处于panic模式
    
    char *source;            // 源代码（用于错误报告）
} Parser;

/* 初始化Parser */
Parser *parser_create(Token *tokens, size_t count, char *source);

/* 释放Parser */
void parser_free(Parser *p);

/* 解析入口 */
ASTNode *parser_parse(Parser *p);
```

### 辅助函数

```c
/* Token操作 */
Token *current_token(Parser *p);
Token *peek_token(Parser *p, size_t lookahead);
bool check(Parser *p, TokenKind kind);
bool match(Parser *p, TokenKind kind);
Token *advance(Parser *p);
Token *expect(Parser *p, TokenKind kind, const char *message);

/* 错误处理 */
void parser_error(Parser *p, Token *token, const char *message);
void parser_error_at_current(Parser *p, const char *message);
void synchronize(Parser *p);

/* 内存管理 */
ASTNode *ast_node_create(ASTNodeKind kind, SourceLocation loc);
void ast_node_free(ASTNode *node);
```

---

## 🔄 递归下降解析策略

### 解析顺序

```
parser_parse()
  └─> parse_program()
      └─> parse_statement() (循环)
          ├─> parse_var_decl()
          ├─> parse_func_decl()
          ├─> parse_if_stmt()
          ├─> parse_loop_stmt()
          ├─> parse_return_stmt()
          ├─> parse_assign_or_expr_stmt()
          │   ├─> parse_expr()
          │   │   └─> parse_logical_or()
          │   │       └─> parse_logical_and()
          │   │           └─> ... (按优先级递归)
          │   │               └─> parse_primary()
          │   └─> (如果是'=', 转换为赋值语句)
          └─> parse_block()
```

### 核心解析函数

```c
/* 顶层解析 */
ASTNode *parse_program(Parser *p);
ASTNode *parse_statement(Parser *p);

/* 声明解析 */
ASTNode *parse_var_decl(Parser *p);
ASTNode *parse_const_decl(Parser *p);
ASTNode *parse_func_decl(Parser *p);

/* 语句解析 */
ASTNode *parse_if_stmt(Parser *p);
ASTNode *parse_loop_stmt(Parser *p);
ASTNode *parse_return_stmt(Parser *p);
ASTNode *parse_block(Parser *p);
ASTNode *parse_expr_stmt(Parser *p);
ASTNode *parse_assign_stmt(Parser *p, ASTNode *target);

/* 表达式解析（按优先级） */
ASTNode *parse_expr(Parser *p);
ASTNode *parse_logical_or(Parser *p);
ASTNode *parse_logical_and(Parser *p);
ASTNode *parse_bitwise_or(Parser *p);
ASTNode *parse_bitwise_xor(Parser *p);
ASTNode *parse_bitwise_and(Parser *p);
ASTNode *parse_equality(Parser *p);
ASTNode *parse_relational(Parser *p);
ASTNode *parse_additive(Parser *p);
ASTNode *parse_multiplicative(Parser *p);
ASTNode *parse_power(Parser *p);
ASTNode *parse_unary(Parser *p);
ASTNode *parse_postfix(Parser *p);
ASTNode *parse_primary(Parser *p);

/* 字面量解析 */
ASTNode *parse_array_literal(Parser *p);
ASTNode *parse_object_literal(Parser *p);
```

### 关键解析逻辑示例

#### 1. 区分变量声明与赋值

```c
ASTNode *parse_statement(Parser *p) {
    Token *tok = current_token(p);
    
    if (tok->kind == TK_IDENT) {
        Token *next = peek_token(p, 1);
        
        // x := ... (变量声明)
        if (next->kind == TK_DEFINE) {
            return parse_var_decl(p);
        }
        // x : ... (常量或类型标注)
        else if (next->kind == TK_COLON) {
            Token *after_colon = peek_token(p, 2);
            if (after_colon->kind == TK_L_PAREN) {
                return parse_const_decl(p);  // x :(type)= ...
            } else {
                return parse_var_decl(p);     // x :[type]= ...
            }
        }
        // x = ... (赋值语句)
        else if (next->kind == TK_ASSIGN) {
            ASTNode *target = parse_primary(p);
            advance(p);  // 跳过 '='
            return parse_assign_stmt(p, target);
        }
        // 否则是表达式语句
        else {
            return parse_expr_stmt(p);
        }
    }
    
    // 其他语句类型...
}
```

#### 2. 处理函数声明

```c
ASTNode *parse_var_decl(Parser *p) {
    Token *name_tok = expect(p, TK_IDENT, "Expected variable name");
    
    // 检查是否为函数声明: f := (params) { ... }
    if (check(p, TK_DEFINE)) {
        advance(p);  // 跳过 :=
        if (check(p, TK_L_PAREN)) {
            // 是函数声明
            return parse_func_decl_after_name(p, name_tok);
        }
    }
    
    // 普通变量声明...
}
```

#### 3. if语句的多条件支持

```c
ASTNode *parse_if_stmt(Parser *p) {
    expect(p, TK_KW_IF, "Expected 'if'");
    
    // 第一个条件
    expect(p, TK_L_PAREN, "Expected '('");
    ASTNode *cond1 = parse_expr(p);
    expect(p, TK_R_PAREN, "Expected ')'");
    ASTNode *then1 = parse_block(p);
    
    // 收集额外的条件 (elseif链)
    while (check(p, TK_L_PAREN)) {
        advance(p);
        ASTNode *cond = parse_expr(p);
        expect(p, TK_R_PAREN, "Expected ')'");
        ASTNode *then_block = parse_block(p);
        // 添加到条件数组...
    }
    
    // else块
    ASTNode *else_block = NULL;
    if (check(p, TK_L_BRACE)) {
        else_block = parse_block(p);
    }
    
    // 构建AST节点...
}
```

#### 4. 链式调用解析

```c
ASTNode *parse_postfix(Parser *p) {
    ASTNode *expr = parse_primary(p);
    
    while (true) {
        if (match(p, TK_L_PAREN)) {
            // 函数调用: expr(args)
            expr = parse_call_expr(p, expr);
        }
        else if (match(p, TK_L_BRACKET)) {
            // 索引访问: expr[index]
            expr = parse_index_expr(p, expr);
        }
        else if (match(p, TK_DOT)) {
            // 成员访问: expr.prop
            expr = parse_member_expr(p, expr, false);
        }
        else if (match(p, TK_DOT_CHAIN)) {
            // 链式调用: expr.>method
            expr = parse_chain_expr(p, expr);
        }
        else {
            break;
        }
    }
    
    return expr;
}
```

---

## ⚠️ 错误处理

### 错误恢复策略

1. **Panic模式**: 遇到错误后跳过tokens直到同步点
2. **同步点**: 语句边界（`;`, `}`, 关键字开始等）
3. **继续解析**: 报告错误但继续解析，收集更多错误

```c
void synchronize(Parser *p) {
    p->panic_mode = false;
    
    while (current_token(p)->kind != TK_EOF) {
        // 在分号后同步
        if (previous_token(p)->kind == TK_SEMI) return;
        
        // 在语句开始处同步
        switch (current_token(p)->kind) {
            case TK_KW_IF:
            case TK_KW_LOOP:
            case TK_KW_RETURN:
            case TK_IDENT:
            case TK_L_BRACE:
                return;
            default:
                advance(p);
        }
    }
}
```

### 错误消息格式

```
Error at line 15, column 8:
    x := 123 456
             ^^^
Expected ';' or newline after expression
```

---

## 📋 实现计划

### 第一阶段：基础框架

- [ ] 创建 `include/flyuxc/parser.h`
- [ ] 创建 `src/core/parser.c`
- [ ] 实现Parser结构和辅助函数
- [ ] 实现AST节点内存管理

### 第二阶段：字面量与简单表达式

- [ ] 实现 `parse_primary()`
  - [ ] 数字字面量
  - [ ] 字符串字面量
  - [ ] 布尔字面量
  - [ ] 标识符
- [ ] 实现一元表达式解析
- [ ] 实现二元表达式解析（算术、比较、逻辑）

### 第三阶段：复杂表达式

- [ ] 实现数组字面量解析
- [ ] 实现对象字面量解析
- [ ] 实现函数调用解析
- [ ] 实现成员访问与索引解析
- [ ] 实现链式调用解析

### 第四阶段：语句解析

- [ ] 实现变量声明解析
- [ ] 实现常量声明解析
- [ ] 实现函数声明解析
- [ ] 实现赋值语句解析
- [ ] 实现表达式语句解析

### 第五阶段：控制流

- [ ] 实现if语句解析
- [ ] 实现L>循环语句解析（三种类型）
- [ ] 实现R>返回语句解析
- [ ] 实现代码块解析

### 第六阶段：测试与优化

- [ ] 编写单元测试
- [ ] 集成测试（使用testfx/目录测试文件）
- [ ] 错误处理完善
- [ ] 性能优化
- [ ] 内存泄漏检查

---

## 📊 测试策略

### 单元测试用例

```c
// 测试1: 简单变量声明
// 输入: x := 123
// 期望AST:
//   VarDecl(name="x", type=null, is_const=false, 
//           init=NumLiteral(123))

// 测试2: 带类型的变量声明
// 输入: y :[num]= 456
// 期望AST:
//   VarDecl(name="y", 
//           type=TypeAnnotation(TK_TYPE_NUM, is_const=false),
//           init=NumLiteral(456))

// 测试3: 函数声明
// 输入: f := (a, b) { R> a + b }
// 期望AST:
//   FuncDecl(name="f", params=["a","b"],
//            body=Block([ReturnStmt(BinaryExpr(+, a, b))]))

// 测试4: if语句
// 输入: if (x > 0) { print(x) }
// 期望AST:
//   IfStmt(conditions=[BinaryExpr(>, x, 0)],
//          then_blocks=[Block([ExprStmt(Call(print, [x]))])])

// 测试5: 复杂表达式
// 输入: result := a**2 + b*c & d | e
// 期望AST:
//   VarDecl(..., init=BinaryExpr(|, 
//                     BinaryExpr(&, 
//                         BinaryExpr(+, Power(a,2), Mul(b,c)),
//                         d),
//                     e))
```

### 集成测试

使用现有的 `testfx/` 测试文件：

```bash
./build/flyuxc testfx/demo.fx --parse-only --print-ast
./build/flyuxc testfx/complex_test.fx --parse-only --print-ast
```

---

## 🔍 AST可视化（调试用）

```c
/* AST打印函数（用于调试） */
void ast_print(ASTNode *node, int indent);

/* 示例输出 */
Program
├── VarDecl "x"
│   └── NumLiteral: 123
├── FuncDecl "add"
│   ├── params: ["a", "b"]
│   └── Block
│       └── ReturnStmt
│           └── BinaryExpr '+'
│               ├── Identifier "a"
│               └── Identifier "b"
└── ExprStmt
    └── CallExpr
        ├── callee: Identifier "print"
        └── args:
            └── CallExpr
                ├── callee: Identifier "add"
                └── args:
                    ├── NumLiteral: 5
                    └── NumLiteral: 3
```

---

## 📚 参考资料

### 相关文件

- `FLYUX_SYNTAX.md` - FLYUX语言语法定义
- `include/flyuxc/lexer.h` - Lexer接口和Token定义
- `OPERATOR_PRECEDENCE.md` - 运算符优先级参考

### 编译器理论

- 递归下降解析（Recursive Descent Parsing）
- Pratt解析器（用于表达式）
- 抽象语法树（Abstract Syntax Tree）

### 待解决问题

1. **分号插入**: 是否自动插入分号（类似JavaScript ASI）？
2. **链式调用语义**: `.>` 的左侧如何作为右侧函数的第一个参数？
3. **类型标注**: 是否在Parser阶段验证类型合法性？
4. **emoji标识符**: 如何正确处理Unicode标识符？

---

**文档版本**: 1.0  
**最后更新**: 2025-11-17
