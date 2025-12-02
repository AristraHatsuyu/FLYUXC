#ifndef FLYUXC_CODEGEN_H
#define FLYUXC_CODEGEN_H

#include "flyuxc/frontend/ast.h"
#include <stdio.h>

/* 数组元数据 */
typedef struct ArrayMetadata {
    char *var_name;         /* 变量名 */
    char *array_ptr;        /* 数组指针 (LLVM 临时变量) */
    size_t elem_count;      /* 元素数量 */
    struct ArrayMetadata *next;
} ArrayMetadata;

/* 对象属性元数据 */
typedef struct ObjectField {
    char *field_name;       /* 属性名 */
    char *field_ptr;        /* 属性的LLVM指针 */
    struct ObjectField *next;
} ObjectField;

/* 对象元数据 */
typedef struct ObjectMetadata {
    char *var_name;         /* 变量名 */
    ObjectField *fields;    /* 属性链表 */
    struct ObjectMetadata *next;
} ObjectMetadata;

/* 符号表条目 - 记录已定义的变量 */
typedef struct SymbolEntry {
    char *name;             /* 变量名 */
    struct SymbolEntry *next;
} SymbolEntry;

/* 作用域内的局部变量条目 - 用于P2作用域退出清理 */
typedef struct LocalVarEntry {
    char *name;                     /* 变量名 */
    struct LocalVarEntry *next;
} LocalVarEntry;

/* 作用域跟踪器 - 跟踪当前函数作用域的所有局部变量 */
typedef struct ScopeTracker {
    LocalVarEntry *locals;          /* 当前作用域的局部变量链表 */
} ScopeTracker;

/* 循环作用域条目 - 用于嵌套循环时的 break/next 清理 */
typedef struct LoopScopeEntry {
    ScopeTracker *loop_scope;       /* 这个循环的作用域跟踪器 */
    char *loop_end_label;           /* 这个循环的结束标签 (break) */
    char *loop_continue_label;      /* 这个循环的继续标签 (next) */
    char *label;                    /* 循环标签名（用于多级 break/next） */
    struct LoopScopeEntry *outer;   /* 外层循环 (renamed from next to avoid confusion) */
} LoopScopeEntry;

/* 中间值条目 - 跟踪表达式求值中创建的临时 Value* */
typedef struct TempValueEntry {
    char *temp_name;                /* 临时变量名（LLVM %tN 格式） */
    struct TempValueEntry *next;    /* 链表下一个 */
} TempValueEntry;

/* 中间值栈 - 跟踪当前表达式求值中的所有临时值 */
typedef struct TempValueStack {
    TempValueEntry *entries;        /* 临时值链表头 */
    int count;                      /* 临时值数量 */
} TempValueStack;

/* 前向声明 */
typedef struct CapturedVars CapturedVars;

/* 代码生成器结构 */
typedef struct CodeGen {
    FILE *output;           /* 最终输出文件 */
    FILE *globals_buf;      /* 全局声明缓冲区 */
    FILE *code_buf;         /* 代码缓冲区 */
    FILE *entry_alloca_buf; /* 函数入口alloca缓冲区 */
    int temp_count;         /* 临时变量计数器 */
    int label_count;        /* 标签计数器 */
    int string_count;       /* 字符串常量计数器 */
    ArrayMetadata *arrays;  /* 数组元数据链表 */
    ObjectMetadata *objects; /* 对象元数据链表 */
    SymbolEntry *symbols;   /* 符号表 - 已定义的变量 */
    SymbolEntry *globals;   /* 全局变量表 - has_main时使用LLVM全局变量 */
    const char *current_var_name;  /* 当前正在赋值的变量名（用于数组/对象跟踪） */
    int in_try_catch;       /* 是否在 Try-Catch 块中 */
    char *try_catch_label;  /* 当前 Try-Catch 的 catch 标签 */
    char *loop_end_label;   /* 当前循环的结束标签（用于 break） */
    char *loop_continue_label; /* 当前循环的继续标签（用于 next） */
    ScopeTracker *scope;    /* 当前函数作用域跟踪器 */
    LoopScopeEntry *loop_scope_stack;  /* 循环作用域栈（用于 break 清理） */
    int block_terminated;   /* 当前基本块是否已终止（有 ret/br） */
    TempValueStack *temp_values;  /* 中间值栈（表达式求值期间的临时值） */
    int has_error;          /* 是否有编译错误 */
    char *error_message;    /* 错误消息 */
    /* 变量映射表（用于错误消息中显示原始名字） */
    void *varmap_entries;   /* VarMapEntry 数组的指针 */
    size_t varmap_count;    /* 映射表大小 */
    /* 原始源代码（用于错误消息中显示源码行） */
    const char *original_source;
    /* 闭包捕获变量 - 用于嵌套函数/匿名函数 */
    CapturedVars *current_captured;  /* 当前正在生成的闭包的捕获变量 */
    SymbolEntry *functions;  /* 函数名表 - 顶层定义的函数 */
} CodeGen;

/* 创建代码生成器 */
CodeGen *codegen_create(FILE *output);

/* 设置变量映射表（用于错误消息中显示原始变量名） */
void codegen_set_varmap(CodeGen *gen, void *entries, size_t count);

/* 设置原始源代码（用于错误消息中显示源码行） */
void codegen_set_original_source(CodeGen *gen, const char *source);

/* 生成LLVM IR */
void codegen_generate(CodeGen *gen, ASTNode *ast);

/* 检查是否有错误 */
int codegen_has_error(CodeGen *gen);

/* 获取错误消息 */
const char *codegen_get_error(CodeGen *gen);

/* 释放代码生成器 */
void codegen_free(CodeGen *gen);

/* ========================================
 * 闭包捕获变量分析
 * ======================================== */

/* 捕获变量列表 - 完整定义（前面有前向声明） */
struct CapturedVars {
    char **names;           /* 捕获的变量名数组 */
    size_t count;           /* 变量数量 */
    size_t capacity;        /* 数组容量 */
};

/* 创建捕获变量列表 */
CapturedVars *captured_vars_create(void);

/* 添加捕获变量（如果不存在） */
void captured_vars_add(CapturedVars *cv, const char *name);

/* 检查变量是否已在列表中 */
int captured_vars_contains(CapturedVars *cv, const char *name);

/* 释放捕获变量列表 */
void captured_vars_free(CapturedVars *cv);

/* 分析匿名函数，收集需要捕获的外部变量 
 * gen: 代码生成器（用于检查已定义的符号）
 * func_node: 函数AST节点
 * params: 函数参数名数组
 * param_count: 参数数量
 * 返回：捕获的变量列表（调用者需释放） */
CapturedVars *analyze_captured_vars(CodeGen *gen, ASTNode *func_body, 
                                    char **params, size_t param_count);

#endif /* FLYUXC_CODEGEN_H */
